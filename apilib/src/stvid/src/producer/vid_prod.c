/*******************************************************************************

File name   : vid_prod.c

Description : Video producer source file

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
26 Jan 2004        Created from decoder module                       HG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Define to add debug info and traces */
/* Note : To add traces add STVID_PRODUCER_TRACE_UART in environment variables */

#ifdef STVID_DEBUG_PRODUCER
#define STTBX_REPORT
#define PRODUCER_DUMP_FRAME_BUFFER
#endif

#ifndef STVID_VALID
/* For stapi compilation disable lack of frame buffer reports by default */
/* Debug session may undefine this flag to re-enable those reports */
#define STVID_DONT_REPORT_LACK_OF_BUFFER
#endif

#if defined ST_vcodmpg4p2 || defined ST_vcodvc1 ||  defined ST_vcodavs
#define COMPUTE_PRESENTATION_ID	  1
#endif

#define DONT_COMPUTE_NB_DECODED_FRAME_BUFFER /* use producer variable to store current # of decoded picture after the start instead of looking at the decoded picture table */

#ifndef ST_OSWINCE
#ifdef ST_OS21
    #define FORMAT_SPEC_OSCLOCK "ll"
#else
    #define FORMAT_SPEC_OSCLOCK ""
#endif


#define OSCLOCK_T_MILLE     ((STOS_Clock_t) 1000)
#define OSCLOCK_T_MILLION   ((STOS_Clock_t) 1000000)
#endif /* ST_OSWINCE */

#if defined(TRACE_PRODUCER)
#define TRACE_UART
#endif /* TRACE_PRODUCER */

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

#include "producer.h"

#include "vid_prod.h"

#include "vidcodec.h"

#include "sttbx.h"

#include "stevt.h"

#if defined TRACE_UART
#include "trace.h"
#define TracePictureType(x) TraceMessage x
#define TraceGraph(x) TraceVal x
#endif /* TRACE_UART */


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

/* Dummy value that we set to check that a handle is valid, only one chance out of 2^32 to get it by accident ! */
#define VIDPROD_VALID_HANDLE 0x01951950

/* Minimum size of user data for internal process (in case asked less). */
#define MIN_USER_DATA_SIZE  15

/* Choose here the default mode to pass picture to display when relevant: at decode end or at decode start. */
#define VIDPROD_DEFAULT_UPDATE_DISPLAY_QUEUE_MODE    VIDPROD_UPDATE_DISPLAY_QUEUE_AT_DECODE_END

/* Max pictures in decode pipe : Max number of pictures in state between processing and decode */
#define MAX_PICTURES_IN_DECODE_PIPE     (MAX_NUMBER_OF_PREPROCESSING_PICTURES + 1)

/* Max time waiting for an order: set to 10 Vsync to avoid to schedule producer task too often */
#define MAX_WAIT_ORDER_TIME (10*STVID_MAX_VSYNC_DURATION)

#ifdef ST_OSWINCE
  #define MAX_WAIT_ORDER_TIME_MS (10*1000/(STVID_DEFAULT_VALID_VTG_FRAME_RATE/1000))   //200 ms
#endif

/* Max time before stopping decoder and notifying the decoder has stopped. When a delayed
stop is asked (STVID_STOP_END_OF_DATA), the decoder should stop by itself if it can't
find a free buffer after some time */
/* PC : Very high value due to mme commands dump to file */
/* #define MAX_TIME_BEFORE_STOPPING_DECODER (10000 * STVID_MAX_VSYNC_DURATION) */
#define MAX_TIME_BEFORE_STOPPING_DECODER (50 * STVID_MAX_VSYNC_DURATION)

/* Max start code search time 20 field display time */
/*#define MAX_PICTURE_SEARCH_TIME (20 * STVID_MAX_VSYNC_DURATION)*/
/* TODO_CL: something goes wrong when start condition is bit buffer fullness in real time, we need more than 20 VSYNC to reach start condition */
/* in old decode module we were setting the BBT in the HAL and using threshold interrupt to check start condition, now we do polling on parser's bit buffer level... */
#define MAX_PICTURE_SEARCH_TIME (30 * STVID_MAX_VSYNC_DURATION)

/* Max timeout for VIDPROD_Stop() to allow current decode to finish prior to return to API STVID_Stop() call (set to 4 Vsyncs by decode) */
#define MAX_WAIT_STOP_TIME (4 * MAX_PICTURES_IN_DECODE_PIPE * STVID_MAX_VSYNC_DURATION)


#ifdef STVID_TRICKMODE_BACKWARD
/* Bit buffer address is aligned on 2048 for omega 2*/
/* Bit buffer address is aligned on 1024 for omega 1*/
/* Bit buffer address is aligned on 256 in genbuff */
/* Backward linear BB are obtained by splitting the circular BB into two equal parts */
/* So for backward, we need to have a circular size multiple of 4096 to match all cases */
#define DEFAULT_BIT_BUFFER_SIZE_H264_SD_LIVE    2146304
#define DEFAULT_BIT_BUFFER_SIZE_H264_HD_LIVE    5124096
#else
#define DEFAULT_BIT_BUFFER_SIZE_H264_SD_LIVE    2145024 /* From deltaMu Green spreadsheet aligned on 256 bytes boundary */
#define DEFAULT_BIT_BUFFER_SIZE_H264_HD_LIVE    5120768 /* From deltaMu Green spreadsheet aligned on 256 bytes boundary */
#endif  /* STVID_TRICKMODE_BACKWARD */

#if defined(DVD_SECURED_CHIP)
    #define H264_ESCOPY_BUFFER_RESIZE_FACTOR       25 /* Means ESCopy buffer size = Bit buffer size/RESIZE_FACTOR */
    #define VC1_ESCOPY_BUFFER_RESIZE_FACTOR        25 /* Means ESCopy buffer size = Bit buffer size/RESIZE_FACTOR */
#endif /* DVD_SECURED_CHIP */

/* Max vbv_delay to wait according to specifiactions.   */
/* (i.e. 597*2048 = 1194 KBytes)                        */
#define MPEG_MAX_VBV_SIZE_TO_REACH_BEFORE_START_IN_BYTES  (597 * 2048)

/* Default bit buffer sizes */
#if 0/*def STVID_TRICKMODE_BACKWARD*/
/* Increase bit buffer for HDD playback. From STPRM driver : it is better to
 * have a bitbuffer size aligned on a transport packet size (188) and a sector
 * size (512). */
#define PRM_ALIGNED_1M                      (188*512*11) /*1058816*/
#define DEFAULT_BIT_BUFFER_SIZE_H264_SD     (10*PRM_ALIGNED_1M*2)   /* TO BE CHECKED ... */
#define DEFAULT_BIT_BUFFER_SIZE_H264_HD      (5*PRM_ALIGNED_1M*2)   /* TO BE CHECKED ... */
#define DEFAULT_BIT_BUFFER_SIZE_VC1_SD     (10*PRM_ALIGNED_1M*2)   /* TO BE CHECKED ... */
#define DEFAULT_BIT_BUFFER_SIZE_VC1_HD      (5*PRM_ALIGNED_1M*2)   /* TO BE CHECKED ... */
#define DEFAULT_BIT_BUFFER_SIZE_MPEG4P2_SD  (10*PRM_ALIGNED_1M*2)   /* TO BE CHECKED ... */
#define DEFAULT_BIT_BUFFER_SIZE_MPEG_SD     (188*512*44) /* 0x40A000, 4M used also as 2Mx2 for each linear bitbuffer */
#define DEFAULT_BIT_BUFFER_SIZE_MPEG_HD     (2105344) /* TODO */
#else /* STVID_TRICKMODE_BACKWARD */
#define DEFAULT_BIT_BUFFER_SIZE_H264_SD         DEFAULT_BIT_BUFFER_SIZE_H264_SD_LIVE
#define DEFAULT_BIT_BUFFER_SIZE_H264_HD         DEFAULT_BIT_BUFFER_SIZE_H264_HD_LIVE
#define DEFAULT_BIT_BUFFER_SIZE_VC1_SD          2665216 /* aligned on 256 bytes boundary */
#define DEFAULT_BIT_BUFFER_SIZE_VC1_HD          5330176 /* aligned on 256 bytes boundary */
#define DEFAULT_BIT_BUFFER_SIZE_MPEG4P2_SD       512000       /* 229376 min  for ASP 5*/
#define DEFAULT_BIT_BUFFER_SIZE_MPEG_SD      (380928)
#define DEFAULT_BIT_BUFFER_SIZE_MPEG_HD      (2105344) /* TODO */
#endif /* STVID_TRICKMODE_BACKWARD */

/* After too much errors without a good decode, we consider the problem could be inside the decoder. We do a Soft Reset.  */
#define ACCEPTANCE_ERROR_AMOUNT_HIGHEST_ERROR_RECOVERY 3       /* Amount of consecutive errors for Error Recovery Full    */
#define ACCEPTANCE_ERROR_AMOUNT_LOWEST_ERROR_RECOVERY 10       /* Amount of consecutive errors for Error Recovery not none */

#ifdef ST_speed
#define VIDPROD_TIME_FACTOR                  0x1000  /* 2^n is fast to process */
#define VIDPROD_SIZE_UNIT_FACTOR             50      /* convert bytes -> 400bits unit */
#define VIDPROD_MAX_BITRATE                             37500
#endif /*ST_speed*/
#ifdef STVID_TRICKMODE_BACKWARD
#define VIDPROD_DEFAULT_BITRATE                         37500
#define VIDPROD_BACKWARD_PICTURES_NB_BY_BUFFER          150
#define VIDPROD_DEFAULT_FRAME_RATE                      25
#endif /* STVID_TRICKMODE_BACKWARD */
/* Private Variables (static)------------------------------------------------ */
#if defined(VALID_TOOLS) && defined(TRACE_UART)

#define VIDPROD_TRACE_BLOCKNAME "VIDPROD"

static TRACE_Item_t vidprod_TraceItemList[] =
{
    {"GLOBAL",              TRACE_PRODUCER_COLOR,                       TRUE, FALSE},
    {"INIT_TERM",           TRACE_PRODUCER_COLOR,                       TRUE, FALSE},
    {"RESET",               TRACE_PRODUCER_COLOR,                       TRUE, FALSE},
    {"DISPLAY_INSERT",      TRACE_PRODUCER_COLOR,                       TRUE, FALSE},
    {"PRESENTATION",        TRACE_PRODUCER_COLOR,                       TRUE, FALSE},
    {"PARSER_START",        TRACE_PRODUCER_COLOR,                       TRUE, FALSE},
    {"PARSER_JOB_END",      TRACE_PRODUCER_COLOR,                       TRUE, FALSE},
    {"PARSING_ERRORS",      TRACE_PRODUCER_COLOR | TRC_ATTR_REVERSE,    TRUE, FALSE},
    {"BBL",                 TRACE_PRODUCER_COLOR,                       TRUE, FALSE},
    {"INFORM_READ_ADDR",    TRACE_PRODUCER_COLOR,                       TRUE, FALSE},
    {"DECODER_START",       TRACE_PRODUCER_COLOR,                       TRUE, FALSE},
    {"DECODER_CONFIRM",     TRACE_PRODUCER_COLOR,                       TRUE, FALSE},
    {"DECODER_JOB_END",     TRACE_PRODUCER_COLOR,                       TRUE, FALSE},
    {"DECODER_PROCESSING",  TRACE_PRODUCER_COLOR,                       TRUE, FALSE},
    {"DECODING_TIMEOUT",    TRACE_PRODUCER_COLOR | TRC_ATTR_REVERSE,    TRUE, FALSE},
    {"DECODING_ERRORS",     TRACE_PRODUCER_COLOR | TRC_ATTR_REVERSE,    TRUE, FALSE},
    {"PROCESSING_TIME",     TRACE_PRODUCER_COLOR,                       TRUE, FALSE},
    {"DPB_MANAGEMENT",      TRACE_PRODUCER_COLOR,                       TRUE, FALSE},
    {"DPB_STATUS",          TRACE_PRODUCER_COLOR,                       TRUE, FALSE}
};

#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */
#if defined(VALID_TOOLS) || defined(ST_crc) /* && (defined(STVID_FRAME_BUFFER_DUMP) || defined(STVID_FB_ADD_DATA_DUMP)) */
#define OMEGA2_420 0x94
#define FILENAME_MAX_LENGTH 200
#endif /* defined(VALID_TOOLS) && (defined(STVID_FRAME_BUFFER_DUMP) || defined(STVID_FB_ADD_DATA_DUMP)) */

#ifdef STVID_PRESERVE_LAST_PICTURES_IN_BB
#define READ_ADDRESS_FIFO_DEPTH 13
static void    *ReadAddressFifo[READ_ADDRESS_FIFO_DEPTH];
static U32      ReadAdressFifoNextOutputIndex;
static U32      ReadAddressFifoNextInputIndex;
static U32      ReadAddressFifoNbUsedEntries;
#endif /* STVID_PRESERVE_LAST_PICTURES_IN_BB */

/* Private Macros ----------------------------------------------------------- */

/* For debug macro */
#ifdef STTBX_REPORT
#define DECREMENT_NB_PICTURES_IN_QUEUE(Handle) vidprod_DecrementNbPicturesInQueue(Handle, __LINE__)
#else
#define DECREMENT_NB_PICTURES_IN_QUEUE(Handle) vidprod_DecrementNbPicturesInQueue(Handle)
#endif /* STTBX_REPORT */

/* Error recovery strategy macros :                                                                             */
/*                                                                                                              */
/* ===========================================================================================================  */
/*                                                                   |   NONE  | PARTIAL |  HIGH   |   FULL  |  */
/* ===========================================================================================================  */
/* Is non reference Picture To Be Decoded If Missing Reference       |    X    |         |         |         |  */
/* Is reference Picture To Be Decoded If Missing Reference           |    X    |    X    |    X    |         |  */
/* Is Spoiled Picture To Be Displayed                                |    X    |    X    |    X    |         |  */
/* Is Spoiled Reference Loosing All References Until Next Entry point|         |         |         |    X    |  */
/* Is TimeOut On StartCode Search Taken Into Account                 |         |         |    X    |    X    |  */
/* Is Picture To Be Skipped If its preprocessing fails               |         |    X    |    X    |    X    |  normal operation (safest for H264 firmware) */
/* Is Picture To Be Skipped If its preprocessing fails               |         |         |         |    X    |  best display fluency operation (risked for H264 firmware) STVID_DONT_SKIP_BADLY_PREPROCESSED_PICTURE defined */
/* ===========================================================================================================  */

#define IsNonRefPictureToBeDecodedIfMissingReference(ErrorRecoveryMode) ((ErrorRecoveryMode) == STVID_ERROR_RECOVERY_NONE)
#define IsRefPictureToBeDecodedIfMissingReference(ErrorRecoveryMode) ((ErrorRecoveryMode) != STVID_ERROR_RECOVERY_FULL)
#define IsSpoiledPictureToBeDisplayed(ErrorRecoveryMode) ((ErrorRecoveryMode) != STVID_ERROR_RECOVERY_FULL)
#define IsSpoiledReferenceLoosingAllReferencesUntilNextRandomAccessPoint(ErrorRecoveryMode) ((ErrorRecoveryMode) == STVID_ERROR_RECOVERY_FULL)
#define IsTimeOutOnStartCodeSearchTakenIntoAccount(ErrorRecoveryMode) (((ErrorRecoveryMode) == STVID_ERROR_RECOVERY_FULL) || ((ErrorRecoveryMode) == STVID_ERROR_RECOVERY_HIGH))

/* If STVID_DONT_SKIP_BADLY_PREPROCESSED_PICTURE is define at compilation level,
 * error recovery will not skip picture in case of preprocessing errors except for FULL mode */
#ifdef STVID_DONT_SKIP_BADLY_PREPROCESSED_PICTURE
/* Badly preprocessed errors will _not_ be sent to the firmware _only_ for the FULL mode to avoid to display macroblocks */
/* modes HIGH & PARTIAL may makes firmware crash if the picture is heavily corrupted !!! */
#define IsPictureToBeSkippedInCaseOfPreProcessingError(ErrorRecoveryMode) ((ErrorRecoveryMode) == STVID_ERROR_RECOVERY_FULL)
#else
/* Badly preprocessed errors will _not_ be sent to the firmware except in NONE mode */
/* This case is the safest for the firmware (no crash risk) if the picture is heavily corrupted !!! */
#define IsPictureToBeSkippedInCaseOfPreProcessingError(ErrorRecoveryMode) ((ErrorRecoveryMode) != STVID_ERROR_RECOVERY_NONE)
#endif

#define Virtual(DevAdd) (void*)((U32)(DevAdd)\
        -(U32) (VIDPROD_Data_p->VirtualMapping.PhysicalAddressSeenFromDevice_p)\
        +(U32) (VIDPROD_Data_p->VirtualMapping.VirtualBaseAddress_p))

#define BufferFullnessToReachWhenTooBig(BitBufferSize) (((BitBufferSize) * 7) / 10)
#define IsFramePicture(Picture_p) ((Picture_p)->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
#define IsFrameOrFirstOfTwoFields(Picture_p) ((IsFramePicture(Picture_p)) || ((Picture_p)->ParsedPictureInformation.PictureGenericData.IsFirstOfTwoFields))
#define IsSecondOfTwoFields(Picture_p) (!(IsFramePicture(Picture_p)) && !((Picture_p)->ParsedPictureInformation.PictureGenericData.IsFirstOfTwoFields))
#define IsFirstOfTwoFieldsOfIFrame(Picture_p) (((Picture_p)->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I) && \
        ((Picture_p)->ParsedPictureInformation.PictureGenericData.IsFirstOfTwoFields))

#ifdef ST_speed
#define IsIOnlyDecodeOnGoing(SkipMode) ((((SkipMode).AllPPictures) && ((SkipMode).AllBPictures)) || (((SkipMode).AllPPicturesSpeed) && ((SkipMode).AllBPicturesSpeed)))
#else
#define IsIOnlyDecodeOnGoing(SkipMode) (((SkipMode).AllPPictures) && ((SkipMode).AllBPictures))
#endif /* ST_speed */

#define IsDynamicAllocationProfile(Profile) (((Profile).ApiProfile.NbFrameStore == STVID_VARIABLE_IN_FIXED_SIZE_NB_FRAME_STORE_ID) || ((Profile).ApiProfile.NbFrameStore == STVID_VARIABLE_IN_FULL_PARTITION_NB_FRAME_STORE_ID))

#define  MPEGFrame2Char(Type)  (                                        \
               (  (Type) == STVID_MPEG_FRAME_I)                         \
                ? 'I' : (  ((Type) == STVID_MPEG_FRAME_P)               \
                         ? 'P' : (  ((Type) == STVID_MPEG_FRAME_B)      \
                                  ? 'B'  : 'U') ) )
#define Min(a,b)    ((a) < (b) ? (a) : (b))

/* Private Function prototypes ---------------------------------------------- */

static void InitProducerStructure(const VIDPROD_Handle_t ProducerHandle);
static void TermProducerStructure(const VIDPROD_Handle_t ProducerHandle);
static ST_ErrorCode_t StartProducerTask(const VIDPROD_Handle_t ProducerHandle);
static ST_ErrorCode_t StopProducerTask(const VIDPROD_Handle_t ProducerHandle);
static void NewPictureBufferAvailableCB(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event,  const void *EventData_p, const  void *SubscriberData_p);
static void ParserJobCompletedCB(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
static void ParserPictureSkippedCB(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
static void ParserFindDiscontinuityCB(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
#ifdef STVID_TRICKMODE_BACKWARD
static void ParserBitBufferFullyParsedCB(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
#endif
static void ParserUserDataCB(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
static void DecoderJobCompletedCB(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
static void DecoderConfirmBufferCB(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
static void DecoderInformReadAddressCB(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
static void ProducerTaskFunc(const VIDPROD_Handle_t ProducerHandle);
static void ProducerTaskFuncForAutomaticControlMode(const VIDPROD_Handle_t ProducerHandle);
static void DoStopNow(const VIDPROD_Handle_t ProducerHandle, const BOOL SendStopEvent);
static void SeekBeginningOfStream(const VIDPROD_Handle_t ProducerHandle);
static void ClearAllPendingControllerCommands(const VIDPROD_Handle_t ProducerHandle);
static ST_ErrorCode_t GetBuffers(const VIDPROD_Handle_t ProducerHandle,
                                 const VIDBUFF_PictureBuffer_t *PictureBuffer_p,
                                 VIDBUFF_PictureBuffer_t ** DecodedPicture_p_p,
                                 VIDBUFF_PictureBuffer_t ** DecodedDecimPicture_p_p );
static void CopyPictureBuffer(VIDBUFF_PictureBuffer_t * DestPicture_p, VIDBUFF_PictureBuffer_t * SrcPicture_p);
static void vidprod_UpdatePictureParsedPictureInformation(VIDBUFF_PictureBuffer_t * PictureBuffer_p, VIDCOM_ParsedPictureInformation_t * ParsedPictureInformation_p);
static void GetFreePreprocessingPictureBuffer(const VIDPROD_Handle_t ProducerHandle,
                                              VIDBUFF_PictureBuffer_t **PictureBuffer_p_p);
#ifdef ST_speed
#ifdef STVID_TRICKMODE_BACKWARD
static ST_ErrorCode_t vidprod_AllocateLinearBitBuffers(VIDPROD_Handle_t ProducerHandle);
static void           vidprod_SmoothBackwardInitStructure(const VIDPROD_Handle_t  ProducerHandle);
static void           vidprod_AskForLinearBitBufferFlush(const VIDPROD_Handle_t ProducerHandle,
                                                            VIDPROD_LinearBitBuffer_t * BufferToBeFilled_p,
                                                           const VIDPROD_FlushBitBufferManner_t FlushManner);
static BOOL PutPictureStartCodeAndComputeValidDataBuffer(const VIDPROD_Handle_t ProducerHandle, VIDPROD_LinearBitBuffer_t * const LinearBitBuffer_p, const BOOL Overlap);
static ST_ErrorCode_t GetFreePreprocessingPictureBufferBackward(const VIDPROD_Handle_t ProducerHandle,
                                              VIDBUFF_PictureBuffer_t **PictureBuffer_p_p, U32 DecodingOrderID);
static void vidprod_RemovePictureFromPreprocessingTableBackward(const VIDPROD_Handle_t ProducerHandle, VIDBUFF_PictureBuffer_t *PictureBuffer_p);
#endif /* STVID_TRICKMODE_BACKWARD */
#endif /*ST_speed*/
static void AddPictureToDecodePictureTable(const VIDPROD_Handle_t ProducerHandle,
                                           VIDBUFF_PictureBuffer_t *PictureBuffer_p);
static void RemovePictureFromDecodePictureTable(const VIDPROD_Handle_t ProducerHandle,
                                                VIDBUFF_PictureBuffer_t *PictureBuffer_p, U32 CallLine);
static void UpdateReferenceList(const VIDPROD_Handle_t ProducerHandle,
                                VIDBUFF_PictureBuffer_t *PictureBuffer_p);
static void ResetReferenceList(const VIDPROD_Handle_t ProducerHandle);

static BOOL BumpPictureToDisplayQueue(const VIDPROD_Handle_t ProducerHandle);
static ST_ErrorCode_t vidprod_ConfigureEvent(const VIDPROD_Handle_t ProducerHandle, const STEVT_EventID_t Event, const STVID_ConfigureEventParams_t * const Params_p);
static ST_ErrorCode_t vidprod_DataInjectionCompleted(const VIDPROD_Handle_t ProducerHandle, const STVID_DataInjectionCompletedParams_t * const Params_p);
#ifdef STTBX_REPORT
static ST_ErrorCode_t vidprod_DecrementNbPicturesInQueue(const VIDPROD_Handle_t ProducerHandle, const U32 LineNumber);
#else
static ST_ErrorCode_t vidprod_DecrementNbPicturesInQueue(const VIDPROD_Handle_t ProducerHandle);
#endif /* STTBX_REPORT */
static ST_ErrorCode_t vidprod_FlushInjection(const VIDPROD_Handle_t ProducerHandle);
static ST_ErrorCode_t vidprod_Freeze(const VIDPROD_Handle_t ProducerHandle);
static ST_ErrorCode_t vidprod_GetBitBufferFreeSize(const VIDPROD_Handle_t ProducerHandle, U32 * const FreeSize_p);
static ST_ErrorCode_t vidprod_GetDecodedPictures(const VIDPROD_Handle_t ProducerHandle, STVID_DecodedPictures_t * const DecodedPictures_p);
static ST_ErrorCode_t vidprod_GetDecoderSpecialOperatingMode (const VIDPROD_Handle_t ProducerHandle, VIDPROD_DecoderOperatingMode_t * const DecoderOperatingMode_p);
static ST_ErrorCode_t vidprod_GetErrorRecoveryMode(const VIDPROD_Handle_t ProducerHandle, STVID_ErrorRecoveryMode_t * const Mode_p);
static ST_ErrorCode_t vidprod_GetHDPIPParams(const VIDPROD_Handle_t ProducerHandle, STVID_HDPIPParams_t * const HDPIPParams_p);
static ST_ErrorCode_t vidprod_GetLastDecodedPictureInfos(const VIDPROD_Handle_t ProducerHandle, STVID_PictureInfos_t * const PictureInfos_p);
static ST_ErrorCode_t vidprod_GetInfos(const VIDPROD_Handle_t ProducerHandle, VIDPROD_Infos_t * const Infos_p);
#ifdef STVID_DEBUG_GET_STATISTICS
static ST_ErrorCode_t vidprod_GetStatistics(const VIDPROD_Handle_t ProducerHandle, STVID_Statistics_t * const Statistics_p);
static ST_ErrorCode_t vidprod_ResetStatistics(const VIDPROD_Handle_t ProducerHandle, const STVID_Statistics_t * const Statistics_p);
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
static ST_ErrorCode_t vidprod_GetStatus(const VIDPROD_Handle_t ProducerHandle, STVID_Status_t * const Status_p);
#endif /* STVID_DEBUG_GET_STATUS */
static ST_ErrorCode_t vidprod_Init(const VIDPROD_InitParams_t * const InitParams_p, VIDPROD_Handle_t * const ProducerHandle_p);
static ST_ErrorCode_t vidprod_NewReferencePicture(const VIDPROD_Handle_t ProducerHandle, VIDBUFF_PictureBuffer_t * const Picture_p);
static ST_ErrorCode_t vidprod_NotifyDecodeEvent(const VIDPROD_Handle_t ProducerHandle, STEVT_EventID_t EventID, const void * EventData);
static ST_ErrorCode_t vidprod_Resume(const VIDPROD_Handle_t ProducerHandle);
static ST_ErrorCode_t vidprod_SetDecodedPictures(const VIDPROD_Handle_t ProducerHandle, const STVID_DecodedPictures_t DecodedPictures);
static ST_ErrorCode_t vidprod_SetErrorRecoveryMode(const VIDPROD_Handle_t ProducerHandle, const STVID_ErrorRecoveryMode_t ErrorRecoveryMode);
static ST_ErrorCode_t vidprod_SetHDPIPParams(const VIDPROD_Handle_t ProducerHandle, const STVID_HDPIPParams_t * const HDPIPParams_p);
static ST_ErrorCode_t vidprod_Setup(const VIDPROD_Handle_t ProducerHandle, const STVID_SetupParams_t * const SetupParams_p);
static ST_ErrorCode_t vidprod_SkipData(const VIDPROD_Handle_t ProducerHandle, const VIDPROD_SkipMode_t Mode);

#ifdef ST_speed
static void EvaluateBitRateValue(const VIDPROD_Handle_t ProducerHandle, U32 NbFieldsDisplayNextDecode, U32 FrameRate, U32 OutputCounter);
#endif /* ST_speed */
static ST_ErrorCode_t vidprod_Start(const VIDPROD_Handle_t ProducerHandle, const VIDPROD_StartParams_t * const StartParams_p);
static ST_ErrorCode_t vidprod_StartUpdatingDisplay(const VIDPROD_Handle_t ProducerHandle);
static ST_ErrorCode_t vidprod_Stop(const VIDPROD_Handle_t ProducerHandle, const STVID_Stop_t StopMode,const BOOL StopAfterBuffersTimeOut);
static ST_ErrorCode_t vidprod_Term(const VIDPROD_Handle_t ProducerHandle);
#ifdef ST_deblock
static void vidprod_SetDeblockingMode(const VIDPROD_Handle_t ProducerHandle, const BOOL DeblockingEnabled);
#ifdef VIDEO_DEBLOCK_DEBUG
static void vidprod_SetDeblockingStrength(const VIDPROD_Handle_t ProducerHandle, const int DeblockingStrength);
#endif /* VIDEO_DEBLOCK_DEBUG  */
#endif /* ST_deblock */

/* extension for injection controle */
static ST_ErrorCode_t vidprod_SetDataInputInterface(const VIDPROD_Handle_t ProducerHandle,
     ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle, void ** const Address_p),
     void (*InformReadAddress)(void * const Handle, void * const Address),
     void * const Handle ); /* Handle to pass to the 2 function calls */
static ST_ErrorCode_t vidprod_DeleteDataInputInterface(const VIDPROD_Handle_t ProducerHandle);
static ST_ErrorCode_t vidprod_GetDataInputBufferParams(const VIDPROD_Handle_t ProducerHandle,
                                void ** const BaseAddress_p,
                                U32 * const Size_p);
static ST_ErrorCode_t vidprod_GetBitBufferParams(const VIDPROD_Handle_t ProducerHandle,  void ** const Address_p,  U32 * const Size_p);

static BOOL IsRequestingAdditionnalData(const VIDPROD_Data_t * const VIDPROD_Data_p);
static void MonitorBitBufferLevel(const VIDPROD_Handle_t ProducerHandle);
static void vidprod_ReleaseNonPushedNonRefPictures(const VIDPROD_Handle_t ProducerHandle);
static void vidprod_ReleasePushedNonRefPictures(const VIDPROD_Handle_t ProducerHandle);
static void vidprod_DecodeStartConditionCheck(const VIDPROD_Handle_t ProducerHandle);
static void vidprod_CheckSequenceInfoChangeAndNotify(const VIDPROD_Handle_t ProducerHandle, const STVID_SequenceInfo_t SequenceInfo);
static BOOL vidprod_IsEventToBeNotified(const VIDPROD_Handle_t ProducerHandle, const U32 Event_ID);

static void vidprod_PatchColorSpaceConversionForMPEG(const VIDPROD_Handle_t ProducerHandle, VIDBUFF_PictureBuffer_t *PictureBuffer_p);
static void vidprod_PatchColorSpaceConversionForH264(const VIDPROD_Handle_t ProducerHandle, VIDBUFF_PictureBuffer_t *PictureBuffer_p);
static void vidprod_PatchColorSpaceConversionForMPEG4P2(const VIDPROD_Handle_t ProducerHandle, VIDBUFF_PictureBuffer_t *PictureBuffer_p);
static void InitUnderflowEvent(const VIDPROD_Handle_t ProducerHandle);
static void vidprod_RemovePictureFromDecodeStatusTable(const VIDPROD_Handle_t ProducerHandle, VIDBUFF_PictureBuffer_t *PictureBuffer_p);
static void vidprod_RemovePictureFromPreprocessingTable(const VIDPROD_Handle_t ProducerHandle, VIDBUFF_PictureBuffer_t *PictureBuffer_p);

static void vidprod_GetNbParsedNotPushedOrReferencePictures(const VIDPROD_Handle_t ProducerHandle, U32 *NbParsedNotPushedPictures);
static void vidprod_TryToPushPictureToDisplay(const VIDPROD_Handle_t ProducerHandle);
#ifdef CL_FILL_BUFFER_PATTERN
static void vidprod_FillFrameBufferWithPattern(const VIDPROD_Handle_t ProducerHandle, VIDBUFF_PictureBuffer_t *PictureBuffer_p);
#endif
static void vidprod_SearchAndConfirmBuffer(const VIDPROD_Handle_t ProducerHandle, STOS_Clock_t TOut_ClockTicks);
static void vidprod_LaunchDecode(const VIDPROD_Handle_t ProducerHandle);
static void vidprod_ControllerReset(const VIDPROD_Handle_t ProducerHandle);
#if defined(PRODUCER_DUMP_FRAME_BUFFER)
static void vidprod_DumpFBContent(const VIDPROD_Handle_t ProducerHandle);
#endif
static void ComputeNbFieldsDisplayNextDecode(const STVID_MPEGStandard_t MPEGStandard, VIDCOM_PictureGenericData_t * PictureGenericData_p);


#ifdef STVID_DEBUG_PRODUCER
static void CheckPictureAndRefsValidity(VIDBUFF_PictureBuffer_t *PictureBuffer_p);
#endif

static ST_ErrorCode_t vidprod_FlushPresentationQueue(const VIDPROD_Handle_t ProducerHandle, VIDBUFF_PictureBuffer_t * PictureBuffer_p);

/* Global Variables --------------------------------------------------------- */

const VIDPROD_FunctionsTable_t   VIDPROD_Functions =
{
    vidprod_ConfigureEvent,
    vidprod_DataInjectionCompleted,
    vidprod_FlushInjection,
    vidprod_Freeze,
    vidprod_GetBitBufferFreeSize,
    vidprod_GetDecodedPictures,
    vidprod_GetDecoderSpecialOperatingMode,
    vidprod_GetErrorRecoveryMode,
    vidprod_GetHDPIPParams,
    vidprod_GetLastDecodedPictureInfos,
    vidprod_GetInfos,
#ifdef STVID_DEBUG_GET_STATISTICS
    vidprod_GetStatistics,
    vidprod_ResetStatistics,
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
    vidprod_GetStatus,
#endif /* STVID_DEBUG_GET_STATUS */
    vidprod_Init,
    vidprod_NewReferencePicture,
    vidprod_NotifyDecodeEvent,
    vidprod_Resume,
    vidprod_SetDecodedPictures,
    vidprod_SetErrorRecoveryMode,
    vidprod_SetHDPIPParams,
    vidprod_Setup,
    vidprod_SkipData,
    vidprod_Start,
    vidprod_StartUpdatingDisplay,
    vidprod_Stop,
    vidprod_Term,
#ifdef ST_deblock
    vidprod_SetDeblockingMode,
#ifdef VIDEO_DEBLOCK_DEBUG
    vidprod_SetDeblockingStrength,
#endif /* VIDEO_DEBLOCK_DEBUG  */
#endif /* ST_deblock */
/* extension for injection controle */
    vidprod_SetDataInputInterface,
    vidprod_DeleteDataInputInterface,
    vidprod_GetDataInputBufferParams,
    vidprod_GetBitBufferParams
};

#ifdef ST_Mpeg2CodecFromOldDecodeModule
/* Temporary Imported Functions ---------------------------------------------------- */
extern ST_ErrorCode_t VIDDEC_Init(const VIDPROD_InitParams_t * const InitParams_p, VIDPROD_Handle_t * const DecodeHandle_p);
extern ST_ErrorCode_t VIDDEC_Term(const VIDPROD_Handle_t DecodeHandle);
#endif

/* Public Functions --------------------------------------------------------- */
#if defined(VALID_TOOLS) || defined(ST_crc) /* && defined(STVID_FRAME_BUFFER_DUMP) */
BOOL vidprod_SavePicture(U8 format, U32 width, U32 height, void * LumaAddress_p, void * ChromaAddress_p, char * Filename_p)
{
#if defined ST_OSLINUX
        STTBX_Print(("Save Picture Unsupported under linux\n"));
        return FALSE;
#else
FILE    *Fp;
U32     rows=0;
U32     cols=0;
U32     lumaBytes=0;
U32     chromaBytes=0;
U32     header[6];
char    Filename[FILENAME_MAX_LENGTH];

    strcpy(Filename, Filename_p);
    strcat(Filename, ".gam");

    /* Fill header */
    header[1] = (U32)format & 0xffff;
    header[2] = width;
    header[3] = height;

    switch (format)
    {
    case OMEGA2_420:        /* Omega2 4:2:0 */
        header[0] = (0x420F << 16) | 0x6;
        header[1] |= 0x00100000;
        rows = ((height+31) >> 5) << 1;
        cols = (width+15) >> 4;
        lumaBytes = header[4] = rows*cols*256;  /* size of luma data buffer */
        chromaBytes = header[5] = (cols & 1) && ((rows>>1) & 1) ? rows*(cols+1)*128 : rows*cols*128;    /* size of chroma data buffer */
        break;
    default:
        STTBX_Print(("Unsupported format\n"));
        return FALSE;
        break;
    }

    /* Open files */
    if ((Fp = fopen(Filename, "wb")) == NULL)
    {
        STTBX_Print(("Error opening file %s\n", Filename));
        return FALSE;
    }

    /* Write data to files */
    fwrite(header, 4, 6, Fp);
#if !defined(STVID_SKIP_FRAME_BUFFER_DATA)
    fwrite(LumaAddress_p, 4, lumaBytes/4, Fp);
    fwrite(ChromaAddress_p, 4, chromaBytes/4, Fp);
#endif
    fclose(Fp);
    return FALSE;
#endif
}
#endif /* defined(VALID_TOOLS) && defined(STVID_FRAME_BUFFER_DUMP) */

#if defined(VALID_TOOLS) /* && defined(STVID_FB_ADD_DATA_DUMP) */
BOOL vidprod_SaveAddData(U32 ProfileAndLevelIndication, U32 width, U32 height, void * AddDataAddress_p, char * Filename_p)

{
    FILE    *Fp;
    U32     rows=0;
    U32     cols=0;
    U32     DataBytes=0;
    char    Filename[FILENAME_MAX_LENGTH];

    strcpy(Filename, Filename_p);
    strcat(Filename, ".bin");

    if((ProfileAndLevelIndication & 0xFF) < 40)
    {
        DataBytes = (((width+15)/16)*((height+15)/16)) * 160;  /* size of data : 160 bytes/MB for level < 4 */
    }
    else
    {
        DataBytes = (((width+15)/16)*((height+15)/16)) * 64;  /* size of data : 64 bytes/MB for level >= 4 */
    }

    /* Open files */
    if ((Fp = fopen(Filename, "wb")) == NULL)
    {
        STTBX_Print(("Error opening file %s\n", Filename));
        return FALSE;
    }

    /* Write data to files */
#if !defined(STVID_SKIP_FRAME_BUFFER_DATA)
    fwrite(AddDataAddress_p, 1, DataBytes, Fp);
#endif
    fclose(Fp);
    return FALSE;
}
#endif /* defined(VALID_TOOLS) && defined(STVID_FB_ADD_DATA_DUMP) */

/*******************************************************************************
Name        : vidprod_Init
Description : Initialise decode
Parameters  : Init params, pointer to video decode handle returned if success
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if bad parameter
              ST_ERROR_NO_MEMORY if memory allocation fails
              STVID_ERROR_EVENT_REGISTRATION if problem registering events
*******************************************************************************/
ST_ErrorCode_t vidprod_Init(const VIDPROD_InitParams_t * const InitParams_p, VIDPROD_Handle_t * const ProducerHandle_p)
{
    VIDPROD_Data_t *                VIDPROD_Data_p;
    STEVT_OpenParams_t              STEVT_OpenParams;
    STEVT_DeviceSubscribeParams_t   STEVT_SubscribeParams;
    DECODER_InitParams_t            DECODERInitParams;
    PARSER_InitParams_t             PARSERInitParams;
    VIDBUFF_AllocateBufferParams_t  AllocateParams;
#ifdef ST_avsync
    STCLKRV_OpenParams_t            STCkrvOpenParams;
#endif /* ST_avsync */
    U32                             UserDataSize;
    ST_ErrorCode_t                  ErrorCode;
    U32                             LoopCounter;
#ifdef ST_Mpeg2CodecFromOldDecodeModule /* Temporary for quick mpeg2 codec implementation based on viddec */
    VIDPROD_Handle_t                VIDDEC_Handle;
    VIDPROD_InitParams_t            InitParamsForViddec;
#endif

    if ((ProducerHandle_p == NULL) || (InitParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    *ProducerHandle_p = NULL;

    /* Allocate a VIDPROD structure */
    VIDPROD_Data_p = (VIDPROD_Data_t *) STOS_MemoryAllocate(InitParams_p->CPUPartition_p, sizeof(VIDPROD_Data_t));
    if (VIDPROD_Data_p == NULL)
    {
        /* Error: allocation failed */
        return(ST_ERROR_NO_MEMORY);
    }

#ifdef ST_Mpeg2CodecFromOldDecodeModule
    /* VIDEC_Init not made, set NULL handle to prevent VIDDEC_Term in VIDPROD_Term */
    VIDPROD_Data_p->VIDDEC_Handle = NULL;
#endif

    /* Remember partition */
    VIDPROD_Data_p->CPUPartition_p = InitParams_p->CPUPartition_p;

    /* Stores CodecMode */
    VIDPROD_Data_p->CodecMode = InitParams_p->CodecMode;

    /* Allocation succeeded: make handle point on structure */
    *ProducerHandle_p = (VIDPROD_Handle_t *) VIDPROD_Data_p;
    VIDPROD_Data_p->ValidityCheck = VIDPROD_VALID_HANDLE;

    /* Init producer data structure */
    InitProducerStructure(*ProducerHandle_p);

    VIDPROD_Data_p->PreprocessingPicture.NbUsedPictureBuffers = 0;
    for(LoopCounter = 0 ; LoopCounter < MAX_NUMBER_OF_PREPROCESSING_PICTURES ; LoopCounter++)
    {
        VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_EMPTY;
        VIDPROD_Data_p->PreprocessingPicture.IsPictureBufferUsed[LoopCounter] = FALSE;
        VIDPROD_Data_p->PreprocessingPicture.IsPictureTimedOut[LoopCounter] = FALSE;
    }

    /* Allow UserData event if required. */
    VIDPROD_Data_p->UserDataEventEnabled = (InitParams_p->UserDataSize != 0);

    /* Allocate table of user data */
    UserDataSize = InitParams_p->UserDataSize;
    /* So that we can eploit uesr data for our purpose, we need a minimum amount of user data (even if requested 0 for example) */
    if (UserDataSize < MIN_USER_DATA_SIZE)
    {
        UserDataSize = MIN_USER_DATA_SIZE;
    }

    /* Store parameters */
    VIDPROD_Data_p->BufferManagerHandle  = InitParams_p->BufferManagerHandle;
    VIDPROD_Data_p->OrderingQueueHandle  = InitParams_p->OrderingQueueHandle;

#ifdef ST_inject
    VIDPROD_Data_p->InjecterHandle       = InitParams_p->InjecterHandle;
#endif /* ST_inject */
    VIDPROD_Data_p->DeviceType           = InitParams_p->DeviceType;
    VIDPROD_Data_p->DecoderNumber        = InitParams_p->DecoderNumber;

    /* Get AV mem mapping */
    STAVMEM_GetSharedMemoryVirtualMapping(&(VIDPROD_Data_p->VirtualMapping));

    /* Open EVT handle */
    ErrorCode = STEVT_Open(InitParams_p->EventsHandlerName, &STEVT_OpenParams, &(VIDPROD_Data_p->EventsHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not open EVT !"));
        vidprod_Term(*ProducerHandle_p);
        *ProducerHandle_p = NULL;
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    /* Prepare common subscribe parameters */
    STEVT_SubscribeParams.SubscriberData_p = (void *) (*ProducerHandle_p);
#ifdef ST_avsync
    /* Open clock recovery driver */
    ErrorCode = STCLKRV_Open(InitParams_p->ClockRecoveryName, &STCkrvOpenParams,
            &(VIDPROD_Data_p->StartCondition.STClkrvHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module AVSync init: could not open CLKRV !"));
        vidprod_Term(*ProducerHandle_p);
        *ProducerHandle_p = NULL;
        return(STVID_ERROR_SYSTEM_CLOCK);
    }
#endif /* ST_avsync */


    /* Register events */
    ErrorCode = STEVT_RegisterDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName, STVID_SEQUENCE_INFO_EVT, &(VIDPROD_Data_p->RegisteredEventsID[VIDPROD_SEQUENCE_INFO_EVT_ID]));
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName, STVID_USER_DATA_EVT, &(VIDPROD_Data_p->RegisteredEventsID[VIDPROD_USER_DATA_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName, STVID_NEW_PICTURE_DECODED_EVT, &(VIDPROD_Data_p->RegisteredEventsID[VIDPROD_NEW_PICTURE_DECODED_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName, STVID_DATA_OVERFLOW_EVT, &(VIDPROD_Data_p->RegisteredEventsID[VIDPROD_DATA_OVERFLOW_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName, STVID_DATA_UNDERFLOW_EVT, &(VIDPROD_Data_p->RegisteredEventsID[VIDPROD_DATA_UNDERFLOW_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName, VIDPROD_READY_TO_DECODE_NEW_PICTURE_EVT, &(VIDPROD_Data_p->RegisteredEventsID[VIDPROD_READY_TO_DECODE_NEW_PICTURE_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName, STVID_DATA_ERROR_EVT, &(VIDPROD_Data_p->RegisteredEventsID[VIDPROD_DATA_ERROR_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName, VIDPROD_STOPPED_EVT, &(VIDPROD_Data_p->RegisteredEventsID[VIDPROD_STOPPED_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName, STVID_PICTURE_DECODING_ERROR_EVT, &(VIDPROD_Data_p->RegisteredEventsID[VIDPROD_PICTURE_DECODING_ERROR_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName, VIDPROD_PICTURE_SKIPPED_EVT, &(VIDPROD_Data_p->RegisteredEventsID[VIDPROD_PICTURE_SKIPPED_EVT_ID]));
    }
#ifdef STVID_TRICKMODE_BACKWARD
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName, VIDPROD_BUFFER_FULLY_PARSED_EVT, &(VIDPROD_Data_p->RegisteredEventsID[VIDPROD_BUFFER_FULLY_PARSED_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName, VIDPROD_DECODE_FIRST_PICTURE_BACKWARD_EVT, &(VIDPROD_Data_p->RegisteredEventsID[VIDPROD_DECODE_FIRST_PICTURE_BACKWARD_EVT_ID]));
    }
#endif /* STVID_TRICKMODE_BACKWARD */
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName, VIDPROD_DECODE_ONCE_READY_EVT, &(VIDPROD_Data_p->RegisteredEventsID[VIDPROD_DECODE_ONCE_READY_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName, VIDPROD_RESTARTED_EVT, &(VIDPROD_Data_p->RegisteredEventsID[VIDPROD_RESTARTED_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName, VIDPROD_DECODE_LAUNCHED_EVT, &(VIDPROD_Data_p->RegisteredEventsID[VIDPROD_DECODE_LAUNCHED_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName, VIDPROD_SKIP_LAUNCHED_EVT, &(VIDPROD_Data_p->RegisteredEventsID[VIDPROD_SKIP_LAUNCHED_EVT_ID]));
    }
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: registration failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not register events !"));
        vidprod_Term(*ProducerHandle_p);
        *ProducerHandle_p = NULL;
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    /* Map registers */
    /* This mapping has to be done even for OS not needing registers mapping such as OS21 */
    /* This constraint is due to the fact most of the OS (WINCE, Linux, ...) need register mapping before */
    /* being able to access them ... */
    VIDPROD_Data_p->VidMappedBaseAddress_p = (unsigned long *) STOS_MapRegisters(InitParams_p->RegistersBaseAddress, STVID_MAPPED_WIDTH, "VID");
    STTBX_Print(("VID virtual io kernel address of phys 0x%.8x = 0x%.8x\n", (U32)InitParams_p->RegistersBaseAddress, (U32)VIDPROD_Data_p->VidMappedBaseAddress_p));

    /* Set bit buffer */
    if (!(InitParams_p->BitBufferAllocated))
    {
        /* Bit buffer was not allocated: allocate it. */
        switch (InitParams_p->DeviceType)
        {
            case STVID_DEVICE_TYPE_7100_H264 :
            case STVID_DEVICE_TYPE_7109_H264 :
            case STVID_DEVICE_TYPE_7200_H264 :
#ifdef SUPPORT_ONLY_SD_PROFILES
                AllocateParams.BufferSize = DEFAULT_BIT_BUFFER_SIZE_H264_SD;
#else
                AllocateParams.BufferSize = DEFAULT_BIT_BUFFER_SIZE_H264_HD;
#endif
                break;
            case STVID_DEVICE_TYPE_7109_VC1 :
            case STVID_DEVICE_TYPE_7200_VC1 :
#ifdef SUPPORT_ONLY_SD_PROFILES
                AllocateParams.BufferSize = DEFAULT_BIT_BUFFER_SIZE_VC1_SD;
#else
                AllocateParams.BufferSize = DEFAULT_BIT_BUFFER_SIZE_VC1_HD;
#endif
                break;
            case STVID_DEVICE_TYPE_7100_MPEG4P2 :
            case STVID_DEVICE_TYPE_7109_MPEG4P2 :
            case STVID_DEVICE_TYPE_7200_MPEG4P2 :
            case STVID_DEVICE_TYPE_7109_AVS : /* PLE_TO_DO : create a convenient default bit buffer size for AVS SD */
                AllocateParams.BufferSize = DEFAULT_BIT_BUFFER_SIZE_MPEG4P2_SD;
                break;
            case STVID_DEVICE_TYPE_7100_MPEG :
            case STVID_DEVICE_TYPE_7109_MPEG :
            case STVID_DEVICE_TYPE_7109D_MPEG :
            case STVID_DEVICE_TYPE_7200_MPEG :
            default :
#ifdef SUPPORT_ONLY_SD_PROFILES
                AllocateParams.BufferSize = DEFAULT_BIT_BUFFER_SIZE_MPEG_SD;
#else
                AllocateParams.BufferSize = DEFAULT_BIT_BUFFER_SIZE_MPEG_HD;
#endif
                break;
        }

        AllocateParams.AllocationMode = VIDBUFF_ALLOCATION_MODE_SHARED_MEMORY; /* !!! Should be passed from higher level */
        AllocateParams.BufferType = VIDBUFF_BUFFER_SIMPLE;

        ErrorCode = VIDBUFF_AllocateBitBuffer(VIDPROD_Data_p->BufferManagerHandle, &AllocateParams, &(VIDPROD_Data_p->BitBuffer));
        if (ErrorCode != ST_NO_ERROR)
        {
            /* Error: cannot allocate bit buffer, undo initialisations done */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not allocate bit buffer !"));
            vidprod_Term(*ProducerHandle_p);
            *ProducerHandle_p = NULL;
            return(ST_ERROR_NO_MEMORY);
        }

        VIDPROD_Data_p->BitBufferAllocatedByUs = TRUE;
    }
    else
    {
        /* Bit buffer allocated by user: just copy parameters */
        VIDPROD_Data_p->BitBuffer.Address_p  = InitParams_p->BitBufferAddress_p;
        VIDPROD_Data_p->BitBuffer.TotalSize  = InitParams_p->BitBufferSize;
        VIDPROD_Data_p->BitBuffer.BufferType = VIDBUFF_BUFFER_SIMPLE;

        VIDPROD_Data_p->BitBufferAllocatedByUs = FALSE;
    }

#if defined (STVID_VALID) || defined (STUB_INJECT)
#ifdef ST_OSLINUX
    VIDPROD_Data_p->BitBuffer.Address_p  = (void *)((((U32)(VIDPROD_Data_p->BitBuffer.Address_p) + LINUX_PAGE_ALIGNMENT - 1)/LINUX_PAGE_ALIGNMENT)*LINUX_PAGE_ALIGNMENT);   /* Cast the address to size page */
    VIDPROD_Data_p->BitBuffer.TotalSize  = (VIDPROD_Data_p->BitBuffer.TotalSize/LINUX_PAGE_ALIGNMENT)*LINUX_PAGE_ALIGNMENT;


    StubInject_SetBBInfo(VIDPROD_Data_p->BitBuffer.Address_p, VIDPROD_Data_p->BitBuffer.TotalSize);

    STTBX_Report("VID_BBADDRESS: %8x\n", (U32)VIDPROD_Data_p->BitBuffer.Address_p);
    STTBX_Report("VID_BBSIZE: %d (0x%.8x)\n", (U32)VIDPROD_Data_p->BitBuffer.TotalSize, (U32)VIDPROD_Data_p->BitBuffer.TotalSize);
#else
    STTST_AssignInteger("VID_BBADDRESS", (S32)VIDPROD_Data_p->BitBuffer.Address_p, FALSE);
    STTST_AssignInteger("VID_BBSIZE", VIDPROD_Data_p->BitBuffer.TotalSize, FALSE);
#endif  /* ST_OSLINUX */
#endif /* STUB_INJECT | STVID_VALID */

    VIDPROD_Data_p->DecoderStatus.LastReadAddress_p = VIDPROD_Data_p->BitBuffer.Address_p;

#ifdef ST_Mpeg2CodecFromOldDecodeModule
#if defined(TRACE_UART) && defined(VALID_TOOLS)
    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_INIT_TERM, "pVIDDEC_Ini "));
#endif
    STOS_memcpy(&InitParamsForViddec, InitParams_p, sizeof(VIDPROD_InitParams_t));

    /** affecting VID Base Adress with mapped Base Adress **/
    /* This has to be done after memcpying the init params */
    InitParamsForViddec.RegistersBaseAddress = VIDPROD_Data_p->VidMappedBaseAddress_p;

    if (VIDPROD_Data_p->BitBufferAllocatedByUs)
    {
        InitParamsForViddec.BitBufferAllocated = TRUE;
        InitParamsForViddec.BitBufferAddress_p = VIDPROD_Data_p->BitBuffer.Address_p;
        InitParamsForViddec.BitBufferSize = VIDPROD_Data_p->BitBuffer.TotalSize;
    }
    ErrorCode = VIDDEC_Init(&InitParamsForViddec, &VIDDEC_Handle);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: cannot intitialise viddec (parser & decoder module), undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Producer init: could not initialize VIDDEC sub-module !"));
        vidprod_Term(*ProducerHandle_p);
        *ProducerHandle_p = NULL;
        return(STVID_ERROR_NOT_AVAILABLE);
    }
    /* Handles are the same as VIDDEC one */
    /* since VIDDEC actually handles parser and decoder */
    VIDPROD_Data_p->VIDDEC_Handle = VIDDEC_Handle;
    VIDPROD_Data_p->ParserHandle = (PARSER_Handle_t) VIDDEC_Handle;
    VIDPROD_Data_p->DecoderHandle = (DECODER_Handle_t) VIDDEC_Handle;
#endif /* ST_Mpeg2CodecFromOldDecodeModule */
#ifdef STVID_TRICKMODE_BACKWARD
        /* Cross references for buffers A and B. */
        VIDPROD_Data_p->LinearBitBuffers.BufferA.AssociatedLinearBitBuffer_p = &(VIDPROD_Data_p->LinearBitBuffers.BufferB);
        VIDPROD_Data_p->LinearBitBuffers.BufferB.AssociatedLinearBitBuffer_p = &(VIDPROD_Data_p->LinearBitBuffers.BufferA);

        /* init */
        vidprod_SmoothBackwardInitStructure(*ProducerHandle_p);
        vidprod_AllocateLinearBitBuffers(*ProducerHandle_p);
#endif /* STVID_TRICKMODE_BACKWARD */
    PARSERInitParams.CPUPartition_p = VIDPROD_Data_p->CPUPartition_p;
    strcpy(PARSERInitParams.EventsHandlerName, InitParams_p->EventsHandlerName);
    strcpy(PARSERInitParams.VideoName, InitParams_p->VideoName);              /* Name of the video instance */
    PARSERInitParams.BitBufferAddress_p = VIDPROD_Data_p->BitBuffer.Address_p;
    PARSERInitParams.BitBufferSize = VIDPROD_Data_p->BitBuffer.TotalSize;
    PARSERInitParams.UserDataSize = UserDataSize;
    PARSERInitParams.DeviceType = InitParams_p->DeviceType;
    PARSERInitParams.SharedMemoryBaseAddress_p = InitParams_p->SharedMemoryBaseAddress_p;
    PARSERInitParams.AvmemPartitionHandle = InitParams_p->AvmemPartitionHandle;

    /** affecting VID Base Adress with mapped Base Adress **/
    PARSERInitParams.RegistersBaseAddress_p = VIDPROD_Data_p->VidMappedBaseAddress_p;
    PARSERInitParams.CompressedDataInputBaseAddress_p = InitParams_p->CompressedDataInputBaseAddress_p;
    PARSERInitParams.DecoderNumber = InitParams_p->DecoderNumber;  /* As defined in vid_com.c. See definition there. */
#ifdef ST_inject
    PARSERInitParams.InjecterHandle = VIDPROD_Data_p->InjecterHandle;
#endif /* ST_inject */

#if defined(TRACE_UART) && defined(VALID_TOOLS)
    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_INIT_TERM, "pPARS_Ini "));
#endif
    ErrorCode = PARSER_Init(&PARSERInitParams, &(VIDPROD_Data_p->ParserHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: cannot intitialise parser, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Producer init: could not initialize Parser sub-module !"));
        vidprod_Term(*ProducerHandle_p);
        *ProducerHandle_p = NULL;
        return(STVID_ERROR_NOT_AVAILABLE);
    }

    DECODERInitParams.CPUPartition_p = VIDPROD_Data_p->CPUPartition_p;
    DECODERInitParams.AvmemPartitionHandle = InitParams_p->AvmemPartitionHandle;
    strcpy(DECODERInitParams.EventsHandlerName,InitParams_p->EventsHandlerName);
    DECODERInitParams.SharedMemoryBaseAddress_p = InitParams_p->SharedMemoryBaseAddress_p;
#ifdef ST_XVP_ENABLE_FGT
    DECODERInitParams.FGTHandle = InitParams_p->FGTHandle;
#endif

    /** affecting VID Base Adress with physical address which is needed by the decoder to init the firware transformer **/
    DECODERInitParams.RegistersBaseAddress_p = (U32 *) InitParams_p->RegistersBaseAddress;

#if defined(ST_7200)
    memset(DECODERInitParams.MMETransportName, 0, 32);
    strncpy(DECODERInitParams.MMETransportName, InitParams_p->MMETransportName, strlen(InitParams_p->MMETransportName));
#endif /* #if defined(ST_7200) ... */

    strcpy(DECODERInitParams.VideoName, InitParams_p->VideoName);              /* Name of the video instance */
    DECODERInitParams.BitBufferAddress_p = VIDPROD_Data_p->BitBuffer.Address_p;
    DECODERInitParams.BitBufferSize = VIDPROD_Data_p->BitBuffer.TotalSize;
    DECODERInitParams.DeviceType = InitParams_p->DeviceType;
    DECODERInitParams.DecoderNumber = InitParams_p->DecoderNumber;  /* As defined in vid_com.c. See definition there. */
    DECODERInitParams.SourceType = DECODER_SOURCE_TYPE_H264_COMPRESSED_PICTURE; /* Forced to H264 for quick development !! */
    DECODERInitParams.ErrorRecoveryMode = DECODER_ERROR_RECOVERY_MODE1;     /*  TODO_CL : Not yet defined, should be defined according to VIDPROD_Data_p->ErrorRecoveryMode value */
    DECODERInitParams.SharedItParams.Event = 0;
    DECODERInitParams.SharedItParams.Level = InitParams_p->InterruptLevel;
    DECODERInitParams.SharedItParams.Number = InitParams_p->InterruptNumber;
    DECODERInitParams.CodecMode = VIDPROD_Data_p->CodecMode;

#if defined(TRACE_UART) && defined(VALID_TOOLS)
    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_INIT_TERM, "pDEC_Ini "));
#endif
    ErrorCode = DECODER_Init(&DECODERInitParams, &(VIDPROD_Data_p->DecoderHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: cannot intitialise decoder, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Producer init: could not initialize Decoder sub-module !"));
        vidprod_Term(*ProducerHandle_p);
        *ProducerHandle_p = NULL;
        return(STVID_ERROR_NOT_AVAILABLE);
    }

#if defined(TRACE_UART) && defined(VALID_TOOLS)
    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_INIT_TERM, "pDEC_Info "));
#endif
    /* Assume level 3.2 by default, just to pass a valid value at the init stage */
    ErrorCode = DECODER_GetCodecInfo(VIDPROD_Data_p->DecoderHandle, &(VIDPROD_Data_p->CodecInfo), 32, VIDPROD_Data_p->CodecMode);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* TODO_CL: error handling to be defined */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER_GetCodecInfo failed Error=%d!!", ErrorCode));
        vidprod_Term(*ProducerHandle_p);
        *ProducerHandle_p = NULL;
        return(STVID_ERROR_NOT_AVAILABLE);
    }

    /* Start decode task */
    ErrorCode = StartProducerTask(*ProducerHandle_p);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: task creation failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not start display task ! (%s)", (ErrorCode == ST_ERROR_ALREADY_INITIALIZED?"already exists":"failed")));
        vidprod_Term(*ProducerHandle_p);
        *ProducerHandle_p = NULL;
        if (ErrorCode == ST_ERROR_ALREADY_INITIALIZED)
        {
            /* Change error code because ST_ERROR_ALREADY_INITIALIZED is not allowed for _Init() functions */
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        return(ErrorCode);
    }

    /* Subscribe to VIDBUFF new picture buffer available EVT */
    STEVT_SubscribeParams.NotifyCallback = NewPictureBufferAvailableCB;
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName,
                    VIDBUFF_NEW_PICTURE_BUFFER_AVAILABLE_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not subscribe to VIDBUFF_NEW_PICTURE_BUFFER_AVAILABLE_EVT !"));
        vidprod_Term(*ProducerHandle_p);
        *ProducerHandle_p = NULL;
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Subscribe to PARSER  PARSER_JOB_COMPLETED_EVT */
    STEVT_SubscribeParams.NotifyCallback = ParserJobCompletedCB;
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName,
                    PARSER_JOB_COMPLETED_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not subscribe to PARSER_JOB_COMPLETED_EVT !"));
        vidprod_Term(*ProducerHandle_p);
        *ProducerHandle_p = NULL;
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Subscribe to PARSER  PARSER_PICTURE_SKIPPED_EVT */
    STEVT_SubscribeParams.NotifyCallback = ParserPictureSkippedCB;
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName,
                    PARSER_PICTURE_SKIPPED_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not subscribe to PARSER_PICTURE_SKIPPED_EVT !"));
        vidprod_Term(*ProducerHandle_p);
        return(ST_ERROR_BAD_PARAMETER);
    }

#ifdef STVID_TRICKMODE_BACKWARD
    /* Subscribe to PARSER PARSER_BITBUFFER_FULLY_PARSED_EVT */
    STEVT_SubscribeParams.NotifyCallback = ParserBitBufferFullyParsedCB;
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName,
                    PARSER_BITBUFFER_FULLY_PARSED_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not subscribe to PARSER_BITBUFFER_FULLY_PARSED_EVT !"));
        vidprod_Term(*ProducerHandle_p);
        *ProducerHandle_p = NULL;
        return(ST_ERROR_BAD_PARAMETER);
    }
#endif

    /* Subscribe to PARSER PARSER_FIND_DISCONTINUITY_EVT */
    STEVT_SubscribeParams.NotifyCallback = ParserFindDiscontinuityCB;
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName,
                    PARSER_FIND_DISCONTINUITY_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not subscribe to PARSER_FIND_DISCONTINUITY_EVT !"));
        vidprod_Term(*ProducerHandle_p);
        *ProducerHandle_p = NULL;
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Subscribe to PARSER  PARSER_USER_DATA_EVT */
    STEVT_SubscribeParams.NotifyCallback = ParserUserDataCB;
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName,
                    PARSER_USER_DATA_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not subscribe to PARSER_USER_DATA_EVT !"));
        vidprod_Term(*ProducerHandle_p);
        *ProducerHandle_p = NULL;
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Subscribe to DECODER DECODER_JOB_COMPLETED_EVT */
    STEVT_SubscribeParams.NotifyCallback = DecoderJobCompletedCB;
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName,
                    DECODER_JOB_COMPLETED_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not subscribe to DECODER_JOB_COMPLETED_EVT !"));
        vidprod_Term(*ProducerHandle_p);
        *ProducerHandle_p = NULL;
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Subscribe to DECODER DECODER_CONFIRM_BUFFER_EVT  */
    STEVT_SubscribeParams.NotifyCallback = DecoderConfirmBufferCB;
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName,
                    DECODER_CONFIRM_BUFFER_EVT , &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not subscribe to DECODER_CONFIRM_BUFFER_EVT !"));
        vidprod_Term(*ProducerHandle_p);
        *ProducerHandle_p = NULL;
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Subscribe to DECODER DECODER_INFORM_READ_ADDRESS_EVT  */
    STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) DecoderInformReadAddressCB;
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDPROD_Data_p->EventsHandle, InitParams_p->VideoName,
                    DECODER_INFORM_READ_ADDRESS_EVT , &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not subscribe to DECODER_INFORM_READ_ADDRESS_EVT !"));
        vidprod_Term(*ProducerHandle_p);
        *ProducerHandle_p = NULL;
        return(ST_ERROR_BAD_PARAMETER);
    }
    InitUnderflowEvent(*ProducerHandle_p);
    VIDPROD_Data_p->NeedToDecreaseBBL = FALSE;
    VIDPROD_Data_p->OverFlowEvtAlreadySent = FALSE;

#if defined(TRACE_UART) && !defined(VALID_TOOLS)
    TraceBuffer(("\r\nProducer module initialized."));
#endif /* TRACE_UART */

    return(ErrorCode);

} /* end of vidprod_Init() function */


/*******************************************************************************
Name        : vidprod_Setup
Description : Setup an object in the decoder. STVID_SetupObject_t supported:
              - STVID_SETUP_DECODER_INTERMEDIATE_BUFFER_PARTITION
              - STVID_SETUP_FDMA_NODES_PARTITION
              - STVID_SETUP_ES_COPY_BUFFER
Parameters  : Video decode handle, setup params
Assumptions : For secure platforms: ES Copy buffer allocated as a single buffer and STVID_SETUP_ES_COPY_BUFFER_PARTITION already set.
Limitations :
Returns     : ST_NO_ERROR if done
              ST_ERROR_BAD_PARAMETER if bad parameter
              ST_ERROR_NO_MEMORY        if no memory to allocate object
*******************************************************************************/
ST_ErrorCode_t vidprod_Setup(const VIDPROD_Handle_t ProducerHandle, const STVID_SetupParams_t * const SetupParams_p)
{
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (SetupParams_p == NULL)
    {
        return( ST_ERROR_BAD_PARAMETER);
    }

    switch (SetupParams_p->SetupObject)
    {
/*****/ case STVID_SETUP_DECODER_INTERMEDIATE_BUFFER_PARTITION:   /***********************************************************************/
            if (!(VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7100_H264)
             && !(VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_H264)
             && !(VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7200_H264))
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            ErrorCode = DECODER_Setup(VIDPROD_Data_p->DecoderHandle, SetupParams_p);
            break;

/*****/ case STVID_SETUP_FDMA_NODES_PARTITION:   /****************************************************************************************/
            if (!(VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_H264)
             && !(VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_VC1)
             && !(VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2)
             && !(VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS)
             && !(VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7200_H264)
             && !(VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7200_VC1)
             && !(VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2))
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            ErrorCode = PARSER_Setup(VIDPROD_Data_p->ParserHandle, SetupParams_p);
            break;

/*****/ case STVID_SETUP_PARSING_RESULTS_BUFFER_PARTITION:   /********************************************************************************************/
/*****/ case STVID_SETUP_DATA_INPUT_BUFFER_PARTITION:   /****************************************************************************/
            if (!(VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_H264)
/*             && !(VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG)          */
/*             && !(VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_DIGITAL_INPUT) */
             && !(VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_VC1)
/*             && !(VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2)       */
/*             && !(VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS)       */
            )
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            else
            {
                ErrorCode = PARSER_Setup(VIDPROD_Data_p->ParserHandle, SetupParams_p);
            }
            break;

/*****/ case STVID_SETUP_BIT_BUFFER_PARTITION:   /****************************************************************************************/
            if (VIDPROD_Data_p->BitBufferAllocatedByUs)
            {
                VIDBUFF_AllocateBufferParams_t  AllocateParams;

                switch (VIDPROD_Data_p->DeviceType)
                {
                    case STVID_DEVICE_TYPE_7109_H264 :


                    case STVID_DEVICE_TYPE_7109_VC1 :
                        /* Save original Bit Buffer parameters to reallocate with same characteristics */
                        AllocateParams.BufferSize     = VIDPROD_Data_p->BitBuffer.TotalSize;
                        AllocateParams.AllocationMode = VIDPROD_Data_p->BitBuffer.AllocationMode;
                        AllocateParams.BufferType     = VIDPROD_Data_p->BitBuffer.BufferType;
                        ErrorCode = VIDBUFF_ReAllocateBitBuffer(VIDPROD_Data_p->BufferManagerHandle,
                                                                &AllocateParams, &(VIDPROD_Data_p->BitBuffer),
                                                                SetupParams_p->SetupSettings.AVMEMPartition);
                        break;
                    default :
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDPROD_Setup: bit buffer allocation not supported for this codec!"));
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                }

                if (ErrorCode == ST_NO_ERROR)
                {
                    PARSER_ExtraParams_t    BitBufferInitParams;

                    /* Initialize parser with ES Copy values */
                    BitBufferInitParams.BufferAddress_p  = VIDPROD_Data_p->BitBuffer.Address_p;
                    BitBufferInitParams.BufferSize       = VIDPROD_Data_p->BitBuffer.TotalSize;

                    /* Update parser's injection pointer with bit buffer's new addresses */
                    ErrorCode = PARSER_BitBufferInjectionInit(VIDPROD_Data_p->ParserHandle, &BitBufferInitParams);
                }

                if (ErrorCode == ST_NO_ERROR)
                {
                    /* Update decoder's addresses for new bit buffer */
                    ErrorCode = VIDPROD_UpdateBitBufferParams(ProducerHandle,
                                                              VIDPROD_Data_p->BitBuffer.Address_p,
                                                              VIDPROD_Data_p->BitBuffer.TotalSize);
                }
            }
            else
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDPROD_Setup: Can't reallocate bit buffer not originally allocated by driver!"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            break;

#if defined(DVD_SECURED_CHIP)
/*****/ case STVID_SETUP_ES_COPY_BUFFER:         /***************************************************************************************/
                if (!(SetupParams_p->SetupSettings.SingleBuffer.BufferAllocated))
                {
                    VIDBUFF_AllocateBufferParams_t  AllocateParams;

                    /*****************************************/
                    /* Buffer was not allocated: allocate it */
                    /*****************************************/

                    switch (VIDPROD_Data_p->DeviceType)
                    {
                        case STVID_DEVICE_TYPE_7109_H264 :
                            AllocateParams.BufferSize = VIDPROD_Data_p->BitBuffer.TotalSize / H264_ESCOPY_BUFFER_RESIZE_FACTOR;
                            break;
                        case STVID_DEVICE_TYPE_7109_VC1 :
                            AllocateParams.BufferSize = VIDPROD_Data_p->BitBuffer.TotalSize / VC1_ESCOPY_BUFFER_RESIZE_FACTOR;
                            break;
                        default :
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDPROD_Setup: ES Copy buffer allocation not supported for this codec!"));
                            return(ST_ERROR_BAD_PARAMETER);
                    }

                    AllocateParams.AllocationMode = VIDBUFF_ALLOCATION_MODE_SHARED_MEMORY;
                    AllocateParams.BufferType     = VIDBUFF_BUFFER_SIMPLE;

                    ErrorCode = VIDBUFF_AllocateESCopyBuffer(VIDPROD_Data_p->BufferManagerHandle,
                                                             &AllocateParams, &(VIDPROD_Data_p->ESCopyBuffer));

                    if (ErrorCode != ST_NO_ERROR)
                    {
                        /* Cannot allocate ES Copy buffer */
                        /* DG TO DO: how do we handle failure? */
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDPROD_Setup: could not allocate ES Copy buffer !"));
                        ErrorCode = ST_ERROR_NO_MEMORY;
                    }
                    else
                    {
                        VIDPROD_Data_p->ESCopyBufferAllocatedByUs = TRUE;
                    }
                }
                else
                {
                    /**********************************************************/
                    /* ES Copy buffer allocated by user: just copy parameters */
                    /**********************************************************/

                    /* DG TO DO: First, test that parameters are somewhat valid: */
                    /* - Address_p != 0 */
                    /* - MAX_SIZE > TotalSize > 0 */

                    VIDPROD_Data_p->ESCopyBuffer.Address_p  = SetupParams_p->SetupSettings.SingleBuffer.BufferAddress_p;
                    VIDPROD_Data_p->ESCopyBuffer.TotalSize  = SetupParams_p->SetupSettings.SingleBuffer.BufferSize;
                    VIDPROD_Data_p->ESCopyBuffer.BufferType = VIDBUFF_BUFFER_SIMPLE;
                    VIDPROD_Data_p->ESCopyBufferAllocatedByUs = FALSE;
                }

                if (ErrorCode == ST_NO_ERROR)
                {
                    PARSER_ExtraParams_t    SecureInitParams;

                    /* Initialize parser with ES Copy values */
                    SecureInitParams.BufferAddress_p  = VIDPROD_Data_p->ESCopyBuffer.Address_p;
                    SecureInitParams.BufferSize       = VIDPROD_Data_p->ESCopyBuffer.TotalSize;

                    ErrorCode = PARSER_SecureInit(VIDPROD_Data_p->ParserHandle, &SecureInitParams);
                }
                break;
#endif /* DVD_SECURED_CHIP */

/*****/ default:   /**********************************************************************************************************************/
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDPROD_Setup(): Object not supported !"));
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(ErrorCode);

} /* End of vidprod_Setup() function */


#if defined(DVD_SECURED_CHIP)
/*******************************************************************************
Name        : VIDPROD_SetupForH264PreprocWA_GNB42332
Description : Setup WA_42332 h264 preproc data partition
Parameters  : Video decode handle, partition
Assumptions : For secure platforms
Limitations :
Returns     : ST_NO_ERROR if done
              ST_ERROR_BAD_PARAMETER if bad parameter
              ST_ERROR_NO_MEMORY        if no memory to allocate object
*******************************************************************************/
ST_ErrorCode_t VIDPROD_SetupForH264PreprocWA_GNB42332(const VIDPROD_Handle_t ProducerHandle, const STAVMEM_PartitionHandle_t AVMEMPartition)
{
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (AVMEMPartition == (STAVMEM_PartitionHandle_t) NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_H264))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    ErrorCode = DECODER_SetupForH264PreprocWA_GNB42332(VIDPROD_Data_p->DecoderHandle, AVMEMPartition);

    return(ErrorCode);
} /* End of VIDPROD_SetupForH264PreprocWA_GNB42332() */
#endif /* DVD_SECURED_PARTITION */

/*******************************************************************************
Name        : vidprod_Term
Description : Terminate producer, undo all what was done at init
Parameters  : Video producer handle
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_INVALID_HANDLE if not initialised
*******************************************************************************/
ST_ErrorCode_t vidprod_Term(const VIDPROD_Handle_t ProducerHandle)
{
    VIDPROD_Data_t *    VIDPROD_Data_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Find structure */
    VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

    if (VIDPROD_Data_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDPROD_Term() : Unexpected NULL handle"));
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (VIDPROD_Data_p->ValidityCheck != VIDPROD_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
#ifdef ST_Mpeg2CodecFromOldDecodeModule
#if defined(TRACE_UART) && defined(VALID_TOOLS)
    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_INIT_TERM, "pVIDDEC_Term "));
#endif
    if (VIDPROD_Data_p->VIDDEC_Handle != NULL)
    {
        ErrorCode = VIDDEC_Term(VIDPROD_Data_p->VIDDEC_Handle);
    }
#endif /* ST_Mpeg2CodecFromOldDecodeModule */

#if defined(TRACE_UART) && defined(VALID_TOOLS)
    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_INIT_TERM, "pDEC_Term "));
#endif
    if (VIDPROD_Data_p->DecoderHandle != NULL)
    {
        ErrorCode = DECODER_Term(VIDPROD_Data_p->DecoderHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            /* TODO_CL: error handling to be defined */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER_Term failed Error=%d!!", ErrorCode));
        }
    }

#if defined(TRACE_UART) && defined(VALID_TOOLS)
    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_INIT_TERM, "pPARS_Term "));
#endif
    if (VIDPROD_Data_p->ParserHandle != NULL)
    {
        ErrorCode = PARSER_Term(VIDPROD_Data_p->ParserHandle);
        if (ErrorCode != ST_NO_ERROR)
        {
            /* TODO_CL: error handling to be defined */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "PARSER_Term failed Error=%d!!", ErrorCode));
        }
    }

    if (VIDPROD_Data_p->ProducerTask.Task_p != NULL)
    {
        StopProducerTask(ProducerHandle);
    }

    /* Un-subscribe and un-register events: this is done automatically by STEVT_Close() */
    /* Close instances opened at init */
    ErrorCode = STEVT_Close(VIDPROD_Data_p->EventsHandle);

#ifdef ST_avsync
    STCLKRV_Close(VIDPROD_Data_p->StartCondition.STClkrvHandle);
#endif /* ST_avsync */

    /* De-allocate bit buffer */
    if (VIDPROD_Data_p->BitBufferAllocatedByUs)
    {
        VIDBUFF_DeAllocateBitBuffer(VIDPROD_Data_p->BufferManagerHandle, &(VIDPROD_Data_p->BitBuffer));
/*        VIDPROD_Data_p->BitBufferAllocatedByUs = FALSE;*/ /* Useless, data is lost */
    }

    #if defined(DVD_SECURED_CHIP)
        if (VIDPROD_Data_p->ESCopyBufferAllocatedByUs)
        {
            /* ES Copy buffer was allocated inside driver, deallocate now */
            VIDBUFF_DeAllocateESCopyBuffer(VIDPROD_Data_p->BufferManagerHandle, &(VIDPROD_Data_p->ESCopyBuffer));
        }
    #endif /* DVD_SECURED_CHIP */

    STOS_UnmapRegisters(VIDPROD_Data_p->VidMappedBaseAddress_p, STVID_MAPPED_WIDTH);

    /* Destroy decode structure (delete semaphore, etc) */
    TermProducerStructure(ProducerHandle);

    /* De-validate structure */
    VIDPROD_Data_p->ValidityCheck = 0; /* not VIDPROD_VALID_HANDLE */

    /* De-allocate last: data inside cannot be used after ! */
    STOS_MemoryDeallocate(VIDPROD_Data_p->CPUPartition_p, (void *) VIDPROD_Data_p);

    return(ErrorCode);
} /* end of vidprod_Term() function */

/*******************************************************************************
Name        : vidprod_Start
Description : Start decoding
Parameters  : Video decode handle, start params
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if decoder started
              ST_ERROR_BAD_PARAMETER if bad parameter
              STVID_ERROR_DECODER_RUNNING if the decoder is already started
              STVID_ERROR_DECODER_PAUSING if the decoder is paused
*******************************************************************************/
ST_ErrorCode_t vidprod_Start(const VIDPROD_Handle_t ProducerHandle, const VIDPROD_StartParams_t * const StartParams_p)
{
#if COMPUTE_PRESENTATION_ID
    U32 LoopCounter;
#endif /* #if COMPUTE_PRESENTATION_ID */
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "vidprod_Start() called"));

    if (StartParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        VIDBUFF_GetMemoryProfile(VIDPROD_Data_p->BufferManagerHandle, &VIDPROD_Data_p->ForTask.CurrentProfile);
#if COMPUTE_PRESENTATION_ID
    for (LoopCounter = 0 ; LoopCounter < STORED_PICTURE_BUFFER_SIZE ; LoopCounter++)
    {
        VIDPROD_Data_p->StoredPictureBuffer[LoopCounter] = NULL;
    }
    VIDPROD_Data_p->StoredPictureCounter = 0;
    VIDPROD_Data_p->NumOfPictToWaitBeforePush = 2;
    VIDPROD_Data_p->PreviousPictureCount = 0;
    VIDPROD_Data_p->CurrentPresentationOrderID.ID = 0x80000000;	/* minimum value -2147483648 */
    VIDPROD_Data_p->CurrentPresentationOrderID.IDExtension = 0;
#endif /* #if COMPUTE_PRESENTATION_ID */

        VIDPROD_Data_p->ParserStatus.StreamType = StartParams_p->StreamType;
        VIDPROD_Data_p->ParserStatus.StreamID = StartParams_p->StreamID;
        VIDPROD_Data_p->ParserStatus.IsPictureSkipJustHappened = FALSE;

        VIDPROD_Data_p->BroadcastProfile = StartParams_p->BroadcastProfile;
        vidprod_SetDecodedPictures(ProducerHandle, VIDPROD_Data_p->DecodedPictures);

        if (StartParams_p->UpdateDisplay)
        {
            VIDPROD_Data_p->UpdateDisplayQueue = VIDPROD_DEFAULT_UPDATE_DISPLAY_QUEUE_MODE;
        }
        else
        {
            VIDPROD_Data_p->UpdateDisplayQueue = VIDPROD_UPDATE_DISPLAY_QUEUE_NEVER;
        }

        VIDPROD_Data_p->DecodeOnce                   = StartParams_p->DecodeOnce;
        VIDPROD_Data_p->RealTime                     = StartParams_p->RealTime;

        VIDPROD_Data_p->SendSequenceInfoEventOnceAfterStart = TRUE;

        VIDPROD_Data_p->ForTask.SoftResetPending = TRUE;
        VIDPROD_Data_p->ForTask.SoftResetPosted = TRUE;
        VIDPROD_Data_p->ForTask.NeedForExtraAllocation = TRUE;
        VIDPROD_Data_p->ForTask.MaxSizeReachedForExtraAllocation = FALSE;
        VIDPROD_Data_p->ForTask.MaxSizeReachedForExtraAllocationDecimated = FALSE;
        if (VIDPROD_Data_p->ForTask.CurrentProfile.ApplicableDecimationFactor != STVID_DECIMATION_FACTOR_NONE)
        {
            VIDPROD_Data_p->ForTask.NeedForExtraAllocationDecimated = TRUE;
        }
        VIDPROD_Data_p->ForTask.DynamicAllocatedMainFrameBuffers = 0;
        if (!IsDynamicAllocationProfile(VIDPROD_Data_p->ForTask.CurrentProfile))
        {
            if (VIDPROD_Data_p->ForTask.CurrentProfile.ApiProfile.NbFrameStore == STVID_OPTIMISED_NB_FRAME_STORE_ID)
            {
                VIDPROD_Data_p->ForTask.DynamicAllocatedMainFrameBuffers = VIDPROD_Data_p->ForTask.CurrentProfile.ApiProfile.FrameStoreIDParams.OptimisedNumber.Main;
            }
            else
            {
                VIDPROD_Data_p->ForTask.DynamicAllocatedMainFrameBuffers = VIDPROD_Data_p->ForTask.CurrentProfile.ApiProfile.NbFrameStore - 2;
            }
        }

        /* After a START, we re-send the underflow event : for that, the flag   */
        /* DataUnderflowEventNotSatisfied must be set to FALSE.                 */
        InitUnderflowEvent(ProducerHandle);

#if defined(TRACE_UART) && !defined(VALID_TOOLS)
        TraceBuffer(("* %d Push RESET*** \r\n", __LINE__));
#endif /* TRACE_UART */

        PushControllerCommand(ProducerHandle, CONTROLLER_COMMAND_RESET);
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "push Producer RESET"));

        /* Signal controller that a new command is given: do that as VERY LAST action,
        because as generally the decode task has higher priority than the calling task, it is
        triggered immediately, so all VIDPROD data must be valid before the signal ! */
        STOS_SemaphoreSignal(VIDPROD_Data_p->ProducerOrder_p);
    }

    return(ErrorCode);

} /* End of vidprod_Start() function */

/*******************************************************************************
Name        : vidprod_ConfigureEvent
Description : Configures notification of video decoder events.
Parameters  : Video producer handle
              Event to allow or forbid.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if done successfully
              ST_ERROR_BAD_PARAMETER if the event is not supported.
*******************************************************************************/
ST_ErrorCode_t vidprod_ConfigureEvent(const VIDPROD_Handle_t ProducerHandle, const STEVT_EventID_t Event,
        const STVID_ConfigureEventParams_t * const Params_p)
{
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;

    switch (Event)
    {
        case STVID_NEW_PICTURE_DECODED_EVT :
            VIDPROD_Data_p->EventNotificationConfiguration[VIDPROD_NEW_PICTURE_DECODED_EVT_ID].NotificationSkipped = 0;
            VIDPROD_Data_p->EventNotificationConfiguration[VIDPROD_NEW_PICTURE_DECODED_EVT_ID].ConfigureEventParam = (*Params_p);
            break;

        default :
            /* Error case. This event can't be configured. */
            ErrCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    /* Return the configuration's result. */
    return(ErrCode);

} /* End of vidprod_ConfigureEvent() function */

/* Functions Exported in module --------------------------------------------- */


/* Private Functions -------------------------------------------------------- */
/*******************************************************************************
Name        : IsRequestingAdditionnalData
Description : Check if PPB buffers are needed
Parameters  : DeviceType
Assumptions :
Limitations :
Returns     : Result
*******************************************************************************/
static BOOL IsRequestingAdditionnalData(const VIDPROD_Data_t * const VIDPROD_Data_p)
{
    BOOL  DataRequested = FALSE;

    switch (VIDPROD_Data_p->DeviceType)
    {
        /* STi 7100 */
        case STVID_DEVICE_TYPE_7100_H264:
        /* Sti7109 */
        case STVID_DEVICE_TYPE_7109_H264:
        case STVID_DEVICE_TYPE_7109_VC1:
        /* STi7200 */
        case STVID_DEVICE_TYPE_7200_H264:
        case STVID_DEVICE_TYPE_7200_VC1:
            DataRequested = TRUE;
            break;

        /* PPB needed for transcode */
        case STVID_DEVICE_TYPE_7109D_MPEG:
            if (VIDPROD_Data_p->CodecMode == STVID_CODEC_MODE_TRANSCODE)
            {
                DataRequested = TRUE;
            }
            break;

        default:
            break;
    }

    return (DataRequested);
}

/*******************************************************************************
Name        : InitProducerStructure
Description : Initialise producer structure
Parameters  : Video producer handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void InitProducerStructure(const VIDPROD_Handle_t ProducerHandle)
{
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    U32 LoopCounter;

    VIDPROD_Data_p->EventsHandle = (STEVT_Handle_t) NULL;
    VIDPROD_Data_p->BufferManagerHandle = NULL;
    VIDPROD_Data_p->OrderingQueueHandle = NULL;
    VIDPROD_Data_p->ParserHandle = NULL;
    VIDPROD_Data_p->DecoderHandle = NULL;
#ifdef ST_Mpeg2CodecFromOldDecodeModule
    VIDPROD_Data_p->VIDDEC_Handle = NULL;
#endif
#ifdef ST_inject
    VIDPROD_Data_p->InjecterHandle = NULL;
#endif /* ST_inject */
    VIDPROD_Data_p->ProducerTask.Task_p = NULL;

    VIDPROD_Data_p->BitBufferAllocatedByUs = FALSE;
    VIDPROD_Data_p->LastBitBufferLevel = 0;

    VIDPROD_Data_p->ProducerState = VIDPROD_DECODER_STATE_STOPPED;

    VIDPROD_Data_p->DecoderIsFreezed = FALSE;
    VIDPROD_Data_p->DecodedPictures = STVID_DECODED_PICTURES_ALL;
    VIDPROD_Data_p->DecodedPicturesBeforeLastFreeze = STVID_DECODED_PICTURES_ALL;

    VIDPROD_Data_p->ReplacementReference_p = NULL;
    VIDPROD_Data_p->ReplacementReference_IsAvailable = FALSE;

    VIDPROD_Data_p->Skip.AllIPictures = FALSE;
    VIDPROD_Data_p->Skip.AllPPictures = FALSE;
    VIDPROD_Data_p->Skip.AllBPictures = FALSE;
    VIDPROD_Data_p->Skip.NextBPicture = FALSE;

#if defined(DVD_SECURED_CHIP)
    VIDPROD_Data_p->ESCopyBufferAllocatedByUs = FALSE;
#endif /* DVD_SECURED_CHIP */

#ifdef ST_speed
    VIDPROD_Data_p->Skip.SkipMode = VIDPROD_SKIP_NO_PICTURE;
    VIDPROD_Data_p->SearchFirstI = FALSE;
    VIDPROD_Data_p->Skip.AllExceptFirstI = FALSE;
    VIDPROD_Data_p->Skip.AllBPicturesSpeed = FALSE;
    VIDPROD_Data_p->Skip.AllPPicturesSpeed = FALSE;
    VIDPROD_Data_p->Skip.NextIPicture = FALSE;
    VIDPROD_Data_p->Skip.UnderflowOccured = FALSE;
	VIDPROD_Data_p->Skip.UntilNextIPictureSpeed = FALSE;
    /* Initialize Bit Rate computing Params */
    VIDPROD_Data_p->BitRateValueEvaluate.LastBitBufferOutputCounter      = 0;
    VIDPROD_Data_p->BitRateValueEvaluate.BitRateValueIsComputable        = TRUE;
    VIDPROD_Data_p->BitRateValueEvaluate.BitRateValidValueNb             = 0;
    VIDPROD_Data_p->BitRateValueEvaluate.BitRateIndex                    = 0;
    VIDPROD_Data_p->BitRateValueEvaluate.BitRateValueIsUnderComputing    = FALSE;
    for (LoopCounter = 0; LoopCounter < VIDPROD_NB_PICTURES_USED_FOR_BITRATE; LoopCounter++)
    {
        VIDPROD_Data_p->BitRateValueEvaluate.Time[LoopCounter]           = 0;
        VIDPROD_Data_p->BitRateValueEvaluate.BitBufferSize[LoopCounter]  = 0;
    }
    VIDPROD_Data_p->IsBitRateComputable = TRUE;
#ifdef STVID_TRICKMODE_BACKWARD
    VIDPROD_Data_p->Backward.IsBackward = FALSE;
    VIDPROD_Data_p->Backward.IsPreviousInjectionFine = TRUE;
    VIDPROD_Data_p->Backward.IsInjectionNeeded = FALSE;
    VIDPROD_Data_p->Backward.DoNotTreatParserJobCompletedEvt = FALSE;
    VIDPROD_Data_p->Backward.InjectDiscontinuityCanBeAccepted = TRUE;
    VIDPROD_Data_p->Backward.WaitingProducerReadyToSwitchBackward = FALSE;
    VIDPROD_Data_p->Backward.WaitingProducerReadyToSwitchForward = FALSE;
    VIDPROD_Data_p->Backward.ChangingDirection = TRICKMODE_NOCHANGE;
    VIDPROD_Data_p->Backward.JustSwitchForward = FALSE;
    VIDPROD_Data_p->Backward.NbDecodedPictures = 0;
    VIDPROD_Data_p->Backward.TimeToDecodeIPicture = 0;
    VIDPROD_Data_p->DriftTime = 0;
#endif /*STVID_TRICKMODE_BACKWARD*/
#endif /*ST_speed*/
    VIDPROD_Data_p->ForTask.NewPictureDecodeOrSkipEventData.NbFieldsForDisplay = 0;
    VIDPROD_Data_p->ForTask.NewPictureDecodeOrSkipEventData.PictureInfos_p = NULL;
    VIDPROD_Data_p->ForTask.NewPictureDecodeOrSkipEventData.TemporalReference = 0;
    VIDPROD_Data_p->ForTask.NewPictureDecodeOrSkipEventData.VbvDelay = 0;

    VIDPROD_Data_p->ForTask.NeedForExtraAllocation = FALSE;
    VIDPROD_Data_p->ForTask.NeedForExtraAllocationDecimated = FALSE;
    VIDPROD_Data_p->ForTask.MaxSizeReachedForExtraAllocation = FALSE;
    VIDPROD_Data_p->ForTask.MaxSizeReachedForExtraAllocationDecimated = FALSE;
    VIDPROD_Data_p->ForTask.DynamicAllocatedMainFrameBuffers = 0;

    VIDPROD_Data_p->Skip.UntilNextIPicture = FALSE;
    VIDPROD_Data_p->Skip.UntilNextSequence = FALSE;
    VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint = FALSE;
    VIDPROD_Data_p->Skip.UntilNextRandomAccessPointJustRequested = FALSE;
    VIDPROD_Data_p->Skip.RecoveringNextEntryPoint = FALSE;

    VIDPROD_Data_p->Stop.WhenNextReferencePicture = FALSE;
    VIDPROD_Data_p->Stop.WhenNoData = FALSE;
    VIDPROD_Data_p->Stop.StopAfterBuffersTimeOut = FALSE;
    VIDPROD_Data_p->Stop.DiscontinuityStartCodeInserted = FALSE;
    VIDPROD_Data_p->Stop.WhenNextIPicture = FALSE;
    VIDPROD_Data_p->Stop.AfterCurrentPicture = FALSE;
    VIDPROD_Data_p->Stop.IsPending = FALSE;

    VIDPROD_Data_p->Stop.ActuallyStoppedSemaphore_p = STOS_SemaphoreCreateFifoTimeOut(VIDPROD_Data_p->CPUPartition_p, 0);

    VIDPROD_Data_p->SendSequenceInfoEventOnceAfterStart = FALSE;

    /* Decode events notification configuration ----------------------------- */

    /* Initialise the event notification configuration. */
    for (LoopCounter = 0; LoopCounter < VIDPROD_NB_REGISTERED_EVENTS_IDS; LoopCounter++)
    {
        VIDPROD_Data_p->EventNotificationConfiguration[LoopCounter].NotificationSkipped                     = 0;
        VIDPROD_Data_p->EventNotificationConfiguration[LoopCounter].ConfigureEventParam.NotificationsToSkip = 0;
        VIDPROD_Data_p->EventNotificationConfiguration[LoopCounter].ConfigureEventParam.Enable              = TRUE;
    }
    /* Set the default value of specific event. */
    VIDPROD_Data_p->EventNotificationConfiguration[VIDPROD_NEW_PICTURE_DECODED_EVT_ID].ConfigureEventParam.Enable = FALSE;

    /* Controller ----------------------------------------------------------- */

    VIDPROD_Data_p->ProducerTask.IsRunning  = FALSE;

#ifdef STVID_MEASURES
    VIDPROD_Data_p->ProducerOrder_p = STOS_SemaphoreCreateFifo(VIDPROD_Data_p->CPUPartition_p, 0);
#else /* STVID_MEASURES */
    VIDPROD_Data_p->ProducerOrder_p = STOS_SemaphoreCreateFifoTimeOut(VIDPROD_Data_p->CPUPartition_p, 0);
#endif /* not STVID_MEASURES */

#ifdef BENCHMARK_WINCESTAPI
	P_ADDSEMAPHORE(VIDPROD_Data_p->ProducerOrder_p , "VIDPROD_Data_p->ProducerOrder_p");
#endif
    /* Initialise commands queue */
    VIDPROD_Data_p->CommandsBuffer.Data_p         = VIDPROD_Data_p->CommandsData;
    VIDPROD_Data_p->CommandsBuffer.TotalSize      = sizeof(VIDPROD_Data_p->CommandsData);
    VIDPROD_Data_p->CommandsBuffer.BeginPointer_p = VIDPROD_Data_p->CommandsBuffer.Data_p;
    VIDPROD_Data_p->CommandsBuffer.UsedSize       = 0;
    VIDPROD_Data_p->CommandsBuffer.MaxUsedSize    = 0;

    VIDPROD_Data_p->UpdateDisplayQueue = VIDPROD_UPDATE_DISPLAY_QUEUE_NEVER;
    VIDPROD_Data_p->DecodeOnce    = FALSE;
    VIDPROD_Data_p->RealTime      = FALSE;

    VIDPROD_Data_p->UnsuccessfullDecodeCount = 0;

    VIDPROD_Data_p->DecodePicture.NbUsedPictureBuffers = 0;
    for (LoopCounter = 0 ; LoopCounter < MAX_NUMBER_OF_DECODE_PICTURES ; LoopCounter++)
    {
        VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter] = NULL;
        VIDPROD_Data_p->DecodePicture.IsPictureTimedOut[LoopCounter] = FALSE;
    }
    for (LoopCounter = 0 ; LoopCounter < MAX_CURRENT_PICTURES ; LoopCounter++)
    {
        VIDPROD_Data_p->DecoderStatus.CurrentPictures_p[LoopCounter] = NULL;
    }
    VIDPROD_Data_p->DecoderStatus.NbIndices = 0;
    VIDPROD_Data_p->DecoderStatus.NextOutputIndex = 0;

    VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p = STOS_SemaphoreCreatePriority(VIDPROD_Data_p->CPUPartition_p, 1);
    VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureSemaphore_p = STOS_SemaphoreCreatePriority(VIDPROD_Data_p->CPUPartition_p, 1);
#ifdef STVID_TRICKMODE_BACKWARD
    VIDPROD_Data_p->LinearBitBuffers.PreprocessingPictureSemaphore_p = STOS_SemaphoreCreatePriority(VIDPROD_Data_p->CPUPartition_p, 1);
#endif
#ifdef BENCHMARK_WINCESTAPI
	P_ADDSEMAPHORE(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p , "VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p");
	P_ADDSEMAPHORE(VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureSemaphore_p , "VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureSemaphore_p");
#endif
#if defined(PARSE_TABLE)/*parsed picture table removed */
    VIDPROD_Data_p->ParsedPicture.ParsedPictureSemaphore_p = STOS_SemaphoreCreatePriority(VIDPROD_Data_p->CPUPartition_p, 1);
#endif /*ST_speed*/
    /* Parser --------------------------------------------------------------- */
    VIDPROD_Data_p->ParserStatus.IsSearchingPicture = FALSE;
    VIDPROD_Data_p->ParserStatus.MaxPictureSearchTime = 0;
    VIDPROD_Data_p->BroadcastProfile = STVID_BROADCAST_DVB; /* no sense, just not to have 0... */
    VIDPROD_Data_p->ParserStatus.StreamType = STVID_STREAM_TYPE_PES;     /* Stream Type (ES/PES ...) */
    VIDPROD_Data_p->ParserStatus.StreamID = 0;                           /* PES Stream ID */
    VIDPROD_Data_p->ParserStatus.SearchNextRandomAccessPoint = TRUE;     /* Start with "begining" of stream */

    /* Decoder -------------------------------------------------------------- */
    VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue = 0;
    VIDPROD_Data_p->DecoderStatus.LastReadAddress_p = 0;
    VIDPROD_Data_p->DecoderStatus.LastDecodedPicture_p = NULL;
    VIDPROD_Data_p->DecoderStatus.MaxBufferWaitingTime = 0;
    VIDPROD_Data_p->DecoderStatus.MaxErrorRecoveryBufferWaitingTime = 0;
    VIDPROD_Data_p->DecoderStatus.IsPicturePushStarted = FALSE;
    VIDPROD_Data_p->DecoderStatus.FrameBufferUsedSinceLastRAP = 0;
    VIDPROD_Data_p->DecoderStatus.CurrentMaxDecFrameBuffering = ULONG_MAX-1000; /* -1000 to avoid wrap around when adding a few frame number when used for test */

    VIDPROD_Data_p->UserDataEventEnabled = FALSE;

    /* Asynchronous commands ------------------------------------------------ */
    VIDPROD_Data_p->AsynchronousStatus.PresentationFreezed = FALSE;

    /* Initialize the Next Actions. */
    VIDPROD_Data_p->NextAction.LaunchDecodeWhenPossible = 0;
    VIDPROD_Data_p->NextAction.ConfirmBufferWhenPossible = 0;
    VIDPROD_Data_p->NextAction.LaunchSkipWhenPossible = FALSE;
    VIDPROD_Data_p->NextAction.LaunchDisplayWhenPossible = FALSE;
    VIDPROD_Data_p->NextAction.NumberOfDisplayToLaunch = 0;

    VIDPROD_Data_p->DecoderOperatingMode.IsLowDelay = FALSE;

    VIDPROD_Data_p->StartCondition.FirstPictureAfterReset = TRUE;
    VIDPROD_Data_p->StartCondition.StartOnPictureDelay = FALSE;
    VIDPROD_Data_p->StartCondition.StartWithBufferFullness = FALSE;
    VIDPROD_Data_p->StartCondition.MaxWaitForBufferLevel = (STOS_Clock_t) 0;
    VIDPROD_Data_p->StartCondition.BufferFullnessToReachBeforeStartInBytes = 0;
    VIDPROD_Data_p->StartCondition.StartWithPTSTimeComparison = FALSE;
    VIDPROD_Data_p->StartCondition.FirstPicturePTSAfterReset.PTS   = 0;
    VIDPROD_Data_p->StartCondition.FirstPicturePTSAfterReset.PTS33 = FALSE;
    VIDPROD_Data_p->StartCondition.IsDecodeEnable = FALSE;

    VIDPROD_Data_p->HDPIPParams.Enable = FALSE;
    VIDPROD_Data_p->HDPIPParams.WidthThreshold = 1920;
    VIDPROD_Data_p->HDPIPParams.HeightThreshold = 1088;

    VIDPROD_Data_p->ErrorRecoveryMode = STVID_ERROR_RECOVERY_NONE;  /* Default for validation */

#ifdef STVID_DEBUG_GET_STATISTICS
    /* Reset statistics parameters */
    VIDPROD_Data_p->Statistics.DecodeHardwareSoftReset = 0;
    VIDPROD_Data_p->Statistics.DecodeStartCodeFound = 0;
    VIDPROD_Data_p->Statistics.DecodeSequenceFound = 0;
    VIDPROD_Data_p->Statistics.DecodeUserDataFound = 0;
    VIDPROD_Data_p->Statistics.DecodePictureFound = 0;
    VIDPROD_Data_p->Statistics.DecodePictureFoundMPEGFrameI = 0;
    VIDPROD_Data_p->Statistics.DecodePictureFoundMPEGFrameP = 0;
    VIDPROD_Data_p->Statistics.DecodePictureFoundMPEGFrameB = 0;
    VIDPROD_Data_p->Statistics.DecodePictureSkippedRequested = 0;
    VIDPROD_Data_p->Statistics.DecodePictureSkippedNotRequested = 0;
    VIDPROD_Data_p->Statistics.DecodePictureDecodeLaunched = 0;
    VIDPROD_Data_p->Statistics.DecodeStartConditionVbvDelay = 0;
    VIDPROD_Data_p->Statistics.DecodeStartConditionPtsTimeComparison = 0;
    VIDPROD_Data_p->Statistics.DecodeStartConditionVbvBufferSize = 0;
    VIDPROD_Data_p->Statistics.DecodeInterruptStartDecode = 0;
    VIDPROD_Data_p->Statistics.DecodeInterruptPipelineIdle = 0;
    VIDPROD_Data_p->Statistics.DecodeInterruptDecoderIdle = 0;
    VIDPROD_Data_p->Statistics.DecodeInterruptBitBufferEmpty = 0;
    VIDPROD_Data_p->Statistics.DecodeInterruptBitBufferFull = 0;
    VIDPROD_Data_p->Statistics.DecodePbStartCodeFoundInvalid = 0;
    VIDPROD_Data_p->Statistics.DecodePbStartCodeFoundVideoPES = 0;
    VIDPROD_Data_p->Statistics.DecodePbMaxNbInterruptSyntaxErrorPerPicture = 0;
    VIDPROD_Data_p->Statistics.DecodePbInterruptSyntaxError = 0;
    VIDPROD_Data_p->Statistics.DecodePbInterruptDecodeOverflowError = 0;
    VIDPROD_Data_p->Statistics.DecodePbInterruptDecodeUnderflowError = 0;
    VIDPROD_Data_p->Statistics.DecodePbDecodeTimeOutError = 0;
    VIDPROD_Data_p->Statistics.DecodePbInterruptMisalignmentError = 0;
    VIDPROD_Data_p->Statistics.DecodePbInterruptQueueOverflow = 0;
    VIDPROD_Data_p->Statistics.DecodePbHeaderFifoEmpty = 0;
    VIDPROD_Data_p->Statistics.DecodePbVbvSizeGreaterThanBitBuffer = 0;
    VIDPROD_Data_p->Statistics.DecodeMinBitBufferLevelReached = 0;
    VIDPROD_Data_p->Statistics.DecodeMaxBitBufferLevelReached = 0;
    VIDPROD_Data_p->Statistics.DecodePbSequenceNotInMemProfileSkipped = 0;
    VIDPROD_Data_p->Statistics.DecodePbParserError = 0;
    VIDPROD_Data_p->Statistics.DecodePbPreprocError = 0;
    VIDPROD_Data_p->Statistics.DecodePbFirmwareError = 0;
    VIDPROD_Data_p->Statistics.MaxBitRate = 0;
    VIDPROD_Data_p->Statistics.MinBitRate = 0xFFFFFFFF;
    VIDPROD_Data_p->Statistics.LastBitRate = 0;
    VIDPROD_Data_p->Statistics.NbDecodedPicturesB = 0;
    VIDPROD_Data_p->Statistics.NbDecodedPicturesP = 0;
    VIDPROD_Data_p->Statistics.NbDecodedPicturesI = 0;
    VIDPROD_Data_p->Statistics.MaxPositiveDriftRequested = 0;
    VIDPROD_Data_p->Statistics.MaxNegativeDriftRequested = 0;
#endif /* STVID_DEBUG_GET_STATISTICS */
    VIDPROD_Data_p->CurrentPictureNumber = 0;

#if COMPUTE_PRESENTATION_ID
    for (LoopCounter = 0 ; LoopCounter < STORED_PICTURE_BUFFER_SIZE ; LoopCounter++)
    {
        VIDPROD_Data_p->StoredPictureBuffer[LoopCounter] = NULL;
    }
    VIDPROD_Data_p->StoredPictureCounter = 0;
    VIDPROD_Data_p->NumOfPictToWaitBeforePush = 2;
    VIDPROD_Data_p->PreviousPictureCount = 0;
    VIDPROD_Data_p->CurrentPresentationOrderID.ID = 0x80000000;	/* minimum value -2147483648 */
    VIDPROD_Data_p->CurrentPresentationOrderID.IDExtension = 0;
#endif /* #if COMPUTE_PRESENTATION_ID */
} /* end of InitProducerStructure() */

/*******************************************************************************
Name        : StartProducerTask
Description : Start the task of decode control
Parameters  : Video producer handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_ALREADY_INITIALIZED if task already running
              ST_ERROR_BAD_PARAMETER if problem of creation
*******************************************************************************/
static ST_ErrorCode_t StartProducerTask(const VIDPROD_Handle_t ProducerHandle)
{
    VIDCOM_Task_t *Task_p = &(((VIDPROD_Data_t *) ProducerHandle)->ProducerTask);
    char TaskName[22] = "STVID[?].ProducerTask";
    ST_ErrorCode_t Error;

    if (Task_p->IsRunning)
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* In TaskName string, replace '?' with decoder number */
    TaskName[6] = (char) ((U8) '0' + ((VIDPROD_Data_t *) ProducerHandle)->DecoderNumber);
    Task_p->ToBeDeleted = FALSE;
#ifdef STVID_MEASURES
    Error = STOS_TaskCreate((void (*) (void*)) ProducerTaskFunc,
                              (void *) ProducerHandle,
                              ((VIDPROD_Data_t *) ProducerHandle)->CPUPartition_p,
                              STVID_TASK_STACK_SIZE_DECODE,
                              &(Task_p->TaskStack),
                              ((VIDPROD_Data_t *) ProducerHandle)->CPUPartition_p,
                              &(Task_p->Task_p),
                              &(Task_p->TaskDesc),
                              STVID_TASK_PRIORITY_DECODE,
                              TaskName,
                              task_flags_high_priority_process | task_flags_no_min_stack_size);
#else /* STVID_MEASURES */
    Error = STOS_TaskCreate((void (*) (void*)) ProducerTaskFunc,
                              (void *) ProducerHandle,
                              ((VIDPROD_Data_t *) ProducerHandle)->CPUPartition_p,
                              STVID_TASK_STACK_SIZE_DECODE,
                              &(Task_p->TaskStack),
                              ((VIDPROD_Data_t *) ProducerHandle)->CPUPartition_p,
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
/*    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "producer task created."));*/
    return(ST_NO_ERROR);
} /* End of StartProducerTask() function */

/*******************************************************************************
Name        : NewPictureBufferAvailableCB
Description : Function to subscribe the VIDBUFF event new picture buffer available
              No more module is using this picture (all released)
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void NewPictureBufferAvailableCB(STEVT_CallReason_t Reason,
    const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event,
     const void *EventData_p,
     const void *SubscriberData_p
)
{
    VIDBUFF_PictureBuffer_t * Picture_p;
    VIDPROD_Data_t * VIDPROD_Data_p;
    VIDPROD_Handle_t ProducerHandle;

    /* shortcuts */
    ProducerHandle   = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDPROD_Handle_t, SubscriberData_p);
    VIDPROD_Data_p  = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDPROD_Data_t *, SubscriberData_p);
    Picture_p = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDBUFF_PictureBuffer_t *, EventData_p);

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    if (Picture_p != NULL)
    {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_PRESENTATION, "pPOD(%d-%d/%d) ", Picture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, Picture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, Picture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "NewPictureBufferAvailableCB called with NULL picture_p !!!"));
    }
    /* !!! To go try to decode next pic as soon as previous decode is over, while
     waiting for a command (otherwise blocked until timeout of command wait) */
    /* Signal controller that a new command is given */
    STOS_SemaphoreSignal(VIDPROD_Data_p->ProducerOrder_p);

} /* End of NewPictureBufferAvailableCB() function */

/*******************************************************************************
Name        : ParserJobCompletedCB
Description : Function to subscribe the PARSER event Job completed
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void ParserJobCompletedCB(STEVT_CallReason_t Reason,
    const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event,
    const void *EventData_p,
    const void *SubscriberData_p
)
{
    VIDPROD_Handle_t                    ProducerHandle;
    VIDPROD_Data_t *                    VIDPROD_Data_p;
    PARSER_ParsingJobResults_t *        ParsingJobResults_p;
    VIDBUFF_PictureBuffer_t *           PictureBuffer_p = NULL;
    VIDCOM_ParsedPictureInformation_t * ParsedPictureInformation_p;
#ifdef STVID_TRICKMODE_BACKWARD
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
#endif
    /* shortcuts */
    ProducerHandle   = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDPROD_Handle_t, SubscriberData_p);
    VIDPROD_Data_p  = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDPROD_Data_t *, SubscriberData_p);
    ParsingJobResults_p = CAST_CONST_HANDLE_TO_DATA_TYPE(PARSER_ParsingJobResults_t *, EventData_p);

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
#if defined TRACE_UART
    TracePictureType(("ParseType","%c(%d-%d/%d)",
        MPEGFrame2Char(ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame),
        ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.DecodingOrderFrameID,
        ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
        ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID
    ));
#endif
#ifdef STVID_TRICKMODE_BACKWARD
    if (!VIDPROD_Data_p->Backward.DoNotTreatParserJobCompletedEvt)
#endif
{
#if defined(TRACE_UART) && defined(VALID_TOOLS)
    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_PARSER_JOB_END, "pPARS_JC(%d-%d/%d) %s %c PTS(%c-%d/%d) @BB=0x%x ",
            ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.DecodingOrderFrameID, ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID,
            ((ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.IsReference)?"REF":"NONREF"),
            MPEGFrame2Char(ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame),
            ((ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.PictureInfos.VideoParams.PTS.Interpolated) ? 'I' : 'R'),
            ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.PictureInfos.VideoParams.PTS.PTS,
            ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.PictureInfos.VideoParams.PTS.PTS33,
            ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.BitBufferPictureStartAddress));
    {
        U32 ReferenceCounter;
        char RefMsg[256];
        sprintf(RefMsg, "");
        for(ReferenceCounter = 0 ; ReferenceCounter < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES ; ReferenceCounter++)
        {
            if (ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.IsValidIndexInReferenceFrame[ReferenceCounter])
            {
                sprintf(RefMsg, "%s %d", RefMsg, ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.FullReferenceFrameList[ReferenceCounter]);
            }
        }
        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_PARSER_JOB_END, "Refs (%s)\r\n", RefMsg));
    }
#endif
#ifdef STVID_TRICKMODE_BACKWARD
    STOS_SemaphoreWait(VIDPROD_Data_p->LinearBitBuffers.PreprocessingPictureSemaphore_p);
#endif /* STVID_TRICKMODE_BACKWARD */
    {
#ifdef STVID_TRICKMODE_BACKWARD
    if (((VIDPROD_Data_p->Backward.IsBackward) && (VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p != NULL)) || (!VIDPROD_Data_p->Backward.IsBackward))
	{
    if (((VIDPROD_Data_p->Backward.IsBackward) && (!VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.BitBufferFullyParsed))
            || (!VIDPROD_Data_p->Backward.IsBackward))
#endif /* STVID_TRICKMODE_BACKWARD */
    {
    if ((VIDPROD_Data_p->ProducerState == VIDPROD_DECODER_STATE_STOPPED) || (VIDPROD_Data_p->Stop.IsPending))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "New picture parsed but producer is STOPPED (ignored, no action) ProdState=%d, StopPending=%d.",
                VIDPROD_Data_p->ProducerState, VIDPROD_Data_p->Stop.IsPending));
#ifdef STVID_TRICKMODE_BACKWARD
    STOS_SemaphoreSignal(VIDPROD_Data_p->LinearBitBuffers.PreprocessingPictureSemaphore_p);
#endif
        return;
    }
    ParsedPictureInformation_p = ParsingJobResults_p->ParsedPictureInformation_p;

    if (ParsedPictureInformation_p != NULL)
    {
#ifdef STVID_DEBUG_GET_STATISTICS
        if (ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B)
        {
            VIDPROD_Data_p->Statistics.DecodePictureFoundMPEGFrameB++;
        }
        else
        {
            if (ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_P)
            {
                VIDPROD_Data_p->Statistics.DecodePictureFoundMPEGFrameP++;
            }
            else
            {
                VIDPROD_Data_p->Statistics.DecodePictureFoundMPEGFrameI++;
            }
        }
        VIDPROD_Data_p->Statistics.DecodePictureFound++;
#endif /* STVID_DEBUG_GET_STATISTICS */

        if (ParsingJobResults_p->ParsingJobStatus.HasErrors)
        {
/* TODO_PC (ParserJobCompletedCB) : Parsing error present, add Error processing */
#if defined(TRACE_UART) && defined(VALID_TOOLS)
            TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_PARSING_ERRORS, "pPARS_JCErr1(%d-%d/%d) ", ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.DecodingOrderFrameID, ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
        }
#ifdef STVID_TRICKMODE_BACKWARD
        if (VIDPROD_Data_p->Backward.IsBackward)
        {
            if (ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)
            {
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->NbIPicturesInBuffer ++;
                ErrorCode = GetFreePreprocessingPictureBufferBackward(ProducerHandle, &PictureBuffer_p, ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.DecodingOrderFrameID);
                if ( ErrorCode == ST_ERROR_BAD_PARAMETER)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Producer ParserJobCompletedCB : parser notify twice the same picture (%d-%d/%d) ", ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.DecodingOrderFrameID, ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID));
                    STOS_SemaphoreSignal(VIDPROD_Data_p->LinearBitBuffers.PreprocessingPictureSemaphore_p);
                    return;
                }
            }
        }
        if (!VIDPROD_Data_p->Backward.IsBackward)
#endif  /* STVID_TRICKMODE_BACKWARD */
        {
            GetFreePreprocessingPictureBuffer(ProducerHandle, &PictureBuffer_p);
        }

            if (PictureBuffer_p == NULL)
            {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_PARSING_ERRORS, "pPARS_JCErr2(%d-%d/%d) ", ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.DecodingOrderFrameID, ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Producer ParserJobCompletedCB : Unable to find free PictureBuffer for new parsed picture (%d-%d/%d) (BUG-BUG-BUG)", ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.DecodingOrderFrameID, ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#ifdef STVID_TRICKMODE_BACKWARD
    STOS_SemaphoreSignal(VIDPROD_Data_p->LinearBitBuffers.PreprocessingPictureSemaphore_p);
#endif
                return;
            }
			else
			{
#ifdef ST_speed
                if (!VIDPROD_Data_p->IsBitRateComputable)
                {
                    VIDPROD_Data_p->IsBitRateComputable = TRUE;
                }
                if (VIDPROD_Data_p->IsBitRateComputable)
                {
                    EvaluateBitRateValue(ProducerHandle, 2,
                    ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.PictureInfos.VideoParams.FrameRate, ParsingJobResults_p->DiffPSC);
                }
#endif /* ST_speed */
                /* Save the last Parsed Picture informatin */
                VIDPROD_Data_p->LastParsedPictureDecodingOrderFrameID = ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.DecodingOrderFrameID;
                VIDPROD_Data_p->LastParsedPictureExtendedPresentationOrderPictureID = ParsingJobResults_p->ParsedPictureInformation_p->PictureGenericData.ExtendedPresentationOrderPictureID;

                vidprod_UpdatePictureParsedPictureInformation(PictureBuffer_p, ParsingJobResults_p->ParsedPictureInformation_p);
                /* Compute NbFieldDisplay for the parsed picture, result is set in PictureGenericData.NbFieldsDisplay */
                ComputeNbFieldsDisplayNextDecode(PictureBuffer_p->GlobalDecodingContext.GlobalDecodingContextGenericData.SequenceInfo.MPEGStandard, &PictureBuffer_p->ParsedPictureInformation.PictureGenericData);
#ifdef STVID_TRICKMODE_BACKWARD
                if (VIDPROD_Data_p->Backward.IsBackward)
                {
                    if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p != NULL)
                    {
                        VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->TotalNbIFields += PictureBuffer_p->ParsedPictureInformation.PictureGenericData.NbFieldsDisplay;
                    }
                }
#endif /* STVID_TRICKMODE_BACKWARD */
                PictureBuffer_p->IsPushed = FALSE;
                VIDPROD_Data_p->ForTask.NewParsedPicture_p = PictureBuffer_p;
                /* Lock Access to Decode Pictures */
                STOS_SemaphoreWait(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);
                VIDPROD_Data_p->DecoderStatus.CurrentPictures_p[(VIDPROD_Data_p->DecoderStatus.NextOutputIndex + VIDPROD_Data_p->DecoderStatus.NbIndices) % MAX_CURRENT_PICTURES] = PictureBuffer_p;
                VIDPROD_Data_p->DecoderStatus.NbIndices++;
                /* UnLock Access to Decode Pictures */
                STOS_SemaphoreSignal(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);
                PushControllerCommand(ProducerHandle, CONTROLLER_COMMAND_NEW_PARSED_PICTURE);
                STOS_SemaphoreSignal(VIDPROD_Data_p->ProducerOrder_p);
            }
#ifdef ST_XVP_ENABLE_FGT
            STOS_memcpy(    &(PictureBuffer_p->ParsedPictureInformation.PictureFilmGrainSpecificData),
                            &(ParsedPictureInformation_p->PictureFilmGrainSpecificData),
                            sizeof(VIDCOM_FilmGrainSpecificData_t));
#endif /* ST_XVP_ENABLE_FGT */
    }
    else
    {
/* TODO_PC (ParserJobCompletedCB) : Parsing error present, no picture, add Error processing */
#if defined(TRACE_UART) && defined(VALID_TOOLS)
        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_PARSING_ERRORS, "pPARS_JCErr3 "));
#endif
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Producer ParserJobCompletedCB : Parser returned no picture !!"));
    }
    }/*not fully parsed, for backward*/
#ifdef STVID_TRICKMODE_BACKWARD
	}
 #endif /* STVID_TRICKMODE_BACKWARD */
    }
#ifdef STVID_TRICKMODE_BACKWARD
    STOS_SemaphoreSignal(VIDPROD_Data_p->LinearBitBuffers.PreprocessingPictureSemaphore_p);
#endif

}
} /* End of ParserJobCompletedCB() function */

/*******************************************************************************
Name        : ParserPictureSkippedCB
Description : Function to subscribe the PARSER event Picture Skipped
Parameters  : EVT API
Assumptions : WARNING, the skipped picture shall not be entirely parsed, only its
              Stop Address in nit buffer is reliable and then passed through the PARSER_PICTURE_SKIPPED_EVT event
Limitations :
Returns     : EVT API
*******************************************************************************/
static void ParserPictureSkippedCB(STEVT_CallReason_t Reason,
    const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event,
    const void *EventData_p,
    const void *SubscriberData_p
)
{
    VIDPROD_Handle_t                    ProducerHandle;
    VIDPROD_Data_t *                    VIDPROD_Data_p;

    VIDCOM_PictureGenericData_t *       PictureGenericData_p;
    U32                                 LastByteAddress;

    /* shortcuts */
    ProducerHandle   = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDPROD_Handle_t, SubscriberData_p);
    VIDPROD_Data_p  = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDPROD_Data_t *, SubscriberData_p);
    PictureGenericData_p = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDCOM_PictureGenericData_t *, EventData_p);

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    /* Compute the address of the last byte of the picture */
    {
        LastByteAddress = (U32)PictureGenericData_p->BitBufferPictureStopAddress;
        /* Go back with 4 bytes in the bitbuffer = 00 00 01 xx */
        LastByteAddress -= 4;
#ifdef STVID_TRICKMODE_BACKWARD
        if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
        {
            /* Manage bitbuffer loop, if the bitbuffer is circular */
            if(LastByteAddress < (U32)VIDPROD_Data_p->BitBuffer.Address_p)
            {
                LastByteAddress += VIDPROD_Data_p->BitBuffer.TotalSize;
            }
        }
    }
    VIDPROD_Data_p->ParserStatus.LastPictureSkippedStopAddress_p = (void*)LastByteAddress;

    /* A new skip happened, push a new command, only if the previous skip was already
     * handled within the producer task. */
    if(!VIDPROD_Data_p->ParserStatus.IsPictureSkipJustHappened)
    {
        /* Remember that a picture skip happened */
        VIDPROD_Data_p->ParserStatus.IsPictureSkipJustHappened = TRUE;

        PushControllerCommand(ProducerHandle, CONTROLLER_COMMAND_PARSER_PICTURE_SKIPPED);
        STOS_SemaphoreSignal(VIDPROD_Data_p->ProducerOrder_p);
    }

    /* Compute the number of fields skipped */
    ComputeNbFieldsDisplayNextDecode(VIDPROD_Data_p->PreviousSequenceInfo.MPEGStandard, PictureGenericData_p);
#ifdef STVID_TRICKMODE_BACKWARD
    if (VIDPROD_Data_p->Backward.IsBackward)
    {
        if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p != NULL)
        {
            VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->NbNotIPicturesInBuffer ++;
            VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->TotalNbNotIFields += PictureGenericData_p->NbFieldsDisplay;
        }
    }
#endif  /* STVID_TRICKMODE_BACKWARD */
#ifdef ST_speed
    if (VIDPROD_Data_p->IsBitRateComputable)
    {
        EvaluateBitRateValue(ProducerHandle, PictureGenericData_p->NbFieldsDisplay,
            PictureGenericData_p->PictureInfos.VideoParams.FrameRate,PictureGenericData_p->DiffPSC  );
    }
#endif /* ST_speed*/
    /* Notify producer skip event */
    STEVT_Notify(VIDPROD_Data_p->EventsHandle, VIDPROD_Data_p->RegisteredEventsID[VIDPROD_PICTURE_SKIPPED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST (&PictureGenericData_p->NbFieldsDisplay));
} /* End of ParserPictureSkippedCB() function */

/*******************************************************************************
Name        : ParserFindDiscontinuityCB
Description : Function to subscribe the PARSER event Find discontinuity
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void ParserFindDiscontinuityCB(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p)
{
    VIDPROD_Handle_t                    ProducerHandle;
    VIDPROD_Data_t *                    VIDPROD_Data_p;

    /* shortcuts */
    ProducerHandle   = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDPROD_Handle_t, SubscriberData_p);
    VIDPROD_Data_p  = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDPROD_Data_t *, SubscriberData_p);

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData_p);

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Parser found discountinuity"));
    if (VIDPROD_Data_p->Stop.WhenNoData)
    {
        VIDPROD_Data_p->Stop.AfterCurrentPicture = TRUE;
    }
#ifdef ST_speed
    if (VIDPROD_Data_p->Skip.AllExceptFirstI)
    {
        VIDPROD_Data_p->SearchFirstI = TRUE;
    }
#endif
} /* End of ParserFindDiscontinuityCB() function */
/*******************************************************************************
Name        : ParserBitBufferFullyParsedCB
Description : Function to subscribe the PARSER event Linear bit buffer fully parsed
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
#ifdef STVID_TRICKMODE_BACKWARD
static void ParserBitBufferFullyParsedCB(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p)
{
    VIDPROD_Handle_t                    ProducerHandle;
    VIDPROD_Data_t *                    VIDPROD_Data_p;
    VIDPROD_BitBUfferInfos_t            BitBufferInfos;

    /* shortcuts */
    ProducerHandle   = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDPROD_Handle_t, SubscriberData_p);
    VIDPROD_Data_p  = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDPROD_Data_t *, SubscriberData_p);

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData_p);

    if ((VIDPROD_Data_p->Backward.IsBackward) && (VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p != NULL))
    {
        VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.BitBufferFullyParsed = TRUE;
        BitBufferInfos.NbIPicturesInBuffer = VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->NbIPicturesInBuffer;
        BitBufferInfos.NbNotIPicturesInBuffer = VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->NbNotIPicturesInBuffer;
        BitBufferInfos.TotalNbIFields = VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->TotalNbIFields;
        BitBufferInfos.TotalNbNotIFields = VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->TotalNbNotIFields;
        BitBufferInfos.TimeToDecodeIPicture = VIDPROD_Data_p->Backward.TimeToDecodeIPicture;
        if ( VIDPROD_Data_p->Backward.BufferDecodeTime > VIDPROD_Data_p->Backward.InjectionTime)
        {
            BitBufferInfos.BufferTreatmentTime = VIDPROD_Data_p->Backward.BufferDecodeTime;
        }
        else
        {
            BitBufferInfos.BufferTreatmentTime = VIDPROD_Data_p->Backward.InjectionTime;
        }
        BitBufferInfos.BufferDecodeTime = VIDPROD_Data_p->Backward.BufferDecodeTime;
        if (BitBufferInfos.TotalNbIFields != 0)
        {
            STEVT_Notify(VIDPROD_Data_p->EventsHandle, VIDPROD_Data_p->RegisteredEventsID[VIDPROD_BUFFER_FULLY_PARSED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(BitBufferInfos));
        }
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "BB fully parsed \r\n"));
    }

}/* End of ParserBitBufferFullyParsedCB() function */
#endif

/*******************************************************************************
Name        : ParserUserDataCB
Description : Function to subscribe the PARSER event user data
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void ParserUserDataCB(STEVT_CallReason_t Reason,
    const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event,
    const void *EventData_p,
    const void *SubscriberData_p
)
{
    STVID_UserData_t *          UserData_p;
    VIDPROD_Data_t *            VIDPROD_Data_p;

    /* shortcuts */
    VIDPROD_Data_p  = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDPROD_Data_t *, SubscriberData_p);
    UserData_p = CAST_CONST_HANDLE_TO_DATA_TYPE(STVID_UserData_t *, EventData_p);


    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    if (!VIDPROD_Data_p->UserDataEventEnabled)
    {
        return;
    }
#if defined(TRACE_UART) && defined(VALID_TOOLS)
    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_PARSER_JOB_END, "pPARS_UD_length=%d ", UserData_p->Length));
#endif
    if ((VIDPROD_Data_p->ProducerState == VIDPROD_DECODER_STATE_STOPPED) || (VIDPROD_Data_p->Stop.IsPending))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "New user data found but producer is STOPPED (ignored, no action)."));
        return;
    }
    /* Immediately notify the user data event for STVID API */
    /* Special function call for USER_DATA_EVT event since different management between OS2x and Linux */
    vidcom_NotifyUserData(VIDPROD_Data_p->EventsHandle,
                          VIDPROD_Data_p->RegisteredEventsID[VIDPROD_USER_DATA_EVT_ID],
                          UserData_p);
} /* End of ParserUserDataCB() function */


/*******************************************************************************
Name        : DecoderJobCompletedCB
Description : Function to subscribe the DECODER event Job completed
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void DecoderJobCompletedCB(STEVT_CallReason_t Reason,
    const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event,
    const void *EventData_p,
    const void *SubscriberData_p
)
{
    VIDPROD_Handle_t                  ProducerHandle;
    DECODER_DecodingJobResults_t *    DecodingJobResults_p;
    VIDPROD_Data_t *                  VIDPROD_Data_p;

    /* shortcuts */
    ProducerHandle   = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDPROD_Handle_t, SubscriberData_p);
    VIDPROD_Data_p  = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDPROD_Data_t *, SubscriberData_p);
    DecodingJobResults_p = CAST_CONST_HANDLE_TO_DATA_TYPE(DECODER_DecodingJobResults_t *, EventData_p);

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

#if defined(TRACE_UART) && defined(VALID_TOOLS)
    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODER_JOB_END, "pDEC_JC%d ", DecodingJobResults_p->CommandId));
#endif
    if (VIDPROD_Data_p->ProducerState == VIDPROD_DECODER_STATE_STOPPED)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "New picture decoded (Job %d) but producer is STOPPED (Ignored, no action).", DecodingJobResults_p->CommandId));
        return;
    }

    if (VIDPROD_Data_p->ForTask.NbCompletedIndex >= MAX_NUMBER_OF_COMPLETED_PICTURES)
    {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_ERRORS, "pDEC_JCErr1-%d ", DecodingJobResults_p->CommandId));
#endif
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "!!! New decoder job completed event received but queue is full. New ID=%d !!!", DecodingJobResults_p->CommandId));
    }

    VIDPROD_Data_p->ForTask.CompletedPictureResults[(VIDPROD_Data_p->ForTask.NextCompletedIndex + VIDPROD_Data_p->ForTask.NbCompletedIndex) % MAX_NUMBER_OF_COMPLETED_PICTURES] = *DecodingJobResults_p;
    VIDPROD_Data_p->ForTask.NbCompletedIndex++;

    PushControllerCommand(ProducerHandle, CONTROLLER_COMMAND_NEW_DECODED_PICTURE);
    STOS_SemaphoreSignal(VIDPROD_Data_p->ProducerOrder_p);
} /* End of DecoderJobCompletedCB() function */

/*******************************************************************************
Name        : DecoderConfirmBufferCB
Description : Function to subscribe the DECODER event DECODER_CONFIRM_BUFFER_EVT
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void DecoderConfirmBufferCB(STEVT_CallReason_t Reason,
    const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event,
    const void *EventData_p,
    const void *SubscriberData_p
)
{
    VIDPROD_Handle_t                ProducerHandle;
    DECODER_DecodingJobResults_t *  DecodingJobResults_p;
    VIDPROD_Data_t *                VIDPROD_Data_p;

    /* shortcuts */
    ProducerHandle   = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDPROD_Handle_t, SubscriberData_p);
    VIDPROD_Data_p  = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDPROD_Data_t *, SubscriberData_p);
    DecodingJobResults_p = CAST_CONST_HANDLE_TO_DATA_TYPE(DECODER_DecodingJobResults_t *, EventData_p);

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

#if defined(TRACE_UART) && defined(VALID_TOOLS)
    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODER_CONFIRM, "pDEC_CB%d ", DecodingJobResults_p->CommandId));
#endif
    if ((VIDPROD_Data_p->ProducerState == VIDPROD_DECODER_STATE_STOPPED) || (VIDPROD_Data_p->Stop.IsPending))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Confirm buffer requested jobID %d but producer is STOPPED (ignored, no action).", DecodingJobResults_p->CommandId));
        return;
    }
    else if (VIDPROD_Data_p->ForTask.SoftResetPending)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Confirm buffer requested jobID %d but soft reset is pending (ignored, no action).", DecodingJobResults_p->CommandId));
        return;
    }

    if (VIDPROD_Data_p->ForTask.NbIndexToConfirm >= MAX_NUMBER_OF_PREPROCESSING_PICTURES)
    {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_ERRORS, "pDEC_CBErr2-%d ", DecodingJobResults_p->CommandId));
#endif
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "!!! New confirmbuffer event received but queue is full (NbIndexToConfirm=%d). New ID=%d !!!", VIDPROD_Data_p->ForTask.NbIndexToConfirm, DecodingJobResults_p->CommandId));
    }

        VIDPROD_Data_p->ForTask.PictureToConfirmResult[(VIDPROD_Data_p->ForTask.NextIndexToConfirm + VIDPROD_Data_p->ForTask.NbIndexToConfirm) % MAX_NUMBER_OF_PREPROCESSING_PICTURES] = *DecodingJobResults_p;
        VIDPROD_Data_p->ForTask.NbIndexToConfirm++;

    PushControllerCommand(ProducerHandle, CONTROLLER_COMMAND_CONFIRM_PICTURE_BUFFER);
    STOS_SemaphoreSignal(VIDPROD_Data_p->ProducerOrder_p);

} /* End of DecoderConfirmBufferCB() function */

/*******************************************************************************
Name        : DecoderInformReadAddressCB
Description : Callback function to subscribe the DECODER event inform read address
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void DecoderInformReadAddressCB(STEVT_CallReason_t Reason,
    const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event,
    void *EventData_p,
    void *SubscriberData_p
)
{
    VIDPROD_Handle_t                ProducerHandle;
    VIDPROD_Data_t *                VIDPROD_Data_p;
    DECODER_InformReadAddress_t *   InformReadAddress_p = (DECODER_InformReadAddress_t *) EventData_p;

    ProducerHandle   = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDPROD_Handle_t, SubscriberData_p);
    VIDPROD_Data_p  = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDPROD_Data_t *, SubscriberData_p);

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

#if defined(TRACE_UART) && defined(VALID_TOOLS)
    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_INFORM_READ_ADDR, "pDEC_IRA "));
#endif

    /* Here we assume we receive DECODER_INFORM_READ_ADDRESS_EVT events sorted accorded to the frame decoding order */
    VIDPROD_Data_p->DecoderStatus.LastReadAddress_p = InformReadAddress_p->DecoderReadAddress_p;

    PushControllerCommand(ProducerHandle, CONTROLLER_COMMAND_DECODER_READ_ADDRESS_UPDATED);
    STOS_SemaphoreSignal(VIDPROD_Data_p->ProducerOrder_p);

} /* End of DecoderInformReadAddressCB() function */


/*******************************************************************************
Name        : StopProducerTask
Description : Stop the task of producer control
Parameters  : Video producer handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if no problem,
              ST_ERROR_ALREADY_INITIALIZED if task not running
*******************************************************************************/
static ST_ErrorCode_t StopProducerTask(const VIDPROD_Handle_t ProducerHandle)
{
    VIDCOM_Task_t * const Task_p = &(((VIDPROD_Data_t *) ProducerHandle)->ProducerTask);
    task_t *TaskList_p = Task_p->Task_p;

    if (!(Task_p->IsRunning))
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* End the function of the task here... */
    Task_p->ToBeDeleted = TRUE;

    STOS_SemaphoreSignal(((VIDPROD_Data_t *) ProducerHandle)->ProducerOrder_p);
    STOS_TaskWait(&TaskList_p, TIMEOUT_INFINITY);
    STOS_TaskDelete ( Task_p->Task_p,
                      ((VIDPROD_Data_t *) ProducerHandle)->CPUPartition_p,
                      &Task_p->TaskStack,
                      ((VIDPROD_Data_t *) ProducerHandle)->CPUPartition_p);

    Task_p->IsRunning  = FALSE;

    return(ST_NO_ERROR);
} /* End of StopProducerTask() function */

/*******************************************************************************
Name        : TermProducerStructure
Description : Terminate producer structure
Parameters  : Video producer handle
Assumptions :
Limitations :
Returns     : Nothing
 *******************************************************************************/
static void TermProducerStructure(const VIDPROD_Handle_t ProducerHandle)
{
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

    STOS_SemaphoreDelete(VIDPROD_Data_p->CPUPartition_p, VIDPROD_Data_p->ProducerOrder_p);
    STOS_SemaphoreDelete(VIDPROD_Data_p->CPUPartition_p, VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);
    STOS_SemaphoreDelete(VIDPROD_Data_p->CPUPartition_p, VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureSemaphore_p);
#ifdef STVID_TRICKMODE_BACKWARD
    STOS_SemaphoreDelete(VIDPROD_Data_p->CPUPartition_p, VIDPROD_Data_p->LinearBitBuffers.PreprocessingPictureSemaphore_p);
#endif

    STOS_SemaphoreDelete(VIDPROD_Data_p->CPUPartition_p, VIDPROD_Data_p->Stop.ActuallyStoppedSemaphore_p);
} /* end of TermProducerStructure() */
/*******************************************************************************
Name        : ProducerTaskFunc
Description : Function fo the producer task
              Set VIDPROD_Data_p->ProducerTask.ToBeDeleted to end the task
Parameters  : Video producer handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void ProducerTaskFunc(const VIDPROD_Handle_t ProducerHandle)
{
    osclock_t MonitorBBLTime;
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

    STOS_TaskEnter(NULL);

    VIDPROD_Data_p->ForTask.NoDisplayAfterNextDecode = FALSE;
    VIDPROD_Data_p->ForTask.SoftResetPending = FALSE;
    VIDPROD_Data_p->ForTask.SoftResetPosted = FALSE;
    VIDPROD_Data_p->ForTask.NewParsedPicture_p = NULL;
    VIDPROD_Data_p->ForTask.NbIndexToConfirm = 0;
    VIDPROD_Data_p->ForTask.NextIndexToConfirm = 0;
    VIDPROD_Data_p->ForTask.PictureToDecode_p = NULL;
    VIDPROD_Data_p->ForTask.PictureToConfirm_p = NULL;
    VIDPROD_Data_p->ForTask.NextCompletedIndex = 0;
    VIDPROD_Data_p->ForTask.NbCompletedIndex = 0;
    VIDPROD_Data_p->ForTask.NbPicturesOutOfDisplay = 0;
    VIDPROD_Data_p->ForTask.LastDecodedPicture_p = NULL;
    VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = FALSE;

    /* Big loop executed until ToBeDeleted is set --------------------------- */
    MonitorBBLTime=time_now();
    do
    { /* while not ToBeDeleted */

        if ((VIDPROD_Data_p->ProducerState == VIDPROD_DECODER_STATE_STOPPED) && (!(VIDPROD_Data_p->ProducerTask.ToBeDeleted)))
        {
            /* Decoder stopped: there is no monitoring to do, so semaphore waits infinitely.
            This avoid task switching not really needed when video stopped. */
            STOS_SemaphoreWait(VIDPROD_Data_p->ProducerOrder_p);
        }
        else
        {
            if (!VIDPROD_Data_p->StartCondition.IsDecodeEnable)
            {
                /* while decode has not been enabled yet allow to enter very often in ProducerTaskFuncForAutomaticControlMode() to check if it's time to start... */
                VIDPROD_Data_p->ForTask.MaxWaitOrderTime = time_plus(time_now(), (STVID_MAX_VSYNC_DURATION /5));
            }
            else
            {
                /* Decoder running: semaphore has normal time-out to do polling of actions even if semaphore was not signaled. */
                VIDPROD_Data_p->ForTask.MaxWaitOrderTime = time_plus(time_now(), MAX_WAIT_ORDER_TIME);
            }
            /* Check if an order is available */
            STOS_SemaphoreWaitTimeOut(VIDPROD_Data_p->ProducerOrder_p, &(VIDPROD_Data_p->ForTask.MaxWaitOrderTime));
            while (STOS_SemaphoreWaitTimeOut(VIDPROD_Data_p->ProducerOrder_p, TIMEOUT_IMMEDIATE) == 0)
            {            /* Nothing to do: just to decrement counter until 0, to avoid multiple looping on semaphore wait */
            }
        }

        if (!VIDPROD_Data_p->ProducerTask.ToBeDeleted)
        {
            ProducerTaskFuncForAutomaticControlMode(ProducerHandle);

            /* Monitor bit buffer level */
            if (time_minus(time_now(),MonitorBBLTime) > STVID_MAX_VSYNC_DURATION)
            {
                MonitorBBLTime=time_now();
                #ifdef STVID_TRICKMODE_BACKWARD
                if ((VIDPROD_Data_p->Backward.ChangingDirection == TRICKMODE_NOCHANGE) && (!VIDPROD_Data_p->Backward.IsBackward))
                #endif /* STVID_TRICKMODE_BACKWARD */
                {
                    MonitorBitBufferLevel(ProducerHandle);
                }
            }
        }
    } while (!(VIDPROD_Data_p->ProducerTask.ToBeDeleted));

    STOS_TaskExit(NULL);

} /* End of ProducerTaskFunc() function */

/*******************************************************************************
Name        : ProducerTaskFuncForAutomaticControlMode
Description : Function fo the producer task
Parameters  : Video producer handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void ProducerTaskFuncForAutomaticControlMode(const VIDPROD_Handle_t ProducerHandle)
{
    /* Overall variables of the task */
    VIDPROD_Data_t *                        VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    PARSER_GetPictureParams_t               PARSER_GetPictureParams;
    VIDBUFF_PictureBuffer_t *               PictureBuffer_p;
    PARSER_InformReadAddressParams_t        InformReadAddressParams;
#ifdef ST_avsync
    STCLKRV_ExtendedSTC_t                   ExtendedSTC;
#endif /* ST_avsync */
    ST_ErrorCode_t                          ErrorCode;
    U8                                      Tmp8;
    U32                                     ExpectedPictureOutputIndex; /* for debugging purpose */
    VIDBUFF_PictureBuffer_t *               ExpectedPictureInDecodeOrder_p;
    VIDBUFF_PictureBuffer_t *               LastConfirmedPicture_p;
    VIDBUFF_PictureBuffer_t *               RefusedInsertPicture_p = NULL;
    U32                                     LoopCounter, RememberIndex;
    DECODER_DecodingJobResults_t            DecodingJobResult;
    STOS_Clock_t                        TOut_ClockTicks;
    STVID_PictureInfos_t                    ExportedPictureInfos;
    BOOL                                    ThereIsPendingDecode;
    BOOL                                    FoundDecodeToAbort = TRUE;
    U32                                     HighestDecodeOrderIDFound = 0;
    STVID_PictureParams_t                   CurrentPictureParams;
    U32                                     CodecPlusOneVSyncDecodeLatencyInBytes;
#ifndef STVID_PRODUCER_NO_TIMEOUT
    /* Check if max preprocessing time is over for all preproc command sent and not finished */
    STOS_Clock_t TOut_MaxAllowedTime;
    STOS_Clock_t TOut_MaxAllowedDuration;
    STOS_Clock_t TOut_Now;
    STOS_Clock_t TOut_MaxVSyncDuration;
#endif /* STVID_PRODUCER_NO_TIMEOUT */
#ifdef STVID_DEBUG_GET_STATISTICS
    U32 PreprocessTime;
    U32 BufferSearchTime;
    U32 DecodeTime;
    U32 FullDecodeTime;
#ifdef STVID_VALID_MEASURE_TIMING
    U32 LXDecodeTimeInMicros;
#endif
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_TRICKMODE_BACKWARD
	PARSER_StartParams_t        PARSER_StartParams;
#endif /* STVID_TRICKMODE_BACKWARD */

#if defined(VALID_TOOLS) && (defined(STVID_FRAME_BUFFER_DUMP) || defined(STVID_FB_ADD_DATA_DUMP) || defined(STVID_DECIMATED_FRAME_BUFFER_DUMP))
    char    Filename_p[FILENAME_MAX_LENGTH];
#ifdef STVID_DECIMATED_FRAME_BUFFER_DUMP
    U32 H_DecimationFactor;
    U32 V_DecimationFactor;
#endif
#endif /* defined(VALID_TOOLS) && (defined(STVID_FRAME_BUFFER_DUMP) || defined(STVID_FB_ADD_DATA_DUMP)) */
    /* LaunchDecodeWhenPossible: Flag to trigger the decode/display:
      Set to TRUE to launch the decode/display of the current picture.
      Warning: setting this flag to FALSE requires launch of the start code detector:
       - this is done automatically when launching decode
       - this must be done manually in other cases ! */
	ErrorCode = ST_NO_ERROR;
    TOut_ClockTicks = (STOS_Clock_t) ST_GetClocksPerSecond();

    /* Process all user commands together only if no start code commands... */
    while (PopControllerCommand(ProducerHandle, &Tmp8) == ST_NO_ERROR)
    {
        switch ((ControllerCommandID_t) Tmp8)
        {
            case CONTROLLER_COMMAND_RESET :
#if defined(TRACE_UART) && !defined(VALID_TOOLS)
                TraceBuffer(("Prod reset \r\n"));
#endif
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_RESET, "pPARS_Stop "));
#endif
                ErrorCode = PARSER_Stop(VIDPROD_Data_p->ParserHandle);
                if (ErrorCode != ST_NO_ERROR)
                {
                    /* TODO_CL: error handling to be defined */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "PARSER_Stop failed Error=%d!!", ErrorCode));
                }
                STOS_InterruptLock();
                VIDPROD_Data_p->ForTask.NbIndexToConfirm = 0;
                VIDPROD_Data_p->ForTask.NextIndexToConfirm = 0;
                VIDPROD_Data_p->ForTask.NextCompletedIndex = 0;
                VIDPROD_Data_p->ForTask.NbCompletedIndex = 0;
                STOS_InterruptUnlock();
                VIDPROD_Data_p->ForTask.PictureToConfirm_p = NULL;
                VIDPROD_Data_p->NextAction.LaunchDisplayWhenPossible = FALSE;
                VIDPROD_Data_p->NextAction.NumberOfDisplayToLaunch = 0;
                VIDPROD_Data_p->NextAction.ConfirmBufferWhenPossible = 0;
                VIDPROD_Data_p->LastBitBufferLevel = 0;
                /* Reset decoder to find first sequence */
                SeekBeginningOfStream(ProducerHandle);
#ifdef STVID_PRESERVE_LAST_PICTURES_IN_BB
                ReadAdressFifoNextOutputIndex = 0;
                ReadAddressFifoNextInputIndex = 0;
                ReadAddressFifoNbUsedEntries  = 0;
#endif /* STVID_PRESERVE_LAST_PICTURES_IN_BB */
#ifdef STVID_DEBUG_GET_STATISTICS
                VIDPROD_Data_p->Statistics.DecodeTimeNbPictures             = 0;
                VIDPROD_Data_p->Statistics.DecodeTimeMaxPreprocessTime      = 0;
                VIDPROD_Data_p->Statistics.DecodeTimeMaxDecodeTime          = 0;
                VIDPROD_Data_p->Statistics.DecodeTimeMaxFullDecodeTime      = 0;
                VIDPROD_Data_p->Statistics.DecodeTimeMaxBufferSearchTime    = 0;
                VIDPROD_Data_p->Statistics.DecodeTimeSumOfPreprocessTime    = 0;
                VIDPROD_Data_p->Statistics.DecodeTimeSumOfDecodeTime        = 0;
                VIDPROD_Data_p->Statistics.DecodeTimeSumOfFullDecodeTime    = 0;
                VIDPROD_Data_p->Statistics.DecodeTimeSumOfBufferSearchTime  = 0;
                VIDPROD_Data_p->Statistics.DecodeTimeMinPreprocessTime      = ULONG_MAX;
                VIDPROD_Data_p->Statistics.DecodeTimeMinDecodeTime          = ULONG_MAX;
                VIDPROD_Data_p->Statistics.DecodeTimeMinFullDecodeTime      = ULONG_MAX;
                VIDPROD_Data_p->Statistics.DecodeTimeMinBufferSearchTime    = ULONG_MAX;
#ifdef STVID_VALID_MEASURE_TIMING
                VIDPROD_Data_p->Statistics.DecodeTimeMinLXDecodeTime        = ULONG_MAX;
                VIDPROD_Data_p->Statistics.DecodeTimeMaxLXDecodeTime        = 0;
                VIDPROD_Data_p->Statistics.DecodeTimeSumOfLXDecodeTime      = 0;
#endif /* STVID_VALID_MEASURE_TIMING */

#endif /* STVID_DEBUG_GET_STATISTICS */
#if defined(TRACE_UART) && !defined(VALID_TOOLS)
                TraceBuffer(("***PROD Reset*** \r\n"));
#endif /* TRACE_UART */
                VIDPROD_Data_p->ProducerState = VIDPROD_DECODER_STATE_DECODING;
                VIDPROD_Data_p->ForTask.SoftResetPending = FALSE;
                VIDPROD_Data_p->ForTask.SoftResetPosted = FALSE;
                VIDPROD_Data_p->NeedToDecreaseBBL = FALSE;
                VIDPROD_Data_p->OverFlowEvtAlreadySent = FALSE;
                InitUnderflowEvent(ProducerHandle);
                VIDPROD_Data_p->Stop.IsPending = FALSE;
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "PROD RESET"));
#ifdef STVID_TRICKMODE_BACKWARD
                if ((VIDPROD_Data_p->Backward.ChangingDirection == TRICKMODE_CHANGING_BACKWARD))
                {
					VIDPROD_ChangeDirection(ProducerHandle, TRICKMODE_CHANGING_BACKWARD);
				}
                else if ((VIDPROD_Data_p->Backward.IsBackward) && (VIDPROD_Data_p->Backward.ChangingDirection == TRICKMODE_NOCHANGE))
                {

                    vidprod_AskForLinearBitBufferFlush(ProducerHandle,
                        VIDPROD_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p,
                        VIDPROD_FLUSH_BBFSIZE_BACKWARD_JUMP);
                }
				else
#endif
				{
                	PushControllerCommand(ProducerHandle, CONTROLLER_COMMAND_SEARCH_PICTURE);
				}

                /* No need because already in comand loop STOS_SemaphoreSignal(VIDPROD_Data_p->ProducerOrder_p); */
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_RESET, "pRST "));
#endif
                break;

            case CONTROLLER_COMMAND_STOP :
                /* TODO_CL: manage all stop modes out of the state machine (see old decode module) */
                /* NOTE_CL: For now only make a stop now at this stage */
                /* As we currently only support stop now, parsed ans decode results must be ignored in callbacks when the decoder is stopped */
                /* The current stop behavior is :
                 * - stop the parser
                 * - ignore all new parsed picture
                 * - treat normally confirm buffer requests and decode end of job
                 * - when we have no more preprocessing picture pending, we consider the process is actually stopped then
                 *   we bump the whole reordering queue to the display and mark the producer state as STOPPED */
                VIDPROD_Data_p->Stop.IsPending = TRUE;
                VIDPROD_Data_p->Stop.AfterCurrentPicture = FALSE;
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_RESET, "pPARS_Stop "));
#endif
                ErrorCode = PARSER_Stop(VIDPROD_Data_p->ParserHandle);
                if (ErrorCode != ST_NO_ERROR)
                {
                    /* TODO_CL: error handling to be defined */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "PARSER_Stop failed Error=%d!!", ErrorCode));
                }
                break;

            case CONTROLLER_COMMAND_CHANGE_DIRECTION :
#ifdef STVID_TRICKMODE_BACKWARD
                if (VIDPROD_Data_p->Backward.ChangingDirection == TRICKMODE_CHANGING_FORWARD)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Change speed FWD >>> : Ask for skip until next RAP"));
                    VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint = TRUE;
                    VIDPROD_Data_p->Skip.RecoveringNextEntryPoint = TRUE;
                    VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = TRUE;
                    PARSER_Stop(VIDPROD_Data_p->ParserHandle);
                }
                else if (VIDPROD_Data_p->Backward.ChangingDirection == TRICKMODE_CHANGING_BACKWARD)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Change speed BWD <<< : Ask for skip until next RAP"));
                    VIDPROD_Data_p->Skip.AllPPictures = TRUE;
                    VIDPROD_Data_p->Skip.AllBPictures = TRUE;
                    VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint = TRUE;
                    VIDPROD_Data_p->Skip.RecoveringNextEntryPoint = TRUE;
                    VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = TRUE;
                    VIDPROD_Data_p->Backward.WaitingProducerReadyToSwitchBackward = FALSE;
                    VIDPROD_Data_p->Skip.AllPicturesNb = 0;
                    VIDPROD_Data_p->DriftTime = 0;
                    PARSER_FlushInjection(VIDPROD_Data_p->ParserHandle);
                    PARSER_Stop(VIDPROD_Data_p->ParserHandle);
                }
#else
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Backward is not supported !!!"));
#endif
                break;

            case CONTROLLER_COMMAND_SEARCH_PICTURE :
                VIDPROD_Data_p->ParserStatus.IsSearchingPicture = TRUE;
                if (VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint)
                {
#if defined(TRACE_UART) && !defined(VALID_TOOLS)
                    TraceBuffer(("Launch search for next RAP"));
#endif
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Launch search for next RAP"));
                    VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = TRUE;
                    VIDPROD_Data_p->ParserStatus.SearchNextRandomAccessPoint = TRUE;
                    VIDPROD_Data_p->Skip.UntilNextRandomAccessPointJustRequested = TRUE;
                    VIDPROD_Data_p->DecoderStatus.IsPicturePushStarted = FALSE;
                }
                PARSER_GetPictureParams.GetRandomAccessPoint = VIDPROD_Data_p->ParserStatus.SearchNextRandomAccessPoint;
                PARSER_GetPictureParams.ErrorRecoveryMode = VIDPROD_Data_p->ErrorRecoveryMode;
#ifdef ST_speed
                if ((VIDPROD_Data_p->Skip.AllPPicturesSpeed) || (VIDPROD_Data_p->Skip.AllPPictures))
                {
                    PARSER_GetPictureParams.DecodedPictures = STVID_DECODED_PICTURES_I;
                }
                else if ((VIDPROD_Data_p->Skip.AllBPicturesSpeed) || (VIDPROD_Data_p->Skip.AllBPictures))
                {
                    PARSER_GetPictureParams.DecodedPictures = STVID_DECODED_PICTURES_IP;
                }
                else
                {
                    PARSER_GetPictureParams.DecodedPictures = STVID_DECODED_PICTURES_ALL;
                }
#else
                PARSER_GetPictureParams.DecodedPictures = VIDPROD_Data_p->DecodedPictures;
#endif
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_PARSER_START, "pPARS_Start %s",
                        ((PARSER_GetPictureParams.GetRandomAccessPoint) ? "(RA) " : " ")));
#endif
                ErrorCode = PARSER_GetPicture(VIDPROD_Data_p->ParserHandle,&PARSER_GetPictureParams);
                if (ErrorCode != ST_NO_ERROR)
                {
                    VIDPROD_Data_p->ParserStatus.IsSearchingPicture = FALSE;
                    /* TODO_CL: error handling to be defined.
                     * We can imagine parser may not be available then shall we repost CONTROLLER_COMMAND_SEARCH_PICTURE and wait on timeout ?
                     * Other error cases have to be defined */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "PARSER_GetPicture failed Error=%d!!", ErrorCode));
                }
                else
                {
                    VIDPROD_Data_p->ParserStatus.MaxPictureSearchTime = time_plus(time_now(), MAX_PICTURE_SEARCH_TIME);
                    VIDPROD_Data_p->ParserStatus.SearchNextRandomAccessPoint = FALSE;
                    /* Now waiting for event from PARSER */
                }
                break;

            case CONTROLLER_COMMAND_NEW_PARSED_PICTURE : /* New parsed picture to process received from PARSER */
#ifdef STVID_TRICKMODE_BACKWARD
                if (VIDPROD_Data_p->ForTask.NewParsedPicture_p != NULL)
#endif
                {
                VIDPROD_Data_p->ParserStatus.IsSearchingPicture = FALSE;
                PictureBuffer_p = VIDPROD_Data_p->ForTask.NewParsedPicture_p;
                VIDPROD_Data_p->ForTask.NewParsedPicture_p = NULL;
#ifdef STVID_TRICKMODE_BACKWARD
                VIDPROD_Data_p->Backward.LevelAndProfile = PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.ProfileAndLevelIndication;
#endif
                VIDPROD_Data_p->CurrentPictureNumber++;
                PictureBuffer_p->PictureNumber = VIDPROD_Data_p->CurrentPictureNumber;
                if (VIDPROD_Data_p->Skip.UntilNextRandomAccessPointJustRequested)
                {
                    VIDPROD_Data_p->Skip.UntilNextRandomAccessPointJustRequested = FALSE;
                    VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint = FALSE;
                    VIDPROD_Data_p->NeedToDecreaseBBL = FALSE;
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "First picture found after next RAP request %d-%d/%d ParseError=%d",
                            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ParsingError));
                }

                /* Update reference list with this new parsed picture if a such request is pending */
                if (VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList)
                {
                    UpdateReferenceList(ProducerHandle, PictureBuffer_p);
                    VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = FALSE;
                }
                /* SaveMaxDecFrameBuffering for use to push pictures to display */
                VIDPROD_Data_p->DecoderStatus.CurrentMaxDecFrameBuffering = PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.MaxDecFrameBuffering;

                /* Patch ColorSpace identifier according to the broadcast profile if no suitable ColorSpaceConversion ID has been found in the stream */
                if (PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion == STGXOBJ_CONVERSION_MODE_UNKNOWN)
                {
                    if ((VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7100_H264)
                     || (VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_H264)
                     || (VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7200_H264))
                    {
                        vidprod_PatchColorSpaceConversionForH264(ProducerHandle, PictureBuffer_p);
                    }
                    else if ((VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2)
                          || (VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2)
                          || (VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS) /* PLE_TO_DO : check that the MPEG4P2 space conversion is available also for AVS */
                          || (VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2))
                    {
                        vidprod_PatchColorSpaceConversionForMPEG4P2(ProducerHandle, PictureBuffer_p);
                    }
                    else
                    {
                        vidprod_PatchColorSpaceConversionForMPEG(ProducerHandle, PictureBuffer_p);
                    }
                }
                /* Fill in Controller data for this picture */
                PictureBuffer_p->IsReferenceForDecoding = PictureBuffer_p->ParsedPictureInformation.PictureGenericData.IsReference;
                if (PictureBuffer_p->ParsedPictureInformation.PictureGenericData.AsynchronousCommands.ResumePresentationOnThisFrame)
                {
                    VIDPROD_Data_p->AsynchronousStatus.PresentationFreezed = FALSE;
                }
                PictureBuffer_p->IsNeededForPresentation = !(VIDPROD_Data_p->AsynchronousStatus.PresentationFreezed);
                PictureBuffer_p->IsPushed = FALSE;
                if (PictureBuffer_p->ParsedPictureInformation.PictureGenericData.AsynchronousCommands.FreezePresentationOnThisFrame)
                {
                    VIDPROD_Data_p->AsynchronousStatus.PresentationFreezed = TRUE;
                }
                PictureBuffer_p->FrameBuffer_p = NULL;  /* No frame buffer allocated yet, in this simplified controller, we wait for
                                                            CONFIRM_BUFFUER_EVT to allocate frame buffer */
                PictureBuffer_p->PPB.Address_p = NULL;
                PictureBuffer_p->PPB.Size = 0;
                PictureBuffer_p->PPB.FieldCounter = 0;
                PictureBuffer_p->PPB.AvmemPartitionHandleForAdditionnalData = (STAVMEM_PartitionHandle_t) 0;
                PictureBuffer_p->PPB.AvmemBlockHandle = (STAVMEM_BlockHandle_t) 0;

                VIDPROD_Data_p->DecoderOperatingMode.IsLowDelay = PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.IsLowDelay;

                /* PictureBuffer_p->Buffers... is not used before ConfirmBuffer in simplified controller */
                PictureBuffer_p->Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_PARSED;
                PictureBuffer_p->Decode.DecodingError         = VIDCOM_ERROR_LEVEL_NONE;
                PictureBuffer_p->AssociatedDecimatedPicture_p = NULL;
                PictureBuffer_p->ReferenceDecodingError       = VIDCOM_ERROR_LEVEL_NONE;
                if (PictureBuffer_p->ParsedPictureInformation.PictureGenericData.AsynchronousCommands.FlushPresentationQueue)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Flush Presentation Queue order received"));
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_PARSER_START, "pPARS_FlushQueue "));
#endif
                    vidprod_FlushPresentationQueue(ProducerHandle, PictureBuffer_p);
                }

                if ( (PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ParsingError >= VIDCOM_ERROR_LEVEL_BAD_DISPLAY) &&
                     !(IsSpoiledPictureToBeDisplayed(VIDPROD_Data_p->ErrorRecoveryMode)) )
                {
                    PictureBuffer_p->IsNeededForPresentation = FALSE;
                    if (PictureBuffer_p->AssociatedDecimatedPicture_p != NULL)
                    {
                        PictureBuffer_p->AssociatedDecimatedPicture_p->IsNeededForPresentation = FALSE;
                    }
                }

                /* Extract data for decode start condition check */
                if (VIDPROD_Data_p->StartCondition.FirstPictureAfterReset)
                {
                    /* Wait 700 ms maximum for bit buffer level completion */
                    VIDPROD_Data_p->StartCondition.MaxWaitForBufferLevel = time_plus(time_now(), (TOut_ClockTicks * (STOS_Clock_t)7 / (STOS_Clock_t)10));
#ifdef ST_avsync
                    if (!(VIDPROD_Data_p->StartCondition.StartWithPTSTimeComparison))
                    {
                    /* First picture after sequence, check if start is to be done on PTS reach condition */
                        if ( PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PTS.IsValid &&
                             !(PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PTS.Interpolated))
                        {
                            /* Take PTS information into account only if STC is available */
                            ErrorCode = STCLKRV_GetExtendedSTC(VIDPROD_Data_p->StartCondition.STClkrvHandle, &ExtendedSTC);
                            if (ErrorCode == ST_NO_ERROR)
                            {
                                /* True PTS and start on vbv_delay not activated: allow the start on PTS comparison time */
                                VIDPROD_Data_p->StartCondition.StartWithPTSTimeComparison = TRUE;
                                /* Save its PTS as first decode action condition */
                                VIDPROD_Data_p->StartCondition.FirstPicturePTSAfterReset = PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PTS;
                                VIDPROD_Data_p->StartCondition.FirstPictureAfterReset = FALSE;
                            }
                        } /* true PTS */
                    } /* Start not on vbv_delay */
#endif /* ST_avsync */
                    /* video start must be done with DecodeStartTime only if available !! */
                    if ((PictureBuffer_p->ParsedPictureInformation.PictureGenericData.IsDecodeStartTimeValid) && !(VIDPROD_Data_p->StartCondition.StartWithPTSTimeComparison))
                    {
                        /* the value of vbv_delay is the number of periods of a 90 kHz clock derived from the 27 MHz system clock */
                        VIDPROD_Data_p->StartCondition.StartOnPictureDelay = TRUE;
                        VIDPROD_Data_p->StartCondition.StartWithBufferFullness = FALSE;
                        VIDPROD_Data_p->StartCondition.PictureDelayTimerExpiredDate = PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodeStartTime;
                        VIDPROD_Data_p->StartCondition.FirstPictureAfterReset = FALSE;
                    }

                    if (!(VIDPROD_Data_p->StartCondition.StartOnPictureDelay) && !(VIDPROD_Data_p->StartCondition.StartWithPTSTimeComparison))
                    {
                        /* If no Picture Delay, try to start on bit buffer fullness */
                        /* If cpb_size/vbv-buffersize larger than bit buffer, arbitrary set the level
                        to reach before starting the decode of first picture at 2/3. */
                        if (PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.VBVBufferSize < VIDPROD_Data_p->BitBuffer.TotalSize)
                        {
                            /* By default, try to wait VBVBufferSize before starting the decode of first picture after Start(). */
                            VIDPROD_Data_p->StartCondition.BufferFullnessToReachBeforeStartInBytes = PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.VBVBufferSize;
                        }
                        else
                        {
#ifdef STVID_DEBUG_GET_STATISTICS
                            VIDPROD_Data_p->Statistics.DecodePbVbvSizeGreaterThanBitBuffer ++;
#endif /* STVID_DEBUG_GET_STATISTICS */
                            /* we will run at a Bit Buffer Level egal to 80% (arbitrary!) */
                            switch (VIDPROD_Data_p->DeviceType)
                            {
                                case STVID_DEVICE_TYPE_7100_H264 :
                                case STVID_DEVICE_TYPE_7109_H264 :
                                case STVID_DEVICE_TYPE_7200_H264 :
                                    /* if level is 3 or less, start on SD bit buffer size basis */
                                    if (((PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.ProfileAndLevelIndication & 0xFF) <= 30) && (VIDPROD_Data_p->BitBuffer.TotalSize > DEFAULT_BIT_BUFFER_SIZE_H264_SD_LIVE))
                                    {
                                        VIDPROD_Data_p->StartCondition.BufferFullnessToReachBeforeStartInBytes = BufferFullnessToReachWhenTooBig((U32) Min(VIDPROD_Data_p->BitBuffer.TotalSize, DEFAULT_BIT_BUFFER_SIZE_H264_SD_LIVE));
                                    }
                                    else
                                    {
                                        VIDPROD_Data_p->StartCondition.BufferFullnessToReachBeforeStartInBytes = BufferFullnessToReachWhenTooBig((U32) Min(VIDPROD_Data_p->BitBuffer.TotalSize, DEFAULT_BIT_BUFFER_SIZE_H264_HD_LIVE));
                                    }
                                    break;
                                case STVID_DEVICE_TYPE_7109_VC1 :
                                case STVID_DEVICE_TYPE_7200_VC1 :
                                    /* TODO Refine value */
                                    VIDPROD_Data_p->StartCondition.BufferFullnessToReachBeforeStartInBytes = BufferFullnessToReachWhenTooBig((U32) VIDPROD_Data_p->BitBuffer.TotalSize);
                                    break;
                                case STVID_DEVICE_TYPE_7100_MPEG :
                                case STVID_DEVICE_TYPE_7109_MPEG :
                                case STVID_DEVICE_TYPE_7109D_MPEG :
                                case STVID_DEVICE_TYPE_7200_MPEG :
                                default :
                                    VIDPROD_Data_p->StartCondition.BufferFullnessToReachBeforeStartInBytes = BufferFullnessToReachWhenTooBig((U32) VIDPROD_Data_p->BitBuffer.TotalSize);
                                    /* Test if the VBVSizeToReachBeforeStart is available according   */
                                    /* to maximum values allowed by ATSC, ... specifications.         */
                                    if (VIDPROD_Data_p->StartCondition.BufferFullnessToReachBeforeStartInBytes > MPEG_MAX_VBV_SIZE_TO_REACH_BEFORE_START_IN_BYTES)
                                    {
                                        VIDPROD_Data_p->StartCondition.BufferFullnessToReachBeforeStartInBytes = MPEG_MAX_VBV_SIZE_TO_REACH_BEFORE_START_IN_BYTES;
                                    }
                                    break;
                            }
                        }

                        /* Adjust BufferFullnessToReachBeforeStartInBytes according to codec latency */
                        /* Evaluate codec latency in term of bit buffer filling */
                        /* Evaluate 1 VSync in term of bit buffer filling */
                        CodecPlusOneVSyncDecodeLatencyInBytes = ((PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.BitRate * VIDPROD_Data_p->CodecInfo.MaximumDecodingLatencyInSystemClockUnit) / STVID_MPEG_CLOCKS_ONE_SECOND_DURATION / (U32)8 ) +
                            ((PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.BitRate *  (U32)1000) / (STVID_DEFAULT_VALID_VTG_FRAME_RATE * (U32)8));

                        if (VIDPROD_Data_p->StartCondition.BufferFullnessToReachBeforeStartInBytes > CodecPlusOneVSyncDecodeLatencyInBytes)
                        {
                            VIDPROD_Data_p->StartCondition.BufferFullnessToReachBeforeStartInBytes = VIDPROD_Data_p->StartCondition.BufferFullnessToReachBeforeStartInBytes - CodecPlusOneVSyncDecodeLatencyInBytes;
                        }
                        VIDPROD_Data_p->StartCondition.StartWithBufferFullness = TRUE;
                        VIDPROD_Data_p->StartCondition.FirstPictureAfterReset = FALSE;
                    }
                }

                VIDPROD_Data_p->NextAction.LaunchDecodeWhenPossible++;
                if (PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ParsingError <= VIDCOM_ERROR_LEVEL_INFORMATION)
                {
                /* Notify sequence info event if necessary */
                vidprod_CheckSequenceInfoChangeAndNotify(ProducerHandle, PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo);
                }
                }
                vidprod_TryToPushPictureToDisplay(ProducerHandle);
                break;

            case CONTROLLER_COMMAND_CONFIRM_PICTURE_BUFFER : /* DECODER asked to confirm frame buffer for current decode */
                DecodingJobResult = VIDPROD_Data_p->ForTask.PictureToConfirmResult[VIDPROD_Data_p->ForTask.NextIndexToConfirm];
                VIDPROD_Data_p->ForTask.CurrentCommandIDToConfirm = DecodingJobResult.CommandId;
                STOS_InterruptLock();
                if (VIDPROD_Data_p->ForTask.NbIndexToConfirm > 0)
                {
                    VIDPROD_Data_p->ForTask.NbIndexToConfirm--;
                }
                else
                {
                    VIDPROD_Data_p->ForTask.CurrentCommandIDToConfirm = 0;
                }
                STOS_InterruptUnlock();

                if (VIDPROD_Data_p->ForTask.CurrentCommandIDToConfirm == 0)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "NbIndexToConfirm was already == 0 !!! "));
                }

                VIDPROD_Data_p->ForTask.NextIndexToConfirm = (VIDPROD_Data_p->ForTask.NextIndexToConfirm+1) % MAX_NUMBER_OF_PREPROCESSING_PICTURES;

                LoopCounter = 0;
                LastConfirmedPicture_p = NULL;
#ifdef STVID_TRICKMODE_BACKWARD
                if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
                {
                while ((LoopCounter < MAX_NUMBER_OF_PREPROCESSING_PICTURES) &&
                    (LastConfirmedPicture_p == NULL))
                {
                    if (VIDPROD_Data_p->PreprocessingPicture.IsPictureBufferUsed[LoopCounter] &&
                    (VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId == VIDPROD_Data_p->ForTask.CurrentCommandIDToConfirm))
                    {
                        VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.StartTime = DecodingJobResult.DecodingJobStatus.JobSubmissionTime;
                        VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.ConfirmEVTTime = DecodingJobResult.DecodingJobStatus.JobCompletionTime;
                        VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_WAITING_FRAME_BUFFER;
                        LastConfirmedPicture_p = &(VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter]);
                    }
                    else
                    {
                        LoopCounter++;
                    }
                }
                }
#ifdef STVID_TRICKMODE_BACKWARD
                else if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p != NULL) /* is backward*/
                {
                    while ((LoopCounter < MAX_BACKWARD_PICTURES) &&
                        (LastConfirmedPicture_p == NULL))
                    {
                        if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.IsPictureBufferUsed[LoopCounter] &&
                        (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId == VIDPROD_Data_p->ForTask.CurrentCommandIDToConfirm))
                        {
                            VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.StartTime = DecodingJobResult.DecodingJobStatus.JobSubmissionTime;
                            VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.ConfirmEVTTime = DecodingJobResult.DecodingJobStatus.JobCompletionTime;
                            VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_WAITING_FRAME_BUFFER;
                            LastConfirmedPicture_p = &(VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter]);
							VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->Index = LoopCounter;
                        }
                        else
                        {
                            LoopCounter++;
                        }
                    }
                }
#endif /* STVID_TRICKMODE_BACKWARD */

                if (LastConfirmedPicture_p == NULL)
                {
                    if (VIDPROD_Data_p->ForTask.CurrentCommandIDToConfirm != 0)
                    {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_ERRORS, "pDEC_CBErr3-%d ", VIDPROD_Data_p->ForTask.CurrentCommandIDToConfirm));
                        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODER_PROCESSING, "pDEC_CBAbort-%d ", VIDPROD_Data_p->ForTask.CurrentCommandIDToConfirm));
#endif
                        if ((VIDPROD_Data_p->Stop.IsPending) || (VIDPROD_Data_p->Skip.RecoveringNextEntryPoint) || (VIDPROD_Data_p->ForTask.SoftResetPending))
                        {
                            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Module Producer DecoderConfirmBufferCB : Abort because CommandID %d has been already removed from Preprocessing pictures table", VIDPROD_Data_p->ForTask.CurrentCommandIDToConfirm));
                        }
                        else
                        {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Producer DecoderConfirmBufferCB : Abort because CommandID %d not found in Preprocessing pictures table", VIDPROD_Data_p->ForTask.CurrentCommandIDToConfirm));
                        ErrorCode = DECODER_Abort(VIDPROD_Data_p->DecoderHandle, VIDPROD_Data_p->ForTask.CurrentCommandIDToConfirm);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            /* NOTE_CL: unexpected error, decoder internal data failure or bad producer data handling */
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER_Abort rejected ID=%d!!", VIDPROD_Data_p->ForTask.CurrentCommandIDToConfirm));
                        }
                        }
                        /* Dont' try to release anything form preprocessing table, This is an unexpected producer internal error */
                        VIDPROD_Data_p->ForTask.CurrentCommandIDToConfirm = 0;
#ifdef STVID_TRICKMODE_BACKWARD
                        VIDPROD_Data_p->Backward.NbDecodedPictures ++;
#endif
                    }
                }
                else
                {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODER_PROCESSING, "pDEC_CBProcess%d(%d-%d/%d) ", VIDPROD_Data_p->ForTask.CurrentCommandIDToConfirm, LastConfirmedPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, LastConfirmedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, LastConfirmedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
#ifndef STVID_PRODUCER_NO_TIMEOUT
#ifdef STVID_TRICKMODE_BACKWARD
                    if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
                    {
                    if(VIDPROD_Data_p->PreprocessingPicture.IsPictureTimedOut[LoopCounter])
                    {
                        VIDPROD_Data_p->UnsuccessfullDecodeCount--;
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_TIMEOUT, "pDEC_PPTOEnded(%d-%d/%d) ",
                                    LastConfirmedPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                    LastConfirmedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                    LastConfirmedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Preprocessing finally ENDED on picture DOFID=%d ExtPOC=%d/%d (duration=%"FORMAT_SPEC_OSCLOCK"d us)",
                                    LastConfirmedPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                    LastConfirmedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                    LastConfirmedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                                    time_minus(LastConfirmedPicture_p->Decode.ConfirmEVTTime, LastConfirmedPicture_p->Decode.StartTime)*OSCLOCK_T_MILLION/TOut_ClockTicks));
                    }
                    }
#endif /* not STVID_PRODUCER_NO_TIMEOUT */

                    if(DecodingJobResult.ErrorCode != DECODER_NO_ERROR)
                    {
#ifdef STVID_DEBUG_GET_STATISTICS
                        VIDPROD_Data_p->Statistics.DecodePbInterruptSyntaxError++;
#endif /* STVID_DEBUG_GET_STATISTICS */
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "CONTROLLER_COMMAND_CONFIRM_PICTURE_BUFFER -> PreProc returned Errors CmdId=%d Picture %c (%d-%d/%d) start@ 0x%x end@ 0x%x",
                                                                 VIDPROD_Data_p->ForTask.CurrentCommandIDToConfirm,
                                                                 MPEGFrame2Char(LastConfirmedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame),
                                                                 LastConfirmedPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                                                 LastConfirmedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                                                 LastConfirmedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                                                                 LastConfirmedPicture_p->ParsedPictureInformation.PictureGenericData.BitBufferPictureStartAddress,
                                                                 LastConfirmedPicture_p->ParsedPictureInformation.PictureGenericData.BitBufferPictureStopAddress
                                                                 ));
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_ERRORS, "pDEC_CBErr1-%d(%d-%d/%d)",
                                                     VIDPROD_Data_p->ForTask.CurrentCommandIDToConfirm,
                                                     LastConfirmedPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                                     LastConfirmedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                                     LastConfirmedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
#ifdef STVID_DEBUG_GET_STATISTICS
                        VIDPROD_Data_p->Statistics.DecodePbPreprocError++;
#endif /* STVID_DEBUG_GET_STATISTICS */
                        LastConfirmedPicture_p->Decode.DecodingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
                        if (IsPictureToBeSkippedInCaseOfPreProcessingError(VIDPROD_Data_p->ErrorRecoveryMode))
                        {
                            /* Abort the picture decode on pre processing error */
                            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Abort the decode cmdID=%d", LastConfirmedPicture_p->Decode.CommandId));
                            ErrorCode = DECODER_Abort(VIDPROD_Data_p->DecoderHandle, LastConfirmedPicture_p->Decode.CommandId);
                            if (ErrorCode != ST_NO_ERROR)
                            {
                                /* NOTE_CL: unexpected error, decoder internal data failure or bad producer data handling */
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER_Abort rejected ID=%d!!", LastConfirmedPicture_p->Decode.CommandId));
                            }
#ifdef STVID_TRICKMODE_BACKWARD
                            if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
                            {
                                vidprod_RemovePictureFromPreprocessingTable(ProducerHandle, LastConfirmedPicture_p);
                            }
#ifdef STVID_TRICKMODE_BACKWARD
                            else
                            {
                                vidprod_RemovePictureFromPreprocessingTableBackward(ProducerHandle, LastConfirmedPicture_p);
                                VIDPROD_Data_p->Backward.NbDecodedPictures ++;
                            }
#endif /* STVID_TRICKMODE_BACKWARD */
                            /* This decode is aborted, decrease VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue */
                            DECREMENT_NB_PICTURES_IN_QUEUE(ProducerHandle);
                            if ((LastConfirmedPicture_p->IsReferenceForDecoding) && IsSpoiledReferenceLoosingAllReferencesUntilNextRandomAccessPoint(VIDPROD_Data_p->ErrorRecoveryMode))
                            {
                                if ((!IsIOnlyDecodeOnGoing(VIDPROD_Data_p->Skip)) || (IsFirstOfTwoFieldsOfIFrame(LastConfirmedPicture_p)))
								{
                                    /* Search next entry point because a reference picture has been aborted and we are not decoding I only */
                                	STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Ask for skip until next RAP"));
                                    VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint = TRUE;
                                    VIDPROD_Data_p->Skip.RecoveringNextEntryPoint = TRUE;
                                    VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = TRUE;
								}
                            }
                        }
                        else
                        {
                            VIDPROD_Data_p->NextAction.ConfirmBufferWhenPossible++;
                        }
                    }
                    else
                    {
                        VIDPROD_Data_p->NextAction.ConfirmBufferWhenPossible++;
                    }
                }
                break;

            case CONTROLLER_COMMAND_PARSER_PICTURE_SKIPPED:
#ifdef STVID_TRICKMODE_BACKWARD
                if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
                {
                    /* When a picture skip at the parse level (generally because we are
                    searching for the next RAP, when there is no parsed picture waiting for decode and the current decodes are
                    finished, the read point is actually the parser read point */
                    if ((VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue == 0) && (VIDPROD_Data_p->PreprocessingPicture.NbUsedPictureBuffers == 0))
                    {
                        InformReadAddressParams.DecoderReadAddress_p = VIDPROD_Data_p->ParserStatus.LastPictureSkippedStopAddress_p;
                        ErrorCode = PARSER_InformReadAddress(VIDPROD_Data_p->ParserHandle, &InformReadAddressParams);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            /* TODO_CL: error handling to be defined.
                             * error cases have to be defined */
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "PARSER_InformReadAddress failed Error=%d!!", ErrorCode));
                        }
                        /* The picture skip has been taken into account */
                        VIDPROD_Data_p->ParserStatus.IsPictureSkipJustHappened = FALSE;
                    }
                }
                break;
            case CONTROLLER_COMMAND_DECODER_READ_ADDRESS_UPDATED : /* DECODER sent its new last read address, update PARSER */
#ifdef STVID_TRICKMODE_BACKWARD
                if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
                {
#ifdef STVID_PRESERVE_LAST_PICTURES_IN_BB
                ReadAddressFifo[ReadAddressFifoNextInputIndex] = VIDPROD_Data_p->DecoderStatus.LastReadAddress_p;
                ReadAddressFifoNextInputIndex = (ReadAddressFifoNextInputIndex + 1) % READ_ADDRESS_FIFO_DEPTH;
                ReadAddressFifoNbUsedEntries++;
                if(ReadAddressFifoNbUsedEntries == READ_ADDRESS_FIFO_DEPTH)
                {
                    InformReadAddressParams.DecoderReadAddress_p = ReadAddressFifo[ReadAdressFifoNextOutputIndex];
                    ReadAdressFifoNextOutputIndex = (ReadAdressFifoNextOutputIndex + 1) % READ_ADDRESS_FIFO_DEPTH;
                    ReadAddressFifoNbUsedEntries--;
#else /* STVID_PRESERVE_LAST_PICTURES_IN_BB */
                InformReadAddressParams.DecoderReadAddress_p = VIDPROD_Data_p->DecoderStatus.LastReadAddress_p;
#endif /* not STVID_PRESERVE_LAST_PICTURES_IN_BB */
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_INFORM_READ_ADDR, "pPARS_IRA "));
#endif /* VALID_TOOLS */

                if ((VIDPROD_Data_p->ProducerState == VIDPROD_DECODER_STATE_STOPPED) || (VIDPROD_Data_p->Stop.IsPending))
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DECODER_INFORM_READ_ADDRESS_EVT received (@ 0x%x) while producer is STOPPING or STOPPED. no action", InformReadAddressParams.DecoderReadAddress_p));
                }
                else
                {
                ErrorCode = PARSER_InformReadAddress(VIDPROD_Data_p->ParserHandle, &InformReadAddressParams);
                if (ErrorCode != ST_NO_ERROR)
                {
                    /* TODO_CL: error handling to be defined.
                     * error cases have to be defined */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "PARSER_InformReadAddress failed Error=%d!!", ErrorCode));
                    }
                }
#ifdef STVID_PRESERVE_LAST_PICTURES_IN_BB
                }
#endif /* not STVID_PRESERVE_LAST_PICTURES_IN_BB */
                }
                break;

            case CONTROLLER_COMMAND_NEW_DECODED_PICTURE : /* Decoded picture received from DECODER */
                STOS_InterruptLock();
                VIDPROD_Data_p->ForTask.NewPictureDecodeResults = VIDPROD_Data_p->ForTask.CompletedPictureResults[VIDPROD_Data_p->ForTask.NextCompletedIndex];
                VIDPROD_Data_p->ForTask.NbCompletedIndex--;
                VIDPROD_Data_p->ForTask.NextCompletedIndex = (VIDPROD_Data_p->ForTask.NextCompletedIndex+1) % MAX_NUMBER_OF_COMPLETED_PICTURES;
                STOS_InterruptUnlock();

                LoopCounter = 0;
                PictureBuffer_p = NULL;
                while ((LoopCounter < MAX_NUMBER_OF_DECODE_PICTURES) &&
                    (PictureBuffer_p == NULL))
                {
                    if ((VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter] != NULL) &&
                    (VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->Decode.CommandId == VIDPROD_Data_p->ForTask.NewPictureDecodeResults.CommandId) &&
                    (VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_DECODING))
                    {
                        PictureBuffer_p = VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter];
                    }
                    else
                    {
                        LoopCounter++;
                    }
                }
				RememberIndex = LoopCounter;

                if (PictureBuffer_p != NULL)
                {
                    if ((VIDPROD_Data_p->ProducerState == VIDPROD_DECODER_STATE_STOPPED) || (VIDPROD_Data_p->Stop.IsPending))
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "PicDECODED while stopping CmdID=%d (%d-%d/%d)",
                            PictureBuffer_p->Decode.CommandId,
                            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
                    }
                    if ( vidprod_IsEventToBeNotified(ProducerHandle, VIDPROD_NEW_PICTURE_DECODED_EVT_ID) )
                    {
                        /* Notify STVID_NEW_PICTURE_DECODED_EVT event */
                        ExportedPictureInfos = PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos;
                        ExportedPictureInfos.BitmapParams.Data1_p = Virtual(ExportedPictureInfos.BitmapParams.Data1_p);
                        ExportedPictureInfos.BitmapParams.Data2_p = Virtual(ExportedPictureInfos.BitmapParams.Data2_p);

                        /* Setting the right value of DecimationFactors, as defined in requirement document, before sending the event */
                        if(PictureBuffer_p->AssociatedDecimatedPicture_p != NULL)
                        {
                            ExportedPictureInfos.VideoParams.DecimationFactors
                                = PictureBuffer_p->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimationFactors;
                            ExportedPictureInfos.VideoParams.DecimatedBitmapParams.Data1_p
                                = Virtual(ExportedPictureInfos.VideoParams.DecimatedBitmapParams.Data1_p);
                            ExportedPictureInfos.VideoParams.DecimatedBitmapParams.Data2_p
                                = Virtual(ExportedPictureInfos.VideoParams.DecimatedBitmapParams.Data2_p);
                        }

                        STEVT_Notify(VIDPROD_Data_p->EventsHandle,
                                VIDPROD_Data_p->RegisteredEventsID[VIDPROD_NEW_PICTURE_DECODED_EVT_ID],
                                STEVT_EVENT_DATA_TYPE_CAST &(ExportedPictureInfos));

                    }
#ifdef STVID_TRICKMODE_BACKWARD
                    PictureBuffer_p->SmoothBackwardPictureInfo_p = VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p;
					if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p!=NULL) /* Is Backward */
					{
						if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->FirstIPictureInBuffer_p == NULL)
						{
							STTBX_Report((STTBX_REPORT_LEVEL_INFO, "First I picture in decode buffer is NULL"));
						}
                        if (!VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->IsFirstPictureDecoded)
                        {
                            VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->IsFirstPictureDecoded = TRUE;
                            STEVT_Notify(VIDPROD_Data_p->EventsHandle, VIDPROD_Data_p->RegisteredEventsID[VIDPROD_DECODE_FIRST_PICTURE_BACKWARD_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
                        }
                        VIDPROD_Data_p->Backward.NbDecodedPictures++;
					}
#endif
                    PictureBuffer_p->Decode.DecodeStartTime  = VIDPROD_Data_p->ForTask.NewPictureDecodeResults.DecodingJobStatus.JobSubmissionTime;
                    PictureBuffer_p->Decode.JobCompletedTime = VIDPROD_Data_p->ForTask.NewPictureDecodeResults.DecodingJobStatus.JobCompletionTime;
#ifndef STVID_PRODUCER_NO_TIMEOUT
                    if (VIDPROD_Data_p->DecodePicture.IsPictureTimedOut[LoopCounter])
                    {
                        VIDPROD_Data_p->UnsuccessfullDecodeCount--;
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_TIMEOUT, "pDEC_FMWTOEnded(%d-%d/%d) ",
                                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Decode finally ENDED on picture DOFID=%d ExtPOC=%d/%d (duration=%"FORMAT_SPEC_OSCLOCK"d us)",
                                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                                    time_minus(PictureBuffer_p->Decode.JobCompletedTime, PictureBuffer_p->Decode.DecodeStartTime)*OSCLOCK_T_MILLION/TOut_ClockTicks));
                    } /* if (VIDPROD_Data_p->DecodePicture.IsPictureTimedOut[LoopCounter]) */
#endif /* not STVID_PRODUCER_NO_TIMEOUT */

#ifdef STVID_DEBUG_GET_STATISTICS
                    VIDPROD_Data_p->Statistics.DecodeTimeNbPictures++;
                    PreprocessTime = (U32)(time_minus(PictureBuffer_p->Decode.ConfirmEVTTime, PictureBuffer_p->Decode.StartTime)*OSCLOCK_T_MILLION/TOut_ClockTicks);
                    BufferSearchTime = (U32)(time_minus(PictureBuffer_p->Decode.ConfirmedTime, PictureBuffer_p->Decode.BufferSeachStartTime)*OSCLOCK_T_MILLION/TOut_ClockTicks);
                    DecodeTime = (U32)(time_minus(PictureBuffer_p->Decode.JobCompletedTime, PictureBuffer_p->Decode.DecodeStartTime)*OSCLOCK_T_MILLION/TOut_ClockTicks);
                    FullDecodeTime = (U32)(time_minus(PictureBuffer_p->Decode.JobCompletedTime, PictureBuffer_p->Decode.StartTime)*OSCLOCK_T_MILLION/TOut_ClockTicks);
#ifdef STVID_VALID_MEASURE_TIMING
                    LXDecodeTimeInMicros = PictureBuffer_p->Decode.LXDecodeTimeInMicros;
#endif /* STVID_VALID_MEASURE_TIMING */
                    if (!IsFramePicture(PictureBuffer_p))
                    {
                        PreprocessTime *= 2;
                        DecodeTime  *= 2;
                        FullDecodeTime *= 2;
#ifdef STVID_VALID_MEASURE_TIMING
                        LXDecodeTimeInMicros *= 2;
#endif /* STVID_VALID_MEASURE_TIMING */
                    }

                    if (PreprocessTime > VIDPROD_Data_p->Statistics.DecodeTimeMaxPreprocessTime)
                    {
                        VIDPROD_Data_p->Statistics.DecodeTimeMaxPreprocessTime = PreprocessTime;
                    }
                    if (PreprocessTime < VIDPROD_Data_p->Statistics.DecodeTimeMinPreprocessTime)
                    {
                        VIDPROD_Data_p->Statistics.DecodeTimeMinPreprocessTime = PreprocessTime;
                    }
                    VIDPROD_Data_p->Statistics.DecodeTimeSumOfPreprocessTime += PreprocessTime;

                    if (BufferSearchTime > VIDPROD_Data_p->Statistics.DecodeTimeMaxBufferSearchTime)
                    {
                        VIDPROD_Data_p->Statistics.DecodeTimeMaxBufferSearchTime = BufferSearchTime;
                    }
                    if (BufferSearchTime < VIDPROD_Data_p->Statistics.DecodeTimeMinBufferSearchTime)
                    {
                        VIDPROD_Data_p->Statistics.DecodeTimeMinBufferSearchTime = BufferSearchTime;
                    }
                    VIDPROD_Data_p->Statistics.DecodeTimeSumOfBufferSearchTime += BufferSearchTime;

                    if (DecodeTime > VIDPROD_Data_p->Statistics.DecodeTimeMaxDecodeTime)
                    {
                        VIDPROD_Data_p->Statistics.DecodeTimeMaxDecodeTime = DecodeTime;
                    }
                    if (DecodeTime < VIDPROD_Data_p->Statistics.DecodeTimeMinDecodeTime)
                    {
                        VIDPROD_Data_p->Statistics.DecodeTimeMinDecodeTime = DecodeTime;
                    }
                    VIDPROD_Data_p->Statistics.DecodeTimeSumOfDecodeTime += DecodeTime;

                    if (FullDecodeTime > VIDPROD_Data_p->Statistics.DecodeTimeMaxFullDecodeTime)
                    {
                        VIDPROD_Data_p->Statistics.DecodeTimeMaxFullDecodeTime = FullDecodeTime;
                    }
                    if (FullDecodeTime < VIDPROD_Data_p->Statistics.DecodeTimeMinFullDecodeTime)
                    {
                        VIDPROD_Data_p->Statistics.DecodeTimeMinFullDecodeTime = FullDecodeTime;
                    }
                    VIDPROD_Data_p->Statistics.DecodeTimeSumOfFullDecodeTime += FullDecodeTime;
#ifdef STVID_VALID_MEASURE_TIMING
                    if (LXDecodeTimeInMicros > VIDPROD_Data_p->Statistics.DecodeTimeMaxLXDecodeTime)
                    {
                        VIDPROD_Data_p->Statistics.DecodeTimeMaxLXDecodeTime = LXDecodeTimeInMicros;
                    }
                    if (LXDecodeTimeInMicros < VIDPROD_Data_p->Statistics.DecodeTimeMinLXDecodeTime)
                    {
                        VIDPROD_Data_p->Statistics.DecodeTimeMinLXDecodeTime = LXDecodeTimeInMicros;
                    }
                    VIDPROD_Data_p->Statistics.DecodeTimeSumOfLXDecodeTime += LXDecodeTimeInMicros;
#endif /* STVID_VALID_MEASURE_TIMING */
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_TRICKMODE_BACKWARD
                    if (VIDPROD_Data_p->Backward.IsBackward) /* For smoothbackward, need to add a test on picture type */
                    {
                        VIDPROD_Data_p->Backward.TimeToDecodeIPicture = (U32)(time_minus(PictureBuffer_p->Decode.JobCompletedTime, PictureBuffer_p->Decode.DecodeStartTime)*OSCLOCK_T_MILLION/TOut_ClockTicks);
                    }
#endif /* STVID_TRICKMODE_BACKWARD */
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODER_PROCESSING, "pDEC_JCProcess%d(%d-%d/%d) ", VIDPROD_Data_p->ForTask.NewPictureDecodeResults.CommandId, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#ifdef STVID_DEBUG_GET_STATISTICS
                    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_PROCESSING_TIME, "pPP:%"FORMAT_SPEC_OSCLOCK"u(%u-%u)/SB:%"FORMAT_SPEC_OSCLOCK"u(%u-%u)/DEC:%"FORMAT_SPEC_OSCLOCK"u(%u-%u)/ALL:%"FORMAT_SPEC_OSCLOCK"d(%u-%u) ",
                                                    time_minus(PictureBuffer_p->Decode.ConfirmEVTTime, PictureBuffer_p->Decode.StartTime)*OSCLOCK_T_MILLION/TOut_ClockTicks,
                                                 VIDPROD_Data_p->Statistics.DecodeTimeMinPreprocessTime,
                                                 VIDPROD_Data_p->Statistics.DecodeTimeMaxPreprocessTime,
                                                    time_minus(PictureBuffer_p->Decode.ConfirmedTime, PictureBuffer_p->Decode.BufferSeachStartTime)*OSCLOCK_T_MILLION/TOut_ClockTicks,
                                                 VIDPROD_Data_p->Statistics.DecodeTimeMinBufferSearchTime,
                                                 VIDPROD_Data_p->Statistics.DecodeTimeMaxBufferSearchTime,
                                                    time_minus(PictureBuffer_p->Decode.JobCompletedTime, PictureBuffer_p->Decode.DecodeStartTime)*OSCLOCK_T_MILLION/TOut_ClockTicks,
                                                 VIDPROD_Data_p->Statistics.DecodeTimeMinDecodeTime,
                                                 VIDPROD_Data_p->Statistics.DecodeTimeMaxDecodeTime,
                                                    time_minus(PictureBuffer_p->Decode.JobCompletedTime, PictureBuffer_p->Decode.StartTime)*OSCLOCK_T_MILLION/TOut_ClockTicks,
                                                 VIDPROD_Data_p->Statistics.DecodeTimeMinFullDecodeTime,
                                                 VIDPROD_Data_p->Statistics.DecodeTimeMaxFullDecodeTime));
#else /* STVID_DEBUG_GET_STATISTICS */
                        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_PROCESSING_TIME, "pPP:%d/SB:%d/DEC:%d/ALL:%d ",
                                                    (U32)time_minus(PictureBuffer_p->Decode.ConfirmEVTTime, PictureBuffer_p->Decode.StartTime)*1000/TOut_ClockTicks,
                                                    (U32)time_minus(PictureBuffer_p->Decode.ConfirmedTime, PictureBuffer_p->Decode.BufferSeachStartTime)*1000/TOut_ClockTicks,
                                                    (U32)time_minus(PictureBuffer_p->Decode.JobCompletedTime, PictureBuffer_p->Decode.DecodeStartTime)*1000/TOut_ClockTicks,
                                                    (U32)time_minus(PictureBuffer_p->Decode.JobCompletedTime, PictureBuffer_p->Decode.StartTime)*1000/TOut_ClockTicks));
#endif /* not STVID_DEBUG_GET_STATISTICS */
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */
                    if (VIDPROD_Data_p->ForTask.NewPictureDecodeResults.DecodingJobStatus.JobState == DECODER_JOB_COMPLETED)
                    {
                        if ((VIDPROD_Data_p->Stop.AfterCurrentPicture) &&
                            (VIDPROD_Data_p->LastParsedPictureDecodingOrderFrameID == PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID) &&
                            (IsFramePicture(PictureBuffer_p) || IsSecondOfTwoFields(PictureBuffer_p)) &&
                            (vidcom_ComparePictureID(&(VIDPROD_Data_p->LastParsedPictureExtendedPresentationOrderPictureID),
                                                     &(PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID)) == 0))
                        {
                            /* Order stop now */
                            PushControllerCommand(ProducerHandle, CONTROLLER_COMMAND_STOP);
                            VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint = TRUE;
                            VIDPROD_Data_p->Skip.RecoveringNextEntryPoint = TRUE;
                            VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = TRUE;
                        }
                        VIDPROD_Data_p->ForTask.LastDecodedPicture_p = PictureBuffer_p;
                        /* Copy last decoded picture in VIDPROD_Data_p->DecoderStatus for future use in STOPPED event notification */
                        VIDPROD_Data_p->DecoderStatus.LastDecodedPicture_p = PictureBuffer_p;

                        if (VIDPROD_Data_p->ForTask.NewPictureDecodeResults.ErrorCode != DECODER_NO_ERROR)
                        {
#ifdef STVID_DEBUG_GET_STATISTICS
                            VIDPROD_Data_p->Statistics.DecodePbInterruptSyntaxError++;
#endif /* STVID_DEBUG_GET_STATISTICS */
                            VIDPROD_Data_p->UnsuccessfullDecodeCount++;
                            vidcom_PictureInfosToPictureParams(&(VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos),
                                    &CurrentPictureParams, VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID);
                            STEVT_Notify(VIDPROD_Data_p->EventsHandle, VIDPROD_Data_p->RegisteredEventsID[VIDPROD_PICTURE_DECODING_ERROR_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(CurrentPictureParams));
                            VIDPROD_Data_p->ForTask.LastDecodedPicture_p->Decode.DecodingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                            TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_ERRORS, "pDEC_JCErr3-%d(%d-%d/%d) ", VIDPROD_Data_p->ForTask.NewPictureDecodeResults.CommandId, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Producer CONTROLLER_COMMAND_NEW_DECODED_PICTURE : Decoder returned errors ID=%d %c (%d-%d/%d)!!",
                                    VIDPROD_Data_p->ForTask.NewPictureDecodeResults.CommandId,
                                    MPEGFrame2Char(PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame),
                                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
                            /* Search next entry point if the picture was a reference */
                            if ((VIDPROD_Data_p->ForTask.LastDecodedPicture_p->IsReferenceForDecoding) && IsSpoiledReferenceLoosingAllReferencesUntilNextRandomAccessPoint(VIDPROD_Data_p->ErrorRecoveryMode))
                            {
                                if ((!IsIOnlyDecodeOnGoing(VIDPROD_Data_p->Skip)) || (IsFirstOfTwoFieldsOfIFrame(VIDPROD_Data_p->ForTask.LastDecodedPicture_p)))
								{
                                    /* Search next entry point because a reference picture has been badly decoded and we are not decoding I only */
                                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Ask for skip until next RAP"));
                                    VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint = TRUE;
                                    VIDPROD_Data_p->Skip.RecoveringNextEntryPoint = TRUE;
                                    VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = TRUE;
								}
                            }
                        }
                        PictureBuffer_p->Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_DECODED;
                        if (PictureBuffer_p->AssociatedDecimatedPicture_p != NULL)
                        {
                            PictureBuffer_p->AssociatedDecimatedPicture_p->Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_DECODED;
                        }
                        DECREMENT_NB_PICTURES_IN_QUEUE(ProducerHandle);
                    }
                    else
                    {
/* TODO_PC (CONTROLLER_COMMAND_NEW_DECODED_PICTURE) : Add Job Not completed error management  */
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_ERRORS, "pDEC_JCErr4-%d(%d-%d/%d) ", VIDPROD_Data_p->ForTask.NewPictureDecodeResults.CommandId, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Producer CONTROLLER_COMMAND_NEW_DECODED_PICTURE : Job not completed ID=%d(%d-%d/%d) !!!!! !!", VIDPROD_Data_p->ForTask.NewPictureDecodeResults.CommandId, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
                        return;
                    }

                    /* here, PictureBuffer_p is not NULL so VIDPROD_Data_p->ForTask.LastDecodedPicture_p should not bel NULL as well
                    * else it's a bug in the test above  */
                    if (VIDPROD_Data_p->ForTask.LastDecodedPicture_p == NULL)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "BUG-BUG-BUG Module Producer unexpected NULL Picture found !"));
                        return;
                    }

                    /* don't display spoiled pictures */
                    if ( ((VIDPROD_Data_p->ForTask.LastDecodedPicture_p->Decode.DecodingError != VIDCOM_ERROR_LEVEL_NONE) ||
                            (VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ReferenceDecodingError != VIDCOM_ERROR_LEVEL_NONE)) &&
                        !(IsSpoiledPictureToBeDisplayed(VIDPROD_Data_p->ErrorRecoveryMode)))
                    {
                        VIDPROD_Data_p->ForTask.LastDecodedPicture_p->IsNeededForPresentation = FALSE;
                        if (VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p != NULL)
                        {
                            VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p->IsNeededForPresentation = FALSE;
                        }
                    }
#if !(defined(VALID_TOOLS) && (defined(STVID_FRAME_BUFFER_DUMP) || defined(STVID_FB_ADD_DATA_DUMP) || defined(STVID_DECIMATED_FRAME_BUFFER_DUMP)))
                    if ( (VIDPROD_Data_p->ProducerState == VIDPROD_DECODER_STATE_STOPPED) || (VIDPROD_Data_p->Stop.IsPending) )
                    {
                        VIDPROD_Data_p->ForTask.LastDecodedPicture_p->IsNeededForPresentation = FALSE;
                        if (VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p != NULL)
                        {
                            VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p->IsNeededForPresentation = FALSE;
                        }
                    }
#endif /* !(defined(VALID_TOOLS) && (defined(STVID_FRAME_BUFFER_DUMP) || defined(STVID_FB_ADD_DATA_DUMP))) */

#if defined(TRACE_UART) && defined(VALID_TOOLS)
                    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODER_JOB_END, "NewDecoded(%d-%d/%d) PTS(%c-%d/%d)\r\n",
                            VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                            VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                            VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                            ((VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PTS.Interpolated) ? 'I' : 'R'),
                            VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PTS.PTS,
                            VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PTS.PTS33));
#endif

                    PictureBuffer_p = NULL;
                    if (VIDPROD_Data_p->DecoderStatus.NbIndices > 0)
                    {
                        /* New picture decoded, update NextOutputIndex to prepare the next picture decoding */
                        STOS_SemaphoreWait(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);
                        ExpectedPictureOutputIndex = VIDPROD_Data_p->DecoderStatus.NextOutputIndex;
                        ExpectedPictureInDecodeOrder_p = VIDPROD_Data_p->DecoderStatus.CurrentPictures_p[VIDPROD_Data_p->DecoderStatus.NextOutputIndex];
                        /* Clear unused pointer for correct debug tools behavior */
                        VIDPROD_Data_p->DecoderStatus.CurrentPictures_p[VIDPROD_Data_p->DecoderStatus.NextOutputIndex] = NULL;
                        VIDPROD_Data_p->DecoderStatus.NbIndices--;
                        VIDPROD_Data_p->DecoderStatus.NextOutputIndex = (VIDPROD_Data_p->DecoderStatus.NextOutputIndex+1) % MAX_CURRENT_PICTURES;
                        STOS_SemaphoreSignal(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);

                        /* CL comment: here we check we have decoded the right picture */
                        /* to check it we compare the entries logged in tables in the parser callback and in the decoder callback, they must match */

                        /* Only temporary to prevent crashes as we have holes in Currentpictures_p table when switching back in forward */
                         if (ExpectedPictureInDecodeOrder_p != NULL)
                         {
#ifdef STVID_TRICKMODE_BACKWARD
                        /* For now, don't check decoded picture order versus VIDPROD_Data_p->DecoderStatus.CurrentPictures_p as this table is not updated by parser in the reverse order in backward today */
                        if ((!VIDPROD_Data_p->Backward.IsBackward) && (ExpectedPictureInDecodeOrder_p != VIDPROD_Data_p->ForTask.LastDecodedPicture_p))
#else
                        if (ExpectedPictureInDecodeOrder_p != VIDPROD_Data_p->ForTask.LastDecodedPicture_p)
#endif /* STVID_TRICKMODE_BACKWARD */
                        {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                            TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_ERRORS, "pDEC_JCErr5-%d(%d-%d/%d)(%d-%d/%d) ",
                                                        VIDPROD_Data_p->ForTask.NewPictureDecodeResults.CommandId,
                                                        VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                                        VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                                        VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                                                        ExpectedPictureInDecodeOrder_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                                        ExpectedPictureInDecodeOrder_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                                        ExpectedPictureInDecodeOrder_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));

#endif
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Producer : Picture is not the last in decoding FIFO ID=%d(%d-%d/%d) Last is (%d-%d/%d) %s !! (BUG-BUG-BUG) !!",
                                            VIDPROD_Data_p->ForTask.NewPictureDecodeResults.CommandId,
                                            VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                            VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                            VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                                            ExpectedPictureInDecodeOrder_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                            ExpectedPictureInDecodeOrder_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                            ExpectedPictureInDecodeOrder_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                                            (ExpectedPictureInDecodeOrder_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_EMPTY)               ? "EMPTY" :
                                            (ExpectedPictureInDecodeOrder_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_PARSED)              ? "PARSED" :
                                            (ExpectedPictureInDecodeOrder_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_WAITING_CONFIRM_EVT) ? "WAIT_CBEVT" :
                                            (ExpectedPictureInDecodeOrder_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_WAITING_FRAME_BUFFER)? "WAIT_FB" :
                                            (ExpectedPictureInDecodeOrder_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_DECODING)            ? "DECODING" :
                                            (ExpectedPictureInDecodeOrder_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_DECODED)             ? "DECODED" : "INVALID"
                                            ));

                            /* Remove the expected picture from decodepicture queue because we should have missed it */
                            if (ExpectedPictureInDecodeOrder_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_DECODING)
                            {
                                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Remove expected picture (%d-%d/%d) from Decodepicture table & status table",
                                            ExpectedPictureInDecodeOrder_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                            ExpectedPictureInDecodeOrder_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                            ExpectedPictureInDecodeOrder_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));

                                RemovePictureFromDecodePictureTable(ProducerHandle, ExpectedPictureInDecodeOrder_p, __LINE__);
                                /* Lock Access to Decode Pictures */
                                STOS_SemaphoreWait(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);
                                /* Clean this picture in DecoderStatus table */
                                vidprod_RemovePictureFromDecodeStatusTable(ProducerHandle, ExpectedPictureInDecodeOrder_p);
                                STOS_SemaphoreSignal(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);
                                if (ExpectedPictureInDecodeOrder_p->AssociatedDecimatedPicture_p != NULL)
                                {
                                    RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, ExpectedPictureInDecodeOrder_p->AssociatedDecimatedPicture_p, VIDCOM_VIDPROD_MODULE_BASE);
                                }
                                RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, ExpectedPictureInDecodeOrder_p, VIDCOM_VIDPROD_MODULE_BASE);
                            }
                            else
                            {
                                if (ExpectedPictureInDecodeOrder_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_WAITING_CONFIRM_EVT)
                                {
                                    vidprod_RemovePictureFromPreprocessingTable(ProducerHandle, ExpectedPictureInDecodeOrder_p);
                                }
                            }
                            /* Lock Access to Decode Pictures */
                            STOS_SemaphoreWait(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);
                            /* Move forward NextOutputIndex as the current first entry is now actually the picture we just get decoded */
                            VIDPROD_Data_p->DecoderStatus.NextOutputIndex = (VIDPROD_Data_p->DecoderStatus.NextOutputIndex+1) % MAX_CURRENT_PICTURES;
                            DECREMENT_NB_PICTURES_IN_QUEUE(ProducerHandle);
                            /* UnLock Access to Decode Pictures */
                            STOS_SemaphoreSignal(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);
                        } /* if (ExpectedPictureInDecodeOrder_p != VIDPROD_Data_p->ForTask.LastDecodedPicture_p) */
#ifndef STVID_TRICKMODE_BACKWARD
                        /* holes in Currentpictures_p table have been observed, this is expected because Currentpictures_p table is not currently properly managed in backward
                         * BUT for normal play (live, only FWD from memory or HDD) this situation is unexpected and must be debugged */
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unexpect missing picture index %d in Currentpictures_p table ", ExpectedPictureOutputIndex));
#endif
                        } /*ExpectedPictureInDecodeOrder_p != NULL*/


                        /* CL comment: */
                        /*  To update references we need information about the NEXT parsed picture which is going to be decoded
                        *  We look for this information in VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[] and then in pre processed pictures table
                        * */
                        PictureBuffer_p = NULL;
#ifdef STVID_TRICKMODE_BACKWARD
                        if ((VIDPROD_Data_p->DecoderStatus.NbIndices > 0) &&  (!VIDPROD_Data_p->Backward.IsBackward))
#else
                        if (VIDPROD_Data_p->DecoderStatus.NbIndices > 0)
#endif /* STVID_TRICKMODE_BACKWARD */
                        {
                            PictureBuffer_p = VIDPROD_Data_p->DecoderStatus.CurrentPictures_p[VIDPROD_Data_p->DecoderStatus.NextOutputIndex];
                        }
#ifdef STVID_TRICKMODE_BACKWARD
                        else if ((VIDPROD_Data_p->DecoderStatus.NbIndices > 0) && (VIDPROD_Data_p->Backward.IsBackward))
						{
                            PictureBuffer_p = VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[RememberIndex];
						}
#endif /* STVID_TRICKMODE_BACKWARD */
                        if (PictureBuffer_p == NULL)
                        {
                            /* No more parsed picture available, set a flag to perform UpdateReferenceList() on the next parsed picture event */
                            VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = TRUE;
                        }
                    } /* if (VIDPROD_Data_p->DecoderStatus.NbIndices > 0) */

                    if (VIDPROD_Data_p->ForTask.LastDecodedPicture_p->IsNeededForPresentation)
                    {
                        /* Insert picture in ordering queue as it has to be displayed */
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DISPLAY_INSERT, "pDispInsert(%d-%d/%d@%08x/%08x) ",
                                                    VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                                    VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                                    VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                                                    VIDPROD_Data_p->ForTask.LastDecodedPicture_p->FrameBuffer_p->Allocation.Address_p,
                                                    VIDPROD_Data_p->ForTask.LastDecodedPicture_p->FrameBuffer_p->Allocation.Address2_p));
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */
                        /* Lock Access to Decode Pictures */
                        STOS_SemaphoreWait(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);

#if COMPUTE_PRESENTATION_ID
                        if (VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.IsExtendedPresentationOrderIDValid)
                        {
							/* Resynchronize the CurrentPresentationOrderID with the current picture info */
                            VIDPROD_Data_p->CurrentPresentationOrderID = VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID;
						}
						else	/* ID is not known */
						{
                            if(VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.IsReference)
						    {
							    /* Store the current picture to compute the ID later */
                                VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->StoredPictureCounter] = VIDPROD_Data_p->ForTask.LastDecodedPicture_p;
                                /* TODO_CL : check with BC why it is still necessary to lock this picture here and now ... */
/*                                TAKE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->StoredPictureCounter], VIDCOM_VIDPROD_MODULE_BASE);*/

							    /* Calculate the previous position in the store picture array */
                                VIDPROD_Data_p->PreviousPictureCount = (VIDPROD_Data_p->StoredPictureCounter + STORED_PICTURE_BUFFER_SIZE - 1) % STORED_PICTURE_BUFFER_SIZE;
							    /* Increase the array counter to point to the next position in the array for the next picture */
                                VIDPROD_Data_p->StoredPictureCounter = (VIDPROD_Data_p->StoredPictureCounter + 1) % STORED_PICTURE_BUFFER_SIZE;


                                if (VIDPROD_Data_p->NumOfPictToWaitBeforePush > 0)
                                    VIDPROD_Data_p->NumOfPictToWaitBeforePush -= 1;

                                /* If it is not the first picture */
                                if (VIDPROD_Data_p->NumOfPictToWaitBeforePush == 0)
                                {
                                    if (!(VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->ParsedPictureInformation.PictureGenericData.IsExtendedPresentationOrderIDValid))
                                    {
                                        /* Assign the Previous picture the CurrentPresentationOrderID value and set the IsExtendedPresentationOrderIDValid to true */
                                        VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID = VIDPROD_Data_p->CurrentPresentationOrderID;
                                        VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->ParsedPictureInformation.PictureGenericData.IsExtendedPresentationOrderIDValid = TRUE;
                                    }
                                }
                            }
                            else
                            {
                                VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID = VIDPROD_Data_p->CurrentPresentationOrderID;
		                        VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.IsExtendedPresentationOrderIDValid = TRUE;
                            }
                        }
#endif	/* #if COMPUTE_PRESENTATION_ID */

                        RefusedInsertPicture_p = NULL;
#if COMPUTE_PRESENTATION_ID
                        if (VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.IsExtendedPresentationOrderIDValid)
                        {
#endif	/* #if COMPUTE_PRESENTATION_ID */
                        /* Insert current picture in Queue */
#ifdef STVID_TRICKMODE_BACKWARD
                        if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
                        {
                            ErrorCode = VIDQUEUE_InsertPictureInQueue(VIDPROD_Data_p->OrderingQueueHandle,
                                                                    VIDPROD_Data_p->ForTask.LastDecodedPicture_p,
                                                                    VIDQUEUE_INSERTION_ORDER_INCREASING);
                        }
#ifdef STVID_TRICKMODE_BACKWARD
                        else
                        {
                                ErrorCode = VIDQUEUE_InsertPictureInQueue(VIDPROD_Data_p->OrderingQueueHandle,
                                                                    VIDPROD_Data_p->ForTask.LastDecodedPicture_p,
                                                                    VIDQUEUE_INSERTION_ORDER_LAST_PLACE);
                        }
#endif /* STVID_TRICKMODE_BACKWARD */
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            RefusedInsertPicture_p = VIDPROD_Data_p->ForTask.LastDecodedPicture_p;
                            /* The picture hasn't been inserted, mark it as not needed for display to manage it properly in the reference list update process */
                            VIDPROD_Data_p->ForTask.LastDecodedPicture_p->IsNeededForPresentation = FALSE;
                            VIDPROD_Data_p->ForTask.LastDecodedPicture_p->IsPushed = TRUE;
                            if (VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p != NULL)
                            {
                                VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p->IsNeededForPresentation = FALSE;
                            }
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DISPLAY_INSERT, "pDispInsertRefused(%d-%d/%d) Err=%d",
                                                    VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                                    VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                                    VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                                                    ErrorCode));
#endif
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDQ_Insert refused Error (%d-%d/%d) error %d PB 0x%x %c",
                                    VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                    VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                    VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                                    ErrorCode,
                                    VIDPROD_Data_p->ForTask.LastDecodedPicture_p,
                                    MPEGFrame2Char(VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame)
                                    ));
                        }
#if COMPUTE_PRESENTATION_ID
                        } /* if (PictureGenericData.IsExtendedPresentationOrderIDValid) */
                        else
                        {
							/* if it is not the first picture */
                            if (VIDPROD_Data_p->NumOfPictToWaitBeforePush == 0)
                            {
                                /* Insert the previous picture in the Queue */
#ifdef STVID_TRICKMODE_BACKWARD
                                if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
                                {
                                    ErrorCode = VIDQUEUE_InsertPictureInQueue(VIDPROD_Data_p->OrderingQueueHandle,
                                                                          VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount],
                                                                          VIDQUEUE_INSERTION_ORDER_INCREASING);
                                }
#ifdef STVID_TRICKMODE_BACKWARD
                                else
                                {
                                    ErrorCode = VIDQUEUE_InsertPictureInQueue(VIDPROD_Data_p->OrderingQueueHandle,
                                                                            VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount],
                                                                            VIDQUEUE_INSERTION_ORDER_LAST_PLACE);
                                }
#endif /* STVID_TRICKMODE_BACKWARD */
                                if (ErrorCode != ST_NO_ERROR)
                                {
                                    RefusedInsertPicture_p = VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount];
                                    /* The picture hasn't been inserted, mark it as not needed for display to manage it properly in the reference list update process */
                                    VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->IsNeededForPresentation = FALSE;
                                    VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->IsPushed = TRUE;
                                    if (VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->AssociatedDecimatedPicture_p != NULL)
                                    {
                                        VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->AssociatedDecimatedPicture_p->IsNeededForPresentation = FALSE;
                                    }
                                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDQ_Insert refused Error (%d-%d/%d) error %d",
                                            VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                            VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                            VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                                            ErrorCode));
                                }
                            }
                        } /* else if (PictureGenericData.IsExtendedPresentationOrderIDValid) */
#endif	/* #if COMPUTE_PRESENTATION_ID */

                        /* UnLock Access to Decode Pictures */
                        STOS_SemaphoreSignal(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);

                        if (RefusedInsertPicture_p != NULL)
                        {
                            /* Ordering queue refused the insertion, we need to release the picture if it is not a reference */
                            if (!RefusedInsertPicture_p->IsReferenceForDecoding)
                            {
                                RemovePictureFromDecodePictureTable(ProducerHandle, RefusedInsertPicture_p, __LINE__);
                                /* We can release the non ref decimated picture if exists right now as it is has been pushed */
                                if (RefusedInsertPicture_p->AssociatedDecimatedPicture_p != NULL)
                                {
                                    RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, RefusedInsertPicture_p->AssociatedDecimatedPicture_p, VIDCOM_VIDPROD_MODULE_BASE);
                                }
                                RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, RefusedInsertPicture_p, VIDCOM_VIDPROD_MODULE_BASE);
                            }
                        }
                        else
                        {
                        if (VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.IsDisplayBoundPictureIDValid)
                        {
                            ErrorCode = VIDQUEUE_PushPicturesToDisplay(VIDPROD_Data_p->OrderingQueueHandle,
                                       &(VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.DisplayBoundPictureID));
#if defined(VALID_TOOLS) && defined(STVID_FRAME_BUFFER_DUMP)
                            sprintf(Filename_p, "../../decoded_frames/frame%03d_%03d_%03d_%03d",
                                    VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                    VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                    VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                                    VIDPROD_Data_p->ForTask.LastDecodedPicture_p->PictureNumber);
                            /* WARNING : If the avmem partition used is cached, the line below might not work properly.
                            * vidprod_SavePicture access to memory should be replaced with forced uncached accesses */
                            vidprod_SavePicture(OMEGA2_420, VIDPROD_Data_p->ForTask.LastDecodedPicture_p->GlobalDecodingContext.GlobalDecodingContextGenericData.SequenceInfo.Width,
                                    VIDPROD_Data_p->ForTask.LastDecodedPicture_p->GlobalDecodingContext.GlobalDecodingContextGenericData.SequenceInfo.Height,
                                    STAVMEM_DeviceToCPU(VIDPROD_Data_p->ForTask.LastDecodedPicture_p->FrameBuffer_p->Allocation.Address_p, &VIDPROD_Data_p->VirtualMapping) /* LumaAddress */,
                                    STAVMEM_DeviceToCPU(VIDPROD_Data_p->ForTask.LastDecodedPicture_p->FrameBuffer_p->Allocation.Address2_p, &VIDPROD_Data_p->VirtualMapping) /* ChromaAddress */,
                                    Filename_p);
#endif /* defined(VALID_TOOLS) && defined(STVID_FRAME_BUFFER_DUMP) */
#if defined(VALID_TOOLS) && defined(STVID_FB_ADD_DATA_DUMP)
                            if(VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.MPEGStandard == STVID_MPEG_STANDARD_ISO_IEC_14496)
                            {
                                sprintf(Filename_p, "../../decoded_frames/add_data%03d_%03d_%03d_%03d",
                                        VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                        VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                        VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                                        VIDPROD_Data_p->ForTask.LastDecodedPicture_p->PictureNumber);
                            /* WARNING : If the avmem partition used is cached, the line below might not work properly.
                             * vidprod_SavePicture access to memory should be replaced with forced uncached accesses */
                                vidprod_SaveAddData(VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.ProfileAndLevelIndication,
                                        VIDPROD_Data_p->ForTask.LastDecodedPicture_p->GlobalDecodingContext.GlobalDecodingContextGenericData.SequenceInfo.Width,
                                        VIDPROD_Data_p->ForTask.LastDecodedPicture_p->GlobalDecodingContext.GlobalDecodingContextGenericData.SequenceInfo.Height,
                                        STAVMEM_DeviceToCPU(VIDPROD_Data_p->ForTask.LastDecodedPicture_p->PPB.Address_p, &VIDPROD_Data_p->VirtualMapping) /* AdditionnalDataAddress */,
                                        Filename_p);
                            }
#endif /* defined(VALID_TOOLS) && defined(STVID_FB_ADD_DATA_DUMP) */
#if defined(VALID_TOOLS) && defined(STVID_DECIMATED_FRAME_BUFFER_DUMP)
/*To Add Check Decimation mode*/
                            if(VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p != NULL)
                            {
                                if(VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.MPEGStandard == STVID_MPEG_STANDARD_ISO_IEC_14496)
                                {
                                    sprintf(Filename_p, "../../decoded_frames/decim_frame%03d_%03d_%03d_%03d",
                                            VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                            VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                            VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                                            VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p->PictureNumber);
                                switch (VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p->FrameBuffer_p->DecimationFactor)
                                {
                                case STVID_DECIMATION_FACTOR_NONE :
                                    H_DecimationFactor = 0;
                                    V_DecimationFactor = 0;
                                    break;
                                case STVID_DECIMATION_FACTOR_H2 :
                                    H_DecimationFactor = 2;
                                    V_DecimationFactor = 0;
                                    break;
                                case STVID_DECIMATION_FACTOR_V2 :
                                    H_DecimationFactor = 0;
                                    V_DecimationFactor = 2;
                                    break;
                                case STVID_DECIMATION_FACTOR_H4 :
                                    H_DecimationFactor = 4;
                                    V_DecimationFactor = 0;
                                    break;
                                case STVID_DECIMATION_FACTOR_V4 :
                                    H_DecimationFactor = 0;
                                    V_DecimationFactor = 4;
                                    break;
                                case STVID_DECIMATION_FACTOR_2 :
                                    H_DecimationFactor = 2;
                                    V_DecimationFactor = 2;
                                    break;
                                case STVID_DECIMATION_FACTOR_4 :
                                    H_DecimationFactor = 4;
                                    V_DecimationFactor = 4;
                                    break;
                                }
                                vidprod_SavePicture(OMEGA2_420, VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p->GlobalDecodingContext.GlobalDecodingContextGenericData.SequenceInfo.Width/H_DecimationFactor,
                                            VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p->GlobalDecodingContext.GlobalDecodingContextGenericData.SequenceInfo.Height/V_DecimationFactor,
                                            STAVMEM_DeviceToCPU(VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Address_p, &VIDPROD_Data_p->VirtualMapping) /* LumaAddress */,
                                            STAVMEM_DeviceToCPU(VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Address2_p, &VIDPROD_Data_p->VirtualMapping) /* ChromaAddress */,
                                            Filename_p);
                                }
                            } /* if(VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p != NULL) */
#endif /* defined(VALID_TOOLS) && defined(STVID_DECIMATED_FRAME_BUFFER_DUMP) */
                            /* As picture is pushed to the display queue it is considered as already presented */
                            VIDPROD_Data_p->ForTask.LastDecodedPicture_p->IsNeededForPresentation = FALSE;
                            if (VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p != NULL)
                            {
                                VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p->IsNeededForPresentation = FALSE;
                            }

                            /* We can release and remove from the DecodePicture table the picture if it is a non reference picture as it has been pushed */
                            if (!VIDPROD_Data_p->ForTask.LastDecodedPicture_p->IsReferenceForDecoding)
                            {

                                RemovePictureFromDecodePictureTable(ProducerHandle, VIDPROD_Data_p->ForTask.LastDecodedPicture_p, __LINE__);
                                /* We can release the non ref decimated picture if exists right now as it is has been pushed */
                                if (VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p != NULL)
                                {
                                    RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p, VIDCOM_VIDPROD_MODULE_BASE);
                                }
                                RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, VIDPROD_Data_p->ForTask.LastDecodedPicture_p, VIDCOM_VIDPROD_MODULE_BASE);
                            }

                        } /* end if VIDPROD_Data_p->ForTask.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.IsDisplayBoundPictureIDValid */
                        } /* if (RefusedInsertPicture_p != NULL) */
                        if (ErrorCode == ST_NO_ERROR)
                        {
                            /* If all P&B pictures are skipped, there is no more need for reordering as I pictures shall be displayed in their decode order for all codecs */
                            if (IsIOnlyDecodeOnGoing(VIDPROD_Data_p->Skip))
                            {
                                BumpPictureToDisplayQueue(ProducerHandle);
                                vidprod_ReleasePushedNonRefPictures(ProducerHandle);
                            }
                        }
#if COMPUTE_PRESENTATION_ID
                        if (VIDPROD_Data_p->NumOfPictToWaitBeforePush == 0)
                        {
                            /* Increment CurrentPresentationOrderID and take care of rollover */
                            /* The value of signed integer ranges from -2147483648 to 2147483647 */
                            if (VIDPROD_Data_p->CurrentPresentationOrderID.ID == 0x7FFFFFFF)
                            {
                                VIDPROD_Data_p->CurrentPresentationOrderID.ID = 0x80000000;	/* minimum value -2147483648 */
                                if (VIDPROD_Data_p->CurrentPresentationOrderID.IDExtension == 0xFFFFFFFF)
                                {
                                    VIDPROD_Data_p->CurrentPresentationOrderID.IDExtension = 0;
                                }
                                else
                                {
                                    VIDPROD_Data_p->CurrentPresentationOrderID.IDExtension++;
                                }
                            }
                            else
                            {
                                VIDPROD_Data_p->CurrentPresentationOrderID.ID++;
                            }
                        }
#endif	/* #if COMPUTE_PRESENTATION_ID */
                    } /* end of VIDPROD_Data_p->ForTask.LastDecodedPicture_p->IsNeededForPresentation */
                    else
                    {
                        /* We can release and remove from the DecodePicture table the picture if it is a non reference picture as it is not sent to the display */
                        if (!VIDPROD_Data_p->ForTask.LastDecodedPicture_p->IsReferenceForDecoding)
                        {

                            RemovePictureFromDecodePictureTable(ProducerHandle, VIDPROD_Data_p->ForTask.LastDecodedPicture_p, __LINE__);
                            if (VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p != NULL)
                            {
                                RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, VIDPROD_Data_p->ForTask.LastDecodedPicture_p->AssociatedDecimatedPicture_p, VIDCOM_VIDPROD_MODULE_BASE);
                            }
                            RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, VIDPROD_Data_p->ForTask.LastDecodedPicture_p, VIDCOM_VIDPROD_MODULE_BASE);
                        }
                    }

                    if (PictureBuffer_p != NULL)
                    {
                        UpdateReferenceList(ProducerHandle, PictureBuffer_p);
                    }
                    else
                    if (VIDPROD_Data_p->DecoderStatus.NbIndices > 0)
                    {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_ERRORS, "pDEC_JCWrn1-%d", VIDPROD_Data_p->ForTask.NewPictureDecodeResults.CommandId));
#endif
                        /* To update references we need information about the next parsed picture which is going to be decoded */
                        /* but we didn't find it ! */
                        STTBX_Report((STTBX_REPORT_LEVEL_WARNING, "Module Producer CONTROLLER_COMMAND_NEW_DECODED_PICTURE : No next picture in decoding FIFO ID=%d !! ", VIDPROD_Data_p->ForTask.NewPictureDecodeResults.CommandId));
                    }
                    VIDPROD_Data_p->ForTask.LastDecodedPicture_p = NULL;
                }
                else
                {
                    if ((VIDPROD_Data_p->ProducerState != VIDPROD_DECODER_STATE_STOPPED) && (!VIDPROD_Data_p->Stop.IsPending))
                    {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_ERRORS, "pDEC_JCErr2-%d", VIDPROD_Data_p->ForTask.NewPictureDecodeResults.CommandId));
#endif
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Producer CONTROLLER_COMMAND_NEW_DECODED_PICTURE : Picture not found ID=%d!!", VIDPROD_Data_p->ForTask.NewPictureDecodeResults.CommandId));
                }
                    else
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Module Producer CONTROLLER_COMMAND_NEW_DECODED_PICTURE : Picture removed by stop for job ID=%d", VIDPROD_Data_p->ForTask.NewPictureDecodeResults.CommandId));
                    }
                }
                vidprod_TryToPushPictureToDisplay(ProducerHandle);
                /* DecodeOnce : pause or freeze after first picture is passed to display. */
                if (VIDPROD_Data_p->DecodeOnce)
                {
                    VIDPROD_Data_p->DecodeOnce = FALSE;
                    BumpPictureToDisplayQueue(ProducerHandle);
                    vidprod_ReleasePushedNonRefPictures(ProducerHandle);
                    STEVT_Notify(VIDPROD_Data_p->EventsHandle, VIDPROD_Data_p->RegisteredEventsID[VIDPROD_DECODE_ONCE_READY_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
                }

                break;
            default :
                break;
        } /* end switch(command) */
    } /* end while(controller command) */
    /* Do actions at each loop, not depending on orders (comes here at least each MAX_WAIT_ORDER_TIME ticks */

    /* Decode start condition check */
    if (!VIDPROD_Data_p->StartCondition.IsDecodeEnable)
    {
    vidprod_DecodeStartConditionCheck(ProducerHandle);
    }

    /* Push pictures to display as soon as possible */
    /*vidprod_TryToPushPictureToDisplay(ProducerHandle);*/
/* NOTE_PC : Simplified Confirm Buffer */

#ifdef STVID_TRICKMODE_BACKWARD
    if (((VIDPROD_Data_p->PreprocessingPicture.NbUsedPictureBuffers > 0) && (!VIDPROD_Data_p->Stop.IsPending) && (!VIDPROD_Data_p->Skip.RecoveringNextEntryPoint) && (!VIDPROD_Data_p->ForTask.SoftResetPending)
            && (!VIDPROD_Data_p->Backward.IsBackward))
            ||
        ((VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p != NULL) && (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.NbUsedPictureBuffers > 0)
            && (!VIDPROD_Data_p->Skip.RecoveringNextEntryPoint) && (VIDPROD_Data_p->Backward.IsBackward) && (!VIDPROD_Data_p->Stop.IsPending)))
#else
    if ((VIDPROD_Data_p->PreprocessingPicture.NbUsedPictureBuffers > 0) && (!VIDPROD_Data_p->Stop.IsPending) && (!VIDPROD_Data_p->Skip.RecoveringNextEntryPoint) && (!VIDPROD_Data_p->ForTask.SoftResetPending))
#endif /* STVID_TRICKMODE_BACKWARD */

    {
        vidprod_SearchAndConfirmBuffer(ProducerHandle, TOut_ClockTicks);
    }

/* NOTE_PC : Simplified decode launch */
#ifdef STVID_TRICKMODE_BACKWARD
    if (((VIDPROD_Data_p->PreprocessingPicture.NbUsedPictureBuffers > 0) && (!VIDPROD_Data_p->Stop.IsPending) && (!VIDPROD_Data_p->Skip.RecoveringNextEntryPoint) && (!VIDPROD_Data_p->ForTask.SoftResetPending)
            && (VIDPROD_Data_p->NextAction.LaunchDecodeWhenPossible != 0) && (VIDPROD_Data_p->StartCondition.IsDecodeEnable) && (!VIDPROD_Data_p->Backward.IsBackward))
        ||
        ((VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p != NULL) && (!VIDPROD_Data_p->Stop.IsPending) && (!VIDPROD_Data_p->Backward.LastPictureDecoded)
        && (!VIDPROD_Data_p->Skip.RecoveringNextEntryPoint) && (VIDPROD_Data_p->StartCondition.IsDecodeEnable) && (VIDPROD_Data_p->Backward.IsBackward)
		&& (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.NbParsedPicturesLeft != 0)))
#else
    if ((VIDPROD_Data_p->PreprocessingPicture.NbUsedPictureBuffers > 0) && (!VIDPROD_Data_p->Stop.IsPending) && (!VIDPROD_Data_p->Skip.RecoveringNextEntryPoint) && (!VIDPROD_Data_p->ForTask.SoftResetPending) && (VIDPROD_Data_p->NextAction.LaunchDecodeWhenPossible != 0) && (VIDPROD_Data_p->StartCondition.IsDecodeEnable))
#endif /* STVID_TRICKMODE_BACKWARD */
    {
        vidprod_LaunchDecode(ProducerHandle);
    }
#ifdef STVID_TRICKMODE_BACKWARD
    if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p != NULL)
    {
        if ((VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p == NULL)
        && (VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.BitBufferFullyParsed)
        && (VIDPROD_Data_p->Backward.ChangingDirection != TRICKMODE_CHANGING_FORWARD)
        && (!VIDPROD_Data_p->DataUnderflowEventNotSatisfied))
        {
            VIDPROD_Data_p->Backward.InjectDiscontinuityCanBeAccepted = FALSE;
            VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p = VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p;
            VIDPROD_Data_p->Backward.NbDecodedPictures = 0;
            VIDPROD_Data_p->Backward.LastPictureDecoded = FALSE;
            VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->IsFirstPictureDecoded = FALSE;
            if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->NbIPicturesInBuffer != 0)
            {
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->AverageNbPicturesBetweenTwoI =
                (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->NbIPicturesInBuffer +
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->NbNotIPicturesInBuffer) / VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->NbIPicturesInBuffer;
            }
            else
            {
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->AverageNbPicturesBetweenTwoI = VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->NbNotIPicturesInBuffer;
            }
            if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->NbIPicturesInBuffer > MAX_BACKWARD_PICTURES)
            {
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->NbIPicturesInBuffer = MAX_BACKWARD_PICTURES;
            }

            vidprod_AskForLinearBitBufferFlush(ProducerHandle,
                                VIDPROD_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p,
                                VIDPROD_FLUSH_BBFSIZE_BACKWARD_JUMP);
			VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue = 0;
            VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->FirstIPictureInBuffer_p = &(VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[0]);
            VIDPROD_Data_p->Backward.InjectionTime = (U32)(time_minus(time_now(), VIDPROD_Data_p->Backward.InjectionStartTime)*OSCLOCK_T_MILLION/TOut_ClockTicks);
            VIDPROD_Data_p->Backward.InjectionStartTime = time_now();
			STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Free buffer under parsing %d \r\n", VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->NbIPicturesInBuffer));
#if defined(TRACE_UART) && !defined(VALID_TOOLS)
            TraceBuffer(("Free buffer under parsing %d \r\n", VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->NbIPicturesInBuffer));
#endif
        }
    }
#endif /* STVID_TRICKMODE_BACKWARD */

/* NOTE_PC : Simplified parser launch */
#ifdef STVID_TRICKMODE_BACKWARD
    if ((VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p != NULL) && (!VIDPROD_Data_p->DataUnderflowEventNotSatisfied))
    {
       if ((VIDPROD_Data_p->ProducerState == VIDPROD_DECODER_STATE_DECODING) &&
       (!(VIDPROD_Data_p->ForTask.SoftResetPending)) &&
       (!(VIDPROD_Data_p->Stop.IsPending)) &&
       /*(VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue < MAX_PICTURES_IN_DECODE_PIPE) &&*/
       (!VIDPROD_Data_p->ParserStatus.IsSearchingPicture) /*&&
       (VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.NbUsedPictureBuffers < MAX_BACKWARD_PICTURES)*/
       && (!VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.BitBufferFullyParsed))
        {
            /* If producer is running and MAX_PICTURES_IN_DECODE_PIPE not reached, look for next picture */
            PushControllerCommand(ProducerHandle, CONTROLLER_COMMAND_SEARCH_PICTURE);
            STOS_SemaphoreSignal(VIDPROD_Data_p->ProducerOrder_p);
        }
	}
    else if (!VIDPROD_Data_p->Backward.IsBackward)
    {
#endif /* STVID_TRICKMODE_BACKWARD */
    if ((VIDPROD_Data_p->ProducerState == VIDPROD_DECODER_STATE_DECODING) &&
       (!(VIDPROD_Data_p->ForTask.SoftResetPending)) &&
       (!(VIDPROD_Data_p->Stop.IsPending)) &&
       (VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue < MAX_PICTURES_IN_DECODE_PIPE) &&
       (VIDPROD_Data_p->PreprocessingPicture.NbUsedPictureBuffers < MAX_NUMBER_OF_PREPROCESSING_PICTURES) &&
       (!VIDPROD_Data_p->ParserStatus.IsSearchingPicture))
    {
        /* If producer is running and MAX_PICTURES_IN_DECODE_PIPE not reached, look for next picture */
        PushControllerCommand(ProducerHandle, CONTROLLER_COMMAND_SEARCH_PICTURE);
        STOS_SemaphoreSignal(VIDPROD_Data_p->ProducerOrder_p);
    }
#ifdef STVID_TRICKMODE_BACKWARD
    }

/* TO_DO RHA do sure that  NextBufferToBeTransfered_p is available*/
	if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p != NULL)
	{
        if ((VIDPROD_Data_p->Backward.NbDecodedPictures == VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->NbIPicturesInBuffer) ||
           ((VIDPROD_Data_p->Skip.AllExceptFirstI) && (VIDPROD_Data_p->Backward.NbDecodedPictures == 1)) )
		{
            VIDPROD_Data_p->Backward.LastPictureDecoded = TRUE;
		}
        if ((VIDPROD_Data_p->Backward.IsBackward) && ( (/*(VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue == 0) &&*/ (VIDPROD_Data_p->Backward.LastPictureDecoded) ) ||
		   	(VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->NbIPicturesInBuffer ==  0)) &&
                (VIDPROD_Data_p->Backward.ChangingDirection != TRICKMODE_CHANGING_FORWARD))
        {
            VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.NbUsedPictureBuffers = 0;
        	VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.NbParsedPicturesLeft = 0;
        	VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.BitBufferFullyParsed = FALSE;
        	for(LoopCounter = 0 ; LoopCounter < MAX_BACKWARD_PICTURES ; LoopCounter++)
        	{
                	VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_EMPTY;
                	VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.IsPictureBufferUsed[LoopCounter] = FALSE;
                	VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.IsPictureTimedOut[LoopCounter] = FALSE;
        	}
        	VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p = NULL;
            VIDPROD_Data_p->Backward.LastPictureDecoded = FALSE;
            vidprod_ReleaseNonPushedNonRefPictures(ProducerHandle);
			ResetReferenceList(ProducerHandle);
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Free buffer under decode \r\n"));
            VIDPROD_Data_p->Backward.BufferDecodeTime = (U32)(time_minus(time_now(), VIDPROD_Data_p->Backward.DecodeStartTime)*OSCLOCK_T_MILLION/TOut_ClockTicks);
            VIDPROD_Data_p->Backward.DecodeStartTime = time_now();
#if defined(TRACE_UART) && !defined(VALID_TOOLS)
            TraceBuffer(("Free buffer under decode \r\n"));
#endif
            if (!VIDPROD_Data_p->Backward.IsPreviousInjectionFine)
            {
                vidprod_AskForLinearBitBufferFlush(ProducerHandle,
                                VIDPROD_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p,
                                VIDPROD_FLUSH_BBFSIZE_BACKWARD_JUMP);
                VIDPROD_Data_p->Backward.IsPreviousInjectionFine = TRUE;
                VIDPROD_Data_p->Backward.IsInjectionNeeded = TRUE;
            }
    	}
	}
#endif  /* STVID_TRICKMODE_BACKWARD */

    /* Stop is requested, wait for the completion of all pending decode jobs prior to put the producer in STOPPED mode */
    if ((VIDPROD_Data_p->Stop.IsPending) || (VIDPROD_Data_p->Skip.RecoveringNextEntryPoint) || (VIDPROD_Data_p->ForTask.SoftResetPending))
    {
        if (VIDPROD_Data_p->Skip.RecoveringNextEntryPoint)
        {
            /* Notification of this new entry point research to advise outside world. */
            STEVT_Notify(VIDPROD_Data_p->EventsHandle, VIDPROD_Data_p->RegisteredEventsID[VIDPROD_RESTARTED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
        }

        /* look for pre processing picture currently in decode */
        LoopCounter = 0;
        ThereIsPendingDecode = FALSE;
        FoundDecodeToAbort = FALSE;
#ifdef STVID_TRICKMODE_BACKWARD
        if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
        {
        while (LoopCounter < MAX_NUMBER_OF_PREPROCESSING_PICTURES)
        {
            if (VIDPROD_Data_p->PreprocessingPicture.IsPictureBufferUsed[LoopCounter])
            {
                switch (VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.PictureDecodingStatus)
                {
                    case VIDBUFF_PICTURE_BUFFER_DECODING :
                        /* This case should not happen */
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "A picture under preprocessing stage is marked as DECODING %d (%d-%d/%d)!!!(BUG-BUG-BUG)",
                                VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId,
                                VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID
                                ));
                        break;
                    case VIDBUFF_PICTURE_BUFFER_WAITING_CONFIRM_EVT :
                        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Abort CmdID %d (%d-%d/%d) ending@ 0x%x while waiting confirm event due to \r\n\tStop.IsPending=%d Skip.RecoveringNextEntryPoint=%d SoftResetPending=%d",
                            VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId,
                            VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                            VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                            VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                            VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.BitBufferPictureStopAddress,
                            VIDPROD_Data_p->Stop.IsPending, VIDPROD_Data_p->Skip.RecoveringNextEntryPoint, VIDPROD_Data_p->ForTask.SoftResetPending
                            ));
                        ErrorCode = DECODER_Abort(VIDPROD_Data_p->DecoderHandle, VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            /* NOTE_CL: unexpected error, decoder internal data failure or bad producer data handling */
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER_Abort rejected ID=%d!!", VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId));
                        }
                        if (!FoundDecodeToAbort)
                        {
                            FoundDecodeToAbort = TRUE;
                            HighestDecodeOrderIDFound = VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId;
                            InformReadAddressParams.DecoderReadAddress_p = VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.BitBufferPictureStopAddress;
                        }
                        else
                        {
                            if (VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId > HighestDecodeOrderIDFound)
                            {
                                HighestDecodeOrderIDFound = VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId;
                                InformReadAddressParams.DecoderReadAddress_p = VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.BitBufferPictureStopAddress;
                            }
                        }
                        vidprod_RemovePictureFromPreprocessingTable(ProducerHandle, &(VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter]));
                        /* This decode is aborted, decrease VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue */
                        DECREMENT_NB_PICTURES_IN_QUEUE(ProducerHandle);
                        break;
                    case VIDBUFF_PICTURE_BUFFER_WAITING_FRAME_BUFFER :
                        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Abort CmdID %d (%d-%d/%d) ending@ 0x%x while waiting Frame buffer due to \r\n\tStop.IsPending=%d Skip.RecoveringNextEntryPoint=%d SoftResetPending=%d",
                            VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId,
                            VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                            VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                            VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                            VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.BitBufferPictureStopAddress,
                            VIDPROD_Data_p->Stop.IsPending, VIDPROD_Data_p->Skip.RecoveringNextEntryPoint, VIDPROD_Data_p->ForTask.SoftResetPending
                            ));
                        ErrorCode = DECODER_Abort(VIDPROD_Data_p->DecoderHandle, VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            /* NOTE_CL: unexpected error, decoder internal data failure or bad producer data handling */
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER_Abort rejected ID=%d!!", VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId));
                        }

                        if (VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_WAITING_FRAME_BUFFER)
                        {
                            if (VIDPROD_Data_p->NextAction.ConfirmBufferWhenPossible > 0)
                            {
                                VIDPROD_Data_p->NextAction.ConfirmBufferWhenPossible--;
                            }
                            else
                            {
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ConfirmBufferWhenPossible was already == 0 !!! "));
                            }
                            VIDPROD_Data_p->ForTask.PictureToConfirm_p = NULL;
                        }

                        if (!FoundDecodeToAbort)
                        {
                            FoundDecodeToAbort = TRUE;
                            HighestDecodeOrderIDFound = VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId;
                            InformReadAddressParams.DecoderReadAddress_p = VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.BitBufferPictureStopAddress;
                        }
                        else
                        {
                            if (VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId > HighestDecodeOrderIDFound)
                            {
                                HighestDecodeOrderIDFound = VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId;
                                InformReadAddressParams.DecoderReadAddress_p = VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.BitBufferPictureStopAddress;
                            }
                        }

                        vidprod_RemovePictureFromPreprocessingTable(ProducerHandle, &(VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter]));
                        /* This decode is aborted, decrease VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue */
                        DECREMENT_NB_PICTURES_IN_QUEUE(ProducerHandle);
                        break;
                    default:
                        /* Nothing to do */
                        break;
                }
            }
            LoopCounter++;
        }

        }
#ifdef STVID_TRICKMODE_BACKWARD
        else if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p != NULL)  /* is backward */
        {
            while (LoopCounter < MAX_BACKWARD_PICTURES)
            {
                if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.IsPictureBufferUsed[LoopCounter])
                {
                    switch (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.PictureDecodingStatus)
                    {
                        case VIDBUFF_PICTURE_BUFFER_DECODING :
                            /* This case should not happen */
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "A picture under preprocessing stage is marked as DECODING %d (%d-%d/%d)!!!(BUG-BUG-BUG)",
                                    VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId,
                                    VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                    VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                    VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID
                                    ));
                            break;

						case VIDBUFF_PICTURE_BUFFER_PARSED :
                            vidprod_RemovePictureFromPreprocessingTableBackward(ProducerHandle, &(VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter]));
                            if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->NbIPicturesInBuffer != 0)
                            {
                                VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->NbIPicturesInBuffer --;
                            }
							break;
                        case VIDBUFF_PICTURE_BUFFER_WAITING_CONFIRM_EVT :
                            VIDPROD_Data_p->Backward.NbDecodedPictures ++;
                            ErrorCode = DECODER_Abort(VIDPROD_Data_p->DecoderHandle, VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId);
                            if (ErrorCode != ST_NO_ERROR)
                            {
                                /* NOTE_CL: unexpected error, decoder internal data failure or bad producer data handling */
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER_Abort rejected ID=%d!!", VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId));
                            }
                            /* NOTE_CL no call to PARSER_InformReadAddress() in backward */
                            vidprod_RemovePictureFromPreprocessingTableBackward(ProducerHandle, &(VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter]));
                            /* This decode is aborted, decrease VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue */
                            DECREMENT_NB_PICTURES_IN_QUEUE(ProducerHandle);
                            break;
                        case VIDBUFF_PICTURE_BUFFER_WAITING_FRAME_BUFFER :
                            VIDPROD_Data_p->Backward.NbDecodedPictures ++;
                            ErrorCode = DECODER_Abort(VIDPROD_Data_p->DecoderHandle, VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId);
                            if (ErrorCode != ST_NO_ERROR)
                            {
                                /* NOTE_CL: unexpected error, decoder internal data failure or bad producer data handling */
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER_Abort rejected ID=%d!!", VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId));
                            }
                            if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_WAITING_FRAME_BUFFER)
                            {
                                if (VIDPROD_Data_p->NextAction.ConfirmBufferWhenPossible > 0)
                                {
                                    VIDPROD_Data_p->NextAction.ConfirmBufferWhenPossible--;
                                }
                                else
                                {
                                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ConfirmBufferWhenPossible was already == 0 !!! "));
                                }
                                VIDPROD_Data_p->ForTask.PictureToConfirm_p = NULL;
                            }
                            vidprod_RemovePictureFromPreprocessingTableBackward(ProducerHandle, &(VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter]));
                            /* This decode is aborted, decrease VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue */
                            DECREMENT_NB_PICTURES_IN_QUEUE(ProducerHandle);
                            break;
                        default:
                            /* Nothing to do */
                            break;
                    }
                }
                LoopCounter++;
            }
        }
#endif /* STVID_TRICKMODE_BACKWARD */
        LoopCounter = 0;
        while ( (!ThereIsPendingDecode) && (LoopCounter < MAX_NUMBER_OF_DECODE_PICTURES))
        {
            if (VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter] != NULL)
            {
                switch (VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->Decode.PictureDecodingStatus)
                {
                    case VIDBUFF_PICTURE_BUFFER_DECODING :
                        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "%sdecode pending ID=%d (%d-%d/%d) ending@ 0x%x",
                                    (VIDPROD_Data_p->DecodePicture.IsPictureTimedOut[LoopCounter] ? "TimedOUT " : ""),
                                    VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->Decode.CommandId,
                                    VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                    VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                    VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                                    VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->ParsedPictureInformation.PictureGenericData.BitBufferPictureStopAddress
                                     ));
                        if (!VIDPROD_Data_p->DecodePicture.IsPictureTimedOut[LoopCounter])
                        {
                            ThereIsPendingDecode = TRUE;
                        }
                        break;
                    case VIDBUFF_PICTURE_BUFFER_WAITING_CONFIRM_EVT :
                    case VIDBUFF_PICTURE_BUFFER_WAITING_FRAME_BUFFER :
                        /* This case should not happen */
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "A picture is still waiting for Frame Buffer confirmation JobID=%d, (BUG-BUG-BUG)",
                                VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->Decode.CommandId));
                        break;
                    default:
                        /* Nothing to do */
                        break;
                }
            }
            LoopCounter++;
        }

        if (!ThereIsPendingDecode)
        {
            if ((VIDPROD_Data_p->Skip.RecoveringNextEntryPoint) && (!VIDPROD_Data_p->Stop.IsPending))
            {
                /*  NOTE_CL: Don't call PARSER_InformReadAddress() here to avoid inappropriate bit buffer level modifications */
                VIDQUEUE_CancelAllPictures(VIDPROD_Data_p->OrderingQueueHandle);
                vidprod_ReleaseNonPushedNonRefPictures(ProducerHandle);
                ResetReferenceList(ProducerHandle);
#if COMPUTE_PRESENTATION_ID
                for (LoopCounter = 0 ; LoopCounter < STORED_PICTURE_BUFFER_SIZE ; LoopCounter++)
                {
                    VIDPROD_Data_p->StoredPictureBuffer[LoopCounter] = NULL;
                }
                VIDPROD_Data_p->StoredPictureCounter = 0;
                VIDPROD_Data_p->NumOfPictToWaitBeforePush = 2;
                VIDPROD_Data_p->PreviousPictureCount = 0;
#endif /* COMPUTE_PRESENTATION_ID */
                /* Allow to make bitbuffer's read pointer updated automatically in PARSER until a valid picture has been found
                 * (this must be set only when there is no decode performing else there is risk of data corruption for current decoding picture */
                VIDPROD_Data_p->Skip.RecoveringNextEntryPoint = FALSE;
#ifdef STVID_TRICKMODE_BACKWARD
                if (VIDPROD_Data_p->Backward.ChangingDirection == TRICKMODE_CHANGING_BACKWARD)
                {
                    VIDPROD_Data_p->Backward.WaitingProducerReadyToSwitchBackward = TRUE;
                }
                else if (VIDPROD_Data_p->Backward.ChangingDirection == TRICKMODE_CHANGING_FORWARD)
                {
                    VIDPROD_Data_p->Backward.WaitingProducerReadyToSwitchForward = TRUE;
                }
#endif /* STVID_TRICKMODE_BACKWARD */
            }

            /* there is no more picture decode pending in the decode engine the producer can stop, skip to the next entry point or reset */
            if (VIDPROD_Data_p->Stop.IsPending)
            {
                /* Dont send event if stop request is STOP_NOW */
                if ((VIDPROD_Data_p->Stop.WhenNextReferencePicture) ||
                    (VIDPROD_Data_p->Stop.WhenNoData) ||
                    (VIDPROD_Data_p->Stop.WhenNextIPicture))
                {
                    DoStopNow(ProducerHandle, TRUE);
                }
                else
                {
                    DoStopNow(ProducerHandle, FALSE);
                }
            }
            else
            if (   (VIDPROD_Data_p->ForTask.SoftResetPending)
                && (!VIDPROD_Data_p->ForTask.SoftResetPosted))
            {
                /* Notification of this internal reset to advise outside world. */
                STEVT_Notify(VIDPROD_Data_p->EventsHandle, VIDPROD_Data_p->RegisteredEventsID[VIDPROD_RESTARTED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);

                /* Flush all pictures locked in the ordering queue */
                VIDQUEUE_CancelAllPictures(VIDPROD_Data_p->OrderingQueueHandle);
                vidprod_ReleaseNonPushedNonRefPictures(ProducerHandle);
                ResetReferenceList(ProducerHandle);
#if COMPUTE_PRESENTATION_ID
                for (LoopCounter = 0 ; LoopCounter < STORED_PICTURE_BUFFER_SIZE ; LoopCounter++)
                {
                    VIDPROD_Data_p->StoredPictureBuffer[LoopCounter] = NULL;
                }
                VIDPROD_Data_p->StoredPictureCounter = 0;
                VIDPROD_Data_p->NumOfPictToWaitBeforePush = 2;
                VIDPROD_Data_p->PreviousPictureCount = 0;
#endif /* COMPUTE_PRESENTATION_ID */
                VIDPROD_Data_p->ForTask.SoftResetPosted = TRUE;
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "SoftReset is pending, post CONTROLLER_COMMAND_RESET now"));
                PushControllerCommand(ProducerHandle, CONTROLLER_COMMAND_RESET);
                /* Signal controller that a new command is given */
                STOS_SemaphoreSignal(VIDPROD_Data_p->ProducerOrder_p);
            }
        } /* end of if (ThereIsPendingDecode) */

    }
#ifdef STVID_TRICKMODE_BACKWARD
    if ((VIDPROD_Data_p->Backward.WaitingProducerReadyToSwitchBackward) && (!VIDPROD_Data_p->DataUnderflowEventNotSatisfied))
    {
        VIDPROD_Data_p->Backward.InjectDiscontinuityCanBeAccepted = FALSE;
        VIDPROD_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p = &VIDPROD_Data_p->LinearBitBuffers.BufferA;
        VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p = NULL;
        VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p = NULL;
        VIDPROD_Data_p->Backward.InjectionStartTime = time_now();
        VIDPROD_Data_p->Backward.DecodeStartTime = time_now();
        DECODER_SetDecodingDirection(VIDPROD_Data_p->DecoderHandle, DECODING_BACKWARD);
        vidprod_AskForLinearBitBufferFlush(ProducerHandle,
                            VIDPROD_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p,
                            VIDPROD_FLUSH_BBFSIZE_BACKWARD_JUMP);
        VIDPROD_Data_p->Backward.IsBackward = TRUE;
#if defined(TRACE_UART) && !defined(VALID_TOOLS)
        TraceBuffer(("Switched backward \r\n"));
#endif
        VIDPROD_Data_p->Backward.WaitingProducerReadyToSwitchBackward = FALSE;
        VIDPROD_Data_p->Backward.ChangingDirection = TRICKMODE_NOCHANGE;
        VIDPROD_Data_p->Backward.DoNotTreatParserJobCompletedEvt = FALSE;
    }
    if ((VIDPROD_Data_p->Backward.WaitingProducerReadyToSwitchForward) && (!VIDPROD_Data_p->DataUnderflowEventNotSatisfied))
    {
        PARSER_Stop(VIDPROD_Data_p->ParserHandle);
        PARSER_SetBitBuffer(VIDPROD_Data_p->ParserHandle, NULL, 0, BIT_BUFFER_CIRCULAR, FALSE);
        VIDPROD_Data_p->Backward.WaitingProducerReadyToSwitchForward = FALSE;
        VIDPROD_Data_p->Skip.AllPPictures = FALSE;
        VIDPROD_Data_p->Skip.AllBPictures = FALSE;
        VIDPROD_Data_p->ForTask.NbCompletedIndex = 0;
        VIDPROD_Data_p->Backward.JustSwitchForward = TRUE;
        /* Lock Access to Decode Pictures */
        STOS_SemaphoreWait(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);
        for (LoopCounter = 0 ; LoopCounter < MAX_CURRENT_PICTURES ; LoopCounter++)
        {
            VIDPROD_Data_p->DecoderStatus.CurrentPictures_p[LoopCounter] = NULL;
        }
        VIDPROD_Data_p->DecoderStatus.NextOutputIndex = 0;
        VIDPROD_Data_p->DecoderStatus.NbIndices = 0;

        /* Unlock Access to Decode Pictures */
        STOS_SemaphoreSignal(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);
        STOS_SemaphoreWait(VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureSemaphore_p);
        VIDPROD_Data_p->PreprocessingPicture.NbUsedPictureBuffers = 0;
    	for(LoopCounter = 0 ; LoopCounter < MAX_NUMBER_OF_PREPROCESSING_PICTURES ; LoopCounter++)
    	{
        	VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_EMPTY;
        	VIDPROD_Data_p->PreprocessingPicture.IsPictureBufferUsed[LoopCounter] = FALSE;
        	VIDPROD_Data_p->PreprocessingPicture.IsPictureTimedOut[LoopCounter] = FALSE;
    	}
        STOS_SemaphoreSignal(VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureSemaphore_p);
		PARSER_StartParams.StreamType = VIDPROD_Data_p->ParserStatus.StreamType;
    	PARSER_StartParams.StreamID = VIDPROD_Data_p->ParserStatus.StreamID;
    	PARSER_StartParams.BroadcastProfile = VIDPROD_Data_p->BroadcastProfile;
    	PARSER_StartParams.ErrorRecoveryMode = PARSER_ERROR_RECOVERY_MODE1;     /*  TODO_CL : Not yet defined, should be defined according to VIDPROD_Data_p->ErrorRecoveryMode value */
    	PARSER_StartParams.OverwriteFullBitBuffer = TRUE;

        #if defined(DVD_SECURED_CHIP)
            /* For secured chip, instead of giving the address of the bit buffer to the parser, the ES Copy buffer */
            /* address is given instead. Thus, the rest of the parser code remains unchanged. */
            PARSER_StartParams.BitBufferWriteAddress_p = VIDPROD_Data_p->ESCopyBuffer.Address_p;
    	#else
    	    PARSER_StartParams.BitBufferWriteAddress_p = VIDPROD_Data_p->BitBuffer.Address_p;
    	#endif /* DVD_SECURED_CHIP */

    	PARSER_StartParams.DefaultGDC_p = NULL;
    	PARSER_StartParams.DefaultPDC_p = NULL;
    	PARSER_StartParams.RealTime = VIDPROD_Data_p->RealTime;
        PARSER_StartParams.ResetParserContext = TRUE;

    	ErrorCode = PARSER_Start(VIDPROD_Data_p->ParserHandle,&PARSER_StartParams);
#if defined(TRACE_UART) && !defined(VALID_TOOLS)
        TraceBuffer(("Switched forward \r\n"));
#endif
        DECODER_SetDecodingDirection(VIDPROD_Data_p->DecoderHandle,DECODING_FORWARD);
        VIDPROD_Data_p->Backward.IsBackward = FALSE;
		VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p = NULL;
    	VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p = NULL;
        VIDPROD_Data_p->Backward.ChangingDirection = TRICKMODE_NOCHANGE;
        VIDPROD_Data_p->Backward.InjectDiscontinuityCanBeAccepted = TRUE;
        VIDPROD_Data_p->Backward.DoNotTreatParserJobCompletedEvt = FALSE;
	}
#endif /* STVID_TRICKMODE_BACKWARD */
#ifndef STVID_PRODUCER_NO_TIMEOUT
    if ((VIDPROD_Data_p->ProducerState != VIDPROD_DECODER_STATE_STOPPED) &&
        (!(VIDPROD_Data_p->ForTask.SoftResetPending)) &&
        (!(VIDPROD_Data_p->Stop.IsPending)))
    {
#ifdef STVID_TRICKMODE_BACKWARD
        if ((!VIDPROD_Data_p->Backward.IsBackward) &&
            (!VIDPROD_Data_p->Backward.JustSwitchForward) &&
            (VIDPROD_Data_p->Backward.ChangingDirection == TRICKMODE_NOCHANGE))
#endif /* PRODCUDER_BACKWARD */
        {
            /* Check if max picture search time is over for last parser command sent */
            if (VIDPROD_Data_p->ParserStatus.IsSearchingPicture)
            {
                if ( (IsTimeOutOnStartCodeSearchTakenIntoAccount(VIDPROD_Data_p->ErrorRecoveryMode)) &&
                     (VIDPROD_Data_p->StartCondition.IsDecodeEnable) &&
                     (STOS_time_after(time_now(), VIDPROD_Data_p->ParserStatus.MaxPictureSearchTime) != 0))
                {
                    /* start code search success should occur immediately after its launch if LastBitBufferLevel is */
                    /* above its middle size.*/
#ifdef ST_speed
                    /*if (VIDPROD_Data_p->LastBitBufferLevel > ((VIDPROD_Data_p->BitBuffer.TotalSize / 10)*9))*/
                    if (VIDPROD_Data_p->LastBitBufferLevel > VIDPROD_Data_p->BitBuffer.TotalSize / 2)
#else
                    if (VIDPROD_Data_p->LastBitBufferLevel > VIDPROD_Data_p->BitBuffer.TotalSize / 2)
#endif /*ST_speed*/
                    {
                        /* Too long time parser was launched, there must have been a problem */
#if defined(TRACE_UART) && !defined(VALID_TOOLS)
                        STOS_Clock_t Now = time_now();
                        TraceBuffer(("\r\n-**********Picture search overtime[%d]**********\r\n", time_minus(Now, VIDPROD_Data_p->ParserStatus.MaxPictureSearchTime)));
#endif /* TRACE_UART */

                        /* Full reset */
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Picture search overtime, producer RESET"));
                        vidprod_ControllerReset(ProducerHandle);
                    }
                }
            }
	    }


        /* Check if max preprocessing time is over for all preproc command sent and not finished */
        TOut_MaxVSyncDuration = (STOS_Clock_t) STVID_MAX_VSYNC_DURATION;
#ifdef STVID_TRICKMODE_BACKWARD
        if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
        {
        for (LoopCounter = 0 ; LoopCounter < MAX_NUMBER_OF_PREPROCESSING_PICTURES ; LoopCounter++)
        {
            if (VIDPROD_Data_p->PreprocessingPicture.IsPictureBufferUsed[LoopCounter])
            {
                if ((VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_WAITING_CONFIRM_EVT) &&
                    (!VIDPROD_Data_p->PreprocessingPicture.IsPictureTimedOut[LoopCounter]))
                {
                    TOut_Now = time_now();
#ifdef HARDWARE_EMULATOR
                    TOut_MaxAllowedDuration = ((TOut_MaxVSyncDuration * (STOS_Clock_t)(4 + 8)) * (STOS_Clock_t)2);  /* Based on stats on 5x6 MB streams */
                    TOut_MaxAllowedDuration /= ((STOS_Clock_t) 8160 /
                        (STOS_Clock_t)(((VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.Width+15)/16) *
                                    ((VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.Height+15)/16)));
#else /* HARDWARE_EMULATOR */
                    TOut_MaxAllowedDuration = (TOut_MaxVSyncDuration * (STOS_Clock_t)(8));
#endif /* NOT HARDWARE_EMULATOR */
                    TOut_MaxAllowedTime = TOut_MaxAllowedDuration + VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.StartTime;
                    /* 4 for preproc, 4 for buffer availability (who said arbitrary ?!) and 2 times for margin */
                    /* TODO_CL: use timeout values from codec_getinfo instead of these hard coded values */
                    if (STOS_time_after(TOut_Now, TOut_MaxAllowedTime))
                    {
                        /* Too long time decode was launched, there must have been a problem */
                        /* One more picture with an error */
                        VIDPROD_Data_p->UnsuccessfullDecodeCount ++;
                        VIDPROD_Data_p->PreprocessingPicture.IsPictureTimedOut[LoopCounter] = TRUE;
#if defined(TRACE_UART) && !defined(VALID_TOOLS)
                        TraceBuffer(("\r\n-**********DEC/SKIP overtime[%"FORMAT_SPEC_OSCLOCK"d]**********\a\r\n", time_minus(TOut_Now, TOut_MaxAllowedTime)));
#endif /* TRACE_UART */
#ifdef STVID_DEBUG_GET_STATISTICS
                        VIDPROD_Data_p->Statistics.DecodePbDecodeTimeOutError ++;
#endif /* STVID_DEBUG_GET_STATISTICS */
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_TIMEOUT, "pDEC_PPTOErr(%d-%d/%d) ",
                                VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Preprocessing never ending on picture DOFID=%d ExtPOC=%d/%d (Maxdelay=%"FORMAT_SPEC_OSCLOCK"d us, actual=%"FORMAT_SPEC_OSCLOCK"d us, diff=%"FORMAT_SPEC_OSCLOCK"d us)",
                        VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                        VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                        VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                        TOut_MaxAllowedDuration*OSCLOCK_T_MILLION/TOut_ClockTicks,
                        time_minus(TOut_Now, VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.StartTime)*OSCLOCK_T_MILLION/TOut_ClockTicks,
                        time_minus(TOut_Now, TOut_MaxAllowedTime)*OSCLOCK_T_MILLION/TOut_ClockTicks));
#if !defined (STVID_VALID)
                        /* Abort the decode which is never ending */
                        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Abort the decode cmdID=%d", VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId));
                        ErrorCode = DECODER_Abort(VIDPROD_Data_p->DecoderHandle, VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            /* NOTE_CL: unexpected error, decoder internal data failure or bad producer data handling */
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER_Abort rejected ID=%d!!", VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId));
                        }
                            if ((VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].IsReferenceForDecoding) && IsSpoiledReferenceLoosingAllReferencesUntilNextRandomAccessPoint(VIDPROD_Data_p->ErrorRecoveryMode) )
                        {
                            if ((!IsIOnlyDecodeOnGoing(VIDPROD_Data_p->Skip)) || (IsFirstOfTwoFieldsOfIFrame(&VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter])))
								{
                                /* Search next entry point because a reference picture has been aborted and we are not decoding I only */
                                	STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Ask for skip until next RAP"));
                                	VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint = TRUE;
                                    VIDPROD_Data_p->Skip.RecoveringNextEntryPoint = TRUE;
                                    VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = TRUE;
								}
                            }
                            vidprod_RemovePictureFromPreprocessingTable(ProducerHandle, &(VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter]));
                            /* This decode is aborted, decrease VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue */
                            DECREMENT_NB_PICTURES_IN_QUEUE(ProducerHandle);
#endif /* !defined (STVID_VALID) */
                        }
                    }
                }
            } /* end for LoopCounter for pre processing time out */
        }/* not backward*/
#ifdef STVID_TRICKMODE_BACKWARD
        else if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p != NULL)
        {
            for (LoopCounter = 0 ; LoopCounter < MAX_BACKWARD_PICTURES ; LoopCounter++)
            {
                if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.IsPictureBufferUsed[LoopCounter])
                {
                    if ((VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_WAITING_CONFIRM_EVT) &&
                        (!VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.IsPictureTimedOut[LoopCounter]))
                    {
                        TOut_Now = time_now();
                        TOut_MaxAllowedDuration = (TOut_MaxVSyncDuration * (STOS_Clock_t)(16));
                        TOut_MaxAllowedDuration = TOut_MaxAllowedDuration*16;
                        TOut_MaxAllowedTime = time_plus(TOut_MaxAllowedDuration, VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.StartTime);
                        /* 4 for preproc, 4 for buffer availability (who said arbitrary ?!) and 2 times for margin */
                        /* TODO_CL: use timeout values from codec_getinfo instead of these hard coded values */
                        if (STOS_time_after(TOut_Now, TOut_MaxAllowedTime))
                        {
                            /* Too long time decode was launched, there must have been a problem */
                            /* One more picture with an error */
                            VIDPROD_Data_p->UnsuccessfullDecodeCount ++;
                            VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.IsPictureTimedOut[LoopCounter] = TRUE;
    #ifdef STVID_DEBUG_GET_STATISTICS
                            VIDPROD_Data_p->Statistics.DecodePbDecodeTimeOutError ++;
    #endif /* STVID_DEBUG_GET_STATISTICS */

                            /* Abort the decode which is never ending */
                            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Abort the decode cmdID=%d", VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId));
                            VIDPROD_Data_p->Backward.NbDecodedPictures ++;
                            ErrorCode = DECODER_Abort(VIDPROD_Data_p->DecoderHandle, VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId);
                            if (ErrorCode != ST_NO_ERROR)
                            {
                                /* NOTE_CL: unexpected error, decoder internal data failure or bad producer data handling */
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER_Abort rejected ID=%d!!", VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId));
                            }
                            if ((VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].IsReferenceForDecoding)
                                && IsSpoiledReferenceLoosingAllReferencesUntilNextRandomAccessPoint(VIDPROD_Data_p->ErrorRecoveryMode))
                            {
                                if ((!IsIOnlyDecodeOnGoing(VIDPROD_Data_p->Skip)) || (IsFirstOfTwoFieldsOfIFrame(&VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter])))
								{
                                    /* Search next entry point because a reference picture has been aborted and we are not decoding I only */
                                	STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Ask for skip until next RAP"));
                                	VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint = TRUE;
                                    VIDPROD_Data_p->Skip.RecoveringNextEntryPoint = TRUE;
                                    VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = TRUE;
								}
                            }
                            /* TO_DO RHA create remove from table for backward */
                            vidprod_RemovePictureFromPreprocessingTableBackward(ProducerHandle, &(VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter]));
                            /* This decode is aborted, decrease VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue */
                            DECREMENT_NB_PICTURES_IN_QUEUE(ProducerHandle);
                        }
                    }
                }
            } /* end for LoopCounter for pre processing time out */
        }
#endif /* STVID_TRICKMODE_BACKWARD */


            /* Check if max decode time is over for all decode command sent and not finished */

        for (LoopCounter = 0 ; LoopCounter < MAX_NUMBER_OF_DECODE_PICTURES ; LoopCounter++)
        {
            if (VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter] != NULL)
            {
                if ((VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_DECODING) &&
                    (!VIDPROD_Data_p->DecodePicture.IsPictureTimedOut[LoopCounter]))
                {
                    TOut_Now = time_now();

#ifdef HARDWARE_EMULATOR
                    TOut_MaxAllowedDuration = ((TOut_MaxVSyncDuration * (STOS_Clock_t)(2 + 50)) * (STOS_Clock_t)2 );  /* Based on stats on 5x6 MB streams */
                    TOut_MaxAllowedDuration /= ((STOS_Clock_t) 8160/
                                (STOS_Clock_t)(((VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.Width +15)/16) *
                                            ((VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.Height+15)/16)));
#else /* HARDWARE_EMULATOR */
                    TOut_MaxAllowedDuration = (TOut_MaxVSyncDuration * (STOS_Clock_t)(16));
#endif /* NOT HARDWARE_EMULATOR */
                    TOut_MaxAllowedTime = time_plus(TOut_MaxAllowedDuration, VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->Decode.ConfirmedTime);
/* TODO_CL: use timeout values from codec_getinfo instead of these hard coded values */
                    /* 2 for decode, 2 for decoder availability and 2 times for margin */
                    /* Multiplied by ratio of picture size to max picture size (1920*1088 = 8160 Macroblocks */
                    if (STOS_time_after(TOut_Now, TOut_MaxAllowedTime))
                    {
                        /* Too long time decode was launched, there must have been a problem */
                        /* One more picture with an error */
                        VIDPROD_Data_p->UnsuccessfullDecodeCount ++;
                        VIDPROD_Data_p->DecodePicture.IsPictureTimedOut[LoopCounter] = TRUE;
#if defined(TRACE_UART) && !defined(VALID_TOOLS)
                        TraceBuffer(("\r\n-**********DEC/SKIP overtime[%"FORMAT_SPEC_OSCLOCK"d] (%d-%d/%d)********** \a\r\n", time_minus(TOut_Now, TOut_MaxAllowedTime),
                            VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                            VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                            VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID ));

#endif /* TRACE_UART */
#ifdef STVID_DEBUG_GET_STATISTICS
                        VIDPROD_Data_p->Statistics.DecodePbDecodeTimeOutError ++;
#endif /* STVID_DEBUG_GET_STATISTICS */
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_TIMEOUT, "pDEC_FMWTOErr(%d-%d/%d) ",
                            VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                            VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                            VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Decode never ending on picture DOFID=%d ExtPOC=%d/%d (Maxdelay=%"FORMAT_SPEC_OSCLOCK"d us, actual=%"FORMAT_SPEC_OSCLOCK"d us, diff=%"FORMAT_SPEC_OSCLOCK"d us)",
                            VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                            VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                            VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                            TOut_MaxAllowedDuration*OSCLOCK_T_MILLION/TOut_ClockTicks,
                            time_minus(TOut_Now, VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->Decode.ConfirmedTime)*OSCLOCK_T_MILLION/TOut_ClockTicks,
                            time_minus(TOut_Now, TOut_MaxAllowedTime)*OSCLOCK_T_MILLION/TOut_ClockTicks));
#if !defined (STVID_VALID)
                        /* Abort the decode which is never ending */
                        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Abort the decode cmdID=%d", VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->Decode.CommandId));
#ifdef STVID_TRICKMODE_BACKWARD
                        VIDPROD_Data_p->Backward.NbDecodedPictures ++;
#endif
                        ErrorCode = DECODER_Abort(VIDPROD_Data_p->DecoderHandle, VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->Decode.CommandId);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            /* NOTE_CL: unexpected error, decoder internal data failure or bad producer data handling */
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER_Abort rejected ID=%d!!", VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->Decode.CommandId));
                        }
                        if ((VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->IsReferenceForDecoding) && IsSpoiledReferenceLoosingAllReferencesUntilNextRandomAccessPoint(VIDPROD_Data_p->ErrorRecoveryMode))
                        {
                            if ((!IsIOnlyDecodeOnGoing(VIDPROD_Data_p->Skip)) || (IsFirstOfTwoFieldsOfIFrame(VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter])))
							{
                                /* Search next entry point because a reference picture has been aborted and we are not decoding I only */
                            	STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Ask for skip until next RAP"));
                            	VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint = TRUE;
                                VIDPROD_Data_p->Skip.RecoveringNextEntryPoint = TRUE;
                                VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = TRUE;
							}
                        }
                        /* Lock Access to Decode Pictures */
                        STOS_SemaphoreWait(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);
                        VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->IsReferenceForDecoding = FALSE;
                        /* Clean this picture in DecoderStatus table */
                        vidprod_RemovePictureFromDecodeStatusTable(ProducerHandle, VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]);
                        /* This decode is aborted, decrease VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue */
                        DECREMENT_NB_PICTURES_IN_QUEUE(ProducerHandle);
                        /* UnLock Access to Decode Pictures */
                        STOS_SemaphoreSignal(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);

                        /* Clean this picture in VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[] table */
                        PictureBuffer_p = VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter];
                        RemovePictureFromDecodePictureTable(ProducerHandle, VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter], __LINE__);
                        /* Release the picture buffer (and thus frame buffer) allocated for this decode */
                        if (PictureBuffer_p->AssociatedDecimatedPicture_p != NULL)
                        {
                            RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, PictureBuffer_p->AssociatedDecimatedPicture_p, VIDCOM_VIDPROD_MODULE_BASE);
                        }
                        RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, PictureBuffer_p, VIDCOM_VIDPROD_MODULE_BASE);
#endif /* if !defined (STVID_VALID) */
                    } /* if (STOS_time_after(TOut_Now, TOut_MaxAllowedTime)) */
                }
            }
        } /* end for LoopCounter for decode time out */



#if 0  /* producer RESET needs further work to achieve properly */
        /* Is level of amount of errors is reached ? */
        if (
            ((VIDPROD_Data_p->UnsuccessfullDecodeCount >= ACCEPTANCE_ERROR_AMOUNT_LOWEST_ERROR_RECOVERY) &&
            (VIDPROD_Data_p->ErrorRecoveryMode != STVID_ERROR_RECOVERY_NONE)) ||
            ((VIDPROD_Data_p->UnsuccessfullDecodeCount >= ACCEPTANCE_ERROR_AMOUNT_HIGHEST_ERROR_RECOVERY) &&
            (VIDPROD_Data_p->ErrorRecoveryMode == STVID_ERROR_RECOVERY_FULL))
           )
        {
#if defined(TRACE_UART) && !defined(VALID_TOOLS)
            /*TraceBuffer(("\r\nDecodeCountSoftRt\r\n"));*/
#endif /* TRACE_UART */
            VIDPROD_Data_p->UnsuccessfullDecodeCount = 0;
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Too much errors, producer RESET"));
            vidprod_ControllerReset(ProducerHandle);
        }
#endif /* #if 0 producer RESET needs further work to achieve properly */
    }

#endif /* not STVID_PRODUCER_NO_TIMEOUT */

} /* End of ProducerTaskFuncForAutomaticControlMode() function */

/*******************************************************************************
Name        : DoStopNow
Description : Stop the decode process
Parameters  :
Assumptions :
Limitations : Caution: this function can be called from API level, i.e. from user direct call !
Limitations : Caution: This function must never be called with SendStopEvent = TRUE inside a VIDPROD_Xxxx() function, as notification of event is forbidden there !
Returns     : ST_NO_ERROR
*******************************************************************************/
static void DoStopNow(const VIDPROD_Handle_t ProducerHandle, const BOOL SendStopEvent)
{
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    STVID_PictureParams_t LastDecodedPictureParams;
    STVID_PictureParams_t * LastDecodedPictureParams_p;
    U32 LoopCounter;

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DoStopNow called"));
#if defined(TRACE_UART) && !defined(VALID_TOOLS)
    TraceBuffer(("\r\nProducer DoStopNow."));
#endif /* TRACE_UART */

    /* Set state to stopped */
    VIDPROD_Data_p->ProducerState = VIDPROD_DECODER_STATE_STOPPED;
    VIDPROD_Data_p->DecoderIsFreezed = FALSE;

#ifdef ST_ordqueue
    /* Flush all pictures in ordering queue to display */
    VIDQUEUE_PushAllPicturesToDisplay(VIDPROD_Data_p->OrderingQueueHandle);
#endif /* ST_ordqueue */
    vidprod_ReleasePushedNonRefPictures(ProducerHandle);

    VIDPROD_Data_p->Stop.WhenNextReferencePicture = FALSE;
    VIDPROD_Data_p->Stop.WhenNoData = FALSE;
    VIDPROD_Data_p->Stop.StopAfterBuffersTimeOut = FALSE;
    VIDPROD_Data_p->Stop.DiscontinuityStartCodeInserted = FALSE;
    VIDPROD_Data_p->Stop.WhenNextIPicture = FALSE;
    VIDPROD_Data_p->Stop.AfterCurrentPicture = FALSE;

    /* Reset the state do not do actions between a stop and a start. */
    VIDPROD_Data_p->NextAction.LaunchDecodeWhenPossible = 0;
    VIDPROD_Data_p->NextAction.LaunchDisplayWhenPossible = 0;
    VIDPROD_Data_p->NextAction.NumberOfDisplayToLaunch = 0;
    VIDPROD_Data_p->NextAction.LaunchSkipWhenPossible = FALSE;
    VIDPROD_Data_p->ForTask.NoDisplayAfterNextDecode = FALSE;
    VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = FALSE;

    VIDPROD_Data_p->Stop.IsPending = FALSE;

    /* Remove all inserted but not yet displayed pictures */
    VIDQUEUE_CancelAllPictures(VIDPROD_Data_p->OrderingQueueHandle);
    vidprod_ReleaseNonPushedNonRefPictures(ProducerHandle);
    ResetReferenceList(ProducerHandle);
#if COMPUTE_PRESENTATION_ID
    for (LoopCounter = 0 ; LoopCounter < STORED_PICTURE_BUFFER_SIZE ; LoopCounter++)
    {
        VIDPROD_Data_p->StoredPictureBuffer[LoopCounter] = NULL;
    }
    VIDPROD_Data_p->StoredPictureCounter = 0;
    VIDPROD_Data_p->NumOfPictToWaitBeforePush = 2;
    VIDPROD_Data_p->PreviousPictureCount = 0;
#endif /* COMPUTE_PRESENTATION_ID */
    /* Release vidprod_stop prior to notify the stopped event */
    STOS_SemaphoreSignal(VIDPROD_Data_p->Stop.ActuallyStoppedSemaphore_p);

    if (SendStopEvent)
    {
        if (VIDPROD_Data_p->DecoderStatus.LastDecodedPicture_p!=NULL)
        {
            vidcom_PictureInfosToPictureParams(&(VIDPROD_Data_p->DecoderStatus.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos),
                &LastDecodedPictureParams, VIDPROD_Data_p->DecoderStatus.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID);
            LastDecodedPictureParams_p = &LastDecodedPictureParams;
        }
        else
        {
            LastDecodedPictureParams_p = NULL;
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Producer DoStopNow event notification required but LastDecodedPicture_p is NULL."));
        }
        STEVT_Notify(VIDPROD_Data_p->EventsHandle, VIDPROD_Data_p->RegisteredEventsID[VIDPROD_STOPPED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST LastDecodedPictureParams_p);
#if defined(TRACE_UART) && !defined(VALID_TOOLS)
        TraceBuffer((" [notify STOPPED evt] \r\n"));
#endif /* TRACE_UART */
    }
} /* End of DoStopNow() function */

/*******************************************************************************
Name        : SeekBeginningOfStream
Description : Seek beginning of stream (sequence start code)
Parameters  : Video producer handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SeekBeginningOfStream(const VIDPROD_Handle_t ProducerHandle)
{
    VIDPROD_Data_t *            VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    PARSER_StartParams_t        PARSER_StartParams;
    U32                         LoopCounter;
    ST_ErrorCode_t              ErrorCode;
    VIDBUFF_PictureBuffer_t *   LoopPictureBuffer_p;

    /* Clear all controller commands already queued */
     ClearAllPendingControllerCommands(ProducerHandle);
    /* Reset task variables that need to be reset after a hardware SoftReset */
    VIDPROD_Data_p->NextAction.LaunchDecodeWhenPossible = 0;
    VIDPROD_Data_p->NextAction.LaunchSkipWhenPossible = 0;
    VIDPROD_Data_p->ForTask.NoDisplayAfterNextDecode = FALSE;
    VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = FALSE;

    /* Clear currently preprocessing pictures buffers */
    /* Lock Access to Preprocessing Pictures */
    STOS_SemaphoreWait(VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureSemaphore_p);

    VIDPROD_Data_p->PreprocessingPicture.NbUsedPictureBuffers = 0;
    for(LoopCounter = 0 ; LoopCounter < MAX_NUMBER_OF_PREPROCESSING_PICTURES ; LoopCounter++)
    {
        VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_EMPTY;
        VIDPROD_Data_p->PreprocessingPicture.IsPictureBufferUsed[LoopCounter] = FALSE;
        VIDPROD_Data_p->PreprocessingPicture.IsPictureTimedOut[LoopCounter] = FALSE;
    }

    /* UnLock Access to Preprocessing Pictures */
    STOS_SemaphoreSignal(VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureSemaphore_p);

    /* Lock Access to Reference Pictures */
    STOS_SemaphoreWait(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);

    /* DecodePictureBuffer pictures have already been initialised. Then, they can not be reseted to NULL without being UNLOCKED, */
    /* it their status are RESERVED. Otherwise, one of the frame buffers may be stay definitively set to RESERVED status  */
    /* and then unusable. */
    for (LoopCounter = 0 ; LoopCounter < MAX_NUMBER_OF_DECODE_PICTURES ; LoopCounter++)
    {
        LoopPictureBuffer_p = VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter];
        if (LoopPictureBuffer_p != NULL)
        {
            if (LoopPictureBuffer_p->Buffers.PictureLockCounter > 0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Delete LOCKED entry DecodePictureBuffer[%d]=0x%x (%d-%d/%d) with status : IsInUse=%d dispstatus=%d cntlock=%d IsInDisplayQ=%d",
                    LoopCounter,
                    LoopPictureBuffer_p,
                    LoopPictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                    LoopPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                    LoopPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                    LoopPictureBuffer_p->Buffers.IsInUse,
                    LoopPictureBuffer_p->Buffers.DisplayStatus,
                    LoopPictureBuffer_p->Buffers.PictureLockCounter,
                    LoopPictureBuffer_p->Buffers.IsInDisplayQueue
                    ));
                if (LoopPictureBuffer_p->FrameBuffer_p == NULL)
            {
                /* NOTE_CL: this should never occur, test has been put in place for STOP/START implementation debugging */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Producer DecodePictureBuffer[%d]=0x%x (%d-%d/%d) has NULL FrameBuffer pointer (BUG-BUG-BUG)",
                        LoopCounter,
                            LoopPictureBuffer_p,
                            LoopPictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                            LoopPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                            LoopPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
                }
            }
            else
            {
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Delete entry DecodePictureBuffer[%d]=0x%x (%d-%d/%d) with status : IsInUse=%d dispstatus=%d cntlock=%d IsInDisplayQ=%d",
                    LoopCounter,
                    LoopPictureBuffer_p,
                    LoopPictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                    LoopPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                    LoopPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                    LoopPictureBuffer_p->Buffers.IsInUse,
                    LoopPictureBuffer_p->Buffers.DisplayStatus,
                    LoopPictureBuffer_p->Buffers.PictureLockCounter,
                    LoopPictureBuffer_p->Buffers.IsInDisplayQueue
                    ));
            }
            VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter] = NULL;
            VIDPROD_Data_p->DecodePicture.IsPictureTimedOut[LoopCounter] = FALSE;
        }
    }
    VIDPROD_Data_p->DecodePicture.NbUsedPictureBuffers = 0;

    for (LoopCounter = 0 ; LoopCounter < MAX_CURRENT_PICTURES ; LoopCounter++)
    {
        VIDPROD_Data_p->DecoderStatus.CurrentPictures_p[LoopCounter] = NULL;
    }
    VIDPROD_Data_p->DecoderStatus.NextOutputIndex = 0;
    VIDPROD_Data_p->DecoderStatus.NbIndices = 0;

    /* UnLock Access to Reference Pictures */
    STOS_SemaphoreSignal(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);

    /* After Soft reset: as this flushes the bit buffer, we need to wait again for vbv_delay level to be reached in bit buffer
       exactly as after Start(). Otherwise, if there are some errors, the decode starts again with a bit buffer
       at a bad level, and underflow or overflow quickly happens causing more errors on some streams or synchronisation
       difficulties. In ATSC it is very very bad. (This is the same problem as when video didn't take vbv_delay level into account.) */
    /* By default the decode is enabled at start */
    if (VIDPROD_Data_p->RealTime)
    {
         VIDPROD_Data_p->StartCondition.IsDecodeEnable = FALSE;
    }
    else
    {
        VIDPROD_Data_p->StartCondition.IsDecodeEnable = TRUE;
    }

    /* Start Condition flags init */
    VIDPROD_Data_p->StartCondition.FirstPictureAfterReset = TRUE;
    VIDPROD_Data_p->StartCondition.StartOnPictureDelay = FALSE;
    VIDPROD_Data_p->StartCondition.StartWithBufferFullness = FALSE;
    VIDPROD_Data_p->StartCondition.BufferFullnessToReachBeforeStartInBytes = 0;
    VIDPROD_Data_p->StartCondition.StartWithPTSTimeComparison = FALSE;
    VIDPROD_Data_p->StartCondition.FirstPicturePTSAfterReset.PTS   = 0;
    VIDPROD_Data_p->StartCondition.FirstPicturePTSAfterReset.PTS33 = FALSE;

    PARSER_StartParams.StreamType = VIDPROD_Data_p->ParserStatus.StreamType;
    PARSER_StartParams.StreamID = VIDPROD_Data_p->ParserStatus.StreamID;
    PARSER_StartParams.BroadcastProfile = VIDPROD_Data_p->BroadcastProfile;
    PARSER_StartParams.ErrorRecoveryMode = PARSER_ERROR_RECOVERY_MODE1;     /*  TODO_CL : Not yet defined, should be defined according to VIDPROD_Data_p->ErrorRecoveryMode value */
    PARSER_StartParams.OverwriteFullBitBuffer = TRUE;

    #if defined(DVD_SECURED_CHIP)
        /* For secured chip, instead of giving the address of the bit buffer to the parser, the ES Copy buffer */
        /* address is given instead. Thus, the rest of the parser code remains unchanged. */
        PARSER_StartParams.BitBufferWriteAddress_p = VIDPROD_Data_p->ESCopyBuffer.Address_p;
	#else
	    PARSER_StartParams.BitBufferWriteAddress_p = VIDPROD_Data_p->BitBuffer.Address_p;
	#endif /* DVD_SECURED_CHIP */

    PARSER_StartParams.DefaultGDC_p = NULL;
    PARSER_StartParams.DefaultPDC_p = NULL;
    PARSER_StartParams.RealTime = VIDPROD_Data_p->RealTime;
    PARSER_StartParams.ResetParserContext = TRUE;

#if defined(TRACE_UART) && defined(VALID_TOOLS)
    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_RESET, "pPARS_Set "));
#endif
    ErrorCode = PARSER_Start(VIDPROD_Data_p->ParserHandle,&PARSER_StartParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* TODO_CL: error handling to be defined */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "PARSER_Start failed Error=%d!!", ErrorCode));
    }

    VIDPROD_Data_p->ParserStatus.IsSearchingPicture = FALSE;
    VIDPROD_Data_p->ParserStatus.SearchNextRandomAccessPoint = TRUE;   /* Next picture search will be for a Random Access Point ("beginning of stream") */
    VIDPROD_Data_p->DecoderStatus.LastReadAddress_p = VIDPROD_Data_p->BitBuffer.Address_p; /* Start search from beginning of bit buffer */

    /* Reset DECODER : any decode on-going will be aborted ! */
#if defined(TRACE_UART) && defined(VALID_TOOLS)
    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_RESET, "pDEC_Rst "));
#endif
    ErrorCode = DECODER_Reset(VIDPROD_Data_p->DecoderHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* TODO_CL: error handling to be defined */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER_Reset failed Error=%d!!", ErrorCode));
    }
    VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue = 0;
    VIDPROD_Data_p->UnsuccessfullDecodeCount = 0;

#ifdef STVID_DEBUG_GET_STATISTICS
    VIDPROD_Data_p->Statistics.DecodeHardwareSoftReset++;
#endif /* STVID_DEBUG_GET_STATISTICS */

    /* Notification of this soft reset to advise outside world. */
    STEVT_Notify(VIDPROD_Data_p->EventsHandle, VIDPROD_Data_p->RegisteredEventsID[VIDPROD_RESTARTED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);

} /* End of SeekBeginningOfStream() function */

/*******************************************************************************
Name        : ClearAllPendingControllerCommands
Description : Clears commands pending in queue of controller commands
              To be used at SoftReset, so that eventual previous commandsdon't perturb new start
Parameters  : Video producer handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void ClearAllPendingControllerCommands(const VIDPROD_Handle_t ProducerHandle)
{
    U8   ControllerCommand;

    while (PopControllerCommand(ProducerHandle, &ControllerCommand) == ST_NO_ERROR)
    {
        ; /* Nothing to do, just pop all start code pushed... */
    }
} /* End of ClearAllPendingControllerCommands() function */

/*******************************************************************************
Name        : GetBuffers
Description : Buffers configuration to decode
Parameters  : Video producer handle
            OUT: DecodedPicture_p_p,
            OUT: DecodedDecimPicture_p_p
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static ST_ErrorCode_t GetBuffers(const VIDPROD_Handle_t ProducerHandle,
                                 const VIDBUFF_PictureBuffer_t *PictureBuffer_p,
                                 VIDBUFF_PictureBuffer_t ** DecodedPicture_p_p,
                                 VIDBUFF_PictureBuffer_t ** DecodedDecimPicture_p_p )
{
    VIDPROD_Data_t *                        VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    VIDBUFF_GetUnusedPictureBufferParams_t  GetPictureBufferParams;
    ST_ErrorCode_t                          ErrorMain = ST_NO_ERROR;
    ST_ErrorCode_t                          ErrorSecondary = ST_NO_ERROR;
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;
    BOOL                                    NewBufferAllocated = FALSE;
    U32                                     MainBuffers, DecimBuffers;
    VIDPROD_DisplayedReconstruction_t       NecessaryReconstruction;

    /* Get a picture buffer to decode in , and another for decimated */
    /* reconstruction */
    GetPictureBufferParams.MPEGFrame            = PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame;
    GetPictureBufferParams.PictureStructure     = PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure;
    GetPictureBufferParams.TopFieldFirst        = PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.TopFieldFirst;
    if (!IsFramePicture(PictureBuffer_p))
    {
        GetPictureBufferParams.ExpectingSecondField = PictureBuffer_p->ParsedPictureInformation.PictureGenericData.IsFirstOfTwoFields;
    }
    else
    {
        GetPictureBufferParams.ExpectingSecondField = FALSE;
    }
    GetPictureBufferParams.DecodingOrderFrameID = PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID;
    GetPictureBufferParams.PictureWidth         = PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.Width;
    GetPictureBufferParams.PictureHeight        = PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.Height;

    *DecodedDecimPicture_p_p    = NULL;
    *DecodedPicture_p_p         = NULL;

    /* Update the Applicable Decimation Factor */
    VIDBUFF_GetApplicableDecimationFactor(VIDPROD_Data_p->BufferManagerHandle, &VIDPROD_Data_p->ForTask.CurrentProfile.ApplicableDecimationFactor);
    if ((VIDPROD_Data_p->ForTask.CurrentProfile.ApplicableDecimationFactor != STVID_DECIMATION_FACTOR_NONE)
       || (VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2)
       || (VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2)
       || (VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS) /* PLE_TO_DO : check that both displayed reconstruction is the right one for AVS */
       || (VIDPROD_Data_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2))
   {
       NecessaryReconstruction = VIDPROD_DISPLAYED_BOTH;
   }
   else
   {
       NecessaryReconstruction = VIDPROD_DISPLAYED_PRIMARY_ONLY;
   }

    switch(NecessaryReconstruction)
    {
        case VIDPROD_DISPLAYED_UNKNOWN:
        case VIDPROD_DISPLAYED_NONE:
            return(ST_ERROR_NO_MEMORY);
        break;
        case VIDPROD_DISPLAYED_BOTH: /* we must get the two buffers */
            ErrorMain = VIDBUFF_GetAndTakeUnusedPictureBuffer( VIDPROD_Data_p->BufferManagerHandle, &GetPictureBufferParams, DecodedPicture_p_p, VIDCOM_VIDPROD_MODULE_BASE);
            if ((ErrorMain == ST_ERROR_NO_MEMORY) && VIDPROD_Data_p->ForTask.NeedForExtraAllocation)
            {
                ErrorCode = VIDBUFF_AllocateNewFrameBuffer(VIDPROD_Data_p->BufferManagerHandle, &(VIDPROD_Data_p->ForTask.NeedForExtraAllocation), &(VIDPROD_Data_p->ForTask.MaxSizeReachedForExtraAllocation));
                if (ErrorCode == ST_NO_ERROR)
                {
                    NewBufferAllocated = TRUE;
                    ErrorMain = VIDBUFF_GetAndTakeUnusedPictureBuffer(VIDPROD_Data_p->BufferManagerHandle, &GetPictureBufferParams, DecodedPicture_p_p, VIDCOM_VIDPROD_MODULE_BASE);
                }
            }

            if (ErrorMain == ST_NO_ERROR)
            {
                ErrorSecondary = VIDBUFF_GetAndTakeUnusedDecimatedPictureBuffer(VIDPROD_Data_p->BufferManagerHandle, &GetPictureBufferParams, DecodedDecimPicture_p_p, VIDCOM_VIDPROD_MODULE_BASE);
            if ((ErrorSecondary == ST_ERROR_NO_MEMORY) && VIDPROD_Data_p->ForTask.NeedForExtraAllocationDecimated)
            {
                ErrorCode = VIDBUFF_AllocateNewDecimatedFrameBuffer(VIDPROD_Data_p->BufferManagerHandle, &(VIDPROD_Data_p->ForTask.NeedForExtraAllocationDecimated), &(VIDPROD_Data_p->ForTask.MaxSizeReachedForExtraAllocationDecimated));
                if (ErrorCode == ST_NO_ERROR)
                {
                        ErrorSecondary = VIDBUFF_GetAndTakeUnusedDecimatedPictureBuffer(VIDPROD_Data_p->BufferManagerHandle, &GetPictureBufferParams, DecodedDecimPicture_p_p, VIDCOM_VIDPROD_MODULE_BASE);
                    }
                }
                if (ErrorSecondary != ST_NO_ERROR)
                {
                    /* The main picture has been taken, we must release it before to exit */
                    UNTAKE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, (*DecodedPicture_p_p), VIDCOM_VIDPROD_MODULE_BASE);
                }
            }

            if ((ErrorMain != ST_NO_ERROR)||(ErrorSecondary != ST_NO_ERROR))
            {
                if (ErrorMain != ST_NO_ERROR)
                {
                    return(ErrorMain);
                }
                else
                {
                    return(ErrorSecondary);
                }
            }
            /* else : no error -> continue to linkage/reconstruction */
        break;

        case VIDPROD_DISPLAYED_PRIMARY_ONLY: /* we must get at least the primary buffer */
            ErrorMain = VIDBUFF_GetAndTakeUnusedPictureBuffer(VIDPROD_Data_p->BufferManagerHandle, &GetPictureBufferParams, DecodedPicture_p_p, VIDCOM_VIDPROD_MODULE_BASE);
            if ((ErrorMain == ST_ERROR_NO_MEMORY) && VIDPROD_Data_p->ForTask.NeedForExtraAllocation)
            {
                ErrorCode = VIDBUFF_AllocateNewFrameBuffer(VIDPROD_Data_p->BufferManagerHandle, &(VIDPROD_Data_p->ForTask.NeedForExtraAllocation), &(VIDPROD_Data_p->ForTask.MaxSizeReachedForExtraAllocation));
                if (ErrorCode == ST_NO_ERROR)
                {
                    NewBufferAllocated = TRUE;
                    ErrorMain = VIDBUFF_GetAndTakeUnusedPictureBuffer(VIDPROD_Data_p->BufferManagerHandle, &GetPictureBufferParams, DecodedPicture_p_p, VIDCOM_VIDPROD_MODULE_BASE);
                }
            }
            if (ErrorMain != ST_NO_ERROR)
            {
                return(ErrorMain);
            }
             ErrorSecondary = VIDBUFF_GetAndTakeUnusedDecimatedPictureBuffer(VIDPROD_Data_p->BufferManagerHandle, &GetPictureBufferParams, DecodedDecimPicture_p_p, VIDCOM_VIDPROD_MODULE_BASE);
            if ((ErrorSecondary == ST_ERROR_NO_MEMORY) && VIDPROD_Data_p->ForTask.NeedForExtraAllocationDecimated)
            {
                ErrorCode = VIDBUFF_AllocateNewDecimatedFrameBuffer(VIDPROD_Data_p->BufferManagerHandle, &(VIDPROD_Data_p->ForTask.NeedForExtraAllocationDecimated), &(VIDPROD_Data_p->ForTask.MaxSizeReachedForExtraAllocationDecimated));
                if (ErrorCode == ST_NO_ERROR)
                {
                    ErrorSecondary = VIDBUFF_GetAndTakeUnusedDecimatedPictureBuffer(VIDPROD_Data_p->BufferManagerHandle, &GetPictureBufferParams, DecodedDecimPicture_p_p, VIDCOM_VIDPROD_MODULE_BASE);
                }
            }
            /* don't check ErrorSecondary, continue anyway */
        break;

        case VIDPROD_DISPLAYED_SECONDARY_ONLY:
            ErrorMain = VIDBUFF_GetAndTakeUnusedPictureBuffer(VIDPROD_Data_p->BufferManagerHandle, &GetPictureBufferParams, DecodedPicture_p_p, VIDCOM_VIDPROD_MODULE_BASE);
            if ((ErrorMain == ST_ERROR_NO_MEMORY) && VIDPROD_Data_p->ForTask.NeedForExtraAllocation)
            {
                ErrorCode = VIDBUFF_AllocateNewFrameBuffer(VIDPROD_Data_p->BufferManagerHandle, &(VIDPROD_Data_p->ForTask.NeedForExtraAllocation), &(VIDPROD_Data_p->ForTask.MaxSizeReachedForExtraAllocation));
                if (ErrorCode == ST_NO_ERROR)
                {
                    NewBufferAllocated = TRUE;
                    ErrorMain = VIDBUFF_GetAndTakeUnusedPictureBuffer(VIDPROD_Data_p->BufferManagerHandle, &GetPictureBufferParams, DecodedPicture_p_p, VIDCOM_VIDPROD_MODULE_BASE);
                }
            }
            if (ErrorMain != ST_NO_ERROR)
            {
                return(ErrorMain);
            }
            ErrorSecondary = VIDBUFF_GetAndTakeUnusedDecimatedPictureBuffer(VIDPROD_Data_p->BufferManagerHandle, &GetPictureBufferParams, DecodedDecimPicture_p_p, VIDCOM_VIDPROD_MODULE_BASE);
            if ((ErrorSecondary == ST_ERROR_NO_MEMORY) && VIDPROD_Data_p->ForTask.NeedForExtraAllocationDecimated)
            {
                ErrorCode = VIDBUFF_AllocateNewDecimatedFrameBuffer(VIDPROD_Data_p->BufferManagerHandle, &(VIDPROD_Data_p->ForTask.NeedForExtraAllocationDecimated), &(VIDPROD_Data_p->ForTask.MaxSizeReachedForExtraAllocationDecimated));
                if (ErrorCode == ST_NO_ERROR)
                {
                    ErrorSecondary = VIDBUFF_GetAndTakeUnusedDecimatedPictureBuffer(VIDPROD_Data_p->BufferManagerHandle, &GetPictureBufferParams, DecodedDecimPicture_p_p, VIDCOM_VIDPROD_MODULE_BASE);
                }
            }
            if (ErrorSecondary != ST_NO_ERROR)
            {
                /* The main picture has been taken, we must release it before to exit */
                UNTAKE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, (*DecodedPicture_p_p), VIDCOM_VIDPROD_MODULE_BASE);
                return(ErrorSecondary);
            }
            /* continue to linkage/reconstruction if OK */
        break;

        default:
            return(ST_ERROR_BAD_PARAMETER);
        break;
    }

    /* Update VIDPROD_Data_p->ForTask.DynamicAllocatedMainFrameBuffers in case of dynamic allocation */
    if ((VIDPROD_Data_p->ForTask.NeedForExtraAllocation) && (NewBufferAllocated))
    {
        VIDBUFF_GetAllocatedFrameNumbers(VIDPROD_Data_p->BufferManagerHandle, &MainBuffers, &DecimBuffers);
        VIDPROD_Data_p->ForTask.DynamicAllocatedMainFrameBuffers = MainBuffers;
    }

    /* Sanity control */
    if ( ((ErrorMain == ST_NO_ERROR) && (*DecodedPicture_p_p == NULL)) ||
         ((ErrorSecondary == ST_NO_ERROR) && (*DecodedDecimPicture_p_p == NULL)))
    {
#if defined(TRACE_UART) && !defined(VALID_TOOLS)
        TraceBuffer(("\r\nGetBuffers sanity check failed ErrMain=%d Pict=0x%x ErrDecim=%d Pict=0x%x",
            MPEGFrame2Char(PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame),
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID
            ));
#endif
        if (*DecodedPicture_p_p != NULL)
        {
            UNTAKE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, (*DecodedPicture_p_p), VIDCOM_VIDPROD_MODULE_BASE);
        }
        if (*DecodedDecimPicture_p_p != NULL)
        {
            UNTAKE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, (*DecodedDecimPicture_p_p), VIDCOM_VIDPROD_MODULE_BASE);
        }
        return(ST_ERROR_NO_MEMORY);
    }

    /* Patch the reconstruction because of possible substitution */
    if (*DecodedPicture_p_p != NULL)
    {
        (*DecodedPicture_p_p)->AssociatedDecimatedPicture_p = NULL;
        (*DecodedPicture_p_p)->FrameBuffer_p->AvailableReconstructionMode = VIDBUFF_MAIN_RECONSTRUCTION;
        (*DecodedPicture_p_p)->FrameBuffer_p->DecimationFactor = STVID_DECIMATION_FACTOR_NONE;
    }
    if (*DecodedDecimPicture_p_p != NULL)
    {
        (*DecodedDecimPicture_p_p)->AssociatedDecimatedPicture_p = NULL;
        if ((*DecodedDecimPicture_p_p)->FrameBuffer_p->AvailableReconstructionMode != VIDBUFF_SECONDARY_RECONSTRUCTION)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Decimated FB @ 0x%x not defined as VIDBUFF_SECONDARY_RECONSTRUCTION !!!",
                    (*DecodedDecimPicture_p_p)->FrameBuffer_p->Allocation.Address_p));
        }
    }
   /* linkage picture-decimated */
    if ((*DecodedDecimPicture_p_p != NULL) && (*DecodedPicture_p_p != NULL))
    {
        (*DecodedDecimPicture_p_p)->AssociatedDecimatedPicture_p = *DecodedPicture_p_p;
        (*DecodedPicture_p_p)->AssociatedDecimatedPicture_p = *DecodedDecimPicture_p_p;
   }

   return(ST_NO_ERROR);
} /* End of GetBuffers() function */

/*******************************************************************************
Name        : BumpPictureToDisplayQueue
Description : Find oldest (in ExtendedPresentationPictureID order)
              NeededForPresentation picture in DecodePicutre
              and post it to display queue then remove it from DecodePicture
              if it is not (or no more) a reference
Parameters  : Producer Handle

Assumptions :
Limitations :
Returns     : True if a picture has been posted, false if all pictures are
              "not needed for presentation"
*******************************************************************************/
static BOOL BumpPictureToDisplayQueue(const VIDPROD_Handle_t ProducerHandle)
{
    VIDPROD_Data_t *                        VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    VIDBUFF_PictureBuffer_t *               LoopPictureBuffer_p;
    VIDBUFF_PictureBuffer_t *               PictureBufferToPush_p;
    VIDCOM_PictureID_t                      OldestExtendedPresentationOrderPictureID;
    U32                                     OldestPictureNumber;
    U32                                     LoopCounter;
    ST_ErrorCode_t                          ErrorCode;
#if defined(VALID_TOOLS)
    VIDBUFF_PictureBuffer_t *               OtherFieldPictureBuffer_p;
    BOOL                                    AreBothFieldsPushed;
#endif /* VALID_TOOLS */
#if defined(VALID_TOOLS) && (defined(STVID_FRAME_BUFFER_DUMP) || defined(STVID_FB_ADD_DATA_DUMP) || defined(STVID_DECIMATED_FRAME_BUFFER_DUMP) )
    char    Filename_p[FILENAME_MAX_LENGTH];
#ifdef STVID_DECIMATED_FRAME_BUFFER_DUMP
    U32 H_DecimationFactor;
    U32 V_DecimationFactor;
#endif
#endif /* defined(VALID_TOOLS) && (defined(STVID_FRAME_BUFFER_DUMP) || defined(STVID_FB_ADD_DATA_DUMP)) */

    OldestExtendedPresentationOrderPictureID.ID = (S32)LONG_MAX;
    OldestExtendedPresentationOrderPictureID.IDExtension = (U32)ULONG_MAX;
    OldestPictureNumber = (U32)ULONG_MAX;
    PictureBufferToPush_p = NULL;
/* Look for the oldest picture (in display order) in DecodePicture Table */
#ifdef STVID_TRICKMODE_BACKWARD
    if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
	{
        for (LoopCounter = 0 ; LoopCounter < MAX_NUMBER_OF_DECODE_PICTURES ; LoopCounter++)
        {
            LoopPictureBuffer_p = VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter];
            if (LoopPictureBuffer_p != NULL)
            {
                if ((LoopPictureBuffer_p->IsNeededForPresentation) && (!LoopPictureBuffer_p->IsPushed))
                {
                    if (vidcom_ComparePictureID(&(LoopPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID), &(OldestExtendedPresentationOrderPictureID)) < 0)
                    {
                        PictureBufferToPush_p = LoopPictureBuffer_p;
                        OldestExtendedPresentationOrderPictureID = LoopPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID;
                        OldestPictureNumber = LoopPictureBuffer_p->PictureNumber;
                    }
                    else if (vidcom_ComparePictureID(&(LoopPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID), &(OldestExtendedPresentationOrderPictureID)) == 0)
                    {
                        /* Same PictureID: first check the presentationStartTime if available for both pictures */
                        if ((PictureBufferToPush_p != NULL) &&                                                              /* paranoid check... */
                            (LoopPictureBuffer_p->ParsedPictureInformation.PictureGenericData.IsPresentationStartTimeValid) &&
                            (PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.IsPresentationStartTimeValid))
                        {
                            if (STOS_time_after(PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.PresentationStartTime,
                                                LoopPictureBuffer_p->ParsedPictureInformation.PictureGenericData.PresentationStartTime))
                            {
                                PictureBufferToPush_p = LoopPictureBuffer_p;
                                OldestPictureNumber = LoopPictureBuffer_p->PictureNumber;
                            }
                        }
                        else
                        {
                            if(LoopPictureBuffer_p->PictureNumber < OldestPictureNumber)
                            {
                                PictureBufferToPush_p = LoopPictureBuffer_p;
                                OldestPictureNumber = LoopPictureBuffer_p->PictureNumber;

                            }
                        }
                        OldestExtendedPresentationOrderPictureID = LoopPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID;
                    }
                }
            }
        }
	}
#ifdef STVID_TRICKMODE_BACKWARD
	else
	{
		OldestExtendedPresentationOrderPictureID.ID = (S32)0;
    	OldestExtendedPresentationOrderPictureID.IDExtension = (U32)0;
    	OldestPictureNumber = (U32)0;

    	for (LoopCounter = 0 ; LoopCounter < MAX_NUMBER_OF_DECODE_PICTURES ; LoopCounter++)
    	{
            LoopPictureBuffer_p = VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter];
            if (LoopPictureBuffer_p != NULL)
        	{
                if ((LoopPictureBuffer_p->IsNeededForPresentation) && (!LoopPictureBuffer_p->IsPushed))
            	{
                    if (vidcom_ComparePictureID(&(LoopPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID), &(OldestExtendedPresentationOrderPictureID)) > 0)
                	{
                        PictureBufferToPush_p = LoopPictureBuffer_p;
                        OldestExtendedPresentationOrderPictureID = LoopPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID;
                        OldestPictureNumber = LoopPictureBuffer_p->PictureNumber;
                	}
                    else if (vidcom_ComparePictureID(&(LoopPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID), &(OldestExtendedPresentationOrderPictureID)) == 0)
                	{
                        if(LoopPictureBuffer_p->PictureNumber > OldestPictureNumber)
                    	{
                            PictureBufferToPush_p = LoopPictureBuffer_p;
                            OldestExtendedPresentationOrderPictureID = LoopPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID;
                            OldestPictureNumber = LoopPictureBuffer_p->PictureNumber;
                    	}
                	}
            	}
        	}
    	}
	}
#endif /* STVID_TRICKMODE_BACKWARD */
    if ((PictureBufferToPush_p != NULL) && (!IsIOnlyDecodeOnGoing(VIDPROD_Data_p->Skip)))
    {
/* Check if there is no older picture found by parser but not yet in DecodePicture table */
#ifdef STVID_TRICKMODE_BACKWARD
        if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
        {
        LoopCounter = 0;
        while ((LoopCounter < MAX_NUMBER_OF_PREPROCESSING_PICTURES) && (PictureBufferToPush_p != NULL))
        {
            if (VIDPROD_Data_p->PreprocessingPicture.IsPictureBufferUsed[LoopCounter])
            {
                if (VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].IsNeededForPresentation)
                {
                    if (vidcom_ComparePictureID(&(VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID), &(OldestExtendedPresentationOrderPictureID)) < 0)
                    { /* If there is an older picture in PreprocessingPicture, picture found in DecodePicture can't be sent to display yet */
                        PictureBufferToPush_p = NULL;
                    }
                }
            }
            LoopCounter++;
        }
    }
    }
    if (PictureBufferToPush_p != NULL)
    {
        if (PictureBufferToPush_p->Decode.PictureDecodingStatus != VIDBUFF_PICTURE_BUFFER_DECODED)
        {   /* oldest picture is not decoded, thus it can't be pushed to the display yet */
            PictureBufferToPush_p = NULL;
        }
    }
    if (PictureBufferToPush_p == NULL)
    {   /* No picture to bump found */
        return(FALSE);
    }
    else if (PictureBufferToPush_p->FrameBuffer_p == NULL)
    {   /* No FrameBuffer associated to the picture to bump. This is an unexpected error !*/
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Picture to bump (%d-%d/%d) has NULL frame buffer pointer (BUG-BUG-BUG)!!",
                PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
        return(FALSE);
    }
    else
    {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DISPLAY_INSERT, "pDispPush(%d-%d/%d) ",
                                    PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                    PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                    PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
#if defined(VALID_TOOLS)
        if (IsFramePicture(PictureBufferToPush_p))
        {
            AreBothFieldsPushed = TRUE;
        }
        else
        {
            if (PictureBufferToPush_p->FrameBuffer_p->FrameOrFirstFieldPicture_p != PictureBufferToPush_p)
            {
                OtherFieldPictureBuffer_p = PictureBufferToPush_p->FrameBuffer_p->FrameOrFirstFieldPicture_p;
            }
            else
            {
                OtherFieldPictureBuffer_p = PictureBufferToPush_p->FrameBuffer_p->NothingOrSecondFieldPicture_p;
            }
            if (OtherFieldPictureBuffer_p != NULL)
            {
                if ((PictureBufferToPush_p->PictureNumber == (OtherFieldPictureBuffer_p->PictureNumber+1)) ||
                    (PictureBufferToPush_p->PictureNumber == (OtherFieldPictureBuffer_p->PictureNumber-1)))
            {
                AreBothFieldsPushed = (!OtherFieldPictureBuffer_p->IsNeededForPresentation) || (OtherFieldPictureBuffer_p->IsPushed);
            }
            else
            {
                AreBothFieldsPushed = FALSE;
            }
        }
            else
            {
                AreBothFieldsPushed = FALSE;
            }
        }
#if defined(VALID_TOOLS) && defined(STVID_FRAME_BUFFER_DUMP)
        if(AreBothFieldsPushed)
        {
            sprintf(Filename_p, "../../decoded_frames/frame%03d_%03d_%03d_%03d",
                                PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                                PictureBufferToPush_p->PictureNumber);
            /* WARNING : If the avmem partition used is cached, the line below might not work properly.
             * vidprod_SavePicture access to memory should be replaced with forced uncached accesses */
            vidprod_SavePicture(OMEGA2_420, PictureBufferToPush_p->GlobalDecodingContext.GlobalDecodingContextGenericData.SequenceInfo.Width,
                                    PictureBufferToPush_p->GlobalDecodingContext.GlobalDecodingContextGenericData.SequenceInfo.Height,
                                    STAVMEM_DeviceToCPU(PictureBufferToPush_p->FrameBuffer_p->Allocation.Address_p, &VIDPROD_Data_p->VirtualMapping) /* LumaAddress */,
                                    STAVMEM_DeviceToCPU(PictureBufferToPush_p->FrameBuffer_p->Allocation.Address2_p, &VIDPROD_Data_p->VirtualMapping) /* ChromaAddress */,
                                    Filename_p);
        }
#endif /* defined(VALID_TOOLS) && defined(STVID_FRAME_BUFFER_DUMP) */
#if defined(VALID_TOOLS) && defined(STVID_FB_ADD_DATA_DUMP)
        if((AreBothFieldsPushed) &&
           (PictureBufferToPush_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.MPEGStandard == STVID_MPEG_STANDARD_ISO_IEC_14496))
        {
            sprintf(Filename_p, "../../decoded_frames/add_data%03d_%03d_%03d_%03d",
                                PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                                PictureBufferToPush_p->PictureNumber);
            /* WARNING : If the avmem partition used is cached, the line below might not work properly.
             * vidprod_SavePicture access to memory should be replaced with forced uncached accesses */
            vidprod_SaveAddData(PictureBufferToPush_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.ProfileAndLevelIndication,
                        PictureBufferToPush_p->GlobalDecodingContext.GlobalDecodingContextGenericData.SequenceInfo.Width,
                        PictureBufferToPush_p->GlobalDecodingContext.GlobalDecodingContextGenericData.SequenceInfo.Height,
                        STAVMEM_DeviceToCPU(PictureBufferToPush_p->PPB.Address_p, &VIDPROD_Data_p->VirtualMapping) /* AdditionnalDataAddress */,
                        Filename_p);
        }
#endif /* defined(VALID_TOOLS) && defined(STVID_FB_ADD_DATA_DUMP) */
#endif /* defined(VALID_TOOLS) */

#if COMPUTE_PRESENTATION_ID
        if (PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.IsExtendedPresentationOrderIDValid)
        {
#endif	/* #if COMPUTE_PRESENTATION_ID */
		/* Push the current picture to display */
        ErrorCode = VIDQUEUE_PushPicturesToDisplay(VIDPROD_Data_p->OrderingQueueHandle,
                                &(PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID));
        if (ErrorCode != ST_NO_ERROR)
        {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DISPLAY_INSERT, "pDispPushRefused(%d-%d/%d) Err=%d",
                                                 PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                                 PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                                 PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                                                 ErrorCode));
#endif
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDQ_Push refused Error (%d-%d/%d) error %d",
                            PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                            PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                            PictureBufferToPush_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                            ErrorCode));
        }
        /* As picture is pushed to the display queue it is considered as already presented */
        PictureBufferToPush_p->IsNeededForPresentation = FALSE;
        if (PictureBufferToPush_p->AssociatedDecimatedPicture_p != NULL)
        {
            PictureBufferToPush_p->AssociatedDecimatedPicture_p->IsNeededForPresentation = FALSE;
        }

        /* We can release and remove from the DecodePicture table the picture if it is a non reference picture as it has been pushed */
        if (!PictureBufferToPush_p->IsReferenceForDecoding)
        {

            RemovePictureFromDecodePictureTable(ProducerHandle, PictureBufferToPush_p, __LINE__);
            /* We can release the non ref decimated picture if exists right now as it is has been pushed */
            if (PictureBufferToPush_p->AssociatedDecimatedPicture_p != NULL)
            {
                RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, PictureBufferToPush_p->AssociatedDecimatedPicture_p, VIDCOM_VIDPROD_MODULE_BASE);
            }
            RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, PictureBufferToPush_p, VIDCOM_VIDPROD_MODULE_BASE);
        }
#if COMPUTE_PRESENTATION_ID
        }
        else /* if the IsExtendedPresentationOrderIDValid of current picture is Invalid */
        {
            /* if it is not the first picture */
            if (VIDPROD_Data_p->NumOfPictToWaitBeforePush == 0)
            {
                /* If the IsExtendedPresentationOrderIDValid of previous picture is Valid */
                if (VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->ParsedPictureInformation.PictureGenericData.IsExtendedPresentationOrderIDValid &&
                    ((VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->IsNeededForPresentation) && (!VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->IsPushed)))
                {
				/* Push the previous picture to display */
                ErrorCode = VIDQUEUE_PushPicturesToDisplay(VIDPROD_Data_p->OrderingQueueHandle,
                                  &(VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID));
                if (ErrorCode != ST_NO_ERROR)
                {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DISPLAY_INSERT, "pDispPushRefused(%d-%d/%d) Err=%d",
                                                 VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                                 VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                                 VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                                                 ErrorCode));
#endif
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDQ_Push refused Error (%d-%d/%d) error %d",
                            VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                            VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                            VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                            ErrorCode));
                }
                /* As picture is pushed to the display queue it is considered as already presented */
                VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->IsNeededForPresentation = FALSE;
                if (VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->AssociatedDecimatedPicture_p != NULL)
                    {
                        VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->AssociatedDecimatedPicture_p->IsNeededForPresentation = FALSE;
                    }
                    /* We can release and remove from the DecodePicture table the picture if it is a non reference picture as it has been pushed */
                    if (!VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->IsReferenceForDecoding)
                    {

                        RemovePictureFromDecodePictureTable(ProducerHandle, VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount], __LINE__);
                        /* We can release the non ref decimated picture if exists right now as it is has been pushed */
                        if (VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->AssociatedDecimatedPicture_p != NULL)
                        {
                            RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount]->AssociatedDecimatedPicture_p, VIDCOM_VIDPROD_MODULE_BASE);
                        }
                        RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, VIDPROD_Data_p->StoredPictureBuffer[VIDPROD_Data_p->PreviousPictureCount], VIDCOM_VIDPROD_MODULE_BASE);
                    }
                }
                else
                {
                    return(FALSE);
                }
            }
            else
            {
                return(FALSE);
            }
        }
#endif /* #if COMPUTE_PRESENTATION_ID */

        return(TRUE);
    }
} /* End of BumpPictureToDisplayQueue() function */

/*******************************************************************************
Name        : CopyPictureBuffer
Description : Copy a picturebuffer to another one, fixing pointers
Parameters  : Destination PictureBuffer pointer
              Source PictureBuffer pointer
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void CopyPictureBuffer(VIDBUFF_PictureBuffer_t * DestPicture_p, VIDBUFF_PictureBuffer_t * SrcPicture_p)
{
    VIDBUFF_FrameBuffer_t *     FrameBuffer_p;
    VIDBUFF_PictureBuffer_t *   AssociatedDecimatedPicture_p;

    /* Save initial fields values which must be maintained in DestPicture_p */
    FrameBuffer_p = DestPicture_p->FrameBuffer_p;
    AssociatedDecimatedPicture_p = DestPicture_p->AssociatedDecimatedPicture_p;

    /* copy */
    STOS_memcpy(DestPicture_p, SrcPicture_p, sizeof(VIDBUFF_PictureBuffer_t));

    /* Restore initial fields values which must be maintained */
    DestPicture_p->FrameBuffer_p = FrameBuffer_p;
    DestPicture_p->AssociatedDecimatedPicture_p = AssociatedDecimatedPicture_p;

    /* Update pointers */
    DestPicture_p->ParsedPictureInformation.PictureSpecificData_p = &(DestPicture_p->PictureSpecificData);
    DestPicture_p->ParsedPictureInformation.PictureDecodingContext_p = &(DestPicture_p->PictureDecodingContext);
    DestPicture_p->ParsedPictureInformation.PictureDecodingContext_p->PictureDecodingContextSpecificData_p = &(DestPicture_p->PictureDecodingContextSpecificData);
    DestPicture_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p = &(DestPicture_p->GlobalDecodingContext);
    DestPicture_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p = &(DestPicture_p->GlobalDecodingContextSpecificData);
} /* End of CopyPictureBuffer() function */

/*******************************************************************************
Name        : vidprod_UpdatePictureParsedPictureInformation
Description : Copy a ParsedPictureInformation structure to update a picturebuffer
Parameters  : Destination PictureBuffer pointer
              Source ParsedPictureInformation pointer
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void vidprod_UpdatePictureParsedPictureInformation(VIDBUFF_PictureBuffer_t * PictureBuffer_p, VIDCOM_ParsedPictureInformation_t * ParsedPictureInformation_p)
{
    U32 SizeOfPictureSpecificDataInByte;
    U32 SizeOfPictureDecodingContextSpecificDataInByte;
    U32 SizeOfGlobalDecodingContextSpecificDataInByte;

    SizeOfPictureSpecificDataInByte = ParsedPictureInformation_p->SizeOfPictureSpecificDataInByte;
    if (SizeOfPictureSpecificDataInByte > VIDCOM_MAX_PICTURE_SPECIFIC_DATA_SIZE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Producer ParserJobCompletedCB : BUG !! PictureSpecificData too large for table Actual Size is %d, Allowed size is %d",
                                                SizeOfPictureSpecificDataInByte,
                                                VIDCOM_MAX_PICTURE_SPECIFIC_DATA_SIZE));
        SizeOfPictureSpecificDataInByte = VIDCOM_MAX_PICTURE_SPECIFIC_DATA_SIZE;
    }
    PictureBuffer_p->ParsedPictureInformation.SizeOfPictureSpecificDataInByte = SizeOfPictureSpecificDataInByte;
    PictureBuffer_p->ParsedPictureInformation.PictureSpecificData_p = &(PictureBuffer_p->PictureSpecificData);
    STOS_memcpy(PictureBuffer_p->ParsedPictureInformation.PictureSpecificData_p, ParsedPictureInformation_p->PictureSpecificData_p,
                SizeOfPictureSpecificDataInByte);

    /*PictureBuffer_p->ParsedPictureInformation.PictureGenericData_p = &(PictureBuffer_p->ParsedPictureInformation.PictureGenericData);*/
    STOS_memcpy(&(PictureBuffer_p->ParsedPictureInformation.PictureGenericData), &(ParsedPictureInformation_p->PictureGenericData), sizeof(VIDCOM_PictureGenericData_t));
    PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p = &(PictureBuffer_p->PictureDecodingContext);

    SizeOfPictureDecodingContextSpecificDataInByte = ParsedPictureInformation_p->PictureDecodingContext_p->SizeOfPictureDecodingContextSpecificDataInByte;
    if (SizeOfPictureDecodingContextSpecificDataInByte > VIDCOM_MAX_PICTURE_DECODING_CONTEXT_SPECIFIC_DATA_SIZE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Producer ParserJobCompletedCB : BUG !! PictureDecodingContextSpecificData too large for table Actual Size is %d, Allowed size is %d",
                                                SizeOfPictureDecodingContextSpecificDataInByte,
                                                VIDCOM_MAX_PICTURE_DECODING_CONTEXT_SPECIFIC_DATA_SIZE));
        SizeOfPictureDecodingContextSpecificDataInByte = VIDCOM_MAX_PICTURE_DECODING_CONTEXT_SPECIFIC_DATA_SIZE;
    }
    PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->SizeOfPictureDecodingContextSpecificDataInByte = SizeOfPictureDecodingContextSpecificDataInByte;
    PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->PictureDecodingContextSpecificData_p = &(PictureBuffer_p->PictureDecodingContextSpecificData);
    STOS_memcpy(PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->PictureDecodingContextSpecificData_p,
                ParsedPictureInformation_p->PictureDecodingContext_p->PictureDecodingContextSpecificData_p,
                SizeOfPictureDecodingContextSpecificDataInByte);

    PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p = &(PictureBuffer_p->GlobalDecodingContext);
    STOS_memcpy(&(PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData),
                &(ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData),
                sizeof(VIDCOM_GlobalDecodingContextGenericData_t));

    SizeOfGlobalDecodingContextSpecificDataInByte = ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->SizeOfGlobalDecodingContextSpecificDataInByte;
    if (SizeOfGlobalDecodingContextSpecificDataInByte > VIDCOM_MAX_GLOBAL_DECODING_CONTEXT_SPECIFIC_DATA_SIZE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Producer ParserJobCompletedCB : BUG !! GlobalDecodingContextSpecificData too large for table Actual Size is %d, Allowed size is %d",
                                                SizeOfGlobalDecodingContextSpecificDataInByte,
                                                VIDCOM_MAX_GLOBAL_DECODING_CONTEXT_SPECIFIC_DATA_SIZE));
        SizeOfGlobalDecodingContextSpecificDataInByte = VIDCOM_MAX_GLOBAL_DECODING_CONTEXT_SPECIFIC_DATA_SIZE;
    }
    PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->SizeOfGlobalDecodingContextSpecificDataInByte = SizeOfGlobalDecodingContextSpecificDataInByte;
    PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p = &(PictureBuffer_p->GlobalDecodingContextSpecificData);
    STOS_memcpy(PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p,
                ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p,
                SizeOfGlobalDecodingContextSpecificDataInByte);
} /* end of vidprod_UpdatePictureParsedPictureInformation() function */


/*******************************************************************************
Name        : GetFreePreprocessingPictureBuffer
Description : Find a free Picture Buffer in PreprocessingPicture table
Parameters  : VIdeo producer handle
              OUT : pointer on free PictureBuffer or NULL if not found
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void GetFreePreprocessingPictureBuffer(const VIDPROD_Handle_t ProducerHandle,
                                              VIDBUFF_PictureBuffer_t **PictureBuffer_p_p)
{
    VIDPROD_Data_t *    VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    U32                 LoopCounter;

    /* Lock Access to Preprocessing Pictures */
    STOS_SemaphoreWait(VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureSemaphore_p);

    *PictureBuffer_p_p = NULL;
    LoopCounter = 0;

    do
    {
        if (!VIDPROD_Data_p->PreprocessingPicture.IsPictureBufferUsed[LoopCounter])
        {
            *PictureBuffer_p_p = &(VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter]);
            VIDPROD_Data_p->PreprocessingPicture.IsPictureBufferUsed[LoopCounter] = TRUE;
            VIDPROD_Data_p->PreprocessingPicture.IsPictureTimedOut[LoopCounter] = FALSE;
            VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_EMPTY;
            VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId = 0;
            VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.DecodingError = VIDCOM_ERROR_LEVEL_NONE;
            VIDPROD_Data_p->PreprocessingPicture.NbUsedPictureBuffers++;
        }
        LoopCounter++;
    } while ((*PictureBuffer_p_p == NULL) &&
             (LoopCounter < MAX_NUMBER_OF_PREPROCESSING_PICTURES));
    /* UnLock Access to Preprocessing Pictures */
    STOS_SemaphoreSignal(VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureSemaphore_p);

}  /* End of GetFreePreprocessingPictureBuffer() function */
#ifdef STVID_TRICKMODE_BACKWARD
/*******************************************************************************
Name        : GetFreePreprocessingPictureBufferBackward
Description : Find a free Picture Buffer in PreprocessingPicture table
Parameters  : VIdeo producer handle
              OUT : pointer on free PictureBuffer or NULL if not found
Assumptions : Must be called protected by VIDPROD_Data_p->LinearBitBuffers.PreprocessingPictureSemaphore_p semaphore
Limitations :
Returns     : Nothing
*******************************************************************************/
static ST_ErrorCode_t GetFreePreprocessingPictureBufferBackward(const VIDPROD_Handle_t ProducerHandle,
                                              VIDBUFF_PictureBuffer_t **PictureBuffer_p_p, U32 DecodingOrderID)
{
    U32                 LoopCounter;
    BOOL                AlreadyInTable = FALSE;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    VIDPROD_Data_t *    VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;


    *PictureBuffer_p_p = NULL;
    LoopCounter = 0;

    do
    {
        if (!VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.IsPictureBufferUsed[LoopCounter])
        {
            if (LoopCounter > 0)
            {
            if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter-1].ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID
               != DecodingOrderID)
            {
                *PictureBuffer_p_p = &(VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter]);
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.IsPictureBufferUsed[LoopCounter] = TRUE;
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.IsPictureTimedOut[LoopCounter] = FALSE;
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_EMPTY;
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId = 0;
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.DecodingError = VIDCOM_ERROR_LEVEL_NONE;
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.NbUsedPictureBuffers++;
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.NbParsedPicturesLeft++;
            }
            else
            {
                AlreadyInTable = TRUE;
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->NbIPicturesInBuffer --;
                VIDPROD_Data_p->ParserStatus.IsSearchingPicture = FALSE;
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            }
            else
            {
                *PictureBuffer_p_p = &(VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter]);
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.IsPictureBufferUsed[LoopCounter] = TRUE;
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.IsPictureTimedOut[LoopCounter] = FALSE;
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_EMPTY;
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.CommandId = 0;
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.DecodingError = VIDCOM_ERROR_LEVEL_NONE;
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.NbUsedPictureBuffers++;
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.NbParsedPicturesLeft++;

            }
        }
        LoopCounter++;
    } while ((*PictureBuffer_p_p == NULL) &&
            (LoopCounter < MAX_BACKWARD_PICTURES) && (!AlreadyInTable));

    if ((LoopCounter == MAX_BACKWARD_PICTURES) && (*PictureBuffer_p_p == NULL))
    {
        ErrorCode = ST_ERROR_NO_MEMORY;
    }

    return(ErrorCode);

}  /* End of GetFreePreprocessingPictureBufferBackward() function */
#endif /* STVID_TRICKMODE_BACKWARD */

/*******************************************************************************
Name        : AddPictureToDecodePictureTable
Description : Adds a PictureBuffer pointer in currently used by decoder picture list
Parameters  : VIdeo producer handle
              PictureBuffer pointer to add to list
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void AddPictureToDecodePictureTable(const VIDPROD_Handle_t ProducerHandle,
                                           VIDBUFF_PictureBuffer_t *PictureBuffer_p)
{
    VIDPROD_Data_t *    VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    U32                 LoopCounter;
    BOOL                FoundAPlace;
    BOOL                AlreadyInList;


    /* Lock Access to Decode Pictures */
    STOS_SemaphoreWait(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);

#if defined(TRACE_UART) && defined(VALID_TOOLS)
    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DPB_MANAGEMENT, "pAddToList%d(%d-%d/%d) ", PictureBuffer_p->Decode.CommandId, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
#if defined(TRACE_UART) && !defined(VALID_TOOLS)
/*    TraceBuffer(("\r\npAddToList 0x%x %d(%d-%d/%d) %dx%d", PictureBuffer_p, PictureBuffer_p->Decode.CommandId, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,*/
/*            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Width,*/
/*            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Height*/
/*            ));*/
#endif

    AlreadyInList = FALSE;
    for(LoopCounter=0 ; LoopCounter < MAX_NUMBER_OF_DECODE_PICTURES ; LoopCounter++)
    {
        if (PictureBuffer_p == VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter])
        {

#if defined(TRACE_UART) && defined(VALID_TOOLS)
    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DPB_MANAGEMENT, "pDblToList%d(%d-%d/%d) ", PictureBuffer_p->Decode.CommandId, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "picture 0x%x (%d-%d/%d) is already in list rank %d (normal _only_ if it was on display while the producer restarted)",
                (U32)PictureBuffer_p,
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                    LoopCounter));
            AlreadyInList = TRUE;
        }
    }

    if (!AlreadyInList)
    {
        LoopCounter = 0;
        FoundAPlace = FALSE;
        do
        {
            if (VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter] == NULL)
            {
                VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter] = PictureBuffer_p;
                VIDPROD_Data_p->DecodePicture.NbUsedPictureBuffers++;
                FoundAPlace = TRUE;
            }
            LoopCounter++;
        } while ((LoopCounter < MAX_NUMBER_OF_DECODE_PICTURES) &&
                !FoundAPlace);

    #if defined(TRACE_UART) && defined(VALID_TOOLS)
        {
            U32 i;
            U32 NbPictures;
            char    TmpString[512];
            VIDBUFF_PictureBuffer_t *TmpPictureBuffer_p;

            TmpString[0] = 0;
            NbPictures = 0;
            for(i=0 ; i < MAX_NUMBER_OF_DECODE_PICTURES ; i++)
            {
                if (VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[i] != NULL)
                {
                    NbPictures++;
                    TmpPictureBuffer_p = VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[i];
                    sprintf(TmpString, "%s(%d-%d/%d)", TmpString, TmpPictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, TmpPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, TmpPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID);
                    if (TmpPictureBuffer_p->FrameBuffer_p == NULL)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "AddPictureToDecodePictureTable : FB NULL for picture (%d-%d/%d) (BUG-BUG-BUG)",
                            TmpPictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                            TmpPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                            TmpPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
                    }
                }
            }
            TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DPB_STATUS, "pDPB(%d)=%s ", NbPictures, TmpString));
            TraceGraph(("DPB", "%d ", NbPictures));
        }
#endif /*  defined(TRACE_UART) && defined(VALID_TOOLS) */
    } /* !AlreadyInList */
    else
    {
        FoundAPlace = TRUE;
    }
    /* UnLock Access to Decode Pictures */
    STOS_SemaphoreSignal(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);

    if (!FoundAPlace)
    {
/* TODO_PC (AddPictureToDecodePictureTable) : Add no room left error management */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "AddPictureToDecodePictureTable : No room in DecodePicture list for (%d-%d/%d) (BUG ?)",
                PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
    }
}  /* End of AddPictureToDecodePictureTable() function */

/*******************************************************************************
Name        : RemovePictureFromDecodePictureTable
Description : Removes the given picture from Decode picture list
Parameters  : Video producer handle
              Pointer to PictureBuffer to remove from list
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void RemovePictureFromDecodePictureTable(const VIDPROD_Handle_t ProducerHandle,
                                                VIDBUFF_PictureBuffer_t *PictureBuffer_p, U32 CallLine)
{
    VIDPROD_Data_t *            VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    U32                         LoopCounter;
    BOOL                        FoundPicture;

    UNUSED_PARAMETER(CallLine);

    /* Lock Access to Decode Pictures */
    STOS_SemaphoreWait(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);

#if defined(TRACE_UART) && defined(VALID_TOOLS)
    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DPB_MANAGEMENT, "pRemFromList%d(%d-%d/%d) ", PictureBuffer_p->Decode.CommandId, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
    TraceGraph(("DPBm"));
#endif

#if defined(TRACE_UART) && !defined(VALID_TOOLS)
    /*TraceBuffer(("\r\npRemFromList 0x%x %d PPB.fc=%d lckcnt=%d", PictureBuffer_p, CallLine, PictureBuffer_p->PPB.FieldCounter, PictureBuffer_p->Buffers.PictureLockCounter));*/
#endif

    LoopCounter = 0;
    FoundPicture = FALSE;
    do
    {
        if (VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter] == PictureBuffer_p)
        {
            VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_EMPTY;
            VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->IsReferenceForDecoding = FALSE;
            if (VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->AssociatedDecimatedPicture_p != NULL)
            {
                VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->AssociatedDecimatedPicture_p->Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_EMPTY;
                VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->IsReferenceForDecoding = FALSE;
            }
            VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter] = NULL;
            VIDPROD_Data_p->DecodePicture.IsPictureTimedOut[LoopCounter] = FALSE;
            VIDPROD_Data_p->DecodePicture.NbUsedPictureBuffers--;
            FoundPicture = TRUE;
        }
        LoopCounter++;
    } while ((LoopCounter < MAX_NUMBER_OF_DECODE_PICTURES) &&
             !FoundPicture);

#if defined(TRACE_UART) && defined(VALID_TOOLS)
    if (VIDPROD_Data_p->ProducerState != VIDPROD_DECODER_STATE_STOPPED)
    {
        U32 i;
        U32 NbPictures;
        char    TmpString[512];
        VIDBUFF_PictureBuffer_t *TmpPictureBuffer_p;

        TmpString[0] = 0;
        NbPictures = 0;
        for(i=0 ; i < MAX_NUMBER_OF_DECODE_PICTURES ; i++)
        {
            if (VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[i] != NULL)
            {
                NbPictures++;
                TmpPictureBuffer_p = VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[i];
                sprintf(TmpString, "%s(%d-%d/%d)", TmpString, TmpPictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, TmpPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, TmpPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID);
                if ((TmpPictureBuffer_p->FrameBuffer_p == NULL) && (!VIDPROD_Data_p->Stop.IsPending))
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "RemovePictureFromDecodePictureTable : FB NULL for picture (%d-%d/%d) (BUG-BUG-BUG)",
                        TmpPictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                        TmpPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                        TmpPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
                }
            }
        }
        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DPB_STATUS, "pDPB(%d)=%s ", NbPictures, TmpString));
        TraceGraph(("DPB", "%d ", NbPictures));
    }
#endif

    /* UnLock Access to Decode Pictures */
    STOS_SemaphoreSignal(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);

    if (!FoundPicture)
    {
/* TODO_PC (RemovePictureFromDecodePictureTable) : Add picture not found error management */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Producer RemovePictureFromDecodePictureTable called from %d : picture %d(%d-%d/%d) not found in decode picture list (BUG-BUG-BUG)",
                CallLine,
                PictureBuffer_p->Decode.CommandId, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
    }
}  /* End of RemovePictureFromDecodePictureTable() function */

/*******************************************************************************
Name        : UpdateReferenceList
Description : Removes pictures that are no more reference for PictureBuffer_p from Decode picture list
              This must be called prior to Get_Buffers() because a frame buffer may be freed by UpdateReferenceList()
Parameters  : Video producer handle
              PictureBuffer pointer to last decoded picture
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void UpdateReferenceList(const VIDPROD_Handle_t ProducerHandle,
                                VIDBUFF_PictureBuffer_t *PictureBuffer_p)
{
    VIDPROD_Data_t *            VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    U32                         PictureCounter;
    U32                         ReferenceCounter;
    BOOL                        FoundAsReference;
    VIDBUFF_PictureBuffer_t     *RefPictureBuffer_p;
    U32                         DPB_ref_count = 0;        /* number of references in the DPB */

    for(PictureCounter = 0 ; PictureCounter < MAX_NUMBER_OF_DECODE_PICTURES ; PictureCounter++)
    {
        RefPictureBuffer_p = VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[PictureCounter];
        if (RefPictureBuffer_p != NULL)

        {
#ifdef STVID_TRICKMODE_BACKWARD
            if (((RefPictureBuffer_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_DECODED) &&
              (RefPictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID < PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID)
              && (!VIDPROD_Data_p->Backward.IsBackward)) ||
              ((RefPictureBuffer_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_DECODED) &&
              (RefPictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID > PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID)
              && (VIDPROD_Data_p->Backward.IsBackward) && (VIDPROD_Data_p->Skip.AllPPictures) && (VIDPROD_Data_p->Skip.AllBPictures)))
#else
            if ((RefPictureBuffer_p->IsReferenceForDecoding) &&
                (RefPictureBuffer_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_DECODED) &&
                (RefPictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID < PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID))
#endif /* STVID_TRICKMODE_BACKWARD */
            {
                FoundAsReference = FALSE;
                ReferenceCounter = 0;
                /* If we decode I only pictures we can reset reference list ASAP */
                if (!IsIOnlyDecodeOnGoing(VIDPROD_Data_p->Skip))
                {
                	do
                	{
                    	if ((PictureBuffer_p->ParsedPictureInformation.PictureGenericData.IsValidIndexInReferenceFrame[ReferenceCounter]) &&
                            (RefPictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID == PictureBuffer_p->ParsedPictureInformation.PictureGenericData.FullReferenceFrameList[ReferenceCounter]))
                    	{
                        	FoundAsReference = TRUE;
                    	}
                    	ReferenceCounter++;
                    } while ((ReferenceCounter < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES) && (!FoundAsReference));
                }
                if (!FoundAsReference)
                {
                    RefPictureBuffer_p->IsReferenceForDecoding = FALSE;
                    /* Remove from the decoded picture list only if the picture has been displayed (i.e. not inserted or already pushed)
                     * if we don't check this picture will stay locked in ordering Q and never pushed */
                    if ((!RefPictureBuffer_p->IsNeededForPresentation) || (RefPictureBuffer_p->IsPushed))
                    {

                        RemovePictureFromDecodePictureTable(ProducerHandle, RefPictureBuffer_p, __LINE__);
                        if (RefPictureBuffer_p->AssociatedDecimatedPicture_p != NULL)
                        {
                            RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, RefPictureBuffer_p->AssociatedDecimatedPicture_p, VIDCOM_VIDPROD_MODULE_BASE);
                        }
                        RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, RefPictureBuffer_p, VIDCOM_VIDPROD_MODULE_BASE);
                    }
                }
                else
                {
                    DPB_ref_count++;
                }
            }
        }
    }

    /* Once we have decoded enough references, we can remove the replacement one */
    if ((DPB_ref_count >= PictureBuffer_p->GlobalDecodingContext.GlobalDecodingContextGenericData.NumberOfReferenceFrames) &&
         (VIDPROD_Data_p->ReplacementReference_IsAvailable))
    {
        if (IsRequestingAdditionnalData(VIDPROD_Data_p))
        {
            VIDBUFF_FreeAdditionnalDataBuffer(VIDPROD_Data_p->BufferManagerHandle, VIDPROD_Data_p->ReplacementReference_p);
        }

   /* VIDBUFF_GetPictureLockStatus(VIDPROD_Data_p->BufferManagerHandle, &PictureLockStatus, VIDPROD_Data_p->ReplacementReference_p);
        PictureLockStatus = VIDBUFF_PICTURE_LOCK_STATUS_UNLOCKED;
       VIDBUFF_SetPictureLockStatus(VIDPROD_Data_p->BufferManagerHandle, PictureLockStatus, VIDPROD_Data_p->ReplacementReference_p);*/
		/* OLGN check if correct !! */
		RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, VIDPROD_Data_p->ReplacementReference_p, VIDCOM_VIDPROD_MODULE_BASE);
        VIDPROD_Data_p->ReplacementReference_IsAvailable = FALSE;
    }
}  /* End of UpdateReferenceList() function */

/*******************************************************************************
Name        : ResetReferenceList
Description : Removes all reference pictures from Decode picture list
              This must be called in DoStopNow()
Parameters  : Video producer handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void ResetReferenceList(const VIDPROD_Handle_t ProducerHandle)
{
    VIDPROD_Data_t *            VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    U32                         PictureCounter;
    VIDBUFF_PictureBuffer_t    *CurrentPicture_p;

    for(PictureCounter = 0 ; PictureCounter < MAX_NUMBER_OF_DECODE_PICTURES ; PictureCounter++)
    {
        CurrentPicture_p = VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[PictureCounter];
        if ((CurrentPicture_p != NULL) && (CurrentPicture_p->IsReferenceForDecoding))
        {
            RemovePictureFromDecodePictureTable(ProducerHandle, CurrentPicture_p, __LINE__);
            if (CurrentPicture_p->AssociatedDecimatedPicture_p != NULL)
            {
                RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, CurrentPicture_p->AssociatedDecimatedPicture_p, VIDCOM_VIDPROD_MODULE_BASE);
            }
            RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, CurrentPicture_p, VIDCOM_VIDPROD_MODULE_BASE);
        }
    }
}  /* End of ResetReferenceList() function */

/*******************************************************************************
Name        : VIDPROD_DataInjectionCompleted
Description : Inform decoder that data injection is completed
Parameters  : Video producer handle, params
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t vidprod_DataInjectionCompleted(const VIDPROD_Handle_t ProducerHandle, const STVID_DataInjectionCompletedParams_t * const Params_p)
{
    /* Acknowledge data injection completed even before checking parameters */
    VIDPROD_Data_t      * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

    VIDPROD_Data_p->DataUnderflowEventNotSatisfied = FALSE;

#if defined(TRACE_UART) && !defined(VALID_TOOLS)
    TraceBuffer(("Data injection completed \r\n"));
#endif

    if (Params_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

#ifdef STVID_TRICKMODE_BACKWARD
    if (VIDPROD_Data_p->Backward.IsBackward)
    {
    	/* Find the buffer which just has been transfered */
    	if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p == NULL)
    	{
        	/* if LinearBitBufferJustTransfered_p == NULL it means that the current data injection is an injection */
        	/* required by decode by itself and that smooth backward is under initializing. No more UNDERFLOW events */
        	/* should be required by decode. we can ask buffer A to be flushed */
            vidprod_AskForLinearBitBufferFlush(ProducerHandle, &(VIDPROD_Data_p->LinearBitBuffers.BufferA), VIDPROD_FLUSH_BBFSIZE_BACKWARD_JUMP);
            /*STOS_SemaphoreSignal(VIDTRICK_Data_p->TrickModeParamsSemaphore_p);*/
        	return (ST_NO_ERROR);
     	}
        /* Add a picture start code just above the end address of Transfered Data buffer */
        /* if PutPictureStartCodeAndComputeValidDataBuffer returns FALSE, there is a problem */
        /* reading size of just flushed buffer */
        if (VIDPROD_Data_p->Backward.ChangingDirection != TRICKMODE_CHANGING_FORWARD)
        {
            if (! PutPictureStartCodeAndComputeValidDataBuffer( ProducerHandle,
                                                                VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p,
                                                                Params_p->TransferRelatedToPrevious))
            {
#if defined(TRACE_UART) && !defined(VALID_TOOLS)
                TraceBuffer(("Data injection ERROR \r\n"));
#endif
                return(ST_ERROR_BAD_PARAMETER);
            }
        }
    }
#endif

    return(ST_NO_ERROR);

} /* End of vidprod_DataInjectionCompleted() function */

/*******************************************************************************
Name        : vidprod_FlushInjection
Description : Flush decoder injected data
Parameters  : Video producer handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t vidprod_FlushInjection(const VIDPROD_Handle_t ProducerHandle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
#ifdef STVID_TRICKMODE_BACKWARD
    if (((VIDPROD_Data_p->Backward.InjectDiscontinuityCanBeAccepted) && (VIDPROD_Data_p->Backward.IsBackward)) || (!VIDPROD_Data_p->Backward.IsBackward))
#endif
    {
        PARSER_FlushInjection(VIDPROD_Data_p->ParserHandle);
    }
#ifdef STVID_TRICKMODE_BACKWARD
    if (VIDPROD_Data_p->Backward.IsBackward)
    {
        VIDPROD_Data_p->Backward.InjectDiscontinuityCanBeAccepted = FALSE;
    }
#endif

    return(ErrorCode);
} /* End of vidprod_FlushInjection() function */

/*******************************************************************************
Name        : vidprod_Freeze
Description : Informs that Decoder is Freezed.
Parameters  : Video producer handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t vidprod_Freeze(const VIDPROD_Handle_t ProducerHandle)
{
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVID_DecodedPictures_t CurrentDecodedPictures;
    STVID_DecodedPictures_t DecodedPicturesToApply;
    VIDPROD_Data_t      * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

    /* inits */
    CurrentDecodedPictures = VIDPROD_Data_p->DecodedPictures;

    DecodedPicturesToApply = CurrentDecodedPictures;

#if 0
    switch (CurrentDecodedPictures)
    {
        case STVID_DECODED_PICTURES_I :
            if (VIDPROD_Data_p->ForTask.CurrentProfile.NbMainFrameStore == 1)
            {
                return (ST_ERROR_NO_MEMORY);
            }
            break;

        case STVID_DECODED_PICTURES_IP :
            if (VIDPROD_Data_p->ForTask.CurrentProfile.NbMainFrameStore == 2)  /* can not be less than 2 */
            {
                DecodedPicturesToApply = STVID_DECODED_PICTURES_I;
            }
            break;

        case STVID_DECODED_PICTURES_ALL :
            if (VIDPROD_Data_p->ForTask.CurrentProfile.NbMainFrameStore == 3)  /* can not be less than 3 */
            {
                DecodedPicturesToApply = STVID_DECODED_PICTURES_IP;
            }
            break;

        default :
            /* Unrecognised mode: don't take it into account ! */
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
#else
    /* For now don't change decoded pictures filtering for freeze mode, this need to be refined according to MAXDPBSize & profile */
    DecodedPicturesToApply = CurrentDecodedPictures;
#endif

    if (ErrorCode == ST_NO_ERROR)
    {
        if (DecodedPicturesToApply != CurrentDecodedPictures)
        {
            ErrorCode = vidprod_SetDecodedPictures(ProducerHandle, DecodedPicturesToApply);
        }
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        /* remember decoded pictures state for next resume */
        VIDPROD_Data_p->DecodedPicturesBeforeLastFreeze = CurrentDecodedPictures;

        /* Freezing */
        VIDPROD_Data_p->DecoderIsFreezed = TRUE;

        /* Cancel DecodeOnce if no more running */
        VIDPROD_Data_p->DecodeOnce = FALSE;
    }

    return (ErrorCode);
} /* End of vidprod_Freeze() function */

/*******************************************************************************
Name        : vidprod_GetBitBufferFreeSize
Description : Get the bit buffer free size.
Parameters  : Video producer handle, pointer on U32 to fill
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if bad parameter
              STVID_ERROR_NOT_AVAILABLE if can not give this information
*******************************************************************************/
ST_ErrorCode_t vidprod_GetBitBufferFreeSize(const VIDPROD_Handle_t ProducerHandle, U32 * const FreeSize_p)
{
    VIDPROD_Data_t *    VIDPROD_Data_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

    if (VIDPROD_Data_p->ProducerState == VIDPROD_DECODER_STATE_STOPPED)
    {
        *FreeSize_p = VIDPROD_Data_p->BitBuffer.TotalSize;
    }
    else
    {
        *FreeSize_p = VIDPROD_Data_p->BitBuffer.TotalSize - VIDPROD_Data_p->LastBitBufferLevel;
    }

    return(ErrorCode);
} /* End of vidprod_GetBitBufferFreeSize() function */

/*******************************************************************************
Name        : vidprod_GetDecodedPictures
Description : Get the picture type of the decoded picture.
Parameters  : Video producer handle, pointer to fill.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t vidprod_GetDecodedPictures(const VIDPROD_Handle_t ProducerHandle, STVID_DecodedPictures_t * const DecodedPictures_p)
{
    if (DecodedPictures_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    *DecodedPictures_p = ((VIDPROD_Data_t *) ProducerHandle)->DecodedPictures;

    return(ST_NO_ERROR);
} /* End of vidprod_GetDecodedPictures() function */

/*******************************************************************************
Name        : vidprod_GetDecoderSpecialOperatingMode
Description : Get the decoder operating mode (low_delay flag, or or other future
              special cases).
Parameters  : Video producer handle, decoder operating mode.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t vidprod_GetDecoderSpecialOperatingMode (const VIDPROD_Handle_t ProducerHandle, VIDPROD_DecoderOperatingMode_t * const DecoderOperatingMode_p)
{
    /* Get the error recovery mode. */
    if (DecoderOperatingMode_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    *DecoderOperatingMode_p = ((VIDPROD_Data_t *) ProducerHandle)->DecoderOperatingMode;

    return(ST_NO_ERROR);
} /* End of vidprod_GetDecoderSpecialOperatingMode() function */

/*******************************************************************************
Name        : vidprod_GetErrorRecoveryMode
Description : Get the error recovery mode from the API to the decoder.
Parameters  : Video producer handle, error recovery mode.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t vidprod_GetErrorRecoveryMode(const VIDPROD_Handle_t ProducerHandle, STVID_ErrorRecoveryMode_t * const Mode_p)
{
    /* Get the error recovery mode. */
    if (Mode_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    *Mode_p = ((VIDPROD_Data_t *) ProducerHandle)->ErrorRecoveryMode;

    return(ST_NO_ERROR);
} /* End of vidprod_GetErrorRecoveryMode() function */

/*******************************************************************************
Name        : vidprod_GetHDPIPParams
Description : Get the current HD-PIP threshold.
Parameters  : Video producer handle, HD-PIP parameters to set.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t vidprod_GetHDPIPParams(const VIDPROD_Handle_t ProducerHandle,
        STVID_HDPIPParams_t * const HDPIPParams_p)
{
    /* Exit now if parameters are bad */
    if (HDPIPParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    *HDPIPParams_p = ((VIDPROD_Data_t *) ProducerHandle)->HDPIPParams;

    return(ST_NO_ERROR);

} /* End of vidprod_GetHDPIPParams() function */

/*******************************************************************************
Name        : vidprod_GetLastDecodedPictureInfos
Description : Get infos of last decoded picture
Parameters  : Video producer handle, pointer on structure to fill
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if bad parameter
              STVID_ERROR_DECODER_RUNNING if decoder is not stopped: can't get info
              STVID_ERROR_NOT_AVAILABLE if no picture decoded
*******************************************************************************/
ST_ErrorCode_t vidprod_GetLastDecodedPictureInfos(const VIDPROD_Handle_t ProducerHandle, STVID_PictureInfos_t * const PictureInfos_p)
{
    if (PictureInfos_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (((VIDPROD_Data_t *) ProducerHandle)->ProducerState != VIDPROD_DECODER_STATE_STOPPED)
    {
        return(STVID_ERROR_DECODER_RUNNING);
    }

    if (((VIDPROD_Data_t *) ProducerHandle)->DecoderStatus.LastDecodedPicture_p == NULL)
    {
        return(STVID_ERROR_NOT_AVAILABLE);
    }

    *PictureInfos_p = ((VIDPROD_Data_t *) ProducerHandle)->DecoderStatus.LastDecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos;

    return(ST_NO_ERROR);
} /* End of vidprod_GetLastDecodedPictureInfos() function */

/*******************************************************************************
Name        : vidprod_GetInfos
Description : Get codec dependant infos
Parameters  : Video producer handle, pointer on structure to fill
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if bad parameter
              STVID_ERROR_DECODER_RUNNING if decoder is not stopped: can't get info
              STVID_ERROR_NOT_AVAILABLE if no picture decoded
*******************************************************************************/
ST_ErrorCode_t vidprod_GetInfos(const VIDPROD_Handle_t ProducerHandle, VIDPROD_Infos_t * const Infos_p)
{
    if (Infos_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Infos_p->MaximumDecodingLatencyInSystemClockUnit = ((VIDPROD_Data_t *) ProducerHandle)->CodecInfo.MaximumDecodingLatencyInSystemClockUnit;
    Infos_p->FrameBufferAdditionalDataBytesPerMB = ((VIDPROD_Data_t *) ProducerHandle)->CodecInfo.FrameBufferAdditionalDataBytesPerMB;

    return(ST_NO_ERROR);
} /* End of vidprod_GetInfos() function */

#ifdef STVID_DEBUG_GET_STATISTICS
/*******************************************************************************
Name        : vidprod_GetStatistics
Description : Gets the decode module statistics.
Parameters  : Video producer handle, statistics.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t vidprod_GetStatistics(const VIDPROD_Handle_t ProducerHandle, STVID_Statistics_t * const Statistics_p)
{
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    STVID_Statistics_t ParserStatistics, DecoderStatistics;

    if (Statistics_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Retrieve statistics from parser and decoder */
    PARSER_GetStatistics(VIDPROD_Data_p->ParserHandle, &ParserStatistics);
    DECODER_GetStatistics(VIDPROD_Data_p->DecoderHandle, &DecoderStatistics);

    /* patch statistics which are not yet implemented in the H264 codec */
    ParserStatistics.DecodeStartCodeFound = -1;
    ParserStatistics.DecodeSequenceFound  = -1;
    ParserStatistics.DecodeUserDataFound  = -1;
    ParserStatistics.DecodePbStartCodeFoundInvalid  = -1;
    ParserStatistics.DecodePbStartCodeFoundVideoPES = -1;
    ParserStatistics.DecodePbHeaderFifoEmpty = -1;

    DecoderStatistics.DecodeInterruptStartDecode  = -1;
    DecoderStatistics.DecodeInterruptPipelineIdle = -1;
    DecoderStatistics.DecodeInterruptDecoderIdle  = -1;
    DecoderStatistics.DecodeInterruptBitBufferEmpty = -1;
    DecoderStatistics.DecodeInterruptBitBufferFull  = -1;
    DecoderStatistics.DecodePbMaxNbInterruptSyntaxErrorPerPicture  = -1;
    DecoderStatistics.DecodePbInterruptSyntaxError  = -1;
    DecoderStatistics.DecodePbInterruptDecodeOverflowError  = -1;
    DecoderStatistics.DecodePbInterruptDecodeUnderflowError = -1;
    DecoderStatistics.DecodePbInterruptMisalignmentError = -1;
    DecoderStatistics.DecodePbInterruptQueueOverflow = -1;
    /* endof statistics patch */

    /* Then consolidate with retrieve producer statistics */
    Statistics_p->DecodeStartCodeFound                     = ParserStatistics.DecodeStartCodeFound;
    Statistics_p->DecodeSequenceFound                      = ParserStatistics.DecodeSequenceFound;
    Statistics_p->DecodeUserDataFound                      = ParserStatistics.DecodeUserDataFound;
    Statistics_p->DecodeInterruptStartDecode               = DecoderStatistics.DecodeInterruptStartDecode;
    Statistics_p->DecodePbStartCodeFoundInvalid            = ParserStatistics.DecodePbStartCodeFoundInvalid;
    Statistics_p->DecodePbStartCodeFoundVideoPES           = ParserStatistics.DecodePbStartCodeFoundVideoPES;
    Statistics_p->DecodeHardwareSoftReset                  = VIDPROD_Data_p->Statistics.DecodeHardwareSoftReset;
    Statistics_p->DecodePictureFound                       = VIDPROD_Data_p->Statistics.DecodePictureFound;
    Statistics_p->DecodePictureFoundMPEGFrameI             = VIDPROD_Data_p->Statistics.DecodePictureFoundMPEGFrameI;
    Statistics_p->DecodePictureFoundMPEGFrameP             = VIDPROD_Data_p->Statistics.DecodePictureFoundMPEGFrameP;
    Statistics_p->DecodePictureFoundMPEGFrameB             = VIDPROD_Data_p->Statistics.DecodePictureFoundMPEGFrameB;
    Statistics_p->DecodePictureSkippedRequested            = VIDPROD_Data_p->Statistics.DecodePictureSkippedRequested;
    Statistics_p->DecodePictureSkippedNotRequested         = VIDPROD_Data_p->Statistics.DecodePictureSkippedNotRequested;
    Statistics_p->DecodePictureDecodeLaunched              = VIDPROD_Data_p->Statistics.DecodePictureDecodeLaunched;
    Statistics_p->DecodeStartConditionVbvDelay             = VIDPROD_Data_p->Statistics.DecodeStartConditionVbvDelay;
    Statistics_p->DecodeStartConditionPtsTimeComparison    = VIDPROD_Data_p->Statistics.DecodeStartConditionPtsTimeComparison;
    Statistics_p->DecodeStartConditionVbvBufferSize        = VIDPROD_Data_p->Statistics.DecodeStartConditionVbvBufferSize;
    Statistics_p->DecodeInterruptPipelineIdle              = DecoderStatistics.DecodeInterruptPipelineIdle;
    Statistics_p->DecodeInterruptDecoderIdle               = DecoderStatistics.DecodeInterruptDecoderIdle;
    Statistics_p->DecodeInterruptBitBufferEmpty            = DecoderStatistics.DecodeInterruptBitBufferEmpty;
    Statistics_p->DecodeInterruptBitBufferFull             = DecoderStatistics.DecodeInterruptBitBufferFull;
    Statistics_p->DecodePbMaxNbInterruptSyntaxErrorPerPicture = DecoderStatistics.DecodePbMaxNbInterruptSyntaxErrorPerPicture;
    Statistics_p->DecodePbInterruptSyntaxError             = VIDPROD_Data_p->Statistics.DecodePbInterruptSyntaxError;
    Statistics_p->DecodePbInterruptDecodeOverflowError     = DecoderStatistics.DecodePbInterruptDecodeOverflowError;
    Statistics_p->DecodePbInterruptDecodeUnderflowError    = DecoderStatistics.DecodePbInterruptDecodeUnderflowError;
    Statistics_p->DecodePbInterruptMisalignmentError       = DecoderStatistics.DecodePbInterruptMisalignmentError;
    Statistics_p->DecodePbDecodeTimeOutError               = VIDPROD_Data_p->Statistics.DecodePbDecodeTimeOutError;
    Statistics_p->DecodePbInterruptQueueOverflow           = DecoderStatistics.DecodePbInterruptQueueOverflow;
    Statistics_p->DecodePbHeaderFifoEmpty                  = ParserStatistics.DecodePbHeaderFifoEmpty;
    Statistics_p->DecodePbVbvSizeGreaterThanBitBuffer      = VIDPROD_Data_p->Statistics.DecodePbVbvSizeGreaterThanBitBuffer;
    Statistics_p->DecodeMinBitBufferLevelReached           = VIDPROD_Data_p->Statistics.DecodeMinBitBufferLevelReached;
    Statistics_p->DecodeMaxBitBufferLevelReached           = VIDPROD_Data_p->Statistics.DecodeMaxBitBufferLevelReached;
    Statistics_p->DecodePbSequenceNotInMemProfileSkipped   = VIDPROD_Data_p->Statistics.DecodePbSequenceNotInMemProfileSkipped;
    Statistics_p->DecodePbParserError                      = VIDPROD_Data_p->Statistics.DecodePbParserError;
    Statistics_p->DecodePbPreprocError                     = VIDPROD_Data_p->Statistics.DecodePbPreprocError;
    Statistics_p->DecodePbFirmwareError                    = VIDPROD_Data_p->Statistics.DecodePbFirmwareError;
    Statistics_p->DecodeGNBvd42696Error                    = DecoderStatistics.DecodeGNBvd42696Error;


    Statistics_p->DecodeTimeNbPictures                     = VIDPROD_Data_p->Statistics.DecodeTimeNbPictures;
    Statistics_p->DecodeTimeMinPreprocessTime              = VIDPROD_Data_p->Statistics.DecodeTimeMinPreprocessTime;
    Statistics_p->DecodeTimeMaxPreprocessTime              = VIDPROD_Data_p->Statistics.DecodeTimeMaxPreprocessTime;
    Statistics_p->DecodeTimeSumOfPreprocessTime            = VIDPROD_Data_p->Statistics.DecodeTimeSumOfPreprocessTime;
    Statistics_p->DecodeTimeMinBufferSearchTime            = VIDPROD_Data_p->Statistics.DecodeTimeMinBufferSearchTime;
    Statistics_p->DecodeTimeMaxBufferSearchTime            = VIDPROD_Data_p->Statistics.DecodeTimeMaxBufferSearchTime;
    Statistics_p->DecodeTimeSumOfBufferSearchTime          = VIDPROD_Data_p->Statistics.DecodeTimeSumOfBufferSearchTime;
    Statistics_p->DecodeTimeMinDecodeTime                  = VIDPROD_Data_p->Statistics.DecodeTimeMinDecodeTime;
    Statistics_p->DecodeTimeMaxDecodeTime                  = VIDPROD_Data_p->Statistics.DecodeTimeMaxDecodeTime;
    Statistics_p->DecodeTimeSumOfDecodeTime                = VIDPROD_Data_p->Statistics.DecodeTimeSumOfDecodeTime;
    Statistics_p->DecodeTimeMinFullDecodeTime              = VIDPROD_Data_p->Statistics.DecodeTimeMinFullDecodeTime;
    Statistics_p->DecodeTimeMaxFullDecodeTime              = VIDPROD_Data_p->Statistics.DecodeTimeMaxFullDecodeTime;
    Statistics_p->DecodeTimeSumOfFullDecodeTime            = VIDPROD_Data_p->Statistics.DecodeTimeSumOfFullDecodeTime;
#ifdef STVID_VALID_MEASURE_TIMING
    Statistics_p->DecodeTimeMinLXDecodeTime                = VIDPROD_Data_p->Statistics.DecodeTimeMinLXDecodeTime;
    Statistics_p->DecodeTimeMaxLXDecodeTime                = VIDPROD_Data_p->Statistics.DecodeTimeMaxLXDecodeTime;
    Statistics_p->DecodeTimeSumOfLXDecodeTime              = VIDPROD_Data_p->Statistics.DecodeTimeSumOfLXDecodeTime;
#endif
    Statistics_p->MaxBitRate = VIDPROD_Data_p->Statistics.MaxBitRate;
    Statistics_p->MinBitRate = VIDPROD_Data_p->Statistics.MinBitRate;
    Statistics_p->LastBitRate = VIDPROD_Data_p->Statistics.LastBitRate;
    Statistics_p->MaxPositiveDriftRequested = VIDPROD_Data_p->Statistics.MaxPositiveDriftRequested;
    Statistics_p->MaxNegativeDriftRequested = VIDPROD_Data_p->Statistics.MaxNegativeDriftRequested;
    Statistics_p->NbDecodedPicturesB = VIDPROD_Data_p->Statistics.NbDecodedPicturesB;
    Statistics_p->NbDecodedPicturesP = VIDPROD_Data_p->Statistics.NbDecodedPicturesP;
    Statistics_p->NbDecodedPicturesI = VIDPROD_Data_p->Statistics.NbDecodedPicturesI;

    Statistics_p->InjectFdmaTransfers = ParserStatistics.InjectFdmaTransfers;
    Statistics_p->InjectDataInputReadPointer = ParserStatistics.InjectDataInputReadPointer;
    Statistics_p->InjectDataInputWritePointer = ParserStatistics.InjectDataInputWritePointer;

    return(ST_NO_ERROR);
} /* End of vidprod_GetStatistics() function */

/*******************************************************************************
Name        : vidprod_ResetStatistics
Description : Resets the decode module statistics.
Parameters  : Video producer handle, statistics.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success, ST_ERROR_BAD_PARAMETER if Statistics_p == NULL
*******************************************************************************/
ST_ErrorCode_t vidprod_ResetStatistics(const VIDPROD_Handle_t ProducerHandle, const STVID_Statistics_t * const Statistics_p)
{
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

    if (Statistics_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Reset parser and decoder statistics */
    PARSER_ResetStatistics(VIDPROD_Data_p->ParserHandle, Statistics_p);
    DECODER_ResetStatistics(VIDPROD_Data_p->DecoderHandle, Statistics_p);

    /* Reset parameters that are said to be reset (giving value != 0) */
    if (Statistics_p->DecodeHardwareSoftReset != 0)
    {
        VIDPROD_Data_p->Statistics.DecodeHardwareSoftReset = 0;
    }
    if (Statistics_p->DecodePictureFound != 0)
    {
        VIDPROD_Data_p->Statistics.DecodePictureFound = 0;
    }
    if (Statistics_p->DecodePictureFoundMPEGFrameI != 0)
    {
        VIDPROD_Data_p->Statistics.DecodePictureFoundMPEGFrameI = 0;
    }
    if (Statistics_p->DecodePictureFoundMPEGFrameP != 0)
    {
        VIDPROD_Data_p->Statistics.DecodePictureFoundMPEGFrameP = 0;
    }
    if (Statistics_p->DecodePictureFoundMPEGFrameB != 0)
    {
        VIDPROD_Data_p->Statistics.DecodePictureFoundMPEGFrameB = 0;
    }
    if (Statistics_p->DecodePictureSkippedRequested != 0)
    {
        VIDPROD_Data_p->Statistics.DecodePictureSkippedRequested = 0;
    }
    if (Statistics_p->DecodePictureSkippedNotRequested != 0)
    {
        VIDPROD_Data_p->Statistics.DecodePictureSkippedNotRequested = 0;
    }
    if (Statistics_p->DecodePictureDecodeLaunched != 0)
    {
        VIDPROD_Data_p->Statistics.DecodePictureDecodeLaunched = 0;
    }
    if (Statistics_p->DecodeStartConditionVbvDelay != 0)
    {
        VIDPROD_Data_p->Statistics.DecodeStartConditionVbvDelay = 0;
    }
    if (Statistics_p->DecodeStartConditionPtsTimeComparison != 0)
    {
        VIDPROD_Data_p->Statistics.DecodeStartConditionPtsTimeComparison = 0;
    }
    if (Statistics_p->DecodeStartConditionVbvBufferSize != 0)
    {
        VIDPROD_Data_p->Statistics.DecodeStartConditionVbvBufferSize = 0;
    }
    if (Statistics_p->DecodeInterruptPipelineIdle != 0)
    {
        VIDPROD_Data_p->Statistics.DecodeInterruptPipelineIdle = 0;
    }
    if (Statistics_p->DecodeInterruptDecoderIdle != 0)
    {
        VIDPROD_Data_p->Statistics.DecodeInterruptDecoderIdle = 0;
    }
    if (Statistics_p->DecodeInterruptBitBufferEmpty != 0)
    {
        VIDPROD_Data_p->Statistics.DecodeInterruptBitBufferEmpty = 0;
    }
    if (Statistics_p->DecodeInterruptBitBufferFull != 0)
    {
        VIDPROD_Data_p->Statistics.DecodeInterruptBitBufferFull = 0;
    }
    if (Statistics_p->DecodePbMaxNbInterruptSyntaxErrorPerPicture != 0)
    {
        VIDPROD_Data_p->Statistics.DecodePbMaxNbInterruptSyntaxErrorPerPicture = 0;
    }
    if (Statistics_p->DecodePbInterruptSyntaxError != 0)
    {
        VIDPROD_Data_p->Statistics.DecodePbInterruptSyntaxError = 0;
    }
    if (Statistics_p->DecodePbInterruptDecodeOverflowError != 0)
    {
        VIDPROD_Data_p->Statistics.DecodePbInterruptDecodeOverflowError = 0;
    }
    if (Statistics_p->DecodePbInterruptDecodeUnderflowError != 0)
    {
        VIDPROD_Data_p->Statistics.DecodePbInterruptDecodeUnderflowError = 0;
    }
    if (Statistics_p->DecodePbDecodeTimeOutError != 0)
    {
        VIDPROD_Data_p->Statistics.DecodePbDecodeTimeOutError = 0;
    }
    if (Statistics_p->DecodePbInterruptMisalignmentError != 0)
    {
        VIDPROD_Data_p->Statistics.DecodePbInterruptMisalignmentError = 0;
    }
    if (Statistics_p->DecodePbInterruptQueueOverflow != 0)
    {
        VIDPROD_Data_p->Statistics.DecodePbInterruptQueueOverflow = 0;
    }
    if (Statistics_p->DecodePbHeaderFifoEmpty != 0)
    {
        VIDPROD_Data_p->Statistics.DecodePbHeaderFifoEmpty = 0;
    }
    if (Statistics_p->DecodePbVbvSizeGreaterThanBitBuffer != 0)
    {
        VIDPROD_Data_p->Statistics.DecodePbVbvSizeGreaterThanBitBuffer = 0;
    }
    if (Statistics_p->DecodeMinBitBufferLevelReached != 0)
    {
        VIDPROD_Data_p->Statistics.DecodeMinBitBufferLevelReached = 0xFFFFFFFF;
    }
    if (Statistics_p->DecodeMaxBitBufferLevelReached != 0)
    {
        VIDPROD_Data_p->Statistics.DecodeMaxBitBufferLevelReached = 0;
    }
    if (Statistics_p->DecodePbSequenceNotInMemProfileSkipped != 0)
    {
        VIDPROD_Data_p->Statistics.DecodePbSequenceNotInMemProfileSkipped = 0;
    }
    if (Statistics_p->DecodePbParserError != 0)
    {
        VIDPROD_Data_p->Statistics.DecodePbParserError = 0;
    }
    if (Statistics_p->DecodePbPreprocError != 0)
    {
        VIDPROD_Data_p->Statistics.DecodePbPreprocError = 0;
    }
    if (Statistics_p->DecodePbFirmwareError != 0)
    {
        VIDPROD_Data_p->Statistics.DecodePbFirmwareError = 0;
    }
    VIDPROD_Data_p->Statistics.DecodeTimeNbPictures = 0;
    VIDPROD_Data_p->Statistics.DecodeTimeMinPreprocessTime = ULONG_MAX;
    VIDPROD_Data_p->Statistics.DecodeTimeMaxPreprocessTime = 0;
    VIDPROD_Data_p->Statistics.DecodeTimeSumOfPreprocessTime = 0;
    VIDPROD_Data_p->Statistics.DecodeTimeMinBufferSearchTime = ULONG_MAX;
    VIDPROD_Data_p->Statistics.DecodeTimeMaxBufferSearchTime = 0;
    VIDPROD_Data_p->Statistics.DecodeTimeSumOfBufferSearchTime = 0;
    VIDPROD_Data_p->Statistics.DecodeTimeMinDecodeTime = ULONG_MAX;
    VIDPROD_Data_p->Statistics.DecodeTimeMaxDecodeTime = 0;
    VIDPROD_Data_p->Statistics.DecodeTimeSumOfDecodeTime = 0;
    VIDPROD_Data_p->Statistics.DecodeTimeMinFullDecodeTime = ULONG_MAX;
    VIDPROD_Data_p->Statistics.DecodeTimeMaxFullDecodeTime = 0;
    VIDPROD_Data_p->Statistics.DecodeTimeSumOfFullDecodeTime = 0;
#ifdef STVID_VALID_MEASURE_TIMING
    VIDPROD_Data_p->Statistics.DecodeTimeMinLXDecodeTime        = ULONG_MAX;
    VIDPROD_Data_p->Statistics.DecodeTimeMaxLXDecodeTime        = 0;
    VIDPROD_Data_p->Statistics.DecodeTimeSumOfLXDecodeTime      = 0;
#endif /* STVID_VALID_MEASURE_TIMING */
    VIDPROD_Data_p->Statistics.MaxBitRate = 0;
    VIDPROD_Data_p->Statistics.MinBitRate = 0xFFFFFFFF;
    VIDPROD_Data_p->Statistics.LastBitRate = 0;
    VIDPROD_Data_p->Statistics.MaxPositiveDriftRequested = 0;
    VIDPROD_Data_p->Statistics.MaxNegativeDriftRequested = 0;
    VIDPROD_Data_p->Statistics.NbDecodedPicturesB = 0;
    VIDPROD_Data_p->Statistics.NbDecodedPicturesP = 0;
    VIDPROD_Data_p->Statistics.NbDecodedPicturesI = 0;

    return(ST_NO_ERROR);
} /* End of vidprod_ResetStatistics() function */
#endif /* STVID_DEBUG_GET_STATISTICS */

#ifdef STVID_DEBUG_GET_STATUS
/*******************************************************************************
Name        : vidprod_GetStatus
Description : Gets the decode module status.
Parameters  : Video producer handle, status.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t vidprod_GetStatus(const VIDPROD_Handle_t ProducerHandle, STVID_Status_t * const Status_p)
{
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

    if (Status_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Retrieve status from decoder */
	PARSER_GetStatus(VIDPROD_Data_p->ParserHandle, Status_p);
	DECODER_GetStatus(VIDPROD_Data_p->DecoderHandle, Status_p);

    return(ST_NO_ERROR);
} /* End of vidprod_GetStatus() function */
#endif /* STVID_DEBUG_GET_STATUS */

#if defined(VALID_TOOLS)
ST_ErrorCode_t vidprod_RegisterTrace(void)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

#if defined(TRACE_UART)
    DECODER_RegisterTrace();
    if (ErrorCode == ST_NO_ERROR)
    {
        PARSER_RegisterTrace();
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        TRACE_ConditionalRegister(VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_BLOCKNAME, vidprod_TraceItemList, sizeof(vidprod_TraceItemList)/sizeof(TRACE_Item_t));
    }
#endif /* defined(TRACE_UART) */

    return(ErrorCode);
}
#endif /* VALID_TOOLS */

/*******************************************************************************
Name        : vidprod_NewReferencePicture
Description : update reference picture list from an asynchronous external call
              The goal of this function is the API STVID_Clear function support.
              When STVID_Clear calls this function, it provides a frame buffer and we have
              to keep it as reference for the next decode start.
Parameters  : Video producer handle, new reference picture
Assumptions : The producer is stopped when this function is called (checked at API level)
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if could no notify event
*******************************************************************************/
ST_ErrorCode_t vidprod_NewReferencePicture(const VIDPROD_Handle_t ProducerHandle, VIDBUFF_PictureBuffer_t * const Picture_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

    /* first check if the decoding context is available (in case of vid_clear before vid_start) */
    if (Picture_p->ParsedPictureInformation.PictureDecodingContext_p != NULL)
    {
        if (IsRequestingAdditionnalData(VIDPROD_Data_p))
        {
            /* Allocate an additionnal buffer (if not already done because of successive calls to STVID_Clear) */
            if (Picture_p->PPB.Size == 0)
            {
                ErrorCode = VIDBUFF_GetAdditionnalDataBuffer(VIDPROD_Data_p->BufferManagerHandle, Picture_p);
            }

            /* fill the buffer */
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = DECODER_FillAdditionnalDataBuffer(VIDPROD_Data_p->DecoderHandle, Picture_p);
            }
        }

        if (ErrorCode == ST_NO_ERROR)
        {
            VIDPROD_Data_p->ReplacementReference_p = Picture_p;
            VIDPROD_Data_p->ReplacementReference_IsAvailable = TRUE;
        }
    }
    return(ErrorCode);
} /* End of vidprod_NewReferencePicture() function */

/*******************************************************************************
Name        : vidprod_NotifyDecodeEvent
Description : Notify decode event
Parameters  : Video producer handle, event parameters
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if could no notify event
*******************************************************************************/
ST_ErrorCode_t vidprod_NotifyDecodeEvent(const VIDPROD_Handle_t ProducerHandle, STEVT_EventID_t EventID, const void * EventData)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

    switch (EventID)
    {
        case STVID_DATA_OVERFLOW_EVT :
            STEVT_Notify(VIDPROD_Data_p->EventsHandle, VIDPROD_Data_p->RegisteredEventsID[VIDPROD_DATA_OVERFLOW_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST EventData);
            VIDPROD_Data_p->OverFlowEvtAlreadySent = TRUE;
            break;

        case STVID_DATA_UNDERFLOW_EVT :
            STEVT_Notify(VIDPROD_Data_p->EventsHandle, VIDPROD_Data_p->RegisteredEventsID[VIDPROD_DATA_UNDERFLOW_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST ((const STVID_DataUnderflow_t *) EventData));
            /* TODO_CL: manage bit buffer underflow duration requests */
            VIDPROD_Data_p->DataUnderflowEventNotSatisfied = TRUE;
            VIDPROD_Data_p->OverFlowEvtAlreadySent = FALSE;
            break;

        case STVID_DATA_ERROR_EVT :
            STEVT_Notify(VIDPROD_Data_p->EventsHandle, VIDPROD_Data_p->RegisteredEventsID[VIDPROD_DATA_ERROR_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
            break;

        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

    return(ST_NO_ERROR);
} /* End of vidprod_NotifyDecodeEvent() function */

/*******************************************************************************
Name        : vidprod_Resume
Description : Resume decoding
Parameters  : Video producer handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if decoder resumed
              STVID_ERROR_DECODER_NOT_PAUSING if the decoder was not paused
*******************************************************************************/
ST_ErrorCode_t vidprod_Resume(const VIDPROD_Handle_t ProducerHandle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VIDPROD_Data_t      * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

    if (!(VIDPROD_Data_p->DecoderIsFreezed))
    {
        return(STVID_ERROR_DECODER_NOT_PAUSING);
    }

    VIDPROD_Data_p->DecoderIsFreezed = FALSE;
    if ((VIDPROD_Data_p->DecodedPictures) != (VIDPROD_Data_p->DecodedPicturesBeforeLastFreeze))
    {
        ErrorCode = vidprod_SetDecodedPictures(ProducerHandle, VIDPROD_Data_p->DecodedPicturesBeforeLastFreeze);
    }

    return(ErrorCode);
} /* End of vidprod_Resume() function */

/*******************************************************************************
Name        : vidprod_SetDecodedPictures
Description : Set decoded pictures
Parameters  : Video producer handle, decoded pictures
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if unsupported mode
*******************************************************************************/
ST_ErrorCode_t vidprod_SetDecodedPictures(const VIDPROD_Handle_t ProducerHandle, const STVID_DecodedPictures_t DecodedPictures)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

    if (VIDPROD_Data_p->DecoderIsFreezed)
    {
        return (STVID_ERROR_DECODER_FREEZING);
    }
    switch (DecodedPictures)
    {
        case STVID_DECODED_PICTURES_I :
        case STVID_DECODED_PICTURES_IP :
        case STVID_DECODED_PICTURES_ALL :
            break;
#ifdef ST_speed
        case STVID_DECODED_PICTURES_FIRST_I :
            break;
#endif /* ST_speed */
        default :
            /* Unrecognised mode: don't take it into account ! */
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Valid mode: adopt it */
        if (VIDPROD_Data_p->DecodedPictures != DecodedPictures)
        {
            VIDPROD_Data_p->DecodedPictures = DecodedPictures;
            switch (VIDPROD_Data_p->DecodedPictures)
            {
                case STVID_DECODED_PICTURES_I :
                    VIDPROD_Data_p->Skip.AllPPictures = TRUE;
                    VIDPROD_Data_p->Skip.AllBPictures = TRUE;
#ifdef ST_speed
                    VIDPROD_Data_p->SearchFirstI = FALSE;
                    VIDPROD_Data_p->Skip.AllExceptFirstI = FALSE;
#endif /* ST_speed */
                    break;
                case STVID_DECODED_PICTURES_IP :
                    VIDPROD_Data_p->Skip.AllPPictures = FALSE;
                    VIDPROD_Data_p->Skip.AllBPictures = TRUE;
#ifdef ST_speed
                    VIDPROD_Data_p->SearchFirstI = FALSE;
                    VIDPROD_Data_p->Skip.AllExceptFirstI = FALSE;
#endif /* ST_speed */
                    break;
        case STVID_DECODED_PICTURES_ALL :
            VIDPROD_Data_p->Skip.AllPPictures = FALSE;
            VIDPROD_Data_p->Skip.AllBPictures = FALSE;
#ifdef STVID_TRICKMODE_BACKWARD
            if (VIDPROD_Data_p->Backward.IsBackward)
            {
                VIDPROD_Data_p->Skip.AllPPictures = TRUE;
                VIDPROD_Data_p->Skip.AllBPictures = TRUE;
            }
#endif /* STVID_TRICKMODE_BACKWARD */
#ifdef ST_speed
            VIDPROD_Data_p->SearchFirstI = FALSE;
            VIDPROD_Data_p->Skip.AllExceptFirstI= FALSE;
#endif /* ST_speed */
            break;
#ifdef ST_speed
        case STVID_DECODED_PICTURES_FIRST_I :
            VIDPROD_Data_p->Skip.AllPPictures = TRUE;
            VIDPROD_Data_p->Skip.AllBPictures = TRUE;
            VIDPROD_Data_p->SearchFirstI = TRUE;
            VIDPROD_Data_p->Skip.AllExceptFirstI = TRUE;
            break;
#endif /* ST_speed */
                default:
                    break;
            }
        }
    }
    return(ErrorCode);
} /* End of vidprod_SetDecodedPictures() function */

/*******************************************************************************
Name        : vidprod_SetErrorRecoveryMode
Description : Set the error recovery mode from the API to the decoder.
Parameters  : Video producer handle, error recovery mode.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t vidprod_SetErrorRecoveryMode(const VIDPROD_Handle_t ProducerHandle, const STVID_ErrorRecoveryMode_t ErrorRecoveryMode)
{
    /* Set the error recovery mode. */
    ((VIDPROD_Data_t *) ProducerHandle)->ErrorRecoveryMode = ErrorRecoveryMode;

    return(ST_NO_ERROR);
} /* End of vidprod_SetErrorRecoveryMode() function */

/*******************************************************************************
Name        : vidprod_SetHDPIPParams
Description : Set the HD-PIP threshold.
Parameters  : Video producer handle, HD-PIP parameters to get.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if succes, STVID_ERROR_DECODER_RUNNING if not
              in STOP mode.
*******************************************************************************/
ST_ErrorCode_t vidprod_SetHDPIPParams(const VIDPROD_Handle_t ProducerHandle,
                const STVID_HDPIPParams_t * const HDPIPParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (HDPIPParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (((VIDPROD_Data_t *) ProducerHandle)->ProducerState != VIDPROD_DECODER_STATE_STOPPED)
    {
        return(STVID_ERROR_DECODER_RUNNING);
    }

    ((VIDPROD_Data_t *) ProducerHandle)->HDPIPParams = *HDPIPParams_p;

    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module producer HD PIP : Not yet implemented !"));
    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;

    return(ErrorCode);
} /* End of vidprod_SetHDPIPParams() function. */
/*******************************************************************************
Name        : vidprod_SkipData
Description : Skip data in decoder
Parameters  : Video producer handle, skip mode
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if problem
*******************************************************************************/
ST_ErrorCode_t vidprod_SkipData(const VIDPROD_Handle_t ProducerHandle, const VIDPROD_SkipMode_t SkipMode)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

    switch (SkipMode)
    {
        case VIDPROD_SKIP_NEXT_B_PICTURE :
           VIDPROD_Data_p->Skip.NextBPicture = TRUE;
            break;
#ifdef ST_speed
        case VIDPROD_SKIP_ALL_B_PICTURES :
        case VIDPROD_SKIP_ALL_P_PICTURES :
        case VIDPROD_SKIP_NO_PICTURE :
        case VIDPROD_SKIP_NEXT_I :
		case VIDPROD_SKIP_UNTIL_NEXT_I_SPEED :
		   break;
#endif /* ST_speed */
        case VIDPROD_SKIP_UNTIL_NEXT_I :
        case VIDPROD_SKIP_UNTIL_NEXT_SEQUENCE :
            break;

        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Valid mode: adopt it */
        if (VIDPROD_Data_p->Skip.SkipMode != SkipMode)
        {
#ifdef ST_speed
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "SkipMode = %s",
                (SkipMode == VIDPROD_SKIP_NEXT_B_PICTURE)               ? "VIDPROD_SKIP_NEXT_B_PICTURE" :
                (SkipMode == VIDPROD_SKIP_ALL_B_PICTURES)              ? "VIDPROD_SKIP_ALL_B_PICTURES" :
                (SkipMode == VIDPROD_SKIP_ALL_P_PICTURES) ? "VIDPROD_SKIP_ALL_P_PICTURES" :
                (SkipMode == VIDPROD_SKIP_NO_PICTURE)? "VIDPROD_SKIP_NO_PICTURE" :
                (SkipMode == VIDPROD_SKIP_NEXT_I)            ? "VIDPROD_SKIP_NEXT_I" :
                (SkipMode == VIDPROD_SKIP_UNTIL_NEXT_I_SPEED)            ? "VIDPROD_SKIP_UNTIL_NEXT_I_SPEED" :
                (SkipMode == VIDPROD_SKIP_UNTIL_NEXT_I)            ? "VIDPROD_SKIP_UNTIL_NEXT_I" :
                (SkipMode == VIDPROD_SKIP_UNTIL_NEXT_SEQUENCE)             ? "VIDPROD_SKIP_UNTIL_NEXT_SEQUENCE" : "?"
            ));
#else
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "SkipMode = %d", SkipMode));
#endif /* ST_speed */
            VIDPROD_Data_p->Skip.SkipMode = SkipMode;

            switch (VIDPROD_Data_p->Skip.SkipMode)
            {
                case VIDPROD_SKIP_NEXT_B_PICTURE :
                VIDPROD_Data_p->Skip.NextBPicture = TRUE;
                    break;
#ifdef ST_speed
        case VIDPROD_SKIP_ALL_B_PICTURES :
           VIDPROD_Data_p->Skip.AllBPicturesSpeed = TRUE;
           VIDPROD_Data_p->Skip.AllPPicturesSpeed = FALSE;
           VIDPROD_Data_p->Skip.NextBPicture = FALSE;
           VIDPROD_Data_p->Skip.NextIPicture = FALSE;
           VIDPROD_Data_p->Skip.UntilNextIPicture = FALSE;
           VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint = FALSE;
           VIDPROD_Data_p->Skip.UntilNextIPictureSpeed = FALSE;
            break;
        case VIDPROD_SKIP_ALL_P_PICTURES :
           VIDPROD_Data_p->Skip.AllBPicturesSpeed = TRUE;
           VIDPROD_Data_p->Skip.AllPPicturesSpeed = TRUE;
            break;
        case VIDPROD_SKIP_NO_PICTURE :
            VIDPROD_Data_p->Skip.AllBPicturesSpeed = FALSE;
            VIDPROD_Data_p->Skip.AllPPicturesSpeed = FALSE;
            VIDPROD_Data_p->Skip.NextBPicture = FALSE;
            VIDPROD_Data_p->Skip.NextIPicture = FALSE;
            VIDPROD_Data_p->Skip.UntilNextIPicture = FALSE;
            VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint = FALSE;
			VIDPROD_Data_p->Skip.UntilNextIPictureSpeed = FALSE;
            break;
        case VIDPROD_SKIP_NEXT_I :
            VIDPROD_Data_p->Skip.NextIPicture = TRUE;
            break;
		case VIDPROD_SKIP_UNTIL_NEXT_I_SPEED :
           VIDPROD_Data_p->Skip.UntilNextIPictureSpeed = TRUE;
           VIDPROD_Data_p->Skip.AllBPicturesSpeed = TRUE;
           VIDPROD_Data_p->Skip.AllPPicturesSpeed = TRUE;
		   break;
#endif /* ST_speed */
        case VIDPROD_SKIP_UNTIL_NEXT_I :
           VIDPROD_Data_p->Skip.UntilNextIPicture = TRUE;
            /* No break here, UntilNextIPicture shall be treated like a next random access point search */
        case VIDPROD_SKIP_UNTIL_NEXT_SEQUENCE :
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "vidprod_SkipData Ask for skip until next RAP"));
            VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint = TRUE;
            VIDPROD_Data_p->Skip.RecoveringNextEntryPoint = TRUE;
            VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = TRUE;
            break;
            }

        }
    }
    return(ErrorCode);
} /* End of vidprod_SkipData() function */

/*******************************************************************************
Name        : vidprod_StartUpdatingDisplay
Description : Start updating the display
Parameters  : Video producer handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t vidprod_StartUpdatingDisplay(const VIDPROD_Handle_t ProducerHandle)
{
    /* Update the display from now (not if it was already the case) */
    if (((VIDPROD_Data_t *) ProducerHandle)->UpdateDisplayQueue == VIDPROD_UPDATE_DISPLAY_QUEUE_NEVER)
    {
        ((VIDPROD_Data_t *) ProducerHandle)->UpdateDisplayQueue = VIDPROD_DEFAULT_UPDATE_DISPLAY_QUEUE_MODE;
    }

    return(ST_NO_ERROR);
} /* end of vidprod_StartUpdatingDisplay() function */

/*******************************************************************************
Name        : vidprod_Stop
Description : Stop decoding
Parameters  : Video producer handle, stop mode
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if decoder stopped
              STVID_ERROR_DECODER_STOPPED if the decoder is already stopped
*******************************************************************************/
ST_ErrorCode_t vidprod_Stop(const VIDPROD_Handle_t ProducerHandle, const STVID_Stop_t StopMode, const BOOL StopAfterBuffersTimeOut)
{
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

    UNUSED_PARAMETER(StopAfterBuffersTimeOut);

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "vidprod_Stop() called"));

    /* TODO_CL: add stop structure to VIDPROD_Data_t and manage stop mode requests */
    /* For now, only stop now is supported */
/*    VIDPROD_Data_p->Stop.StopAfterBuffersTimeOut = StopAfterBuffersTimeOut;*/
    switch (StopMode)
    {
        case STVID_STOP_WHEN_NEXT_REFERENCE :
            /* TODO_CL:
            * - push (or flush ?) re-ordering display queue content in order to release all frame buffers except the currently displayed one
            * - Reset PARSER & DECODER to their IDLE state
            * - generate API stopped evt according to StopMode requested
            * VIDPROD_Data_p->Stop.StopAfterBuffersTimeOut must be set with StopAfterBuffersTimeOut value and
            * the producer decode task has to manage to stop the decode in case of lack of frame buffer. (see example in vid_dec.c)
            */
/*            VIDPROD_Data_p->Stop.WhenNextReferencePicture = TRUE;*/
            break;

        case STVID_STOP_WHEN_END_OF_DATA :
            /* TODO_CL:
            * - push (or flush ?) re-ordering display queue content in order to release all frame buffers except the currently displayed one
            * - Reset PARSER & DECODER to their IDLE state
            * - generate API stopped evt according to StopMode requested
            * VIDPROD_Data_p->Stop.StopAfterBuffersTimeOut must be set with StopAfterBuffersTimeOut value and
            * the producer decode task has to manage to stop the decode in case of lack of frame buffer. (see example in vid_dec.c)
            */
            vidprod_FlushInjection(ProducerHandle);
            VIDPROD_Data_p->Stop.WhenNoData = TRUE;
            break;

        case STVID_STOP_WHEN_NEXT_I :
            /* TODO_CL:
            * - push (or flush ?) re-ordering display queue content in order to release all frame buffers except the currently displayed one
            * - Reset PARSER & DECODER to their IDLE state
            * - generate API stopped evt according to StopMode requested
            * VIDPROD_Data_p->Stop.StopAfterBuffersTimeOut must be set with StopAfterBuffersTimeOut value and
            * the producer decode task has to manage to stop the decode in case of lack of frame buffer. (see example in vid_dec.c)
            */
/*            VIDPROD_Data_p->Stop.WhenNextIPicture = TRUE;*/
            break;

        case STVID_STOP_NOW :
        default :
            PushControllerCommand(ProducerHandle, CONTROLLER_COMMAND_STOP);
            /* Stop now, and don't send STOPPED event */
            /* CL:
            * - push (or flush ?) re-ordering display queue content in order to release all frame buffers except the currently displayed one
            * - Reset PARSER & DECODER to their IDLE state
            */
            break;
    } /* switch StopMode */
    while (STOS_SemaphoreWaitTimeOut(VIDPROD_Data_p->Stop.ActuallyStoppedSemaphore_p, TIMEOUT_IMMEDIATE) == 0)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Clean previous ActuallyStoppedSemaphore_p signal"));
        /* Nothing to do: just to decrement counter until 0, to consume previously signaled stop */;
    }
    STOS_SemaphoreSignal(VIDPROD_Data_p->ProducerOrder_p);
    /* wait for actual stop completion */
    if (StopMode == STVID_STOP_NOW)
    {
        VIDPROD_Data_p->Stop.MaxWaitStopTime = time_plus(time_now(), MAX_WAIT_STOP_TIME);
        if (STOS_SemaphoreWaitTimeOut(VIDPROD_Data_p->Stop.ActuallyStoppedSemaphore_p, &(VIDPROD_Data_p->Stop.MaxWaitStopTime)) != 0)
        {
            /* Set state to stopped */
            VIDPROD_Data_p->ProducerState = VIDPROD_DECODER_STATE_STOPPED;
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "vidprod_Stop() returns on SemaphoreWait timeout !"));
        }
    }
    return(ST_NO_ERROR);
} /* End of vidprod_Stop() function */

#ifdef ST_deblock
/*******************************************************************************
Name        : vidprod_SetDeblockingMode
Description : cascaded to hal decode
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void vidprod_SetDeblockingMode(const VIDPROD_Handle_t ProducerHandle, const BOOL DeblockingEnabled)
{
    UNUSED_PARAMETER(ProducerHandle);
    UNUSED_PARAMETER(DeblockingEnabled);
} /* End of VIDDEC_SetDeblockingMode() function */

#ifdef VIDEO_DEBLOCK_DEBUG
/*******************************************************************************
Name        : vidprod_SetDeblockingStrength
Description : cascaded to hal decode
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void vidprod_SetDeblockingStrength(const VIDPROD_Handle_t ProducerHandle, const int DeblockingStrength)
{
    UNUSED_PARAMETER(ProducerHandle);
    UNUSED_PARAMETER(DeblockingStrength);
} /* End of vidprod_SetDeblockingStrength() function */
#endif /* VIDEO_DEBLOCK_DEBUG  */
#endif /* ST_deblock */

/*******************************************************************************
Name        : vidprod_SetDataInputInterface
Description : cascaded to hal decode
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t vidprod_SetDataInputInterface(const VIDPROD_Handle_t ProducerHandle,
        ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle,
                                            void ** const Address_p),
        void (*InformReadAddress)(void * const Handle, void * const Address),
        void * const Handle )
{
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    ErrorCode = PARSER_SetDataInputInterface(VIDPROD_Data_p->ParserHandle, GetWriteAddress, InformReadAddress, Handle);

    return(ErrorCode);
} /* End of vidprod_SetDataInputInterface() function */

/*******************************************************************************
Name        : vidprod_DeleteDataInputInterface
Description : cascaded to hal decode
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t vidprod_DeleteDataInputInterface(
                                    const VIDPROD_Handle_t ProducerHandle)
{
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    ErrorCode = PARSER_SetDataInputInterface(VIDPROD_Data_p->ParserHandle, NULL, NULL, 0);

    return(ErrorCode);
} /* End of vidprod_DeleteDataInputInterface() function */

/*******************************************************************************
Name        : vidprod_GetDataInputBufferParams
Description : cascaded to hal decode
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t vidprod_GetDataInputBufferParams(
                                const VIDPROD_Handle_t ProducerHandle,
                                void ** const BaseAddress_p,
                                U32 * const Size_p)
{
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    ST_ErrorCode_t ErrorCode;

    ErrorCode = PARSER_GetDataInputBufferParams(VIDPROD_Data_p->ParserHandle, BaseAddress_p, Size_p);

    return(ErrorCode);
}

/*******************************************************************************
Name        : MonitorBitBufferLevel
Description : Monitor bit buffer level, update statistics
Parameters  : producer handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void MonitorBitBufferLevel(const VIDPROD_Handle_t ProducerHandle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    PARSER_GetBitBufferLevelParams_t PARSER_GetBitBufferLevelParams;
    U32 BitBufferLevel;
#if defined(TRACE_UART) && defined(VALID_TOOLS)
    #define BBL_TRACE_DECIMATION 3
    static  U32 TraceCount = BBL_TRACE_DECIMATION - 1;
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */
    U32 UnderflowReactionThreshold, OverflowReactionThreshold, OverflowRealTimeReactionThreshold, EndofOverflowRealTimeReactionThreshold;
#ifdef ST_speed
    UnderflowReactionThreshold = (VIDPROD_Data_p->BitBuffer.TotalSize / 4)*3; /*20*/ /* underflow limit set to 5% of bit buffer size*/
#else
    UnderflowReactionThreshold = (VIDPROD_Data_p->BitBuffer.TotalSize / 20); /* underflow limit set to 5% of bit buffer size*/
#endif
    OverflowReactionThreshold  = (VIDPROD_Data_p->BitBuffer.TotalSize * 9) /10; /* non real time overflow limit set to 90% of bit buffer size*/
    OverflowRealTimeReactionThreshold = (VIDPROD_Data_p->BitBuffer.TotalSize * 90) /100; /* overflow limit set to 90% of bit buffer size*/
    EndofOverflowRealTimeReactionThreshold = (VIDPROD_Data_p->BitBuffer.TotalSize * 50 ) / 100; /* threshold reflecting a back to normal BBL situation to end decrease BBL actions */
    ErrorCode = PARSER_GetBitBufferLevel(VIDPROD_Data_p->ParserHandle, &PARSER_GetBitBufferLevelParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* TODO_CL: error handling to be defined.
            * error cases have to be defined */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "PARSER_GetBitBufferLevel failed Error=%d!!", ErrorCode));
    }
    else
    {
         BitBufferLevel = PARSER_GetBitBufferLevelParams.BitBufferLevelInBytes;
         VIDPROD_Data_p->LastBitBufferLevel = BitBufferLevel;
#if defined(TRACE_UART) && defined(VALID_TOOLS)
        if(!(TraceCount = ((TraceCount + 1) % BBL_TRACE_DECIMATION)))
        {  /* This test is to display BBL on trace only once over 5 time. As MonitorBitBufferLevel function is called
              approximately 10 times per frame, this displays BBL twice a frame duration */
            TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_BBL, "pBBL=%d \n\r",BitBufferLevel));
        }
#endif
#if defined (STVID_DEBUG_GET_STATISTICS)
        if (BitBufferLevel < VIDPROD_Data_p->Statistics.DecodeMinBitBufferLevelReached)
        {
            VIDPROD_Data_p->Statistics.DecodeMinBitBufferLevelReached = BitBufferLevel;
        }
        if (BitBufferLevel > VIDPROD_Data_p->Statistics.DecodeMaxBitBufferLevelReached)
        {
            VIDPROD_Data_p->Statistics.DecodeMaxBitBufferLevelReached = BitBufferLevel;
        }
#endif /* STVID_DEBUG_GET_STATISTICS */
    /* TODO_CL: add bit buffer level management, request skips if necessary to decrease bit buffer level and prevent overflow */
    /*          notify STVID_DATA_UNDERFLOW_EVT & STVID_DATA_OVERFLOW_EVT events */
        if (VIDPROD_Data_p->ProducerState == VIDPROD_DECODER_STATE_DECODING)
        {
            /* Preform action after monitor only if decoder started */
            if(BitBufferLevel <= OverflowRealTimeReactionThreshold)
            {
                VIDPROD_Data_p->NeedToDecreaseBBL = FALSE;
            }

            if (BitBufferLevel > OverflowReactionThreshold)
            {
                if (!VIDPROD_Data_p->OverFlowEvtAlreadySent)
                {
                    /* Data overflow ! */
                    vidprod_NotifyDecodeEvent(ProducerHandle, STVID_DATA_OVERFLOW_EVT, (const void *) NULL);
#ifndef STVID_VALID
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "BitBuffer Overflow, BBL=%d", BitBufferLevel));
#endif /* STVID_VALID */
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_BBL, "OVFLW pBBL=%d\r\n",BitBufferLevel));
#endif
                }
                if((VIDPROD_Data_p->RealTime) && (BitBufferLevel > OverflowRealTimeReactionThreshold) && (!VIDPROD_Data_p->NeedToDecreaseBBL))
                {
                    /* Prevent bitbuffer overflow during a real-time injection */
                    /* = try to decrease bitbuffer level -> skip picture       */
                    /* CL_NOTE: For now, just use VIDPROD_Data_p->NeedToDecreaseBBL in producer task to skip all non ref pictures to reduce BBL */
                    VIDPROD_Data_p->NeedToDecreaseBBL = TRUE;
                    /* Search next entry point and ask for a reset of list of references */
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Ask for skip until next RAP (Overflow BBL = %d)", BitBufferLevel));
                    VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint = TRUE;
                    VIDPROD_Data_p->Skip.RecoveringNextEntryPoint = TRUE;
                    VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = TRUE;
                } /* end  if BitBufferLevel > OverflowRealTimeReactionThreshold*/
            } /* end  if BitBufferLevel > OverflowReactionThreshold */
            else
            {
                VIDPROD_Data_p->OverFlowEvtAlreadySent = FALSE;
                if (BitBufferLevel < UnderflowReactionThreshold)
                {
                    VIDPROD_Data_p->OverFlowEvtAlreadySent = FALSE;
                    /* Data undeflow ! */
                    if (!VIDPROD_Data_p->DataUnderflowEventNotSatisfied)
                    {
#ifdef ST_speed
#ifdef STVID_TRICKMODE_BACKWARD
                        if (VIDPROD_Data_p->Backward.JustSwitchForward)
						{
							VIDPROD_Data_p->DataUnderflow.BitRateValue = 11400;
                            VIDPROD_Data_p->Backward.JustSwitchForward = FALSE;
						}
						else
#endif /* STVID_TRICKMODE_BACKWARD */
						{
                        	VIDPROD_GetBitRateValue(ProducerHandle, &(VIDPROD_Data_p->DataUnderflow.BitRateValue));
						}
#endif /*ST_speed*/
                        VIDPROD_Data_p->DataUnderflow.RequiredTimeJump = 0;
                        VIDPROD_Data_p->DataUnderflow.RequiredDuration = 0;
                        VIDPROD_Data_p->DataUnderflow.BitBufferFreeSize = VIDPROD_Data_p->BitBuffer.TotalSize - VIDPROD_Data_p->LastBitBufferLevel;
#ifdef STVID_DEBUG_GET_STATISTICS
#ifdef ST_speed
                        if (VIDPROD_Data_p->DriftTime != 0)
                        {
                            VIDPROD_Data_p->DriftTime = VIDPROD_Data_p->DriftTime /90000 ;
                            if (VIDPROD_Data_p->DriftTime >= 0)
                            {
                                if (VIDPROD_Data_p->DriftTime > VIDPROD_Data_p->Statistics.MaxPositiveDriftRequested)
                                {
                                    VIDPROD_Data_p->Statistics.MaxPositiveDriftRequested = VIDPROD_Data_p->DriftTime;
                                }
                            }
                            else
                            {
                                if (VIDPROD_Data_p->DriftTime < VIDPROD_Data_p->Statistics.MaxNegativeDriftRequested)
                                {
                                    VIDPROD_Data_p->Statistics.MaxNegativeDriftRequested = VIDPROD_Data_p->DriftTime;
                                }
                            }
                            VIDPROD_Data_p->DriftTime = 0;
                        }
#endif /* ST_speed */
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef ST_speed
                        if (VIDPROD_Data_p->DataUnderflow.BitRateValue != 0)
                        {
                            if (VIDPROD_Data_p->DataUnderflow.BitBufferFreeSize < ((U32)ULONG_MAX / STVID_DURATION_COMPUTE_FACTOR))
                            {
                                VIDPROD_Data_p->DataUnderflow.RequiredDuration =
                                        (VIDPROD_Data_p->DataUnderflow.BitBufferFreeSize * STVID_DURATION_COMPUTE_FACTOR) / VIDPROD_Data_p->DataUnderflow.BitRateValue;
                            }
                            else
                            {
                                VIDPROD_Data_p->DataUnderflow.RequiredDuration =
                                        (((VIDPROD_Data_p->DataUnderflow.BitBufferFreeSize * (STVID_DURATION_COMPUTE_FACTOR / 100)) / (VIDPROD_Data_p->DataUnderflow.BitRateValue)) * 100);
                            }
                        }
                        VIDPROD_Data_p->Skip.UnderflowOccured = TRUE;
                        VIDPROD_Data_p->IsBitRateComputable = FALSE;
#endif /*ST_speed*/
                        /* Bit buffer free size is filled in MonitorBitBufferLevel() function */
                        vidprod_NotifyDecodeEvent(ProducerHandle, STVID_DATA_UNDERFLOW_EVT, (const void *) &(VIDPROD_Data_p->DataUnderflow));

#ifndef STVID_VALID
                        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "BitBuffer Underflow, BBL=%d", BitBufferLevel));
#endif /* STVID_VALID */

#if defined(TRACE_UART) && !defined(VALID_TOOLS)
                        TraceBuffer(("UnderFlw, BBL=%d, bit rate %d, jump %d, duration %d\r\n", BitBufferLevel, VIDPROD_Data_p->DataUnderflow.BitRateValue,
            VIDPROD_Data_p->DataUnderflow.RequiredTimeJump, VIDPROD_Data_p->DataUnderflow.RequiredDuration));
#endif
                    }
                } /* end if BitBufferLevel < UnderflowReactionThreshold */
            }
        }/* endif DecoderState == VIDPROD_DECODER_STATE_DECODING */
        else
        {
            VIDPROD_Data_p->NeedToDecreaseBBL = FALSE;
        }
    }

} /* end of MonitorBitBufferLevel */

/*******************************************************************************
Name        : vidprod_ReleaseNonPushedNonRefPictures
Description : Release and remove from DecodePictureBuffer table all non ref pictures which have not yet been pushed
Parameters  : producer handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void vidprod_ReleaseNonPushedNonRefPictures(const VIDPROD_Handle_t ProducerHandle)
{
    VIDPROD_Data_t             *VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    U32                         LoopCounter;
    VIDBUFF_PictureBuffer_t    *CurrentPicture_p;

    for (LoopCounter = 0 ; LoopCounter < MAX_NUMBER_OF_DECODE_PICTURES ; LoopCounter++)
    {
        CurrentPicture_p = VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter];
        if ((CurrentPicture_p != NULL) && (CurrentPicture_p->IsNeededForPresentation) && (CurrentPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_DECODED))
        {
            if ((!CurrentPicture_p->IsReferenceForDecoding) && (!CurrentPicture_p->IsPushed))
            {
                RemovePictureFromDecodePictureTable(ProducerHandle, CurrentPicture_p, __LINE__);
                /* Picture is non ref, release both picture and decimated if exist */
                if (CurrentPicture_p->AssociatedDecimatedPicture_p != NULL)
                {
                    RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, CurrentPicture_p->AssociatedDecimatedPicture_p, VIDCOM_VIDPROD_MODULE_BASE);
                }
                RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, CurrentPicture_p, VIDCOM_VIDPROD_MODULE_BASE);
            }
        }
    }

} /* end of vidprod_ReleaseNonPushedNonRefPictures */

/*******************************************************************************
Name        : vidprod_ReleasePushedNonRefPictures
Description : Release non ref pictures which have been pushed to the display queue
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void vidprod_ReleasePushedNonRefPictures(const VIDPROD_Handle_t ProducerHandle)
{
    VIDPROD_Data_t             *VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    U32                         LoopCounter;
    VIDBUFF_PictureBuffer_t    *CurrentPicture_p;

    for (LoopCounter = 0 ; LoopCounter < MAX_NUMBER_OF_DECODE_PICTURES ; LoopCounter++)
    {
        CurrentPicture_p = VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter];
        if ((CurrentPicture_p != NULL) && (CurrentPicture_p->IsPushed))
        {
            if (!CurrentPicture_p->IsReferenceForDecoding)
            {
                RemovePictureFromDecodePictureTable(ProducerHandle, CurrentPicture_p, __LINE__);
                /* Picture is non ref, release both picture and decimated if exist */
                if (CurrentPicture_p->AssociatedDecimatedPicture_p != NULL)
                {
                    RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, CurrentPicture_p->AssociatedDecimatedPicture_p, VIDCOM_VIDPROD_MODULE_BASE);
                }
                RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, CurrentPicture_p, VIDCOM_VIDPROD_MODULE_BASE);
            }
        }
    }

} /* End of vidprod_ReleasePushedNonRefPictures() function */

/*******************************************************************************
Name        : vidprod_DecodeStartConditionCheck
Description : Monitor bit buffer level, update statistics
Parameters  : producer handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void vidprod_DecodeStartConditionCheck(const VIDPROD_Handle_t ProducerHandle)
{
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
#ifdef ST_avsync
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    STCLKRV_ExtendedSTC_t                   ExtendedSTC;
    STVID_PTS_t                             CurrentSTC;
#endif /* ST_avsync */
    U32 DBPSize;

#ifdef ST_avsync
        if (VIDPROD_Data_p->StartCondition.StartWithPTSTimeComparison)
        {
            /* Compare the current STC to the first picture's PTS.  */
            /* Get the current STC time and compare it the the picture's PTS.       */
            ErrorCode = STCLKRV_GetExtendedSTC(VIDPROD_Data_p->StartCondition.STClkrvHandle, &ExtendedSTC);
            /* Convert STCLKRV STC to our PTS format */
            CurrentSTC.PTS      = ExtendedSTC.BaseValue;
            CurrentSTC.PTS33    = (ExtendedSTC.BaseBit32 != 0);
            if (ErrorCode == ST_NO_ERROR)
            {
                /* STC got with no error. Add a VSYNC duration and test its validity.   */
                vidcom_AddU32ToPTS(CurrentSTC, ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION * 1000) / STVID_DEFAULT_VALID_VTG_FRAME_RATE));
                /* Add Codec latency duration */
                vidcom_AddU32ToPTS(CurrentSTC, VIDPROD_Data_p->CodecInfo.MaximumDecodingLatencyInSystemClockUnit);
                /* Add bumping picture latency duration */
                DBPSize = VIDPROD_Data_p->DecoderStatus.CurrentMaxDecFrameBuffering + 2;
                if (!IsDynamicAllocationProfile(VIDPROD_Data_p->ForTask.CurrentProfile))
                {
                    if (VIDPROD_Data_p->ForTask.CurrentProfile.ApiProfile.NbFrameStore == STVID_OPTIMISED_NB_FRAME_STORE_ID)
                        {
                        if (DBPSize > VIDPROD_Data_p->ForTask.CurrentProfile.ApiProfile.FrameStoreIDParams.OptimisedNumber.Main)
                        {
                            DBPSize = VIDPROD_Data_p->ForTask.CurrentProfile.ApiProfile.FrameStoreIDParams.OptimisedNumber.Main;
                        }
                    }
                    else
                    {
                        if (DBPSize > VIDPROD_Data_p->ForTask.CurrentProfile.ApiProfile.NbFrameStore)
                        {
                            DBPSize = VIDPROD_Data_p->ForTask.CurrentProfile.ApiProfile.NbFrameStore;
                        }
                    }
                }
                vidcom_AddU32ToPTS(CurrentSTC, DBPSize * ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION * 1000) / STVID_DEFAULT_VALID_VTG_FRAME_RATE));
                /* Test a too big difference.                                           */
                if (!IsThereHugeDifferenceBetweenPTSandSTC(CurrentSTC,
                        VIDPROD_Data_p->StartCondition.FirstPicturePTSAfterReset))
                {
                    /* No, test if it's time to launch first decode.                    */
                    if (vidcom_IsPTSGreaterThanPTS(CurrentSTC, VIDPROD_Data_p->StartCondition.FirstPicturePTSAfterReset))
                    {
                        /* OK, enable first decode.                                     */
                        VIDPROD_Data_p->StartCondition.IsDecodeEnable = TRUE;
                        VIDPROD_Data_p->StartCondition.StartOnPictureDelay = FALSE;
                        VIDPROD_Data_p->StartCondition.StartWithPTSTimeComparison = FALSE;
#if defined (STVID_DEBUG_GET_STATISTICS)
                        VIDPROD_Data_p->Statistics.DecodeStartConditionPtsTimeComparison++;
#endif /* STVID_DEBUG_GET_STATISTICS */

#if defined(TRACE_UART) && defined(VALID_TOOLS)
                        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODER_START, "DecEnableOnPTS(%d) STC=%d ", VIDPROD_Data_p->StartCondition.FirstPicturePTSAfterReset.PTS, CurrentSTC.PTS));
#endif
                        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DecEnableOnPTS(%lu/%d) STC adjusted=%lu/%d STCraw=%lu/%d", VIDPROD_Data_p->StartCondition.FirstPicturePTSAfterReset.PTS, VIDPROD_Data_p->StartCondition.FirstPicturePTSAfterReset.PTS33, CurrentSTC.PTS, CurrentSTC.PTS33, ExtendedSTC.BaseValue, ExtendedSTC.BaseBit32));
                    }
                }
                else
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Huge diff between PTS(%d/%d) & STC(%d/%d) ",
                            VIDPROD_Data_p->StartCondition.FirstPicturePTSAfterReset.PTS, VIDPROD_Data_p->StartCondition.FirstPicturePTSAfterReset.PTS33,
                            CurrentSTC.PTS, CurrentSTC.PTS33));
                    /* Start on picture delay or by default on arbitrary bitbuffer level */
                    VIDPROD_Data_p->StartCondition.StartWithPTSTimeComparison = FALSE;
                    if (!VIDPROD_Data_p->StartCondition.StartOnPictureDelay)
                    {
                        VIDPROD_Data_p->StartCondition.StartWithBufferFullness = TRUE;
                    }
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Start asap Recovery huge diff (PTS %lu) STC adjusted=%lu STCraw=%lu STCExtended=%d", VIDPROD_Data_p->StartCondition.FirstPicturePTSAfterReset.PTS, CurrentSTC, ExtendedSTC.BaseValue, ExtendedSTC.BaseBit32));
                }
            } /* Valid STC */
            else
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Can't get STC !"));
                /* Start on picture delay or by default on arbitrary bitbuffer level */
                VIDPROD_Data_p->StartCondition.StartWithPTSTimeComparison = FALSE;
                if (!VIDPROD_Data_p->StartCondition.StartOnPictureDelay)
                {
                    VIDPROD_Data_p->StartCondition.StartWithBufferFullness = TRUE;
                }
            }
        } /* Start with PTS */
#endif /* ST_avsync */

        if (VIDPROD_Data_p->StartCondition.StartOnPictureDelay)
        {
            if ((STOS_time_after(time_now(), VIDPROD_Data_p->StartCondition.PictureDelayTimerExpiredDate) != 0))
            {
                VIDPROD_Data_p->StartCondition.IsDecodeEnable = TRUE;
#if defined (STVID_DEBUG_GET_STATISTICS)
                VIDPROD_Data_p->Statistics.DecodeStartConditionVbvDelay++;
#endif /* STVID_DEBUG_GET_STATISTICS */
#if defined(TRACE_UART) && defined(VALID_TOOLS)
            TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODER_START, "DecEnableVBVdelay "));
#endif
            }
        }

        if (VIDPROD_Data_p->StartCondition.StartWithBufferFullness)
        {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
            TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_BBL, "pPARS_BBL \r\n"));
#endif
            if ((VIDPROD_Data_p->LastBitBufferLevel >= VIDPROD_Data_p->StartCondition.BufferFullnessToReachBeforeStartInBytes) ||
                (STOS_time_after(time_now(), VIDPROD_Data_p->StartCondition.MaxWaitForBufferLevel) != 0))
            {
                /* We start the decode on the vbv_buffer_size. IsDecodeEnable becomes true only when the Bit Buffer Level goes up to vbv_buffer_size */
                /* Decode disabled if waiting for vbv_size in bit buffer */
                /* vbv_buffer_size reached in bit buffer: enable decodes. */
                VIDPROD_Data_p->StartCondition.IsDecodeEnable = TRUE;
#if defined (STVID_DEBUG_GET_STATISTICS)
                VIDPROD_Data_p->Statistics.DecodeStartConditionVbvBufferSize++;
#endif /* STVID_DEBUG_GET_STATISTICS */
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODER_START, "DecEnableVBVBuffSize "));
#endif
            }
        }
} /* End of decode start condition check */

/*******************************************************************************
Name        : vidprod_CheckSequenceInfoChangeAndNotify
Description : Compare current sequence info structure content to previous one
              and send appropriate event if a change has been detected
Parameters  : Producer handle, current sequence info structure
Assumptions :
Limitations :
Returns     : Boolean, indicating if the event can be notified.
*******************************************************************************/
static void vidprod_CheckSequenceInfoChangeAndNotify(const VIDPROD_Handle_t ProducerHandle, STVID_SequenceInfo_t SequenceInfo)
{
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    STVID_SequenceInfo_t SequenceInfoToSend;

    if ((VIDPROD_Data_p->SendSequenceInfoEventOnceAfterStart) || /* Requested by vidprod_Start() */
        (VIDPROD_Data_p->PreviousSequenceInfo.Height    != SequenceInfo.Height) || /* Height changing or... */
        (VIDPROD_Data_p->PreviousSequenceInfo.Width     != SequenceInfo.Width) || /* Width changing or... */
        (VIDPROD_Data_p->PreviousSequenceInfo.Aspect    != SequenceInfo.Aspect) || /* Aspect changing or... */
        (VIDPROD_Data_p->PreviousSequenceInfo.ScanType  != SequenceInfo.ScanType) || /* ScanType changing or... */
        (VIDPROD_Data_p->PreviousSequenceInfo.FrameRate != SequenceInfo.FrameRate) || /* FrameRate changing */
        (VIDPROD_Data_p->PreviousSequenceInfo.MPEGStandard != SequenceInfo.MPEGStandard) /* MPEGStandard changing */
        )
    {
        /* Send EVENT_ID_SEQUENCE_INFO EVT, it can only be send after having next start code after the sequence */
        SequenceInfoToSend = SequenceInfo;
        /* Change VBVBufferSize unit (in bytes for the producer and its codecs) to MPEG norm notation to comply with STVID API */
        SequenceInfoToSend.VBVBufferSize = SequenceInfo.VBVBufferSize / 2048;
        STEVT_Notify(VIDPROD_Data_p->EventsHandle, VIDPROD_Data_p->RegisteredEventsID[VIDPROD_SEQUENCE_INFO_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(SequenceInfoToSend));
        /* Store this sequence information */
        VIDPROD_Data_p->PreviousSequenceInfo = SequenceInfo;
        /* Event has been sent once after start: now only when parameters are changing */
        VIDPROD_Data_p->SendSequenceInfoEventOnceAfterStart = FALSE;
    }
}

/*******************************************************************************
Name        : vidprod_IsEventToBeNotified
Description : Test if the corresponding event is to be notified (enabled,  ...)
Parameters  : Decoder handle,
              event ID (defined in vid_dec.h).
Assumptions : Event_ID is supposed to be valid.
Limitations :
Returns     : Boolean, indicating if the event can be notified.
*******************************************************************************/
static BOOL vidprod_IsEventToBeNotified(const VIDPROD_Handle_t ProducerHandle, const U32 Event_ID)
{
    BOOL RetValue = FALSE;
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

    STVID_ConfigureEventParams_t * ConfigureEventParams_p;
    U32                          * NotificationSkipped_p;

    /* Get the event configuration. */
    ConfigureEventParams_p = &VIDPROD_Data_p->EventNotificationConfiguration[Event_ID].ConfigureEventParam;
    NotificationSkipped_p  = &VIDPROD_Data_p->EventNotificationConfiguration[Event_ID].NotificationSkipped;

    /* test if the event is enabled. */
    if (ConfigureEventParams_p->Enable)
    {
        switch (ConfigureEventParams_p->NotificationsToSkip)
        {
            case 0 :
                RetValue = TRUE;
                break;

            case 0xFFFFFFFF :
                if ( (*NotificationSkipped_p) == 0xFFFFFFFF)
                {
                    RetValue = TRUE;
                    *NotificationSkipped_p = 0;
                }
                else
                {
                    (*NotificationSkipped_p)++;
                }
                break;

            default :
                if ( (*NotificationSkipped_p)++ >= ConfigureEventParams_p->NotificationsToSkip)
                {
                    RetValue = TRUE;
                    (*NotificationSkipped_p) = 0;
                }
                break;
        }
    }

    /* Return the result... */
    return(RetValue);

} /* End of vidprod_IsEventToBeNotified() function. */

/*******************************************************************************
Name        : vidprod_PatchColorSpaceConversionForMPEG
Description : Set default ColorSpaceConversion for MPEG PES/ES according to broadcast profile
Parameters  : Producer handle,
              picture to patch
Assumptions :
Limitations :
Returns     : void
*******************************************************************************/
static void vidprod_PatchColorSpaceConversionForMPEG(const VIDPROD_Handle_t ProducerHandle, VIDBUFF_PictureBuffer_t *PictureBuffer_p)
{
    #define PROFILE_MAIN                            0x40
    #define LEVEL_MAIN                              0x08
    #define PROFILE_AND_LEVEL_IDENTIFICATION_MASK   0x7F
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    STGXOBJ_ColorSpaceConversionMode_t ColorSpaceConversionMode;
    STVID_SequenceInfo_t *SequenceInfo_p;

    switch (VIDPROD_Data_p->BroadcastProfile)
    {
#ifdef STVID_DIRECTV
        case STVID_BROADCAST_DIRECTV : /* DirecTV super-sets MPEG-2 norm */
            ColorSpaceConversionMode = STGXOBJ_SMPTE_170M;
            break;
#endif /* STVID_DIRECTV */
        case STVID_BROADCAST_DVB : /* DVB super-sets MPEG-2 norm */
            SequenceInfo_p = &(PictureBuffer_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo);
            switch (SequenceInfo_p->ProfileAndLevelIndication & PROFILE_AND_LEVEL_IDENTIFICATION_MASK)
            {
                case (PROFILE_MAIN | LEVEL_MAIN) : /* MP@ML , SD */
                    switch (SequenceInfo_p->FrameRate)
                    {
                        case 25000 : /* 25 Hz */
                        case 50000 :
                            ColorSpaceConversionMode = STGXOBJ_ITU_R_BT470_2_BG;
                            break;

                        default : /* 30 Hz */
                            ColorSpaceConversionMode = STGXOBJ_SMPTE_170M;
                            break;
                    }
                    break;

                default : /* HD */
                    ColorSpaceConversionMode = STGXOBJ_ITU_R_BT709;
                    break;
            }
            break; /* STVID_BROADCAST_DVB */

        case STVID_BROADCAST_ATSC : /* ATSC follows MPEG-2 norm */
        case STVID_BROADCAST_ARIB : /* ARIB follows MPEG-2 norm */
        default :   /* Default in MPEG-2 norm */
            ColorSpaceConversionMode = STGXOBJ_ITU_R_BT709;
            break;
    } /* end switch (BroadcastProfile) */
    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion = ColorSpaceConversionMode;
} /* End of vidprod_PatchColorSpaceConversionForMPEG */

/*******************************************************************************
Name        : vidprod_PatchColorSpaceConversionForH264
Description : Set default ColorSpaceConversion for H264 PES/ES according to broadcast profile
Parameters  : Producer handle,
              picture to patch
Assumptions :
Limitations :
Returns     : void
*******************************************************************************/
static void vidprod_PatchColorSpaceConversionForH264(const VIDPROD_Handle_t ProducerHandle, VIDBUFF_PictureBuffer_t *PictureBuffer_p)
{
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    STGXOBJ_ColorSpaceConversionMode_t ColorSpaceConversionMode;

    /* To avoid warnings */
    VIDPROD_Data_p = VIDPROD_Data_p;

    /* TODO_CL: implement ColorSpaceConversionMode choice according to DirecTV & DVB recommendations for H264 boradcasting */
    ColorSpaceConversionMode = STGXOBJ_ITU_R_BT709;
    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion = ColorSpaceConversionMode;
} /* End of vidprod_PatchColorSpaceConversionForH264 */

/*******************************************************************************
Name        : vidprod_PatchColorSpaceConversionForMPEG4P2
Description : Set default ColorSpaceConversion for MPEG4P2 PES/ES according to broadcast profile
Parameters  : Producer handle,
              picture to patch
Assumptions :
Limitations :
Returns     : void
*******************************************************************************/
static void vidprod_PatchColorSpaceConversionForMPEG4P2(const VIDPROD_Handle_t ProducerHandle, VIDBUFF_PictureBuffer_t *PictureBuffer_p)
{
    STGXOBJ_ColorSpaceConversionMode_t ColorSpaceConversionMode;

    UNUSED_PARAMETER(ProducerHandle);
    ColorSpaceConversionMode = STGXOBJ_ITU_R_BT709;
    PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion = ColorSpaceConversionMode;
} /* End of vidprod_PatchColorSpaceConversionForMPEG4P2 */

/*******************************************************************************
Name        : InitUnderflowEvent
Description : Initialise the data associated to the Underflow event.
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void InitUnderflowEvent(const VIDPROD_Handle_t ProducerHandle)
{
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

    VIDPROD_Data_p->DataUnderflow.BitBufferFreeSize = 0;
    VIDPROD_Data_p->DataUnderflow.BitRateValue = 0;
    VIDPROD_Data_p->DataUnderflow.RequiredTimeJump = 0;
    VIDPROD_Data_p->DataUnderflow.RequiredDuration = 0;
    VIDPROD_Data_p->DataUnderflowEventNotSatisfied = FALSE;
} /* end of InitUnderflowEvent() */
/*******************************************************************************
Name        : vidprod_RemovePictureFromDecodeStatusTable
Description : Remove a picture from the DecoderStatus table (used in error recovery for parsed but not yet decoded pictures)
Parameters  : Producer handle, picture to remove from DecoderStatus table
Assumptions : Access to Decode Pictures has been locked by STOS_SemaphoreWait call with VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p
Limitations :
Returns     : Nothing
*******************************************************************************/
static void vidprod_RemovePictureFromDecodeStatusTable(const VIDPROD_Handle_t ProducerHandle, VIDBUFF_PictureBuffer_t *PictureBuffer_p)
{
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    U32 PPIndex, NextPPIndex, CurrWritePtr;

    if (PictureBuffer_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "NULL pointer passed !!! (BUG-BUG-BUG)"));
        return;
    }

    /* remove current picture from CurrentPictures_p[] table */
    if (VIDPROD_Data_p->DecoderStatus.NbIndices > 0)
    {
        PPIndex = VIDPROD_Data_p->DecoderStatus.NextOutputIndex;
        CurrWritePtr = (VIDPROD_Data_p->DecoderStatus.NextOutputIndex + VIDPROD_Data_p->DecoderStatus.NbIndices) % MAX_CURRENT_PICTURES;
        while (PPIndex != CurrWritePtr)
        {
            if (PictureBuffer_p == VIDPROD_Data_p->DecoderStatus.CurrentPictures_p[PPIndex])
            {
                /* picture found, overtake it and compact the table */
                PictureBuffer_p->Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_EMPTY;
                if (PictureBuffer_p->AssociatedDecimatedPicture_p != NULL)
                {
                    PictureBuffer_p->AssociatedDecimatedPicture_p->Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_EMPTY;
                }
                NextPPIndex = (PPIndex + 1) % MAX_CURRENT_PICTURES;
                while (NextPPIndex != CurrWritePtr)
                {
                    VIDPROD_Data_p->DecoderStatus.CurrentPictures_p[PPIndex] = VIDPROD_Data_p->DecoderStatus.CurrentPictures_p[NextPPIndex];
                    PPIndex = (PPIndex + 1) % MAX_CURRENT_PICTURES;
                    NextPPIndex = (PPIndex + 1) % MAX_CURRENT_PICTURES;
                }
                /* Clear unused pointer for correct debug tools behavior */
                VIDPROD_Data_p->DecoderStatus.CurrentPictures_p[PPIndex] = NULL;
                break;
            }
            PPIndex = (PPIndex + 1) % MAX_CURRENT_PICTURES;
        }
        VIDPROD_Data_p->DecoderStatus.NbIndices--;

    }
} /* end of vidprod_RemovePictureFromDecodeStatusTable */

/*******************************************************************************
Name        : vidprod_RemovePictureFromPreprocessingTable
Description : Remove a picture from the preprocessing table (used in error recovery for parsed but not yet decoded pictures)
Parameters  : Producer handle, picture to remove from preprocessing table
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void vidprod_RemovePictureFromPreprocessingTable(const VIDPROD_Handle_t ProducerHandle, VIDBUFF_PictureBuffer_t *PictureBuffer_p)
{
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    U32 IndexToRemove, LoopCounter;

    if (PictureBuffer_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "NULL pointer passed !!! (BUG-BUG-BUG)"));
        return;
    }

    /* Lock Access to Preprocessing Pictures */
    STOS_SemaphoreWait(VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureSemaphore_p);
    IndexToRemove = MAX_NUMBER_OF_PREPROCESSING_PICTURES;
    for(LoopCounter = 0 ; LoopCounter < MAX_NUMBER_OF_PREPROCESSING_PICTURES ; LoopCounter++)
    {
        if (PictureBuffer_p == &(VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter]))
        {
            IndexToRemove = LoopCounter;
        }
    }
    if (IndexToRemove == MAX_NUMBER_OF_PREPROCESSING_PICTURES)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unable to find picture to remove (%d-%d/%d) in preprocessing table !!! (BUG-BUG-BUG)",
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
            PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
    }
    else
    {
        /* Then free PictureBuffer_p from PreprocessingPicture Pool */
        VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[IndexToRemove].Decode.CommandId = 0;
        VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[IndexToRemove].Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_EMPTY;
        VIDPROD_Data_p->PreprocessingPicture.IsPictureBufferUsed[IndexToRemove] = FALSE;
        VIDPROD_Data_p->PreprocessingPicture.IsPictureTimedOut[IndexToRemove] = FALSE;
        VIDPROD_Data_p->PreprocessingPicture.NbUsedPictureBuffers--;
        /* Lock Access to Decode Pictures */
        STOS_SemaphoreWait(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);
        /* Remove picture from DecoderStatus table too */
        vidprod_RemovePictureFromDecodeStatusTable(ProducerHandle, PictureBuffer_p);
        /* UnLock Access to Decode Pictures */
        STOS_SemaphoreSignal(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);
    }

    /* UnLock Access to Preprocessing Pictures */
    STOS_SemaphoreSignal(VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureSemaphore_p);
} /* end of vidprod_RemovePictureFromPreprocessingTable */
#ifdef STVID_TRICKMODE_BACKWARD
/*******************************************************************************
Name        : vidprod_RemovePictureFromPreprocessingTableBackward
Description : Remove a picture from the preprocessing table (used in error recovery for parsed but not yet decoded pictures)
Parameters  : Producer handle, picture to remove from preprocessing table
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void vidprod_RemovePictureFromPreprocessingTableBackward(const VIDPROD_Handle_t ProducerHandle, VIDBUFF_PictureBuffer_t *PictureBuffer_p)
{
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    U32 IndexToRemove, LoopCounter;

    if (PictureBuffer_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "NULL pointer passed !!! (BUG-BUG-BUG)"));
        return;
    }


    if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p != NULL)
    {
        /* Lock Access to Preprocessing Pictures */
        STOS_SemaphoreWait(VIDPROD_Data_p->LinearBitBuffers.PreprocessingPictureSemaphore_p);

        IndexToRemove = MAX_BACKWARD_PICTURES;

        for(LoopCounter = 0 ; LoopCounter < MAX_BACKWARD_PICTURES ; LoopCounter++)
        {
            if (PictureBuffer_p == &(VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter]))
            {
                IndexToRemove = LoopCounter;
            }
        }
        if (IndexToRemove == MAX_BACKWARD_PICTURES)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unable to find picture to remove (%d-%d/%d) in preprocessing table backward!!! (BUG-BUG-BUG)",
                PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
        }
        else
        {
            /* Then free PictureBuffer_p from PreprocessingPicture Pool */
            VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[IndexToRemove].Decode.CommandId = 0;
            VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[IndexToRemove].Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_EMPTY;
            VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.IsPictureBufferUsed[IndexToRemove] = FALSE;
            VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.IsPictureTimedOut[IndexToRemove] = FALSE;
            VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.NbUsedPictureBuffers--;
            /* Lock Access to Decode Pictures */
            STOS_SemaphoreWait(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);
            /* Remove picture from DecoderStatus table too */
            vidprod_RemovePictureFromDecodeStatusTable(ProducerHandle, PictureBuffer_p);
            /* UnLock Access to Decode Pictures */
            STOS_SemaphoreSignal(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);
        }

        /* UnLock Access to Preprocessing Pictures */
        STOS_SemaphoreSignal(VIDPROD_Data_p->LinearBitBuffers.PreprocessingPictureSemaphore_p);
    }
} /* end of vidprod_RemovePictureFromPreprocessingTableBackward */
#endif /* STVID_TRICKMODE_BACKWARD */
#ifdef CL_FILL_BUFFER_PATTERN
/*******************************************************************************
Name        : vidprod_FillPattern
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void vidprod_FillFrameBufferWithPattern(const VIDPROD_Handle_t ProducerHandle, VIDBUFF_PictureBuffer_t *PictureBuffer_p)
{
    #define LUMA_WORD_FOR_GREY_COLOR       0x80808080
    #define CHROMA_WORD_FOR_GREY_COLOR     0x80808080

    VIDPROD_Data_t *    VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    STVID_ClearParams_t ClearFrameBufferParams;
    U32             LumaPattern;
    U32             ChromaPattern;

    LumaPattern   = LUMA_WORD_FOR_GREY_COLOR;
    ChromaPattern = CHROMA_WORD_FOR_GREY_COLOR;
    ClearFrameBufferParams.ClearMode = STVID_CLEAR_DISPLAY_PATTERN_FILL;
    ClearFrameBufferParams.PatternAddress1_p = &LumaPattern;
    ClearFrameBufferParams.PatternSize1 = 4;
    ClearFrameBufferParams.PatternAddress2_p = &ChromaPattern;
    ClearFrameBufferParams.PatternSize2 = 4;
    VIDBUFF_ClearFrameBuffer(VIDPROD_Data_p->BufferManagerHandle, &(PictureBuffer_p->FrameBuffer_p->Allocation), &ClearFrameBufferParams);
} /* end of vidprod_FillPattern */
#endif

/*******************************************************************************
Name        : vidprod_SearchAndConfirmBuffer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void vidprod_SearchAndConfirmBuffer(const VIDPROD_Handle_t ProducerHandle, STOS_Clock_t TOut_ClockTicks)
{
    VIDPROD_Data_t          * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    ST_ErrorCode_t          ErrorCode;
    U32                     LoopCounter;
    VIDBUFF_PictureBuffer_t * NextPictureInDecodeOrder_p;
    VIDBUFF_PictureBuffer_t * DecodedPicture_p;
    VIDBUFF_PictureBuffer_t * DecodedDecimPicture_p;
    BOOL                    BrokenLinkActive;
    BOOL                    PictureToConfirmHasMissingRef;
    U32                     ReferenceCounter;
    U32                     PPIndex;
    U32                     CurrWritePtr;
    DECODER_ConfirmBufferParams_t ConfirmBufferParams;
    VIDBUFF_Infos_t         BufferInfos;
    U32                     StreamWidth, StreamHeight;
    U8                      MaxNumberOfFrames;
    BOOL                    NeedToChangeFrameBufferParams;
    BOOL                    ConfirmBufferSuccess;
#ifdef ST_speed
	VIDPROD_NewPictureDecodeOrSkip_t NewPictureDecodeOrSkipEventData;
#endif
    BOOL                    CurrentPictureMustBeSkipped = FALSE;

#if !defined(STTBX_REPORT) || defined(STVID_DONT_REPORT_LACK_OF_BUFFER)
    /* TOut_clockTicks is used only in a STTBX_PRINT, flag it unused to avoid a warning in non-debug mode */
    UNUSED_PARAMETER(TOut_ClockTicks);
#endif

    if ((VIDPROD_Data_p->NextAction.ConfirmBufferWhenPossible != 0) &&
        (VIDPROD_Data_p->ForTask.PictureToConfirm_p == NULL))
    {
        LoopCounter = 0;
        /* Look for oldest picture waiting for confirm buffer in decoding order */
#ifdef STVID_TRICKMODE_BACKWARD
        if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
		{
            while ((LoopCounter < VIDPROD_Data_p->DecoderStatus.NbIndices) && (VIDPROD_Data_p->ForTask.PictureToConfirm_p == NULL))
            {
                NextPictureInDecodeOrder_p = VIDPROD_Data_p->DecoderStatus.CurrentPictures_p[(VIDPROD_Data_p->DecoderStatus.NextOutputIndex+LoopCounter) % MAX_CURRENT_PICTURES];
                /* Only temporary to prevent crashes as we have holes in Currentpictures_p table when switching back in forward */
                if (NextPictureInDecodeOrder_p != NULL)
                {
                    if(NextPictureInDecodeOrder_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_WAITING_FRAME_BUFFER)
                    {
                        VIDPROD_Data_p->ForTask.PictureToConfirm_p = NextPictureInDecodeOrder_p;
                    }
                }
                LoopCounter++;
            }
		}
#ifdef STVID_TRICKMODE_BACKWARD
		else
		{
			LoopCounter = 0;
        	/*while ((LoopCounter < MAX_BACKWARD_PICTURES) && (VIDPROD_Data_p->ForTask.PictureToConfirm_p == NULL))*/
			for(LoopCounter = 0 ; LoopCounter < MAX_BACKWARD_PICTURES ; LoopCounter++)
        	{
                NextPictureInDecodeOrder_p =
                    &(VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter ]);
                if (VIDPROD_Data_p->ForTask.PictureToConfirm_p == NULL)
                {
                    if(NextPictureInDecodeOrder_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_WAITING_FRAME_BUFFER)
                    {
                        VIDPROD_Data_p->ForTask.PictureToConfirm_p = NextPictureInDecodeOrder_p;
                    }
                }
                else
                {
                    if ((NextPictureInDecodeOrder_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_WAITING_FRAME_BUFFER) &&
						(NextPictureInDecodeOrder_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID > VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID))
                    {
                        VIDPROD_Data_p->ForTask.PictureToConfirm_p = NextPictureInDecodeOrder_p;
                    }
                }
        	}
		}
#endif /* STVID_TRICKMODE_BACKWARD */
        if (VIDPROD_Data_p->ForTask.PictureToConfirm_p == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDPROD_Data_p->NextAction.ConfirmBufferWhenPossible = %d inconsistant (BUG-BUG-BUG) !!", VIDPROD_Data_p->NextAction.ConfirmBufferWhenPossible));
            VIDPROD_Data_p->NextAction.ConfirmBufferWhenPossible = 0;
            vidprod_ControllerReset(ProducerHandle);
            return;
        }
        /* Look for found picture in PreprocessingPictureTable to retrieve the index */
        LoopCounter = 0;
        VIDPROD_Data_p->ForTask.PreprocessingPictureNumber = 0;
#ifdef STVID_TRICKMODE_BACKWARD
        if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
        {
        while ((LoopCounter < MAX_NUMBER_OF_PREPROCESSING_PICTURES) && (VIDPROD_Data_p->ForTask.PreprocessingPictureNumber == 0))
        {
            if(VIDPROD_Data_p->ForTask.PictureToConfirm_p == &(VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter]))
            {
                VIDPROD_Data_p->ForTask.PreprocessingPictureNumber = LoopCounter;
            }
            LoopCounter++;
        }
        }
#ifdef STVID_TRICKMODE_BACKWARD
        else if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p != NULL)
        {
            while ((LoopCounter < MAX_BACKWARD_PICTURES) && (VIDPROD_Data_p->ForTask.PreprocessingPictureNumber == 0))
            {
                if(VIDPROD_Data_p->ForTask.PictureToConfirm_p == &(VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter]))
                {
                    VIDPROD_Data_p->ForTask.PreprocessingPictureNumber = LoopCounter;
                }
                LoopCounter++;
            }
        }
#endif /* STVID_TRICKMODE_BACKWARD */
        /*  Fix timeout for getting a new free buffer */
        VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.BufferSeachStartTime = time_now();
        VIDPROD_Data_p->DecoderStatus.MaxBufferWaitingTime = time_plus(VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.BufferSeachStartTime, MAX_TIME_BEFORE_STOPPING_DECODER);
        if (VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ParsingError >= VIDCOM_ERROR_LEVEL_CRASH)
        {
            CurrentPictureMustBeSkipped = TRUE;
        }
        /* Update reference frames pointers */
        VIDPROD_Data_p->ForTask.PictureToConfirm_p->ReferenceDecodingError = VIDCOM_ERROR_LEVEL_NONE;
        PictureToConfirmHasMissingRef = FALSE;
        BrokenLinkActive = FALSE;
        ReferenceCounter = 0;
        while ((!CurrentPictureMustBeSkipped) && (ReferenceCounter < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES))
        {
            VIDPROD_Data_p->ForTask.PictureToConfirm_p->ReferencesFrameBufferPointer[ReferenceCounter] = NULL;
            if (VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.IsValidIndexInReferenceFrame[ReferenceCounter])
            {
                /* look for reference frame buffer according to DecodingOrderFrameID */
                /* and compute VIDPROD_Data_p->ForTask.PictureToConfirm_p->ReferenceDecodingError for future use in error recovery,
                    * if the picture is decodable (i.e. all its refs are available) some of its refs may have decoding errors which may alter the final visual result */
                LoopCounter = 0;
                do
                {
                    if ((VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter] != NULL) &&
                        (VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID == VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.FullReferenceFrameList[ReferenceCounter]))
                    {
                        VIDPROD_Data_p->ForTask.PictureToConfirm_p->ReferencesFrameBufferPointer[ReferenceCounter] = VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->FrameBuffer_p;
                        if (VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.DecodingError > VIDPROD_Data_p->ForTask.PictureToConfirm_p->ReferenceDecodingError)
                        {
                            VIDPROD_Data_p->ForTask.PictureToConfirm_p->ReferenceDecodingError = VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.DecodingError;
                        }
                    }
                    LoopCounter++;
                } while ((LoopCounter < MAX_NUMBER_OF_DECODE_PICTURES) &&
                            (VIDPROD_Data_p->ForTask.PictureToConfirm_p->ReferencesFrameBufferPointer[ReferenceCounter] == NULL));

                if (VIDPROD_Data_p->ForTask.PictureToConfirm_p->ReferencesFrameBufferPointer[ReferenceCounter] == NULL)
                {
                    /* In case of decode I only mode, if we get I picture with not empty reflist ignore missing refs and patch references FB with current decode FB */
                    /* The actual patch of reference is done later in this function after the call to GetBuffers() */
                    if (IsIOnlyDecodeOnGoing(VIDPROD_Data_p->Skip) &&
                        (VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I))
                    {
                        PictureToConfirmHasMissingRef = FALSE;
                        VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.IsValidIndexInReferenceFrame[ReferenceCounter] = FALSE;
                    }
                    else
                    {
                    PictureToConfirmHasMissingRef = TRUE;
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_ERRORS, "pDEC_CBErr4-%d RefNotFound DOID=%d",
                        VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId,
                        VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.FullReferenceFrameList[ReferenceCounter]));
#endif
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "PROD_CB: jobID %d %c(%d-%d/%d), RefPicture not found DOID=%d!!",
                        VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId,
                            MPEGFrame2Char(VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame),
                            VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                            VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                            VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                        VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.FullReferenceFrameList[ReferenceCounter]));
                    }
                }

                if (VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.IsBrokenLinkIndexInReferenceFrame[ReferenceCounter])
                {
                    /* TODO_CL: add here usage of reference picture set by vidprod_NewReferencePicture() in order to decode the no I streams */
                    /* Replace references marked as broken link with a suitable default frame buffer. */
                    /* This will be usefull when producer decides to decode the picture even with a missing reference if the error recovery level allows this */
                    /* VIDPROD_Data_p->ForTask.PictureToConfirm_p->ReferencesFrameBufferPointer[ReferenceCounter] = Frame buffer containing the picture passed to last vidprod_NewReferencePicture() call */
                    /* For now we consider there is no suitable reference to use so we treat broken link flag as missing ref information for the picture */
                    PictureToConfirmHasMissingRef = TRUE;
                    BrokenLinkActive = TRUE;
                }
            }

            ReferenceCounter++;
        } /* end while ((!CurrentPictureMustBeSkipped) && (ReferenceCounter < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES)) */

#ifdef STVID_DEBUG_GET_STATISTICS
        if (CurrentPictureMustBeSkipped)
        {
            VIDPROD_Data_p->Statistics.DecodePictureSkippedNotRequested++;
        }
#endif

        if ( ((!VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.IsReference) && /* Cases of skip for non ref pictures : */
              ((BrokenLinkActive) ||
               ((PictureToConfirmHasMissingRef) &&
               (!(IsNonRefPictureToBeDecodedIfMissingReference(VIDPROD_Data_p->ErrorRecoveryMode)))))) || /* no reference present, in some error recovery modes */
             ((VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.IsReference) && /* Cases of skip for reference pictures : */
               (((PictureToConfirmHasMissingRef) &&
               (!(IsRefPictureToBeDecodedIfMissingReference(VIDPROD_Data_p->ErrorRecoveryMode)))))) ||  /* no reference present, in some error recovery modes */
            (CurrentPictureMustBeSkipped))
        {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
            TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_ERRORS, "pDEC_CBErr6-%d ", VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId));
            TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODER_PROCESSING, "DEC_CBAbort-%d ", VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId));
#endif
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER_Abort ID=%d (%d-%d/%d)", VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId,
                    VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                    VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                    VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID
                    ));

            VIDPROD_Data_p->ForTask.NewPictureDecodeOrSkipEventData.NbFieldsForDisplay = VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.NbFieldsDisplay;
            STEVT_Notify(VIDPROD_Data_p->EventsHandle, VIDPROD_Data_p->RegisteredEventsID[VIDPROD_PICTURE_SKIPPED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST (&VIDPROD_Data_p->ForTask.NewPictureDecodeOrSkipEventData.NbFieldsForDisplay));
#ifdef STVID_TRICKMODE_BACKWARD
            VIDPROD_Data_p->Backward.NbDecodedPictures ++;
#endif
            ErrorCode = DECODER_Abort(VIDPROD_Data_p->DecoderHandle, VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId);
            if (ErrorCode != ST_NO_ERROR)
            {
                /* NOTE_CL: unexpected error, decoder internal data failure or bad producer data handling */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER_Abort rejected ID=%d!!", VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId));
            }
            if ((VIDPROD_Data_p->Stop.AfterCurrentPicture) &&
                (VIDPROD_Data_p->LastParsedPictureDecodingOrderFrameID ==
                    VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID) &&
                (vidcom_ComparePictureID(&(VIDPROD_Data_p->LastParsedPictureExtendedPresentationOrderPictureID),
                    &(VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID)) == 0))
            {
                /* Order stop now */
                PushControllerCommand(ProducerHandle, CONTROLLER_COMMAND_STOP);
                VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint = TRUE;
                VIDPROD_Data_p->Skip.RecoveringNextEntryPoint = TRUE;
                VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = TRUE;
            }

            if ((VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.IsReference) && IsSpoiledReferenceLoosingAllReferencesUntilNextRandomAccessPoint(VIDPROD_Data_p->ErrorRecoveryMode))
            {
                if ((!IsIOnlyDecodeOnGoing(VIDPROD_Data_p->Skip)) || (IsFirstOfTwoFieldsOfIFrame(VIDPROD_Data_p->ForTask.PictureToConfirm_p)))
				{
                    /* Search next entry point because a reference picture has been aborted and we are not decoding I only */
                	STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Ask for skip until next RAP"));
                	VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint = TRUE;
                    VIDPROD_Data_p->Skip.RecoveringNextEntryPoint = TRUE;
                    VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = TRUE;
				}
            }
            /* Release preprocessing picture used by this aborted picture */
#ifdef STVID_TRICKMODE_BACKWARD
            if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
            {
                vidprod_RemovePictureFromPreprocessingTable(ProducerHandle, VIDPROD_Data_p->ForTask.PictureToConfirm_p);
            }
#ifdef STVID_TRICKMODE_BACKWARD
            else
            {
                vidprod_RemovePictureFromPreprocessingTableBackward(ProducerHandle, VIDPROD_Data_p->ForTask.PictureToConfirm_p);
            }
#endif /* STVID_TRICKMODE_BACKWARD */
            /* This decode is aborted, decrease VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue */
            DECREMENT_NB_PICTURES_IN_QUEUE(ProducerHandle);
            /* We get an error before confirming the decode.
            * We have to order the stop.                    */
            if ((VIDPROD_Data_p->Stop.AfterCurrentPicture) &&
                (VIDPROD_Data_p->LastParsedPictureDecodingOrderFrameID ==
                    VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID) &&
                (vidcom_ComparePictureID(&(VIDPROD_Data_p->LastParsedPictureExtendedPresentationOrderPictureID),
                    &(VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID)) == 0))
            {
                /* Order stop now */
                PushControllerCommand(ProducerHandle, CONTROLLER_COMMAND_STOP);
                VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint = TRUE;
                VIDPROD_Data_p->Skip.RecoveringNextEntryPoint = TRUE;
                VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = TRUE;
            }
            if (VIDPROD_Data_p->NextAction.ConfirmBufferWhenPossible > 0)
            {
                VIDPROD_Data_p->NextAction.ConfirmBufferWhenPossible--;
            }
            else
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ConfirmBufferWhenPossible was already == 0 !!! "));
            }
            VIDPROD_Data_p->ForTask.PictureToConfirm_p = NULL;
        }
    }

    /* Current picture to confirm is VIDPROD_Data_p->ForTask.PictureToConfirm_p */
    if (VIDPROD_Data_p->ForTask.PictureToConfirm_p != NULL)
    {
        /* Notify event VIDPROD_READY_TO_DECODE_NEW_PICTURE_EVT */
        if(vidcom_ComparePictureID(&VIDPROD_Data_p->ForTask.NewPictureDecodeOrSkipEventData.ExtendedPresentationOrderPictureID, &VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID) != 0)
        {
            VIDPROD_Data_p->ForTask.NewPictureDecodeOrSkipEventData.PictureInfos_p = &(VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.PictureInfos);
            VIDPROD_Data_p->ForTask.NewPictureDecodeOrSkipEventData.NbFieldsForDisplay = VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.NbFieldsDisplay;
            VIDPROD_Data_p->ForTask.NewPictureDecodeOrSkipEventData.TemporalReference = VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID;
            VIDPROD_Data_p->ForTask.NewPictureDecodeOrSkipEventData.ExtendedPresentationOrderPictureID = VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID;
            VIDPROD_Data_p->ForTask.NewPictureDecodeOrSkipEventData.IsExtendedPresentationOrderIDValid = VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.IsExtendedPresentationOrderIDValid;
            VIDPROD_Data_p->ForTask.NewPictureDecodeOrSkipEventData.VbvDelay = 0; /* TODO_CL, where to catch this vbvdelay ? */

            STEVT_Notify(VIDPROD_Data_p->EventsHandle, VIDPROD_Data_p->RegisteredEventsID[VIDPROD_READY_TO_DECODE_NEW_PICTURE_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(VIDPROD_Data_p->ForTask.NewPictureDecodeOrSkipEventData));
        }

#if defined(TRACE_UART) && defined(VALID_TOOLS)
        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODER_PROCESSING, "pDEC_CBSearch%d(%d-%d/%d) ", VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
        if (IsDynamicAllocationProfile(VIDPROD_Data_p->ForTask.CurrentProfile))
        {
            StreamWidth = VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Width;
            StreamHeight = VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Height;
            MaxNumberOfFrames = VIDPROD_Data_p->ForTask.PictureToConfirm_p->GlobalDecodingContext.GlobalDecodingContextGenericData.MaxDecFrameBuffering + 2;
            NeedToChangeFrameBufferParams = VIDBUFF_NeedToChangeFrameBufferParams(VIDPROD_Data_p->BufferManagerHandle, StreamWidth, StreamHeight, MaxNumberOfFrames);
            if (NeedToChangeFrameBufferParams)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "VIDBUFF_ChangeFrameBufferParams Requested while searching FB for Job ID=%d (%d-%d/%d)",
                        VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId,
                        VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                        VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                        VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID
                         ));
                VIDPROD_Data_p->ForTask.NeedForExtraAllocation = TRUE;

                /* Update Applicable Decimation Factor */
                VIDBUFF_GetApplicableDecimationFactor(VIDPROD_Data_p->BufferManagerHandle, &VIDPROD_Data_p->ForTask.CurrentProfile.ApplicableDecimationFactor);
                if (VIDPROD_Data_p->ForTask.CurrentProfile.ApplicableDecimationFactor != STVID_DECIMATION_FACTOR_NONE)
                {
                    VIDPROD_Data_p->ForTask.NeedForExtraAllocationDecimated = TRUE;
                }

                ErrorCode = VIDBUFF_ChangeFrameBufferParams(VIDPROD_Data_p->BufferManagerHandle, StreamWidth, StreamHeight, MaxNumberOfFrames);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDBUFF_ChangeFrameBufferParams Error (0x%x) while looking for buffer for Job ID=%d!!", ErrorCode, VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId));
                }
            }
        }
        else
        {
            VIDPROD_Data_p->ForTask.NeedForExtraAllocation = FALSE;
            VIDPROD_Data_p->ForTask.NeedForExtraAllocationDecimated = FALSE;
        }
        ErrorCode = GetBuffers(ProducerHandle, VIDPROD_Data_p->ForTask.PictureToConfirm_p, &DecodedPicture_p, &DecodedDecimPicture_p);
        if ((ErrorCode != ST_NO_ERROR) && (ErrorCode != ST_ERROR_NO_MEMORY))
        {
            /* We get an error before confirming the decode.
             * We have to order the stop.                    */
            if ((VIDPROD_Data_p->Stop.AfterCurrentPicture) &&
                (VIDPROD_Data_p->LastParsedPictureDecodingOrderFrameID ==
                    VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID) &&
                (vidcom_ComparePictureID(&(VIDPROD_Data_p->LastParsedPictureExtendedPresentationOrderPictureID),
                    &(VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID)) == 0))
            {
                /* Order stop now */
                PushControllerCommand(ProducerHandle, CONTROLLER_COMMAND_STOP);
                VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint = TRUE;
                VIDPROD_Data_p->Skip.RecoveringNextEntryPoint = TRUE;
                VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = TRUE;
            }
        }

        if (ErrorCode == ST_ERROR_NO_MEMORY)
        {
            vidprod_TryToPushPictureToDisplay(ProducerHandle);
#if !defined(STVID_PRODUCER_NO_TIMEOUT) && !defined(STVID_DONT_REPORT_LACK_OF_BUFFER)
            if (STOS_time_after(VIDPROD_Data_p->DecoderStatus.MaxBufferWaitingTime, time_now()) == 0)
            {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_ERRORS, "pDEC_CBSearchTO-%d(%d-%d/%d) ", VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
                TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODER_PROCESSING, "pDEC_CBAbort-%d(%d-%d/%d) ", VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Producer DecoderConfirmBufferCB : Time out on frame buffer availability ID=%d(%c%d-%d/%d) (Maxdelay=%"FORMAT_SPEC_OSCLOCK"d us, actual=%"FORMAT_SPEC_OSCLOCK"d us, diff=%"FORMAT_SPEC_OSCLOCK"d us)",
                        VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId,
                        MPEGFrame2Char(VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame),
                        VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                        VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                        VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                        time_minus(VIDPROD_Data_p->DecoderStatus.MaxBufferWaitingTime, VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.BufferSeachStartTime)*OSCLOCK_T_MILLION/TOut_ClockTicks,
                        time_minus(time_now(), VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.BufferSeachStartTime)*OSCLOCK_T_MILLION/TOut_ClockTicks,
                        time_minus(time_now(), VIDPROD_Data_p->DecoderStatus.MaxBufferWaitingTime)*OSCLOCK_T_MILLION/TOut_ClockTicks));
#if defined(PRODUCER_DUMP_FRAME_BUFFER)
                vidprod_DumpFBContent(ProducerHandle);
#endif /* PRODUCER_DUMP_FRAME_BUFFER */
            }
#endif /* !defined(STVID_PRODUCER_NO_TIMEOUT) && !defined(STVID_DONT_REPORT_LACK_OF_BUFFER) */
        }
        else if (ErrorCode == STVID_ERROR_NOT_AVAILABLE)
        {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
            TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_ERRORS, "pDEC_CBSearchDanglingField-%d(%d-%d/%d) ", VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
            TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODER_PROCESSING, "pDEC_CBAbort-%d(%d-%d/%d) ", VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Producer DecoderConfirmBufferCB : Dangling field decode abort ID=%d(%d-%d/%d) !!", VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
            ErrorCode = DECODER_Abort(VIDPROD_Data_p->DecoderHandle, VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId);
            if (ErrorCode != ST_NO_ERROR)
            {
                /* NOTE_CL: unexpected error, decoder internal data failure or bad producer data handling */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER_Abort rejected ID=%d!!", VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId));
            }
#ifdef STVID_TRICKMODE_BACKWARD
			if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p != NULL)
			{
                VIDPROD_Data_p->Backward.NbDecodedPictures ++;
			}
#endif

            if ((VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.IsReference) && IsSpoiledReferenceLoosingAllReferencesUntilNextRandomAccessPoint(VIDPROD_Data_p->ErrorRecoveryMode))
            {
                if ((!IsIOnlyDecodeOnGoing(VIDPROD_Data_p->Skip)) || (IsFirstOfTwoFieldsOfIFrame(VIDPROD_Data_p->ForTask.PictureToConfirm_p)))
                {
                    /* Search next entry point because a reference picture has been aborted and we are not decoding I only */
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Ask for skip until next RAP"));
                    VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint = TRUE;
                    VIDPROD_Data_p->Skip.RecoveringNextEntryPoint = TRUE;
                    VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = TRUE;
                }
            }
            /* Release preprocessing picture used by this aborted picture */
#ifdef STVID_TRICKMODE_BACKWARD
            if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
            {
                vidprod_RemovePictureFromPreprocessingTable(ProducerHandle, VIDPROD_Data_p->ForTask.PictureToConfirm_p);
            }
#ifdef STVID_TRICKMODE_BACKWARD
            else
            {
                vidprod_RemovePictureFromPreprocessingTableBackward(ProducerHandle, VIDPROD_Data_p->ForTask.PictureToConfirm_p);
            }
#endif /* STVID_TRICKMODE_BACKWARD */
            /* This decode is aborted, decrease VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue */
            DECREMENT_NB_PICTURES_IN_QUEUE(ProducerHandle);
            if (VIDPROD_Data_p->NextAction.ConfirmBufferWhenPossible > 0)
            {
                VIDPROD_Data_p->NextAction.ConfirmBufferWhenPossible--;
            }
            else
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ConfirmBufferWhenPossible was already == 0 !!! "));
            }
            VIDPROD_Data_p->ForTask.PictureToConfirm_p = NULL;
            VIDPROD_Data_p->ForTask.PreprocessingPictureNumber = 0;
        }
        else if (ErrorCode == STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE)
        {
            /* Picture too big for profile: skip all pictures until next sequence. */
            /* Search next entry point */
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Ask for skip until next RAP"));
            VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint = TRUE;
            VIDPROD_Data_p->Skip.RecoveringNextEntryPoint = TRUE;
            VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = TRUE;
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Mem Prof error sets Skip until RAP"));

            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "GetBuffer Error \"Impossible with current profile\" while looking for buffer for Job ID=%d!!", VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId));
#ifdef STVID_DEBUG_GET_STATISTICS
            VIDPROD_Data_p->Statistics.DecodePbSequenceNotInMemProfileSkipped ++;
#endif /* STVID_DEBUG_GET_STATISTICS */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Producer DecoderConfirmBufferCB : Abort because picture out of memory profile ID=%d(%d-%d/%d) !!", VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
            ErrorCode = DECODER_Abort(VIDPROD_Data_p->DecoderHandle, VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId);
            if (ErrorCode != ST_NO_ERROR)
            {
                /* NOTE_CL: unexpected error, decoder internal data failure or bad producer data handling */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER_Abort rejected ID=%d!!", VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId));
            }
#ifdef STVID_TRICKMODE_BACKWARD
			if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p!=NULL)
			{
                VIDPROD_Data_p->Backward.NbDecodedPictures++;
			}
#endif

            /* Release preprocessing picture used by this aborted picture */
#ifdef STVID_TRICKMODE_BACKWARD
            if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
            {
                vidprod_RemovePictureFromPreprocessingTable(ProducerHandle, VIDPROD_Data_p->ForTask.PictureToConfirm_p);
            }
#ifdef STVID_TRICKMODE_BACKWARD
            else
            {
                vidprod_RemovePictureFromPreprocessingTableBackward(ProducerHandle, VIDPROD_Data_p->ForTask.PictureToConfirm_p);
            }
#endif /* STVID_TRICKMODE_BACKWARD */
            /* This decode is aborted, decrease VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue */
            DECREMENT_NB_PICTURES_IN_QUEUE(ProducerHandle);
            if (VIDPROD_Data_p->NextAction.ConfirmBufferWhenPossible > 0)
            {
                VIDPROD_Data_p->NextAction.ConfirmBufferWhenPossible--;
            }
            else
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ConfirmBufferWhenPossible was already == 0 !!! "));
            }
            VIDPROD_Data_p->ForTask.PictureToConfirm_p = NULL;
            VIDPROD_Data_p->ForTask.PreprocessingPictureNumber = 0;
        }
        else if ((ErrorCode != ST_NO_ERROR) || (DecodedPicture_p == NULL))
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "GetBuffer Error (0x%x) while looking for buffer for Job ID=%d!!", ErrorCode, VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId));
        }
        else
        {
            if (!VIDPROD_Data_p->DecoderStatus.IsPicturePushStarted)
            {
                if (IsFrameOrFirstOfTwoFields(VIDPROD_Data_p->ForTask.PictureToConfirm_p))
                {
                    VIDPROD_Data_p->DecoderStatus.FrameBufferUsedSinceLastRAP++;
                }
            }

            if (DecodedPicture_p->FrameBuffer_p == NULL)
            {
                /* TODO_CL: unexpected error, for debugging purpose only */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "GetBuffer Error (NULL FB) while looking for buffer for Job ID=%d!!", VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId));
            }

#ifdef STVID_FILL_FB_BEFORE_USE
            if (IsFrameOrFirstOfTwoFields(VIDPROD_Data_p->ForTask.PictureToConfirm_p))
            {
                STOS_memsetUncached(DecodedPicture_p->FrameBuffer_p->Allocation.Address_p, STVID_FILL_FB_BEFORE_USE, DecodedPicture_p->FrameBuffer_p->Allocation.TotalSize);
            }
#endif /* STVID_FILL_FB_BEFORE_USE */
            /* PictureBuffer found for new picture : DecodedPicture_p and DecodedPicture_p->AssociatedDecimatedPicture_p,
             * We need to update generic picture data from VIDPROD_Data_p->ForTask.PictureToConfirm_p into DecodedPicture_p and DecodedPicture_p->AssociatedDecimatedPicture_p
             * One solution is to copy whole VIDPROD_Data_p->ForTask.PictureToConfirm_p content into both DecodedPicture_p and DecodedPicture_p->AssociatedDecimatedPicture_p but
             * we need to keep unchanged the frame buffers related fields in DecodedPicture_p and DecodedPicture_p->AssociatedDecimatedPicture_p which have been supplied by vid_buff */

            /* Update Buffers fields from just get DecodedPicture_p to avoid to replace it with the unrelevant VIDPROD_Data_p->ForTask.PictureToConfirm_p->Buffers
             * CopyPictureBuffer() doesn't handle this information but deals correctly with DestPicture_p->FrameBuffer_p and DestPicture_p->AssociatedDecimatedPicture_p */
            STOS_memcpy(&(VIDPROD_Data_p->ForTask.PictureToConfirm_p->Buffers), &(DecodedPicture_p->Buffers), sizeof(DecodedPicture_p->Buffers));
            CopyPictureBuffer(DecodedPicture_p, VIDPROD_Data_p->ForTask.PictureToConfirm_p);
            DecodedPicture_p->Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_DECODING;
            /* Update decimated picture buffer if available */
            if (DecodedPicture_p->AssociatedDecimatedPicture_p != NULL)
            {
                /* Update Buffers fields from just get DecodedPicture_p->AssociatedDecimatedPicture_p to avoid to replace it with the unrelevant VIDPROD_Data_p->ForTask.PictureToConfirm_p->Buffers
                 * CopyPictureBuffer() doesn't handle this information but deals correctly with DestPicture_p->FrameBuffer_p and DestPicture_p->AssociatedDecimatedPicture_p */
                STOS_memcpy(&(VIDPROD_Data_p->ForTask.PictureToConfirm_p->Buffers), &(DecodedPicture_p->AssociatedDecimatedPicture_p->Buffers), sizeof(DecodedPicture_p->AssociatedDecimatedPicture_p->Buffers));
                CopyPictureBuffer(DecodedPicture_p->AssociatedDecimatedPicture_p, VIDPROD_Data_p->ForTask.PictureToConfirm_p);
                DecodedPicture_p->AssociatedDecimatedPicture_p->Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_DECODING;
                /* Decimated picture is never considered as reference picture */
                DecodedPicture_p->AssociatedDecimatedPicture_p->IsReferenceForDecoding = FALSE;
            }

            ErrorCode = DECODER_GetCodecInfo(VIDPROD_Data_p->DecoderHandle, &(VIDPROD_Data_p->CodecInfo), DecodedPicture_p->ParsedPictureInformation.PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo.ProfileAndLevelIndication, VIDPROD_Data_p->CodecMode);
            BufferInfos.FrameBufferAdditionalDataBytesPerMB = VIDPROD_Data_p->CodecInfo.FrameBufferAdditionalDataBytesPerMB;
            ErrorCode = VIDBUFF_SetInfos(VIDPROD_Data_p->BufferManagerHandle,  &BufferInfos);
            if (IsRequestingAdditionnalData(VIDPROD_Data_p))
            {
                /* If picture is frame or first of 2 fields allocate a new PPB */
                if (!IsSecondOfTwoFields(DecodedPicture_p))
                {
                    ErrorCode = VIDBUFF_GetAdditionnalDataBuffer(VIDPROD_Data_p->BufferManagerHandle, DecodedPicture_p);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDBUFF_GetAdditionnalDataBuffer Error (0x%x) while looking for PPB for Job ID=%d!!", ErrorCode, DecodedPicture_p->Decode.CommandId));
                        /* Unlock the current picture buffer */
                        if (DecodedPicture_p->AssociatedDecimatedPicture_p != NULL)
                        {
                            RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, DecodedPicture_p->AssociatedDecimatedPicture_p, VIDCOM_VIDPROD_MODULE_BASE);
                        }
                        RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, DecodedPicture_p, VIDCOM_VIDPROD_MODULE_BASE);

                        /* Post a producer reset command and leave */
                        vidprod_ControllerReset(ProducerHandle);
                        return;
                    }
                    if (IsFramePicture(DecodedPicture_p))
                    {
                        DecodedPicture_p->PPB.FieldCounter = 2;
                    }
                    else
                    {
                        DecodedPicture_p->PPB.FieldCounter = 1;
                    }
                }
                else
                {

                    /* second of two fields, reuse paired 1st field existing PPB and update counters */
                    DecodedPicture_p->FrameBuffer_p->FrameOrFirstFieldPicture_p->PPB.FieldCounter += 1;
                    DecodedPicture_p->PPB = DecodedPicture_p->FrameBuffer_p->FrameOrFirstFieldPicture_p->PPB;
#if defined(TRACE_UART) && !defined(VALID_TOOLS)
/*                    TraceBuffer(("\r\nReusePPB of 0x%x for 0x%x",*/
/*                            DecodedPicture_p->FrameBuffer_p->FrameOrFirstFieldPicture_p,*/
/*                            DecodedPicture_p*/
/*                            ));*/
#endif
                }
            }

#ifdef  STVID_DEBUG_PRODUCER /* for debug purpose only */
            /* CL: following code is track unexpected bug, second filed marked as valid but with invalid picture structure informations */
            if ( (IsFramePicture(DecodedPicture_p)) && (DecodedPicture_p->FrameBuffer_p->NothingOrSecondFieldPicture_p != NULL))
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "GetBuffer Error Frame picture with SecondFieldPicture_p not NULL while looking for buffer for Job ID=%d (%d-%d/%d)!!", VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId,
                        VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
                if (DecodedPicture_p->FrameBuffer_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation.PictureSpecificData_p == NULL)
                {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_ERRORS, "pDEC_CB_BufferError-%d(%d-%d/%d) ", VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "GetBuffer Error Frame picture with SecondFieldPicture_p not NULL & SpecificDataPointeur is NULL while looking for buffer for Job ID=%d (%d-%d/%d)!!", VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId,
                            VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
                }
            }
            else if ((IsSecondOfTwoFields(DecodedPicture_p)) && (DecodedPicture_p->FrameBuffer_p->NothingOrSecondFieldPicture_p != NULL))
            {
                if (DecodedPicture_p->FrameBuffer_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation.PictureSpecificData_p == NULL)
                {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_ERRORS, "pDEC_CB_BufferError-%d(%d-%d/%d) ", VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "GetBuffer Error (NULL Pict specific Data pointer) while looking for buffer for Job ID=%d (%d-%d/%d)!!", VIDPROD_Data_p->ForTask.PictureToConfirm_p->Decode.CommandId,
                            VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, VIDPROD_Data_p->ForTask.PictureToConfirm_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
                }
            }
#endif /* if STVID_DEBUG_PRODUCER */

            AddPictureToDecodePictureTable(ProducerHandle, DecodedPicture_p);
            ConfirmBufferParams.NewPictureBuffer_p = DecodedPicture_p;

            /* Lock Access to Preprocessing Pictures */
#ifdef STVID_TRICKMODE_BACKWARD
            if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
            {
                STOS_SemaphoreWait(VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureSemaphore_p);
            }
#ifdef STVID_TRICKMODE_BACKWARD
            else
            {
                STOS_SemaphoreWait(VIDPROD_Data_p->LinearBitBuffers.PreprocessingPictureSemaphore_p);
            }
#endif /* STVID_TRICKMODE_BACKWARD */
            /* Lock Access to Decode Pictures */
            STOS_SemaphoreWait(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);
            /* Searchs for picture in CurrentPictures_p Table in order to update pointer */
            if (VIDPROD_Data_p->DecoderStatus.NbIndices > 0)
            {
                PPIndex = VIDPROD_Data_p->DecoderStatus.NextOutputIndex;
                CurrWritePtr = (VIDPROD_Data_p->DecoderStatus.NextOutputIndex + VIDPROD_Data_p->DecoderStatus.NbIndices) % MAX_CURRENT_PICTURES;
#ifdef STVID_TRICKMODE_BACKWARD
                if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
                {
                while (PPIndex != CurrWritePtr)
                {
                    if (&VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[VIDPROD_Data_p->ForTask.PreprocessingPictureNumber] == VIDPROD_Data_p->DecoderStatus.CurrentPictures_p[PPIndex])
                    {
                        /* picture found, update CurrentPictures_p table content */
                        VIDPROD_Data_p->DecoderStatus.CurrentPictures_p[PPIndex] = DecodedPicture_p;
                        break;
                    }
                    PPIndex = (PPIndex + 1) % MAX_CURRENT_PICTURES;
                }
            }
#ifdef STVID_TRICKMODE_BACKWARD
                else if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p != NULL)
                {
                    while (PPIndex != CurrWritePtr)
                    {
                        if (&VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[VIDPROD_Data_p->ForTask.PreprocessingPictureNumber] == VIDPROD_Data_p->DecoderStatus.CurrentPictures_p[PPIndex])
                        {
                            /* picture found, update CurrentPictures_p table content */
                            VIDPROD_Data_p->DecoderStatus.CurrentPictures_p[PPIndex] = DecodedPicture_p;
                            break;
                        }
                        PPIndex = (PPIndex + 1) % MAX_CURRENT_PICTURES;
                    }
                }
#endif /* STVID_TRICKMODE_BACKWARD */
            }
            else
            {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_ERRORS, "pDEC_CBErr5-%d(%d-%d/%d) ",
                    VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[VIDPROD_Data_p->ForTask.PreprocessingPictureNumber].Decode.CommandId,
                    VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[VIDPROD_Data_p->ForTask.PreprocessingPictureNumber].ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                    VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[VIDPROD_Data_p->ForTask.PreprocessingPictureNumber].ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                    VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[VIDPROD_Data_p->ForTask.PreprocessingPictureNumber].ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Producer : Picture not found in CurrentPictures_p at ConfirmBuffer ID=%d(%d-%d/%d) !! (BUG-BUG-BUG) !!",
                    VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[VIDPROD_Data_p->ForTask.PreprocessingPictureNumber].Decode.CommandId,
                    VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[VIDPROD_Data_p->ForTask.PreprocessingPictureNumber].ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                    VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[VIDPROD_Data_p->ForTask.PreprocessingPictureNumber].ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                    VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[VIDPROD_Data_p->ForTask.PreprocessingPictureNumber].ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
            }
#ifdef STVID_TRICKMODE_BACKWARD
            if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
            {
            /* Then free PictureBuffer_p from PreprocessingPicture Pool */
            VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[VIDPROD_Data_p->ForTask.PreprocessingPictureNumber].Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_EMPTY;
            VIDPROD_Data_p->PreprocessingPicture.IsPictureBufferUsed[VIDPROD_Data_p->ForTask.PreprocessingPictureNumber] = FALSE;
            VIDPROD_Data_p->PreprocessingPicture.IsPictureTimedOut[VIDPROD_Data_p->ForTask.PreprocessingPictureNumber] = FALSE;
            VIDPROD_Data_p->PreprocessingPicture.NbUsedPictureBuffers--;
            /* From this point VIDPROD_Data_p->ForTask.PictureToConfirm_p is no longer valid as the PreprocessingPictureBufferTable entry associated to the confirmed picture
             * will be now available for a new parsed picture */
            VIDPROD_Data_p->ForTask.PictureToConfirm_p = NULL;
            VIDPROD_Data_p->ForTask.PreprocessingPictureNumber = 0;
            /* UnLock Access to Decode Pictures */
            STOS_SemaphoreSignal(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);
            /* UnLock Access to Preprocessing Pictures */
            STOS_SemaphoreSignal(VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureSemaphore_p);
            }
#ifdef STVID_TRICKMODE_BACKWARD
            else
            {
                /* Then free PictureBuffer_p from PreprocessingPicture Pool */
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[VIDPROD_Data_p->ForTask.PreprocessingPictureNumber].Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_EMPTY;
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.IsPictureBufferUsed[VIDPROD_Data_p->ForTask.PreprocessingPictureNumber] = FALSE;
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.IsPictureTimedOut[VIDPROD_Data_p->ForTask.PreprocessingPictureNumber] = FALSE;
                VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.NbUsedPictureBuffers--;
                /* From this point VIDPROD_Data_p->ForTask.PictureToConfirm_p is no longer valid as the PreprocessingPictureBufferTable entry associated to the confirmed picture
                * will be now available for a new parsed picture */
                VIDPROD_Data_p->ForTask.PictureToConfirm_p = NULL;
                VIDPROD_Data_p->ForTask.PreprocessingPictureNumber = 0;
                /* UnLock Access to Decode Pictures */
                STOS_SemaphoreSignal(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);
                /* UnLock Access to Preprocessing Pictures */
                STOS_SemaphoreSignal(VIDPROD_Data_p->LinearBitBuffers.PreprocessingPictureSemaphore_p);
            }
#endif /* STVID_TRICKMODE_BACKWARD */
            for (ReferenceCounter = 0; ReferenceCounter < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; ReferenceCounter++)
            {
                if (ConfirmBufferParams.NewPictureBuffer_p->ParsedPictureInformation.PictureGenericData.IsValidIndexInReferenceFrame[ReferenceCounter])
                {
                    if (ConfirmBufferParams.NewPictureBuffer_p->ReferencesFrameBufferPointer[ReferenceCounter] == NULL)
                    {
                        if (VIDPROD_Data_p->ReplacementReference_IsAvailable)  /* If it is available, use ReplacementReference_p to replace the missing ref, and patch it to get the correct structure (field/frame) */
                        {
                            ConfirmBufferParams.NewPictureBuffer_p->ReferencesFrameBufferPointer[ReferenceCounter]= VIDPROD_Data_p->ReplacementReference_p->FrameBuffer_p;
                        }
                        else    /* else, just make sure to avoid a NULL dereference in decoder */
                        {
                            ConfirmBufferParams.NewPictureBuffer_p->ReferencesFrameBufferPointer[ReferenceCounter] = ConfirmBufferParams.NewPictureBuffer_p->FrameBuffer_p;
                        }
                    }
                    else
                    if (ConfirmBufferParams.NewPictureBuffer_p->ReferencesFrameBufferPointer[ReferenceCounter]->FrameOrFirstFieldPicture_p == NULL)
                    {
                        /* unexpected error, use destination FrameBuffer as reference as the context looks corrupted */
                        if (VIDPROD_Data_p->Stop.IsPending)
                        {
                            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Module Producer : One ref picture (FB @ 0x%x) for ConfirmBuffer ID=%d (%d-%d/%d) has been de-allocated while stop is pending",
                                ConfirmBufferParams.NewPictureBuffer_p->ReferencesFrameBufferPointer[ReferenceCounter]->Allocation.Address_p,
                                ConfirmBufferParams.NewPictureBuffer_p->Decode.CommandId,
                                ConfirmBufferParams.NewPictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                                ConfirmBufferParams.NewPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                ConfirmBufferParams.NewPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
                        }
                        else
                        {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_ERRORS, "pDEC_CBErr8-%d(%d-%d/%d) ",
                            ConfirmBufferParams.NewPictureBuffer_p->Decode.CommandId,
                            ConfirmBufferParams.NewPictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                            ConfirmBufferParams.NewPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                            ConfirmBufferParams.NewPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Producer : No Picture attached to the reference frame buffer 0x%x at ConfirmBuffer ID=%d(%d-%d/%d) !! (BUG-BUG-BUG) !!",
                            ConfirmBufferParams.NewPictureBuffer_p->ReferencesFrameBufferPointer[ReferenceCounter],
                            ConfirmBufferParams.NewPictureBuffer_p->Decode.CommandId,
                            ConfirmBufferParams.NewPictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                            ConfirmBufferParams.NewPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                            ConfirmBufferParams.NewPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
                        }

                        if (VIDPROD_Data_p->ReplacementReference_IsAvailable)  /* If it is available, use ReplacementReference_p to replace the missing ref, and patch it to get the correct structure (field/frame) */
                        {
                            ConfirmBufferParams.NewPictureBuffer_p->ReferencesFrameBufferPointer[ReferenceCounter]= VIDPROD_Data_p->ReplacementReference_p->FrameBuffer_p;
                        }
                        else    /* else, just make sure to avoid a NULL dereference in decoder */
                        {
                            ConfirmBufferParams.NewPictureBuffer_p->ReferencesFrameBufferPointer[ReferenceCounter] = ConfirmBufferParams.NewPictureBuffer_p->FrameBuffer_p;
                        }
                    }
                }
            }

            VIDPROD_Data_p->DecoderStatus.MaxErrorRecoveryBufferWaitingTime = 0;
            ConfirmBufferParams.CommandId = ConfirmBufferParams.NewPictureBuffer_p->Decode.CommandId;
            ConfirmBufferParams.ConfirmBuffer = FALSE; /* Always FALSE in simplified controller because no buffer allocated at DECODER_DecodePicture time */
            if (VIDPROD_Data_p->NextAction.ConfirmBufferWhenPossible > 0)
            {
                VIDPROD_Data_p->NextAction.ConfirmBufferWhenPossible--;
            }
            else
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ConfirmBufferWhenPossible was already == 0 !!! "));
            }
#if defined(TRACE_UART) && defined(VALID_TOOLS)
            TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODER_PROCESSING, "pDEC_CBNew%d(%d-%d/%d@%08x/%08x/%08x) ",
                ConfirmBufferParams.CommandId,
                ConfirmBufferParams.NewPictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, ConfirmBufferParams.NewPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, ConfirmBufferParams.NewPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                ConfirmBufferParams.NewPictureBuffer_p->FrameBuffer_p->Allocation.Address_p,
                ConfirmBufferParams.NewPictureBuffer_p->FrameBuffer_p->Allocation.Address2_p,
                ConfirmBufferParams.NewPictureBuffer_p->PPB.Address_p));
#endif

#ifdef ST_speed
		NewPictureDecodeOrSkipEventData.PictureInfos_p = &(DecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos);
        NewPictureDecodeOrSkipEventData.NbFieldsForDisplay = DecodedPicture_p->ParsedPictureInformation.PictureGenericData.NbFieldsDisplay;
        NewPictureDecodeOrSkipEventData.TemporalReference = DecodedPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID; /* TODO_CL, should use PresentationOrderPictureID instead of TemporalReference in the type -> major change in vid_dec + vid_trick */
        NewPictureDecodeOrSkipEventData.VbvDelay = 0; /* TODO_CL, where to catch this vbvdelay ? */

        NewPictureDecodeOrSkipEventData.UnderflowOccured = VIDPROD_Data_p->Skip.UnderflowOccured;

        /* DECODER_ConfirmBuffer() actually starts the decode, so send notification of beginning of decode */
        STEVT_Notify(VIDPROD_Data_p->EventsHandle, VIDPROD_Data_p->RegisteredEventsID[VIDPROD_DECODE_LAUNCHED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(NewPictureDecodeOrSkipEventData));
#else /* ST_speed */
        /* DECODER_ConfirmBuffer() actually starts the decode, so send notification of beginning of decode */
        /* NewPictureDecodeOrSkipEventData param is only needed by the SPEED module */
        STEVT_Notify(VIDPROD_Data_p->EventsHandle, VIDPROD_Data_p->RegisteredEventsID[VIDPROD_DECODE_LAUNCHED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
#endif /*ST_speed*/

#ifdef ST_speed
        VIDPROD_Data_p->Skip.UnderflowOccured = FALSE;

		if ((DecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I) &&
            (VIDPROD_Data_p->Skip.UntilNextIPictureSpeed))
		{
			VIDPROD_Data_p->Skip.UntilNextIPictureSpeed = FALSE;
			VIDPROD_Data_p->Skip.AllBPicturesSpeed = FALSE;
            VIDPROD_Data_p->Skip.AllPPicturesSpeed = FALSE;
		}
#endif /* ST_speed */

            /* update frame buffer allocation data in picture infos structure */
            DecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Data1_p  = DecodedPicture_p->FrameBuffer_p->Allocation.Address_p;
            DecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Size1    = DecodedPicture_p->FrameBuffer_p->Allocation.TotalSize -
                                                                                                            DecodedPicture_p->FrameBuffer_p->Allocation.Size2;
            DecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Data2_p  = DecodedPicture_p->FrameBuffer_p->Allocation.Address2_p;
            DecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Size2    = DecodedPicture_p->FrameBuffer_p->Allocation.Size2;
            DecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.CompressionLevel = DecodedPicture_p->FrameBuffer_p->CompressionLevel;
            DecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimationFactors = DecodedPicture_p->FrameBuffer_p->DecimationFactor;
            /* The Non-Decimated DecodedPicture does exist */
            DecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DoesNonDecimatedExist = TRUE;
            DecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.PictureBufferHandle = (STVID_PictureBufferHandle_t) DecodedPicture_p;
            DecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.DecimatedPictureBufferHandle = (STVID_PictureBufferHandle_t) DecodedPicture_p->AssociatedDecimatedPicture_p;

            if (DecodedPicture_p->AssociatedDecimatedPicture_p != NULL)
            {
                /* Setting DecimatedBitmapParams fields */
                DecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimatedBitmapParams.Data1_p
                    = DecodedPicture_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Address_p;
                DecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimatedBitmapParams.Size1
                    = DecodedPicture_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.TotalSize
                    - DecodedPicture_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Size2;
                DecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimatedBitmapParams.Data2_p
                    = DecodedPicture_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Address2_p;
                DecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimatedBitmapParams.Size2
                    = DecodedPicture_p->AssociatedDecimatedPicture_p->FrameBuffer_p->Allocation.Size2;

                /* copy the whole PictureInfos structure into decimated picture generic data */
                DecodedPicture_p->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos
                                            = DecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos;
                /* but restore  the decimated frame buffer decimation factor */
                DecodedPicture_p->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimationFactors
                                            = DecodedPicture_p->AssociatedDecimatedPicture_p->FrameBuffer_p->DecimationFactor;
#if defined(FORCE_SECONDARY_ON_AUX_DISPLAY)
                /* Force secondary reconstruction with H&V decim factor = 1 to support dual display when main decode is located in LMI_VID
                * (decimated frame buffers shall be allocated in LMI_SYS and sized to 720x576 minimum) */
                if (DecodedPicture_p->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Width <= 720)
                {
                    DecodedPicture_p->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimationFactors&=~(STVID_DECIMATION_FACTOR_H2|STVID_DECIMATION_FACTOR_H4);
                }
                if (DecodedPicture_p->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.BitmapParams.Height <= 576)
                {
                    DecodedPicture_p->AssociatedDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimationFactors&=~(STVID_DECIMATION_FACTOR_V2|STVID_DECIMATION_FACTOR_V4);
                }
#endif /* defined(FORCE_SECONDARY_ON_AUX_DISPLAY) */
            }

#ifdef STVID_DEBUG_PRODUCER
/*            CheckPictureAndRefsValidity(ConfirmBufferParams.NewPictureBuffer_p);*/
#endif /* STVID_DEBUG_PRODUCER */

            ConfirmBufferParams.NewPictureBuffer_p->Decode.ConfirmedTime = time_now();
            ConfirmBufferSuccess = FALSE;
            if (!VIDPROD_Data_p->Stop.IsPending)
            {
                ErrorCode = DECODER_ConfirmBuffer(VIDPROD_Data_p->DecoderHandle, &ConfirmBufferParams);
                if (ErrorCode == ST_NO_ERROR)
                {
                    ConfirmBufferSuccess = TRUE;
                }
                else
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER rejected confirm buffer ID=%d!!", ConfirmBufferParams.NewPictureBuffer_p->Decode.CommandId));
                }
            }
            if (!ConfirmBufferSuccess)
            {
                /* Release picture buffer as the decoder rejected the confirm buffer */
                /* Lock Access to Decode Pictures */
                STOS_SemaphoreWait(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);
                /* Clean this picture in DecoderStatus table */
                vidprod_RemovePictureFromDecodeStatusTable(ProducerHandle, ConfirmBufferParams.NewPictureBuffer_p);
                STOS_SemaphoreSignal(VIDPROD_Data_p->DecodePicture.DecodePictureSemaphore_p);
                RemovePictureFromDecodePictureTable(ProducerHandle, ConfirmBufferParams.NewPictureBuffer_p, __LINE__);
                if (ConfirmBufferParams.NewPictureBuffer_p->AssociatedDecimatedPicture_p != NULL)
                {
                    RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, ConfirmBufferParams.NewPictureBuffer_p->AssociatedDecimatedPicture_p, VIDCOM_VIDPROD_MODULE_BASE);
                }
                RELEASE_PICTURE_BUFFER(VIDPROD_Data_p->BufferManagerHandle, ConfirmBufferParams.NewPictureBuffer_p, VIDCOM_VIDPROD_MODULE_BASE);
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DECODER confirm buffer ID=%d cancelled, stop is on going", ConfirmBufferParams.NewPictureBuffer_p->Decode.CommandId));
            }
        }
    }
} /* end of vidprod_SearchAndConfirmBuffer */

/*******************************************************************************
Name        : vidprod_LaunchDecode
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void vidprod_LaunchDecode(const VIDPROD_Handle_t ProducerHandle)
{
    VIDPROD_Data_t                  * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    ST_ErrorCode_t                  ErrorCode;
    U32                             LoopCounter;
    VIDBUFF_PictureBuffer_t         * NextPictureInDecodeOrder_p;
    DECODER_DecodePictureParams_t   DECODER_DecodePictureParams;
    DECODER_Status_t                DECODER_Status;
    DECODER_DecodingJobResults_t    DecodingJobResult;
    U32                             SkippedPictureNbFields;

    /*  NOTE_CL: Don't call PARSER_InformReadAddress() here to avoid inappropriate bit buffer level modifications */
    /*PARSER_InformReadAddressParams_t InformReadAddressParams;*/
    BOOL CurrentPictureMustBeSkipped = FALSE;

    /* look for oldest parsed picture waiting for decode launch */
    LoopCounter = 0;
    VIDPROD_Data_p->ForTask.PictureToDecode_p = NULL;

#ifdef STVID_TRICKMODE_BACKWARD
    if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p != NULL)
    {
		/* If not all the pictures in PreprocessingPictureBufferTable have been treated, then search for the temporally previous I, starting from the index of
		 * previous I really sent to decode minus 1 */
        if (IsIOnlyDecodeOnGoing(VIDPROD_Data_p->Skip))
		{
        	if ( VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.NbParsedPicturesLeft != 0)
        	{
				LoopCounter = VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.NbParsedPicturesLeft;
				if (((&VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter-1]) != NULL) &&
                	(VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter -1].ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I))
            	{
					VIDPROD_Data_p->ForTask.PictureToDecode_p =
							&(VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter -1]);
				}
				VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.NbParsedPicturesLeft --;
			}
		}
    }
    else
#endif /* STVID_TRICKMODE_BACKWARD */
	{
	LoopCounter = 0;
    while ((LoopCounter < VIDPROD_Data_p->DecoderStatus.NbIndices) && (VIDPROD_Data_p->ForTask.PictureToDecode_p == NULL))
    {
        NextPictureInDecodeOrder_p = VIDPROD_Data_p->DecoderStatus.CurrentPictures_p[(VIDPROD_Data_p->DecoderStatus.NextOutputIndex+LoopCounter) % MAX_CURRENT_PICTURES];
		if (NextPictureInDecodeOrder_p != NULL)
		{
        if (NextPictureInDecodeOrder_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_PARSED)
        {
            VIDPROD_Data_p->ForTask.PictureToDecode_p = NextPictureInDecodeOrder_p;
        }
		}
        LoopCounter++;
    }
}
    if (VIDPROD_Data_p->ForTask.PictureToDecode_p != NULL) /* Found the next picture to decode */
    {
        if (VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.ParsingError != VIDCOM_ERROR_LEVEL_NONE)
        {
            /* Send DATA ERROR event */
            vidprod_NotifyDecodeEvent(ProducerHandle, STVID_DATA_ERROR_EVT, (const void *) NULL);
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Parser returned errors in picture %c (%d-%d/%d) %d!!",
                    MPEGFrame2Char(VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame),
                    VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                    VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                    VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                    VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.ParsingError));
        }

#ifdef ST_speed
        if ( (((VIDPROD_Data_p->Skip.NextBPicture) || (VIDPROD_Data_p->Skip.AllBPictures) || (VIDPROD_Data_p->Skip.AllBPicturesSpeed)) && (VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B)) ||
             (((VIDPROD_Data_p->Skip.AllPPictures) || (VIDPROD_Data_p->Skip.AllPPicturesSpeed)) && (VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_P)) ||
             ((VIDPROD_Data_p->Skip.NextIPicture) && (VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)) ||
             (((!VIDPROD_Data_p->SearchFirstI) && (VIDPROD_Data_p->Skip.AllExceptFirstI)) && (VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)) ||
             ((VIDPROD_Data_p->Skip.AllIPictures) && (VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)) )
#else /* ST_speed */
        if ( (((VIDPROD_Data_p->Skip.NextBPicture) || (VIDPROD_Data_p->Skip.AllBPictures)) && (VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B)) ||
             (((VIDPROD_Data_p->Skip.AllPPictures)) && (VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_P)) ||
             ((VIDPROD_Data_p->Skip.AllIPictures) && (VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)) )
#endif /* ST_speed */
        {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
            TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODER_START, "Skip(%d-%d/%d)", VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
            CurrentPictureMustBeSkipped = TRUE;
#ifdef STVID_DEBUG_GET_STATISTICS
            VIDPROD_Data_p->Statistics.DecodePictureSkippedRequested++;
#endif /* STVID_DEBUG_GET_STATISTICS */

            STEVT_Notify(VIDPROD_Data_p->EventsHandle, VIDPROD_Data_p->RegisteredEventsID[VIDPROD_SKIP_LAUNCHED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
            if (VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B)
            {
                VIDPROD_Data_p->Skip.NextBPicture = FALSE;
            }
#ifdef ST_speed
            if (VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)
            {
            VIDPROD_Data_p->Skip.NextIPicture = FALSE;
            }
#endif

        }

        if (!CurrentPictureMustBeSkipped &&
            VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.ParsingError >= VIDCOM_ERROR_LEVEL_CRASH )
        {
            if (VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.ParsingError == VIDCOM_ERROR_LEVEL_RESET)
            {
                CurrentPictureMustBeSkipped = TRUE;
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "parser reported RESET level error, producer RESET"));
                vidprod_ControllerReset(ProducerHandle);
            }
        }

    }

    if (CurrentPictureMustBeSkipped)
    {
        SkippedPictureNbFields = VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.NbFieldsDisplay;
        STEVT_Notify(VIDPROD_Data_p->EventsHandle, VIDPROD_Data_p->RegisteredEventsID[VIDPROD_PICTURE_SKIPPED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST (&SkippedPictureNbFields));

        /* We have to skip this picture so check if it's the picture to stop on. */
        if ((VIDPROD_Data_p->Stop.AfterCurrentPicture) &&
            (VIDPROD_Data_p->LastParsedPictureDecodingOrderFrameID ==
                VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID) &&
            (vidcom_ComparePictureID(&(VIDPROD_Data_p->LastParsedPictureExtendedPresentationOrderPictureID),
                &(VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID)) == 0))
        {
            /* Order stop now */
            PushControllerCommand(ProducerHandle, CONTROLLER_COMMAND_STOP);
            VIDPROD_Data_p->Skip.UntilNextRandomAccessPoint = TRUE;
            VIDPROD_Data_p->Skip.RecoveringNextEntryPoint = TRUE;
            VIDPROD_Data_p->ForTask.NeedForUpdateReferenceList = TRUE;
        }
#ifdef STVID_TRICKMODE_BACKWARD
        if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
        {
            vidprod_RemovePictureFromPreprocessingTable(ProducerHandle, VIDPROD_Data_p->ForTask.PictureToDecode_p);
        }
#ifdef STVID_TRICKMODE_BACKWARD
        else
        {
            if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p != NULL)
            {
                vidprod_RemovePictureFromPreprocessingTableBackward(ProducerHandle, VIDPROD_Data_p->ForTask.PictureToDecode_p);
                if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->NbIPicturesInBuffer != 0)
                {
                    VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->NbIPicturesInBuffer --;
                }
            }
        }
#endif /* STVID_TRICKMODE_BACKWARD */
        if (VIDPROD_Data_p->NextAction.LaunchDecodeWhenPossible > 0)
        {
            VIDPROD_Data_p->NextAction.LaunchDecodeWhenPossible--;
        }
else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "LaunchDecodeWhenPossible was already == 0 !!! "));
        }
#ifdef STVID_TRICKMODE_BACKWARD
        if (VIDPROD_Data_p->Backward.IsBackward)
        {
            VIDPROD_Data_p->Backward.NbDecodedPictures ++;
        }
#endif

        /* TODO_CL :manage requested skip on P and I picture type */
    }

    /* If the picture is actually ready for decode, continue */
    if ((!CurrentPictureMustBeSkipped) && (VIDPROD_Data_p->ForTask.PictureToDecode_p != NULL)) /* Found the next picture to decode */
    {
#ifdef ST_speed
        if (((VIDPROD_Data_p->SearchFirstI) && (VIDPROD_Data_p->Skip.AllExceptFirstI)) && (VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I))
        {
            VIDPROD_Data_p->SearchFirstI = FALSE;
        }
#endif /* ST_speed */
        DECODER_DecodePictureParams.PictureInformation = VIDPROD_Data_p->ForTask.PictureToDecode_p;
        DECODER_DecodePictureParams.ErrorRecoveryMode = DECODER_ERROR_RECOVERY_MODE1;     /*  TODO_CL : Not yet defined, should be defined according to VIDPROD_Data_p->ErrorRecoveryMode value */
#if defined(TRACE_UART) && defined(VALID_TOOLS)
        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODER_START, "pDEC_Start(%d-%d/%d) ", VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
        ErrorCode = DECODER_DecodePicture(VIDPROD_Data_p->DecoderHandle, &DECODER_DecodePictureParams, &DECODER_Status);
        if (ErrorCode == ST_ERROR_BAD_PARAMETER)
		{
            /* TODO_CL: error handling to be defined */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "DECODER_DecodePicture failed Error=%d BAD PARAM !!", ErrorCode));
		}

        if ((ErrorCode == ST_NO_ERROR) && DECODER_Status.IsJobSubmitted)
        {
#ifdef STVID_DEBUG_GET_STATISTICS
        VIDPROD_Data_p->Statistics.DecodePictureDecodeLaunched ++;
#ifdef ST_speed
        if (VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)
        {
            VIDPROD_Data_p->Statistics.NbDecodedPicturesI ++;
        }
        if (VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_P)
        {
            VIDPROD_Data_p->Statistics.NbDecodedPicturesP ++;
        }
        if (VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B)
        {
            VIDPROD_Data_p->Statistics.NbDecodedPicturesB ++;
        }
#endif /*ST_speed*/
#endif /* STVID_DEBUG_GET_STATISTICS */
#if defined(TRACE_UART) && defined(VALID_TOOLS)
        TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODER_START, "pDEC_PrepStart(%d-%d/%d) ", VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif

            VIDPROD_Data_p->ForTask.PictureToDecode_p->Decode.CommandId = DECODER_Status.CommandId;
            VIDPROD_Data_p->ForTask.PictureToDecode_p->Decode.StartTime = DECODER_Status.DecodingJobStatus.JobSubmissionTime;
            VIDPROD_Data_p->ForTask.PictureToDecode_p->Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_WAITING_CONFIRM_EVT;
            VIDPROD_Data_p->ForTask.PictureToDecode_p = NULL;
            if (VIDPROD_Data_p->NextAction.LaunchDecodeWhenPossible > 0)
            {
                VIDPROD_Data_p->NextAction.LaunchDecodeWhenPossible--;
            }
            else
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "LaunchDecodeWhenPossible was already == 0 !!! "));
            }
            VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue++;
#if defined(TRACE_UART) && defined(VALID_TOOLS)
            TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODER_START, "pID%d ",DECODER_Status.CommandId));
#endif
            /* Decoder may ask for immediate buffer confirmation (decoder won't send buffer confirmation request event for this job) */
            if ( DECODER_Status.DecodingJobStatus.JobState == DECODER_JOB_WAITING_CONFIRM_BUFFER )
            {
                DecodingJobResult.DecodingJobStatus = DECODER_Status.DecodingJobStatus;
                DecodingJobResult.ErrorCode = DECODER_NO_ERROR;
                DecodingJobResult.CommandId = DECODER_Status.CommandId;
                if (VIDPROD_Data_p->ForTask.NbIndexToConfirm >= MAX_NUMBER_OF_PREPROCESSING_PICTURES)
                {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                    TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODING_ERRORS, "pDEC_CBErr7-%d ", DecodingJobResult.CommandId));
#endif
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "!!! New confirmbuffer requested but queue is full(NbIndexToConfirm=%d). New ID=%d !!!", VIDPROD_Data_p->ForTask.NbIndexToConfirm, DecodingJobResult.CommandId));
                }
                else
                {
                    STOS_InterruptLock();
                    VIDPROD_Data_p->ForTask.PictureToConfirmResult[(VIDPROD_Data_p->ForTask.NextIndexToConfirm + VIDPROD_Data_p->ForTask.NbIndexToConfirm) % MAX_NUMBER_OF_PREPROCESSING_PICTURES] = DecodingJobResult;
                    VIDPROD_Data_p->ForTask.NbIndexToConfirm++;
                    STOS_InterruptUnlock();
                    PushControllerCommand(ProducerHandle, CONTROLLER_COMMAND_CONFIRM_PICTURE_BUFFER);
                    STOS_SemaphoreSignal(VIDPROD_Data_p->ProducerOrder_p);
                }
            }
        } /* If decode not submitted, retry later */
        else
        {
#ifdef STVID_TRICKMODE_BACKWARD
            if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p != NULL)
			{
                if (IsIOnlyDecodeOnGoing(VIDPROD_Data_p->Skip))
				{
					VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.NbParsedPicturesLeft ++;
				}
            }
#endif
#if defined(TRACE_UART) && defined(VALID_TOOLS)
            TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DECODER_START, "pRefusedDEC_Start(%d-%d/%d)", VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, VIDPROD_Data_p->ForTask.PictureToDecode_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
#endif
        }
    }
} /* end of vidprod_LaunchDecode */

/*******************************************************************************
Name        : vidprod_ControllerReset
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void vidprod_ControllerReset(const VIDPROD_Handle_t ProducerHandle)
{
    VIDPROD_Data_t      * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

    VIDPROD_Data_p->ForTask.SoftResetPending = TRUE;
    VIDPROD_Data_p->ForTask.SoftResetPosted = FALSE;
} /* end of vidprod_ControllerReset */

/*******************************************************************************
Name        : vidprod_GetNbParsedNotPushedOrReferencePictures
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void vidprod_GetNbParsedNotPushedOrReferencePictures(const VIDPROD_Handle_t ProducerHandle, U32 *NbParsedNotPushedPictures)
{
    VIDPROD_Data_t          * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    U32                     LoopCounter;
    VIDBUFF_PictureBuffer_t *PictureBuffer_p;
    U32                     NbRefPictures;
    U32                     NumberOfReferenceFrames;

    *NbParsedNotPushedPictures = 0;
    NbRefPictures = 0;
    NumberOfReferenceFrames = 16; /* default value */
    for (LoopCounter = 0 ; LoopCounter < MAX_NUMBER_OF_DECODE_PICTURES ; LoopCounter++)
    {
        PictureBuffer_p = VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter];
        if ((PictureBuffer_p != NULL) && (IsFrameOrFirstOfTwoFields(PictureBuffer_p)))
        {
            NumberOfReferenceFrames = PictureBuffer_p->GlobalDecodingContext.GlobalDecodingContextGenericData.NumberOfReferenceFrames;
            if ((PictureBuffer_p->IsReferenceForDecoding))
            {
                    NbRefPictures++;
                }
            else
            {
                if ((PictureBuffer_p->IsNeededForPresentation) && (!PictureBuffer_p->IsPushed))
                {
                        (*NbParsedNotPushedPictures)++;
                }
            }
        }
    }
    if (NbRefPictures > NumberOfReferenceFrames)
    {
        /* We may have decoded more reference picture than requested by the stream if we have more frame buffers than requested
         * In such case it is safer to clip this figure in order to prevent to push pictures too early to the display */
        NbRefPictures = NumberOfReferenceFrames;
    }
    (*NbParsedNotPushedPictures) += NbRefPictures;
#ifdef STVID_TRICKMODE_BACKWARD
    if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
    {
    for (LoopCounter = 0 ; LoopCounter < MAX_NUMBER_OF_PREPROCESSING_PICTURES ; LoopCounter++)
    {
        if (VIDPROD_Data_p->PreprocessingPicture.IsPictureBufferUsed[LoopCounter])
        {
            PictureBuffer_p = &VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter];
            if ((PictureBuffer_p->IsNeededForPresentation) && (IsFrameOrFirstOfTwoFields(PictureBuffer_p)))
            {
                (*NbParsedNotPushedPictures)++;
            }
        }
    }
    }
#ifdef STVID_TRICKMODE_BACKWARD
    else
    {
        if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p != NULL)
        {
            for (LoopCounter = 0 ; LoopCounter < MAX_BACKWARD_PICTURES ; LoopCounter++)
            {
                if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.IsPictureBufferUsed[LoopCounter])
                {
                    PictureBuffer_p = &VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter];
                    if ((PictureBuffer_p->IsNeededForPresentation) && (IsFrameOrFirstOfTwoFields(PictureBuffer_p)))
                    {
                            (*NbParsedNotPushedPictures)++;
                    }
                }
            }
        }
    }
#endif /* STVID_TRICKMODE_BACKWARD */
} /* end of vidprod_GetNbParsedNotPushedOrReferencePictures */

#ifndef DONT_COMPUTE_NB_DECODED_FRAME_BUFFER
/*******************************************************************************
Name        : vidprod_GetNbDecodedFrameBuffers
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void vidprod_GetNbDecodedFrameBuffers(const VIDPROD_Handle_t ProducerHandle, U32 *NbDecodedFrameBuffers)
{
    VIDPROD_Data_t          *VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    U32                     LoopCounter;
    VIDBUFF_FrameBuffer_t   *PreviousFrameBuffer_p;

    *NbDecodedFrameBuffers = 0;
    PreviousFrameBuffer_p = NULL;
    for (LoopCounter = 0 ; LoopCounter < MAX_NUMBER_OF_DECODE_PICTURES ; LoopCounter++)
    {
        if (VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter] != NULL)
        {
            if((VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->IsNeededForPresentation) &&
               (VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_DECODED))
            {
                if(VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->FrameBuffer_p != PreviousFrameBuffer_p)
                {
                    (*NbDecodedFrameBuffers)++;
                }
                PreviousFrameBuffer_p = VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->FrameBuffer_p;
            }
        }
    }
} /* End of vidprod_GetNbDecodedFrameBuffers */
#endif /* DONT_COMPUTE_NB_DECODED_FRAME_BUFFER */

/*******************************************************************************
Name        : vidprod_TryToPushPictureToDisplay
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void vidprod_TryToPushPictureToDisplay(const VIDPROD_Handle_t ProducerHandle)
{
    VIDPROD_Data_t          * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    U32                     NbDecodedFrameBuffers;
    BOOL                    PictureHasBeenPushed;
    U32                     NbParsedNotPushedPictures;
    U32                     RequestedDecodedFrameBuffers;

    if (IsIOnlyDecodeOnGoing(VIDPROD_Data_p->Skip))
    {
        /* Special case for I only decode, allow to push pictures as soon as possible */
        RequestedDecodedFrameBuffers = 1;
    }
    else
    {
    /* Apply bumping process to insert a picture in display queue to free a frame buffer */
    RequestedDecodedFrameBuffers = VIDPROD_Data_p->DecoderStatus.CurrentMaxDecFrameBuffering;
#if !defined (STVID_VALID)
    /* Force picture push start condition taking into account memory profile passed by the application */
    /* Memory profile # of frame buffer includes DPB + 1 for decode + 1 for display */
    if (!IsDynamicAllocationProfile(VIDPROD_Data_p->ForTask.CurrentProfile))
    {
        if (VIDPROD_Data_p->ForTask.CurrentProfile.ApiProfile.NbFrameStore == STVID_OPTIMISED_NB_FRAME_STORE_ID)
        {
            if ((S32)RequestedDecodedFrameBuffers > (VIDPROD_Data_p->ForTask.CurrentProfile.ApiProfile.FrameStoreIDParams.OptimisedNumber.Main - 2))
            {
                RequestedDecodedFrameBuffers = VIDPROD_Data_p->ForTask.CurrentProfile.ApiProfile.FrameStoreIDParams.OptimisedNumber.Main - 2;
            }
        }
        else
        {
            if ((S32)RequestedDecodedFrameBuffers > (VIDPROD_Data_p->ForTask.CurrentProfile.ApiProfile.NbFrameStore - 2))
            {
                RequestedDecodedFrameBuffers = VIDPROD_Data_p->ForTask.CurrentProfile.ApiProfile.NbFrameStore - 2;
    }
    }
    }
    else
    {
		/* Dynamic profiles */
        if ((!VIDPROD_Data_p->ForTask.NeedForExtraAllocation) || (VIDPROD_Data_p->ForTask.MaxSizeReachedForExtraAllocation))
        {
            /* All needed frame buffers have been allocated */
            if ((S32)RequestedDecodedFrameBuffers > (VIDPROD_Data_p->ForTask.DynamicAllocatedMainFrameBuffers - 2))
            {
                RequestedDecodedFrameBuffers = VIDPROD_Data_p->ForTask.DynamicAllocatedMainFrameBuffers - 2;
        }
            if (VIDPROD_Data_p->ForTask.MaxSizeReachedForExtraAllocation)
            {
/*                STTBX_Report((STTBX_REPORT_LEVEL_WARNING, "Can't allocate MaxDecFrameBuffering frame buffers as requested by the stream, this may lead to bad picture ordering !"));*/
            }
        }
    }
#endif /* !defined (STVID_VALID) */
    } /* if (IsIOnlyDecodeOnGoing(VIDPROD_Data_p->Skip)) */

    if (!VIDPROD_Data_p->DecoderStatus.IsPicturePushStarted)
    {
#ifndef DONT_COMPUTE_NB_DECODED_FRAME_BUFFER
        vidprod_GetNbDecodedFrameBuffers(ProducerHandle, &NbDecodedFrameBuffers);
#else
        NbDecodedFrameBuffers = VIDPROD_Data_p->DecoderStatus.FrameBufferUsedSinceLastRAP;
#endif /* DONT_COMPUTE_NB_DECODED_FRAME_BUFFER */
        if (NbDecodedFrameBuffers > RequestedDecodedFrameBuffers)
        {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
                TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DISPLAY_INSERT, "StartPush DecFB=%d MaxDFB=%d ", NbDecodedFrameBuffers, RequestedDecodedFrameBuffers));
#endif
            VIDPROD_Data_p->DecoderStatus.IsPicturePushStarted = TRUE;
            VIDPROD_Data_p->DecoderStatus.FrameBufferUsedSinceLastRAP = 0;
        }
    }

    do
    {
        vidprod_GetNbParsedNotPushedOrReferencePictures(ProducerHandle, &NbParsedNotPushedPictures);

        if(VIDPROD_Data_p->DecoderStatus.IsPicturePushStarted &&
           (NbParsedNotPushedPictures > RequestedDecodedFrameBuffers))
        {
#if defined(TRACE_UART) && defined(VALID_TOOLS)
            TraceBufferColorConditional((VIDPROD_TRACE_BLOCKID, VIDPROD_TRACE_DISPLAY_INSERT, "pCallBP-C "));
#endif
            PictureHasBeenPushed = BumpPictureToDisplayQueue(ProducerHandle);
            if (PictureHasBeenPushed)
            {
                vidprod_ReleasePushedNonRefPictures(ProducerHandle);
                /* If a picture has been posted to display queue, reset time out */
                VIDPROD_Data_p->DecoderStatus.MaxBufferWaitingTime = time_plus(time_now(), MAX_TIME_BEFORE_STOPPING_DECODER);
            }
        }
        else
        {
            PictureHasBeenPushed = FALSE;
        }
    } while(PictureHasBeenPushed);
} /* End of vidprod_TryToPushPictureToDisplay */

#if defined(PRODUCER_DUMP_FRAME_BUFFER)
/*******************************************************************************
Name        : vidprod_DumpFBContent
Description : debug function to display FB content
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void vidprod_DumpFBContent(const VIDPROD_Handle_t ProducerHandle)
{
    #define PRINT_DISP_STAT(ds) (   ((ds) == VIDBUFF_NOT_ON_DISPLAY)        ? "NOT" : \
                                    ((ds) == VIDBUFF_ON_DISPLAY)            ? "DIS" : \
                                    ((ds) == VIDBUFF_LAST_FIELD_ON_DISPLAY) ? "LFD" : "???" )

    VIDPROD_Data_t        * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    VIDBUFF_FrameBuffer_t *BufferPool_p, *DecimatedBufferPool_p;
    VIDBUFF_FrameBuffer_t * CurrentFB_p;
    U32 LoopCounter;
    U32 NbPictures;
    char    TmpString[512];
    VIDBUFF_PictureBuffer_t *TmpPictureBuffer_p;

    BufferPool_p = NULL;
    DecimatedBufferPool_p = NULL;
    TmpString[0] = 0;
    NbPictures = 0;
    for(LoopCounter=0 ; LoopCounter < MAX_NUMBER_OF_DECODE_PICTURES ; LoopCounter++)
    {
        if (VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter] != NULL)
        {
            NbPictures++;
            TmpPictureBuffer_p = VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter];
            BufferPool_p = TmpPictureBuffer_p->FrameBuffer_p;
            if (TmpPictureBuffer_p->AssociatedDecimatedPicture_p != NULL)
            {
                DecimatedBufferPool_p = TmpPictureBuffer_p->AssociatedDecimatedPicture_p->FrameBuffer_p;
            }
            sprintf(TmpString, "%s(%d-%d/%d)", TmpString, TmpPictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID, TmpPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension, TmpPictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID);
        }
    }
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Current DPB(%d)=%s ", NbPictures, TmpString));

    if (BufferPool_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "vidprod_DumpFBContent function. Failed to find a picture in DecodePictureBuffer table !!!"));
    }
    else
    {
        CurrentFB_p = BufferPool_p;
        do
        {
            if (CurrentFB_p == NULL)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "    CurrentFB_p == NULL"));
            }
            else
            if (CurrentFB_p->FrameOrFirstFieldPicture_p == NULL)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "    [FB 0x%x @ 0x%x empty]", (U32)CurrentFB_p, CurrentFB_p->Allocation.Address_p));
            }
            else
            {
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "FB 0x%x @ 0x%x (%d) 0x%x %s (%d-%d/%d) %s lckcnt=%d TobeKilled=%d disp=%s", (U32)CurrentFB_p,
                    CurrentFB_p->Allocation.Address_p,
                    CurrentFB_p->Allocation.TotalSize,
                    CurrentFB_p->FrameOrFirstFieldPicture_p,
                    (CurrentFB_p->FrameOrFirstFieldPicture_p->IsReferenceForDecoding)?"REF":"NONREF",
                    CurrentFB_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                    CurrentFB_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                    CurrentFB_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                    (CurrentFB_p->FrameOrFirstFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_EMPTY)               ? "EMPTY" :
                    (CurrentFB_p->FrameOrFirstFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_PARSED)              ? "PARSED" :
                    (CurrentFB_p->FrameOrFirstFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_WAITING_CONFIRM_EVT) ? "WAIT_CBEVT" :
                    (CurrentFB_p->FrameOrFirstFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_WAITING_FRAME_BUFFER)? "WAIT_FB" :
                    (CurrentFB_p->FrameOrFirstFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_DECODING)            ? "DECODING" :
                    (CurrentFB_p->FrameOrFirstFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_DECODED)             ? "DECODED" : "INVALID",
                    CurrentFB_p->FrameOrFirstFieldPicture_p->Buffers.PictureLockCounter,
                    CurrentFB_p->ToBeKilledAsSoonAsPossible,
                    PRINT_DISP_STAT(CurrentFB_p->FrameOrFirstFieldPicture_p->Buffers.DisplayStatus)
                    ));
                if (CurrentFB_p->NothingOrSecondFieldPicture_p != NULL)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "     2ndfield 0x%x %s (%d-%d/%d) %s lckcnt=%d  disp=%s",
                    CurrentFB_p->NothingOrSecondFieldPicture_p,
                    (CurrentFB_p->NothingOrSecondFieldPicture_p->IsReferenceForDecoding)?"REF":"NONREF",
                    CurrentFB_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                    CurrentFB_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                    CurrentFB_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                    (CurrentFB_p->NothingOrSecondFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_EMPTY)               ? "EMPTY" :
                    (CurrentFB_p->NothingOrSecondFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_PARSED)              ? "PARSED" :
                    (CurrentFB_p->NothingOrSecondFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_WAITING_CONFIRM_EVT) ? "WAIT_CBEVT" :
                    (CurrentFB_p->NothingOrSecondFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_WAITING_FRAME_BUFFER)? "WAIT_FB" :
                    (CurrentFB_p->NothingOrSecondFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_DECODING)            ? "DECODING" :
                    (CurrentFB_p->NothingOrSecondFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_DECODED)             ? "DECODED" : "INVALID",
                    CurrentFB_p->NothingOrSecondFieldPicture_p->Buffers.PictureLockCounter,
                    PRINT_DISP_STAT(CurrentFB_p->NothingOrSecondFieldPicture_p->Buffers.DisplayStatus)
                        ));
                }
            }
            CurrentFB_p = CurrentFB_p->NextAllocated_p;
        } while ((CurrentFB_p != BufferPool_p) && (CurrentFB_p != NULL));
    }

    if (DecimatedBufferPool_p != NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Decimated buffers"));
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "-----------------"));
        CurrentFB_p = DecimatedBufferPool_p;
        do
        {
            if (CurrentFB_p == NULL)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "    CurrentFB_p == NULL"));
            }
            else
            if (CurrentFB_p->FrameOrFirstFieldPicture_p == NULL)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "    [FB 0x%x empty]", (U32)CurrentFB_p));
            }
            else
            {
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "FB 0x%x @ 0x%x (%d) 0x%x %s (%d-%d/%d) %s lckcnt=%d TobeKilled=%d disp=%s", (U32)CurrentFB_p,
                    CurrentFB_p->Allocation.Address_p,
                    CurrentFB_p->Allocation.TotalSize,
                    CurrentFB_p->FrameOrFirstFieldPicture_p,
                    (CurrentFB_p->FrameOrFirstFieldPicture_p->IsReferenceForDecoding)?"REF":"NONREF",
                    CurrentFB_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                    CurrentFB_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                    CurrentFB_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                    (CurrentFB_p->FrameOrFirstFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_EMPTY)               ? "EMPTY" :
                    (CurrentFB_p->FrameOrFirstFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_PARSED)              ? "PARSED" :
                    (CurrentFB_p->FrameOrFirstFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_WAITING_CONFIRM_EVT) ? "WAIT_CBEVT" :
                    (CurrentFB_p->FrameOrFirstFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_WAITING_FRAME_BUFFER)? "WAIT_FB" :
                    (CurrentFB_p->FrameOrFirstFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_DECODING)            ? "DECODING" :
                    (CurrentFB_p->FrameOrFirstFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_DECODED)             ? "DECODED" : "INVALID",
                    CurrentFB_p->FrameOrFirstFieldPicture_p->Buffers.PictureLockCounter,
                    CurrentFB_p->ToBeKilledAsSoonAsPossible,
                    PRINT_DISP_STAT(CurrentFB_p->FrameOrFirstFieldPicture_p->Buffers.DisplayStatus)
                    ));
                if (CurrentFB_p->NothingOrSecondFieldPicture_p != NULL)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "     2ndfield 0x%x %s (%d-%d/%d) %s lckcnt=%d disp=%s",
                    CurrentFB_p->NothingOrSecondFieldPicture_p,
                    (CurrentFB_p->NothingOrSecondFieldPicture_p->IsReferenceForDecoding)?"REF":"NONREF",
                    CurrentFB_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                    CurrentFB_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                    CurrentFB_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                    (CurrentFB_p->NothingOrSecondFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_EMPTY)               ? "EMPTY" :
                    (CurrentFB_p->NothingOrSecondFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_PARSED)              ? "PARSED" :
                    (CurrentFB_p->NothingOrSecondFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_WAITING_CONFIRM_EVT) ? "WAIT_CBEVT" :
                    (CurrentFB_p->NothingOrSecondFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_WAITING_FRAME_BUFFER)? "WAIT_FB" :
                    (CurrentFB_p->NothingOrSecondFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_DECODING)            ? "DECODING" :
                    (CurrentFB_p->NothingOrSecondFieldPicture_p->Decode.PictureDecodingStatus == VIDBUFF_PICTURE_BUFFER_DECODED)             ? "DECODED" : "INVALID",
                    CurrentFB_p->NothingOrSecondFieldPicture_p->Buffers.PictureLockCounter,
                    PRINT_DISP_STAT(CurrentFB_p->NothingOrSecondFieldPicture_p->Buffers.DisplayStatus)
                        ));
                }

            }
            CurrentFB_p = CurrentFB_p->NextAllocated_p;
        } while ((CurrentFB_p != DecimatedBufferPool_p) && (CurrentFB_p != NULL));
    }
} /* end of vidprod_DumpFBContent */

#endif

#ifdef STVID_DEBUG_PRODUCER
static void CheckPictureAndRefsValidity(VIDBUFF_PictureBuffer_t *PictureBuffer_p)
{
    VIDBUFF_FrameBuffer_t * CurrentFB_p;
    VIDBUFF_FrameBuffer_t * ReferenceFrameBuff_p;
    U32 i;

    CurrentFB_p = PictureBuffer_p->FrameBuffer_p;
    if (CurrentFB_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "NULL FB"));
    }
    else
    {
        for (i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; i++)
        {
            if (PictureBuffer_p->ParsedPictureInformation.PictureGenericData.IsValidIndexInReferenceFrame[i])
            {
                ReferenceFrameBuff_p = PictureBuffer_p->ReferencesFrameBufferPointer[i];
                if (ReferenceFrameBuff_p == NULL)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ReferenceFrameBuff_p == NULL Index %d", i));
                }
                else if (ReferenceFrameBuff_p->FrameOrFirstFieldPicture_p == NULL)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ReferenceFrameBuff_p->FrameOrFirstFieldPicture_p == NULL Index %d", i));
                }
                else
                if (!IsFramePicture(ReferenceFrameBuff_p->FrameOrFirstFieldPicture_p)) /* Current Picture is FIELD and Reference is FIELD */
                {
                    /* Reference First Field not NULL has been already tested above */
                    /* Reference Second Field test... */
                    if (ReferenceFrameBuff_p->NothingOrSecondFieldPicture_p == NULL)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ReferenceFrameBuff_p->NothingOrSecondFieldPicture_p == NULL Index %d", i));
                    }
                    else
                    {
                        if (ReferenceFrameBuff_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation.PictureSpecificData_p == NULL)
                        {
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "ReferenceFrameBuff_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation.PictureSpecificData_p == NULL Index %d", i));
                        }
                    }
                }
            }
        }
	}
} /* end of CheckPictureAndRefsValidity */
#endif /* STVID_DEBUG_PRODUCER */

/*******************************************************************************
Name        : computeNbFieldsDisplayNextDecode
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void ComputeNbFieldsDisplayNextDecode(const STVID_MPEGStandard_t MPEGStandard, VIDCOM_PictureGenericData_t * PictureGenericData_p)
{
	U32 TopCounter = 0, BottomCounter = 0, RepeatFirstCounter = 0, RepeatProgressiveCounter = 0;

	if (MPEGStandard == STVID_MPEG_STANDARD_ISO_IEC_11172)
    {
        /* Picture coding extension not valid in MPEG1 */
        TopCounter    = 1;
        BottomCounter = 1;
    }
	else
	{
        switch (PictureGenericData_p->PictureInfos.VideoParams.PictureStructure)
        {
            case STVID_PICTURE_STRUCTURE_TOP_FIELD :
                /* Field picture: top_field_first is 0, decode outputs one field picture */
                PictureGenericData_p->PictureInfos.VideoParams.TopFieldFirst = TRUE;
                TopCounter = 1;
                BottomCounter = 0;
                RepeatFirstCounter = 0;
                RepeatProgressiveCounter = PictureGenericData_p->RepeatProgressiveCounter;
                break;
            case STVID_PICTURE_STRUCTURE_BOTTOM_FIELD :
                /* Field picture: top_field_first is 0, decode outputs one field picture */
                PictureGenericData_p->PictureInfos.VideoParams.TopFieldFirst = FALSE;
                TopCounter = 0;
                BottomCounter = 1;
                RepeatFirstCounter = 0;
                RepeatProgressiveCounter = PictureGenericData_p->RepeatProgressiveCounter;
                break;
            case STVID_PICTURE_STRUCTURE_FRAME :
            default :
                /* Frame picture: top_field_first indicates which field is output first */
                /* Two fields: first field, second field */
                TopCounter    = 1;
                BottomCounter = 1;
                RepeatFirstCounter = 0;
                RepeatProgressiveCounter = PictureGenericData_p->RepeatProgressiveCounter;
                RepeatFirstCounter = PictureGenericData_p->RepeatFirstField;
                break;
        } /* end switch(picture_structure) */
	}
    PictureGenericData_p->NbFieldsDisplay  = (TopCounter + BottomCounter + RepeatFirstCounter) * (RepeatProgressiveCounter + 1);
}

#ifdef ST_speed
/*******************************************************************************
Name        : VIDPROD_GetBitRateValue
Description : Get bit rate value of the decoded stream in units of 400 bits/second.
              Compares the evaluated value with the value extracted from the stream. If there is no cal
Parameters  : Video decode handle, pointer on U32 to fill
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if bad parameter
              STVID_ERROR_NOT_AVAILABLE if can not give this information
*******************************************************************************/
ST_ErrorCode_t VIDPROD_GetBitRateValue(const VIDPROD_Handle_t ProducerHandle, U32 * const BitRateValue_p)
{
    U32 CalculatedBitRateValue = 0;
    U8 i;
    BitRateValueEvaluate_t * BitRate_p;
    U32 BitBufferSize,Time;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VIDPROD_Data_t        * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

    if (BitRateValue_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    BitRate_p = &(VIDPROD_Data_p->BitRateValueEvaluate);
    if (BitRate_p->BitRateValidValueNb > 0)
    {

        BitBufferSize = 0;
        Time          = 0;
        /* sum the total time and the total size */
        for (i = 0; i < BitRate_p->BitRateValidValueNb; i++)
        {
            BitBufferSize += BitRate_p->BitBufferSize[i];
            Time          += BitRate_p->Time[i];
        }

        /* bitrate = nb bits / sec */
        if (Time != 0)
        {
            if (BitBufferSize > (((U32)ULONG_MAX) / VIDPROD_TIME_FACTOR))
            {
                if ((BitBufferSize / VIDPROD_SIZE_UNIT_FACTOR) > (((U32)ULONG_MAX) / VIDPROD_TIME_FACTOR))
                {
                    CalculatedBitRateValue = ((BitBufferSize / VIDPROD_SIZE_UNIT_FACTOR) / Time) * VIDPROD_TIME_FACTOR;
                }
                else
                {
                   CalculatedBitRateValue = ((BitBufferSize / VIDPROD_SIZE_UNIT_FACTOR) * VIDPROD_TIME_FACTOR) / Time;
                }
            }
            else
            {
                CalculatedBitRateValue = ((BitBufferSize * VIDPROD_TIME_FACTOR) / VIDPROD_SIZE_UNIT_FACTOR) / Time;
            }
        }
        else
        {
            return (STVID_ERROR_NOT_AVAILABLE);
        }

        if ((ErrorCode == ST_NO_ERROR) )
        {
                *BitRateValue_p =  CalculatedBitRateValue;
        }
	}
    else
    {
        /* If there is not calculated value, extracted value CAN NOT be returned. */
        return (STVID_ERROR_NOT_AVAILABLE);
    }
#ifdef STVID_DEBUG_GET_STATISTICS
    VIDPROD_Data_p->Statistics.LastBitRate = *BitRateValue_p;
    if ( *BitRateValue_p > VIDPROD_Data_p->Statistics.MaxBitRate )
    {
        VIDPROD_Data_p->Statistics.MaxBitRate =  *BitRateValue_p;
    }
    if ( *BitRateValue_p < VIDPROD_Data_p->Statistics.MinBitRate )
    {
        VIDPROD_Data_p->Statistics.MinBitRate =  *BitRateValue_p;
    }
#endif /*STVID_DEBUG_GET_STATISTICS*/
    return(ST_NO_ERROR);

} /* End of vidprod_GetBitRateValue() function */
/*******************************************************************************
Description : Compute Bit Rate Value
Parameters  : DecodeHanlde,
Assumptions : should be called every time a picture is to be decoded or skipped (when Trick mode is not requiring HDD jumps)
Limitations :
Returns     : Nothing
*******************************************************************************/
static void EvaluateBitRateValue(const VIDPROD_Handle_t ProducerHandle, U32 NbFieldsDisplayNextDecode, U32 FrameRate, U32 OutputCounter)
{
    VIDPROD_Data_t * VIDPROD_Data_p;
    BitRateValueEvaluate_t * BitRate_p;
    U32 BitBufferOutputCounter, DifWithLastBitBufferOutputCounter ;


    /* Find structures */
    VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    BitRate_p = &(VIDPROD_Data_p->BitRateValueEvaluate);



    /* Take bit buffer counter */
#ifdef STVID_TRICKMODE_BACKWARD
    if (!VIDPROD_Data_p->Backward.IsBackward)
    {
        BitBufferOutputCounter = PARSER_GetBitBufferOutputCounter(VIDPROD_Data_p->ParserHandle);
    }
    else
#endif
    {
        BitBufferOutputCounter = OutputCounter ;
    }
    if (FrameRate != 0)
    {
            if (BitBufferOutputCounter < BitRate_p->LastBitBufferOutputCounter)
            {
                DifWithLastBitBufferOutputCounter = (U32)(BitBufferOutputCounter + (0xFFFFFFFF - BitRate_p->LastBitBufferOutputCounter));
            }
            else
            {
                DifWithLastBitBufferOutputCounter = (U32)(BitBufferOutputCounter - BitRate_p->LastBitBufferOutputCounter);
            }
            /* store the data in current index */
            BitRate_p->BitBufferSize[BitRate_p->BitRateIndex] = DifWithLastBitBufferOutputCounter;
            BitRate_p->Time[BitRate_p->BitRateIndex]          = (((NbFieldsDisplayNextDecode * 1000 * VIDPROD_TIME_FACTOR)
                                                               / FrameRate) / 2);
                                                               /* * 1000 : Frame Rate: convert from mHz to Hz */
                                                               /* * VIDPROD_TIME_FACTOR : keep accuracy before div */
                                                                /* /2 : Frame->fields */
            /* index of stored values management */
            BitRate_p->BitRateIndex++;
            if (BitRate_p->BitRateIndex == VIDPROD_NB_PICTURES_USED_FOR_BITRATE)
            {
                BitRate_p->BitRateIndex = 0;
            }
            /* available stored values management */
            if (BitRate_p->BitRateValidValueNb < VIDPROD_NB_PICTURES_USED_FOR_BITRATE)
            {
                BitRate_p->BitRateValidValueNb++;
            }
    }/* endif  FrameRate != 0 */
    /* prepare next call */
    BitRate_p->LastBitBufferOutputCounter  = BitBufferOutputCounter;
} /* End of EvaluateBitRateValue() function */
#ifdef STVID_TRICKMODE_BACKWARD
/*******************************************************************************
Name        : SmoothBackwardInitStructure
Description : parameters to be initialized when trickmode is initialized
              and when switching from positive to negative speed
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void vidprod_SmoothBackwardInitStructure(const VIDPROD_Handle_t  ProducerHandle)
{
    VIDPROD_Data_t    * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

    /* inits */
    VIDPROD_Data_p->LinearBitBuffers.BufferA.DiscontinuityWithPreviousBuffer = FALSE;
    VIDPROD_Data_p->LinearBitBuffers.BufferB.DiscontinuityWithPreviousBuffer = FALSE;
    VIDPROD_Data_p->LinearBitBuffers.BufferA.InjectionDuration = (VIDPROD_BACKWARD_PICTURES_NB_BY_BUFFER * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) / VIDPROD_DEFAULT_FRAME_RATE;
    VIDPROD_Data_p->LinearBitBuffers.BufferB.InjectionDuration = (VIDPROD_BACKWARD_PICTURES_NB_BY_BUFFER * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) / VIDPROD_DEFAULT_FRAME_RATE;
    VIDPROD_Data_p->LinearBitBuffers.BufferA.BeginningOfInjectionBitrateValue = VIDPROD_MAX_BITRATE;
    VIDPROD_Data_p->LinearBitBuffers.BufferB.BeginningOfInjectionBitrateValue = VIDPROD_Data_p->LinearBitBuffers.BufferA.BeginningOfInjectionBitrateValue;
    VIDPROD_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p = NULL;
    VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p = NULL;
    VIDPROD_Data_p->LinearBitBuffers.BufferUnderDecode_p = NULL;


} /* end of vidprod_SmoothBackwardInitStructure() function */

/*******************************************************************************
Name        : PutSequenceStartCodeAndComputeLinearBitBuffers
Description : Put a sequence start code at the beginning of the buffer in order
              to synchronize the programmable start code pointer.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void PutSequenceStartCodeAndComputeLinearBitBuffers(
        const VIDPROD_Handle_t  ProducerHandle,
        VIDPROD_LinearBitBuffer_t * const  LinearBitBuffer_p)
{
    U32                 AlignmentInBytes = 0;
    U8                * BufferPointer_p;

    UNUSED_PARAMETER(ProducerHandle);

    /* Put Sequence start code at the begin of the buffer.                   */
    BufferPointer_p = (U8 *)(LinearBitBuffer_p->WholeBufferStartAddress_p);

    /* Here we have the Data buffer Start Address. Store it.                 */
    LinearBitBuffer_p->ValidDataBufferStartAddress_p = (void *)((U32)(BufferPointer_p) + AlignmentInBytes);

    /* Compute the end of the buffer : a space has to be set to write picture start */
    /* if valid data buffer code if transfered data */
    BufferPointer_p = (U8 *)(LinearBitBuffer_p->WholeBufferEndAddress_p);
    LinearBitBuffer_p->TransferDataMaxEndAddress_p = (void *)((U32)(BufferPointer_p) - AlignmentInBytes);
    LinearBitBuffer_p->TransferDataMaxSize = ((U32)LinearBitBuffer_p->TransferDataMaxEndAddress_p - (U32)LinearBitBuffer_p->ValidDataBufferStartAddress_p);

    /* PICTURE_START_CODE put here and now is not really useful */
    LinearBitBuffer_p->ValidDataBufferEndAddress_p = NULL;

    /* Data buffer size compute */
    LinearBitBuffer_p->ValidDataBufferSize = 0; /* (U32)(LinearBitBuffer_p->ValidDataBufferEndAddress_p) - (U32)(LinearBitBuffer_p->ValidDataBufferStartAddress_p); */
} /* end of PutSequenceStartCodeAndComputeLinearBitBuffers() function */

/*******************************************************************************
Name        : AllocateLinearBitBuffers
Description : Allocated two linear bit buffers to store streams
Parameters  :
Assumptions :
Limitations :
Returns     : ST_ERROR_NO_MEMORY, ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t vidprod_AllocateLinearBitBuffers(VIDPROD_Handle_t ProducerHandle)
{
    VIDPROD_Data_t  * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

    /* Return params of the buffer A */
    VIDPROD_Data_p->LinearBitBuffers.BufferA.WholeBufferStartAddress_p = (void*)VIDPROD_Data_p->BitBuffer.Address_p;
    VIDPROD_Data_p->LinearBitBuffers.BufferA.WholeBufferEndAddress_p   = (void *)
                                                                          (  ( (U32)VIDPROD_Data_p->BitBuffer.Address_p
                                                                              + ((VIDPROD_Data_p->BitBuffer.TotalSize + 1)/2))
                                                                          - 1 );
    /* Return params of the buffer B */
    VIDPROD_Data_p->LinearBitBuffers.BufferB.WholeBufferStartAddress_p = (void *)
                                                                           ( (U32)VIDPROD_Data_p->BitBuffer.Address_p
                                                                            + ((VIDPROD_Data_p->BitBuffer.TotalSize + 1)/ 2));
    VIDPROD_Data_p->LinearBitBuffers.BufferB.WholeBufferEndAddress_p   = (void *)
                                                                          (  ( (U32)VIDPROD_Data_p->BitBuffer.Address_p
                                                                              + VIDPROD_Data_p->BitBuffer.TotalSize)
                                                                          - 1);

    /* Linear Bit Buffers settings */
    PutSequenceStartCodeAndComputeLinearBitBuffers(ProducerHandle, &(VIDPROD_Data_p->LinearBitBuffers.BufferA));
    PutSequenceStartCodeAndComputeLinearBitBuffers(ProducerHandle, &(VIDPROD_Data_p->LinearBitBuffers.BufferB));

    return(ST_NO_ERROR);
} /* End of AllocateLinearBitBuffers() function */

/*******************************************************************************
Name        : AskForLinearBitBufferFlush
Description : Ask the decode part to send Data Underflow event
Parameters  : TrickModeHandle, BufferToBeFilled, ReFlush : if FALSE, same tansfert as the last done is required.
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void vidprod_AskForLinearBitBufferFlush( const VIDPROD_Handle_t ProducerHandle,
                                                VIDPROD_LinearBitBuffer_t * BufferToBeFilled_p,
                                                const VIDPROD_FlushBitBufferManner_t FlushManner)
{
	PARSER_StartParams_t        PARSER_StartParams;
    VIDPROD_Data_t    * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    U32                 MaxDataToTransfer = 0;

    U32                 LoopCounter;
    S32                 OverlapDuration = 0;
	OverlapDuration = (2 * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) / 25;

    STOS_SemaphoreWait(VIDPROD_Data_p->LinearBitBuffers.PreprocessingPictureSemaphore_p);
    VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p = NULL;
    STOS_SemaphoreSignal(VIDPROD_Data_p->LinearBitBuffers.PreprocessingPictureSemaphore_p);
    if (   (BufferToBeFilled_p == NULL)
        || (BufferToBeFilled_p->ValidDataBufferStartAddress_p == NULL))
    {
        return;
    }

    /* Use the bitrate value computed in the last linear bitbuffer */
    VIDPROD_GetBitRateValue(ProducerHandle, &(VIDPROD_Data_p->DataUnderflow.BitRateValue));
    if(VIDPROD_Data_p->DataUnderflow.BitRateValue == 0)
    {
        VIDPROD_Data_p->DataUnderflow.BitRateValue = VIDPROD_MAX_BITRATE;
    }

    VIDPROD_Data_p->DataUnderflow.BitBufferFreeSize = ( VIDPROD_Data_p->DataUnderflow.BitRateValue ) ;  /* to have 1s in the buffer */
    VIDPROD_Data_p->DataUnderflow.BitBufferFreeSize = VIDPROD_Data_p->DataUnderflow.BitBufferFreeSize *50; /* *400 /8 for bytes */

    MaxDataToTransfer = BufferToBeFilled_p->TransferDataMaxSize - 1024;
    if (VIDPROD_Data_p->DataUnderflow.BitBufferFreeSize > MaxDataToTransfer)
    {
        VIDPROD_Data_p->DataUnderflow.BitBufferFreeSize = MaxDataToTransfer ;
    }
    if (VIDPROD_Data_p->DataUnderflow.BitBufferFreeSize < ((U32)ULONG_MAX / STVID_DURATION_COMPUTE_FACTOR))
    {
        VIDPROD_Data_p->DataUnderflow.RequiredDuration =
            (VIDPROD_Data_p->DataUnderflow.BitBufferFreeSize * STVID_DURATION_COMPUTE_FACTOR) / VIDPROD_Data_p->DataUnderflow.BitRateValue;
    }
    else
    {
        VIDPROD_Data_p->DataUnderflow.RequiredDuration =
            ((VIDPROD_Data_p->DataUnderflow.BitBufferFreeSize*18) / (VIDPROD_Data_p->DataUnderflow.BitRateValue/100)) ;
    }

    PARSER_SetBitBuffer(VIDPROD_Data_p->ParserHandle, BufferToBeFilled_p->ValidDataBufferStartAddress_p, VIDPROD_Data_p->DataUnderflow.BitBufferFreeSize, BIT_BUFFER_LINEAR, TRUE);

    switch (FlushManner)
    {
        case VIDPROD_FLUSH_WITH_NORMAL_OVERLAP : /* no overlap, OK for I only */
            if (VIDPROD_Data_p->DriftTime != 0)
            {
                VIDPROD_Data_p->Backward.InjectDiscontinuityCanBeAccepted = TRUE;
            }
            VIDPROD_Data_p->DataUnderflow.RequiredTimeJump = - (2 * VIDPROD_Data_p->DataUnderflow.RequiredDuration) + VIDPROD_Data_p->DriftTime;
			VIDPROD_Data_p->DriftTime = 0;
            VIDPROD_Data_p->DataUnderflow.TransferRelatedToPrevious = (OverlapDuration > 0) ? TRUE : FALSE;
            break;

        case VIDPROD_FLUSH_BBFSIZE_BACKWARD_JUMP :
            if (VIDPROD_Data_p->DriftTime != 0)
            {
                VIDPROD_Data_p->Backward.InjectDiscontinuityCanBeAccepted = TRUE;
            }
            VIDPROD_Data_p->DataUnderflow.RequiredTimeJump = - (2 * VIDPROD_Data_p->DataUnderflow.RequiredDuration) - VIDPROD_Data_p->DriftTime;
            VIDPROD_Data_p->DriftTime = 0;
            VIDPROD_Data_p->DataUnderflow.TransferRelatedToPrevious = FALSE;
            break;

            case VIDPROD_FLUSH_BECAUSE_OF_NO_I_PICTURE :
            VIDPROD_Data_p->DataUnderflow.RequiredTimeJump = - (3 * VIDPROD_Data_p->DataUnderflow.RequiredDuration / 2) + OverlapDuration;
            VIDPROD_Data_p->DataUnderflow.TransferRelatedToPrevious = FALSE;
            break;

        default :
            VIDPROD_Data_p->DataUnderflow.RequiredDuration = 0;
            VIDPROD_Data_p->DataUnderflow.RequiredTimeJump = 0;
            VIDPROD_Data_p->DataUnderflow.TransferRelatedToPrevious = FALSE;
            break;
    }
    if (VIDPROD_Data_p->Backward.ChangingDirection == TRICKMODE_CHANGING_BACKWARD)
	{
		VIDPROD_Data_p->DataUnderflow.RequiredTimeJump = 2 * VIDPROD_Data_p->DataUnderflow.RequiredTimeJump;
	}
    /* Buffer is now under transfer */
    STOS_SemaphoreWait(VIDPROD_Data_p->LinearBitBuffers.PreprocessingPictureSemaphore_p);
    VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p = BufferToBeFilled_p;
	VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->NbNotIPicturesInBuffer = 0;
    for(LoopCounter = 0 ; LoopCounter < MAX_BACKWARD_PICTURES ; LoopCounter++)
    {
        VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].Decode.PictureDecodingStatus = VIDBUFF_PICTURE_BUFFER_EMPTY;
        VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.IsPictureBufferUsed[LoopCounter] = FALSE;
        VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.IsPictureTimedOut[LoopCounter] = FALSE;
    }
    VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->TotalNbIFields = 0;
    VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->TotalNbNotIFields = 0;
    VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.NbParsedPicturesLeft = 0;
    VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->NbIPicturesInBuffer = 0;
    VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->NbNotIPicturesInBuffer = 0;

    /* next to be transfered is the associated buffer */
    VIDPROD_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p = BufferToBeFilled_p->AssociatedLinearBitBuffer_p;
    STOS_SemaphoreSignal(VIDPROD_Data_p->LinearBitBuffers.PreprocessingPictureSemaphore_p);

    VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue = 0;

	PARSER_StartParams.StreamType = VIDPROD_Data_p->ParserStatus.StreamType;
    PARSER_StartParams.StreamID = VIDPROD_Data_p->ParserStatus.StreamID;
    PARSER_StartParams.BroadcastProfile = VIDPROD_Data_p->BroadcastProfile;
    PARSER_StartParams.ErrorRecoveryMode = PARSER_ERROR_RECOVERY_MODE1;     /*  TODO_CL : Not yet defined, should be defined according to VIDPROD_Data_p->ErrorRecoveryMode value */
    PARSER_StartParams.OverwriteFullBitBuffer = TRUE;
    PARSER_StartParams.BitBufferWriteAddress_p = VIDPROD_Data_p->BitBuffer.Address_p;
    PARSER_StartParams.DefaultGDC_p = NULL;
    PARSER_StartParams.DefaultPDC_p = NULL;
    PARSER_StartParams.RealTime = VIDPROD_Data_p->RealTime;
    PARSER_StartParams.ResetParserContext = FALSE;

    ErrorCode = PARSER_Start(VIDPROD_Data_p->ParserHandle,&PARSER_StartParams);

	/* Ask the decode to notify this event by itself */
    vidprod_NotifyDecodeEvent(ProducerHandle, STVID_DATA_UNDERFLOW_EVT, (const void *) &(VIDPROD_Data_p->DataUnderflow));
	STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Underflow %d \r\n", VIDPROD_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p));
	STTBX_Report((STTBX_REPORT_LEVEL_INFO,"BR=%d Jump=%d Dur=%d Ovlap=%d Dst=0x%.8x\r\n",
            VIDPROD_Data_p->DataUnderflow.BitRateValue,
            VIDPROD_Data_p->DataUnderflow.RequiredTimeJump,
            VIDPROD_Data_p->DataUnderflow.RequiredDuration,
            OverlapDuration,
            BufferToBeFilled_p->WholeBufferStartAddress_p));

#if defined(TRACE_UART) && !defined(VALID_TOOLS)
TraceBuffer(("Send underflow backward BR=%d, Free size %d,  Jump=%d Dur=%d Ovlap=%d Dst=0x%.8x\r\n",
            VIDPROD_Data_p->DataUnderflow.BitRateValue,
            VIDPROD_Data_p->DataUnderflow.BitBufferFreeSize,
            VIDPROD_Data_p->DataUnderflow.RequiredTimeJump,
            VIDPROD_Data_p->DataUnderflow.RequiredDuration,
            OverlapDuration,
            BufferToBeFilled_p->WholeBufferStartAddress_p));
#endif

} /* end of vidprod_AskForLinearBitBufferFlush() function */

/*******************************************************************************
Name        : VIDPROD_ChangeDirection
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void VIDPROD_ChangeDirection(const VIDPROD_Handle_t ProducerHandle, const DecodeDirection_t Change)
{
    VIDPROD_Data_t    * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

    if (Change == TRICKMODE_NOCHANGE)
    {
        /* No direction change requested returns immediately */
        return;
    }

    VIDPROD_Data_p->Backward.NbDecodedPictures = 0;
    VIDPROD_Data_p->Backward.WaitingProducerReadyToSwitchBackward = FALSE;
    VIDPROD_Data_p->Backward.WaitingProducerReadyToSwitchForward = FALSE;

    /* Set Direction Change request */
    VIDPROD_Data_p->Backward.ChangingDirection = Change;

#if defined(TRACE_UART) && !defined(VALID_TOOLS)
    TraceBuffer(("Ask speed change to %s\r\n", (Change == TRICKMODE_CHANGING_BACKWARD) ? "Backward" : "Forward"));
#endif
    VIDPROD_Data_p->Backward.DoNotTreatParserJobCompletedEvt = TRUE;
    VIDPROD_Data_p->ParserStatus.IsSearchingPicture = FALSE;    /* Does not search anymore for pictures until new data are injected */
    PushControllerCommand(ProducerHandle, CONTROLLER_COMMAND_CHANGE_DIRECTION);
    STOS_SemaphoreSignal(VIDPROD_Data_p->ProducerOrder_p);
}
/*******************************************************************************
Name        : PutPictureStartCodeAndComputeValidDataBuffer
Description : Put a picture start code at the end of the buffer. It will be used
              in order to stop the parsing.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static BOOL PutPictureStartCodeAndComputeValidDataBuffer(const VIDPROD_Handle_t ProducerHandle, VIDPROD_LinearBitBuffer_t * const LinearBitBuffer_p, const BOOL Overlap)
{
    U32                 MinDataInjected;
    U32                 SCDAlignment;
    U8 *                BufferPointer_p;
    PARSER_GetBitBufferLevelParams_t PARSER_GetBitBufferLevelParams;
    VIDPROD_Data_t    * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

    UNUSED_PARAMETER(Overlap);

    /* CAUTION : DataEndAddress_p must have the same alignement than SCD_RD  */
    /* register! As the size of the data of a Start Code is less or egal     */
    /* than 257 bytes, we will discard as many blocks to fill 257 + SC + 1.  */
    /* Indeed there is a risk that a SC in this area could be not complete.  */
    /* Although there is no risk with a size greater of 1.                   */

    /* Get the size of the data transfered in the buffer                     */
    ErrorCode = PARSER_GetBitBufferLevel(VIDPROD_Data_p->ParserHandle, &PARSER_GetBitBufferLevelParams);
    LinearBitBuffer_p->TransferedDataSize = PARSER_GetBitBufferLevelParams.BitBufferLevelInBytes;
    /* Computing MinDataInjected with Bit rate may not be incoherent if various bit rate ... */
    /* we prefer a fixed size */
    MinDataInjected = (2 * 1024);

    /* Workaround for an eventual failing of the hardware (seen on 5518, 5517). */
    if (LinearBitBuffer_p->TransferedDataSize > ((U32)LinearBitBuffer_p->WholeBufferEndAddress_p - (U32)LinearBitBuffer_p->WholeBufferStartAddress_p + 1))
    {
        /* Size is not convenient : ReFlush the just flushed buffer */
        vidprod_AskForLinearBitBufferFlush(ProducerHandle, LinearBitBuffer_p->AssociatedLinearBitBuffer_p, VIDPROD_FLUSH_BBFSIZE_BACKWARD_JUMP);
               return (FALSE);
    }
    else if (LinearBitBuffer_p->TransferedDataSize < MinDataInjected)
    {
        PARSER_FlushInjection(VIDPROD_Data_p->ParserHandle);
        VIDPROD_Data_p->Backward.IsPreviousInjectionFine = FALSE;
        PARSER_Stop(VIDPROD_Data_p->ParserHandle);
        if (VIDPROD_Data_p->Backward.IsInjectionNeeded)
        {
            vidprod_AskForLinearBitBufferFlush(ProducerHandle, LinearBitBuffer_p->AssociatedLinearBitBuffer_p, VIDPROD_FLUSH_BBFSIZE_BACKWARD_JUMP);
        }

#if defined(TRACE_UART) && !defined(VALID_TOOLS)
        TraceBuffer(("Put Picture ERROR %d \r\n", LinearBitBuffer_p->TransferedDataSize));
#endif
        return (FALSE);

    }
    /* End Address of the valid data buffer */
    LinearBitBuffer_p->TransferedDataEndAddress_p = (void *)((U32)(LinearBitBuffer_p->ValidDataBufferStartAddress_p) + LinearBitBuffer_p->TransferedDataSize);

    /* Put a Picture Start Code at the end of the buffer in unit of the SCD  */
    /* pointer. It will be useful to find the end of the buffer.             */
    SCDAlignment = 1;

    /* Write the Picture Start Code.                                        */
    BufferPointer_p = (U8*)(((U32)LinearBitBuffer_p->TransferedDataEndAddress_p) & (U32)(~(SCDAlignment - 1)));
    PARSER_WriteStartCode(VIDPROD_Data_p->ParserHandle, 0x0, (void*)BufferPointer_p);

    /* Useful data are stored before the cut data.                           */
    LinearBitBuffer_p->ValidDataBufferEndAddress_p =
            (void *) ((U32)BufferPointer_p - 263);

    /* Data buffer size compute.                                             */
    LinearBitBuffer_p->ValidDataBufferSize =
            (U32)(LinearBitBuffer_p->ValidDataBufferEndAddress_p) -
            (U32)(LinearBitBuffer_p->ValidDataBufferStartAddress_p);
    PARSER_SetBitBuffer(VIDPROD_Data_p->ParserHandle, LinearBitBuffer_p->ValidDataBufferStartAddress_p, LinearBitBuffer_p->ValidDataBufferSize, BIT_BUFFER_LINEAR, FALSE);
    if ((VIDPROD_Data_p->ErrorRecoveryMode == STVID_ERROR_RECOVERY_FULL) || (VIDPROD_Data_p->ErrorRecoveryMode == STVID_ERROR_RECOVERY_HIGH))
    {
        VIDPROD_Data_p->ParserStatus.SearchNextRandomAccessPoint = TRUE;
    }
    VIDPROD_Data_p->Backward.IsInjectionNeeded =  FALSE;
    PushControllerCommand(ProducerHandle, CONTROLLER_COMMAND_SEARCH_PICTURE);
    STOS_SemaphoreSignal(VIDPROD_Data_p->ProducerOrder_p);
	STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Put picture start code \r\n"));
    return (TRUE);
} /* end of PutPictureStartCodeAndComputeValidDataBuffer() function */

#endif   /* STVID_TRICKMODE_BACKWARD */
#endif /*ST_speed*/

/*******************************************************************************
Name        : vidprod_FlushPresentationQueue
Description : Flush ordering Queue and mark all pending decode as not needed for presentation
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if bad parameter
              STVID_ERROR_NOT_AVAILABLE if can not give this information
*******************************************************************************/
static ST_ErrorCode_t vidprod_FlushPresentationQueue(const VIDPROD_Handle_t ProducerHandle, VIDBUFF_PictureBuffer_t * PictureBuffer_p)
{
    VIDPROD_Data_t        * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    U32 LoopCounter;

    /* Flush presentation queue and mark all previous picture with NeededForPresentation = FALSE */
    VIDQUEUE_CancelAllPictures(VIDPROD_Data_p->OrderingQueueHandle);
    vidprod_ReleaseNonPushedNonRefPictures(ProducerHandle);
    for (LoopCounter = 0; LoopCounter < MAX_NUMBER_OF_DECODE_PICTURES ; LoopCounter++)
    {
        if (VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter] != NULL)
        {
            if (VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID < PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID)
            {
                VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->IsNeededForPresentation = FALSE;
                if (VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->AssociatedDecimatedPicture_p != NULL)
                {
                    VIDPROD_Data_p->DecodePicture.DecodePictureBuffer[LoopCounter]->AssociatedDecimatedPicture_p->IsNeededForPresentation = FALSE;
                }
            }
        }
    }
#ifdef STVID_TRICKMODE_BACKWARD
    if (!VIDPROD_Data_p->Backward.IsBackward)
    {
#endif /* STVID_TRICKMODE_BACKWARD */
        for(LoopCounter = 0; LoopCounter < MAX_NUMBER_OF_PREPROCESSING_PICTURES ; LoopCounter++)
        {
            if (VIDPROD_Data_p->PreprocessingPicture.IsPictureBufferUsed[LoopCounter])
            {
                if (VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID < PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID)
                {
                    VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].IsNeededForPresentation = FALSE;
                    if (VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].AssociatedDecimatedPicture_p != NULL)
                    {
                        VIDPROD_Data_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].AssociatedDecimatedPicture_p->IsNeededForPresentation = FALSE;
                    }
                }
            }
        }
#ifdef STVID_TRICKMODE_BACKWARD
    }
    else /* is backward */
    {
        for(LoopCounter = 0; LoopCounter < MAX_BACKWARD_PICTURES ; LoopCounter++)
        {
            if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.IsPictureBufferUsed[LoopCounter])
            {
                if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID < PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID)
                {
                    VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].IsNeededForPresentation = FALSE;
                    if (VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].AssociatedDecimatedPicture_p != NULL)
                    {
                        VIDPROD_Data_p->LinearBitBuffers.BufferUnderTransferandParsing_p->PreprocessingPicture.PreprocessingPictureBufferTable[LoopCounter].AssociatedDecimatedPicture_p->IsNeededForPresentation = FALSE;
                    }
                }
            }
        }
    }
#endif /* STVID_TRICKMODE_BACKWARD */

#if COMPUTE_PRESENTATION_ID
                for (LoopCounter = 0 ; LoopCounter < STORED_PICTURE_BUFFER_SIZE ; LoopCounter++)
                {
                    VIDPROD_Data_p->StoredPictureBuffer[LoopCounter] = NULL;
                }
                VIDPROD_Data_p->StoredPictureCounter = 0;
                VIDPROD_Data_p->NumOfPictToWaitBeforePush = 2;
                VIDPROD_Data_p->PreviousPictureCount = 0;
#endif /* COMPUTE_PRESENTATION_ID */

    return(ST_NO_ERROR);

} /* End of vidprod_FlushPresentationQueue() function */
#ifdef ST_speed
/*******************************************************************************
Name        : VIDPROD_SetDecodingMode
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void VIDPROD_SetDecodingMode(const VIDPROD_Handle_t ProducerHandle, const U8 Mode)
{
	VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
	DECODER_SetDecodingMode(VIDPROD_Data_p->DecoderHandle, Mode);
}

/*******************************************************************************
Name        : VIDPROD_UpdateDriftTime
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void VIDPROD_UpdateDriftTime(const VIDPROD_Handle_t ProducerHandle, const S32 DriftTime)
{
	VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
	VIDPROD_Data_p->DriftTime += DriftTime;
}
/*******************************************************************************
Name        : VIDPROD_ResetDriftTime
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void VIDPROD_ResetDriftTime(const VIDPROD_Handle_t ProducerHandle)
{
	VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    VIDPROD_Data_p->DriftTime = 0;
}
#endif /* ST_speed */

/*******************************************************************************
Name        : vidprod_DecrementNbPicturesInQueue
Description : Decrements by 1 the number of pictures to be decoded, and updates
              the bitbuffer read pointer in case a skip happened
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
#ifdef STTBX_REPORT
static ST_ErrorCode_t vidprod_DecrementNbPicturesInQueue(const VIDPROD_Handle_t ProducerHandle, const U32 LineNumber)
#else
static ST_ErrorCode_t vidprod_DecrementNbPicturesInQueue(const VIDPROD_Handle_t ProducerHandle)
#endif /* STTBX_REPORT */
{
    VIDPROD_Data_t        * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;
    PARSER_InformReadAddressParams_t InformReadAddressParams;
    ST_ErrorCode_t        ErrorCode;

    if (VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue > 0)
    {
        VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue--;

#ifdef STVID_TRICKMODE_BACKWARD
        if (!VIDPROD_Data_p->Backward.IsBackward)
#endif /* STVID_TRICKMODE_BACKWARD */
        {
            /* When a picture skip at the parse level (generally because we are
               searching for the next RAP, when there is no parsed picture waiting for decode and the current decodes are
               finished, the read point is actually the parser read point */
            if ((VIDPROD_Data_p->ParserStatus.IsPictureSkipJustHappened) && (VIDPROD_Data_p->DecoderStatus.NbPicturesInQueue == 0) && (VIDPROD_Data_p->PreprocessingPicture.NbUsedPictureBuffers == 0))
            {
                InformReadAddressParams.DecoderReadAddress_p = VIDPROD_Data_p->ParserStatus.LastPictureSkippedStopAddress_p;
                ErrorCode = PARSER_InformReadAddress(VIDPROD_Data_p->ParserHandle, &InformReadAddressParams);
                if (ErrorCode != ST_NO_ERROR)
                {
                    /* TODO_CL: error handling to be defined.
                     * error cases have to be defined */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "line %lu, PARSER_InformReadAddress failed Error=%d!!", LineNumber, ErrorCode));
                }
                VIDPROD_Data_p->ParserStatus.IsPictureSkipJustHappened = FALSE;
            }
        }
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "line %lu, NbPicturesInQueue was already == 0 !!! ", LineNumber));
    }
    return(ST_NO_ERROR);
} /* End of vidprod_DecrementNbPicturesInQueue() function */

/*******************************************************************************
Name        : VIDPROD_UpdateBitBufferParams
Description : Update Delta decoder with new bit buffer info
Parameters  :
Assumptions : The Bit buffer was reallocated via STVID_Setup()
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDPROD_UpdateBitBufferParams(const VIDPROD_Handle_t ProducerHandle, void * const Address_p, const U32 Size)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
	VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

	Error = DECODER_UpdateBitBufferParams(VIDPROD_Data_p->DecoderHandle, Address_p, Size);

    return(Error);
} /* End of VIDPROD_UpdateBitBufferParams() */

/*******************************************************************************
Name        : vidprod_GetBitBufferParams
Description : Get bit buffer info
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t vidprod_GetBitBufferParams(const VIDPROD_Handle_t ProducerHandle,  void ** const Address_p,  U32 * const Size_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    VIDPROD_Data_t * VIDPROD_Data_p = (VIDPROD_Data_t *) ProducerHandle;

   if ( (Address_p == NULL) || (Size_p == NULL) )
    {
        Error = ST_ERROR_BAD_PARAMETER;
    }

   *Address_p = VIDPROD_Data_p->BitBuffer.Address_p;
   *Size_p    = VIDPROD_Data_p->BitBuffer.TotalSize;

    return(Error);
} /* End of vidprod_GetBitBufferParams() */


/* End of vid_prod.c */
