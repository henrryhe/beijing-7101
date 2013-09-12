/************************************************************************
COPYRIGHT (C) STMicroelectronics 2006

File name   : vid_util.c
Description : Definition of video extra commands (utilities)
 *            in order to test the features of STVID API 3.0.0 or later
 *            with Testtool
 *
Note        : All functions return TRUE if error for Testtool compatibility
 *
Date          Modification                                      Initials
----          ------------                                      --------
14 Aug 2000   Creation (adaptation of previous vid_test.c)         FQ
22 Jan 2002   Debug VID_TestSynchro() function. Now fine always.   HG
04 Apr 2003   Suppress VID_PIDChangeTest command no more used      CL
09 Apr 2003   Added db573 (7020 STEM) support                      CL
18 Jan 2005   OS21 porting                                         MH
01 Feb 2005   Move video injection management in vid_inj.c         FQ
03 May 2005   Porting to Linux                                     CD
************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

#if defined (ST_7020) && (defined(mb382) || defined(mb314) || defined(mb376))
#define db573 /* clearer #define for below */
#endif

#if defined (mb290) || defined (db573)
/* As stdevice.h includes st5514.h or st5517.h and st7020.h, force defines for 7020 */
#define USE_7020_GENERIC_ADDRESSES
#endif

/* Defining the variable below allows to run video injection without a PTI, using C loops.
   This is usefull when wanting no dependency on STPTI, STPTI4, etc. */
/*#define STPTI_UNAVAILABLE*/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "testcfg.h"
#ifdef VALID_TOOLS
    #include "msglog.h"
#endif /* VALID_TOOLS */

#include "stos.h"

#ifdef ST_OSLINUX
#if !defined DVD_TRANSPORT_STPTI
#define DVD_TRANSPORT_STPTI
#endif
#undef DVD_TRANSPORT_PTI

#else

#include "stlite.h"
#endif  /* ST_OSLINUX */

/* Driver as to be used as backend */
#define USE_AS_BACKEND
#include "genadd.h"
#include "stddefs.h"
#include "stdevice.h"

#ifdef ST_OS20
#include "debug.h"
#endif /* ST_OS20 */

#if defined (ST_OS21) && !defined (ST_OSWINCE)
#include <unistd.h>     /* read() functions */
#include "os21debug.h"
#include <sys/stat.h>
#include <fcntl.h>
#endif /* ST_OS21 */

#include "sttbx.h"

#ifdef USE_COLOR_TRACES
/* Print STTBX message in red using ANSI Escape code */
/* Need a terminal supporting escape codes           */
#define STTBX_ErrorPrint(x)  \
{ \
    STTBX_Print(("\033[31m")); \
    STTBX_Print(x); \
    STTBX_Print(("\033[0m")); \
}
#else
#define STTBX_ErrorPrint(x)  STTBX_Print(x)
#endif

#include "testtool.h"
#include "api_cmd.h"
#include "clevt.h"
#include "startup.h"

#if (defined (DVD_TRANSPORT_STPTI) || defined (DVD_TRANSPORT_STPTI4)) && !defined(STPTI_UNAVAILABLE)
#include "stpti.h"
#endif

#if (defined (DVD_TRANSPORT_PTI) || defined (DVD_TRANSPORT_LINK)) && !defined(STPTI_UNAVAILABLE)
#include "pti.h"
#endif

#ifdef DVD_TRANSPORT_LINK /* workaround for link 2.0.1 */
#include "ptilink.h"
#endif

#ifdef USE_DEMUX
#include "stdemux.h"
#endif

#include "stavmem.h"
#include "stsys.h"
#include "stevt.h"

#ifdef USE_CLKRV
#include "stclkrv.h"
#include "clclkrv.h"
#else
#include "stcommon.h" /* for ST_GetClocksPerSecond() */
#endif /* USE_CLKRV */

#include "stpwm.h"
#include "stvtg.h"
#include "vid_cmd.h"
#include "vid_inj.h"
#include "vid_util.h"
#include "vtg_cmd.h"

#ifdef VIDEO_TEST_HARNESS_BUILD
#include "vid_still.h"
#endif

#if defined (mb290) /* allow avmem_reinit() on atv2 */
 #include "clboot.h"
 #ifdef USE_GPDMA
 #include "clgpdma.h"
 #endif /*#ifdef USE_GPDMA*/
#endif /* mb290 */

#if !defined ST_OSLINUX
#include "clavmem.h" /* needed for MAX_PARTITIONS */
#endif

#ifdef TRACE_UART
#include "trace.h"
#endif

#if defined(USE_OSPLUS_FOR_INJECT) || defined(USE_OSPLUS)
 #include "filesystem.h"
#endif /* USE_OSPLUS_FOR_INJECT */

#define DELTATOP_MME_VERSION 10
#include "DeltaTop_TransformerTypes.h"
#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
#include <mme.h>
#endif /* defined(7100) || defined(7109) || defined(7200) */
#define PING_TIME_OUT   32


#define DeviceToVirtual(DevAdd, Mapping) (void*)((U32)(DevAdd)\
        -(U32)((Mapping).PhysicalAddressSeenFromDevice_p)\
        +(U32)((Mapping).VirtualBaseAddress_p))

/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Constants (default values) -------------------------------------- */
#ifndef ARCHITECTURE_SPARC
#if defined(ST_7710) || defined(ST_5105) || defined(ST_5107)  || defined(ST_5188) || defined(ST_7100) || defined(ST_7109) || defined(ST_ZEUS) || defined(ST_7200)
/* Injection task priority increased to be able to inject properly in HD mode !!!				*/
#define VID_INJECTION_TASK_PRIORITY (MIN_USER_PRIORITY+1) 	/* Stapidemo is MIN_USER_PRIORITY+8 */
#else
#define VID_INJECTION_TASK_PRIORITY (MIN_USER_PRIORITY) 	/* Stapidemo is MIN_USER_PRIORITY+8 */
#endif
#else
#define VID_INJECTION_TASK_PRIORITY    18
#endif

#define NB_MAX_OF_COPIED_PICTURES  2         /* pictures in memory for show/hide tests */

#define NB_OF_VIDEO_EVENTS         (STVID_NEW_CRC_QUEUED_EVT-STVID_DRIVER_BASE) /* need for LATEST event name check in stvid.h each time stvid.h is altered */

#define NB_MAX_OF_STACKED_EVENTS   120

#define STRING_DEVICE_LENGTH 80

#define LUMA_WORD_FOR_BLACK_COLOR       0x10101010
#define CHROMA_WORD_FOR_BLACK_COLOR     0x80808080


#ifdef STVID_VALID

#if defined(FILENAME_MAX) && defined(ST_OS20)
    #undef FILENAME_MAX
    #define FILENAME_MAX 1024
#endif

#ifdef VALID_TOOLS
#define STVID_MAX_LOG_FILE_SIZE 100000
#endif /* VALID_TOOLS */

#endif /* STVID_VALID */


#if defined(STVID_USE_CRC) || defined(STVID_VALID)

#define LX_VERSION_PREFIX "VideoLX"
#define LX_VERSION_PREFIX_SIZE (strlen(LX_VERSION_PREFIX))

#define MAX_REFCRC_LINE     64

#ifndef STVID_TASK_STACK_SIZE_CRC
 #define STVID_TASK_STACK_SIZE_CRC         16384
#endif
#ifndef STVID_TASK_PRIORITY_CRC
 #define STVID_TASK_PRIORITY_CRC MIN_USER_PRIORITY+1
#endif

/* Max allowed preproc/decode time in us */
#if defined(ST_7100) || (defined(ST_7109) && defined(ST_CUT_1_X))
    #define MAX_ALLOWED_PREPROC_TIME 66666  /* deltaphi/deltamu_blue with 2 PP) */
#else /*if defined(ST_7100) || (defined(ST_7109) && defined(ST_CUT_1_X))*/
    #define MAX_ALLOWED_PREPROC_TIME 33333  /* deltamu_green with 1 PP) */
#endif /*if defined(ST_7100) || (defined(ST_7109) && defined(ST_CUT_1_X))*/

#define MAX_ALLOWED_DECODE_TIME 33333
#define MEAN_LX_HOST_OVERHEAD     800
#define MAX_LX_HOST_OVERHEAD     2000
#define MAX_ALLOWED_LX_DECODE_TIME (MAX_ALLOWED_DECODE_TIME-MEAN_LX_HOST_OVERHEAD)

#endif /* STVID_USE_CRC || STVID_VALID */

#if defined STUB_INJECT && !defined ST_ZEUS
#ifdef ST_OSLINUX
extern ST_ErrorCode_t StubInject_GetBBInfo(void ** BaseAddress_pp, U32 * Size_p);
extern ST_ErrorCode_t StubInject_SetStreamSize(U32 Size);
#endif /* ST_OSLINUX */
#endif /* STUB_INJECT && !defined ST_ZEUS */

#define DELAY_1S                    1000000     /* Expressed in microseconds */

/* --- Global variables ------------------------------------------------ */

/* Variables used with STVID_Clear */
static U8 LumaPattern[256];
static U8 ChromaPattern[256];

static STAVMEM_BlockHandle_t  CopiedPictureHandle[NB_MAX_OF_COPIED_PICTURES];
static void *  CopiedPictureAddress[NB_MAX_OF_COPIED_PICTURES];

static char VID_Msg[1024];                             /* text for trace */

static BOOL EventLoggingEnabled = FALSE;
static U32 TicksPerOneSecond;

extern VID_Injection_t VID_Injection[VIDEO_MAX_DEVICE];
extern STVID_Handle_t  DefaultVideoHandle;

VID_BufferLoad_t VID_BufferLoad[VID_MAX_BUFFER_LOAD];   /* address of stream buffer */

/* Events */
typedef union
{
    STVID_PictureInfos_t        PictInfo;
    STVID_PictureParams_t       PictParam;
    STVID_SequenceInfo_t        SeqInfo;
    STVID_DisplayAspectRatio_t  DisplayAR;
    STVID_UserData_t            UserData;
    STVID_SynchroInfo_t         SyncInfo;
    STVID_SpeedDriftThreshold_t ThresholdInfo;
    STVID_DataUnderflow_t       DataUnderflow;
    STVID_MemoryProfile_t       MemoryProfile;
    STVID_SynchronisationInfo_t SynchroCheckInfo;
#ifdef STVID_HARDWARE_ERROR_EVENT
    STVID_HardwareError_t       HardwareError;
#endif
} EventData_t;

typedef struct
{
    long EvtNum;          /* event number */
    osclock_t EvtTime;      /* time of event detection */
    EventData_t EvtData;  /* associated data */
} RcvEventInfo_t ;

extern STEVT_Handle_t  EvtSubHandle;

static STEVT_EventID_t EventID[NB_OF_VIDEO_EVENTS+1]; /* event IDs (one ID per event number) */
static RcvEventInfo_t  RcvEventStack[NB_MAX_OF_STACKED_EVENTS+1]; /* list of event numbers occured */
static S32             RcvWrongEventStack[NB_MAX_OF_STACKED_EVENTS+1]; /* list of wrong event numbers occured */
static char            RcvUserDataStack[NB_MAX_OF_STACKED_EVENTS+1][VID_MAX_USER_DATA]; /* list of user data received */
static ST_DeviceName_t EventVTGDevice;
static ST_DeviceName_t EventVIDDevice;

static U32 ReceiveNewFrameEvtCount;   /* event counting for VID_ReceiveEvtForTimeMeasurement() */
static U32 VSyncCount;                /* VSync event counting for VID_ReceiveVSync() */
static U32 VSyncCountForDisplay;      /* VSync counter when a new pict. is to displayed */

short RcvEventCnt[NB_OF_VIDEO_EVENTS+1];             /* events counting (one count per event number)*/
static short NbOfRcvEvents=0;                        /* nb of events in stack */
static short NewRcvEvtIndex=0;                       /* top of the stack */
static short LastRcvEvtIndex=0;                      /* bottom of the stack */
static BOOL EvtOpenAlreadyDone=FALSE;              /* flag */
static BOOL EvtEnableAlreadyDone=FALSE;            /* flag */

/* Counting with VID_TestPictDelay() : */
static U32 Pict_Counted;
static S32 Pict_Delayed;
static S32 Pict_Advanced;
static U32 Pict_StartTime;
static U32 Pict_PreviousTime;
static U32 Pict_LastTime;
static S32 Pict_AdvanceTime;
static S32 Pict_DelayTime;
static S32 Pict_FrameDelayed;
static S32 Pict_BiggerDelayTime;
static S32 Pict_FrameDuration;

/* Counting with VID_ReceivePictTimeCode() : */
static U32 Pict_FrameNb;
static U32 Pict_Difference;
static U32 Pict_MinDifference;
static U32 Pict_MaxDifference;
static U32 Pict_TotalOfDifferences;
static BOOL Pict_DisplayIsFluent;
static BOOL Pict_NbOfNegativeDiff;
static U32 Pict_NbOfNewRef;

/* Counting with VID_ReceivePictAndVSync() and VID_TestNbVSync() : */
static S32 NbMaxOfVSyncToLog;
static U32 *VSyncLog;
static BOOL NbOfVSyncIsConstant;
static U32 NbOfVSyncBetween2Pict;
static U32 NbMaxOfVSyncBetween2Pict;
static U32 PreviousPictureVSyncCount;
static U32 TotalOfVSync;
static S32 NbOfNewPict;

static enum
{
    TM_NO_ACTIVITY,                 /* The automaton of time measurement is inactive  */
    TM_ARMED,                       /* The automaton is activated, and is waiting for its first event */
    TM_ASK_IF_IN_PROGRESS,          /* The automaton is aked for any activity (end test) */
    TM_IN_PROGRESS                  /* The automaton still receive event */
} TimeMeasurementState;

static osclock_t InitialTime;         /* Time's safe of the firts occurance of the event */
static osclock_t IntermediaryTime;    /* Current time of the event */
static S32     TimeOutEvent;        /* Time out of waiting an event occurs */

typedef struct {
    void *                              StartAddress_p;
    U32                                 BufferSize;
    U32                                 LastFilledTransferSize;
    STAVMEM_BlockHandle_t               AvmemBlockHandle;
} VIDTRICK_BackwardBuffer_t;

VIDTRICK_BackwardBuffer_t BufferA;

/* Synchro Test */
#ifdef USE_CLKRV
static STCLKRV_Handle_t CLKRVHndl0;
static semaphore_t      *SynchroLostSemaphore_p;
static S32              SynchroDelta;
static BOOL             AutoSynchro;
#endif /* USE_CLKRV */
static U32 BackSync;
static U32 OutSync;
static U32 DeltaPos;
static U32 DeltaNeg;
#ifdef USE_CLKRV
static U32 SynchroLost;
static U32 BackToSync;
static U32 LoosingSync;
static U32 SyncOk;
static U32 SyncNOk;
static U32 StandardDeviationSquared;
static U32 SquareAverage;
static U32 SyncInfoOccurences;
static U32 TotalLoosingSync;
static S32 DeltaClock;
static S32 DeltaSynchro;
static U32 PreviousSTCBaseValue;
static BOOL             StreamProblemOccured;
static S32              VSyncDelta;  /* duration between 2 VSync */
static S32              SynchroMaxDelta; /* max. advance/delay allowed for synchro */
static S32              PicturePTS;
static S32              NbOfPTSInterpolated;
#endif /* USE_CLKRV */

/* PTS Inconsistency Test */
static semaphore_t      *PTSSemaphore_p;
static U32              NumberOfPTSInconsistency = 0;
static STVID_PTS_t      CurrentPTS, LastPTS, PTSFoundInTheEvent;
static BOOL             NewPTS = FALSE;

/* Start Stop test */
static semaphore_t      *StartStopSemaphore_p;

/* Variable necessary for the test of Error Recovery */
static STVID_PictureBufferHandle_t HandleOfLastPictureBufferDisplayed;

/* Picture Params Test */
static U32              ParamTestResult;
static semaphore_t      *PictParamTestFlag_p;
#ifdef USE_CLKRV
static U32 DisynchroValue;  /* in milli-seconds */
#endif

#ifndef ARCHITECTURE_SPARC
static U8 vid_DummyRead;
#endif /* not ARCHITECTURE_SPARC */

VID_ExternalBitBuffer_t GlobalExternalBitBuffer = {FALSE, STAVMEM_INVALID_BLOCK_HANDLE, 0, 0, 0};

#if defined(STVID_USE_CRC) || defined(STVID_VALID)
VID_CRC_t VID_CRC[VIDEO_MAX_DEVICE];
static int CRCCount = 0;
#endif /* defined(STVID_USE_CRC) || defined(STVID_VALID) */

/* --- Externals ------------------------------------------------------- */

extern STVID_ViewPortHandle_t  ViewPortHndl0;       /* handle for viewport */

#ifndef ARCHITECTURE_SPARC
#if !defined(mb411) && !defined (mb8kg5) && !defined(mb519)
extern ST_Partition_t *DataPartition_p;
#endif /* !(mb411) && !(mb8kg5) && !(mb519) */
#else
ST_Partition_t *DataPartition_p;
#endif

#if defined(mb295)
extern ST_Partition_t *Data2Partition_p;
#endif

extern S16  NbOfCopiedPictures;
extern STVID_PictureParams_t CopiedPictureParams[NB_MAX_OF_COPIED_PICTURES];
STVID_PictureInfos_t  PictInfos;

#ifdef ARCHITECTURE_SPARC
extern void STWRAP_Big2Little(void *memPointer, ui32 size);
#endif

#ifdef ST_OSWINCE
extern void TestPerf(void);
#endif

#ifdef ST_OSLINUX
extern void * STAPIGAT_UserToKernel(void * VirtUserAddress_p);
extern ST_ErrorCode_t STAPIGAT_AllocData(U32 Size, U32 Alignment, void ** Address_p);
extern ST_ErrorCode_t STAPIGAT_FreeData(void * Address_p);
#endif



static BOOL VID_TraceEvtData(S16);
BOOL VID_GetEventNumber(STTST_Parse_t *pars_p, STEVT_EventConstant_t *EventNumber);
void VID_PictureInfosToPictureParams(const STVID_PictureInfos_t * const PictureInfos_p, STVID_PictureParams_t * const Params_p);
void SynchroPrintEvent(void);


/*#######################################################################*/
/*########################## EXTRA COMMANDS #############################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : VID_GetHandleFromName
 *
 * Input    : Video device name
 * Parameter:
 * Output   : Handle associated to the passed name
 * Return   : FALSE if DeviceName found, TRUE else.
 * ----------------------------------------------------------------------*/
static BOOL VID_GetHandleFromName(ST_DeviceName_t DeviceName, STVID_Handle_t *CurrentHandle)
{
    BOOL HandleFound;
    S32 LVar;

    HandleFound = FALSE;
    LVar = 0;
    while ((LVar<=VIDEO_MAX_DEVICE) && (!HandleFound))
    {
        if ( VID_Inj_IsDeviceNameIdentical(LVar,DeviceName)==TRUE )
        {
            HandleFound = TRUE;
            *CurrentHandle = VID_Inj_GetDriverHandle(LVar);
        }
        else
        {
            LVar ++;
        }
    }
    if (!HandleFound)
    {
        STTBX_Print(("Warning: %s not found in Table for handle-device name association !!!\n", DeviceName));
    }
    return(!HandleFound);
}


#if defined(STVID_USE_CRC) || defined(STVID_VALID)
static U32 VID_GetSTVIDNumberFromHandle(STVID_Handle_t VideoHandle)
{
    U32 InjectNumber;

    InjectNumber = 0;
    while((InjectNumber < VIDEO_MAX_DEVICE) &&
          (VID_Inj_GetDriverHandle(InjectNumber) != VideoHandle))
    {
        InjectNumber++;
    }
    return(InjectNumber);
}
#endif
/*-------------------------------------------------------------------------
 * Function : VID_Load
 *            Load a mpeg file in memory
 * Input    : *pars_p, *result_sym_p
 * Output   : FALSE if no error
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_Load(STTST_Parse_t *pars_p, char *result_sym_p )
{
    char FileName[VID_MAX_STREAM_NAME];
    BOOL RetErr;
    S32 LVar;
    void * AllocatedBuffer_p = NULL;
#if defined(STUB_INJECT) || defined(ST_ZEUS)
    S32 LVar2;
    U32 BufferSize;
	ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;
#endif /*  defined(STUB_INJECT) || (defined(ST_ZEUS) */

#ifdef USE_OSPLUS_FOR_INJECT
    void *FileDescriptor;
#else /* #ifdef USE_OSPLUS_FOR_INJECT */
#ifdef ST_OS21
    struct stat FileStatus;
#endif /* #ifdef ST_OS21 */
#ifndef ARCHITECTURE_SPARC
  #if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
      FILE *FileDescriptor;
  #else /* #if defined(ST_ZEUS) && !defined(USE_ST20_DCU) */
    long int FileDescriptor;
  #endif /*defined(ST_ZEUS) && !defined(USE_ST20_DCU)*/
    long int FileSize;
    BOOL Truncated;
    ST_Partition_t* Partition_p = NULL;
    U8 *Ptr;
#else /* #ifndef ARCHITECTURE_SPARC */
    volatile long int FileDescriptor;
#endif /* #ifndef ARCHITECTURE_SPARC */
#endif /* not USE_OSPLUS_FOR_INJECT */

#ifdef ST_OSLINUX
	FILE *pf;
#endif

#if defined(mb411) || defined(mb8kg5) || defined(mb519)
#if !defined(ST_OSLINUX) && !defined(ST_ZEUS) && !defined(STUB_INJECT)
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
#endif /* #if !defined(ST_OSLINUX) && !(defined(ST_ZEUS) && !defined(STUB_INJECT) */
#endif /* #if defined(mb411) || defined(mb8kg5) || defined(mb519)*/

    /* get file name */
    memset( FileName, 0, sizeof(FileName));
    RetErr = STTST_GetString( pars_p, "", FileName, sizeof(FileName) );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected FileName" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 1, &LVar);
    if (( RetErr ) || (LVar < 1) || (LVar > VID_MAX_BUFFER_LOAD))
    {
        sprintf(VID_Msg, "expected load buffer number (default is 1. max is %d)", VID_MAX_BUFFER_LOAD);
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }
#if defined(ST_ZEUS)
    RetErr = STTST_GetInteger( pars_p, 0, &LVar2);
    if (( RetErr ) || (LVar2 == 0))
    {
        sprintf(VID_Msg, "expected bit buffer size");
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }
    BufferSize = (U32)LVar2;

    RetErr = STTST_GetInteger( pars_p, 0, &LVar2);
    if (( RetErr ) || (LVar2 == 0))
    {
        sprintf(VID_Msg, "expected bit buffer address");
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }

    AllocatedBuffer_p = (void *)LVar2;
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );
    VID_BufferLoad[LVar-1].AllocatedBuffer_p = STAVMEM_DeviceToCPU(AllocatedBuffer_p, &VirtualMapping);
    VID_BufferLoad[LVar-1].AlignedBuffer_p = STAVMEM_VirtualToCPU(AllocatedBuffer_p, &VirtualMapping);
    AllocatedBuffer_p = VID_BufferLoad[LVar-1].AlignedBuffer_p;
    memset(VID_BufferLoad[LVar-1].AlignedBuffer_p,0x00,BufferSize);

#endif /*#if defined(ST_ZEUS)*/

#if defined STUB_INJECT && !defined ST_ZEUS
#ifdef ST_OSLINUX
    LVar2 = LVar2;  /* To avoid warnings */
    if (StubInject_GetBBInfo(&AllocatedBuffer_p, &BufferSize) != ST_NO_ERROR)
    {
        sprintf(VID_Msg, "Cannot retrieve BB info");
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }
    printf("AllocatedBuffer_p: 0x%8x, BufferSize: %d\n", AllocatedBuffer_p, BufferSize);

    ErrorCode = STAPIGAT_Ioctl_Map((U32) AllocatedBuffer_p, (U32)BufferSize);
    if (ErrorCode != ST_NO_ERROR)
    {
        printf("%s: stapigat mapping problem !!!\n", __FUNCTION__);
        RetErr = TRUE;
    }
    else
    {
        /* Map the address in the User space */
        VID_BufferLoad[LVar-1].AllocatedBuffer_p = (void *) STAPIGAT_MMap((U32) AllocatedBuffer_p, (U32)BufferSize);
        if (VID_BufferLoad[LVar-1].AllocatedBuffer_p == (void *)(-1))
        {
            /* unmapping */
            STAPIGAT_Ioctl_Unmap((U32) AllocatedBuffer_p, (U32) BufferSize);
            RetErr = TRUE;
            printf("INJVID mapping Failed !\n");
        }
        else
        {
            printf("InjVid mapping at %x from %x to %x\n",
                                    VID_BufferLoad[LVar-1].AllocatedBuffer_p,
                                    AllocatedBuffer_p,
                                    AllocatedBuffer_p + BufferSize);
        }
    }

    AllocatedBuffer_p = VID_BufferLoad[LVar-1].AllocatedBuffer_p;
#else /* ST_OSLINUX */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar2);
    if (( RetErr ) || (LVar2 == 0))
    {
        sprintf(VID_Msg, "expected bit buffer size");
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }
    BufferSize = (U32)LVar2;

    RetErr = STTST_GetInteger( pars_p, 0, &LVar2);
    if (( RetErr ) || (LVar2 == 0))
    {
        sprintf(VID_Msg, "expected bit buffer address");
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }

    AllocatedBuffer_p = (void *)LVar2;
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );
    VID_BufferLoad[LVar-1].AllocatedBuffer_p = STAVMEM_DeviceToCPU(AllocatedBuffer_p, &VirtualMapping);
    AllocatedBuffer_p = VID_BufferLoad[LVar-1].AllocatedBuffer_p;
#endif  /* !ST_OSLINUX */
#endif /* #if defined STUB_INJECT && !defined ST_ZEUS */

    if(!strcmp(VID_BufferLoad[LVar-1].StreamName, FileName))
    {
        sprintf(VID_Msg, "file [%s] already loaded in load buffer %d\n", FileName, LVar);
        STTBX_Print((VID_Msg));
        return(FALSE);
    }

    if(VID_BufferLoad[LVar-1].UsedCounter != 0)
    {
        STTBX_Print(( "Buffer cannot be loaded because already in used !!\n" ));
        return(TRUE);
    }

#if !defined STUB_INJECT && !defined(ST_ZEUS)
    if (VID_BufferLoad[LVar-1].AllocatedBuffer_p != NULL)
    {
#if defined (mb411) || defined(mb519)
#ifdef ST_OSLINUX
        /* Just to be compatible with vidutil_Deallocate() prototype */
        STAVMEM_PartitionHandle_t   AvmemPartitionHandle[1] = {0};
#endif /* #ifdef ST_OSLINUX */
        vidutil_Deallocate(AvmemPartitionHandle[0], &(VID_BufferLoad[LVar-1].BlockHandle), VID_BufferLoad[LVar-1].AllocatedBuffer_p);
#else /* defined (mb411)|| defined(mb519) */
        STOS_MemoryDeallocate(VID_BufferLoad[LVar-1].Partition_p, VID_BufferLoad[LVar-1].AllocatedBuffer_p);
#endif /* defined (mb411)|| defined(mb519) */
        /* remark : no result is returned by this void function */
        VID_BufferLoad[LVar-1].AllocatedBuffer_p = NULL;
    }
#endif /* #if !defined STUB_INJECT && !defined(ST_ZEUS) */


    sprintf(VID_Msg, "Looking for %s ... ", FileName);
    STTBX_Print((VID_Msg));

    /* Get memory to store file data */
#ifdef USE_OSPLUS_FOR_INJECT
    FileDescriptor = SYS_FOpen(FileName,"rb");
#else /* #ifdef USE_OSPLUS_FOR_INJECT */

#ifndef ARCHITECTURE_SPARC

 #ifdef ST_OS20
    #if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
      FileDescriptor = (FILE *)Memfopen(FileName, "rb" );
    #else /*defined(ST_ZEUS) && !defined(USE_ST20_DCU)*/
        #if defined(ST_ZEUS) && defined(USE_ST20_DCU)
            FileDescriptor = debugopen(FileName, "rb" );
        #else /* defined(ST_ZEUS) && defined(USE_ST20_DCU) */
            FileDescriptor = debugopen(FileName, "rb" );
        #endif /* defined(ST_ZEUS) && defined(USE_ST20_DCU) */
    #endif /*defined(ST_ZEUS) && !defined(USE_ST20_DCU)*/
 #endif /* #ifdef ST_OS20 */

#ifdef ST_OS21
#ifdef NATIVE_CORE
    FileDescriptor = open(FileName, O_RDONLY );
#else /* NATIVE_CORE */
    FileDescriptor = open(FileName, O_RDONLY | O_BINARY);
#endif /* NATIVE_CORE */
#endif /* ST_OS21 */


 #ifdef ST_OSLINUX
    pf = fopen(FileName,"rb");
    if (pf != NULL)
	{
		/* File open has no error */
		FileDescriptor = fileno(pf);
	}
	else
	{
		printf ("File open error...\n");
		FileDescriptor = -1;
	}
 #endif /* ST_OSLINUX */
#else /* #ifndef ARCHITECTURE_SPARC */
    FileDescriptor = debugopen(FileName, "r" );
#endif /* #ifndef ARCHITECTURE_SPARC */

#endif /* #ifdef USE_OSPLUS_FOR_INJECT */

#ifdef USE_OSPLUS_FOR_INJECT
    if (FileDescriptor == NULL)
#else /* #ifdef USE_OSPLUS_FOR_INJECT */
    #if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
    if (FileDescriptor == NULL)
    #else
        if (FileDescriptor < 0 )
    #endif /*defined(ST_ZEUS) && !defined(USE_ST20_DCU)*/
#endif /* #ifdef USE_OSPLUS_FOR_INJECT */

    {
        sprintf(VID_Msg, "<<< ERROR : Unable to open %s ! >>>\n", FileName);
        STTBX_Print((VID_Msg));
        return (API_EnableError ? TRUE : FALSE );
    }

    STTBX_Print(("loading in buffer no %d ...\n", LVar));

#ifdef USE_OSPLUS_FOR_INJECT
    SYS_FSeek(FileDescriptor,0,SEEK_END);
    FileSize = SYS_FTell(FileDescriptor);
    SYS_FSeek(FileDescriptor,0,SEEK_SET);
#else /* #ifdef USE_OSPLUS_FOR_INJECT */
#ifdef ST_OS20
    #if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
      FileSize = Memfilesize(FileDescriptor);
    #else
      FileSize = debugfilesize(FileDescriptor);
    #endif /*defined(ST_ZEUS) && !defined(USE_ST20_DCU)*/
#endif /* ST_OS20 */

#ifdef ST_OS21
    fstat(FileDescriptor, &FileStatus);
    FileSize = FileStatus.st_size;
#endif /* ST_OS21 */
#endif /* #ifdef USE_OSPLUS_FOR_INJECT */

#if defined(USE_OSPLUS_FOR_INJECT) && !defined(STUB_INJECT)
    if(SYS_FIsOSPlusPath(FileName))
    {
        VID_BufferLoad[LVar-1].AllocatedBuffer_p = NULL;
        VID_BufferLoad[LVar-1].AlignedBuffer_p = NULL;
        VID_BufferLoad[LVar-1].Partition_p = NULL;
#if defined (mb411)
        VID_BufferLoad[LVar-1].BlockHandle = 0;
#endif /* mb411 */
        VID_BufferLoad[LVar-1].NbBytes = (U32) FileSize;
        strcpy(VID_BufferLoad[LVar-1].StreamName, FileName);
        VID_BufferLoad[LVar-1].FileHandle = FileDescriptor;
    }
    else
    {
#endif /* defined(USE_OSPLUS_FOR_INJECT) && !defined(STUB_INJECT) */

#ifdef ST_OSLINUX
    FileSize = debugfilesize(FileDescriptor);
#endif /* #ifdef ST_OSLINUX */

#ifndef ARCHITECTURE_SPARC
    /* ---  allocated buffer will be aligned later. Manage room to round up to a multiple of 16 bytes --- */
    Truncated = FALSE;

#ifdef STUB_INJECT
    if (FileSize > BufferSize)
    {
        Truncated = TRUE;
        FileSize = BufferSize;
    }
#endif /*#ifdef STUB_INJECT*/

#if !defined STUB_INJECT && !defined ST_ZEUS

#if defined(NATIVE_CORE)	/* for VSOC, we limit file size to 6Mb otherwise VID_Start does not succeed */
#define MAX_FILE_SIZE	6*1024*1024
	if (FileSize > MAX_FILE_SIZE)
    {
        Truncated = TRUE;
        FileSize = MAX_FILE_SIZE;
    }
#endif

    while ((FileSize > 0) && (AllocatedBuffer_p == NULL ))
    {
#if defined (mb411) || defined(mb519)
#ifdef ST_OSLINUX
        printf("Detected size: %ld\n", FileSize);
        /* Actually this variable is not used */
        /* Just set to be compatible with vidutil_Allocate() */
        Partition_p = (ST_Partition_t *) 1;
#else /* #ifdef ST_OSLINUX */
        #if defined(DVD_SECURED_CHIP) && defined(ST_7109)
            Partition_p = (ST_Partition_t *) AvmemPartitionHandle[0];   /* Non-secure partition */
        #else
            Partition_p = (ST_Partition_t *) AvmemPartitionHandle[0];
        #endif /* Secure 7109 */
#endif  /* #ifdef ST_OSLINUX  */
        if (vidutil_Allocate((STAVMEM_PartitionHandle_t)Partition_p,
                             (U32)(FileSize),
                             16,
                             &AllocatedBuffer_p,
                             &(VID_BufferLoad[LVar-1].BlockHandle)) != ST_NO_ERROR)
        {
            STTBX_Print(("VID_Load(): Error in vidutil_Allocate !!!\n"));
        }
#else
        AllocatedBuffer_p = (char *)STOS_MemoryAllocate(DataPartition_p, (U32)((FileSize + 15) ));
        Partition_p = DataPartition_p;
#endif /* #if defined (mb411) || defined(mb519) */

#if defined(mb295)
        /* mb295 have two memory bank for CPU                 */
        /* Unable to allocate memory, try the second          */
        if (AllocatedBuffer_p == NULL)
        {
            AllocatedBuffer_p = (char *)STOS_MemoryAllocate(Data2Partition_p, (U32)((FileSize + 15) ));
            Partition_p = Data2Partition_p;
        }
#endif /* mb295 */
        if (AllocatedBuffer_p == NULL)
        {
            Truncated = TRUE;
            FileSize-=200000; /* file too big : truncate the size */
            if ( FileSize < 0 )
            {
                STTST_AssignInteger(result_sym_p, TRUE, FALSE);
                sprintf(VID_Msg, "Not enough memory for file loading\n");
                STTBX_Print((VID_Msg));
                return(API_EnableError ? TRUE : FALSE );
            }
        }
    }
#endif /* #if !defined STUB_INJECT && !defined ST_ZEUS */

#if defined (ST_ZEUS) && !defined(USE_ST20_DCU) && !defined(STUB_INJECT)
        AllocatedBuffer_p = (U32 *)File2MemAddress(FileDescriptor);
        Partition_p = (ST_Partition_t *) AvmemPartitionHandle[0];
#endif

    if (Truncated)
    {
        sprintf(VID_Msg, "Not enough memory for file loading : truncated to %ld \n",FileSize);
        STTBX_Print((VID_Msg));
    }
#else /* #ifndef ARCHITECTURE_SPARC */
    AllocatedBuffer_p = (U32) mem_NewPointer(kMem_VideoHeap,(U32) FileSize);
    STTBX_Print(("total bytes in file <%d>...\n", FileSize));
#endif /* #ifndef ARCHITECTURE_SPARC */

#if !defined(ST_ZEUS)
    /* --- Setup the alligned buffer, load file and return --- */
    VID_BufferLoad[LVar-1].AllocatedBuffer_p = AllocatedBuffer_p;

#ifndef ARCHITECTURE_SPARC
    VID_BufferLoad[LVar-1].Partition_p = Partition_p;

#if defined(mb411) || defined(mb519)
#ifdef ST_OSLINUX
    VID_BufferLoad[LVar-1].AlignedBuffer_p = AllocatedBuffer_p;
#else /* #ifdef ST_OSLINUX */
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
    VID_BufferLoad[LVar-1].AlignedBuffer_p = STAVMEM_VirtualToCPU(AllocatedBuffer_p, &VirtualMapping);
#endif /* #ifdef ST_OSLINUX */
#else /* #if defined(mb411) || defined(mb519) */
    VID_BufferLoad[LVar-1].AlignedBuffer_p =
        (void *)(((unsigned int) VID_BufferLoad[LVar-1].AllocatedBuffer_p));
#endif  /* #if defined(mb411) || defined(mb519) */

#else /* #ifndef ARCHITECTURE_SPARC */
    VID_BufferLoad[LVar-1].Partition_p = DataPartition_p;
    VID_BufferLoad[LVar-1].AlignedBuffer_p =
        (void *)(((unsigned int) VID_BufferLoad[LVar-1].AllocatedBuffer_p + 15) & (~15));
#endif /* #ifndef ARCHITECTURE_SPARC */
#endif /*#if !defined(ST_ZEUS)*/

#if defined(ST_ZEUS) && !defined(STUB_INJECT) && defined(USE_ST20_DCU)
    VID_BufferLoad[LVar-1].NbBytes = (U32) BufferSize;
#else
    VID_BufferLoad[LVar-1].NbBytes = (U32) FileSize;
#endif
    strcpy(VID_BufferLoad[LVar-1].StreamName, FileName);

#if defined(ST200_OS21)
{
    U32 RemainingDataToLoad;
    U32 MaxReadSize = 100*1024;

    /* On ST200, if you try to load big files (>2M) at once, you will have a core dump (I guess a toolset bug). So, here
     * we split reading into packets of 100K */
    RemainingDataToLoad = FileSize;
    while(RemainingDataToLoad != 0)
    {
        if(RemainingDataToLoad > MaxReadSize)
        {
#ifdef USE_OSPLUS_FOR_INJECT
            SYS_FRead((void*)((U32)(VID_BufferLoad[LVar-1].AlignedBuffer_p) + (FileSize - RemainingDataToLoad)),  (size_t) MaxReadSize, 1, FileDescriptor);
#else /* USE_OSPLUS_FOR_INJECT */
            debugread(FileDescriptor, (void*)((U32)(VID_BufferLoad[LVar-1].AlignedBuffer_p) + (FileSize - RemainingDataToLoad)), (size_t) MaxReadSize);
#endif /* not USE_OSPLUS_FOR_INJECT */
            RemainingDataToLoad -= MaxReadSize;
        }
        else
        {
#ifdef USE_OSPLUS_FOR_INJECT
            SYS_FRead((void*)((U32)(VID_BufferLoad[LVar-1].AlignedBuffer_p) + (FileSize - RemainingDataToLoad)),  (size_t) RemainingDataToLoad, 1, FileDescriptor);
#else /* USE_OSPLUS_FOR_INJECT */
            debugread(FileDescriptor, (void*)((U32)(VID_BufferLoad[LVar-1].AlignedBuffer_p) + (FileSize - RemainingDataToLoad)), (size_t) RemainingDataToLoad);
#endif /* not USE_OSPLUS_FOR_INJECT */
            RemainingDataToLoad = 0;
        }
    }
}
#else /* #if defined(ST200_OS21) */
#ifdef USE_OSPLUS_FOR_INJECT
    SYS_FRead(VID_BufferLoad[LVar-1].AlignedBuffer_p,  (size_t) FileSize, 1, FileDescriptor);
#else /* #ifdef USE_OSPLUS_FOR_INJECT */
  #if defined(ST_ZEUS) && !defined(USE_ST20_DCU) && !defined(STUB_INJECT)
          /* We don't need to copy the file to memory. It exists already */
         /*   Memfread(VID_BufferLoad[LVar-1].AlignedBuffer_p, 1, FileSize, FileDescriptor);*/
  #endif /* #if defined(ST_ZEUS) && !defined(USE_ST20_DCU) && !defined(STUB_INJECT)*/
  #if defined(ST_ZEUS) && !defined(USE_ST20_DCU) && defined(STUB_INJECT)
        Memfread(VID_BufferLoad[LVar-1].AlignedBuffer_p, 1, FileSize, FileDescriptor);
  #else /* normal cases */
        debugread(FileDescriptor, VID_BufferLoad[LVar-1].AlignedBuffer_p, (size_t) FileSize);
  #endif /* #if defined(ST_ZEUS) && defined(USE_ST20_DCU) */
#endif /* #ifdef USE_OSPLUS_FOR_INJECT */
#endif /* #if defined(ST200_OS21) */

#ifdef USE_OSPLUS_FOR_INJECT
    SYS_FClose(FileDescriptor);
#else /* #ifdef USE_OSPLUS_FOR_INJECT */
  #if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
     Memfclose( FileDescriptor );
  #else
    debugclose( FileDescriptor );
  #endif /*defined(ST_ZEUS) && !defined(USE_ST20_DCU)*/
#endif /* #ifdef USE_OSPLUS_FOR_INJECT */


#ifndef ARCHITECTURE_SPARC
#if !defined(STVID_VALID) || (!defined(ST_DELTAPHI_HE) && !defined(ST_DELTAMU_HE) && !defined(ST_ZEUS)) /* do not slow emulation */
    /* parse again the loaded buffer to force the Data Cache to flush */
    for ( Ptr = (U8 *) VID_BufferLoad[LVar-1].AlignedBuffer_p;
        Ptr < ((U8 *) VID_BufferLoad[LVar-1].AlignedBuffer_p + VID_BufferLoad[LVar-1].NbBytes); )
    {
        vid_DummyRead = *Ptr++;
    }
#endif /* #if !defined(STVID_VALID) || (!defined(ST_DELTAPHI_HE) && !defined(ST_DELTAMU_HE) && !defined(ST_ZEUS)) */
#endif /* #ifndef ARCHITECTURE_SPARC */

#if defined(USE_OSPLUS_FOR_INJECT) && !defined(STUB_INJECT)
    }
#endif /* #if defined(USE_OSPLUS_FOR_INJECT) && !defined(STUB_INJECT) */

#ifdef STUB_INJECT
#ifdef ST_OSLINUX
    printf("Size detected: %d\n", FileSize);
    StubInject_SetStreamSize(FileSize);
#else /* #ifdef ST_OSLINUX */
    STTST_AssignInteger("VID_STREAMSIZE", FileSize, FALSE);
#endif /* #ifdef ST_OSLINUX */
#endif /* #ifdef STUB_INJECT */

#ifdef USE_OSPLUS_FOR_INJECT
    if(VID_BufferLoad[LVar-1].AlignedBuffer_p == NULL)
    {
        sprintf(VID_Msg, "file [%s] ready for injection from OSPLUS : size %ld\n",
            FileName, FileSize);
    }
    else
    {
#endif /* #ifdef USE_OSPLUS_FOR_INJECT */
#if defined(ST_ZEUS) && defined(USE_ST20_DCU)
        printf("file [%s] loaded : size %ld at address %x \n",
            FileName, FileSize, (int)VID_BufferLoad[LVar-1].AlignedBuffer_p);
#else
        sprintf(VID_Msg, "file [%s] loaded: size %ld from address 0x%08x to 0x%08x\n",
            FileName, FileSize, (U32)VID_BufferLoad[LVar-1].AlignedBuffer_p, (U32)FileSize + (U32)VID_BufferLoad[LVar-1].AlignedBuffer_p);
#endif
#ifdef USE_OSPLUS_FOR_INJECT
    }
#endif /* #ifdef USE_OSPLUS_FOR_INJECT */

#if defined(VALID_TOOLS) && defined(STVID_VALID_SBAG)
    printf("%s", VID_Msg);
#endif
#if !(defined(ST_ZEUS) && defined(USE_ST20_DCU))
    STTBX_Print((VID_Msg));
#endif

#ifdef ARCHITECTURE_SPARC
    /* Convert From BigEndian to Little Endian */
    STWRAP_Big2Little((void*)(AllocatedBuffer_p), FileSize);
#endif /* #ifdef ARCHITECTURE_SPARC */

    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return(API_EnableError ? RetErr : FALSE );
}  /* end of VID_Load */


#ifdef VIDEO_TEST_HARNESS_BUILD
#if !defined ST_OSLINUX
/*-------------------------------------------------------------------------
 * Function : VID_EraseFrameBuffer
 *            fill frame buffer with black pattern
 * Input    : *pars_p, *result_sym_p
 * Output   : FALSE if no error
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_EraseFrameBuffer(STTST_Parse_t *pars_p, char *result_sym_p )
{
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    STGXOBJ_Bitmap_t                        ErasedBitmap;
    STGXOBJ_Bitmap_t                        *BitmapParams_p;
    BOOL RetErr;
    S32 LVar;

    U32*            VirtualAddress_p;
    U32*            VirtualAddress2_p;
    void*           PatternAddress1_p;
    void*           PatternAddress2_p;
    U32             PatternSize1;
    U32             PatternSize2;
    ST_ErrorCode_t  ErrorCode;
    U32             LumaBlackPattern;
    U32             ChromaBlackPattern;

    LumaBlackPattern   = LUMA_WORD_FOR_BLACK_COLOR;
    ChromaBlackPattern = CHROMA_WORD_FOR_BLACK_COLOR;

    /* Set patterns */
     PatternAddress1_p = &LumaBlackPattern;
     PatternSize1      = 4;
     PatternAddress2_p = &ChromaBlackPattern;
     PatternSize2      = 4;

     RetErr = STTST_GetInteger(pars_p, 0, &LVar );
     if ( RetErr || LVar<=0 )
     {
         STTST_TagCurrentLine( pars_p, "expected bitmap address " );
         return(TRUE);
     }
     BitmapParams_p = (void*)LVar ;
     ErasedBitmap = *BitmapParams_p;

    /* Convert luma and chroma device addresses to virtual addresses */
    /* registered handle = index in cd-fifo array */
    ErrorCode = STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
    if (ErrorCode == ST_NO_ERROR)
    {
        VirtualAddress_p     = (U32*)DeviceToVirtual((ErasedBitmap.Data1_p), VirtualMapping);
        VirtualAddress2_p    = (U32*)DeviceToVirtual((ErasedBitmap.Data2_p), VirtualMapping);

        /* Fill the buffer with patterns */
        ErrorCode =  STAVMEM_FillBlock1D( (char*) PatternAddress1_p, PatternSize1, VirtualAddress_p, (ErasedBitmap.Size1 - ErasedBitmap.Size2));
        if(ErrorCode == ST_NO_ERROR)
        {
            ErrorCode = STAVMEM_FillBlock1D( (char*) PatternAddress2_p, PatternSize2, VirtualAddress2_p, ErasedBitmap.Size2);
        }
    }
    STTST_AssignInteger(result_sym_p, ErrorCode, FALSE);

    /* Translate error to an STVID private error before leaving */
    if(ErrorCode != ST_NO_ERROR)
    {
        return(TRUE);
    }

    return(FALSE);
}


/*-------------------------------------------------------------------------
 * Function : VID_LoadStill
 *            Load a mpeg still picture in memory
 * Input    : *pars_p, *result_sym_p
 * Output   : FALSE if no error
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_LoadStill(STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    BOOL Truncated;
    S32 LVar;
    U32 SizeOccupiedInMemory;
    void * AllocatedBuffer_p = NULL;
    ST_Partition_t* Partition_p = NULL;

#if defined(mb411) || defined(mb519)
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
#endif /* mb411 || mb519 */

    RetErr = STTST_GetInteger( pars_p, 1, &LVar);
    if (( RetErr ) || (LVar < 1) || (LVar > VID_MAX_BUFFER_LOAD))
    {
        sprintf(VID_Msg, "expected load buffer number (default is 1. max is %d)", VID_MAX_BUFFER_LOAD);
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }

    if(VID_BufferLoad[LVar-1].UsedCounter != 0)
    {
        STTBX_Print(( "Buffer cannot be load because already in used !!\n" ));
        return(TRUE);
    }

    if (VID_BufferLoad[LVar-1].AllocatedBuffer_p != NULL)
    {
#if defined(mb411) || defined(mb519)
#ifdef ST_OSLINUX
        /* Just to be compatible with vidutil_Deallocate() prototype */
        STAVMEM_PartitionHandle_t   AvmemPartitionHandle[1] = {0};
#endif

        vidutil_Deallocate(AvmemPartitionHandle[0], &(VID_BufferLoad[LVar-1].BlockHandle), VID_BufferLoad[LVar-1].AllocatedBuffer_p);
#else /* mb411 || mb519 */
        STOS_MemoryDeallocate(VID_BufferLoad[LVar-1].Partition_p, VID_BufferLoad[LVar-1].AllocatedBuffer_p);
#endif /* mb411 || mb519 */
        /* remark : no result is returned by this void function */
        VID_BufferLoad[LVar-1].AllocatedBuffer_p = NULL;
    }

    STTBX_Print(("loading in buffer no %d ...\n", LVar));

    /* ---  allocated buffer will be aligned later. Manage room to round up to a multiple of 16 bytes --- */
    Truncated = FALSE;
    /* For most of the file formats, 1 byte in file occupies one file in memory... */
    SizeOccupiedInMemory = VID_MPEGSTILL_SIZE;

    while ((SizeOccupiedInMemory > 0) && (AllocatedBuffer_p == NULL ))
    {
#if defined(mb411) || defined(mb519)
        vidutil_Allocate(AvmemPartitionHandle[0], (U32)(SizeOccupiedInMemory), 16, &AllocatedBuffer_p, &(VID_BufferLoad[LVar-1].BlockHandle));
        Partition_p = (ST_Partition_t *) AvmemPartitionHandle[0];
#else /* mb411 || mb519 */
        AllocatedBuffer_p = (char *)STOS_MemoryAllocate(DataPartition_p, (U32)((SizeOccupiedInMemory + 15) ));
        Partition_p = DataPartition_p;
#endif /* mb411 || mb519 */
#if defined(mb295)
        /* mb295 have two memory bank for CPU                 */
        /* Unable to allocate memory, try the second          */
        if (AllocatedBuffer_p == NULL)
        {
            AllocatedBuffer_p = (char *)STOS_MemoryAllocate(Data2Partition_p, (U32)((SizeOccupiedInMemory + 15) ));
            Partition_p = Data2Partition_p;
        }
#endif /* mb295 */
        if (AllocatedBuffer_p == NULL)
        {
            Truncated = TRUE;
            /* file too big : truncate the size */
            if (SizeOccupiedInMemory < 200000)
            {
                SizeOccupiedInMemory = 0;
            }
            else
            {
                SizeOccupiedInMemory -= 200000;
            }
            if ( SizeOccupiedInMemory == 0 )
            {
                STTST_AssignInteger(result_sym_p, TRUE, FALSE);
                sprintf(VID_Msg, "Not enough memory for file loading\n");
                STTBX_Print((VID_Msg));
                return(API_EnableError ? TRUE : FALSE );
            }
        }
    } /* end while */
    if (Truncated)
    {
        sprintf(VID_Msg, "Not enough memory for file loading : truncated to %d \n", SizeOccupiedInMemory);
        STTBX_Print((VID_Msg));
    }

    /* --- Setup the alligned buffer, load file and return --- */
    VID_BufferLoad[LVar-1].AllocatedBuffer_p = AllocatedBuffer_p;
    VID_BufferLoad[LVar-1].Partition_p = Partition_p;
#if defined(mb411) || defined(mb519)
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
    VID_BufferLoad[LVar-1].AlignedBuffer_p = STAVMEM_VirtualToCPU(AllocatedBuffer_p, &VirtualMapping);
#else /* mb411 || mb519 */
    VID_BufferLoad[LVar-1].AlignedBuffer_p =
        (void *)(((unsigned int) VID_BufferLoad[LVar-1].AllocatedBuffer_p + 15) & (~15));
#endif /* mb411 || mb519 */
    VID_BufferLoad[LVar-1].NbBytes = SizeOccupiedInMemory;
    strcpy(VID_BufferLoad[LVar-1].StreamName, "still");

    /* All other file formats are treated as binary */
    STAVMEM_CopyBlock1D((void *)(U32)VID_MpegStillPicture, VID_BufferLoad[LVar-1].AlignedBuffer_p, SizeOccupiedInMemory);

    sprintf(VID_Msg, "still picture loaded : size %d at address %x \n",
            SizeOccupiedInMemory, (int)VID_BufferLoad[LVar-1].AlignedBuffer_p);
    STTBX_Print((VID_Msg));

    STTST_AssignInteger("RET_VAL1",(int) VID_BufferLoad[LVar-1].AlignedBuffer_p, FALSE);

    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return(API_EnableError ? RetErr : FALSE );

} /* end of VID_LoadStill() */
#endif /* !LINUX */
#endif /* VIDEO_TEST_HARNESS_BUILD */

/*-------------------------------------------------------------------------
 * Function : VID_ParamsEvt
 *            Sets parameters for the events registers commands
 * Input    : *pars_p, *result_sym_p
 * Output   : FALSE if no error
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_ParamsEvt(STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;

    RetErr = STTST_GetString( pars_p, STVID_DEVICE_NAME1, EventVIDDevice, sizeof(ST_DeviceName_t));
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected video device name" );
        return(TRUE);
    }

    RetErr = STTST_GetString( pars_p, STVTG_DEVICE_NAME, EventVTGDevice, sizeof(ST_DeviceName_t));
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected vtg device name" );
        return(TRUE);
    }

    STTBX_Print(("VID_ParamsEvt(VID=\"%s\",VTG=\"%s\") : ok\n", EventVIDDevice, EventVTGDevice));
    STTST_AssignInteger(result_sym_p, RetErr, FALSE);

    return(FALSE);
}

/*-------------------------------------------------------------------------
 * Function : Vid_Copy
 *            Copy last displayed picture into memory
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_Copy(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    STVID_Handle_t  CurrentHandle;
    STAVMEM_AllocBlockParams_t AvmemAllocParams;
#if !defined ST_OSLINUX
    STAVMEM_FreeBlockParams_t AvmemFreeParams;
#endif
    STVID_PictureParams_t SourcePictureParams, DestPictureParams;
    STAVMEM_BlockHandle_t  DestPictureHandle;
    void *ForbidBorder[3];
    void *Address_p;
    char Param[80];
    ST_ErrorCode_t Error;

    RetErr = STTST_GetInteger( pars_p, DefaultVideoHandle, (S32*)&CurrentHandle);
    if ( RetErr )
    {
        STTST_TagCurrentLine(pars_p, "expected video handle");
        return(TRUE);
    }
    DefaultVideoHandle = CurrentHandle;

    /* Free memory ? */
    Param[0] = '\0';
    RetErr = STTST_GetString( pars_p, "", Param, sizeof(Param) );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Param (\"FREE\" to free pictures)" );
    }

    if ( strcmp(Param,"FREE")==0 || strcmp(Param,"free")==0 )
    {
        while (NbOfCopiedPictures > 0 )
        {
            if (CopiedPictureHandle[NbOfCopiedPictures-1] != STAVMEM_INVALID_BLOCK_HANDLE)
            {
#ifdef ST_OSLINUX
		/* Just to be compatible with vidutil_Deallocate() prototype */
        	STAVMEM_PartitionHandle_t   AvmemPartitionHandle[1] = {0};
		vidutil_Deallocate(AvmemPartitionHandle[0], &(CopiedPictureHandle[NbOfCopiedPictures-1]), CopiedPictureAddress[NbOfCopiedPictures-1]);
#else /* LINUX & FULL */
                AvmemFreeParams.PartitionHandle = AvmemPartitionHandle[0];
                if (STAVMEM_FreeBlock(&AvmemFreeParams, &(CopiedPictureHandle[NbOfCopiedPictures-1])) != ST_NO_ERROR)
                {
                    /* Error handling */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error freeing in VID_Copy()"));
                }
#endif	/* !LINUX || !FULL */
                CopiedPictureHandle[NbOfCopiedPictures-1] = STAVMEM_INVALID_BLOCK_HANDLE;
            }
            NbOfCopiedPictures--;
            STTBX_Print( ("VID_Copy() : memory for picture buffer %d is now free\n", NbOfCopiedPictures+1));
        }
        return(FALSE);
    } /* end if free required */

    if ( NbOfCopiedPictures >= NB_MAX_OF_COPIED_PICTURES )
    {
        STTBX_Print( ("VID_Copy() : too many pictures in memory !\n"));
        return(API_EnableError ? TRUE : FALSE );
    }
    /*CopiedPictureHandle[NbOfCopiedPictures] = STAVMEM_INVALID_BLOCK_HANDLE;*/

    /* Get source picture info */

    Error = STVID_GetPictureParams(DefaultVideoHandle, STVID_PICTURE_DISPLAYED, &SourcePictureParams);
    if (Error != ST_NO_ERROR)
    {
        sprintf(VID_Msg, "VID_Copy() : error STVID_GetPictureParams ");
        switch (Error)
        {
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case STVID_ERROR_DECODER_RUNNING :
                API_ErrorCount++;
                strcat(VID_Msg, "can't retrieve info. while decoding !\n" );
                break;
            case STVID_ERROR_NOT_AVAILABLE :
                API_ErrorCount++;
                strcat(VID_Msg, "no picture of the required picture type available !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%serror [%X] !\n", VID_Msg, Error );
                break;
        }
        STTBX_Print((VID_Msg));
    }
    else
    {
        /* Get Size and alignment for next Avmem allocation */
        Error = STVID_GetPictureAllocParams(DefaultVideoHandle, &SourcePictureParams, &AvmemAllocParams);
        if (Error != ST_NO_ERROR)
        {
            sprintf( VID_Msg, "VID_Copy() : error STVID_GetPictureAllocParams %d\n", Error);
            STTBX_Print((VID_Msg));
        }
        else
        {
#ifdef ST_OSLINUX
		/* Just to be compatible with vidutil_Deallocate() prototype */
		STAVMEM_PartitionHandle_t   AvmemPartitionHandle[1] = {0};
#endif	/* LINUX && FULL */
            /* Caution (Nov 2002) : AvmemAllocParams.Size may be smaller than PictParam.Size !                      */
            /* Example :  with memory profile is 720x576, AvmemAllocParams.Size=623104 PictParam.Size=623104        */
            /*           if SIF stream like 'susie' is decoded, AvmemAllocParams.Size=128256 PictParam.Size=623104  */
            /*           because the frame buffer contains [LLLLLL......CCC......] where L=Luma, C=Chroma, .=empty  */
            /*           STVID_DuplicatePicture() copies [LLLLLL......CCC......] without removing the [..] sections */
            if (AvmemAllocParams.Size < SourcePictureParams.Size)
            {
                STTBX_Print(("VID_Copy(): notice that AvmemAllocParams.Size=%d is lower than PictureParams.Size=%d\n",
                        AvmemAllocParams.Size, SourcePictureParams.Size));
                AvmemAllocParams.Size = SourcePictureParams.Size ;
                /* avoid crash of debugwrite(xxx,PictParam.Size) in vid_store */
            }

            /* Memory allocation */
            /* with  64 Mbytes SDRAM : bank size is 2 Mb     */
            /* with 128 Mbytes SDRAM : bank size is 4 Mb     */
            /* on Omega1, data must be contained in 1 bank   */
            /* (need to inform Avmem where bank borders are) */
            AvmemAllocParams.PartitionHandle = AvmemPartitionHandle[0];
            AvmemAllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
            AvmemAllocParams.NumberOfForbiddenRanges = 0;
            AvmemAllocParams.ForbiddenRangeArray_p = NULL;
#if defined(ST_7015) || defined(ST_7020)
            AvmemAllocParams.NumberOfForbiddenBorders = 0;
            AvmemAllocParams.ForbiddenBorderArray_p = NULL;
            ForbidBorder[0] = (void *) (SDRAM_BASE_ADDRESS + 0x400000);
#elif defined (ST_5516) || defined (ST_5517)
            AvmemAllocParams.NumberOfForbiddenBorders = 3;
            AvmemAllocParams.ForbiddenBorderArray_p = &ForbidBorder[0];
            ForbidBorder[0] = (void *) (SDRAM_BASE_ADDRESS + 0x400000);
            ForbidBorder[1] = (void *) (SDRAM_BASE_ADDRESS + 0x800000);
            ForbidBorder[2] = (void *) (SDRAM_BASE_ADDRESS + 0xC00000);
#else
            AvmemAllocParams.NumberOfForbiddenBorders = 3;
            AvmemAllocParams.ForbiddenBorderArray_p = &ForbidBorder[0];
            ForbidBorder[0] = (void *) (SDRAM_BASE_ADDRESS + 0x200000);
            ForbidBorder[1] = (void *) (SDRAM_BASE_ADDRESS + 0x400000);
            ForbidBorder[2] = (void *) (SDRAM_BASE_ADDRESS + 0x600000);
#endif

#ifdef ST_OSLINUX
            Error = vidutil_Allocate(AvmemAllocParams.PartitionHandle, AvmemAllocParams.Size, AvmemAllocParams.Alignment, &Address_p, &(DestPictureHandle));
#else
            if ((Error = STAVMEM_AllocBlock(&AvmemAllocParams, &(DestPictureHandle))) != ST_NO_ERROR)
            {
                /* Error handling */
                sprintf(VID_Msg, "VID_Copy() : error STAVMEM_AllocBlock, size %d\n", AvmemAllocParams.Size);
                STTBX_Print((VID_Msg));
            }
            else
            {
                if ((Error = STAVMEM_GetBlockAddress(DestPictureHandle, &Address_p)) != ST_NO_ERROR)
                {
                    /* Error handling */
                    STTBX_Print(("Error getting address in VID_Copy()"));
                }
            }
#endif	/* LINUX */

            if (Error == ST_NO_ERROR)
            {
                /* Store source picture in memory */
#ifdef ST_OSLINUX
                DestPictureParams.Data = (void *)STAPIGAT_UserToKernel( (void *)Address_p);
#else
                DestPictureParams.Data = Address_p;
#endif
                Error = STVID_DuplicatePicture(&SourcePictureParams, &DestPictureParams);
	            if (Error != ST_NO_ERROR)
                {
                    sprintf(VID_Msg, "VID_Copy() : STVID_DuplicatePicture error %d\n", Error);
                    STTBX_Print((VID_Msg));
                }
  	            else
                {
                    STTBX_Print(("VID_Copy() : %d bytes copied in buffer %d d=%08x\n",
                            DestPictureParams.Size, NbOfCopiedPictures+1, *(U32 *)Address_p ));
                    CopiedPictureParams[NbOfCopiedPictures] = DestPictureParams;
                    CopiedPictureHandle[NbOfCopiedPictures] = DestPictureHandle;
                    CopiedPictureAddress[NbOfCopiedPictures] = Address_p;
                    NbOfCopiedPictures++;
                    RetErr =  FALSE;
                }
            }
        }
    }

    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return(API_EnableError ? RetErr : FALSE );
} /* end of VID_Copy() */

/*-------------------------------------------------------------------------
 * Function : VID_StorePicture
 *            Store a copied picture
 * Input    : *pars_p, *result_sym_p
 * Output   : FALSE if no error
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_StorePicture( STTST_Parse_t *pars_p, char *result_sym_p )
{
STVID_PictureParams_t PictParams;
BOOL RetErr;
short PictureNo;
S32 SVar;
long int FilePict;
char FileName[80];

#ifdef ST_OSLINUX
	FILE *pf;
#endif

#if defined(mb_295)
    STTBX_Print( ("VID_Store unavailable on EVAL7020 board (due to DDR 8Mo Window tricky management)\n"));
    return(API_EnableError ? TRUE : FALSE );
#endif

    if ( NbOfCopiedPictures == 0 )
    {
        STTST_TagCurrentLine( pars_p, "VID_StorePicture() : no picture to store placed in memory !" );
        return(API_EnableError ? TRUE : FALSE );
    }

    /* get picture number */
    RetErr = STTST_GetInteger( pars_p, NbOfCopiedPictures, &SVar );
    if ( RetErr )
    {
        sprintf( VID_Msg, "expected Picture No. (max is %d)", NbOfCopiedPictures );
        STTST_TagCurrentLine( pars_p, VID_Msg );
        return(API_EnableError ? TRUE : FALSE );
    }
    PictureNo = SVar;
	memset( FileName, 0, sizeof(FileName));
    RetErr = STTST_GetString( pars_p, "../../../scripts/pictyuv", FileName, sizeof(FileName) );
    if ( RetErr )
    {
      STTST_TagCurrentLine( pars_p, "expected FileName" );
    }


    if ( PictureNo > NbOfCopiedPictures )
    {
        PictureNo = NbOfCopiedPictures ;
    }
    if ( PictureNo <= 0 )
    {
        PictureNo = 1 ;
    }

    PictParams = CopiedPictureParams[PictureNo-1];

#ifdef ST_OS20
    FilePict = debugopen( FileName,"wb" );
#endif /* ST_OS20 */

#ifdef ST_OS21
#ifdef NATIVE_CORE
    FilePict = open(FileName, O_WRONLY );
#else
    FilePict = open(FileName, O_WRONLY | O_BINARY  );
#endif
#endif /* ST_OS21 */


#ifdef ST_OSLINUX
    pf = fopen(FileName,"wb");
    if (pf != NULL)
	{
		/* File open has no error */
		FilePict = fileno(pf);
	}
	else
	{
		printf ("File open error...\n");
		FilePict = -1;
	}
#endif /* ST_OSLINUX */

    if (FilePict == -1)
	{
            STTBX_Print(("Failed to open %s in write mode.\n", FileName));
            return (FALSE);
	}

    debugwrite( FilePict,&PictParams,sizeof(STVID_PictureParams_t)) ;
    debugwrite( FilePict,PictParams.Data,PictParams.Size) ;

    /* Close the files */
    debugclose(FilePict);

    STTST_AssignInteger(result_sym_p, FALSE, FALSE);
    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_StorePicture() */

/*-------------------------------------------------------------------------
 * Function : VID_RestorePicture
 *            Restore a stored picture structure
 * Input    : *pars_p, *result_sym_p
 * Output   : FALSE if no error
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_RestorePicture( STTST_Parse_t *pars_p, char *result_sym_p )
{
STVID_PictureParams_t PictParams;
BOOL RetErr;
long int FilePict, FileSize;
char FileName[80];
STAVMEM_AllocBlockParams_t AvmemAllocParams;
STAVMEM_BlockHandle_t  PictHandle;
void *ForbidBorder[3];
void *Address_p;
ST_ErrorCode_t Error;
#ifdef ST_OS21
    struct stat FileStatus;
#endif /* ST_OS21 */
#ifdef ST_OSLINUX
	FILE *pf;
#endif


#if defined(mb_295)
    STTBX_Print( ("VID_Restore unavailable on EVAL7020 board (due to DDR 8Mo Window tricky management)\n"));
    return(API_EnableError ? TRUE : FALSE );
#endif

    if ( NbOfCopiedPictures >= NB_MAX_OF_COPIED_PICTURES )
    {
        STTBX_Print( ("VID_Restore : too many pictures in memory !\n"));
        return(API_EnableError ? TRUE : FALSE );
    }

	memset( FileName, 0, sizeof(FileName));
    RetErr = STTST_GetString( pars_p, "../../../scripts/pictyuv", FileName, sizeof(FileName) );
    if ( RetErr )
    {
      STTST_TagCurrentLine( pars_p, "expected FileName" );
    }

#ifdef ST_OS20
    FilePict = debugopen( FileName,"rb" );
#endif /* ST_OS20 */

#ifdef ST_OS21
#ifdef NATIVE_CORE
    FilePict = open(FileName, O_WRONLY );
#else
    FilePict = open(FileName, O_WRONLY | O_BINARY  );
#endif
#endif /* ST_OS21 */


#ifdef ST_OSLINUX
    pf = fopen(FileName,"rb");
    if (pf != NULL)
	{
		/* File open has no error */
		FilePict = fileno(pf);
	}
	else
	{
		printf ("File open error...\n");
		FilePict = -1;
	}
#endif /* ST_OSLINUX */

    if (FilePict == -1)
	{
            STTBX_Print(("Failed to open %s in read mode.\n", FileName));
            return(API_EnableError ? TRUE : FALSE );
	}

#if defined (ST_OS20) || defined(ST_OSLINUX)
    FileSize = debugfilesize(FilePict);
#endif /* ST_OS20 */

#ifdef ST_OS21
    fstat(FilePict, &FileStatus);
    FileSize = FileStatus.st_size;
#endif /* ST_OS21 */

    debugread( FilePict,&PictParams,sizeof(STVID_PictureParams_t)) ;

    /* Get Size and alignment */
    Error = STVID_GetPictureAllocParams(DefaultVideoHandle, &PictParams, &AvmemAllocParams);
    if (Error != ST_NO_ERROR)
    {
        sprintf( VID_Msg, "VID_Restore() : error STVID_GetPictureAllocParams %d\n", Error);
        STTBX_Print((VID_Msg));
        RetErr = TRUE;
    }
    else
    {
        /* to avoid memory crash, check data consistency */
        if ((U32)FileSize != (PictParams.Size + sizeof(PictParams)))
        {
            STTBX_Print(("VID_Restore() : file inconsistency !\n"));
            STTBX_Print(("              : file_size=%ld bytes but header_size+data_size=%d+%d=%d !\n",
                        FileSize, sizeof(PictParams), PictParams.Size, (PictParams.Size + sizeof(PictParams)) ));
            STTBX_Print(("                command cancelled\n"));
            RetErr =  TRUE;
        }
        else
        {
#ifdef ST_OSLINUX
            /* Just to be compatible with vidutil_Deallocate() prototype */
            STAVMEM_PartitionHandle_t   AvmemPartitionHandle[1] = {0};
#endif  /* LINUX && FULL */
            /* Caution (Nov 2002) : AvmemAllocParams.Size may be smaller than PictParam.Size !            */
            /* It occurs when the file contains [LLLLLL......CCC......] where L=Luma, C=Chroma, .=empty   */
            /* The 1st size is [L]+[C] size, the 2nd size is the whole size including [.] sections        */
            if (AvmemAllocParams.Size < PictParams.Size)
            {
                STTBX_Print(("VID_Restore(): notice that AvmemAllocParams.Size=%d is lower than PictParams.Size=%d\n",
                        AvmemAllocParams.Size, PictParams.Size));
                AvmemAllocParams.Size = PictParams.Size ;
                /* avoid a crash when filling a too small buffer */
            }
            /* Memory allocation */
            /* with  64 Mbytes SDRAM : bank size is 2 Mb     */
            /* with 128 Mbytes SDRAM : bank size is 4 Mb     */
            /* on Omega1, data must be contained in 1 bank   */
            /* (need to inform Avmem where bank borders are) */
            AvmemAllocParams.PartitionHandle = AvmemPartitionHandle[0];
            AvmemAllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
            AvmemAllocParams.NumberOfForbiddenRanges = 0;
            AvmemAllocParams.ForbiddenRangeArray_p = NULL;
#if defined(ST_7015) || defined(ST_7020)
            AvmemAllocParams.NumberOfForbiddenBorders = 0;
            AvmemAllocParams.ForbiddenBorderArray_p = NULL;
            ForbidBorder[0] = (void *) (SDRAM_BASE_ADDRESS + 0x400000);
#elif defined (ST_5516) || defined (ST_5517)
            AvmemAllocParams.NumberOfForbiddenBorders = 3;
            AvmemAllocParams.ForbiddenBorderArray_p = &ForbidBorder[0];
            ForbidBorder[0] = (void *) (SDRAM_BASE_ADDRESS + 0x400000);
            ForbidBorder[1] = (void *) (SDRAM_BASE_ADDRESS + 0x800000);
            ForbidBorder[2] = (void *) (SDRAM_BASE_ADDRESS + 0xC00000);
#else
            AvmemAllocParams.NumberOfForbiddenBorders = 3;
            AvmemAllocParams.ForbiddenBorderArray_p = &ForbidBorder[0];
            ForbidBorder[0] = (void *) (SDRAM_BASE_ADDRESS + 0x200000);
            ForbidBorder[1] = (void *) (SDRAM_BASE_ADDRESS + 0x400000);
            ForbidBorder[2] = (void *) (SDRAM_BASE_ADDRESS + 0x600000);
#endif
#ifdef ST_OSLINUX
            Error = vidutil_Allocate(AvmemAllocParams.PartitionHandle, AvmemAllocParams.Size, AvmemAllocParams.Alignment, &Address_p, &(PictHandle));
#else
            if ((Error = STAVMEM_AllocBlock(&AvmemAllocParams, &(PictHandle))) != ST_NO_ERROR)
            {
                /* Error handling */
                sprintf(VID_Msg, "VID_Restore : error STAVMEM_AllocBlock, size %d\n", AvmemAllocParams.Size);
                STTBX_Print((VID_Msg));
                RetErr =  TRUE;
            }
            else
            {
                if ((Error = STAVMEM_GetBlockAddress(PictHandle, &Address_p)) != ST_NO_ERROR)
                {
                    /* Error handling */
                    STTBX_Print(("Error getting address in VID_RestorePicture()"));
                    RetErr =  TRUE;
                }
            }
#endif  /* LINUX */

            if (Error == ST_NO_ERROR)
                {
                    /* Store source picture in memory */
                    PictParams.Data = Address_p;

                    CopiedPictureParams[NbOfCopiedPictures] = PictParams;
                    CopiedPictureHandle[NbOfCopiedPictures] = PictHandle;
                    CopiedPictureAddress[NbOfCopiedPictures] = Address_p;
                    debugread( FilePict, PictParams.Data, PictParams.Size) ;
                    NbOfCopiedPictures++;
                    RetErr =  FALSE;
                    STTBX_Print(("VID_Restore : %d bytes loaded in buffer %d\n", PictParams.Size, NbOfCopiedPictures));
                }
        } /* end of consistency test */
    }

    /* Close the files */
    debugclose(FilePict);

    STTST_AssignInteger(result_sym_p, FALSE, FALSE);
    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_RestorePicture() */

#ifndef STVID_NO_DISPLAY
/*-------------------------------------------------------------------------
 * Function : Vid_InteractiveIO
 *            Interactive test of STVID_SetIOWindows()
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL Vid_InteractiveIO(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr =  TRUE ;
    ST_ErrorCode_t ErrCode;
    BOOL SetIO = FALSE;
    S32 InputWinPositionX;
    S32 InputWinPositionY;
    U32 InputWinWidth;
    U32 InputWinHeight;
    S32 OutputWinPositionX;
    S32 OutputWinPositionY;
    U32 OutputWinWidth;
    U32 OutputWinHeight;
    S32 AlignInputWinPositionX;
    S32 AlignInputWinPositionY;
    U32 AlignInputWinWidth;
    U32 AlignInputWinHeight;
    S32 AlignOutputWinPositionX;
    S32 AlignOutputWinPositionY;
    U32 AlignOutputWinWidth;
    U32 AlignOutputWinHeight;
    S32 Delta=1;
    char WinType='i';
    char Hit;
    S32 VideoViewportHandle;

    RetErr = STTST_GetInteger( pars_p, (S32)ViewPortHndl0, &VideoViewportHandle);
    if ( RetErr )
    {
        STTST_TagCurrentLine(pars_p, "expected video viewport handle");
        return(TRUE);
    }

    STTBX_Print(("Run STVID_SetIOWindows(ix,iy,iw,ih,ox,oy,ow,oh) interactively\nPress x/y/w/h to set x/y/w/h ; +/- to increase/decrease ; i/o to select input/output\nPress Q or Enter to quit\n"));
    /* get current windows parameters */
    ErrCode = STVID_GetIOWindows( (STVID_ViewPortHandle_t)(VideoViewportHandle), &InputWinPositionX, &InputWinPositionY,
                                  &InputWinWidth, &InputWinHeight,
                                  &OutputWinPositionX, &OutputWinPositionY,
                                  &OutputWinWidth, &OutputWinHeight );
    Hit=0;
    while ( Hit!='q' && Hit!='Q' && Hit!='\r')
    {
        STTBX_Print(("Press a key (X,Y,W,H,I,O,-,+) or Q to Quit\n"));
        STTBX_InputChar(&Hit);
        /*STTBX_InputChar(STTBX_INPUT_DEVICE_NORMAL, &Hit);*/
        switch(Hit)
        {
            case 'x':
            case 'X':
                if (WinType=='i')
                    InputWinPositionX+=Delta;
                else
                    OutputWinPositionX+=Delta;
                SetIO= TRUE;
                break;
            case 'y':
            case 'Y':
                if (WinType=='i')
                    InputWinPositionY+=Delta;
                else
                    OutputWinPositionY+=Delta;
                SetIO= TRUE;
                break;
            case 'w':
            case 'W':
                if (WinType=='i')
                    InputWinWidth+=Delta;
                else
                    OutputWinWidth+=Delta;
                SetIO= TRUE;
                break;
            case 'h':
            case 'H':
                if (WinType=='i')
                    InputWinHeight+=Delta;
                else
                    OutputWinHeight+=Delta;
                SetIO= TRUE;
                break;
            case 'i':
            case 'I':
                WinType='i';
                sprintf(VID_Msg, "Now input window will change %d\n",Delta);
                STTBX_Print((VID_Msg));
                break;
            case 'o':
            case 'O':
                WinType='o';
                sprintf(VID_Msg, "Now output window will change %d\n",Delta);
                STTBX_Print((VID_Msg));
                break;
            case '-':
                Delta--;
                sprintf(VID_Msg, "Increase/decrease value=%d\n",Delta);
                STTBX_Print((VID_Msg));
                break;
            case '+':
                Delta++;
                sprintf(VID_Msg, "Increase/decrease value=%d\n",Delta);
                STTBX_Print((VID_Msg));
                break;
            default:
                break;
        } /* end switch */
        if ( SetIO )
        {
            ErrCode = STVID_SetIOWindows( (STVID_ViewPortHandle_t)VideoViewportHandle,
                                      InputWinPositionX, InputWinPositionY,
                                      InputWinWidth, InputWinHeight,
                                      OutputWinPositionX, OutputWinPositionY,
                                      OutputWinWidth, OutputWinHeight );
            sprintf( VID_Msg, "STVID_SetIOWindows(%d,%d,%d,%d,%d,%d,%d,%d,%d)\n", VideoViewportHandle,
                     InputWinPositionX, InputWinPositionY, InputWinWidth, InputWinHeight,
                     OutputWinPositionX, OutputWinPositionY, OutputWinWidth, OutputWinHeight );
            STTBX_Print((VID_Msg));

            ErrCode = STVID_GetAlignIOWindows( (STVID_ViewPortHandle_t)VideoViewportHandle,
                                            &AlignInputWinPositionX, &AlignInputWinPositionY,
                                            &AlignInputWinWidth, &AlignInputWinHeight,
                                            &AlignOutputWinPositionX, &AlignOutputWinPositionY,
                                            &AlignOutputWinWidth, &AlignOutputWinHeight );
            sprintf(VID_Msg, "STVID_GetAlignIOWindows(%d,%d,%d,%d,%d,%d,%d,%d,%d)\n",
                    VideoViewportHandle,
                    AlignInputWinPositionX, AlignInputWinPositionY,
                    AlignInputWinWidth, AlignInputWinHeight,
                    AlignOutputWinPositionX, AlignOutputWinPositionY,
                    AlignOutputWinWidth, AlignOutputWinHeight );
            STTBX_Print((VID_Msg));
        }
        SetIO= FALSE;

    }
    STTST_AssignInteger(result_sym_p, FALSE, FALSE);
    return(FALSE);

} /* end of Vid_InteractiveIO() */
#endif /*#ifndef STVID_NO_DISPLAY*/

/*#######################################################################*/
/*################### CALLBACK FUNCTIONS FOR EVENT TESTS ################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : VID_ReceiveEvt
 *            Log the occured event (callback function)
 * Input    :
 * Output   : RcvEventStack[]
 *            NewRcvEvtIndex
 *            NbOfRcvEvents
 *            RcvEventCnt[]
 * Return   :
 * ----------------------------------------------------------------------*/
static void VID_ReceiveEvt ( STEVT_CallReason_t Reason,
                             const ST_DeviceName_t RegistrantName,
                             STEVT_EventConstant_t Event,
                             void *EventData,
                             void *SubscriberData_p )
{
    S32 CountIndex;
    U32 NbOfBytes;
    BOOL OverwriteEvent;
    STVID_UserData_t *PtUserdata;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(SubscriberData_p);

    if (EventLoggingEnabled)
    {
        STTBX_Print(("Received event 0x%ulx \n",Event));
    }

    if ( NewRcvEvtIndex >= NB_MAX_OF_STACKED_EVENTS ) /* if the stack is full...*/
    {
        return; /* ignore current event */
    }
    OverwriteEvent = FALSE;

    /* Process event data before using them ... */
    if (STVID_PreprocessEvent(RegistrantName, Event, (void *)EventData) != ST_NO_ERROR)
    {
        #if defined(ST_OS20) || defined(ST_OSWINCE)
            STTBX_Print(("%s(%d): Preprocess event pb ! 0x%.8x \n", __FILE__, __LINE__, Event));
        #else
            STTBX_Print(("%s(): Preprocess event pb ! 0x%.8x \n", __FUNCTION__, Event));
        #endif
    }

    switch(Event)
    {
        case STVID_ASPECT_RATIO_CHANGE_EVT:
            CountIndex = STVID_ASPECT_RATIO_CHANGE_EVT;
            if (EventData != NULL)
            {
            memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STVID_DisplayAspectRatio_t) );
            }
            break;
        case STVID_FRAME_RATE_CHANGE_EVT:
            CountIndex = STVID_FRAME_RATE_CHANGE_EVT;
            if (EventData != NULL)
            {
                memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STVID_PictureParams_t) );
            }
            break;
        case STVID_SCAN_TYPE_CHANGE_EVT:
            CountIndex = STVID_SCAN_TYPE_CHANGE_EVT;
            if (EventData != NULL)
            {
                memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STVID_PictureParams_t) );
            }
            break;
        case STVID_RESOLUTION_CHANGE_EVT:
            CountIndex = STVID_RESOLUTION_CHANGE_EVT;
            if (EventData != NULL)
            {
                memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STVID_PictureParams_t) );
            }
            break;
        case STVID_SEQUENCE_INFO_EVT:
            CountIndex = STVID_SEQUENCE_INFO_EVT;
            if (EventData != NULL)
            {
            memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STVID_SequenceInfo_t) );
            }
            break;
        case STVID_USER_DATA_EVT:
            CountIndex = STVID_USER_DATA_EVT;
            if (EventData != NULL)
            {
              memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STVID_UserData_t) );
              if ( RcvEventStack[NewRcvEvtIndex].EvtData.UserData.Buff_p != 0 ) /* check address of user data */
              {
                PtUserdata = (STVID_UserData_t *)(U32)EventData ;
                NbOfBytes = (PtUserdata->Length < VID_MAX_USER_DATA) ? PtUserdata->Length : VID_MAX_USER_DATA ;
                memcpy( &RcvUserDataStack[NewRcvEvtIndex],
                        RcvEventStack[NewRcvEvtIndex].EvtData.UserData.Buff_p,
                        NbOfBytes );
              }
            }
            break;
        case STVID_DATA_ERROR_EVT:
            CountIndex = STVID_DATA_ERROR_EVT;
            /* no data */
            break;
        case STVID_DISPLAY_NEW_FRAME_EVT:
            CountIndex = STVID_DISPLAY_NEW_FRAME_EVT;
            if (EventData != NULL)
            {
                memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STVID_PictureParams_t) );
            }
            /* April 2002 : 2 new pict. can be send to the Display task during the same Vsync */
            /* In such case, we overwrite the new one in the stack (the previous is lost)     */
            /* Assumption : no other kind of event occured between them                       */
            if ( VSyncCount == VSyncCountForDisplay )
            {
                /* overwrite current event if stack not empty */
                if ( NewRcvEvtIndex > LastRcvEvtIndex )
                {
                    if ( (long)Event == RcvEventStack[NewRcvEvtIndex-1].EvtNum )
                    {
                        if (EventData != NULL)
                    {
                        memcpy( &RcvEventStack[NewRcvEvtIndex-1].EvtData, EventData, sizeof(STVID_PictureParams_t) );
                        }
                        OverwriteEvent = TRUE;
                    }
                }
            }
            VSyncCountForDisplay = VSyncCount;
            break;
        case STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT:
            CountIndex = STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT;
            if (EventData != NULL)
            {
                memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STVID_PictureInfos_t) );
                HandleOfLastPictureBufferDisplayed = ((STVID_PictureInfos_t *)EventData)->PictureBufferHandle;
            }
            /* April 2002 : 2 new pict. can be send to the Display task during the same Vsync */
            /* In such case, we overwrite the new one in the stack (the previous is lost)     */
            /* Assumption : no other kind of event occured between them                       */
            if ( VSyncCount == VSyncCountForDisplay )
            {
                /* overwrite current event if stack not empty */
                if ( NewRcvEvtIndex > LastRcvEvtIndex )
                {
                    if ( (long)Event == RcvEventStack[NewRcvEvtIndex-1].EvtNum )
                    {
                        if (EventData != NULL)
                    {
                        memcpy( &RcvEventStack[NewRcvEvtIndex-1].EvtData, EventData, sizeof(STVID_PictureInfos_t) );
                        }
                        OverwriteEvent = TRUE;
                    }
                }
            }
            VSyncCountForDisplay = VSyncCount;
            break;
        case STVID_IMPOSSIBLE_WITH_MEM_PROFILE_EVT:
            CountIndex = STVID_IMPOSSIBLE_WITH_MEM_PROFILE_EVT;
            if (EventData != NULL)
            {
            memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STVID_MemoryProfile_t) );
            }
            break;
        case STVID_PICTURE_DECODING_ERROR_EVT:
            CountIndex = STVID_PICTURE_DECODING_ERROR_EVT;
            if (EventData != NULL)
            {
                memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STVID_PictureParams_t) );
            }
            break;
        case STVID_STOPPED_EVT:
            CountIndex = STVID_STOPPED_EVT;
            if (EventData != NULL)
            {
                memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STVID_PictureParams_t) );
            }
            break;
        case STVID_BACK_TO_SYNC_EVT:
            CountIndex = STVID_BACK_TO_SYNC_EVT;
            /* no data */
            break;
        case STVID_OUT_OF_SYNC_EVT:
            CountIndex = STVID_OUT_OF_SYNC_EVT;
            /* no data */
            break;
        case STVID_SYNCHRO_EVT:
            CountIndex = STVID_SYNCHRO_EVT;
            if (EventData != NULL)
            {
            memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STVID_SynchroInfo_t) );
            }
            break;
        case STVID_NEW_PICTURE_DECODED_EVT:
            CountIndex = STVID_NEW_PICTURE_DECODED_EVT;
            if (EventData != NULL)
            {
                memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STVID_PictureInfos_t) );
            }
            break;
        case STVID_NEW_PICTURE_ORDERED_EVT:
            CountIndex = STVID_NEW_PICTURE_ORDERED_EVT;
            if (EventData != NULL)
            {
                memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STVID_PictureInfos_t) );
            }
            break;
        case STVID_SPEED_DRIFT_THRESHOLD_EVT:
            CountIndex = STVID_SPEED_DRIFT_THRESHOLD_EVT;
            if (EventData != NULL)
            {
            memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STVID_SpeedDriftThreshold_t) );
            }
            break;
        case STVID_DATA_OVERFLOW_EVT:
            CountIndex = STVID_DATA_OVERFLOW_EVT;
            /* no data */
            break;
        case STVID_DATA_UNDERFLOW_EVT:
            CountIndex = STVID_DATA_UNDERFLOW_EVT;
            if (EventData != NULL)
            {
            memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STVID_DataUnderflow_t) );
            }
            break;
        case STVID_SYNCHRONISATION_CHECK_EVT:
            CountIndex = STVID_SYNCHRONISATION_CHECK_EVT;
            if (EventData != NULL)
            {
            memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STVID_SynchronisationInfo_t) );
            }
            break;

/* #if defined(ST_7015) */
        /* case STVID_WORKAROUND_PLEASE_STOP_INJECTION_EVT: */
            /* CountIndex = STVID_WORKAROUND_PLEASE_STOP_INJECTION_EVT; */
            /* no data */
            /* break; */
 /* */
        /* case STVID_WORKAROUND_THANKS_INJECTION_CAN_GO_ON_EVT: */
            /* CountIndex = STVID_WORKAROUND_THANKS_INJECTION_CAN_GO_ON_EVT; */
            /* no data */
            /* break; */
/* #endif */

#ifdef STVID_HARDWARE_ERROR_EVENT
        case STVID_HARDWARE_ERROR_EVT:
            CountIndex = STVID_HARDWARE_ERROR_EVT;
            if (EventData != NULL)
            {
            memcpy( &RcvEventStack[NewRcvEvtIndex].EvtData, EventData, sizeof(STVID_HardwareError_t) );
            }
            break;
#endif
        case STVID_SEQUENCE_END_CODE_EVT:
            CountIndex = STVID_SEQUENCE_END_CODE_EVT;
            /* no data */
            break;
        default:
            CountIndex = -1; /* unknown video event */
            RcvWrongEventStack[NewRcvEvtIndex]=NewRcvEvtIndex; /* index in stak of the wrong evt. */
    } /* end switch */
    RcvEventStack[NewRcvEvtIndex].EvtNum = Event;
    RcvEventStack[NewRcvEvtIndex].EvtTime = STOS_time_now();

    if ( !(OverwriteEvent) )
    {
        NewRcvEvtIndex++;
        /* counting */
        if ( CountIndex >= 0 )
        {
            RcvEventCnt[CountIndex-STVID_DRIVER_BASE]++;
        }
        NbOfRcvEvents++;
    }
} /* end of VID_ReceiveEvt() */

/*-------------------------------------------------------------------------
 * Function : VID_ReceiveVSync
 *            Be informed of a STVTG_VSYNC_EVT (callback function)
 *            Increment a counter
 * Input    : event data
 * Output   : VSyncCount++
 * Return   :
 * ----------------------------------------------------------------------*/
static void VID_ReceiveVSync ( STEVT_CallReason_t Reason,
                               const ST_DeviceName_t RegistrantName,
                               STEVT_EventConstant_t Event,
                               void *EventData,
                               void *SubscriberData_p )
{
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData);
    UNUSED_PARAMETER(SubscriberData_p);
    VSyncCount++;
} /* end of VID_ReceiveVSync() */

/*-------------------------------------------------------------------------
 * Function : VID_ReceiveNewPict
 *            Be informed of a STVID_NEW_PICTURE (callback function)
 *            Measure the delay between current and previous one
 *            Check delay with theoretical time, according to frame rate
 *
 * Input    : PICTDELAY_NB nb of delayed frames
 *            PICTDELAY_TOTAL delay
 * Output   : PICTDELAY_NB nb of delayed frames
 *            PICTDELAY_TOTAL delay
 * Return   :
 * ----------------------------------------------------------------------*/
static void VID_ReceiveNewPict ( STEVT_CallReason_t Reason,
                                const ST_DeviceName_t RegistrantName,
                                STEVT_EventConstant_t Event,
                                void *EventData,
                                void *SubscriberData_p )
{
    STVID_PictureInfos_t PictInfo;
    U32 TickPerSecond;
    S32 Difference;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(SubscriberData_p);

    if ( Event != STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT && EventData != NULL )
    {
        return;
    }

    /* Process event data before using them ... */
    if (STVID_PreprocessEvent(RegistrantName, Event, (void *)EventData) != ST_NO_ERROR)
    {
#if defined(ST_OS20) || defined(ST_OSWINCE)
        STTBX_Print(("%s(%d): Preprocess event pb ! 0x%.8x \n", __FILE__, __LINE__, Event));
#else
        STTBX_Print(("%s(): Preprocess event pb ! 0x%.8x \n", __FUNCTION__, Event));
#endif
    }

    /* if it is the 1st measure */
    if ( Pict_StartTime == 0 )
    {
        Pict_LastTime = STOS_time_now();
        Pict_PreviousTime = Pict_LastTime;
        Pict_StartTime = Pict_PreviousTime;
        Pict_Counted++;
        return;
    }
    Pict_PreviousTime = Pict_LastTime;

    /* notice current time and framerate */
    Pict_LastTime = STOS_time_now();
    memcpy( &PictInfo, EventData, sizeof(STVID_PictureInfos_t) );

    /* difference between 2 pictures (nb of ticks) */
    Difference = (S32)STOS_time_minus(Pict_LastTime, Pict_PreviousTime);

    /* delay (nb of nanoseconds) */
    TickPerSecond = ST_GetClocksPerSecond();
    Difference = (Difference * 1000000) / TickPerSecond;

    /* delay between 2 frames (nanoseconds) */
    /* ex: 40000 ns at rate 25000 milliHz, 33333 ns in NTSC */
    Pict_FrameDuration = 1000000000 / PictInfo.VideoParams.FrameRate;

    /* difference between measure and expectation (milliseconds) */
    Difference = (Difference - Pict_FrameDuration)/10000;

    /* statistics */
    Pict_Counted++;
    if ( Difference < 0)
    {
        Pict_AdvanceTime += Difference;
        Pict_Advanced++;
        if ( (-Difference) > Pict_FrameDuration )
        {
            Pict_FrameDelayed++; /* a top/bottom field may have been skipped at display */
        }
    }
    if ( Difference > 0)
    {
        Pict_DelayTime += Difference;
        Pict_Delayed++;
        if ( Difference > Pict_FrameDuration )
        {
            Pict_FrameDelayed++; /* a top/bottom field may have been displayed twice */
        }
        if ( Difference > Pict_BiggerDelayTime )
        {
            Pict_BiggerDelayTime = Difference ;
        }
    }

} /* end of VID_ReceiveNewPict() */

/*-------------------------------------------------------------------------
 * Function : VID_ReceivePictAndVSync
 *            Be informed of STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT
 *            Measure the number of VSync between 2 displayed pictures
 *            (callback function)
 * Input    : NbOfNewPict, NbOfVSyncBetween2Pict,
 *            NbOfVSyncIsConstant, TotalOfVSync
 *            VSyncCount, PreviousPictureVSyncCount
 * Output   : NbOfNewPict, NbOfVSyncBetween2Pict,
 *            TotalOfVSync
 *            PreviousPictureVSyncCount
 * Return   : (none)
 * ----------------------------------------------------------------------*/
static void VID_ReceivePictAndVSync ( STEVT_CallReason_t Reason,
                                const ST_DeviceName_t RegistrantName,
                                STEVT_EventConstant_t Event,
                                void *EventData,
                                void *SubscriberData_p )
{
    U32 Average ;
    U32 PictureVSyncCount;
    S32 CurrSpeed;
    static short PictAndVSyncState; /* state machine, values 0->1->2 */

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(EventData);
    UNUSED_PARAMETER(SubscriberData_p);

    /* Process event data before using them ... */
    if (STVID_PreprocessEvent(RegistrantName, Event, (void *)EventData) != ST_NO_ERROR)
    {
#if defined(ST_OS20) || defined(ST_OSWINCE)
        STTBX_Print(("%s(%d): Preprocess event pb ! 0x%.8x \n", __FILE__, __LINE__, Event));
#else
        STTBX_Print(("%s(): Preprocess event pb ! 0x%.8x \n", __FUNCTION__, Event));
#endif
    }

    /* STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT received */
    if (NbOfNewPict==1)
    {
        if (PictAndVSyncState==0 && VSyncCount-PreviousPictureVSyncCount<=20)
        {
            /* wait end of transitory speed regime until video speed is really what expected */
            /* because video speed is supposed to be good after 20 VSync */
            /* (do not count 20 VSync from 'vid_evt' call, but from 1st new_picture) */
            return;
        }
        if ( PictAndVSyncState==0 )
        {
            /* many VSync occured since the 1st picture ; replace it by the current */
            /* to avoid counting a lot of VSync between the 1st picture and the next one */
            PictAndVSyncState = 1; /* replace 1st by current */
            PreviousPictureVSyncCount = VSyncCount;
            return;
        }
        PictAndVSyncState = 2; /* start logging & counting */
    }
    if ( NbOfNewPict>0 )
    {
        /* VSyncCount may vary during the following actions, keep its current value */
        PictureVSyncCount = VSyncCount ;
        if ( PictureVSyncCount == PreviousPictureVSyncCount )
        {
            /* no new VSync detected since previous picture */
            /* STVID will ignore the previous picture, the current one will be displayed */
        }
        else
        {
            if (NbOfNewPict>=2)
            {
                /* 3 events required at least, to compare NbOfVSyncBetween2Pict with previous one */
                if (NbOfVSyncBetween2Pict != (PictureVSyncCount - PreviousPictureVSyncCount))
                {
                        NbOfVSyncIsConstant = FALSE;
                }
            }
            NbOfVSyncBetween2Pict = PictureVSyncCount - PreviousPictureVSyncCount ;
            if ( NbOfNewPict < NbMaxOfVSyncToLog )
            {
                /* for pictures 1 to N, put the (N-1) differences in array[1] to [N] */
                VSyncLog[NbOfNewPict] = NbOfVSyncBetween2Pict;
            }
            if ( NbOfVSyncBetween2Pict>NbMaxOfVSyncBetween2Pict )
            {
                NbMaxOfVSyncBetween2Pict = NbOfVSyncBetween2Pict;
            }
            /* sum of measures */
            TotalOfVSync = TotalOfVSync + NbOfVSyncBetween2Pict ;
            /* average of measures (used factor 1000 to avoid a rounded value after division) */
            Average = (TotalOfVSync * 1000)/ NbOfNewPict ;
            NbOfNewPict++ ;
            PreviousPictureVSyncCount = PictureVSyncCount;
        }
    }
    else
    {
        /* 1st measure : previous VSyncCount is unknown */
        NbOfNewPict++ ;
        PreviousPictureVSyncCount = VSyncCount;
        PictAndVSyncState = 2; /* start counting */
        STVID_GetSpeed(DefaultVideoHandle, &CurrSpeed);
        if( CurrSpeed>100 )
        {
            PictAndVSyncState = 0; /* skip first pictures */
        }
    }
} /* end of VID_ReceivePictAndVSync() */

/*-------------------------------------------------------------------------
 * Function : VID_ReceivePictTimeCode
 *            Be informed of STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT
 *            Measure the number of TimeCode.Frames between 2 displayed pictures
 *            in order to check that input pictures are regularly displayed
 *            (callback function)
 * Input    : Pict_FrameNb, Pict_Difference
 * Output   : Pict_FrameNb, Pict_Difference, Pict_MinDifference,
 *            Pict_MaxDifference, Pict_DisplayIsFluent,
 *            Pict_TotalOfDifferences, Pict_NbOfNewRef
 * Constraint: IPB structure should not vary in the input stream
 *            TimeCode inside GOP should be well set in the input stream
 *            do not loop while injecting the input stream
 * Return   : (none)
 * ----------------------------------------------------------------------*/
static void VID_ReceivePictTimeCode ( STEVT_CallReason_t Reason,
                                const ST_DeviceName_t RegistrantName,
                                STEVT_EventConstant_t Event,
                                void *EventData,
                                void *SubscriberData_p )
{
    U32 FrameNb;
    U32 Coeff;
    STVID_PictureInfos_t *PictPt;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(SubscriberData_p);

    /* Process event data before using them ... */
    if (STVID_PreprocessEvent(RegistrantName, Event, (void *)EventData) != ST_NO_ERROR)
    {
#if defined(ST_OS20) || defined(ST_OSWINCE)
        STTBX_Print(("%s(%d): Preprocess event pb ! 0x%.8x \n", __FILE__, __LINE__, Event));
#else
        STTBX_Print(("%s(): Preprocess event pb ! 0x%.8x \n", __FUNCTION__, Event));
#endif
    }

    if ( VSyncCount<20 || EventData == NULL )
    {
        /* wait end of transitory speed regime until video speed is really what expected */
        /* because video speed is supposed to be good after 20 VSync */
        return;
    }
    /* STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT received */
    PictPt = (STVID_PictureInfos_t *)(U32)EventData;
    if ( PictPt->VideoParams.MPEGFrame != STVID_MPEG_FRAME_B )
    {
        /* A reference picture is received */
        /* Convert TimeCode to a linear counter */
        FrameNb = PictPt->VideoParams.TimeCode.Seconds ;
        FrameNb = FrameNb + ( PictPt->VideoParams.TimeCode.Minutes * 60 ) ;
        FrameNb = FrameNb + ( PictPt->VideoParams.TimeCode.Hours * 3600 ) ;
        Coeff = PictPt->VideoParams.FrameRate/1000 ;
        if ( PictPt->VideoParams.FrameRate%1000 > 0 )
        {
            /* example : for 29.97Hz, coeff is 30 */
            Coeff++;
        }
        FrameNb = FrameNb * Coeff ;
        FrameNb = FrameNb + PictPt->VideoParams.TimeCode.Frames ;
        /* FrameNb should increase with a constant range at each new event */
        /* Example :    if HH MM SS Fr = 00 00 01 29, FrameNb is 59 in NTSC */
        /* at next ref. pict. it will be 00 00 02 02, FrameNb is 62 */
        /* the difference is equal to 3, and must remain constant*/
        if ( FrameNb < Pict_FrameNb )
        {
            /* TimeCode inside the input GOP is not up-to-date, or injection loop done */;
            Pict_NbOfNegativeDiff++;
            /* ignore it */
        }
        else
        {
            if ( Pict_NbOfNewRef>=2 )
            {
                /* 2 or more measures done, current and previous differences can be compared */
                if ( (FrameNb - Pict_FrameNb) != Pict_Difference )
                {
                    /* difference not constant : it is not fluent */
                    /* (example : bargraph not filled regularly on TV) */
                    Pict_DisplayIsFluent = FALSE;
                }
            }
            if ( Pict_NbOfNewRef>0 )
            {
                /* 1 or more measures done, difference between the 2 TimeCode can be set */
                if ( FrameNb >= Pict_FrameNb )
                {
                    Pict_Difference = FrameNb - Pict_FrameNb ;
                    if ( Pict_Difference > Pict_MaxDifference )
                    {
                        Pict_MaxDifference = Pict_Difference ;
                    }
                    if ( Pict_NbOfNewRef==1 )
                    {
                        Pict_MinDifference = Pict_Difference ;
                    }
                    if ( Pict_Difference < Pict_MinDifference )
                    {
                        Pict_MinDifference = Pict_Difference ;
                    }
                    Pict_TotalOfDifferences += Pict_Difference ;
                }
            }
            Pict_FrameNb = FrameNb ;
            Pict_NbOfNewRef++ ;
        } /* end if good difference */
    } /* end if reference picture */

} /* end of VID_ReceivePictTimeCode() */

/*-------------------------------------------------------------------------
 * Function : VID_ReceiveEvtForTimeMeasurement
 *            Log the occured event (callback function) in order to measure
 *            the total length of activity for a specifuc event.
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void VID_ReceiveEvtForTimeMeasurement ( STEVT_CallReason_t Reason,
                             const ST_DeviceName_t RegistrantName,
                             STEVT_EventConstant_t Event,
                             void *EventData,
                             void *SubscriberData_p )
{
    ST_ErrorCode_t ErrCode;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(EventData);
    UNUSED_PARAMETER(SubscriberData_p);

    /* Process event data before using them ... */
    ErrCode = STVID_PreprocessEvent(RegistrantName, Event, (void *)EventData);
    if (ErrCode != ST_NO_ERROR)
    {
#if defined(ST_OS20) || defined(ST_OSWINCE)
        STTBX_Print(("%s(%d): Preprocess event pb ! 0x%.8x \n", __FILE__, __LINE__, Event));
#else
        STTBX_Print(("%s(): Preprocess event pb ! 0x%.8x \n", __FUNCTION__, Event));
#endif
    }

    /* Only one event is supposed to arrive at this point */
    switch (TimeMeasurementState)
    {
        case TM_ARMED :
            /* Storage of the inital time of this event */
            InitialTime = STOS_time_now();

        case TM_ASK_IF_IN_PROGRESS :
        case TM_IN_PROGRESS :
            /* Modifiy the state of the automaton to signal its progress */
            TimeMeasurementState = TM_IN_PROGRESS;
            /* Storage of the current time */
            IntermediaryTime = STOS_time_now();
            break;

        default :
            /* The event is just consumed, and that's all */
            break;
    }
    if (Event==STVID_DISPLAY_NEW_FRAME_EVT)
    {
        ReceiveNewFrameEvtCount++;
    }

} /* end of VID_ReceiveEvtForTimeMeasurement() */


/*#######################################################################*/
/*########################### EVT FUNCTIONS #############################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : VID_GetEventNumber
 *            Get the event number or name
 *            If it is a name, convert it into a number
 *            If nothing, return the number 0 (it means "all events")
 * Input    : pars_p
 * Output   : EventNumber
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL VID_GetEventNumber(STTST_Parse_t *pars_p, STEVT_EventConstant_t *EventNumber)
{
    BOOL RetErr;
    STEVT_EventConstant_t EvtNo;
    S32 SVar;
    char ItemEvtNo[80];

    memset(ItemEvtNo, 0, sizeof(ItemEvtNo));
    EvtNo = 0;

    /* Get event name or number */

    RetErr = STTST_GetItem( pars_p, "", ItemEvtNo, sizeof(ItemEvtNo));
    if ( strlen(ItemEvtNo)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemEvtNo, &SVar, 10);
        EvtNo = (STEVT_EventConstant_t)SVar;
        if ( RetErr )
        {
            if ( strcmp(ItemEvtNo,"STVID_ASPECT_RATIO_CHANGE_EVT")==0 )
            {
                EvtNo = STVID_ASPECT_RATIO_CHANGE_EVT;
            }
            else if ( strcmp(ItemEvtNo,"STVID_BACK_TO_SYNC_EVT")==0 )
            {
                EvtNo = STVID_BACK_TO_SYNC_EVT;
            }
            else if ( strcmp(ItemEvtNo,"STVID_DATA_ERROR_EVT")==0 )
            {
                EvtNo = STVID_DATA_ERROR_EVT;
            }
            else if ( strcmp(ItemEvtNo,"STVID_DATA_OVERFLOW_EVT")==0 )
            {
                EvtNo = STVID_DATA_OVERFLOW_EVT;
            }
            else if ( strcmp(ItemEvtNo,"STVID_DATA_UNDERFLOW_EVT")==0 )
            {
                EvtNo = STVID_DATA_UNDERFLOW_EVT;
            }
            else if ( strcmp(ItemEvtNo,"STVID_DISPLAY_NEW_FRAME_EVT")==0 )
            {
                EvtNo = STVID_DISPLAY_NEW_FRAME_EVT;
            }
            else if ( strcmp(ItemEvtNo,"STVID_FRAME_RATE_CHANGE_EVT")==0 )
            {
                EvtNo = STVID_FRAME_RATE_CHANGE_EVT;
            }
            else if ( strcmp(ItemEvtNo,"STVID_IMPOSSIBLE_WITH_MEM_PROFILE_EVT")==0 )
            {
                EvtNo = STVID_IMPOSSIBLE_WITH_MEM_PROFILE_EVT;
            }
            else if ( strcmp(ItemEvtNo,"STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT")==0 )
            {
                EvtNo = STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT;
            }
            else if ( strcmp(ItemEvtNo,"STVID_NEW_PICTURE_DECODED_EVT")==0 )
            {
                EvtNo = STVID_NEW_PICTURE_DECODED_EVT;
            }
            else if ( strcmp(ItemEvtNo,"STVID_NEW_PICTURE_ORDERED_EVT")==0 )
            {
                EvtNo = STVID_NEW_PICTURE_ORDERED_EVT;
            }
            else if ( strcmp(ItemEvtNo,"STVID_OUT_OF_SYNC_EVT")==0 )
            {
                EvtNo = STVID_OUT_OF_SYNC_EVT;
            }
            else if ( strcmp(ItemEvtNo,"STVID_PICTURE_DECODING_ERROR_EVT")==0 )
            {
                EvtNo = STVID_PICTURE_DECODING_ERROR_EVT;
            }
            else if ( strcmp(ItemEvtNo,"STVID_RESOLUTION_CHANGE_EVT")==0 )
            {
                EvtNo = STVID_RESOLUTION_CHANGE_EVT;
            }
            else if ( strcmp(ItemEvtNo,"STVID_SCAN_TYPE_CHANGE_EVT")==0 )
            {
                EvtNo = STVID_SCAN_TYPE_CHANGE_EVT;
            }
            else if ( strcmp(ItemEvtNo,"STVID_SEQUENCE_INFO_EVT")==0 )
            {
                EvtNo = STVID_SEQUENCE_INFO_EVT;
            }
            else if ( strcmp(ItemEvtNo,"STVID_SPEED_DRIFT_THRESHOLD_EVT")==0 )
            {
                EvtNo = STVID_SPEED_DRIFT_THRESHOLD_EVT;
            }
            else if ( strcmp(ItemEvtNo,"STVID_STOPPED_EVT")==0 )
            {
                EvtNo = STVID_STOPPED_EVT;
            }
            else if ( strcmp(ItemEvtNo,"STVID_SYNCHRO_EVT")==0 )
            {
                EvtNo = STVID_SYNCHRO_EVT;
            }
            else if ( strcmp(ItemEvtNo,"STVID_USER_DATA_EVT")==0 )
            {
                EvtNo = STVID_USER_DATA_EVT;
            }
            else if ( strcmp(ItemEvtNo, "STVID_SYNCHRONISATION_CHECK_EVT")==0)
            {
                EvtNo = STVID_SYNCHRONISATION_CHECK_EVT;
            }
#ifdef STVID_HARDWARE_ERROR_EVENT
            else if ( strcmp(ItemEvtNo,"STVID_HARDWARE_ERROR_EVT")==0 )
            {
                EvtNo = STVID_HARDWARE_ERROR_EVT;
            }
#endif
            else if ( strcmp(ItemEvtNo,"STVID_SEQUENCE_END_CODE_EVT")==0 )
            {
                EvtNo = STVID_SEQUENCE_END_CODE_EVT;
            }
            else
            {
                STTST_TagCurrentLine( pars_p, "expected event name with no quotes,like STVID_DISPLAY_NEW_FRAME_EVT, or event number" );
                return(API_EnableError ? TRUE : FALSE );
            }
        } /* end if it is an event name */
        if ( EvtNo!=0 && (EvtNo<STVID_DRIVER_BASE && EvtNo>=STVID_DRIVER_BASE+NB_OF_VIDEO_EVENTS) )
        {
            sprintf( VID_Msg,
                     "expected event number (%d to %d), or name, or none if all events are requested",
                     STVID_DRIVER_BASE, STVID_DRIVER_BASE+NB_OF_VIDEO_EVENTS-1 );
            STTST_TagCurrentLine( pars_p, VID_Msg );
            return(API_EnableError ? TRUE : FALSE );
        }
    } /* if length not null */

    *EventNumber = EvtNo;
    return( FALSE );

} /* end of VID_GetEventNumber() */

/*-------------------------------------------------------------------------
 * Function : VID_LogEvtEnable
 *            Enables msg output on event receipt (see VID_ReceiveEvt)
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_LogEvtEnable(STTST_Parse_t *pars_p, char *result_sym_p)
{
BOOL RetErr;

    RetErr = STTST_GetInteger( pars_p, TRUE, &EventLoggingEnabled);
    STTST_AssignInteger(result_sym_p, RetErr, FALSE);

    return(FALSE);
}

/*-------------------------------------------------------------------------
 * Function : VID_LogEvtDisable
 *            Disables msg output on event receipt (see VID_ReceiveEvt)
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_LogEvtDisable(STTST_Parse_t *pars_p, char *result_sym_p)
{
BOOL RetErr;

    RetErr = STTST_GetInteger( pars_p, FALSE, &EventLoggingEnabled);
    STTST_AssignInteger(result_sym_p, RetErr, FALSE);

    return(FALSE);
}

/*-------------------------------------------------------------------------
 * Function : VID_DeleteEvt
 *            Reset the contents of the event stack
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_DeleteEvt(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;

    UNUSED_PARAMETER(pars_p);

    if ( !(EvtEnableAlreadyDone) )
    {
        NewRcvEvtIndex = 0;
        LastRcvEvtIndex = 0;
        NbOfRcvEvents = 0;
        memset(RcvEventStack, 0, sizeof(RcvEventStack));
        memset(RcvWrongEventStack, -1, sizeof(RcvWrongEventStack));
        memset(RcvEventCnt, 0, sizeof(RcvEventCnt));
        STTBX_Print(("VID_DeleteEvt() : events stack purged \n"));
        TimeMeasurementState = TM_NO_ACTIVITY;
        RetErr = FALSE;
    }
    else
    {
        STTBX_Print(("VID_DeleteEvt() : events stack is in used (evt. not disabled) ! \n"));
        RetErr = TRUE;
    }
    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return(API_EnableError ? RetErr : FALSE );

} /* end of VID_DeleteEvt() */

/*-------------------------------------------------------------------------
 * Function : VID_DisableEvt
 *            Disable one event (unsubscribe)
 *            or close the event handler (unsubscribe all events/close/term)
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_DisableEvt(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr;
    /* S16 Count; */
    STEVT_EventConstant_t EvtNo;

    EventLoggingEnabled=FALSE;

    /* Get event name or number, change it into a number */

    RetErr = VID_GetEventNumber(pars_p, &EvtNo);
    if ( RetErr )
    {
        return(API_EnableError ? TRUE : FALSE );
    }

    /* Disable all unreleased events */

    if ( EvtNo == 0 ) /* all events */
    {
        RetErr = ST_NO_ERROR;
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_ASPECT_RATIO_CHANGE_EVT);
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_FRAME_RATE_CHANGE_EVT);
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_SCAN_TYPE_CHANGE_EVT);
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_RESOLUTION_CHANGE_EVT);
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_SEQUENCE_INFO_EVT);
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_USER_DATA_EVT);
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_DATA_ERROR_EVT);
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                         (U32)STVID_DISPLAY_NEW_FRAME_EVT);
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT);
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_IMPOSSIBLE_WITH_MEM_PROFILE_EVT);
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_PICTURE_DECODING_ERROR_EVT);
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_STOPPED_EVT);
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_BACK_TO_SYNC_EVT);
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_OUT_OF_SYNC_EVT);
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_SYNCHRO_EVT);
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_NEW_PICTURE_DECODED_EVT);
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_NEW_PICTURE_ORDERED_EVT);
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_DATA_UNDERFLOW_EVT);
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_DATA_OVERFLOW_EVT);
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_SPEED_DRIFT_THRESHOLD_EVT);
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_SYNCHRONISATION_CHECK_EVT);

/* #if defined(ST_7015) */
            /* RetErr |= STEVT_UnSubscribeDeviceEvent(EvtSubHandle, EventVIDDevice, */
                            /* (STEVT_EventConstant_t)STVID_WORKAROUND_PLEASE_STOP_INJECTION_EVT); */
            /* RetErr |= STEVT_UnSubscribeDeviceEvent(EvtSubHandle, EventVIDDevice, */
                            /* (STEVT_EventConstant_t)STVID_WORKAROUND_THANKS_INJECTION_CAN_GO_ON_EVT); */
/* #endif */

#ifdef STVID_HARDWARE_ERROR_EVENT
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_HARDWARE_ERROR_EVT);
#endif
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVTGDevice,
                            (STEVT_EventConstant_t)STVTG_VSYNC_EVT);
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_SEQUENCE_END_CODE_EVT);

        if ( RetErr != ST_NO_ERROR )
        {
/*              STTBX_Print(("VID_DisableEvt() : unsubscribe event error %x !\n", RetErr)); */
        }
        else
        {
            STTBX_Print(("VID_DisableEvt() : unsubscribe events done\n"));
        }

        /* Close */

/*        RetErr = STEVT_Close(EvtSubHandle);*/
/*        if ( RetErr!= ST_NO_ERROR )*/
/*        {*/
/*            sprintf(VID_Msg, "VID_DisableEvt() : close subscriber evt. handler error %d !\n", RetErr);*/
/*            STTBX_Print((VID_Msg));*/
/*        }*/
/*        else*/
/*        {*/
/*            STTBX_Print( ("VID_DisableEvt() : close subscriber evt. handler done\n"));*/
            EvtEnableAlreadyDone = FALSE;
            EvtOpenAlreadyDone = FALSE;
/*            for ( Count=0 ; Count<NB_OF_VIDEO_EVENTS ; Count++ )*/
/*            {*/
/*                EventID[Count] = -1;*/
/*            }*/
/*        }*/
        if ( VSyncLog != NULL )
        {
            STOS_MemoryDeallocate(SystemPartition_p, VSyncLog);
            VSyncLog = NULL;
        }
    }
    else
    {
        /* Disable only one event */

        RetErr = STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                        (U32)EvtNo);

        if ( RetErr != ST_NO_ERROR )
        {
            sprintf(VID_Msg, "VID_DisableEvt() : unsubscribe event 0x%x error 0x%x !\n", EvtNo, RetErr);
            STTBX_Print((VID_Msg));
        }
        else
        {
            sprintf(VID_Msg, "VID_DisableEvt() : unsubscribe event 0x%x done\n", EvtNo);
            STTBX_Print((VID_Msg));
            EvtEnableAlreadyDone = FALSE;
        }
        if ( EvtNo == STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT
            || EvtNo == STVID_DISPLAY_NEW_FRAME_EVT )
        {
            RetErr = STEVT_UnsubscribeDeviceEvent( EvtSubHandle, EventVTGDevice, (U32)STVTG_VSYNC_EVT);
            if ( RetErr != ST_NO_ERROR )
            {
                sprintf(VID_Msg, "VID_DisableEvt() : unsubscribe event STVTG_VSYNC_EVENT error 0x%x !\n", RetErr);
                STTBX_Print((VID_Msg));
            }
        }
    }

    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return(API_EnableError ? RetErr : FALSE );

} /* end of VID_DisableEvt() */


/*-------------------------------------------------------------------------
 * Function : VID_EnableEvt
 *            If not already done, init the event handler (init/open/reg)
 *            If no argument, enable all events (subscribe)
 *                      else, enable one event (subscribe)
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_EnableEvt(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr;
    STEVT_DeviceSubscribeParams_t DevSubscribeParams;
    STEVT_EventConstant_t EvtNo;
    /* Flag indicating if the measurement of the event is enabled */
    S32 CallbackFunctionNb;
    STVID_Handle_t CurrentHandle;

    /* Get event name or number, change it into a number */
    RetErr = VID_GetEventNumber(pars_p, &EvtNo);
    if ( RetErr )
    {
        return(API_EnableError ? TRUE : FALSE );
    }

    /* Get callback function number */
    RetErr = STTST_GetInteger( pars_p, 0, &CallbackFunctionNb);
    if ( ( RetErr ) || ( CallbackFunctionNb > 4 ) || ( CallbackFunctionNb && !(EvtNo) ) )
    {
        sprintf( VID_Msg,
                 "expected callback function nb (0=event storage 1=time measur. 2=NEW_PICT event measur. 3=VSync count 4=Frames control)");
        STTST_TagCurrentLine( pars_p, VID_Msg );
        return(API_EnableError ? TRUE : FALSE );
    }

    /* Get additional parameter */
    if ( CallbackFunctionNb==1 )
    {
        /* Get time out (for callback function nb 1) */
        RetErr = STTST_GetInteger( pars_p, 0, &TimeOutEvent);
        if ( ( RetErr ) || ( (CallbackFunctionNb == 1) && !EvtNo ) )
        {
            sprintf( VID_Msg,
                    "expected time-out of event validity with a unique event ");
            STTST_TagCurrentLine( pars_p, VID_Msg );
            return(API_EnableError ? TRUE : FALSE );
        }
    }
    if (!TimeOutEvent)
    {
        TimeOutEvent = 5;
    }
    if ( CallbackFunctionNb == 3 )
    {
        /* Get number of VSync measures to log (for callback function nb 3) */
        RetErr = STTST_GetInteger( pars_p, 0, &NbMaxOfVSyncToLog);
        if ( RetErr || NbMaxOfVSyncToLog < 0 )
        {
            sprintf( VID_Msg,
                    "expected max nb of VSync counts to be logged for variance calculation");
            STTST_TagCurrentLine( pars_p, VID_Msg );
            return(API_EnableError ? TRUE : FALSE );
        }
    }

    /* --- Initialization --- */

    RetErr = ST_NO_ERROR ;
    RetErr = VID_GetHandleFromName(EventVIDDevice, &CurrentHandle);
    if (RetErr != ST_NO_ERROR)
    {
        sprintf( VID_Msg,
                 "Can't retrieve video handle associated to %s ", EventVIDDevice);
        STTST_TagCurrentLine( pars_p, VID_Msg );
        return(API_EnableError ? TRUE : FALSE );
    }

    if ( !EvtOpenAlreadyDone )
    {
        /* Open the event handler for the subscriber */
        /* RetErr = STEVT_Open(STEVT_DEVICE_NAME, &OpenParams, &EvtSubHandle); */
        /* if ( RetErr!= ST_NO_ERROR ) */
        /* { */
            /* sprintf(VID_Msg, "VID_EnableEvt() : STEVT_Open() error %d for subscriber !\n", RetErr); */
            /* STTBX_Print((VID_Msg)); */
        /* } */
        /* else */
        /* { */
            EvtOpenAlreadyDone = TRUE;
            /* STTBX_Print(("VID_EnableEvt() : STEVT_Open() for subscriber done\n")); */
        /* } */

    } /* end if init not done */

    /* --- Subscription --- */

    if ((RetErr == ST_NO_ERROR) && (EvtOpenAlreadyDone))
    {
        switch(CallbackFunctionNb)
        {
            case 0 :
                /* event reception in a stack (based on the selected event(s) */
                DevSubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)VID_ReceiveEvt;
                break;
            case 1 :
                /* event measurement for trick mode tests (based on NEW_FRAME event) */
                DevSubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)VID_ReceiveEvtForTimeMeasurement;
                ReceiveNewFrameEvtCount = 0;
                /* Modify the automaton's state to begin its life */
                TimeMeasurementState = TM_ARMED;
                break;
            case 2 :
                /* picture delay measurement (based on NEW_PICTURE event) */
                DevSubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)VID_ReceiveNewPict;
                /* reset measure values */
                Pict_Counted = 0;
                Pict_Delayed = 0;
                Pict_StartTime = 0;
                Pict_PreviousTime = 0;
                Pict_LastTime = 0;
                Pict_FrameDelayed = 0;
                Pict_Advanced = 0;
                Pict_AdvanceTime = 0;
                Pict_DelayTime = 0;
                Pict_FrameDuration = 0;
                Pict_BiggerDelayTime = 0;
                break;
            case 3 :
                /* VSync counting between 2 pictures */
                NbOfVSyncIsConstant = TRUE ;
                NbOfVSyncBetween2Pict = 0 ;
                NbMaxOfVSyncBetween2Pict = 0 ;
                PreviousPictureVSyncCount = 0 ;
                TotalOfVSync = 0 ;
                NbOfNewPict = 0 ;
                VSyncCount = 0; /* in case of NEW_PICT detected by callback before VSYNC */
                DevSubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)VID_ReceivePictAndVSync;
                if ( NbMaxOfVSyncToLog>0 )
                {
                    VSyncLog = (U32 *)STOS_MemoryAllocate(SystemPartition_p, (NbMaxOfVSyncToLog*sizeof(U32))+15 );
                    if ( VSyncLog == NULL )
                    {
                        STTBX_Print(("Unable to allocate memory, %d bytes for logging VSync !\n", (NbMaxOfVSyncToLog*sizeof(U32)) ));
                        RetErr = TRUE;
                    }
                }
                break;
            case 4 :
                /* TimeCode.Frames progress between 2 pictures */
                Pict_FrameNb = 0;
                Pict_Difference = 0;
                Pict_MinDifference = 0;
                Pict_MaxDifference = 0;
                Pict_TotalOfDifferences = 0;
                Pict_DisplayIsFluent = TRUE;
                Pict_NbOfNegativeDiff = 0;
                Pict_NbOfNewRef = 0;
                PreviousPictureVSyncCount = 0 ;
                VSyncCount = 0;
                DevSubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)VID_ReceivePictTimeCode;
                break;
            default:
                RetErr = TRUE;
                break;
        } /* end switch */
        DevSubscribeParams.SubscriberData_p = (void *)CurrentHandle;

        if ( EvtNo == 0 ) /* if all events required ... */
        {
            RetErr =  STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_ASPECT_RATIO_CHANGE_EVT, &DevSubscribeParams);
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_FRAME_RATE_CHANGE_EVT, &DevSubscribeParams);
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_SCAN_TYPE_CHANGE_EVT, &DevSubscribeParams);
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_RESOLUTION_CHANGE_EVT, &DevSubscribeParams);
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_SEQUENCE_INFO_EVT, &DevSubscribeParams);
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_USER_DATA_EVT, &DevSubscribeParams);
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_DATA_ERROR_EVT, &DevSubscribeParams);
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_DISPLAY_NEW_FRAME_EVT, &DevSubscribeParams);
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT, &DevSubscribeParams);
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_IMPOSSIBLE_WITH_MEM_PROFILE_EVT, &DevSubscribeParams);
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_PICTURE_DECODING_ERROR_EVT, &DevSubscribeParams);
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_STOPPED_EVT, &DevSubscribeParams);
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_BACK_TO_SYNC_EVT, &DevSubscribeParams);
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_OUT_OF_SYNC_EVT, &DevSubscribeParams);
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_SYNCHRO_EVT, &DevSubscribeParams);
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_NEW_PICTURE_DECODED_EVT, &DevSubscribeParams);
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_NEW_PICTURE_ORDERED_EVT, &DevSubscribeParams);
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_DATA_UNDERFLOW_EVT, &DevSubscribeParams);
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_DATA_OVERFLOW_EVT, &DevSubscribeParams);
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_SPEED_DRIFT_THRESHOLD_EVT, &DevSubscribeParams);
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_SYNCHRONISATION_CHECK_EVT, &DevSubscribeParams);
/* #if defined(ST_7015) */
            /* RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice, */
                            /* (STEVT_EventConstant_t)STVID_WORKAROUND_PLEASE_STOP_INJECTION_EVT, &DevSubscribeParams); */
            /* RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice, */
                            /* (STEVT_EventConstant_t)STVID_WORKAROUND_THANKS_INJECTION_CAN_GO_ON_EVT, &DevSubscribeParams); */
/* #endif */

#ifdef STVID_HARDWARE_ERROR_EVENT
            RetErr |= STEVT_SubscribeDeviceEvent( EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_HARDWARE_ERROR_EVT, &DevSubscribeParams);
#endif
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)STVID_SEQUENCE_END_CODE_EVT, &DevSubscribeParams);
            VSyncCount = 0;
            VSyncCountForDisplay = -1;
            DevSubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)VID_ReceiveVSync;
            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVTGDevice,
                            (STEVT_EventConstant_t)STVTG_VSYNC_EVT, &DevSubscribeParams);

            if ( RetErr != ST_NO_ERROR )
            {
                sprintf(VID_Msg, "VID_EnableEvt() : subscribe events error 0x%x !\n", RetErr);
                STTBX_Print((VID_Msg));
                EvtEnableAlreadyDone = FALSE;
            }
            else
            {
                STTBX_Print( ("VID_EnableEvt() : subscribe events done\n"));
                EvtEnableAlreadyDone = TRUE;
            }
        }
        else /* else only 1 event required... */
        {
            RetErr = STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVIDDevice,
                            (STEVT_EventConstant_t)EvtNo, &DevSubscribeParams);
            if ( RetErr != ST_NO_ERROR )
            {
                sprintf(VID_Msg, "VID_EnableEvt() : subscribe event 0x%x error 0x%x !\n", EvtNo, RetErr);
                STTBX_Print((VID_Msg));
                EvtEnableAlreadyDone = FALSE;
            }
            else
            {
                sprintf(VID_Msg, "VID_EnableEvt() : subscribe event 0x%x done\n", EvtNo);
                STTBX_Print((VID_Msg));
                EvtEnableAlreadyDone = TRUE;
            }
            /* Assumption : subscribe only to 1 of the following display event */
            if ( EvtNo == STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT
              || EvtNo == STVID_DISPLAY_NEW_FRAME_EVT )
            {
                VSyncCount = 0;
                VSyncCountForDisplay = -1;
                DevSubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)VID_ReceiveVSync;
                RetErr = STEVT_SubscribeDeviceEvent(EvtSubHandle, EventVTGDevice,
                                (STEVT_EventConstant_t)STVTG_VSYNC_EVT, &DevSubscribeParams);
                if ( RetErr != ST_NO_ERROR )
                {
                    sprintf(VID_Msg, "VID_EnableEvt() : subscribe event STVTG_VSYNC_EVT error 0x%x !\n", RetErr);
                    STTBX_Print((VID_Msg));
                }
            }
        }
    }

    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return(API_EnableError ? RetErr : FALSE );

} /* end of VID_EnableEvt() */


/*-------------------------------------------------------------------------
 * Function : VID_ShowEvt
 *            Show event counters & stack
 * Input    :
 * Output   : # of events received
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_ShowEvt(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr;
    char Number[10];
    char ShowOption[10];
    S16 Count;
    S16 WrongCount;

    RetErr = STTST_GetString( pars_p, "", ShowOption, sizeof(ShowOption));
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected option (\"ALL\"=counts+events with data, default=counts+events)" );
        return(FALSE);
    }

    /* --- Statistics (event counts) --- */

    sprintf( VID_Msg,
             "VID_ShowEvt() : ARatio=%d FrameRate=%d ScanType=%d Resol=%d SeqInfo=%d \n",
             RcvEventCnt[STVID_ASPECT_RATIO_CHANGE_EVT-STVID_DRIVER_BASE],
             RcvEventCnt[STVID_FRAME_RATE_CHANGE_EVT-STVID_DRIVER_BASE],
             RcvEventCnt[STVID_SCAN_TYPE_CHANGE_EVT-STVID_DRIVER_BASE],
             RcvEventCnt[STVID_RESOLUTION_CHANGE_EVT-STVID_DRIVER_BASE],
             RcvEventCnt[STVID_SEQUENCE_INFO_EVT-STVID_DRIVER_BASE] );
    STTBX_Print((VID_Msg));
    sprintf( VID_Msg,
             "                UserData=%d DataError=%d NewFrame=%d NewPictToDispl=%d \n",
             RcvEventCnt[STVID_USER_DATA_EVT-STVID_DRIVER_BASE],
             RcvEventCnt[STVID_DATA_ERROR_EVT-STVID_DRIVER_BASE],
             RcvEventCnt[STVID_DISPLAY_NEW_FRAME_EVT-STVID_DRIVER_BASE],
             RcvEventCnt[STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT-STVID_DRIVER_BASE] );
    STTBX_Print((VID_Msg));
    sprintf( VID_Msg,
             "                Impossible=%d PictDecodingError=%d Stopped=%d \n",
             RcvEventCnt[STVID_IMPOSSIBLE_WITH_MEM_PROFILE_EVT-STVID_DRIVER_BASE],
             RcvEventCnt[STVID_PICTURE_DECODING_ERROR_EVT-STVID_DRIVER_BASE],
             RcvEventCnt[STVID_STOPPED_EVT-STVID_DRIVER_BASE] );
    STTBX_Print((VID_Msg));
    sprintf( VID_Msg,
             "                BackToSync=%d OutOfSync=%d SyncCheck=%d Sync=%d\n",
             RcvEventCnt[STVID_BACK_TO_SYNC_EVT-STVID_DRIVER_BASE],
             RcvEventCnt[STVID_OUT_OF_SYNC_EVT-STVID_DRIVER_BASE],
             RcvEventCnt[STVID_SYNCHRONISATION_CHECK_EVT-STVID_DRIVER_BASE],
             RcvEventCnt[STVID_SYNCHRO_EVT-STVID_DRIVER_BASE] );
    STTBX_Print((VID_Msg));
    sprintf( VID_Msg,
             "                NewPictureDecoded=%d Ordered=%d Threshold=%d Overflow=%d Underflow=%d\n",
             RcvEventCnt[STVID_NEW_PICTURE_DECODED_EVT-STVID_DRIVER_BASE],
             RcvEventCnt[STVID_NEW_PICTURE_ORDERED_EVT-STVID_DRIVER_BASE],
             RcvEventCnt[STVID_SPEED_DRIFT_THRESHOLD_EVT-STVID_DRIVER_BASE],
             RcvEventCnt[STVID_DATA_OVERFLOW_EVT-STVID_DRIVER_BASE],
             RcvEventCnt[STVID_DATA_UNDERFLOW_EVT-STVID_DRIVER_BASE]);
    STTBX_Print((VID_Msg));
#ifdef STVID_HARDWARE_ERROR_EVENT
    sprintf( VID_Msg,
             "                HardwareError=%d\n",
             RcvEventCnt[STVID_HARDWARE_ERROR_EVT-STVID_DRIVER_BASE]);
    STTBX_Print((VID_Msg));
#endif
    sprintf( VID_Msg,
             "                EndOfSeq=%d\n",
             RcvEventCnt[STVID_SEQUENCE_END_CODE_EVT-STVID_DRIVER_BASE]);
    STTBX_Print((VID_Msg));
    sprintf( VID_Msg,
             "                Vsync=%d\n",
             VSyncCount);
    STTBX_Print((VID_Msg));

    sprintf( VID_Msg,
             "                Nb of stacked events=%d (index %d to %d)\n",
             NbOfRcvEvents, LastRcvEvtIndex, NewRcvEvtIndex );
    STTBX_Print((VID_Msg));

    /* --- Show events stack --- */

    if ( strcmp(ShowOption,"ALL")==0 || strcmp(ShowOption,"all")==0 )
    {
        /* List of events with their data */
        Count = LastRcvEvtIndex;
        while ( Count < NewRcvEvtIndex )
        {
            VID_TraceEvtData(Count);
            Count++;
        }
    }
    else
    {
        /* List of event numbers */
        strcpy( VID_Msg, "   Events list = ");
        if ( NbOfRcvEvents == 0 )
        {
            strcat( VID_Msg, "(empty)");
        }
        else
        {
            Count = 0;
            while ( Count < NbOfRcvEvents )
            {
                if ( Count%8==0 )
                {
                    strcat(VID_Msg, "\n   ");
                    STTBX_Print((VID_Msg));
                    VID_Msg[0] = '\0';
                }
                sprintf( Number, "0x%lx ", RcvEventStack[LastRcvEvtIndex+Count].EvtNum );
                strcat( VID_Msg, Number );
                Count++;
            }
        }
        strcat( VID_Msg, "\n");
        STTBX_Print((VID_Msg));
    }

    /* List of wrong event numbers */
    STTBX_Print(("   Unknown video events found in the list above = "));
    VID_Msg[0]='\0';
    WrongCount = 0;
    Count = 0;
    while ( Count < NbOfRcvEvents )
    {
        if ( Count%7==0 && VID_Msg[0]!= '\0' )
        {
            strcat(VID_Msg, "\n   ");
            STTBX_Print((VID_Msg));
            VID_Msg[0] = '\0';
        }
        if ( RcvWrongEventStack[Count] != -1 )
        {
            sprintf( Number, "(%d) 0x%lx  ",
                RcvWrongEventStack[Count], RcvEventStack[RcvWrongEventStack[Count]].EvtNum );
            strcat( VID_Msg, Number );
            WrongCount++;
        }
        Count++;
    }
    if ( WrongCount > 0)
    {
        strcat( VID_Msg, "\n");
    }
    else
    {
        strcat( VID_Msg, "(none)\n");
    }
    STTBX_Print((VID_Msg));

    /* number of events remaining in the stack */
    STTST_AssignInteger(result_sym_p, NbOfRcvEvents, FALSE);

    return(API_EnableError ? RetErr : FALSE );

} /* end of VID_ShowEvt() */

/*-------------------------------------------------------------------------
 * Function : VID_CountEvt
 *            Return # of received events
 * Input    :
 * Output   : # of events received
 * Return   : FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_CountEvt(STTST_Parse_t *pars_p, char *result_sym_p)
{
    UNUSED_PARAMETER(pars_p);

    STTST_AssignInteger(result_sym_p, NbOfRcvEvents, FALSE);

    return(FALSE);

} /* end of VID_CountEvt() */

/*-------------------------------------------------------------------------
 * Function : VID_TestEvtAspectRatio
 *            Check contents of aspect ratio. associated to an event
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_TestEvtAspectRatio(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr, CompareErr;
    char ItemValue[80];
    S32 SValue;

    CompareErr = FALSE;
    RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.DisplayAR != SValue )
            {
                STTBX_Print( ("   --> bad DisplayAR %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.DisplayAR, SValue ));
                CompareErr = TRUE;
            }
        }
        else
        {
            if ( strcmp(ItemValue,"NC")==0 )
            {
                /* do Not Check this parameter */
            }
            /* else check enum name (to be coded here) */
        }
    }
    STTST_AssignInteger(result_sym_p, CompareErr, FALSE);
    return ( CompareErr );
} /* end of VID_TestEvtAspectRatio() */

/*-------------------------------------------------------------------------
 * Function : VID_TestEvtUnderflow
 *            Check contents of underflow associated to an event
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_TestEvtUnderflow(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr, CompareErr;
    char ItemValue[80];
    S32 SValue;

    CompareErr = FALSE;
    RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.DataUnderflow.BitBufferFreeSize != SValue)
            {
                STTBX_Print( ("   --> bad BitBufferFreeSize %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.DataUnderflow.BitBufferFreeSize, SValue ));
                CompareErr = TRUE;
            }
        }
        else
        {
            if ( strcmp(ItemValue,"NC")==0 )
            {
                /* do Not Check this parameter */
            }
            /* else check enum name (to be coded here) */
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.DataUnderflow.BitRateValue != SValue )
            {
                STTBX_Print( ("   --> bad BitRateValue %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.DataUnderflow.BitRateValue, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( RcvEventStack[LastRcvEvtIndex].EvtData.DataUnderflow.RequiredTimeJump != SValue )
            {
                STTBX_Print( ("   --> bad RequiredTimeJump %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.DataUnderflow.RequiredTimeJump, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.DataUnderflow.RequiredDuration != SValue )
            {
                STTBX_Print( ("   --> bad RequiredDuration %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.DataUnderflow.RequiredDuration, SValue ));
                CompareErr = TRUE;
            }
        }
    }
    STTST_AssignInteger(result_sym_p, CompareErr, FALSE);
    return ( CompareErr );

} /* end of VID_TestEvtUnderflow() */

/*-------------------------------------------------------------------------
 * Function : VID_TestEvtDriftThreshold
 *            Check contents of DriftThreshold associated to an event
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_TestEvtDriftThreshold(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr, CompareErr;
    char ItemValue[80];
    S32 SValue;

    CompareErr = FALSE;
    RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( RcvEventStack[LastRcvEvtIndex].EvtData.ThresholdInfo.DriftTime != SValue)
            {
                STTBX_Print( ("   --> bad DriftTime %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.ThresholdInfo.DriftTime, SValue ));
                CompareErr = TRUE;
            }
        }
        else
        {
            if ( strcmp(ItemValue,"NC")==0 )
            {
                /* do Not Check this parameter */
            }
            /* else check enum name (to be coded here) */
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.ThresholdInfo.BitRateValue != SValue )
            {
                STTBX_Print( ("   --> bad BitRateValue %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.ThresholdInfo.BitRateValue, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.ThresholdInfo.SpeedRatio != SValue )
            {
                STTBX_Print( ("   --> bad SpeedRatio %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.ThresholdInfo.SpeedRatio, SValue ));
                CompareErr = TRUE;
            }
        }
    }
    STTST_AssignInteger(result_sym_p, CompareErr, FALSE);
    return ( CompareErr );

} /* end of VID_TestEvtDriftThreshold() */

/*-------------------------------------------------------------------------
 * Function : VID_TestEvtPictInfo
 *            Check contents of picture info associated to an event
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_TestEvtPictInfo(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr, CompareErr;
    char ItemValue[80];
    S32 SValue;
#if !defined ST_7100 && !defined(ST_7109) && !defined(ST_7200)
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
#endif

    CompareErr = FALSE;

    /* Whatever the paremters to check, first test ranges */
    if (RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Width > 1920)
    {
        STTBX_Print( ("   --> BitmapParams.Width %d is more than max allowed !\n",
            RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Width));
        CompareErr = TRUE;
    }
    if (RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Height > 1152)
    {
        STTBX_Print( ("   --> BitmapParams.Height %d is more than max allowed !\n",
            RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Height));
        CompareErr = TRUE;
    }
    if (RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Size1 > (1152 * 1920))
    {
        STTBX_Print( ("   --> BitmapParams.Size1 %d is more than max allowed !\n",
            RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Size1));
        CompareErr = TRUE;
    }
    if (RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Size2 > (1152 * 1920))
    {
        STTBX_Print( ("   --> BitmapParams.Size2 %d is more than max allowed !\n",
            RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Size2));
        CompareErr = TRUE;
    }

#if !defined ST_OSLINUX
    /* These tests are no more relevant under Linux */
    /* since Frame buffer addresses are user land mapped addresses */

#if defined (ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_ZEUS)
    if (((U32) RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Data1_p < PARTITION0_START) ||
		((U32) RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Data1_p > PARTITION1_STOP) ||
        (((U32) RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Data1_p > PARTITION0_STOP) &&
        ((U32) RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Data1_p < PARTITION1_START)))
#else
    STAVMEM_GetSharedMemoryVirtualMapping(&(VirtualMapping));
    if (((U32) RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Data1_p < (U32) (VirtualMapping.VirtualBaseAddress_p)) ||
        ((U32) RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Data1_p > (U32) (VirtualMapping.VirtualBaseAddress_p) + VirtualMapping.VirtualSize))
#endif
    {
        STTBX_Print( ("   --> BitmapParams.Data1_p 0x%x is outside Shared Memory !\n",
            RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Data1_p));
        CompareErr = TRUE;
    }

#if defined ST_7100 || defined(ST_7109)  || defined(ST_7200) || defined(ST_ZEUS)
    if (((U32) RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Data2_p < PARTITION0_START) ||
		((U32) RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Data2_p > PARTITION1_STOP) ||
        (((U32) RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Data2_p > PARTITION0_STOP) &&
        ((U32) RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Data2_p < PARTITION1_START)))
#else
    if (((U32) RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Data2_p < (U32) (VirtualMapping.VirtualBaseAddress_p)) ||
        ((U32) RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Data2_p > (U32) (VirtualMapping.VirtualBaseAddress_p) + VirtualMapping.VirtualSize))
#endif
    {
        STTBX_Print( ("   --> BitmapParams.Data2_p 0x%x is outside Shared Memory !\n",
            RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Data2_p));
        CompareErr = TRUE;
    }
#endif  /* ST_OSLINUX */

    if (((U32) RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.FrameRate < 1000) ||
        ((U32) RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.FrameRate > 1000000))
    {
        STTBX_Print( ("   --> VideoParams.FrameRate %d is outside reasonable range !\n",
            RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.FrameRate));
        CompareErr = TRUE;
    }

    /* Now check only the paremters required */
    RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.ColorType != SValue)
            {
                STTBX_Print( ("   --> bad PictInfo.BitmapParams.ColorType %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.ColorType, SValue ));
                CompareErr = TRUE;
            }
        }
        else
        {
            if ( strcmp(ItemValue,"NC")==0 )
            {
                /* do Not Check this parameter */
            }
            /* else check enum name (to be coded here) */
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    /* --- BitmapParams --- */
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.BitmapType != SValue )
            {
                STTBX_Print( ("   --> bad BitmapParams.BitmapType %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.BitmapType, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.PreMultipliedColor != SValue )
            {
                STTBX_Print( ("   --> bad BitmapParams.PreMultipliedColor %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.PreMultipliedColor, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.ColorSpaceConversion != SValue )
            {
                STTBX_Print( ("   --> bad BitmapParams.ColorSpaceConversion %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.ColorSpaceConversion, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.AspectRatio != SValue )
            {
                STTBX_Print( ("   --> bad BitmapParams.AspectRatio %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.AspectRatio, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Width != SValue )
            {
                STTBX_Print( ("   --> bad BitmapParams.Width %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Width, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Height != SValue )
            {
                STTBX_Print( ("   --> bad BitmapParams.Height %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Height, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Pitch != SValue )
            {
                STTBX_Print( ("   --> bad BitmapParams.Pitch %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Pitch, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Offset != SValue )
            {
                STTBX_Print( ("   --> bad BitmapParams.Offset %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Offset, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Data1_p != (void *)SValue )
            {
                STTBX_Print( ("   --> bad BitmapParams.Data1_p 0x%x (expected=0x%x)\n",
                    (int)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Data1_p, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Size1 != SValue )
            {
                STTBX_Print( ("   --> bad BitmapParams.Size1 %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Size1, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Data2_p != (void *)SValue )
            {
                STTBX_Print( ("   --> bad BitmapParams.Data2_p 0x%x (expected=0x%x)\n",
                    (int)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Data2_p, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Size2 != SValue )
            {
                STTBX_Print( ("   --> bad BitmapParams.Size2 %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.Size2, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.SubByteFormat != SValue )
            {
                STTBX_Print( ("   --> bad BitmapParams.SubByteFormat %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.BitmapParams.SubByteFormat, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }

    /* --- VideoParam --- */
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.FrameRate != SValue )
            {
                STTBX_Print( ("   --> bad VideoParams.FrameRate %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.FrameRate, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.ScanType != SValue )
            {
                STTBX_Print( ("   --> bad VideoParams.ScanType %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.ScanType, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.MPEGFrame != SValue )
            {
                STTBX_Print( ("   --> bad VideoParams.MPEGFrame %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.MPEGFrame, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.PictureStructure != SValue )
            {
                STTBX_Print( ("   --> bad VideoParams.PictureStructure %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.PictureStructure, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.TopFieldFirst != SValue )
            {
                STTBX_Print( ("   --> bad VideoParams.TopFieldFirst %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.TopFieldFirst, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.TimeCode.Hours != SValue )
            {
                STTBX_Print( ("   --> bad VideoParams.TimeCode.Hours %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.TimeCode.Hours, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.TimeCode.Minutes != SValue )
            {
                STTBX_Print( ("   --> bad VideoParams.TimeCode.Minutes %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.TimeCode.Minutes, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.TimeCode.Seconds != SValue )
            {
                STTBX_Print( ("   --> bad VideoParams.TimeCode.Seconds %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.TimeCode.Seconds, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.TimeCode.Frames != SValue )
            {
                STTBX_Print( ("   --> bad VideoParams.TimeCode.Frames %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.TimeCode.Frames, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.TimeCode.Interpolated != SValue )
            {
                STTBX_Print( ("   --> bad VideoParams.TimeCode.Interpolated %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.TimeCode.Interpolated, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.PTS.PTS != SValue )
            {
                STTBX_Print( ("   --> bad VideoParams.TimeCode.PTS.PTS %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.PTS.PTS, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.PTS.PTS33 != SValue )
            {
                STTBX_Print( ("   --> bad VideoParams.TimeCode.PTS.PTS33 %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.PTS.PTS33, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.PTS.Interpolated != SValue )
            {
                STTBX_Print( ("   --> bad VideoParams.TimeCode.PTS.Interpolated %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.PTS.Interpolated, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.CompressionLevel != SValue )
            {
                STTBX_Print( ("   --> bad PictInfo.VideoParams.CompressionLevel %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.CompressionLevel, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.DecimationFactors != SValue )
            {
                STTBX_Print( ("   --> bad PictInfo.VideoParams.DecimationFactors %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.DecimationFactors, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.DoesNonDecimatedExist != SValue )
            {
                STTBX_Print( ("   --> bad PictInfo.VideoParams.DoesNonDecimatedExist %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo.VideoParams.DoesNonDecimatedExist, SValue ));
                CompareErr = TRUE;
            }
        }
    }

    PictInfos =  RcvEventStack[LastRcvEvtIndex].EvtData.PictInfo ;
    STTST_AssignInteger(result_sym_p, CompareErr, FALSE);

    return ( CompareErr );

} /* end of VID_TestEvtPictInfo() */

/*-------------------------------------------------------------------------
 * Function : VID_TestEvtPictParam
 *            Check contents of picture param. associated to an event
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_TestEvtPictParam(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr, CompareErr;
    char ItemValue[80];
    S32 SValue;

    CompareErr = FALSE;
    RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.CodingMode != SValue )
            {
                STTBX_Print( ("   --> bad PictParam.CodingMode %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.CodingMode, SValue ));
                CompareErr = TRUE;
            }
        }
        else
        {
            if ( strcmp(ItemValue,"NC")==0 )
            {
                /* do Not Check this parameter */
            }
            /* else check enum name (to be coded here) */
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.ColorType != SValue )
            {
                STTBX_Print( ("   --> bad PictParam.ColorType %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.ColorType, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.FrameRate != SValue )
            {
                STTBX_Print( ("   --> bad PictParam.FrameRate %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.FrameRate, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.Data != (void *)SValue )
            {
                STTBX_Print( ("   --> bad PictParam.Data %d (expected=%d)\n",
                    (int)RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.Data, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.Size != SValue )
            {
                STTBX_Print( ("   --> bad PictParam.Size %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.Size, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.Height != SValue )
            {
                STTBX_Print( ("   --> bad PictParam.Height %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.Height, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.Width != SValue )
            {
                STTBX_Print( ("   --> bad PictParam.Width %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.Width, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.Aspect != SValue )
            {
                STTBX_Print( ("   --> bad PictParam.Aspect %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.Aspect, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.ScanType != SValue )
            {
                STTBX_Print( ("   --> bad PictParam.ScanType %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.ScanType, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.MPEGFrame != SValue )
            {
                STTBX_Print( ("   --> bad PictParam.MPEGFrame %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.MPEGFrame, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.TopFieldFirst != SValue )
            {
                STTBX_Print( ("   --> bad PictParam.TopFieldFirst %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.TopFieldFirst, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.TimeCode.Hours != SValue )
            {
                STTBX_Print( ("   --> bad PictParam.TimeCode.Hours %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.TimeCode.Hours, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.TimeCode.Minutes != SValue )
            {
                STTBX_Print( ("   --> bad PictParam.TimeCode.Minutes %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.TimeCode.Minutes, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.TimeCode.Seconds != SValue )
            {
                STTBX_Print( ("   --> bad PictParam.TimeCode.Seconds %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.TimeCode.Seconds, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.TimeCode.Frames != SValue )
            {
                STTBX_Print( ("   --> bad PictParam.TimeCode.Frames %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.TimeCode.Frames, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.TimeCode.Interpolated != SValue )
            {
                STTBX_Print( ("   --> bad PictParam.TimeCode.Interpolated %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.TimeCode.Interpolated, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.PTS.PTS != SValue )
            {
                STTBX_Print( ("   --> bad PictParam.TimeCode.PTS.PTS %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.PTS.PTS, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.PTS.PTS33 != SValue )
            {
                STTBX_Print( ("   --> bad PictParam.TimeCode.PTS.PTS33 %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.PTS.PTS33, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.PTS.Interpolated != SValue )
            {
                STTBX_Print( ("   --> bad PictParam.TimeCode.PTS.Interpolated %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.PTS.Interpolated, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    /* --- Private data --- */
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.pChromaOffset != SValue )
            {
                STTBX_Print( ("   --> bad PictParam.TimeCode.pChromaOffset %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.pChromaOffset, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.pTemporalReference != SValue )
            {
                STTBX_Print( ("   --> bad PictParam.TimeCode.pTemporalReference %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.PictParam.pTemporalReference, SValue ));
                CompareErr = TRUE;
            }
        }
    }
    STTST_AssignInteger(result_sym_p, CompareErr, FALSE);
    return(CompareErr);

} /* end of VID_TestEvtPictParam() */

/*-------------------------------------------------------------------------
 * Function : VID_TestEvtSeqInfo
 *            Check contents of sequence info. associated to an event
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_TestEvtSeqInfo(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr, CompareErr;
    char ItemValue[80];
    S32 SValue;

    CompareErr = FALSE;
    RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.Height != SValue )
            {
                STTBX_Print( ("   --> bad SeqInfo.Height %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.Height, SValue ));
                CompareErr = TRUE;
            }
        }
        else
        {
            if ( strcmp(ItemValue,"NC")==0 )
            {
                /* do Not Check this parameter */
            }
            /* else check enum name (to be coded here) */
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.Width != SValue )
            {
                STTBX_Print( ("   --> bad SeqInfo.Width %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.Width, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.Aspect != SValue )
            {
                STTBX_Print( ("   --> bad SeqInfo.Aspect %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.Aspect, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.ScanType != SValue )
            {
                STTBX_Print( ("   --> bad SeqInfo.ScanType %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.ScanType, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.FrameRate != SValue )
            {
                STTBX_Print( ("   --> bad SeqInfo.FrameRate %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.FrameRate, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.BitRate != SValue )
            {
                STTBX_Print( ("   --> bad SeqInfo.BitRate %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.BitRate, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.MPEGStandard != SValue )
            {
                STTBX_Print( ("   --> bad SeqInfo.MPEGStandard %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.MPEGStandard, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.IsLowDelay != SValue )
            {
                STTBX_Print( ("   --> bad SeqInfo.IsLowDelay %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.IsLowDelay, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.VBVBufferSize != SValue )
            {
                STTBX_Print( ("   --> bad SeqInfo.VBVBufferSize %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.VBVBufferSize, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.StreamID != SValue )
            {
                STTBX_Print( ("   --> bad SeqInfo.StreamID %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.StreamID, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.ProfileAndLevelIndication != SValue )
            {
                STTBX_Print( ("   --> bad SeqInfo.ProfileAndLevelIndication %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.ProfileAndLevelIndication, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.FrameRateExtensionN != SValue )
            {
                STTBX_Print( ("   --> bad SeqInfo.FrameRateExtensionN %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.FrameRateExtensionN, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.FrameRateExtensionD != SValue )
            {
                STTBX_Print( ("   --> bad SeqInfo.FrameRateExtensionD %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.FrameRateExtensionD, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.VideoFormat != SValue )
            {
                STTBX_Print( ("   --> bad SeqInfo.VideoFormat %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.SeqInfo.VideoFormat, SValue ));
                CompareErr = TRUE;
            }
        }
    }
    STTST_AssignInteger(result_sym_p, CompareErr, FALSE);
    return ( CompareErr );

} /* end of VID_TestEvtSeqInfo() */

/*-------------------------------------------------------------------------
 * Function : VID_TestEvtSynchro
 *            Check contents of synchro data associated to an event
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_TestEvtSynchro(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr, CompareErr;
    char ItemValue[80];
    S32 SValue;

    CompareErr = FALSE;
    RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.SyncInfo.Action != SValue )
            {
                STTBX_Print( ("   --> bad SyncInfo.Action %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.SyncInfo.Action, SValue ));
                CompareErr = TRUE;
            }
        }
        else
        {
            if ( strcmp(ItemValue,"NC")==0 )
            {
                /* do Not Check this parameter */
            }
            /* else check enum name (to be coded here) */
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.SyncInfo.EffectiveTime != SValue )
            {
                STTBX_Print( ("   --> bad SyncInfo.EffectiveTime %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.SyncInfo.EffectiveTime, SValue ));
                CompareErr = TRUE;
            }
        }
    }
    STTST_AssignInteger(result_sym_p, CompareErr, FALSE);
    return ( CompareErr );

} /* end of VID_TestEvtSynchro() */

/*-------------------------------------------------------------------------
 * Function : VID_TestEvtUserData
 *            Check contents of user data associated to an event
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_TestEvtUserData(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr, RetErr2, CompareErr;
    char ItemValue[80];
    S32 SPosition;
    S32 SLength;
    S32 SValue;
    S16 Count;
    char *Pt;

    CompareErr = FALSE;
    RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.UserData.BroadcastProfile != SValue )
            {
                STTBX_Print( ("   --> bad UserData.BroadcastProfile %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.UserData.BroadcastProfile, SValue ));
                CompareErr = TRUE;
            }
        }
        else
        {
            if ( strcmp(ItemValue,"NC")==0 )
            {
                /* do Not Check this parameter */
            }
            /* else check enum name (to be coded here) */
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SPosition, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.UserData.PositionInStream != SPosition )
            {
                STTBX_Print( ("   --> bad UserData.PositionInStream %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.UserData.PositionInStream, SPosition ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( SPosition!=STVID_USER_DATA_AFTER_SEQUENCE)
            {
                if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.UserData.pTemporalReference != SValue )
                {
                    STTBX_Print( ("   --> bad UserData.pTemporalReference %d (expected=%d)\n",
                        RcvEventStack[LastRcvEvtIndex].EvtData.UserData.pTemporalReference, SValue ));
                    CompareErr = TRUE;
                }
            }
            else
            {
                STTBX_Print(("   UserData.pTemporalReference value not meaningful here\n"));
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SLength, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.UserData.Length != SLength )
            {
                STTBX_Print( ("   --> bad UserData.Length %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.UserData.Length, SLength ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.UserData.BufferOverflow != SValue )
            {
                STTBX_Print( ("   --> bad UserData.BufferOverflow %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.UserData.BufferOverflow, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( RcvEventStack[LastRcvEvtIndex].EvtData.UserData.Buff_p != (void *)SValue )
            {
                STTBX_Print( ("   --> bad UserData.Buff_p %d (expected=%d)\n",
                    (int)RcvEventStack[LastRcvEvtIndex].EvtData.UserData.Buff_p, SValue ));
                CompareErr = TRUE;
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( SPosition!=STVID_USER_DATA_AFTER_SEQUENCE)
            {
                if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.UserData.PTS.PTS != SValue )
                {
                    STTBX_Print( ("   --> bad UserData.PTS.PTS %d (expected=%d)\n",
                        RcvEventStack[LastRcvEvtIndex].EvtData.UserData.PTS.PTS, SValue ));
                    CompareErr = TRUE;
                }
            }
            else
            {
                STTBX_Print(("   UserData.PTS.PTS value not meaningful here\n"));
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( SPosition!=STVID_USER_DATA_AFTER_SEQUENCE)
            {
                if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.UserData.PTS.PTS33 != SValue )
                {
                    STTBX_Print( ("   --> bad UserData.PTS.PTS33 %d (expected=%d)\n",
                        RcvEventStack[LastRcvEvtIndex].EvtData.UserData.PTS.PTS33, SValue ));
                    CompareErr = TRUE;
                }
            }
            else
            {
                STTBX_Print(("   UserData.PTS.PTS33 value not meaningful here\n"));
            }
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.UserData.PTS.Interpolated != SValue )
            {
                STTBX_Print( ("   --> bad UserData.PTS.Interpolated %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.UserData.PTS.Interpolated, SValue ));
                CompareErr = TRUE;
            }
        }
        /*RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));*/
    }
    if ( !(RetErr) )
    {
        RetErr2 = STTST_GetInteger( pars_p, -1, (S32 *)&SValue);
        Count = 0;
        /* while there is input argument on command line */
        Pt = (char *)RcvUserDataStack[LastRcvEvtIndex] ;
        while ( (!(RetErr2) ) && (Count < SLength) && (SValue!= -1) )
        {
            if ( (U8)*Pt!=(U8)SValue ) /* if received is not expected */
            {
                STTBX_Print(("   --> bad user data 0x%02x in byte %d (expected=0x%02x)\n",
                    (U8)*Pt, Count+1, (U8)SValue ));
                CompareErr = TRUE;
                break;
            }
            Pt++; /* next user data received */
            /* get next input data if any */
            RetErr2 = STTST_GetInteger( pars_p, -1, (S32 *)&SValue);
            Count++;
        }
    }
    STTST_AssignInteger(result_sym_p, CompareErr, FALSE);

    return ( CompareErr );

} /* end of VID_TestEvtUserData() */

/*-------------------------------------------------------------------------
 * Function : VID_TestEvtImpossibleWithProfile
 *            Check contents of user data associated to an event
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_TestEvtImpossibleWithProfile(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr, CompareErr;
    char ItemValue[80];
    S32 SValue;

    CompareErr = FALSE;
    RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.MemoryProfile.MaxWidth != SValue )
            {
                STTBX_Print( ("   --> bad MemoryProfile.MaxWidth %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.MemoryProfile.MaxWidth, SValue ));
                CompareErr = TRUE;
            }
        }
        else
        {
            if ( strcmp(ItemValue,"NC")==0 )
            {
                /* do Not Check this parameter */
            }
            /* else check enum name (to be coded here) */
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.MemoryProfile.MaxHeight != SValue )
            {
                STTBX_Print( ("   --> bad MemoryProfile.MaxHeight %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.MemoryProfile.MaxHeight, SValue ));
                CompareErr = TRUE;
            }
        }
        else
        {
            if ( strcmp(ItemValue,"NC")==0 )
            {
                /* do Not Check this parameter */
            }
            /* else check enum name (to be coded here) */
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            /* Test the cases of specific ID set to NbFrameStore */
            switch (RcvEventStack[LastRcvEvtIndex].EvtData.MemoryProfile.NbFrameStore)
            {
                /* case of Optimised ID for frame buffers */
                case STVID_OPTIMISED_NB_FRAME_STORE_ID :
                    if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.MemoryProfile.FrameStoreIDParams.OptimisedNumber.Main != SValue )
                    {
                        STTBX_Print( ("   --> bad MemoryProfile.OptimisedNumber.Main %d (expected=%d)\n",
                            RcvEventStack[LastRcvEvtIndex].EvtData.MemoryProfile.FrameStoreIDParams.OptimisedNumber.Main, SValue ));
                        CompareErr = TRUE;
                    }
                    break;

                default:
                    if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.MemoryProfile.NbFrameStore != SValue )
                    {
                        STTBX_Print( ("   --> bad MemoryProfile.NbFrameStore %d (expected=%d)\n",
                            RcvEventStack[LastRcvEvtIndex].EvtData.MemoryProfile.NbFrameStore, SValue ));
                        CompareErr = TRUE;
                    }
                    break;
            }
        }
        else
        {
            if ( strcmp(ItemValue,"NC")==0 )
            {
                /* do Not Check this parameter */
            }
            /* else check enum name (to be coded here) */
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.MemoryProfile.CompressionLevel != SValue )
            {
                STTBX_Print( ("   --> bad MemoryProfile.CompressionLevel %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.MemoryProfile.CompressionLevel, SValue ));
                CompareErr = TRUE;
            }
        }
        else
        {
            if ( strcmp(ItemValue,"NC")==0 )
            {
                /* do Not Check this parameter */
            }
            /* else check enum name (to be coded here) */
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( (S32)RcvEventStack[LastRcvEvtIndex].EvtData.MemoryProfile.DecimationFactor != SValue )
            {
                STTBX_Print( ("   --> bad MemoryProfile.DecimationFactor %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.MemoryProfile.DecimationFactor, SValue ));
                CompareErr = TRUE;
            }
        }
        else
        {
            if ( strcmp(ItemValue,"NC")==0 )
            {
                /* do Not Check this parameter */
            }
            /* else check enum name (to be coded here) */
        }
        /*RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));*/
    }
    STTST_AssignInteger(result_sym_p, CompareErr, FALSE);
    return ( CompareErr );

} /* end of VID_TestEvtImpossibleWithProfile() */

/*-------------------------------------------------------------------------
 * Function : VID_TestEvtSynchronisationCheck
 *            Check contents of synchro data associated to an event
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_TestEvtSynchronisationCheck(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr, CompareErr;
    char ItemValue[80];
    S32 SValue;

    CompareErr = FALSE;
    RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( RcvEventStack[LastRcvEvtIndex].EvtData.SynchroCheckInfo.ClocksDifference != SValue )
            {
                STTBX_Print( ("   --> bad SyncInfo.ClocksDifference %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.SynchroCheckInfo.ClocksDifference, SValue ));
                CompareErr = TRUE;
            }
        }
        else
        {
            if ( strcmp(ItemValue,"NC")==0 )
            {
                /* do Not Check this parameter */
            }
            /* else check enum name (to be coded here) */
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( RcvEventStack[LastRcvEvtIndex].EvtData.SynchroCheckInfo.IsSynchronisationOk != SValue )
            {
                STTBX_Print( ("   --> bad SyncInfo.IsSynchronisationOk %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.SynchroCheckInfo.IsSynchronisationOk, SValue ));
                CompareErr = TRUE;
            }
        }
        else
        {
            if ( strcmp(ItemValue,"NC")==0 )
            {
                /* do Not Check this parameter */
            }
            /* else check enum name (to be coded here) */
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( RcvEventStack[LastRcvEvtIndex].EvtData.SynchroCheckInfo.IsLoosingSynchronisation != SValue )
            {
                STTBX_Print( ("   --> bad SyncInfo.IsLoosingSynchronisation %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.SynchroCheckInfo.IsLoosingSynchronisation, SValue ));
                CompareErr = TRUE;
            }
        }
        else
        {
            if ( strcmp(ItemValue,"NC")==0 )
            {
                /* do Not Check this parameter */
            }
            /* else check enum name (to be coded here) */
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    if ( strlen(ItemValue)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemValue, &SValue, 10);
        if ( !(RetErr) )
        {
            if ( RcvEventStack[LastRcvEvtIndex].EvtData.SynchroCheckInfo.IsBackToSynchronisation != SValue )
            {
                STTBX_Print( ("   --> bad SyncInfo.IsBackToSynchronisation %d (expected=%d)\n",
                    RcvEventStack[LastRcvEvtIndex].EvtData.SynchroCheckInfo.IsBackToSynchronisation, SValue ));
                CompareErr = TRUE;
            }
        }
        else
        {
            if ( strcmp(ItemValue,"NC")==0 )
            {
                /* do Not Check this parameter */
            }
            /* else check enum name (to be coded here) */
        }
        RetErr = STTST_GetItem( pars_p, "NC", ItemValue, sizeof(ItemValue));
    }
    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    VID_TraceEvtData(LastRcvEvtIndex);;

    return ( CompareErr );
} /* end of VID_TestEvtSynchronisationCheck() */

/*-------------------------------------------------------------------------
 * Function : vid_GenericTestEvt
 *            Wait for an event in the stack
 *            Test if it is the expected event number
 *            Test if associated are equal to expected values (optionnal)
 *            Purge the event from the stack
 * Input    :
 * Output   : FALSE if no checking error,
 *            RET_VAL1 FALSE if no event
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL vid_GenericTestEvt(STTST_Parse_t *pars_p, char *result_sym_p, BOOL verbose)
{
    ST_ErrorCode_t RetErr;
    STEVT_EventConstant_t ReqEvtNo;
    S16 Count;

    /* Get event name or number, change it into a number */
    RetErr = VID_GetEventNumber(pars_p, &ReqEvtNo);
    if ( RetErr )
    {
        return(API_EnableError ? TRUE : FALSE );
    }

    /* --- Wait until the stack is not empty --- */

    Count = 0;
    while ( NbOfRcvEvents == 0 && Count < 100 ) /* wait the 1st event */
    {
        STOS_TaskDelayUs(DELAY_1S/10); /* 100ms */
        Count++;
    }
    if ( NbOfRcvEvents == 0 && Count >= 100 )
    {
        sprintf(VID_Msg, "VID_TestEvt() : event 0x%x not happened (timeout) ! \n", ReqEvtNo);
        STTBX_Print((VID_Msg));
   	    API_ErrorCount++;
        RetErr = TRUE;
        STTST_AssignInteger("RET_VAL1", FALSE, FALSE); /* event not detected */
    }
    else
    {
        /* --- Is it the expected event ? --- */

        if ( RcvEventStack[LastRcvEvtIndex].EvtNum != (long)ReqEvtNo )
        {
            sprintf(VID_Msg, "VID_TestEvt() : event 0x%lx detected instead of 0x%x ! \n",
                           RcvEventStack[LastRcvEvtIndex].EvtNum, ReqEvtNo);
            STTBX_Print((VID_Msg));
   		    API_ErrorCount++;
            RetErr = FALSE;
        }
        else
        {
            if (verbose)
            {
            sprintf(VID_Msg, "VID_TestEvt() : event 0x%x detected \n", ReqEvtNo );
            STTBX_Print((VID_Msg));
            }

            /* --- Parse and Control the associated data --- */
            switch( ReqEvtNo )
            {
                case STVID_ASPECT_RATIO_CHANGE_EVT :
                    RetErr = VID_TestEvtAspectRatio( pars_p, result_sym_p);
                    break;
                case STVID_SEQUENCE_INFO_EVT :
                    RetErr = VID_TestEvtSeqInfo( pars_p, result_sym_p);
                    break;
                case STVID_DISPLAY_NEW_FRAME_EVT :
                case STVID_FRAME_RATE_CHANGE_EVT :
                case STVID_PICTURE_DECODING_ERROR_EVT:
                case STVID_RESOLUTION_CHANGE_EVT :
                case STVID_SCAN_TYPE_CHANGE_EVT :
                case STVID_STOPPED_EVT:
                    RetErr = VID_TestEvtPictParam( pars_p, result_sym_p);
                    break;
                case STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT:
                case STVID_NEW_PICTURE_DECODED_EVT:
                case STVID_NEW_PICTURE_ORDERED_EVT:
                    RetErr = VID_TestEvtPictInfo( pars_p, result_sym_p);
                    break;
                case STVID_SYNCHRO_EVT:
                    RetErr = VID_TestEvtSynchro( pars_p, result_sym_p);
                    break;
                case STVID_SPEED_DRIFT_THRESHOLD_EVT:
                    RetErr = VID_TestEvtDriftThreshold( pars_p, result_sym_p);
                    break;
                case STVID_DATA_UNDERFLOW_EVT:
                    RetErr = VID_TestEvtUnderflow( pars_p, result_sym_p);
                    break;
                case STVID_USER_DATA_EVT:
                    RetErr = VID_TestEvtUserData( pars_p, result_sym_p);
                    break;
                case STVID_IMPOSSIBLE_WITH_MEM_PROFILE_EVT :
                    RetErr = VID_TestEvtImpossibleWithProfile( pars_p, result_sym_p);
                    break;
                case STVID_SYNCHRONISATION_CHECK_EVT :
                    RetErr = VID_TestEvtSynchronisationCheck( pars_p, result_sym_p);
                    break;
                default:
                    break;
            }
            if (RetErr )
            {
                API_ErrorCount++;
                /* --- Trace the data in case of error --- */
                VID_TraceEvtData(LastRcvEvtIndex);
            }
        } /* end else evt ok */

        /* --- Purge last element in stack (bottom) --- */

        STOS_InterruptLock();
        if ( LastRcvEvtIndex < NewRcvEvtIndex )
        {
            LastRcvEvtIndex++;
        }
        NbOfRcvEvents--;
        STOS_InterruptUnlock();

        STTST_AssignInteger("RET_VAL1", TRUE, FALSE); /* event detected */

    } /* end if stack not empty */

    STTST_AssignInteger(result_sym_p, RetErr, FALSE);

    return(API_EnableError ? RetErr : FALSE );

} /* end of vid_GenericTestEvt(() */

/*-------------------------------------------------------------------------
 * Function : VID_TestEvt
 *            Wait for an event in the stack
 *            Test if it is the expected event number
 *            Test if associated are equal to expected values (optionnal)
 *            Purge the event from the stack
 * Input    :
 * Output   : FALSE if no checking error,
 *            RET_VAL1 FALSE if no event
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_TestEvt(STTST_Parse_t *pars_p, char *result_sym_p)
{
    return( vid_GenericTestEvt(pars_p, result_sym_p, TRUE) );

} /* end of VID_TestEvt() */

/*-------------------------------------------------------------------------
 * Function : VID_TestQuietEvt
 *            Same as VID_TestEvt but without output if test is successful
 *            (tests run faster thanks to this function)
 * Input    :
 * Output   : FALSE if no checking error,
 *            RET_VAL1 FALSE if no event
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_TestQuietEvt(STTST_Parse_t *pars_p, char *result_sym_p)
{
    return( vid_GenericTestEvt(pars_p, result_sym_p, FALSE) );

} /* end of VID_TestQuietEvt() */

/*-------------------------------------------------------------------------
 * Function : VID_TraceEvtData
 *            Display evt name with its associated data found in the stack
 *
 * Input    : EvtIndex (position in the stack)
 * Output   :
 * Return   : FALSE if no error, TRUE if error
 * ----------------------------------------------------------------------*/
static BOOL VID_TraceEvtData(S16 EvtIndex)
{
    BOOL RetErr;
    U32 Count;
    char *Pt;

    RetErr = FALSE;
    if ( EvtIndex <0 || EvtIndex >= NB_MAX_OF_STACKED_EVENTS )
    {
        return(TRUE);
    }

    /* Event name */
    switch(RcvEventStack[EvtIndex].EvtNum)
    {
        case STVID_BACK_TO_SYNC_EVT:
            sprintf( VID_Msg, "  STVID_BACK_TO_SYNC_EVT : \n   no data\n");
            break;
        case STVID_ASPECT_RATIO_CHANGE_EVT:
            sprintf( VID_Msg, "  STVID_ASPECT_RATIO_CHANGE_EVT :");
            break;
        case STVID_FRAME_RATE_CHANGE_EVT:
            sprintf( VID_Msg, "  STVID_FRAME_RATE_CHANGE_EVT :");
            break;
        case STVID_SCAN_TYPE_CHANGE_EVT:
            sprintf( VID_Msg, "  STVID_SCAN_TYPE_CHANGE_EVT :");
            break;
        case STVID_RESOLUTION_CHANGE_EVT:
            sprintf( VID_Msg, "  STVID_RESOLUTION_CHANGE_EVT :");
            break;
        case STVID_STOPPED_EVT:
            sprintf( VID_Msg, "  STVID_STOPPED_EVT :");
            break;
        case STVID_DISPLAY_NEW_FRAME_EVT:
            sprintf( VID_Msg, "  STVID_DISPLAY_NEW_FRAME_EVT : ");
            break;
        case STVID_PICTURE_DECODING_ERROR_EVT:
            sprintf( VID_Msg, "  STVID_PICTURE_DECODING_ERROR_EVT : ");
            break;
        case STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT:
            sprintf( VID_Msg, "  STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT : ");
            break;
        case STVID_SEQUENCE_INFO_EVT:
            sprintf( VID_Msg, "  STVID_SEQUENCE_INFO_EVT : ");
            break;
        case STVID_USER_DATA_EVT:
            sprintf( VID_Msg, "  STVID_USER_DATA_EVT : ");
            break;
        case STVID_SYNCHRO_EVT:
            sprintf( VID_Msg, "  STVID_SYNCHRO_EVT : \n   no data\n");
            break;
        case STVID_DATA_ERROR_EVT:
            sprintf( VID_Msg, "  STVID_DATA_ERROR_EVT : \n   no data\n");
            break;
        case STVID_DATA_OVERFLOW_EVT:
            sprintf( VID_Msg, "  STVID_DATA_OVERFLOW_EVT : \n   no data\n");
            break;
        case STVID_DATA_UNDERFLOW_EVT:
            sprintf( VID_Msg, "  STVID_DATA_UNDERFLOW_EVT : ");
            break;
        case STVID_IMPOSSIBLE_WITH_MEM_PROFILE_EVT:
            sprintf( VID_Msg, "  STVID_IMPOSSIBLE_WITH_MEM_PROFILE_EVT : ");
            break;
        case STVID_NEW_PICTURE_DECODED_EVT:
            sprintf( VID_Msg, "  STVID_NEW_PICTURE_DECODED_EVT : ");
            break;
        case STVID_NEW_PICTURE_ORDERED_EVT:
            sprintf( VID_Msg, "  STVID_NEW_PICTURE_ORDERED_EVT : ");
            break;
        case STVID_OUT_OF_SYNC_EVT:
            sprintf( VID_Msg, "  STVID_OUT_OF_SYNC_EVT : \n   no data\n");
            break;
        case STVID_SPEED_DRIFT_THRESHOLD_EVT:
            sprintf( VID_Msg, "  STVID_SPEED_DRIFT_THRESHOLD_EVT : ");
            break;
        case STVID_SYNCHRONISATION_CHECK_EVT:
            sprintf( VID_Msg, "  STVID_SYNCHRONISATION_CHECK_EVT : ");
            break;
        case STVID_SEQUENCE_END_CODE_EVT:
            sprintf( VID_Msg, "  STVID_SEQUENCE_END_CODE_EVT : \n   no data\n");
            break;

        default:
            sprintf( VID_Msg, "  ! UNKNOWN EVENT 0x%lx ! : \n   data can't be analysed\n",
                     RcvEventStack[EvtIndex].EvtNum);
            break;
    }

    /* Event data */
    switch(RcvEventStack[EvtIndex].EvtNum)
    {
          case STVID_DATA_UNDERFLOW_EVT:
            sprintf( VID_Msg, "%s BBFreeSize=%d BitRate(bps)=%d ReqTimeJump=%d ReqDuration=%d \n",
                     VID_Msg,
                     RcvEventStack[EvtIndex].EvtData.DataUnderflow.BitBufferFreeSize,
                     RcvEventStack[EvtIndex].EvtData.DataUnderflow.BitRateValue,
                     RcvEventStack[EvtIndex].EvtData.DataUnderflow.RequiredTimeJump,
                     RcvEventStack[EvtIndex].EvtData.DataUnderflow.RequiredDuration );
            STTST_Print((VID_Msg));
            break;
         case STVID_IMPOSSIBLE_WITH_MEM_PROFILE_EVT:
            /* Test the cases of specific ID set to NbFrameStore */
            switch (RcvEventStack[LastRcvEvtIndex].EvtData.MemoryProfile.NbFrameStore)
            {
                /* case of Optimised ID for frame buffers */
                case STVID_OPTIMISED_NB_FRAME_STORE_ID :
                    if (RcvEventStack[LastRcvEvtIndex].EvtData.MemoryProfile.DecimationFactor != STVID_DECIMATION_FACTOR_NONE)
                    {
                        sprintf( VID_Msg, "%s Size (%d/%d) NbFrame=%d Comp=%d Dec=%d OptNbFrame.Main=%d OptNbFrame.Dec=%d \n",
                                 VID_Msg,
                                 RcvEventStack[EvtIndex].EvtData.MemoryProfile.MaxWidth,
                                 RcvEventStack[EvtIndex].EvtData.MemoryProfile.MaxHeight,
                                 RcvEventStack[EvtIndex].EvtData.MemoryProfile.NbFrameStore,
                                 RcvEventStack[EvtIndex].EvtData.MemoryProfile.CompressionLevel,
                                 RcvEventStack[EvtIndex].EvtData.MemoryProfile.DecimationFactor,
                                 RcvEventStack[EvtIndex].EvtData.MemoryProfile.FrameStoreIDParams.OptimisedNumber.Main,
                                 RcvEventStack[EvtIndex].EvtData.MemoryProfile.FrameStoreIDParams.OptimisedNumber.Decimated );
                    }
                    else
                    {
                       sprintf( VID_Msg, "%s Size (%d/%d) NbFrame=%d Comp=%d Dec=%d OptNbFrame.Main=%d \n",
                                VID_Msg,
                                RcvEventStack[EvtIndex].EvtData.MemoryProfile.MaxWidth,
                                RcvEventStack[EvtIndex].EvtData.MemoryProfile.MaxHeight,
                                RcvEventStack[EvtIndex].EvtData.MemoryProfile.NbFrameStore,
                                RcvEventStack[EvtIndex].EvtData.MemoryProfile.CompressionLevel,
                                RcvEventStack[EvtIndex].EvtData.MemoryProfile.DecimationFactor,
                                RcvEventStack[EvtIndex].EvtData.MemoryProfile.FrameStoreIDParams.OptimisedNumber.Main );
                    }
                    break;

                default:
                    sprintf( VID_Msg, "%s Size (%d/%d) NbFrame=%d Comp=%d Dec=%d \n",
                             VID_Msg,
                             RcvEventStack[EvtIndex].EvtData.MemoryProfile.MaxWidth,
                             RcvEventStack[EvtIndex].EvtData.MemoryProfile.MaxHeight,
                             RcvEventStack[EvtIndex].EvtData.MemoryProfile.NbFrameStore,
                             RcvEventStack[EvtIndex].EvtData.MemoryProfile.CompressionLevel,
                             RcvEventStack[EvtIndex].EvtData.MemoryProfile.DecimationFactor );
                    break;
            }
            STTBX_Print((VID_Msg));
            break;
        case STVID_ASPECT_RATIO_CHANGE_EVT:
            sprintf( VID_Msg, "%s DisplayAR=%d \n",
                     VID_Msg,
                     RcvEventStack[EvtIndex].EvtData.DisplayAR );
            STTBX_Print((VID_Msg));
            break;
        case STVID_FRAME_RATE_CHANGE_EVT:
        case STVID_SCAN_TYPE_CHANGE_EVT:
        case STVID_RESOLUTION_CHANGE_EVT:
        case STVID_STOPPED_EVT:
        case STVID_DISPLAY_NEW_FRAME_EVT:
        case STVID_PICTURE_DECODING_ERROR_EVT:
            sprintf( VID_Msg, "%s CodingMode=%d ColorType=%d FrameRate=%d\n   Data=0x%x Size=%d Height=%d Width=%d \n",
                    VID_Msg,
                    RcvEventStack[EvtIndex].EvtData.PictParam.CodingMode,
                    RcvEventStack[EvtIndex].EvtData.PictParam.ColorType,
                    (S32)RcvEventStack[EvtIndex].EvtData.PictParam.FrameRate,
                    (S32)RcvEventStack[EvtIndex].EvtData.PictParam.Data,
                    (S32)RcvEventStack[EvtIndex].EvtData.PictParam.Size,
                    RcvEventStack[EvtIndex].EvtData.PictParam.Height,
                    RcvEventStack[EvtIndex].EvtData.PictParam.Width );
            STTBX_Print((VID_Msg));
            sprintf( VID_Msg, "   Aspect=%d ScanType=%d MPEGFrame=%d TopFieldFirst=%d\n",
                    RcvEventStack[EvtIndex].EvtData.PictParam.Aspect,
                    RcvEventStack[EvtIndex].EvtData.PictParam.ScanType,
                    (S32)RcvEventStack[EvtIndex].EvtData.PictParam.MPEGFrame,
                    (S32)RcvEventStack[EvtIndex].EvtData.PictParam.TopFieldFirst );
            STTBX_Print((VID_Msg));
            sprintf( VID_Msg, "   TimeCode : Hours=%d Minutes=%d Seconds=%d Frames=%d Interpolated=%d \n   PTS : PTS=%d PTS33=%d Interpolated=%d \n",
                    RcvEventStack[EvtIndex].EvtData.PictParam.TimeCode.Hours,
                    RcvEventStack[EvtIndex].EvtData.PictParam.TimeCode.Minutes,
                    (S32)RcvEventStack[EvtIndex].EvtData.PictParam.TimeCode.Seconds,
                    (S32)RcvEventStack[EvtIndex].EvtData.PictParam.TimeCode.Frames,
                    (S32)RcvEventStack[EvtIndex].EvtData.PictParam.TimeCode.Interpolated,
                    RcvEventStack[EvtIndex].EvtData.PictParam.PTS.PTS,
                    RcvEventStack[EvtIndex].EvtData.PictParam.PTS.PTS33,
					RcvEventStack[EvtIndex].EvtData.PictParam.PTS.Interpolated );
            STTBX_Print((VID_Msg));
            sprintf( VID_Msg, "   Private : pChroma=%d TemporalRef=%d\n",
                    RcvEventStack[EvtIndex].EvtData.PictParam.pChromaOffset,
					RcvEventStack[EvtIndex].EvtData.PictParam.pTemporalReference );
            STTBX_Print((VID_Msg));
            break;
        case STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT:
        case STVID_NEW_PICTURE_DECODED_EVT:
        case STVID_NEW_PICTURE_ORDERED_EVT:
            sprintf( VID_Msg, "%s BitmapParams.ColorType=%d \n",
                    VID_Msg,
                    RcvEventStack[EvtIndex].EvtData.PictInfo.BitmapParams.ColorType );
            sprintf( VID_Msg, "%s   BitmapType=%d PreMultipliedColor=%d ColorSpaceConversion=%d \n",
                    VID_Msg,
                    RcvEventStack[EvtIndex].EvtData.PictInfo.BitmapParams.BitmapType,
                    RcvEventStack[EvtIndex].EvtData.PictInfo.BitmapParams.PreMultipliedColor,
                    RcvEventStack[EvtIndex].EvtData.PictInfo.BitmapParams.ColorSpaceConversion );
            sprintf( VID_Msg, "%s   AspectRatio=%d Height=%d Width=%d Pitch=%d Offset=%d\n",
                    VID_Msg,
                    RcvEventStack[EvtIndex].EvtData.PictInfo.BitmapParams.AspectRatio,
                    RcvEventStack[EvtIndex].EvtData.PictInfo.BitmapParams.Height,
                    RcvEventStack[EvtIndex].EvtData.PictInfo.BitmapParams.Width,
                    RcvEventStack[EvtIndex].EvtData.PictInfo.BitmapParams.Pitch,
                    RcvEventStack[EvtIndex].EvtData.PictInfo.BitmapParams.Offset );
            STTBX_Print((VID_Msg));
            sprintf( VID_Msg, "   Data1_p=0x%x Size1=%d Data2_p=0x%x Size2=%d SubByteFormat=%d\n",
                    (int)RcvEventStack[EvtIndex].EvtData.PictInfo.BitmapParams.Data1_p,
                    RcvEventStack[EvtIndex].EvtData.PictInfo.BitmapParams.Size1,
                    (int)RcvEventStack[EvtIndex].EvtData.PictInfo.BitmapParams.Data2_p,
                    RcvEventStack[EvtIndex].EvtData.PictInfo.BitmapParams.Size2,
                    RcvEventStack[EvtIndex].EvtData.PictInfo.BitmapParams.SubByteFormat );
            STTBX_Print((VID_Msg));
            sprintf( VID_Msg, "   VideoParams.FrameRate=%d ScanType=%d MPEGFrame=%d TopFldFirst=%d \n",
                    RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.FrameRate,
                    RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.ScanType,
                    (S32)RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.MPEGFrame,
                    (S32)RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.TopFieldFirst );
            sprintf( VID_Msg, "%s   TimeCode.Hours=%d TimeCode.Minutes=%d TimeCode.Seconds=%d \n",
                    VID_Msg,
                    RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.TimeCode.Hours,
                    RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.TimeCode.Minutes,
                    (S32)RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.TimeCode.Seconds );
            sprintf( VID_Msg, "%s   TimeCode.Frames=%d TimeCode.Interpolated=%d \n",
                    VID_Msg,
                    RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.TimeCode.Frames,
                    RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.TimeCode.Interpolated );
            sprintf( VID_Msg, "%s   PTS.PTS=%d PTS.PTS33=%d PTS.Interpolated=%d \n",
                    VID_Msg,
                    RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.PTS.PTS,
                    RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.PTS.PTS33,
                    RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.PTS.Interpolated );
            if(RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.DecimationFactors != STVID_DECIMATION_FACTOR_NONE)
            {
                sprintf( VID_Msg, "%s   CompressionLevel=%d DecimationFactors=%d DoesNonDecimatedExist=%x \n",
                     VID_Msg,
                    (S32)RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.CompressionLevel,
                    (S32)RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.DecimationFactors,
                    (S32)RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.DoesNonDecimatedExist );
            }
            else
            {
                sprintf( VID_Msg, "%s   CompressionLevel=%d DecimationFactors=%d \n",
                    VID_Msg,
                    (S32)RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.CompressionLevel,
                    (S32)RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.DecimationFactors );
            }
            STTBX_Print((VID_Msg));
            /*  Print DecimatedBitmapParams data. */
            if((RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.DecimationFactors != STVID_DECIMATION_FACTOR_NONE)
               && (RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.DoesNonDecimatedExist))
            {
                sprintf( VID_Msg, "   DecimatedBitmapParams: Data1_p=0x%x Size1=%d Data2_p=0x%x Size2=%d \n",
                         (int)RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.DecimatedBitmapParams.Data1_p,
                         RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.DecimatedBitmapParams.Size1,
                         (int)RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.DecimatedBitmapParams.Data2_p,
                         RcvEventStack[EvtIndex].EvtData.PictInfo.VideoParams.DecimatedBitmapParams.Size2 );
                STTBX_Print((VID_Msg));
            }
            break;
        case STVID_SEQUENCE_INFO_EVT:
            sprintf( VID_Msg, "%s Height=%d Width=%d Aspect=%d \n   ScanType=%d FrameRate=%d BitRate=%d MPEGStandard=%d\n",
                    VID_Msg,
                    RcvEventStack[EvtIndex].EvtData.SeqInfo.Height,
                    RcvEventStack[EvtIndex].EvtData.SeqInfo.Width,
                    RcvEventStack[EvtIndex].EvtData.SeqInfo.Aspect,
                    RcvEventStack[EvtIndex].EvtData.SeqInfo.ScanType,
                    (int)RcvEventStack[EvtIndex].EvtData.SeqInfo.FrameRate,
                    RcvEventStack[EvtIndex].EvtData.SeqInfo.BitRate,
                    RcvEventStack[EvtIndex].EvtData.SeqInfo.MPEGStandard );
            STTBX_Print((VID_Msg));
            sprintf( VID_Msg, "   IsLowDelay=%d VBVBufferSize=%d StreamID=%d \n",
                    RcvEventStack[EvtIndex].EvtData.SeqInfo.IsLowDelay,
                    RcvEventStack[EvtIndex].EvtData.SeqInfo.VBVBufferSize,
                    RcvEventStack[EvtIndex].EvtData.SeqInfo.StreamID );
            sprintf( VID_Msg, "%s   ProfileAndLevelIndication=0x%x FrameRateExtensionN=0x%x \n   FrameRateExtensionD=0x%x VideoFormat=0x%x\n",
                    VID_Msg,
                    RcvEventStack[EvtIndex].EvtData.SeqInfo.ProfileAndLevelIndication,
                    RcvEventStack[EvtIndex].EvtData.SeqInfo.FrameRateExtensionN,
                    RcvEventStack[EvtIndex].EvtData.SeqInfo.FrameRateExtensionD,
                    RcvEventStack[EvtIndex].EvtData.SeqInfo.VideoFormat );
            STTBX_Print((VID_Msg));
            break;
        case STVID_USER_DATA_EVT:
            sprintf( VID_Msg, "%s BroadcastProfile=%d PositionInStream=%d ",
                    VID_Msg,
                    RcvEventStack[EvtIndex].EvtData.UserData.BroadcastProfile,
                    RcvEventStack[EvtIndex].EvtData.UserData.PositionInStream);
            if ( RcvEventStack[EvtIndex].EvtData.UserData.PositionInStream==STVID_USER_DATA_AFTER_SEQUENCE)
            {
                strcat( VID_Msg, "(sequence)\n");
            }
            else if ( RcvEventStack[EvtIndex].EvtData.UserData.PositionInStream==STVID_USER_DATA_AFTER_GOP)
            {
                strcat( VID_Msg, "(GOP)\n");
            }
            else if ( RcvEventStack[EvtIndex].EvtData.UserData.PositionInStream==STVID_USER_DATA_AFTER_PICTURE)
            {
                strcat( VID_Msg, "(picture)\n");
            }
            else
            {
                strcat( VID_Msg, "(undefined position!)\n");
            }
            sprintf( VID_Msg, "%s                Length=%d BufferOverflow=%d Buff_p=0x%x\n",
                    VID_Msg,
                    RcvEventStack[EvtIndex].EvtData.UserData.Length,
                    RcvEventStack[EvtIndex].EvtData.UserData.BufferOverflow,
                    (U32)RcvEventStack[EvtIndex].EvtData.UserData.Buff_p );
            sprintf( VID_Msg, "%s                PTS.PTS=%d PTS.PTS33=%d PTS.Interpol=%d TemporalRef=%d \n                Data= ",
                    VID_Msg,
					RcvEventStack[EvtIndex].EvtData.UserData.PTS.PTS,
                    RcvEventStack[EvtIndex].EvtData.UserData.PTS.PTS33,
                    RcvEventStack[EvtIndex].EvtData.UserData.PTS.Interpolated,
					RcvEventStack[EvtIndex].EvtData.UserData.pTemporalReference);
            STTBX_Print((VID_Msg));
            Pt = RcvUserDataStack[EvtIndex];
            Count = 0;
            while ( Count < RcvEventStack[EvtIndex].EvtData.UserData.Length )
            {
                STTBX_Print(("0x%02X ", (U8)Pt[Count]));
                Count++;
            }
            STTBX_Print(("\n"));
            break;
        case STVID_SYNCHRO_EVT:
            sprintf( VID_Msg, "%s Action=%d EffectiveTime=%d \n",
                    VID_Msg,
                    RcvEventStack[EvtIndex].EvtData.SyncInfo.Action,
                    RcvEventStack[EvtIndex].EvtData.SyncInfo.EffectiveTime );
            STTBX_Print((VID_Msg));
            break;
        case STVID_SPEED_DRIFT_THRESHOLD_EVT:
            sprintf( VID_Msg, "%s DriftTime=%d BitRateValue=%d SpeedRatio=%d\n",
                    VID_Msg,
                    RcvEventStack[EvtIndex].EvtData.ThresholdInfo.DriftTime,
                    RcvEventStack[EvtIndex].EvtData.ThresholdInfo.BitRateValue,
                    RcvEventStack[EvtIndex].EvtData.ThresholdInfo.SpeedRatio );
            STTBX_Print((VID_Msg));
            break;
        case STVID_SYNCHRONISATION_CHECK_EVT:
            sprintf( VID_Msg, "%s ClocksDiff=%d IsSyncOk=%d IsLoosingSync=%d IsBackToSync=%d\n",
                    VID_Msg,
                    RcvEventStack[EvtIndex].EvtData.SynchroCheckInfo.ClocksDifference,
                    RcvEventStack[EvtIndex].EvtData.SynchroCheckInfo.IsSynchronisationOk,
                    RcvEventStack[EvtIndex].EvtData.SynchroCheckInfo.IsLoosingSynchronisation,
                    RcvEventStack[EvtIndex].EvtData.SynchroCheckInfo.IsBackToSynchronisation );
            STTBX_Print((VID_Msg));
            break;
        default:
            STTBX_Print((VID_Msg));
            break;
    }

    return(RetErr);

} /* end of VID_TraceEvtData() */

/*-------------------------------------------------------------------------
 * Function : VID_EvtFound
 *            wait and found for the expected event in the event stack
 *
 * Input    : ExpectedEvt
 * Output   : EvtArrivalTime_p
 * Return   : TRUE if not found, FALSE if found
 * ----------------------------------------------------------------------*/
static BOOL VID_EvtFound(long ExpectedEvt, osclock_t *EvtArrivalTime_p)
{
    BOOL  NotFound;
    BOOL  TimeOut;
    S16 EvtCnt;
    S16 WaitCnt;

    NotFound = TRUE;
    TimeOut = FALSE;

    WaitCnt = 0;
    while ( (!(TimeOut)) && (NotFound) )
    {
        /* search in the stack (if it is not empty) */
        EvtCnt = LastRcvEvtIndex;
        while ( (EvtCnt < NewRcvEvtIndex) && (NotFound))
        {
            if ( RcvEventStack[EvtCnt].EvtNum == ExpectedEvt )
            {
                NotFound = FALSE ;
                *EvtArrivalTime_p = RcvEventStack[EvtCnt].EvtTime ;
            }
            EvtCnt++;
        }

        WaitCnt++;
        if ( NotFound )
        {
            STOS_TaskDelayUs(DELAY_1S/2); /* wait 0.5 sec. */
            if ( WaitCnt >= 10 )
            {
                TimeOut = TRUE;
            }
        }
    }
    return ( NotFound );

} /* end of VID_EvtFound() */


/*#######################################################################*/
/*##########################  SPECIFIC TESTS    #########################*/
/*#######################################################################*/
/*-------------------------------------------------------------------------
 * Function : VID_TestCapability
 *            Test the results provided by STVID_GetCapabality()
 *            accoridng to expectation
 *            (C code program because macro are not convenient for this test)
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_TestCapability(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STVID_Capability_t Capability;
    char DevName[ST_MAX_DEVICE_NAME];
    ST_DeviceName_t DeviceName;


	/* get DeviceName */
    RetErr = STTST_GetString( pars_p, STVID_DEVICE_NAME1, DevName, sizeof(DevName) );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected DeviceName" );
        return(FALSE);
    }
    strcpy( DeviceName, DevName) ;

    RetErr = FALSE;
    ErrCode = STVID_GetCapability(DeviceName, &Capability);
    if ( ErrCode == ST_NO_ERROR )
    {
        /* all chips */
        if (   (Capability.SupportedBroadcastProfile != (STVID_BROADCAST_DVB | STVID_BROADCAST_DIRECTV | STVID_BROADCAST_ATSC))
            || (Capability.SupportedCodingMode != STVID_CODING_MODE_MB)
            || (Capability.SupportedColorType  != STVID_COLOR_TYPE_YUV420)
            || (Capability.SupportedDisplayARConversion != (STVID_DISPLAY_AR_CONVERSION_PAN_SCAN |
                    STVID_DISPLAY_AR_CONVERSION_LETTER_BOX |
                    STVID_DISPLAY_AR_CONVERSION_COMBINED |
                    STVID_DISPLAY_AR_CONVERSION_IGNORE))
            || (Capability.SupportedDisplayAspectRatio != (STVID_DISPLAY_ASPECT_RATIO_16TO9 |
                    STVID_DISPLAY_ASPECT_RATIO_4TO3))
            || (Capability.SupportedFreezeMode != (STVID_FREEZE_MODE_NONE |
                    STVID_FREEZE_MODE_FORCE | STVID_FREEZE_MODE_NO_FLICKER))
            || (Capability.SupportedFreezeField != (STVID_FREEZE_FIELD_TOP |
                    STVID_FREEZE_FIELD_BOTTOM |
                    STVID_FREEZE_FIELD_CURRENT | STVID_FREEZE_FIELD_NEXT ))
            || (Capability.SupportedPicture != (STVID_PICTURE_LAST_DECODED | STVID_PICTURE_DISPLAYED))
            || (Capability.SupportedScreenScanType != (STVID_SCAN_TYPE_INTERLACED |
                    STVID_SCAN_TYPE_PROGRESSIVE))
            || (Capability.SupportedStop != (STVID_STOP_NOW |
                    STVID_STOP_WHEN_NEXT_REFERENCE | STVID_STOP_WHEN_NEXT_I | STVID_STOP_WHEN_END_OF_DATA ))
            || (Capability.SupportedStreamType != (STVID_STREAM_TYPE_ES |
                    STVID_STREAM_TYPE_PES | STVID_STREAM_TYPE_MPEG1_PACKET))
            || (Capability.SupportedWinAlign != (STVID_WIN_ALIGN_TOP_LEFT |
                                            STVID_WIN_ALIGN_VCENTRE_LEFT |
                                            STVID_WIN_ALIGN_BOTTOM_LEFT |
                                            STVID_WIN_ALIGN_TOP_RIGHT |
                                            STVID_WIN_ALIGN_VCENTRE_RIGHT |
                                            STVID_WIN_ALIGN_BOTTOM_RIGHT |
                                            STVID_WIN_ALIGN_BOTTOM_HCENTRE |
                                            STVID_WIN_ALIGN_TOP_HCENTRE |
                                            STVID_WIN_ALIGN_VCENTRE_HCENTRE))
            || (Capability.SupportedWinSize != STVID_WIN_SIZE_DONT_CARE) )
        {
            RetErr = TRUE;
            STTBX_Print(("STVID_GetCapability() : unexpected Capability.SupportedXxxx data !\n" ));
        }
        if (   (Capability.StillPictureCapable != TRUE)
            || (Capability.ManualInputWindowCapable != TRUE)
            || (Capability.ManualOutputWindowCapable != TRUE) )
        {
            RetErr = TRUE;
            STTBX_Print(("STVID_GetCapability() : unexpected Capability.XxxxCapable data !\n" ));
        }
        if (   (Capability.InputWindowHeightMin != 0)
            || (Capability.InputWindowWidthMin != 0)
            || (Capability.InputWindowPositionXPrecision !=1 )
#if defined (ST_5512)
            || (Capability.InputWindowPositionYPrecision !=2 )
#elif defined (ST_5510)
            || (Capability.InputWindowPositionYPrecision !=16 )
#else
            || (Capability.InputWindowPositionYPrecision !=1 )
#endif
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5107) || defined (ST_5301)
            || (Capability.InputWindowHeightPrecision != 1)
#else
            || (Capability.InputWindowHeightPrecision != 2)
#endif
            || (Capability.InputWindowWidthPrecision != 1) )
        {
            RetErr = TRUE;
            STTBX_Print(("STVID_GetCapability() : unexpected Capability.InputXxxx data !\n" ));
        }
        if (   (Capability.OutputWindowHeightMin != 0)
            || (Capability.OutputWindowWidthMin != 0)
            || (Capability.OutputWindowPositionYPrecision != 1)
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5107) || defined (ST_5301)
            || (Capability.OutputWindowPositionXPrecision != 1)
            || (Capability.OutputWindowWidthPrecision != 1)
            || (Capability.OutputWindowHeightPrecision != 1) )
#else
            || (Capability.OutputWindowPositionXPrecision != 2)
            || (Capability.OutputWindowWidthPrecision != 2)
            || (Capability.OutputWindowHeightPrecision != 2) )
#endif
        {
            RetErr = TRUE;
            STTBX_Print(("STVID_GetCapability() : unexpected Capability.OutputXxxx data !\n" ));
        }
        if (  (Capability.SupportedHorizontalScaling.Continuous != TRUE)
           || (Capability.SupportedHorizontalScaling.NbScalingFactors != 2)
           || (Capability.SupportedHorizontalScaling.ScalingFactors_p[0].N != 1)
#if defined (ST_7020)
           || (Capability.SupportedHorizontalScaling.ScalingFactors_p[0].M != 3)
           || (Capability.SupportedHorizontalScaling.ScalingFactors_p[1].N != 6)
#elif defined (ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5301)|| defined(ST_5525)
           || (Capability.SupportedHorizontalScaling.ScalingFactors_p[0].M != 16)
           || (Capability.SupportedHorizontalScaling.ScalingFactors_p[1].N != 8)
#else
           || (Capability.SupportedHorizontalScaling.ScalingFactors_p[0].M != 4)
           || (Capability.SupportedHorizontalScaling.ScalingFactors_p[1].N != 8)
#endif
           || (Capability.SupportedHorizontalScaling.ScalingFactors_p[1].M != 1) )
        {
            RetErr = TRUE;
            STTBX_Print(("STVID_GetCapability() : unexpected Capability.SupportedHorizontalScaling.Xxxx data !\n" ));
        }
#if defined (ST_5510) | defined (ST_5512)
        if (    (Capability.SupportedVerticalScaling.Continuous != FALSE)
             || (Capability.SupportedVerticalScaling.NbScalingFactors != 7) )
        {
            RetErr = TRUE;
            STTBX_Print(("STVID_GetCapability() : unexpected Capability.SupportedVerticalScaling.Xxxx data !\n" ));
        }
        if (  (Capability.SupportedVerticalScaling.Continuous != FALSE)
           || (Capability.SupportedVerticalScaling.NbScalingFactors != 7)
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[0].N != 1)
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[0].M != 4)
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[1].N != 3)
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[1].M != 8)
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[2].N != 1)
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[2].M != 2)
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[3].N != 3)
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[3].M != 4)
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[4].N != 7)
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[4].M != 8)
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[5].N != 1)
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[5].M != 1)
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[6].N != 2)
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[6].M != 1) )
        {
            RetErr = TRUE;
            STTBX_Print(("STVID_GetCapability() : unexpected Capability.SupportedHorizontalScaling.Xxxx data !\n" ));
        }
#else
        if (  (Capability.SupportedVerticalScaling.Continuous != TRUE)
           || (Capability.SupportedVerticalScaling.NbScalingFactors != 2) )
        {
            RetErr = TRUE;
            STTBX_Print(("STVID_GetCapability() : unexpected Capability.SupportedVerticalScaling.Xxxx data !\n" ));
        }
        if (  (Capability.SupportedVerticalScaling.Continuous != TRUE)
           || (Capability.SupportedVerticalScaling.NbScalingFactors != 2)
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[0].N != 1)
    #if defined (ST_7020)
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[0].M != 4)
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[1].N != 6)
    #elif defined (ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5301) || defined(ST_5525)
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[0].M != 16)
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[1].N != 8)
    #else
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[0].M != 4)
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[1].N != 8)
    #endif
           || (Capability.SupportedVerticalScaling.ScalingFactors_p[1].M != 1) )
        {
            RetErr = TRUE;
            STTBX_Print(("STVID_GetCapability() : unexpected Capability.SupportedVerticalScaling.Xxxx data !\n" ));
        }
#endif
    }
    else
    {
        STTBX_Print(("STVID_GetCapability(%s,%ld) : data can't be tested due to error %d !\n",
             DeviceName, (long)&Capability, ErrCode ));
        RetErr = TRUE;
    }

    /* test result */
    if ( !(RetErr) )
    {
        STTBX_Print(( "### VID_TestCapability() result : ok \n    software capabilities of the chip are like expectations\n"));
    }
    else
    {
        STTBX_Print(( "### VID_TestCapability() result : failed !\n"));
    }
    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return (RetErr);

} /* end of VID_TestCapability() */

/*******************************************************************************
Name        : VID_PictureInfosToPictureParams
Description : Translate picture infos to old structure picture params
Parameters  : pointers to structures to translate from/to
Assumptions : Both pointer are not NULL
Limitations : private field pTemporalReference is not set !
Returns     : Nothing
*******************************************************************************/
void VID_PictureInfosToPictureParams(const STVID_PictureInfos_t * const PictureInfos_p, STVID_PictureParams_t * const Params_p)
{
    switch (PictureInfos_p->BitmapParams.BitmapType)
    {
        case STGXOBJ_BITMAP_TYPE_MB :
        default :
            Params_p->CodingMode = STVID_CODING_MODE_MB;
            break;
    }
    switch (PictureInfos_p->BitmapParams.ColorType)
    {
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422 :
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422 :
            Params_p->ColorType = STVID_COLOR_TYPE_YUV422;
            break;

        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444 :
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444 :
        case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444 :
            Params_p->ColorType = STVID_COLOR_TYPE_YUV444;
            break;

        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_420 :
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420 :
        default :
            Params_p->ColorType = STVID_COLOR_TYPE_YUV420;
            break;
    }
    Params_p->FrameRate     = PictureInfos_p->VideoParams.FrameRate;
    Params_p->Data          = PictureInfos_p->BitmapParams.Data1_p;
    Params_p->Size          = PictureInfos_p->BitmapParams.Size1 + PictureInfos_p->BitmapParams.Size2;
    Params_p->Height        = PictureInfos_p->BitmapParams.Height;
    Params_p->Width         = PictureInfos_p->BitmapParams.Width;
    switch (PictureInfos_p->BitmapParams.AspectRatio)
    {
        case STGXOBJ_ASPECT_RATIO_16TO9 :
            Params_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_16TO9;
            break;

        case STGXOBJ_ASPECT_RATIO_221TO1 :
            Params_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_221TO1;
            break;

        case STGXOBJ_ASPECT_RATIO_SQUARE :
            Params_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_SQUARE;
            break;

        case STGXOBJ_ASPECT_RATIO_4TO3 :
        default :
            Params_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
            break;
    }
    switch (PictureInfos_p->VideoParams.ScanType)
    {
        case STGXOBJ_PROGRESSIVE_SCAN :
            Params_p->ScanType = STVID_SCAN_TYPE_PROGRESSIVE;
            break;

        case STGXOBJ_INTERLACED_SCAN :
        default :
            Params_p->ScanType = STVID_SCAN_TYPE_INTERLACED;
            break;
    }
    Params_p->MPEGFrame     = PictureInfos_p->VideoParams.MPEGFrame;
    Params_p->TopFieldFirst = PictureInfos_p->VideoParams.TopFieldFirst;
    Params_p->TimeCode      = PictureInfos_p->VideoParams.TimeCode;
    Params_p->PTS           = PictureInfos_p->VideoParams.PTS;

    /* Private data */
    Params_p->pChromaOffset = (U32) PictureInfos_p->BitmapParams.Data2_p - (U32) PictureInfos_p->BitmapParams.Data1_p;
/*    Params_p->pTemporalReference = (U16) ???;*/

} /* End of PictureInfosToPictureParams() function */

/*-------------------------------------------------------------------------
 * Function : VID_PictParamCallback
 *            picture params comparison for test VID_TestPictureParams()
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void VID_PictParamCallback(STEVT_CallReason_t CC_Reason,
                            const ST_DeviceName_t RegistrantName,
                            STEVT_EventConstant_t Event,
                            void* EventData,
                            void *SubscriberData_p)
{
    STVID_PictureParams_t   ParamsGet;
    STVID_PictureParams_t   ParamsStop;
    STVID_PictureInfos_t    InfosGet;
    ST_ErrorCode_t          RetErr;

    UNUSED_PARAMETER(CC_Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(SubscriberData_p);

    if ( EventData == NULL )
    {
        return; /* unexpected null pointer provided by driver */
    }

    /* Process event data before using them ... */
    if (STVID_PreprocessEvent(RegistrantName, Event, (void *)EventData) != ST_NO_ERROR)
    {
#if defined(ST_OS20) || defined(ST_OSWINCE)
        STTBX_Print(("%s(%d): Preprocess event pb ! 0x%.8x \n", __FILE__, __LINE__, Event));
#else
        STTBX_Print(("%s(): Preprocess event pb ! 0x%.8x \n", __FUNCTION__, Event));
#endif
    }

    RetErr = ST_NO_ERROR;
    ParamTestResult = 0x00000000;
    memcpy(&ParamsStop, EventData, sizeof(STVID_PictureParams_t));
    /*ParamsStop = *(STVID_PictureParams_t *)EventData;*/

    /* Comparing PictureParams returned by STOPPED_EVENT assoc. data and by STVID_GetPictureParams(PICTURE_DISPLAYED) */
    RetErr = STVID_GetPictureParams(DefaultVideoHandle,STVID_PICTURE_DISPLAYED,
                            &ParamsGet);
    if((ParamsStop.TopFieldFirst    != ParamsGet.TopFieldFirst)
        ||(ParamsStop.FrameRate     != ParamsGet.FrameRate)
        ||(ParamsStop.Data          != ParamsGet.Data)
        ||(ParamsStop.Size          != ParamsGet.Size)
        ||(ParamsStop.Width         != ParamsGet.Width)
        ||(ParamsStop.Height        != ParamsGet.Height)
        ||(ParamsStop.Aspect        != ParamsGet.Aspect)
        ||(ParamsStop.ScanType      != ParamsGet.ScanType)
        ||(ParamsStop.MPEGFrame     != ParamsGet.MPEGFrame)
        ||(ParamsStop.CodingMode    != ParamsGet.CodingMode)
        ||(ParamsStop.ColorType     != ParamsGet.ColorType))
    {
        ParamTestResult |= 0x00000003;
    }
    switch(RetErr)
    {
        case STVID_ERROR_DECODER_RUNNING:
            ParamTestResult |= 0x00000030;
            break;
        case STVID_ERROR_NOT_AVAILABLE:
            ParamTestResult |= 0x00000300;
            break;
        default:
            break;
    }
    /* Comparing PictureParams returned by STOPPED_EVENT assoc. data and by STVID_GetPictureParams(LAST_DECODED) */
    /* WARNING : testing stream must follow the condition last decoded is last displayed */
    RetErr = STVID_GetPictureParams(DefaultVideoHandle,STVID_PICTURE_LAST_DECODED,
                            &ParamsGet);
    if((ParamsStop.TopFieldFirst    != ParamsGet.TopFieldFirst)
        ||(ParamsStop.FrameRate     != ParamsGet.FrameRate)
        ||(ParamsStop.Data          != ParamsGet.Data)
        ||(ParamsStop.Size          != ParamsGet.Size)
        ||(ParamsStop.Width         != ParamsGet.Width)
        ||(ParamsStop.Height        != ParamsGet.Height)
        ||(ParamsStop.Aspect        != ParamsGet.Aspect)
        ||(ParamsStop.ScanType      != ParamsGet.ScanType)
        ||(ParamsStop.MPEGFrame     != ParamsGet.MPEGFrame)
        ||(ParamsStop.CodingMode    != ParamsGet.CodingMode)
        ||(ParamsStop.ColorType     != ParamsGet.ColorType))
    {
        ParamTestResult |= 0x00030000;
    }

    switch(RetErr)
    {
        case STVID_ERROR_DECODER_RUNNING:
            ParamTestResult |= 0x00300000;
            break;
        case STVID_ERROR_NOT_AVAILABLE:
            ParamTestResult |= 0x03000000;
            break;
        default:
            break;
    }

    /* Comparing PictureParams returned by STOPPED_EVENT assoc. data and by STVID_GetPictureInfo(LAST_DECODED) */
    RetErr = STVID_GetPictureInfos(DefaultVideoHandle,STVID_PICTURE_DISPLAYED,
                            &InfosGet);
    if (RetErr == ST_NO_ERROR)
    {
        VID_PictureInfosToPictureParams(&InfosGet, &ParamsGet);

        if((ParamsStop.TopFieldFirst    != ParamsGet.TopFieldFirst)
            ||(ParamsStop.FrameRate     != ParamsGet.FrameRate)
            ||(ParamsStop.Data          != ParamsGet.Data)
            ||(ParamsStop.Size          != ParamsGet.Size)
            ||(ParamsStop.Width         != ParamsGet.Width)
            ||(ParamsStop.Height        != ParamsGet.Height)
            ||(ParamsStop.Aspect        != ParamsGet.Aspect)
            ||(ParamsStop.ScanType      != ParamsGet.ScanType)
            ||(ParamsStop.MPEGFrame     != ParamsGet.MPEGFrame)
            ||(ParamsStop.CodingMode    != ParamsGet.CodingMode)
            ||(ParamsStop.ColorType     != ParamsGet.ColorType))
        {
            ParamTestResult |= 0x00000004;
        }
    }
    else
    {
        ParamTestResult |= 0x00000008;
    }
    switch(RetErr)
    {
        case STVID_ERROR_DECODER_RUNNING:
            ParamTestResult |= 0x000000c0;
            break;
        case STVID_ERROR_NOT_AVAILABLE:
            ParamTestResult |= 0x00000c00;
            break;
        default:
            break;
    }
    /* Comparing PictureParams returned by event to PictureParams returned by the API STVID_GetPictureInfos */
    /* Comparing to the last decoded picture */
    /* WARNING : testing stream must follow the condition last decoded is last displayed */

    RetErr = STVID_GetPictureInfos(DefaultVideoHandle,STVID_PICTURE_LAST_DECODED,
                            &InfosGet);

    if (RetErr == ST_NO_ERROR)
    {
        VID_PictureInfosToPictureParams(&InfosGet, &ParamsGet);

        if((ParamsStop.TopFieldFirst    != ParamsGet.TopFieldFirst)
            ||(ParamsStop.FrameRate     != ParamsGet.FrameRate)
            ||(ParamsStop.Data          != ParamsGet.Data)
            ||(ParamsStop.Size          != ParamsGet.Size)
            ||(ParamsStop.Width         != ParamsGet.Width)
            ||(ParamsStop.Height        != ParamsGet.Height)
            ||(ParamsStop.Aspect        != ParamsGet.Aspect)
            ||(ParamsStop.ScanType      != ParamsGet.ScanType)
            ||(ParamsStop.MPEGFrame     != ParamsGet.MPEGFrame)
            ||(ParamsStop.CodingMode    != ParamsGet.CodingMode)
            ||(ParamsStop.ColorType     != ParamsGet.ColorType))
        {
            ParamTestResult |= 0x00040000;
        }
    }
    else
    {
        ParamTestResult |= 0x00080000;
    }
    switch(RetErr)
    {
        case STVID_ERROR_DECODER_RUNNING:
            ParamTestResult |= 0x00c00000;
            break;
        case STVID_ERROR_NOT_AVAILABLE:
            ParamTestResult |= 0x0c000000;
            break;
        default:
            break;
    }

    semaphore_signal(PictParamTestFlag_p);
}
/*-------------------------------------------------------------------------
 * Function : VID_TestPictureParams
 *            Compare pict. params between stopped_event and decolast display
 *            Compare pict. params between stopped_event and last decoded
 *            Compare pict. params between stopped_event and displ. pict. infos
 *            Compare pict. params between stopped_event and decoded pict. infos
 * Input    :
 * Output   :
 * Return   : False if no error, else True
-------------------------------------------------------------------------*/
static BOOL VID_TestPictureParams(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL                            RetErr = FALSE;
    STEVT_DeviceSubscribeParams_t   SubscribeParams;
    STVID_Freeze_t                  Freeze;
    osclock_t                       Time;
    STVID_PictureParams_t           LastDecodedPictParams;
    ST_DeviceName_t                 VIDName;
    STVID_Handle_t                  CurrentHandle;
    ST_ErrorCode_t                  RetCode;

    RetErr = STTST_GetString( pars_p, STVID_DEVICE_NAME1, VIDName, sizeof(ST_DeviceName_t));
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected video device name" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, DefaultVideoHandle, (S32*)&CurrentHandle);
    if ( RetErr )
    {
        STTST_TagCurrentLine(pars_p, "expected video handle");
        return(TRUE);
    }
    DefaultVideoHandle = CurrentHandle;

    ParamTestResult = 0xf0000000;
    SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)VID_PictParamCallback;
    SubscribeParams.SubscriberData_p = NULL;
    RetErr = STEVT_SubscribeDeviceEvent(EvtSubHandle,
                                    VIDName,
                                    STVID_STOPPED_EVT,
                                    &SubscribeParams);

    Freeze.Mode     = STVID_FREEZE_MODE_NONE;
    Freeze.Field    = STVID_FREEZE_FIELD_CURRENT;
    STTBX_Print(( "Call STVID_Stop(WHEN_END_OF_DATA) and wait...\n"));
    RetCode = STVID_Stop(DefaultVideoHandle,STVID_STOP_WHEN_END_OF_DATA,&Freeze);
    if(RetCode!=ST_NO_ERROR)
    {
        semaphore_signal(PictParamTestFlag_p);
        STTBX_Print(( "STVID_Stop(WHEN_END_OF_DATA) : failure=0x%x !\n",RetCode));
    }
    Time = STOS_time_plus(STOS_time_now(), TicksPerOneSecond*6);
    semaphore_wait_timeout(PictParamTestFlag_p,&Time);
    if(ParamTestResult != 0xf0000000)
    {
        STTBX_Print(( "STVID_Stop(WHEN_END_OF_DATA) : done\n"));
    }
    else
    {
        STTBX_Print(( "STVID_Stop(WHEN_END_OF_DATA) : timeout !\n"));
        RetErr = STVID_GetPictureParams(DefaultVideoHandle,STVID_PICTURE_LAST_DECODED,
                            &LastDecodedPictParams);
        switch(RetErr)
        {
            case STVID_ERROR_DECODER_RUNNING:
                ParamTestResult |= 0x00300000;
                break;
            case STVID_ERROR_NOT_AVAILABLE:
                ParamTestResult |= 0x03000000;
                break;
            default:
                break;
        }
        if (RetErr == 0)
        {
            VID_PictParamCallback(0,0,0,( void*)&LastDecodedPictParams ,0);
        }
    }
    RetErr = STEVT_UnsubscribeDeviceEvent(EvtSubHandle,
                                    VIDName,
                                    STVID_STOPPED_EVT);
    if ( ParamTestResult != 0 )
    {
        STTBX_Print((" Error Codes for STVID_PictureParams_t structure :\n" ));
        STTBX_Print((" 0x00000003 : different data between STOPPED_EVENT and STVID_PICTURE_DISPLAYED\n"));
        STTBX_Print((" 0x00000030 : STVID_ERROR_DECODER_RUNNING returned by STVID_GetPictureParams(PICTURE_DISPLAYED)\n"));
        STTBX_Print((" 0x00000300 : STVID_ERROR_NOT_AVAILABLE returned by STVID_GetPictureParams(PICTURE_DISPLAYED)\n"));
        STTBX_Print((" 0x00030000 : different data between STOPPED_EVENT and STVID_PICTURE_LAST_DECODED.\n"));
        STTBX_Print((" 0x00300000 : STVID_ERROR_DECODER_RUNNING returned by STVID_GetPictureParams(LAST_DECODED)\n"));
        STTBX_Print((" 0x03000000 : STVID_ERROR_NOT_AVAILABLE returned by STVID_GetPictureParams(LAST_DECODED)\n"));
        STTBX_Print((" 0x30000000 : Test not completed.\n" ));
        STTBX_Print(("### PictureParams Test result : error=0x%x\n\n",
                    ParamTestResult & 0x33333333));
        STTBX_Print((" Error Codes for STVID_PictureInfos_t structure :\n" ));
        STTBX_Print((" 0x00000004 : different data between STOPPED_EVENT and STVID_PICTURE_DISPLAYED\n"));
        STTBX_Print((" 0x00000008 : STVID_GetPictureInfos(PICTURE_DISPLAYED) error\n"));
        STTBX_Print((" 0x000000c0 : STVID_ERROR_DECODER_RUNNING returned by STVID_GetPictureInfos(PICTURE_DISPLAYED)\n"));
        STTBX_Print((" 0x00000c00 : STVID_ERROR_NOT_AVAILABLE returned by STVID_GetPictureInfos(PICTURE_DISPLAYED)\n"));
        STTBX_Print((" 0x00040000 : different data between STOPPED_EVENT and STVID_PICTURE_LAST_DECODED\n"));
        STTBX_Print((" 0x00080000 : STVID_GetPictureInfos(PICTURE_DECODED) error\n"));
        STTBX_Print((" 0x00c00000 : STVID_ERROR_DECODER_RUNNING returned by STVID_GetPictureInfos(LAST_DECODED)\n"));
        STTBX_Print((" 0x0c000000 : STVID_ERROR_NOT_AVAILABLE returned by STVID_GetPictureInfos(STVID_PICTURE_LAST_DECODED)\n"));
        STTBX_Print((" 0xc0000000 : Test not completed.\n" ));
        STTBX_Print(("### PictureInfos Test result  : error=0x%x\n",
                    ParamTestResult & 0xcccccccc));
        STTBX_Print(("### VID_TestPictureParams() result : failed !\n"));
        RetErr = TRUE;
    }
    else
    {
        STTBX_Print(( "### VID_TestPictureParams() result : data comparison ok \n" ));
        RetErr = FALSE;
    }

    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return(API_EnableError ? RetErr : FALSE );
}/* end of VID_TestPictureParams() */
/*-------------------------------------------------------------------------
 * Function : VID_ReturnStatisticsValue
 *            Get the value of the statistic passed as parameter.
 * Input    : DeviceName, SpeedMaxPositiveDriftRequested
 * Input list can be append by increasing StrMaskList table and VID_MAX_STATISTICS_LIST
 * Output   : Value of the statistic passed as parameter
 * Return   : False if no error, True is 1 or more errors
 * ----------------------------------------------------------------------*/
static BOOL VID_ReturnStatisticsValue(STTST_Parse_t *pars_p, char *result_sym_p)
{
#ifndef STVID_DEBUG_GET_STATISTICS
    return(FALSE);
#else /* STVID_DEBUG_GET_STATISTICS */
#define VID_MAX_STATISTICS_LIST 1

    BOOL                    CriteriaIsFound;
    U32                     i ;
    BOOL RetErr;
    STVID_Statistics_t      Statistics;
    ST_ErrorCode_t ErrCode;
    char Name[2*ST_MAX_DEVICE_NAME]; /* allow big name for error case test */
    ST_DeviceName_t *DeviceName;

    char StrMask[256], * StrFound_p; /* Mask to check only some parameters */
    const char StrMaskList [VID_MAX_STATISTICS_LIST][32] = {
        "SpeedMaxPositiveDriftRequested"
    };


    RetErr = STTST_GetString(pars_p, STVID_DEVICE_NAME1, Name, sizeof(Name) );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected DeviceName" );
        return(TRUE);
    }
    DeviceName = (ST_DeviceName_t *)Name ;

    RetErr = STTST_GetString(pars_p, "SpeedMaxPositiveDriftRequested",
                            StrMask, sizeof(StrMask) );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p,
                              "expected string mask (default is \"SpeedMaxPositiveDriftRequested\")");
    }

    ErrCode = STVID_GetStatistics(*DeviceName, &Statistics);
    switch (ErrCode)
    {
        case ST_NO_ERROR :
            strcat(VID_Msg, "ok\n" );
            RetErr = FALSE;
            STTST_Print((VID_Msg ));
            CriteriaIsFound = FALSE;

            for (i = 0; i < VID_MAX_STATISTICS_LIST; i++)
            {
                StrFound_p = strstr(StrMask, StrMaskList[i]);
                if (StrFound_p != NULL)
                {
                    CriteriaIsFound = TRUE;

                    switch(i)
                    {
                        case 0:
                            RetErr = STTST_AssignInteger("RET_VAL1",  Statistics.MaxPositiveDriftRequested, FALSE);
                            break;
                            /* more cases to be added if we wand the function to get other statistics. */
                        default:
                            STTBX_Print(("   Statistics not implemented !!\n"));
                            break;
                    }
                }
            }
            if (!(CriteriaIsFound))
            {
                STTBX_Print(("   Unknown mask [%s]\n",StrMask));
            }
            break;
        case ST_ERROR_BAD_PARAMETER:
            API_ErrorCount++;
            strcat(VID_Msg, "one parameter is invalid !\n" );
            STTST_Print((VID_Msg ));
            break;
        case ST_ERROR_UNKNOWN_DEVICE:
            API_ErrorCount++;
            strcat(VID_Msg, "the specified device is not initialized !\n" );
            STTST_Print((VID_Msg ));
            break;
        default:
            API_ErrorCount++;
            sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
            STTST_Print((VID_Msg ));
            break;
    } /* end switch */
    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return (API_EnableError ? RetErr : FALSE );
#endif /* STVID_DEBUG_GET_STATISTICS */

} /* end VID_ReturnStatisticsValue */

/*-------------------------------------------------------------------------
 * Function : VID_TestStatistics
 *            Check some decoded/displayed/pb data from stvid_getstatistics()
 *            . pict found == decode launch == start decode == expectation
              . all 'pb' data == 0 or -1, all 'skip' not requestetd == 0
              . speeddisplayI/P/B like expectations
              . decodelaunch >= displayedbymain == total of display expectation
 * Input    : DeviceName, NbOfExpectedDecodedFrames, NbOfExpectedDisplayedIFrames,
 *            NbOfExpectedDisplayedPFrames, NbOfExpectedDisplayedBFrames,
 * Output   :
 * Return   : False if no error, True is 1 or more errors
 * ----------------------------------------------------------------------*/
static BOOL VID_TestStatistics(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrorCode;
    char Name[2*ST_MAX_DEVICE_NAME]; /* allow big name for error case test */
    ST_DeviceName_t *DeviceName;
    U32  ExpectedDecodedFrames;
    U32  ExpectedDisplayedIFrames;
    U32  ExpectedDisplayedPFrames;
    U32  ExpectedDisplayedBFrames;
    STVID_Statistics_t Stat;
    S32  SInputValue;
    U32  Value;

    RetErr = FALSE;

    /* --- Get input parameters --- */

    RetErr = STTST_GetString(pars_p, STVID_DEVICE_NAME1, Name, sizeof(Name) );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected DeviceName" );
        return(TRUE);
    }
    DeviceName = (ST_DeviceName_t *)Name ;

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&SInputValue);
    if ( RetErr )
    {
        STTST_TagCurrentLine(pars_p, "expected nb of expected decode frames");
        return(TRUE);
    }
    ExpectedDecodedFrames = (U32)SInputValue ;
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&SInputValue);
    if ( RetErr )
    {
        STTST_TagCurrentLine(pars_p, "expected nb of expected displayed I frames");
        return(TRUE);
    }
    ExpectedDisplayedIFrames = (U32)SInputValue ;
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&SInputValue);
    if ( RetErr )
    {
        STTST_TagCurrentLine(pars_p, "expected nb of expected displayed P frames");
        return(TRUE);
    }
    ExpectedDisplayedPFrames = (U32)SInputValue ;
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&SInputValue);
    if ( RetErr )
    {
        STTST_TagCurrentLine(pars_p, "expected nb of expected displayed B frames");
        return(TRUE);
    }
    ExpectedDisplayedBFrames = (U32)SInputValue ;

    /* --- Get decode and display statistics --- */

    ErrorCode = STVID_GetStatistics(*DeviceName, &Stat);
    if ( ErrorCode != ST_NO_ERROR )
    {
        RetErr = TRUE;
        STTST_Print(("VID_TestStatistics(): STVID_GetStatistics() failure 0x%x !\n", ErrorCode));
    }
    else
    {
        /* --- Check number of decoded frames --- */

        if ( Stat.DecodePictureFound != ExpectedDecodedFrames )
        {
            STTST_Print(("VID_TestStatistics(): DecodePictureFound=%d but expected=%d !\n",
                     Stat.DecodePictureFound, ExpectedDecodedFrames));
            RetErr = TRUE;
        }
        if ( Stat.DecodePictureDecodeLaunched != ExpectedDecodedFrames )
        {
            STTST_Print(("VID_TestStatistics(): DecodePictureDecodeLaunched=%d but expected=%d !\n",
                     Stat.DecodePictureDecodeLaunched, ExpectedDecodedFrames));
            RetErr = TRUE;
        }
        Value = Stat.DecodePictureFoundMPEGFrameI + Stat.DecodePictureFoundMPEGFrameP + Stat.DecodePictureFoundMPEGFrameB ;
        if ( Value != ExpectedDecodedFrames )
        {
            STTST_Print(("VID_TestStatistics(): DecodePictureFoundMPEGFrameI+B+P=%d but expected=%d !\n",
                     Value, ExpectedDecodedFrames));
            RetErr = TRUE;
        }

        /* --- Check decode problems --- */

        if ( Stat.DecodePictureSkippedRequested > 0 )
        {
            STTST_Print(("VID_TestStatistics(): DecodePictureSkippedRequested=%d but expected=0 !\n",
                     Stat.DecodePictureSkippedRequested));
            RetErr = TRUE;
        }
        if ( Stat.DecodePictureSkippedNotRequested > 0 )
        {
            STTST_Print(("VID_TestStatistics(): DecodePictureSkippedNotRequested=%d but expected=0 !\n",
                     Stat.DecodePictureSkippedNotRequested));
            RetErr = TRUE;
        }
        Value = 0;
        /* no pb. expected ; ignore -1 values */
        if ( Stat.DecodePbStartCodeFoundInvalid != (U32)-1 )
            Value += Stat.DecodePbStartCodeFoundInvalid ;
        if ( Stat.DecodePbStartCodeFoundVideoPES != (U32)-1 )
            Value += Stat.DecodePbStartCodeFoundVideoPES ;
        if ( Stat.DecodePbMaxNbInterruptSyntaxErrorPerPicture != (U32)-1 )
            Value += Stat.DecodePbMaxNbInterruptSyntaxErrorPerPicture ;
        if ( Stat.DecodePbInterruptSyntaxError != (U32)-1 )
            Value += Stat.DecodePbInterruptSyntaxError ;
        if ( Stat.DecodePbInterruptDecodeOverflowError != (U32)-1 )
            Value += Stat.DecodePbInterruptDecodeOverflowError ;
        if ( Stat.DecodePbInterruptDecodeUnderflowError != (U32)-1 )
            Value += Stat.DecodePbInterruptDecodeUnderflowError ;
        if ( Stat.DecodePbDecodeTimeOutError != (U32)-1 )
            Value += Stat.DecodePbDecodeTimeOutError ;
        if ( Stat.DecodePbInterruptMisalignmentError != (U32)-1 )
            Value += Stat.DecodePbInterruptMisalignmentError ;
        if ( Stat.DecodePbInterruptQueueOverflow != (U32)-1 )
            Value += Stat.DecodePbInterruptQueueOverflow ;
        if ( Stat.DecodePbVbvSizeGreaterThanBitBuffer != (U32)-1 )
            Value += Stat.DecodePbVbvSizeGreaterThanBitBuffer ;
        if ( Stat.DecodePbParserError != (U32)-1 )
            Value += Stat.DecodePbParserError ;
        if ( Stat.DecodePbPreprocError != (U32)-1 )
            Value += Stat.DecodePbPreprocError ;
        if ( Stat.DecodePbFirmwareError != (U32)-1 )
            Value += Stat.DecodePbFirmwareError ;
        if ( Value > 0 )
        {
            STTST_Print(("VID_TestStatistics(): at least one DecodePbXxxx data value is greater than 0 (total=%d) !\n", Value));
            RetErr = TRUE;
        }

        /* --- Check number of displayed frames --- */

        if ( Stat.SpeedDisplayIFramesNb != ExpectedDisplayedIFrames )
        {
            STTST_Print(("VID_TestStatistics(): SpeedDisplayIFramesNb=%d but expected=%d !\n",
                    Stat.SpeedDisplayIFramesNb, ExpectedDisplayedIFrames));
            RetErr = TRUE;
        }
        if ( Stat.SpeedDisplayPFramesNb != ExpectedDisplayedPFrames )
        {
            STTST_Print(("VID_TestStatistics(): SpeedDisplayPFramesNb=%d but expected=%d !\n",
                    Stat.SpeedDisplayPFramesNb, ExpectedDisplayedPFrames));
            RetErr = TRUE;
        }
        if ( Stat.SpeedDisplayBFramesNb != ExpectedDisplayedBFrames )
        {
            STTST_Print(("VID_TestStatistics(): SpeedDisplayBFramesNb=%d but expected=%d !\n",
                    Stat.SpeedDisplayBFramesNb, ExpectedDisplayedBFrames));
            RetErr = TRUE;
        }
        Value = Stat.SpeedDisplayIFramesNb + Stat.SpeedDisplayPFramesNb + Stat.SpeedDisplayBFramesNb;
        if ( Stat.DisplayPictureDisplayedByMain != Value )
        {
            STTST_Print(("VID_TestStatistics(): DisplayPictureDisplayedByMain=%d but expected I+B+P=%d !\n",
                    Stat.DisplayPictureDisplayedByMain, Value));
            RetErr = TRUE;
        }

        /* --- Check differences between decoded and displayed frames --- */

        if ( Stat.SpeedDisplayIFramesNb > Stat.DecodePictureFoundMPEGFrameI )
        {
            STTST_Print(("VID_TestStatistics(): SpeedDisplayIFramesNb=%d greater than DecodePictureFoundMPEGFrameI=%d !\n",
                     Stat.SpeedDisplayIFramesNb, Stat.DecodePictureFoundMPEGFrameI));
            RetErr = TRUE;
        }
        if ( Stat.SpeedDisplayPFramesNb > Stat.DecodePictureFoundMPEGFrameP )
        {
            STTST_Print(("VID_TestStatistics(): SpeedDisplayPFramesNb=%d greater than DecodePictureFoundMPEGFrameP=%d !\n",
                     Stat.SpeedDisplayPFramesNb, Stat.DecodePictureFoundMPEGFrameP));
            RetErr = TRUE;
        }
        if ( Stat.SpeedDisplayBFramesNb > Stat.DecodePictureFoundMPEGFrameB )
        {
            STTST_Print(("VID_TestStatistics(): SpeedDisplayBFramesNb=%d greater than DecodePictureFoundMPEGFrameB=%d !\n",
                     Stat.SpeedDisplayBFramesNb, Stat.DecodePictureFoundMPEGFrameB));
            RetErr = TRUE;
        }
        if ( Stat.DisplayPictureDisplayedByMain > Stat.DecodePictureDecodeLaunched )
        {
            STTST_Print(("VID_TestStatistics(): DisplayPictureDisplayedByMain=%d is greater than DecodePictureDecodeLaunched=%d !\n",
                     Stat.DisplayPictureDisplayedByMain, Stat.DecodePictureDecodeLaunched));
            RetErr = TRUE;
        }
    }
    if ( RetErr == FALSE )
    {
        STTST_Print(("VID_TestStatistics(): decode, pb, and display data are like expectations\n"));
    }
    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return(API_EnableError ? RetErr : FALSE );

} /* VID_TestStatistics */

#if defined USE_CLKRV

/*-------------------------------------------------------------------------
 * Function : VID_SynchroCallback
 *            Check if the current picture is displayed at expected time
 * Input    : SynchroMaxDelta, PicturePTS, NbOfPTSInterpolated, NbOfNewPict
 * Output   : SynchroDelta, DeltaPos, DeltaNeg, OutSync, BackSync,
 *            PicturePTS, NbOfPTSInterpolated, NbOfNewPict
 * Return   :
 * ----------------------------------------------------------------------*/
static void VID_SynchroCallback(STEVT_CallReason_t CC_Reason,
                                const ST_DeviceName_t RegistrantName,
                                STEVT_EventConstant_t Event,
                                void* EventData,
                                void *SubscriberData_p)
{
#if !defined STPTI_UNAVAILABLE
    STCLKRV_ExtendedSTC_t ExtendedSTC;
#endif
    STVID_PictureInfos_t *PictPt;
    STVID_SynchronisationInfo_t *SyncPt;
    ST_ErrorCode_t      ErrorCode;

    UNUSED_PARAMETER(CC_Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(SubscriberData_p);

    /* Process event data before using them ... */
    if (STVID_PreprocessEvent(RegistrantName, Event, (void *)EventData) != ST_NO_ERROR)
    {
#if defined(ST_OS20) || defined(ST_OSWINCE)
        STTBX_Print(("%s(%d): Preprocess event pb ! 0x%.8x \n", __FILE__, __LINE__, Event));
#else
        STTBX_Print(("%s(): Preprocess event pb ! 0x%.8x \n", __FUNCTION__, Event));
#endif
    }

    switch (Event)
    {
        case STVTG_VSYNC_EVT :

            /* CAUTION: VSYNC event is notified under interrupt, so process here should :
              -be VERY VERY short  -use no semaphore protection  -call no driver function */

            /* Process below has been moved inside VSync to treat only the last NEW_PICTURE_TO_BE_DISPLAYED
            event before a VSync. Specially in 3 buffers, it is possible that two of these events
            are notified per VSync, then only the last one is valid ! */
            if (PicturePTS==0)
            {
                /* No new picture to be displayed */
                return;
            }

#if !defined STPTI_UNAVAILABLE
            ErrorCode = STCLKRV_GetExtendedSTC(CLKRVHndl0, &ExtendedSTC);
            if (ErrorCode != ST_NO_ERROR)
            {
                /* No STC */
                return;
            }
            if (ExtendedSTC.BaseValue <= PreviousSTCBaseValue)
            {
                StreamProblemOccured = TRUE;
            }
            PreviousSTCBaseValue = ExtendedSTC.BaseValue;
            SynchroDelta = PicturePTS - ExtendedSTC.BaseValue;
            if (SynchroDelta > SynchroMaxDelta) /* picture in advance */
            {
                DeltaPos ++;
            }
            if (SynchroDelta < -(SynchroMaxDelta)) /* picture too late */
            {
                DeltaNeg ++;
            }
            NbOfNewPict++ ; /* nb of new pictures checked  */
            /* Margin largely over: consider mis-synchronisation is reached by adding 1 Vsync duration */
            if ((SynchroDelta > (S32)(90 * DisynchroValue + VSyncDelta)) || (SynchroDelta < (S32)(- 90 * DisynchroValue - VSyncDelta)))
            {
                if (SynchroLost == 0)
                {
                    semaphore_signal(SynchroLostSemaphore_p);
                }
                SynchroLost ++;
            }
#ifdef TRACE_UART
            TraceBuffer(("\r\nVSYNC Evt : STC=%d PTS=%d Delta=%d DeltaPos=%d DeltaNeg=%d SynchroLost=%d",
                    PreviousSTCBaseValue, PicturePTS, SynchroDelta,
                    DeltaPos, DeltaNeg, SynchroLost));
#endif
            PicturePTS = 0; /* reset (this picture is now  "already checked") */
            /* CAUTION: VSYNC event is notified under interrupt, so process here should :
              -be VERY VERY short  -use no semaphore protection  -call no driver function */

#else /* STPTI_UNAVAILABLE */

            return;
#endif /* ! STPTI_UNAVAILABLE */
            break;

        case STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT:
            if ( EventData != NULL )
            {
                PictPt = (STVID_PictureInfos_t *)(U32)EventData;
                if (PictPt->VideoParams.PTS.Interpolated)
                {
                    NbOfPTSInterpolated++;
                    return;
                }
                PicturePTS = PictPt->VideoParams.PTS.PTS ;
            }
#ifdef TRACE_UART
            TraceBuffer(("\r\nNEW_PICTURE_TO_BE_DISPLAYED Evt : PicturePTS=%d",
                    PicturePTS ));
#endif
            /* We get PTS of picture to be displayed on next VSync,
               but detected somewhere in between next VSync and previous VSync */
            break;

        case STVID_OUT_OF_SYNC_EVT:
            OutSync ++;
            break;

        case STVID_BACK_TO_SYNC_EVT:
            BackSync ++;
            break;
        case STVID_SYNCHRONISATION_CHECK_EVT :
            if ( EventData != NULL )
            {
              SyncPt = (STVID_SynchronisationInfo_t *)(U32)EventData;
              if (SyncPt->IsBackToSynchronisation)
              {
                BackToSync++;
              }
              if (SyncPt->IsLoosingSynchronisation)
              {
                LoosingSync++;
                TotalLoosingSync++;
              }
              if (SyncPt->IsSynchronisationOk)
              {
                SyncOk++;
              }
              else
              {
                SyncNOk++;
              }
              DeltaClock = SyncPt->ClocksDifference;
            }
            DeltaSynchro += DeltaClock;
            DeltaSynchro = DeltaSynchro/2;
            SquareAverage += DeltaClock*DeltaClock;
            SquareAverage = SquareAverage/2;
            StandardDeviationSquared = SquareAverage - (DeltaSynchro*DeltaSynchro);
            SyncInfoOccurences++;

        default:
            break;
    }
}
#endif /* USE_CLKRV & !7100 */

/*-------------------------------------------------------------------------
 * Function : SynchroPrintEvent
 *            Print synchro test results
 * Input    : DeltaPos, DeltaNeg, OutSync, BackSync
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
void SynchroPrintEvent(void)
{
    STTBX_Print(("Nb of picture PTS checked          : %i\n",NbOfNewPict));
    STTBX_Print(("STVID_BACK_TO_SYNC_EVT occurrences : %i\n",BackSync));
    STTBX_Print(("STVID_OUT_OF_SYNC_EVT occurrences  : %i\n",OutSync));
    STTBX_Print(("Positive derives occurrences       : %i\n",DeltaPos));
    STTBX_Print(("Negative derives occurrences       : %i\n",DeltaNeg));
#ifdef USE_CLKRV
    STTBX_Print(("STVID_SYNCHRONISATION_CHECK_EVT \n"));
    STTBX_Print(("Nb of event occurences                            : %i\n",SyncInfoOccurences));
    STTBX_Print(("Nb of IsBackToSynchronisation equals TRUE         : %i\n",BackToSync));
    STTBX_Print(("Nb of IsLoosingSynchronisation equals TRUE        : %i\n",LoosingSync));
    STTBX_Print(("Nb of Synchro Ok                                  : %i\n",SyncOk));
    STTBX_Print(("Nb of Synchro Nok                                 : %i\n",SyncNOk));
    STTBX_Print(("Nb of total loosing sync                          : %i\n",TotalLoosingSync));
    STTBX_Print(("PTS and STC ClocksDifference average              : %i\n",DeltaSynchro));
    STTBX_Print(("Standard deviation squared                        : %i\n",StandardDeviationSquared));

#endif /* USE_CLKRV */
}

/*-------------------------------------------------------------------------
 * Function : VID_TestSynchro
 *            Check synchronization bewteen displayed pictures and with STC
 * Input    :
 * Output   :
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/
#ifndef STVID_VALID
static BOOL VID_TestSynchro(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL                            RetErr = FALSE;
    U32                             TestResult = 0;
#ifdef USE_CLKRV
    STEVT_DeviceSubscribeParams_t   SubscribeParams;
    osclock_t                       Timeout;
    U32                             Speed, Tmp32, TrySkip;
	S32 Offset;
    S32                             ThisSynchroDelta;
    U32                             DeltaSyncRate = 105;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    ST_ErrorCode_t                  ErrCod = ST_NO_ERROR;
    char                            VTGDeviceName[STRING_DEVICE_LENGTH];
    ST_DeviceName_t                 VIDDeviceName;
    ST_DeviceName_t                 ClkrvName;
    STCLKRV_ExtendedSTC_t           ExtendedSTC;
    STCLKRV_OpenParams_t            stclkrv_OpenParams;
    STVTG_Handle_t                  VTGHandle;
    STVTG_TimingMode_t              VTGTimingMode;
    STVTG_ModeParams_t              VTGModeParams;
    STVTG_OpenParams_t              VTGOpenParams;
    STVID_Handle_t                  CurrentHandle;
#else /* not USE_CLKRV */

    RetErr = TRUE;
    STTBX_Print(("STCLKRV not in use, synchro test cant be run !\n"));
#endif /* not USE_CLKRV */

#if defined USE_CLKRV
    /* ------ Get and check command parameters ------ */

    RetErr = STTST_GetString( pars_p, STVID_DEVICE_NAME1 ,VIDDeviceName, sizeof(ST_DeviceName_t));
    if ( RetErr )
    {
        STTST_TagCurrentLine(pars_p, "expected video device name");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, DefaultVideoHandle, (S32*)&CurrentHandle);
    if ( RetErr )
    {
        STTST_TagCurrentLine(pars_p, "expected video handle");
        return(TRUE);
    }
    DefaultVideoHandle = CurrentHandle;

    RetErr = STTST_GetInteger( pars_p, (int)1, (S32 *)&AutoSynchro);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected test mode (1=auto=default|0=manu)" );
        return( FALSE );
    }
    RetErr = STTST_GetInteger( pars_p, (int)90, (S32 *)&Speed);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected speed used for desynchro (default=90,min=50,max=200)" );
        return( FALSE );
    }
    RetErr = STTST_GetInteger( pars_p, (int)40, (S32 *)&DisynchroValue);
    if ( RetErr )
    {
        /* note : AVSYNCDriftThreshold always set to 3600 by 'vid_init' command */
        STTST_TagCurrentLine( pars_p, "Expected time of desynchro (default is AVSYNCDriftThreshold = 2 Vsync = 40ms)");
        STTBX_Print(("This parameter is used to check greater disynchronisation than DriftThreshold value (ex= 3 seconds)\n"));
        return( FALSE );
    }
    /* Get device name */
    RetErr = STTST_GetString( pars_p, STVTG_DEVICE_NAME, VTGDeviceName, sizeof(VTGDeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected VTG DeviceName for VSYNC event");
        return( FALSE );
    }
    /* Get CLKRV device name */
    RetErr = STTST_GetString( pars_p, STCLKRV_DEVICE_NAME, ClkrvName, sizeof(ClkrvName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected clock recovery device name" );
        return( FALSE );
    }
    /* Get Max allowed advance or delay */
    RetErr = STTST_GetInteger( pars_p, (int)105, (S32 *)&DeltaSyncRate);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected Max allowed advance or delay (default is 105)" );
        return( FALSE );
    }

    /* ------ Calculate the time between 2 VSync ------ */

    ErrorCode = STVTG_Open(VTGDeviceName, &VTGOpenParams, &VTGHandle);
    if ( ErrorCode!= ST_NO_ERROR )
    {
        STTBX_Print(( "STVTG-Open(%s) : failure=0x%x ! \n",
                VTGDeviceName, ErrorCode ));
        return( FALSE );
    }
    ErrorCode = STVTG_GetMode( VTGHandle, &VTGTimingMode, &VTGModeParams );
    if ( ErrorCode!= ST_NO_ERROR )
    {
        STTBX_Print(( "STVTG_Getmode(Hndl=%d) : failure, can't retrieve VTG frequency !\n", (int)VTGHandle ));
        STVTG_Close(VTGHandle);
        return( FALSE );
    }
    STVTG_Close(VTGHandle);
    if (VTGModeParams.FrameRate==0)
    {
        STTBX_Print(( "STVTG_Getmode(Hndl=%d) : unexpected FrameRate !\n", (int)VTGHandle ));
        STTBX_Print(( "    TimingCode=%d FrameRate=%d \n", (int)VTGTimingMode, VTGModeParams.FrameRate ));
        return( FALSE );
    }
    /* at 50000 milliHz, 1 VSync each 20.00 ms or each 1800.0 units of (system clock freq./300) */
    /* at 59940 milliHz, 1 VSync each 16.68 ms or each 1501.5 */
    /* at 60000 milliHz, 1 VSync each 16.66 ms or each 1500.0 */
    VSyncDelta = (1800*50000)/VTGModeParams.FrameRate; /* 1800 for PAL, 1500 for NTSC... */

	Offset = 7 * VSyncDelta;

    /* ------ Setting for test ------ */

    PicturePTS = 0;
    NbOfNewPict = 0;
    NbOfPTSInterpolated = 0;
    /* Max allowed advance or delay : 1 VSync + 5% = (1800*50000*1.05)/framerate
     *                                2 VSync + 5% = (1800*50000*2.05)/framerate   */
    SynchroMaxDelta = ((DeltaSyncRate*1800*500)/VTGModeParams.FrameRate); /* 1800+5% for PAL, 1500+5% for NTSC... */

    if(DisynchroValue < 40)
    {
        STTBX_Print(("Apply minimum disynchronisation of 40 ms\n"));
        DisynchroValue = 40;
    }

    if(DisynchroValue != 40)
    {
        STTBX_Print(("Testing a disynchronization of %d ms\n", DisynchroValue));
        if((Speed > 100) && (DisynchroValue > 100))
        {
            STTBX_Print(("Not allowed to disynchronize more than 100 ms with speed > 100 !!\n"));
            return( FALSE );
        }
    }

    if(Speed == 100)
    {
        STTBX_Print(("Used speed = 90, because 100 won't dis-synchronize !"));
        Speed = 90;
    }
    if(Speed < 50)
    {
        STTBX_Print(("Used speed = 50 (min)"));
        Speed = 50;
    }
    if(Speed > 200)
    {
        STTBX_Print(("Used speed = 200 (max)"));
        Speed = 200;
    }

    /* Calculate time to wait at specified speed to theoritically dis-synchronize
    - if speed < 100: (100-Speed) is ~ the number of fields repeated in 2 seconds (50Hz)
    - if speed > 100: (Speed-100) is ~ the number of fields skipped in 2 seconds (50Hz)
    We should be dis-synchronized when 2 fields drift is reached, but to be sure wait for 4 more ... */
    /* Store the number of ms in Tmp32, to reach drift (default is 10 fields) */
    if (Speed == 100)
    {
        Speed = 101; /* Another security against division by 0 */
    }
    if (Speed < 100)
    {
        Tmp32 = 4*2*(DisynchroValue/20) * 1000 / (100 - Speed); /* 10 * 1000ms / (100 - speed) */
    }
    else
    {
        Tmp32 = 4*2*(DisynchroValue/20) * 1000 * Speed / 100 / (Speed - 100); /* 10 * 1000ms speed / (speed - 100) */
    }
    if(Tmp32 > 300000)
    {
        /* More than 5 minutes is too long */
        STTBX_Print(("Test take too much time > 5 minutes. Abort test !!! \n"));
        API_ErrorCount++;
        return( FALSE );
    }
    /* DVB spec: 1 PTS every 0.7 sec => Wait at least to be sure 2 seconds = 2000 ms*/
    if(Tmp32 < 2000)
    {
        Tmp32 = 2000;
    }

    /* Add before installing call back 10 ms = 1/2 VSync to reduce error of STC */
    /* So, default value for DisynchroValue is 50 ms */
    DisynchroValue += 10;

    ErrorCode = STCLKRV_Open(ClkrvName, &stclkrv_OpenParams, &CLKRVHndl0);
    if ( ErrorCode == ST_NO_ERROR)
    {
#ifndef STPTI_UNAVAILABLE
        ErrorCode = STCLKRV_GetExtendedSTC(CLKRVHndl0, &ExtendedSTC);
    }
    if (ErrorCode != ST_NO_ERROR)
    {
        /* No STC: can't proceed with test ! */
        STTBX_Print(("No STC found from STCLKRV driver (0x%x), can't proceed with test !\n", ErrorCode));
        STTBX_Print(("Test Synchro failed: Check if PCR PID is set correctly...\n"));
        API_ErrorCount++;
        STCLKRV_Close(CLKRVHndl0);
        return(FALSE);
#else
        /* No PTI, no STC: can't proceed with test ! */
        STTBX_Print(("No STC provided by PTI to STCLKRV driver !\n"));
        STTBX_Print(("Test Synchro failed: cant proceed this test with no PTI\n"));
        API_ErrorCount++;
        STCLKRV_Close(CLKRVHndl0);
        return(FALSE);
#endif /* ifdef STPTI_UNAVAILABLE */
    }

    /* MUST initialise semaphore before subscribing to events, because a signal is done in the callback */
    SynchroLostSemaphore_p = semaphore_create_fifo_timeout(0);
    /* Prevent signal of semaphore in the first part of the test */
    SynchroLost = 10;
    SynchroDelta = 0;
    PreviousSTCBaseValue = 0;
    StreamProblemOccured = FALSE;

    /* ------ Install callback ------ */
    SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)VID_SynchroCallback;
    SubscribeParams.SubscriberData_p = NULL; /* The purpose of this event handler is synchro test => not used with vid_dec */
    ErrorCode = STEVT_SubscribeDeviceEvent(EvtSubHandle,
                                        VIDDeviceName,
                                        STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT,
                                        &SubscribeParams);
    ErrorCode |= STEVT_SubscribeDeviceEvent(EvtSubHandle,
                                        VIDDeviceName,
                                        STVID_OUT_OF_SYNC_EVT,
                                        &SubscribeParams);
    ErrorCode |= STEVT_SubscribeDeviceEvent(EvtSubHandle,
                                        VIDDeviceName,
                                        STVID_BACK_TO_SYNC_EVT,
                                        &SubscribeParams);
    ErrorCode |= STEVT_SubscribeDeviceEvent(EvtSubHandle,
                                        VTGDeviceName,
                                        STVTG_VSYNC_EVT,
                                        &SubscribeParams);
    ErrorCode |= STEVT_SubscribeDeviceEvent(EvtSubHandle,
                                        VIDDeviceName,
                                        STVID_SYNCHRONISATION_CHECK_EVT,
                                        &SubscribeParams);

    if(ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("Problem found while installing the callback !!!\n"));
        API_ErrorCount++;
    }

    STTBX_Print(( " Setting : 1 VSync each %d, max. allowed derive=+/-%d units of clock freq/300 \n",
            VSyncDelta, SynchroMaxDelta ));
    STTBX_Print(( "           AVSYNCDriftThreshold=40ms for out_of_sync/back_to_sync events \n" ));

    /* ------ Step 1 : test Synchro ------ */

    /* Synchronized during 6 sec */
	ErrCod = STVID_SetSpeed(DefaultVideoHandle, 101);
	if (ErrCod != ST_ERROR_FEATURE_NOT_SUPPORTED) /* No Trick Mode : SetSpeed isn't supported so we must use SetSTCOffset to simulate dis-sync*/
    {
		ErrorCode |= STVID_SetSpeed(DefaultVideoHandle, 100);
    }
#if !defined(STCLKRV_NO_PTI)
	else
    {
		ErrorCode |= STCLKRV_SetSTCOffset(CLKRVHndl0, 0);
    }
#endif  /* #if !defined(STCLKRV_NO_PTI) ... */

    ErrorCode = STVID_EnableSynchronisation(DefaultVideoHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        TestResult |= 0x1000;
    }
    STTBX_Print(( "\n Step 1 : Enable Synchro, check derive (please wait ...) \n"));
    STOS_TaskDelayUs(DELAY_1S); /* wait 1 sec */
    BackSync    = 0;
    OutSync     = 0;
    DeltaPos    = 0;
    DeltaNeg    = 0;
    BackToSync  = 0;
    LoosingSync = 0;
    SyncOk      = 0;
    SyncNOk     = 0;
    SquareAverage = 0;
    StandardDeviationSquared = 0;
    SyncInfoOccurences = 0;
    TotalLoosingSync = 0;
    DeltaSynchro    = 0;
    DeltaClock      = 0;

    STOS_TaskDelayUs(5*DELAY_1S); /* wait 5 sec */
    /* Now check what happened... */

    SynchroPrintEvent();
    if(BackSync != 0)
    {
        TestResult |= 0x0001;
        STTBX_Print(("--> STVID_BACK_TO_SYNC_EVT occurrences > 0 !\n" ));
        BackSync = 0;
    }
    if(OutSync != 0)
    {
        TestResult |= 0x0001;
        STTBX_Print(("--> STVID_OUT_OF_SYNC_EVT occurrences > 0 !\n"));
        OutSync = 0;
        LoosingSync = 0;
    }
    if((DeltaPos != 0) || (DeltaNeg != 0))
    {
        TestResult |= 0x0001;
        STTBX_Print(("-->  Derive occurrences > 0 !\n"));
        DeltaPos = 0;
        DeltaNeg = 0;
    }
    if (NbOfNewPict == 0) /* no pictures checked */
    {
        TestResult |= 0x100000;
        if (NbOfPTSInterpolated > 0) /* if the received PTS are interpolated */
        {
            STTBX_Print(("-->  No check done because %d interpolated PTS received instead of 0 ! \n", NbOfPTSInterpolated));
        }
        else
        {
            STTBX_Print(("-->  No check done because no NEW_PICTURE_TO_BE_DISPLAYED_EVT detected !\n"));
        }
    }
    if (StandardDeviationSquared > 10000 )
    {
        STTBX_Print(("--> Standard Deviation Squared > 10000 !\n"));
        STTBX_Print(("--> Standard Deviation > 1ms !\n"));
        StandardDeviationSquared = 0;
    }

    /* ------ Step 2 : test Dis-Synchro ------ */

    /* Try to desynchro */
    STTBX_Print(( "\n Step 2 : Disable synchro + change speed to %d. Please wait (max is %d millisec)...\n", Speed, Tmp32));
	if (TicksPerOneSecond > 1000000)
	{
		/* Huge number of ticks per VSync, so take care of a possible overflow.	*/
		Timeout = STOS_time_plus( STOS_time_now(), (TicksPerOneSecond / 1000) * Tmp32 );
	}
	else
	{
		Timeout = STOS_time_plus( STOS_time_now(), (TicksPerOneSecond * Tmp32) / 1000);
	}

    /* Allow one signal of SynchroLostSemaphore_p */
    SynchroLost = 0;
    /* Disable sync and change speed: this must dis-synchronize */
    ErrorCode = STVID_DisableSynchronisation(DefaultVideoHandle);

if (ErrCod != ST_ERROR_FEATURE_NOT_SUPPORTED) /* No Trick Mode */
    {
		ErrorCode |= STVID_SetSpeed(DefaultVideoHandle, Speed);
    }
#if !defined(STCLKRV_NO_PTI)
    else
	{
		if (Speed > 100)
		   Offset = -Offset;

		ErrorCode |= STCLKRV_SetSTCOffset(CLKRVHndl0, (S32)Offset);
	}
#endif /* #if !defined(STCLKRV_NO_PTI) ... */


    /* Wait for timeout or dis-synchronization detected */
    if (semaphore_wait_timeout(SynchroLostSemaphore_p, &Timeout) == -1)
    {
        STTBX_Print(("Error : Didn't got signal that synchro is lost...\n"));
        TestResult |= 0x10000;
    }
    /* Back to nominal speed */
	if (ErrCod != ST_ERROR_FEATURE_NOT_SUPPORTED) /* Trick Mode */
		ErrorCode |= STVID_SetSpeed(DefaultVideoHandle, 100);

    if (ErrorCode != ST_NO_ERROR)
    {
        TestResult |= 0x1000;
    }
    /* Now check what happened... */
    SynchroPrintEvent();
    if((OutSync-BackSync) > 1)
    {
        /* This is not an error: we may be dis-synchronized just a little, and
        by chance we come back to synchro automatically with no action (in
        3 buffers playback this can occur often) */
        TestResult |= 0x0010;
        STTBX_Print(("--> STVID_BACK_TO_SYNC_EVT occurrences = %i !\n", BackSync));
    }
    if(OutSync == 0 )
    {
        TestResult |= 0x0010;
        STTBX_Print(("--> STVID_OUT_OF_SYNC_EVT occurrences = %i !\n", OutSync));
    }
    if (StandardDeviationSquared > 10000 )
    {
        STTBX_Print(("--> Standard Deviation Squared > 10000 !\n"));
        STTBX_Print(("--> Standard Deviation > 1ms !\n"));
        StandardDeviationSquared = 0;
    }
    OutSync = 0;
    LoosingSync = 0;
    SyncOk      = 0;
    SyncNOk     = 0;
    SquareAverage = 0;
    StandardDeviationSquared = 0;
    SyncInfoOccurences = 0;
    DeltaSynchro    = 0;
    DeltaClock      = 0;

	if (ErrCod != ST_ERROR_FEATURE_NOT_SUPPORTED) /* Trick Mode */
	{
		if((DeltaPos > 100) || (DeltaNeg == 100))
		{
			TestResult |= 0x0010;
			STTBX_Print(("-->  Too many derive occurrences !\n"));
		}
	}

    /* ------ Step 3 : test Re-Synchro ------ */

    /* Try to restore synchro */
    ThisSynchroDelta = SynchroDelta;
    BackSync = 0;
    if(AutoSynchro)
    {
        ErrorCode = STVID_EnableSynchronisation(DefaultVideoHandle);
        STTBX_Print(( "\n Step 3 : Restore speed + Synchronisation Auto (please wait ...)\n" ));
    }
    else if (ThisSynchroDelta < 0)
    {
        /* At most try 30 times to skip to re-synchronize */
        /* Add 1/2 field for better accuracy: if time corresponds to 3.8 fiels, will give 4 and not 3 ! */
        ThisSynchroDelta -= (VSyncDelta/2) ; /* 900 for PAL, 750 for NTSC... */
        ErrorCode = STVID_SkipSynchro(DefaultVideoHandle, ((-1) * (ThisSynchroDelta)));
        TrySkip = 1;
        STTBX_Print(( "\n Step 3 : Restore speed + Manual STVID_SkipSynchro(%d) (please wait ...)\n", (-1) * (ThisSynchroDelta)));
        STOS_TaskDelayUs(DELAY_1S/4); /* wait 1/4 sec */
        /* Treat special case of skip, because skipping is not always possible immediately
        (specially for long decodes like HD, ATSC, or 3 buffers), as it requires new decodes ! */
        /* When disynchronisation time is greater, skipping needs to skip even more */
        while (((ErrorCode != ST_NO_ERROR) || (BackSync == 0)) && (TrySkip < 30) && (ThisSynchroDelta < -3600))
        {
            /* As skip is not always possible, try it many 30 times. This is normal until a certain extent ! */
            ThisSynchroDelta = SynchroDelta - (VSyncDelta/2); /* Add 1/2 field for better accuracy: if time corresponds to 3.8 fiels, will give 4 and not 3 ! */
            if (ThisSynchroDelta < 0)
            {
                ErrorCode = STVID_SkipSynchro(DefaultVideoHandle, ((-1) * (ThisSynchroDelta)));
                TrySkip ++;
                STTBX_Print(( "#%d Manual skip(%d)...\n", TrySkip, (-1) * (ThisSynchroDelta)));
                STOS_TaskDelayUs(DELAY_1S/4); /* wait 1/4 sec */
            }
            else
            {
                break; /* end while */
            }
        }
        if (BackSync)
        {
            /* Here ErrorCode may be != ST_NO_ERROR if last skip was not 100% completed, but if back to sync was reached this is fine ! */
            ErrorCode = ST_NO_ERROR;
        }
    }
    else if(ThisSynchroDelta > 0)
    {
        ThisSynchroDelta += (VSyncDelta/2); /* Add 1/2 field for better accuracy: if time corresponds to 3.8 fiels, will give 4 and not 3 ! */
        ErrorCode = STVID_PauseSynchro(DefaultVideoHandle, ThisSynchroDelta);
        STTBX_Print(( "\n Step 3 : Restore speed + Manual STVID_PauseSynchro(%d) (please wait ...)\n", ThisSynchroDelta));
    }
    else
    {
        STTBX_Print(( "\n Step 3 : Restore speed. no synchro to be done \n" ));
        ErrorCode = ST_NO_ERROR;
    }
    if (ErrorCode != ST_NO_ERROR)
    {
        TestResult |= 0x1000;
    }
    /* After action is done, and during time: check if any delta is seen */
    DeltaPos    = 0;
    DeltaNeg    = 0;
    /* Now should be synchronized OK: check during a few seconds. */
    STOS_TaskDelayUs(5*DELAY_1S); /* wait 5 sec */
    /* Now check what happened... */
    SynchroPrintEvent();
    if(BackSync == 0)
    {
        TestResult |= 0x0100;
        STTBX_Print(("--> STVID_BACK_TO_SYNC_EVT occurrences = 0 !\n"));
    }
    if (SynchroMaxDelta==105)
    {
        if(OutSync != 0)
        {
            TestResult |= 0x0100;
            STTBX_Print(("--> STVID_OUT_OF_SYNC_EVT occurrences > 0 !\n"));
        }
    }
	if (ErrCod != ST_ERROR_FEATURE_NOT_SUPPORTED) /* Trick Mode */
	{
		if((DeltaPos > 100) || (DeltaNeg > 100))
		{
			TestResult |= 0x0100;
			STTBX_Print(("-->  Too many derive occurrences !\n"));
		}
	}
    if (StandardDeviationSquared > 10000 )
    {
        STTBX_Print(("--> Standard Deviation Squared > 10000 !\n"));
        STTBX_Print(("--> Standard Deviation > 1ms !\n"));
    }
    /* ------ Test termination ------ */

    /* Test REsult */
    if(TestResult == 0)
    {
        STTBX_Print(( "\nTest Synchro successful\n" ));
    }
    else
    {
        STTBX_Print(( "\nTest Synchro failed (0x%x) !\n", TestResult));
        if (StreamProblemOccured)
        {
            STTBX_Print(( "Warning: a stream looping was detected, this may have caused the problem !\n" ));
        }
        API_ErrorCount++;
    }
    /* UnInstall callbacks */
    ErrorCode = STEVT_UnsubscribeDeviceEvent(EvtSubHandle,
                                        VIDDeviceName,
                                        STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT);
    ErrorCode |= STEVT_UnsubscribeDeviceEvent(EvtSubHandle,
                                        VIDDeviceName,
                                        STVID_OUT_OF_SYNC_EVT);
    ErrorCode |= STEVT_UnsubscribeDeviceEvent(EvtSubHandle,
                                        VIDDeviceName,
                                        STVID_BACK_TO_SYNC_EVT);
    ErrorCode |= STEVT_UnsubscribeDeviceEvent(EvtSubHandle,
                                        VTGDeviceName,
                                        STVTG_VSYNC_EVT);
    ErrorCode |= STEVT_UnsubscribeDeviceEvent(EvtSubHandle,
                                        VIDDeviceName,
                                        STVID_SYNCHRONISATION_CHECK_EVT);


    if(ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("Problem found while un-installing the callback !!!\n"));
        API_ErrorCount++;
    }

    STVID_EnableSynchronisation(DefaultVideoHandle);

    /* Not used any more */
    semaphore_delete(SynchroLostSemaphore_p);

#if !defined(STCLKRV_NO_PTI)
	ErrorCode |= STCLKRV_SetSTCOffset(CLKRVHndl0, 0);
#endif  /* #if !defined(STCLKRV_NO_PTI) ... */

    STCLKRV_Close(CLKRVHndl0);

#endif /* USE_CLKRV & !7100 */
    STTST_AssignInteger(result_sym_p, TestResult, FALSE);
    return(API_EnableError ? RetErr : FALSE );
}/* end of VID_TestSynchro() */
#endif /* not STVID_VALID */

/*-------------------------------------------------------------------------
 * Function : VID_PTSCallback
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void VID_PTSCallback(STEVT_CallReason_t CC_Reason,
                                const ST_DeviceName_t RegistrantName,
                                STEVT_EventConstant_t Event,
                                void* EventData,
                                void *SubscriberData_p)
{
    STVID_PictureInfos_t *PictPt;

    UNUSED_PARAMETER(CC_Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(SubscriberData_p);

    /* Process event data before using them ... */
    if (STVID_PreprocessEvent(RegistrantName, Event, (void *)EventData) != ST_NO_ERROR)
    {
#if defined(ST_OS20) || defined(ST_OSWINCE)
        STTBX_Print(("%s(%d): Preprocess event pb ! 0x%.8x \n", __FILE__, __LINE__, Event));
#else
        STTBX_Print(("%s(): Preprocess event pb ! 0x%.8x \n", __FUNCTION__, Event));
#endif
    }

    switch (Event)
    {
        case STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT:
            PictPt = (STVID_PictureInfos_t *)(U32)EventData;
            if (!(PictPt->VideoParams.PTS.Interpolated))
            {
                NewPTS = TRUE;
                PTSFoundInTheEvent = PictPt->VideoParams.PTS;
            }
            else
            {
                NewPTS = FALSE;
            }
            break;

        case STVTG_VSYNC_EVT:
            if (NewPTS)
            {
                semaphore_signal(PTSSemaphore_p);
            }
            break;
        default:
            break;
    } /* end switch */
}

/*-------------------------------------------------------------------------*/
static BOOL VID_TestPTSInconsistency(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL                            RetErr = FALSE;
    STEVT_DeviceSubscribeParams_t   SubscribeParams;
    osclock_t                       Timeout;
    U32                             TestResult = 0;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    ST_DeviceName_t                 VIDDeviceName;
    STVID_PTS_t                     NullPTS;
    osclock_t                       TimeOriginal, TimeElapsed;
    U32                             TestTimeDurationInSeconds ; /* in sec */
    char                            VTGDeviceName[STRING_DEVICE_LENGTH];
    STEVT_OpenParams_t              EvtParams;
    STEVT_Handle_t                  EvtSubHandleLocal;         /* handle for the subscriber */
    STVID_Handle_t                  CurrentHandle;

    RetErr = STTST_GetString( pars_p, STVID_DEVICE_NAME1 ,VIDDeviceName, sizeof(ST_DeviceName_t));
    if ( RetErr )
    {
        STTST_TagCurrentLine(pars_p, "expected video device name");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, DefaultVideoHandle, (S32*)&CurrentHandle);
    if ( RetErr )
    {
        STTST_TagCurrentLine(pars_p, "expected video handle");
        return(TRUE);
    }
    DefaultVideoHandle = CurrentHandle;

    RetErr = STTST_GetInteger( pars_p, (int)30, (S32 *)&TestTimeDurationInSeconds);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected test time duration in sec (30=default)" );
        return( FALSE );
    }
    /* Get device name */
    RetErr = STTST_GetString( pars_p, STVTG_DEVICE_NAME, VTGDeviceName, sizeof(VTGDeviceName) );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected VTG DeviceName for VSYNC event");
        return( FALSE );
    }

    /* MUST initialise semaphore before subscribing to events, because a signal is done in the callback */
    PTSSemaphore_p = semaphore_create_fifo_timeout(0);

    /* Install callbacks */
    if (STEVT_Open(STEVT_DEVICE_NAME, &EvtParams, &EvtSubHandleLocal) != ST_NO_ERROR)
    {
        STTBX_Print(("Failed to open handle on event device !!\n"));
        RetErr =TRUE;
    }
    SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)VID_PTSCallback;
    SubscribeParams.SubscriberData_p = NULL; /* The purpose of this event handler is synchro test => not used with vid_dec */
    ErrorCode = STEVT_SubscribeDeviceEvent(EvtSubHandleLocal,
                                        VIDDeviceName,
                                        STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT,
                                        &SubscribeParams);
    ErrorCode |= STEVT_SubscribeDeviceEvent(EvtSubHandleLocal,
                                        VTGDeviceName,
                                        STVTG_VSYNC_EVT,
                                        &SubscribeParams);

    if(ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("Problem found while installing the callback !!!\n"));
        API_ErrorCount++;
    }

    /* Count the number of PTS Inconsistency */
    NumberOfPTSInconsistency = 0;

    /* Take the original time. */
    TimeOriginal = STOS_time_now();

    /* Initialize the PTS values used to make the test. */
    NullPTS.PTS = 0;
    NullPTS.PTS33 = FALSE;
    NullPTS.Interpolated = FALSE;
    NullPTS.IsValid = FALSE;
    LastPTS = NullPTS;
    CurrentPTS = NullPTS;
    PTSFoundInTheEvent = NullPTS;

    TimeElapsed = STOS_time_minus(STOS_time_now(), TimeOriginal) / TicksPerOneSecond;
    while ( TimeElapsed < TestTimeDurationInSeconds )
    {
        /* Wait for a PTS */
        Timeout = STOS_time_plus( STOS_time_now(), 30 * (TicksPerOneSecond/25)); /* 30x the worst time to get a picture (1s) */
        if (semaphore_wait_timeout(PTSSemaphore_p, &Timeout) == 0)
        {
            /* Ready to take a new PTS */
            NewPTS = FALSE;

            /* Locally used the CurrentPTS value */
            CurrentPTS = PTSFoundInTheEvent;

            /* Nothing to compare until LastPTS has not really affected one time by a true PTS. */
            if (LastPTS.PTS != NullPTS.PTS)
            {
                /* If CurrentPTS is greater or egal to LastPTS, we are detecting an inconsistency. */
                if ((((LastPTS).PTS33 != (CurrentPTS).PTS33) && ((LastPTS).PTS < (CurrentPTS).PTS)) ||
                    (((LastPTS).PTS33 == (CurrentPTS).PTS33) && ((LastPTS).PTS >= (CurrentPTS).PTS)))
                {
                    /* Possible actions to take : stop/start, disable synchro */
                    if (NumberOfPTSInconsistency == 0)
                    {
                        STVID_DisableSynchronisation(DefaultVideoHandle);
                        TestResult |= 0x10;
                    }
                    NumberOfPTSInconsistency++;
                }
            }

            /* Update LastPTS */
            LastPTS = CurrentPTS;
        }

        /* Update the Time Elapsed */
        TimeElapsed = STOS_time_minus(STOS_time_now(), TimeOriginal) / TicksPerOneSecond;

        /* We have found PTS */
        TestResult |= 0x1;
    }

    /* Test REsult */
    if(TestResult == 0)
    {
        STTBX_Print(( "\nTest PTS Inconsistency failed : no PTS found !\n"));
        API_ErrorCount++;
    }
    else if ((TestResult & 0x01) == 0x01)
    {
        STTBX_Print(( "\nTest PTS Inconsistency successful\n" ));
    }
    else if ((TestResult & 0x10) == 0x10)
    {
        STTBX_Print(( "\nTest PTS Inconsistency failed : %d inconsistency found !\n", NumberOfPTSInconsistency));
        API_ErrorCount++;
    }

    /* UnInstall callbacks */
    ErrorCode = STEVT_UnsubscribeDeviceEvent(EvtSubHandleLocal,
                                        VIDDeviceName,
                                        STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT);
    ErrorCode |= STEVT_UnsubscribeDeviceEvent(EvtSubHandleLocal,
                                        VTGDeviceName,
                                        STVTG_VSYNC_EVT);
    ErrorCode |= STEVT_Close(EvtSubHandleLocal);

    if(ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("Problem found while un-installing the callback !!!\n"));
        API_ErrorCount++;
    }

    /* Not used any more */
    semaphore_delete(PTSSemaphore_p);

    STTST_AssignInteger(result_sym_p, TestResult, FALSE);
    return(API_EnableError ? RetErr : FALSE );
}/* end of VID_TestPTSInconsistency() */

#ifndef STVID_NO_DISPLAY
/*-------------------------------------------------------------------------
 * Function : VID_NullPointersTest
 *            Bad parameter test : function call with null pointer
 *            (C code program because macro are not convenient for this test)
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_NullPointersTest(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    U16 NbErr;
    ST_ErrorCode_t ErrCode;
    STVID_PictureParams_t PictParams;
    STAVMEM_AllocBlockParams_t AllocParams;
    STVID_ViewPortParams_t ViewPortParams;
    STVID_ViewPortHandle_t* ViewPortHndl;
    STVID_WindowParams_t WinParams;
    STVID_InitParams_t InitParams;
    STVID_ClearParams_t ClearParams;
    STVID_Freeze_t Freeze;
    U32 UVal;
    S32 SVal;
    BOOL BVar;

    UNUSED_PARAMETER(pars_p);

    STTBX_Print(( "Call API functions with null pointers...\n" ));
    STTBX_Print(( "Caution : STVID should be 'stopped' before this test\n" ));
    NbErr = 0;

    /* sorted in alphabetic order */

    ErrCode = STVID_DataInjectionCompleted(DefaultVideoHandle, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_DataInjectionCompleted(...,NULL) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_DuplicatePicture(NULL, &PictParams);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_DuplicatePicture(NULL,...) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_DuplicatePicture(&PictParams, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_DuplicatePicture(...,NULL) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_GetCapability(STVID_DEVICE_NAME1, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_GetCapability(...,NULL) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_GetDisplayAspectRatioConversion(&ViewPortHndl, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_GetDisplayAspectRatioConversion(...,NULL) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_GetHDPIPParams(DefaultVideoHandle, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_GetHDPIPParams(...,NULL,...) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_GetInputWindowMode(ViewPortHndl, NULL, &WinParams);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_GetInputWindowMode(...,NULL,...) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_GetInputWindowMode(ViewPortHndl, &BVar, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_GetInputWindowMode(...,...,NULL) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_GetIOWindows(ViewPortHndl, NULL, &SVal, &UVal, &UVal, &SVal, &SVal, &UVal, &UVal);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_GetInputWindowMode(..., ...NULL... ) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_GetIOWindows(ViewPortHndl, &SVal, NULL, &UVal, &UVal, &SVal, &SVal, &UVal, &UVal);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_GetInputWindowMode(..., ...NULL... ) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_GetIOWindows(ViewPortHndl, &SVal, &SVal, NULL, &UVal, &SVal, &SVal, &UVal, &UVal);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_GetInputWindowMode(..., ...NULL... ) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_GetIOWindows(ViewPortHndl, &SVal, &SVal, &UVal, NULL, &SVal, &SVal, &UVal, &UVal);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_GetInputWindowMode(..., ...NULL... ) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_GetIOWindows(ViewPortHndl, &SVal, &SVal, &UVal, &UVal, NULL, &SVal, &UVal, &UVal);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_GetInputWindowMode(..., ...NULL... ) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_GetIOWindows(ViewPortHndl, &SVal, &SVal, &UVal, &UVal, &SVal, NULL, &UVal, &UVal);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_GetInputWindowMode(..., ...NULL... ) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_GetIOWindows(ViewPortHndl, &SVal, &SVal, &UVal, &UVal, &SVal, &SVal, NULL, &UVal);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_GetInputWindowMode(..., ...NULL... ) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_GetIOWindows(ViewPortHndl, &SVal, &SVal, &UVal, &UVal, &SVal, &SVal, &UVal, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_GetInputWindowMode(..., ...NULL... ) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_GetOutputWindowMode(ViewPortHndl, NULL, &WinParams);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_GetOutputWindowMode(...,NULL,...) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_GetOutputWindowMode(ViewPortHndl, &BVar, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_GetOutputWindowMode(...,...,NULL) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_GetPictureAllocParams(DefaultVideoHandle, NULL, &AllocParams);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_GetPictureAllocParams(...,NULL,...) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_GetPictureAllocParams(DefaultVideoHandle, &PictParams, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_GetPictureAllocParams(...,...,NULL) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_GetPictureInfos(DefaultVideoHandle, STVID_PICTURE_DISPLAYED, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_GetPictureInfos(...,...,NULL) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_GetPictureParams(DefaultVideoHandle, STVID_PICTURE_DISPLAYED, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_GetPictureParams(...,...,NULL) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_GetSpeed(DefaultVideoHandle, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_GetSpeed(...,...,NULL) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_Init(STVID_DEVICE_NAME1, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_Init(...,NULL) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_Open(STVID_DEVICE_NAME1, NULL, &DefaultVideoHandle);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_Open(...,NULL,...) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_OpenViewPort(DefaultVideoHandle, NULL, ViewPortHndl);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_OpenViewPort(...,NULL,...) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_OpenViewPort(DefaultVideoHandle, &ViewPortParams, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_OpenViewPort(...,...,NULL) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_Pause(DefaultVideoHandle, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_Pause(...,...,NULL) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_SetHDPIPParams(DefaultVideoHandle, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_SetHDPIPParams(...,NULL,...) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_SetInputWindowMode(ViewPortHndl, TRUE, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_SetInputWindowMode(...,...,NULL) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_SetMemoryProfile(DefaultVideoHandle, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_SetMemoryProfile(...,NULL) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_SetOutputWindowMode(ViewPortHndl, TRUE, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_SetOutputWindowMode(...,...,NULL) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_Setup(DefaultVideoHandle, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_Setup(...,NULL) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_ShowPicture(ViewPortHndl, NULL, &Freeze);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_ShowPicture(...,NULL,...) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_ShowPicture(ViewPortHndl, &PictParams, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_ShowPicture(...,...,NULL) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    ErrCode = STVID_Clear(DefaultVideoHandle, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_Clear(...,NULL) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }

    /* cases of structures containing a null pointer */
    memset( &InitParams, 0, sizeof(InitParams));
    ErrCode = STVID_Init("VID9999", &InitParams); /* use another device name to avoid already init. error here */
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_Init(...,(NULL,NULL,...)) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }
    memset( &ClearParams, 0, sizeof(ClearParams));
    ClearParams.ClearMode = STVID_CLEAR_DISPLAY_PATTERN_FILL;
    ErrCode = STVID_Clear(DefaultVideoHandle, &ClearParams);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_Clear(...,(PATTERN,NULL,...,NULL,...)) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }

    ErrCode = STVID_Init("", &InitParams);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_Init(NULL,...)) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }

    /* other case */
    ErrCode = STVID_Init("VeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryBigDeviceName", &InitParams);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "STVID_Init(...,(NULL,NULL,...)) : unexpected return code=0x%x !\n", ErrCode ));
        NbErr++;
    }

    /* test result */
    if ( NbErr == 0 )
    {
        STTBX_Print(( "### VID_NullPointersTest() result : ok \n   ST_ERROR_BAD_PARAMETER returned 41 times as expected\n" ));
        RetErr = FALSE;
    }
    else
    {
        STTBX_Print(( "### VID_NullPointersTest() result : failed !\n   ST_ERROR_BAD_PARAMETER returned %d times instead of 41\n", 41-NbErr ));
        RetErr = TRUE;
    }

    /*API_ErrorCount = API_ErrorCount + NbErr;*/

    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return(API_EnableError ? RetErr : FALSE );

} /* end of VID_NullPointersTest() */
#endif /*#ifndef STVID_NO_DISPLAY*/

/*-------------------------------------------------------------------------
 * Function : VID_TestPictDelay
 *            according to input param,
 *            - enable STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT
 *              and starts time delay measurement between pictures
 *            - display statictics
 *            - disable event and measurement
 *            Result are meaningfull only is frame rate does not change
 * Caution  :
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_TestPictDelay(STTST_Parse_t *pars_p, char *result_sym_p)
{
    U32 TestDuration;
    U32 TestEvtCount;
    S32 ExpectedFrameDuration;
    S32 ExpectedPictNb;

    /* --- Display statistics --- */

    UNUSED_PARAMETER(pars_p);

    /* values consistency needs no interrupt */
    STOS_InterruptLock();
    TestEvtCount = Pict_Counted ;
    TestDuration = (U32)STOS_time_minus( Pict_LastTime, Pict_StartTime);
    STOS_InterruptUnlock();

    /* use nano seconds to avoid rounded values */
    ExpectedFrameDuration = Pict_FrameDuration ;
    if ( ExpectedFrameDuration == 0 )
        ExpectedFrameDuration = 40000 ; /* default */
    ExpectedPictNb = 0;
    /* convert test duration in milliseconds */
    /* be very careful with U32 size limit   */
    if ( TestDuration < 0x353F7C ) /* 0xCFFFFFF / 1000 */
    {
        TestDuration = (TestDuration * 1000) / ST_GetClocksPerSecond();
        ExpectedPictNb = ((TestDuration * 1000)/ ExpectedFrameDuration) + 1 ;
    }
    else
    {
        TestDuration = (TestDuration / ST_GetClocksPerSecond()) * 1000;
        ExpectedPictNb = ( (TestDuration / ExpectedFrameDuration) * 1000 ) ;
        ExpectedPictNb = ExpectedPictNb + ( (TestDuration%ExpectedFrameDuration)/(ExpectedFrameDuration/1000) ) + 1 ;
    }
    STTST_Print(("VID_TestPictDelay() : \n   Measures : %d NEW_PICTURE events during %ld ms\n",
                TestEvtCount, TestDuration));
    STTST_Print(("   Expected : %d NEW_PICTURE events each %d ns %d\n",
                ExpectedPictNb, ExpectedFrameDuration ));
    STTST_Print(("   Advanced : %d pictures with a total of %d ms\n   Delayed  : %d pictures with a total of %d ms\n",
                Pict_Advanced, Pict_AdvanceTime,
                Pict_Delayed, Pict_DelayTime ));
    STTST_Print(("   Nb of delays greater than a frame duration=%d \n   Biggest delay registered=%d ms\n",
                 Pict_FrameDelayed, Pict_BiggerDelayTime ));

    STTST_AssignInteger("RET_VAL1", TestEvtCount, FALSE);
    STTST_AssignInteger("RET_VAL2", TestDuration, FALSE);
    STTST_AssignInteger("RET_VAL3", ExpectedPictNb, FALSE);
    STTST_AssignInteger("RET_VAL4", ExpectedFrameDuration, FALSE);
    STTST_AssignInteger("RET_VAL5", Pict_Advanced, FALSE);
    STTST_AssignInteger("RET_VAL6", Pict_AdvanceTime, FALSE);
    STTST_AssignInteger("RET_VAL7", Pict_Delayed, FALSE);
    STTST_AssignInteger("RET_VAL8", Pict_DelayTime, FALSE);
    STTST_AssignInteger("RET_VAL9", Pict_FrameDelayed, FALSE);
    STTST_AssignInteger("RET_VAL10", Pict_BiggerDelayTime, FALSE);


    STTST_AssignInteger(result_sym_p, FALSE, FALSE);
    return(FALSE);

} /* end of VID_TestPictDelay() */

/*-------------------------------------------------------------------------
 * Function : VID_TestNbFrames
 *            Display statistics
 *            related to the number of frames between 2 displayed pictures
 *            with data provided by callback VID_ReceivePictTimeCode()
 * Caution  :
 * Input    : Pict_NbOfNewRef, Pict_DisplayIsFluent, Pict_MinDifference
*             Pict_MaxDifference, Pict_TotalOfDifferences, Pict_Difference
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_TestNbFrames(STTST_Parse_t *pars_p, char *result_sym_p)
{
    UNUSED_PARAMETER(pars_p);

    STTST_Print(("VID_TestNbFrames() : \n   Nb of displayed reference pictures = %d\n",
                Pict_NbOfNewRef ));
    if (Pict_DisplayIsFluent)
    {
        STTST_Print(("   Nb of frames between 2 reference pictures is constant = TRUE\n"));
    }
    else
    {
        STTST_Print(("   Nb of frames between 2 reference pictures is constant = FALSE\n"));
    }
    /* there are N-1 differences data for N pictures */
    STTST_Print(("   Average nb of frames between 2 reference pictures = %d.%03d\n",
            (Pict_TotalOfDifferences/(Pict_NbOfNewRef-1)), (Pict_TotalOfDifferences%(Pict_NbOfNewRef-1)) ));
    STTST_Print(("   Min difference between 2 reference pictures = %d\n   Max difference between 2 reference pictures = %d\n",
            Pict_MinDifference, Pict_MaxDifference ));
    if (Pict_NbOfNegativeDiff>0)
    {
        STTST_Print(("   %d negative differences found and ignored ;\n   it may come from the input : wrong TimeCode, various IPB structure, injection loops\n",
            Pict_NbOfNegativeDiff ));
    }
    STTST_Print(("   Note : measures are started after the 20 first VSync\n" ));
    STTST_AssignInteger("RET_VAL1", Pict_NbOfNewRef, FALSE);
    STTST_AssignInteger("RET_VAL2", Pict_DisplayIsFluent, FALSE);
    STTST_AssignInteger("RET_VAL3", (1000*Pict_TotalOfDifferences)/(Pict_NbOfNewRef-1), FALSE);
    STTST_AssignInteger("RET_VAL4", Pict_MinDifference, FALSE);
    STTST_AssignInteger("RET_VAL5", Pict_MaxDifference, FALSE);
    STTST_AssignInteger("RET_VAL6", Pict_NbOfNegativeDiff, FALSE);

    STTST_AssignInteger(result_sym_p, FALSE, FALSE);
    return(FALSE);

} /* end of VID_TestNbFrames */

/*-------------------------------------------------------------------------
 * Function : VID_TestNbVSync
 *            Display statistics
 *            related to the number of VSync between 2 displayed pictures
 *            with data provided by callback VID_ReceivePictandVSync()
 * Caution  :
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_TestNbVSync(STTST_Parse_t *pars_p, char *result_sym_p)
{
    U32 Average;
    static U32 Variance;
    U32 Cpt;
    U32 CptMax;
    S32 TraceOn ;
    BOOL RetErr ;

    RetErr = STTST_GetInteger( pars_p, 0, &TraceOn);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected TraceOn (0=default=no trace 1=display measures)");
        return(TRUE);
    }

    /* --- variance calculation based on logged measures --- */

    Average = 0;
    if ( NbOfNewPict > 0 )
    {
        Average = (TotalOfVSync * 1000)/ (NbOfNewPict-1) ;
        if ( VSyncLog != NULL )
        {
            /* calculate the variance only on the NbMaxOfVSyncToLog last measures done */
            CptMax = NbOfNewPict - 1;
            if ( NbOfNewPict >= NbMaxOfVSyncToLog )
            {
                CptMax = NbMaxOfVSyncToLog - 1;
            }
            /* 1st difference known on 2nd picture, stored in VSyncLog[1] */
            Cpt = 1;
            Variance = 0;
            while ( Cpt <= CptMax )
            {
                if ( TraceOn!=0 )
                {
                    STTST_Print(("%d ", VSyncLog[Cpt]));
                }
                Variance = Variance + ( ((1000*VSyncLog[Cpt])-Average) * ((1000*VSyncLog[Cpt])-Average) );
                Cpt++;
            }
            Variance = Variance / CptMax;
            if ( TraceOn!=0 )
            {
                STTST_Print(("\n"));
            }
        }
    }

    /* --- display statistics --- */

    STTST_Print(("VID_TestNbVSync() : \n   Nb of NEW_PICTURE_TO_BE_DISPLAYED_EVT    = %d\n   Nb of VSync between 1st and last picture = %d\n",
                NbOfNewPict, TotalOfVSync ));
    if (NbOfVSyncIsConstant)
    {
        STTST_Print(("   Nb of VSync between 2 pictures is constant = TRUE\n" ));
    }
    else
    {
        STTST_Print(("   Nb of VSync between 2 pictures is constant = FALSE\n" ));
    }
    STTST_Print(("   Average nb of VSync between 2 pictures = %d.%03d \n",
                (Average/1000), (Average%1000) ));
    STTST_Print(("   Max nb of VSync between 2 pictures = %d\n",
                NbMaxOfVSyncBetween2Pict ));
    STTST_Print(("   Variance = %d.%03d \n",
                (Variance/1000000), (Variance%1000000) ));
    if ( NbOfNewPict > NbMaxOfVSyncToLog )
    {
        STTST_Print(("   Variance calculated on the %d last measures only \n", NbMaxOfVSyncToLog ));
    }
    STTST_Print(("   Note : at speed>100, measures are started after the 20 first VSync\n" ));
    STTST_AssignInteger("RET_VAL1", NbOfNewPict, FALSE);
    STTST_AssignInteger("RET_VAL2", TotalOfVSync, FALSE);
    STTST_AssignInteger("RET_VAL3", NbOfVSyncIsConstant, FALSE);
    STTST_AssignInteger("RET_VAL4", Average, FALSE);
    STTST_AssignInteger("RET_VAL5", NbMaxOfVSyncBetween2Pict, FALSE);
    STTST_AssignInteger("RET_VAL6", Variance, FALSE);

    STTST_AssignInteger(result_sym_p, FALSE, FALSE);
    return(FALSE);

} /* end of VID_TestNbVSync */

/*-------------------------------------------------------------------------
 * Function : VID_StartTest
 *            Time measurement
 *            (C code program because precision is required)
 * Caution  : purge the event stack & subscribe events before this test
 * Input    :
 * Output   : FALSE if no error
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_StartTest(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_StartParams_t VidStartParams;
    BOOL RetErr;
    osclock_t Time1;
    osclock_t Time2;
    osclock_t Time3;
    osclock_t Delay1;
    osclock_t Delay2;

    UNUSED_PARAMETER(pars_p);
    Time1 = STOS_time_now() ;
    Time2 = Time1 ;
    Time3 = Time1 ;

    VidStartParams.RealTime = TRUE;
    VidStartParams.UpdateDisplay = TRUE;
    VidStartParams.StreamType = STVID_STREAM_TYPE_PES;
    VidStartParams.StreamID = (U8)STVID_IGNORE_ID;
    VidStartParams.BrdCstProfile = (U8)STVID_BROADCAST_DVB;
    ErrCode = STVID_Start(DefaultVideoHandle, &VidStartParams );

    /* --- Wait for decoding (evt. new sequence) --- */

    RetErr = VID_EvtFound(STVID_SEQUENCE_INFO_EVT, &Time2);
    if ( RetErr )
    {
        /*STTBX_Print(( "Reg = 0x%x %x\n",
            STSYS_ReadRegDev8((void*)(0x16)),
            STSYS_ReadRegDev8((void*)(0x17)) )) ;*/
        STTBX_Print(("VID_StartTest() : STVID_SEQUENCE_INFO_EVT not found !\n"));
        API_ErrorCount++;
    }

    /* --- Wait for displaying (evt. new frame) --- */

    RetErr = VID_EvtFound(STVID_DISPLAY_NEW_FRAME_EVT, &Time3);
    if ( RetErr )
    {
        STTBX_Print(("VID_StartTest() : STVID_DISPLAY_NEW_FRAME_EVT not found !\n"));
        API_ErrorCount++;
    }

    /* --- Output the results --- */

    Delay1 = STOS_time_minus(Time2, Time1); /* decoding delay */
    Delay2 = STOS_time_minus(Time3, Time1); /* displaying delay */

    RetErr = STTST_AssignInteger("RET_VAL1",  Delay1, FALSE);
    RetErr = STTST_AssignInteger("RET_VAL2",  Delay2, FALSE);
    STTST_AssignInteger(result_sym_p, RetErr, FALSE);

    return(API_EnableError ? RetErr : FALSE );

} /* end of VID_StartTest */

/*-------------------------------------------------------------------------
 * Function : VID_LengthTest
 *            Stream Time measurement.
 *            (C code program because precision is required)
 * Caution  : purge the event stack & subscribe events before this test.
 *            The decode is started.
 * Input    : None
 * Output   : StreamDuration, duration of the stream played just before
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_LengthTest(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL    RetErr=FALSE;
    BOOL    EndOfLoop=FALSE;
    osclock_t   StreamDuration;
    U32     TickPerSecond;
    S32     NbOfMilliSeconds;

    UNUSED_PARAMETER(pars_p);

    /* Get the time base */
    TickPerSecond = ST_GetClocksPerSecond();

    /* Wait for the diplay to begin */
    STOS_TaskDelayUs ( 3 * TimeOutEvent * DELAY_1S );

    /* protection of the automaton's state  */
    STOS_InterruptLock();

    /* Check if the stream is gone on */
    if ( TimeMeasurementState != TM_IN_PROGRESS)
    {
        /* No, an error will be signaled */
        RetErr = TRUE;
        /* Not necessary to loop */
        EndOfLoop=TRUE;
        if ( TimeMeasurementState == TM_NO_ACTIVITY )
            STTBX_Print(("VID_LengthTest() : state=TM_NO_ACTIVITY, don't wait end of stream\n"));
        if ( TimeMeasurementState==TM_ARMED )
            STTBX_Print(("VID_LengthTest() : state=TM_ARMED, don't wait end of stream\n"));
        if ( TimeMeasurementState==TM_ASK_IF_IN_PROGRESS )
            STTBX_Print(("VID_LengthTest() : state=TM_ASK_IF_IN_PROGRESS, don't wait end of stream\n"));
    }
    else
    {
        /* else, test the event activity */
        TimeMeasurementState = TM_ASK_IF_IN_PROGRESS;
    }
    STOS_InterruptUnlock();

    if (!RetErr)
    {
        /* Trace the beginning of the stream time measurement */
        STTBX_Print(("VID_LengthTest() : Started at time %d\n", (int)InitialTime));

        while (!(EndOfLoop))
        {
            /* Wait the next event */
            STOS_TaskDelayUs ( TimeOutEvent * DELAY_1S );

            /* protection of the automaton's state  */
            STOS_InterruptLock();
            /* Check if the stream is finished */
            if ( TimeMeasurementState == TM_IN_PROGRESS)
            {
                /* Test the event activity */
                TimeMeasurementState = TM_ASK_IF_IN_PROGRESS;
            }
            else
            {
                EndOfLoop = TRUE;  /* no new event received after timeout */
            }
            STOS_InterruptUnlock();
        }
        /* Trace the end of the stream time measurement */
        STTBX_Print(("VID_LengthTest() : finished at time=%d \n   TimeMeasurementState=%d ReceiveNewFrameEvtCount=%d\n",
            (int)IntermediaryTime, TimeMeasurementState, ReceiveNewFrameEvtCount ));

        /* --- Output the results --- */
        StreamDuration = STOS_time_minus(IntermediaryTime, InitialTime);
        if ( StreamDuration < 4294967 ) /* if lower than hFFFFFFFF/1000 to avoid overflow */
        {
            NbOfMilliSeconds = (StreamDuration*1000)/TickPerSecond ;
        }
        else
        {
            if (StreamDuration < TickPerSecond)
            {
                NbOfMilliSeconds = StreamDuration / ( TickPerSecond / 1000 );
            }
            else
            {
                NbOfMilliSeconds = (StreamDuration/TickPerSecond)*1000 ;
            }
        }
        RetErr = STTST_AssignInteger("RET_VAL1",  NbOfMilliSeconds, FALSE);
    }
    else
    {
        /* Trace the error case */
        STTBX_Print(("VID_LengthTest() : Feature not initialized, or no event occured \n"));
    }

    /* Rearm the automaton */
    TimeMeasurementState = TM_ARMED;

    /* Return the standard result. */
    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return(API_EnableError ? RetErr : FALSE );

} /* end of VID_LengthTest */


/*******************************************************************************
Name        : VID_StopStart_Callback
Description : Callback function to subscribe the VIDBUFF event new picture to be
              displayed. A new picture is decoded, the fct is called
              through event-callback interface.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void VID_StopStart_Callback(STEVT_CallReason_t Reason,
                                    const ST_DeviceName_t RegistrantName,
                                    STEVT_EventConstant_t Event,
                                    void *EventData_p,
                                    void *SubscriberData_p)
{
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData_p);
    UNUSED_PARAMETER(SubscriberData_p);

    semaphore_signal(StartStopSemaphore_p);
}

/*******************************************************************************
Name        : VID_StopStart_CloseVP
Description : VID_StopStart sub routine to clove the viewport
Parameters  :
Assumptions :
Limitations :
Returns     : FALSE if OK, TRUE if error
*******************************************************************************/
static BOOL VID_StopStart_CloseVP(STVID_ViewPortHandle_t CurrentViewportHndl)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;

    /* Close Viewport ***************************/
    RetErr = TRUE;
    sprintf(VID_Msg, "STVID_CloseViewPort(viewportHndl=0x%x) : ", (S32) CurrentViewportHndl );
    ErrCode = STVID_CloseViewPort(CurrentViewportHndl );
    switch (ErrCode)
    {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [0x%x] !\n", VID_Msg, ErrCode );
                break;
    } /* end switch */
    STTBX_Print((VID_Msg ));

    return RetErr;
}



/*******************************************************************************
Name        : VID_StopStart_Clear
Description : VID_StopStart sub routine to handle the call to stvid_clear
Parameters  :
Assumptions :
Limitations :
Returns     : FALSE if OK, TRUE if error
*******************************************************************************/
static BOOL VID_StopStart_Clear(STVID_Handle_t CurrentHandle)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STVID_ClearParams_t ClearParams;

    memset( &ClearParams, 0, sizeof(ClearParams));
    ClearParams.ClearMode = STVID_CLEAR_FREEING_DISPLAY_FRAME_BUFFER;

    RetErr = TRUE;
    ErrCode = STVID_Clear(CurrentHandle, &ClearParams);

    sprintf(VID_Msg, "STVID_Clear(handle=0x%x,Mode=%s) : ",
                 CurrentHandle,(ClearParams.ClearMode == STVID_CLEAR_DISPLAY_BLACK_FILL)   ? "BLACK_FILL" :
                 (ClearParams.ClearMode == STVID_CLEAR_DISPLAY_PATTERN_FILL) ? "PATTERN_FILL" : "FREEING");
    switch (ErrCode)
    {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break ;
            case STVID_ERROR_DECODER_RUNNING :
                API_ErrorCount++;
                RetErr = TRUE;
                strcat(VID_Msg, "decoder not stopped !\n" );
                break ;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                RetErr = TRUE;
                strcat(VID_Msg, "bad parameter !\n" );
                break ;
            case ST_ERROR_INVALID_HANDLE :
                API_ErrorCount++;
                RetErr = TRUE;
                strcat(VID_Msg, "invalid handle !\n" );
                break ;
             case STVID_ERROR_MEMORY_ACCESS :
                API_ErrorCount++;
                RetErr = TRUE;
                strcat(VID_Msg, "memory access error !\n" );
                break ;
             case STVID_ERROR_NOT_AVAILABLE :
                API_ErrorCount++;
                RetErr = TRUE;
                strcat(VID_Msg, "video error not available !\n" );
                break ;
             default :
                API_ErrorCount++;
                RetErr = TRUE;
                strcat(VID_Msg, "unknown error !\n" );
                break ;
    }
    STTBX_Print((VID_Msg ));
    return RetErr;
}



/*-------------------------------------------------------------------------
 * Function : VID_TestStopStart
 *            Stop and immediately start the decoding and display
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_TestStopStart(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    STVID_Stop_t StopMode;
    STVID_StartParams_t StartParams;
    STVID_ViewPortParams_t ViewPortParams;
    STVID_Freeze_t Freeze;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    U32 StopStartMode;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    STEVT_DeviceSubscribeParams_t STEVT_SubscribeParams;
    osclock_t                       Timeout;

    /* default modes */
    StopStartMode = 0;
    StopMode = STVID_STOP_NOW;
    Freeze.Mode = STVID_FREEZE_MODE_FORCE;
    Freeze.Field = STVID_FREEZE_FIELD_TOP;
    TicksPerOneSecond = ST_GetClocksPerSecond();
    RetErr = FALSE;
    ErrCode = ST_ERROR_INVALID_HANDLE;


   /* Parse Parameters ********************************** */
    /* --- get Video Handle --- */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }

    /* --- get ViewPortHandle --- */
    RetErr = STTST_GetInteger(pars_p, (int)ViewPortHndl0, &LVar );
    CurrentViewportHndl = (STVID_ViewPortHandle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected ViewPortHandle" );
    }

    /* get LayerName */
    if (!(RetErr))
    {
        RetErr = STTST_GetString(pars_p, STLAYER_DEVICE_NAME1, ViewPortParams.LayerName, \
                        sizeof(ViewPortParams.LayerName) );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected LayerName");
        }
    }

    /* --- Get the StopStartParameter --- */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected StopStart mode (0=stop/start, 1=same+closeVP/openVP, 2=same+clear, 3=2 in different order)" );
        }
    }
    if (!(RetErr))
    {
        StopStartMode = (U32)LVar;
    }

    /* Get the StartParams...  */
    /* get RealTime */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, TRUE, &LVar );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected RealTime decoding (TRUE=yes FALSE=no)" );
        }
    }
    /* get UpdateDisplay  */
    if (!(RetErr))
    {
        StartParams.RealTime = (BOOL)LVar;
        RetErr = STTST_GetInteger(pars_p, TRUE, &LVar );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected UpdateDisplay (TRUE=yes FALSE=no)" );
        }
    }
    /* get stream type */
    if (!(RetErr))
    {
        StartParams.UpdateDisplay = (BOOL)LVar;
#ifdef STVID_DIRECTV
        RetErr = STTST_GetInteger(pars_p, STVID_STREAM_TYPE_ES, &LVar);
#else
        RetErr = STTST_GetInteger(pars_p, STVID_STREAM_TYPE_PES, &LVar);
#endif
        if (RetErr)
        {
          sprintf(VID_Msg, "expected StreamType (%x=ES,%x=PES,%x=Mpeg1 pack.)",
                   STVID_STREAM_TYPE_ES, STVID_STREAM_TYPE_PES,
                   STVID_STREAM_TYPE_MPEG1_PACKET );
          STTST_TagCurrentLine(pars_p, VID_Msg );
        }
    }
    /* get stream id (default : accept all packets in decoder) */
    if (!(RetErr))
    {
        StartParams.StreamType = LVar;
        RetErr = STTST_GetInteger(pars_p, STVID_IGNORE_ID, &LVar);
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected id (from 0 to 15)");
        }
    }
    /* get broadcast type */
    if (!(RetErr))
    {
        StartParams.StreamID = (U8)LVar;
#ifdef STVID_DIRECTV
        RetErr = STTST_GetInteger(pars_p, STVID_BROADCAST_DIRECTV, &LVar);
#else
        /* Default is DVB for everybody */
        RetErr = STTST_GetInteger(pars_p, STVID_BROADCAST_DVB, &LVar);
#endif
        if (RetErr)
        {
            sprintf(VID_Msg, "expected broadcast type (%d=DVB,%d=DirecTV,%d=ATSC,%d=DVD)",
                    STVID_BROADCAST_DVB, STVID_BROADCAST_DIRECTV,
                    STVID_BROADCAST_ATSC, STVID_BROADCAST_DVD );
            STTST_TagCurrentLine(pars_p, VID_Msg );
        }
    }
    /* get DecodeOnce */
    if (!(RetErr))
    {
        StartParams.BrdCstProfile = (U8)LVar;
        RetErr = STTST_GetInteger(pars_p, FALSE, &LVar);
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected DecodeOnce (FALSE=all picture, TRUE=1 picture)" );
        }
        else
        {
            StartParams.DecodeOnce = (U8)LVar;
        }
    }


    /* Call VID_STOP  *************************************/
    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "STVID_Stop(handle=0x%x,stopmode=%d,&freeze=0x%x) :",
                 CurrentHandle, StopMode, (int)&Freeze );
        ErrCode = STVID_Stop(CurrentHandle, StopMode, &Freeze );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                /* Give info to injection task that the video is stopped for this handle */
                /* because VID_GetBufferFreeSize() return total freee size when stopped  */
                /* This means that the injection task is feeding the bit buffer.         */
                for(LVar=0;LVar<VIDEO_MAX_DEVICE;LVar++)
                {
                    if ((VID_Inj_GetDriverHandle(LVar) == CurrentHandle))
                        break;
                }
                if (LVar != VIDEO_MAX_DEVICE)
                {
                    VID_Inj_SetVideoDriverState(LVar,STATE_DRIVER_STOPPED);
                }
                break;
            case STVID_ERROR_DECODER_STOPPED:
                API_ErrorCount++;
                strcat(VID_Msg, "decoder already stopped !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED:
                API_ErrorCount++;
                strcat(VID_Msg, "one feature is not supported !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        STTBX_Print((VID_Msg ));
    }

    /* depending on StopStartMode, closeViewport and Clear, or Clear and close viewport */
    if (!RetErr)
    {
        switch (StopStartMode)
        {
            case 0:
                  break;  /*do nothing */

            case 1:       /* Close Viewport */
                  RetErr = VID_StopStart_CloseVP(CurrentViewportHndl);
                  break;

            case 2:       /* Close VP and call Stvid Clear */
                  RetErr = VID_StopStart_CloseVP(CurrentViewportHndl);
                  if (!RetErr)
                  {
                      RetErr = VID_StopStart_Clear(CurrentHandle);
                  }
                  break;

            case 3:     /* Clear, and close VP */
                  RetErr = VID_StopStart_Clear(CurrentHandle);
                  if (!RetErr)
                  {
                      RetErr = VID_StopStart_CloseVP(CurrentViewportHndl);
                  }
                  break;

            default:
                  STTBX_Print((VID_Msg, "VID_StopStart Mode must be between 0 and 3!\n" ));
                  RetErr = TRUE;
        }
    }


    /* Start again  ***************************/
    if (!RetErr )
    {
        RetErr = TRUE;
        sprintf(VID_Msg, "STVID_Start(handle=0x%x,&param=0x%x) : ",
                 CurrentHandle, (S32)&StartParams );
        ErrCode = STVID_Start(CurrentHandle, &StartParams );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                /* Store info (for injection task) that the video is started for this handle  */
                /* because VID_GetBufferFreeSize() return total free size when stopped        */
                /* This means that the injection task is feeding continuously the bit buffer. */
                for(LVar=0;LVar<VIDEO_MAX_DEVICE;LVar++)
                {
                    if (VID_Inj_GetDriverHandle(LVar) == CurrentHandle)
                    {
                        VID_Inj_SetVideoDriverState(LVar, STATE_DRIVER_STARTED);
                    }
                }
                break;
            case STVID_ERROR_DECODER_PAUSING:
                API_ErrorCount++;
                strcat(VID_Msg, "decoder already pausing !\n" );
                break;
            case STVID_ERROR_DECODER_FREEZING:
                API_ErrorCount++;
                strcat(VID_Msg, "decoder is freezing !\n" );
                break;
            case STVID_ERROR_DECODER_RUNNING:
                API_ErrorCount++;
                strcat(VID_Msg, "decoder already decoding !\n" );
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER:
                API_ErrorCount++;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            case ST_ERROR_NO_MEMORY:
                API_ErrorCount++;
                strcat(VID_Msg, "not enough space !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        STTBX_Print((VID_Msg ));
    }


    /* Open VP ***************************/
    if ((!RetErr) && (StopStartMode >= 1))
    {
        RetErr = TRUE;
        ErrCode = STVID_OpenViewPort(CurrentHandle, &ViewPortParams, &CurrentViewportHndl );
        sprintf(VID_Msg, "STVID_OpenViewPort(handle=0x%x,layer=%s,viewporthndl=0x%x) : ",
                 CurrentHandle, ViewPortParams.LayerName, (U32)CurrentViewportHndl );
        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                /*if (API_TestMode == TEST_MODE_MONO_HANDLE)
                {*/
                    ViewPortHndl0 = CurrentViewportHndl; /* default viewport */
                /*}*/
                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                /* Pass a NULL handle in case of bad initialization so that the driver   */
                /* recognizes that the handle value is incorrect (for subsequent call to */
                /* VID_EnableOutputWindow() for example) */
                ViewPortHndl0 = NULL;
                CurrentViewportHndl = NULL;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                /* Pass a NULL handle in case of bad initialization so that the driver   */
                /* recognizes that the handle value is incorrect (for subsequent call to */
                /* VID_EnableOutputWindow() for example) */
                ViewPortHndl0 = NULL;
                CurrentViewportHndl = NULL;
                strcat(VID_Msg, "one parameter is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                /* Pass a NULL handle in case of bad initialization so that the driver   */
                /* recognizes that the handle value is incorrect (for subsequent call to */
                /* VID_EnableOutputWindow() for example) */
                ViewPortHndl0 = NULL;
                CurrentViewportHndl = NULL;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        }
        STTBX_Print((VID_Msg ));
    }

    /* Subscribe to VIDBUFF new picture to be displayed EVT **************************** */
    if ( (!RetErr) && (StopStartMode >= 1) )
    {
        RetErr = TRUE;
        /* create the semaphore for signaling the new pict to be displayed */
          StartStopSemaphore_p = semaphore_create_fifo_timeout(0);

        /* Handle is valid: point to device */
       /*vhdl->Device_p->DeviceData_p->EventsHandle */

        STEVT_SubscribeParams.NotifyCallback   = (STEVT_DeviceCallbackProc_t)VID_StopStart_Callback;
        STEVT_SubscribeParams.SubscriberData_p = (void *) (CurrentHandle);
        sprintf(VID_Msg, "STEVT_SubscribeDeviceEvent (...): ");
        ErrCode = STEVT_SubscribeDeviceEvent(EvtSubHandle,
                                      EventVIDDevice,
                                      STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT,
                                      &STEVT_SubscribeParams);

        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );
                break ;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                RetErr = TRUE;
                strcat(VID_Msg, "Subscribe event bad parameter\n" );
                break ;
             default :
                API_ErrorCount++;
                RetErr = TRUE;
                strcat(VID_Msg, "unknown error !\n" );
                break ;
        }
        STTBX_Print((VID_Msg ));
    }



    /* wait for a picture to be displayed ***************** */
    /* And Enable viewport *************************** */
    if ( (!RetErr) && (StopStartMode >= 1) )
    {
        RetErr = TRUE;
        /* Wait for a PTS */
        Timeout = STOS_time_plus( STOS_time_now(), 30 * (TicksPerOneSecond/25)); /* 30x the worst time to get a picture (1s) */
        if (semaphore_wait_timeout(StartStopSemaphore_p, &Timeout) == 0)
        {
            sprintf(VID_Msg, "STVID_EnableOutputWindow(viewportHndl=0x%x) : ", (S32) CurrentViewportHndl );
            ErrCode = STVID_EnableOutputWindow(CurrentViewportHndl );
            switch (ErrCode)
            {
                case ST_NO_ERROR :
                    strcat(VID_Msg, "ok\n" );
                    RetErr=FALSE;
                    break;
                case ST_ERROR_INVALID_HANDLE:
                    API_ErrorCount++;
                    strcat(VID_Msg, "handle is invalid !\n" );
                    break;
                default:
                    API_ErrorCount++;
                    sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                    break;
            } /* end switch */
        }
        else
        {
             sprintf(VID_Msg, "No STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT event found!!!!\n");
             STTBX_Print((VID_Msg ));
             API_ErrorCount++;
             /* Restore OutputWindow anyway */
            sprintf(VID_Msg, "STVID_EnableOutputWindow(viewportHndl=0x%x) : ", (S32) CurrentViewportHndl );
            ErrCode = STVID_EnableOutputWindow(CurrentViewportHndl );
            switch (ErrCode)
            {
                case ST_NO_ERROR :
                    strcat(VID_Msg, "ok\n" );
                    break;
                case ST_ERROR_INVALID_HANDLE:
                    API_ErrorCount++;
                    strcat(VID_Msg, "handle is invalid !\n" );
                    break;
                default:
                    API_ErrorCount++;
                    sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                    break;
            } /* end switch */
        }
        STTBX_Print((VID_Msg ));

        /* UnInstall callbacks */
        ErrCode = STEVT_UnsubscribeDeviceEvent(EvtSubHandle,
                                        STVID_DEVICE_NAME1,

        STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("Problem found while un-installing the callback !!!\n"));
            API_ErrorCount++;
        }

        /* Not used any more */
        semaphore_delete(StartStopSemaphore_p);

    } /* end if (!RetErr) && (StopStartMode >= 1) */

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* VID_TestStopStart() */

/*-------------------------------------------------------------------------
 * Function : VID_GetHandleOfLastPictureBufferDisplayed
 *            This function returns the Picture Buffer handle of the last picture displayed.
 * Caution  : Before using this function you should enable STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT event
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetHandleOfLastPictureBufferDisplayed(STTST_Parse_t *pars_p, char *result_sym_p)
{
#if 0 /* The video handle is not necessary for the moment */
    STVID_Handle_t CurrentHandle;
#endif
    BOOL RetErr;

    UNUSED_PARAMETER(pars_p);

#if 0 /* The video handle is not necessary for the moment */
    /* --- get Video Handle --- */
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
        else
        {
            DefaultVideoHandle = CurrentHandle;
        }
    }
    else
    {
        CurrentHandle = VID_Inj_GetDriverHandle(0);
    }
#endif

    STTST_AssignInteger(result_sym_p, (S32)HandleOfLastPictureBufferDisplayed, FALSE);
    RetErr = STTST_AssignInteger("RET_VAL1", (S32) HandleOfLastPictureBufferDisplayed, FALSE);

    return (API_EnableError ? RetErr : FALSE );
} /* end of VID_LengthTest */


#if defined (mb290)
/*-------------------------------------------------------------------------
 * Function : AVMEM_Reinit
 *            Terminate, then initialize AVMEM
 * Input    :
 * Parameter: True=use all memory range(default), False=use 1 range only
 *            (vid_show/vid_hide need one range to avoid bad access to 2 ranges)
 * Output   :
 * Return   : FALSE if no error
 * ----------------------------------------------------------------------*/
static BOOL AVMEM_Reinit( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL                            RetErr;
    ST_ErrorCode_t                  ErrorCode;
    STAVMEM_TermParams_t            TermParams;
    STAVMEM_DeletePartitionParams_t DeleteParams;
    STAVMEM_InitParams_t            InitAvmem;
    STAVMEM_CreatePartitionParams_t CreateParams;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
    STAVMEM_MemoryRange_t PartitionMapTable[MAX_PARTITIONS];
    S32 UseAllMemoryRange;
    U32 Count;

    /* Code extracted from stapigat/src/clavmem.c */
    ErrorCode = ST_NO_ERROR;

    RetErr = STTST_GetInteger( pars_p, 1, &UseAllMemoryRange);
    if ( RetErr )
    {
        sprintf(VID_Msg, "expected UseAllMemoryRange (0=only 1 range, 1=all range=default)");
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(API_EnableError ? RetErr : FALSE );
    }

    /* --- Terminate --- */

    DeleteParams.ForceDelete =FALSE;
    for (Count=0; Count < MAX_PARTITIONS; Count++)
    {
        ErrorCode = STAVMEM_DeletePartition(STAVMEM_DEVICE_NAME, &DeleteParams, &AvmemPartitionHandle[Count]);
        if (ErrorCode == STAVMEM_ERROR_ALLOCATED_BLOCK)
        {
            STTBX_Print(("AVMEM_Reinit() : Warning STAVMEM_DeletePartition(%s) #%d ! Still allocated block\n",
                     STAVMEM_DEVICE_NAME, Count));
            DeleteParams.ForceDelete = TRUE;
            STAVMEM_DeletePartition(STAVMEM_DEVICE_NAME, &DeleteParams, &AvmemPartitionHandle[Count]);
        }
    }
    TermParams.ForceTerminate = FALSE;
    ErrorCode = STAVMEM_Term(STAVMEM_DEVICE_NAME, &TermParams);
    if (ErrorCode == STAVMEM_ERROR_CREATED_PARTITION)
    {
        STTBX_Print(("AVMEM_Reinit() : Warning STAVMEM_Term(%s) ! Still created partition\n",
                 STAVMEM_DEVICE_NAME));
        TermParams.ForceTerminate = TRUE;
        ErrorCode = STAVMEM_Term(STAVMEM_DEVICE_NAME, &TermParams);
    }
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("AVMEM_Term(%s): STAVMEM_Term() error 0x%0x !\n", STAVMEM_DEVICE_NAME, ErrorCode));
        RetErr = TRUE;
        return(API_EnableError ? RetErr : FALSE );
    }

    /* --- Initialize --- */

    PartitionMapTable[0].StartAddr_p = (void *) (PARTITION0_START);
    if (UseAllMemoryRange)
    {
        PartitionMapTable[0].StopAddr_p  = (void *) (PARTITION0_STOP);
        STTBX_Print(("AVMEM_Reinit() : allow access to %d bytes\n",
                (PARTITION0_STOP - PARTITION0_START)));
    }
    else
    {
        /* mem. access restricted to the 1st 32Mbytes for ST20 and STi7020 */
        PartitionMapTable[0].StopAddr_p  = (void *) ((SDRAM_BASE_ADDRESS + SDRAM_WINDOW_SIZE - 1));
        STTBX_Print(("AVMEM_Reinit() : allow access to %d bytes only\n",
                (SDRAM_BASE_ADDRESS + SDRAM_WINDOW_SIZE - 1 - PARTITION0_START)));
    }

    VirtualMapping.PhysicalAddressSeenFromCPU_p    = (void *)PHYSICAL_ADDRESS_SEEN_FROM_CPU;
    VirtualMapping.PhysicalAddressSeenFromDevice_p = (void *)PHYSICAL_ADDRESS_SEEN_FROM_DEVICE;
    VirtualMapping.PhysicalAddressSeenFromDevice2_p= (void *)PHYSICAL_ADDRESS_SEEN_FROM_DEVICE2;
    VirtualMapping.VirtualBaseAddress_p            = (void *)VIRTUAL_BASE_ADDRESS;
    VirtualMapping.VirtualSize                     = VIRTUAL_SIZE;
    VirtualMapping.VirtualWindowOffset             = 0;
    VirtualMapping.VirtualWindowSize               = VIRTUAL_WINDOW_SIZE;

    InitAvmem.MaxPartitions                 = MAX_PARTITIONS;
    InitAvmem.MaxBlocks                     = 58;    /* video:       1 bits buffer + 4 frame buffers
                                                        tests video: 2 dest buffer
                                                        audio:       1 bits buffer
                                                        osd :        50 */
    InitAvmem.MaxForbiddenRanges            = 3 /* =0 without OSD */;   /* forbidden ranges not used */
    InitAvmem.MaxForbiddenBorders           = 3 /* =1 without OSD */;  /* video will use one forbidden border */
    InitAvmem.CPUPartition_p                = SystemPartition_p;
    InitAvmem.NCachePartition_p             = NCachePartition_p;
    InitAvmem.MaxNumberOfMemoryMapRanges    = MAX_PARTITIONS; /* because each partition has a single range here */
    InitAvmem.BlockMoveDmaBaseAddr_p        = (void *) 0x00 /* no BM_BASE_ADDRESS*/;
    InitAvmem.CacheBaseAddr_p               = (void *) ST5514_CACHE_BASE_ADDRESS /*CACHE_BASE_ADDRESS*/;
    InitAvmem.VideoBaseAddr_p               = (void *) VIDEO_BASE_ADDRESS;
    InitAvmem.SDRAMBaseAddr_p               = (void *) MPEG_SDRAM_BASE_ADDRESS;
    InitAvmem.SDRAMSize                     = MPEG_SDRAM_SIZE;
    InitAvmem.NumberOfDCachedRanges         = (DCacheMapSize / sizeof(STBOOT_DCache_Area_t)) - 1;
    InitAvmem.DCachedRanges_p               = (STAVMEM_MemoryRange_t *) DCacheMap_p;
    InitAvmem.DeviceType                    = STAVMEM_DEVICE_TYPE_VIRTUAL;
    InitAvmem.SharedMemoryVirtualMapping_p  = &VirtualMapping;
    InitAvmem.OptimisedMemAccessStrategy_p  = NULL;
#ifdef USE_GPDMA
    strcpy(InitAvmem.GpdmaName, STGPDMA_DEVICE_NAME);
#endif /*#ifdef USE_GPDMA*/

    if (STAVMEM_Init(STAVMEM_DEVICE_NAME, &InitAvmem) != ST_NO_ERROR)
    {
        STTBX_Print(("AVMEM_Init() failed !\n"));
        RetErr = TRUE;
    }
    else
    {
        for (Count=0; (Count < MAX_PARTITIONS) && (!RetErr); Count++)
        {
            CreateParams.NumberOfPartitionRanges = (sizeof(PartitionMapTable[Count]) / sizeof(STAVMEM_MemoryRange_t));;
            CreateParams.PartitionRanges_p       = &PartitionMapTable[Count];
            ErrorCode = STAVMEM_CreatePartition(STAVMEM_DEVICE_NAME, &CreateParams, &AvmemPartitionHandle[Count]);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Print(("AVMEM_Reinit() : AVMEM_CreatePartition() #%d failed !\n", Count));
                RetErr = TRUE;
            }
        }
        if (!(RetErr))
        {
            /* Init and CreatePartition successed */
            STTBX_Print(("AVMEM_Reinit() : STAVMEM initialized,\trevision=%s\n",STAVMEM_GetRevision() ));
        }
    }
    return(API_EnableError ? RetErr : FALSE );
}
#endif /* mb290 */

/*-------------------------------------------------------------------------
 * Function : VID_SetPattern
 *            Fills the arrays LumaPattern and ChromaPattern with the given
 *            patterns
 * Input    : The array to fill in, a series of bytes for the pattern
 * Parameter:
 *
 * Output   :
 * Return   : FALSE if no error
 * ----------------------------------------------------------------------*/
static BOOL VID_SetPattern(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL    RetErr;
    S16     TokenCount;
    U32     i;
    S32     LVar;
    U8*     Pattern_p;

    RetErr = STTST_GetTokenCount(pars_p, &TokenCount);
    if(RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Can't count the tokens" );
        return(TRUE);
    }
    /* Get the pattern to fill */
    RetErr = STTST_GetInteger(pars_p, 0, &LVar );
    if(RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected (0=Luma, 1=Chroma)" );
        return(TRUE);
    }
    Pattern_p = (LVar == 0) ? LumaPattern : ChromaPattern;

    /* Get the different bytes values of the pattern */
    for(i=0; (i < (U32)TokenCount-2) && (!RetErr); i++)
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        if(RetErr)
        {
            STTST_TagCurrentLine(pars_p, "Expected a byte value" );
            return(TRUE);
        }
        else
        {
            Pattern_p[i] = (U8)LVar;
        }
    }
    STTBX_Print(("VID_PATTERN(%s, pattern) : Filled buffer at %d with %d bytes\r\n",
                (Pattern_p == LumaPattern) ? "Luma=0" : "Chroma=1", (int)Pattern_p, (TokenCount-2)));

    STTST_AssignInteger("RET_VAL1", (S32)Pattern_p, FALSE);
    STTST_AssignInteger("RET_VAL2", (S32)(TokenCount-2), FALSE);
    STTST_AssignInteger(result_sym_p, (int)Pattern_p, FALSE);

    return(RetErr);
}


#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_ZEUS) || defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
/*-------------------------------------------------------------------------
 * Function : VID_GetDeltaLXTopInfo
 *            Gets and prints Delta LX top level infos through
 *            DeltaTop transformer GetCapability function
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static BOOL VID_GetDeltaLXTopInfo(STTST_Parse_t *pars_p, char *result_sym_p )
{
#if defined(ST_OSLINUX)
    STTBX_Print(("ERROR: Not supported on linux\n"));
    return FALSE;
#else
    BOOL                                   RetErr = FALSE;
    S32                                     LVar;
    char                                    TransformerName[MME_MAX_TRANSFORMER_NAME];
    U32                                     CPURegistersBaseAddress;
    U32                                     DeviceRegistersBaseAddress;
    MME_ERROR                         ErrorCode;
    U32                                     Trials;
    U32                                     MaxNbLX;
    U32                                     DefaultLX;
    MME_TransformerCapability_t             MME_TransformerCapability;
    DeltaTop_TransformerCapability_t        DeltaTop_TransformerCapability;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
    MaxNbLX = 1;
    DefaultLX = 1;
#elif defined(ST_7100) || defined(ST_7109) || defined(ST_ZEUS)
    MaxNbLX = 2;
    DefaultLX = 1;
#elif defined(ST_7200)
    MaxNbLX = 4;
    DefaultLX = 2;
#else
    STTBX_Print((STTBX_REPORT_LEVEL_ERROR, "No Delta in this IC\n"));
#endif

    /* get LX Number */
    RetErr = STTST_GetInteger(pars_p, DefaultLX, &LVar);
    if (RetErr || (LVar > MaxNbLX))
    {
        STTST_TagCurrentLine(pars_p, "expected LX Number" );
        return(TRUE);
    }

#if defined(ST_7100)
    if(LVar == 1)
        CPURegistersBaseAddress = ST7100_DELTA_BASE_ADDRESS;
    else
        CPURegistersBaseAddress = 0;
#elif defined(ST_7109)
    if(LVar == 1)
        CPURegistersBaseAddress = ST7109_DELTA_BASE_ADDRESS;
    else
        CPURegistersBaseAddress = 0;
#elif defined(ST_7200)
    if(LVar == 2)
        CPURegistersBaseAddress = ST7200_DELTAMU_0_BASE_ADDRESS;
    else if(LVar == 4)
        CPURegistersBaseAddress = ST7200_DELTAMU_1_BASE_ADDRESS;
    else
        CPURegistersBaseAddress = 0;
#elif defined(ST_ZEUS)
    if(LVar == 1)
        CPURegistersBaseAddress = ZEUS_DELTA0_BASE_ADDRESS;
    else if(LVar == 2)
        CPURegistersBaseAddress = ZEUS_DELTA1_BASE_ADDRESS;
    else
        CPURegistersBaseAddress = 0;
#elif defined(ST_DELTAPHI_HE)
    if(LVar == 1)
        CPURegistersBaseAddress = DELTAPHI_HE_VIDEO_BASE_ADDRESS;
    else
        CPURegistersBaseAddress = 0;
#elif defined(ST_DELTAMU_HE)
    if(LVar == 1)
        CPURegistersBaseAddress = DELTAPHI_HE_VIDEO_BASE_ADDRESS;
    else
        CPURegistersBaseAddress = 0;
#else
    STTBX_Print((STTBX_REPORT_LEVEL_ERROR, "No Delta in this IC\n"));
#endif

    if(CPURegistersBaseAddress == 0)
    {
        STTST_TagCurrentLine(pars_p, "Invalid DeviceNumber" );
        return(TRUE);
    }
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
#if !defined( ST_7200)
    DeviceRegistersBaseAddress = (U32)STAVMEM_CPUToDevice(CPURegistersBaseAddress,&VirtualMapping);
#else /* do not map registers for 7200 */
    DeviceRegistersBaseAddress = CPURegistersBaseAddress;
#endif

    MME_TransformerCapability.StructSize = sizeof(MME_TransformerCapability_t);
    MME_TransformerCapability.TransformerInfoSize = sizeof(DeltaTop_TransformerCapability_t);
    MME_TransformerCapability.TransformerInfo_p = &DeltaTop_TransformerCapability;

    sprintf(TransformerName, "%s_%08X", DELTATOP_MME_TRANSFORMER_NAME, DeviceRegistersBaseAddress);
    Trials = 0;
    ErrorCode = MME_UNKNOWN_TRANSFORMER;
    while ((ErrorCode != MME_SUCCESS) && (Trials < PING_TIME_OUT))
    {
        ErrorCode = MME_GetTransformerCapability(TransformerName, &MME_TransformerCapability);
        Trials++;
    }

    if(ErrorCode != MME_SUCCESS)
    {
        STTBX_Print(("Not able to query DeltaTop informations for video device %d (tried %d times)\n", LVar, Trials));
    }
    else
    {
        STTBX_Print(("DeltaTop information for video device %d (succeded after %d trials)\n", LVar, Trials));
        STTBX_Print(("  Heap usage (from libc mallinfo() function)\n"));
        STTBX_Print(("    arena    : %10d bytes (total space allocated from system)\n",DeltaTop_TransformerCapability.DeltaTop_HeapAllocationStatus.arena));
        STTBX_Print(("    ordblks  : %10d (number of non-inuse chunks)\n",DeltaTop_TransformerCapability.DeltaTop_HeapAllocationStatus.ordblks));
        STTBX_Print(("    hblks    : %10d (number of mmapped regions)\n",DeltaTop_TransformerCapability.DeltaTop_HeapAllocationStatus.hblks));
        STTBX_Print(("    hblkhd   : %10d (total space in mmapped regions)\n",DeltaTop_TransformerCapability.DeltaTop_HeapAllocationStatus.hblkhd));
        STTBX_Print(("    uordblks : %10d bytes (total allocated space)\n",DeltaTop_TransformerCapability.DeltaTop_HeapAllocationStatus.uordblks));
        STTBX_Print(("    fordblks : %10d bytes (total non-inuse space)\n",DeltaTop_TransformerCapability.DeltaTop_HeapAllocationStatus.fordblks));
        STTBX_Print(("    keepcost : %10d bytes (top-most, releasable (via malloc_trim) space)\n",DeltaTop_TransformerCapability.DeltaTop_HeapAllocationStatus.keepcost));
    }

    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return(API_EnableError ? RetErr : FALSE );
#endif /* #if defined(ST_OSLINUX) ... #else ... */
} /* end of VID_GetDeltaLXTopInfo() */

/*-------------------------------------------------------------------------
 * Function : VID_PingLX
 *            Gets and prints LX heap usage and version string using
 *            Ping transformer GetCapability function
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static BOOL VID_PingLX(STTST_Parse_t *pars_p, char *result_sym_p )
{
#if defined(ST_OSLINUX)
    STTBX_Print(("ERROR: Not supported on linux\n"));
    return FALSE;
#else
    #define  MAX_RELEASE_STRING_SIZE        2048

    BOOL                                    RetErr = FALSE;
    U32                                     Trials;
    S32                                     LVar;
    U32                                     MaxNbLX;
    U32                                     DefaultLX;
    char                                    TransformerName[MME_MAX_TRANSFORMER_NAME];
    MME_ERROR                               ErrorCode;
    MME_TransformerCapability_t             MME_TransformerCapability;
    char                                    ReleaseString[MAX_RELEASE_STRING_SIZE];
    char                                    *TokStr;

#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
    MaxNbLX = 1;
    DefaultLX = 1;
#elif defined(ST_7100) || defined(ST_7109) || defined(ST_ZEUS)
    MaxNbLX = 2;
    DefaultLX = 1;
#elif defined(ST_7200)
    MaxNbLX = 4;
    DefaultLX = 2;
#else
    STTBX_Print((STTBX_REPORT_LEVEL_ERROR, "No Delta in this IC\n"));
#endif

    /* get LX Number */
    RetErr = STTST_GetInteger(pars_p, DefaultLX, &LVar);
    if (RetErr || (LVar > MaxNbLX))
    {
        STTST_TagCurrentLine(pars_p, "expected LX Number" );
        return(TRUE);
    }

    MME_TransformerCapability.StructSize = sizeof(MME_TransformerCapability_t);
    MME_TransformerCapability.TransformerInfoSize = MAX_RELEASE_STRING_SIZE;
    MME_TransformerCapability.TransformerInfo_p = &(ReleaseString[0]);

    sprintf(TransformerName, "%s_%d", PING_MME_TRANSFORMER_NAME, (U32)LVar);
    Trials = 0;
    ErrorCode = MME_UNKNOWN_TRANSFORMER;
    while ((ErrorCode != MME_SUCCESS) && (Trials < PING_TIME_OUT))
    {
        ErrorCode = MME_GetTransformerCapability(TransformerName, &MME_TransformerCapability);
        Trials++;
    }
    if(ErrorCode != MME_SUCCESS)
    {
        STTBX_Print(("Not able to Ping LX %d (tried %d times)\n", LVar, Trials));
    }
    else
    {
        STTBX_Print(("Ping information for LX %d (succeded after %d trials)\n", LVar, Trials));
        STTBX_Print(("  *** Heap usage ***\n"));
        STTBX_Print(("    Total Heap  : %10d bytes\n",*((int *) &MME_TransformerCapability.InputType.FourCC)));
        STTBX_Print(("    Used Heap   : %10d bytes\n",*((int *) &MME_TransformerCapability.OutputType.FourCC)));
        STTBX_Print(("  *** Version string ***\n"));
        TokStr = strtok((char *)&(ReleaseString[0]), "\n");
        while(TokStr != NULL)
        {
            STTBX_Print(("    %s\n", TokStr));
            TokStr = strtok(NULL, "\n");
        }
    }

    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return(API_EnableError ? RetErr : FALSE );
#endif /* #if defined(ST_OSLINUX) ... #else ... */
} /* end of VID_PingLX() */
#endif /* defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_ZEUS) || defined(DELTAPHI_HE) || defined(ST_DELTAMU_HE) */



#if defined(STVID_USE_CRC) || defined(STVID_VALID)
/*-------------------------------------------------------------------------
 * Function : VVID_ReplaceExtension
 *            Replace extension in a filename
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void VVID_ReplaceExtension(char * OutputString, char * InputString, const char * ReplacementString)
{
  char * StreamNameExtension;
  int Dot = '.';

	StreamNameExtension = strrchr(InputString, Dot);
	if (StreamNameExtension != NULL)
	{
	  strncpy(OutputString, InputString, strlen(InputString)-strlen(StreamNameExtension)+1);
	  OutputString[strlen(InputString)-strlen(StreamNameExtension)+1]=0;
  }
  else
  {
    strncpy(OutputString, InputString, VID_MAX_STREAM_NAME);
	  OutputString[strlen(InputString)]=0;
  }
  strncat(OutputString, ReplacementString, VID_MAX_STREAM_NAME);
} /* End of VVID_ReplaceExtension function */

/*-------------------------------------------------------------------------
 * Function : VVID_UpdateVVID_NUM_PIC_DISPLAY
 *            Testtool function called every N seconds by the application to
 *            monitor the number of pictures displayed
 * Input    : ProducerHandle, Picture number of picture just pushed to the display
 * Output   : none
 * Return   : none
 * ----------------------------------------------------------------------*/
static BOOL VVID_UpdateVVID_NUM_PIC_DISPLAY(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL                       RetErr;
    STVID_Handle_t       CurrentHandle;
    S32                         LVar;
    STVID_Statistics_t    Statistics;
    U32                         InjectNumber;
    ST_DeviceName_t    DeviceName;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = FALSE;

    /* get Handle */
    RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
    CurrentHandle = (STVID_Handle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected Handle" );
        return (TRUE);
    }
    else
    {
        DefaultVideoHandle = CurrentHandle;
    }

    if (!(RetErr))
    {
        InjectNumber = VID_GetSTVIDNumberFromHandle(CurrentHandle);

        VID_Inj_GetDeviceName(InjectNumber,DeviceName);
        STVID_GetStatistics(DeviceName, &Statistics);
        STTST_AssignInteger("VVID_NUM_PIC_DISPLAY",Statistics.QueuePicturePushedToDisplay,0);
    }
    return RetErr;
} /* End of VVID_UpdateVVID_NUM_PIC_DISPLAY() function */

/*-------------------------------------------------------------------------
 * Function : VVID_ReadMultiStream
 *            Finds the next line in the multi-stream file that needs to
 *            be read, reads out that line, and records the next line to be read.
 * Input    : *pars_p, *result_sym_p
 * Output   : FALSE if no error
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VVID_ReadMultiStream(STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    char MessageLine[256];
    char NextStreamFileName[VID_MAX_STREAM_NAME];
    char TempNextStreamFileName[VID_MAX_STREAM_NAME];
#ifdef USE_OSPLUS
    void* NextStreamFile;
    void* CRCDumpFile;
    void* CRCLogFile;
#ifdef STVID_VALID_DEC_TIME
    void* TimDumpFile;
#endif /* STVID_VALID_DEC_TIME */
#else /* USE_OSPLUS */
    FILE* NextStreamFile;
    FILE* CRCDumpFile;
    FILE* CRCLogFile;
#ifdef STVID_VALID_DEC_TIME
    FILE* TimDumpFile;
#endif /* STVID_VALID_DEC_TIME */
#endif /* not USE_OSPLUS */
    U32  NumFileName;
    char TotalStreamName[VID_MAX_STREAM_NAME];
    char TotalCRCName[VID_MAX_STREAM_NAME];
    char TempTotalCRCName[VID_MAX_STREAM_NAME];
    char TotalCRCLogName[VID_MAX_STREAM_NAME];
    char TempTotalCRCLogName[VID_MAX_STREAM_NAME];
    char TotalCRCDumpName[VID_MAX_STREAM_NAME];
    char TempTotalCRCDumpName[VID_MAX_STREAM_NAME];
#ifdef STVID_VALID_DEC_TIME
    char TotalTimDumpName[VID_MAX_STREAM_NAME];
    char TempTotalTimDumpName[VID_MAX_STREAM_NAME];
#endif /* STVID_VALID_DEC_TIME */
    char StreamCategory[VID_MAX_STREAM_NAME];
    char NextStreamName[VID_MAX_STREAM_NAME];
    char NextStreamPath[VID_MAX_STREAM_NAME];
    char VvidAutoLogDir[VID_MAX_STREAM_NAME];
    char TempVvidKillMeFileName[VID_MAX_STREAM_NAME];
    char VvidKillMeFileName[VID_MAX_STREAM_NAME];
    U32  BitBufferSize;
    U32  IHSize;
    U32  IVSize;
    U32  DPBSize;
    char Vid1Codec[30];
    U32  VPHSize;
    U32  VPVSize;
    U32  OHSize;
    U32  OVSize;
    U32  NumPicturesToDisplay;
    char VvidTestingMode[32];

#ifdef ST_ZEUS
	char Str_Buffer[250];
#endif

  UNUSED_PARAMETER(result_sym_p);

  strncpy(VvidKillMeFileName, "../../", VID_MAX_STREAM_NAME);
  RetErr = STTST_EvaluateString("VVID_KILL_ME_FILENAME",TempVvidKillMeFileName,VID_MAX_STREAM_NAME);
  strncat(VvidKillMeFileName, TempVvidKillMeFileName, VID_MAX_STREAM_NAME);
  STTST_AssignString("VVID_KILL_ME_FILENAME",VvidKillMeFileName,0);


  strncpy(NextStreamFileName, "../../", VID_MAX_STREAM_NAME);
  RetErr = STTST_EvaluateString("VVID_NEXT_STREAM_FILENAME", TempNextStreamFileName, VID_MAX_STREAM_NAME);
  if (RetErr)
  {
      STTST_TagCurrentLine( pars_p, "VVID_NEXT_STREAM_FILENAME not defined!!!" );
      return(TRUE);
  }
  strncat(NextStreamFileName, TempNextStreamFileName, VID_MAX_STREAM_NAME);
#if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
	if((NextStreamFile = (FILE*) Memfopen(NextStreamFileName,"r")) == NULL)
#else
	if((NextStreamFile = fopen(NextStreamFileName,"r")) == NULL)
#endif
	{
        sprintf(MessageLine, "Unable to open file \"%s\" (VVID_NEXT_STREAM_FILENAME)", NextStreamFileName);
        STTST_TagCurrentLine( pars_p, MessageLine);
        return(TRUE);
	}

#if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
  Memfread(Str_Buffer,4,250,NextStreamFile);
  sscanf(Str_Buffer, "%s %s %s %u %u %u %u %s %u %u %u %u %u",
		    (char *)StreamCategory,(char *)NextStreamName, (char *)NextStreamPath, &BitBufferSize, &IHSize,
			  &IVSize, &DPBSize, (char *)Vid1Codec, &VPHSize, &VPVSize, &OHSize,
	        &OVSize, &NumPicturesToDisplay);
if (Str_Buffer == NULL)
	{
        sprintf(MessageLine, "Unable to read definition of the next stream from file \"%s\" (VVID_NEXT_STREAM_FILENAME)", NextStreamFileName);
		STTST_TagCurrentLine( pars_p, MessageLine);
        return(TRUE);
	}

#else
	if(fscanf(NextStreamFile, "%s %s %s %u %u %u %u %s %u %u %u %u %u",
		    (char *)StreamCategory,(char *)NextStreamName, (char *)NextStreamPath, &BitBufferSize, &IHSize,
			&IVSize, &DPBSize, (char *)Vid1Codec, &VPHSize, &VPVSize, &OHSize,
	        &OVSize, &NumPicturesToDisplay) == EOF)
	{
        sprintf(MessageLine, "Unable to read definition of the next stream from file \"%s\" (VVID_NEXT_STREAM_FILENAME)", NextStreamFileName);
		STTST_TagCurrentLine( pars_p, MessageLine);
        return(TRUE);
	}
#endif


	RetErr = STTST_EvaluateString("VVID_STREAM_ROOT_DIR",TotalStreamName,VID_MAX_STREAM_NAME);

    if (TotalStreamName[strlen(TotalStreamName)-1] != '/')
    {
        strncat(TotalStreamName, "/", VID_MAX_STREAM_NAME);
    }
	if (RetErr)
	{
        STTST_TagCurrentLine( pars_p, "root directory for streams (VVID_STREAM_ROOT_DIR) not defined.");
		return(TRUE);
	}

	strncat(TotalStreamName, NextStreamPath, VID_MAX_STREAM_NAME);
    strncpy(TempTotalCRCName, TotalStreamName, VID_MAX_STREAM_NAME);
	strncat(TotalStreamName, NextStreamName, VID_MAX_STREAM_NAME);

	strncat(TempTotalCRCName, "crc/", VID_MAX_STREAM_NAME); /* CRC are in a sub directory called crc */
	strncat(TempTotalCRCName, NextStreamName, VID_MAX_STREAM_NAME);
#ifdef STVID_USE_DEBLOCK
        VVID_ReplaceExtension(TotalCRCName, TempTotalCRCName, "deblock_crc");
#else /* STVID_USE_DEBLOCK */
    /* Use field CRCs for vertical size > 720 lines */
    /* NOTE PC : Currently only for H264 because reference for VC1 and MPEG2 not available yet */
    /*           Update VIDCRC_IsCRCModeField() in vid_crc.c accordingly */
    if(IVSize > 720)
    {
        VVID_ReplaceExtension(TotalCRCName, TempTotalCRCName, "field_crc");
    }
    else
    {
        VVID_ReplaceExtension(TotalCRCName, TempTotalCRCName, "crc");
    }
#endif /* Not STVID_USE_DEBLOCK */

  /* CRC log file set up */
    RetErr = STTST_EvaluateString("VVID_AUTOLOGDIR",VvidAutoLogDir,VID_MAX_STREAM_NAME);
    if((VvidAutoLogDir[0] != '/') &&
       (VvidAutoLogDir[0] != '\\') &&
       (VvidAutoLogDir[1] != ':'))
    { /* Path is not absolute, add ../../ to cope with obj/STxx subdirectory */
        strncpy(TempTotalCRCLogName, "../../", VID_MAX_STREAM_NAME);
    }
    else
    {
        TempTotalCRCLogName[0] = 0;
    }
    strncat(TempTotalCRCLogName, VvidAutoLogDir, VID_MAX_STREAM_NAME);
    strncat(TempTotalCRCLogName, "/", VID_MAX_STREAM_NAME);
    strncat(TempTotalCRCLogName, NextStreamName, VID_MAX_STREAM_NAME);
    VVID_ReplaceExtension(TotalCRCLogName, TempTotalCRCLogName, "crc.log");
    strcpy(TempTotalCRCLogName, TotalCRCLogName);
    NumFileName = 0;
    do
    {
#ifdef USE_OSPLUS
        CRCLogFile = SYS_FOpen(TotalCRCLogName, "r");
#else /* USE_OSPLUS */
#if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
        CRCLogFile = (FILE*)Memfopen(TotalCRCLogName, "r");
#else
        CRCLogFile = fopen(TotalCRCLogName, "r");
#endif /* not ST_ZEUS */
#endif /* not USE_OSPLUS */
      if (CRCLogFile != NULL)
      {
#ifdef USE_OSPLUS
        SYS_FClose(CRCLogFile);
#else /* USE_OSPLUS */
#if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
       Memfclose(CRCLogFile);
#else
        fclose(CRCLogFile);
#endif /* not ST_ZEUS */
#endif /* not USE_OSPLUS */
        NumFileName++;
        sprintf(TotalCRCLogName, "%s.%04d", TempTotalCRCLogName, NumFileName);
      }
  } while (CRCLogFile != NULL);

  /* CRC dump file set up */
  RetErr = STTST_EvaluateString("VVID_AUTOLOGDIR",VvidAutoLogDir,VID_MAX_STREAM_NAME);
  if((VvidAutoLogDir[0] != '/') &&
     (VvidAutoLogDir[0] != '\\') &&
     (VvidAutoLogDir[1] != ':'))
  { /* Path is not absolute, add ../../ to cope with obj/STxx subdirectory */
    strncpy(TempTotalCRCDumpName, "../../", VID_MAX_STREAM_NAME);
  }
  else
  {
    TempTotalCRCDumpName[0] = 0;
  }
  strncat(TempTotalCRCDumpName, VvidAutoLogDir, VID_MAX_STREAM_NAME);
  strncat(TempTotalCRCDumpName, "/", VID_MAX_STREAM_NAME);
  strncat(TempTotalCRCDumpName, NextStreamName, VID_MAX_STREAM_NAME);
  VVID_ReplaceExtension(TotalCRCDumpName, TempTotalCRCDumpName, "crc.dump");
  strcpy(TempTotalCRCDumpName, TotalCRCDumpName);
  NumFileName = 0;
  do
  {
#ifdef USE_OSPLUS
    CRCDumpFile = SYS_FOpen(TotalCRCDumpName, "r");
#else /* USE_OSPLUS */
#if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
    CRCDumpFile = (FILE*)Memfopen(TotalCRCDumpName, "r");
#else
    CRCDumpFile = fopen(TotalCRCDumpName, "r");
#endif /* not ST_ZEUS */
#endif /* not USE_OSPLUS */
      if (CRCDumpFile != NULL)
      {
#ifdef USE_OSPLUS
        SYS_FClose(CRCDumpFile);
#else /* USE_OSPLUS */
#if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
        Memfclose(CRCDumpFile);
#else
        fclose(CRCDumpFile);
#endif /* not ST_ZEUS */
#endif /* not USE_OSPLUS */
        NumFileName++;
        sprintf(TotalCRCDumpName, "%s.%04d", TempTotalCRCDumpName, NumFileName);
      }
  } while (CRCDumpFile != NULL);

#ifdef STVID_VALID_DEC_TIME
  /* Time dump file set up */
  RetErr = STTST_EvaluateString("VVID_AUTOLOGDIR",VvidAutoLogDir,VID_MAX_STREAM_NAME);
  if((VvidAutoLogDir[0] != '/') &&
     (VvidAutoLogDir[0] != '\\') &&
     (VvidAutoLogDir[1] != ':'))
  { /* Path is not absolute, add ../../ to cope with obj/STxx subdirectory */
    strncpy(TempTotalTimDumpName, "../../", VID_MAX_STREAM_NAME);
  }
  else
  {
    TempTotalTimDumpName[0] = 0;
  }
  strncat(TempTotalTimDumpName, VvidAutoLogDir, VID_MAX_STREAM_NAME);
  strncat(TempTotalTimDumpName, "/", VID_MAX_STREAM_NAME);
  strncat(TempTotalTimDumpName, NextStreamName, VID_MAX_STREAM_NAME);
  VVID_ReplaceExtension(TotalTimDumpName, TempTotalCRCDumpName, "tim.dump");
  strcpy(TempTotalTimDumpName, TotalTimDumpName);
  NumFileName = 0;
  do
  {
#ifdef USE_OSPLUS
    TimDumpFile = SYS_FOpen(TotalTimDumpName, "r");
#else /* USE_OSPLUS */
#if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
    TimDumpFile = (FILE*)Memfopen(TotalTimDumpName, "r");
#else
    TimDumpFile = fopen(TotalTimDumpName, "r");
#endif /* not ST_ZEUS */
#endif /* not USE_OSPLUS */
      if (TimDumpFile != NULL)
      {
#ifdef USE_OSPLUS
        SYS_FClose(TimDumpFile);
#else /* USE_OSPLUS */
#if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
        Memfclose(TimDumpFile);
#else
        fclose(TimDumpFile);
#endif /* not ST_ZEUS */
#endif /* not USE_OSPLUS */
        NumFileName++;
        sprintf(TotalTimDumpName, "%s.%04d", TempTotalTimDumpName, NumFileName);
      }
  } while (TimDumpFile != NULL);
#endif /* STVID_VALID_DEC_TIME */

	/* CRC number of pictures to display is 0 for current CRC implementation */
	/* CRC is taking care about signalling to the application that the test is ended */
  RetErr = STTST_EvaluateString("VVID_TESTING_MODE",VvidTestingMode,32);
  if (!strcmp(VvidTestingMode, "crc"))
  {
    NumPicturesToDisplay = 0;
  }

#ifdef USE_OSPLUS
	if((NextStreamFile = SYS_FOpen(TotalStreamName, "r")) == NULL)
	{
        sprintf(MessageLine, "Unable to access stream file \"%s\" specified in \"%s\" (VVID_NEXT_STREAM_FILENAME)", TotalStreamName, NextStreamFileName);
        STTST_TagCurrentLine(pars_p, MessageLine);
        return(TRUE);
	}
	SYS_FClose(NextStreamFile);
#else /* USE_OSPLUS */

#if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
  if((NextStreamFile = (FILE*) Memfopen(TotalStreamName, "r")) == NULL)
	{
        sprintf(MessageLine, "Unable to access stream file \"%s\" specified in \"%s\" (VVID_NEXT_STREAM_FILENAME)", TotalStreamName, NextStreamFileName);
        STTST_TagCurrentLine(pars_p, MessageLine);
        return(TRUE);
	}

  	Memfclose(NextStreamFile);

#else
	if((NextStreamFile = fopen(TotalStreamName, "r")) == NULL)
	{
        sprintf(MessageLine, "Unable to access stream file \"%s\" specified in \"%s\" (VVID_NEXT_STREAM_FILENAME)", TotalStreamName, NextStreamFileName);
        STTST_TagCurrentLine(pars_p, MessageLine);
        return(TRUE);
	}
	fclose(NextStreamFile);
#endif /* not ST_ZEUS */

#endif /* not USE_OSPLUS */

	STTST_AssignString("STREAM_CATEGORY",StreamCategory,0);
	STTST_AssignString("STREAM_NAME",TotalStreamName,0);
	STTST_AssignInteger("BIT_BUFFER_SIZE",BitBufferSize,0);
	STTST_AssignInteger("IHSIZE",IHSize,0);
	STTST_AssignInteger("IVSIZE",IVSize,0);
	STTST_AssignInteger("DPBSIZE",DPBSize,0);
	STTST_AssignString("VID1_CODEC",Vid1Codec,0);
	STTST_AssignInteger("VPHSIZE",VPHSize,0);
	STTST_AssignInteger("VPVSIZE",VPVSize,0);
	STTST_AssignInteger("OHSIZE",OHSize,0);
	STTST_AssignInteger("OVSIZE",OVSize,0);
	STTST_AssignInteger("VVID_TOTAL_PICTURES",NumPicturesToDisplay,0);
	STTST_AssignString("VVID_CRC_NAME",TotalCRCName,0);
	STTST_AssignString("VVID_CRC_LOG",TotalCRCLogName,0);
	STTST_AssignString("VVID_CRC_DUMP",TotalCRCDumpName,0);
#ifdef STVID_VALID_DEC_TIME
	STTST_AssignString("VVID_TIM_DUMP",TotalTimDumpName,0);
#endif /* STVID_VALID_DEC_TIME */

    return(FALSE);
} /* end VVID_ReadMultiStream() */

/*---------------------------------------------------
 * GetFirwareReleaseInfo
 *  Parses the Video LX firmware code in memory to find the
 *  version info string and extract the firmware version
 * --------------------------------------------------*/
static void GetFirwareReleaseInfo(STVID_DeviceType_t DeviceType, void *BaseAddress_p, char *FirmwareRelease)
{
    char   *CurrentPos;
    char   *EndPos;
    char   *MaxPos;
    BOOL    Found;
    char   H264FirmwareVersion[256];
    char   VC1FirmwareVersion[256];
    char   MPEG2FirmwareVersion[256];
    char   MPEG4P2FirmwareVersion[256];
    char   *address;
    size_t  maxsize;

#if !defined(ST_ZEUS)
    UNUSED_PARAMETER(BaseAddress_p);
#endif

#if defined(ST_OSLINUX)
    strcpy(FirmwareRelease, "Firmware Release N/A under Linux ");
#else

#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
    address = (void*)DELTAPHI_HE_LX_LOAD_ADDRESS;
    maxsize = DELTAPHI_HE_LX_CODE_SIZE;
#elif defined ST_7100 || defined ST_7109
/*    address = (void*)LX_VIDEO_CODE_ADDRESS; */
/* Cannot use LX_VIDEO_CODE_ADRESS since it's defined in a LxLoader.c file*/
/* but Video LX code placed at the very beginning of the LMI_SYS */
    address = (void*)(LMI_SYS_BASE);
    maxsize = LX_VIDEO_CODE_MEMORY_SIZE;

#elif defined ST_ZEUS
    if(BaseAddress_p == (void *)ZEUS_DELTA0_BASE_ADDRESS)
    {
        address = (void*)LX1_VIDEO_CODE_ADDRESS;
        maxsize = LX1_VIDEO_CODE_MEMORY_SIZE;
    }
    else
    {
        address = (void*)LX2_VIDEO_CODE_ADDRESS;
        maxsize = LX2_VIDEO_CODE_MEMORY_SIZE;
    }
#endif

    MaxPos = address+maxsize;
    CurrentPos = address;
    Found = FALSE;
    while(!Found &&
          (CurrentPos != NULL) &&
          (CurrentPos < MaxPos))
    {
        CurrentPos = memchr(CurrentPos, LX_VERSION_PREFIX[0], MaxPos - CurrentPos);
        if(CurrentPos != NULL)
        {
            if(!memcmp(CurrentPos, LX_VERSION_PREFIX, LX_VERSION_PREFIX_SIZE))
            {
                Found = TRUE;
            }
            else
            {
                CurrentPos++;
            }
        }
    }
    if(Found)
    {
        CurrentPos = strchr(CurrentPos, '\n');  /* Skip header */
        CurrentPos++;

        CurrentPos = strchr(CurrentPos, '\n');  /* Skip target identification */
        CurrentPos++;

        CurrentPos = strchr(CurrentPos, '\n');  /* Skip delta top application identification */
        CurrentPos++;

        EndPos = strchr(CurrentPos, '\n');  /* find end of line */
        EndPos[0] = 0;
        strcpy(H264FirmwareVersion, CurrentPos); /* Get H264 firmware identification */
        strcat(H264FirmwareVersion, "\n");
        EndPos[0] = '\n';
        CurrentPos = EndPos + 1;
        EndPos = strchr(CurrentPos, '\n');  /* find end of line */
        EndPos[0] = 0;
        strcat(H264FirmwareVersion, CurrentPos); /* Get H264 IPF firmware identification */
        EndPos[0] = '\n';
        CurrentPos = EndPos + 1;

        EndPos = strchr(CurrentPos, '\n');  /* find end of line */
        EndPos[0] = 0;
        strcpy(VC1FirmwareVersion, CurrentPos); /* Get VC1 firmware identification */
        strcat(VC1FirmwareVersion, "\n");
        EndPos[0] = '\n';
        CurrentPos = EndPos + 1;
        EndPos = strchr(CurrentPos, '\n');  /* find end of line */
        EndPos[0] = 0;
        strcat(VC1FirmwareVersion, CurrentPos); /* Get VC1 IPF firmware identification */
        EndPos[0] = '\n';
        CurrentPos = EndPos + 1;

        EndPos = strchr(CurrentPos, '\n');  /* find end of line */
        EndPos[0] = 0;
        strcpy(MPEG2FirmwareVersion, CurrentPos); /* Get MPEG2 firmware identification */
        strcat(MPEG2FirmwareVersion, "\n");
        EndPos[0] = '\n';
        CurrentPos = EndPos + 1;
        EndPos = strchr(CurrentPos, '\n');  /* find end of line */
        EndPos[0] = 0;
        strcat(MPEG2FirmwareVersion, CurrentPos); /* Get MPEG2 IPF firmware identification */
        EndPos[0] = '\n';
        CurrentPos = EndPos + 1;

        EndPos = strchr(CurrentPos, '\n');  /* find end of line */
        EndPos[0] = 0;
        strcpy(MPEG4P2FirmwareVersion, CurrentPos); /* Get MPEG2 firmware identification */
        strcat(MPEG4P2FirmwareVersion, "\n");
        EndPos[0] = '\n';
        CurrentPos = EndPos + 1;
        EndPos = strchr(CurrentPos, '\n');  /* find end of line */
        EndPos[0] = 0;
        strcat(MPEG4P2FirmwareVersion, CurrentPos); /* Get MPEG2 IPF firmware identification */
        EndPos[0] = '\n';
        CurrentPos = EndPos + 1;

        switch(DeviceType)
        {
            case    STVID_DEVICE_TYPE_7109D_MPEG :
            case    STVID_DEVICE_TYPE_ZEUS_MPEG :
                strcpy(FirmwareRelease, MPEG2FirmwareVersion);
                break;
            case    STVID_DEVICE_TYPE_7100_H264 :
            case    STVID_DEVICE_TYPE_7109_H264 :
            case    STVID_DEVICE_TYPE_ZEUS_H264 :
                strcpy(FirmwareRelease, H264FirmwareVersion);
                break;
            case    STVID_DEVICE_TYPE_7100_MPEG4P2 :
            case    STVID_DEVICE_TYPE_7109_MPEG4P2 :
                strcpy(FirmwareRelease, MPEG4P2FirmwareVersion);
                break;
            case    STVID_DEVICE_TYPE_7109_VC1 :
            case    STVID_DEVICE_TYPE_ZEUS_VC1 :
                strcpy(FirmwareRelease, VC1FirmwareVersion);
                break;
            case    STVID_DEVICE_TYPE_5508_MPEG :
            case    STVID_DEVICE_TYPE_5510_MPEG :
            case    STVID_DEVICE_TYPE_5512_MPEG :
            case    STVID_DEVICE_TYPE_5514_MPEG :
            case    STVID_DEVICE_TYPE_5516_MPEG :
            case    STVID_DEVICE_TYPE_5517_MPEG :
            case    STVID_DEVICE_TYPE_5518_MPEG :
            case    STVID_DEVICE_TYPE_7015_MPEG :
            case    STVID_DEVICE_TYPE_7020_MPEG :
            case    STVID_DEVICE_TYPE_5528_MPEG :
            case    STVID_DEVICE_TYPE_5100_MPEG :
/*            case    STVID_DEVICE_TYPE_5107_MPEG : Same as previous */
            case    STVID_DEVICE_TYPE_5525_MPEG :
            case    STVID_DEVICE_TYPE_7710_MPEG :
            case    STVID_DEVICE_TYPE_5105_MPEG :
/*            case    STVID_DEVICE_TYPE_5188_MPEG : Same as previous */
            case    STVID_DEVICE_TYPE_STD2000_MPEG :
            case    STVID_DEVICE_TYPE_5301_MPEG :
            case    STVID_DEVICE_TYPE_7100_MPEG :
            case    STVID_DEVICE_TYPE_7109_MPEG :
            default :
                strcpy(FirmwareRelease, "Firmware Release N/A");
                break;
        }
    }
#endif  /* End of if not defined(ST_OSLINUX) */
} /* End of GetFirwareReleaseInfo() function */

/* -------------------------------------------------------------------------- */
/* VID_StoreTestParamsToCSVFile()                                             */
/* -------------------------------------------------------------------------- */
static ST_ErrorCode_t  VID_StoreTestParamsToCSVFile(char *CsvResultFileName, U32 InjectNumber)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    U32             ResultFPos;
    char           *TmpStringPtr;
    char            TmpString[256];
    char            ResultString[1024];
    char            FirmwareRelease[256];
    char            NetlistVersion[256];
    char            CutAndNetlistVersion[256];
    char            CutId[256];
    char            StreamName[256];
    char            StreamCategory[256];
    U32             IHSize;
    U32             IVSize;
    char            DateString[256];
#ifdef USE_OSPLUS
    void           *ResultFp;
#else /* USE_OSPLUS */
    FILE           *ResultFp;
#endif /* not USE_OSPLUS */

#ifdef USE_OSPLUS
    if ((ResultFp = SYS_FOpen(CsvResultFileName, "ab")) == NULL)
#else /* USE_OSPLUS */
    if ((ResultFp = fopen(CsvResultFileName, "ab")) == NULL)
#endif /* not USE_OSPLUS */
    {
        STTBX_ErrorPrint(( "Error opening file %s\n", CsvResultFileName));
        return(ST_ERROR_BAD_PARAMETER);
    }

    STTST_EvaluateString("STREAM_CATEGORY", TmpString, 256);
    TmpStringPtr = strrchr(TmpString, ':');
    if(TmpStringPtr != NULL)
    {
        TmpStringPtr++;
        strcpy(StreamCategory, TmpStringPtr);
    }
    else
    {
        strcpy(StreamCategory, TmpString);
    }
    STTST_EvaluateString("STREAM_NAME", TmpString, 256);
    TmpStringPtr = strrchr(TmpString, '/');
    if(TmpStringPtr != NULL)
    {
        TmpStringPtr++;
        strcpy(StreamName, TmpStringPtr);
    }
    else
    {
        strcpy(StreamName, TmpString);
    }
    STTST_EvaluateInteger("IHSIZE",(S32*)&IHSize,0);
    STTST_EvaluateInteger("IVSIZE",(S32*)&IVSize,0);
    GetFirwareReleaseInfo(VID_CRC[InjectNumber].DeviceType, VID_CRC[InjectNumber].BaseAddress_p, FirmwareRelease);
    /*            strcpy(FirmwareRelease, "H264 Firmware H264.1.8.3 + TLB miss partial fix + scaling lists fix"); */
    STTST_EvaluateString("API_CUTID", CutId, 256);
    STTST_EvaluateString("VVID_DELTA_VERSION", NetlistVersion, 256);
#ifdef ST_7100
    strcpy(TmpString, "STi7100 cut");
    if(!strcmp(CutId, "1.0"))
    {
        sprintf(CutAndNetlistVersion, "%s%s/deltaphi", TmpString, CutId);
    }
    else if(!strcmp(CutId, "1.1"))
    {
        sprintf(CutAndNetlistVersion, "%s%s/deltaphi", TmpString, CutId);
    }
    else if(!strcmp(CutId, "1.2"))
    {
        sprintf(CutAndNetlistVersion, "%s%s/deltaphi", TmpString, CutId);
    }
    else if(!strcmp(CutId, "1.3"))
    {
        sprintf(CutAndNetlistVersion, "%s%s/deltaphi", TmpString, CutId);
    }
    else if(!strcmp(CutId, "2.0"))
    {
        sprintf(CutAndNetlistVersion, "%s%s/deltamu blue 2.191_10", TmpString, CutId);
    }
    else if(!strcmp(CutId, "3.0"))
    {
        sprintf(CutAndNetlistVersion, "%s%s/deltamu blue 2.191_10", TmpString, CutId);
    }
    else if(!strcmp(CutId, "3.1"))
    {
        sprintf(CutAndNetlistVersion, "%s%s/deltamu blue 2.191_12", TmpString, CutId);
    }
    else if(!strcmp(CutId, "3.2"))
    {
        sprintf(CutAndNetlistVersion, "%s%s/deltamu blue 2.191_12", TmpString, CutId);
    }
    else if(!strcmp(CutId, "3.3"))
    {
        sprintf(CutAndNetlistVersion, "%s%s/deltamu blue 2.191_12", TmpString, CutId);
    }
    else if(!strcmp(CutId, "3.4"))
    {
        sprintf(CutAndNetlistVersion, "%s%s/deltamu blue 2.191_12", TmpString, CutId);
    }
    else if(!strcmp(CutId, "3.5"))
    {
        sprintf(CutAndNetlistVersion, "%s%s/deltamu blue 2.191_12", TmpString, CutId);
    }
    else if(!strcmp(CutId, "3.6"))
    {
        sprintf(CutAndNetlistVersion, "%s%s/deltamu blue 2.191_12", TmpString, CutId);
    }
    else if(!strcmp(CutId, "3.7"))
    {
        sprintf(CutAndNetlistVersion, "%s%s/deltamu blue 2.191_12", TmpString, CutId);
    }
    else
    {
        sprintf(CutAndNetlistVersion, "%s%s/delta???", TmpString, CutId);
    }
#elif defined(ST_7109)
    strcpy(TmpString, "STi7109 cut");
    if(!strcmp(CutId, "1.0"))
    {
        sprintf(CutAndNetlistVersion, "%s%s/deltamu blue 2.211_2", TmpString, CutId);
    }
    else if(!strcmp(CutId, "1.1"))
    {
        sprintf(CutAndNetlistVersion, "%s%s/deltamu blue 2.211_2", TmpString, CutId);
    }
    else if (!strcmp(CutId, "2.0"))
    {
        sprintf(CutAndNetlistVersion, "%s%s/deltamu green 1.440", TmpString, CutId);
    }
    else if (!strcmp(CutId, "3.0"))
    {
        sprintf(CutAndNetlistVersion, "%s%s/deltamu green 2.155", TmpString, CutId);
    }
    else if (!strcmp(CutId, "3.1"))
    {
        sprintf(CutAndNetlistVersion, "%s%s/deltamu green 2.155", TmpString, CutId);
    }
    else
    {
        sprintf(CutAndNetlistVersion, "%s%s/delta???", TmpString, CutId);
    }
#elif defined(ST_DELTAPHI_HE)
    strcpy(TmpString, "DeltaphiHE");
    sprintf(CutAndNetlistVersion, "%s/%s", TmpString, NetlistVersion);
#elif defined(ST_DELTAMU_HE)
#if defined(ST_DELTAMU_GREEN_HE)
    strcpy(TmpString, "DeltamuGreenHE");
    sprintf(CutAndNetlistVersion, "%s/%s", TmpString, NetlistVersion);
#else /* defined(ST_DELTAMU_GREEN_HE) */
    strcpy(TmpString, "DeltamuBlueHE");
    sprintf(CutAndNetlistVersion, "%s/%s", TmpString, NetlistVersion);
#endif /* not defined(ST_DELTAMU_GREEN_HE) */
#endif /* Platform selection */
    STTST_EvaluateString("VVID_CURRENT_DATE", DateString, 256);

#ifdef USE_OSPLUS
    ResultFPos = SYS_FTell(ResultFp);
#else /* USE_OSPLUS */
    ResultFPos = ftell(ResultFp);
#endif /* not USE_OSPLUS */

    sprintf(ResultString, "%s%s,%s,%d,%d,\"%s\n%s\",%s",
            (ResultFPos == 0 ? "" : "\n"),
            StreamCategory,
            StreamName,
            IHSize,
            IVSize,
            FirmwareRelease,
            CutAndNetlistVersion,
            DateString);
#ifdef USE_OSPLUS
    SYS_FWrite(ResultString, 1, strlen(ResultString), ResultFp);
#else /* USE_OSPLUS */
    fwrite(ResultString, 1, strlen(ResultString), ResultFp);
#endif /* not USE_OSPLUS */
#ifdef USE_OSPLUS
    SYS_FClose(ResultFp);
#else /* USE_OSPLUS */
    fclose(ResultFp);
#endif /* not USE_OSPLUS */

    return(ErrorCode);
} /* End of VID_StoreTestParamsToCSVFile() function */

/*******************************************************************************
Name        : VID_ReportTestStatistics
Description : Write test statistics to file
Parameters  : InjectNumber, Statistics from STVID driver
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void VID_ReportTestStatistics(U32 InjectNumber, char *CsvResultFileName, STVID_Statistics_t *Statistics, BOOL CRCChecked)
{
#ifdef USE_OSPLUS
    void               *ResultFp;
#else /* USE_OSPLUS */
    FILE               *ResultFp;
#endif /* not USE_OSPLUS */
    char                TmpString[256];
    char                ResultString[2048];
    BOOL                CommentStringStarted;
    char                CodecName[64];
    BOOL                IsH264;
    BOOL                IsVC1;
    BOOL                IsMPEG_DELTA;
    U32                 AveragePreprocTime;
    U32                 AverageDecodeTime;
#ifdef STVID_VALID_MULTICOM_PROFILING
    U32                 AverageHostSideLXDecodeTime;
    U32                 AverageLXSideLXDecodeTime;
#endif
    BOOL                NotAllPicturesChecked;
    BOOL                CRCErrorPresent;
    BOOL                CRCWarningPresent;
    BOOL                DecodeGNBvd42696ErrorsPresent;
    BOOL                IsMaxPreprocessTimeExceeded;
    BOOL                IsMaxLXDecodeTimeExceeded;
    BOOL                IsMaxHostDecodeTimeExceeded;
    BOOL                IsMaxLX_HostDecodeTimeOverHeadExceeded;
    BOOL                IsDecodeTimeoutPresent;
    BOOL                IsDisplayQLockPresent;
    BOOL                IsProfileErrorPresent;
    BOOL                IsPictureToLateErrorPresent;
    BOOL                IsParserErrorPresent;
    BOOL                IsPreprocErrorPresent;
    BOOL                IsFirmwareErrorPresent;

/* Cehck error conditions and set booleans accordingly */
    STTST_EvaluateString("VID1_CODEC", CodecName, 64);
    IsH264 = (strcmp(CodecName, "H264") == 0);
    IsVC1 = (strcmp(CodecName, "VC1") == 0);
    IsMPEG_DELTA = (strcmp(CodecName, "DMPEG") == 0);

    if(CRCChecked)
    {
        NotAllPicturesChecked = (VID_CRC[InjectNumber].CRCCheckResult.NbRefCRCChecked < VID_CRC[InjectNumber].NbPictureInRefCRC);

        CRCErrorPresent = (VID_CRC[InjectNumber].CRCCheckResult.NbErrors != 0);
        CRCWarningPresent = (VID_CRC[InjectNumber].CRCCheckResult.NbErrorsAndWarnings != VID_CRC[InjectNumber].CRCCheckResult.NbErrors);
    }
    else
    {
        NotAllPicturesChecked = FALSE;
        CRCErrorPresent = FALSE;
        CRCWarningPresent = FALSE;
    }

    DecodeGNBvd42696ErrorsPresent = ((Statistics->DecodeGNBvd42696Error != 0) &&
                                     IsH264);

    IsMaxPreprocessTimeExceeded = FALSE;
#if !defined(ST_DELTAPHI_HE) && !defined(ST_DELTAMU_HE)
    IsMaxPreprocessTimeExceeded = ((Statistics->DecodeTimeMaxPreprocessTime > MAX_ALLOWED_PREPROC_TIME) &&
                                   IsH264);
#endif /* !defined(ST_DELTAPHI_HE) && !defined(ST_DELTAMU_HE) */

    IsMaxLXDecodeTimeExceeded = FALSE;
#if !defined(ST_DELTAPHI_HE) && !defined(ST_DELTAMU_HE) && defined(STVID_VALID_MEASURE_TIMING)
    IsMaxLXDecodeTimeExceeded = ((Statistics->DecodeTimeMaxLXDecodeTime > MAX_ALLOWED_LX_DECODE_TIME) &&
                                    IsH264);
#endif /* !defined(ST_DELTAPHI_HE) && !defined(ST_DELTAMU_HE) && defined(STVID_VALID_MEASURE_TIMING) */

    IsMaxHostDecodeTimeExceeded = FALSE;
#if !defined(ST_DELTAPHI_HE) && !defined(ST_DELTAMU_HE)
    IsMaxHostDecodeTimeExceeded = (Statistics->DecodeTimeMaxDecodeTime > MAX_ALLOWED_DECODE_TIME);
#endif /* !defined(ST_DELTAPHI_HE) && !defined(ST_DELTAMU_HE) */

    IsMaxLX_HostDecodeTimeOverHeadExceeded = FALSE;
#if !defined(ST_DELTAPHI_HE) && !defined(ST_DELTAMU_HE) && defined(STVID_VALID_MEASURE_TIMING)
    IsMaxLX_HostDecodeTimeOverHeadExceeded = (((Statistics->DecodeTimeMaxDecodeTime - Statistics->DecodeTimeMaxLXDecodeTime) > MAX_LX_HOST_OVERHEAD) &&
                                    IsH264);
#endif /* !defined(ST_DELTAPHI_HE) && !defined(ST_DELTAMU_HE) && defined(STVID_VALID_MEASURE_TIMING) */

    IsDecodeTimeoutPresent = (Statistics->DecodePbDecodeTimeOutError != 0);

    IsDisplayQLockPresent = (Statistics->DisplayPbQueueLockedByLackOfPicture != 0);

    IsProfileErrorPresent = (Statistics->DecodePbSequenceNotInMemProfileSkipped != 0);

    IsPictureToLateErrorPresent = (Statistics->DisplayPbPictureTooLateRejectedByMain != 0);

    IsParserErrorPresent = (Statistics->DecodePbParserError != 0);

    IsPreprocErrorPresent = ((Statistics->DecodePbPreprocError != 0) &&
                              IsH264);

    IsFirmwareErrorPresent = (Statistics->DecodePbFirmwareError != 0);

#ifdef USE_OSPLUS
    if ((ResultFp = SYS_FOpen(CsvResultFileName, "ab")) == NULL)
#else /* USE_OSPLUS */
    if ((ResultFp = fopen(CsvResultFileName, "ab")) == NULL)
#endif /* not USE_OSPLUS */
    {
        STTBX_Print(("Error opening file %s\n", CsvResultFileName));
    }
    else
    {
        if(CRCErrorPresent ||
           NotAllPicturesChecked ||
           DecodeGNBvd42696ErrorsPresent ||
           ((IsMaxPreprocessTimeExceeded ||
             IsMaxLXDecodeTimeExceeded ||
             IsMaxHostDecodeTimeExceeded) &&
            IsDisplayQLockPresent) ||
           IsProfileErrorPresent ||
           IsPictureToLateErrorPresent ||
           IsParserErrorPresent ||
           IsPreprocErrorPresent ||
           IsFirmwareErrorPresent)
        {
            strcpy(ResultString, ",N");
        }
        else
        {
            strcpy(ResultString, ",Y");
        }

        if(DecodeGNBvd42696ErrorsPresent)
        {  /* If GNBvd42696 detected, don't care about other errors */
            strcat(ResultString, ",GNBvd42696,\"");
        }
        else if(IsParserErrorPresent ||
                IsPreprocErrorPresent ||
                IsFirmwareErrorPresent)
        {  /* then if decode errors, don't care about other errors */
            strcat(ResultString, ",DECODE ERR,\"");
        }
        else if(IsProfileErrorPresent)
        {  /* then if profile error, don't care about other errors */
            strcat(ResultString, ",PROFILE,\"");
        }
        else if(IsPictureToLateErrorPresent)
        {  /* then if pictures rejected by display, don't care about other errors */
            strcat(ResultString, ",PICT TOO LATE,\"");
        }
        else if(NotAllPicturesChecked)
        {  /* Everything is OK but there are not checked pictures (should never happen) */
            strcat(ResultString, ",NOT FULLY CHECKED,\"");
        }
        else if(CRCErrorPresent)
        {  /* There are only CRC errors */
            strcat(ResultString, ",CRC,\"");
        }
        else if((IsMaxPreprocessTimeExceeded ||
                 IsMaxLXDecodeTimeExceeded ||
                 IsMaxHostDecodeTimeExceeded) &&
                 IsDisplayQLockPresent)
        {  /* There are only "too long processing time" errors */
            strcat(ResultString, ",TIMEOUT,\"");
        }
        else if(IsDisplayQLockPresent)
        {  /* There are only "queue locked by lack of picture" errors */
            strcat(ResultString, ",DISPLAY Q,\"");
        }
        else if(CRCWarningPresent)
        {  /* There are only "Inverted fields" warnings */
            strcat(ResultString, ",INV FIELDS,\"");
        }
        else if(NotAllPicturesChecked ||
                CRCErrorPresent ||
                IsDisplayQLockPresent)
        {
            strcat(ResultString, ",MULTIPLE,\"");
        }
        else
        { /* No error */
            strcat(ResultString, ",,\"");
        }
        CommentStringStarted = FALSE;
        if(NotAllPicturesChecked)
        {
            sprintf(TmpString, "%sCRC KO, NOT FULLY CHECKED, %d errors",
                               (CommentStringStarted ? "\n" : ""),
                               VID_CRC[InjectNumber].CRCCheckResult.NbErrors);
            strcat(ResultString, TmpString);
            CommentStringStarted = TRUE;
        }
        else if(CRCErrorPresent)
        {
            sprintf(TmpString, "%sCRC KO, %d errors",
                               (CommentStringStarted ? "\n" : ""),
                               VID_CRC[InjectNumber].CRCCheckResult.NbErrors);
            strcat(ResultString, TmpString);
            CommentStringStarted = TRUE;
        }
        else
        {
            if (CRCChecked)
            {
                sprintf(TmpString, "%sCRC OK",
                                   (CommentStringStarted ? "\n" : ""));
            }
            else
            {
                sprintf(TmpString, "%sCRC Not Checked (Performance Mode)",
                                   (CommentStringStarted ? "\n" : ""));
            }
            strcat(ResultString, TmpString);
            CommentStringStarted = TRUE;
        }
        if(CRCWarningPresent)
        {
            sprintf(TmpString, "%s%d Inverted fields",
                               (CommentStringStarted ? "\n" : ""),
                               VID_CRC[InjectNumber].CRCCheckResult.NbErrorsAndWarnings - VID_CRC[InjectNumber].CRCCheckResult.NbErrors);
            strcat(ResultString, TmpString);
            CommentStringStarted = TRUE;
        }
        if(DecodeGNBvd42696ErrorsPresent)
        {
            sprintf(TmpString, "%s%d GNBvd42696 errors",
                               (CommentStringStarted ? "\n" : ""),
                                Statistics->DecodeGNBvd42696Error);
            strcat(ResultString, TmpString);
            CommentStringStarted = TRUE;
        }
        if(IsParserErrorPresent)
        {
            sprintf(TmpString, "%s%d errors reported by Parser",
                              (CommentStringStarted ? "\n" : ""),
                              Statistics->DecodePbParserError);
            strcat(ResultString, TmpString);
            CommentStringStarted = TRUE;
        }
        if(IsPreprocErrorPresent)
        {
            sprintf(TmpString, "%s%d errors reported by Preprocessor",
                              (CommentStringStarted ? "\n" : ""),
                              Statistics->DecodePbPreprocError);
            strcat(ResultString, TmpString);
            CommentStringStarted = TRUE;
        }
        if(IsFirmwareErrorPresent)
        {
            sprintf(TmpString, "%s%d errors reported by Firmware",
                               (CommentStringStarted ? "\n" : ""),
                               Statistics->DecodePbFirmwareError);
            strcat(ResultString, TmpString);
            CommentStringStarted = TRUE;
        }
        if(IsProfileErrorPresent)
        {
            sprintf(TmpString, "%s%d Sequence not in memory profile",
                               (CommentStringStarted ? "\n" : ""),
                               Statistics->DecodePbSequenceNotInMemProfileSkipped);
            strcat(ResultString, TmpString);
            CommentStringStarted = TRUE;
        }
        if(IsPictureToLateErrorPresent)
        {
            sprintf(TmpString, "%s%d picture too late rejected by display queue",
                               (CommentStringStarted ? "\n" : ""),
                               Statistics->DisplayPbPictureTooLateRejectedByMain);
            strcat(ResultString, TmpString);
            CommentStringStarted = TRUE;
        }
        if(IsMaxPreprocessTimeExceeded)
        {
            sprintf(TmpString, "%sMax preproc time > %dus",
                               (CommentStringStarted ? "\n" : ""),
                               MAX_ALLOWED_PREPROC_TIME);
            strcat(ResultString, TmpString);
            CommentStringStarted = TRUE;
        }
        if(IsMaxLXDecodeTimeExceeded)
        {
            sprintf(TmpString, "%sMax LX decode time > %dus",
                               (CommentStringStarted ? "\n" : ""),
                               MAX_ALLOWED_LX_DECODE_TIME);
            strcat(ResultString, TmpString);
            CommentStringStarted = TRUE;
        }
        if(IsMaxLX_HostDecodeTimeOverHeadExceeded)
        {
            sprintf(TmpString, "%sMax LX/Host overhead > %dus",
                               (CommentStringStarted ? "\n" : ""),
                               MAX_LX_HOST_OVERHEAD);
            strcat(ResultString, TmpString);
            CommentStringStarted = TRUE;
        }
        if(IsMaxHostDecodeTimeExceeded)
        {
            sprintf(TmpString, "%sMax Host decode time > %dus",
                               (CommentStringStarted ? "\n" : ""),
                               MAX_ALLOWED_DECODE_TIME);
            strcat(ResultString, TmpString);
            CommentStringStarted = TRUE;
        }
        if(IsDecodeTimeoutPresent)
        {
            sprintf(TmpString, "%s%d Decode timeout",
                               (CommentStringStarted ? "\n" : ""),
                               Statistics->DecodePbDecodeTimeOutError);
            strcat(ResultString, TmpString);
            CommentStringStarted = TRUE;
        }
        if(IsDisplayQLockPresent)
        {
            sprintf(TmpString, "%s%d display queue lock by lack of picture",
                               (CommentStringStarted ? "\n" : ""),
                               Statistics->DisplayPbQueueLockedByLackOfPicture);
            strcat(ResultString, TmpString);
            CommentStringStarted = TRUE;
        }
        if(IsH264)
        {
            if( Statistics->DecodeTimeNbPictures != 0)
                AveragePreprocTime = Statistics->DecodeTimeSumOfPreprocessTime / Statistics->DecodeTimeNbPictures;
        }

        if(Statistics->DecodeTimeNbPictures != 0)
            AverageDecodeTime = Statistics->DecodeTimeSumOfDecodeTime / Statistics->DecodeTimeNbPictures;

#ifdef STVID_VALID_MEASURE_TIMING
        if(IsH264)
        {
            AverageLXDecodeTime = Statistics->DecodeTimeSumOfLXDecodeTime / Statistics->DecodeTimeNbPictures;
#ifdef STVID_VALID_MULTICOM_PROFILING
            AverageHostSideLXDecodeTime = Statistics->DecodeTimeSumOfHostSideLXDecodeTime / Statistics->DecodeTimeNbPictures;
            AverageLXSideLXDecodeTime = Statistics->DecodeTimeSumOfLXSideLXDecodeTime / Statistics->DecodeTimeNbPictures;
#endif
        }
#endif /* STVID_VALID_MEASURE_TIMING */
        if(CRCChecked)
        {
            sprintf(TmpString, "\n%d CRC in stream\n%d CRC checked",
                               VID_CRC[InjectNumber].NbPictureInRefCRC,
                               VID_CRC[InjectNumber].CRCCheckResult.NbRefCRCChecked);
            strcat(ResultString, TmpString);
        }

        sprintf(TmpString, "\nNb decoded pictures %d",
                           Statistics->DecodeTimeNbPictures);
        strcat(ResultString, TmpString);

        if(IsH264)
        {
            sprintf(TmpString, "\nMax preproc time=%u us\n"\
                               "Avg preproc time=%u us",
                               Statistics->DecodeTimeMaxPreprocessTime,
                               AveragePreprocTime);
            strcat(ResultString, TmpString);
        }
        sprintf(TmpString, "\nHost Max decode  time=%u us\n"\
                           "Host Avg decode  time=%u us",
                           Statistics->DecodeTimeMaxDecodeTime,
                           AverageDecodeTime);
        strcat(ResultString, TmpString);

#ifdef STVID_VALID_MEASURE_TIMING
        if(IsH264)
        {
            sprintf(TmpString, "\nLX Max decode time=%u us\n"\
                               "LX Avg decode time=%u us",
                               Statistics->DecodeTimeMaxLXDecodeTime,
                               AverageLXDecodeTime);
            strcat(ResultString, TmpString);
#ifdef STVID_VALID_MULTICOM_PROFILING
            sprintf(TmpString, "\nHostSideLX Max decode time=%u us\n"\
                               "HostSideLX Avg decode time=%u us\n"\
                               "LXSideLX Max decode time=%u us\n"\
                               "LXSideLX Avg decode time=%u us",
                               Statistics->DecodeTimeMaxHostSideLXDecodeTime,
                               AverageHostSideLXDecodeTime,
                               Statistics->DecodeTimeMaxLXSideLXDecodeTime,
                               AverageLXSideLXDecodeTime);
            strcat(ResultString, TmpString);
#endif /* STVID_VALID_MULTICOM_PROFILING */
        }
#endif /* STVID_VALID_MEASURE_TIMING */

       strcat(ResultString, "\"");

#ifdef USE_OSPLUS
        SYS_FWrite(ResultString, 1, strlen(ResultString), ResultFp);
#else /* USE_OSPLUS */
        fwrite(ResultString, 1, strlen(ResultString), ResultFp);
#endif /* not USE_OSPLUS */
#ifdef USE_OSPLUS
        SYS_FClose(ResultFp);
#else /* USE_OSPLUS */
        fclose(ResultFp);
#endif /* not USE_OSPLUS */
    }
} /* End of VID_ReportTestStatistics() function */

/*-------------------------------------------------------------------------
 * Function : VVID_LogTestToCSVFile
 * Input    :
 * Parameter:
 *
 * Output   :
 * Return   : FALSE if no error
 * ----------------------------------------------------------------------*/
static BOOL VVID_LogTestToCSVFile(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL            RetErr;
    STVID_Handle_t  CurrentHandle;
    S32             LVar;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    char            CsvResultFileName[FILENAME_MAX];
    U32             InjectNumber;

    UNUSED_PARAMETER(result_sym_p);

    strcpy(VID_Msg, "");

    /* get Handle */

    RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
    CurrentHandle = (STVID_Handle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected Handle" );
        return (TRUE);
    }
    else
    {
        DefaultVideoHandle = CurrentHandle;
    }
    InjectNumber = VID_GetSTVIDNumberFromHandle(CurrentHandle);

    /* get CSV Summary result File name (mandatory) */
    memset(CsvResultFileName, 0, sizeof(CsvResultFileName));
    RetErr = STTST_GetString( pars_p, "", CsvResultFileName, sizeof(CsvResultFileName) );
    if ( RetErr ||
        (CsvResultFileName[0] == 0))
    {
        STTST_TagCurrentLine( pars_p, "expected CSV Summary Result File Name" );
        return(TRUE);
    }

    /* get Error log File name (optionnal) */
    memset(VID_CRC[InjectNumber].LogFileName, 0, sizeof(VID_CRC[InjectNumber].LogFileName));
    RetErr = STTST_GetString( pars_p, "", VID_CRC[InjectNumber].LogFileName, sizeof(VID_CRC[InjectNumber].LogFileName) );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Log File Name" );
        return(TRUE);
    }

#ifdef VALID_TOOLS
    if (MSGLOG_InitLog(STVID_MAX_LOG_FILE_SIZE, VID_CRC[InjectNumber].LogFileName, TRUE, &(VID_CRC[InjectNumber].MSGLOG_Handle)) != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unable to initialize result log '%s' ***\n", VID_CRC[InjectNumber].LogFileName));
    }
    ErrorCode = STVID_SetMSGLOGHandle(CurrentHandle, VID_CRC[InjectNumber].MSGLOG_Handle);
#endif /* VALID_TOOLS */

    ErrorCode = VID_StoreTestParamsToCSVFile(CsvResultFileName, InjectNumber);
    switch (ErrorCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(VID_Msg, "ok\n" );
            break;
        case ST_ERROR_INVALID_HANDLE:
            strcat(VID_Msg, "handle is invalid !\n" );
            break;
        case ST_ERROR_BAD_PARAMETER:
            strcat(VID_Msg, "bad parameter !\n" );
            break;
        default:
            sprintf(VID_Msg, "%sunexpected error [0x%x] !\n", VID_Msg, RetErr );
            break;
    } /* end switch */
    STTBX_Print((VID_Msg));

    strcpy(VID_Msg, "");

    return(RetErr);
}/* End of VVID_LogTestToCSVFile function */

/*-------------------------------------------------------------------------
 * Function : VVID_ReportTestStatistics
 * Input    :
 * Parameter:
 *
 * Output   :
 * Return   : FALSE if no error
 * ----------------------------------------------------------------------*/
static BOOL VVID_ReportTestStatistics(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL                      RetErr;
    STVID_Handle_t       CurrentHandle;
    S32                         LVar;
    U32                         InjectNumber;
    STVID_Statistics_t  Statistics;
    char                CsvResultFileName[FILENAME_MAX];
    char                DecTimeFileName[FILENAME_MAX];
#ifdef STVID_VALID_DEC_TIME
    char                CodecName[64];
    BOOL               IsH264;
    BOOL               IsVC1;
    BOOL               IsMPEG_DELTA;
#endif
    ST_DeviceName_t     DeviceName;

    UNUSED_PARAMETER(result_sym_p);

    /* get Handle */
    RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
    CurrentHandle = (STVID_Handle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected Handle" );
        return (TRUE);
    }
    else
    {
        DefaultVideoHandle = CurrentHandle;
    }
    InjectNumber = VID_GetSTVIDNumberFromHandle(CurrentHandle);

    /* get CSV Summary result File name (mandatory) */
    memset(CsvResultFileName, 0, sizeof(CsvResultFileName));
    RetErr = STTST_GetString( pars_p, "", CsvResultFileName, sizeof(CsvResultFileName) );
    if ( RetErr ||
        (CsvResultFileName[0] == 0))
    {
        STTST_TagCurrentLine( pars_p, "expected CSV Summary Result File Name" );
        return(TRUE);
    }

    /* get Decode time dump result File name (optionnal) */
    memset(DecTimeFileName, 0, sizeof(DecTimeFileName));
    RetErr = STTST_GetString( pars_p, "", DecTimeFileName, sizeof(DecTimeFileName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected Decode Time Result File Name" );
        return(TRUE);
    }

    STTBX_Print(("Writing Error Log to file..."));
#if defined(VALID_TOOLS)
    MSGLOG_TermLog(VID_CRC[InjectNumber].MSGLOG_Handle);
#endif
    STTBX_Print(("done\n"));

    VID_Inj_GetDeviceName(InjectNumber,DeviceName);
    STVID_GetStatistics(DeviceName, &Statistics);

    VID_ReportTestStatistics(InjectNumber, CsvResultFileName, &Statistics, FALSE);

#ifdef STVID_VALID_DEC_TIME
    STTST_EvaluateString("VID1_CODEC", CodecName, 64);
    IsH264 = (strcmp(CodecName, "H264") == 0);
    IsVC1 = (strcmp(CodecName, "VC1") == 0);
    IsMPEG_DELTA = (strcmp(CodecName, "DMPEG") == 0);

    if(DecTimeFileName[0] != 0)
    {
        STTBX_Print(("Printing Time Dump to file..."));
#if defined(ST_7109) || defined(ST_DELTAMU_HE)
        if (IsMPEG_DELTA)
        {
            VVID_MPEG2DEC_TimeDump_Command_function(DecTimeFileName, Statistics.QueuePicturePushedToDisplay);
        }
        if (IsVC1)
        {
            VVID_VC1DEC_TimeDump_Command_function(DecTimeFileName, Statistics.QueuePicturePushedToDisplay);
        }
#endif /*#if defined(ST_7109) || defined(ST_DELTAMU_HE)*/
        if (IsH264)
        {
            VVID_DEC_TimeDump_Command_function(DecTimeFileName, Statistics.QueuePicturePushedToDisplay);
        }
        STTBX_Print(("done\n"));
    }
#endif /* STVID_VALID_DEC_TIME */

    return(RetErr);
}/* End of VVID_ReportTestStatistics function */

#endif /* defined(STVID_USE_CRC) || defined(STVID_VALID) */

#ifdef STVID_USE_CRC
/* -------------------------------------------------------------------------- */
/* VID_FGets()                                                                */
/* Implement fgets for files with lines terminated by 13 (CR) only            */
/*   because this is the spec of CRC reference files                          */
/*   CAUTION !!! This code doesn't work for UNIX files which are              */
/* line terminated with 10 (LF) only                                          */
/* -------------------------------------------------------------------------- */
static char *VID_FGets(char *string, U32 n, void *fp)
{
    char    CharRead;
    uint     NbChar;
    char    *StringPtr;
    BOOL    EndOfFile;
    BOOL    EndOfLine;
    U32     CurrentPos;
    char    PreviousChar;

#ifdef USE_OSPLUS
    EndOfFile = (SYS_FEof(fp) != 0);
#else /* USE_OSPLUS */
    EndOfFile = (feof((FILE *)fp) != 0);
#endif /* not USE_OSPLUS */

    if((string != NULL) &&
       !EndOfFile &&
       (n > 1))
    {
        NbChar = 0;
        StringPtr = string;
        EndOfLine = FALSE;
        do
        {
#ifdef USE_OSPLUS
            SYS_FRead(&CharRead, 1, 1, fp);
            EndOfFile = (SYS_FEof(fp) != 0);
#else /* USE_OSPLUS */
            CharRead = getc((FILE *)fp);
            EndOfFile = (feof((FILE *)fp) != 0);
#endif /* not USE_OSPLUS */
            if((CharRead != 13) &&
               (CharRead != 10) &&
               !EndOfFile)
            {
                *StringPtr++ = CharRead;
                NbChar++;
            }
            else if((EndOfFile) ||
                    (NbChar != 0))
            {   /* If at end of file or if 10 or 13 not as first char
                   this is the end of the line */
                EndOfLine = TRUE;
            }
            else
            {   /* If first char of line is 10 or 13, check if it is part of
                   previous "end of line" or not */
#ifdef USE_OSPLUS
                CurrentPos = SYS_FTell(fp);
#else /* USE_OSPLUS */
                CurrentPos = ftell(fp);
#endif /* not USE_OSPLUS */
                if(CurrentPos >= 1)
                {
#ifdef USE_OSPLUS
                    SYS_FSeek(fp, CurrentPos-2, SEEK_SET);
                    SYS_FRead(&PreviousChar, 1, 1, fp);
#else /* USE_OSPLUS */
                    fseek(fp, CurrentPos-2, SEEK_SET);
                    fread(&PreviousChar, 1, 1, fp);
#endif /* not USE_OSPLUS */
                    if(!(((PreviousChar == 10) && (CharRead == 13)) ||
                        ((PreviousChar == 13) && (CharRead == 10))))
                    {   /* If this 2 chars sequence is not 10/13 or 13/10,
                           they are not part of the same "end of line"
                           so this is a new line */
                        EndOfLine = TRUE;
                    }
#ifdef USE_OSPLUS
                    SYS_FSeek(fp, CurrentPos, SEEK_SET);
#else /* USE_OSPLUS */
                    fseek(fp, CurrentPos, SEEK_SET);
#endif /* not USE_OSPLUS */
                }
            }
        } while((NbChar < n-1) &&
                !EndOfLine);
        if(NbChar == 0)
        {
            return(NULL);
        }
        else
        {
            *StringPtr = 0;
            return(string);
        }
    }
    else
    {
        if((string !=  NULL) &&
           !EndOfFile)
        {
            *string = 0;
        }
        return(NULL);
    }
} /* End of VID_Gets function */

/* -------------------------------------------------------------------------- */
/* VID_ReadRefCRCLine()                                                       */
/*      Reads a line from Reference CRC file                                  */
/*          Removes empty line                                                */
/*          Removes non numerical character at start of line                  */
/* -------------------------------------------------------------------------- */
static BOOL VID_ReadRefCRCLine(FILE *RefFp, char *RefCRCLine)
{
    char    TmpLine[MAX_REFCRC_LINE];
    char    *LinePtr;
    BOOL    EndOfFile;

    do
    {
        LinePtr = VID_FGets(TmpLine, MAX_REFCRC_LINE, RefFp);
        if(LinePtr != NULL)
        {
            while(!isdigit(*LinePtr) && (*LinePtr != 0))
            {
                LinePtr++;
            }
        }
#ifdef USE_OSPLUS
        EndOfFile = (SYS_FEof(RefFp) != 0);
#else /* USE_OSPLUS */
        EndOfFile = (feof(RefFp) != 0);
#endif /* not USE_OSPLUS */
    } while(!EndOfFile && (*LinePtr == 0));
    if(LinePtr != NULL)
    {
        strcpy(RefCRCLine, LinePtr);
    }
    else
    {
        RefCRCLine[0] = 0;
    }

    return(RefCRCLine[0] != 0);
} /* End of VID_ReadRefCRCLine() function */

/* -------------------------------------------------------------------------- */
/* VID_ReadRefCRCFile()                                                    */
/* -------------------------------------------------------------------------- */
static ST_ErrorCode_t VID_ReadRefCRCFile(U32 InjectNumber, STVID_RefCRCEntry_t **RefCRCTable, U32 *NbPictureInRefCRC_p, char *RefCRCFileName)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    char            RefCRCLine[MAX_REFCRC_LINE];
    U32             RefCRCIndex;
    U32             AllocSize;
    char            FrameField;
    FILE            *RefFp;

    /*open ref file*/
#ifdef USE_OSPLUS
    if ((RefFp = SYS_FOpen(RefCRCFileName, "rb")) == NULL)
#else /* USE_OSPLUS */
    if ((RefFp = fopen(RefCRCFileName, "rb")) == NULL)
#endif /* not USE_OSPLUS */
    {
        STTBX_ErrorPrint(( "Error opening file %s\n", RefCRCFileName));
#ifdef VALID_TOOLS
        MSGLOG_LogPrint(VID_CRC[InjectNumber].MSGLOG_Handle, "Error opening file %s\n", RefCRCFileName);
#endif /* VALID_TOOLS */
        return FALSE;
    }

/* Count number of pictures in ref CRC file */
    *NbPictureInRefCRC_p = 0;
    while (VID_ReadRefCRCLine(RefFp, RefCRCLine))
    {
        (*NbPictureInRefCRC_p)++;
    }

/* Allocate Ref CRC table */
    AllocSize = ((*NbPictureInRefCRC_p) * sizeof(STVID_RefCRCEntry_t));
    *RefCRCTable = (STVID_RefCRCEntry_t *) STOS_MemoryAllocate(VID_CRC[InjectNumber].CPUPartition_p, AllocSize);
    if(*RefCRCTable == NULL)
    {
        STTBX_ErrorPrint(( "Unable to allocate memory forReference CRC table (%d bytes)\n", (*NbPictureInRefCRC_p) * sizeof(STVID_RefCRCEntry_t)));
#ifdef VALID_TOOLS
        MSGLOG_LogPrint(VID_CRC[InjectNumber].MSGLOG_Handle, "Unable to allocate memory forReference CRC table (%d bytes)\n", (*NbPictureInRefCRC_p) * sizeof(STVID_RefCRCEntry_t));
#endif /* VALID_TOOLS */
        ErrorCode = ST_ERROR_NO_MEMORY;
    }
    else
    {
/* Read Ref CRC */
#ifdef USE_OSPLUS
        SYS_FSeek(RefFp, 0, SEEK_SET);
#else /* USE_OSPLUS */
        fseek(RefFp, 0, SEEK_SET);
#endif /* not USE_OSPLUS */

        for(RefCRCIndex = 0 ; RefCRCIndex < *NbPictureInRefCRC_p ; RefCRCIndex++)
        {
            VID_ReadRefCRCLine(RefFp, RefCRCLine);
            FrameField = '\0';
            sscanf(RefCRCLine, "%u %d %u %u %c",
                    &((*RefCRCTable)[RefCRCIndex].IDExtension),
                    &((*RefCRCTable)[RefCRCIndex].ID),
                    &((*RefCRCTable)[RefCRCIndex].LumaCRC),
                    &((*RefCRCTable)[RefCRCIndex].ChromaCRC),
                    &FrameField);
                    (*RefCRCTable)[RefCRCIndex].FieldPicture = (FrameField == 'F');
        }
    }

#ifdef USE_OSPLUS
    SYS_FClose(RefFp);
#else /* USE_OSPLUS */
    fclose(RefFp);
#endif /* not USE_OSPLUS */
    return(ErrorCode);
} /* End of VID_ReadRefCRCFile() function */

/*******************************************************************************
Name        : VID_ReportCRCErrorLog
Description : Write CRC error log table to file
Parameters  : InjectNumber
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void VID_ReportCRCErrorLog(U32 InjectNumber)
{
    U32     ErrorLogIndex;
    U32     NumberOfErrorLog;
    U32     CurrentErrorLog;
#ifdef USE_OSPLUS
    void   *LogFile_p = NULL;
#else /* USE_OSPLUS */
    FILE   *LogFile_p = NULL;
#endif /* not USE_OSPLUS */
    char    TmpString[256];
    U32     NbLumaErrors;
    U32     NbChromaErrors;
    U32     NbInvertedFields;

#ifdef USE_OSPLUS
    LogFile_p = SYS_FOpen(VID_CRC[InjectNumber].LogFileName, "a");
    SYS_FSeek(LogFile_p, 0, SEEK_END);
#else /* USE_OSPLUS */
  #if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
    LogFile_p = (FILE *)Memfopen(VID_CRC[InjectNumber].LogFileName, "a");
    Memfseek(LogFile_p, 0, SEEK_END);
#else
    LogFile_p = fopen(VID_CRC[InjectNumber].LogFileName, "a");
    fseek(LogFile_p, 0, SEEK_END);
#endif /*   #if defined(ST_ZEUS) && !defined(USE_ST20_DCU) */
#endif /* not USE_OSPLUS */
    if (LogFile_p == NULL)
    {
        STTBX_ErrorPrint(( "fopen error on file '%s' ***\n", VID_CRC[InjectNumber].LogFileName));
        return;
    }

    if(VID_CRC[InjectNumber].CRCCheckResult.NbErrorsAndWarnings <= VID_CRC[InjectNumber].MaxNbPictureInErrorLog)
    {   /* Error log table doesn't wrapped around */
        ErrorLogIndex = 0;
        NumberOfErrorLog = VID_CRC[InjectNumber].CRCCheckResult.NbErrorsAndWarnings;
    }
    else
    {   /* Error log table wrapped around */
        ErrorLogIndex = VID_CRC[InjectNumber].CRCCheckResult.NbErrorsAndWarnings % VID_CRC[InjectNumber].MaxNbPictureInErrorLog;
        NumberOfErrorLog = VID_CRC[InjectNumber].MaxNbPictureInErrorLog;
    }

    NbLumaErrors = 0;
    NbChromaErrors = 0;
    NbInvertedFields = 0;
    for(CurrentErrorLog = 0 ; CurrentErrorLog < NumberOfErrorLog ; CurrentErrorLog++)
    {
        if(VID_CRC[InjectNumber].ErrorLogTable[ErrorLogIndex].InvertedFields != STVID_CRC_INVFIELD_NONE)
        {
            NbInvertedFields++;
            sprintf(TmpString,
                    "Warning on picture (DOFID=%5d) IDExtension : %3u ID : %5d Inverted field with %s\n",
                    VID_CRC[InjectNumber].ErrorLogTable[ErrorLogIndex].DecodingOrderFrameID,
                    VID_CRC[InjectNumber].ErrorLogTable[ErrorLogIndex].IDExtension,
                    VID_CRC[InjectNumber].ErrorLogTable[ErrorLogIndex].ID,
                    (VID_CRC[InjectNumber].ErrorLogTable[ErrorLogIndex].InvertedFields == STVID_CRC_INVFIELD_WITH_PREVIOUS ? "previous" : "next"));
#ifdef USE_OSPLUS
            SYS_FWrite((char *)TmpString, 1, strlen(TmpString), LogFile_p);
#else /* USE_OSPLUS */
#if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
            Memfwrite((char *)TmpString, 1, strlen(TmpString), LogFile_p);
#else
            fwrite((char *)TmpString, 1, strlen(TmpString), LogFile_p);
#endif /*   #if defined(ST_ZEUS) && !defined(USE_ST20_DCU) */
#endif /* not USE_OSPLUS */
        }
        if(VID_CRC[InjectNumber].ErrorLogTable[ErrorLogIndex].LumaError)
        {
            NbLumaErrors++;
            sprintf(TmpString,
                    "Error on picture (DOFID=%5d) IDExtension : %3u ID : %5d Actual Luma CRC   = %10u // Expected Luma CRC   = %10u\n",
                    VID_CRC[InjectNumber].ErrorLogTable[ErrorLogIndex].DecodingOrderFrameID,
                    VID_CRC[InjectNumber].ErrorLogTable[ErrorLogIndex].IDExtension,
                    VID_CRC[InjectNumber].ErrorLogTable[ErrorLogIndex].ID,
                    VID_CRC[InjectNumber].ErrorLogTable[ErrorLogIndex].LumaCRC,
                    VID_CRC[InjectNumber].RefCRCTable[VID_CRC[InjectNumber].ErrorLogTable[ErrorLogIndex].RefCRCIndex % VID_CRC[InjectNumber].NbPictureInRefCRC].LumaCRC);
#ifdef USE_OSPLUS
            SYS_FWrite((char *)TmpString, 1, strlen(TmpString), LogFile_p);
#else /* USE_OSPLUS */
#if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
            Memfwrite((char *)TmpString, 1, strlen(TmpString), LogFile_p);
#else
            fwrite((char *)TmpString, 1, strlen(TmpString), LogFile_p);
#endif /*   #if defined(ST_ZEUS) && !defined(USE_ST20_DCU) */
#endif /* not USE_OSPLUS */
        }
        if(VID_CRC[InjectNumber].ErrorLogTable[ErrorLogIndex].ChromaError)
        {
            NbChromaErrors++;
            sprintf(TmpString,
                    "Error on picture (DOFID=%5d) IDExtension : %3u ID : %5d Actual Chroma CRC = %10u // Expected Chroma CRC = %10u\n",
                    VID_CRC[InjectNumber].ErrorLogTable[ErrorLogIndex].DecodingOrderFrameID,
                    VID_CRC[InjectNumber].ErrorLogTable[ErrorLogIndex].IDExtension,
                    VID_CRC[InjectNumber].ErrorLogTable[ErrorLogIndex].ID,
                    VID_CRC[InjectNumber].ErrorLogTable[ErrorLogIndex].ChromaCRC,
                    VID_CRC[InjectNumber].RefCRCTable[VID_CRC[InjectNumber].ErrorLogTable[ErrorLogIndex].RefCRCIndex % VID_CRC[InjectNumber].NbPictureInRefCRC].ChromaCRC);
#ifdef USE_OSPLUS
            SYS_FWrite((char *)TmpString, 1, strlen(TmpString), LogFile_p);
#else /* USE_OSPLUS */
#if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
            Memfwrite((char *)TmpString, 1, strlen(TmpString), LogFile_p);
#else
            fwrite((char *)TmpString, 1, strlen(TmpString), LogFile_p);
#endif /*   #if defined(ST_ZEUS) && !defined(USE_ST20_DCU) */
#endif /* not USE_OSPLUS */
        }
        ErrorLogIndex = (ErrorLogIndex + 1) % VID_CRC[InjectNumber].MaxNbPictureInErrorLog;
    }
    sprintf(TmpString,
            "CRC Check completed: %d pictures in reference %d Pictures checked \n%d Luma Error detected\n%d Chroma Error detected\n%d Inverted fields detected\n",
             VID_CRC[InjectNumber].NbPictureInRefCRC, VID_CRC[InjectNumber].CRCCheckResult.NbRefCRCChecked,
             NbLumaErrors, NbChromaErrors, NbInvertedFields);
    STTBX_Print(("%s", TmpString));
#ifdef USE_OSPLUS
    SYS_FWrite((char *)TmpString, 1, strlen(TmpString), LogFile_p);
#else /* USE_OSPLUS */
#if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
    Memfwrite((char *)TmpString, 1, strlen(TmpString), LogFile_p);
#else
    fwrite((char *)TmpString, 1, strlen(TmpString), LogFile_p);
#endif /*   #if defined(ST_ZEUS) && !defined(USE_ST20_DCU) */
#endif /* not USE_OSPLUS */

#ifdef USE_OSPLUS
    SYS_FClose(LogFile_p);
#else /* USE_OSPLUS */
  #if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
    Memfclose(LogFile_p);
#else
    fclose(LogFile_p);
#endif /*   #if defined(ST_ZEUS) && !defined(USE_ST20_DCU) */
#endif /* not USE_OSPLUS */
} /* End of VID_ReportCRCErrorLog() function */

/*-------------------------------------------------------------------------
 * Function : DumpCRC
 * Input    :
 * Parameter:
 *
 * Output   :
 * Return   : FALSE if no error
 * ----------------------------------------------------------------------*/
static ST_ErrorCode_t DumpCRC(VID_CRC_t *VID_CRC_p, const char *DumpCRCFileName)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    U32             CompCRCIndex;
    U32             NumberOfCompCRC;
    U32             CurrentCompCRC;
#ifdef USE_OSPLUS
    void           *CrcDumpFile_p = NULL;
#else /* USE_OSPLUS */
    FILE           *CrcDumpFile_p = NULL;
#endif /* not USE_OSPLUS */
    char            TmpString[256];

#ifdef USE_OSPLUS
    CrcDumpFile_p = SYS_FOpen(DumpCRCFileName, "w");
#else /* USE_OSPLUS */
  #if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
    CrcDumpFile_p = (FILE *)Memfopen(DumpCRCFileName, "w");
#else
    CrcDumpFile_p = fopen(DumpCRCFileName, "w");
#endif /*   #if defined(ST_ZEUS) && !defined(USE_ST20_DCU) */
#endif /* not USE_OSPLUS */
    if (CrcDumpFile_p == NULL)
    {
        STTBX_ErrorPrint(( "fopen error on file '%s' ***\n", DumpCRCFileName));
        return(ST_ERROR_OPEN_HANDLE);
    }

    if(VID_CRC_p->CRCCheckResult.NbCompCRC <= VID_CRC_p->MaxNbPictureInCompCRC)
    {   /* Error log table doesn't wrapped around */
        CompCRCIndex = 0;
        NumberOfCompCRC = VID_CRC_p->CRCCheckResult.NbCompCRC;
    }
    else
    {   /* Error log table wrapped around */
        CompCRCIndex = VID_CRC_p->CRCCheckResult.NbCompCRC % VID_CRC_p->MaxNbPictureInCompCRC;
        NumberOfCompCRC = VID_CRC_p->MaxNbPictureInCompCRC;
    }

    if(VID_CRC_p->CompCRCTable == NULL)
    {
        STTBX_ErrorPrint(( "CRC checking has been stopped. Data no more available...\n"));
        return(ST_ERROR_NO_MEMORY);
    }

    for(CurrentCompCRC = 0 ; CurrentCompCRC < NumberOfCompCRC ; CurrentCompCRC++)
    {
        sprintf(TmpString, "%u %d %u %u",
                    VID_CRC_p->CompCRCTable[CompCRCIndex].IDExtension,
                    VID_CRC_p->CompCRCTable[CompCRCIndex].ID,
                    VID_CRC_p->CompCRCTable[CompCRCIndex].LumaCRC,
                    VID_CRC_p->CompCRCTable[CompCRCIndex].ChromaCRC);

        if (VID_CRC_p->CompCRCTable[CompCRCIndex].FieldCRC)
        {
           sprintf(TmpString, "%s F", TmpString);
        }
        if(VID_CRC_p->CompCRCTable[CompCRCIndex].NbRepeatedLastPicture != 0)
        {
           sprintf(TmpString, "%s REP=%d", TmpString, VID_CRC_p->CompCRCTable[CompCRCIndex].NbRepeatedLastPicture);
        }
        if(VID_CRC_p->CompCRCTable[CompCRCIndex].NbRepeatedLastButOnePicture != 0)
        {
           sprintf(TmpString, "%s REPLBO=%d", TmpString, VID_CRC_p->CompCRCTable[CompCRCIndex].NbRepeatedLastButOnePicture);
        }
        sprintf(TmpString, "%s\n", TmpString);
#ifdef USE_OSPLUS
        SYS_FWrite((char *)TmpString, 1, strlen(TmpString), CrcDumpFile_p);
#else /* USE_OSPLUS */
  #if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
        Memfwrite((char *)TmpString, 1, strlen(TmpString), CrcDumpFile_p);
#else
        fwrite((char *)TmpString, 1, strlen(TmpString), CrcDumpFile_p);
#endif /*   #if defined(ST_ZEUS) && !defined(USE_ST20_DCU) */
#endif /* not USE_OSPLUS */
        CompCRCIndex = (CompCRCIndex + 1) % VID_CRC_p->MaxNbPictureInCompCRC;
    }

#ifdef USE_OSPLUS
    SYS_FClose(CrcDumpFile_p);
#else /* USE_OSPLUS */
  #if defined(ST_ZEUS) && !defined(USE_ST20_DCU)
    Memfclose(CrcDumpFile_p);
#else
    fclose(CrcDumpFile_p);
#endif /*   #if defined(ST_ZEUS) && !defined(USE_ST20_DCU) */
#endif /* not USE_OSPLUS */

    return ErrorCode;
} /* End of DumpCRC() Function */

/*******************************************************************************
Name        : CRCCheckCleanUp
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void CRCCheckCleanUp( STVID_Handle_t VideoHandle)
{
    U32 InjectNumber;

    /* free allocated buffers */
    InjectNumber = VID_GetSTVIDNumberFromHandle(VideoHandle);
    if(VID_CRC[InjectNumber].RefCRCTable  != NULL)
    {
         STOS_MemoryDeallocate(VID_CRC[InjectNumber].CPUPartition_p,VID_CRC[InjectNumber].RefCRCTable);
         VID_CRC[InjectNumber].RefCRCTable = NULL;
    }

    if(VID_CRC[InjectNumber].CompCRCTable != NULL)
    {
         STOS_MemoryDeallocate(VID_CRC[InjectNumber].CPUPartition_p,VID_CRC[InjectNumber].CompCRCTable);
         VID_CRC[InjectNumber].CompCRCTable = NULL;
    }

    if(VID_CRC[InjectNumber].ErrorLogTable!= NULL)
    {
         STOS_MemoryDeallocate(VID_CRC[InjectNumber].CPUPartition_p,VID_CRC[InjectNumber].ErrorLogTable);
         VID_CRC[InjectNumber].ErrorLogTable = NULL;
    }
}

/*******************************************************************************
Name        : VID_CRCReportFunc
Description : Function of the CRC report task
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void VID_CRCReportFunc(const STVID_Handle_t VideoHandle)
{
    ST_ErrorCode_t      ErrCode;
    STVID_Statistics_t  Statistics;
    BOOL                AllPicturesChecked;
    char                  CodecName[64];
    BOOL                IsH264;
    BOOL                IsVC1;
    BOOL                IsMPEG_DELTA;
    STVID_Freeze_t      FreezeParams;
    U32                 InjectNumber;
    ST_DeviceName_t     DeviceName;

    STOS_TaskEnter(NULL);

    InjectNumber = VID_GetSTVIDNumberFromHandle(VideoHandle);

    VID_Inj_GetDeviceName(InjectNumber, DeviceName);
    STVID_GetStatistics(DeviceName, &Statistics);

    /* Stop decoder in order to avoid errors due to CPU locked by file accesses */
    FreezeParams.Mode  = STVID_FREEZE_MODE_FORCE;
    FreezeParams.Field = STVID_FREEZE_FIELD_TOP;
    ErrCode = STVID_Stop(VideoHandle, STVID_STOP_NOW, &FreezeParams);

#ifdef STVID_VALID_SBAG
    STSYS_WriteRegDev32LE(SBAG_MSG_FPF_DATA_1, SBAG_END_OF_TEST); /* SBAG trigger */
#endif

    STTST_EvaluateString("VID1_CODEC", CodecName, 64);
    IsH264 = (strcmp(CodecName, "H264") == 0);
    IsVC1 = (strcmp(CodecName, "VC1") == 0);
    IsMPEG_DELTA = (strcmp(CodecName, "DMPEG") == 0);

    if(VID_CRC[InjectNumber].NbPictureInRefCRC > VID_CRC[InjectNumber].CRCCheckResult.NbRefCRCChecked)
    {
        AllPicturesChecked = FALSE;
        STTBX_Print(("Warning : Picture checked(%d) < Stream's Picture Nb(%d)\n",
                    VID_CRC[InjectNumber].CRCCheckResult.NbRefCRCChecked,
                    VID_CRC[InjectNumber].NbPictureInRefCRC));
 #if defined(VALID_TOOLS)
       MSGLOG_LogPrint(VID_CRC[InjectNumber].MSGLOG_Handle,
                    "Warning : Picture checked(%d) < Stream's Picture Nb(%d)\n",
                    VID_CRC[InjectNumber].CRCCheckResult.NbRefCRCChecked,
                    VID_CRC[InjectNumber].NbPictureInRefCRC);
 #endif
    }
    else
    {
        AllPicturesChecked = TRUE;
    }

    STTBX_Print(("Writing CRC Error Log to file...\n"));
#if defined(VALID_TOOLS)
    MSGLOG_TermLog(VID_CRC[InjectNumber].MSGLOG_Handle);
#endif

    VID_ReportCRCErrorLog(InjectNumber);
    STTBX_Print(("done\n"));

    VID_ReportTestStatistics(InjectNumber, VID_CRC[InjectNumber].CsvResultFileName, &Statistics, TRUE);

    if(VID_CRC[InjectNumber].DumpFileName[0] != 0)
    {
        STTBX_Print(("Printing CRC Dump to file...\n"));
        DumpCRC(&(VID_CRC[InjectNumber]), VID_CRC[InjectNumber].DumpFileName);
        STTBX_Print(("done\n"));

#ifdef STVID_VALID_DEC_TIME
        if(VID_CRC[InjectNumber].DecTimeFileName[0] != 0)
        {
            STTBX_Print(("Printing Time Dump to file...\n"));
#if defined(ST_7109) || defined(ST_DELTAMU_HE)
            if (IsMPEG_DELTA)
            {
                VVID_MPEG2DEC_TimeDump_Command_function(VID_CRC[InjectNumber].DecTimeFileName, VID_CRC[InjectNumber].NbPictureInRefCRC);
            }
            if (IsVC1)
            {
                VVID_VC1DEC_TimeDump_Command_function(VID_CRC[InjectNumber].DecTimeFileName, VID_CRC[InjectNumber].NbPictureInRefCRC);
            }
#endif /*#if defined(ST_7109) || defined(ST_DELTAMU_HE)*/
            if (IsH264)
            {
                VVID_DEC_TimeDump_Command_function(VID_CRC[InjectNumber].DecTimeFileName, VID_CRC[InjectNumber].NbPictureInRefCRC);
            }
            STTBX_Print(("done\n"));
        }
#endif /* STVID_VALID_DEC_TIME */
    }

    /* Signal end of test to the calling script */
    STTST_AssignInteger("VVID_TOTAL_PICTURES", 1,0);

    /* clean up allocated data since they are not needed anymore */
    CRCCheckCleanUp(VideoHandle);

    STOS_TaskExit(NULL);
} /* End of VID_CRCReportFunc() function */


/*******************************************************************************
Name        : CRCEndOfCheckCB
Description : Function to subscribe the VIDCRC event STVID_END_OF_CRC_CHECK_EVT_ID
Parameters  : EVT API
Assumptions :
Limitations : Created report task is never deleted
Returns     : EVT API
*******************************************************************************/
static void CRCEndOfCheckCB(STEVT_CallReason_t Reason,
                                                        const ST_DeviceName_t RegistrantName,
                                                        STEVT_EventConstant_t Event,
                                                        const void *EventData_p,
                                                        const void *SubscriberData_p
)
{
    STVID_Handle_t VideoHandle = (STVID_Handle_t)SubscriberData_p;  /* Handle is an U32 */
    U32 InjectNumber;
    task_t * Task_p;
    tdesc_t   TaskDesc;
    void *    TaskStack;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData_p);

    InjectNumber = VID_GetSTVIDNumberFromHandle(VideoHandle);

    /* needed because for linux it copies back the kernel crc data to user space */
    STVID_CRCGetCurrentResults(VideoHandle, &(VID_CRC[InjectNumber].CRCCheckResult));

    /* Here we need to create a new task to report results because it may take some time */
    /* and the event callbacks are synchronous under OS21 so it can block STVID */
    /* KNOWN LIMITATION: The created task is never deleted so a phantom task remain after each call */
    STOS_TaskCreate ((void (*) (void*)) VID_CRCReportFunc, (void *) VideoHandle,
                                  VID_CRC[InjectNumber].CPUPartition_p, STVID_TASK_STACK_SIZE_CRC, &TaskStack,
                                  VID_CRC[InjectNumber].CPUPartition_p, &Task_p, &TaskDesc,
                                  STVID_TASK_PRIORITY_CRC,
                                  "VID_CRCReportTask",
                                  0);
    if (Task_p == NULL)
    {
        STTBX_ErrorPrint(( "CRCEndOfCheckCB unable to create report task"));
    }
} /* End of CRCEndOfCheckCB() function */

/*-------------------------------------------------------------------------
 * Function : VID_CRCStartCheck
 * Input    :
 * Parameter:
 *
 * Output   :
 * Return   : FALSE if no error
 * ----------------------------------------------------------------------*/
static BOOL VID_CRCStartCheck(STTST_Parse_t *pars_p, char *result_sym_p)
{
    STVID_Handle_t                  CurrentHandle;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    STVID_CRCStartParams_t          StartParams;
    STEVT_DeviceSubscribeParams_t   STEVT_SubscribeParams;
    BOOL                            RetErr;
    S32                             LVar;
    U32                             NbPicToCheck;
    U32                             NbStreamLoops;
    BOOL                            DumpFailingFrames;
    U32                             NbPicturesToCheck;
    U32                             InjectNumber;
    char                            RefCRCFileName[FILENAME_MAX];

    UNUSED_PARAMETER(result_sym_p);

    strcpy(VID_Msg, "");

#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
    printf("!!!! WARNING !!!!! new CRC driver has not been tested on Palladium.\nFor any issue, contact P.Chelius\n");
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "!!!! WARNING !!!!! new CRC driver has not been tested on Palladium.\nFor any issue, contact P.Chelius\n"));
#endif /* defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE) */

    /* get Handle */
    RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
    CurrentHandle = (STVID_Handle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected Handle" );
        return (TRUE);
    }
    else
    {
        DefaultVideoHandle = CurrentHandle;
    }
    InjectNumber = VID_GetSTVIDNumberFromHandle(CurrentHandle);
    VID_CRC[InjectNumber].VideoHandle = CurrentHandle;

    /* get Number of pictures to check */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar);
    if (RetErr)
    {
        sprintf(VID_Msg, "expected nbre of picture to check");
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }
    NbPicToCheck = (U32)LVar;

    /* get Number of stream loops to check */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar);
    if (RetErr)
    {
        sprintf(VID_Msg, "expected nbre of stream loops to check");
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }
    NbStreamLoops = (U32)LVar;

    /* get DumpFailingFrames enable/disable */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar);
    if (RetErr ||
        ((LVar != 0) &&
         (LVar != 1)))
    {
        sprintf(VID_Msg, "expected DumpFailingFrames 0|1");
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }
    DumpFailingFrames = (U32)LVar;

    /* get Reference CRC File name (mandatory) */
    memset(RefCRCFileName, 0, sizeof(RefCRCFileName));
    RetErr = STTST_GetString( pars_p, "", RefCRCFileName, sizeof(RefCRCFileName));
    if (RetErr ||
        (RefCRCFileName[0] == 0))
    {
        STTST_TagCurrentLine( pars_p, "expected Reference CRC File Name" );
        return(TRUE);
    }

    /* get CSV Summary result File name (mandatory) */
    memset(VID_CRC[InjectNumber].CsvResultFileName, 0, sizeof(VID_CRC[InjectNumber].CsvResultFileName));
    RetErr = STTST_GetString( pars_p, "", VID_CRC[InjectNumber].CsvResultFileName, sizeof(VID_CRC[InjectNumber].CsvResultFileName) );
    if ( RetErr ||
        (VID_CRC[InjectNumber].CsvResultFileName[0] == 0))
    {
        STTST_TagCurrentLine( pars_p, "expected CSV Summary Result File Name" );
        return(TRUE);
    }

    /* get Error log File name (optionnal) */
    memset(VID_CRC[InjectNumber].LogFileName, 0, sizeof(VID_CRC[InjectNumber].LogFileName));
    RetErr = STTST_GetString( pars_p, "", VID_CRC[InjectNumber].LogFileName, sizeof(VID_CRC[InjectNumber].LogFileName) );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Log File Name" );
        return(TRUE);
    }

    /* get CRC Values dump File name (optionnal) */
    memset(VID_CRC[InjectNumber].DumpFileName, 0, sizeof(VID_CRC[InjectNumber].DumpFileName));
    RetErr = STTST_GetString( pars_p, "", VID_CRC[InjectNumber].DumpFileName, sizeof(VID_CRC[InjectNumber].DumpFileName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected CRC Dump File Name" );
        return(TRUE);
    }

    /* get Decode times dump File name (optionnal) */
    memset(VID_CRC[InjectNumber].DecTimeFileName, 0, sizeof(VID_CRC[InjectNumber].DecTimeFileName));
    RetErr = STTST_GetString( pars_p, "", VID_CRC[InjectNumber].DecTimeFileName, sizeof(VID_CRC[InjectNumber].DecTimeFileName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected Dec Time Dump File Name" );
        return(TRUE);
    }

    /* free previously allocated data in case of multiple start check */
    CRCCheckCleanUp(CurrentHandle);

    STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)CRCEndOfCheckCB;
    STEVT_SubscribeParams.SubscriberData_p = (void *)CurrentHandle;
    ErrorCode = STEVT_SubscribeDeviceEvent(EvtSubHandle, VID_Injection[InjectNumber].Driver.DeviceName,
                    (STEVT_EventConstant_t)STVID_END_OF_CRC_CHECK_EVT, &STEVT_SubscribeParams);
    if ((ErrorCode != ST_NO_ERROR) &&
        (ErrorCode != STEVT_ERROR_ALREADY_SUBSCRIBED))
    {
        /* Error: subscribe failed, undo initialisations done */
        STTBX_ErrorPrint(( "Module Crc start: could not subscribe to STVID_END_OF_CRC_CHECK_EVT_ID !"));
        return(TRUE);
    }

    ErrorCode = VID_StoreTestParamsToCSVFile(VID_CRC[InjectNumber].CsvResultFileName, InjectNumber);
    if(ErrorCode != ST_NO_ERROR)
    {
        return(ErrorCode);
    }

#ifdef VALID_TOOLS
    if (MSGLOG_InitLog(STVID_MAX_LOG_FILE_SIZE, VID_CRC[InjectNumber].LogFileName, TRUE, &(VID_CRC[InjectNumber].MSGLOG_Handle)) != ST_NO_ERROR)
    {
        STTBX_ErrorPrint(( "Unable to initialize result log '%s' ***\n", VID_CRC[InjectNumber].LogFileName));
    }
#endif /* VALID_TOOLS */

    ErrorCode = VID_ReadRefCRCFile(InjectNumber, &(VID_CRC[InjectNumber].RefCRCTable), &(VID_CRC[InjectNumber].NbPictureInRefCRC), RefCRCFileName);
    if(ErrorCode != ST_NO_ERROR)
    {
        return(ErrorCode);
    }

    /* Compute the nimber of pictures to check */
    NbPicturesToCheck = (NbPicToCheck >= VID_CRC[InjectNumber].NbPictureInRefCRC*NbStreamLoops ? NbPicToCheck : VID_CRC[InjectNumber].NbPictureInRefCRC*NbStreamLoops);

    if(NbPicturesToCheck != 0)
    {
        STTBX_Print(("Check will be done on %d pictures\n", NbPicturesToCheck));
    }
    else
    {
        STTBX_Print(("Check will be done until stopped by user\n"));
    }
    /* Allocate table for computed CRC log */
    if(NbPicturesToCheck != 0)
    {
        VID_CRC[InjectNumber].MaxNbPictureInCompCRC = NbPicturesToCheck;
    }
    else
    {
        VID_CRC[InjectNumber].MaxNbPictureInCompCRC = VID_CRC[InjectNumber].NbPictureInRefCRC;
    }
    /* Allocated 2 times the number of pictures in order to be able to log CRC of repeated pictures
       when CRC of repeated presentation is different from first one */
    VID_CRC[InjectNumber].MaxNbPictureInCompCRC *= 2;
    VID_CRC[InjectNumber].CompCRCTable = (STVID_CompCRCEntry_t *)STOS_MemoryAllocate(VID_CRC[InjectNumber].CPUPartition_p, VID_CRC[InjectNumber].MaxNbPictureInCompCRC * sizeof(STVID_CompCRCEntry_t));

    if(VID_CRC[InjectNumber].CompCRCTable == NULL)
    {
        STTBX_ErrorPrint(( "Unable to allocate memory for Computed CRC table (%d bytes)\n", VID_CRC[InjectNumber].MaxNbPictureInCompCRC * sizeof(STVID_CompCRCEntry_t)));
#ifdef VALID_TOOLS
        MSGLOG_LogPrint(VID_CRC[InjectNumber].MSGLOG_Handle, "Unable to allocate memory for Computed CRC table (%d bytes)\n", VID_CRC[InjectNumber].MaxNbPictureInCompCRC * sizeof(STVID_CompCRCEntry_t));
#endif /* VALID_TOOLS */
        return(ST_ERROR_NO_MEMORY);
    }

    /* Allocate table for CRC error log */
    VID_CRC[InjectNumber].MaxNbPictureInErrorLog = VID_CRC[InjectNumber].MaxNbPictureInCompCRC;
    VID_CRC[InjectNumber].ErrorLogTable = (STVID_CRCErrorLogEntry_t *) STOS_MemoryAllocate(VID_CRC[InjectNumber].CPUPartition_p, VID_CRC[InjectNumber].MaxNbPictureInErrorLog * sizeof(STVID_CRCErrorLogEntry_t));
    if(VID_CRC[InjectNumber].ErrorLogTable == NULL)
    {
        STTBX_ErrorPrint(( "Unable to allocate memory for Computed CRC table (%d bytes)\n", VID_CRC[InjectNumber].MaxNbPictureInErrorLog * sizeof(STVID_CRCErrorLogEntry_t)));
#ifdef VALID_TOOLS
        MSGLOG_LogPrint(VID_CRC[InjectNumber].MSGLOG_Handle, "Unable to allocate memory for Computed CRC table (%d bytes)\n", VID_CRC[InjectNumber].MaxNbPictureInErrorLog * sizeof(STVID_CRCErrorLogEntry_t));
#endif /* VALID_TOOLS */
        return(ST_ERROR_NO_MEMORY);
    }

    StartParams.RefCRCTable      = VID_CRC[InjectNumber].RefCRCTable;
    StartParams.CompCRCTable  = VID_CRC[InjectNumber].CompCRCTable;
    StartParams.ErrorLogTable     = VID_CRC[InjectNumber].ErrorLogTable;
    StartParams.NbPictureInRefCRC       = VID_CRC[InjectNumber].NbPictureInRefCRC;
    StartParams.MaxNbPictureInCompCRC   = VID_CRC[InjectNumber].MaxNbPictureInCompCRC;
    StartParams.MaxNbPictureInErrorLog  = VID_CRC[InjectNumber].MaxNbPictureInErrorLog;
    StartParams.NbPicturesToCheck       = NbPicturesToCheck;
    StartParams.DumpFailingFrames       = DumpFailingFrames;

#ifdef VALID_TOOLS
    ErrorCode = STVID_SetMSGLOGHandle(CurrentHandle, VID_CRC[InjectNumber].MSGLOG_Handle);
#endif /* VALID_TOOLS */

    ErrorCode = STVID_CRCStartCheck(CurrentHandle, &StartParams);
    switch (ErrorCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(VID_Msg, "CRC Check started\n" );
            break;
        case ST_ERROR_INVALID_HANDLE:
            strcat(VID_Msg, "handle is invalid !\n" );
            break;
        case ST_ERROR_DEVICE_BUSY:
            strcat(VID_Msg, "Device is busy !\n" );
            break;
        default:
            sprintf(VID_Msg, "%sunexpected error [0x%x] !\n", VID_Msg, RetErr );
            break;
    } /* end switch */

    STTBX_Print((VID_Msg));
    strcpy(VID_Msg, "");

    return(RetErr);
} /* end of VID_CRCStartCheck() function */

/*-------------------------------------------------------------------------
 * Function : VID_StopCheckCRC
 * Input    :
 * Parameter:
 *
 * Output   :
 * Return   : FALSE if no error
 * ----------------------------------------------------------------------*/
static BOOL VID_CRCStopCheck(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL                 RetErr;
    STVID_Handle_t  CurrentHandle;
    S32                    LVar;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    UNUSED_PARAMETER(result_sym_p);

    sprintf(VID_Msg, "Stopping CRC check: ");

    /* get Handle */

    RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
    CurrentHandle = (STVID_Handle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected Handle" );
        return (TRUE);
    }
    else
    {
        DefaultVideoHandle = CurrentHandle;
    }

    ErrorCode = STVID_CRCStopCheck(CurrentHandle);

    switch (ErrorCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(VID_Msg, "ok\n" );
            break;
        case ST_ERROR_INVALID_HANDLE:
            strcat(VID_Msg, "handle is invalid !\n" );
            break;
        case ST_ERROR_BAD_PARAMETER:
            strcat(VID_Msg, "bad parameter !\n" );
            break;
        default:
            sprintf(VID_Msg, "%sunexpected error [0x%x] !\n", VID_Msg, ErrorCode );
            break;
    } /* end switch */
    STTBX_Print((VID_Msg));

    return(RetErr);
}/* End of VID_CRCStopCheck function */

/*-------------------------------------------------------------------------
 * Function : VID_DumpCRC
 * Input    :
 * Parameter:
 *
 * Output   :
 * Return   : FALSE if no error
 * ----------------------------------------------------------------------*/
static BOOL VID_DumpCRC(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL            RetErr = TRUE;
    S32             LVar;
    STVID_Handle_t  CurrentHandle;
    U32             InjectNumber;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    char            DumpCRCFileName[VID_MAX_STREAM_NAME];

    UNUSED_PARAMETER(result_sym_p);

    strcpy(VID_Msg, "");

    /* get Handle */

    ErrorCode = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
    CurrentHandle = (STVID_Handle_t) LVar;
    if (ErrorCode != ST_NO_ERROR)
    {
        STTST_TagCurrentLine(pars_p, "expected Handle" );
        return (TRUE);
    }
    else
    {
        DefaultVideoHandle = CurrentHandle;
    }

    ErrorCode = STTST_GetString( pars_p, "", DumpCRCFileName, sizeof(DumpCRCFileName) );
    if (ErrorCode != ST_NO_ERROR)
    {
        STTST_TagCurrentLine( pars_p, "expected CRC DumpFile Name" );
        return(TRUE);
    }

    InjectNumber = VID_GetSTVIDNumberFromHandle(CurrentHandle);

    ErrorCode = STVID_CRCGetCurrentResults(CurrentHandle, &(VID_CRC[InjectNumber].CRCCheckResult));
    if(ErrorCode != ST_NO_ERROR)
    {
        STTBX_ErrorPrint(( "Unable to get current computed CRCs"));
        return(TRUE);
    }
    ErrorCode = DumpCRC(&(VID_CRC[InjectNumber]), DumpCRCFileName);
    switch (ErrorCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(VID_Msg, "ok\n" );
            break;
        case ST_ERROR_INVALID_HANDLE:
            strcat(VID_Msg, "handle is invalid !\n" );
            break;
        case ST_ERROR_BAD_PARAMETER:
            strcat(VID_Msg, "bad parameter !\n" );
            break;
        default:
            sprintf(VID_Msg, "%sunexpected error [0x%x] !\n", VID_Msg, RetErr );
            break;
    } /* end switch */
    STTBX_Print((VID_Msg));
    strcpy(VID_Msg, "");

    return(RetErr);
} /* End of VID_DumpCRC() function */

/*******************************************************************************
Name        : CRCEndOfCheckCB
Description : Function to subscribe the VIDCRC event STVID_END_OF_CRC_CHECK_EVT_ID
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void CRCPrintValue(STEVT_CallReason_t Reason,
                                              const ST_DeviceName_t RegistrantName,
                                              STEVT_EventConstant_t Event,
                                              const void *EventData_p,
                                              const void *SubscriberData_p
)
{
    #define CRC_TRACE_PACKING_SIZE 1

    STVID_Handle_t VideoHandle = (STVID_Handle_t)SubscriberData_p;  /* Handle is an U32 */
    STVID_CRCReadMessages_t ReadCRCMessages;
    STVID_CRCDataMessage_t messages[CRC_TRACE_PACKING_SIZE];
    int i;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData_p);

    CRCCount ++;
    if(CRCCount >= CRC_TRACE_PACKING_SIZE)
    {
        ReadCRCMessages.NbToRead = CRCCount;
        ReadCRCMessages.Messages_p = messages;

        STVID_CRCGetQueue(VideoHandle, &ReadCRCMessages);

        for(i=0; i < ReadCRCMessages.NbToRead;i++)
        {
            STTBX_Print(("CRC %u : Luma=%u  Chroma=%u IsReferencePicture:%d\n", ReadCRCMessages.Messages_p[i].PictureOrderCount,
                                                                                             ReadCRCMessages.Messages_p[i].LumaCRC,
                                                                                             ReadCRCMessages.Messages_p[i].ChromaCRC,
                                                                                             ReadCRCMessages.Messages_p[i].IsReferencePicture));
        }

        CRCCount = 0;
    }
} /* End of CRCEndOfCheckCB() function */

/*-------------------------------------------------------------------------
 * Function : VID_CRCStartTrace
 * Input    :
 * Parameter:
 *
 * Output   :
 * Return   : FALSE if no error
 * ----------------------------------------------------------------------*/
static BOOL VID_CRCStartTrace(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL                 RetErr;
    STVID_Handle_t  CurrentHandle;
    S32                    LVar;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    U32                    InjectNumber;
    STEVT_DeviceSubscribeParams_t   STEVT_SubscribeParams;

    UNUSED_PARAMETER(result_sym_p);

    sprintf(VID_Msg, "Starting CRC traces: ");

    /* get Handle */

    RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
    CurrentHandle = (STVID_Handle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected Handle" );
        return (TRUE);
    }
    else
    {
        DefaultVideoHandle = CurrentHandle;
    }

    InjectNumber = VID_GetSTVIDNumberFromHandle(CurrentHandle);
    VID_CRC[InjectNumber].VideoHandle = CurrentHandle;
    CRCCount = 0;

    /* subscribe to new CRC queued event */
    STEVT_SubscribeParams.NotifyCallback = CRCPrintValue;
    STEVT_SubscribeParams.SubscriberData_p = (void *)CurrentHandle;
    ErrorCode = STEVT_SubscribeDeviceEvent(EvtSubHandle, VID_Injection[InjectNumber].Driver.DeviceName,
                       (STEVT_EventConstant_t)STVID_NEW_CRC_QUEUED_EVT, &STEVT_SubscribeParams);
    if ((ErrorCode != ST_NO_ERROR) &&
        (ErrorCode != STEVT_ERROR_ALREADY_SUBSCRIBED))
    {
        /* Error: subscribe failed, undo initialisations done */
        STTBX_ErrorPrint(( "Module Crc start: could not subscribe to STVID_NEW_CRC_QUEUED_EVT !"));
        return(TRUE);
    }

    /* start the CRC queueing */
    ErrorCode = STVID_CRCStartQueueing(CurrentHandle);
    switch (ErrorCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(VID_Msg, "ok\n" );
            break;
        case ST_ERROR_INVALID_HANDLE:
            strcat(VID_Msg, "handle is invalid !\n" );
            break;
        case ST_ERROR_BAD_PARAMETER:
            strcat(VID_Msg, "bad parameter !\n" );
            break;
        default:
            sprintf(VID_Msg, "%s unexpected error [0x%x] !\n", VID_Msg, RetErr );
            break;
    } /* end switch */
    STTBX_Print((VID_Msg));

    return(RetErr);
}/* End of VID_CRCStartTrace function */

/*-------------------------------------------------------------------------
 * Function : VID_CRCStopTrace
 * Input    :
 * Parameter:
 *
 * Output   :
 * Return   : FALSE if no error
 * ----------------------------------------------------------------------*/
static BOOL VID_CRCStopTrace(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL                 RetErr;
    STVID_Handle_t  CurrentHandle;
    S32                    LVar;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    UNUSED_PARAMETER(result_sym_p);

    sprintf(VID_Msg, "Stopping CRC traces: ");

    /* get Handle */

    RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
    CurrentHandle = (STVID_Handle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected Handle" );
        return (TRUE);
    }
    else
    {
        DefaultVideoHandle = CurrentHandle;
    }

    /* stop the CRC queueing */
    ErrorCode = STVID_CRCStopQueueing(CurrentHandle);
    switch (ErrorCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(VID_Msg, "ok\n" );
            break;
        case ST_ERROR_INVALID_HANDLE:
            strcat(VID_Msg, "handle is invalid !\n" );
            break;
        case ST_ERROR_BAD_PARAMETER:
            strcat(VID_Msg, "bad parameter !\n" );
            break;
        default:
            sprintf(VID_Msg, "%s unexpected error [0x%x] !\n", VID_Msg, ErrorCode );
            break;
    } /* end switch */
    STTBX_Print((VID_Msg));


    ErrorCode = STEVT_UnsubscribeDeviceEvent(EvtSubHandle,
                                        VID_Injection[VID_GetSTVIDNumberFromHandle(CurrentHandle)].Driver.DeviceName,
                                        (STEVT_EventConstant_t)STVID_NEW_CRC_QUEUED_EVT);
    if(ErrorCode != ST_NO_ERROR)
    {
        RetErr = FALSE;
        STTBX_Print(("Problem found while un-installing the CRC trace callback !!!\n"));
    }

    return(RetErr);
}/* End of VID_CRCStopTrace function */


#endif /* STVID_USE_CRC */

#if defined(ST_DELTAMU_HE)
/*-------------------------------------------------------------------------
 * Function : VID_SetHEDispConfig
 * Input    :
 * Parameter:
 *
 * Output   :
 * Return   : FALSE if no error
 * ----------------------------------------------------------------------*/
static BOOL VID_SetHEDispConfig(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL                            RetErr;
    S32                             LVar;
    STVID_Handle_t                  CurrentHandle;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    STVID_SetHEDispConfigParams_t   SetHEDispConfigParams;

    sprintf(VID_Msg, "");

    /* get Handle */
    RetErr = STTST_GetInteger(pars_p, (int)DefaultVideoHandle, &LVar );
    CurrentHandle = (STVID_Handle_t) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected Handle" );
        return (TRUE);
    }
    else
    {
        DefaultVideoHandle = CurrentHandle;
    }

    RetErr = STTST_GetInteger( pars_p, 0, &LVar);
    if (RetErr)
    {
        sprintf(VID_Msg, "expected \"Dump frame\" TRUE|FALSE");
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }
    if(LVar != 0)
    {
        SetHEDispConfigParams.DumpFrames = TRUE;
    }
    else
    {
        SetHEDispConfigParams.DumpFrames = FALSE;
    }

    RetErr = STTST_GetInteger( pars_p, 0, &LVar);
    if (RetErr)
    {
        sprintf(VID_Msg, "expected \"Compute CRC\" TRUE|FALSE");
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }
    if(LVar != 0)
    {
        SetHEDispConfigParams.ComputeCRC = TRUE;
    }
    else
    {
        SetHEDispConfigParams.ComputeCRC = FALSE;
    }

    RetErr = STTST_GetInteger( pars_p, 0, &LVar);
    if (RetErr)
    {
        sprintf(VID_Msg, "expected \"Use Decimated Picture\" TRUE|FALSE");
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }
    if(LVar != 0)
    {
        SetHEDispConfigParams.UseDecimatedPicture = TRUE;
    }
    else
    {
        SetHEDispConfigParams.UseDecimatedPicture = FALSE;
    }

    RetErr = STVID_SetHEDispConfig(CurrentHandle, &SetHEDispConfigParams);
    switch (RetErr)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(VID_Msg, "HEDisplay config done\n" );
            break;
        case ST_ERROR_INVALID_HANDLE:
            strcat(VID_Msg, "handle is invalid !\n" );
            break;
        default:
            sprintf(VID_Msg, "%sunexpected error [0x%x] !\n", VID_Msg, RetErr );
            break;
    } /* end switch */
    STTBX_Print((VID_Msg));
    sprintf(VID_Msg, "");

    return(RetErr);
} /* end of VID_SetHEDispConfig() function */
#endif /* defined(ST_DELTAMU_HE) */

/*-------------------------------------------------------------------------
 * Function : VID_AllocateBitBuffer
 *            Allocates bit buffer externally to the video driver, to pass it as init params to it.
 *            Can be de-allocated by VID_DeAllocateBitBuffer().
 * Input    : size of the bit buffer to allocate
 * Parameter:
 *
 * Output   :
 * Return   : FALSE if no error
 * ----------------------------------------------------------------------*/
#if !defined ST_OSLINUX
static BOOL VID_AllocateBitBuffer(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL    RetErr;
    S32     LVar, PartNumber;
    STAVMEM_AllocBlockParams_t  AvmemAllocParams;
    STAVMEM_BlockHandle_t       BufferHandle = STAVMEM_INVALID_BLOCK_HANDLE;
    void *                      BufferAddress;
    STAVMEM_FreeBlockParams_t   AvmemFreeParams;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;

    /* Check if bit buffer is already allocated */
    if (GlobalExternalBitBuffer.IsAllocated)
    {
        sprintf(VID_Msg, "Bit Buffer already allocated !");
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }

    /* Check required bit buffer size */
    RetErr = STTST_GetInteger( pars_p, 100000, &LVar);
    AvmemAllocParams.Size = (U32) LVar;
    if ( RetErr || (LVar <= 0) )
    {
        sprintf(VID_Msg, "expected bit buffer size");
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }

    /* Check partition index  */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar);
    if ( RetErr || (LVar >= MAX_PARTITIONS) )
    {
        sprintf(VID_Msg, "expected AVMEM partition index, valid values from 0 to %d", (MAX_PARTITIONS-1));
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }
    PartNumber = LVar;

    /* Prepare buffer allocation */
    AvmemAllocParams.Alignment = 256; /* OK for all platforms */
    AvmemAllocParams.PartitionHandle = AvmemPartitionHandle[PartNumber];
    AvmemAllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AvmemAllocParams.NumberOfForbiddenRanges = 0;
    AvmemAllocParams.ForbiddenRangeArray_p = NULL;
    AvmemAllocParams.NumberOfForbiddenBorders = 0;
    AvmemAllocParams.ForbiddenBorderArray_p = NULL;
    ErrorCode = STAVMEM_AllocBlock(&AvmemAllocParams, &BufferHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error handling */
        sprintf(VID_Msg, "Allocation of bit buffer in STAVMEM failed (error=0x%x) !", ErrorCode);
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }
    ErrorCode = STAVMEM_GetBlockAddress(BufferHandle, &BufferAddress);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Try to free block before leaving */
        AvmemFreeParams.PartitionHandle = AvmemPartitionHandle[PartNumber];
        STAVMEM_FreeBlock(&AvmemFreeParams, &BufferHandle);
        sprintf(VID_Msg, "Get address of bit buffer in STAVMEM failed (error=0x%x) !", ErrorCode);
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }

    /* Sucessfull allocation of the bit buffer */
    GlobalExternalBitBuffer.IsAllocated = TRUE;
    GlobalExternalBitBuffer.AvmemBlockHandle = BufferHandle;
    GlobalExternalBitBuffer.PartitionHandle = AvmemPartitionHandle[PartNumber];
    /* provide a virtual address */
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
    GlobalExternalBitBuffer.Address_p   = STAVMEM_VirtualToDevice(BufferAddress, &VirtualMapping);
    GlobalExternalBitBuffer.Size        = AvmemAllocParams.Size;

    sprintf(VID_Msg, "VID_ALLOBB(%d) : External video bit buffer allocated in AVMEM partition %d at address 0x%0x.\r\n",
            GlobalExternalBitBuffer.Size, PartNumber, (U32) GlobalExternalBitBuffer.Address_p);
    STTBX_Print((VID_Msg));

    STTST_AssignInteger("VID_BBADDRESS", (S32)GlobalExternalBitBuffer.Address_p, FALSE);
    STTST_AssignInteger("VID_BBSIZE", GlobalExternalBitBuffer.Size, FALSE);
    STTST_AssignInteger(result_sym_p, ErrorCode, FALSE);

    return(RetErr);
} /* end of VID_AllocateBitBuffer() function */
#endif /* !LINUX */

/*-------------------------------------------------------------------------
 * Function : VID_CopyInExternalBitBuffer
 *            copy memory block from source adress to external allocated bit buffer adress
 * Input    : destination address , source addresss , size of the bit buffer
 * Parameter:
 *
 * Output   :
 * Return   : FALSE if no error
 * ----------------------------------------------------------------------*/
#if !defined ST_OSLINUX
static BOOL VID_CopyInExternalBitBuffer(STTST_Parse_t *pars_p, char *result_sym_p)
{
    void * DestinationAddress_p , * SourceAddress_p ;
    BOOL RetErr;
    S32 LVar;
    U32 Size ;

    if (GlobalExternalBitBuffer.IsAllocated)
    {
        /* Retrieve parameters from globally allocated bit buffer. */
        RetErr = STTST_GetInteger(pars_p, (S32) GlobalExternalBitBuffer.Address_p, &LVar );
    }
    else
    {
        RetErr = STTST_GetInteger(pars_p, (S32)NULL, &LVar );
    }
    DestinationAddress_p = (void *) LVar;
    if (DestinationAddress_p!=NULL)
    {
        RetErr = STTST_GetInteger(pars_p, (U32) GlobalExternalBitBuffer.Size, &LVar );
        Size = (U32) LVar  ;

        RetErr = STTST_GetInteger(pars_p, (S32) (int)VID_BufferLoad[LVar-1].AlignedBuffer_p, &LVar );
        SourceAddress_p = (void *) LVar;

        if(STAVMEM_CopyBlock1D(SourceAddress_p, DestinationAddress_p, Size) == ST_NO_ERROR)
        {
            RetErr = FALSE;
        }
        else
        {
            RetErr = TRUE;
        }
    }
    else
    {
        RetErr = TRUE ;
    }
    STTST_AssignInteger(result_sym_p, RetErr, FALSE);

    return(RetErr);
}/* end of VID_CopyInExternalBitBuffer */
#endif /* !LINUX */

/*-------------------------------------------------------------------------
 * Function : VID_DeAllocateBitBuffer
 *            De-Allocates bit buffer externally to the video driver, allocated with VID_AllocateBitBuffer().
 * Input    : size of the bit buffer to allocate
 * Parameter:
 *
 * Output   :
 * Return   : FALSE if no error
 * ----------------------------------------------------------------------*/
#if !defined ST_OSLINUX
static BOOL VID_DeAllocateBitBuffer(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL    RetErr = FALSE;
    STAVMEM_FreeBlockParams_t   AvmemFreeParams;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

    /* Check if bit buffer is allocated */
    if (!(GlobalExternalBitBuffer.IsAllocated))
    {
        sprintf(VID_Msg, "Bit Buffer not allocated !");
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }

    /* Free buffer */
    AvmemFreeParams.PartitionHandle = GlobalExternalBitBuffer.PartitionHandle;
    ErrorCode = STAVMEM_FreeBlock(&AvmemFreeParams, &(GlobalExternalBitBuffer.AvmemBlockHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error handling */
        sprintf(VID_Msg, "De-allocation of bit buffer from STAVMEM failed (error=0x%x) !", ErrorCode);
        STTST_TagCurrentLine( pars_p, VID_Msg);
        return(TRUE);
    }

    /* Sucessfull de-allocation of the bit buffer */
    GlobalExternalBitBuffer.IsAllocated = FALSE;
    GlobalExternalBitBuffer.AvmemBlockHandle = STAVMEM_INVALID_BLOCK_HANDLE;
    GlobalExternalBitBuffer.Size        = 0;

    sprintf(VID_Msg, "VID_DEALLOBB() : External video bit buffer de-allocated.\r\n");
    STTST_AssignInteger(result_sym_p, ErrorCode, FALSE);
    STTBX_Print((VID_Msg));

    return(RetErr);
} /* end of VID_DeAllocateBitBuffer() function */
#endif /* !LINUX */

#if defined (mb411) || defined (mb8kg5) || defined(mb519)
/* Memory allocaton and desallocation from user space */
ST_ErrorCode_t vidutil_Allocate(STAVMEM_PartitionHandle_t PartitionHandle, U32 Size, U32 Alignment, void **Address_p, STAVMEM_BlockHandle_t *Handle_p )
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;

#ifdef ST_OSLINUX
    ErrCode = STAPIGAT_AllocData(Size, Alignment, Address_p);
#else
    STAVMEM_AllocBlockParams_t AvmemAllocParams;
    void * VirtualAddress;

    *Handle_p = STAVMEM_INVALID_BLOCK_HANDLE;
    AvmemAllocParams.PartitionHandle = PartitionHandle;
    AvmemAllocParams.Size = Size;
    AvmemAllocParams.Alignment = Alignment;
    AvmemAllocParams.AllocMode = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AvmemAllocParams.NumberOfForbiddenRanges = 0;
    AvmemAllocParams.ForbiddenRangeArray_p = NULL;
    AvmemAllocParams.NumberOfForbiddenBorders = 0;
    AvmemAllocParams.ForbiddenBorderArray_p = NULL;
    ErrCode = STAVMEM_AllocBlock(&AvmemAllocParams, Handle_p);
    if (ErrCode == ST_NO_ERROR)
    {
        ErrCode = STAVMEM_GetBlockAddress(*Handle_p, &VirtualAddress);
        *Address_p = VirtualAddress;
    }
#endif

    return (ErrCode);
}

void vidutil_Deallocate(STAVMEM_PartitionHandle_t PartitionHandle, STAVMEM_BlockHandle_t *Handle_p, void * Address_p)
{
#ifdef ST_OSLINUX
    /* To avoid warnings */
    PartitionHandle = PartitionHandle;
    Handle_p = Handle_p;

    STAPIGAT_FreeData(Address_p);
#else
    STAVMEM_FreeBlockParams_t    FreeParams;

    /* To avoid warnings */
    Address_p = Address_p;

    FreeParams.PartitionHandle = PartitionHandle;
    STAVMEM_FreeBlock(&FreeParams, Handle_p);
#endif
}

#endif

/*#######################################################################*/
/*#########################  GLOBAL FUNCTIONS  ##########################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : VID_RegisterCmd2
 *            Definition of the register commands
 * Input    :
 * Output   :
 * Return   : TRUE if success
 * ----------------------------------------------------------------------*/
BOOL VID_RegisterCmd2 (void)
{
    BOOL RetErr;

    TicksPerOneSecond = ST_GetClocksPerSecond();

    /* Macro's name, link to C-function, help message  */
    /* (by alphabetic order of macros = display order) */

    RetErr = FALSE;
#if defined(STVID_USE_CRC) || defined(STVID_VALID)
    RetErr |= STTST_RegisterCommand("VVID_READMULTISTREAM", VVID_ReadMultiStream,   "[<Handle>] Reads in the information for the next test stream and sets TESTTOOL variable accordingly");
    RetErr |= STTST_RegisterCommand("VVID_UPDATE_VVID_NUM_PIC_DISPLAY", VVID_UpdateVVID_NUM_PIC_DISPLAY, "[<Handle>] Updates the Testtool variable VVID_NUM_PIC_DISPLAY; For automatic mode");
    RetErr |= STTST_RegisterCommand("VVID_LOGTESTPARAMSTOCSV", VVID_LogTestToCSVFile, "[<Handle>] [.csv file name] <error_log_file> Write parameters of test to .csv excel file and initializes error log");
    RetErr |= STTST_RegisterCommand("VVID_REPORTTESTSTAT", VVID_ReportTestStatistics, "[<Handle>] [.csv file name] <decode time report file name> Write results of test to file");
#endif /* defined(STVID_USE_CRC) || defined(STVID_VALID) */
#ifdef STVID_USE_CRC
    RetErr |= STTST_RegisterCommand( "VID_CRCSTART",    VID_CRCStartCheck, "<Handle> <nb_pictures> <nb_loops> <dump failing frames 0|1> <CRC_reference_file> <CSV_result_file> <error_log_file> <CRC_dump_file> <DecTime_dump_file>");
    RetErr |= STTST_RegisterCommand( "VID_CRCDUMP",     VID_DumpCRC, "<Handle> <filename>");
    RetErr |= STTST_RegisterCommand( "VID_CRCSTOP",     VID_CRCStopCheck, "<Handle>");
    RetErr |= STTST_RegisterCommand( "VID_CRCTRACEON",     VID_CRCStartTrace, "<Handle>");
    RetErr |= STTST_RegisterCommand( "VID_CRCTRACEOFF",     VID_CRCStopTrace, "<Handle>");
#endif /* STVID_USE_CRC */

#if defined(ST_DELTAMU_HE)
    RetErr |= STTST_RegisterCommand( "VID_SETHEDISP",   VID_SetHEDispConfig, "<Handle> <Dump frame TRUE|FALSE> <Compute CRC TRUE|FALSE> <Use decimated frames TRUE|FALSE>");
#endif /* defined(ST_DELTAMU_HE) */

#if !defined ST_OSLINUX
    RetErr |= STTST_RegisterCommand( "VID_CPInBB",   VID_CopyInExternalBitBuffer, "<destination adress> <source adress>");
    RetErr |= STTST_RegisterCommand( "VID_ALLOBB",   VID_AllocateBitBuffer, "<size> <AVMEM partition index> Allocate external video bit buffer in the specified AVMEM partition");
#endif
    RetErr |= STTST_RegisterCommand( "VID_COPY",     VID_Copy,      "<Handle> Copy last displayed picture into memory\n\t\t\"FREE\" Free the pictures in memory");
#if !defined ST_OSLINUX
    RetErr |= STTST_RegisterCommand( "VID_DEALLOBB", VID_DeAllocateBitBuffer, "De-allocate external video bit buffer");
#endif
    RetErr |= STTST_RegisterCommand( "VID_COUNTEVT", VID_CountEvt,   "Returns counts of received events");
    RetErr |= STTST_RegisterCommand( "VID_DELEVT",   VID_DeleteEvt, "Delete events stack");
    RetErr |= STTST_RegisterCommand( "VID_EVT",      VID_EnableEvt,
            "<Evt><Flag><TimeOut> Enable 1 or all events (init+open+reg+subs)\n\t\t Flag=0 : log event and data\n\t\t Flag=1 : time measurement (TimeOut in milli-seconds)\n\t\t Flag=2 : delay control between 2 NEW_PICT \n\t\t Flag=3 : VSync control between 2 NEW_PICT \n\t\t Flag=4 : frames nb control between 2 NEW_PICT ");
#ifndef STVID_NO_DISPLAY
    RetErr |= STTST_RegisterCommand( "VID_IO",       Vid_InteractiveIO,  "<ViewportHndl> Interactive SetIoWindow");
#endif /*#ifndef STVID_NO_DISPLAY*/

#ifdef VIDEO_TEST_HARNESS_BUILD
#if !defined ST_OSLINUX
    RetErr |= STTST_RegisterCommand( "VID_LDstill",  VID_LoadStill, "Load still picture (from compiled still.h)");
    RetErr |= STTST_RegisterCommand( "VID_EraseFrameB",  VID_EraseFrameBuffer, "<BitmapAddress> Fill frame buffer with black ");
#endif
#endif /* VIDEO_TEST_HARNESS_BUILD */
    RetErr |= STTST_RegisterCommand( "VID_LOGEVTENABLE",   VID_LogEvtEnable, "enable event real time display");
    RetErr |= STTST_RegisterCommand( "VID_LOGEVTDISABLE",   VID_LogEvtDisable, "disable event real time display");
    RetErr |= STTST_RegisterCommand( "VID_LOAD",     VID_Load,      "<Stream name><Load buffer nb> Load a file in memory");
    RetErr |= STTST_RegisterCommand( "VID_NOEVT",    VID_DisableEvt,"<Evt> Disable 1 or all events (unsubs. or unsubs.+close+term)");
    RetErr |= STTST_RegisterCommand( "VID_PARAMEVT", VID_ParamsEvt, "<Video name><VTG name> Configure parameters for test event functions");
    RetErr |= STTST_RegisterCommand( "VID_PATTERN",  VID_SetPattern, "<luma/chroma> <byte1> <byte2> ... <byten>");
    RetErr |= STTST_RegisterCommand( "VID_RESTORE",  VID_RestorePicture, "<FileName> Restore an yuv frame with its picture param. previously stored");
    RetErr |= STTST_RegisterCommand( "VID_SHOWEVT",  VID_ShowEvt,   "<\"ALL\"> Show counts and received events (ALL=see also associated data)");
    RetErr |= STTST_RegisterCommand( "VID_STORE",    VID_StorePicture, "<PictNo><FileName> Store the picture buffer with its picture param.");
    RetErr |= STTST_RegisterCommand( "VID_TESTCAPA", VID_TestCapability, "Test of the capabilities");
    RetErr |= STTST_RegisterCommand( "VID_TESTNBFRAMES", VID_TestNbFrames, "Gives Frames statistics about display fluidity\n\t\t  if 'vid_evt STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT 4' done before");
    RetErr |= STTST_RegisterCommand( "VID_TESTNBVSYNC", VID_TestNbVSync, "<Trace> Gives VSync statistics about display fluidity\n\t\t  if 'vid_evt STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT 3' done before");
#ifndef STVID_VALID
    RetErr |= STTST_RegisterCommand( "VID_TESTSYNC", VID_TestSynchro, "<1=auto(default)|0=manu><video speed><VTG name><CLKRVName> Test of the synchro mechanism \n\t\t speed : used to desynchronise the video (default=99)");
#endif /* not STVID_VALID */
    RetErr |= STTST_RegisterCommand( "VID_TESTPTSINCONSISTENCY", VID_TestPTSInconsistency, "<Test Time Duration (sec)> Test the PTS are growing when playing in live \n\t\t");
    RetErr |= STTST_RegisterCommand( "VID_TESTPICPARAM", VID_TestPictureParams, "Test of the picture params");
    RetErr |= STTST_RegisterCommand( "VID_TESTSTAT", VID_TestStatistics, "<DeviceName><NbDecode><NbI><NbP><NbP> Test some decode/display statisctics");
    RetErr |= STTST_RegisterCommand( "VID_RETURNSTAT", VID_ReturnStatisticsValue, "<DeviceName><SpeedMaxPositiveDriftRequested> Return the value of the passed statisctic");
    RetErr |= STTST_RegisterCommand( "VID_TESTEVT",  VID_TestEvt,   "<Evt><Data1><Data2><Data3><Data4> Wait for an event");
    RetErr |= STTST_RegisterCommand( "VID_TESTQUIETEVT",  VID_TestQuietEvt,   "Same as VID_TESTEVT but no output trace if successful");

#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_ZEUS) || defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
    RetErr |= STTST_RegisterCommand( "VID_GETDELTATOPINFO", VID_GetDeltaLXTopInfo, " <DeviceNumber> Prints DeltaTop information (heap usage)");
    RetErr |= STTST_RegisterCommand( "VID_PINGLX", VID_PingLX, " <LXNumber> Prints LX information (heap usage, firmware version string)");
#endif /* defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_ZEUS) || defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE) */

    RetErr |= STTST_RegisterCommand( "TEST_TIMEEVT", VID_LengthTest,"Get the duration of event's occurence");
#ifndef STVID_NO_DISPLAY
    RetErr |= STTST_RegisterCommand( "TEST_NULLPT",  VID_NullPointersTest,
                                "Call API functions with null pointers (bad param. test)");
#endif /*#ifndef STVID_NO_DISPLAY*/
    RetErr |= STTST_RegisterCommand( "TEST_PICTDELAY", VID_TestPictDelay, "Gives delays statistics about display fluidity\n\t\t  if 'vid_evt STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT 2' done before");
    RetErr |= STTST_RegisterCommand( "TEST_START",VID_StartTest,
                                "Start video with time measure");
#ifdef ST_OSWINCE
	  RetErr |= STTST_RegisterCommand( "PERF",TestPerf, "measure perf");
#endif
    RetErr |= STTST_RegisterCommand("VID_TESTSTOPSTART",      VID_TestStopStart,  "[<VideoHandle>] [<ViewportHandle>]<LAYER name>\n\t\t<StopStartMode><RealTime><UpdateDisplay><StreamType><StreamID><Broadcast><DecodeOnce>\n\t\t Stop and restart immediately the decoding of the input stream");
    RetErr |= STTST_RegisterCommand("VID_GETLASTPBUFFHANDLE",      VID_GetHandleOfLastPictureBufferDisplayed,  "[<VideoHandle>] \n\t\t Returns the Picture Buffer handle of the last picture displayed");
#if defined (mb290)
    /* to allow vid_show/vid_hide tests, it is mandatory to access to only 1st range of memory */
    /* because the ST20 can't access to all of the 64Mb of memory */
    RetErr |= STTST_RegisterCommand( "AVMEM_REINIT", AVMEM_Reinit,
                                     " <TRUE|FALSE> Term & init AVMEM for all memory range (True) or only 1 (False)");
#endif

    /* Event management : */
    memset(EventID, 0, sizeof(EventID));
    memset(RcvEventStack, 0, sizeof(RcvEventStack));
    memset(RcvWrongEventStack, -1, sizeof(RcvWrongEventStack));
    memset(RcvEventCnt, 0, sizeof(RcvEventCnt));
    VSyncLog = NULL;

    /* AV buffer management : */
    NbOfCopiedPictures = 0;
    memset(CopiedPictureHandle, 0, sizeof(CopiedPictureHandle));
    memset(CopiedPictureAddress, 0, sizeof(CopiedPictureAddress));
    memset(CopiedPictureParams, 0, sizeof(CopiedPictureParams));

    /* BufferLoad : initialization done in vid_inj.c */

    /* Init for test event */
    strcpy(EventVTGDevice, STVTG_DEVICE_NAME);
    strcpy(EventVIDDevice, STVID_DEVICE_NAME1);

    /* Init for TestPictureParams */
    PictParamTestFlag_p = semaphore_create_fifo_timeout(0);

    /* Print result */
    if ( !RetErr )
    {
        sprintf( VID_Msg, "VID_RegisterCmd2() \t: ok\n");
    }
    else
    {
        sprintf( VID_Msg, "VID_RegisterCmd2() \t: failed !\n" );
    }
    STTST_Print(( VID_Msg ));

    return(!RetErr);

} /* end VID_RegisterCmd2 */

/*#######################################################################*/
