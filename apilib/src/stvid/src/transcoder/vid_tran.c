/*******************************************************************************

File name   : vid_tran.c

Description : Video Trancoder source file

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
17 Aug 2006        Created from producer module                       OD
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Define to add debug info and traces */
/* #define STVID_DEBUG_TRANSCODER */
#define TRACE_TRAN
#define TRACE_TRAN_VERBOSE

#define STTBX_REPORT
/*#endif*/

#ifdef ST_OS21
    #define FORMAT_SPEC_OSCLOCK "ll"
    #define OSCLOCK_T_MILLE     1000LL
    #define OSCLOCK_T_MILLION   1000000LL
#else
    #define FORMAT_SPEC_OSCLOCK ""
    #define OSCLOCK_T_MILLE     1000
    #define OSCLOCK_T_MILLION   1000000
#endif

/* Includes ----------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
#include <limits.h>
#include <stdio.h>
#include <string.h>
#endif

#include "stddefs.h"

#include "stvid.h"
#include "vid_com.h"
#include "vid_ctcm.h"
#include "buffers.h"

#include "transcoder.h"
#include "vid_tran.h"
#include "vid_trqueue.h"
#include "transcodec.h"

#include "sttbx.h"
#include "stevt.h"
#include "stdevice.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */
#ifdef TRACE_TRAN /* define or uncomment in ./makefile to set traces */
  #ifdef TRACE_TRAN_VERBOSE
    #define TraceVerbose(x) TraceBuffer(x)
  #else
    #define TraceVerbose(x)
  #endif
  #include "vid_trc.h"
#else
  #define TraceInfo(x)
  #define TraceBuffer(x)
  #define TraceVerbose(x)
#endif

#ifdef TRACE_UART
 #include "trace.h"
#endif

/* Dummy value that we set to check that a handle is valid, only one chance out of 2^32 to get it by accident ! */
#define VIDTRAN_VALID_HANDLE 0x01951950

/* Max time waiting for an order: 5 times per field display (TODO: update for transcoder) */
#define MAX_WAIT_ORDER_TIME (STVID_MAX_VSYNC_DURATION / STVID_DECODE_TASK_FREQUENCY_BY_VSYNC)

/* Max time to wait for a stop command */
#define EXTRA_WAIT_STOP_TIME             (10 * STVID_MAX_VSYNC_DURATION)   /* In TICKS */
#define MAX_TICKS_TO_WAIT_FOR_A_STOP     ((TRANSCODE_QUEUE_MAX_PICTURE * STVID_MAX_VSYNC_DURATION) + EXTRA_WAIT_STOP_TIME)

/* Nb of pictures to calculate transcode average speed */
#define TRANSCODE_SPEED_WINDOW           25

/* Define the following to output in PES (to be changed for dynamic control) */
#define TRANSCODE_PES_OUTPUT

/* Define masks */
#define PTS32TO30_MASK                   0x2
#define PTS32TO30_SHIFT                  30
#define PTS33_SHIFT                      3

#define PTS29TO15_MASK                   0x7FFF
#define PTS29TO15_SHIFT                  15

#define PTS14TO0_MASK                    0x7FFF
#define PTS14TO0_SHIFT                   0

#define PTSMARKERBIT                     0x01
#define PTSHEADER_0010                   0x03 /* supposed to be 0x02 but 0x03 seen in samples !!! */

/* Private Variables (static)------------------------------------------------ */
#ifdef TRANSCODE_PES_OUTPUT
static VIDTRAN_PES_Header_t TranscoderOutputPESHeader = {
/*       1, packet_start_code_prefix */
/*    0xE0, stream_id. Default */
0xE0010000, /* packet_start_code_prefix_AND_stream_id */
/*       0, PES_packet_length. Keep 0 */
/*       2, Header_10 */                        /* b10 */
/*       0, PES_scrambling_control. Not Scrambled */
/*       0, PES_priority */
/*       0, data_alignment_indicator */
/*       0, copyright (not protected)*/
/*       0, original_or_copy (copy) */
/*       0, PTS_DTS_flags */                    /* b10 */
/*       0, ESCR_flag */
/*       0, ES_rate_flag */
/*       0, DSM_trick_mode_flag */
/*       0, additional_copy_info_flag */
/*       0, PES_CRC_flag */
/*       0, PES_extension_flag */
0x00800000, /* PES_Flag */
/*       0, PES_header_data_length */
      0x00  /* PES_header_data_length */
};

static VIDTRAN_PES_Header_PTS_t TranscoderOutputPESHeaderPTS = {
      0x21, /* Header_0010 + PTS_32_30 + marker_bit1 = H_PTS_32_30_MarkerBit1*/
/*       0, PTS_29_15 */   /* PTS value to be changed */
/*       1, marker_bit2 */
    0x0100, /* PTS_29_15_marker_bit2 */
/*       0, PTS_14_0 */    /* PTS value to be changed */
/*       1, marker_bit3 */
    0x0100  /* PTS_14_0_marker_bit3 */
};
#endif /* TRANSCODE_PES_OUTPUT */

/* Private Macros ----------------------------------------------------------- */
#define MAX_VALUE(a,b)  ((a)>(b)?a:b)

/* Private Function prototypes ---------------------------------------------- */

static void InitTranscoderStructure(const VIDTRAN_Handle_t TranscoderHandle);
static ST_ErrorCode_t StartTranscoderTask(const VIDTRAN_Handle_t TranscoderHandle);
static ST_ErrorCode_t StopTranscoderTask(const VIDTRAN_Handle_t TranscoderHandle);
static void TermTranscoderStructure(const VIDTRAN_Handle_t TranscoderHandle);
static void TranscoderTaskLoop(const VIDTRAN_Handle_t TranscoderHandle);
static void TranscoderTaskFunc(const VIDTRAN_Handle_t TranscoderHandle);

static ST_ErrorCode_t vidtran_Init(const VIDTRAN_InitParams_t * const InitParams_p, VIDTRAN_Handle_t * const TranscoderHandle_p);
static ST_ErrorCode_t vidtran_Term(const VIDTRAN_Handle_t TranscoderHandle);
static ST_ErrorCode_t vidtran_Open(const VIDTRAN_Handle_t TranscoderHandle, const VIDTRAN_OpenParams_t * const OpenParams_p);
static ST_ErrorCode_t vidtran_Close(const VIDTRAN_Handle_t TranscoderHandle, const VIDTRAN_CloseParams_t * const CloseParams_p);
static ST_ErrorCode_t vidtran_Start(const VIDTRAN_Handle_t TranscoderHandle, const VIDTRAN_StartParams_t * const StartParams_p);
static ST_ErrorCode_t vidtran_Stop(const VIDTRAN_Handle_t TranscoderHandle, const VIDTRAN_StopParams_t * const StopParams_p);
static ST_ErrorCode_t vidtran_SetParams(const VIDTRAN_Handle_t TranscoderHandle, const VIDTRAN_SetParams_t * const SetParams_p);
static ST_ErrorCode_t vidtran_SetMode(const VIDTRAN_Handle_t TranscoderHandle, const VIDTRAN_SetMode_t * const SetMode_p);
static ST_ErrorCode_t vidtran_Link(const VIDTRAN_Handle_t TranscoderHandle, const STVID_TranscoderLinkParams_t * const LinkParams_p);
static ST_ErrorCode_t vidtran_Unlink(const VIDTRAN_Handle_t TranscoderHandle);
static ST_ErrorCode_t vidtran_InformReadAddress(const VIDTRAN_Handle_t TranscoderHandle, void * const Read_p);
static ST_ErrorCode_t vidtran_AllocatePESBuffer(const VIDTRAN_Handle_t TranscoderHandle, const VIDTRAN_InitParams_t * const InitParams_p);
static ST_ErrorCode_t vidtran_InitPESBuffer(const VIDTRAN_Handle_t TranscoderHandle);
static ST_ErrorCode_t vidtran_TranscodePicture(const VIDTRAN_Handle_t TranscoderHandle, const VIDTRAN_Picture_t * const TrPictureToTranscode_p);
static ST_ErrorCode_t vidtran_SetErrorRecoveryMode(const VIDTRAN_Handle_t TranscoderHandle, const VIDTRAN_ErrorRecoveryMode_t Mode);
static ST_ErrorCode_t vidtran_GetErrorRecoveryMode(const VIDTRAN_Handle_t TranscoderHandle, VIDTRAN_ErrorRecoveryMode_t * const Mode_p);

#ifdef STVID_DEBUG_GET_STATISTICS
static void InitStatisticsTranscoderStructure(const VIDTRAN_Handle_t TranscoderHandle);
static ST_ErrorCode_t vidtran_GetStatistics(const VIDTRAN_Handle_t TranscoderHandle, STVID_TranscoderStatistics_t * const Statistics_p);
static ST_ErrorCode_t vidtran_ResetAllStatistics(const VIDTRAN_Handle_t TranscoderHandle, STVID_TranscoderStatistics_t * const Statistics_p);
#endif /* STVID_DEBUG_GET_STATISTICS */

#ifdef TRANSCODE_PES_OUTPUT
static BOOL vidtran_AddPESHeaderInTranscoderOutputBuffer(const VIDTRAN_Handle_t TranscoderHandle);
#endif /* TRANSCODE_PES_OUTPUT */
static BOOL vidtran_CheckPictureEligibilityForTranscode(const VIDTRAN_Handle_t TranscoderHandle, VIDCOM_PictureGenericData_t * PictureGenericData_p);
static BOOL vidtran_RegulateTranscodeQueue(const VIDTRAN_Handle_t TranscoderHandle, VIDTRAN_Picture_t * const TrCurrentPicture_p);
static void vidtran_FlushOutputTranscodedBuffer(const VIDTRAN_Handle_t TranscoderHandle, const BOOL IsLastFlush);
static void vidtran_MonitorOutputBuffer(const VIDTRAN_Handle_t TranscoderHandle, VIDTRAN_MonitorOutputBufferParams_t * const MonitorOutputBufferParams_p);
static void vidtran_ResetReferenceList(const VIDTRAN_Handle_t TranscoderHandle);
static void vidtran_BuildReferenceList(const VIDTRAN_Handle_t TranscoderHandle, VIDTRAN_Picture_t * const TrCurrentPicture_p);
static void vidtran_UpdateReferenceList(const VIDTRAN_Handle_t TranscoderHandle, VIDTRAN_Picture_t * const TrCurrentPicture_p);

#ifdef LINEAR_OUTPUT_BUFFER
static BOOL vidtran_UpdateTranscoderOutputBuffer(const VIDTRAN_Handle_t TranscoderHandle);
#endif

static void SourceDecoderNewPictureAvailableCB(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
static void TargetDecoderNewPictureDecodedCB(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
/* Not an event because the Xcoder has no RegistrantName */
static void XcoderJobCompletedCB(VIDTRAN_Handle_t TranscoderHandle, XCODER_TranscodingJobResults_t XCODER_TranscodingJobResults);

/* Global Variables --------------------------------------------------------- */
const VIDTRAN_FunctionsTable_t   VIDTRAN_Functions =
{
#ifdef STVID_DEBUG_GET_STATISTICS
  vidtran_GetStatistics,
  vidtran_ResetAllStatistics,
#endif /* STVID_DEBUG_GET_STATISTICS */
  vidtran_Init,
  vidtran_Term,
  vidtran_Open,
  vidtran_Close,
  vidtran_Start,
  vidtran_Stop,
  vidtran_SetParams,
  vidtran_SetMode,
  vidtran_Link,
  vidtran_Unlink,
  vidtran_InformReadAddress,
  vidtran_SetErrorRecoveryMode,
  vidtran_GetErrorRecoveryMode
};

#ifdef STVID_DEBUG_GET_STATISTICS
    U32 CountEndOfTranscode = 0;
    U32 CurrentEndOfTranscodeTime, LastEndOfTranscodeTime, DiffEndOfTranscodeTime;
#endif /* STVID_DEBUG_GET_STATISTICS */

/* Public Functions --------------------------------------------------------- */
/*******************************************************************************
Name        : vidtran_AllocatePESBuffer
Description : Allocates the PES buffer shared with the application
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if bad parameter
              ST_ERROR_NO_MEMORY if memory allocation fails
              STVID_ERROR_EVENT_REGISTRATION if problem registering events
*******************************************************************************/
ST_ErrorCode_t vidtran_AllocatePESBuffer(const VIDTRAN_Handle_t TranscoderHandle_p, const VIDTRAN_InitParams_t * const InitParams_p)
{
    VIDTRAN_Data_t *           VIDTRAN_Data_p;
    STAVMEM_AllocBlockParams_t AllocBlockParams;
    void *                     VirtualAddress;

    VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle_p;

    /* Modifying default value of size if required */
    if (VIDTRAN_Data_p->TranscoderOutputBuffer.Size == 0)
    {
        VIDTRAN_Data_p->TranscoderOutputBuffer.Size = TRANSCODER_OUTPUT_BUFFER_TOTAL_SIZE;
    }

    AllocBlockParams.PartitionHandle          = InitParams_p->AVMEMPartition;
    AllocBlockParams.Alignment                = 32;
    AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocBlockParams.NumberOfForbiddenRanges  = 0;
    AllocBlockParams.ForbiddenRangeArray_p    = NULL;
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;

    AllocBlockParams.Size = VIDTRAN_Data_p->TranscoderOutputBuffer.Size;

    if (STAVMEM_AllocBlock(&AllocBlockParams, &VIDTRAN_Data_p->TranscoderOutputBuffer.Handle) != ST_NO_ERROR)
    {
        return(ST_ERROR_NO_MEMORY);
    }
    if (STAVMEM_GetBlockAddress(VIDTRAN_Data_p->TranscoderOutputBuffer.Handle, (void **)&VirtualAddress) != ST_NO_ERROR)
    {
        return(ST_ERROR_NO_MEMORY);
    }
    VIDTRAN_Data_p->TranscoderOutputBuffer.Base_p = STAVMEM_VirtualToDevice(VirtualAddress,&VIDTRAN_Data_p->VirtualMapping);
    VIDTRAN_Data_p->TranscoderOutputBuffer.Top_p = (void *)((U8 *)VIDTRAN_Data_p->TranscoderOutputBuffer.Base_p + VIDTRAN_Data_p->TranscoderOutputBuffer.Size);
    VIDTRAN_Data_p->TranscoderOutputBuffer.IsAllocatedByUs = TRUE;

#ifdef XCODE_DEMO
    memset(VirtualAddress, 0x00, VIDTRAN_Data_p->TranscoderOutputBuffer.Size); /* fill in PES buffer with 0x00 by default */
#endif

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : vidtran_InitPESBuffer
Description : Initialize the PES buffer shared with the application
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t vidtran_InitPESBuffer(const VIDTRAN_Handle_t TranscoderHandle_p)
{
    VIDTRAN_Data_t *           VIDTRAN_Data_p;
    ST_ErrorCode_t ErrorCode;

    VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle_p;
    ErrorCode = ST_NO_ERROR;

    VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p = VIDTRAN_Data_p->TranscoderOutputBuffer.Base_p;
    VIDTRAN_Data_p->TranscoderOutputBuffer.Read_p = VIDTRAN_Data_p->TranscoderOutputBuffer.Base_p;
    VIDTRAN_Data_p->TranscoderOutputBuffer.Loop_p = VIDTRAN_Data_p->TranscoderOutputBuffer.Base_p;

    return(ErrorCode);
}

/*******************************************************************************
Name        : vidtran_Init
Description : Initialise transcode
Parameters  : Init params, pointer to video transcode handle returned if success
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if bad parameter
              ST_ERROR_NO_MEMORY if memory allocation fails
              STVID_ERROR_EVENT_REGISTRATION if problem registering events
*******************************************************************************/
ST_ErrorCode_t vidtran_Init(const VIDTRAN_InitParams_t * const InitParams_p, VIDTRAN_Handle_t * const TranscoderHandle_p)
{
    VIDTRAN_Data_t *                VIDTRAN_Data_p;
    STEVT_OpenParams_t              STEVT_OpenParams;
    STEVT_DeviceSubscribeParams_t   STEVT_SubscribeParams;
    ST_ErrorCode_t                  ErrorCode;
    TARGET_InitParams_t             TARGETInitParams;
    XCODE_InitParams_t              XCODEInitParams;
    STVID_OpenParams_t              OpenParams;

    if ((TranscoderHandle_p == NULL) || (InitParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Allocate a VIDTRAN structure */
    VIDTRAN_Data_p = (VIDTRAN_Data_t *) memory_allocate(InitParams_p->CPUPartition_p, sizeof(VIDTRAN_Data_t));
    if (VIDTRAN_Data_p == NULL)
    {
        /* Error: allocation failed */
        return(ST_ERROR_NO_MEMORY);
    }

    /* Allocation succeeded: make handle point on structure */
    *TranscoderHandle_p = (VIDTRAN_Handle_t *) VIDTRAN_Data_p;
    VIDTRAN_Data_p->ValidityCheck = VIDTRAN_VALID_HANDLE;

    /* Init transcoder data structure */
    InitTranscoderStructure(*TranscoderHandle_p);

    /* Remember partitions */
    VIDTRAN_Data_p->CPUPartition_p = InitParams_p->CPUPartition_p;
    VIDTRAN_Data_p->AVMEMPartition = InitParams_p->AVMEMPartition;

    /* Copy parameters in structure. To be used in TranscoderLink */
    VIDTRAN_Data_p->OutputStreamProfile = InitParams_p->OutputStreamProfile;

    /* Get AV mem mapping */
    STAVMEM_GetSharedMemoryVirtualMapping(&(VIDTRAN_Data_p->VirtualMapping));

    /* Stores global EvtHandlerName (for linking) */
    strcpy(VIDTRAN_Data_p->EvtHandlerName, InitParams_p->EvtHandlerName);

    /* Open EVT handle */
    ErrorCode = STEVT_Open(InitParams_p->EvtHandlerName, &STEVT_OpenParams, &(VIDTRAN_Data_p->EventsHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Transcode init: could not open EVT !"));
        vidtran_Term(*TranscoderHandle_p);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    /* Register events */
    ErrorCode = STEVT_RegisterDeviceEvent(VIDTRAN_Data_p->EventsHandle, InitParams_p->TranscoderName, STVID_TRANSCODER_XCODER_JOB_COMPLETED_EVT, &(VIDTRAN_Data_p->RegisteredEventsID[STVID_TRANSCODER_XCODER_JOB_COMPLETED_EVT_ID]));
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDTRAN_Data_p->EventsHandle, InitParams_p->TranscoderName, STVID_ENCODER_STOPPED_EVT, &(VIDTRAN_Data_p->RegisteredEventsID[STVID_TRANSCODER_STOPPED_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDTRAN_Data_p->EventsHandle, InitParams_p->TranscoderName, STVID_ENCODER_NEW_PICTURE_ENCODED_EVT, &(VIDTRAN_Data_p->RegisteredEventsID[STVID_TRANSCODER_NEW_PICTURE_TRANSCODED_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDTRAN_Data_p->EventsHandle, InitParams_p->TranscoderName, STVID_ENCODER_FLUSH_ENCODED_BUFFER_EVT, &(VIDTRAN_Data_p->RegisteredEventsID[STVID_TRANSCODER_FLUSH_TRANSCODED_BUFFER_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDTRAN_Data_p->EventsHandle, InitParams_p->TranscoderName, STVID_ENCODER_ERROR_EVT, &(VIDTRAN_Data_p->RegisteredEventsID[STVID_TRANSCODER_ERROR_EVT_ID]));
    }
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: registration failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Transcode init: could not register events !"));
        vidtran_Term(*TranscoderHandle_p);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    /* Retrieving size of buffer */
    VIDTRAN_Data_p->TranscoderOutputBuffer.Size  = InitParams_p->OutputBufferSize;

    /* Set Transcoder PES buffer */
    if (!(InitParams_p->OutputBufferAllocated))
    {
        ErrorCode = vidtran_AllocatePESBuffer(*TranscoderHandle_p, InitParams_p);
        if (ErrorCode != ST_NO_ERROR)
        {
          TraceVerbose(("Error code: %d ", ErrorCode));
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Transcode Init: cannot allocate PES buffer\n"));
          vidtran_Term(*TranscoderHandle_p);
          return(ST_ERROR_NO_MEMORY);
        }
        VIDTRAN_Data_p->TranscoderOutputBuffer.IsAllocatedByUs = TRUE;
    }
    else
    {
        /* Transcoder PES buffer allocated by application: just copy parameters */
        VIDTRAN_Data_p->TranscoderOutputBuffer.Base_p  = InitParams_p->OutputBufferAddress_p;
        VIDTRAN_Data_p->TranscoderOutputBuffer.Top_p = (void *)((U32)(VIDTRAN_Data_p->TranscoderOutputBuffer.Base_p) + VIDTRAN_Data_p->TranscoderOutputBuffer.Size);
        VIDTRAN_Data_p->TranscoderOutputBuffer.IsAllocatedByUs = FALSE;
    }
    vidtran_InitPESBuffer(*TranscoderHandle_p);

    /* Start decode task */
    ErrorCode = StartTranscoderTask(*TranscoderHandle_p);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: task creation failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Transcode init: could not start transcode task ! (%s)", (ErrorCode == ST_ERROR_ALREADY_INITIALIZED?"already exists":"failed")));
#ifdef STVID_VALID_LOG_MESSAGES
        MSGLOG_LogPrint(AutoCheck_ResultLog_Handle, "Module Transcode init: could not transcode display task ! (%s)\n", (ErrorCode == ST_ERROR_ALREADY_INITIALIZED?"already exists":"failed"));
#endif /* STVID_VALID_LOG_MESSAGES */
        vidtran_Term(*TranscoderHandle_p);
        if (ErrorCode == ST_ERROR_ALREADY_INITIALIZED)
        {
            /* Change error code because ST_ERROR_ALREADY_INITIALIZED is not allowed for _Init() functions */
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        return(ErrorCode);
    }

    /* Initialize Xcoder */
    XCODEInitParams.STVID_XcodeInitParams = InitParams_p->Xcode_InitParams;
    strcpy(XCODEInitParams.STVID_XcodeInitParams.EvtHandlerName, InitParams_p->EvtHandlerName);
    XCODEInitParams.XcoderJobCompletedCB = XcoderJobCompletedCB;
    XCODEInitParams.TranscoderHandle = VIDTRAN_Data_p;
    ErrorCode = XCODE_Init(&XCODEInitParams, &(VIDTRAN_Data_p->XcodeHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        vidtran_Term(*TranscoderHandle_p);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* TODO: Checks whether Target decode already in use by another transcode */

    /* Link target decoder */
    strcpy(VIDTRAN_Data_p->Target.DecoderDeviceName, InitParams_p->TargetDeviceName);
    if (strlen(VIDTRAN_Data_p->Target.DecoderDeviceName) > ST_MAX_DEVICE_NAME - 1) /* Device name length should be respected */
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        vidtran_Term(*TranscoderHandle_p);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Resets Structure */
    memset(&OpenParams, 0, sizeof(OpenParams));

    /* Open Target Decode and get handle */
    OpenParams.SyncDelay = 0; /* To add as a variable ? */
    ErrorCode = STVID_Open(VIDTRAN_Data_p->Target.DecoderDeviceName, &OpenParams, (STVID_Handle_t *) &VIDTRAN_Data_p->Target.DecoderHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Transcode Open: Cannot open target decoder !"));
        return(ErrorCode);
    }

    /* Retrieve Target Decode Data Parameters */
    ErrorCode = stvid_GetDecoderDeviceData(VIDTRAN_Data_p->Target.DecoderDeviceName, &(VIDTRAN_Data_p->Target.DeviceData_p));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Transcode: Cannot find target decoder data !"));
        return(ErrorCode);
    }

    strcpy(TARGETInitParams.EvtHandlerName, InitParams_p->EvtHandlerName);
    TARGETInitParams.DeviceType = VIDTRAN_Data_p->Target.DeviceData_p->DeviceType;
    TARGETInitParams.CPUPartition_p = VIDTRAN_Data_p->Target.DeviceData_p->CPUPartition_p;

    /* Init of Target parameters for transcoder */
    ErrorCode = TARGET_Init(&TARGETInitParams, &(VIDTRAN_Data_p->Target.TranscodecHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        vidtran_Term(*TranscoderHandle_p);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Subscribe to the target decoder "STVID_NEW_PICTURE_DECODED_EVT" */
    STEVT_SubscribeParams.SubscriberData_p = (void *) (*TranscoderHandle_p);
    STEVT_SubscribeParams.NotifyCallback = TargetDecoderNewPictureDecodedCB;
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDTRAN_Data_p->EventsHandle, VIDTRAN_Data_p->Target.DecoderDeviceName,
                    STVID_NEW_PICTURE_DECODED_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Transcode init: could not subscribe to Target decoder STVID_NEW_PICTURE_DECODED_EVT !"));
        vidtran_Term(*TranscoderHandle_p);
        return(ST_ERROR_BAD_PARAMETER);
    }

    return(ErrorCode);

} /* end of vidtran_Init() function */

/*******************************************************************************
Name        : vidtran_Open
Description : Open transcode
Parameters  : Open params, pointer to source and target transcode handle returned if success
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t vidtran_Open(const VIDTRAN_Handle_t TranscoderHandle, const VIDTRAN_OpenParams_t * const TranscoderOpenParams_p)
{
    VIDTRAN_Data_t *            VIDTRAN_Data_p;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

    if (!TranscoderOpenParams_p)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Find structure */
    VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;

    return(ErrorCode);
}
/*******************************************************************************
Name        : vidtran_Close
Description : Close transcode
Parameters  : Close transcoder and target decoder
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t vidtran_Close(const VIDTRAN_Handle_t TranscoderHandle, const STVID_TranscoderCloseParams_t * const TranscoderCloseParams_p)
{
    VIDTRAN_Data_t *    VIDTRAN_Data_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!TranscoderCloseParams_p)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Find structure */
    VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;

    /* Closing Target decoder */
    ErrorCode = STVID_Close((STVID_Handle_t)(VIDTRAN_Data_p->Target.DecoderHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Transcode Close: cannot close target decoder"));
        return(ErrorCode);
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : vidtran_Term
Description : Terminate transcoder, undo all what was done at init
Parameters  : Video transcoder handle
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
              Terminating the target video decoder is outsider Transcoder
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_INVALID_HANDLE if not initialised
*******************************************************************************/
ST_ErrorCode_t vidtran_Term(const VIDTRAN_Handle_t TranscoderHandle)
{
    VIDTRAN_Data_t *    VIDTRAN_Data_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAVMEM_FreeBlockParams_t   	FreeParams;

    /* Find structure */
    VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;

    if (VIDTRAN_Data_p->ValidityCheck != VIDTRAN_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Term of Target parameters for transcoder */
    if (VIDTRAN_Data_p->Target.TranscodecHandle != NULL)
    {
        TARGET_Term(VIDTRAN_Data_p->Target.TranscodecHandle);
    }

    /* Closing Target decoder */
    ErrorCode = STVID_Close((STVID_Handle_t)(VIDTRAN_Data_p->Target.DecoderHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Not an error, just an information, continuing */
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Module Transcode Term: cannot close target decoder"));
    }

    if (VIDTRAN_Data_p->XcodeHandle != NULL)
    {
#if 1 /* Firmware Term workaround */
        if (VIDTRAN_Data_p->IsTranscodeOccur)
#endif
        {
            XCODE_Term(VIDTRAN_Data_p->XcodeHandle);
        }
    }

    if (VIDTRAN_Data_p->TranscoderTask.Task_p != NULL)
    {
        StopTranscoderTask(TranscoderHandle);
    }

    /* De-allocate Transcoder PES buffer */
    if (VIDTRAN_Data_p->TranscoderOutputBuffer.IsAllocatedByUs)
    {
        FreeParams.PartitionHandle = VIDTRAN_Data_p->AVMEMPartition;
        ErrorCode = STAVMEM_FreeBlock(&FreeParams, &(VIDTRAN_Data_p->TranscoderOutputBuffer.Handle));
        if (ErrorCode != ST_NO_ERROR)
        {
            return(ST_ERROR_BAD_PARAMETER);
        }
    }

    /* Destroy decode structure (delete semaphore, etc) */
    TermTranscoderStructure(TranscoderHandle);

    /* Un-subscribe and un-register events: this is done automatically by STEVT_Close() */
    /* Close instances opened at init */
    ErrorCode = STEVT_Close(VIDTRAN_Data_p->EventsHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Not an error, just information, continuing */
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Module Transcode Term: Error closing EVT"));
    }

    /* De-validate structure */
    VIDTRAN_Data_p->ValidityCheck = 0; /* not VIDTRAN_VALID_HANDLE */

    /* De-allocate last: data inside cannot be used after ! */
    memory_deallocate(VIDTRAN_Data_p->CPUPartition_p, (void *) VIDTRAN_Data_p);

    return(ErrorCode);
} /* end of vidtran_Term() function */

/*******************************************************************************
Name        : vidtran_Start
Description : Start transcoding
Parameters  : Video transcode handle, start params
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if decoder started
              ST_ERROR_BAD_PARAMETER if bad parameter
              STVID_ERROR_DECODER_RUNNING if the decoder is already started
              STVID_ERROR_DECODER_PAUSING if the decoder is paused
*******************************************************************************/
ST_ErrorCode_t vidtran_Start(const VIDTRAN_Handle_t TranscoderHandle, const VIDTRAN_StartParams_t * const TranscoderStartParams_p)
{
    VIDTRAN_Data_t * VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!TranscoderStartParams_p)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Checks the transcoder is linked or not */
    if (!VIDTRAN_Data_p->Source.DecoderHandle)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Transcode Start: No source decode !"));
        ErrorCode = ST_ERROR_INVALID_HANDLE;
        return(ErrorCode);
    }

    /* Copy parameters for info */
    /* strcpy(TranscoderStartParams_p->SourceDecoderName, VIDTRAN_Data_p->Source.DecoderDeviceName); */
    /* strcpy(TranscoderStartParams_p->TargetDecoderName, VIDTRAN_Data_p->Target.DecoderDeviceName); */

    if (VIDTRAN_Data_p->TranscoderState != VIDTRAN_TRANSCODER_STATE_STOPPED)
    {
        ErrorCode = ST_ERROR_ALREADY_INITIALIZED;
        return(ErrorCode);
    }

#ifdef STVID_DEBUG_GET_STATISTICS
    CountEndOfTranscode = 0;
    VIDTRAN_Data_p->Statistics.EncodePicturesAverageSpeed = 0;
#endif /* STVID_DEBUG_GET_STATISTICS */


    VIDTRAN_Data_p->ReferenceStatus = VIDTRAN_AVAILABLE_REFERENCE_NONE;
    VIDTRAN_Data_p->TranscoderState = VIDTRAN_TRANSCODER_STATE_STARTING;
    /* Force reschedule of transcoder task */
    STOS_SemaphoreSignal(VIDTRAN_Data_p->TranscoderOrder_p);

    return(ErrorCode);

} /* End of vidtran_Start() function */
/*******************************************************************************
Name        : vidtran_Stop
Description : Stop transcoding
Parameters  : Video transcoder handle, stop mode
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if transcoder stopped
              STVID_ERROR_DECODER_STOPPED if the transcoder is already stopped
*******************************************************************************/
ST_ErrorCode_t vidtran_Stop(const VIDTRAN_Handle_t TranscoderHandle, const VIDTRAN_StopParams_t * const TranscoderStopParams_p)
{
    VIDTRAN_Data_t  * VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;
    ST_ErrorCode_t    ErrorCode = ST_NO_ERROR;
    STOS_Clock_t      StopTimeFailed;

    if (!TranscoderStopParams_p)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (VIDTRAN_Data_p->TranscoderState == VIDTRAN_TRANSCODER_STATE_STOPPED)
    {
        ErrorCode = STVID_ERROR_ENCODER_STOPPED;
        return(ErrorCode);
    }

    /* References are lost (To be changed) */
    VIDTRAN_Data_p->ReferenceStatus = VIDTRAN_AVAILABLE_REFERENCE_NONE;

    /* EndOfBitstreamFlag to do */

    /* Set counter to zero */
    while (STOS_SemaphoreWaitTimeOut(VIDTRAN_Data_p->TranscoderStop_p, TIMEOUT_IMMEDIATE) == 0)
    {
        /* Nothing to do: just to decrement counter until 0 */;
    }

    /* Changing to temporary stop state depending on mode */
    switch (TranscoderStopParams_p->EncoderStopMode)
    {
        /* Stop when queue empty */
        case STVID_ENCODER_STOP_WHEN_END_OF_DATA :
            /* Force TranscoderState: No more picture processed into queue */
            VIDTRAN_Data_p->TranscoderState = VIDTRAN_TRANSCODER_STATE_STOP_WHEN_QUEUE_EMPTY;

            /* Push command to the transcoder task */
            PushTranscoderCommand(TranscoderHandle, TRANSCODER_FLUSH_CHECK_QUEUE_AND_STOP);
            STOS_SemaphoreSignal(VIDTRAN_Data_p->TranscoderOrder_p);
        break;

        /* Stop and flush (queue) */
        case STVID_ENCODER_STOP_NOW :
            /* Will flush and release pictures in queue one by one */
            VIDTRAN_Data_p->TranscoderState = VIDTRAN_TRANSCODER_STATE_STOP_AND_FLUSH;

            /* Push command to the transcoder task */
            PushTranscoderCommand(TranscoderHandle, TRANSCODER_FLUSH_CHECK_QUEUE_AND_STOP);
            STOS_SemaphoreSignal(VIDTRAN_Data_p->TranscoderOrder_p);
        break;

#if 0
        /* Stop urgently (no queue flush, no extra process) */
        case STVID_ENCODER_STOP_AND_KEEP_CONTEXT :

            /* Immediately stopping filling the queue and process */
            VIDTRAN_Data_p->TranscoderState = VIDTRAN_TRANSCODER_STATE_STOPPED;

            /* Flushing the Transcoded output buffer. Last flush */
            vidtran_FlushOutputTranscodedBuffer(TranscoderHandle, TRUE);

        break;
#endif

        default:

            /* Immediately stopping filling the queue and process */
            VIDTRAN_Data_p->TranscoderState = VIDTRAN_TRANSCODER_STATE_STOPPED;

        break;
    }

    /* In the case we are waiting for the state VIDTRAN_TRANSCODER_STATE_STOPPED */
    if ((TranscoderStopParams_p->EncoderStopMode == STVID_ENCODER_STOP_WHEN_END_OF_DATA)
     || (TranscoderStopParams_p->EncoderStopMode == STVID_ENCODER_STOP_NOW))
    {
        if (TranscoderStopParams_p->StopAfterTimeOut == TRUE)
        {
            /* Waiting the transcoder to stop */
            StopTimeFailed = STOS_time_plus(STOS_time_now(), MAX_TICKS_TO_WAIT_FOR_A_STOP);
            STOS_SemaphoreWaitTimeOut(VIDTRAN_Data_p->TranscoderStop_p, &StopTimeFailed);

            /* Now transcoder should have stopped */
            if (VIDTRAN_Data_p->TranscoderState != VIDTRAN_TRANSCODER_STATE_STOPPED)
            {
                ErrorCode = STVID_ERROR_ENCODER_RUNNING;
            }

            /* Note: Notification is done in transcoder task */
        }

    }
    else
    {
        /* Notifying transcoder has stopped */
        STEVT_Notify(VIDTRAN_Data_p->EventsHandle,
                     VIDTRAN_Data_p->RegisteredEventsID[STVID_TRANSCODER_STOPPED_EVT_ID],
                     NULL);
    }

#ifdef STVID_DEBUG_GET_STATISTICS
    VIDTRAN_Data_p->Statistics.EncodePicturesAverageSpeed = 0;
#endif /* STVID_DEBUG_GET_STATISTICS */

    return(ErrorCode);

} /* End of vidtran_Stop() function */

/*******************************************************************************
Name        : vidtran_Link
Description : Link transcoder
Parameters  : Transcoder handle, Video Handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t vidtran_Link(const VIDTRAN_Handle_t TranscoderHandle, const STVID_TranscoderLinkParams_t * const TranscoderLinkParams_p)
{
    VIDTRAN_Data_t *                VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;
    STVID_OpenParams_t              OpenParams;
    STEVT_DeviceSubscribeParams_t   STEVT_SubscribeParams;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    SOURCE_InitParams_t             SOURCEInitParams;
    STVID_DecimationFactor_t        DecimationFactor;
    VIDCOM_InternalProfile_t        InternalProfile;

    if (!TranscoderLinkParams_p)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Checks whether already linked */
    if (VIDTRAN_Data_p->Source.DecoderHandle)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Transcode Link: already linked to a video decode"));
        ErrorCode = ST_ERROR_INVALID_HANDLE;
        return(ErrorCode);
    }

    /* Associates source decoder and validates link */
    strcpy(VIDTRAN_Data_p->Source.DecoderDeviceName, TranscoderLinkParams_p->SourceDeviceName);

    /* Open Source Decode and get handle */
    OpenParams.SyncDelay = 0; /* To add as a variable ? */
    ErrorCode = STVID_Open(VIDTRAN_Data_p->Source.DecoderDeviceName, &OpenParams, (STVID_Handle_t *) &VIDTRAN_Data_p->Source.DecoderHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Transcode Open: cannot open source decoder"));
        return(ErrorCode);
    }

    /* Retrieve Source Decode Data Parameters */
    stvid_GetDecoderDeviceData(VIDTRAN_Data_p->Source.DecoderDeviceName, &(VIDTRAN_Data_p->Source.DeviceData_p));

    /* Sets Decimation factor for transcode from source parameters */
    ErrorCode = VIDBUFF_GetMemoryProfile(VIDTRAN_Data_p->Source.DeviceData_p->BuffersHandle, &InternalProfile);

    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_LinkTranscoder() : Cannot get parameters from source"));
        return(ErrorCode);
    }

    /* ***************************************************************** */
    /* Today we assume enough buffers are available vs needed for decode */
    /* ToDo: Trying to add extra framebuffers in the poll (if dynamic    */
    /* allocation is in use                                              */
    /* ***************************************************************** */

    DecimationFactor = InternalProfile.ApplicableDecimationFactor;

    /* Disregard invalid Decimation factor */
    if (DecimationFactor &
        ~(STVID_DECIMATION_FACTOR_H2 |
          STVID_DECIMATION_FACTOR_V2 |
          STVID_DECIMATION_FACTOR_H4 |
          STVID_DECIMATION_FACTOR_V4 |
          STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2
         )
        )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "STVID_LinkTranscoder() : Invalid decimation factor !"));
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Store Decimation factor for transcode */
    VIDTRAN_Data_p->XCODE_TranscodePictureParams.TargetDecimationFactor = InternalProfile.ApplicableDecimationFactor;

    /* Get Source params */
    SOURCEInitParams.DeviceType = VIDTRAN_Data_p->Source.DeviceData_p->DeviceType;
    SOURCEInitParams.CPUPartition_p = VIDTRAN_Data_p->Source.DeviceData_p->CPUPartition_p;
    strcpy(SOURCEInitParams.EvtHandlerName, VIDTRAN_Data_p->EvtHandlerName);

    /* Init of Source parameters for transcoder */
    ErrorCode = SOURCE_Init(&SOURCEInitParams, &(VIDTRAN_Data_p->Source.TranscodecHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        vidtran_Unlink(TranscoderHandle);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Heritates the error recovery from source decoder */
    switch(VIDTRAN_Data_p->Source.DeviceData_p->ErrorRecoveryMode)
    {
        case STVID_ERROR_RECOVERY_FULL :
            VIDTRAN_Data_p->ErrorRecoveryMode = STVID_ENCODER_MODE_ERROR_RECOVERY_FULL;
        break;

        case STVID_ERROR_RECOVERY_NONE :
        case STVID_ERROR_RECOVERY_PARTIAL :
        case STVID_ERROR_RECOVERY_HIGH :
            VIDTRAN_Data_p->ErrorRecoveryMode = STVID_ENCODER_MODE_ERROR_RECOVERY_NONE;
        break;

        default:
           /* Error recovery mode not changed */
        break;
    }

    /* Force TranscoderState */
    VIDTRAN_Data_p->TranscoderState = VIDTRAN_TRANSCODER_STATE_STOPPED;

    /* Subscribe to the source decoder "STVID_NEW_PICTURE_DECODED_EVT" or "STVID_NEW_PICTURE_ORDERED_EVT" */
    STEVT_SubscribeParams.SubscriberData_p = (void *) (TranscoderHandle);
    STEVT_SubscribeParams.NotifyCallback = SourceDecoderNewPictureAvailableCB;
    if (VIDTRAN_Data_p->OutputStreamProfile == STVID_ENCODER_OUTPUT_H264_BASELINE_PROFILE)
    {
        ErrorCode = STEVT_SubscribeDeviceEvent(VIDTRAN_Data_p->EventsHandle, VIDTRAN_Data_p->Source.DecoderDeviceName,
                      STVID_NEW_PICTURE_ORDERED_EVT, &STEVT_SubscribeParams);
    }
    else
    {
        ErrorCode = STEVT_SubscribeDeviceEvent(VIDTRAN_Data_p->EventsHandle, VIDTRAN_Data_p->Source.DecoderDeviceName,
                     STVID_NEW_PICTURE_DECODED_EVT, &STEVT_SubscribeParams);
    }
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Transcode init: could not subscribe to Source decoder STVID_NEW_PICTURE_DECODED/ORDERED_EVT !"));
        /* Unvalidates link */
        vidtran_Unlink(TranscoderHandle);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Keeps BuffersHandle for transcoder queue */
    VIDTRAN_Data_p->BufferManagerHandle = VIDTRAN_Data_p->Source.DeviceData_p->BuffersHandle;

    /* Transcode is not started but will at next stage: Init TrQueue */
    vidtrqueue_InitTranscoderQueue(VIDTRAN_Data_p);

    /* Reset reference list */
    /* vidtran_ResetReferenceList(TranscoderHandle); */

    return(ST_NO_ERROR);
} /* End of vidtran_Link() function */

/*******************************************************************************
Name        : vidtran_Unlink
Description : Unlink transcoding
Parameters  : Video transcoder handle, stop mode
Assumptions : Transcoder MUST be stopped
Limitations :
Returns     : ST_NO_ERROR if transcoder stopped
              STVID_ERROR_DECODER_STOPPED if the transcoder is already stopped
*******************************************************************************/
ST_ErrorCode_t vidtran_Unlink(const VIDTRAN_Handle_t TranscoderHandle)
{
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    VIDTRAN_Data_t *                VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;

    if (!VIDTRAN_Data_p->Source.DecoderHandle)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Transcode Unlink: No current link to a video decode ! "));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        return(ErrorCode);
    }

    if (VIDTRAN_Data_p->TranscoderState != VIDTRAN_TRANSCODER_STATE_STOPPED)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Transcode Unlink: Transcoder not stopped !"));
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Unsubscribe to the source decoder "STVID_NEW_PICTURE_DECODED_EVT" or "STVID_NEW_PICTURE_ORDERED_EVT" */
    if (VIDTRAN_Data_p->OutputStreamProfile == STVID_ENCODER_OUTPUT_H264_BASELINE_PROFILE)
    {
        ErrorCode = STEVT_UnsubscribeDeviceEvent(VIDTRAN_Data_p->EventsHandle, VIDTRAN_Data_p->Source.DecoderDeviceName,
                      STVID_NEW_PICTURE_ORDERED_EVT);
    }
    else
    {
        ErrorCode = STEVT_UnsubscribeDeviceEvent(VIDTRAN_Data_p->EventsHandle, VIDTRAN_Data_p->Source.DecoderDeviceName,
                     STVID_NEW_PICTURE_DECODED_EVT);
    }
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Transcode Unlink: could not unsubscribe to Source decoder STVID_NEW_PICTURE_DECODED/ORDERED_EVT !"));
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Close Source Decode */
    ErrorCode = STVID_Close((STVID_Handle_t) VIDTRAN_Data_p->Source.DecoderHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Transcode Unlink: cannot close source decoder"));
        return(ErrorCode);
    }

    /* Unvalidates link */
    VIDTRAN_Data_p->Source.DecoderDeviceName[0] = '\0';
    VIDTRAN_Data_p->Source.DecoderHandle = (STVID_Handle_t) NULL;

    /* Term of Source parameters for transcoder */
    SOURCE_Term(VIDTRAN_Data_p->Source.TranscodecHandle);
    VIDTRAN_Data_p->Source.TranscodecHandle = (STVID_Handle_t) NULL;

    return(ErrorCode);
} /* End of vidtran_Unlink() function */



/*******************************************************************************
Name        : vidtran_SetParams
Description :
Parameters  : Video transcoder handle, stop mode
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if transcoder stopped
              STVID_ERROR_DECODER_STOPPED if the transcoder is already stopped
*******************************************************************************/
ST_ErrorCode_t vidtran_SetParams(const VIDTRAN_Handle_t TranscoderHandle, const VIDTRAN_SetParams_t * const SetParams_p)
{
    VIDTRAN_Data_t * VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;

    /* Save parameters in data structure (only CBR supported) */
    VIDTRAN_Data_p->XCODE_TranscodePictureParams.TargetFrameRate        = SetParams_p->FrameRate;
    VIDTRAN_Data_p->XCODE_TranscodePictureParams.TargetBitRate          = SetParams_p->CompressionParams.CompressionControl.CBR.BitRate;
    VIDTRAN_Data_p->XCODE_TranscodePictureParams.RateControlMode        = SetParams_p->CompressionParams.RateControlMode;

    /* LM: For on the fly modifications, a specific command should be sent */

    return(ST_NO_ERROR);
} /* End of vidtran_SetParams() function */


/*******************************************************************************
Name        : vidtran_SetMode
Description :
Parameters  : Video transcoder handle, Mode
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t vidtran_SetMode(const VIDTRAN_Handle_t TranscoderHandle, const STVID_EncoderSetMode_t * const SetMode_p)
{
    VIDTRAN_Data_t * VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;
    ST_ErrorCode_t   ErrorCode = ST_NO_ERROR;

    /* Save parameters in data structure */
    VIDTRAN_Data_p->TranscoderMode = *SetMode_p;

    /* Checking the source decode has been linked */
    if (!VIDTRAN_Data_p->Source.DecoderHandle)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Transcode SetMode: No source decoder currently linked ! "));
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        return(ErrorCode);
    }

    /* Enable/disable display of source decoder upon mode selection */
#ifdef ST_display
    if (VIDTRAN_Data_p->Source.DeviceData_p->DisplayHandleIsValid)
    {
        /* We are in real time mode. We must be sure the display is on */
        if (*SetMode_p == STVID_ENCODER_MODE_SLAVE)
        {
    	    VIDDISP_EnableDisplay(VIDTRAN_Data_p->Source.DeviceData_p->DisplayHandle, VIDTRAN_Data_p->Source.DecoderDeviceName);
        }
        else
        {
            /* Master mode. Display must be disabled */
    	    VIDDISP_DisableDisplay(VIDTRAN_Data_p->Source.DeviceData_p->DisplayHandle, VIDTRAN_Data_p->Source.DecoderDeviceName);
        }
    }
    else
    {
        /* Display has not been initilized */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        return(ErrorCode);
    }
#else
    if (*SetMode_p == STVID_ENCODER_MODE_SLAVE)
    {
        /* Realtime mode is asked but no display */
        ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
        return(ErrorCode);
    }
#endif

    /* Sets the parameter in the data structure */
    VIDTRAN_Data_p->TranscoderMode = *SetMode_p;

    /* LM: For on the fly modifications, a specific command should be sent */

    return(ST_NO_ERROR);
} /* End of vidtran_SetMode() function */

/*******************************************************************************
Name        : vidtran_SetErrorRecoveryMode
Description :
Parameters  : Video transcoder handle, error recovery mode
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t vidtran_SetErrorRecoveryMode(const VIDTRAN_Handle_t TranscoderHandle, const VIDTRAN_ErrorRecoveryMode_t Mode)
{
    VIDTRAN_Data_t * VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;

    /* Error Recovery */
    VIDTRAN_Data_p->ErrorRecoveryMode = Mode;

    return(ST_NO_ERROR);

} /* End of vidtran_SetErrorRecoveryMode() function */

/*******************************************************************************
Name        : vidtran_GetErrorRecoveryMode
Description :
Parameters  : Video transcoder handle, error recovery mode
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t vidtran_GetErrorRecoveryMode(const VIDTRAN_Handle_t TranscoderHandle, VIDTRAN_ErrorRecoveryMode_t * const Mode_p)
{
    VIDTRAN_Data_t * VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;

    /* Error Recovery */
    *Mode_p = VIDTRAN_Data_p->ErrorRecoveryMode;

    return(ST_NO_ERROR);

} /* End of vidtran_GetErrorRecoveryMode() function */

#ifdef STVID_DEBUG_GET_STATISTICS
/*******************************************************************************
Name        : vidtran_GetStatistics
Description : Gets the transcode module statistics.
Parameters  : Video transcoder handle, statistics.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
static ST_ErrorCode_t vidtran_GetStatistics(const VIDTRAN_Handle_t TranscoderHandle, STVID_TranscoderStatistics_t * const Statistics_p)
{
    VIDTRAN_Data_t          * VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;
    VIDTRQUEUE_QueueInfo_t  TrQueue_Info;
/*    STVID_Statistics_t        TranscoderStatistics; */

    if ((Statistics_p == NULL) || (VIDTRAN_Data_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Filling main statistics */
    Statistics_p->EncodePicturesAverageSpeed                        = VIDTRAN_Data_p->Statistics.EncodePicturesAverageSpeed;
    Statistics_p->EncodePicturesSkippedBeforeStart                  = VIDTRAN_Data_p->Statistics.EncodePicturesSkippedBeforeStart;
    Statistics_p->EncodeSourcePictureTaken                          = VIDTRAN_Data_p->Statistics.EncodeSourcePictureTaken;
    Statistics_p->EncodeSourcePictureReleased                       = VIDTRAN_Data_p->Statistics.EncodeSourcePictureReleased;
    Statistics_p->EncodePicturesInsertedInQueue                     = VIDTRAN_Data_p->Statistics.EncodePicturesInsertedInQueue;
    Statistics_p->EncodePicturesRemovedFromQueue                    = VIDTRAN_Data_p->Statistics.EncodePicturesRemovedFromQueue;
    Statistics_p->EncodePicturesTranscodeNedeed                     = VIDTRAN_Data_p->Statistics.EncodePicturesTranscodeNedeed;
    Statistics_p->EncodePicturesTranscoded                          = VIDTRAN_Data_p->Statistics.EncodePicturesTranscoded;
    Statistics_p->EncodeIPicturesTranscoded                         = VIDTRAN_Data_p->Statistics.EncodeIPicturesTranscoded;
    Statistics_p->EncodePPicturesTranscoded                         = VIDTRAN_Data_p->Statistics.EncodePPicturesTranscoded;
    Statistics_p->EncodeBPicturesTranscoded                         = VIDTRAN_Data_p->Statistics.EncodeBPicturesTranscoded;
    Statistics_p->EncodePicturesSkippedAtQueueInsertion             = VIDTRAN_Data_p->Statistics.EncodePicturesSkippedAtQueueInsertion;
    Statistics_p->EncodePicturesSkippedByNonEligibility             = VIDTRAN_Data_p->Statistics.EncodePicturesSkippedByNonEligibility;
    Statistics_p->EncodePicturesSkippedByQueueLevel                 = VIDTRAN_Data_p->Statistics.EncodePicturesSkippedByQueueLevel;
    Statistics_p->EncodePicturesAdjustedByQueueLevel                = VIDTRAN_Data_p->Statistics.EncodePicturesAdjustedByQueueLevel;
    Statistics_p->EncodeNbOfOutputBufferFlushed                     = VIDTRAN_Data_p->Statistics.EncodeNbOfOutputBufferFlushed;
    Statistics_p->EncodeNbOfOutputBufferBytesFlushed                = VIDTRAN_Data_p->Statistics.EncodeNbOfOutputBufferBytesFlushed;
    Statistics_p->EncodePbSourcePicturesSkippedByQueueFull          = VIDTRAN_Data_p->Statistics.EncodePbSourcePicturesSkippedByQueueFull;
    Statistics_p->EncodePbSourcePicturesSkippedByError              = VIDTRAN_Data_p->Statistics.EncodePbSourcePicturesSkippedByError;
    Statistics_p->EncodePbOutputBufferFull                          = VIDTRAN_Data_p->Statistics.EncodePbOutputBufferFull;
    Statistics_p->EncodePbOutputBufferOverflow                      = VIDTRAN_Data_p->Statistics.EncodePbOutputBufferOverflow;

    /* Retrieve instant queue statistics */
    vidtrqueue_InformationOnPicturesInTranscoderQueue(VIDTRAN_Data_p, &TrQueue_Info);
    /* Cumulative updates */
    VIDTRAN_Data_p->Statistics.EncodeQueueMaxPicturesQueued         = MAX_VALUE(VIDTRAN_Data_p->Statistics.EncodeQueueMaxPicturesQueued,TrQueue_Info.NbOfPicturesValidInQueue);
    VIDTRAN_Data_p->Statistics.EncodeQueueMaxPictureToBeTranscoded  = MAX_VALUE(VIDTRAN_Data_p->Statistics.EncodeQueueMaxPictureToBeTranscoded, TrQueue_Info.NbOfPicturesToBeTranscoded);

    /* Instant data */
    Statistics_p->EncodeQueuePicturesValid                          = TrQueue_Info.NbOfPicturesValidInQueue;
    Statistics_p->EncodeQueueStillInDisplayQueue                    = TrQueue_Info.NbOfPicturesStillInDisplayQueue;
    Statistics_p->EncodeQueuePictureToBeTranscoded                  = TrQueue_Info.NbOfPicturesToBeTranscoded;
    Statistics_p->EncodeQueuePictureReleaseRequested                = TrQueue_Info.NbOfPicturesReleaseRequested;
    Statistics_p->EncodeQueuePictureFree                            = TrQueue_Info.NbOfPicturesFree;

    /* Cumulative data */
    Statistics_p->EncodeQueueMaxPicturesQueued                      = VIDTRAN_Data_p->Statistics.EncodeQueueMaxPicturesQueued;
    Statistics_p->EncodeQueueMaxPictureToBeTranscoded               = VIDTRAN_Data_p->Statistics.EncodeQueueMaxPictureToBeTranscoded;

#ifdef STVID_DEBUG_TRQUEUE
    vidtrqueue_HistoryOfPicturesInTranscoderQueue(VIDTRAN_Data_p);
#endif /* STVID_DEBUG_TRQUEUE */

    return(ST_NO_ERROR);
} /* End of vidtran_GetStatistics() function */

/*******************************************************************************
Name        : vidtran_ResetAllStatistics
Description : Resets the transcode module statistics.
Parameters  : Video transcoder handle, statistics.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success, ST_ERROR_BAD_PARAMETER if Statistics_p == NULL
*******************************************************************************/
static ST_ErrorCode_t vidtran_ResetAllStatistics(const VIDTRAN_Handle_t TranscoderHandle, STVID_TranscoderStatistics_t * const Statistics_p)
{
    if ((Statistics_p == NULL) || (TranscoderHandle == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    InitStatisticsTranscoderStructure(TranscoderHandle);

    return(ST_NO_ERROR);
} /* End of vidtran_ResetAllStatistics() function */
#endif /* STVID_DEBUG_GET_STATISTICS */

/* Functions Exported in module --------------------------------------------- */


/* Private Functions -------------------------------------------------------- */
#ifdef STVID_DEBUG_GET_STATISTICS
/*******************************************************************************
Name        : InitStatisticsTranscoderStructure
Description : Initialise transcoder statistics structure
Parameters  : Video transcoder handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void InitStatisticsTranscoderStructure(const VIDTRAN_Handle_t TranscoderHandle)
{
    VIDTRAN_Data_t * VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;

    /* Reset statistics parameters */
    CountEndOfTranscode = 0;
    VIDTRAN_Data_p->Statistics.EncodePicturesAverageSpeed = 0;
    VIDTRAN_Data_p->Statistics.EncodePicturesSkippedBeforeStart = 0;
    VIDTRAN_Data_p->Statistics.EncodeSourcePictureTaken = 0;
    VIDTRAN_Data_p->Statistics.EncodeSourcePictureReleased = 0;
    VIDTRAN_Data_p->Statistics.EncodePicturesInsertedInQueue = 0;
    VIDTRAN_Data_p->Statistics.EncodePicturesRemovedFromQueue = 0;
    VIDTRAN_Data_p->Statistics.EncodePicturesTranscodeNedeed = 0;
    VIDTRAN_Data_p->Statistics.EncodePicturesTranscoded = 0;
    VIDTRAN_Data_p->Statistics.EncodeIPicturesTranscoded = -1;
    VIDTRAN_Data_p->Statistics.EncodePPicturesTranscoded = -1;
    VIDTRAN_Data_p->Statistics.EncodeBPicturesTranscoded = -1;
    VIDTRAN_Data_p->Statistics.EncodePicturesSkippedAtQueueInsertion = 0;
    VIDTRAN_Data_p->Statistics.EncodePicturesSkippedByNonEligibility = 0;
    VIDTRAN_Data_p->Statistics.EncodePicturesSkippedByQueueLevel = 0;
    VIDTRAN_Data_p->Statistics.EncodePicturesAdjustedByQueueLevel = 0;
    VIDTRAN_Data_p->Statistics.EncodeQueuePicturesValid = 0;
    VIDTRAN_Data_p->Statistics.EncodeQueueMaxPicturesQueued = 0;
    VIDTRAN_Data_p->Statistics.EncodeQueueStillInDisplayQueue = 0;
    VIDTRAN_Data_p->Statistics.EncodeQueuePictureToBeTranscoded = 0;
    VIDTRAN_Data_p->Statistics.EncodeQueueMaxPictureToBeTranscoded = 0;
    VIDTRAN_Data_p->Statistics.EncodeQueuePictureReleaseRequested = 0;
    VIDTRAN_Data_p->Statistics.EncodeQueuePictureFree = 0;
    VIDTRAN_Data_p->Statistics.EncodeNbOfOutputBufferFlushed = 0;
    VIDTRAN_Data_p->Statistics.EncodeNbOfOutputBufferBytesFlushed = 0;
    VIDTRAN_Data_p->Statistics.EncodePbSourcePicturesSkippedByQueueFull = 0;
    VIDTRAN_Data_p->Statistics.EncodePbSourcePicturesSkippedByError = 0;
    VIDTRAN_Data_p->Statistics.EncodePbOutputBufferFull = 0;
    VIDTRAN_Data_p->Statistics.EncodePbOutputBufferOverflow = 0;

    return;
}
#endif /* STVID_DEBUG_GET_STATISTICS */

/*******************************************************************************
Name        : InitTranscoderStructure
Description : Initialise transcoder structure
Parameters  : Video transcoder handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void InitTranscoderStructure(const VIDTRAN_Handle_t TranscoderHandle)
{
    VIDTRAN_Data_t * VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;
    U32 LoopCounter;

    /* Source decoder is not linked. Set to NULL */
    VIDTRAN_Data_p->Source.DecoderHandle = (STVID_Handle_t) NULL;

    /* Initialise transcoder queue */
    VIDTRAN_Data_p->CommandsBuffer.Data_p         = VIDTRAN_Data_p->CommandsData;
    VIDTRAN_Data_p->CommandsBuffer.TotalSize      = sizeof(VIDTRAN_Data_p->CommandsData);
    VIDTRAN_Data_p->CommandsBuffer.BeginPointer_p = VIDTRAN_Data_p->CommandsBuffer.Data_p;
    VIDTRAN_Data_p->CommandsBuffer.UsedSize       = 0;
    VIDTRAN_Data_p->CommandsBuffer.MaxUsedSize    = 0;

    /* Initialise the event notification configuration. */
    for (LoopCounter = 0; LoopCounter < VIDTRAN_NB_REGISTERED_EVENTS_IDS; LoopCounter++)
    {
        VIDTRAN_Data_p->EventNotificationConfiguration[LoopCounter].NotificationSkipped                     = 0;
        VIDTRAN_Data_p->EventNotificationConfiguration[LoopCounter].ConfigureEventParam.NotificationsToSkip = 0;
        VIDTRAN_Data_p->EventNotificationConfiguration[LoopCounter].ConfigureEventParam.Enable              = TRUE;
    }

    /* Ouput buffer --------------------------------------------------------- */
    VIDTRAN_Data_p->TranscoderOutputBuffer.IsAllocatedByUs = FALSE;
    VIDTRAN_Data_p->FreeSpaceInOutputBufferSem_p = STOS_SemaphoreCreateFifoTimeOut(NULL, 0);

#ifdef STVID_DEBUG_GET_STATISTICS
    /* Statistics ----------------------------------------------------------- */
    InitStatisticsTranscoderStructure(TranscoderHandle);
#endif /* STVID_DEBUG_GET_STATISTICS */

    /* Task ------------------------------------------------------------------*/
    VIDTRAN_Data_p->TranscoderState = VIDTRAN_TRANSCODER_STATE_STOPPED;
    VIDTRAN_Data_p->TranscoderTask.IsRunning  = FALSE;
#ifdef STVID_MEASURES
    VIDTRAN_Data_p->TranscoderOrder_p = STOS_SemaphoreCreateFifo(NULL, 0);
#else /* STVID_MEASURES */
    VIDTRAN_Data_p->TranscoderOrder_p = STOS_SemaphoreCreateFifoTimeOut(NULL, 0);
#endif /* not STVID_MEASURES */

    /* Semaphore for the stop operations */
    VIDTRAN_Data_p->TranscoderStop_p = STOS_SemaphoreCreateFifoTimeOut(NULL, 0);

    /* Initialise commands queue */
    VIDTRAN_Data_p->CommandsBuffer.Data_p         = VIDTRAN_Data_p->CommandsData;
    VIDTRAN_Data_p->CommandsBuffer.TotalSize      = sizeof(VIDTRAN_Data_p->CommandsData);
    VIDTRAN_Data_p->CommandsBuffer.BeginPointer_p = VIDTRAN_Data_p->CommandsBuffer.Data_p;
    VIDTRAN_Data_p->CommandsBuffer.UsedSize       = 0;
    VIDTRAN_Data_p->CommandsBuffer.MaxUsedSize    = 0;

    /* Default parameters for transcode --------------------------------------*/
    VIDTRAN_Data_p->XCODE_TranscodePictureParams.TargetFrameRate        = 15000;   /* Default: 15 Frame/s */
    VIDTRAN_Data_p->XCODE_TranscodePictureParams.TargetBitRate          = 1500000; /* Default: 1.5MBit/s */
    VIDTRAN_Data_p->XCODE_TranscodePictureParams.RateControlMode        = STVID_RATE_CONTROL_MODE_NONE;
    VIDTRAN_Data_p->XCODE_TranscodePictureParams.SpeedMode              = XCODER_MODE_SPEED;

    /* Init other parameters -------------------------------------------------*/
    VIDTRAN_Data_p->XCODE_TranscodePictureParams.MainPictureBufferIsValid = FALSE;
    VIDTRAN_Data_p->XCODE_TranscodePictureParams.DecimatedPictureBufferIsValid = FALSE;

    VIDTRAN_Data_p->TranscoderMode = STVID_ENCODER_MODE_SLAVE;
    VIDTRAN_Data_p->ErrorRecoveryMode = STVID_ENCODER_MODE_ERROR_RECOVERY_NONE;

    VIDTRAN_Data_p->OutputStreamProfile = STVID_ENCODER_OUTPUT_H264_BASELINE_PROFILE;

    VIDTRAN_Data_p->IsLastFlush = FALSE;

#if 1 /* Firmware Term workaround */
    VIDTRAN_Data_p->IsTranscodeOccur = FALSE;
#endif


} /* end of InitTranscoderStructure() */

/*******************************************************************************
Name        : StartTranscoderTask
Description : Start the task of transcode control
Parameters  : Video transcoder handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_ALREADY_INITIALIZED if task already running
              ST_ERROR_BAD_PARAMETER if problem of creation
*******************************************************************************/
static ST_ErrorCode_t StartTranscoderTask(const VIDTRAN_Handle_t TranscoderHandle)
{
    VIDCOM_Task_t *Task_p = &(((VIDTRAN_Data_t *) TranscoderHandle)->TranscoderTask);
    char TaskName[24] = "STVID[?].TranscoderTask";
    ST_ErrorCode_t Error;

    if (Task_p->IsRunning)
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* In TaskName string, replace '?' with transcoder number */
    TaskName[6] = (char) ((U8) '0' + ((VIDTRAN_Data_t *) TranscoderHandle)->DecoderNumber);
    Task_p->ToBeDeleted = FALSE;
#ifdef STVID_MEASURES
    Error = STOS_TaskCreate((void (*) (void*)) TranscoderTaskLoop,
                              (void *) TranscoderHandle,
                              ((VIDTRAN_Data_t *) TranscoderHandle)->CPUPartition_p,
                              STVID_TASK_STACK_SIZE_DECODE,
                              &(Task_p->TaskStack),
                              ((VIDTRAN_Data_t *) TranscoderHandle)->CPUPartition_p,
                              &(Task_p->Task_p),
                              &(Task_p->TaskDesc),
                              STVID_TASK_PRIORITY_DECODE,
                              TaskName, 
                              task_flags_high_priority_process | task_flags_no_min_stack_size);
#else /* STVID_MEASURES */
    Error = STOS_TaskCreate((void (*) (void*)) TranscoderTaskLoop,
                              (void *) TranscoderHandle,
                              ((VIDTRAN_Data_t *) TranscoderHandle)->CPUPartition_p,
                              STVID_TASK_STACK_SIZE_DECODE,
                              &(Task_p->TaskStack),
                              ((VIDTRAN_Data_t *) TranscoderHandle)->CPUPartition_p,
                              &(Task_p->Task_p),
                              &(Task_p->TaskDesc),
                              STVID_TASK_PRIORITY_DECODE,
                              TaskName,
                              task_flags_no_min_stack_size);
#endif /* not STVID_MEASURES */

    if (Error != ST_NO_ERROR)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Task_p->IsRunning  = TRUE;
/*    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "transcoder task created."));*/
    return(ST_NO_ERROR);
} /* End of StartTranscoderTask() function */

/*******************************************************************************
Name        : StopTranscoderTask
Description : Stop the task of transcoder control
Parameters  : Video transcoder handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if no problem,
              ST_ERROR_ALREADY_INITIALIZED if task not running
*******************************************************************************/
static ST_ErrorCode_t StopTranscoderTask(const VIDTRAN_Handle_t TranscoderHandle)
{
    VIDCOM_Task_t * const Task_p = &(((VIDTRAN_Data_t *) TranscoderHandle)->TranscoderTask);
    task_t *TaskList_p = Task_p->Task_p;

    if (!(Task_p->IsRunning))
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* End the function of the task here... */
    Task_p->ToBeDeleted = TRUE;

    /* Signal semaphore to release task that may wait infinitely if stopped */
    STOS_SemaphoreSignal(((VIDTRAN_Data_t *) TranscoderHandle)->TranscoderOrder_p);

    STOS_TaskWait(&TaskList_p, TIMEOUT_INFINITY);
    STOS_TaskDelete ( Task_p->Task_p,
                      ((VIDTRAN_Data_t *) TranscoderHandle)->CPUPartition_p,
                      &Task_p->TaskStack,
                      ((VIDTRAN_Data_t *) TranscoderHandle)->CPUPartition_p);

    Task_p->IsRunning  = FALSE;

    return(ST_NO_ERROR);
} /* End of StopTranscoderTask() function */

/*******************************************************************************
Name        : TermTranscoderStructure
Description : Terminate transcoder structure
Parameters  : Video transcoder handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void TermTranscoderStructure(const VIDTRAN_Handle_t TranscoderHandle)
{
    VIDTRAN_Data_t * VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;

    /* Source decoder is no longer linked. Set to NULL */
    VIDTRAN_Data_p->Source.DecoderHandle = (STVID_Handle_t) NULL;

    STOS_SemaphoreDelete(NULL, VIDTRAN_Data_p->TranscoderOrder_p);
    STOS_SemaphoreDelete(NULL, VIDTRAN_Data_p->TranscoderStop_p);

} /* end of TermTranscoderStructure() */

/*******************************************************************************
Name        : TranscoderTaskLoop
Description : Main Loop for the transcoder task
              Set VIDTRAN_Data_p->TranscoderTask.ToBeDeleted to end the task
Parameters  : Video transcoder handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void TranscoderTaskLoop(const VIDTRAN_Handle_t TranscoderHandle)
{
    VIDTRAN_Data_t * VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;

    STOS_TaskEnter(NULL);

    /* Big loop executed until ToBeDeleted is set --------------------------- */
    do
    { /* while not ToBeDeleted */

        if (  (VIDTRAN_Data_p->TranscoderState == VIDTRAN_TRANSCODER_STATE_STOPPED)
           && (!(VIDTRAN_Data_p->TranscoderTask.ToBeDeleted)))
        {
            /* Transcoder stopped: there is no monitoring to do, so semaphore waits infinitely.
            This avoid task switching not really needed when video stopped. */
            STOS_SemaphoreWait(VIDTRAN_Data_p->TranscoderOrder_p);
        }
        else
        {
            /* Transcoder running: semaphore has normal time-out to do polling of actions even if semaphore was not signaled. */
            VIDTRAN_Data_p->ForTask.MaxWaitOrderTime = STOS_time_plus(STOS_time_now(), MAX_WAIT_ORDER_TIME);
            /* Check if an order is available */
            STOS_SemaphoreWaitTimeOut(VIDTRAN_Data_p->TranscoderOrder_p, &(VIDTRAN_Data_p->ForTask.MaxWaitOrderTime));
        }

        while (STOS_SemaphoreWaitTimeOut(VIDTRAN_Data_p->TranscoderOrder_p, TIMEOUT_IMMEDIATE) == 0)
        {
            /* Nothing to do: just to decrement counter until 0, to avoid multiple looping on semaphore wait */;
        }

        if (   (VIDTRAN_Data_p->TranscoderState != VIDTRAN_TRANSCODER_STATE_STOPPED)
            && (!VIDTRAN_Data_p->TranscoderTask.ToBeDeleted))
        {
            TranscoderTaskFunc(TranscoderHandle);
        }   /* if CallTask */
    } while (!(VIDTRAN_Data_p->TranscoderTask.ToBeDeleted));

    STOS_TaskExit(NULL);

} /* End of TranscoderTaskLoop() function */

/*******************************************************************************
Name        : TranscoderTaskFunc
Description : Function fo the Transcoder task
Parameters  : Video transcoder handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void TranscoderTaskFunc(const VIDTRAN_Handle_t TranscoderHandle)
{
    /* Overall variables of the task */
    VIDTRAN_Data_t *                      VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;
    ST_ErrorCode_t                        ErrorCode = ST_NO_ERROR;
    U8                                    Tmp8;
    VIDCOM_PictureGenericData_t *         PictureGenericData_p;
    BOOL                                  IsTranscodeNeeded = FALSE;
    BOOL                                  TranscodeDone;
    VIDTRAN_Picture_t                     *TrCurrentPicture_p;
    STVID_TranscoderError_t               ErrorType;

    /* Process all commands together only if no start code commands... */
    while ((ErrorCode = PopTranscoderCommand(TranscoderHandle, &Tmp8)) == ST_NO_ERROR)
    {
        switch ((TranscoderCommandID_t) Tmp8)
        {
            case TRANSCODER_NEW_SOURCE_PICTURE_AVAILABLE :
                /* Retrieving the pictures from queue. */
                /* Element to be released are not viewed (already removed but from queue) */
                TrCurrentPicture_p = VIDTRAN_Data_p->Queue_p; /* First picture in queue */
                if (TrCurrentPicture_p == NULL)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Transcode: Queue has no element to be processed ! \n"));
                }
                else
                {
                    PictureGenericData_p = (VIDCOM_PictureGenericData_t *) &(TrCurrentPicture_p->MainPictureBuffer_p->ParsedPictureInformation.PictureGenericData);

                    switch (VIDTRAN_Data_p->TranscoderState)
                    {
                        case VIDTRAN_TRANSCODER_STATE_STARTING :
                            /* Waiting for a I picture */
                            if (PictureGenericData_p->PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)
                            {
                                /* An I image is now available in buffers */
                                VIDTRAN_Data_p->ReferenceStatus = VIDTRAN_AVAILABLE_REFERENCE_I;
                                VIDTRAN_Data_p->TranscoderState = VIDTRAN_TRANSCODER_STATE_RUNNING;

                                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Transcoding has started"));

#ifdef STVID_DEBUG_GET_STATISTICS
                                LastEndOfTranscodeTime = STOS_time_now();
#endif /* STVID_DEBUG_GET_STATISTICS */

                                /* Command to transcode picture */
                                PushTranscoderCommand(TranscoderHandle, TRANSCODER_NEW_SOURCE_PICTURE_AVAILABLE);
                                STOS_SemaphoreSignal(VIDTRAN_Data_p->TranscoderOrder_p);

                                /* Rem: Picture is kept on transcode queue */
                            }
                            else
                            {
                                /* Picture is removed from queue. It is the first element */
                                vidtrqueue_RemovePicturesFromTranscoderQueue(TranscoderHandle, TrCurrentPicture_p);
#ifdef STVID_DEBUG_GET_STATISTICS
                                VIDTRAN_Data_p->Statistics.EncodePicturesSkippedBeforeStart++;
#endif /* STVID_DEBUG_GET_STATISTICS */
                            }

                        break;

                        case VIDTRAN_TRANSCODER_STATE_RUNNING :
                        case VIDTRAN_TRANSCODER_STATE_STOP_WHEN_QUEUE_EMPTY :
                        case VIDTRAN_TRANSCODER_STATE_RESUMED :

                            IsTranscodeNeeded = FALSE;

                            /* Update reference list. Mark pictures for release. */
                            /*vidtran_UpdateReferenceList(TranscoderHandle, TrCurrentPicture_p);*/

                            /* Mark pictures for release. Release now needed ? */

                            /* Check picture and queue to determinate if the picture has to be transcoded */
                            IsTranscodeNeeded = vidtran_CheckPictureEligibilityForTranscode(TranscoderHandle, PictureGenericData_p)
                                             && vidtran_RegulateTranscodeQueue(TranscoderHandle, TrCurrentPicture_p);

                            TranscodeDone = FALSE;

                            /* Checking whether a transcode is needed */
                            /* Prepare the transcode of the current picture in queue */
                            if (IsTranscodeNeeded == TRUE)
                            {
#ifdef STVID_DEBUG_GET_STATISTICS
                                VIDTRAN_Data_p->Statistics.EncodePicturesTranscodeNedeed++;
#endif /* STVID_DEBUG_GET_STATISTICS */

                                ErrorCode = vidtran_TranscodePicture(TranscoderHandle, TrCurrentPicture_p);
                                if (ErrorCode == ST_NO_ERROR)
                                {
                                    TranscodeDone = TRUE;
#ifdef STVID_DEBUG_GET_STATISTICS
                                    VIDTRAN_Data_p->Statistics.EncodePicturesTranscoded++;

                                    /* Calculating # of pictures transcoded/second */
                                    CountEndOfTranscode++;
                                    if (CountEndOfTranscode >= TRANSCODE_SPEED_WINDOW)
                                    {
                                        CurrentEndOfTranscodeTime = STOS_time_now();
                                        DiffEndOfTranscodeTime = STOS_time_minus(CurrentEndOfTranscodeTime, LastEndOfTranscodeTime);
                                        if (DiffEndOfTranscodeTime != 0)
                                        {
                                            VIDTRAN_Data_p->Statistics.EncodePicturesAverageSpeed = (TRANSCODE_SPEED_WINDOW * ST_GetClocksPerSecond()) / DiffEndOfTranscodeTime;
                                        }
                                        else
                                        {
                                            /* Too fast ! */
                                            VIDTRAN_Data_p->Statistics.EncodePicturesAverageSpeed = -1;
                                        }
                                        LastEndOfTranscodeTime = CurrentEndOfTranscodeTime;
                                        CountEndOfTranscode = 0;
                                    }
#endif /* STVID_DEBUG_GET_STATISTICS */

                                    /* ---- Notifying a new picture has been transcoded ---- */
                                    /*      Disabled until this event is really needed       */
                                    /* WARNING: USE WITH CARE ! -> Possible delays in the CB */
                                    STEVT_Notify(VIDTRAN_Data_p->EventsHandle,
                                                 VIDTRAN_Data_p->RegisteredEventsID[STVID_TRANSCODER_NEW_PICTURE_TRANSCODED_EVT_ID],
                                                 STEVT_EVENT_DATA_TYPE_CAST &(TrCurrentPicture_p->MainPictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos));
                                }
                                else
                                {
                                    /* If we are running, stalling transcoder else let it go */
                                    if (VIDTRAN_Data_p->TranscoderState == VIDTRAN_TRANSCODER_STATE_RUNNING)
                                    {
                                        /* If running, we stall it */
                                        VIDTRAN_Data_p->TranscoderState = VIDTRAN_TRANSCODER_STATE_STALLED;
                                    }

                                    if (VIDTRAN_Data_p->TranscoderState == VIDTRAN_TRANSCODER_STATE_STOP_WHEN_QUEUE_EMPTY)
                                    {
                                        /* If running, we stall it */
                                        VIDTRAN_Data_p->TranscoderState = VIDTRAN_TRANSCODER_STATE_WAITING;
                                    }

                                    /* We must do another attempt later */
                                    PushTranscoderCommand(TranscoderHandle, TRANSCODER_NEW_SOURCE_PICTURE_AVAILABLE);
                                    STOS_SemaphoreSignal(VIDTRAN_Data_p->TranscoderOrder_p);

                                    /* Notifying transcoder has output buffer full */
                                    ErrorType = STVID_ENCODER_ERROR_OUTPUT_BUFFER_FULL;
                                    STEVT_Notify(VIDTRAN_Data_p->EventsHandle,
                                                 VIDTRAN_Data_p->RegisteredEventsID[STVID_TRANSCODER_ERROR_EVT_ID],
                                                 STEVT_EVENT_DATA_TYPE_CAST &(ErrorType));

                                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Transcode: No memory for trancoded picture. Transcoder Stopped ! \r\n"));
                                }
                            }

                            /* Removing the picture transcoded from queue except if transcode was requested and not done */
                            if (!((IsTranscodeNeeded == TRUE) && (TranscodeDone == FALSE)))
                            {
                                vidtrqueue_RemovePicturesFromTranscoderQueue(TranscoderHandle, TrCurrentPicture_p);
                            }

                        break;

                        case VIDTRAN_TRANSCODER_STATE_STALLED :
                            /* Checking whether transcode can resume or not */
                            if ((vidtran_UpdateTranscoderOutputBuffer(TranscoderHandle)) == FALSE)
                            {
                                /* Some space is available in output buffer. Resuming transcoder */
                                VIDTRAN_Data_p->TranscoderState = VIDTRAN_TRANSCODER_STATE_RUNNING;
                            }
                            else
                            {
                                /* Things are still bad. Urgently stopping transcoder task */
                                VIDTRAN_Data_p->TranscoderState = VIDTRAN_TRANSCODER_STATE_STOPPED;
                                ErrorType = STVID_ENCODER_ERROR_URGENTLY_STOPPED | STVID_ENCODER_ERROR_OUTPUT_BUFFER_FULL;

                                /* Notifying transcoder has stopped */
                                STEVT_Notify(VIDTRAN_Data_p->EventsHandle,
                                             VIDTRAN_Data_p->RegisteredEventsID[STVID_TRANSCODER_ERROR_EVT_ID],
                                             STEVT_EVENT_DATA_TYPE_CAST &(ErrorType));
                            }
                        break;

                        case VIDTRAN_TRANSCODER_STATE_WAITING :
                            /* Checking whether transcode can resume or not */
                            if ((vidtran_UpdateTranscoderOutputBuffer(TranscoderHandle)) == FALSE)
                            {
                                /* Some space is available in output buffer. Resuming transcoder */
                                VIDTRAN_Data_p->TranscoderState = VIDTRAN_TRANSCODER_STATE_RESUMED;
                            }
                            else
                            {
                                /* Things are still bad but no extra transcode needed as are flushing */
                                /* the queue, so we can wait */
                            }
                        break;

                        case VIDTRAN_TRANSCODER_STATE_STOP_AND_FLUSH :
                            /* Remove picture from queue, do not process it and mark it for release */
                            vidtrqueue_RemovePicturesFromTranscoderQueue(TranscoderHandle, TrCurrentPicture_p);

                        break;

                        case VIDTRAN_TRANSCODER_STATE_STOPPED :
                            /* Task should not run with these states */

                            /* Notifying transcoder has a strange state */
                            ErrorType = STVID_ENCODER_ERROR_WRONG_STATE;
                            STEVT_Notify(VIDTRAN_Data_p->EventsHandle,
                                         VIDTRAN_Data_p->RegisteredEventsID[STVID_TRANSCODER_ERROR_EVT_ID],
                                         STEVT_EVENT_DATA_TYPE_CAST &(ErrorType));

                            TraceVerbose(("Transcode: State stopped !\r\n"));
                        break;

                        default :
                            /* Notifying transcoder has an undefined state */
                            ErrorType = STVID_ENCODER_ERROR_UNDEFINED_STATE;
                            STEVT_Notify(VIDTRAN_Data_p->EventsHandle,
                                         VIDTRAN_Data_p->RegisteredEventsID[STVID_TRANSCODER_ERROR_EVT_ID],
                                         STEVT_EVENT_DATA_TYPE_CAST &(ErrorType));

                            TraceVerbose(("Transcode: Transcoder State undefined ! \r\n"));
                        break;
                    }
                }
                /* Releasing the pictures marked for it */
                vidtrqueue_ReleasePicturesFromTranscoderQueue(TranscoderHandle);

            break;

            case TRANSCODER_FLUSH_CHECK_QUEUE_AND_STOP :

                    switch (VIDTRAN_Data_p->TranscoderState)
                    {
                        case VIDTRAN_TRANSCODER_STATE_WAITING :
                        case VIDTRAN_TRANSCODER_STATE_RESUMED :
                            /* Skipping this command further images need a transcode */
                            /* Re-send a command at the end of transcoder command queue */
                            PushTranscoderCommand(TranscoderHandle, TRANSCODER_FLUSH_CHECK_QUEUE_AND_STOP);
                            STOS_SemaphoreSignal(VIDTRAN_Data_p->TranscoderOrder_p);
                        break;

                        case VIDTRAN_TRANSCODER_STATE_STALLED :
                            /* We have stalled before. One image is to be transcoded */
                            /* We stop stop */

                        case VIDTRAN_TRANSCODER_STATE_STOP_WHEN_QUEUE_EMPTY :
                        case VIDTRAN_TRANSCODER_STATE_STOP_AND_FLUSH :
                            /* Force Queue to empty */
                            while ((TrCurrentPicture_p = VIDTRAN_Data_p->Queue_p) != NULL) /* First picture in queue */
                            {
                                /* Removing the first picture from queue. Mark for release */
                                vidtrqueue_RemovePicturesFromTranscoderQueue(TranscoderHandle, TrCurrentPicture_p);
                            }

                            /* Releasing the pictures marked for it, if any remaining */
                            vidtrqueue_ReleasePicturesFromTranscoderQueue(TranscoderHandle);

                            /* Force TranscoderState */
                            VIDTRAN_Data_p->TranscoderState = VIDTRAN_TRANSCODER_STATE_STOPPED;

                            /* Signaling stop command has stopped */
                            STOS_SemaphoreSignal(VIDTRAN_Data_p->TranscoderStop_p);

                            /* Flushing the Transcoded output buffer. Last flush */
                            vidtran_FlushOutputTranscodedBuffer(TranscoderHandle, TRUE);

                            /* Notifying transcoder has stopped */
                            STEVT_Notify(VIDTRAN_Data_p->EventsHandle,
                                         VIDTRAN_Data_p->RegisteredEventsID[STVID_TRANSCODER_STOPPED_EVT_ID],
                                         NULL);

                            /* Report stop */
                            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Transcoder stopped and flushed."));

                        break;

                        default:
                            /* Notifying transcoder has a strange state */
                            ErrorType = STVID_ENCODER_ERROR_WRONG_STATE;
                            STEVT_Notify(VIDTRAN_Data_p->EventsHandle,
                                         VIDTRAN_Data_p->RegisteredEventsID[STVID_TRANSCODER_ERROR_EVT_ID],
                                         STEVT_EVENT_DATA_TYPE_CAST &(ErrorType));

                        break;
                    }

            break;

            default:
                /* Notifying transcoder has received an undefined command */
                ErrorType = STVID_ENCODER_ERROR_UNDEFINED_COMMAND;
                STEVT_Notify(VIDTRAN_Data_p->EventsHandle,
                             VIDTRAN_Data_p->RegisteredEventsID[STVID_TRANSCODER_ERROR_EVT_ID],
                             STEVT_EVENT_DATA_TYPE_CAST &(ErrorType));

                TraceVerbose(("Transcode: Command not understood ! \r\n"));
            break;

        } /* end switch(command) */
    } /* end while(controller command) */

    /* Do actions at each loop, not depending on orders (comes here at least each MAX_WAIT_ORDER_TIME ticks */

    /* To do */


} /* End of TranscoderTaskFunc() function */

/*******************************************************************************
Name        : vidtran_ResetReferenceList
Description : resets the reference list for the current transcode
Parameters  : Video transcoder handle
Assumptions :
Limitations :
Returns     : TRUE is Transcoding is needed
*******************************************************************************/
static void vidtran_ResetReferenceList(const VIDTRAN_Handle_t TranscoderHandle)
{
    VIDTRAN_Data_t *  VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;

    UNUSED_PARAMETER(VIDTRAN_Data_p);

/*
    memset( &VIDTRAN_Data_p->ReferencesPictureBufferPointer, NULL, sizeof(VIDTRAN_Data_p->ReferencesPictureBufferPointer[VIDTRAN_MAX_NUMBER_OF_REFERENCES_FRAMES]));
*/

    return;
}

/*******************************************************************************
Name        : vidtran_BuildReferenceList
Description : Build the reference list for the current picture
Parameters  : Video transcoder handle
Assumptions :
Limitations :
Returns     : TRUE is Transcoding is needed
*******************************************************************************/
static void vidtran_BuildReferenceList(const VIDTRAN_Handle_t TranscoderHandle, VIDTRAN_Picture_t * const TrCurrentPicture_p)
{
    UNUSED_PARAMETER(TranscoderHandle);
    UNUSED_PARAMETER(TrCurrentPicture_p);

    /* Checking reference for the current picture */

    /* Are these reference
    Parsing all pictures in queue */

    return;

}

/*******************************************************************************
Name        : vidtran_UpdateReferenceList
Description : Parsing the reference list to mark pictures for release if needed.
Parameters  : Video transcoder handle
Assumptions :
Limitations :
Returns     : TRUE is Transcoding is needed
*******************************************************************************/
static void vidtran_UpdateReferenceList(const VIDTRAN_Handle_t TranscoderHandle, VIDTRAN_Picture_t * const TrCurrentPicture_p)
{
    UNUSED_PARAMETER(TranscoderHandle);
    UNUSED_PARAMETER(TrCurrentPicture_p);

    /* Checking reference for the current picture */

    /* Are these reference
    Parsing all pictures in queue */

    return;
}

/*******************************************************************************
Name        : vidtran_CheckPictureEligibilityForTranscode
Description : Check if the picture must be skipped or not (old demo test)
Parameters  : Video transcoder handle, Picture generic data
Assumptions :
Limitations :
Returns     : TRUE is Transcoding is needed
*******************************************************************************/
static BOOL vidtran_CheckPictureEligibilityForTranscode(const VIDTRAN_Handle_t TranscoderHandle, VIDCOM_PictureGenericData_t * PictureGenericData_p)
{
    VIDTRAN_Data_t *                      VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;
    BOOL                                  IsTranscodeNeeded = TRUE;


    IsTranscodeNeeded =  !(SOURCE_IsPictureToBeSkipped(VIDTRAN_Data_p->Source.TranscodecHandle,
                                                       PictureGenericData_p->DecodingOrderFrameID,
                                                       PictureGenericData_p->PictureInfos.VideoParams.MPEGFrame,
                                                       PictureGenericData_p->PictureInfos.VideoParams.FrameRate,
                                                       VIDTRAN_Data_p->XCODE_TranscodePictureParams.TargetFrameRate)
                                                       );
#ifdef STVID_DEBUG_GET_STATISTICS
    if (IsTranscodeNeeded == FALSE)
    {
        VIDTRAN_Data_p->Statistics.EncodePicturesSkippedByNonEligibility++;
    }
#endif /* STVID_DEBUG_GET_STATISTICS */

    return (IsTranscodeNeeded);
}

/*******************************************************************************
Name        : vidtran_RegulateTranscodeQueue
Description : Monitor and regulate the transcode queue to keep the correct level.
Parameters  : Video transcoder handle
Assumptions :
Limitations :
Returns     : TRUE is Transcoding is needed
*******************************************************************************/
static BOOL vidtran_RegulateTranscodeQueue(const VIDTRAN_Handle_t TranscoderHandle, VIDTRAN_Picture_t * const TrCurrentPicture_p)
{
    VIDTRQUEUE_QueueInfo_t                TrQueue_Info;
    VIDTRAN_Data_t *                      VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;
    BOOL                                  IsTranscodeNeeded = TRUE;

    UNUSED_PARAMETER(TrCurrentPicture_p);

    /* Get information on pictures in transcoder queue */
    vidtrqueue_InformationOnPicturesInTranscoderQueue(TranscoderHandle, &TrQueue_Info);
#ifdef STVID_DEBUG_GET_STATISTICS
    /* Cumulative updates */
    VIDTRAN_Data_p->Statistics.EncodeQueueMaxPicturesQueued         = MAX_VALUE(VIDTRAN_Data_p->Statistics.EncodeQueueMaxPicturesQueued,TrQueue_Info.NbOfPicturesValidInQueue);
    VIDTRAN_Data_p->Statistics.EncodeQueueMaxPictureToBeTranscoded  = MAX_VALUE(VIDTRAN_Data_p->Statistics.EncodeQueueMaxPictureToBeTranscoded, TrQueue_Info.NbOfPicturesToBeTranscoded);
#endif /* STVID_DEBUG_GET_STATISTICS */

    switch(VIDTRAN_Data_p->TranscoderMode)
    {
        case STVID_ENCODER_MODE_SLAVE :
            /* Transcoder may underrun */
            if (TrQueue_Info.NbOfPicturesToBeTranscoded > LATE_PICTURE_SKIP_LEVEL)
            {
                /* Transcode is really late */
                TraceVerbose(("Transcode: Very late: Skipping picture ! \r\n"));

                /* This picture is skipped */
                IsTranscodeNeeded = FALSE;

#ifdef STVID_DEBUG_GET_STATISTICS
                VIDTRAN_Data_p->Statistics.EncodePicturesSkippedByQueueLevel++;
#endif /* STVID_DEBUG_GET_STATISTICS */

            }
            else if (TrQueue_Info.NbOfPicturesToBeTranscoded > LATE_PICTURE_ADJUST_LEVEL)
            {
                /* Transcode must be adjusted for next picture in order to recover latence */
                TraceVerbose(("Transcode: Adjusting parameters to speed up transcode ! \r\n"));

                /* ToDo: Changing bitrate or compression */

                /* This picture will be transcoded */
                IsTranscodeNeeded = TRUE;

#ifdef STVID_DEBUG_GET_STATISTICS
                VIDTRAN_Data_p->Statistics.EncodePicturesAdjustedByQueueLevel++;
#endif /* STVID_DEBUG_GET_STATISTICS */

            }
            else
            {
                /* Normal case. The picture is transcoded */
                IsTranscodeNeeded = TRUE;
            }
        break;

        case STVID_ENCODER_MODE_MASTER :
            /* Normal case. The picture is transcoded */
            IsTranscodeNeeded = TRUE;
        break;

        default:
            IsTranscodeNeeded = TRUE;
        break;
    }

    return (IsTranscodeNeeded);
}

/*******************************************************************************
Name        : vidtran_TranscodePicture
Description : Prepare and transcode a Picture.
Parameters  : Video transcoder handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
static ST_ErrorCode_t vidtran_TranscodePicture(const VIDTRAN_Handle_t TranscoderHandle, const VIDTRAN_Picture_t * const TrPictureToTranscode_p)
{
    VIDTRAN_Data_t *                      VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;
    BOOL                                  IsOutputBufferFull;

    /* Filling structure of decoded picture buffer address */
    memcpy(&(VIDTRAN_Data_p->XCODE_TranscodePictureParams.MainPictureBuffer), TrPictureToTranscode_p->MainPictureBuffer_p, sizeof(VIDBUFF_PictureBuffer_t));

    VIDTRAN_Data_p->XCODE_TranscodePictureParams.MainPictureBufferIsValid = TRUE; /* Needed ? What for ? */

    /* Checks if a decimated picture is here */
    if (TrPictureToTranscode_p->DecimatedPictureBuffer_p != NULL)
    {
        /* Filling structure of decoded decimated picture buffer address */
        memcpy(&(VIDTRAN_Data_p->XCODE_TranscodePictureParams.DecimatedPictureBuffer), TrPictureToTranscode_p->DecimatedPictureBuffer_p, sizeof(VIDBUFF_PictureBuffer_t));

        VIDTRAN_Data_p->CurrentPictureToTranscode.Width = VIDTRAN_Data_p->XCODE_TranscodePictureParams.DecimatedPictureBuffer.ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Width;
        VIDTRAN_Data_p->CurrentPictureToTranscode.Height = VIDTRAN_Data_p->XCODE_TranscodePictureParams.DecimatedPictureBuffer.ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Height;

        VIDTRAN_Data_p->XCODE_TranscodePictureParams.DecimatedPictureBufferIsValid = TRUE;
    }
    else
    {
        VIDTRAN_Data_p->CurrentPictureToTranscode.Width = VIDTRAN_Data_p->XCODE_TranscodePictureParams.MainPictureBuffer.ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Width;
        VIDTRAN_Data_p->CurrentPictureToTranscode.Height = VIDTRAN_Data_p->XCODE_TranscodePictureParams.MainPictureBuffer.ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Height;

        VIDTRAN_Data_p->XCODE_TranscodePictureParams.DecimatedPictureBufferIsValid = FALSE;
    }

    /* Handle TranscoderOutputBuffer. Flush it on disk if necessary */
    IsOutputBufferFull = vidtran_UpdateTranscoderOutputBuffer(TranscoderHandle);

    if (IsOutputBufferFull == TRUE)
    {
        /* No space left on output buffer */
        return (ST_ERROR_NO_MEMORY);
    }

#ifdef TRANSCODE_PES_OUTPUT
    /* Handle PES if needed */
    IsOutputBufferFull = vidtran_AddPESHeaderInTranscoderOutputBuffer(TranscoderHandle);
    if (IsOutputBufferFull == TRUE)
    {
        /* No space left on output buffer */
        return (ST_ERROR_NO_MEMORY);
    }
#endif /* TRANSCODE_PES_OUTPUT */

    /* Write pointer given to the firmware */
    VIDTRAN_Data_p->XCODE_TranscodePictureParams.Write_p = VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p;

    /* Calling the firmware for transcoding the picture */
    XCODE_TranscodePicture(VIDTRAN_Data_p->XcodeHandle, &(VIDTRAN_Data_p->XCODE_TranscodePictureParams));

#if 1 /* Firmware Term workaround */
    VIDTRAN_Data_p->IsTranscodeOccur = TRUE;
#endif

    return (ST_NO_ERROR);
} /* End of vidtran_TranscodePicture() function */

#ifdef TRANSCODE_PES_OUTPUT
/*******************************************************************************
Name        : vidtran_AddPESHeaderInTranscoderOutputBuffer
Description : Adds the PES header before transcoding the Picture
Parameters  :
Assumptions :
Limitations :
Returns     : TRUE if PES header was written, else FALSE
*******************************************************************************/
static BOOL vidtran_AddPESHeaderInTranscoderOutputBuffer(const VIDTRAN_Handle_t TranscoderHandle)
{
    VIDTRAN_Data_t *    VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;
    STVID_PTS_t         CurrentPTS;
    BOOL                IsBufferFull = FALSE;
    U32                 PTS_Temp;

    /* Retrieving current PTS structure */
    CurrentPTS = VIDTRAN_Data_p->XCODE_TranscodePictureParams.MainPictureBuffer.ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PTS;

    if (CurrentPTS.IsValid == TRUE)
    {
        /* Indicating we have a PTS */
        TranscoderOutputPESHeader.PES_Flag = TranscoderOutputPESHeader.PES_Flag | 0x80000000; /* PTS, no DTS */
        TranscoderOutputPESHeader.PES_header_data_length = 0x05; /* 5 bytes for PTS */
    }
    else
    {
        /* No PTS */
        TranscoderOutputPESHeader.PES_Flag = TranscoderOutputPESHeader.PES_Flag & ~0xC0000000; /* no PTS, no DTS */
        TranscoderOutputPESHeader.PES_header_data_length = 0x00; /* no byte for option */
    }

    /* Writing bytes in Transcoded Output buffer */
    STOS_memcpy(STAVMEM_DeviceToCPU(VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p, &VIDTRAN_Data_p->VirtualMapping),
                    &TranscoderOutputPESHeader,
                    sizeof(VIDTRAN_PES_Header_t));

    /* Updating pointers */
    VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p = (void *) ((U32)VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p + sizeof(VIDTRAN_PES_Header_t));
    if (VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p >= VIDTRAN_Data_p->TranscoderOutputBuffer.Top_p)
    {
        /* PES header wrote over the top. Should not happen !*/
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Transcode: Output buffer overflow (PES header)!"));
        return (TRUE);
    }

    if (CurrentPTS.IsValid == TRUE)
    {
        /* Writing PTS in PES header structure    */
        /* Bits 32 to 30 !! Changing indianess !! */
        PTS_Temp = (CurrentPTS.PTS >> PTS32TO30_SHIFT) & PTS32TO30_MASK;
        TranscoderOutputPESHeaderPTS.H_PTS_32_30_MarkerBit1 = PTSHEADER_0010 << 4
                                                            | (CurrentPTS.PTS33 ? (1 << PTS33_SHIFT) : 0)
                                                            | (PTS_Temp < 1)
                                                            | PTSMARKERBIT;
        /* Bits 29 to 15 !! Changing indianess !! */
        PTS_Temp = (CurrentPTS.PTS >> PTS29TO15_SHIFT) & PTS29TO15_MASK;
        TranscoderOutputPESHeaderPTS.PTS_29_15_marker_bit2 = (((PTS_Temp << 1) | PTSMARKERBIT) & 0xFF) << 8
                                                           | (((PTS_Temp << 1) | PTSMARKERBIT) & 0xFF00) >> 8;
        /* Bits 14 to 0 !! Changing indianess !! */
        PTS_Temp = (CurrentPTS.PTS >> PTS14TO0_SHIFT) & PTS14TO0_MASK;
        TranscoderOutputPESHeaderPTS.PTS_14_0_marker_bit3 = (((PTS_Temp << 1) | PTSMARKERBIT) & 0xFF) << 8
                                                          | (((PTS_Temp << 1) | PTSMARKERBIT) & 0xFF00) >> 8;
        /* Writing bytes in Transcoded Output buffer */
        STOS_memcpy(STAVMEM_DeviceToCPU(VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p, &VIDTRAN_Data_p->VirtualMapping),
                        &TranscoderOutputPESHeaderPTS,
                        sizeof(VIDTRAN_PES_Header_PTS_t));

        /* Updating pointers */
        VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p = (void *) ((U32)VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p + sizeof(VIDTRAN_PES_Header_PTS_t));
        if (VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p >= VIDTRAN_Data_p->TranscoderOutputBuffer.Top_p)
        {
            /* PES header wrote over the top. Should not happen !*/
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Transcode: Output buffer overflow (PES header PTS)!"));
            return (TRUE);
        }
    }

    return (IsBufferFull);
}
#endif /* TRANSCODE_PES_OUTPUT */

/*******************************************************************************
Name        : vidtran_MonitorOutputBuffer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void vidtran_MonitorOutputBuffer(const VIDTRAN_Handle_t TranscoderHandle, VIDTRAN_MonitorOutputBufferParams_t * const MonitorOutputBufferParams_p)
{
    VIDTRAN_Data_t * VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;

  	MonitorOutputBufferParams_p->BaseAddress_p  = VIDTRAN_Data_p->TranscoderOutputBuffer.Base_p;
	MonitorOutputBufferParams_p->Size           = VIDTRAN_Data_p->TranscoderOutputBuffer.Size;
	MonitorOutputBufferParams_p->Write_p        = VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p;

    if (VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p < VIDTRAN_Data_p->TranscoderOutputBuffer.Read_p)
    {
        /* Write pointer has done a loop, not the Read pointer */
        MonitorOutputBufferParams_p->OutputBufferLevelInBytes =
              (U32) VIDTRAN_Data_p->TranscoderOutputBuffer.Loop_p
            - (U32) VIDTRAN_Data_p->TranscoderOutputBuffer.Read_p
            + (U32) VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p
            - (U32) VIDTRAN_Data_p->TranscoderOutputBuffer.Base_p
            + 1;
    }
    else
    {
        /* Normal: start <= read <= write <= top */
        MonitorOutputBufferParams_p->OutputBufferLevelInBytes =
             (U32) VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p
           - (U32) VIDTRAN_Data_p->TranscoderOutputBuffer.Read_p;
    }

    return;
}

/*******************************************************************************
Name        : vidtran_InformReadAddress
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t vidtran_InformReadAddress(
                                const VIDTRAN_Handle_t TranscoderHandle,
                                void * const Read_p)
{
    VIDTRAN_Data_t * VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    VIDTRAN_Data_p->TranscoderOutputBuffer.Read_p = Read_p;
    /* Force immediate reschedule of transcoder task if new read pointer frees transcoding */
    /* LM: Useless ??? */
    STOS_SemaphoreSignal(VIDTRAN_Data_p->TranscoderOrder_p);

    return(ErrorCode);
}

/*******************************************************************************
Name        : vidtran_FlushOutputTranscodedBuffer
Description : Flushes Transcoded output buffer
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void vidtran_FlushOutputTranscodedBuffer(const VIDTRAN_Handle_t TranscoderHandle, const BOOL IsLastFlush)
{
    VIDTRAN_Data_t              * VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;
    STVID_PacketFlushParam_t    PacketFlushParam;

    PacketFlushParam.IsLastFlush = FALSE;

    if (VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p < VIDTRAN_Data_p->TranscoderOutputBuffer.Read_p)
    {
        /* Output buffer has looped. Notifying two flushes */
        PacketFlushParam.PacketStartAddress_p = VIDTRAN_Data_p->TranscoderOutputBuffer.Read_p;
        PacketFlushParam.PacketStopAddress_p = VIDTRAN_Data_p->TranscoderOutputBuffer.Loop_p;

        /* Flushing data to the top */
        STEVT_Notify(VIDTRAN_Data_p->EventsHandle,
                     VIDTRAN_Data_p->RegisteredEventsID[STVID_TRANSCODER_FLUSH_TRANSCODED_BUFFER_EVT_ID],
                     STEVT_EVENT_DATA_TYPE_CAST &(PacketFlushParam));

#ifdef STVID_DEBUG_GET_STATISTICS
        VIDTRAN_Data_p->Statistics.EncodeNbOfOutputBufferBytesFlushed += (U32)PacketFlushParam.PacketStopAddress_p - (U32)PacketFlushParam.PacketStartAddress_p;
#endif /* STVID_DEBUG_GET_STATISTICS */

        /* Bottom part */
        PacketFlushParam.PacketStartAddress_p = VIDTRAN_Data_p->TranscoderOutputBuffer.Base_p;
        PacketFlushParam.PacketStopAddress_p = VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p;

        if (IsLastFlush == TRUE)
        {
            PacketFlushParam.IsLastFlush = TRUE;
        }

        /* Flushing data from the bottom */
        STEVT_Notify(VIDTRAN_Data_p->EventsHandle,
                     VIDTRAN_Data_p->RegisteredEventsID[STVID_TRANSCODER_FLUSH_TRANSCODED_BUFFER_EVT_ID],
                     STEVT_EVENT_DATA_TYPE_CAST &(PacketFlushParam));

#ifdef STVID_DEBUG_GET_STATISTICS
        VIDTRAN_Data_p->Statistics.EncodeNbOfOutputBufferBytesFlushed += (U32)PacketFlushParam.PacketStopAddress_p - (U32)PacketFlushParam.PacketStartAddress_p;
#endif /* STVID_DEBUG_GET_STATISTICS */
    }
    else
    {
        /* No loop detected */
        PacketFlushParam.PacketStartAddress_p = VIDTRAN_Data_p->TranscoderOutputBuffer.Read_p;
        PacketFlushParam.PacketStopAddress_p = VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p;

        if (IsLastFlush == TRUE)
        {
            PacketFlushParam.IsLastFlush = TRUE;
        }

        /* Flushing data to the top */
        STEVT_Notify(VIDTRAN_Data_p->EventsHandle,
                     VIDTRAN_Data_p->RegisteredEventsID[STVID_TRANSCODER_FLUSH_TRANSCODED_BUFFER_EVT_ID],
                     STEVT_EVENT_DATA_TYPE_CAST &(PacketFlushParam));

#ifdef STVID_DEBUG_GET_STATISTICS
        VIDTRAN_Data_p->Statistics.EncodeNbOfOutputBufferBytesFlushed += (U32)PacketFlushParam.PacketStopAddress_p - (U32)PacketFlushParam.PacketStartAddress_p;
#endif /* STVID_DEBUG_GET_STATISTICS */
    }

#ifdef STVID_DEBUG_GET_STATISTICS
    VIDTRAN_Data_p->Statistics.EncodeNbOfOutputBufferFlushed++;
#endif /* STVID_DEBUG_GET_STATISTICS */

    return;
}

#ifdef LINEAR_OUTPUT_BUFFER
/*******************************************************************************
Name        : vidtran_UpdateTranscoderOutputBuffer
Description : Updates pointer in TranscoderOutputBuffer. Send notification to flush on disk if necessary
Parameters  :
Assumptions :
Limitations :
Returns     : TRUE if updated, FALSE if buffer still full
*******************************************************************************/
static BOOL vidtran_UpdateTranscoderOutputBuffer(const VIDTRAN_Handle_t TranscoderHandle)
{
    VIDTRAN_Data_t                      * VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;
    VIDBUFF_PictureBuffer_t *           CurrentPictureBuffer_p;
    STVID_PictureInfos_t                PictureInfos;
    STVID_DecimationFactor_t            DecimationFactors;
    U32                                 PictureWidth, PictureHeight;
    U32                                 PictureMaxSizeInBits, PictureMaxSizeInBytes;
    BOOL                                IsOutputBufferFull = FALSE;
    VIDTRAN_MonitorOutputBufferParams_t MonitorOutputBufferParams;

    /* Assign current picture buffer */
    if (VIDTRAN_Data_p->XCODE_TranscodePictureParams.DecimatedPictureBufferIsValid)
    {
        CurrentPictureBuffer_p = &(VIDTRAN_Data_p->XCODE_TranscodePictureParams.DecimatedPictureBuffer);
    }
    else
    {
        CurrentPictureBuffer_p = &(VIDTRAN_Data_p->XCODE_TranscodePictureParams.MainPictureBuffer);
    }

    /* ************************************************** */
    /* Compute the maximum size of the transcoded picture */
    /* ************************************************** */

    /* Calculate width and height of picture of be transcoded */
    PictureInfos = CurrentPictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos;
    DecimationFactors = PictureInfos.VideoParams.DecimationFactors;

    /* PictureWidth */
    if (DecimationFactors & STVID_DECIMATION_FACTOR_H2)
    {
        PictureWidth = PictureInfos.BitmapParams.Width/2;
    }
    else if (DecimationFactors & STVID_DECIMATION_FACTOR_H4)
    {
        PictureWidth = PictureInfos.BitmapParams.Width/4;
    }
    else
    {
        PictureWidth = PictureInfos.BitmapParams.Width;
    }

    /* PictureHeight */
    if (DecimationFactors & STVID_DECIMATION_FACTOR_V2)
    {
        PictureHeight = PictureInfos.BitmapParams.Height/2;
    }
    else
    {
        PictureHeight = PictureInfos.BitmapParams.Height;
    }

    /* Compute the maximum size of the transcoded picture */
    PictureMaxSizeInBytes = TARGET_GetPictureMaxSizeInByte(VIDTRAN_Data_p->Target.TranscodecHandle,
                                                           PictureWidth,
                                                           PictureHeight,
                                                           VIDTRAN_Data_p->XCODE_TranscodePictureParams.TargetFrameRate,
                                                           VIDTRAN_Data_p->XCODE_TranscodePictureParams.TargetBitRate);

    PictureMaxSizeInBits = 8 * PictureMaxSizeInBytes;

#ifdef TRANSCODE_PES_OUTPUT
    /* Adding PES buffer size (+ PTS option)*/
    PictureMaxSizeInBits += (8 * sizeof(VIDTRAN_PES_Header_t) + sizeof(VIDTRAN_PES_Header_PTS_t));
#endif

    /* ************************************************************** */
    /*     Compute the available size in TranscoderOutputBuffer       */
    /* (XCODE_TranscodePictureParams.XcoderOutputBufferAvailableSize) */
    /* ************************************************************** */

    IsOutputBufferFull = FALSE;

    /* Returns OutputBufferLevelInBytes in TranscoderOutputBuffer (circular) */
    vidtran_MonitorOutputBuffer(TranscoderHandle, &MonitorOutputBufferParams);

    /* XcoderOutputBufferAvailableSize Unit is bit */
    VIDTRAN_Data_p->XCODE_TranscodePictureParams.XcoderOutputBufferAvailableSize =
        8 * (MonitorOutputBufferParams.Size - MonitorOutputBufferParams.OutputBufferLevelInBytes);

    /* ************************************************************************************************************** */
    /* Determine if a hard disk flush is necessary:                                                                   */
    /* - If The global level of the buffer reached the TRANSCODED_OUTPUT_BUFFER_THRESHOLD_SIZE                        */
    /* - If there is not enough space remaining for the TRANSCODED_OUTPUT_BUFFER_MIN_PICTURE_REMINING coming pictures */
    /* ************************************************************************************************************** */
    if ((MonitorOutputBufferParams.OutputBufferLevelInBytes >= TRANSCODED_OUTPUT_BUFFER_THRESHOLD_SIZE)
     || (VIDTRAN_Data_p->XCODE_TranscodePictureParams.XcoderOutputBufferAvailableSize < (TRANSCODED_OUTPUT_BUFFER_MIN_PICTURE_REMINING * PictureMaxSizeInBits)))
    {
        TraceVerbose(("WR: %d, BF:%d ",VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p,STOS_time_now()));
        /* We need to flush the buffer */
        vidtran_FlushOutputTranscodedBuffer(TranscoderHandle, FALSE);
        TraceVerbose(("WR: %d, AF:%d\n",VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p,STOS_time_now()));

        /* Update output buffer size */
        vidtran_MonitorOutputBuffer(TranscoderHandle, &MonitorOutputBufferParams);

        /* Check XcoderOutputBufferAvailableSize has changed */
        if ( (8 * (MonitorOutputBufferParams.Size - MonitorOutputBufferParams.OutputBufferLevelInBytes)) <=
             VIDTRAN_Data_p->XCODE_TranscodePictureParams.XcoderOutputBufferAvailableSize)
        {
            IsOutputBufferFull = TRUE;
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Transcode: Output buffer full ! \r\n"));
#ifdef STVID_DEBUG_GET_STATISTICS
            VIDTRAN_Data_p->Statistics.EncodePbOutputBufferFull++;
#endif /* STVID_DEBUG_GET_STATISTICS */
        }
    }

    /* A check is needed to know if the write pointer has to wrap around */
    if ((U32)VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p + PictureMaxSizeInBytes >= (U32)VIDTRAN_Data_p->TranscoderOutputBuffer.Top_p)
    {
            /* Updating Loop pointer */
            VIDTRAN_Data_p->TranscoderOutputBuffer.Loop_p = VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p;
            /* Wrapping around */
            VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p = VIDTRAN_Data_p->TranscoderOutputBuffer.Base_p;
    }

    return (IsOutputBufferFull);
}

#endif

/*******************************************************************************
Name        : SourceDecoderNewPictureAvailableCB
Description : Function to subscribe the Source Decoder event new picture decoded
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void SourceDecoderNewPictureAvailableCB(STEVT_CallReason_t Reason,
    const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event,
    const void *EventData_p,
    const void *SubscriberData_p
)
{
    VIDTRAN_Handle_t            TranscoderHandle;
    VIDTRAN_Data_t *            VIDTRAN_Data_p;
    VIDBUFF_PictureBuffer_t     *DecodedPicture_p, *DecimatedDecodedPicture_p;
    VIDTRAN_Picture_t           *TrPictures_p;
    VIDTRAN_StopParams_t        TranscoderStopParams;
    STVID_TranscoderError_t     ErrorType;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    TranscoderHandle = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDTRAN_Handle_t, SubscriberData_p);
    VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;

    switch (VIDTRAN_Data_p->TranscoderState)
    {
        case VIDTRAN_TRANSCODER_STATE_STARTING :
        case VIDTRAN_TRANSCODER_STATE_RUNNING :
        case VIDTRAN_TRANSCODER_STATE_STALLED :

            DecodedPicture_p = CAST_CONST_HANDLE_TO_DATA_TYPE(STVID_PictureInfos_t *,EventData_p)->PictureBufferHandle;
            DecimatedDecodedPicture_p = CAST_CONST_HANDLE_TO_DATA_TYPE(STVID_PictureInfos_t *,EventData_p)->DecimatedPictureBufferHandle;

            if (VIDTRAN_Data_p->ErrorRecoveryMode == STVID_ENCODER_MODE_ERROR_RECOVERY_FULL)
            {
                /* Check if any problem occured in the decode of this new available picture (not done on decimated: To do ?)*/
                if (DecodedPicture_p->Decode.DecodingError != VIDCOM_ERROR_LEVEL_NONE)
                {
                    /* Stopping the transcode */
                    TranscoderStopParams.EncoderStopMode = STVID_ENCODER_STOP_NOW;
                    vidtran_Stop(TranscoderHandle, &TranscoderStopParams);

                    /* Dropping the picture */
#ifdef STVID_DEBUG_GET_STATISTICS
                    VIDTRAN_Data_p->Statistics.EncodePicturesSkippedAtQueueInsertion++;
#endif /* STVID_DEBUG_GET_STATISTICS */
                    return;
                }
            }

            /* Get a transcode queue element */
            TrPictures_p = vidtrqueue_GetTrPictureElement(VIDTRAN_Data_p);
            if (TrPictures_p == NULL)
            {
                /* Notifying transcoder has no room in queue */
                ErrorType = STVID_ENCODER_ERROR_NO_ROOM_IN_QUEUE;
                STEVT_Notify(VIDTRAN_Data_p->EventsHandle,
                             VIDTRAN_Data_p->RegisteredEventsID[STVID_TRANSCODER_ERROR_EVT_ID],
                             STEVT_EVENT_DATA_TYPE_CAST &(ErrorType));

                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "No space available in transcoder Queue \r\n"));
#ifdef STVID_DEBUG_GET_STATISTICS
                    VIDTRAN_Data_p->Statistics.EncodePbSourcePicturesSkippedByQueueFull++;
#endif /* STVID_DEBUG_GET_STATISTICS */
                return;
            }

            /* Push main & decimated picture buffers in transcode queue */
            TrPictures_p->Next_p = NULL;
            TrPictures_p->Previous_p = NULL;
            TrPictures_p->MainPictureBuffer_p = DecodedPicture_p;
            TrPictures_p->DecimatedPictureBuffer_p = NULL;

            /* When no decimation is requested on the source decoder, the pointer to the decimated picture is NULL */
            if ((CAST_CONST_HANDLE_TO_DATA_TYPE(STVID_PictureInfos_t *, EventData_p))->DecimatedPictureBufferHandle != NULL)
            {
                /* Storing for queuing */
                TrPictures_p->DecimatedPictureBuffer_p = DecimatedDecodedPicture_p;
            }

            vidtrqueue_TakePicturesBeforeInsertInTranscoderQueue(VIDTRAN_Data_p, TrPictures_p);
            vidtrqueue_InsertPictureInTranscoderQueue(VIDTRAN_Data_p, TrPictures_p);

            /* Push command to the transcoder task if needed */
            PushTranscoderCommand(TranscoderHandle, TRANSCODER_NEW_SOURCE_PICTURE_AVAILABLE);
            STOS_SemaphoreSignal(VIDTRAN_Data_p->TranscoderOrder_p);

        break;

        case VIDTRAN_TRANSCODER_STATE_STOP_WHEN_QUEUE_EMPTY :
        case VIDTRAN_TRANSCODER_STATE_STOP_AND_FLUSH :
            /* Transcoder is about to stop. No more picture sent */
        break;

        case VIDTRAN_TRANSCODER_STATE_STOPPED :
            /* Transcoder is stopped. Doing nothing */
        break;

        default:
            /* Error Case ? */
#ifdef STVID_DEBUG_GET_STATISTICS
            VIDTRAN_Data_p->Statistics.EncodePbSourcePicturesSkippedByError++;
#endif /* STVID_DEBUG_GET_STATISTICS */
        break;
    }
    return;

} /* End of SourceDecoderNewPictureAvailableCB */

/*******************************************************************************
Name        : TargetDecoderNewPictureDecodedCB
Description : Function to subscribe the Source Decoder event new picture decoded
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void TargetDecoderNewPictureDecodedCB(STEVT_CallReason_t Reason,
    const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event,
    const void *EventData_p,
    const void *SubscriberData_p
)
{
    const VIDTRAN_Handle_t              TranscoderHandle = CAST_CONST_HANDLE_TO_DATA_TYPE(const VIDTRAN_Handle_t, SubscriberData_p);
    VIDTRAN_Data_t *                    VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;

    UNUSED_PARAMETER(VIDTRAN_Data_p);

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData_p);
}

/*******************************************************************************
Name        : XcoderJobCompletedCB
Description : CallBack from Xcoder job completed
Parameters  : TranscoderHandle, XCODER_TranscodingJobResults
Assumptions :
Limitations :
Returns     : void
*******************************************************************************/
static void XcoderJobCompletedCB(VIDTRAN_Handle_t TranscoderHandle, XCODER_TranscodingJobResults_t XCODER_TranscodingJobResults)
{
    VIDTRAN_Data_t              * VIDTRAN_Data_p = (VIDTRAN_Data_t *) TranscoderHandle;
    void                        * TempWrite_p;
    STVID_TranscoderError_t     ErrorType;

    TempWrite_p = VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p;
    /* Error management: TODO */

    /* Update write pointer in Xcoder Output buffer. Buffer is linear and write pointer may never go further than top */
    /* UsedOutputBufferSizeInByte bytes written gives a shift of UsedOutputBufferSizeInByte -1 in address */
    VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p = (void *) ((U32)VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p +
                                                        XCODER_TranscodingJobResults.TranscodingJobStatus.UsedOutputBufferSizeInByte
                                                        );

    if (VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p >= VIDTRAN_Data_p->TranscoderOutputBuffer.Top_p)
    {
        /* Transcoder wrote picture over the top. Should never arrive !*/
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Transcode Xcode: Output buffer overflow (data) !"));
        TraceVerbose(("Firmware wrote over top of output buffer ! \r\n"));
        TraceVerbose(("Top:%d, Prev Wr:%d, Cur Wr:%d \r\n",
                                    VIDTRAN_Data_p->TranscoderOutputBuffer.Top_p,
                                    TempWrite_p,
                                    VIDTRAN_Data_p->TranscoderOutputBuffer.Write_p));

#ifdef STVID_DEBUG_GET_STATISTICS
        VIDTRAN_Data_p->Statistics.EncodePbOutputBufferOverflow++;
#endif /* STVID_DEBUG_GET_STATISTICS */

        /* Stop the transcode urgently. */
        VIDTRAN_Data_p->TranscoderState = VIDTRAN_TRANSCODER_STATE_STOPPED;

        /* Notifying firmware has overflowded the output buffer */
        ErrorType = STVID_ENCODER_ERROR_URGENTLY_STOPPED | STVID_ENCODER_ERROR_OUTPUT_OVERFLOW;
        STEVT_Notify(VIDTRAN_Data_p->EventsHandle,
                     VIDTRAN_Data_p->RegisteredEventsID[STVID_TRANSCODER_ERROR_EVT_ID],
                     STEVT_EVENT_DATA_TYPE_CAST &(ErrorType));
    }
}
/* End of vid_tran.c */

