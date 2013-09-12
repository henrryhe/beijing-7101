/*****************************************************************************

File name   :  hdd.c

Description :  Contains the high level commands for HDD driver

COPYRIGHT (C) ST-Microelectronics 2005

Date               Modification                 Name
----               ------------                 ----
 02/11/00          Created                      FR and SB
 08/01/00          Adapted for STVID (STAPI)    AN
 11/09/01          Adapted for driver STATAPI   FQ
 10/01/02          Put HDD data in N cache area FQ
 06/02/02          Read HDD 64K by 64K bytes    FQ
*****************************************************************************/

/* Flag to use when debug with STTBX... functions. */
 /*#define HDD_TRACE_DEBUG*/

/* Flag to use when debug with uart spy. */
/*#define HDD_TRACE_UART*/
/*#define HDD_INJECTION_TRACE_UART*/

/* Flag to save the content of the PES injected to a file */
/*#define SAVE_TO_DISK*/
/* Includes ----------------------------------------------------------------- */
#if defined (HDD_TRACE_UART) || defined(HDD_INJECTION_TRACE_UART)
#include "trace.h"
#ifdef STVID_TRACE_BUFFER_UART
#if (STVID_TRACE_BUFFER_UART==2)
#define TraceDebug(x) TraceMessage x
#define TraceGraph(x) TraceVal x
#endif  /* STVID_TRACE_BUFFER_UART==2 */
#endif /*STVID_TRACE_BUFFER_UART*/
#else
#define TraceBuffer(x)
#endif

#include <string.h>
#include <stdlib.h>

#include "testcfg.h"

#include "stdevice.h"
#include "stevt.h"
#include "stvid.h"
#include "stavmem.h"
#include "stddefs.h"
#include "sttbx.h"
#include "stcommon.h"
#include "testtool.h"
#include "swconfig.h"
#include "startup.h"

#ifdef DVD_TRANSPORT_STPTI
#include "stpti.h"
#else
#include "pti.h"
#endif
#if defined(USE_FILESYSTEM)
#include "generic_fileio.h"
#endif
#include "hdd.h"
#include "stsys.h"
#include "clevt.h"
#if defined(mb282b)
#include "stfpga.h"
#endif
#if defined(ST_5516) || defined(ST_5517)
#include "stcfg.h"
#endif
#include "vid_inj.h"
#include "vid_util.h"

#if defined(mb411) || defined(mb519)
#ifndef DONT_USE_FDMA
#define DONT_USE_FDMA
#endif
#if !defined ST_OSLINUX
#ifndef DU8
#pragma ST_device(DU8)
typedef volatile unsigned char DU8;
#endif
#ifndef DU16
#pragma ST_device(DU16)
typedef volatile unsigned short DU16;
#endif
#ifndef DU32
#pragma ST_device(DU32)
typedef volatile unsigned int DU32;
#endif
#endif /* !LINUX */
#endif /* defined(mb411) || defined(mb519) */

/* Compilation options...---------------------------------------------------- */
#define WA_USEDRIFTTIMENOTTIMEJUMP
/* STATAPI_OLD_FPGA for 5512 */
/* Private Constants -------------------------------------------------------- */

/* For debugging(saving the injected PES to a file) */
#ifdef SAVE_TO_DISK
#define PES_FILE_NAME       "hdd.dat"
    volatile long int HDDFileDescriptor;
#endif /* SAVE_TO_DISK */

/* Devices */

/* Hardware data */
/* Units used for the user point of vue: 1 LBA = 16 sectors=16*512 bytes */
/* Units used by STATAPI                 1 LBA = 1 sectors = 512 bytes   */
/* Units used in macros                  1 Block = 16 LBA = 16*512 bytes   */

#if defined(ST_7020)
#define NBSECTORSTOINJECTATONCE         16 /* 16 *SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK = 128 KBytes at once  */
#else
#define NBSECTORSTOINJECTATONCE         8 /* 8 *SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK = 64 KBytes at once  */
#endif
#define SECTOR_SIZE                     512
#define HDD_NB_OF_SECTORS_PER_BLOCK     16
#define HDD_MIN_NUM_OF_BLOCK            1
#define HDD_MAX_BLOCK_TO_TRANSFERT      HDD_NB_OF_SECTORS_PER_BLOCK
#define HDD_SIZE_OF_PICTURE_START_CODE  8 /* Nb of byte for picture start code + temporal ref and Picture type + 2 bytes to keep aligned on 4 bytes for 5514 */
#define MS_TO_NEXT_TRANSFER             (3*2)
#define HDD_MAX_FILENAME_LENGTH			128

/* Private Types ------------------------------------------------------------ */

/* Structure of the information concerning a I Picture */
typedef struct
{
    U32 IPicturePosition;   /* Position of the I Picture. */
    U32 NbPictureToLastI;   /* Number of picture to reach the next I Picture. */
} HDD_IPictureCharacteristic_t;

/* Structure of the first block of the zone containing a file. */
typedef struct
{
    U32 SizeOfFile;
    U32 NumberOfIPicture;
    HDD_IPictureCharacteristic_t TabIPicturePosition[((SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK)/sizeof(U32))-2];
} HDD_FirstBlock_t;

/* Private Variables ------------------------------------------------------------ */
static ST_Partition_t *HDD_CPUPartition_p;
static ST_Partition_t *HDD_NCachePartition_p;

static void    *HDD_TaskStack;
static task_t  *HDD_TaskId;
static tdesc_t HDD_TaskDesc;

static STHDD_Handle_t HDD_Handle;         /* Internal parameters of the HDD process. */

/******************************************************************************************/
/************************************ SPEED MANAGEMENT ************************************/
/******************************************************************************************/

static S32  GlobalCurrentSpeed;       /* Current speed wanted by the application. */
static S32  PreviousCurrentBlock;
static S32  PreviousBlocksToRead;

/******************************************************************************************/
/************************************* EVENT  MANAGER *************************************/
/******************************************************************************************/

static STEVT_Handle_t  EvtHandle;  /* handle for the subscriber */

/* Pointer to the copy in memory of the first block stored in HDD for a file. */
static HDD_FirstBlock_t *HDDFirstBlockBuffer_Alloc_p, *HDDFirstBlockBuffer_p;

static U32  HDD_PositionOfIPictureDisplayed;            /* Position of the I Picture displayed during backward */
static BOOL HDD_PositionOfIPictureDisplayedAvailable;   /* Flag to indicate the I Position is available or not. */
static S32  HDD_PreviousDriftTime;

/* Global Variables --------------------------------------------------------- */
extern VID_Injection_t VID_Injection[VIDEO_MAX_DEVICE];

static U32 HDDNbOfVideoUnderflowEvent;          /* statistics : nb of events */
static U32 HDDNbOfTreatedVideoUnderflowEvent;   /* statistics : nb of treated events */
static U32 HDDNbOfVideoDriftThresholdEvent;     /* statistics : nb of events */
static U32 HDDNbOfLBARead;                      /* statistics : LBA count */
static U32 HDDNbOfTemporalDiscontinuity;        /* statistics : nb of discont. */
static U32 HDDNbOfRelatedToPrevious;            /* statistics : nb of Rel. to Prev. */
static U32 HDDBitratesSum;                      /* statistics : sum of all the bitrates */
static U32 HDDNbBitrateZero;                    /* statistics : nb of bitrates equal to zero */
static U32 HDDMaxBitrate;                       /* statistics : max bitrate */
static U32 HDDMinBitrate;                       /* statistics : min bitrate */
static U32 HDDAverageBitrate;                   /* statistics : average bitrate */
static U32 HDDNbOfForwardUnderflow;             /* statistics : nb of forward underflow requests */
static U32 HDDNbOfBackwardUnderflow;            /* statistics : nb of backward underflow requests */

static U32 ShiftInjectionCoef = 0;              /* If different from 0, it means that in backward play
                                                 * we have to shift each injection LBA start position
                                                 * with a size equal to ShiftInjectionCoef%, in order to have
                                                 * a bigger overlap than the one requested by the driver */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

static BOOL     HDD_TrickmodeEventManagerInit (void);
static void     HDD_TrickmodeEventManagerTask (void);

static BOOL     HDD_PlayEvent(STTST_Parse_t * Parse_p, char *result);
static BOOL     HDD_StopEvent(STTST_Parse_t * Parse_p, char *result);
static BOOL     HDD_SetSpeed (STTST_Parse_t * Parse_p, char *result);
static BOOL     HDD_SetFile (STTST_Parse_t * Parse_p, char *result);
static BOOL     HDD_UnSetFile (STTST_Parse_t * Parse_p, char *result);

static BOOL     HDD_ReadBlocks(char *ReadBuffer_p, U32 BufferSize);
static void     HDD_InjectBlocks(char *ReadBuffer_p, U32 BufferSize);
static BOOL     HDD_ReadAndInjectBlocks(char *ReadBuffer_p, U32 LBAPositionToRead, U32 PartNumOfBlockToRead);

#if defined (mb282b) || defined (mb275) || defined (mb275_64) || defined (mb314) || defined (mb290) || defined (mb361) || defined (mb382) || defined (mb376) || defined (espresso) || defined(mb390) || defined(mb391) || defined (mb411) || defined(mb519) || defined(mb634)
/* static void     HDD_TrickModeCallBack ( STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, STEVT_EventConstant_t Event, const void *EventData, const void *SubscriberData_p); */
static void     HDD_TrickModeCallBack ( STEVT_CallReason_t Reason, STEVT_EventConstant_t Event, const void *EventData);
#endif

static void     HDD_BlockJumpManagement (S32 *CurrentBlock, S32 StartBlock, S32 EndBlock,
                                         S32 CurrentSpeed, S32 *BlocksToRead,
                                         S32 JumpToPerform, S32 *NumberOfPlaying,
                                         BOOL *Discontinuity, BOOL *RelatedToPrevious);
static U32      HDD_SeekLastIPictureInFile (U32 CurrentBlockRead, HDD_FirstBlock_t*);

#if defined(ST_5516) || defined(ST_5517) || defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined(ST_7109)
#if 0
static ST_ErrorCode_t DoCfg(void);
#endif
#endif
    /* Extern Functions --------------------------------------------------------- */
#if defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined (ST_7109) || defined(ST_7200)
static ST_ErrorCode_t GetWriteAddress(void * const Handle, void ** const Address_p);
static void InformReadAddress(void * const Handle, void * const Address);
#endif
extern void VID_ReportInjectionAlreadyOccurs(U32 CDFifoNb);


#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_51xx_Device) || defined(ST_7710)
/*-------------------------------------------------------------------------
 * Function : GetWriteAddress of input buffer
 *
 * Input    :
 * Parameter:
 * Output   :
 * Return   : ST_NO_ERROR or ST_ERROR_BAD_PARAMETER
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t GetWriteAddress(void * const Handle, void ** const Address_p)
{
#ifdef ST_OSLINUX

    *Address_p = VID_Injection[(U32)Handle].Write_p;
    return(ST_NO_ERROR);

#else

    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    /* registered handle = index in cd-fifo array */
    if (STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping) == ST_NO_ERROR)
    {
        * Address_p = STAVMEM_VirtualToDevice(VID_Injection[(U32)Handle].Write_p, &VirtualMapping);
        return(ST_NO_ERROR);
    }
    return(ST_ERROR_BAD_PARAMETER);
#endif
}

/*-------------------------------------------------------------------------
 * Function : InformReadAddress of input buffer
 *
 * Input    :
 * Parameter:
 * Output   :
 * Return   : none
 * ----------------------------------------------------------------------*/
void InformReadAddress(void * const Handle, void * const Address_p)
{
#ifdef ST_OSLINUX
    VID_Injection[(U32)Handle].Read_p = Address_p;
#else
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    /* registered handle = index in cd-fifo array */
    if (STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping) == ST_NO_ERROR)
    {
        VID_Injection[(U32)Handle].Read_p = STAVMEM_DeviceToVirtual(Address_p, &VirtualMapping);
    }
#endif
}
#endif  /* 7100 ... */

/*******************************************************************************
Name        : HDD_Init
Description : Initialization of HDD device
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_ else
*******************************************************************************/
ST_ErrorCode_t  HDD_Init (void)
{
    ST_ErrorCode_t ErrCode;

    /* Init file handle */
    HDD_Handle.FileHandle = NULL;
    HDD_Handle.FileSize   = 0;

    /* --- Starts Trickmode Management --- */
    ErrCode = HDD_TrickmodeEventManagerInit();
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(("Trickmode Process Manager init. failed !\n"));
    }

    /* --- Set the default current speed --- */

    GlobalCurrentSpeed = 100;

    return (!ErrCode);
} /* end of HDD_Init() */

/*******************************************************************************
Name        : HDD_Term
Description : Termination of HDD device
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t  HDD_Term (void)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR ;

    if (HDD_Handle.FileHandle != NULL)
    {
        fclose(HDD_Handle.FileHandle);
    }

    return (!ErrCode);
} /* end of HDD_Term() */


/*******************************************************************************
Name        : HDD_PlayEvent
Description : Starts waiting for video's event to read data from HDD.
Parameters  : None
Assumptions :
Limitations :
Returns     : FALSE if no error, else TRUE
*******************************************************************************/
static BOOL     HDD_PlayEvent(STTST_Parse_t * Parse_p, char *result)
{
#if defined (mb282b) || defined (mb275) || defined (mb275_64) || defined (mb314) || defined (mb290) || defined (mb361) || defined (mb382) || defined (mb376) || defined (espresso) || defined(mb390) || defined(mb391) || defined (mb411) || defined(mb519) || defined(mb634)
    BOOL RetErr;
    ST_ErrorCode_t                          ErrorCode;
    STEVT_SubscribeParams_t                 HddSubscribeParams;
    STVID_DataInjectionCompletedParams_t    DataInjectionCompletedParams;

    /* STEVT_DeviceSubscribeParams_t   HddSubscribeParams; */

    VID_Inj_WaitAccess(0);
    if(VID_Inj_GetInjectionType(0) != NO_INJECTION)
    {
        VID_ReportInjectionAlreadyOccurs(1);
        VID_Inj_SignalAccess(0);
        STTBX_Print(("HDD_PlayEvent() : command cancelled !\n"));
        return(TRUE);
    }
#if defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined (ST_7109) || defined(ST_7200)
    else
    {
        U32 InputBuffersize;
        /* Get Input buffer characteristics     */
        ErrorCode = STVID_GetDataInputBufferParams( VID_Inj_GetDriverHandle(0),
            &VID_Injection[0].Base_p, &InputBuffersize);
        if ( ErrorCode!= ST_NO_ERROR )
        {
            STTBX_Print(("HDD_PlayEvent() : STVID_GetDataInputBufferParams() error %d !\n", ErrorCode));
            VID_Inj_SignalAccess(0);
            return(TRUE);
        }

#if defined ST_OSLINUX && !defined STVID_MMAP_PESBUFFERS_IN_USERWRAPPER
        ErrorCode = STVID_MapPESBufferToUserSpace(VID_Inj_GetDriverHandle(0), &VID_Injection[0].Base_p, InputBuffersize);
        if ( ErrorCode!= ST_NO_ERROR )
        {
            STTBX_Print(("HDD_PlayEvent() : STVID_MapPESBufferToUserSpace() error %d !\n", ErrorCode));
            VID_Inj_SignalAccess(0);
            return(TRUE);
        }
#endif

        /* Update injection pointer.            */
        VID_Injection[0].Top_p  = (void *)((U32)VID_Injection[0].Base_p + InputBuffersize - 1);
        /* Align the write and read pointer to the beginning of the input buffer */
        VID_Injection[0].Write_p = VID_Injection[0].Base_p;
        VID_Injection[0].Read_p  = VID_Injection[0].Base_p;

        /* Configure the interface-object       */
        ErrorCode = STVID_SetDataInputInterface(
                                VID_Inj_GetDriverHandle(0),
                                GetWriteAddress,InformReadAddress,(void*)(0));
        if ( ErrorCode!= ST_NO_ERROR )
        {
            STTBX_Print(("HDD_PlayEvent() : STVID_SetDataInputInterface() error %d !\n", ErrorCode));
            VID_Inj_SignalAccess(0);
            return(TRUE);
        }
        VID_Inj_SetInjectionType(0, VID_HDD_INJECTION);

        /* Video Load Buffer in use */
        /* (TODO) VID_BufferLoad[BufferNb-1].UsedCounter++;*/
        /* Free injection access */
    }
#endif /* defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined (ST_7109) || defined(ST_7200) */
    VID_Inj_SignalAccess(0);


    if ( HDD_Handle.ProcessRunning )
    {
        /* The HDD read-out process is already running */
        STTBX_Print(("HDD_PlayEvent() : process already running !\n"));
        return(FALSE);
    }

    /* --- Get input arguments --- */

    RetErr = STTST_GetInteger ( Parse_p , 0 , (S32 *)&HDD_Handle.LBANumber );
    if (RetErr )
    {
        STTST_TagCurrentLine ( Parse_p , "Invalid LBA number selected" );
        return ( TRUE );
    }

    RetErr = STTST_GetInteger ( Parse_p , 0 , &HDD_Handle.NumberOfPlay );
    if (RetErr )
    {
        STTST_TagCurrentLine ( Parse_p , "Invalid number of play (1, 2, 3... or 0=infinite)" );
        return ( TRUE );
    }
    if (HDD_Handle.NumberOfPlay > 0)
    {
        HDD_Handle.InfinitePlaying = FALSE;
    }
    else
    {
        HDD_Handle.InfinitePlaying = TRUE;
    }
    STTBX_Print(("HDD_PlayEvent() : LBA=%d NumberOfPlay=%d InfinitePlaying=%d\n",
        HDD_Handle.LBANumber, HDD_Handle.NumberOfPlay, HDD_Handle.InfinitePlaying));

    /* --- Read the file header --- */

    HDDFirstBlockBuffer_Alloc_p = (HDD_FirstBlock_t *) STOS_MemoryAllocate ( NCachePartition_p, (SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK) + 255 );
    HDDFirstBlockBuffer_p = (HDD_FirstBlock_t *)(((U32)HDDFirstBlockBuffer_Alloc_p + 255) & 0xffffff00);
    if ( HDDFirstBlockBuffer_Alloc_p == NULL )
    {
        STTBX_Print(("HDD_PlayEvent() : Unable to allocate in memory %d bytes(2)\n", (SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK) ));
        HDDFirstBlockBuffer_p = NULL;
        STTST_AssignInteger(result, TRUE, FALSE);
        return(TRUE);
    }

    /* Read-out of the first block of the stream */

    if (HDD_ReadBlocks((char *)HDDFirstBlockBuffer_p, (SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK)) != ST_NO_ERROR)
    {
        return FALSE;
    }
    /* --- Prepare HDD Handle --- */

    HDD_Handle.LBANumber ++;
    HDD_Handle.LBAStart = HDD_Handle.LBANumber;
    /* Retreive of the size of the file : */
#ifdef ST_5512
    if (HDDFirstBlockBuffer_p->NumberOfIPicture != 0)
    {
        HDD_Handle.LBAEnd =
            HDDFirstBlockBuffer_p->TabIPicturePosition[HDDFirstBlockBuffer_p->NumberOfIPicture].IPicturePosition;
    }
    else
#endif
    {
        HDD_Handle.LBAEnd =
            HDD_Handle.LBAStart + ((HDD_Handle.FileSize-1)/(SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK))+1;
    }

#ifdef HDD_TRACE_UART
    TraceBuffer(("\r\n------ HDD_PlayEvent() : LBAStart=%d LBAEnd=%d, NumberOfPlay=%d\r\n",
                HDD_Handle.LBAStart, HDD_Handle.LBAEnd, HDD_Handle.NumberOfPlay));
#endif

    HDD_PositionOfIPictureDisplayed = 0;
    HDD_PositionOfIPictureDisplayedAvailable = FALSE;
    HDD_Handle.UnderFlowEventOccured            = FALSE;
    HDD_Handle.SpeedDriftThresholdEventOccured  = FALSE;
    HDD_PreviousDriftTime = 0;
    HDD_Handle.ProcessRunning = TRUE;

    /* --- Subscribe to trickmode events --- */

    /* Specific call-back function */
    HddSubscribeParams.NotifyCallback       = HDD_TrickModeCallBack;

    ErrorCode =  STEVT_Subscribe (EvtHandle, STVID_DATA_UNDERFLOW_EVT,       &HddSubscribeParams);
    ErrorCode |=  STEVT_Subscribe (EvtHandle, STVID_DATA_OVERFLOW_EVT,        &HddSubscribeParams);
    ErrorCode |=  STEVT_Subscribe (EvtHandle, STVID_SPEED_DRIFT_THRESHOLD_EVT,&HddSubscribeParams);
    if ( ErrorCode!= ST_NO_ERROR )
    {
        STTBX_Print(("HDD_PlayEvent() : STEVT_Subscribe() error %d !\n", ErrorCode));
        RetErr = TRUE;
    }

    /* Do DataInjectionCompleted() to be sure to received a first UNderflow event, because API says
       "The event Underflow is notified once when the bit buffer level is reaching the lower threshold,
       then it is not sent again unless the bit buffer level goes over the upper threshold,
       or unless the function STVID_DataInjectionCompleted() is called." */

    PreviousCurrentBlock = 0x7FFFFFFF;
    PreviousBlocksToRead = 0;
    DataInjectionCompletedParams.TransferRelatedToPrevious = FALSE;
    ErrorCode = STVID_DataInjectionCompleted( VID_Inj_GetDriverHandle(0), &DataInjectionCompletedParams);
    if ( ErrorCode!= ST_NO_ERROR )
    {
        STTBX_Print(("HDD_PlayEvent() : STVID_DataInjectionCompleted() error %d !\n", ErrorCode));
        RetErr = TRUE;
    }
#ifdef HDD_TRACE_UART
    TraceBuffer(("\r\nHDD_PlayEvent() : DataInjectionCompleted(%d) done",DataInjectionCompletedParams.TransferRelatedToPrevious));
#endif
    STTST_AssignInteger(result, ErrorCode, FALSE);

    return ( RetErr );

#else
    return(TRUE);
#endif
} /* end of HDD_PlayEvent() */

/*******************************************************************************
Name        : HDD_StopEvent
Description : Stops the event management while trick mode process.
Parameters  : None
Assumptions :
Limitations :
Returns     : FALSE if no error, else TRUE
*******************************************************************************/
static BOOL     HDD_StopEvent (STTST_Parse_t * Parse_p, char *result)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    BOOL RetErr;
    U32 TimeOut = 10000;

    RetErr = FALSE;
    UNUSED_PARAMETER(Parse_p);

    VID_Inj_WaitAccess(0);
#if defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined (ST_7109) || defined(ST_7200)
    ErrCode = STVID_DeleteDataInputInterface( VID_Inj_GetDriverHandle(0) );
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(("HDD_StopEvent() : STVID_DeleteDataInputInterface() error %d !\n", ErrCode));
        RetErr = TRUE;
    }

#if defined ST_OSLINUX && !defined STVID_MMAP_PESBUFFERS_IN_USERWRAPPER
    ErrCode = STVID_UnmapPESBufferFromUserSpace(VID_Inj_GetDriverHandle(0));
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(("HDD_StopEvent() : STVID_UnmapPESBufferFromUserSpace() error %d !\n", ErrCode));
        RetErr = TRUE;
    }
#endif

#endif /* defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined (ST_7109) || defined(ST_7200) */
    VID_Inj_SetInjectionType(0, NO_INJECTION);
    VID_Inj_SignalAccess(0);
    /* Test if the HDD read-out process is running */
    if ( HDD_Handle.ProcessRunning )
    {
        ErrCode |=  STEVT_Unsubscribe (EvtHandle, (U32)STVID_DATA_UNDERFLOW_EVT);
        ErrCode |=  STEVT_Unsubscribe (EvtHandle, (U32)STVID_DATA_OVERFLOW_EVT);
        ErrCode |=  STEVT_Unsubscribe (EvtHandle, (U32)STVID_SPEED_DRIFT_THRESHOLD_EVT);
        if ( ErrCode!= ST_NO_ERROR )
        {
            STTBX_Print(("HDD_StopEvent() : STEVT_Unsubscribe(), error %d !\n", ErrCode));
            RetErr = TRUE;
        }
        else
        {
            STTBX_Print(("HDD_StopEvent() : STEVT_Unsubscribe() well done\n"));
        }

        while ( (!(HDD_Handle.EndOfLoopReached)) && ( TimeOut >0 ) )
        {
            task_delay ( 1 );
            TimeOut --;
        }
        STTBX_Print(("HDD_StopEvent() : playback stopped around LBANumber=%d NumberOfPlay=%d\n",
                HDD_Handle.LBANumber, HDD_Handle.NumberOfPlay ));

        /* De-allocation of HDDFirstBlockBuffer_p */
        if(HDDFirstBlockBuffer_Alloc_p != NULL)
        {
            STOS_MemoryDeallocate( NCachePartition_p, HDDFirstBlockBuffer_Alloc_p );
            HDDFirstBlockBuffer_Alloc_p = NULL;
            HDDFirstBlockBuffer_p = NULL;
        }
        HDD_Handle.ProcessRunning = FALSE;
    }
#if !(defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined (ST_7109)) || defined(ST_7200)
    else
    {
        STTBX_Print(("%s: do nothing because process not running\n", __FUNCTION__));
    }
#endif /* !(defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined (ST_7109)) || defined(ST_7200) */
#ifdef HDD_TRACE_UART
    TraceBuffer(("\r\n------ HDD_StopEvent() : ProcessRunning=%d EndOfLoopReached=%d NumberOfPlay=%d\r\n",
                HDD_Handle.ProcessRunning, HDD_Handle.EndOfLoopReached, HDD_Handle.NumberOfPlay));
#endif
    STTST_AssignInteger(result, FALSE, FALSE);

    return (RetErr);

} /* end of HDD_StopEvent() */

/*******************************************************************************
Name        : HDD_SetSpeed
Description : Initialization of the current speed in the driver.
Parameters  : None
Assumptions :
Limitations :
Returns     : FALSE if no error, else TRUE
*******************************************************************************/
static BOOL     HDD_SetSpeed (STTST_Parse_t * Parse_p, char *result)
{
    S32     Speed;         /* Speed wanted by the application */
    BOOL    Error;         /* step by step error flag */

    /* Get the the reference of sector parameter */
    Error = STTST_GetInteger ( Parse_p , 0 , &Speed );
    if ( Error )
    {
        STTST_TagCurrentLine ( Parse_p , "Invalid speed parameter" );
    }
    else
    {
        if (Speed >= 0)
        {
            HDD_PositionOfIPictureDisplayedAvailable = FALSE;

            if (GlobalCurrentSpeed < 0)
            {
                if (HDD_PositionOfIPictureDisplayed >= HDDFirstBlockBuffer_p->NumberOfIPicture-1)
                {
                    HDD_Handle.LBANumber = HDD_Handle.LBAStart;
                }
                else
                {
                    HDD_Handle.LBANumber =
                            HDDFirstBlockBuffer_p->TabIPicturePosition[HDD_PositionOfIPictureDisplayed+1].IPicturePosition;
                }
            }
        }

        /* Save locally the speed of the system */
        GlobalCurrentSpeed = Speed;
    }
    STTST_AssignInteger(result, Error, FALSE);

    return(Error);

} /* end of HDD_SetSpeed */


/*******************************************************************************
Name        : HDD_TrickmodeEventManagerInit
Description : Initializes the Trickmode event manager.
              This process manages the events that occur from the video driver
              (trickmode feature), and perform the feeding of the CD FIFO via
              a pti_video_dma_inject(..) function call from the hard disk.
Parameters  : None
Assumptions :
Limitations :
Returns     : FALSE if no error occured, and TRUE otherwise.
*******************************************************************************/
static BOOL     HDD_TrickmodeEventManagerInit ( void )
{

    ST_ErrorCode_t          ErrCode = ST_NO_ERROR;
    STEVT_OpenParams_t      OpenParams;

#ifdef SAVE_TO_DISK
    HDDFileDescriptor = debugopen(PES_FILE_NAME, "wb" );
#endif /* SAVE_TO_DISK */

    /* Initialize the counting semaphore with the value zero */
    HDD_Handle.TrickModeDriverRequest_p = STOS_SemaphoreCreateFifo(NULL, 0);
    ErrCode = STOS_TaskCreate((void (*)(void *)) HDD_TrickmodeEventManagerTask,
                                                 NULL,
                                                 HDD_CPUPartition_p,
                                                 HDD_TASK_SIZE,
                                                 &HDD_TaskStack,
                                                 HDD_CPUPartition_p,
                                                 &HDD_TaskId,
                                                 &HDD_TaskDesc,
                                                 HDD_TASK_PRIORITY,
                                                 "hdd_TrickMode_evt_manager",
                                                 task_flags_no_min_stack_size );
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(("HDD_TrickmodeEventManagerInit () : Unable to create the HDD Trickmode event manager\n"));
        return ( TRUE );
    }

    ErrCode = STEVT_Open(STEVT_DEVICE_NAME, &OpenParams, &EvtHandle);
    if ( ErrCode!= ST_NO_ERROR )
    {
        STTBX_Print(("HDD_TrickmodeEventManagerInit() : STEVT_Open() error %d for subscriber !\n", ErrCode));
        STOS_SemaphoreDelete(NULL, HDD_Handle.TrickModeDriverRequest_p);
        STOS_TaskDelete(HDD_TaskId,
                        HDD_CPUPartition_p,
                        HDD_TaskStack,
                        HDD_CPUPartition_p);
        return ( TRUE );
    }
    else
    {
        STTBX_Print(("HDD_TrickmodeEventManagerInit() : STEVT_Open() for subscriber done\n"));
    }
    return ( FALSE );

} /* end of HDD_TrickmodeEventManagerInit() */


/*******************************************************************************
Name        : HDD_TrickmodeEventManagerTask
Description : Process that keeps running, and perform playback task
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
static void    HDD_TrickmodeEventManagerTask ( void )
{
    ST_ErrorCode_t                          ErrCode = ST_NO_ERROR;
    S32                                     NumOfBlockToRead;       /* Total Number of block to read */
    S32                                     NumOfBlockToSkip;       /* Total number of block to skip */
    S32                                     RealSpeed;              /* Current Speed of the video driver. */
    STEVT_EventConstant_t                   TrickModeEvent = 0;
    STVID_DataInjectionCompletedParams_t    DataInjectionCompletedParams;
    STVID_DataUnderflow_t                   UnderFlowEventData;
    STVID_SpeedDriftThreshold_t             SpeedDriftThresholdData;
    char                *HDDReadBuffer_Alloc_p, *HDDReadBuffer_p;
    BOOL                IsTemporalDiscontinuity ;  /* true when temporal discontinuity with previous injection */
    BOOL                Error;
    BOOL                DoSkip = FALSE;
    U32                 LastLBANumber = 0xFFFFFFFF;
    U32                 PatchedOverlap;
    U32                 BackwardOverlapSize;

#ifdef HDD_TRACE_UART
    /* Variable to measure time. */
    clock_t InjectStartTime;
    U32 TickPerSecond;
    /* Get the time base */
#if !defined(ST_7100)
    TickPerSecond = ST_GetClocksPerSecond();
#endif /*!ST_7100*/
#endif

    STOS_TaskEnter(NULL);

    Error = FALSE;
    STTBX_Print(("HDD_TrickmodeEventManagerTask() : task started, ready to read HDD\n"));

    HDDReadBuffer_Alloc_p = STOS_MemoryAllocate ( NCachePartition_p, (NBSECTORSTOINJECTATONCE*SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK) + 255 );
    /* Put the pointer at an address multipe of 255 (memory alignment) */
    HDDReadBuffer_p = (char *)(((U32)HDDReadBuffer_Alloc_p + 255) & 0xffffff00);
    if ( HDDReadBuffer_p == NULL )
    {
        STTBX_Print(("Unable to allocate %d bytes of memory(6)\n", (SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK) ));
        HDDReadBuffer_p = NULL;

        STOS_TaskExit(NULL);
        return;
    }

    while (1)
    {

        /* --- Stand-by : wait for a video event signaled by HDD_TrickModeCallBack() --- */

        STOS_SemaphoreWait(HDD_Handle.TrickModeDriverRequest_p );

        /* --- Event received --- */

        HDD_Handle.EndOfLoopReached = FALSE;

        STOS_InterruptLock();
        if (HDD_Handle.SpeedDriftThresholdEventOccured )
        {
            TrickModeEvent = STVID_SPEED_DRIFT_THRESHOLD_EVT;
        }
        else if (HDD_Handle.UnderFlowEventOccured)
        {
            TrickModeEvent = STVID_DATA_UNDERFLOW_EVT;
        }
        STOS_InterruptUnlock();

        if ( ( (HDD_Handle.NumberOfPlay != 0) || (HDD_Handle.InfinitePlaying) )
             && (HDD_Handle.ProcessRunning) )
        {

            /* --- Reading&injection required & events management enabled --- */

            switch (TrickModeEvent)
            {
                case STVID_SPEED_DRIFT_THRESHOLD_EVT :

                    /* Get the data of this event */
                    STOS_memcpy(&SpeedDriftThresholdData, &HDD_Handle.SpeedDriftThresholdData, sizeof (STVID_SpeedDriftThreshold_t));

                    /* indicate the event has been used by this process. */
                    STOS_InterruptLock();
                    HDD_Handle.SpeedDriftThresholdEventOccured = FALSE;
                    STOS_InterruptUnlock();

                    /*  Available fields for this parameter  :              */
                    /*    - S32 DriftTime;                                  */
                    /*    - U32 BitRateValue; in units of 400 bits/second   */
                    /*    - U32 SpeedRatio;                                 */

#ifdef HDD_TRACE_UART
                    TraceBuffer(("\r\nDRIFT_THRESHOLD request : DriftTime=%d BitRate=%d BlockToSkip=%d ",
                                 SpeedDriftThresholdData.DriftTime, SpeedDriftThresholdData.BitRateValue,
                                 HDD_Handle.NumberOfBlockToSkip));
#endif

                    /* Check for "nothing to do" cases */
                    if ( (!SpeedDriftThresholdData.DriftTime) || (!SpeedDriftThresholdData.BitRateValue) )
                    {
#ifdef HDD_TRACE_UART
                        TraceBuffer(("\r\n!!!!! invalid DRIFT_THRESHOLD data\r\n"));
#endif
                        STTBX_Print(("HDD_TrickmodeEventManagerTask() invalid STVID_SPEED_DRIFT_THRESHOLD_EVT data\n"));
                        break;
                    }

                    SpeedDriftThresholdData.DriftTime += HDD_PreviousDriftTime;

                    /* Calculate the asked jump that will be performed on next STVID_DATA_UNDERFLOW_EVT event
                     * (based on BitRateValue field) */
					HDD_Handle.NumberOfBlockToSkip =
                        (S32) ( ((SpeedDriftThresholdData.DriftTime/100) * (S32)(SpeedDriftThresholdData.BitRateValue/HDD_NB_OF_SECTORS_PER_BLOCK)) )/(18*(SECTOR_SIZE));

                    if (HDD_Handle.NumberOfBlockToSkip == 0)
                    {
                        DoSkip = FALSE;
                        /* The DriftTime is not big enough to perform a jump action. */
                        /* I store it for the next STVID_SPEED_DRIFT_THRESHOLD_EVT event occurrence */
                        HDD_PreviousDriftTime = SpeedDriftThresholdData.DriftTime;
                    }
                    else
                    {
                        DoSkip = TRUE;
                        HDD_PreviousDriftTime = SpeedDriftThresholdData.DriftTime -
                            (S32)((1800*SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK/(S32)(SpeedDriftThresholdData.BitRateValue))
                                  *(S32)HDD_Handle.NumberOfBlockToSkip) ;
                    }
                    break;

                case STVID_DATA_UNDERFLOW_EVT :

                    /* Get the data of this event */
                    STOS_memcpy( &UnderFlowEventData, &HDD_Handle.UnderflowEventData, sizeof (STVID_DataUnderflow_t));

                    /* indicate the event has been used by this process. */
                    STOS_InterruptLock();
                    HDD_Handle.UnderFlowEventOccured = FALSE;
                    STOS_InterruptUnlock();

                    /* Statistics */
                    HDDNbOfTreatedVideoUnderflowEvent++;
                    if(UnderFlowEventData.BitRateValue == 0)
                    {
                        HDDNbBitrateZero++;
                    }
                    else
                    {
                        if(UnderFlowEventData.BitRateValue > HDDMaxBitrate)
                        {
                            HDDMaxBitrate = UnderFlowEventData.BitRateValue;
                        }
                        if(UnderFlowEventData.BitRateValue < HDDMinBitrate)
                        {
                            HDDMinBitrate = UnderFlowEventData.BitRateValue;
                        }
                        HDDBitratesSum += UnderFlowEventData.BitRateValue;
                        HDDAverageBitrate = HDDBitratesSum / (HDDNbOfTreatedVideoUnderflowEvent - HDDNbBitrateZero);
                    }

                    /*  Available fields for this parameter :               */
                    /*    U32 BitBufferFreeSize;                            */
                    /*    U32 BitRateValue; in units of 400 bits/second     */
                    /*    S32 RequiredTimeJump;                             */
                    /*    U32 RequiredDuration;                             */

#ifdef HDD_TRACE_UART
                    TraceBuffer(("\r\nUNDERFLOW request : BBFree=%d BRate=%d TimeJ=%d Duration=%d",
                                 UnderFlowEventData.BitBufferFreeSize, UnderFlowEventData.BitRateValue,
                                 UnderFlowEventData.RequiredTimeJump, UnderFlowEventData.RequiredDuration));


                    if ( (UnderFlowEventData.RequiredTimeJump<0) &&
                         (-UnderFlowEventData.RequiredTimeJump==UnderFlowEventData.RequiredDuration))
                    {
                        TraceBuffer(("\r\n********** Negative jump equals required duration ! ***************"));
                        STTBX_Print(("\r\n********** Negative jump equals required duration ! ***************\n"));
                    }
#endif
                    /* Statistics */
                    if(UnderFlowEventData.RequiredTimeJump < 0)
                    {
                        HDDNbOfBackwardUnderflow++;
                    }
                    else
                    {
                        HDDNbOfForwardUnderflow++;
                    }

                    IsTemporalDiscontinuity = FALSE;

                    /* Check for "nothing to do" cases */
                    if (UnderFlowEventData.BitBufferFreeSize == 0)
                    {
                        STTBX_Print(("HDD_TrickmodeEventManagerTask(): Invalid STVID_DATA_UNDERFLOW_EVT data\n"));
                        break;
                    }

                    /* Get the speed value */
                    if (STVID_GetSpeed( VID_Inj_GetDriverHandle(0), &RealSpeed) != ST_NO_ERROR)
                    {
                        STTBX_Print(("HDD_TrickmodeEventManagerTask(): Can't get the current speed value\n"));
                        break;
                    }

#ifdef WA_USEDRIFTTIMENOTTIMEJUMP
                    /* --- Calculate the jump required by previous STVID_SPEED_DRIFT_THRESHOLD_EVT --- */

                    if (UnderFlowEventData.RequiredTimeJump == 0)
                    {
                        if (DoSkip)
                        {
                            NumOfBlockToSkip = HDD_Handle.NumberOfBlockToSkip;
                            DoSkip = FALSE;
                        }
                        else
                        {
                            NumOfBlockToSkip = 0;
                        }
                    }
                    else
                    {
                    /* ignore HDD_Handle.NumberOfBlockToSkip set on drift_threshold, because this */
                    /* jump should not be cumulated with the jump required by underflow even      */

                    /* --- Calculate the jump required by current STVID_DATA_UNDERFLOW_EVT --- */

                        NumOfBlockToSkip =
                            (S32) ((((UnderFlowEventData.RequiredTimeJump / 1800) *(S32)(UnderFlowEventData.BitRateValue))
                                    /(HDD_NB_OF_SECTORS_PER_BLOCK*SECTOR_SIZE)));


                    }
#else
                    /* ignore HDD_Handle.NumberOfBlockToSkip set on drift_threshold, because this */
                    /* jump should not be cumulated with the jump required by underflow even      */

                    /* --- Calculate the jump required by current STVID_DATA_UNDERFLOW_EVT --- */

                    NumOfBlockToSkip =
                        (S32) ((((UnderFlowEventData.RequiredTimeJump / HDD_NB_OF_SECTORS_PER_BLOCK) *(S32)(UnderFlowEventData.BitRateValue))
                                /(1800 * SECTOR_SIZE)));
#endif /* WA_USEDRIFTTIMENOTTIMEJUMP */

                    /* --- Calculate the data size required by current STVID_DATA_UNDERFLOW_EVT --- */
                    if ( (UnderFlowEventData.RequiredDuration == 0) || (UnderFlowEventData.BitRateValue == 0) )
                    {
                        NumOfBlockToRead = UnderFlowEventData.BitBufferFreeSize / (SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK);
                    }
                    else
                    {
                        NumOfBlockToRead =
                            (S32) ((((UnderFlowEventData.RequiredDuration / HDD_NB_OF_SECTORS_PER_BLOCK) * (S32)(UnderFlowEventData.BitRateValue))
                                    /(1800 * SECTOR_SIZE)));
                    }
#ifdef HDD_TRACE_UART
                    TraceBuffer(("\r\nUnderflow : jump requested by Underflow is NumOfBlockToSkip=%d NumOfBlockToRead=%d RealSpeed=%d",
                                 NumOfBlockToSkip, NumOfBlockToRead, RealSpeed));
#endif
                    /* --- Check the jump required by current STVID_DATA_UNDERFLOW_EVT --- */

#ifdef ST_5512
                    if (GlobalCurrentSpeed < 0)
                    {
                        /* Backward I only */

                        if (!(HDD_PositionOfIPictureDisplayedAvailable))
                        {
                            HDD_PositionOfIPictureDisplayed = HDD_SeekLastIPictureInFile (HDD_Handle.LBANumber, HDDFirstBlockBuffer_p);
                            HDD_PositionOfIPictureDisplayedAvailable = TRUE;

                            HDD_Handle.LBANumber =
                                HDDFirstBlockBuffer_p->TabIPicturePosition[HDD_PositionOfIPictureDisplayed].IPicturePosition ;
                        }
                        else
                        {
                            HDD_Handle.LBANumber =
                                HDDFirstBlockBuffer_p->TabIPicturePosition[HDD_PositionOfIPictureDisplayed].IPicturePosition ;
                        }
                        NumOfBlockToRead = HDDFirstBlockBuffer_p->TabIPicturePosition[HDD_PositionOfIPictureDisplayed+1].IPicturePosition
                            - HDDFirstBlockBuffer_p->TabIPicturePosition[HDD_PositionOfIPictureDisplayed].IPicturePosition ;

                        IsTemporalDiscontinuity = TRUE;
                    }
                    else
#endif
                    {
                        /* Forward or smooth Backward */
                        HDD_BlockJumpManagement (&HDD_Handle.LBANumber, HDD_Handle.LBAStart,
                                                 HDD_Handle.LBAEnd, RealSpeed,
                                                 &NumOfBlockToRead, NumOfBlockToSkip,
                                                 &HDD_Handle.NumberOfPlay, &IsTemporalDiscontinuity,
                                                 &DataInjectionCompletedParams.TransferRelatedToPrevious);
                    }

                    /* Check if the size of data to transmit is compatible with the BitBufferFreeSize. */
                    if ((NumOfBlockToRead * SECTOR_SIZE * HDD_NB_OF_SECTORS_PER_BLOCK) > (S32) UnderFlowEventData.BitBufferFreeSize)
                    {
                        NumOfBlockToRead = (UnderFlowEventData.BitBufferFreeSize/(SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK));
                        IsTemporalDiscontinuity = TRUE;
                    }

                    /* Now patch the overlap size (WARNING : THIS CODE IS ONLY USED FOR TESTING THE PICTURE MATCHING
                     * ALGORITHME IN BACKWARD PLAYBACK). THIS SHOULD NOT BE CONSIDERED AS THE NORMAL BEHAVIOUR. */
                    if(ShiftInjectionCoef != 0)
                    {
                        /* Overlap patching is enabled */
                        if(  (UnderFlowEventData.RequiredTimeJump < 0)
                           &&((HDD_Handle.LBANumber + NumOfBlockToRead) > (S32)LastLBANumber))
                        {
                            /* We are in smooth backward and we are sure there is an overlap */
                            BackwardOverlapSize = (HDD_Handle.LBANumber + NumOfBlockToRead) - LastLBANumber ;
                            PatchedOverlap      = (NumOfBlockToRead * ShiftInjectionCoef) / 100 ;
                            /* We assume that when we force a different overlap, the new overlap is always bigger bigger
                             * than the one required by the driver. */
                            if(PatchedOverlap > BackwardOverlapSize)
                            {
                                HDD_Handle.LBANumber = (LastLBANumber - NumOfBlockToRead) + PatchedOverlap;
                            }
                        }
                    }
                    /* -------------------------- End of patching --------------------------------------------------*/

                    /* --- Inform STVID of temporal discontinuity before sending data --- */

                    if (IsTemporalDiscontinuity)
                    {
                        STVID_InjectDiscontinuity( VID_Inj_GetDriverHandle(0));
#ifdef HDD_TRACE_UART
                        TraceBuffer((" (discontinuity injected)"));
#endif
                        HDDNbOfTemporalDiscontinuity++;
                    }
#ifdef HDD_TRACE_UART
                    TraceBuffer(("\r\nUnderflow : do read at LBANumber=%d to %d (%d blocks)", HDD_Handle.LBANumber,
                                 (HDD_Handle.LBANumber+NumOfBlockToRead-1), NumOfBlockToRead));
                    InjectStartTime = STOS_time_now();
#endif

                    /* --- Read HDD & send NumOfBlockToRead of data to video driver --- */
                    fseek(HDD_Handle.FileHandle, HDD_Handle.LBANumber*SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK, SEEK_SET);
                    LastLBANumber = HDD_Handle.LBANumber;
                    Error = HDD_ReadAndInjectBlocks( HDDReadBuffer_p,
                                                     HDD_Handle.LBANumber,
                                                     NumOfBlockToRead);

                    HDD_Handle.LBANumber += NumOfBlockToRead;
                    HDDNbOfLBARead += NumOfBlockToRead;
                    if (RealSpeed > 0)
                    {
                        if (HDD_Handle.LBANumber > HDD_Handle.LBAEnd)
                        {
                            HDD_Handle.LBANumber = HDD_Handle.LBAStart;
                            /* when exactly the last LBA has been read at EOF, HDD_BlockJumpManagement() had not decrease the loop count */
                            if (HDD_Handle.NumberOfPlay>0)
                            {
                                HDD_Handle.NumberOfPlay-- ;
                            }
                        }
                    }

                    /* --- Inform STVID that injection is done --- */

                    ErrCode = STVID_DataInjectionCompleted( VID_Inj_GetDriverHandle(0), &DataInjectionCompletedParams);
                    if ( ErrCode != ST_NO_ERROR )
                    {
                        STTBX_Print(("HDD_TrickmodeEventManagerTask() : error %d when calling STVID_DataInjectionCompleted() !\n", ErrCode));
#ifdef HDD_TRACE_UART
                    TraceBuffer((" (injection completed(%d) :%dms)\r\n",
                            DataInjectionCompletedParams.TransferRelatedToPrevious ,
                            (time_minus(time_now(), InjectStartTime)*1000)/TickPerSecond  ));
#endif
                    }
                    else
                    {
                        TraceBuffer(("\r\nInjection done (%d)", NumOfBlockToRead));
                    }
                    if (DataInjectionCompletedParams.TransferRelatedToPrevious)
                    {
                        HDDNbOfRelatedToPrevious++;
                    }
                    if ((HDD_Handle.NumberOfPlay == 0) && (!HDD_Handle.InfinitePlaying))
                    {
#ifdef HDD_TRACE_UART
                        TraceBuffer(("\r\nUnderflow : end of playing, NumOfPlay=%d",
                            HDD_Handle.NumberOfPlay));
#endif
                        break;
                    }
                    break;

                case STVID_DATA_OVERFLOW_EVT :
                    break;

                default :
                    /* Nothing special to do !!! */
                    break;

            } /* End of switch (HDD_Handle.TrickModeEvent) */
            TrickModeEvent = 0; /* set a value different from STVID_DATA_OVERFLOW_EVT or STVID_DATA_UNDERFLOW_EVT */
            /* to avoid executing the case several times for the same event. */
        }
        else
        {
            /* --- Reading&injection is ended or events management is disabled --- */

            STTBX_Print(("HDD_TrickmodeEventManagerTask() : end of playing\n    NumberOfPlay=%d InfinitePlaying=%d ProcessRunning=%d\n",
                    HDD_Handle.NumberOfPlay, HDD_Handle.InfinitePlaying , HDD_Handle.ProcessRunning ));

            /* De-allocation of HDDFirstBlockBuffer_p */
            if (HDDFirstBlockBuffer_Alloc_p != NULL)
            {
                STOS_MemoryDeallocate( NCachePartition_p, HDDFirstBlockBuffer_Alloc_p );
                HDDFirstBlockBuffer_Alloc_p = NULL;
                HDDFirstBlockBuffer_p = NULL;
            }

            ErrCode |=  STEVT_Unsubscribe (EvtHandle, (U32)STVID_DATA_UNDERFLOW_EVT);
            ErrCode |=  STEVT_Unsubscribe (EvtHandle, (U32)STVID_DATA_OVERFLOW_EVT);
            ErrCode |=  STEVT_Unsubscribe (EvtHandle, (U32)STVID_SPEED_DRIFT_THRESHOLD_EVT);

            HDD_Handle.ProcessRunning = FALSE;

            if ( ErrCode!= ST_NO_ERROR )
            {
                STTBX_Print(("HDD_TrickmodeEventManagerTask() : STEVT_Unsubscribe(), error %d !\n", ErrCode));
            }
            else
            {
                STTBX_Print(("HDD_TrickmodeEventManagerTask() : STEVT_Unsubscribe() well done\n"));
            }
        }
        HDD_Handle.EndOfLoopReached = TRUE;

    } /* end of while (1) */

    STOS_MemoryDeallocate( NCachePartition_p, HDDReadBuffer_Alloc_p );

    STOS_TaskExit(NULL);
} /* end of HDD_TrickmodeEventManagerTask */

/*******************************************************************************
Name        : HDD_ReadBlocks
Description : Read data from HDD
Parameters  : Statapi cmd., buffer address, buffer size
Assumptions : buffer allocated for read data
Limitations :
Returns     : TRUE if error
*******************************************************************************/
static BOOL HDD_ReadBlocks(char *ReadBuffer_p, U32 BufferSize)
{
    BOOL    Error = FALSE;
    U32     BytesRead;
    S32     BytesToRead = BufferSize;

    if (HDD_Handle.FileHandle != NULL)
    {
        while (BytesToRead > 0)
        {
            BytesRead = fread(ReadBuffer_p, 1, BytesToRead, HDD_Handle.FileHandle);

            if (!BytesRead)
                break;
            BytesToRead -= BytesRead;
            ReadBuffer_p += BytesRead;
        }
    }
    else
    {
        STTBX_Print(("%s: No opened file, use HDD_SETFILE to open a file\n", __FUNCTION__));
        Error=TRUE;
    }

    return(Error);
} /* HDD_ReadBlocks() */

/*******************************************************************************
Name        : HDD_InjectBlocks
Description : Inject HDD data to video bit buffer
Parameters  : buffer address, buffer size
Assumptions : buffer allocated for read data
Limitations :
Returns     : none
*******************************************************************************/
static void HDD_InjectBlocks(char *ReadBuffer_p, U32 BufferSize)
{
#ifndef DVD_TRANSPORT_STPTI
    BOOL                Error = FALSE;
#endif
#if defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined (ST_7109) || defined(ST_7200)
    U32 InputBufferFreeSize,CopiedNB;
    void*               Read_p; /* Shortcuts */
    void*               Write_p; /* Shortcuts */
#else /* not ST_5100 */
#ifdef DVD_TRANSPORT_STPTI
    STPTI_DMAParams_t   STDMA_Params;
#ifdef DVD_TRANSPORT_STPTI4
    U32 lDMAUSed;
#endif /* DVD_TRANSPORT_STPTI4 */
#endif /* DVD_TRANSPORT_STPTI */
#endif /* not ST_5100 */

    /* --- Data injection in bit buffer --- */

    /* copy 'buffer-size' in input buffer */
    CopiedNB = 0;
#ifdef HDD_INJECTION_TRACE_UART
    TraceBuffer(("Begin copy to input buffer(1)\r\n"));
#endif /* HDD_INJECTION_TRACE_UART */

    while(CopiedNB != BufferSize)
    {
        /* As the Read pointer can be changed by the driver, take a copy at the beginning and work with it */
        Read_p = VID_Injection[0].Read_p;
        Write_p= VID_Injection[0].Write_p;
        if(Read_p > Write_p)
        {
            InputBufferFreeSize = (U32)Read_p
                - (U32)Write_p;
        }
        else
        {
            InputBufferFreeSize = (U32)VID_Injection[0].Top_p
                - (U32)Write_p + 1
                + (U32)Read_p
                - (U32)VID_Injection[0].Base_p;
        }
        /* Leave always 1 empty byte to avoid Read=Write */
        if(InputBufferFreeSize != 0)
        {
            InputBufferFreeSize-=1;
        }

        if(CopiedNB + InputBufferFreeSize >= BufferSize)
        {
            InputBufferFreeSize = BufferSize-CopiedNB;
        }
        /* Input buffer is full ? So, wait for one transfer period */
        if(InputBufferFreeSize == 0)
        {
            /* calculate the periode for transfers (constant) */
#ifdef HDD_INJECTION_TRACE_UART
            TraceBuffer(("Waiting !\r\n"));
#endif /* HDD_INJECTION_TRACE_UART */
            STOS_TaskDelayUs(MS_TO_NEXT_TRANSFER * 1000);
            continue;
        }
        /* Write to the input buffer */
        if((U32)Write_p + InputBufferFreeSize > (U32)VID_Injection[0].Top_p + 1)
        {
            /* First write (in case of loop) */
#ifndef	DONT_USE_FDMA
            STAVMEM_CopyBlock1D(/* src  */ (void*)((U32)ReadBuffer_p + CopiedNB),
                                /* dest */ Write_p,
                                /* size */(U32)VID_Injection[0].Top_p - (U32)Write_p + 1);
#else
            STOS_memcpy              (/* dest */ Write_p,
                                      /* src  */ (void*)((U32)ReadBuffer_p + CopiedNB),
                                      /* size */(U32)VID_Injection[0].Top_p - (U32)Write_p + 1);
#endif  /* DONT_USE_FDMA */
#ifdef HDD_INJECTION_TRACE_UART
            TraceBuffer(("src=%08x dest=%08x bytes=%08x Top=%08x Base=%08x\r\n",
                         ((U32)ReadBuffer_p + CopiedNB),
                         Write_p,
                         (U32)VID_Injection[0].Top_p - (U32)Write_p + 1,
                         (U32)VID_Injection[0].Top_p,
                         (U32)VID_Injection[0].Base_p));
#endif /* HDD_INJECTION_TRACE_UART */
            InputBufferFreeSize -= (U32)VID_Injection[0].Top_p - (U32)Write_p + 1;
            CopiedNB += (U32)VID_Injection[0].Top_p - (U32)Write_p + 1;
            Write_p = VID_Injection[0].Base_p;
            TraceBuffer(("Looping !\r\n"));
        }
#ifndef	DONT_USE_FDMA
        STAVMEM_CopyBlock1D(/* src  */ (void*)((U32)ReadBuffer_p + CopiedNB),
                            /* dest */ Write_p,
                            /* size */InputBufferFreeSize);
#else
        STOS_memcpy				(/* dest */ Write_p,
                                 /* src  */ (void*)((U32)ReadBuffer_p + CopiedNB),
                                 /* size */InputBufferFreeSize);

#endif  /* DONT_USE_FDMA */
#ifdef SAVE_TO_DISK
        debugwrite(HDDFileDescriptor,
                   (void*)((U32)ReadBuffer_p + CopiedNB),
                   InputBufferFreeSize);
#endif /* SAVE_TO_DISK */
#ifdef HDD_INJECTION_TRACE_UART
        TraceBuffer(("src=%08x dest=%08x bytes=%08x Top=%08x Base=%08x\r\n",
                     ((U32)ReadBuffer_p + CopiedNB),
                     Write_p,
                     InputBufferFreeSize,
                     (U32)VID_Injection[0].Top_p,
                     (U32)VID_Injection[0].Base_p));
#endif /* HDD_INJECTION_TRACE_UART */
        Write_p = (void*)((U32)Write_p + InputBufferFreeSize);
        if(Write_p == (void*)((U32)VID_Injection[0].Top_p + 1))
        {
            Write_p = VID_Injection[0].Base_p;
        }
        VID_Injection[0].Write_p = Write_p;
        CopiedNB += InputBufferFreeSize;
    }
} /* HDD_InjectBlocks() */

/*******************************************************************************
Name        : HDD_ReadAndInjectBlocks
Description : Read data from HDD and inject them to the bit buffer
              (do it by step of HDD_NB_OF_SECTORS_PER_BLOCK*SECTOR_SIZE bytes)
Parameters  : address of the buffer, LBA, nb of blocks
Assumptions : buffer allocated with size=2*(HDD_NB_OF_SECTORS_PER_BLOCK*SECTOR_SIZE)
Limitations :
Returns     : TRUE if error
*******************************************************************************/
static BOOL HDD_ReadAndInjectBlocks(char *ReadBuffer_p,
                                    U32 LBAPositionToRead, U32 PartNumOfBlockToRead)
{
    BOOL                Error = FALSE;
    U32                 Count;
    U32                 MaxCount;
    U32                 BytesRead;
    U32                 BufferSize;
    U32                 Coeff;
    U32                 SectorCount;

    /**ErrCode = STATA_ReadMultiple ( 0, LBAPositionToRead, PartNumOfBlockToRead, HDD_NB_OF_SECTORS_PER_BLOCK);**/

    /* Units conversion for data addressing on HDD :                                  */
    /* When STVID requests 1 block, STATAPI should read (16 sectors x 512 bytes)=8Kb  */
    /* When STVID requests the LBA number N, STATAPI should select address (N x 16)   */
    /* because for STATAPI 1 LBA is equal to 1 sectors = 512 bytes                    */

/*#define TRACE_HDD_READ 1*/

    Coeff = NBSECTORSTOINJECTATONCE; /* set it to 1, 2, 3... to read 8KB, 16KB, 24KB at each HDD read */
                /* (buffer pointed by ReadBuffer_p should be sized accordingly)  */
    BufferSize = Coeff*(HDD_NB_OF_SECTORS_PER_BLOCK*SECTOR_SIZE);
    SectorCount = (U8)Coeff*HDD_NB_OF_SECTORS_PER_BLOCK;

    Count=0;
    /* at each loop, read Coeff blocks (or Coeff*HDD_NB_OF_SECTORS_PER_BLOCK sectors) */
    MaxCount = PartNumOfBlockToRead / Coeff ;

#ifdef TRACE_HDD_READ
    STTBX_Print(("Request : LBAPositionToRead=%d PartNumOfBlockToRead=%d (or %d sectors)\n",
                LBAPositionToRead, PartNumOfBlockToRead, PartNumOfBlockToRead*HDD_NB_OF_SECTORS_PER_BLOCK));
    STTBX_Print(("%s: %d times SectorCount=%d in BufferSize=%d\n", __FUNCTION__, MaxCount, SectorCount, BufferSize));
#endif
#ifdef HDD_TRACE_UART
    TraceBuffer(("\r\nRead&Inject : LBAPositionToRead=%d PartNumOfBlockToRead=%d (or %d sectors)",
                LBAPositionToRead, PartNumOfBlockToRead, PartNumOfBlockToRead*HDD_NB_OF_SECTORS_PER_BLOCK));
    TraceBuffer(("%s: %d times SectorCount=%d in BufferSize=%d ", __FUNCTION__, MaxCount, SectorCount, BufferSize));
#endif

    /* --- Read HDD and inject data --- */

    while ( (Count < MaxCount) && (!(Error)) )
    {
        Error = HDD_ReadBlocks(ReadBuffer_p, BufferSize);
        if (!Error)
        {
#ifdef TRACE_HDD_READ
            STTBX_Print(("%s: Inj. Count=%d BufferSize=%d\n", __FUNCTION__, Count, BufferSize));
#endif
            HDD_InjectBlocks(ReadBuffer_p, BufferSize);

            Count++;
        } /* end read completed */
        else
        {
            STTBX_Print(("%s: No opened file. Use HDD_SETFILE to open a file\n", __FUNCTION__));

            Error=TRUE;
        }
    } /* end while Count */

    /* --- Read and inject remaining data --- */

    MaxCount = PartNumOfBlockToRead - (Coeff*MaxCount) ; /* nb of remaining blocks */
    if ( MaxCount > 0 )
    {
        BytesRead = 0;
        BufferSize = MaxCount*(HDD_NB_OF_SECTORS_PER_BLOCK*SECTOR_SIZE);
        SectorCount = (U8)MaxCount*HDD_NB_OF_SECTORS_PER_BLOCK;
#ifdef TRACE_HDD_READ
    STTBX_Print(("%s: Remaining : do 1 read&inject SectorCount=%d in BufferSize=%d\n",
                __FUNCTION__, SectorCount, BufferSize));
#endif
#ifdef HDD_TRACE_UART
    TraceBuffer(("%s: 1 time SectorCount=%d in BufferSize=%d",
                __FUNCTION__, SectorCount, BufferSize));
#endif
        Error = HDD_ReadBlocks(ReadBuffer_p, BufferSize);
        if (!Error)
        {
            HDD_InjectBlocks(ReadBuffer_p, BufferSize);
        } /* end read completed */
    } /* end if remaining blocks */
    return(Error);

} /* end of HDD_ReadAndInjectBlocks() */

#if defined (mb282b) || defined (mb275) || defined (mb275_64) || defined (mb314) || defined (mb290) || defined (mb361) || defined (mb382) || defined (mb376) || defined (espresso) || defined (mb390) || defined(mb391) || defined (mb411) || defined(mb519) || defined(mb634)
/*******************************************************************************
Name        : HDD_TrickModeCallBack
Description : Process running on STVID_DATA_UNDERFLOW_EVT, STVID_DATA_OVERFLOW_EVT
              or STVID_SPEED_DRIFT_THRESHOLD_EVT event occurrence.
Parameters  : Reason, type of the event.
              Event, nature of the event.
              EventData, data associted to the event.
Assumptions : At least one event have been subscribed.
Limitations :
Returns     : Nothing.
*******************************************************************************/
static void     HDD_TrickModeCallBack
/*        ( STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, STEVT_EventConstant_t Event, const void *EventData, const void *SubscriberData_p)*/
        ( STEVT_CallReason_t Reason, STEVT_EventConstant_t Event, const void *EventData)
{
    UNUSED_PARAMETER(Reason);
    switch (Event)
    {
        case STVID_DATA_OVERFLOW_EVT :
            break;

        case STVID_DATA_UNDERFLOW_EVT :
            /* Check if the event is idle, or not yet used */
            if (!(HDD_Handle.UnderFlowEventOccured))
            {
                /* Save the event nature and the event data */
                HDD_Handle.UnderFlowEventOccured = TRUE;

                STOS_memcpy(&HDD_Handle.UnderflowEventData, EventData, sizeof (STVID_DataUnderflow_t));

                /* Signal the occurence of this event. */
                STOS_SemaphoreSignal ( HDD_Handle.TrickModeDriverRequest_p );
            }
            HDDNbOfVideoUnderflowEvent++;
            break;

        case STVID_SPEED_DRIFT_THRESHOLD_EVT :
            /* Check if the event is idle, or not yet used */
            if (!(HDD_Handle.SpeedDriftThresholdEventOccured))
            {
                /* Save the event nature and the event data */
                HDD_Handle.SpeedDriftThresholdEventOccured = TRUE;

                STOS_memcpy(&HDD_Handle.SpeedDriftThresholdData, EventData, sizeof (STVID_SpeedDriftThreshold_t));

                /* Signal the occurence of this event. */
                STOS_SemaphoreSignal( HDD_Handle.TrickModeDriverRequest_p );
            }
            HDDNbOfVideoDriftThresholdEvent++;
            break;

        default :
            /* Nothing special to do !!! */
            break;
    }

} /* end of HDD_TrickModeCallBack */
#endif

/*******************************************************************************
Name        : HDD_BlockJumpManagement
Description : Calculation of new blocks to be read according to Underflow event
Input       : CurrentBlock  (current position of the playing)
              StartBlock    (begin of the stream)
              EndBlock      (end of the stream)
              Speed         (current speed)
              BlocksToRead  (number of blocks to be read)
              JumptoPerform (number of blocks to jump)
              NumberOfPlaying (number of playing the sequence should make)
              Discontinuity  (true if no temporal continuity)
              RelatedToPrevious (true if related to previous)
Output      : CurrentBlock  (new position to be read)
              BlocksToRead  (new number of blocks to be read)
              NumberOfPlaying (number of playing the sequence should make)
              Discontinuity  (true if no temporal continuity)
              RelatedToPrevious (true if related to previous)
Returns     : None
*******************************************************************************/
static void HDD_BlockJumpManagement (S32 *CurrentBlock, S32 StartBlock, S32 EndBlock,
                                     S32 CurrentSpeed, S32 *BlocksToRead,
                                     S32 JumpToPerform, S32 *NumberOfPlaying,
                                     BOOL *Discontinuity, BOOL *RelatedToPrevious)
{
    S32 RelativePosition;
    S32 NewBlock;
    S32 NextEndOfRead, NextStartOfRead;

    RelativePosition = *CurrentBlock + JumpToPerform ;
    *RelatedToPrevious = FALSE;

    NextEndOfRead = RelativePosition + *BlocksToRead - 1;
    NextStartOfRead = RelativePosition;

    if ((JumpToPerform<0) && (*BlocksToRead == -JumpToPerform))
    {
#ifdef HDD_TRACE_UART
    TraceBuffer(("\r\nNegative Jump equals # of block to read %d %d %d\r\n",*CurrentBlock, JumpToPerform, *BlocksToRead));
#endif
    }
    if ((NextStartOfRead >= StartBlock) && (NextEndOfRead <= EndBlock))
    {
        /* requirement is [......###.] on HDD, do read [......###.] */
        NewBlock = RelativePosition;
    }
    else
    {
        if ((*NumberOfPlaying > 1) || (HDD_Handle.InfinitePlaying))
        {
            /* if loop has to be done : sent a all requested amount from new suitable location according to speed sign */
            if (CurrentSpeed < 0)
            {
                /* requirement is ##[#.........] or [........#]## on HDD, do read [.......###] */
                /* Check the request don't lead to unreachable jump if the requested size if greater than the stream size */
                if (*BlocksToRead > (EndBlock - StartBlock+1))
                {
                    /* Requested amount of data exceeds the stream length, just inject the available data */
                    NewBlock = StartBlock;
                    *BlocksToRead = EndBlock - StartBlock + 1;
                }
                else
                {
                    NewBlock = EndBlock - *BlocksToRead + 1 ;
                }
            }
            else if(RelativePosition > EndBlock)
            {
                /* requirement is [...] ### on HDD, do read [###.......] */
                NewBlock = StartBlock;
                /* Check the request don't lead to unreachable jump if the requested size if greater than the stream size */
                if (*BlocksToRead > (EndBlock - StartBlock+1))
                {
                    *BlocksToRead = EndBlock - StartBlock + 1;
                }
            }
            else
            {
                /* requirement is [.........#]## on HDD, do read [.........#] */
                /* truncation : don't read what is after the end of the stream */
                *BlocksToRead = EndBlock - RelativePosition + 1 ;
                NewBlock = RelativePosition;
            }
            (*NumberOfPlaying)-- ;
        }
        else
        {
            /* Single or last loop : send only available taking into account both requested data amount & stream limits */
            if (NextStartOfRead < StartBlock)
            {
                /* requirement is ##[#.........] on HDD, do read [#.........] */
                NewBlock = StartBlock;
                *BlocksToRead = *BlocksToRead - (StartBlock - RelativePosition + 1);
                /* Check the request don't lead to unreachable jump if the requested size if greater than the stream size */
                if (*BlocksToRead > (EndBlock - StartBlock+1))
                {
                    *BlocksToRead = EndBlock - StartBlock + 1;
                }
            }
            else
            {
                if (RelativePosition > EndBlock)
                {
                    /* requirement is [.........] ### on HDD, do injection from beginning of the stream */
                    /* This will lead to unexpected display in case of HDD jump but it is better than injecting nothing
                     * at the driver's point of view */
                    NewBlock = StartBlock;
                    /* Check the request don't lead to unreachable jump if the requested size if greater than the stream size */
                    if (*BlocksToRead > (EndBlock - StartBlock+1))
                    {
                        /* Requested amount of data exceeds the stream length, just inject the available data */
                        *BlocksToRead = EndBlock - StartBlock + 1;
                    }
                }
                else
                {
                    /* requirement is [.........#]## on HDD, do read [.........#] */
                    /* truncation : don't read what is after the end of the stream */
                    *BlocksToRead = EndBlock - RelativePosition + 1 ;
                    NewBlock = RelativePosition;
                }
            }
            (*NumberOfPlaying)-- ;
        }
    }

    /* Set Discountinuity & related to previous flags */
    if ( (PreviousCurrentBlock + PreviousBlocksToRead) == NewBlock)
    {
        /* only RelativePosition value can reveal a forward wraparound, because NewBlock is already adjusted ! */
        if ( RelativePosition > EndBlock)
        {
            /* forward wraparound */
            *RelatedToPrevious = FALSE;
            *Discontinuity = TRUE;
        }
        else
        {
            /* normal case */
            *RelatedToPrevious = TRUE;
            if (CurrentSpeed < 0)
            {
                *Discontinuity = TRUE;
            }
            else
            {
                *Discontinuity = FALSE;
            }
        }
    }
    else
    {
        *Discontinuity = TRUE;
        *RelatedToPrevious = FALSE;
        if ( ((NewBlock > PreviousCurrentBlock) && (NewBlock <= (PreviousCurrentBlock + PreviousBlocksToRead))) ||
             ((NewBlock <= PreviousCurrentBlock) && ((NewBlock + *BlocksToRead) >= PreviousCurrentBlock)) )
        {
            *RelatedToPrevious = TRUE;
        }
    }

    if (CurrentSpeed < 0)
    {
        HDD_PositionOfIPictureDisplayed = HDD_SeekLastIPictureInFile (NewBlock, HDDFirstBlockBuffer_p);
    }

#ifdef HDD_TRACE_UART
    TraceBuffer(("%s: JumpMgt : need jump=%d from %d to %d, do jump to %d, read %d, RelatedToP.=%d NumOfPlay=%d",
            __FUNCTION__, JumpToPerform, *CurrentBlock, RelativePosition, NewBlock, *BlocksToRead,
            *RelatedToPrevious, *NumberOfPlaying));
#endif
    *CurrentBlock = NewBlock; /* new LBA to read */
    PreviousCurrentBlock = *CurrentBlock ;  /* save this value for the next jump */
    PreviousBlocksToRead = *BlocksToRead ;

} /* end of HDD_BlockJumpManagement */


/*******************************************************************************
Name        : HDD_SeekLastIPictureInFile
Description : Function that seek the position of the I picture just after the
              current position.
Parameters  : CurrentBlockRead, pointer to the current position of the playing.
              FirstBlockOfTheFile, pointer to the first block of the played file.
Limitations :
Returns     : The position of the I Picture to display (in FirstBlock array number).
              If the CurrentBlockRead value is low enough so that the last I
              Picture is at the end of the file, le returned position will be
              according this fact.
*******************************************************************************/
static U32      HDD_SeekLastIPictureInFile (U32 CurrentBlockRead, HDD_FirstBlock_t *FirstBlockOfTheFile)
{

    U32     SeekIndex;                  /* Index value for the seek of the position. */
    U32     NumberOfIPicture;           /* Safe of the number of I Picture to play. */

    NumberOfIPicture = FirstBlockOfTheFile->NumberOfIPicture;

    /* Test of the error case : No I Picture counted in the file ... */
    if (!NumberOfIPicture)
    {
        /* Just return the current position. */
        return(0);
    }

    /* Test of the special first */
    if (CurrentBlockRead <= FirstBlockOfTheFile->TabIPicturePosition[0].IPicturePosition)
    {
        return(0);
    }

    /* Test of the special last position. */
    if ( (CurrentBlockRead >= FirstBlockOfTheFile->TabIPicturePosition[NumberOfIPicture-1].IPicturePosition) )
    {
        /* The last I Picture is located at the end of the file. */
        return(FirstBlockOfTheFile->NumberOfIPicture-1);
    }

    SeekIndex = 0;

    while ( ((SeekIndex+50)<FirstBlockOfTheFile->NumberOfIPicture-1)
            && (CurrentBlockRead>FirstBlockOfTheFile->TabIPicturePosition[SeekIndex+50].IPicturePosition) )
    {
        SeekIndex+=50;
    }

    for (; SeekIndex<FirstBlockOfTheFile->NumberOfIPicture; SeekIndex++)
    {
        if (CurrentBlockRead < FirstBlockOfTheFile->TabIPicturePosition[SeekIndex].IPicturePosition)
        {
            return(SeekIndex);
        }
    }

    return(CurrentBlockRead);

} /* End of HDD_SeekLastIPictureInFile */


/*-------------------------------------------------------------------------
 * Function : HDD_ChangeSpeed
 *            Interactive test of STVID_SetSpeed() for trickmode tests
 *            STVID must be running, and HDD injection in progress
 *            hdd_setspeed() must be then launched with specified speed
 *            ( define a macro to repeat this command + hdd_setspeed )
 * Input    :
 * Output   :
 * Return   : FALSE if no error, else TRUE
 * ----------------------------------------------------------------------*/
static BOOL HDD_ChangeSpeed(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    S32 Speed, LVar;
    STVID_Handle_t VideoHndl;
    char Hit;

    RetErr = STTST_GetInteger( Parse_p, (int)VID_Inj_GetDriverHandle(0), &LVar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( Parse_p, "expected video handle" );
        return ( TRUE );
    }
    VideoHndl = (STVID_Handle_t)LVar;


    RetErr = STTST_GetInteger( Parse_p, 100, &Speed );
    if ( RetErr )
    {
        STTST_TagCurrentLine( Parse_p, "expected Speed (100=normal)" );
        return ( TRUE );
    }

    Hit=0;
    while (Hit!='q' && Hit!='Q' && Hit!='\r')
    {
        STTBX_Print(("Change HDD & STVID speed interactively\nPress S=stop N=normal H=high V=very high F=forward B=backward Q=quit\n     + or - to increase/decrease, * or / to multiply/divide by 2\n"));
        STTBX_InputChar(&Hit);
        switch(Hit)
        {
            /**case 'q':
            case 'Q':
                Speed = 99999;
                break;***/
            case 's':
            case 'S':
                Speed = 0;
                break;
            case 'n':
            case 'N':
                Speed = 100;
                break;
            case 'h':
            case 'H':
                Speed = 300;
                break;
            case 'v':
            case 'V':
                Speed = 800;
                break;
            case 'f':
            case 'F':
                if (Speed<0)
                {
                    Speed = Speed * (-1);
                }
                break;
            case 'b':
            case 'B':
                if (Speed>0)
                {
                    Speed = Speed * (-1);
                }
                break;
            case '-':
                if (Speed>100 || Speed<-100)
                {
                    Speed-=10;
                }
                else
                {
                    Speed-=1;
                }
                break;
            case '+':
                if (Speed>100 || Speed<-100)
                {
                    Speed+=10;
                }
                else
                {
                    Speed+=1;
                }
                break;
            case '/':
                Speed = Speed / 2;
                break;
            case '*':
                Speed = Speed * 2;
                break;
            default:
                break;
        } /* end switch */

        /* --- Change the STVID & HDD speed and return its value --- */

        if ( Hit != 'q' && Hit != 'Q' && Hit != '\r' )
        {
            ErrCode = STVID_SetSpeed(VideoHndl, Speed);
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(("STVID_SetSpeed(%d,%d) failed !\n",VideoHndl, Speed));
                RetErr = TRUE;
            }
            else
            {
                if (Speed >= 0)
                {
                    HDD_PositionOfIPictureDisplayedAvailable = FALSE;
                    if (GlobalCurrentSpeed < 0)
                    {
                        if (HDD_PositionOfIPictureDisplayed >= HDDFirstBlockBuffer_p->NumberOfIPicture-1)
                        {
                            HDD_Handle.LBANumber = HDD_Handle.LBAStart;
                        }
                        else
                        {
                            HDD_Handle.LBANumber =
                                HDDFirstBlockBuffer_p->TabIPicturePosition[HDD_PositionOfIPictureDisplayed+1].IPicturePosition;
                        }
                    }
                }
                STTBX_Print(("Speed set to  %d \n", Speed));
                /* Save locally the speed of the system */
                GlobalCurrentSpeed = (S32) (Speed);
            }
        }
    } /* end while */

    STTST_AssignInteger(result_sym_p, Speed, FALSE);

    return ( RetErr );

} /* end of HDD_ChangeSpeed() */

/*-------------------------------------------------------------------------
 * Function : HDD_GetStatistics
 * Input    :
 * Output   :
 * Return   : FALSE if no error, else TRUE
 * ----------------------------------------------------------------------*/
static BOOL HDD_GetStatistics(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    BOOL    EnablePrint = TRUE;
    BOOL    RetErr;

    /* Evaluate the EnablePrint parameter(enables printing of the statistics on the screen) */
    RetErr = STTST_GetInteger( Parse_p, (int)EnablePrint, &EnablePrint);
    if ( RetErr )
    {
        STTST_TagCurrentLine( Parse_p, "expected (TRUE/FALSE)");
        return(TRUE);
    }

    if(EnablePrint)
    {
        STTST_Print((" Nb of UNDERFLOW_EVT received ......: %d\n", HDDNbOfVideoUnderflowEvent ));
        STTST_Print(("    Nb of positive time jumps ..... : %d\n", HDDNbOfForwardUnderflow ));
        STTST_Print(("    Nb of negative time jumps ..... : %d\n", HDDNbOfBackwardUnderflow ));
        STTST_Print(("    Nb of zero bitrate ............ : %d\n", HDDNbBitrateZero ));
        STTST_Print(("    Maximum bitrate received ...... : %d\n", HDDMaxBitrate ));
        STTST_Print(("    Minimal bitrate received ...... : %d\n", HDDMinBitrate ));
        STTST_Print(("    Average bitrate received ...... : %d\n", HDDAverageBitrate ));
        STTST_Print((" Nb of DRIFT_THRESHOLD_EVT received : %d\n", HDDNbOfVideoDriftThresholdEvent ));
        STTST_Print((" Nb of LBA read from HDD .......... : %d\n", HDDNbOfLBARead ));
        STTST_Print((" Nb of TemporalDiscontinuity ...... : %d\n", HDDNbOfTemporalDiscontinuity ));
        STTST_Print((" Nb of RelatedToPrevious .......... : %d\n", HDDNbOfRelatedToPrevious ));
        STTST_Print((" LBA current read position ........ : %d\n", HDD_Handle.LBANumber ));
        STTST_Print((" LBA start position of the stream . : %d\n", HDD_Handle.LBAStart ));
        STTST_Print((" LBA end position of the stream ... : %d\n", HDD_Handle.LBAEnd ));

        if (HDDFirstBlockBuffer_p != NULL)
        {
            STTST_Print((" Nb of bytes of the stream ........ : %d\n", HDD_Handle.FileSize));
            STTST_Print((" Nb of I pictures of the stream ... : %d\n", HDDFirstBlockBuffer_p->NumberOfIPicture ));
        }
    }
    STTST_AssignInteger("RET_VAL1", (int) HDDNbOfVideoUnderflowEvent, FALSE);
    STTST_AssignInteger("RET_VAL2", (int) HDDNbOfForwardUnderflow, FALSE);
    STTST_AssignInteger("RET_VAL3", (int) HDDNbOfBackwardUnderflow, FALSE);
    STTST_AssignInteger("RET_VAL4", (int) HDDNbBitrateZero, FALSE);
    STTST_AssignInteger("RET_VAL5", (int) HDDMaxBitrate, FALSE);
    STTST_AssignInteger("RET_VAL6", (int) HDDMinBitrate, FALSE);
    STTST_AssignInteger("RET_VAL7", (int) HDDAverageBitrate, FALSE);
    STTST_AssignInteger("RET_VAL8", (int) HDDNbOfVideoDriftThresholdEvent, FALSE);
    STTST_AssignInteger("RET_VAL9", (int) HDDNbOfLBARead, FALSE);
    STTST_AssignInteger("RET_VAL10",(int) HDDNbOfTemporalDiscontinuity, FALSE);
    STTST_AssignInteger("RET_VAL11",(int) HDDNbOfRelatedToPrevious, FALSE);

    STTST_AssignInteger(result_sym_p, FALSE, FALSE);
    return ( FALSE );
}

/*-------------------------------------------------------------------------
 * Function : HDD_ResetStatistics
 * Input    :
 * Output   :
 * Return   : FALSE if no error, else TRUE
 * ----------------------------------------------------------------------*/
static BOOL HDD_ResetStatistics(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    UNUSED_PARAMETER(Parse_p);
    UNUSED_PARAMETER(result_sym_p);

    HDDNbOfVideoUnderflowEvent = 0;
    HDDNbOfVideoDriftThresholdEvent = 0;
    HDDNbOfLBARead = 0;
    HDDNbOfTemporalDiscontinuity = 0;
    HDDNbOfRelatedToPrevious = 0;
    HDDNbOfTreatedVideoUnderflowEvent = 0;
    HDDNbBitrateZero = 0;
    HDDBitratesSum = 0;
    HDDMaxBitrate = 0;
    HDDMinBitrate = 0xFFFFFFFF;
    HDDAverageBitrate = 0;
    HDDNbOfForwardUnderflow = 0;
    HDDNbOfBackwardUnderflow = 0;

    return ( FALSE );
}

/* ========================================================================
Name        : HDD_ShiftInjectionStartPosition
Description : Shifts the LBA start of each backward injection according to
              the given coefficient in order to increase the overlap
Parameters  : A coefficient
Assumptions :
Limitations :
Returns     : FALSE if no error, else TRUE
======================================================================== */
static BOOL HDD_ShiftInjectionStartPosition (STTST_Parse_t * Parse_p, char *result)
{
    BOOL Error;

    /* Get the overlap ratio */
    Error = STTST_GetInteger ( Parse_p , 0 , (S32*)&ShiftInjectionCoef);
    if ((Error) || (ShiftInjectionCoef > 100))
    {
        STTST_TagCurrentLine ( Parse_p , "Expected a number between (0 and 100)");
        Error = TRUE;
    }

    STTST_AssignInteger(result, FALSE, FALSE);
    return(Error);
} /* end of HDD_ShiftInjectionStartPosition() */

/*******************************************************************************
Name        : HDD_SetFile
Description : Initialize file handle to be sued by HDD module
Parameters  : None
Assumptions :
Limitations :
Returns     : FALSE if no error, else TRUE
*******************************************************************************/
static BOOL HDD_SetFile(STTST_Parse_t * Parse_p, char *result)
{
    BOOL    Error = FALSE;
    char    FileName[HDD_MAX_FILENAME_LENGTH];

    memset(FileName, 0x00, HDD_MAX_FILENAME_LENGTH);
    /* Get the name of file to be read (or written) */
    Error = STTST_GetString (Parse_p, "",(char *) FileName, HDD_MAX_FILENAME_LENGTH-1);
    if ( ( Error ) || ( strlen(FileName) == 0 ) )
    {
        STTST_TagCurrentLine ( Parse_p , "Invalid host filename (HDD_MAX_FILENAME_LENGTH-1 characters max.)" );
        Error = TRUE;
    }
    else
    {
        if (HDD_Handle.FileHandle != NULL)
        {
            fclose(HDD_Handle.FileHandle);
        }

        HDD_Handle.FileHandle = (void *)fopen(FileName, "r");
        if (HDD_Handle.FileHandle != NULL)
        {
            HDD_Handle.FileSize = ffilesize(HDD_Handle.FileHandle);
            STTBX_Print(("%s: %d (%d blocks)\n", FileName, HDD_Handle.FileSize, (HDD_Handle.FileSize-1)/(SECTOR_SIZE*HDD_NB_OF_SECTORS_PER_BLOCK)));

            fseek(HDD_Handle.FileHandle, 0, SEEK_SET);
        }
        else
        {
            STTST_TagCurrentLine ( Parse_p , "Unable to open file" );
            Error = TRUE;
        }
    }

    STTST_AssignInteger(result, FALSE, FALSE);
    return(Error);
}

/*******************************************************************************
Name        : HDD_UnSetFile
Description : Initialize file handle to be sued by HDD module
Parameters  : None
Assumptions :
Limitations :
Returns     : FALSE if no error, else TRUE
*******************************************************************************/
static BOOL HDD_UnSetFile(STTST_Parse_t * Parse_p, char *result)
{
    if (HDD_Handle.FileHandle != NULL)
    {
        fclose(HDD_Handle.FileHandle);

        HDD_Handle.FileHandle = NULL;
        HDD_Handle.FileSize = 0;
    }

    return FALSE;
}

/*##########################################################################*/
/*###########################  GLOBAL FUNCTIONS  ###########################*/
/*##########################################################################*/

/*******************************************************************************
Name        : HDD_RegisterCmd
Description : Registration of HDD commands
Parameters  : None
Assumptions :
Limitations :
Returns     : FALSE if no error, else TRUE
*******************************************************************************/
BOOL HDD_RegisterCmd(ST_Partition_t *CPUPartition_p, ST_Partition_t *NCachePartition_p)
{
    BOOL RetErr ;

    RetErr =  STTST_RegisterCommand ("HDD_CHANGESPEED", HDD_ChangeSpeed , "Ask and change HDD & video speed");
/*     RetErr |= STTST_RegisterCommand ("HDD_COPY",      HDD_Copy     , "<file> <LBA> 0  copy host file on HDD at LBA position\n\t\t<file> <LBA> <NbLBA> copy NbLBA from HDD stream to host file"); */
    RetErr |= STTST_RegisterCommand ("HDD_GETSTAT",   HDD_GetStatistics , "Get statistics related to HDD Player");
    RetErr |= STTST_RegisterCommand ("HDD_PLAYEVENT", HDD_PlayEvent, "<LBA> <NumOfPlay> Play HDD stream (default: LBA=0, Num=0=infinite");
    RetErr |= STTST_RegisterCommand ("HDD_RESETSTAT", HDD_ResetStatistics , "Reset statistics related to HDD Player");
    RetErr |= STTST_RegisterCommand ("HDD_SET_SPEED", HDD_SetSpeed , "<Speed> init of the speed currently played");
    RetErr |= STTST_RegisterCommand ("HDD_STOPEVENT", HDD_StopEvent, "Stop playing from HDD with event management");
    RetErr |= STTST_RegisterCommand ("HDD_SHIFT", HDD_ShiftInjectionStartPosition, "<percent> Forces the overlap size to be always equal to <percent> of the injection size");
    RetErr |= STTST_RegisterCommand ("HDD_SETFILE", HDD_SetFile, "<Filename> Use <Filename> as file to be injected");
    RetErr |= STTST_RegisterCommand ("HDD_UNSETFILE", HDD_UnSetFile, "Close a previously opened file");

    RetErr |= HDD_ResetStatistics(NULL, NULL);

    HDD_CPUPartition_p    = CPUPartition_p;
    HDD_NCachePartition_p = NCachePartition_p;

    return (RetErr ? FALSE : TRUE);
} /* end of HDD_RegisterCmd() */



#if defined(ST_5516) || defined(ST_5517) || defined(ST_51xx_Device) || defined(ST_7710) || defined(ST_7100) || defined(ST_7109)
/* Error reporting not quite ideal (it or's all the errors together),
 * but if one fails the rest are likely to anyway...
 */
#if 0
static ST_ErrorCode_t DoCfg(void)
{
    ST_ErrorCode_t      error = ST_NO_ERROR;
#if !defined(ST_51xx_Device) && !defined(ST_7710) && !defined(ST_7100) && !defined(ST_7109)
    STCFG_Handle_t      CfgHandle;

    error |= STCFG_Open("cfg", NULL, &CfgHandle);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Error opening STCFG\n"));
    }

    error |= STCFG_CallCmd(CfgHandle, STCFG_CMD_HDDI_ENABLE);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Error enabling HDDI\n"));
    }
    error |= STCFG_CallCmd(CfgHandle, STCFG_CMD_EXTINT_SET_DIRECTION,
                          STCFG_EXTINT_1, STCFG_EXTINT_INPUT);
    if (error != ST_NO_ERROR)
    {
        STTBX_Print(("Error enabling interrupt\n"));
    }

    STCFG_Close(CfgHandle);
#elif defined(ST_51xx_Device)
    /* Ugly, but easy... */

    #define FMICONFIG_BASE_ADDRESS         0x20200000
    #define FMI_GENCFG                     (FMICONFIG_BASE_ADDRESS + 0x28)
    #define FMICONFIGDATA0_BANK0           (FMICONFIG_BASE_ADDRESS + 0x100)
    #define FMICONFIGDATA1_BANK0           (FMICONFIG_BASE_ADDRESS + 0x108)
    #define FMICONFIGDATA2_BANK0           (FMICONFIG_BASE_ADDRESS + 0x110)
    #define FMICONFIGDATA3_BANK0           (FMICONFIG_BASE_ADDRESS + 0x118)

    volatile U32 *regs;
    volatile U8 *regs_8;

    /* Enable HDDI on FMI is done by an EPLD register (EPLD in Bank 1) */
    regs_8 = (volatile U8 *)0x41a00000;
    *regs_8 |= 0x01;

    /* Configuring ATAPI is done using FMI_GENCFG bits 30 (bank0) and 31
     * (bank1). Due to a bug on Cut1.0 of 5100, bit 31 actually
     * configures both. We'll write both 30 and 31 anyway, so we don't
     * have to care which cut we're on.
     */
    regs = (volatile U32 *)FMI_GENCFG;
    *regs = 0xc0000000;

    /* 3- You need to update the ATAPI registers and Interrupt number
     * (as in stbgr-prj-vali5100\boot\interrupt.c and
     * stbgr-prj-vali5100\include\sti5100\sti5100.h) as i did it on 5100
     * validation VOB. */

    /* Configure bank0 for ATAPI */
    regs = (volatile U32 *)FMICONFIGDATA0_BANK0;
    *regs = 0x00540791;
    regs = (volatile U32 *)FMICONFIGDATA1_BANK0;
    *regs = 0x8E003321;
    regs = (volatile U32 *)FMICONFIGDATA2_BANK0;
    *regs = 0x8E003321;
    regs = (volatile U32 *)FMICONFIGDATA3_BANK0;
    *regs = 0x0000000a;
    /* defined(ST_51xx_Device) */
#else
#if defined (mb411) && !defined (SATA_SUPPORTED)

#define FMICONFIG_BASE_ADDRESS     0xBA100000
#define FMI_GENCFG                 (FMICONFIG_BASE_ADDRESS + 0x28)
#define FMICONFIGDATA0_BANK0       (FMICONFIG_BASE_ADDRESS + 0x1C0)
#define FMICONFIGDATA1_BANK0       (FMICONFIG_BASE_ADDRESS + 0x1C8)
#define FMICONFIGDATA2_BANK0       (FMICONFIG_BASE_ADDRESS + 0x1D0)
#define FMICONFIGDATA3_BANK0       (FMICONFIG_BASE_ADDRESS + 0x1D8)

    volatile U8 *regs_8;
    U32 config = 0x00;

    /* Enable ATAPI on EPLD */
    *(DU8 *)0xA3900000 = 0x01;
    while (*(DU8 *)0xA3900000!=0x01) *(DU8 *)0xA3900000=0x01;
    /* Enable ATAPI Interrupt - Set bit 7 of ITM 0 */
    *(DU8 *)0xA3100000 = 0x80;
    /* Setup PIO4 interface */
    /* Please note that this configuration is only working with MAXTOR Hdd */
    *((DU32 *)0xBA1001C0) =  0x00021791; /*0x00021791;*/
    *((DU32 *)0xBA1001C8) =  0x0A004241; /*0x08004141;*/
    *((DU32 *)0xBA1001D0) =  0x0A004241; /*0x08004141;*/
    *((DU32 *)0xBA1001D8) =  0x00000000;

    /* Write EMI_GEN_CFG */
    STSYS_WriteRegDev32LE(((U32 *)FMI_GENCFG), 0x10);
    config = STSYS_ReadRegDev32LE(((U32 *)FMI_GENCFG));
    printf("EMI_GEN_CFG Programmed to %x\n", config);

   /* Optimised PIO Mode 4 timing on Bank 3 */
   *(volatile U32 *)FMICONFIGDATA0_BANK0 = 0x00021791;
   *(volatile U32 *)FMICONFIGDATA1_BANK0 = 0x08004141;
   *(volatile U32 *)FMICONFIGDATA2_BANK0 = 0x08004141;
   *(volatile U32 *)FMICONFIGDATA3_BANK0 = 0x00000000;

    /* Enable HDD on FMI is done by an EPLD register */
    regs_8 = (volatile U8 *)0xA3900000;
    *regs_8 |= 0x01;

    while (*regs_8 != 0x01)
        *regs_8=0x01;
#if defined (ATAPI_USING_INTERRUPTS)
    /* Enable ATAPI Interrupt - Set bit 7 of ITM 0 */
    *(DU8 *)0xA3100000 = 0x80;
#endif

#else   /* 411 & !SATA */

    #define EMI_BANK3_DATA0                 (EMI_BASE_ADDRESS + 0x1c0)
    #define EMI_BANK3_DATA1                 (EMI_BASE_ADDRESS + 0x1c8)
    #define EMI_BANK3_DATA2                 (EMI_BASE_ADDRESS + 0x1d0)
    #define EMI_BANK3_DATA3                 (EMI_BASE_ADDRESS + 0x1d8)

    /* Enable HDDI on FMI is done by an EPLD register */

#if !defined (ST_7100) && !defined (ST_7109)
    volatile U8 *regs_8;
    regs_8 = (volatile U8 *)EPLD_BASE_ADDRESS + EPLD_ATAPI_OFFSET;
    *regs_8 |= 0x01;
#endif /* !defined ST_7100 */

   /* Optimised PIO Mode 4 timing */
   *(volatile U32 *)EMI_BANK3_DATA0 = 0x00118791;
   *(volatile U32 *)EMI_BANK3_DATA1 = 0x8a212100;
   *(volatile U32 *)EMI_BANK3_DATA2 = 0x8a212100;
   *(volatile U32 *)EMI_BANK3_DATA3 = 0x0000000a;

 /* defined(ST_7710) */
#endif

#endif
    return error;
}
#endif  /* 0 */
#endif

/* ------------------------------- End of file ---------------------------- */

