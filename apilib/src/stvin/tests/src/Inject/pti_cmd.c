/************************************************************************
COPYRIGHT (C) STMicroelectronics 1998

File name   : pti_cmd.c
Description : Commands for Demux

Note        : All functions return TRUE if error for CLILIB compatibility

Date          Modification                                    Initials
----          ------------                                    --------
12 Mar 1999   Creation                                        FQ
08 Sep 1999   Change STTBX_Output() by STTBX_Print()          FQ
30 Sep 1999   Move init. PTI buffer in PTI_InitComponent()    FQ
04 Oct 1999   Use stlite.h instead of os20.h                  FQ
12 Jan 2001   Add STPTI commands                              FQ
************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include <stdio.h>
#include <string.h>

#include "stddefs.h"
#include "stlite.h"
#include "sttbx.h"
#include "testtool.h"
#include "stclkrv.h"
#include "api_cmd.h"
#include "startup.h"

#ifdef DVD_TRANSPORT_STPTI
#include "stpti.h"
#else
#include "pti.h"
#endif

#include "stdevice.h"


/*###########################################################################*/
/*############################## DEFINITION #################################*/
/*###########################################################################*/


/* --- Constants ---------------------------------------------------------- */

#if defined(ST_7015) || defined(ST_7020)
/* #define STPTI_MAX_DEVICES     5 --> error 0x04 on STPTI2 at init */
#define STPTI_MAX_DEVICES     1
#else
#define STPTI_MAX_DEVICES     1
#endif

#ifdef DVD_TRANSPORT_STPTI
#define STPTI_DEVICE_NAME1    "STPTI1"
#define STPTI_DEVICE_NAME2    "STPTI2"
#define STPTI_DEVICE_NAME3    "STPTI3"
#define STPTI_DEVICE_NAME4    "STPTI4"
#define STPTI_DEVICE_NAME5    "STPTI5"

typedef unsigned int pid_t; /* defined as int for pti/link, but U16 for spti */
#endif

/* --- Global variables --------------------------------------------------- */

static S16 CurrentVideoFifoNb=1; /* default=1, 1 to 5 for STi7015 */

static BOOL SendingVideo[STPTI_MAX_DEVICES] = FALSE; /* flag */
static BOOL SendingAudio = FALSE;                    /* flag */
static BOOL SendingPCR = FALSE;                      /* flag */

static char PTI_Msg[200]; /* for trace */

#ifdef DVD_TRANSPORT_STPTI
ST_DeviceName_t STPTIDevice[STPTI_MAX_DEVICES];
STPTI_Handle_t STPTIHandle[STPTI_MAX_DEVICES];
STPTI_Slot_t SlotHandle[STPTI_MAX_DEVICES];
#else
slot_t VSlot;
slot_t ASlot;
#endif

static int InitDevNumber = 0;


/* --- Extern variables (from startup.c) ---------------------------------- */


extern STCLKRV_Handle_t CLKRVHndl0;

#ifdef DVD_TRANSPORT_STPTI
extern ST_Partition_t *DriverPartition;
extern ST_Partition_t *NCachePartition;
#endif



/*##########################################################################*/
/*######################### INTERNAL FUNCTIONS #############################*/
/*##########################################################################*/

/*****************************************************************************
Name        : PTI_Init
Description : Initialise the PTI/LINK of STPTI driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*****************************************************************************/
static BOOL PTI_Init(void)
{
    BOOL RetOK;
#ifdef DVD_TRANSPORT_STPTI
    ST_ErrorCode_t ErrCode;
    STPTI_InitParams_t STPTIInitParams;
    STPTI_OpenParams_t STPTIOpenParams;
    STPTI_DMAParams_t CdFifoParams_p;
    STPTI_SlotData_t SlotData;
    S16 DeviceNb;
#else
    BOOL RetErr;
    unsigned int Size;
    unsigned char *Buffer;
    unsigned int PTI_driver_code_version;
    unsigned int PTI_tc_code_version;
	unsigned int PTI_variant;
#endif

#ifndef DVD_TRANSPORT_STPTI
    pti_version( &PTI_driver_code_version, &PTI_tc_code_version, &PTI_variant );
    STTBX_Print(( "PTI      : version=0x%X tc=0x%X variant=0x%X \n",
             PTI_driver_code_version, PTI_tc_code_version, PTI_variant));
#endif

    RetOK = TRUE;

    /* --- Driver initialization --- */

#ifdef DVD_TRANSPORT_STPTI
    /* Set Init param */
    memset((void *)&STPTIInitParams, 0, sizeof(STPTI_InitParams_t));
    STPTIInitParams.Device                  = STPTI_DEVICE_PTI_1 ;  /* PTI1 for 5510 5512 ST20TP3 */
    STPTIInitParams.TCDeviceAddress_p       = (STPTI_DevicePtr_t) PTI_BASE_ADDRESS;
    STPTIInitParams.TCLoader_p              = STPTI_DVBTC1LoaderManualInputcount;
    STPTIInitParams.DescramblerAssociation  = STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_SLOTS;
    STPTIInitParams.TCCodes                 = STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB;
    STPTIInitParams.Partition_p             = DriverPartition;
    STPTIInitParams.NCachePartition_p       = NCachePartition;
    strcpy((char*)&STPTIInitParams.EventHandlerName, (char*)STEVT_DEVICE_NAME);
    STPTIInitParams.EventProcessPriority    = 8; /* MIN_USER_PRIORITY */
    STPTIInitParams.InterruptProcessPriority= 8; /* MAX_USER_PRIORITY */
    STPTIInitParams.InterruptLevel          =  PTI_INTERRUPT_LEVEL;
    STPTIInitParams.InterruptNumber         =  PTI_INTERRUPT;
    STPTIInitParams.SyncLock                = 0;
    STPTIInitParams.SyncDrop                = 0;
	STPTIInitParams.SectionFilterOperatingMode = STPTI_FILTER_OPERATING_MODE_32x8;
#ifdef STPTI_INDEX_SUPPORT
    InitParams.IndexAssociation = STPTI_INDEX_ASSOCIATION_ASSOCIATE_WITH_SLOTS;
    InitParams.IndexProcessPriority = MIN_USER_PRIORITY;
#endif
#ifdef STPTI_CAROUSEL_SUPPORT
    InitParams.CarouselProcessPriority = MIN_USER_PRIORITY;
#endif

    DeviceNb = 0;
    while ( DeviceNb < STPTI_MAX_DEVICES && RetOK == TRUE )
    {
        if ( DeviceNb == 0 )
            strcpy(STPTIDevice[DeviceNb],STPTI_DEVICE_NAME1);
        if ( DeviceNb == 1 )
            strcpy(STPTIDevice[DeviceNb],STPTI_DEVICE_NAME2);
        if ( DeviceNb == 2 )
            strcpy(STPTIDevice[DeviceNb],STPTI_DEVICE_NAME3);
        if ( DeviceNb == 3 )
            strcpy(STPTIDevice[DeviceNb],STPTI_DEVICE_NAME4);
        if ( DeviceNb == 4 )
            strcpy(STPTIDevice[DeviceNb],STPTI_DEVICE_NAME5);
        ErrCode = STPTI_Init(STPTIDevice[DeviceNb], &STPTIInitParams);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("%s init. error 0x%x ! \n",
                          STPTIDevice[DeviceNb], ErrCode));
            RetOK = FALSE;
        }
        else
        {
            STTBX_Print(( "STPTI initialized, \trevision=%s\n", STPTI_GetRevision() ));
        }
        DeviceNb++;
    }
    InitDevNumber = DeviceNb;
#else
    RetErr = pti_init(associate_descramblers_with_slots);
    if (RetErr)
    {
        STTBX_Print(("init. error 0x%x !\n", RetErr));
        RetOK = FALSE;
    }
#endif

    /* --- Connection opening --- */

#ifdef DVD_TRANSPORT_STPTI
    if (RetOK)
    {
        DeviceNb =  0;
        while ( DeviceNb < STPTI_MAX_DEVICES && RetOK == TRUE )
        {
            STPTIOpenParams.DriverPartition_p = DriverPartition;
            STPTIOpenParams.NonCachedPartition_p = NCachePartition;

            ErrCode = STPTI_Open(STPTIDevice[DeviceNb], &STPTIOpenParams, &(STPTIHandle[DeviceNb]));
            if (ErrCode != ST_NO_ERROR)
            {
                STTBX_Print(("STPTI_Open(%s,%d,hndl=%d) : failed ! error=%d\n",
                              STPTIDevice[DeviceNb], &STPTIOpenParams, &(STPTIHandle[DeviceNb]), ErrCode));
                RetOK = FALSE;
            }

            /* --- Slots opening --- */

            if (RetOK)
            {
                SlotData.SlotType = STPTI_SLOT_TYPE_PES;
                SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
                SlotData.SlotFlags.CollectForAlternateOutputOnly = FALSE;
                #ifdef STPTI_CAROUSEL_SUPPORT
                SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
                #endif
                SlotData.SlotFlags.StoreLastTSHeader = FALSE;
                SlotData.SlotFlags.InsertSequenceError = FALSE;

                ErrCode = STPTI_SlotAllocate( STPTIHandle[DeviceNb], &(SlotHandle[DeviceNb]), &SlotData);
                if ( ErrCode != ST_NO_ERROR )
                {
                    STTBX_Print(("STPTI_SlotAllocate(hndl=%d,slotHndl=%d,&data=%d) : failed ! error=%d\n",
                                STPTIHandle[DeviceNb], &(SlotHandle[DeviceNb]), &SlotData, ErrCode));
                    RetOK = FALSE;
                }
            }
            DeviceNb++;

        } /* end while */
    } /* end if RetOK */
#else
    #if defined(ST_5508) || defined(ST_5518)
    if (RetOK)
    {
        RetErr = pti_allocate_dynamic_slot(&VSlot);
        if ( RetErr )
        {
            STTBX_Print(("pti_allocate_dynamic_slot() failed !\n"));
            RetOK = FALSE;
        }
    }
    #endif
#endif

    /* --- Video buffers opening --- */

#ifndef DVD_TRANSPORT_STPTI
    if (RetOK)
    {
        #if defined(ST_5508) || defined(ST_5518)
        RetErr = pti_malloc_buffer(8192, &Size, &Buffer);
        #endif
        #if defined(ST_5510) || defined(ST_5512)
        RetErr = pti_malloc_buffer(4096, &Size, &Buffer);
        #endif
        if ( RetErr )
        {
            STTBX_Print(("pti_malloc_buffer() failed for video !\n"));
            RetOK = FALSE;
        }

        #if defined(ST_5508) || defined(ST_5518)
        /*RetErr = pti_set_buffer(VSlot, stream_type_video_HDD, Buffer, Size, NULL, NULL, NULL , no_flags);*/
        RetErr = pti_set_buffer(VSlot, stream_type_video, Buffer, 0, NULL, NULL, NULL , no_flags);
        #endif
        #if defined(ST_5510) || defined(ST_5512)
        RetErr = pti_set_buffer(PTI_VIDEO_SLOT, stream_type_video, Buffer, Size, NULL, NULL, NULL , no_flags);
        #endif
        if ( RetErr )
        {
            STTBX_Print(("pti_set_buffer() failed  for video !\n"));
            RetOK = FALSE;
        }
    }
#endif

    /* --- Audio buffers opening --- */

#ifndef DVD_TRANSPORT_STPTI
    if (RetOK)
    {
        #if defined(ST_5508) || defined(ST_5518)
        RetErr = pti_malloc_buffer(8192, &Size, &Buffer);
        #endif
        #if defined(ST_5510) || defined(ST_5512)
        RetErr = pti_malloc_buffer(4096, &Size, &Buffer);
        #endif
        if ( RetErr )
        {
            STTBX_Print(("pti_malloc_buffer() failed for audio !\n"));
            RetOK = FALSE;
        }

        #if defined(ST_5508) || defined(ST_5518)
        RetErr = pti_allocate_dynamic_slot(&ASlot);
        if ( RetErr )
        {
            STTBX_Print(("pti_allocate_dynamic_slot() failed !\n"));
            RetOK = FALSE;
        }
        #endif

        #if defined(ST_5508) || defined(ST_5518)
        /*RetErr = pti_set_buffer(ASlot, stream_type_audio_HDD, Buffer, Size, NULL, NULL, NULL , no_flags);*/
        RetErr = pti_set_buffer(ASlot, stream_type_audio, Buffer, 0, NULL, NULL, NULL , no_flags);
        #endif
        #if defined(ST_5510) || defined(ST_5512)
        RetErr = pti_set_buffer(PTI_AUDIO_SLOT, stream_type_audio, Buffer, Size, NULL, NULL, NULL , no_flags);
        #endif
        if ( RetErr )
        {
            STTBX_Print(("pti_set_buffer() failed for audio !\n"));
            RetOK = FALSE;
        }
    }
#endif

    /* --- FIFO selection --- */

#ifdef DVD_TRANSPORT_STPTI
    if (RetOK)
    {
        #if defined (ST_7015) || defined (ST_7020)
        CdFifoParams_p.Destination = VIDEO_CD_FIFO1;
        #else
        CdFifoParams_p.Destination = VIDEO_CD_FIFO;
        #endif
        DeviceNb = 0;
        while ( DeviceNb < STPTI_MAX_DEVICES && RetOK == TRUE )
        {
            /* device N should send the PES on FIFO N through slot N */
            #if defined (ST_7015) || defined (ST_7020)
            if ( DeviceNb==1 )
                CdFifoParams_p.Destination = VIDEO_CD_FIFO2;
            if ( DeviceNb==2 )
                CdFifoParams_p.Destination = VIDEO_CD_FIFO3;
            if ( DeviceNb==3 )
                CdFifoParams_p.Destination = VIDEO_CD_FIFO4;
            if ( DeviceNb==4 )
                CdFifoParams_p.Destination = VIDEO_CD_FIFO5;
            #endif
            CdFifoParams_p.Holdoff = 1;
            CdFifoParams_p.WriteLength = 16; /*16;*/

            ErrCode = STPTI_SlotLinkToCdFifo(SlotHandle[DeviceNb], &CdFifoParams_p);
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(("STPTI_SlotLinkToCdFifo(hndl=%d,dest=%0x) : failed ! device %d error=%d\n",
                              SlotHandle[DeviceNb], &CdFifoParams_p.Destination, DeviceNb, ErrCode));
                RetOK = FALSE;
            }
            DeviceNb++;
        }
        /*CurrentVideoFifoNb = 1;*/
    }
#endif

    if (!RetOK)
    {
        STTBX_Print(("PTI_Init() failed !\n"));
    }
    return(RetOK);
} /* end of PTI_Init() */


/*-------------------------------------------------------------------------
 * Function : PTI_AudioStop
 *            Stop sending current audio program
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_AudioStop(void)
{
    BOOL RetErr;

    /* Stop previous audio DMA's */
#ifdef DVD_TRANSPORT_STPTI
    /*RetErr = STPTI_DMA_SetState(discard_data);
    RetErr |= STPTI_DMA_Reset();*/
#else
    RetErr = pti_audio_dma_state(discard_data);
    RetErr |= pti_audio_dma_reset();
#endif
    SendingAudio = FALSE;

    return(RetErr);
} /* end of PTI_AudioStop() */

/*-------------------------------------------------------------------------
 * Function : PTI_AudioStart
 *            Select/Start sending an audio program
 * Input    : audio PID
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_AudioStart(pid_t PID)
{
    BOOL RetErr;
#ifdef DVD_TRANSPORT_STPTI
    ST_ErrorCode_t ErrCode;
#else
    BOOL Err;
#endif

    RetErr = FALSE;
    if (SendingAudio)
    {
        RetErr = PTI_AudioStop();
    }
#ifndef DVD_TRANSPORT_STPTI
    /* allocate PES channels */
    #if defined(ST_5508) || defined(ST_5518)
    Err = pti_flush_stream(ASlot);
    #endif /**/
    #if defined(ST_5510) || defined(ST_5512)
    Err = pti_flush_stream(PTI_AUDIO_SLOT);
    #endif /**/
    if ( Err )
    {
        STTBX_Print(("PTI_AudioStart() : pti_flush_stream() failed \n"));
        RetErr = TRUE;
    }
    /* change video program */
    #if defined(ST_5508) || defined(ST_5518)
    Err = pti_set_pid(ASlot, PID);
    #endif /**/
    #if defined(ST_5510) || defined(ST_5512)
    Err = pti_set_pid(PTI_AUDIO_SLOT, PID);
    #endif /**/

    if ( Err )
    {
        STTBX_Print(("PTI_AudioStart() : pti_set_pid() failed \n"));
        RetErr = TRUE;
    }
    #if defined(ST_5508) || defined(ST_5518)
    Err = pti_flush_stream(ASlot);
    #endif
    #if defined(ST_5510) || defined(ST_5512)
    Err = pti_flush_stream(PTI_AUDIO_SLOT);
    #endif
    if ( Err )
    {
        STTBX_Print(("PTI_AudioStart() : pti_flush_stream() failed \n"));
        RetErr = TRUE;
    }
#else
    ErrCode = STPTI_HardwareResume(STPTIDevice[CurrentVideoFifoNb-1]);
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(("PTI_AudioStart() : STPTI_HardwareResume() failed ! error=%d\n", ErrCode));
        RetErr = TRUE;
    }
#endif
#ifndef DVD_TRANSPORT_STPTI
    Err = pti_resume();
    if ( Err )
    {
        STTBX_Print(("PTI_AudioStart() : pti_resume() failed \n"));
        RetErr = TRUE;
    }
#endif

#ifdef DVD_TRANSPORT_STPTI
    /*ErrCode = STPTI_DMA_SetState(discard_data);*/
#else
    Err = pti_audio_dma_state(transfer_data);
    if ( Err )
    {
        STTBX_Print(("PTI_AudioStart() : pti_audio_dma_state() failed \n"));
        RetErr = TRUE;
    }
#endif

    /****** before
    RetErr |= pti_set_buffer(PTI_AUDIO_SLOT, stream_type_audio, NULL, 0, NULL, NULL, NULL , no_flags);
    RetErr |= pti_set_pid(PTI_AUDIO_SLOT, PID);
    RetErr |= pti_audio_dma_state(transfer_data);
    ******/
    SendingAudio = TRUE;

    return(RetErr);

} /* end of PTI_AudioStart() */

/*-------------------------------------------------------------------------
 * Function : PTI_VideoStop
 *            Stop sending current video program
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_VideoStop(void)
{
    BOOL RetErr;
#ifdef DVD_TRANSPORT_STPTI
    /*ST_ErrorCode_t ErrCode;*/
#endif

    /* Stops video DMA's */
#ifdef DVD_TRANSPORT_STPTI
    RetErr = FALSE;
    RetErr = STPTI_HardwareReset(STPTIDevice[CurrentVideoFifoNb-1]);
    /*RetErr = STPTI_DMA_SetState(discard_data);
    RetErr |= STPTI_DMA_Reset();*/
#else
    RetErr = pti_video_dma_state(discard_data);
    RetErr |= pti_video_dma_reset();
#endif
    SendingVideo[CurrentVideoFifoNb-1] = FALSE;

    return(RetErr);
} /* end of PTI_VideoStop() */

/*-------------------------------------------------------------------------
 * Function : PTI_VideoStart
 *            Select/Start sending a video program
 * Input    : video PID
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_VideoStart(pid_t PID)
{
    BOOL Err, RetErr;
#ifdef DVD_TRANSPORT_STPTI
    ST_ErrorCode_t ErrCode;
#endif

    RetErr = FALSE;

    /* --- Stop sending data --- */

    if (SendingVideo[CurrentVideoFifoNb-1])
    {
        RetErr = PTI_VideoStop();
        if ( RetErr )
        {
            STTBX_Print(("PTI_VideoStart() : PTI_VideoStop() failed !\n"));
        }
    }
    /* Stop data arriving at the IIF (avoid FIFO overflow) */
#ifdef DVD_TRANSPORT_STPTI
    ErrCode = STPTI_HardwarePause(&STPTIDevice[CurrentVideoFifoNb-1][0]);
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(("PTI_VideoStart() : %s STPTI_HardwarePause(%s) failed ! error=%d\n",
                      STPTI_DEVICE_NAME1, STPTIDevice[CurrentVideoFifoNb-1], ErrCode ));
        RetErr = TRUE;
    }
#else
    Err = pti_pause();
    if ( Err )
    {
        STTBX_Print(("PTI_VideoStart() : pti_pause() failed !\n"));
        RetErr = TRUE;
    }

    #if defined(ST_5508) || defined(ST_5518)
    Err = pti_flush_stream(VSlot);
    #endif
    #if defined(ST_5510) || defined(ST_5512)
    Err = pti_flush_stream(PTI_VIDEO_SLOT);
    #endif
    #if defined(ST_5508) || defined(ST_5518) || defined(ST_5510) || defined(ST_5512)
    if ( Err )
    {
        STTBX_Print(("PTI_VideoStart() : pti_flush_stream() failed ! error=%d\n", Err));
        RetErr = TRUE;
    }
    #endif
#endif

    /* --- Change video program --- */

#ifdef DVD_TRANSPORT_STPTI
    ErrCode = STPTI_SlotSetPid(SlotHandle[CurrentVideoFifoNb-1], PID);
    if ( ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("PTI_VideoStart() : STPTI_SlotSetPid() failed ! error=%d", ErrCode));
    }
#else
    #if defined(ST_5508) || defined(ST_5518)
    Err = pti_set_pid(VSlot, PID);
    #endif
    #if defined(ST_5510) || defined(ST_5512)
    Err = pti_set_pid(PTI_VIDEO_SLOT, PID);
    #endif
    if ( Err )
    {
        STTBX_Print(("PTI_VideoStart() : pti_set_pid() failed ! error=%d\n", Err));
        RetErr = TRUE;
    }
#endif

    /* --- Restart incoming data --- */

#ifdef DVD_TRANSPORT_STPTI
    Err = STPTI_HardwareResume(STPTIDevice[CurrentVideoFifoNb-1]);
    if ( Err )
    {
        STTBX_Print(("PTI_VideoStart() : STPTI_HardwareResume(%s) failed ! error=%d \n",
                      STPTIDevice[CurrentVideoFifoNb-1], Err ));
        RetErr = TRUE;
    }
#else
    #if defined(ST_5508) || defined(ST_5518)
    Err = pti_flush_stream(VSlot);
    #endif
    #if defined(ST_5510) || defined(ST_5512)
    Err = pti_flush_stream(PTI_VIDEO_SLOT);
    #endif
    if ( Err )
    {
        STTBX_Print(("PTI_VideoStart() : pti_flush_stream() failed \n"));
        RetErr = TRUE;
    }

    Err = pti_resume();
    if ( Err )
    {
        STTBX_Print(("PTI_VideoStart() : pti_resume() failed \n"));
        RetErr = TRUE;
    }

    Err = pti_video_dma_state(transfer_data);
    if ( Err )
    {
        STTBX_Print(("PTI_VideoStart() : pti_video_dma_state() failed \n"));
        RetErr = TRUE;
    }
#endif

    /**** before
    pti_set_buffer(PTI_VIDEO_SLOT, stream_type_video, NULL, 0, NULL, NULL, NULL , no_flags);
    pti_set_pid(PTI_VIDEO_SLOT, PID);
    pti_video_dma_state(transfer_data);
    *********/
    SendingVideo[CurrentVideoFifoNb-1] = TRUE;

    return(RetErr);

} /*  end of PTI_VideoStart() */

/*-------------------------------------------------------------------------
 * Function : PTI_PCRStop
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_PCRStop(void)
{
    BOOL RetErr;
#ifdef DVD_TRANSPORT_STPTI
    ST_ErrorCode_t ErrCode;

    ErrCode = STPTI_SlotClearPid(SlotHandle[CurrentVideoFifoNb-1]);
    if ( ErrCode != ST_NO_ERROR )
    {
        RetErr = TRUE ;
    }
#else
    RetErr = pti_clear_pcr_pid();
#endif
    SendingPCR = FALSE;

    return(RetErr);
} /* end of PTI_PCRStop() */

/*-------------------------------------------------------------------------
 * Function : PTI_PCRStart
 *            Select the PCR
 * Input    : PCR PID
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_PCRStart(pid_t PID)
{
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;

    RetErr = FALSE;
    if (SendingPCR)
    {
        RetErr = PTI_PCRStop();
    }

    if (RetErr)
    {
        STTBX_Print(("PTI_PCRStop() failed in PTI_PCRStart() !\n"));
    }
    else
    {
#ifdef DVD_TRANSPORT_STPTI
        RetErr = STPTI_SlotSetPid(SlotHandle[CurrentVideoFifoNb-1], PID);
        if (RetErr)
        {
            STTBX_Print(("PTI_PCRStart() : STPTI_SlotSetPid(%d,%d) failed !\n",
                          SlotHandle[CurrentVideoFifoNb-1], PID));
        }
#else
        RetErr = pti_set_pcr_pid(PID);
        if (RetErr)
        {
            STTBX_Print(("pti_set_pcr_pid() failed in PTI_PCRStart() !\n"));
        }
#endif
        if (!RetErr)
        {
            SendingPCR = TRUE;

            /* Invalidate clock recovery PCR data because PCR channel (PID) is changing. */
            ErrorCode = STCLKRV_InvDecodeClk(CLKRVHndl0);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Print(("PTI_PCRStart() : STCLKRV_InvDecodeClk failed to invalidate PCR data\n"));
                RetErr = TRUE;
            }
        }
    }

    return(RetErr);

} /* end of PTI_PCRStart() */


/*#######################################################################*/
/*########################### PTI COMMANDS ##############################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : PTI_VideoPid
 *            Start sending the specified video program
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL PTI_VideoPID( parse_t *pars_p, char *result_sym_p )
{
    pid_t PID;
    BOOL RetErr;
#if defined (ST_7015) || defined (ST_7020)
    S32 FifoNb;
#endif

    RetErr = STTST_GetInteger( pars_p, 16, (S32 *)&PID);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected PID" );
    }
    else
    {
#if defined (ST_7015) || defined (ST_7020)
        if ( RetErr == TRUE )
        {
            RetErr = STTST_GetInteger( pars_p, 1, &FifoNb);
            if ( RetErr == TRUE || (FifoNb < 1 && FifoNb > STPTI_MAX_DEVICES) )
            {
                sprintf(PTI_Msg, "expected Fifo number (1 to %d)", STPTI_MAX_DEVICES);
                STTST_TagCurrentLine( pars_p, PTI_Msg);
            }
            CurrentVideoFifoNb = (S16)FifoNb;
        }
#endif
        RetErr = PTI_VideoStart(PID);
        sprintf(PTI_Msg, "PTI_VideoPID() : pid=0x%X : err=%d\n", PID, RetErr);
	    STTBX_Print((PTI_Msg));
    }
    return(RetErr);

} /* end of PTI_VideoPID() */

/*-------------------------------------------------------------------------
 * Function : PTI_AudioPID
 *            Start sending the specified audio program
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
 static BOOL PTI_AudioPID(parse_t *pars_p, char *result_sym_p)
{
    pid_t PID;
    BOOL RetErr;

    RetErr = STTST_GetInteger( pars_p, 16, (S32 *)&PID);
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine( pars_p, "expected PID" );
    }
    else
    {
        RetErr = PTI_AudioStart(PID);
        sprintf(PTI_Msg, "PTI_AudioPID() : pid=0x%X : err=%d\n", PID, RetErr);
        STTBX_Print((PTI_Msg));
    }
    return(RetErr);

 } /* end of PTI_AudioPID() */

/*-------------------------------------------------------------------------
 * Function : PTI_PCRPID
 *            Start the selected PCR
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL PTI_PCRPID(parse_t *pars_p, char *result_sym_p)
{
    STCLKRV_ExtendedSTC_t ExtendedSTC;
    pid_t PID;
    BOOL RetErr;
    ST_ErrorCode_t ErrorCode;

    /* get PCR PID */
    RetErr = STTST_GetInteger( pars_p, 16, (S32 *)&PID);
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine( pars_p, "expected PID" );
    }
    else
    {
#ifdef DVD_TRANSPORT_STPTI
        STPTI_HardwarePause(STPTIDevice[CurrentVideoFifoNb-1]);
#else
        pti_pause();
#endif
         /* stop data arriving at the IIF (avoid FIFO overflow) */
        RetErr = PTI_PCRStart(PID);
        if (RetErr)
        {
            STTBX_Print(("PTI_PCRPID() : PTI_PCRStart() failed !\n"));
        }
        else
        {
#ifdef DVD_TRANSPORT_STPTI
            RetErr = STPTI_HardwareResume(STPTIDevice[CurrentVideoFifoNb-1]);
#else
            RetErr = pti_resume();
#endif
            if (RetErr)
            {
                STTBX_Print(("PTI_PCRPID() : pti_resume() failed !\n"));
            }
            else
            {
                /* delay some time so that we can get a successful GetDecodeClock() because PCR was invalidated after change */
                task_delay(20000);/* before : 2000 --> err getdecodeclk() (FQ Jan 2000)*/
                ErrorCode = STCLKRV_GetExtendedSTC(CLKRVHndl0, &ExtendedSTC);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Print(("PTI_PCRPID() : STCLKRV_GetSTC() failed (err=0x%x) !\n", ErrorCode));
                }
                else
                {
                    STTBX_Print(("PTI_PCRPID() : STC=0x%x,%x\n", ExtendedSTC.BaseBit32, ExtendedSTC.BaseValue));
                }
            }
        }
    }
    return(RetErr);

} /* end of PTI_PCRPID() */

/*-------------------------------------------------------------------------
 * Function : PTI_AudioStopPID
 *            Stop the audio program
 * Input    : mode
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_AudioStopPID(parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;

    RetErr = FALSE;
    if (SendingAudio)
    {
        RetErr = PTI_AudioStop();
    }
	return(RetErr);

} /* end of PTI_AudioStopPID() */

/*-------------------------------------------------------------------------
 * Function : PTI_VideoStopPID
 *            Stop the video program
 * Input    : mode
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_VideoStopPID(parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
#if defined (ST_7015) || defined (ST_7020)
    S32 FifoNb;

    RetErr = STTST_GetInteger( pars_p, 1, &FifoNb);
    if ( RetErr == TRUE || (FifoNb < 1 && FifoNb > STPTI_MAX_DEVICES) )
    {
        sprintf(PTI_Msg, "expected Fifo number (1 to %d)", STPTI_MAX_DEVICES);
        STTST_TagCurrentLine( pars_p, PTI_Msg);
    }
    CurrentVideoFifoNb = (S16)FifoNb;
#endif

    RetErr = FALSE;
	if (SendingVideo[CurrentVideoFifoNb])
    {
        RetErr = PTI_VideoStop();
    }
	return(RetErr);
} /* end of PTI_VideoStopPID() */

 /*-------------------------------------------------------------------------
 * Function : PTI_TermComponent
 *            Termination of PTI
 * Input    :
 * Output   :
 * Return   : TRUE if ok
 * ----------------------------------------------------------------------*/
void PTI_TermComponent(void)
{
#ifdef DVD_TRANSPORT_STPTI
    ST_ErrorCode_t ErrCode;
    STPTI_TermParams_t TermParams;
    int DeviceNb;

    DeviceNb = 0;
    while ( DeviceNb < InitDevNumber)
    {
        if ( DeviceNb == 0 )
            strcpy(STPTIDevice[DeviceNb], STPTI_DEVICE_NAME1);
        if ( DeviceNb == 1 )
            strcpy(STPTIDevice[DeviceNb], STPTI_DEVICE_NAME2);
        if ( DeviceNb == 2 )
            strcpy(STPTIDevice[DeviceNb], STPTI_DEVICE_NAME3);
        if ( DeviceNb == 3 )
            strcpy(STPTIDevice[DeviceNb], STPTI_DEVICE_NAME4);
        if ( DeviceNb == 4 )
            strcpy(STPTIDevice[DeviceNb], STPTI_DEVICE_NAME5);
        ErrCode = STPTI_Close(STPTIHandle[DeviceNb]);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("Error closing %d STPTI handle. Error 0x%x ! \n", STPTIHandle[DeviceNb], ErrCode));
        }
        TermParams.ForceTermination = FALSE;
        ErrCode = STPTI_Term(STPTIDevice[DeviceNb], &TermParams);
        if (ErrCode == ST_ERROR_DEVICE_BUSY)
        {
            STTBX_Print(("Warning !! Still open handle on STPTI device %s\n", STPTIDevice[DeviceNb]));
            TermParams.ForceTermination = TRUE;
            ErrCode = STPTI_Term(STPTIDevice[DeviceNb], &TermParams);
        }
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("STPTI device %s Termination. error 0x%x ! \n", STPTIDevice[DeviceNb], ErrCode));
        }
        DeviceNb++;
    }
    InitDevNumber = 0;
#endif
} /* end of PTI_TermComponent() */

/*-------------------------------------------------------------------------
 * Function : PTI_MacroInit
 *            Register the PTI commands
 * Input    :
 * Output   :
 * Return   : TRUE if ok
 * ----------------------------------------------------------------------*/
static BOOL PTI_InitCommand (void)
{
    BOOL RetErr;
    RetErr = FALSE;
    RetErr |= STTST_RegisterCommand( "PTI_AudStartPID", PTI_AudioPID,
                                     "<PID> Start sending audio PID to audio decoder");
    RetErr |= STTST_RegisterCommand( "PTI_AudStopPID",  PTI_AudioStopPID,
                                     "Stop sending audio to audio decoder");
#if defined(ST_7015) || defined (ST_7020)
    RetErr |= STTST_RegisterCommand( "PTI_VidStartPID", PTI_VideoPID,
                                     "<PID> <Fifo#> Start sending video PID to video decoder\n\t\tFifo#=1,2,...or 5 (Fifo N is attached to video decoder N)");
    RetErr |= STTST_RegisterCommand( "PTI_VidStopPID",  PTI_VideoStopPID,
                                     "<Fifo#> Stop sending video to video decoder");
#else
    RetErr |= STTST_RegisterCommand( "PTI_VidStartPID", PTI_VideoPID,
                                     "<PID> Start sending video PID to video decoder");
    RetErr |= STTST_RegisterCommand( "PTI_VidStopPID",  PTI_VideoStopPID,
                                     "Stop sending video to video decoder");
#endif
    RetErr |= STTST_RegisterCommand( "PTI_PCRPID",      PTI_PCRPID,
                                     "<PID> Start sending data PCR PID");

    /* Constants */
    RetErr |= STTST_AssignString("PTI_NAME", STPTI_DEVICE_NAME, TRUE);


    return(RetErr ? FALSE : TRUE);

} /* end of PTI_MacroInit() */



/*##########################################################################*/
/*########################### EXPORTED FUNCTIONS ###########################*/
/*##########################################################################*/

 /*-------------------------------------------------------------------------
 * Function : PTI_InitComponent
 *            Initialization of PTI and its slots
 * Input    :
 * Output   :
 * Return   : TRUE if ok
 * ----------------------------------------------------------------------*/
BOOL PTI_InitComponent(void)
{
    BOOL RetOk;

    CurrentVideoFifoNb = 1;

    RetOk = PTI_Init();

    return(RetOk);

} /* end of PTI_InitComponent() */

 /*-------------------------------------------------------------------------
 * Function : PTI_CmdStart
 *            Register the commands
 * Input    :
 * Output   :
 * Return   : TRUE if success, FALSE if error
 * ----------------------------------------------------------------------*/

BOOL PTI_RegisterCmd()
{
    BOOL RetOk;

    RetOk = PTI_InitCommand();

    if ( RetOk )
    {
        STTBX_Print(( "PTI_InitCommand() \t: ok\n"));
    }
    else
    {
        STTBX_Print(( "PTI_InitCommand() \t: failed !\n" ));
    }

    return(RetOk);
} /* end of PTI_CmdStart() */


