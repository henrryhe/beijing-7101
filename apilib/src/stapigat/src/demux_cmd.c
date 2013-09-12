/************************************************************************

File name   : demux_cmd.c

Description : Commands for STDEMUX

COPYRIGHT (C) STMicroelectronics 2003

Date               Modification                                      Initials
----               ------------                                      --------
02 Fev 2006        Creation                                          OB
************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include <stdio.h>
#include <string.h>
#include "testcfg.h"
#include "stsys.h"

#ifdef USE_DEMUX

#define USE_AS_FRONTEND

#include "genadd.h"
#include "stddefs.h"
#include "stdevice.h"

#include "stcommon.h"
#include "sttbx.h"
#include "testtool.h"
#ifdef USE_CLKRV
#include "stclkrv.h"
#endif
#include "stdemux.h"

#include "clevt.h"
#ifdef USE_CLKRV
#include "clclkrv.h"
#endif
#include "api_cmd.h"
#include "demux_cmd.h"

#ifdef USE_VIDEO
#include "vid_util.h"
#endif

#ifdef ST_5188
#define FEI_SOFT_RESET            (FEI_BASE_ADDRESS + 0x00)
#define FEI_CONFIG                (FEI_BASE_ADDRESS + 0x10)
#define FEI_SECTOR_SIZE           (FEI_BASE_ADDRESS + 0x38)
#define INTERCONNECT_BASE         (0x20402000)
#define CONFIG_CONTROL_C          (INTERCONNECT_BASE + 0x0)
#endif /* ST_5188 */

/*###########################################################################*/
/*############################## DEFINITION #################################*/
/*###########################################################################*/

/*#######################################################################
 *# Platform # Configuration
 *#######################################################################
 *# STi5188  # - MB457 jumper J24 to short pins 2&3
 *#          # - Nim card DB580
 *#          #     SW1 OFF (disable parallel to serial parser mode)
 *#          #     SW2 ON  (enable NIM output)
 *#          #     SW3 OFF (disable byte clock inversion)
 *#          #     SW4 OFF (disable packet sync and dvalid swap)
 *#          # - Packet injector MTX 100
 *#          #     Serial
 *#	     #	   Packet size 204
 *#	     #	   Sync -> Non TS -> PSync Enable 204, Dvalid Enable 188
 *#######################################################################
 *# STi8010  # - (TODO)
 *#          # 
 *#######################################################################*/

/* --- Constants ---------------------------------------------------------- */

/* TODO : stdemux_init for more than 2 devices */
#define DEMUXCMD_MAX_DEVICES     1

#if defined(ST_8010) || defined(ST_5188)
 #define DEMUXCMD_MAX_VIDEO_SLOT  1
#endif

#define DEMUXCMD_MAX_PCR_SLOT    1

#ifdef USE_AUDIO
#define DEMUXCMD_MAX_AUDIO_SLOT  1
#else
#define DEMUXCMD_MAX_AUDIO_SLOT  0
#endif

/* Define for code readibility */
#define DEMUXCMD_MAX_SLOTS (DEMUXCMD_MAX_VIDEO_SLOT+DEMUXCMD_MAX_AUDIO_SLOT+DEMUXCMD_MAX_PCR_SLOT)
#define DEMUXCMD_BASE_VIDEO (0)
#define DEMUXCMD_BASE_AUDIO (DEMUXCMD_MAX_VIDEO_SLOT)
#define DEMUXCMD_BASE_PCR   (DEMUXCMD_MAX_VIDEO_SLOT+DEMUXCMD_MAX_AUDIO_SLOT)

typedef unsigned int ProgId_t; /* pid_t defined as int for demux/link, but U16
				* for sdemux/stdemux4 */
/* --- Global variables --------------------------------------------------- */

/* Following could be move to the common injection structure */
static BOOL DemuxCmdPCRStarted[DEMUXCMD_MAX_DEVICES][DEMUXCMD_MAX_PCR_SLOT];
#ifdef USE_AUDIO
static BOOL DemuxCmdAudioStarted[DEMUXCMD_MAX_DEVICES][DEMUXCMD_MAX_AUDIO_SLOT];
#endif
STDEMUX_Handle_t STDEMUXHandle[DEMUXCMD_MAX_DEVICES];

/* Order is first video, then audio and then PCR */
static STDEMUX_Slot_t SlotHandle[DEMUXCMD_MAX_DEVICES][DEMUXCMD_MAX_SLOTS];

static char DEMUX_Msg[200]; /* for trace */

static STEVT_Handle_t Evt_Handle_For_STDEMUX_Errors;
static BOOL DEMUXCMD_DeviceTypeEventErrorsCapable = TRUE;
static U32 Sections_Discarded_On_Crc_Check_Errors[DEMUXCMD_MAX_SLOTS];
static U32 Buffer_Overflow_Errors[DEMUXCMD_MAX_SLOTS];
static U32 CC_Errors[DEMUXCMD_MAX_SLOTS];
static U32 Packet_Errors[DEMUXCMD_MAX_SLOTS];
static U32 TC_Code_Fault_Errors[DEMUXCMD_MAX_SLOTS];
static U32 Total_Stdemux_Errors;

#if defined(ST_8010) || defined(ST_5188)
static STDEMUX_Device_t DeviceType = STDEMUX_DEVICE_CHANNEL_FEC1;
#endif

static ST_DeviceName_t DEMUXDeviceName[5] = {
    STDEMUX_DEVICE_NAME,
    "STDEMUX2",
    "STDEMUX3",
    "STDEMUX4",
    "STDEMUX5"
};

static osclock_t MaxDelayForLastPacketTransferWhenStdemuxPidCleared = 0;

/* --- Extern variables (from startup.c) ---------------------------------- */
extern ST_Partition_t *DriverPartition_p;
extern ST_Partition_t *NCachePartition_p;

#ifdef USE_VIDEO
extern VID_Injection_t VID_Injection[];
extern void VID_ReportInjectionAlreadyOccurs(U32 CDFifoNb);
#endif

/* Private Function prototypes ---------------------------------------------- */

/*##########################################################################*/
/*######################### INTERNAL FUNCTIONS #############################*/
/*##########################################################################*/

/* INTERFACE OBJECT: WRAPPER FCT TO LINK DEMUX AND VIDEO */
ST_ErrorCode_t GetWritePtrFct(void * const Handle, void ** const Address_p)
{
    ST_ErrorCode_t Err;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

    if (STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping) == ST_NO_ERROR)
    {
        Err = STDEMUX_BufferGetWritePointer((STDEMUX_Buffer_t)Handle,Address_p);
        *Address_p = STAVMEM_VirtualToDevice(*Address_p, &VirtualMapping);
        return(Err);
    }
    return(ST_ERROR_BAD_PARAMETER);
}
void SetReadPtrFct(void * const Handle, void * const Address_p)
{
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    /* registered handle = index in cd-fifo array */
    if (STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping) == ST_NO_ERROR)
    {
        /* just cast the handle */
        STDEMUX_BufferSetReadPointer((STDEMUX_Buffer_t)Handle, STAVMEM_DeviceToVirtual(Address_p,&VirtualMapping));
    }
}

static void DemuxErrorsCallBack(STEVT_CallReason_t Reason,
                          const ST_DeviceName_t RegistrantName,
                          STEVT_EventConstant_t Event,
                          const void *EventData,
                          const void *SubscriberData_p);

/*******************************************************************************
Name        : DemuxErrorsCallBack
Descridemuxon : Counts error events reported by STDEMUX
Parameters  : Reason, type of the event.
              Event, nature of the event.
              EventData, data associted to the event.
Assumptions : At least one event have been subscribed.
Limitations :
Returns     : Nothing.
*******************************************************************************/
static void DemuxErrorsCallBack(STEVT_CallReason_t Reason,
                          const ST_DeviceName_t RegistrantName,
                          STEVT_EventConstant_t Event,
                          const void *EventData,
                          const void *SubscriberData_p)
{
    int SlotIndex;
    STDEMUX_Slot_t EvtSlotHandle;
    int i;
    U32 DeviceNb;

    SlotIndex = -1;
    i = 0;

    DeviceNb = (U32) SubscriberData_p;
    EvtSlotHandle = ((STDEMUX_EventData_t *) EventData)->ObjectHandle;
    while ((SlotIndex == -1) && (i < DEMUXCMD_MAX_SLOTS))
    {
        if (EvtSlotHandle == SlotHandle[DeviceNb][i])
        {
            SlotIndex = i;
        }
        i++;
    }
    if (SlotIndex==-1)
    {
        SlotIndex=0;
    }

    Total_Stdemux_Errors++;

    switch (Event)
    {
        case STDEMUX_EVENT_SECTIONS_DISCARDED_ON_CRC_CHECK_EVT :
            Sections_Discarded_On_Crc_Check_Errors[SlotIndex]++;
            break;

        case STDEMUX_EVENT_BUFFER_OVERFLOW_EVT :
            Buffer_Overflow_Errors[SlotIndex]++;
            break;

        case STDEMUX_EVENT_CC_ERROR_EVT :
            CC_Errors[SlotIndex]++;
            break;

        case STDEMUX_EVENT_PACKET_ERROR_EVT :
            Packet_Errors[SlotIndex]++;
            break;

        case STDEMUX_EVENT_TC_CODE_FAULT_EVT :
            TC_Code_Fault_Errors[SlotIndex]++;
            break;

        default :
            /* Nothing special to do !!! */
            break;
    }

} /* end of DemuxErrorsCallBack */



/*****************************************************************************
Name        : DEMUX_Subscribe_Error_Events
Description : Enable and register to some STDEMUX error events
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*****************************************************************************/
static BOOL DEMUX_Subscribe_Error_Events(U32 DeviceNb)
{
    ST_ErrorCode_t          ErrCode;
    BOOL                    RetOK;
    STEVT_OpenParams_t      OpenParams;
    STEVT_DeviceSubscribeParams_t   LocSubscribeParams;

    RetOK = TRUE;

    Evt_Handle_For_STDEMUX_Errors = 0;
    ErrCode = STEVT_Open(STEVT_DEVICE_NAME, &OpenParams, &Evt_Handle_For_STDEMUX_Errors);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STEVT_Open for STDEMUX errors subscription failed. Error 0x%x ! ", ErrCode));
        RetOK = FALSE;
    }
    else
    {
        LocSubscribeParams.NotifyCallback       = DemuxErrorsCallBack;
        LocSubscribeParams.SubscriberData_p     = (void *) DeviceNb;

        ErrCode |= STDEMUX_EnableErrorEvent(DEMUXDeviceName[DeviceNb], STDEMUX_EVENT_SECTIONS_DISCARDED_ON_CRC_CHECK_EVT);
        ErrCode |= STEVT_SubscribeDeviceEvent (Evt_Handle_For_STDEMUX_Errors, DEMUXDeviceName[DeviceNb],
                STDEMUX_EVENT_SECTIONS_DISCARDED_ON_CRC_CHECK_EVT, &LocSubscribeParams);
        ErrCode |= STDEMUX_EnableErrorEvent(DEMUXDeviceName[DeviceNb], STDEMUX_EVENT_BUFFER_OVERFLOW_EVT);
        ErrCode |= STEVT_SubscribeDeviceEvent (Evt_Handle_For_STDEMUX_Errors, DEMUXDeviceName[DeviceNb],
                STDEMUX_EVENT_BUFFER_OVERFLOW_EVT, &LocSubscribeParams);
        ErrCode |= STDEMUX_EnableErrorEvent(DEMUXDeviceName[DeviceNb], STDEMUX_EVENT_CC_ERROR_EVT);
        ErrCode |= STEVT_SubscribeDeviceEvent (Evt_Handle_For_STDEMUX_Errors, DEMUXDeviceName[DeviceNb],
                STDEMUX_EVENT_CC_ERROR_EVT, &LocSubscribeParams);
        ErrCode |= STDEMUX_EnableErrorEvent(DEMUXDeviceName[DeviceNb], STDEMUX_EVENT_PACKET_ERROR_EVT);
        ErrCode |= STEVT_SubscribeDeviceEvent (Evt_Handle_For_STDEMUX_Errors, DEMUXDeviceName[DeviceNb],
                STDEMUX_EVENT_PACKET_ERROR_EVT, &LocSubscribeParams);
        ErrCode |= STDEMUX_EnableErrorEvent(DEMUXDeviceName[DeviceNb], STDEMUX_EVENT_TC_CODE_FAULT_EVT);
        ErrCode |= STEVT_SubscribeDeviceEvent (Evt_Handle_For_STDEMUX_Errors, DEMUXDeviceName[DeviceNb],
                STDEMUX_EVENT_TC_CODE_FAULT_EVT, &LocSubscribeParams);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("STDEMUX errors subscription failed. Error 0x%x ! ", ErrCode));
            RetOK = FALSE;
        }
    }

    return(RetOK);
} /* end of DEMUX_Subscribe_Error_Events() */


/*****************************************************************************
Name        : DEMUX_Unsubscribe_Error_Events
Description : Enable and register to some STDEMUX error events
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*****************************************************************************/
static BOOL DEMUX_Unsubscribe_Error_Events(U32 DeviceNb)
{
    ST_ErrorCode_t          ErrCode;
    BOOL                    RetOK;

    RetOK = TRUE;

    if (Evt_Handle_For_STDEMUX_Errors==0)
    {
        ErrCode |= STDEMUX_EnableErrorEvent(DEMUXDeviceName[DeviceNb], STDEMUX_EVENT_TC_CODE_FAULT_EVT);
        ErrCode |= STEVT_UnsubscribeDeviceEvent(Evt_Handle_For_STDEMUX_Errors, DEMUXDeviceName[DeviceNb],
                STDEMUX_EVENT_TC_CODE_FAULT_EVT);
        ErrCode |= STDEMUX_EnableErrorEvent(DEMUXDeviceName[DeviceNb], STDEMUX_EVENT_PACKET_ERROR_EVT);
        ErrCode |= STEVT_UnsubscribeDeviceEvent(Evt_Handle_For_STDEMUX_Errors, DEMUXDeviceName[DeviceNb],
                STDEMUX_EVENT_PACKET_ERROR_EVT);
        ErrCode |= STDEMUX_EnableErrorEvent(DEMUXDeviceName[DeviceNb], STDEMUX_EVENT_CC_ERROR_EVT);
        ErrCode |= STEVT_UnsubscribeDeviceEvent(Evt_Handle_For_STDEMUX_Errors, DEMUXDeviceName[DeviceNb],
                STDEMUX_EVENT_CC_ERROR_EVT);
        ErrCode |= STDEMUX_EnableErrorEvent(DEMUXDeviceName[DeviceNb], STDEMUX_EVENT_BUFFER_OVERFLOW_EVT);
        ErrCode |= STEVT_UnsubscribeDeviceEvent(Evt_Handle_For_STDEMUX_Errors, DEMUXDeviceName[DeviceNb],
                STDEMUX_EVENT_BUFFER_OVERFLOW_EVT);
        ErrCode |= STDEMUX_EnableErrorEvent(DEMUXDeviceName[DeviceNb], STDEMUX_EVENT_SECTIONS_DISCARDED_ON_CRC_CHECK_EVT);
        ErrCode |= STEVT_UnsubscribeDeviceEvent(Evt_Handle_For_STDEMUX_Errors, DEMUXDeviceName[DeviceNb],
                STDEMUX_EVENT_SECTIONS_DISCARDED_ON_CRC_CHECK_EVT);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("STDEMUX errors Unsubscription failed. Error 0x%x ! ", ErrCode));
            RetOK = FALSE;
        }
        STEVT_Close(Evt_Handle_For_STDEMUX_Errors);
    }

    return(RetOK);
} /* end of DEMUX_Unsubscribe_Error_Events() */

/*****************************************************************************
Name        : DEMUX_Init
Description : Initialise the STDEMUX driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*****************************************************************************/
static BOOL DEMUX_Init(U32 DeviceNb)
{
    BOOL                            RetOK;
    STDEMUX_InitParams_t              STDEMUXInitParams;
    STDEMUX_OpenParams_t              STDEMUXOpenParams;
    STDEMUX_SlotData_t                SlotData;
    ST_ErrorCode_t                    ErrCode;
    int                               i;


    STTBX_Print(( "STDEMUX initializing, \trevision=%-21.21s ...", STDEMUX_GetRevision() ));
    /* --- Driver initialization --- */
    memset((void *)&STDEMUXInitParams, 0, sizeof(STDEMUX_InitParams_t));
    STDEMUXInitParams.Partition_p = DriverPartition_p;
    STDEMUXInitParams.NCachePartition_p = NCachePartition_p;
    strcpy( STDEMUXInitParams.EventHandlerName, STEVT_DEVICE_NAME);    
    STDEMUXInitParams.EventProcessPriority       = MIN_USER_PRIORITY + 1;
    STDEMUXInitParams.DemuxProcessPriority       = MIN_USER_PRIORITY + 2;
    STDEMUXInitParams.Parser                     = STDEMUX_PARSER_TYPE_TS;
    STDEMUXInitParams.Device                     = DeviceType;
    STDEMUXInitParams.DemuxMessageQueueSize      = 256;
    STDEMUXInitParams.InputPacketSize            = 192; /* FEI buffer is 192
							   bytes */
    STDEMUXInitParams.OutputModeBlocking         = FALSE;
    STDEMUXInitParams.SyncLock                   = 1;
    STDEMUXInitParams.SyncDrop                   = 3;
    STDEMUXInitParams.InputMetaData              = STDEMUX_INPUT_METADATA_PRE;
    STDEMUXInitParams.NumOfPcktsInPesBuffer      = 256;
    STDEMUXInitParams.NumOfPcktsInRawBuffer      = 256;
    STDEMUXInitParams.NumOfPcktsInSecBuffer      = 256;

    ErrCode = STDEMUX_Init(DEMUXDeviceName[DeviceNb], &STDEMUXInitParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("%s init. error 0x%x ! ", DEMUXDeviceName[DeviceNb], ErrCode));
        RetOK = FALSE;
    }
    else
    {
        STTBX_Print(("%s Initialized\n", DEMUXDeviceName[DeviceNb]));
        if (DEMUXCMD_DeviceTypeEventErrorsCapable==TRUE)
        {
            DEMUX_Subscribe_Error_Events(DeviceNb);
        }
    }

    /* --- Connection opening --- */
    if (RetOK)
    {
        memset(&STDEMUXOpenParams,0,sizeof(STDEMUX_OpenParams_t));
        STDEMUXOpenParams.DriverPartition_p = DriverPartition_p;
        STDEMUXOpenParams.NonCachedPartition_p = NCachePartition_p;

        ErrCode = STDEMUX_Open(DEMUXDeviceName[DeviceNb], &STDEMUXOpenParams, &STDEMUXHandle[DeviceNb]);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("STDEMUX_Open(%s,hndl=%d) : failed ! error=%d ",
                         DEMUXDeviceName[DeviceNb], STDEMUXHandle[DeviceNb], ErrCode));
            RetOK = FALSE;
        }
        else
        {
            STTBX_Print(("                                 %s opened as device %d, hndl=%d\n",
                     DEMUXDeviceName[DeviceNb], DeviceNb, STDEMUXHandle[DeviceNb]));
        }
    }

    /* --------------------- */
    /* --- Slots opening --- */
    /* --------------------- */

    /* Allocation for VIDEO */
#ifdef STVID_DIRECTV
    SlotData.SlotType = STDEMUX_SLOT_TYPE_ES;
#else
    SlotData.SlotType = STDEMUX_SLOT_TYPE_PES;
#endif /* STVID_DIRECTV */
    SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
    SlotData.SlotFlags.DiscardOnCrcError = FALSE;
    SlotData.SlotFlags.SoftwareCDFifo = TRUE;

    /* Initialise all video slots */
    for(i = DEMUXCMD_BASE_VIDEO; (i < DEMUXCMD_BASE_VIDEO + DEMUXCMD_MAX_VIDEO_SLOT) && RetOK; i++)
    {
        ErrCode = STDEMUX_SlotAllocate( STDEMUXHandle[DeviceNb], &SlotHandle[DeviceNb][i], &SlotData);
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(("STDEMUX_SlotAllocate(hndl=%d) : failed for video ! error=%d ",
                         STDEMUXHandle[DeviceNb], ErrCode));
            RetOK = FALSE;
        }
        else
        {
            /* Initialize video slot */
            ErrCode = STDEMUX_SlotClearParams(SlotHandle[DeviceNb][i]);
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(("STDEMUX_SlotClearParams(slotHndl=%d) : failed for video ! error=%d ",
                             SlotHandle[DeviceNb][i], ErrCode));
                RetOK = FALSE;
            }
        }
    }
    if (RetOK)
    {
        STTBX_Print(("                                 %d video slot(s) ready for DEMUX device=%d\n",
                 DEMUXCMD_MAX_VIDEO_SLOT, DeviceNb));
    }

#ifdef USE_AUDIO
    SlotData.SlotFlags.SoftwareCDFifo = FALSE;

    /* Initialise all audio slots */
    for(i = DEMUXCMD_BASE_AUDIO; (i < (DEMUXCMD_BASE_AUDIO+DEMUXCMD_MAX_AUDIO_SLOT)) && RetOK; i++)
    {
        ErrCode = STDEMUX_SlotAllocate( STDEMUXHandle[DeviceNb], &SlotHandle[DeviceNb][i], &SlotData);
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(("STDEMUX_SlotAllocate(hndl=%d) : failed for audio ! error=%d ",
                         STDEMUXHandle[DeviceNb], ErrCode));
            RetOK = FALSE;
        }
        else
        {
            ErrCode = STDEMUX_SlotClearParams(SlotHandle[DeviceNb][i]);
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(("STDEMUX_SlotClearParams(slotHndl=%d) : failed for audio ! error=%d ",
                             SlotHandle[DeviceNb][i], ErrCode));
                RetOK = FALSE;
            }
        }
    }
    if (RetOK)
    {
        STTBX_Print(("                                 %d audio slot(s) ready for DEMUX device=%d\n",
                 DEMUXCMD_MAX_AUDIO_SLOT, DeviceNb));
    }
#endif /* #ifdef USE_AUDIO */

#ifdef USE_CLKRV
    /* Allocation for PCR */
    SlotData.SlotType = STDEMUX_SLOT_TYPE_PCR;
    SlotData.SlotFlags.SoftwareCDFifo = FALSE;

    /* Initialise all pcr slots */
    for(i = DEMUXCMD_BASE_PCR; (i < (DEMUXCMD_BASE_PCR+DEMUXCMD_MAX_PCR_SLOT)) && RetOK; i++)
    {
        ErrCode = STDEMUX_SlotAllocate( STDEMUXHandle[DeviceNb], &SlotHandle[DeviceNb][i], &SlotData);
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(("STDEMUX_SlotAllocate(hndl=%d) : failed ! error=%d ",
                         STDEMUXHandle[DeviceNb], ErrCode));
            RetOK = FALSE;
        }
        /* Don't clear PCR ?? Why ??*/
    }
    if (RetOK)
    {
        STTBX_Print(("                                 %d pcr slot(s) ready for DEMUX device=%d\n",
                 DEMUXCMD_MAX_PCR_SLOT, DeviceNb));
    }
#endif /* USE_CLKRV */

#ifdef ST_5188
    /* Bit 21 : Disable IPQPSK, routes TS from PIO to FEI */ 
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_C, 0x00200000);
    /* FEI Configuration */
    /* Packet size (set to 0, to let the fifo be flushed only when the sync
     * signal is seen */
    STSYS_WriteRegDev32LE (FEI_SECTOR_SIZE, 0);
    /* FEC, i_ser_sync active high, i_ser_dvalid active high, i_ser_evalid active
     * high, rising edge to sample*/
    STSYS_WriteRegDev32LE (FEI_CONFIG, 0x5E);
    /* Start FEI */
    STSYS_WriteRegDev32LE (FEI_SOFT_RESET, 0x01);
#endif /* ST_5188 */

    return(RetOK);
} /* end of DEMUX_Init() */

#ifdef USE_AUDIO
/*-------------------------------------------------------------------------
 * Function : DEMUX_AudioStop
 *            Stop sending current audio program
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL DEMUX_AudioStop(U32 DeviceNb, U32 SlotNumber)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;

    RetErr = TRUE;
    if(SlotNumber < DEMUXCMD_MAX_AUDIO_SLOT)
    {
        /* Stop previous audio DMA's */
        ErrCode = STDEMUX_SlotClearParams(SlotHandle[DeviceNb][DEMUXCMD_BASE_AUDIO+SlotNumber]);
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(("DEMUX_AudioStop(device=%d,slot=%d) : failed ! err=%d\n",
                         DeviceNb, DEMUXCMD_MAX_VIDEO_SLOT+SlotNumber, ErrCode));
        }
        else
        {
            RetErr = FALSE;
            STTBX_Print(("DEMUX_AudioStop(device=%d,slot=%d) : done\n",
                         DeviceNb, DEMUXCMD_MAX_VIDEO_SLOT+SlotNumber));
        }
    }
    else
    {
        STTBX_Print(("DEMUX_AudioStop(device=%d,slot=%d) : Slot number too high. max=%d !!\n",
                     DeviceNb, DEMUXCMD_MAX_VIDEO_SLOT+SlotNumber, DEMUXCMD_MAX_AUDIO_SLOT));
    }
    return(RetErr);
} /* end of DEMUX_AudioStop() */

/*-------------------------------------------------------------------------
 * Function : DEMUX_AudioStart
 *            Select/Start sending an audio program
 * Input    : audio PID
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL DEMUX_AudioStart(ProgId_t PID, U32 DeviceNb, U32 SlotNumber)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STDEMUX_StreamParams_t StreamParams;
    
    StreamParams.member.Pid = PID;
    RetErr = TRUE;

    if(SlotNumber < DEMUXCMD_MAX_AUDIO_SLOT)
    {

        ErrCode = STDEMUX_SlotSetParams(SlotHandle[DeviceNb][DEMUXCMD_BASE_AUDIO+SlotNumber], &StreamParams);
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(("DEMUX_AudioStart(device=%d,slot=%d,pid=%d): failed ! error=%d\n",
                         DeviceNb, DEMUXCMD_MAX_VIDEO_SLOT+SlotNumber, PID, ErrCode));
        }
        else
        {
            STTBX_Print(("DEMUX_AudioStart(device=%d,slot=%d,pid=%d): done\n",
                         DeviceNb, DEMUXCMD_MAX_VIDEO_SLOT+SlotNumber, PID ));
            RetErr = FALSE;
        }
    }
    else
    {
        STTBX_Print(("DEMUX_AudioStart(device=%d,slot=%d,pid=%d) : Slot number too high. max=%d !!\n",
                     STDEMUXHandle[DeviceNb], PID, DEMUXCMD_MAX_AUDIO_SLOT));
    }
    return(RetErr);

} /* end of DEMUX_AudioStart() */
#endif /* #ifdef USE_AUDIO */

/*-------------------------------------------------------------------------
 * Function : DEMUX_VideoStop
 *            Stop sending current video program
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL DEMUX_VideoStop(U32 DeviceNb, U32 SlotNumber, BOOL Verbose)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;

    /* Stops video DMA's */
    RetErr = TRUE;

    if(SlotNumber < DEMUXCMD_MAX_VIDEO_SLOT)
    {
        ErrCode = STDEMUX_SlotClearParams(SlotHandle[DeviceNb][DEMUXCMD_BASE_VIDEO+SlotNumber]);
        if ( ErrCode != ST_NO_ERROR )
        {
            if (Verbose)
            {
                STTBX_Print(("DEMUX_VideoStop(device=%d,slot=%d): failed ! err=%d\n",
                             DeviceNb, SlotNumber, ErrCode));
            }
        }
        else
        {
            if (Verbose)
            {
                STTBX_Print(("DEMUX_VideoStop(device=%d,slot=%d) : done\n",
                             DeviceNb, SlotNumber));
            }
            RetErr = FALSE;
            /* Caution: return from STDEMUX_SlotClearPid() is immediate, but there can
               still be transfer of last packect going on. So wait a little bit. */
            /* at least this should be done for the STVID_INJECTION_BREAK_WORKAROUND */
            task_delay(MaxDelayForLastPacketTransferWhenStdemuxPidCleared);
        }
    }
    else
    {
        STTBX_Print(("DEMUX_VideoStop(device=%d,slot=%d) : Slot number too high. max=%d !!\n",
                     DeviceNb, SlotNumber, DEMUXCMD_MAX_VIDEO_SLOT));
    }
    return(RetErr);
} /* end of DEMUX_VideoStop() */

/*-------------------------------------------------------------------------
 * Function : DEMUX_VideoStart
 *            Select/Start sending a video program
 * Input    : video PID
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL DEMUX_VideoStart(ProgId_t PID, U32 DeviceNb, U32 SlotNumber, BOOL Verbose)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STDEMUX_StreamParams_t StreamParams;
    
    StreamParams.member.Pid = PID;
    RetErr = TRUE;

    if(SlotNumber < DEMUXCMD_MAX_VIDEO_SLOT)
    {
        /* --- Inject video according to the PID and Fifo --- */
        ErrCode = STDEMUX_SlotSetParams(SlotHandle[DeviceNb][DEMUXCMD_BASE_VIDEO+SlotNumber], &StreamParams);
        if ( ErrCode != ST_NO_ERROR)
        {
            if (Verbose)
            {
                STTBX_Print(("DEMUX_VideoStart(device=%d,slot=%d,pid=%d) : failed ! error=%d\n",
                             DeviceNb, SlotNumber, PID, ErrCode));
            }
        }
        else
        {
            if (Verbose)
            {
                STTBX_Print(("DEMUX_VideoStart(device=%d,slot=%d,pid=%d) : done\n",
                             DeviceNb, SlotNumber, PID ));
            }
            RetErr = FALSE;
        }
    }
    else
    {
        STTBX_Print(("DEMUX_VideoStart(device=%d,slot=%d,pid=%d) : Slot number too high. max=%d !!\n",
                     DeviceNb, SlotNumber, PID, DEMUXCMD_MAX_VIDEO_SLOT));
    }
    return(RetErr);
} /*  end of DEMUX_VideoStart() */

/*-------------------------------------------------------------------------
 * Function : DEMUX_PCRStop
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL DEMUX_PCRStop(U32 DeviceNb, U32 SlotNumber)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;

    RetErr = TRUE;

    if(SlotNumber < DEMUXCMD_MAX_PCR_SLOT)
    {
        ErrCode = STDEMUX_SlotClearParams(SlotHandle[DeviceNb][DEMUXCMD_BASE_PCR+SlotNumber]);
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(("DEMUX_PCRStop(device=%d,slot=%d) : failed ! error=%d \n",
                         DeviceNb, SlotNumber, ErrCode));
        }
        else
        {
            STTBX_Print(("DEMUX_PCRStop(device=%d,slot=%d) : done\n",
                         DeviceNb, SlotNumber));
            RetErr = FALSE;
       }
    }
    else
    {
        STTBX_Print(("DEMUX_PCRStop(device=%d,slot=%d) : Slot number too high. max=%d !!\n",
                     DeviceNb, SlotNumber, DEMUXCMD_MAX_PCR_SLOT));
    }
    return(RetErr);
} /* end of DEMUX_PCRStop() */

/*-------------------------------------------------------------------------
 * Function : DEMUX_PCRStart
 *            Select the PCR
 * Input    : PCR PID
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL DEMUX_PCRStart(ProgId_t PID, U32 DeviceNb, U32 SlotNumber, ST_DeviceName_t ClkrvName)
{
    BOOL                    RetErr;
#ifdef USE_CLKRV
    ST_ErrorCode_t          ErrCode;
    STCLKRV_SourceParams_t  SourceParams;
    STCLKRV_ExtendedSTC_t   ExtendedSTC;
    ST_DeviceName_t         ClkrvDevName_Associated;
    STCLKRV_Handle_t        ClkrvHdl;
    STCLKRV_OpenParams_t    stclkrv_OpenParams;
    STDEMUX_StreamParams_t  StreamParams;
#endif

    RetErr = FALSE;
#ifdef USE_CLKRV
    StreamParams.member.Pid = PID;
    ClkrvHdl=0;

    if(SlotNumber < DEMUXCMD_MAX_PCR_SLOT)
    {
        strcpy(ClkrvDevName_Associated, ClkrvName);
        ErrCode = STCLKRV_Open(ClkrvDevName_Associated, &stclkrv_OpenParams, &ClkrvHdl);
        if ( ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("DEMUX_PCRStart(device=%d) : STCLKRV_Open(devname=%s) failed. Err=0x%x !\n",
                            DeviceNb, ClkrvDevName_Associated, ErrCode));
            RetErr = TRUE;
            ClkrvHdl=0;
        }
        else
        {
            /* Invalidate the PCR synchronization on PCR channel change */
            STCLKRV_InvDecodeClk(ClkrvHdl);

            ErrCode = STDEMUX_SlotSetParams(SlotHandle[DeviceNb][DEMUXCMD_BASE_PCR+SlotNumber], &StreamParams);
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(("DEMUX_PCRStart(device=%d,slot=%d,pid=%d) : failed error=%d!\n",
                            DeviceNb, SlotNumber, PID, ErrCode));
                RetErr = TRUE;
            }
            else
            {
                STTBX_Print(("DEMUX_PCRStart(device=%d,slot=%d,pid=%d) : done\n",
                            DeviceNb, SlotNumber, PID));

                SourceParams.Source = STCLKRV_PCR_SOURCE_PTI;
                SourceParams.Source_u.STPTI_s.Slot = SlotHandle[DeviceNb][DEMUXCMD_BASE_PCR+SlotNumber];

                ErrCode = STCLKRV_SetPCRSource(ClkrvHdl, &SourceParams );
                if (ErrCode != ST_NO_ERROR)
                {
                    STTBX_Print(("DEMUX_PCRStart(device=%d) : STCLKRV_SetPCRSource(devname=%s) failed. Err=0x%x !\n",
                                    DeviceNb, ClkrvDevName_Associated, ErrCode));
                    RetErr = TRUE;
                }

                if (!(RetErr))
                {
                    ErrCode = STCLKRV_SetSTCSource(ClkrvHdl, STCLKRV_STC_SOURCE_PCR);
                    if (ErrCode != ST_NO_ERROR)
                    {
                        STTBX_Print(("DEMUX_PCRStart(device=%d) : STCLKRV_SetSTCSource(devname=%s) failed. Err=0x%x !\n",
                                    DeviceNb, ClkrvDevName_Associated, ErrCode));
                        RetErr = TRUE;
                    }
                }

                if (!(RetErr))
                {
                    /* delay some time so that we can get a successful GetDecodeClock() because PCR was invalidated after change */
                    /*task_delay(20000);*//* before : 2000 --> err getdecodeclk() (FQ Jan 2000)*/
                    task_delay (ST_GetClocksPerSecond());
                    ErrCode = STCLKRV_GetExtendedSTC(ClkrvHdl, &ExtendedSTC);
                    if (ErrCode != ST_NO_ERROR)
                    {
                        STTBX_Print(("DEMUX_PCRPID(device=%d) : STCLKRV_GetExtendedSTC(devname=%s) error 0x%x !\n",
                                    DeviceNb, ClkrvDevName_Associated, ErrCode));
                        RetErr = TRUE;
                    }
                    else
                    {
                        STTBX_Print(("DEMUX_PCRPID(device=%d) : STC=0x%x,%x\n",
                                    DeviceNb, ExtendedSTC.BaseBit32, ExtendedSTC.BaseValue));
                    }
                }
            }
            ErrCode = STCLKRV_Close(ClkrvHdl);
            if (ErrCode!=ST_NO_ERROR)
            {
                STTBX_Print(("DEMUX_PCRStart(device=%d) : STCLKRV_Close() failed. Err=0x%x !\n", DeviceNb, ErrCode));
            }
        }
    }
    else
    {
        STTBX_Print(("DEMUX_PCRStart(device=%d,slot=%d,pid=%d) : Slot number too high. max=%d !!\n",
                     DeviceNb, SlotNumber, PID, DEMUXCMD_MAX_PCR_SLOT));
    }
#else /* not USE_CLKRV */
    STTBX_Print(("STCLKRV not in use !\n"));
#endif /* not USE_CLKRV */
    return(RetErr);

} /* end of DEMUX_PCRStart() */


/*#######################################################################*/
/*########################### DEMUX COMMANDS ##############################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : DemuxCmdErrorDeviceNumber
 *            Dsiplay error for DeviceNb parameter parsing
 * Input    : *pars_p
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void DemuxCmdErrorDeviceNumber(parse_t *pars_p)
{
#if DEMUXCMD_MAX_DEVICES == 1
    sprintf(DEMUX_Msg, "expected Device Number 0 (only one device initalized)");
#else
    sprintf(DEMUX_Msg, "expected Device Number (between 0 and %d)", DEMUXCMD_MAX_DEVICES);
#endif
    STTST_TagCurrentLine(pars_p, DEMUX_Msg);
}

/*-------------------------------------------------------------------------
 * Function : DemuxCmdErrorSlotNumber
 *            Dsiplay error for DeviceNb parameter parsing
 * Input    : *pars_p
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void DemuxCmdErrorSlotNumber(parse_t *pars_p, U32 MaxSlot)
{
    if( MaxSlot == 1)
    {
        sprintf(DEMUX_Msg, "expected slot Number 0 (only one slot reserved)");
    }
    else
    {
        sprintf(DEMUX_Msg, "expected slot Number (between 0 and %d)", MaxSlot - 1);
    }
    STTST_TagCurrentLine(pars_p, DEMUX_Msg);
}

/*-------------------------------------------------------------------------
 * Function : DEMUX_InitCmd
 *            Start sending the specified video program
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL DEMUX_InitCmd( parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    U32 DeviceNb;

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DeviceNb);
    if ((RetErr) || (DeviceNb >= DEMUXCMD_MAX_DEVICES))
    {
        DemuxCmdErrorDeviceNumber(pars_p);
    }
    else
    {
        RetErr = DEMUX_Init(DeviceNb);
        sprintf(DEMUX_Msg, "DEMUX_Init() : DeviceNb=%d : err=%d\n", DeviceNb, RetErr);
        STTBX_Print((DEMUX_Msg));
    }
    return(API_EnableError ? RetErr : FALSE );

} /* end of DEMUX_InitCmd() */


/*******************************************************************************
Name        : DEMUX_TermComponent
Description : Terminate the STDEMUX driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void DEMUX_TermComponent(void)
{
    ST_ErrorCode_t      ErrCode = ST_NO_ERROR;
    U32                 DeviceNb;
    STDEMUX_TermParams_t  TermParams;


    DeviceNb = 0;
    TermParams.ForceTermination = TRUE;

    if (DEMUXCMD_DeviceTypeEventErrorsCapable==TRUE)
    {
        ErrCode = DEMUX_Unsubscribe_Error_Events(DeviceNb);
    }
    ErrCode = STDEMUX_Term(DEMUXDeviceName[DeviceNb], &TermParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("%s term. error 0x%x ! \n",
                        DEMUXDeviceName[DeviceNb], ErrCode));
    }
    else
    {
        STTBX_Print(("%s Terminated\n", DEMUXDeviceName[DeviceNb]));
    }
} /* end PTI_TermComponent() */


#ifdef USE_VIDEO
/*-------------------------------------------------------------------------
 * Function : DEMUX_VideoPid
 *            Start sending the specified video program
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL DEMUX_VideoPID( parse_t *pars_p, char *result_sym_p )
{
    BOOL            RetErr  = FALSE;
    ST_ErrorCode_t  ErrCode = ST_NO_ERROR;
    U32 DeviceNb, SlotNumber;
    ProgId_t PID;
    void * Add;
    U32 Size;

    RetErr = STTST_GetInteger( pars_p, 16, (S32 *)&PID);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected PID" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DeviceNb);
    if ((RetErr) || (DeviceNb >= DEMUXCMD_MAX_DEVICES))
    {
        DemuxCmdErrorDeviceNumber(pars_p);
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&SlotNumber);
    if ((RetErr) || (SlotNumber >= DEMUXCMD_MAX_VIDEO_SLOT))
    {
        DemuxCmdErrorSlotNumber(pars_p, DEMUXCMD_MAX_VIDEO_SLOT);
        return(TRUE);
    }

    /* Lock injection access */
    semaphore_wait(VID_Injection[SlotNumber].Access_p);

    if((VID_Injection[SlotNumber].Type != NO_INJECTION) &&
       (VID_Injection[SlotNumber].Type != VID_LIVE_INJECTION))
    {
        VID_ReportInjectionAlreadyOccurs(SlotNumber+1);
    }
    else
    {
        if((VID_Injection[SlotNumber].Type == VID_LIVE_INJECTION) &&
           (VID_Injection[SlotNumber].Config.Live.DeviceNb != DeviceNb) &&
           (VID_Injection[SlotNumber].Config.Live.SlotNb != SlotNumber))
        {
            VID_ReportInjectionAlreadyOccurs(SlotNumber+1);
        }
        else
        {
            if ((VID_Injection[SlotNumber].Type == VID_LIVE_INJECTION) &&
                (VID_Injection[SlotNumber].State == STATE_INJECTION_NORMAL))
            {
                RetErr = DEMUX_VideoStop(DeviceNb,SlotNumber, TRUE);
                if (!(RetErr))
                {
                    VID_Injection[SlotNumber].Type = NO_INJECTION;
#if defined(ST_8010) || defined(ST_5188) 
                    ErrCode = STDEMUX_BufferDeallocate (VID_Injection[SlotNumber].HandleForPTI);
#endif /* ST_8010 || ST_5188 */
                }
            }
            if (VID_Injection[SlotNumber].Type == NO_INJECTION)
            {
#if defined(ST_8010) || defined(ST_5188)
                /* No cd-fifo: Initialise a destination buffer */
                ErrCode = STVID_GetDataInputBufferParams(
                        VID_Injection[SlotNumber].Driver.Handle,&Add,&Size);

                if (ErrCode == ST_NO_ERROR)
                {
                    VID_Injection[SlotNumber].Base_p = Add;
                    VID_Injection[SlotNumber].Top_p = (void*)((U32)Add + Size -1);

                    /* Align the write and read pointers to the beginning of the input buffer */
                    VID_Injection[SlotNumber].Write_p = VID_Injection[SlotNumber].Base_p;
                    VID_Injection[SlotNumber].Read_p  = VID_Injection[SlotNumber].Base_p;

                    ErrCode = STDEMUX_BufferAllocateManual(STDEMUXHandle[DeviceNb],
                                (U8*)Add,Size,1,&VID_Injection[SlotNumber].HandleForPTI);
                }
                /* No cd-fifo: Initialise the link between demux and video */
                if (ErrCode == ST_NO_ERROR)
                {
                    ErrCode = STDEMUX_SlotLinkToBuffer(SlotHandle[DeviceNb][SlotNumber],
                            VID_Injection[SlotNumber].HandleForPTI);
                }
                if (ErrCode == ST_NO_ERROR)
                {
                    /* Share the living pointers=register fcts */
                    ErrCode = STVID_SetDataInputInterface(
                                VID_Injection[SlotNumber].Driver.Handle,
                                GetWritePtrFct,SetReadPtrFct,
                                (void * const)VID_Injection[SlotNumber].HandleForPTI);
                }
                if (ErrCode == ST_NO_ERROR)
                {
                    ErrCode = STDEMUX_BufferSetOverflowControl(VID_Injection[SlotNumber].HandleForPTI,TRUE);
                }
#endif /* ST_8010 || ST_5188 */
                if (ErrCode == ST_NO_ERROR)
                {
                    RetErr = DEMUX_VideoStart(PID, DeviceNb, SlotNumber, TRUE);
                }
                else
                {
                    RetErr = TRUE;
                }
                if (!(RetErr))
                {
                    VID_Injection[SlotNumber].Type = VID_LIVE_INJECTION;
                    VID_Injection[SlotNumber].Config.Live.Pid = PID;
                    VID_Injection[SlotNumber].Config.Live.SlotNb = SlotNumber;
                    VID_Injection[SlotNumber].Config.Live.DeviceNb = DeviceNb;
                }
            }
            if ((VID_Injection[SlotNumber].Type == VID_LIVE_INJECTION) &&
                (VID_Injection[SlotNumber].State == STATE_INJECTION_STOPPED_BY_EVT))
            {
                /* Stopped by event => Now order by user to  restart with new PID */
                VID_Injection[SlotNumber].Config.Live.Pid = PID;
            }
        }
    }
    /* Free injection access */
    semaphore_signal(VID_Injection[SlotNumber].Access_p);
    return(API_EnableError ? RetErr : FALSE );

} /* end of DEMUX_VideoPID() */

/*-------------------------------------------------------------------------
 * Function : DEMUX_DisableCCControl
 *            Start sending the specified video program
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL DEMUX_DisableCCControl( parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr = FALSE;
    ST_ErrorCode_t      ErrCode = ST_NO_ERROR;
    U32 DeviceNb, SlotNumber;

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DeviceNb);
    if ((RetErr) || (DeviceNb >= DEMUXCMD_MAX_DEVICES))
    {
        DemuxCmdErrorDeviceNumber(pars_p);
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&SlotNumber);
    if ((RetErr) || (SlotNumber >= DEMUXCMD_MAX_VIDEO_SLOT))
    {
        DemuxCmdErrorSlotNumber(pars_p, DEMUXCMD_MAX_VIDEO_SLOT);
        return(TRUE);
    }

    ErrCode = STDEMUX_SlotSetCCControl(SlotHandle[DeviceNb][DEMUXCMD_BASE_VIDEO+SlotNumber], TRUE);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("%s STDEMUX_SlotSetCCControl failed for video slot %d. error 0x%x ! \n",
                        DEMUXDeviceName[DeviceNb], SlotNumber, ErrCode));
    }
    else
    {
        STTBX_Print(("%s CC check DISABLED for video slot %d.\n", DEMUXDeviceName[DeviceNb], SlotNumber));
    }

    return(API_EnableError ? RetErr : FALSE );

} /* end of DEMUX_DisableCCControl() */

/*-------------------------------------------------------------------------
 * Function : DEMUX_EnableCCControl
 *            Start sending the specified video program
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL DEMUX_EnableCCControl( parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr = FALSE;
    ST_ErrorCode_t      ErrCode = ST_NO_ERROR;
    U32 DeviceNb, SlotNumber;

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DeviceNb);
    if ((RetErr) || (DeviceNb >= DEMUXCMD_MAX_DEVICES))
    {
        DemuxCmdErrorDeviceNumber(pars_p);
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&SlotNumber);
    if ((RetErr) || (SlotNumber >= DEMUXCMD_MAX_VIDEO_SLOT))
    {
        DemuxCmdErrorSlotNumber(pars_p, DEMUXCMD_MAX_VIDEO_SLOT);
        return(TRUE);
    }

    ErrCode = STDEMUX_SlotSetCCControl(SlotHandle[DeviceNb][DEMUXCMD_BASE_VIDEO+SlotNumber], FALSE);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("%s STDEMUX_SlotSetCCControl failed for video slot %d. error 0x%x ! \n",
                        DEMUXDeviceName[DeviceNb], SlotNumber, ErrCode));
    }
    else
    {
        STTBX_Print(("%s CC check ENABLED for video slot %d.\n", DEMUXDeviceName[DeviceNb], SlotNumber));
    }

    return(API_EnableError ? RetErr : FALSE );

} /* end of DEMUX_EnableCCControl() */

#endif /* #ifdef USE_VIDEO */


#ifdef USE_AUDIO
/*-------------------------------------------------------------------------
 * Function : DEMUX_AudioPID
 *            Start sending the specified audio program
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL DEMUX_AudioPID(parse_t *pars_p, char *result_sym_p)
{
    ProgId_t PID;
    BOOL RetErr;
    U32 DeviceNb, SlotNumber;

    RetErr = STTST_GetInteger( pars_p, 16, (S32 *)&PID);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected PID" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DeviceNb);
    if ((RetErr) || (DeviceNb >= DEMUXCMD_MAX_DEVICES))
    {
        DemuxCmdErrorDeviceNumber(pars_p);
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&SlotNumber);
    if ((RetErr)|| (SlotNumber >= DEMUXCMD_MAX_AUDIO_SLOT))
    {
        DemuxCmdErrorSlotNumber(pars_p, DEMUXCMD_MAX_AUDIO_SLOT);
        return(TRUE);
    }

    if (DemuxCmdAudioStarted[DeviceNb][SlotNumber])
    {
        RetErr = DEMUX_AudioStop(DeviceNb, SlotNumber);
    }
    if (!(RetErr))
    {
        DemuxCmdAudioStarted[DeviceNb][SlotNumber] = FALSE;
        RetErr = DEMUX_AudioStart(PID, DeviceNb, SlotNumber);
        if (!(RetErr))
        {
            DemuxCmdAudioStarted[DeviceNb][SlotNumber] = TRUE;
        }
    }
    return(API_EnableError ? RetErr : FALSE );

 } /* end of DEMUX_AudioPID() */
#endif /* #ifdef USE_AUDIO */

/*-------------------------------------------------------------------------
 * Function : DEMUX_PCRPID
 *            Start the selected PCR
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL DEMUX_PCRPID(parse_t *pars_p, char *result_sym_p)
{
    ProgId_t PID;
    BOOL RetErr;
    U32 DeviceNb, SlotNumber;
    ST_DeviceName_t ClkrvName;

    /* get PCR PID */
    RetErr = STTST_GetInteger( pars_p, 16, (S32 *)&PID);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected PID" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DeviceNb);
    if ((RetErr) || (DeviceNb >= DEMUXCMD_MAX_DEVICES))
    {
        DemuxCmdErrorDeviceNumber(pars_p);
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&SlotNumber);
    if ((RetErr) || (SlotNumber >= DEMUXCMD_MAX_PCR_SLOT))
    {
        DemuxCmdErrorSlotNumber(pars_p, DEMUXCMD_MAX_PCR_SLOT);
        return(TRUE);
    }

#ifdef USE_CLKRV
    RetErr = STTST_GetString( pars_p, STCLKRV_DEVICE_NAME, ClkrvName, sizeof(ClkrvName) );
#else
    RetErr = STTST_GetString( pars_p, "", ClkrvName, sizeof(ClkrvName) );
#endif
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected clock recovery device name" );
        return(TRUE);
    }

    if (DemuxCmdPCRStarted[DeviceNb][SlotNumber])
    {
        RetErr = DEMUX_PCRStop(DeviceNb, SlotNumber);
    }
    if (!(RetErr))
    {
        DemuxCmdPCRStarted[DeviceNb][SlotNumber] = FALSE;
        RetErr = DEMUX_PCRStart(PID, DeviceNb, SlotNumber, ClkrvName);
        if (!(RetErr))
        {
            DemuxCmdPCRStarted[DeviceNb][SlotNumber] = TRUE;
        }
    }
    return(API_EnableError ? RetErr : FALSE );

} /* end of DEMUX_PCRPID() */

#ifdef USE_AUDIO
/*-------------------------------------------------------------------------
 * Function : DEMUX_AudioStopPID
 *            Stop the audio program
 * Input    : mode
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL DEMUX_AudioStopPID(parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    U32 DeviceNb, SlotNumber;

    RetErr = FALSE;

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DeviceNb);
    if ((RetErr) || (DeviceNb >= DEMUXCMD_MAX_DEVICES))
    {
        DemuxCmdErrorDeviceNumber(pars_p);
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&SlotNumber);
    if ((RetErr) || (SlotNumber >= DEMUXCMD_MAX_AUDIO_SLOT))
    {
        DemuxCmdErrorSlotNumber(pars_p, DEMUXCMD_MAX_AUDIO_SLOT);
        return(TRUE);
    }

    RetErr = DEMUX_AudioStop(DeviceNb, SlotNumber);
    DemuxCmdAudioStarted[DeviceNb][SlotNumber] = FALSE;

    return(API_EnableError ? RetErr : FALSE );

} /* end of DEMUX_AudioStopPID() */
#endif /* #ifdef USE_AUDIO */

#ifdef USE_VIDEO
/*-------------------------------------------------------------------------
 * Function : DEMUX_VideoStopPID
 *            Stop the video program
 * Input    : mode
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL DEMUX_VideoStopPID(parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    U32 DeviceNb,SlotNumber;

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DeviceNb);
    if ((RetErr) || (DeviceNb >= DEMUXCMD_MAX_DEVICES))
    {
        DemuxCmdErrorDeviceNumber(pars_p);
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&SlotNumber);
    if ((RetErr) || (SlotNumber >= DEMUXCMD_MAX_VIDEO_SLOT))
    {
        DemuxCmdErrorSlotNumber(pars_p, DEMUXCMD_MAX_VIDEO_SLOT);
        return(TRUE);
    }

    /* Lock injection access */
    semaphore_wait(VID_Injection[SlotNumber].Access_p);

    if ((VID_Injection[SlotNumber].Type == VID_LIVE_INJECTION) &&
        (VID_Injection[SlotNumber].Config.Live.SlotNb == SlotNumber) &&
        (VID_Injection[SlotNumber].Config.Live.DeviceNb == DeviceNb))
    {
        if(VID_Injection[SlotNumber].State == STATE_INJECTION_NORMAL)
        {
            RetErr = DEMUX_VideoStop(DeviceNb, SlotNumber, TRUE);
            if (!(RetErr))
            {
                VID_Injection[SlotNumber].Type = NO_INJECTION;
#if defined(ST_8010) || defined(ST_5188) 
                RetErr = STDEMUX_BufferDeallocate (VID_Injection[SlotNumber].HandleForPTI);
#endif /* ST_8010 || ST_5188 */
            }
            else
            {
#if defined(ST_8010) || defined(ST_5188)
                /* Now Unlinking slot and buffer to free dmas */
                RetErr = STVID_DeleteDataInputInterface(VID_Injection[SlotNumber].Driver.Handle);
                STTBX_Print(("STVID_DeleteDataInputInterface(Handle=%x,slot=%d): Ret code=%x\n",
                             VID_Injection[SlotNumber].Driver.Handle, SlotNumber, RetErr));
                RetErr = STDEMUX_SlotUnLink(SlotHandle[DeviceNb][SlotNumber]);
                STTBX_Print(("STDEMUX_SlotUnLink(device=%d,slot=%d): Ret code=%x\n",
                             DeviceNb, SlotNumber, RetErr));
                RetErr = STDEMUX_BufferDeallocate(VID_Injection[SlotNumber].HandleForPTI);
                STTBX_Print(("STDEMUX_BufferDeallocate(device=%d,slot=%d): Ret code=%x\n",
                             DeviceNb, SlotNumber, RetErr));
#endif /* ST_8010 || ST_5188 */
            }
        }
        else
        {
            VID_Injection[SlotNumber].Type = NO_INJECTION;
        }
    }
    /* Free injection access */
    semaphore_signal(VID_Injection[SlotNumber].Access_p);

    return(API_EnableError ? RetErr : FALSE );

} /* end of DEMUX_VideoStopPID() */
#endif /* #ifdef USE_VIDEO */

/*-------------------------------------------------------------------------
 * Function : DEMUXGetInputPacketCount
 *            Display DEMUX Input Packet count
 * Input    : *pars_p, *Result
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL DEMUXGetInputPacketCount( STTST_Parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t ErrCode;
    U32 Count;
    U32 DemuxNum;

    STTST_GetInteger( pars_p, 0, (S32*)&DemuxNum );

    if ((ErrCode = STDEMUX_GetInputPacketCount( DEMUXDeviceName[DemuxNum], &Count)) != ST_NO_ERROR )
    {
        STTBX_Print(("STDEMUX_GetInputPacketCount(%s) : Failed Error 0x%x\n", DEMUXDeviceName[DemuxNum], ErrCode));
        return TRUE;
    }

    STTBX_Print(("STDEMUX_GetInputPacketCount(%s)=%d\n", DEMUXDeviceName[DemuxNum], Count));
    return FALSE;
}

/*-------------------------------------------------------------------------
 * Function : DEMUXGetSlotPacketCount
 *            Display DEMUX Packet Error count
 * Input    : *pars_p, *Result
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL DEMUXGetSlotPacketCount( STTST_Parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t ErrCode;
    U32 Count;
    S32 Slot;
    U32 DemuxNum;

    STTST_GetInteger( pars_p, 0, &Slot );
    if ((Slot < 0) || (Slot >= DEMUXCMD_MAX_SLOTS))
    {
        STTBX_Print(("Invalid slot (0..%d)\n", DEMUXCMD_MAX_SLOTS-1));
        return TRUE;
    }

    STTST_GetInteger( pars_p, 0, (S32*)&DemuxNum );

    if ((ErrCode = STDEMUX_SlotPacketCount( SlotHandle[DemuxNum][Slot], &Count)) != ST_NO_ERROR )
    {
        STTBX_Print(("STDEMUX_SlotPacketCount(%d,%s)= Failed Error 0x%x\n",
                     Slot, DEMUXDeviceName[DemuxNum], ErrCode ));
        return TRUE;
    }

    STTBX_Print(("STDEMUX_GetSlotPacketCount(%d,%s)=%d\n", Slot, DEMUXDeviceName[DemuxNum], Count ));
    return FALSE;
}


/*-------------------------------------------------------------------------
 * Function : DEMUXResetErrorsDEMUX
 *
 * Input    : *pars_p, *Result
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL DEMUXResetErrorsDEMUX( STTST_Parse_t *pars_p, char *Result )
{
    S32 SlotNum;

    if (DEMUXCMD_DeviceTypeEventErrorsCapable==FALSE)
    {
        STTBX_Print(("STDEMUX Error events are only available with DEMUX3 or DEMUX4 device !\n"));
        return FALSE;
    }

    STTST_GetInteger( pars_p, -1, &SlotNum );
    if ((SlotNum < -1) || (SlotNum >= DEMUXCMD_MAX_SLOTS))
    {
        STTBX_Print(("Invalid slot. Type (0..%d) or -1 for all\n", DEMUXCMD_MAX_SLOTS-1));
        return TRUE;
    }
    if (SlotNum==-1)
    {
        int i;
        for (i = 0; i < DEMUXCMD_MAX_SLOTS; i++)
        {
            Sections_Discarded_On_Crc_Check_Errors[i]=0;
            Buffer_Overflow_Errors[i]=0;
            CC_Errors[i]=0;
            Packet_Errors[i]=0;
            TC_Code_Fault_Errors[i]=0;
        }
        Total_Stdemux_Errors=0;
        STTBX_Print(("STDEMUX error events count reset for all slots\n"));
    }
    else
    {
        Total_Stdemux_Errors-=Sections_Discarded_On_Crc_Check_Errors[SlotNum];
        Sections_Discarded_On_Crc_Check_Errors[SlotNum]=0;
        Total_Stdemux_Errors-=Buffer_Overflow_Errors[SlotNum];
        Buffer_Overflow_Errors[SlotNum]=0;
        Total_Stdemux_Errors-=CC_Errors[SlotNum];
        CC_Errors[SlotNum]=0;
        Total_Stdemux_Errors-=Packet_Errors[SlotNum];
        Packet_Errors[SlotNum]=0;
        Total_Stdemux_Errors-=TC_Code_Fault_Errors[SlotNum];
        TC_Code_Fault_Errors[SlotNum]=0;
        STTBX_Print(("STDEMUX error events count reset for slot %d\n", SlotNum));
    }

    return FALSE;
}

/*-------------------------------------------------------------------------
 * Function : DEMUXShowErrorsDEMUX
 *
 * Input    : *pars_p, *Result
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL DEMUXShowErrorsDEMUX( STTST_Parse_t *pars_p, char *Result )
{
    S32 SlotNum;

    if (DEMUXCMD_DeviceTypeEventErrorsCapable==FALSE)
    {
        STTBX_Print(("STDEMUX Error events are only available with DEMUX3 or DEMUX4 device !\n"));
        return FALSE;
    }

    STTST_GetInteger( pars_p, -1, &SlotNum );
    if ((SlotNum < -1) || (SlotNum >= DEMUXCMD_MAX_SLOTS))
    {
        STTBX_Print(("Invalid slot. Type (0..%d) or -1 for all\n", DEMUXCMD_MAX_SLOTS-1));
        return TRUE;
    }

    STTBX_Print(("\tCrc_Errors\tBuf_Overflow\tCC_Errors\tPkt_Errors\tTC_Code_Fault\n"));
    if (SlotNum==-1)
    {
        for (SlotNum = 0; SlotNum < DEMUXCMD_MAX_SLOTS; SlotNum++)
        {
           STTBX_Print(("Slot%2d\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n", SlotNum,
                Sections_Discarded_On_Crc_Check_Errors[SlotNum],
                Buffer_Overflow_Errors[SlotNum],
                CC_Errors[SlotNum],
                Packet_Errors[SlotNum],
                TC_Code_Fault_Errors[SlotNum]
            ));
        }
        STTBX_Print(("Total_Stdemux_Errors = %d\n", Total_Stdemux_Errors));

    }
    else
    {
        STTBX_Print(("Slot%2d\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n", SlotNum,
            Sections_Discarded_On_Crc_Check_Errors[SlotNum],
            Buffer_Overflow_Errors[SlotNum],
            CC_Errors[SlotNum],
            Packet_Errors[SlotNum],
            TC_Code_Fault_Errors[SlotNum]
        ));
    }
#ifdef USE_AUDIO
        STTBX_Print(("First video slot = %d, first audio slot = %d, first PCR slot = %d\n",
                DEMUXCMD_BASE_VIDEO, DEMUXCMD_BASE_AUDIO, DEMUXCMD_BASE_PCR));
#else
        STTBX_Print(("First video slot = %d, first PCR slot = %d\n",
                DEMUXCMD_BASE_VIDEO, DEMUXCMD_BASE_PCR));
#endif

    return FALSE;
}

/*-------------------------------------------------------------------------
 * Function : DEMUX_InitCommand
 *            Register the DEMUX commands
 * Input    :
 * Output   :
 * Return   : TRUE if ok
 * ----------------------------------------------------------------------*/
static BOOL DEMUX_InitCommand (void)
{
    BOOL RetErr;
    RetErr = FALSE;
    RetErr |= STTST_RegisterCommand( "PTI_Init", DEMUX_InitCmd,
                                     "<DeviceNb> Initialize a DEMUX device");
#ifdef USE_AUDIO
    RetErr |= STTST_RegisterCommand( "PTI_AudStartPID", DEMUX_AudioPID,
                                     "<PID><DeviceNb> Start sending audio PID to audio decoder");
    RetErr |= STTST_RegisterCommand( "PTI_AudStopPID",  DEMUX_AudioStopPID,
                                     "<DeviceNb> Stop sending audio to audio decoder");
#endif /* #ifdef USE_AUDIO */
#ifdef USE_VIDEO
    RetErr |= STTST_RegisterCommand( "PTI_VidStartPID", DEMUX_VideoPID,
                                     "<PID> <DeviceNb> <Slot> Start sending video PID to video decoder");
    RetErr |= STTST_RegisterCommand( "PTI_VidStopPID",  DEMUX_VideoStopPID,
                                     "<DeviceNb> <Slot> Stop sending video to video decoder");
    RetErr |= STTST_RegisterCommand( "PTI_DisableCCControl", DEMUX_DisableCCControl,
                                     "<DeviceNb> <Slot>  Disable the CC check");
    RetErr |= STTST_RegisterCommand( "PTI_EnableCCControl", DEMUX_EnableCCControl,
                                     "<DeviceNb> <Slot>  Enable the CC check");
#endif /* #ifdef USE_VIDEO */
    RetErr |= STTST_RegisterCommand( "PTI_PCRPID",      DEMUX_PCRPID,
                                     "<PID> <DeviceNb> <Slot> Start sending data PCR PID");
    RetErr |= STTST_RegisterCommand("PTI_GetInputCnt", DEMUXGetInputPacketCount,
                                    "<demux> Display DEMUX packet count");
    RetErr |= STTST_RegisterCommand("PTI_GetSlotCnt", DEMUXGetSlotPacketCount,
                                    "<Slot><demux> Display DEMUX Slot packet count");
    RetErr |= STTST_RegisterCommand("PTI_ResetErrorsDEMUX", DEMUXResetErrorsDEMUX,
                                    "<Slot> Reset DEMUX error events count");
    RetErr |= STTST_RegisterCommand("PTI_ShowerrorsDEMUX", DEMUXShowErrorsDEMUX,
                                    "<Slot> Show DEMUX error events count");

    return(RetErr ? FALSE : TRUE);

} /* end of DEMUX_InitCommand() */



/*##########################################################################*/
/*########################### EXPORTED FUNCTIONS ###########################*/
/*##########################################################################*/

 /*-------------------------------------------------------------------------
 * Function : DEMUX_InitComponent
 *            Initialization of DEMUX and its slots
 * Input    :
 * Output   :
 * Return   : TRUE if ok
 * ----------------------------------------------------------------------*/
BOOL DEMUX_InitComponent(void)
{
    BOOL RetOk;
    U8 i,j;

    /* Min bit rate = 1000000 bit/s ? (we have streams at 500000, but we take low risk)
       Max size of one packet in STDEMUX = 188 bytes ? */
    MaxDelayForLastPacketTransferWhenStdemuxPidCleared = (ST_GetClocksPerSecond() * 188 * 8) / 1000000;
    if (MaxDelayForLastPacketTransferWhenStdemuxPidCleared == 0)
    {
        MaxDelayForLastPacketTransferWhenStdemuxPidCleared = 1;
    }

    for (i = 0; i < DEMUXCMD_MAX_DEVICES; i++)
    {
#ifdef USE_AUDIO
        for(j=0; j< DEMUXCMD_MAX_AUDIO_SLOT; j++)
        {
            DemuxCmdAudioStarted[i][j] = FALSE;
        }
#endif
        for(j=0; j< DEMUXCMD_MAX_PCR_SLOT; j++)
        {
            DemuxCmdPCRStarted[i][j] = FALSE;
        }
        STDEMUXHandle[i] = 0;
    }

    for (i = 0; i < DEMUXCMD_MAX_SLOTS; i++)
    {
        Sections_Discarded_On_Crc_Check_Errors[i]=0;
        Buffer_Overflow_Errors[i]=0;
        CC_Errors[i]=0;
        Packet_Errors[i]=0;
        TC_Code_Fault_Errors[i]=0;
    }
    Total_Stdemux_Errors=0;

    RetOk = DEMUX_Init(0);

    return(RetOk);

} /* end of DEMUX_InitComponent() */

 /*-------------------------------------------------------------------------
 * Function : DEMUX_RegisterCmd
 *            Register the commands
 * Input    :
 * Output   :
 * Return   : TRUE if success, FALSE if error
 * ----------------------------------------------------------------------*/

BOOL DEMUX_RegisterCmd(void)
{
    BOOL RetOk;

    RetOk = DEMUX_InitCommand();
    if (!RetOk)
    {
        STTBX_Print(( "DEMUX_RegisterCmd() \t: failed ! (STDEMUX)\n" ));
    }
    else
    {
        STTBX_Print(( "DEMUX_RegisterCmd() \t: ok           %s\n", STDEMUX_GetRevision()));
    }
    return(RetOk);
} /* end of DEMUX_RegisterCmd() */


#endif /* USE_DEMUX */
/* End of demux_cmd.c */





