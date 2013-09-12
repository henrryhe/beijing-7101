/*******************************************************************************

File name   : vid_dec.c

Description : Video decode source file

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
 7 Mar 2000        Created                                          HG
14 Feb 2000        Suppressed VIDDEC_Pause(): done by VIDDISP       HG
09 Mar 2001        Decimation and compression management.           GG
16 Mar 2001        Decode Event notification configuration.         GG
23 Apr 2001        VIDDEC event to notify DecodeOnce                HG
 7 Jan 2002        Implement VIDDEC_SkipData() feature              HG
29 Jan 2002        Decode now releases LastButOne_p reference before
                   asking a buffer to decode a reference.           HG
 1 Mar 2002        Better VIDDEC robustness: move MonitorAbnormally-
                   -MissingHALStatus() up, don't process start code
                   if softreset pending, semaphore_wait() of decode
                   orders up in task function, semaphore_wait() with
                   no time-out if decoder is stopped.               HG
16 May 2002        Two time-outs for decode launch: before HW decode
                   start, and until HW decode ends. Allows to keep
                   precise HW decode overtime monitoring even when
                   multi-instance delays the decode start           HG
28 Nov 2002        Take the hw Misalignment Error into account.     AN
22 Jul 2003        Added error recovery mode HIGH                   HG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Temporary: work-arounds 7015 still pending on cut 1.2 and pending on 5528 cuts 1.0 & 1.1 */
/* No more applied to STi7020. Seems to be resolved.     */
#define STI7015_STI5528_WA_GNBvd10059

/* Temporary: work-around that solves a crash issue when STVID_Stop is called */
#define STI7710_STI5105_WA_GNBvd50512

/* Workarounds disappearing on cut 1.2 */
/* none */

/* Define to add debug info and traces */
/*#define TRACE_DECODE */

/* Define to add debug info about Take/Release of Picture Buffers */
/*#define TRACE_TAKE_RELEASE*/

/*#define TRACE_BBL_TRACKING*/

/*#define TRACE_DECODE_ADDITIONAL_IT */ /* DON'T FORGET TO UNCOMMENT the same #define in vid_intr.c IF YOU  */
                                        /* UNCOMMENT HERE (otherwise you receive ITs you don't manage).     */

#if defined(TRACE_DECODE) || defined(TRACE_DECODE_ADDITIONAL_IT) || defined(TRACE_TAKE_RELEASE)
#define TRACE_UART
#endif /* TRACE_DECODE || TRACE_DECODE_ADDITIONAL_IT || TRACE_TAKE_RELEASE */

#if defined TRACE_UART || defined TRACE_BBL_TRACKING
#include "trace.h"
#endif /* TRACE_UART || TRACE_BBL_TRACKING */
/* Define to limit time for calls to VIDBUFF_GetUnusedPictureBuffer() */
/*#define LIMIT_TRY_GET_BUFFER_TIME*/

/* Define to optimize the buffers conso used in dual-display */
#define DONT_DECODE_NON_REF_OPTIM       /*  SET    */

/* Includes ----------------------------------------------------------------- */

#if !defined ST_OSLINUX
#include <limits.h>
#include <stdio.h>
#include <string.h>
#endif

#include "stddefs.h"
#include "stos.h"

#ifdef STVID_STVAPI_ARCHITECTURE
#include "dtvdefs.h"
#include "stgvobj.h"
#include "stavmem.h"
#include "dv_rbm.h"
#include "os20to21.h"
#endif /* end of def STVID_STVAPI_ARCHITECTURE */

#include "stvid.h"

#include "vid_com.h"
#include "vid_ctcm.h"

#include "buffers.h"

#include "vid_dec.h"

#include "halv_dec.h"

#include "vid_bits.h"
#include "vid_head.h"
#include "vid_intr.h"
#include "vid_sc.h"

#ifdef STVID_STVAPI_ARCHITECTURE
#include "vid_mb2r.h"
#endif /* end of def STVID_STVAPI_ARCHITECTURE */

#include "sttbx.h"
#include "stevt.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Dummy value that we set to check that a handle is valid, only one chance out of 2^32 to get it by accident ! */
#define VIDDEC_VALID_HANDLE 0x01551550

/* Minimum size of user data for internal process (in case asked less). */
#define MIN_USER_DATA_SIZE  15

/* Size of the circular bitbuffer */
#define MPEG_BIT_BUFFER_SIZE_MAIN_PROFILE_MAIN_LEVEL    380928 /* 0x5D000 */
#define MPEG_BIT_BUFFER_SIZE_MAIN_PROFILE_HIG_LEVEL   2105344 /* from 7000 tree: good=2105344, theoric=1187840, toobig=2121728 */

#if defined (ST_trickmod)
/* Increase bit buffer for HDD playback. From STPRM driver : it is better to
 * have a bitbuffer size aligned on a transport packet size (188) and a sector
 * size (512). */
#if defined (ST_smoothbackward)
#define DEFAULT_BIT_BUFFER_SIZE_SD   (188*512*44) /* 0x40A000, 4M used also as 2Mx2 for each linear bitbuffer */
#define DEFAULT_BIT_BUFFER_SIZE_HD   (2105344) /* TODO */
#else /* !ST_smoothbackward */
#define DEFAULT_BIT_BUFFER_SIZE_SD   (188*512*11) /* 0x102800, 1M */
#define DEFAULT_BIT_BUFFER_SIZE_HD   2105344 /* TODO */
#endif /* ST_smoothbackward */
#else /* !ST_trickmode */
#define DEFAULT_BIT_BUFFER_SIZE_SD    MPEG_BIT_BUFFER_SIZE_MAIN_PROFILE_MAIN_LEVEL /* 0x5D000 */
#define DEFAULT_BIT_BUFFER_SIZE_HD    MPEG_BIT_BUFFER_SIZE_MAIN_PROFILE_HIG_LEVEL  /* from 7000 tree: good=2105344, theoric=1187840, toobig=2121728 */
#endif /* ST_trickmode */

/* Max start code search time 20 field display time */
/* GNBvd16206/Issue366  : MAX_START_CODE_SEARCH_TIME needs to be > 16 * STVID_MAX_VSYNC_DURATION */
#define MAX_START_CODE_SEARCH_TIME (20 * STVID_MAX_VSYNC_DURATION)

/* Max recover time: almost instantaneous, 0.5 field display time */
#ifdef ST_deblock
#define MAX_RECOVER_TIME (STVID_MAX_VSYNC_DURATION * 2)
#else
#define MAX_RECOVER_TIME (STVID_MAX_VSYNC_DURATION / 2)
#endif /* ST_deblock */

/* Max decode time: 2 or 3 fields display time */
#define MAX_DECODE_TIME_SD (2 * STVID_MAX_VSYNC_DURATION)
#define MAX_DECODE_TIME_HD (3 * STVID_MAX_VSYNC_DURATION)

/* Max time before decode starts: this time is normally 0 in single decode. In multi-decode,
if decodes are pipelined in hardware, then decode of a decode may start after some time. This
time should never be more than 4 VSync's, otherwise it means that hardware is not capable
of decoding quickly enough for this number of instances. */
#define MAX_TIME_BEFORE_DECODE_START (4 * STVID_MAX_VSYNC_DURATION)

/* Max time before stopping decoder and notifying the decoder has stopped. When a delayed
stop is asked (STVID_STOP_END_OF_DATA), the decoder should stop by itself if it can't
find a free buffer after some time */
#define MAX_TIME_BEFORE_STOPPING_DECODER (2 * STVID_MAX_VSYNC_DURATION)

/* Theoritical minimal bitrate value that can be encountred (bits/s) */
#define MIN_BITRATE_VALUE (100*1024)   /* 100 Kbits/s */

/* Max skip time: almost instantaneous, 0.5 field display time (130) */
#define MAX_SKIP_TIME  (STVID_MAX_VSYNC_DURATION / 2)

/* Max time waiting for an order: 5 times per field display (not less) */
#ifdef WINCE_PERF_OPTIMIZE
#define MAX_WAIT_ORDER_TIME 2*STVID_MAX_VSYNC_DURATION
#else
#define MAX_WAIT_ORDER_TIME (STVID_MAX_VSYNC_DURATION / STVID_DECODE_TASK_FREQUENCY_BY_VSYNC)
#endif

/* Max time trying to get a picture buffer for decode: 2 fields display time (520-625) ? */
#define MAX_TRY_GET_BUFFER_TIME ((6 * STVID_MAX_VSYNC_DURATION) + MAX_WAIT_ORDER_TIME)

/* Max time bit buffer level is not changing (no decode + no data coming in), although a SC search was launched and none was found */
#define MAX_BIT_BUFFER_LEVEL_UNCHANGED_WHEN_NO_SC_TIME (3 * STVID_MAX_VSYNC_DURATION)

/* Bite rate value used for the initialization. */
#define DEFAULT_BIT_RATE_VALUE  5000

/* Margin to send bit buffer overflow event (in bytes). */
#define VIDDEC_BIT_BUFFER_OVERFLOW_SD_MARGIN  ((8192 * 8) * 3 / 2)
#define VIDDEC_BIT_BUFFER_OVERFLOW_HD_MARGIN  ((8192 * 8) * 6)

/* Margin on bit buffer to skip decode on real-time decode */
/* note following values must be less than previous (VIDDEC_BIT_BUFFER_OVERFLOW_xD_MARGIN) */
#define VIDDEC_BIT_BUFFER_REAL_TIME_OVERFLOW_SD_MARGIN  (0x2800) /* 10Ko (evaluated with max bit rate injected) */
#define VIDDEC_BIT_BUFFER_REAL_TIME_OVERFLOW_HD_MARGIN  (0xc800) /* 50Ko (evaluated with max bit rate injected) */

/* mult factor used to keep precision before the divisions in bit rate eval */
#define VIDDEC_TIME_FACTOR                  0x1000  /* 2^n is fast to process */
#define VIDDEC_SIZE_UNIT_FACTOR             50      /* convert bytes -> 400bits unit */

/* After too much errors without a good decode, we consider the problem could be inside the decoder. We do a Soft Reset.  */
#define ACCEPTANCE_ERROR_AMOUNT_HIGHEST_ERROR_RECOVERY 3       /* Amount of consecutive errors for Error Recovery Full    */
#define ACCEPTANCE_ERROR_AMOUNT_LOWEST_ERROR_RECOVERY 10       /* Amount of consecutive errors for Error Recovery not none */


#define VIDDEC_BIT_BUFFER_THRESHOLD_IDLE_VALUE  0x200

/* Picture which size is above this limit will be passed to display immediately after
decode launch because there's a risk that their decode lasts for too long.
This helps the big HD streams like ATSC play better. */
#define VIDDEC_LIMIT_PICTURE_SIZE_TO_DISPLAY_AT_DECODE_LAUNCH (1000 * 700)

/* AUTOMATIC mode only : Overflow evt has to be sent it Bit Buffer Free size in under LinearBitBufferSize / LINEAR_BIT_BUFFER_OVERFLOW_RATIO*/
#define LINEAR_BIT_BUFFER_OVERFLOW_RATIO             20

/* Private Variables (static)------------------------------------------------ */

/* Default MPEG matrix: used by all decoders */
static const QuantiserMatrix_t DefaultIntraQuantMatrix = {
    0x08, 0x10, 0x10, 0x13, 0x10, 0x13, 0x16, 0x16,
    0x16, 0x16, 0x16, 0x16, 0x1A, 0x18, 0x1A, 0x1B,
    0x1B, 0x1B, 0x1A, 0x1A, 0x1A, 0x1A, 0x1B, 0x1B,
    0x1B, 0x1D, 0x1D, 0x1D, 0x22, 0x22, 0x22, 0x1D,
    0x1D, 0x1D, 0x1B, 0x1B, 0x1D, 0x1D, 0x20, 0x20,
    0x22, 0x22, 0x25, 0x26, 0x25, 0x23, 0x23, 0x22,
    0x23, 0x26, 0x26, 0x28, 0x28, 0x28, 0x30, 0x30,
    0x2E, 0x2E, 0x38, 0x38, 0x3A, 0x45, 0x45, 0x53
};
static const QuantiserMatrix_t DefaultNonIntraQuantMatrix = {
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10
};

#ifdef TRACE_DECODE
static U32 StartCodes[200];
static U8 StartCodeIndex = 0;
static U32 FrameBuffs[200];
static U32 PicTypes[200];
static U32 PicTempRef[200];
static U8 FrameBuffsIndex = 0;
extern void * CurrentFrameOnDisplay;
static U32 QueueingPic[200];
static U8 QueueingPicIndex = 0;
U32 ITs[200];
U8 ITsIndex = 0;
static clock_t ReadyToDecode[1024];
static clock_t LaunchDecode[1024];
static clock_t InterruptPSD[1024];
static clock_t DecodeMaxTime[1024];
static clock_t FinishedDecode[1024];
static clock_t DecodeOvertime[1024];
static U8 DecodeIndex = 0;
static U32 StopAtPic = 58; /* Extended Temporal Reference of the picture with want to stop on. */
                           /* To be used with a breakpoint on the line where it is used.       */

#endif /* TRACE_DECODE */

#if defined(LCMPEGX1_WA_GNBvd45748)
extern void HALDEC_LCMPEGX1GNBvd45748WA(const HALDEC_Handle_t  HALDecodeHandle);
#endif /* defined(LCMPEGX1_WA_GNBvd45748) */

/* Private Macros ----------------------------------------------------------- */

#define IsHWDecoderIdle(DecodeHandle) (((VIDDEC_Data_t *) (DecodeHandle))->HWDecoder.HWState <= VIDDEC_HARDWARE_STATE_IDLE)
#define IsHWDecoderReadyToDecode(DecodeHandle) (((VIDDEC_Data_t *) (DecodeHandle))->HWDecoder.HWState == VIDDEC_HARDWARE_STATE_FOUND_NEXT_PICTURE)

#define Virtual(DevAdd) (void*)((U32)(DevAdd)\
        -(U32) (VIDDEC_Data_p->VirtualMapping.PhysicalAddressSeenFromDevice_p)\
        +(U32) (VIDDEC_Data_p->VirtualMapping.VirtualBaseAddress_p))

/* Temporary Macro to convert ExtendedTemporalReference to VIDCOM_PictureID_t   */
/* Should be removed has soon the "bumping process" is working in producer !!   */
#define PICTUREID_FROM_EXTTEMPREF(picid, extempref, temprefoffset)  \
        {                                                           \
                (picid).ID = (extempref) - (temprefoffset);         \
                (picid).IDExtension = (temprefoffset);              \
        }

#define  MPEGFrame2Char(Type)  (                                        \
               (  (Type) == STVID_MPEG_FRAME_I)                         \
                ? 'I' : (  ((Type) == STVID_MPEG_FRAME_P)               \
                         ? 'P' : (  ((Type) == STVID_MPEG_FRAME_B)      \
                                  ? 'B'  : 'U') ) )

/* Error recovery strategy macros :                                                                             */
/*                                                                                                              */
/* ===========================================================================================================  */
/*                                                                   |   NONE  | PARTIAL |  HIGH   |   FULL  |  */
/* ===========================================================================================================  */
/* Is Hardware Misalignment Error To Be Cancelled                    |    X    |         |         |         |  */
/* Is B Picture To Be Decoded If Missing Reference                   |    X    |         |         |         |  */
/* Is Picture Decode Hardware SyntaxError To Be Cancelled            |    X    |    X    |         |         |  */
/* Is P Picture To Be Decoded If Missing Reference                   |    X    |    X    |    X    |         |  */
/* Is Event To Be Sent In Case Of Sequence Or Profile Error          |    X    |    X    |         |         |  */
/* Is Event To Be Sent In Case Of StartCode Picture SyntaxError      |    X    |    X    |         |         |  */
/* Is Spoiled Picture To Be Displayed                                |    X    |    X    |    X    |         |  */
/* Is Sequence Lost In Case Of Dummy Picture StartCode               |         |    X    |    X    |    X    |  */
/* Is Spurious Slice StartCode Causing Picture SyntaxError           |         |         |    X    |    X    |  */
/* Is Sequence Lost In Case Of Sequence Or Profile Error             |         |         |    X    |    X    |  */
/* Is Picture To Be Skipped In Case Of StartCode Picture SyntaxError |         |         |    X    |    X    |  */
/* Is Illegal StartCode Error Causing Picture SyntaxError            |         |         |    X    |    X    |  */
/* Is Display Field Counter Taken Into Account                       |         |         |    X    |    X    |  */
/* Is TimeOut On StartCode Search Taken Into Account                 |         |         |    X    |    X    |  */
/* Is Spoiled Reference Loosing All References Until Next I          |         |         |         |    X    |  */
/* ===========================================================================================================  */

/*#define IsSpuriousSliceStartCodeCausingPictureSyntaxError in vid_sc.c*/
#define IsSpoiledPictureToBeDisplayed(ErrorRecoveryMode) ((ErrorRecoveryMode) != STVID_ERROR_RECOVERY_FULL)
#define IsPictureDecodeHardwareSyntaxErrorToBeCancelled(ErrorRecoveryMode) (((ErrorRecoveryMode) != STVID_ERROR_RECOVERY_FULL) && ((ErrorRecoveryMode) != STVID_ERROR_RECOVERY_HIGH))
#define IsHardwareMisalignmentErrorToBeCancelled(ErrorRecoveryMode) ((ErrorRecoveryMode) == STVID_ERROR_RECOVERY_NONE)
#define IsSpoiledReferenceLoosingAllReferencesUntilNextI(ErrorRecoveryMode) ((ErrorRecoveryMode) == STVID_ERROR_RECOVERY_FULL)
#define IsBPictureToBeDecodedIfMissingReference(ErrorRecoveryMode) ((ErrorRecoveryMode) == STVID_ERROR_RECOVERY_NONE)
#define IsPPictureToBeDecodedIfMissingReference(ErrorRecoveryMode) ((ErrorRecoveryMode) != STVID_ERROR_RECOVERY_FULL)
#define IsSequenceLostInCaseOfDummyPictureStartCode(ErrorRecoveryMode) ((ErrorRecoveryMode) != STVID_ERROR_RECOVERY_NONE)
#define IsSequenceLostInCaseOfSequenceOrProfileError(ErrorRecoveryMode) (((ErrorRecoveryMode) == STVID_ERROR_RECOVERY_FULL) || ((ErrorRecoveryMode) == STVID_ERROR_RECOVERY_HIGH))
#define IsPictureToBeSkippedInCaseOfStartCodePictureSyntaxError(ErrorRecoveryMode) (((ErrorRecoveryMode) == STVID_ERROR_RECOVERY_FULL) || ((ErrorRecoveryMode) == STVID_ERROR_RECOVERY_HIGH))
#define IsEventToBeSentInCaseOfSequenceOrProfileError(ErrorRecoveryMode) (((ErrorRecoveryMode) != STVID_ERROR_RECOVERY_FULL) && ((ErrorRecoveryMode) != STVID_ERROR_RECOVERY_HIGH))
#define IsEventToBeSentInCaseOfStartCodePictureSyntaxError(ErrorRecoveryMode) (((ErrorRecoveryMode) != STVID_ERROR_RECOVERY_FULL) && ((ErrorRecoveryMode) != STVID_ERROR_RECOVERY_HIGH))
#define IsIllegalStartCodeErrorCausingPictureSyntaxError(ErrorRecoveryMode) (((ErrorRecoveryMode) == STVID_ERROR_RECOVERY_FULL) || ((ErrorRecoveryMode) == STVID_ERROR_RECOVERY_HIGH))
#define IsDisplayFieldCounterTakenIntoAccount(ErrorRecoveryMode) (((ErrorRecoveryMode) == STVID_ERROR_RECOVERY_FULL) || ((ErrorRecoveryMode) == STVID_ERROR_RECOVERY_HIGH))
#define IsTimeOutOnStartCodeSearchTakenIntoAccount(ErrorRecoveryMode) (((ErrorRecoveryMode) == STVID_ERROR_RECOVERY_FULL) || ((ErrorRecoveryMode) == STVID_ERROR_RECOVERY_HIGH))

/* Private Function prototypes ---------------------------------------------- */
static void CheckAndProcessInterrupt(const VIDDEC_Handle_t DecodeHandle);
static void ClearAllPendingInterrupt(const VIDDEC_Handle_t DecodeHandle);
static void ClearAllPendingStartCodes(const VIDDEC_Handle_t DecodeHandle);
static void ClearAllPendingControllerCommands(const VIDDEC_Handle_t DecodeHandle);
static void DecodePicture(const VIDDEC_Handle_t DecodeHandle, VIDDEC_DecodePictureParams_t * const DecodeParams_p, BOOL * const OverWrittenDisplay_p);
static ST_ErrorCode_t GetBuffers(VIDDEC_Data_t * VIDDEC_Data_p,
                                VIDBUFF_PictureBuffer_t ** DecodedPicture_p_p,
                                VIDBUFF_PictureBuffer_t ** DecodedDecimPicture_p_p );
static ST_ErrorCode_t VIDDEC_SetAsLastReference(const VIDDEC_Handle_t DecodeHandle, VIDBUFF_PictureBuffer_t * const Picture_p);
static ST_ErrorCode_t VIDDEC_ReleaseLastReference(const VIDDEC_Handle_t DecodeHandle);
static  ST_ErrorCode_t VIDDEC_ReleaseLastButOneReference(const VIDDEC_Handle_t DecodeHandle);
static  ST_ErrorCode_t VIDDEC_ReleaseLastDecodedPicture(const VIDDEC_Handle_t DecodeHandle);
#ifdef ST_smoothbackward
static ST_ErrorCode_t GetStartCodeAddress(const VIDDEC_Handle_t DecodeHandle,
        void ** const BufferAddress_p);

static void PrepareDecode(const VIDDEC_Handle_t DecodeHandle, VIDDEC_DecodePictureParams_t * const DecodeParams_p);
#endif /* ST_smoothbackward */
static void DecodeTaskFunc(const VIDDEC_Handle_t DecodeHandle);
static void EvaluateBitRateValue(const VIDDEC_Handle_t DecodeHandleDecodeHandle, STVID_PictureInfos_t * const PictureInfos_p);
#ifdef ST_smoothbackward
static void DecodeTaskFuncForManualControlMode(const VIDDEC_Handle_t DecodeHandle, const BOOL IsThereOrders);
#endif /* ST_smoothbackward */
static void DecodeTaskFuncForAutomaticControlMode(const VIDDEC_Handle_t DecodeHandle, const BOOL IsThereOrders);
static void DoStopNow(const VIDDEC_Handle_t DecodeHandle, const BOOL SendStopEvent);
static ST_ErrorCode_t ExtractBitRateValueFromStream(const VIDDEC_Handle_t DecodeHandle, U32 * const BitRateValue_p);
static void InitDecodeStructure(const VIDDEC_Handle_t DecodeHandle);
static void InitUnderflowEvent(const VIDDEC_Handle_t DecodeHandle);
static void InitStream(MPEG2BitStream_t * const Stream_p,
                       void * const UserDataBuffer_p, const U32 UserDataBufferSize);
static BOOL MonitorAbnormallyMissingHALStatus(const  VIDDEC_Handle_t DecodeHandle);
static BOOL MonitorBitBufferLevel(const VIDDEC_Handle_t DecodeHandle);
static void NewPictureBufferAvailable(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
static void SeekBeginningOfStream(const VIDDEC_Handle_t DecodeHandle);
#if defined STVID_STVAPI_ARCHITECTURE || !defined ST_OSLINUX
static STOS_INTERRUPT_DECLARE(SelfInstalledVideoInterruptHandler, Param);
#endif /* STVID_STVAPI_ARCHITECTURE | !LINUX */
static void SkipPicture(const VIDDEC_Handle_t DecodeHandle);
static ST_ErrorCode_t StartDecodeTask(const VIDDEC_Handle_t DecodeHandle);
static ST_ErrorCode_t StopDecodeTask(const VIDDEC_Handle_t DecodeHandle);
static void TermDecodeStructure(const VIDDEC_Handle_t DecodeHandle);

#ifdef DECODE_DEBUG
static void DecodeError(void);
#endif /* DECODE_DEBUG */

static void viddec_FillPictureBuffer(VIDBUFF_PictureBuffer_t *PB_p, VIDDEC_Data_t *viddec_p);

#ifdef ST_ordqueue
static void MPEGSpecific_CheckPictureToSendToDisplay(const VIDDEC_Handle_t DecodeHandle);
#endif  /* end of def ST_ordqueue */


static ST_ErrorCode_t VIDDEC_ConfigureEvent(const VIDDEC_Handle_t DecodeHandle, const STEVT_EventID_t Event, const STVID_ConfigureEventParams_t * const Params_p);
static ST_ErrorCode_t VIDDEC_DataInjectionCompleted(const VIDDEC_Handle_t DecodeHandle, const STVID_DataInjectionCompletedParams_t * const Params_p);
static ST_ErrorCode_t VIDDEC_FlushInjection(const VIDDEC_Handle_t DecodeHandle);
static ST_ErrorCode_t VIDDEC_Freeze(const VIDDEC_Handle_t DecodeHandle);
static ST_ErrorCode_t VIDDEC_GetBitBufferFreeSize(const VIDDEC_Handle_t DecodeHandle, U32 * const FreeSize_p);
static ST_ErrorCode_t VIDDEC_GetDecodedPictures(const VIDDEC_Handle_t DecodeHandle, STVID_DecodedPictures_t * const DecodedPictures_p);
static ST_ErrorCode_t VIDDEC_GetDecoderSpecialOperatingMode (const VIDDEC_Handle_t DecodeHandle, VIDDEC_DecoderOperatingMode_t * const DecoderOperatingMode_p);
static ST_ErrorCode_t VIDDEC_GetErrorRecoveryMode(const VIDDEC_Handle_t DecodeHandle, STVID_ErrorRecoveryMode_t * const Mode_p);
static ST_ErrorCode_t VIDDEC_GetHDPIPParams(const VIDDEC_Handle_t DecodeHandle, STVID_HDPIPParams_t * const HDPIPParams_p);
static ST_ErrorCode_t VIDDEC_GetLastDecodedPictureInfos(const VIDDEC_Handle_t DecodeHandle, STVID_PictureInfos_t * const PictureInfos_p);
static ST_ErrorCode_t VIDDEC_GetInfos(const VIDDEC_Handle_t DecodeHandle, VIDDEC_Infos_t * const Infos_p);
#ifdef STVID_DEBUG_GET_STATISTICS
static ST_ErrorCode_t VIDDEC_GetStatistics(const VIDDEC_Handle_t DecodeHandle, STVID_Statistics_t * const Statistics_p);
static ST_ErrorCode_t VIDDEC_ResetStatistics(const VIDDEC_Handle_t DecodeHandle, const STVID_Statistics_t * const Statistics_p);
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
static ST_ErrorCode_t VIDDEC_GetStatus(const VIDDEC_Handle_t DecodeHandle, STVID_Status_t * const Status_p);
#endif /* STVID_DEBUG_GET_STATUS */
static ST_ErrorCode_t VIDDEC_Resume(const VIDDEC_Handle_t DecodeHandle);
static ST_ErrorCode_t VIDDEC_SetDecodedPictures(const VIDDEC_Handle_t DecodeHandle, const STVID_DecodedPictures_t DecodedPictures);
static ST_ErrorCode_t VIDDEC_SetErrorRecoveryMode(const VIDDEC_Handle_t DecodeHandle, const STVID_ErrorRecoveryMode_t ErrorRecoveryMode);
static ST_ErrorCode_t VIDDEC_SetHDPIPParams(const VIDDEC_Handle_t DecodeHandle, const STVID_HDPIPParams_t * const HDPIPParams_p);
static ST_ErrorCode_t VIDDEC_Setup(const VIDDEC_Handle_t DecodeHandle, const STVID_SetupParams_t * const SetupParams_p);
static ST_ErrorCode_t VIDDEC_SkipData(const VIDDEC_Handle_t DecodeHandle, const VIDDEC_SkipMode_t Mode);
static ST_ErrorCode_t VIDDEC_Start(const VIDDEC_Handle_t DecodeHandle, const VIDDEC_StartParams_t * const StartParams_p);
static ST_ErrorCode_t VIDDEC_StartUpdatingDisplay(const VIDDEC_Handle_t DecodeHandle);

#ifdef STVID_STVAPI_ARCHITECTURE
static void VIDDEC_SetExternalRasterBufferManagerPoolId(const VIDDEC_Handle_t DecodeHandle, const DVRBM_BufferPoolId_t BufferPoolId);
#endif /* end of def STVID_STVAPI_ARCHITECTURE */

#ifdef ST_deblock
static void (VIDDEC_SetDeblockingMode)(const VIDDEC_Handle_t DecodeHandle, const BOOL EnableDeblocking);
#ifdef VIDEO_DEBLOCK_DEBUG
static void (VIDDEC_SetDeblockingStrength)(const VIDDEC_Handle_t DecodeHandle, const int DeblockingStrength);
#endif /* VIDEO_DEBLOCK_DEBUG */
#endif /* ST_deblock */

/* extension for injection controle */
static ST_ErrorCode_t VIDDEC_SetDataInputInterface(const VIDDEC_Handle_t DecodeHandle,
     ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle, void ** const Address_p),
     void (*InformReadAddress)(void * const Handle, void * const Address),
     void * const Handle ); /* Handle to pass to the 2 function calls */
static ST_ErrorCode_t VIDDEC_DeleteDataInputInterface(const VIDDEC_Handle_t DecodeHandle);
static ST_ErrorCode_t VIDDEC_GetDataInputBufferParams(const VIDDEC_Handle_t DecodeHandle,
                                void ** const BaseAddress_p,
                                U32 * const Size_p);
static ST_ErrorCode_t VIDDEC_GetBitBufferParams(const VIDDEC_Handle_t DecodeHandle,  void ** const Address_p,  U32 * const Size_p);

/* Public Functions --------------------------------------------------------- */
#ifdef ST_multicodec
ST_ErrorCode_t VIDDEC_Init(const VIDDEC_InitParams_t * const InitParams_p, VIDDEC_Handle_t * const DecodeHandle_p);
ST_ErrorCode_t VIDDEC_Term(const VIDDEC_Handle_t DecodeHandle);
#endif

/* Global Variables --------------------------------------------------------- */

const VIDPROD_FunctionsTable_t   VIDDEC_Functions =
{
    VIDDEC_ConfigureEvent,
    VIDDEC_DataInjectionCompleted,
    VIDDEC_FlushInjection,
    VIDDEC_Freeze,
    VIDDEC_GetBitBufferFreeSize,
    VIDDEC_GetDecodedPictures,
    VIDDEC_GetDecoderSpecialOperatingMode,
    VIDDEC_GetErrorRecoveryMode,
    VIDDEC_GetHDPIPParams,
    VIDDEC_GetLastDecodedPictureInfos,
    VIDDEC_GetInfos,
#ifdef STVID_DEBUG_GET_STATISTICS
    VIDDEC_GetStatistics,
    VIDDEC_ResetStatistics,
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
    VIDDEC_GetStatus,
#endif /* STVID_DEBUG_GET_STATUS */
    VIDDEC_Init,
    VIDDEC_NewReferencePicture,
    VIDDEC_NotifyDecodeEvent,
    VIDDEC_Resume,
    VIDDEC_SetDecodedPictures,
    VIDDEC_SetErrorRecoveryMode,
    VIDDEC_SetHDPIPParams,
    VIDDEC_Setup,
    VIDDEC_SkipData,
    VIDDEC_Start,
    VIDDEC_StartUpdatingDisplay,
    VIDDEC_Stop,
    VIDDEC_Term,
#ifdef STVID_STVAPI_ARCHITECTURE
    VIDDEC_SetExternalRasterBufferManagerPoolId,
#endif /* end of def STVID_STVAPI_ARCHITECTURE */
#ifdef ST_deblock
    VIDDEC_SetDeblockingMode,
#ifdef VIDEO_DEBLOCK_DEBUG
    VIDDEC_SetDeblockingStrength,
#endif /* VIDEO_DEBLOCK_DEBUG */
#endif /* ST_deblock */
    /* extension for injection controle */
    VIDDEC_SetDataInputInterface,
    VIDDEC_DeleteDataInputInterface,
    VIDDEC_GetDataInputBufferParams,
    VIDDEC_GetBitBufferParams
};


/* Public Functions --------------------------------------------------------- */


#ifdef ST_smoothbackward
/*******************************************************************************
Name        : VIDDEC_AskToSearchNextStartCode
Description : Search start code from a given address
Parameters  : Video decode handle, search mode, TRUE to detect first slice,
              search address for mode "from address"
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t VIDDEC_AskToSearchNextStartCode(const VIDDEC_Handle_t DecodeHandle, const VIDDEC_StartCodeSearchMode_t SearchMode, const BOOL FirstSliceDetect, void * const SearchAddress_p)
{
    VIDDEC_Data_t  * VIDDEC_Data_p;

    VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

#ifdef TRACE_DECODE
    /*TraceBuffer(("-AskTOSearchNextStartCode function \r\n"));*/
#endif /* TRACE_DECODE */

    VIDDEC_Data_p->RequiredSearch.SearchMode = SearchMode;
    VIDDEC_Data_p->RequiredSearch.FirstSliceDetect = FirstSliceDetect;
    VIDDEC_Data_p->RequiredSearch.SearchAddress_p = SearchAddress_p;
    /* Do that as latest, as this flag is tested in the decode task in
    order to process the order: if done before setting variables above,
    action may start with parameters not set ! */
    VIDDEC_Data_p->RequiredSearch.StartCodeDetectorToLaunch = TRUE;

    /* Signal controller that a new command is given */
    semaphore_signal(VIDDEC_Data_p->DecodeOrder_p);

    return (ST_NO_ERROR);
} /* VIDDEC_AskToSearchNextStartCode() function */
#endif /* ST_smoothbackward */


/*******************************************************************************
Name        : VIDDEC_ConfigureEvent
Description : Configures notification of video decoder events.
Parameters  : Video decode handle
              Event to allow or forbid.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if done successfully
              ST_ERROR_BAD_PARAMETER if the event is not supported.
*******************************************************************************/
ST_ErrorCode_t VIDDEC_ConfigureEvent(const VIDDEC_Handle_t DecodeHandle, const STEVT_EventID_t Event,
        const STVID_ConfigureEventParams_t * const Params_p)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;

    switch (Event)
    {
        case STVID_NEW_PICTURE_DECODED_EVT :
            VIDDEC_Data_p->EventNotificationConfiguration[VIDDEC_NEW_PICTURE_DECODED_EVT_ID].NotificationSkipped = 0;
            VIDDEC_Data_p->EventNotificationConfiguration[VIDDEC_NEW_PICTURE_DECODED_EVT_ID].ConfigureEventParam = (*Params_p);
            break;

        default :
            /* Error case. This event can't be configured. */
            ErrCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    /* Return the configuration's result. */
    return(ErrCode);

} /* End of VIDDEC_ConfigureEvent() function */

/*******************************************************************************
Name        : VIDDEC_AbortBitRateEvaluating
Description : Abort bit rate evaluating
Parameters  : Video decode handle
Assumptions : Has to be called by trick mode when requiring jumps on HDD.
            The entering data are no more contiguous and are
              not conveninent any more
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t VIDDEC_AbortBitRateEvaluating(const VIDDEC_Handle_t DecodeHandle)
{
    /* If an evaluating was under process, Disable it */
    ((VIDDEC_Data_t *)DecodeHandle)->BitRateValueEvaluate.BitRateValueIsUnderComputing = FALSE;

    return(ST_NO_ERROR);
} /* End of VIDDEC_AbortBitRateEvaluating() function */

/*******************************************************************************
Name        : VIDDEC_DeleteDataInputInterface
Description : cascaded to hal decode
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDDEC_DeleteDataInputInterface(
                                    const VIDDEC_Handle_t DecodeHandle)
{

    ST_ErrorCode_t ErrorCode;

    ErrorCode = HALDEC_SetDataInputInterface(
                            ((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle,
                            NULL,NULL,0 );
    return(ErrorCode);
}

/*******************************************************************************
Name        : VIDDEC_DisableBitRateEvaluating
Description : Disables the bit rate evaluating
Parameters  : Video decode handle
Assumptions : Has to be called by trick mode when requiring jumps on HDD. The entering data are no more contiguous and are
              not conveninent any more
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t VIDDEC_DisableBitRateEvaluating(const VIDDEC_Handle_t DecodeHandle)
{
    /* Bit rate Value evaluating is not possible */
    ((VIDDEC_Data_t *)DecodeHandle)->BitRateValueEvaluate.BitRateValueIsComputable = FALSE;

    /* If an evaluating was under process, Disable it */
    ((VIDDEC_Data_t *)DecodeHandle)->BitRateValueEvaluate.BitRateValueIsUnderComputing = FALSE;

    return(ST_NO_ERROR);
} /* End of VIDDEC_DisableBitRateEvaluating() function */

/*******************************************************************************
Name        : VIDDEC_EnableBitRateEvaluating
Description : Enables the bit rate evaluating
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t VIDDEC_EnableBitRateEvaluating(const VIDDEC_Handle_t DecodeHandle)
{
    /* Bit rate Value evaluating is now possible */
    ((VIDDEC_Data_t *)DecodeHandle)->BitRateValueEvaluate.BitRateValueIsComputable = TRUE;
    return(ST_NO_ERROR);
}

#ifdef ST_smoothbackward
/*******************************************************************************
Name        : VIDDEC_DecodePicture
Description : Set infos so that DecodePicture() is called later
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t VIDDEC_DecodePicture(const VIDDEC_Handle_t DecodeHandle, VIDDEC_DecodePictureParams_t * const DecodeParams_p)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    /* First of all, set the VLD. When the VLD will be ready, the hardware will give an ACK throught the TR_OK IT. */
    /* Only from there we will launch the decode. */
    VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible = FALSE ;
    VIDDEC_Data_p->ForTask.TryingToRecoverFromMaxDecodeTimeOverflow = FALSE;

    /* Monitor decode time: first monitor time before decoder effectively starts decode.
    Time-out occuring from now means that the hardware decoder is never scheduled to do our decode ! */
    VIDDEC_Data_p->HWDecoder.MaxDecodeOrSkipTime = time_plus(time_now(), MAX_TIME_BEFORE_DECODE_START);

    /* The state of the decode from a TM point of view is DECODING. */
    VIDDEC_Data_p->HWDecoder.HWState = VIDDEC_HARDWARE_STATE_DECODING;

    /* Set the Hardware. */
    VIDDEC_Data_p->DecodeParams = *DecodeParams_p;
    PrepareDecode(DecodeHandle, &VIDDEC_Data_p->DecodeParams);

    return (ST_NO_ERROR);
} /* End of VIDDEC_DecodePicture() function */
#endif /* ST_smoothbackward */

/*******************************************************************************
Name        : VIDDEC_DataInjectionCompleted
Description : Inform decoder that data injection is completed
Parameters  : Video decode handle, params
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t VIDDEC_DataInjectionCompleted(const VIDDEC_Handle_t DecodeHandle, const STVID_DataInjectionCompletedParams_t * const Params_p)
{

    /* Acknowledge data injection completed even before checking parameters */
    ((VIDDEC_Data_t *) DecodeHandle)->DataUnderflowEventNotSatisfied = FALSE;

    if (Params_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    return(ST_NO_ERROR);
} /* End of VIDDEC_DataInjectionCompleted() function */



#ifdef ST_smoothbackward
/*******************************************************************************
Name        : VIDDEC_DiscardStartCode
Description : Tells if the start code found must be computed or not
Parameters  : Video decode handle,
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t VIDDEC_DiscardStartCode(const VIDDEC_Handle_t DecodeHandle, const BOOL DiscardStartCode)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    VIDDEC_Data_p->StartCodeParsing.DiscardStartCode = DiscardStartCode;

    return (ST_NO_ERROR);
} /* End of VIDDEC_DiscardStartCode() function */
#endif /* ST_smoothbackward */


/*******************************************************************************
Name        : VIDDEC_FlushInjection
Description : Flush decoder injected data
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t VIDDEC_FlushInjection(const VIDDEC_Handle_t DecodeHandle)
{
    BOOL TmpBool;

    HALDEC_FlushDecoder(((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle, &TmpBool);

    return(ST_NO_ERROR);
} /* End of VIDDEC_FlushInjection() function */


/*******************************************************************************
Name        : VIDDEC_Freeze
Description : Informs that Decoder is Freezed.
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t VIDDEC_Freeze(const VIDDEC_Handle_t DecodeHandle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VIDCOM_InternalProfile_t Profile;
    STVID_DecodedPictures_t CurrentDecodedPictures;
    STVID_DecodedPictures_t DecodedPicturesToApply;

#ifdef TRACE_DECODE
    TraceBuffer(("-FREEZE"));
#endif /* TRACE_DECODE */

    /* inits */
    CurrentDecodedPictures = ((VIDDEC_Data_t *) DecodeHandle)->DecodedPictures;
    DecodedPicturesToApply = CurrentDecodedPictures;

    ErrorCode = VIDBUFF_GetMemoryProfile(((VIDDEC_Data_t *) DecodeHandle)->BufferManagerHandle, &Profile);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    switch (CurrentDecodedPictures)
    {
        case STVID_DECODED_PICTURES_I :
            if (Profile.NbMainFrameStore == 1)
            {
                return (ST_ERROR_NO_MEMORY);
            }
            break;

        case STVID_DECODED_PICTURES_IP :
            if (Profile.NbMainFrameStore == 2)  /* can not be less than 2 */
            {
                DecodedPicturesToApply = STVID_DECODED_PICTURES_I;
            }
            break;

        case STVID_DECODED_PICTURES_ALL :
            if (Profile.NbMainFrameStore == 3)  /* can not be less than 3 */
            {
                DecodedPicturesToApply = STVID_DECODED_PICTURES_IP;
            }
            break;

        default :
            /* Unrecognised mode: don't take it into account ! */
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        if (DecodedPicturesToApply != CurrentDecodedPictures)
        {
            ErrorCode = VIDDEC_SetDecodedPictures(DecodeHandle, DecodedPicturesToApply);
        }
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        /* remember decoded pictures state for next resume */
        ((VIDDEC_Data_t *) DecodeHandle)->DecodedPicturesBeforeLastFreeze = CurrentDecodedPictures;

        /* Freezing */
        ((VIDDEC_Data_t *) DecodeHandle)->DecoderIsFreezed = TRUE;

        /* Cancel DecodeOnce if no more running */
        ((VIDDEC_Data_t *) DecodeHandle)->DecodeOnce = FALSE;
    }

    return (ErrorCode);
} /* End of VIDDEC_Freeze() function */


/*******************************************************************************
Name        : VIDDEC_GetBitBufferFreeSize
Description : Get the bit buffer free size.
Parameters  : Video decode handle, pointer on U32 to fill
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if bad parameter
              Caution : no other error allowed ! API STVID_GetBitBufferFreeSize() depends on that.
*******************************************************************************/
ST_ErrorCode_t VIDDEC_GetBitBufferFreeSize(const VIDDEC_Handle_t DecodeHandle, U32 * const FreeSize_p)
{
    /* Overall variables of the task */
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
    U32 BitBufferLevel;

    if (FreeSize_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (VIDDEC_Data_p->DecoderState != VIDDEC_DECODER_STATE_STOPPED)
    {
        /* If decoder not stopped, tell bit buffer occupancy according to hardware */
        BitBufferLevel = HALDEC_GetDecodeBitBufferLevel(VIDDEC_Data_p->HALDecodeHandle);
    }
    else
    {
        /* If decoder stopped, suppose bit buffer is empty as next Start() will empty it. */
        BitBufferLevel = 0;
    }

#ifdef ST_smoothbackward
    if (VIDDEC_Data_p->ControlMode != VIDDEC_AUTOMATIC_CONTROL_MODE)
    {
        if (!VIDDEC_Data_p->DataUnderflowEventNotSatisfied)
        {
            *FreeSize_p = 0;
        }
        else
        {
            *FreeSize_p = VIDDEC_Data_p->LastLinearBitBufferFlushRequestSize - BitBufferLevel;
        }
    }
    else
#endif /* ST_smoothbackward */
    {
        *FreeSize_p = VIDDEC_Data_p->BitBuffer.TotalSize - BitBufferLevel;
    }


    return(ST_NO_ERROR);

} /* End of VIDDEC_GetBitBufferFreeSize() function */
/*******************************************************************************
Name        : VIDDEC_GetBitRateValue
Description : Get bit rate value of the decoded stream in units of 400 bits/second.
              Compares the evaluated value with the value extracted from the stream. If there is no cal
Parameters  : Video decode handle, pointer on U32 to fill
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if bad parameter
              STVID_ERROR_NOT_AVAILABLE if can not give this information
*******************************************************************************/
ST_ErrorCode_t VIDDEC_GetBitRateValue(const VIDDEC_Handle_t DecodeHandle, U32 * const BitRateValue_p)
{
    U32 ExtractedBitRateValue;
    U32 CalculatedBitRateValue = 0;
    U32 ComparisonRatio;
    U8 i;
    BitRateValueEvaluate_t * BitRate_p;
    U32 BitBufferSize,Time;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
#ifdef STVID_DEBUG_GET_STATISTICS
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
#endif

    if (BitRateValue_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Find structures */
    BitRate_p = &(((VIDDEC_Data_t *) DecodeHandle)->BitRateValueEvaluate);

    /* Get Extracted value from the stream */
    ErrorCode = ExtractBitRateValueFromStream(DecodeHandle, &ExtractedBitRateValue);

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
            if (BitBufferSize > (((U32)ULONG_MAX) / VIDDEC_TIME_FACTOR))
            {
                if ((BitBufferSize / VIDDEC_SIZE_UNIT_FACTOR) > (((U32)ULONG_MAX) / VIDDEC_TIME_FACTOR))
                {
                    CalculatedBitRateValue = ((BitBufferSize / VIDDEC_SIZE_UNIT_FACTOR) / Time) * VIDDEC_TIME_FACTOR;
                }
                else
                {
                   CalculatedBitRateValue = ((BitBufferSize / VIDDEC_SIZE_UNIT_FACTOR) * VIDDEC_TIME_FACTOR) / Time;
                }
            }
            else
            {
                CalculatedBitRateValue = ((BitBufferSize * VIDDEC_TIME_FACTOR) / VIDDEC_SIZE_UNIT_FACTOR) / Time;
            }
        }

        if ((ErrorCode == ST_NO_ERROR) && (ExtractedBitRateValue > 0))
        {
            /* Compare both values : */
            /* If the calculated value is very close to the extracted value, we prefer to keep the extracted value.       */
            /* The extracted value is the value parsed in the stream. We consider the values as close when the difference */
            /* between both is less than 20%.                                                                             */
            ComparisonRatio =  CalculatedBitRateValue * 100 / ExtractedBitRateValue;
            if ((ComparisonRatio < 120) && (ComparisonRatio > 80)) /* 20% of difference. */
            {
                /* Extracted Value is the good value */
                *BitRateValue_p =  ExtractedBitRateValue;
            }
            else
            {
                /* if both values don't match, return calculated value */
                *BitRateValue_p =  CalculatedBitRateValue;
            }
        }
        else
        {
            /* If There is not available extracted value, return calculated value */
            *BitRateValue_p =  CalculatedBitRateValue;
        }
    }
    else
    {
        /* If there is not calculated value, extracted value CAN NOT be returned. */
        return (STVID_ERROR_NOT_AVAILABLE);
    }
#ifdef STVID_DEBUG_GET_STATISTICS
    VIDDEC_Data_p->Statistics.LastBitRate = *BitRateValue_p;
    if ( *BitRateValue_p > VIDDEC_Data_p->Statistics.MaxBitRate )
    {
        VIDDEC_Data_p->Statistics.MaxBitRate =  *BitRateValue_p;
    }
    if ( *BitRateValue_p < VIDDEC_Data_p->Statistics.MinBitRate )
    {
        VIDDEC_Data_p->Statistics.MinBitRate =  *BitRateValue_p;
    }
#endif /*STVID_DEBUG_GET_STATISTICS*/

/* #ifdef TRACE_DECODE */
    /* TraceBuffer(("Cal BitRate = %d\r\n", CalculatedBitRateValue * 400)); */
    /* TraceBuffer(("Ext BitRate = %d\r\n", ExtractedBitRateValue * 400)); */
/* #endif */

    return(ST_NO_ERROR);

} /* End of VIDDEC_GetBitRateValue() function */

#ifdef ST_smoothbackward
/*******************************************************************************
Name        : VIDDEC_GetCDFifoAlignmentInBytes
Description :
Parameters  : Video decode handle, pointer on U32 to be filled
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if problem
******************************************************************************/
ST_ErrorCode_t VIDDEC_GetCDFifoAlignmentInBytes(const VIDDEC_Handle_t DecodeHandle, U32 * const CDFifoAlignment_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (CDFifoAlignment_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Call the HAL function */
    ErrorCode = HALDEC_GetCDFifoAlignmentInBytes(((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle, CDFifoAlignment_p);

    return(ErrorCode);
} /* End of VIDDEC_GetCDFifoAlignmentInBytes() function */
#endif /* ST_smoothbackward */


#ifdef ST_smoothbackward
/*******************************************************************************
Name        : VIDDEC_GetControlMode
Description : Returns the current control mode.
Parameters  : Video decode handle, pointer on a boolean to fill
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if problem
******************************************************************************/
ST_ErrorCode_t VIDDEC_GetControlMode(const VIDDEC_Handle_t DecodeHandle, VIDDEC_ControlMode_t * const ControlMode_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (ControlMode_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Fill with the store value */
    *ControlMode_p = ((VIDDEC_Data_t *) DecodeHandle)->ControlMode;

    return(ErrorCode);
} /* End of VIDDEC_GetControlMode() function */
#endif /* ST_smoothbackward */


/*******************************************************************************
Name        : VIDDEC_GetDataInputBufferParams
Description : cascaded to hal decode
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDDEC_GetDataInputBufferParams(
                                const VIDDEC_Handle_t DecodeHandle,
                                void ** const BaseAddress_p,
                                U32 * const Size_p)
{

    ST_ErrorCode_t ErrorCode;

    ErrorCode = HALDEC_GetDataInputBufferParams(
                            ((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle,
                               BaseAddress_p,Size_p);
    return(ErrorCode);
} /* end of VIDDEC_GetDataInputBufferParams() function */

/*******************************************************************************
Name        : VIDDEC_GetDecodedPictures
Description : Get the picture type of the decoded picture.
Parameters  : Video decode handle, pointer to fill.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDDEC_GetDecodedPictures(const VIDDEC_Handle_t DecodeHandle, STVID_DecodedPictures_t * const DecodedPictures_p)
{
    /* Get the error recovery mode. */
    if (DecodedPictures_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    *DecodedPictures_p = ((VIDDEC_Data_t *) DecodeHandle)->DecodedPictures;

    return(ST_NO_ERROR);
} /* End of VIDDEC_GetDecodedPictures() function */

/*******************************************************************************
Name        : VIDDEC_GetDecoderSpecialOperatingMode
Description : Get the decoder operating mode (low_delay flag, or or other future
              special cases).
Parameters  : Video decode handle, decoder operating mode.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDDEC_GetDecoderSpecialOperatingMode (const VIDDEC_Handle_t DecodeHandle, VIDDEC_DecoderOperatingMode_t * const DecoderOperatingMode_p)
{
    /* Get the error recovery mode. */
    if (DecoderOperatingMode_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    *DecoderOperatingMode_p = ((VIDDEC_Data_t *) DecodeHandle)->DecoderOperatingMode;

    return(ST_NO_ERROR);
} /* End of VIDDEC_GetDecoderSpecialOperatingMode() function */

/*******************************************************************************
Name        : VIDDEC_GetErrorRecoveryMode
Description : Get the error recovery mode from the API to the decoder.
Parameters  : Video decode handle, error recovery mode.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDDEC_GetErrorRecoveryMode(const VIDDEC_Handle_t DecodeHandle, STVID_ErrorRecoveryMode_t * const Mode_p)
{
    /* Get the error recovery mode. */
    if (Mode_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    *Mode_p = ((VIDDEC_Data_t *) DecodeHandle)->ErrorRecoveryMode;

    return(ST_NO_ERROR);
} /* End of VIDDEC_GetErrorRecoveryMode() function */

/*******************************************************************************
Name        : VIDDEC_GetHDPIPParams
Description : Get the current HD-PIP threshold.
Parameters  : Video decode handle, HD-PIP parameters to set.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDDEC_GetHDPIPParams(const VIDDEC_Handle_t DecodeHandle,
        STVID_HDPIPParams_t * const HDPIPParams_p)
{
    /* Exit now if parameters are bad */
    if (HDPIPParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    *HDPIPParams_p = ((VIDDEC_Data_t *) DecodeHandle)->HDPIPParams;

    return(ST_NO_ERROR);

} /* End of VIDDEC_GetHDPIPParams() function */



/*******************************************************************************
Name        : VIDDEC_GetLastDecodedPictureInfos
Description : Get infos of last decoded picture
Parameters  : Video decode handle, pointer on structure to fill
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if bad parameter
              STVID_ERROR_DECODER_RUNNING if decoder is not stopped: can't get info
              STVID_ERROR_NOT_AVAILABLE if no picture decoded
*******************************************************************************/
ST_ErrorCode_t VIDDEC_GetLastDecodedPictureInfos(const VIDDEC_Handle_t DecodeHandle, STVID_PictureInfos_t * const PictureInfos_p)
{
    if (PictureInfos_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (((VIDDEC_Data_t *) DecodeHandle)->DecoderState != VIDDEC_DECODER_STATE_STOPPED)
    {
        return(STVID_ERROR_DECODER_RUNNING);
    }

    if (((VIDDEC_Data_t *) DecodeHandle)->HWDecoder.LastDecodedPictureInfos_p == NULL)
    {
        return(STVID_ERROR_NOT_AVAILABLE);
    }

    *PictureInfos_p = *(((VIDDEC_Data_t *) DecodeHandle)->HWDecoder.LastDecodedPictureInfos_p);

    return(ST_NO_ERROR);
} /* End of VIDDEC_GetLastDecodedPictureInfos() function */


/*******************************************************************************
Name        : VIDDEC_GetInfos
Description : Get codec dependant infos
Parameters  : Video producer handle, pointer on structure to fill
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if bad parameter
              STVID_ERROR_DECODER_RUNNING if decoder is not stopped: can't get info
              STVID_ERROR_NOT_AVAILABLE if no picture decoded
*******************************************************************************/
ST_ErrorCode_t VIDDEC_GetInfos(const VIDDEC_Handle_t DecodeHandle, VIDDEC_Infos_t * const Infos_p)
{

    UNUSED_PARAMETER(DecodeHandle);

    if (Infos_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Infos_p->MaximumDecodingLatencyInSystemClockUnit = 0;
    Infos_p->FrameBufferAdditionalDataBytesPerMB = 0;

    return(ST_NO_ERROR);
} /* End of VIDDEC_GetInfos() function */

#ifdef ST_smoothbackward
/*******************************************************************************
Name        : VIDDEC_GetLinearBitBufferEndAddress
Description :
Parameters  : Video decode handle, pointer on U32 to be filled
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if problem
******************************************************************************/
ST_ErrorCode_t VIDDEC_GetLinearBitBufferTransferedDataSize(const VIDDEC_Handle_t DecodeHandle, U32 * const TransferedDataSize_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (TransferedDataSize_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Call the HAL function */
    ErrorCode = HALDEC_GetLinearBitBufferTransferedDataSize(((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle, TransferedDataSize_p);

    return(ErrorCode);
} /* End of VIDDEC_GetLinearBitBufferTransferedDataSize() function */
#endif /* ST_smoothbackward */

#ifdef ST_smoothbackward
/*******************************************************************************
Name        : VIDDEC_GetSCDAlignmentInBytes
Description :
Parameters  : Video decode handle, pointer on U32 to be filled
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if problem
******************************************************************************/
ST_ErrorCode_t VIDDEC_GetSCDAlignmentInBytes(const VIDDEC_Handle_t DecodeHandle, U32 * const SCDAlignment_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (SCDAlignment_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Call the HAL function */
    ErrorCode = HALDEC_GetSCDAlignmentInBytes(((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle, SCDAlignment_p);

    return(ErrorCode);
}
#endif /* ST_smoothbackward */

#ifdef ST_smoothbackward
/*******************************************************************************
Name        : GetStartCodeAddress
Description :
Parameters  : Video decode handle, pointers on void*
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if problem
******************************************************************************/
ST_ErrorCode_t GetStartCodeAddress(const VIDDEC_Handle_t DecodeHandle, void ** const BufferAddress_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (BufferAddress_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* If the startcode was found manually, the address returned by the hardware
     * is the address of the previous startcode (user data startcode). So we
     * need to compute the real address manually */
    if(((VIDDEC_Data_t *) DecodeHandle)->RequiredSearch.StartCodeWasFoundManually)
    {
        /* This function is only usefull with backward trickmodes, so no need to manage bitbuffer looping */
        *BufferAddress_p = (void*)((U32)(((VIDDEC_Data_t *) DecodeHandle)->RequiredSearch.StartCodeAddress_p) + (U32)((VIDDEC_Data_t *) DecodeHandle)->DecodingStream_p->UserData.UsedSize + 4);
    }
    else
    {
        /* Call the HAL function */
        ErrorCode = HALDEC_GetStartCodeAddress(((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle, BufferAddress_p);
    }

    /* Remember the startcode address */
    if(ErrorCode == ST_NO_ERROR)
    {
       ((VIDDEC_Data_t *) DecodeHandle)->RequiredSearch.StartCodeAddress_p = *BufferAddress_p;
    }

    return(ErrorCode);
} /* end of GetStartCodeAddress() function */
#endif /* ST_smoothbackward */


/*******************************************************************************
Name        : VIDDEC_GetSmoothBackwardCapability
Description : Tells whether the decoder is capable of smooth backward playback
Parameters  : Video decode handle,
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if capable
              ST_ERROR_FEATURE_NOT_SUPPORTED if not capable
*******************************************************************************/
ST_ErrorCode_t VIDDEC_GetSmoothBackwardCapability(const VIDDEC_Handle_t DecodeHandle)
{
    ST_ErrorCode_t ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
#ifdef ST_smoothbackward
    U32 SCDAlignment;
#endif /* ST_smoothbackward */

#ifdef ST_smoothbackward
    /* Call the HAL function */
    ErrorCode = HALDEC_GetSCDAlignmentInBytes(((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle, &SCDAlignment);
    if (ErrorCode != ST_NO_ERROR)
    {
        ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
    }
#endif /* ST_smoothbackward */

    return(ErrorCode);
} /* End of VIDDEC_GetSmoothBackwardCapability() function */


#ifdef STVID_DEBUG_GET_STATISTICS
/*******************************************************************************
Name        : VIDDEC_GetStatistics
Description : Gets the decode module statistics.
Parameters  : Video decode handle, statistics.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDDEC_GetStatistics(const VIDDEC_Handle_t DecodeHandle, STVID_Statistics_t * const Statistics_p)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    if (Statistics_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Statistics_p->DecodeHardwareSoftReset                  = VIDDEC_Data_p->Statistics.DecodeHardwareSoftReset;
    Statistics_p->DecodeStartCodeFound                     = VIDDEC_Data_p->Statistics.DecodeStartCodeFound;
    Statistics_p->DecodeSequenceFound                      = VIDDEC_Data_p->Statistics.DecodeSequenceFound;
    Statistics_p->DecodeUserDataFound                      = VIDDEC_Data_p->Statistics.DecodeUserDataFound;
    Statistics_p->DecodePictureFound                       = VIDDEC_Data_p->Statistics.DecodePictureFound;
    Statistics_p->DecodePictureFoundMPEGFrameI             = VIDDEC_Data_p->Statistics.DecodePictureFoundMPEGFrameI;
    Statistics_p->DecodePictureFoundMPEGFrameP             = VIDDEC_Data_p->Statistics.DecodePictureFoundMPEGFrameP;
    Statistics_p->DecodePictureFoundMPEGFrameB             = VIDDEC_Data_p->Statistics.DecodePictureFoundMPEGFrameB;
    Statistics_p->DecodePictureSkippedRequested            = VIDDEC_Data_p->Statistics.DecodePictureSkippedRequested;
    Statistics_p->DecodePictureSkippedNotRequested         = VIDDEC_Data_p->Statistics.DecodePictureSkippedNotRequested;
    Statistics_p->DecodePictureDecodeLaunched              = VIDDEC_Data_p->Statistics.DecodePictureDecodeLaunched;
    Statistics_p->DecodeStartConditionVbvDelay             = VIDDEC_Data_p->Statistics.DecodeStartConditionVbvDelay;
    Statistics_p->DecodeStartConditionPtsTimeComparison    = VIDDEC_Data_p->Statistics.DecodeStartConditionPtsTimeComparison;
    Statistics_p->DecodeStartConditionVbvBufferSize        = VIDDEC_Data_p->Statistics.DecodeStartConditionVbvBufferSize;
    Statistics_p->DecodeInterruptStartDecode               = VIDDEC_Data_p->Statistics.DecodeInterruptStartDecode;
    Statistics_p->DecodeInterruptPipelineIdle              = VIDDEC_Data_p->Statistics.DecodeInterruptPipelineIdle;
    Statistics_p->DecodeInterruptDecoderIdle               = VIDDEC_Data_p->Statistics.DecodeInterruptDecoderIdle;
    Statistics_p->DecodeInterruptBitBufferEmpty            = VIDDEC_Data_p->Statistics.DecodeInterruptBitBufferEmpty;
    Statistics_p->DecodeInterruptBitBufferFull             = VIDDEC_Data_p->Statistics.DecodeInterruptBitBufferFull;
    Statistics_p->DecodePbStartCodeFoundInvalid            = VIDDEC_Data_p->Statistics.DecodePbStartCodeFoundInvalid;
    Statistics_p->DecodePbStartCodeFoundVideoPES           = VIDDEC_Data_p->Statistics.DecodePbStartCodeFoundVideoPES;
    Statistics_p->DecodePbMaxNbInterruptSyntaxErrorPerPicture = VIDDEC_Data_p->Statistics.DecodePbMaxNbInterruptSyntaxErrorPerPicture;
    Statistics_p->DecodePbInterruptSyntaxError             = VIDDEC_Data_p->Statistics.DecodePbInterruptSyntaxError;
    Statistics_p->DecodePbInterruptDecodeOverflowError     = VIDDEC_Data_p->Statistics.DecodePbInterruptDecodeOverflowError;
    Statistics_p->DecodePbInterruptDecodeUnderflowError    = VIDDEC_Data_p->Statistics.DecodePbInterruptDecodeUnderflowError;
    Statistics_p->DecodePbInterruptMisalignmentError       = VIDDEC_Data_p->Statistics.DecodePbInterruptMisalignmentError;
    Statistics_p->DecodePbDecodeTimeOutError               = VIDDEC_Data_p->Statistics.DecodePbDecodeTimeOutError;
    Statistics_p->DecodePbInterruptQueueOverflow           = VIDDEC_Data_p->Statistics.DecodePbInterruptQueueOverflow;
    Statistics_p->DecodePbHeaderFifoEmpty                  = VIDDEC_Data_p->HeaderData.StatisticsDecodePbHeaderFifoEmpty;
    Statistics_p->DecodePbVbvSizeGreaterThanBitBuffer      = VIDDEC_Data_p->Statistics.DecodePbVbvSizeGreaterThanBitBuffer;
    Statistics_p->DecodeMinBitBufferLevelReached           = VIDDEC_Data_p->Statistics.DecodeMinBitBufferLevelReached;
    Statistics_p->DecodeMaxBitBufferLevelReached           = VIDDEC_Data_p->Statistics.DecodeMaxBitBufferLevelReached;
    Statistics_p->DecodePbSequenceNotInMemProfileSkipped   = VIDDEC_Data_p->Statistics.DecodePbSequenceNotInMemProfileSkipped;
    Statistics_p->MaxBitRate = VIDDEC_Data_p->Statistics.MaxBitRate;
    Statistics_p->MinBitRate = VIDDEC_Data_p->Statistics.MinBitRate;
    Statistics_p->LastBitRate = VIDDEC_Data_p->Statistics.LastBitRate;

#ifdef STVID_HARDWARE_ERROR_EVENT
    Statistics_p->DecodePbHardwareErrorMissingPID          = VIDDEC_Data_p->Statistics.DecodePbHardwareErrorMissingPID;
    Statistics_p->DecodePbHardwareErrorSyntaxError         = VIDDEC_Data_p->Statistics.DecodePbHardwareErrorSyntaxError;
    Statistics_p->DecodePbHardwareErrorTooSmallPicture     = VIDDEC_Data_p->Statistics.DecodePbHardwareErrorTooSmallPicture;
#endif /* STVID_HARDWARE_ERROR_EVENT */

    return(ST_NO_ERROR);
} /* End of VIDDEC_GetStatistics() function */

/*******************************************************************************
Name        : VIDDEC_ResetStatistics
Description : Resets the decode module statistics.
Parameters  : Video decode handle, statistics.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success, ST_ERROR_BAD_PARAMETER if Statistics_p == NULL
*******************************************************************************/
ST_ErrorCode_t VIDDEC_ResetStatistics(const VIDDEC_Handle_t DecodeHandle, const STVID_Statistics_t * const Statistics_p)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    if (Statistics_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Reset parameters that are said to be reset (giving value != 0) */
    if (Statistics_p->DecodeHardwareSoftReset != 0)
    {
        VIDDEC_Data_p->Statistics.DecodeHardwareSoftReset = 0;
    }
    if (Statistics_p->DecodeStartCodeFound != 0)
    {
        VIDDEC_Data_p->Statistics.DecodeStartCodeFound = 0;
    }
    if (Statistics_p->DecodeSequenceFound != 0)
    {
        VIDDEC_Data_p->Statistics.DecodeSequenceFound = 0;
    }
    if (Statistics_p->DecodeUserDataFound != 0)
    {
        VIDDEC_Data_p->Statistics.DecodeUserDataFound = 0;
    }
    if (Statistics_p->DecodePictureFound != 0)
    {
        VIDDEC_Data_p->Statistics.DecodePictureFound = 0;
    }
    if (Statistics_p->DecodePictureFoundMPEGFrameI != 0)
    {
        VIDDEC_Data_p->Statistics.DecodePictureFoundMPEGFrameI = 0;
    }
    if (Statistics_p->DecodePictureFoundMPEGFrameP != 0)
    {
        VIDDEC_Data_p->Statistics.DecodePictureFoundMPEGFrameP = 0;
    }
    if (Statistics_p->DecodePictureFoundMPEGFrameB != 0)
    {
        VIDDEC_Data_p->Statistics.DecodePictureFoundMPEGFrameB = 0;
    }
    if (Statistics_p->DecodePictureSkippedRequested != 0)
    {
        VIDDEC_Data_p->Statistics.DecodePictureSkippedRequested = 0;
    }
    if (Statistics_p->DecodePictureSkippedNotRequested != 0)
    {
        VIDDEC_Data_p->Statistics.DecodePictureSkippedNotRequested = 0;
    }
    if (Statistics_p->DecodePictureDecodeLaunched != 0)
    {
        VIDDEC_Data_p->Statistics.DecodePictureDecodeLaunched = 0;
    }
    if (Statistics_p->DecodeStartConditionVbvDelay != 0)
    {
        VIDDEC_Data_p->Statistics.DecodeStartConditionVbvDelay = 0;
    }
    if (Statistics_p->DecodeStartConditionPtsTimeComparison != 0)
    {
        VIDDEC_Data_p->Statistics.DecodeStartConditionPtsTimeComparison = 0;
    }
    if (Statistics_p->DecodeStartConditionVbvBufferSize != 0)
    {
        VIDDEC_Data_p->Statistics.DecodeStartConditionVbvBufferSize = 0;
    }
    if (Statistics_p->DecodeInterruptStartDecode != 0)
    {
        VIDDEC_Data_p->Statistics.DecodeInterruptStartDecode = 0;
    }
    if (Statistics_p->DecodeInterruptPipelineIdle != 0)
    {
        VIDDEC_Data_p->Statistics.DecodeInterruptPipelineIdle = 0;
    }
    if (Statistics_p->DecodeInterruptDecoderIdle != 0)
    {
        VIDDEC_Data_p->Statistics.DecodeInterruptDecoderIdle = 0;
    }
    if (Statistics_p->DecodeInterruptBitBufferEmpty != 0)
    {
        VIDDEC_Data_p->Statistics.DecodeInterruptBitBufferEmpty = 0;
    }
    if (Statistics_p->DecodeInterruptBitBufferFull != 0)
    {
        VIDDEC_Data_p->Statistics.DecodeInterruptBitBufferFull = 0;
    }
    if (Statistics_p->DecodePbStartCodeFoundInvalid != 0)
    {
        VIDDEC_Data_p->Statistics.DecodePbStartCodeFoundInvalid = 0;
    }
    if (Statistics_p->DecodePbStartCodeFoundVideoPES != 0)
    {
        VIDDEC_Data_p->Statistics.DecodePbStartCodeFoundVideoPES = 0;
    }
    if (Statistics_p->DecodePbMaxNbInterruptSyntaxErrorPerPicture != 0)
    {
        VIDDEC_Data_p->Statistics.DecodePbMaxNbInterruptSyntaxErrorPerPicture = 0;
    }
    if (Statistics_p->DecodePbInterruptSyntaxError != 0)
    {
        VIDDEC_Data_p->Statistics.DecodePbInterruptSyntaxError = 0;
    }
    if (Statistics_p->DecodePbInterruptDecodeOverflowError != 0)
    {
        VIDDEC_Data_p->Statistics.DecodePbInterruptDecodeOverflowError = 0;
    }
    if (Statistics_p->DecodePbInterruptDecodeUnderflowError != 0)
    {
        VIDDEC_Data_p->Statistics.DecodePbInterruptDecodeUnderflowError = 0;
    }
    if (Statistics_p->DecodePbDecodeTimeOutError != 0)
    {
        VIDDEC_Data_p->Statistics.DecodePbDecodeTimeOutError = 0;
    }
    if (Statistics_p->DecodePbInterruptMisalignmentError != 0)
    {
        VIDDEC_Data_p->Statistics.DecodePbInterruptMisalignmentError = 0;
    }
    if (Statistics_p->DecodePbInterruptQueueOverflow != 0)
    {
        VIDDEC_Data_p->Statistics.DecodePbInterruptQueueOverflow = 0;
        /* Not so nice because meaning of MaxUsedSize is modified, but OK: it is used for debug only... */
        VIDDEC_Data_p->InterruptsBuffer.MaxUsedSize = VIDDEC_Data_p->InterruptsBuffer.UsedSize;
    }
    if (Statistics_p->DecodePbHeaderFifoEmpty != 0)
    {
        VIDDEC_Data_p->HeaderData.StatisticsDecodePbHeaderFifoEmpty = 0;
    }
    if (Statistics_p->DecodePbVbvSizeGreaterThanBitBuffer != 0)
    {
        VIDDEC_Data_p->Statistics.DecodePbVbvSizeGreaterThanBitBuffer = 0;
    }
    if (Statistics_p->DecodeMinBitBufferLevelReached != 0)
    {
        VIDDEC_Data_p->Statistics.DecodeMinBitBufferLevelReached = VIDDEC_Data_p->HWDecoder.LastBitBufferLevel;
    }
    if (Statistics_p->DecodeMaxBitBufferLevelReached != 0)
    {
        VIDDEC_Data_p->Statistics.DecodeMaxBitBufferLevelReached = 0;
    }
    if (Statistics_p->DecodePbSequenceNotInMemProfileSkipped != 0)
    {
        VIDDEC_Data_p->Statistics.DecodePbSequenceNotInMemProfileSkipped = 0;
    }

#ifdef STVID_HARDWARE_ERROR_EVENT
    if (Statistics_p->DecodePbHardwareErrorMissingPID != 0)
    {
        VIDDEC_Data_p->Statistics.DecodePbHardwareErrorMissingPID = 0;
    }
    if (Statistics_p->DecodePbHardwareErrorSyntaxError != 0)
    {
        VIDDEC_Data_p->Statistics.DecodePbHardwareErrorSyntaxError = 0;
    }
    if (Statistics_p->DecodePbHardwareErrorTooSmallPicture != 0)
    {
        VIDDEC_Data_p->Statistics.DecodePbHardwareErrorTooSmallPicture = 0;
    }
#endif /* STVID_HARDWARE_ERROR_EVENT */
    VIDDEC_Data_p->Statistics.MaxBitRate = 0;
    VIDDEC_Data_p->Statistics.MinBitRate = 0xFFFFFFFF;
    VIDDEC_Data_p->Statistics.LastBitRate = 0;
    return(ST_NO_ERROR);
} /* End of VIDDEC_ResetStatistics() function */
#endif /* STVID_DEBUG_GET_STATISTICS */

#ifdef STVID_DEBUG_GET_STATUS
/*******************************************************************************
Name        : VIDDEC_GetStatus
Description : Gets the decode module status.
Parameters  : Video decode handle, status.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDDEC_GetStatus(const VIDDEC_Handle_t DecodeHandle, STVID_Status_t * const Status_p)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    if (Status_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

	/* Get Sequence Info Status */
	Status_p->SequenceInfo_p = VIDDEC_Data_p->Status.SequenceInfo_p;

	/* Get Decoder Status */
	HALDEC_GetDebugStatus(((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle,Status_p);

    return(ST_NO_ERROR);
} /* End of VIDDEC_GetStatus() function */
#endif /* STVID_DEBUG_GET_STATUS */

#ifdef ST_SecondInstanceSharingTheSameDecoder
/*******************************************************************************
Name        : VIDDEC_InitSecond
Description : Initialise a secondry decode for the special case of still picture decode
Parameters  : Init params, pointer to video decode handle returned if success
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if bad parameter
              ST_ERROR_NO_MEMORY if memory allocation fails
              STVID_ERROR_EVENT_REGISTRATION if problem registering events
*******************************************************************************/
ST_ErrorCode_t VIDDEC_InitSecond(const VIDDEC_InitParams_t * const InitParams_p, const VIDDEC_Handle_t DecodeHandle)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
    STEVT_OpenParams_t STEVT_OpenParams;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Open EVT handle */
    ErrorCode = STEVT_Open(InitParams_p->EventsHandlerName, &STEVT_OpenParams, &(VIDDEC_Data_p->StillDecodeParams.EventsHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init Second : could not open EVT !"));
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    /* Register events */
    ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->StillDecodeParams.EventsHandle, InitParams_p->VideoName, STVID_NEW_PICTURE_DECODED_EVT, &(VIDDEC_Data_p->StillDecodeParams.RegisteredEventsID[VIDDEC_NEW_PICTURE_DECODED_EVT_ID]));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: registration failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init Second : could not register events !"));
        VIDDEC_TermSecond(DecodeHandle);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    /* Bit buffer allocated by user: just copy parameters */
    VIDDEC_Data_p->StillDecodeParams.BitBufferAddress_p  = InitParams_p->BitBufferAddress_p;
    VIDDEC_Data_p->StillDecodeParams.BitBufferSize = InitParams_p->BitBufferSize;
    VIDDEC_Data_p->StillDecodeParams.BufferManagerHandle = InitParams_p->BufferManagerHandle;

    InitStream(&(VIDDEC_Data_p->StillDecodeParams.MPEG2BitStream), VIDDEC_Data_p->UserDataTable_p, InitParams_p->UserDataSize);

#ifdef TRACE_DECODE
    TraceBuffer(("\r\nDecode module initialized for the second instance.\r\n"));
#endif /* TRACE_DECODE */

    return(ErrorCode);

} /* end of VIDDEC_InitSecond() function */
#endif /* ST_SecondInstanceSharingTheSameDecoder */

/*******************************************************************************
Name        : VIDDEC_Init
Description : Initialise decode
Parameters  : Init params, pointer to video decode handle returned if success
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if bad parameter
              ST_ERROR_NO_MEMORY if memory allocation fails
              STVID_ERROR_EVENT_REGISTRATION if problem registering events
*******************************************************************************/
ST_ErrorCode_t VIDDEC_Init(const VIDDEC_InitParams_t * const InitParams_p, VIDDEC_Handle_t * const DecodeHandle_p)
{
    VIDDEC_Data_t *         VIDDEC_Data_p;
#ifndef ST_multicodec /* if ST_oldmpeg2codec only */
    VIDPROD_Properties_t *  VIDPROD_Data_p;
#endif /* not ST_multicodec */
    VIDDEC_Handle_t         VIDDEC_Handle;
    static char             IrqName[] = STVID_IRQ_NAME;

    STEVT_OpenParams_t STEVT_OpenParams;
    STEVT_DeviceSubscribeParams_t STEVT_SubscribeParams;
    HALDEC_InitParams_t HALInitParams;
    VIDBUFF_AllocateBufferParams_t AllocateParams;
#ifdef ST_avsync
    STCLKRV_OpenParams_t STCkrvOpenParams;
#endif /* ST_avsync */
    U32 UserDataSize;
    ST_ErrorCode_t ErrorCode;

    if ((DecodeHandle_p == NULL) || (InitParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

#ifndef ST_multicodec /* if ST_oldmpeg2codec only allocate upper layer data here (done by VIDPROD when used) */
    SAFE_MEMORY_ALLOCATE(VIDPROD_Data_p, VIDPROD_Properties_t *, InitParams_p->CPUPartition_p, sizeof(VIDPROD_Properties_t) );
    if (VIDPROD_Data_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }
    VIDPROD_Data_p->CPUPartition_p   = InitParams_p->CPUPartition_p;
    VIDPROD_Data_p->FunctionsTable_p = &VIDDEC_Functions;
#endif /* not ST_multicodec */

    /* Allocate a VIDDEC structure */
    SAFE_MEMORY_ALLOCATE(VIDDEC_Data_p, VIDDEC_Data_t *, InitParams_p->CPUPartition_p, sizeof(VIDDEC_Data_t) );
    if (VIDDEC_Data_p == NULL)
    {
        /* Error: allocation failed */
        return(ST_ERROR_NO_MEMORY);
    }
#ifndef ST_multicodec
    VIDPROD_Data_p->InternalProducerHandle = (VIDPROD_Handle_t) VIDDEC_Data_p;
#endif

    /* Remember partition */
    VIDDEC_Data_p->CPUPartition_p = InitParams_p->CPUPartition_p;

    /* Allocate stream structure */
    SAFE_MEMORY_ALLOCATE(VIDDEC_Data_p->DecodingStream_p, MPEG2BitStream_t *, InitParams_p->CPUPartition_p, sizeof(MPEG2BitStream_t) );
    if (VIDDEC_Data_p->DecodingStream_p == NULL)
    {
        /* Error: allocation failed, deallocate previous allocations */
        SAFE_MEMORY_DEALLOCATE(VIDDEC_Data_p, VIDDEC_Data_p->CPUPartition_p, sizeof(VIDDEC_Data_t));
        return(ST_ERROR_NO_MEMORY);
    }

    /* Allocation succeeded: make handle point on structure */
#ifndef ST_multicodec /* if ST_oldmpeg2codec only */
    *DecodeHandle_p = (VIDPROD_Handle_t *) VIDPROD_Data_p;
    VIDDEC_Handle   = (VIDDEC_Handle_t *) VIDDEC_Data_p;
/* TODO_PC (VIDDEC_Init) : Check for ValidityCheck of upper layer here ? */
#else /* not ST_multicodec */
    *DecodeHandle_p = (VIDDEC_Handle_t *) VIDDEC_Data_p;
    VIDDEC_Handle   = (VIDDEC_Handle_t *) VIDDEC_Data_p;
#endif /* ST_multicodec */

    VIDDEC_Data_p->ValidityCheck = VIDDEC_VALID_HANDLE;

    /* Init decode structure */
    InitDecodeStructure(VIDDEC_Handle);

    /* Map registers */
    /* This mapping has to be done even for OS not needing registers mapping such as OS21 */
    /* This constraint is due to the fact most of the OS (WINCE, Linux, ...) need register mapping before */
    /* being able to access them ... */
    VIDDEC_Data_p->MappedRegistersBaseAddress_p = (unsigned long *) STOS_MapRegisters(InitParams_p->RegistersBaseAddress, STVID_MAPPED_WIDTH, "VID");
    STTBX_Print(("VID virtual io kernel address of phys 0x%.8x = 0x%.8x\n", (U32)InitParams_p->RegistersBaseAddress, (U32)VIDDEC_Data_p->MappedRegistersBaseAddress_p));

    VIDDEC_Data_p->MappedCompressedDataInputBaseAddress_p = (unsigned long *) STOS_MapRegisters(InitParams_p->CompressedDataInputBaseAddress_p, STVID_MAPPED_WIDTH, "CD FIFO");
    STTBX_Print(("VID compressed virtual io kernel address of phys 0x%.8x = 0x%.8x\n", (U32)InitParams_p->CompressedDataInputBaseAddress_p, (U32)VIDDEC_Data_p->MappedCompressedDataInputBaseAddress_p));

    /* Allow UserData event if required. */
    VIDDEC_Data_p->UserDataEventEnabled = (InitParams_p->UserDataSize != 0);

    /* Allocate table of user data */
    UserDataSize = InitParams_p->UserDataSize;
    /* So that we can eploit uesr data for our purpose, we need a minimum amount of user data (even if requested 0 for example) */
    if (UserDataSize < MIN_USER_DATA_SIZE)
    {
        UserDataSize = MIN_USER_DATA_SIZE;
    }
    SAFE_MEMORY_ALLOCATE( VIDDEC_Data_p->UserDataTable_p, void *, InitParams_p->CPUPartition_p, UserDataSize );
    if (VIDDEC_Data_p->UserDataTable_p == NULL)
    {
        /* Error: allocation failed, undo initialisations done */
        VIDDEC_Term(*DecodeHandle_p);
        return(ST_ERROR_NO_MEMORY);
    }

    /* Init stream structure */
    InitStream(VIDDEC_Data_p->DecodingStream_p, VIDDEC_Data_p->UserDataTable_p, UserDataSize);

    /* Store parameters */
    VIDDEC_Data_p->BufferManagerHandle  = InitParams_p->BufferManagerHandle;
#ifdef ST_ordqueue
    VIDDEC_Data_p->OrderingQueueHandle  = InitParams_p->OrderingQueueHandle;
#endif /* ST_ordqueue */
#ifdef ST_inject
    VIDDEC_Data_p->InjecterHandle       = InitParams_p->InjecterHandle;
#endif /* ST_inject */
    VIDDEC_Data_p->DeviceType           = InitParams_p->DeviceType;
    VIDDEC_Data_p->DecoderNumber        = InitParams_p->DecoderNumber;

    /* Get AV mem mapping */
    STAVMEM_GetSharedMemoryVirtualMapping(&(VIDDEC_Data_p->VirtualMapping));

    /* Open EVT handle */
    ErrorCode = STEVT_Open(InitParams_p->EventsHandlerName, &STEVT_OpenParams, &(VIDDEC_Data_p->EventsHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not open EVT !"));
        VIDDEC_Term(*DecodeHandle_p);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }
    /* Prepare common subscribe parameters */
    STEVT_SubscribeParams.SubscriberData_p = (void *) (VIDDEC_Handle);
#ifdef ST_avsync
    /* Open clock recovery driver */
    ErrorCode = STCLKRV_Open(InitParams_p->ClockRecoveryName, &STCkrvOpenParams,
            &(VIDDEC_Data_p->VbvStartCondition.STClkrvHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module AVSync init: could not open CLKRV !"));
        VIDDEC_Term(*DecodeHandle_p);
        return(STVID_ERROR_SYSTEM_CLOCK);
    }
#endif /* ST_avsync */

    /* Register events */
    ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, STVID_SEQUENCE_INFO_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_SEQUENCE_INFO_EVT_ID]));
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, STVID_USER_DATA_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_USER_DATA_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, STVID_NEW_PICTURE_DECODED_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_NEW_PICTURE_DECODED_EVT_ID]));
    }
#ifdef ST_smoothbackward
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, VIDDEC_START_CODE_FOUND_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_START_CODE_FOUND_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, VIDDEC_START_CODE_SLICE_FOUND_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_START_CODE_SLICE_FOUND_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, VIDDEC_DECODER_IDLE_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_DECODER_IDLE_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, VIDDEC_READY_TO_START_BACKWARD_MODE_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_READY_TO_START_BACKWARD_MODE_EVT_ID]));
    }

    /* if (ErrorCode == ST_NO_ERROR) */
    /* { */
        /* ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, VIDDEC_DATA_INJECTION_EXPIRED_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_DATA_INJECTION_EXPIRED_EVT_ID])); */
    /* } */
#endif /* ST_smoothbackward */
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, STVID_DATA_OVERFLOW_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_DATA_OVERFLOW_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, STVID_DATA_UNDERFLOW_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_DATA_UNDERFLOW_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, VIDDEC_READY_TO_DECODE_NEW_PICTURE_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_READY_TO_DECODE_NEW_PICTURE_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, STVID_DATA_ERROR_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_DATA_ERROR_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, VIDDEC_STOPPED_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_STOPPED_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, STVID_PICTURE_DECODING_ERROR_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_PICTURE_DECODING_ERROR_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, VIDDEC_NEW_PICTURE_SKIPPED_NOT_REQUESTED_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_NEW_PICTURE_SKIPPED_NOT_REQUESTED_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, VIDDEC_DECODE_ONCE_READY_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_DECODE_ONCE_READY_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, VIDDEC_RESTARTED_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_RESTARTED_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, VIDDEC_DECODE_LAUNCHED_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_DECODE_LAUNCHED_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, VIDDEC_SKIP_LAUNCHED_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_SKIP_LAUNCHED_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, STVID_SEQUENCE_END_CODE_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_SEQUENCE_END_CODE_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, VIDDEC_NEW_PICTURE_PARSED_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_NEW_PICTURE_PARSED_EVT_ID]));
    }
#ifdef STVID_HARDWARE_ERROR_EVENT
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, STVID_HARDWARE_ERROR_EVT, &(VIDDEC_Data_p->RegisteredEventsID[VIDDEC_HARDWARE_ERROR_EVT_ID]));
    }
#endif /* STVID_HARDWARE_ERROR_EVENT */
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: registration failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not register events !"));
        VIDDEC_Term(*DecodeHandle_p);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    /* Subscribe to interrupt event */
    VIDDEC_Data_p->VideoInterrupt.InstallHandler = FALSE;
    if (!( InitParams_p->InstallVideoInterruptHandler))
    {
        /* Using interrupt manager to have IT events just subscribe to event */
        STEVT_SubscribeParams.NotifyCallback = viddec_InterruptHandler;
        ErrorCode = STEVT_SubscribeDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->InterruptEventName, InitParams_p->InterruptEvent, &STEVT_SubscribeParams);
        if (ErrorCode != ST_NO_ERROR)
        {
            /* Error: subscribe failed, undo initialisations done */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not subscribe to interrupt event !"));
            VIDDEC_Term(*DecodeHandle_p);
            return(ST_ERROR_BAD_PARAMETER);
        }
    }
    else
    {
        /* Install interrupt handler ourselves: invent an event number that should not be used anywhere else ! */
        STEVT_SubscribeParams.NotifyCallback = viddec_InterruptHandler;
        ErrorCode = STEVT_SubscribeDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, VIDDEC_SELF_INSTALLED_INTERRUPT_EVT, &STEVT_SubscribeParams);

#if defined(ST_OSLINUX) || defined(ST_OSWINCE)

        /* Record interrupt number and level for term operations */
        VIDDEC_Data_p->VideoInterrupt.Level = InitParams_p->InterruptLevel;
        VIDDEC_Data_p->VideoInterrupt.Number = InitParams_p->InterruptNumber;
#endif

        if (ErrorCode != ST_NO_ERROR)
        {
            /* Error: subscribe failed, undo initialisations done */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not subscribe to internal interrupt event !"));
            VIDDEC_Term(*DecodeHandle_p);
            return(ST_ERROR_BAD_PARAMETER);
        }
        else if ( (VIDDEC_Data_p->DeviceType != STVID_DEVICE_TYPE_5100_MPEG) &&
                  (VIDDEC_Data_p->DeviceType != STVID_DEVICE_TYPE_5525_MPEG) &&
                  (VIDDEC_Data_p->DeviceType != STVID_DEVICE_TYPE_5105_MPEG) &&
                  (VIDDEC_Data_p->DeviceType != STVID_DEVICE_TYPE_5301_MPEG) &&
                  (VIDDEC_Data_p->DeviceType != STVID_DEVICE_TYPE_7710_MPEG) &&
                  (VIDDEC_Data_p->DeviceType != STVID_DEVICE_TYPE_7100_MPEG) &&
                  (VIDDEC_Data_p->DeviceType != STVID_DEVICE_TYPE_7109_MPEG))
            /* if we are on 5100, the process of It installation will be
               continued by the hal, which manage the multi instances */

        {
            /* Register ourselves interrupt event */
#ifdef ST_OSLINUX
            ErrorCode = ST_NO_ERROR;
#else
            ErrorCode = STEVT_RegisterDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName, VIDDEC_SELF_INSTALLED_INTERRUPT_EVT, &(VIDDEC_Data_p->VideoInterrupt.EventID));
#endif
            if (ErrorCode != ST_NO_ERROR)
            {
                /* Error: notify failed, undo initialisations done */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not notify internal interrupt event !"));
                VIDDEC_Term(*DecodeHandle_p);
                return(ST_ERROR_BAD_PARAMETER);
            }
            else
            {
                /* Install interrupt handler before HAL is initialised, i.e. before HW is starting being activated or having a default state.
                This is to be sure that no interrupt can occur when before installing the interrupt handler, as recommended in OS20 doc. */
                VIDDEC_Data_p->VideoInterrupt.Level = InitParams_p->InterruptLevel;
                VIDDEC_Data_p->VideoInterrupt.Number = InitParams_p->InterruptNumber;

                STOS_InterruptLock();

                /* 1.  installation of the MPEG decoder interruption */
#ifndef STVID_STVAPI_ARCHITECTURE
                /* Normal STB chips... */

                if (STOS_InterruptInstall(VIDDEC_Data_p->VideoInterrupt.Number,
                                          VIDDEC_Data_p->VideoInterrupt.Level,
#ifdef ST_OSLINUX
                                          STOS_INTERRUPT_CAST(viddec_InterruptHandler),
#else
                                          STOS_INTERRUPT_CAST(SelfInstalledVideoInterruptHandler),
#endif
                                          IrqName,
                                          (void *) STEVT_SubscribeParams.SubscriberData_p) != STOS_SUCCESS)
                {
                    STOS_InterruptUnlock();
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Problem installing interrupt !"));
                    VIDDEC_Term(*DecodeHandle_p);
                    return(ST_ERROR_BAD_PARAMETER);
                }
                STOS_InterruptUnlock();

                VIDDEC_Data_p->VideoInterrupt.InstallHandler = TRUE;

                if (STOS_InterruptEnable(VIDDEC_Data_p->VideoInterrupt.Number, VIDDEC_Data_p->VideoInterrupt.Level) != STOS_SUCCESS)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Problem enabling interrupt !"));
                    VIDDEC_Term(*DecodeHandle_p);
                    return(ST_ERROR_BAD_PARAMETER);
                }

#else /* end of ndef STVID_STVAPI_ARCHITECTURE */

                /* STD2000 chips */
#ifndef INTERRUPT_INSTALL_WORKAROUND
                if (interrupt_install
#else /* ndef INTERRUPT_INSTALL_WORKAROUND */
                if (interrupt_install_shared
#endif /* def INTERRUPT_INSTALL_WORKAROUND */
                                 (interrupt_handle(VIDDEC_Data_p->VideoInterrupt.Number),
                                 (interrupt_handler_t) SelfInstalledVideoInterruptHandler,
                                 (void *) VIDDEC_Data_p) != OS21_SUCCESS)
                {
                    STOS_InterruptUnlock();
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Problem installing interrupt !"));
                    VIDDEC_Term(*DecodeHandle_p);
                    return(ST_ERROR_BAD_PARAMETER);
                }
                STOS_InterruptUnlock();

                VIDDEC_Data_p->VideoInterrupt.InstallHandler = TRUE;

                if ((STOS_InterruptEnable(VIDDEC_Data_p->VideoInterrupt.Number,VIDDEC_Data_p->VideoInterrupt.Level))) != STOS_SUCCESS)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Problem enabling interrupt !"));
                    VIDDEC_Term(*DecodeHandle_p);
                    return(ST_ERROR_BAD_PARAMETER);
                }
                /* 2. installation of the macroblock to raster it (OS21 only) */
                VIDDEC_Data_p->MB2RInterrupt.Level = InitParams_p->MB2RInterruptLevel;
                VIDDEC_Data_p->MB2RInterrupt.Number = InitParams_p->MB2RInterruptNumber;
                STOS_InterruptLock();
#ifndef INTERRUPT_INSTALL_WORKAROUND
                if (interrupt_install_shared
#else /* ndef INTERRUPT_INSTALL_WORKAROUND */
                if (interrupt_install_shared
#endif /* def INTERRUPT_INSTALL_WORKAROUND */
                                 (interrupt_handle(VIDDEC_Data_p->MB2RInterrupt.Number),
                                 (interrupt_handler_t) viddec_MacroBlockToRasterInterruptHandler,
                                 (void *) VIDDEC_Handle) != OS21_SUCCESS )
                {
                    STOS_InterruptUnlock();
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Problem installing interrupt !"));
                    VIDDEC_Term(*DecodeHandle_p);
                    return(ST_ERROR_BAD_PARAMETER);
                }
                STOS_InterruptUnlock();
                if ((STOS_InterruptEnable(VIDDEC_Data_p->MB2RInterrupt.Number, VIDDEC_Data_p->MB2RInterrupt.Level)) != STOS_SUCCESS)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Problem enabling MB2R interrupt !"));
                    VIDDEC_Term(*DecodeHandle_p);
                    return(ST_ERROR_BAD_PARAMETER);
                }
#endif /*  end of def STVID_STVAPI_ARCHITECTURE */

            } /* End interrupt event register OK */
        } /* end interrupt event subscribe OK */
    } /* end install ourselves interrupt handler */

    /* Initialise HAL: interrupts could eventually occur from that moment on. */
    HALInitParams.CPUPartition_p    = InitParams_p->CPUPartition_p;
    HALInitParams.DeviceType        = InitParams_p->DeviceType;

    HALInitParams.RegistersBaseAddress_p = VIDDEC_Data_p->MappedRegistersBaseAddress_p;
    HALInitParams.CompressedDataInputBaseAddress_p = VIDDEC_Data_p->MappedCompressedDataInputBaseAddress_p;

    HALInitParams.SharedMemoryBaseAddress_p = InitParams_p->SharedMemoryBaseAddress_p;
    strcpy(HALInitParams.EventsHandlerName, InitParams_p->EventsHandlerName);
    strcpy(HALInitParams.VideoName, InitParams_p->VideoName);
    HALInitParams.DecoderNumber = VIDDEC_Data_p->DecoderNumber;
    HALInitParams.AvmemPartition = InitParams_p->AvmemPartitionHandle;

    /* The following params will not be used by all HAL's */
    HALInitParams.SharedItParams.Event    = VIDDEC_SELF_INSTALLED_INTERRUPT_EVT;
    HALInitParams.SharedItParams.Level    = InitParams_p->InterruptLevel;
    HALInitParams.SharedItParams.Number   = InitParams_p->InterruptNumber;
    if(InitParams_p->BitBufferAllocated)
    {
        HALInitParams.BitBufferSize           = InitParams_p->BitBufferSize;
    }
    else
    {
        switch (InitParams_p->DeviceType)
        {
            case STVID_DEVICE_TYPE_5508_MPEG :
            case STVID_DEVICE_TYPE_5510_MPEG :
            case STVID_DEVICE_TYPE_5512_MPEG :
            case STVID_DEVICE_TYPE_5514_MPEG :
            case STVID_DEVICE_TYPE_5516_MPEG :
            case STVID_DEVICE_TYPE_5517_MPEG :
            case STVID_DEVICE_TYPE_5518_MPEG :
            case STVID_DEVICE_TYPE_5528_MPEG :
            case STVID_DEVICE_TYPE_5100_MPEG :
            case STVID_DEVICE_TYPE_5525_MPEG :
            case STVID_DEVICE_TYPE_5105_MPEG :
            case STVID_DEVICE_TYPE_5301_MPEG :
                HALInitParams.BitBufferSize = DEFAULT_BIT_BUFFER_SIZE_SD;
                VIDDEC_Data_p->HWStartCodeDetector.MaxBitbufferLevelForFindingStartCode = MAX_VIDEO_ES_SIZE_PROFILE_MAIN_LEVEL_MAIN;
                break;

            default :
#ifdef SUPPORT_ONLY_SD_PROFILES
                HALInitParams.BitBufferSize = DEFAULT_BIT_BUFFER_SIZE_SD;
                VIDDEC_Data_p->HWStartCodeDetector.MaxBitbufferLevelForFindingStartCode = MAX_VIDEO_ES_SIZE_PROFILE_MAIN_LEVEL_MAIN;
#else
                HALInitParams.BitBufferSize = DEFAULT_BIT_BUFFER_SIZE_HD;
                VIDDEC_Data_p->HWStartCodeDetector.MaxBitbufferLevelForFindingStartCode = MAX_VIDEO_ES_SIZE_PROFILE_MAIN_LEVEL_HIGH;
#endif
                break;
        }
    }

#ifdef ST_inject
    HALInitParams.InjecterHandle    = InitParams_p->InjecterHandle;
#endif /* ST_inject */
#ifdef ST_multidecodeswcontroller
    HALInitParams.MultiDecodeHandle = InitParams_p->MultiDecodeHandle;
#endif /* ST_multidecodeswcontroller */
     ErrorCode = HALDEC_Init(&HALInitParams, &(VIDDEC_Data_p->HALDecodeHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: HAL init failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not initialize HAL decode !"));
        VIDDEC_Term(*DecodeHandle_p);
        return(ErrorCode);
    }
    VIDDEC_Data_p->HeaderData.HALDecodeHandle = VIDDEC_Data_p->HALDecodeHandle;

    /* Set bit buffer */
    if (!(InitParams_p->BitBufferAllocated))
    {
        /* Bit buffer was not allocated: allocate it. */
        AllocateParams.BufferSize     = HALInitParams.BitBufferSize;
        AllocateParams.AllocationMode = VIDBUFF_ALLOCATION_MODE_SHARED_MEMORY; /* !!! Should be passed from higher level */
        AllocateParams.BufferType     = VIDBUFF_BUFFER_SIMPLE;

        ErrorCode = VIDBUFF_AllocateBitBuffer(VIDDEC_Data_p->BufferManagerHandle, &AllocateParams, &(VIDDEC_Data_p->BitBuffer));
        if (ErrorCode != ST_NO_ERROR)
        {
            /* Error: cannot allocate bit buffer, undo initialisations done */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not allocate bit buffer !"));
            VIDDEC_Term(*DecodeHandle_p);
            return(ST_ERROR_NO_MEMORY);
        }

        VIDDEC_Data_p->BitBufferAllocatedByUs = TRUE;
    }
    else
    {
        /* Bit buffer allocated by user: just copy parameters */
        VIDDEC_Data_p->BitBuffer.Address_p  = InitParams_p->BitBufferAddress_p;
        VIDDEC_Data_p->BitBuffer.TotalSize  = InitParams_p->BitBufferSize;
        VIDDEC_Data_p->BitBuffer.BufferType = VIDBUFF_BUFFER_SIMPLE;

        VIDDEC_Data_p->BitBufferAllocatedByUs = FALSE;
    }

    /* Set bit buffer */
    HALDEC_SetDecodeBitBuffer(VIDDEC_Data_p->HALDecodeHandle, VIDDEC_Data_p->BitBuffer.Address_p, VIDDEC_Data_p->BitBuffer.TotalSize, HALDEC_BIT_BUFFER_HW_MANAGED_CIRCULAR);
    viddec_SetBitBufferThresholdToIdleValue(VIDDEC_Handle);
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Bit buffer set at address 0x%x.", VIDDEC_Data_p->BitBuffer.Address_p));
    /* Set bit buffer underflow event params  */
    VIDDEC_Data_p->BitBufferUnderflowMargin  = (((VIDCOM_MAX_BITRATE_MPEG_SD * 400) / 8) / VIDCOM_MIN_FRAME_RATE_MPEG) * VIDCOM_DECODER_SPEED_MPEG;
    VIDDEC_Data_p->BitBufferRequiredDuration = (STVID_MPEG_CLOCKS_ONE_SECOND_DURATION / VIDCOM_MIN_FRAME_RATE_MPEG) * VIDCOM_DECODER_SPEED_MPEG;

#ifdef ST_smoothbackward
    /* Configure  HAL in forward mode */
    HALDEC_SetSmoothBackwardConfiguration(VIDDEC_Data_p->HALDecodeHandle, FALSE);
    VIDDEC_Data_p->ControlMode = VIDDEC_AUTOMATIC_CONTROL_MODE;
    VIDDEC_Data_p->ControlModeInsideHAL = VIDDEC_AUTOMATIC_CONTROL_MODE;
#endif /*ST_smoothbackward*/

    /* Start decode task */
    ErrorCode = StartDecodeTask(VIDDEC_Handle);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: task creation failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not start display task ! (%s)", (ErrorCode == ST_ERROR_ALREADY_INITIALIZED?"already exists":"failed")));
        VIDDEC_Term(*DecodeHandle_p);
        if (ErrorCode == ST_ERROR_ALREADY_INITIALIZED)
        {
            /* Change error code because ST_ERROR_ALREADY_INITIALIZED is not allowed for _Init() functions */
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        return(ErrorCode);
    }

    /* Subscribe to VIDBUFF new picture buffer available EVT */
    STEVT_SubscribeParams.NotifyCallback = NewPictureBufferAvailable;
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDDEC_Data_p->EventsHandle, InitParams_p->VideoName,
                    VIDBUFF_NEW_PICTURE_BUFFER_AVAILABLE_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Decode init: could not subscribe to VIDBUFF_NEW_PICTURE_BUFFER_AVAILABLE_EVT !"));
        VIDDEC_Term(*DecodeHandle_p);
        return(ST_ERROR_BAD_PARAMETER);
    }

#ifdef STVID_DEBUG_GET_STATUS
	VIDDEC_Data_p->Status.SequenceInfo_p = NULL;
#endif /* STVID_DEBUG_GET_STATUS */

#ifdef TRACE_DECODE
    TraceBuffer(("\r\nDecode module initialized.\r\n"));
#endif /* TRACE_DECODE */

    return(ErrorCode);

} /* end of VIDDEC_Init() function */

/*******************************************************************************
Name        : VIDDEC_NewReferencePicture
Description : Give new reference picture to the decoder (decoder always remembers 2 reference pictures)
              if Picture_p is NULL, both reference pictures of the decoder are set NULL.
              If the given picture points to same frame buffer than a current reference one, it is discarded. (usefull for field pictures)
Parameters  : Video decode handle, new ref picture
Assumptions : Picture info structures are valid: buffer may use them !
Limitations :
Returns     : Nothing
*******************************************************************************/
ST_ErrorCode_t VIDDEC_NewReferencePicture(const VIDDEC_Handle_t DecodeHandle, VIDBUFF_PictureBuffer_t * const Picture_p)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    if (Picture_p == NULL)
    {
#ifdef TRACE_TAKE_RELEASE
        TraceBuffer(("Error! Invalid reference!\r\n"));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Lock Access to Reference Pictures */
    semaphore_wait(VIDDEC_Data_p->ReferencePicture.ReferencePictureSemaphore_p);

#ifdef TRACE_TAKE_RELEASE
    TraceBuffer(("New ref 0x%x\r\n", Picture_p));
#endif

#ifdef ST_smoothbackward
    if (VIDDEC_Data_p->ControlMode == VIDDEC_MANUAL_CONTROL_MODE)
    {
        /*** MANUAL CONTROL MODE ***/

        /* In Smooth Backward mode, we save the new references pictures but the take/release are performed by vid_tric */
        VIDDEC_Data_p->ReferencePicture.LastButOne_p = VIDDEC_Data_p->ReferencePicture.Last_p;
        VIDDEC_Data_p->ReferencePicture.Last_p = Picture_p;
    }
    else
#endif /* ST_smoothbackward */
    {
        /*** AUTOMATIC CONTROL MODE ***/

        /* Don't allow new reference if given picture points to same frame buffer than "Last_p" */
        if ( (VIDDEC_Data_p->ReferencePicture.Last_p != NULL) &&
             (Picture_p->FrameBuffer_p == VIDDEC_Data_p->ReferencePicture.Last_p->FrameBuffer_p) )
        {
            /* This picture points to the same Frame Buffer than "Last_p": Check if it is the 2nd field of the same frame */
            if ( (Picture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure != STVID_PICTURE_STRUCTURE_FRAME) &&
                 (!(Picture_p->ParsedPictureInformation.PictureGenericData.IsFirstOfTwoFields) ) )
            {
                /* Take the picture if it has not already been taken before */
                if (!IS_PICTURE_BUFFER_USED_BY(Picture_p, VIDCOM_VIDDEC_REF_MODULE_BASE))
                {
                    TAKE_PICTURE_BUFFER(VIDDEC_Data_p->BufferManagerHandle, Picture_p, VIDCOM_VIDDEC_REF_MODULE_BASE);
#ifdef TRACE_TAKE_RELEASE
                    TraceBuffer((" Take 2nd field of Last_p: 0x%x\r\n", Picture_p));
#endif
                }
            }
            else
            {
#ifdef TRACE_TAKE_RELEASE
            TraceBuffer(("Ref not kept because this picture points to the same FB than Last_p (0x%x)\r\n", VIDDEC_Data_p->ReferencePicture.Last_p));
#endif
            }
            /* UnLock Access to Reference Pictures */
            semaphore_signal(VIDDEC_Data_p->ReferencePicture.ReferencePictureSemaphore_p);
            /* Discard this reference */
            return(ST_NO_ERROR);
        }


        /* Don't allow new reference if given picture is a 2nd field. The reference has already been added with the 1st field. */
        /* Normally we have gone out with the previous test except if the decode of the 1st field has had an error.     */
        if ( (Picture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure != STVID_PICTURE_STRUCTURE_FRAME) &&
             (!(Picture_p->ParsedPictureInformation.PictureGenericData.IsFirstOfTwoFields) ) )
        {
#ifdef TRACE_TAKE_RELEASE
            TraceBuffer(("Reference can not be a 2nd Field!\r\n"));
#endif
            /* UnLock Access to Reference Pictures */
            semaphore_signal(VIDDEC_Data_p->ReferencePicture.ReferencePictureSemaphore_p);
            /* Discard this reference */
            return(ST_NO_ERROR);
        }

        /* A new reference is pushed:
         - "LastButOne_p" can be released and discarded
         - "Last_p" reference becomes "LastButOne_p" reference (the picture remains locked)
         - The new picture is taken and set as "Last_p" reference */
        VIDDEC_ReleaseLastButOneReference(DecodeHandle);
        VIDDEC_Data_p->ReferencePicture.LastButOne_p = VIDDEC_Data_p->ReferencePicture.Last_p;
        VIDDEC_SetAsLastReference(DecodeHandle, Picture_p);

        /* Previous reference present */
        VIDDEC_Data_p->ReferencePicture.MissingPrevious = FALSE;
    }

#ifdef TRACE_TAKE_RELEASE
    TraceBuffer(("New references:\r\n"));
    TraceBuffer(("  Last_p=0x%x\r\n", VIDDEC_Data_p->ReferencePicture.Last_p));
    TraceBuffer(("  LastButOne_p=0x%x\r\n", VIDDEC_Data_p->ReferencePicture.LastButOne_p));
#endif

    /* The only case when this function is called and the driver is stopped is throw the call of STVID_Clear,
     * so we have to set the flag KeepLastReference to prevent resetting the last reference to NULL on the next restart */
    if (VIDDEC_Data_p->DecoderState == VIDDEC_DECODER_STATE_STOPPED)
    {
       VIDDEC_Data_p->ReferencePicture.KeepLastReference = TRUE;
    }

    /* UnLock Access to Reference Pictures */
    semaphore_signal(VIDDEC_Data_p->ReferencePicture.ReferencePictureSemaphore_p);

    return(ST_NO_ERROR);

} /* End of VIDDEC_NewReferencePicture() function */

/*******************************************************************************
Name        : VIDDEC_ReleaseReferencePictures
Description : This function release the Last and LastButOne references.
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t VIDDEC_ReleaseReferencePictures(const VIDDEC_Handle_t DecodeHandle)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    /* Lock Access to Reference Pictures */
    semaphore_wait(VIDDEC_Data_p->ReferencePicture.ReferencePictureSemaphore_p);

#ifdef TRACE_TAKE_RELEASE
    TraceBuffer(("Release references\r\n"));
#endif

#ifdef ST_smoothbackward
    if (VIDDEC_Data_p->ControlMode == VIDDEC_MANUAL_CONTROL_MODE)
    {
        /* In Smooth Backward mode, the take/release are performed by vid_tric so we simply reset the references */
        VIDDEC_Data_p->ReferencePicture.LastButOne_p = NULL;
        VIDDEC_Data_p->ReferencePicture.Last_p = NULL;
    }
    else
#endif /* ST_smoothbackward */
    {
        VIDDEC_ReleaseLastButOneReference(DecodeHandle);
        VIDDEC_ReleaseLastReference(DecodeHandle);
    }

    /* Previous reference missing */
    VIDDEC_Data_p->ReferencePicture.MissingPrevious = TRUE;

    /* UnLock Access to Reference Pictures */
    semaphore_signal(VIDDEC_Data_p->ReferencePicture.ReferencePictureSemaphore_p);

    return(ST_NO_ERROR);

} /* End of VIDDEC_ReleaseReferencePictures() function */

/*******************************************************************************
Name        : VIDDEC_SetAsLastReference
Description :
Parameters  : Video decode handle
Assumptions : The call to this function must be protected with VIDDEC_Data_p->ReferencePicture.ReferencePictureSemaphore_p
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t VIDDEC_SetAsLastReference(const VIDDEC_Handle_t DecodeHandle, VIDBUFF_PictureBuffer_t * const Picture_p)
{
    VIDDEC_Data_t *             VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    if (Picture_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    TAKE_PICTURE_BUFFER(VIDDEC_Data_p->BufferManagerHandle, Picture_p, VIDCOM_VIDDEC_REF_MODULE_BASE);
#ifdef TRACE_TAKE_RELEASE
    TraceBuffer((" Take 1st field of Last_p: 0x%x\r\n", Picture_p));
#endif

    /* Save the new reference */
    VIDDEC_Data_p->ReferencePicture.Last_p = Picture_p;

    return(ST_NO_ERROR);
} /* End of VIDDEC_SetAsLastReference() function */

/*******************************************************************************
Name        : VIDDEC_ReleaseLastReference
Description : This function release the Last_p reference picture and the associated 2nd Field (if it exists)
Parameters  : Video decode handle
Assumptions : The call to this function must be protected with VIDDEC_Data_p->ReferencePicture.ReferencePictureSemaphore_p
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t VIDDEC_ReleaseLastReference(const VIDDEC_Handle_t DecodeHandle)
{
    VIDDEC_Data_t *             VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
    VIDBUFF_PictureBuffer_t *   NothingOrSecondFieldPicture_p;

    if (VIDDEC_Data_p->ReferencePicture.Last_p != NULL)
    {
        /* We are about to release the picture buffer "Last_p". We should first check if there is an associated 2nd field
           that should be released also (this happen in case of Field encoding) */
        NothingOrSecondFieldPicture_p = VIDDEC_Data_p->ReferencePicture.Last_p->FrameBuffer_p->NothingOrSecondFieldPicture_p;

        if ( (NothingOrSecondFieldPicture_p != NULL) &&
             (IS_PICTURE_BUFFER_IN_USE(NothingOrSecondFieldPicture_p) ) )
        {
#ifdef TRACE_TAKE_RELEASE
            TraceBuffer((" Release 2nd field of Last_p: 0x%x\r\n", NothingOrSecondFieldPicture_p));
#endif
            RELEASE_PICTURE_BUFFER(VIDDEC_Data_p->BufferManagerHandle, NothingOrSecondFieldPicture_p, VIDCOM_VIDDEC_REF_MODULE_BASE);
            /* It is not allowed to set NothingOrSecondFieldPicture_p to NULL because the link between Frame Buffer and Picture Buffer is managed by VID_BUFF */
        }

         /* We can now release Last_p */
#ifdef TRACE_TAKE_RELEASE
        TraceBuffer((" Release Last_p: 0x%x\r\n", VIDDEC_Data_p->ReferencePicture.Last_p));
#endif
        RELEASE_PICTURE_BUFFER(VIDDEC_Data_p->BufferManagerHandle, VIDDEC_Data_p->ReferencePicture.Last_p, VIDCOM_VIDDEC_REF_MODULE_BASE);
        VIDDEC_Data_p->ReferencePicture.Last_p = NULL;
    }

    return(ST_NO_ERROR);
} /* End of VIDDEC_ReleaseLastReference() function */

/*******************************************************************************
Name        : VIDDEC_ReleaseLastButOneReference
Description : This function release the LastButOne_p reference picture and the associated 2nd Field (if it exists)
Parameters  : Video decode handle
Assumptions : The call to this function must be protected with VIDDEC_Data_p->ReferencePicture.ReferencePictureSemaphore_p
Limitations :
Returns     :
*******************************************************************************/
static  ST_ErrorCode_t VIDDEC_ReleaseLastButOneReference(const VIDDEC_Handle_t DecodeHandle)
{
    VIDDEC_Data_t *             VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
    VIDBUFF_PictureBuffer_t *   NothingOrSecondFieldPicture_p;

    if (VIDDEC_Data_p->ReferencePicture.LastButOne_p != NULL)
    {
        /* We are about to release the picture buffer "LastButOne_p". We should first check if there is an associated 2nd field
           that should be released also (this happen in case of Field encoding) */
        NothingOrSecondFieldPicture_p = VIDDEC_Data_p->ReferencePicture.LastButOne_p->FrameBuffer_p->NothingOrSecondFieldPicture_p;

        if ( (NothingOrSecondFieldPicture_p != NULL) &&
             (IS_PICTURE_BUFFER_IN_USE(NothingOrSecondFieldPicture_p) ) )
        {
#ifdef TRACE_TAKE_RELEASE
            TraceBuffer((" Release 2nd field of LastButOne_p: 0x%x\r\n", NothingOrSecondFieldPicture_p));
#endif
            RELEASE_PICTURE_BUFFER(VIDDEC_Data_p->BufferManagerHandle, NothingOrSecondFieldPicture_p, VIDCOM_VIDDEC_REF_MODULE_BASE);
            /* It is not allowed to set NothingOrSecondFieldPicture_p to NULL because the link between Frame Buffer and Picture Buffer is managed by VID_BUFF */
        }

         /* We can now release LastButOne_p */
#ifdef TRACE_TAKE_RELEASE
        TraceBuffer((" Release LastButOne_p: 0x%x\r\n", VIDDEC_Data_p->ReferencePicture.LastButOne_p));
#endif
        RELEASE_PICTURE_BUFFER(VIDDEC_Data_p->BufferManagerHandle, VIDDEC_Data_p->ReferencePicture.LastButOne_p, VIDCOM_VIDDEC_REF_MODULE_BASE);
        VIDDEC_Data_p->ReferencePicture.LastButOne_p = NULL;
    }

    return(ST_NO_ERROR);
} /* End of VIDDEC_ReleaseLastButOneReference() function */

/*******************************************************************************
Name        : VIDDEC_ReleaseLastDecodedPicture
Description : This function releases the last decoded picture (pointed by VIDDEC_Data_p->ForTask.QueueParams.Picture_p)
Parameters  : Video decode handle
Assumptions : This function should only be called in VIDDEC context.
Limitations :
Returns     :
*******************************************************************************/
static  ST_ErrorCode_t VIDDEC_ReleaseLastDecodedPicture(const VIDDEC_Handle_t DecodeHandle)
{
    VIDDEC_Data_t *    VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    if (VIDDEC_Data_p->ForTask.QueueParams.Picture_p != NULL)
    {
        /* Release the associated decimated picture (if it exists) */
        if (VIDDEC_Data_p->ForTask.QueueParams.Picture_p->AssociatedDecimatedPicture_p != NULL)
        {
#ifdef TRACE_TAKE_RELEASE
            TraceBuffer(("Release Decim Pict: 0x%x\r\n", VIDDEC_Data_p->ForTask.QueueParams.Picture_p->AssociatedDecimatedPicture_p));
#endif
            RELEASE_PICTURE_BUFFER(VIDDEC_Data_p->BufferManagerHandle, VIDDEC_Data_p->ForTask.QueueParams.Picture_p->AssociatedDecimatedPicture_p, VIDCOM_VIDDEC_MODULE_BASE);
            /* It is not allowed to set AssociatedDecimatedPicture_p to NULL because the link between Main and Decimated Picture Buffers is managed by VID_BUFF */
        }

        /* Release the Main picture */
#ifdef TRACE_TAKE_RELEASE
        TraceBuffer(("Release Pict: 0x%x\r\n", VIDDEC_Data_p->ForTask.QueueParams.Picture_p));
#endif
        RELEASE_PICTURE_BUFFER(VIDDEC_Data_p->BufferManagerHandle, VIDDEC_Data_p->ForTask.QueueParams.Picture_p, VIDCOM_VIDDEC_MODULE_BASE);
        /* Reset Picture_p to ensure that vid_dec doesn't use this pointer now that we have released this picture buffer */
        VIDDEC_Data_p->ForTask.QueueParams.Picture_p = NULL;

        /* It is not possible to launch display anymore now that Picture_p has been reset */
        VIDDEC_Data_p->NextAction.LaunchDisplayWhenPossible = FALSE;
    }

    return(ST_NO_ERROR);
} /* End of VIDDEC_ReleaseLastDecodedPicture() function */

/*******************************************************************************
Name        : VIDDEC_NotifyDecodeEvent
Description : Notify decode event
Parameters  : Video decode handle, event parameters
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if could no notify event
*******************************************************************************/
ST_ErrorCode_t VIDDEC_NotifyDecodeEvent(const VIDDEC_Handle_t DecodeHandle, STEVT_EventID_t EventID, const void * EventData)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    switch (EventID)
    {
        case STVID_DATA_OVERFLOW_EVT :
            STEVT_Notify(((VIDDEC_Data_t *) DecodeHandle)->EventsHandle, ((VIDDEC_Data_t *) DecodeHandle)->RegisteredEventsID[VIDDEC_DATA_OVERFLOW_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST EventData);
            VIDDEC_Data_p->OverFlowEvtAlreadySent = TRUE;
            /* TraceBuffer(("OVERLFW\r\n")); */
            break;

        case STVID_DATA_UNDERFLOW_EVT :
            STEVT_Notify(((VIDDEC_Data_t *) DecodeHandle)->EventsHandle, ((VIDDEC_Data_t *) DecodeHandle)->RegisteredEventsID[VIDDEC_DATA_UNDERFLOW_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST ((const STVID_DataUnderflow_t *) EventData));
            VIDDEC_Data_p->DataUnderflowEventNotSatisfied = TRUE;
            VIDDEC_Data_p->OverFlowEvtAlreadySent = FALSE;
            /* TraceBuffer(("UNDFW\r\n")); */
            break;

        case STVID_DATA_ERROR_EVT :
            STEVT_Notify(((VIDDEC_Data_t *) DecodeHandle)->EventsHandle, ((VIDDEC_Data_t *) DecodeHandle)->RegisteredEventsID[VIDDEC_DATA_ERROR_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
            break;

#ifdef STVID_HARDWARE_ERROR_EVENT
        case STVID_HARDWARE_ERROR_EVT :
            STEVT_Notify(((VIDDEC_Data_t *) DecodeHandle)->EventsHandle, ((VIDDEC_Data_t *) DecodeHandle)->RegisteredEventsID[VIDDEC_HARDWARE_ERROR_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST ((STVID_HardwareError_t *)EventData));
            break;
#endif /* STVID_HARDWARE_ERROR_EVENT */
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

    return(ErrorCode);
} /* End of VIDDEC_NotifyDecodeEvent() function */

#ifdef ST_smoothbackward
/*******************************************************************************
Name        : VIDDEC_IsThereARunningDataInjection
Description :
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
BOOL VIDDEC_IsThereARunningDataInjection(const VIDDEC_Handle_t DecodeHandle)
{
     return (((VIDDEC_Data_t *)DecodeHandle)->DataUnderflowEventNotSatisfied);
}
#endif /* ST_smoothbackward */


#ifdef ST_smoothbackward
/*******************************************************************************
Name        : VIDDEC_ResetTemporalRefCounters
Description : Resume decoding
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if decoder resumed
              STVID_ERROR_DECODER_NOT_PAUSING if the decoder was not paused
*******************************************************************************/
ST_ErrorCode_t VIDDEC_ResetTemporalRefCounters(const VIDDEC_Handle_t DecodeHandle)
{
    VIDDEC_Data_t  * VIDDEC_Data_p;

    VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    VIDDEC_Data_p->StartCodeProcessing.TemporalReferenceOffset = 0;
    VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference = 0;
    VIDDEC_Data_p->StartCodeProcessing.PreviousExtendedTemporalReference = (U32) (-1); /* To start offset at 0 and not 1 (if detects that 0 is used then takes 1) */
    VIDDEC_Data_p->StartCodeProcessing.PreviousGreatestReferenceExtendedTemporalReference = 0;
    VIDDEC_Data_p->StartCodeProcessing.PreviousPictureMPEGFrame = STVID_MPEG_FRAME_I;
    VIDDEC_Data_p->StartCodeProcessing.OneTemporalReferenceOverflowed = FALSE;
    VIDDEC_Data_p->StartCodeProcessing.PreviousTemporalReference = IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE; /* Impossible value */
    VIDDEC_Data_p->StartCodeProcessing.LastReferenceTemporalReference = IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE; /* Impossible value */
    VIDDEC_Data_p->StartCodeProcessing.LastButOneReferenceTemporalReference = IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE; /* Impossible value */
    VIDDEC_Data_p->StartCodeProcessing.PictureCounter = 0;
    VIDDEC_Data_p->StartCodeProcessing.PreviousExtendedTemporalReferenceForPTS = (U32) (-1); /* Used for PTS interpolation */
    VIDDEC_Data_p->StartCodeProcessing.tempRefReachedZero = FALSE;
    return(ST_NO_ERROR);
}
#endif /* ST_smoothbackward */

/*******************************************************************************
Name        : VIDDEC_Resume
Description : Resume decoding
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if decoder resumed
              STVID_ERROR_DECODER_NOT_PAUSING if the decoder was not paused
*******************************************************************************/
ST_ErrorCode_t VIDDEC_Resume(const VIDDEC_Handle_t DecodeHandle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

#ifdef TRACE_DECODE
    TraceBuffer(("-RESUME"));
#endif /* TRACE_DECODE */

    if (!(((VIDDEC_Data_t *) DecodeHandle)->DecoderIsFreezed))
    {
        return(STVID_ERROR_DECODER_NOT_PAUSING);
    }

    ((VIDDEC_Data_t *) DecodeHandle)->DecoderIsFreezed = FALSE;
    if ((((VIDDEC_Data_t *) DecodeHandle)->DecodedPictures) != (((VIDDEC_Data_t *) DecodeHandle)->DecodedPicturesBeforeLastFreeze))
    {
        ErrorCode = VIDDEC_SetDecodedPictures(DecodeHandle, ((VIDDEC_Data_t *) DecodeHandle)->DecodedPicturesBeforeLastFreeze);
    }

    return(ErrorCode);
} /* End of VIDDEC_Resume() function */


#ifdef ST_smoothbackward
/*******************************************************************************
Name        : VIDDEC_SearchNextStartCode
Description : Search start code from a given address
Parameters  : Video decode handle, search mode, TRUE to detect first slice,
              search address for mode "from address"
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t VIDDEC_SearchNextStartCode(const VIDDEC_Handle_t DecodeHandle, const VIDDEC_StartCodeSearchMode_t SearchMode, const BOOL FirstSliceDetect, void * const SearchAddress_p)
{
    VIDDEC_Data_t  * VIDDEC_Data_p;

    VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    UNUSED_PARAMETER(FirstSliceDetect);

    switch (SearchMode)
    {
        case VIDDEC_START_CODE_SEARCH_MODE_NORMAL :
            VIDDEC_Data_p->HWStartCodeDetector.IsIdle = FALSE;
            VIDDEC_Data_p->HWStartCodeDetector.MaxStartCodeSearchTime = time_plus(time_now(), MAX_START_CODE_SEARCH_TIME);
            HALDEC_SearchNextStartCode(VIDDEC_Data_p->HALDecodeHandle, HALDEC_START_CODE_SEARCH_MODE_NORMAL, TRUE, NULL);
            break;

        case VIDDEC_START_CODE_SEARCH_MODE_FROM_ADDRESS :
            VIDDEC_Data_p->HWStartCodeDetector.IsIdle = FALSE;
            VIDDEC_Data_p->HWStartCodeDetector.MaxStartCodeSearchTime = time_plus(time_now(), MAX_START_CODE_SEARCH_TIME);
            HALDEC_SearchNextStartCode(VIDDEC_Data_p->HALDecodeHandle, HALDEC_START_CODE_SEARCH_MODE_FROM_ADDRESS, TRUE, SearchAddress_p);
            break;

        default :
            return(ST_ERROR_BAD_PARAMETER);
            break;
     }
#ifdef TRACE_DECODE
    /*TraceBuffer(("SCD Launched \r\n"));*/
#endif /* TRACE_DECODE */

    return (ST_NO_ERROR);
} /* VIDDEC_SearchNextStartCode() function */
#endif /* ST_smoothbackward */


#ifdef ST_smoothbackward

/*******************************************************************************
Name        : VIDDEC_BackwardModeRequested
Description : This function informs VID_DEC that it should stop and switch to backward mode.
              This operation should be done in VID_DEC context in order to ensure that no decoding
              occurs while we stop VID_DEC.
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDDEC_BackwardModeRequested(const VIDDEC_Handle_t DecodeHandle)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    /* Send a command to the Vid_dec task */
    PushControllerCommand(DecodeHandle, CONTROLLER_COMMAND_SWITCH_TO_BACKWARD_MODE);
    semaphore_signal(VIDDEC_Data_p->DecodeOrder_p);

    return (ST_NO_ERROR);
} /* end of VIDDEC_BackwardModeRequested() function */

/*******************************************************************************
Name        : VIDDEC_SwitchToBackwardMode
Description : This function stop VID_DEC and informs VID_TRICK that it can start.
              It is a synchronous function so it should be called only from the VID_DEC's task.
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
static ST_ErrorCode_t VIDDEC_SwitchToBackwardMode(const VIDDEC_Handle_t DecodeHandle)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

#ifdef TRACE_TAKE_RELEASE
    TraceBuffer(("VIDDEC_SwitchToBackwardMode\r\n"));
#endif

    VIDDEC_ReleaseLastDecodedPicture(DecodeHandle);

    /* Reset references used by VID_DEC */
    VIDDEC_ReleaseReferencePictures(DecodeHandle);

    /* Pictures already send to ORDQ and display will be flushed by VID_TRIC when it starts */
    VIDDEC_SetControlMode(DecodeHandle, VIDDEC_MANUAL_CONTROL_MODE);

    /* Inform VID_TRIC that backward can be started */
    STEVT_Notify(VIDDEC_Data_p->EventsHandle, VIDDEC_Data_p->RegisteredEventsID[VIDDEC_READY_TO_START_BACKWARD_MODE_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);

    return (ST_NO_ERROR);
} /* end of VIDDEC_SwitchToBackwardMode() function */

/*******************************************************************************
Name        : VIDDEC_SetControlMode
Description : Set the control mode to Manual or Automatic. The decode task can be
              controlled by outside in Manual (Trick mode for example).
Parameters  : Video decode handle, boolean.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDDEC_SetControlMode(const VIDDEC_Handle_t DecodeHandle, const VIDDEC_ControlMode_t ControlMode)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    if (ControlMode == ((VIDDEC_Data_t *) DecodeHandle)->ControlMode)
    {
        /* Already in desired mode: nothing to do */
        return(ST_NO_ERROR);
    }

    /* Change control mode */
    ((VIDDEC_Data_t *) DecodeHandle)->ControlMode = ControlMode;

    if (ControlMode == VIDDEC_AUTOMATIC_CONTROL_MODE)
    {
        VIDDEC_Data_p->StartCodeProcessing.TimeCode.Interpolated = TRUE;
        VIDDEC_Data_p->StartCodeProcessing.FirstPictureEver = TRUE;
        VIDDEC_Data_p->StartCodeProcessing.TimeCode.Frames = 0;
        VIDDEC_Data_p->StartCodeProcessing.TimeCode.Seconds = 0;
        VIDDEC_Data_p->StartCodeProcessing.TimeCode.Minutes = 0;
        VIDDEC_Data_p->StartCodeProcessing.TimeCode.Hours = 0;
    }

    /* Signal controller that a new command is given, so that it processes mode change ! */
    semaphore_signal(VIDDEC_Data_p->DecodeOrder_p);

    return(ST_NO_ERROR);
} /* End of VIDDEC_SetControlMode() function */
#endif /* ST_smoothbackward */

#ifdef ST_deblock
/*******************************************************************************
Name        : VIDDEC_SetDeblockingMode
Description : cascaded to hal decode
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void VIDDEC_SetDeblockingMode(const VIDDEC_Handle_t DecodeHandle, const BOOL DeblockingEnabled)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    HALDEC_SetDeblockingMode(VIDDEC_Data_p->HALDecodeHandle, DeblockingEnabled);
} /* End of VIDDEC_SetDeblockingMode() function */
#ifdef VIDEO_DEBLOCK_DEBUG
/*******************************************************************************
Name        : VIDDEC_SetDeblockingStrength
Description : cascaded to hal decode
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void VIDDEC_SetDeblockingStrength(const VIDDEC_Handle_t DecodeHandle, const int DeblockingStrength)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    HALDEC_SetDeblockingStrength(VIDDEC_Data_p->HALDecodeHandle, DeblockingStrength);
} /* End of VIDDEC_SetDeblockingStrength() function */
#endif /* VIDEO_DEBLOCK_DEBUG */
#endif /* ST_deblock */

/*******************************************************************************
Name        : VIDDEC_SetDataInputInterface
Description : cascaded to hal decode
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDDEC_SetDataInputInterface(const VIDDEC_Handle_t DecodeHandle,
        ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle,
                                            void ** const Address_p),
        void (*InformReadAddress)(void * const Handle, void * const Address),
        void * const Handle )
{

    ST_ErrorCode_t ErrorCode;

    ErrorCode = HALDEC_SetDataInputInterface(
                            ((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle,
                            GetWriteAddress,InformReadAddress,Handle );
    return(ErrorCode);
}

/*******************************************************************************
Name        : VIDDEC_SetDecodedPictures
Description : Set decoded pictures
Parameters  : Video decode handle, decoded pictures
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if unsupported mode
*******************************************************************************/
ST_ErrorCode_t VIDDEC_SetDecodedPictures(const VIDDEC_Handle_t DecodeHandle, const STVID_DecodedPictures_t DecodedPictures)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (((VIDDEC_Data_t *) DecodeHandle)->DecoderIsFreezed)
    {
        return (STVID_ERROR_DECODER_FREEZING);
    }

    switch (DecodedPictures)
    {
        case STVID_DECODED_PICTURES_I :
            ((VIDDEC_Data_t *) DecodeHandle)->Skip.AllPPictures = TRUE;
            ((VIDDEC_Data_t *) DecodeHandle)->Skip.AllBPictures = TRUE;
            ((VIDDEC_Data_t *) DecodeHandle)->SearchFirstI = FALSE;
            ((VIDDEC_Data_t *) DecodeHandle)->Skip.AllExceptFirstI = FALSE;
            break;

        case STVID_DECODED_PICTURES_IP :
            ((VIDDEC_Data_t *) DecodeHandle)->Skip.AllPPictures = FALSE;
            ((VIDDEC_Data_t *) DecodeHandle)->Skip.AllBPictures = TRUE;
            ((VIDDEC_Data_t *) DecodeHandle)->SearchFirstI = FALSE;
            ((VIDDEC_Data_t *) DecodeHandle)->Skip.AllExceptFirstI = FALSE;
            break;

        case STVID_DECODED_PICTURES_ALL :
            ((VIDDEC_Data_t *) DecodeHandle)->Skip.AllPPictures = FALSE;
            ((VIDDEC_Data_t *) DecodeHandle)->Skip.AllBPictures = FALSE;
            ((VIDDEC_Data_t *) DecodeHandle)->SearchFirstI = FALSE;
            ((VIDDEC_Data_t *) DecodeHandle)->Skip.AllExceptFirstI = FALSE;
            break;

        case STVID_DECODED_PICTURES_FIRST_I :
            ((VIDDEC_Data_t *) DecodeHandle)->Skip.AllPPictures = TRUE;
            ((VIDDEC_Data_t *) DecodeHandle)->Skip.AllBPictures = TRUE;
            ((VIDDEC_Data_t *) DecodeHandle)->SearchFirstI = TRUE;
            ((VIDDEC_Data_t *) DecodeHandle)->Skip.AllExceptFirstI = TRUE;
            break;

        default :
            /* Unrecognised mode: don't take it into account ! */
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Valid mode: adopt it */
        ((VIDDEC_Data_t *) DecodeHandle)->DecodedPictures = DecodedPictures;
    }

    return(ErrorCode);
} /* End of VIDDEC_SetDecodedPictures() function */


#ifdef ST_smoothbackward
/*******************************************************************************
Name        : VIDDEC_SetDecodeBitBufferInLinearMode
Description : Set a smooth backward linear bit buffer to be flushed
Parameters  : Video decode handle.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
void VIDDEC_SetDecodeBitBufferInLinearMode(const VIDDEC_Handle_t DecodeHandle, void * const LinearBitBufferStartAddress_p, const U32 LinearBitBufferSize)
{
    /* Set bit buffer */
    HALDEC_SetDecodeBitBuffer(((VIDDEC_Data_t *)DecodeHandle)->HALDecodeHandle, LinearBitBufferStartAddress_p, LinearBitBufferSize, HALDEC_BIT_BUFFER_SW_MANAGED_LINEAR);
    ((VIDDEC_Data_t *) DecodeHandle)->LastLinearBitBufferFlushRequestSize = LinearBitBufferSize;
}
#endif /* ST_smoothbackward */

/*******************************************************************************
Name        : VIDDEC_SetErrorRecoveryMode
Description : Set the error recovery mode from the API to the decoder.
Parameters  : Video decode handle, error recovery mode.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDDEC_SetErrorRecoveryMode(const VIDDEC_Handle_t DecodeHandle, const STVID_ErrorRecoveryMode_t ErrorRecoveryMode)
{
    /* Set the error recovery mode. */
    ((VIDDEC_Data_t *) DecodeHandle)->ErrorRecoveryMode = ErrorRecoveryMode;

    return(ST_NO_ERROR);
} /* End of VIDDEC_SetErrorRecoveryMode() function */

#ifdef STVID_STVAPI_ARCHITECTURE
/*******************************************************************************
Name        : VIDDEC_SetExternalRasterBufferManagerPoolId
Description : Stores the Raster buffer manager pool identifier. The decode process
              has to use this buffer pool identifier to get anew buffer to
              decode a new raster picture in.
Parameters  : Video decode handle, buffer pool identifier
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void VIDDEC_SetExternalRasterBufferManagerPoolId(const VIDDEC_Handle_t DecodeHandle, const DVRBM_BufferPoolId_t BufferPoolId)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    /* Store the Raster buffer manager pool identifier */
    VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId = BufferPoolId;
    VIDDEC_Data_p->RasterBufManagerParams.BufferPoolIdIsAvailable = TRUE;
}
#endif /* end of def STVID_STVAPI_ARCHITECTURE */

/*******************************************************************************
Name        : VIDDEC_SetHDPIPParams
Description : Set the HD-PIP threshold.
Parameters  : Video decode handle, HD-PIP parameters to get.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if succes, STVID_ERROR_DECODER_RUNNING if not
              in STOP mode.
*******************************************************************************/
ST_ErrorCode_t VIDDEC_SetHDPIPParams(const VIDDEC_Handle_t DecodeHandle,
                const STVID_HDPIPParams_t * const HDPIPParams_p)
{

    /* Exit now if parameters are bad */
    if (HDPIPParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (((VIDDEC_Data_t *) DecodeHandle)->DecoderState != VIDDEC_DECODER_STATE_STOPPED)
    {
        return(STVID_ERROR_DECODER_RUNNING);
    }

    ((VIDDEC_Data_t *) DecodeHandle)->HDPIPParams = *HDPIPParams_p;

    return(ST_NO_ERROR);

} /* End of VIDDEC_SetHDPIPParams() function. */


/*******************************************************************************
Name        : VIDDEC_SetNextDecodeAction
Description : Set action to do for next decode, to be called in callback of VIDDEC_READY_TO_DECODE_NEW_PICTURE_EVT
Parameters  : Video decode handle, next decode action, eventually pointer where to return nb fields skipped
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDDEC_SetNextDecodeAction(const VIDDEC_Handle_t DecodeHandle, const VIDDEC_DecodeAction_t NextDecodeAction)
{
    /* Set next decode action */
    ((VIDDEC_Data_t *) DecodeHandle)->DecodeAction = NextDecodeAction;

    return(ST_NO_ERROR);
} /* End of VIDDEC_SetNextDecodeAction() function */


/*******************************************************************************
Name        : VIDDEC_Setup
Description : Setup an object
Parameters  : Video decode handle, setup parameters.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER,
              ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t VIDDEC_Setup(const VIDDEC_Handle_t DecodeHandle, const STVID_SetupParams_t * const SetupParams_p)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (SetupParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    switch (SetupParams_p->SetupObject)
    {
        case STVID_SETUP_FDMA_NODES_PARTITION :
            if (!(VIDDEC_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG))
            {
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            }
            ErrorCode = HALDEC_Setup(VIDDEC_Data_p->HALDecodeHandle, SetupParams_p);
            break;

#if defined(DVD_SECURED_CHIP)
    /* DG to do: ES_COPY_BUFFER and ES_COPY_BUFFER_PARTITION objects */
#endif /* DVD_SECURED_CHIP */

        default :
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDDEC_Setup(): Object not supported !"));
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(ErrorCode);
} /* End of VIDDEC_Setup() function. */


/*******************************************************************************
Name        : VIDDEC_SkipData
Description : Skip data in decoder
Parameters  : Video decode handle, skip mode
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if problem
*******************************************************************************/
ST_ErrorCode_t VIDDEC_SkipData(const VIDDEC_Handle_t DecodeHandle, const VIDDEC_SkipMode_t SkipMode)
{
    switch (SkipMode)
    {
        case VIDDEC_SKIP_NEXT_B_PICTURE :
           ((VIDDEC_Data_t *) DecodeHandle)->Skip.NextBPicture = TRUE;
            break;

        case VIDDEC_SKIP_UNTIL_NEXT_I :
           ((VIDDEC_Data_t *) DecodeHandle)->Skip.UntilNextIPicture = TRUE;
            break;

        case VIDDEC_SKIP_UNTIL_NEXT_SEQUENCE :
           ((VIDDEC_Data_t *) DecodeHandle)->Skip.UntilNextSequence = TRUE;
            break;

        default :
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }

    return(ST_NO_ERROR);
} /* End of VIDDEC_SkipData() function */

/*******************************************************************************
Name        : VIDDEC_Start
Description : Start decoding
Parameters  : Video decode handle, start params
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if decoder started
              ST_ERROR_BAD_PARAMETER if bad parameter
              STVID_ERROR_DECODER_RUNNING if the decoder is already started
              STVID_ERROR_DECODER_PAUSING if the decoder is paused
*******************************************************************************/
ST_ErrorCode_t VIDDEC_Start(const VIDDEC_Handle_t DecodeHandle, const VIDDEC_StartParams_t * const StartParams_p)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

#if defined(TRACE_DECODE) || defined(TRACE_TAKE_RELEASE)
    TraceBuffer(("VIDDEC_Start"));
#endif /* TRACE_DECODE || TRACE_TAKE_RELEASE */

    if (StartParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check if decoder was requested to stop but didn't: do stop before re-start */
    if ((VIDDEC_Data_p->Stop.WhenNextReferencePicture) ||
        (VIDDEC_Data_p->Stop.WhenNoData) ||
        (VIDDEC_Data_p->Stop.WhenNextIPicture))
    {
        /* Force stop now ! (but don't send event, API level will do it in that error case !) */
        DoStopNow(DecodeHandle, FALSE);
    }

    /* ! State should be tested by API level, here we just force the execution ! */
/*    switch (VIDDEC_Data_p->DecoderState)*/
/*    {*/
/*        case VIDDEC_DECODER_STATE_DECODING :*/
/*            ErrorCode = STVID_ERROR_DECODER_RUNNING;*/
/*            break;*/

/*        default :*/
/*            ErrorCode = ST_NO_ERROR;*/
/*            break;*/
/*    }*/

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Set PES Parser configuration */
        HALDEC_ResetPESParser(VIDDEC_Data_p->HALDecodeHandle);
        HALDEC_SetStreamType(VIDDEC_Data_p->HALDecodeHandle, StartParams_p->StreamType);
        HALDEC_SetStreamID(VIDDEC_Data_p->HALDecodeHandle, StartParams_p->StreamID);

        VIDDEC_Data_p->StartCodeProcessing.SequenceInfo.StreamID = StartParams_p->StreamID;

        VIDDEC_Data_p->BroadcastProfile = StartParams_p->BroadcastProfile;

        VIDDEC_SetDecodedPictures(DecodeHandle, VIDDEC_Data_p->DecodedPictures);
#ifdef ST_ordqueue
        VIDDEC_Data_p->UpdateDisplayQueueMode = StartParams_p->UpdateDisplayQueueMode;
        if (StartParams_p->UpdateDisplay)
        {
            VIDDEC_Data_p->CurrentUpdateDisplayQueueMode = StartParams_p->UpdateDisplayQueueMode;
        }
        else
        {
            VIDDEC_Data_p->CurrentUpdateDisplayQueueMode = VIDPROD_UPDATE_DISPLAY_QUEUE_NEVER;
        }
#endif  /* end of def ST_ordqueue */
        VIDDEC_Data_p->DecodeOnce                   = StartParams_p->DecodeOnce;
        VIDDEC_Data_p->RealTime                     = StartParams_p->RealTime;

        /* Send this event each first sequence after start */
        VIDDEC_Data_p->SendSequenceInfoEventOnceAfterStart = TRUE;

        VIDDEC_Data_p->ForTask.SoftResetPending = TRUE;
        /* Set state to decoding */
        VIDDEC_Data_p->DecoderState = VIDDEC_DECODER_STATE_DECODING;
        /* Remember that no picture was decoded since the current start */
        VIDDEC_Data_p->HWDecoder.LastDecodedPictureInfos_p = NULL;

        /* After a START, we re-send the underflow event : for that, the flag   */
        /* DataUnderflowEventNotSatisfiedmust be set to FALSE.                  */
        InitUnderflowEvent(DecodeHandle);

        PushControllerCommand(DecodeHandle, CONTROLLER_COMMAND_RESET);
        /* Signal controller that a new command is given: do that as VERY LAST action,
        because as generally the decode task has higher priority than the calling task, it is
        triggered immediately, so all VIDDEC data must be valid before the signal ! */
        semaphore_signal(VIDDEC_Data_p->DecodeOrder_p);
    }

    return(ErrorCode);

} /* End of VIDDEC_Start() function */


/*******************************************************************************
Name        : VIDDEC_StartUpdatingDisplay
Description : Start updating the display
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDDEC_StartUpdatingDisplay(const VIDDEC_Handle_t DecodeHandle)
{
#ifdef ST_ordqueue
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    /* Update the display from now (not if it was already the case) */
    if (VIDDEC_Data_p->CurrentUpdateDisplayQueueMode == VIDPROD_UPDATE_DISPLAY_QUEUE_NEVER)
    {
        VIDDEC_Data_p->CurrentUpdateDisplayQueueMode =  VIDDEC_Data_p->UpdateDisplayQueueMode;
    }
#endif  /* end of def ST_ordqueue */
    return(ST_NO_ERROR);
} /* end of VIDDEC_StartUpdatingDisplay() function */


/*******************************************************************************
Name        : VIDDEC_Stop
Description : Stop decoding
Parameters  : Video decode handle, stop mode
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if decoder stopped
              STVID_ERROR_DECODER_STOPPED if the decoder is already stopped
*******************************************************************************/
ST_ErrorCode_t VIDDEC_Stop(const VIDDEC_Handle_t DecodeHandle, const STVID_Stop_t StopMode, const BOOL StopAfterBuffersTimeOut)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

#if defined(TRACE_DECODE) || defined(TRACE_TAKE_RELEASE)
    TraceBuffer(("VIDDEC_Stop (StopMode)=%d\r\n", StopMode));
#endif /* TRACE_DECODE || TRACE_TAKE_RELEASE */

    if (ErrorCode == ST_NO_ERROR)
    {
        VIDDEC_Data_p->Stop.StopAfterBuffersTimeOut = StopAfterBuffersTimeOut;
        switch (StopMode)
        {
            case STVID_STOP_WHEN_NEXT_REFERENCE :
                VIDDEC_Data_p->Stop.WhenNextReferencePicture = TRUE;
                break;

            case STVID_STOP_WHEN_END_OF_DATA :
                VIDDEC_Data_p->Stop.IsPending = TRUE;
                VIDDEC_Data_p->Stop.WhenNoData = TRUE;
                /* Flush after: otherwise we get discontinuity SC and WhenNoData is still FALSE ! */
                HALDEC_FlushDecoder(VIDDEC_Data_p->HALDecodeHandle, &(VIDDEC_Data_p->Stop.DiscontinuityStartCodeInserted));
                /* Compute the max time before sending the stopped evt, if no picture was decoded for a long time. This
                 * is recovery case if no significant data can be found in the bitbuffer for decoding one picture.
                 * The timeout is the max time to fill in the bitbuffer. */
                VIDDEC_Data_p->Stop.TimeoutForSendingStoppedEvent =
                    time_plus(time_now(), (((osclock_t)VIDDEC_Data_p->BitBuffer.TotalSize)*8*ST_GetClocksPerSecond()) / MIN_BITRATE_VALUE);
                VIDDEC_Data_p->Stop.IsPending = FALSE;
                break;

            case STVID_STOP_WHEN_NEXT_I :
                VIDDEC_Data_p->Stop.WhenNextIPicture = TRUE;
                break;

            case STVID_STOP_NOW :
            default :
                /* Stop now, and don't send STOPPED event */
                DoStopNow(DecodeHandle, FALSE);
                break;
        } /* switch StopMode */

    } /* No error */

    return(ErrorCode);
} /* End of VIDDEC_Stop() function */

#ifdef ST_SecondInstanceSharingTheSameDecoder
/*******************************************************************************
Name        : VIDDEC_StopSecond
Description : Stop decoding
Parameters  : Video decode handle, stop mode
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if decoder stopped
              STVID_ERROR_DECODER_STOPPED if the decoder is already stopped
*******************************************************************************/
ST_ErrorCode_t VIDDEC_StopSecond(const VIDDEC_Handle_t DecodeHandle, const STVID_Stop_t StopMode, const BOOL StopAfterBuffersTimeOut)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
/*    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *)(((VIDPROD_Properties_t *) DecodeHandle)->InternalProducerHandle);*/
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

#ifdef TRACE_DECODE
    TraceBuffer(("-STOPSecond"));
#endif /* TRACE_DECODE */

    VIDDEC_Data_p->StillDecodeParams.DecodeStillRequested = TRUE;

    return(ErrorCode);
} /* end of VIDDEC_StopSecond() function */

/*******************************************************************************
Name        : VIDDEC_TermSecond
Description : Terminate decode, undo all what was done at init
Parameters  : Video decode handle
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_INVALID_HANDLE if not initialised
*******************************************************************************/
ST_ErrorCode_t VIDDEC_TermSecond(const VIDDEC_Handle_t DecodeHandle)
{
    ST_ErrorCode_t ErrorCode;
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    ErrorCode = STEVT_Close(VIDDEC_Data_p->StillDecodeParams.EventsHandle);

    return(ST_NO_ERROR);
} /* end of VIDDEC_Term() function */
#endif /* ST_SecondInstanceSharingTheSameDecoder */

/*******************************************************************************
Name        : VIDDEC_Term
Description : Terminate decode, undo all what was done at init
Parameters  : Video decode handle
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_INVALID_HANDLE if not initialised
*******************************************************************************/
ST_ErrorCode_t VIDDEC_Term(const VIDDEC_Handle_t DecodeHandle)
{
    VIDDEC_Data_t * VIDDEC_Data_p;
    VIDDEC_Handle_t        VIDDEC_Handle;
#ifndef ST_multicodec /* if ST_oldmpeg2codec only */
    VIDPROD_Properties_t * VIDPROD_Data_p;
#endif /* not ST_multicodec */
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Find structure */
#ifndef ST_multicodec /* if ST_oldmpeg2codec only */
    VIDPROD_Data_p = (VIDPROD_Properties_t *) DecodeHandle;
/* TODO_PC (VIDDEC_Term) : Check for ValidityCheck of upper layer here ? */
    VIDDEC_Data_p = (VIDDEC_Data_t *) VIDPROD_Data_p->InternalProducerHandle;
    VIDDEC_Handle = (VIDDEC_Handle_t *) VIDDEC_Data_p;
#else /* not ST_multicodec */
    VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
    VIDDEC_Handle = (VIDDEC_Handle_t *) VIDDEC_Data_p;
#endif /* ST_multicodec */



    if (VIDDEC_Data_p->ValidityCheck != VIDDEC_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    StopDecodeTask(VIDDEC_Handle);
    /* Terminate HAL: no interrupt shall be able to occur after that */
    HALDEC_Term(VIDDEC_Data_p->HALDecodeHandle);

    if (VIDDEC_Data_p->VideoInterrupt.InstallHandler)
    {
        /* Un-install interrupt handler after HAL has been terminated, i.e. after HW is fully de-activated.
        This is to be sure that no interrupt can occur after un-installing the interrupt handler, as recommended in OS20 doc. */
        STOS_InterruptLock();

        if (STOS_InterruptDisable(VIDDEC_Data_p->VideoInterrupt.Number, VIDDEC_Data_p->VideoInterrupt.Level) != STOS_SUCCESS)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Problem disabling interrupt !"));
        }

        if (STOS_InterruptUninstall(VIDDEC_Data_p->VideoInterrupt.Number, VIDDEC_Data_p->VideoInterrupt.Level, NULL) != STOS_SUCCESS)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Problem un-installing interrupt !"));
        }

#ifndef ST_OSWINCE
        /* Under WinCE, not possible to enable an uninstalled interrupt - not a share interrupt. */
        if (STOS_InterruptEnable(VIDDEC_Data_p->VideoInterrupt.Number, VIDDEC_Data_p->VideoInterrupt.Level) != STOS_SUCCESS)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Problem re-enbling interrupt !"));
        }
#endif
        STOS_InterruptUnlock();
    }

    /* De-allocate bit buffer */
    if (VIDDEC_Data_p->BitBufferAllocatedByUs)
    {
        VIDBUFF_DeAllocateBitBuffer(VIDDEC_Data_p->BufferManagerHandle, &(VIDDEC_Data_p->BitBuffer));
/*        VIDDEC_Data_p->BitBufferAllocatedByUs = FALSE;*/ /* Useless, data is lost */
    }

    #if defined(DVD_SECURED_CHIP)
        /* DG TO DO: Deallocate ES Copy buffer, call VIDBUFF_DeAllocateESCopyBuffer() */
    #endif /* DVD_SECURED_CHIP */

    /* Un-subscribe and un-register events: this is done automatically by STEVT_Close() */
    /* Close instances opened at init */
    ErrorCode = STEVT_Close(VIDDEC_Data_p->EventsHandle);

#ifdef ST_avsync
    STCLKRV_Close(VIDDEC_Data_p->VbvStartCondition.STClkrvHandle);
#endif /* ST_avsync */

    /* Destroy decode structure (delete semaphore, etc) */
    TermDecodeStructure(VIDDEC_Handle);

    /* De-allocate memory allocated at init */
    if (VIDDEC_Data_p->UserDataTable_p != NULL)
    {
        SAFE_MEMORY_DEALLOCATE(VIDDEC_Data_p->UserDataTable_p, VIDDEC_Data_p->CPUPartition_p,
                               VIDDEC_Data_p->DecodingStream_p->UserData.TotalSize);
    }
    if (((void *) VIDDEC_Data_p->DecodingStream_p) != NULL)
    {
        SAFE_MEMORY_DEALLOCATE(VIDDEC_Data_p->DecodingStream_p, VIDDEC_Data_p->CPUPartition_p,
                               sizeof(MPEG2BitStream_t));
    }

    STOS_UnmapRegisters(VIDDEC_Data_p->MappedRegistersBaseAddress_p, STVID_MAPPED_WIDTH);
    STOS_UnmapRegisters(VIDDEC_Data_p->MappedCompressedDataInputBaseAddress_p, STVID_MAPPED_WIDTH);

    /* De-validate structure */
    VIDDEC_Data_p->ValidityCheck = 0; /* not VIDDEC_VALID_HANDLE */

    /* De-allocate last: data inside cannot be used after ! */
    SAFE_MEMORY_DEALLOCATE(VIDDEC_Data_p, VIDDEC_Data_p->CPUPartition_p, sizeof(VIDDEC_Data_t));

#ifndef ST_multicodec /* if ST_oldmpeg2codec only */
/* TODO_PC (VIDDEC_Term) : Check for ValidityCheck of upper layer here ? */
    SAFE_MEMORY_DEALLOCATE(VIDPROD_Data_p, VIDPROD_Data_p->CPUPartition_p, sizeof(VIDPROD_Data_t));
#endif /* not ST_multicodec */

    return(ErrorCode);
} /* end of VIDDEC_Term() function */




/* Functions Exported in module --------------------------------------------- */


/*******************************************************************************
Name        : viddec_DecodeHasErrors
Description : Reports error of decode: from IT's or from delay overtime
Parameters  : Video decode handle
Assumptions : To be called in decode error interrupts, or if decode never ending
Limitations : Can be called many time for the same picture: doesn't do job twice
              Must be called when decode is not yet idle (before a pipereset for example !)
Returns     : Nothing
*******************************************************************************/
void viddec_DecodeHasErrors(const VIDDEC_Handle_t DecodeHandle, const VIDDEC_DecodeErrorType_t DecodeErrorType)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
    VIDBUFF_PictureBuffer_t * CurrentPicture_p;
    STVID_PictureParams_t CurrentPictureParams;
    U32 NbOfSyntaxError, NbOfCriticalError, NbOfMisalignmentError;

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Detected decode error !"));
#ifdef TRACE_DECODE
    TraceBuffer(("\r\n%d,DecodeError(%d)", time_now(), HALDEC_GetDecodeBitBufferLevel(((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle)));
#endif /* TRACE_DECODE */

    if (IsHWDecoderIdle(DecodeHandle) && (DecodeErrorType != VIDDEC_DECODE_ERROR_TYPE_MISALIGNMENT ))
    {
        /* May happen if an error is detected at many places (like by 2 error interrupts) */
        return;
    }

    /* We have 3 different behaviours for the 3 kind of decode errors : Syntax, Critical, Misalignment. */
    switch (DecodeErrorType)
    {
        case VIDDEC_DECODE_ERROR_TYPE_SYNTAX :    /* Bad Macroblocs. */
            VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.NbOfSyntaxError++;
            NbOfSyntaxError = VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.NbOfSyntaxError;
#ifdef STVID_DEBUG_GET_STATISTICS
            if (NbOfSyntaxError > VIDDEC_Data_p->Statistics.DecodePbMaxNbInterruptSyntaxErrorPerPicture)
            {
                VIDDEC_Data_p->Statistics.DecodePbMaxNbInterruptSyntaxErrorPerPicture = NbOfSyntaxError;
            }
#endif /* STVID_DEBUG_GET_STATISTICS */

            /* No action to take for this kind of error in PARTIAL and NONE */
            if (IsPictureDecodeHardwareSyntaxErrorToBeCancelled(VIDDEC_Data_p->ErrorRecoveryMode))
            {
                return;
            }
            break;

        case VIDDEC_DECODE_ERROR_TYPE_OVERFLOW :  /* Need a Pipeline Reset */
        case VIDDEC_DECODE_ERROR_TYPE_UNDERFLOW :
        case VIDDEC_DECODE_ERROR_TYPE_TIMEOUT :
            VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.NbOfCriticalError++;
            NbOfCriticalError = VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.NbOfCriticalError;

            /* To recover the Hw, we do a pipereset after an overflow, underflow or  TimeOut. */
            /* Caution: pipeline reset skips decoder to next picture ! */
            HALDEC_PipelineReset(VIDDEC_Data_p->HALDecodeHandle);
            break;

        case VIDDEC_DECODE_ERROR_TYPE_MISALIGNMENT : /* Need a Soft Reset */
            VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.NbOfMisalignmentError++;
            NbOfMisalignmentError = VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.NbOfMisalignmentError;

            /* The Misalignment error is not managed in Error Recovery NONE.                           */
            /* The Misalignment error is not managed in Non Real-Time (play from memory and from HDD). */
            if ((!(IsHardwareMisalignmentErrorToBeCancelled(VIDDEC_Data_p->ErrorRecoveryMode))) && (VIDDEC_Data_p->RealTime))
            {
                VIDDEC_Data_p->ForTask.SoftResetPending = TRUE;
                PushControllerCommand(DecodeHandle, CONTROLLER_COMMAND_RESET);
                /* Signal controller that a new command is given */
                semaphore_signal(VIDDEC_Data_p->DecodeOrder_p);
            }
            else
            {
                return;
            }
            break;
        case VIDDEC_DECODE_ERROR_TYPE_NONE :
        default :
#ifdef TRACE_DECODE
            TraceBuffer(("***BUG***\r\n"));
#endif /* TRACE_DECODE */
            break;
   }

    /* Set decode error flag */
    CurrentPicture_p = VIDDEC_Data_p->HWDecoder.DecodingPicture_p;
    /* Access DecodingPicture_p through CurrentPicture_p in case it is changing ! */

#ifdef TRACE_TAKE_RELEASE
    TraceBuffer(("viddec_DecodeHasErrors Pict 0x%x\r\n", CurrentPicture_p));
#endif

    /* Don't go on  */
    if ((CurrentPicture_p != NULL) && (CurrentPicture_p->Decode.DecodingError != VIDCOM_ERROR_LEVEL_NONE))
    {
        /* The error has already been detected before for this picture. */
        return;
    }

    if (CurrentPicture_p != NULL)
    {
        /* Here Decode.DecodingError is affected to else than NONE */
        switch (DecodeErrorType)
        {
            case VIDDEC_DECODE_ERROR_TYPE_SYNTAX :    /* Bad Macroblocs. */
                CurrentPicture_p->Decode.DecodingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
                break;

            case VIDDEC_DECODE_ERROR_TYPE_OVERFLOW :  /* Need a Pipeline Reset */
            case VIDDEC_DECODE_ERROR_TYPE_UNDERFLOW :
            case VIDDEC_DECODE_ERROR_TYPE_TIMEOUT :
                CurrentPicture_p->Decode.DecodingError = VIDCOM_ERROR_LEVEL_CRASH;
                break;

            case VIDDEC_DECODE_ERROR_TYPE_MISALIGNMENT : /* Need a Soft Reset */
                CurrentPicture_p->Decode.DecodingError = VIDCOM_ERROR_LEVEL_RESET;
                break;

            case VIDDEC_DECODE_ERROR_TYPE_NONE :
            default :
                break;
        } /* end switch */

#ifdef ST_ordqueue
        if (!(IsSpoiledPictureToBeDisplayed(VIDDEC_Data_p->ErrorRecoveryMode)))
        {
            /* Inform that picture is spoiled and should not be displayed. This is usefull if picture was given to display
            at the beginning of decode, such that the result of the decode was not known. */
            VIDQUEUE_CancelPicture(VIDDEC_Data_p->OrderingQueueHandle, &CurrentPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID);

            /* We won't display the picture so decrement the counter to keep it consistent.*/
            if (VIDDEC_Data_p->NextAction.NumberOfDisplayToLaunch > 0)
            {
                VIDDEC_Data_p->NextAction.NumberOfDisplayToLaunch--;
            }
        }
#endif  /* end of def ST_ordqueue */

        /* Inform app. */
        vidcom_PictureInfosToPictureParams(&(CurrentPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos),
             &CurrentPictureParams, CurrentPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID);
        STEVT_Notify(VIDDEC_Data_p->EventsHandle, VIDDEC_Data_p->RegisteredEventsID[VIDDEC_PICTURE_DECODING_ERROR_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(CurrentPictureParams));

        if (CurrentPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame != STVID_MPEG_FRAME_B)
        {
            if (IsSpoiledReferenceLoosingAllReferencesUntilNextI(VIDDEC_Data_p->ErrorRecoveryMode))
            {
#ifdef ST_ordqueue
                /* Flush all pictures in ordering queue to display */
                VIDQUEUE_PushAllPicturesToDisplay(VIDDEC_Data_p->OrderingQueueHandle);
#endif /* ST_ordqueue */

                /* A reference picture is lost: skip every picture until next I */
                VIDDEC_ReleaseReferencePictures(DecodeHandle);
            }

            /* Issue 205 / DDTS GNBvd10382                                                             */
            /* If LaunchDecode is TRUE it means that a picture P2 has been parsed during the decode     */
            /* treatment of CurrentPicture_p P1. Here we know that P1 is a reference and is bad.        */
            /* Several cases :                                                                          */
            /* - P2 is a I picture : nothing to do. P1 will automaticaly be discarded in RECOVERY_FULL. */
            /* - P2 is a P or a B picture : need to skip the P2 picture to behave like the other case   */
            /*   of skip when some references miss, the test should be the same as in function          */
            /*   TaskForAutomaticControlMode : skip B's in all modes except NONE, and skip P's if mode  */
            /*   is not FULL. Otherwise without skipping the P1's frame buffer will be used.            */
            if  ( (VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible ) &&
                 (( (VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B)  && /* Cases of skip for B pictures : */
                    ((VIDDEC_Data_p->ReferencePicture.MissingPrevious )  /* checked but always true */ ||
                     (VIDDEC_Data_p->ReferencePicture.LastButOne_p == NULL))                           &&
                    (!(IsBPictureToBeDecodedIfMissingReference(VIDDEC_Data_p->ErrorRecoveryMode))) ) ||
                  ( (VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_P)  && /* Cases of skip for B pictures : */
                    (VIDDEC_Data_p->ReferencePicture.MissingPrevious )  /* checked but always true */  &&
                    (!(IsPPictureToBeDecodedIfMissingReference(VIDDEC_Data_p->ErrorRecoveryMode))) ) ))
            {
                VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible = FALSE;
                VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible = TRUE;
            }
        } /* end picture not a B */
    } /* end CurrentPicture_p != NULL */

    /* Here is a mechanism to prevent a hardware defect. If we get a lot of pictures with errors then we reset the hw decoder */
    /* (soft reset). But as the PARTIAL error recovery mode is more permissive on the errors than the FULL, the reset is      */
    /* launched on 2 different amount of errors.                                                                              */

    /* One more picture with an error */
    VIDDEC_Data_p->UnsuccessfullDecodeCount ++;

    /* Is level of amount of errors is reached ? */
    if (
        ((VIDDEC_Data_p->UnsuccessfullDecodeCount >= ACCEPTANCE_ERROR_AMOUNT_LOWEST_ERROR_RECOVERY) &&
         (VIDDEC_Data_p->ErrorRecoveryMode != STVID_ERROR_RECOVERY_NONE)) ||
        ((VIDDEC_Data_p->UnsuccessfullDecodeCount >= ACCEPTANCE_ERROR_AMOUNT_HIGHEST_ERROR_RECOVERY) &&
         (VIDDEC_Data_p->ErrorRecoveryMode == STVID_ERROR_RECOVERY_FULL))
       )
    {
#ifdef TRACE_DECODE
        /*TraceBuffer(("\r\nDecodeCountSoftRt\r\n"));*/
#endif /* TRACE_DECODE */
        VIDDEC_Data_p->UnsuccessfullDecodeCount = 0;
        VIDDEC_Data_p->ForTask.SoftResetPending = TRUE;
        PushControllerCommand(DecodeHandle, CONTROLLER_COMMAND_RESET);
        /* Signal controller that a new command is given */
        semaphore_signal(VIDDEC_Data_p->DecodeOrder_p);
    }

} /* End of viddec_DecodeHasErrors() function */

/*******************************************************************************
Name        : viddec_HWDecoderEndOfDecode
Description : Actions to take at the end of a decoding
              Caution: DecodingPicture_p is NULL after !
Parameters  : Video decode handle
Assumptions : To be called as soon as decode of picture is finished
Limitations : Should not be called inside VIDDEC_Xxxx() function as it notifies an event outside !
Returns     : Nothing
*******************************************************************************/
void viddec_HWDecoderEndOfDecode(const VIDDEC_Handle_t DecodeHandle)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
#ifndef STVID_STVAPI_ARCHITECTURE
    STVID_PictureInfos_t ExportedPictureInfos;
#else
    STVID_NewDecodedPictureEvtParams_t ExportedPictureInfos;
#endif
    VIDBUFF_PictureBuffer_t * AssociatedPicture_p;

    if (VIDDEC_Data_p->HWDecoder.HWState == VIDDEC_HARDWARE_STATE_DECODING)
    {

#ifdef TRACE_DECODE
        TraceBuffer(("Pict %c%d decoded in 0x%x\r\n",
                MPEGFrame2Char(VIDDEC_Data_p->HWDecoder.DecodingPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame),
                VIDDEC_Data_p->HWDecoder.DecodingPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                VIDDEC_Data_p->HWDecoder.DecodingPicture_p));
#endif
        /* There was a decode on-going: if allowed by the user, inform outside world that a new picture has been decoded. */
        if ( viddec_IsEventToBeNotified (DecodeHandle, VIDDEC_NEW_PICTURE_DECODED_EVT_ID) )
        {
#ifndef STVID_STVAPI_ARCHITECTURE
            HALDEC_GetPictureDescriptors((((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle),
&(VIDDEC_Data_p->HWDecoder.DecodingPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.PictureDescriptors));

            ExportedPictureInfos = VIDDEC_Data_p->HWDecoder.DecodingPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos;

            ExportedPictureInfos.BitmapParams.Data1_p = Virtual(ExportedPictureInfos.BitmapParams.Data1_p);
            ExportedPictureInfos.BitmapParams.Data2_p = Virtual(ExportedPictureInfos.BitmapParams.Data2_p);

            /* Setting Decimation Data before sending the event : */
            AssociatedPicture_p = VIDDEC_Data_p->HWDecoder.DecodingPicture_p->AssociatedDecimatedPicture_p;
            if(AssociatedPicture_p != NULL)
            {
                /* Setting the right DecimationFactors before sending the event */
                ExportedPictureInfos.VideoParams.DecimationFactors
                        = AssociatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimationFactors;

                ExportedPictureInfos.VideoParams.DecimatedBitmapParams.Data1_p = Virtual(ExportedPictureInfos.VideoParams.DecimatedBitmapParams.Data1_p);
                ExportedPictureInfos.VideoParams.DecimatedBitmapParams.Data2_p = Virtual(ExportedPictureInfos.VideoParams.DecimatedBitmapParams.Data2_p);
            }
#else
            ExportedPictureInfos.MainTopPictureId =    VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.MainTopFieldPictureId;
            ExportedPictureInfos.MainBottomPictureId = VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.MainBottomFieldPictureId;
            ExportedPictureInfos.MainRepeatPictureId = VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPictureId.MainRepeatFieldPictureId;
            ExportedPictureInfos.DecimatedTopPictureId = STGVOBJ_kForbiddenPictureId;
            ExportedPictureInfos.DecimatedBottomPictureId = STGVOBJ_kForbiddenPictureId;
            ExportedPictureInfos.DecimatedRepeatPictureId = STGVOBJ_kForbiddenPictureId;
            ExportedPictureInfos.BufferPoolId = VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId;
#endif /* end of def STVID_STVAPI_ARCHITECTURE */
            STEVT_Notify(VIDDEC_Data_p->EventsHandle,
                    VIDDEC_Data_p->RegisteredEventsID[VIDDEC_NEW_PICTURE_DECODED_EVT_ID],
                    STEVT_EVENT_DATA_TYPE_CAST &(ExportedPictureInfos));
        }
#ifdef STVID_STVAPI_ARCHITECTURE
        viddec_UnuseRasterPictures(VIDDEC_Data_p);
#endif

        /* Now consider picture under decode. */
        if (VIDDEC_Data_p->HWDecoder.DecodingPicture_p != NULL)
        {
            /* There was a known picture under decode */
            if (VIDDEC_Data_p->HWDecoder.DecodingPicture_p->Decode.DecodingError == VIDCOM_ERROR_LEVEL_NONE)
            {
                /* Initialize the counter of decode errors: the decode has been successfull or we have done a skip. */
                VIDDEC_Data_p->UnsuccessfullDecodeCount = 0;
            }
        } /* end picture under decode */
    } /* end harware decoding */

    /* Update decoder state */
    VIDDEC_Data_p->HWDecoder.HWState = VIDDEC_HARDWARE_STATE_IDLE;

    /* Update picture under decode information */
    VIDDEC_Data_p->HWDecoder.DecodingPicture_p = NULL;

#ifdef TRACE_DECODE
    /*TraceBuffer(("EndOfDecode=%d \r\n", time_minus(time_now(), LaunchDecode[DecodeIndex])));*/
    if (DecodeIndex > 0)
    {
        FinishedDecode[DecodeIndex - 1] = time_now();
    }
#endif /* TRACE_DECODE */

#ifdef TRACE_DECODE
/*    TraceBuffer(("\r\n%d,EndOfDecode(%d)", time_now(), HALDEC_GetDecodeBitBufferLevel(((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle)));*/
#endif /* TRACE_DECODE */
} /* End of viddec_HWDecoderEndOfDecode() function */


/*******************************************************************************
Name        : viddec_HWDecoderFoundNextPicture
Description : Actions to take when decoder is idle : found the next picture
              start code.
Parameters  : Video decode handle
Assumptions : To be called as soon as decoder is Idle
Limitations :
Returns     : Nothing
*******************************************************************************/
void viddec_HWDecoderFoundNextPicture(const VIDDEC_Handle_t DecodeHandle)
{
    /* Update frame buffer information */
    ((VIDDEC_Data_t *) DecodeHandle)->HWDecoder.HWState = VIDDEC_HARDWARE_STATE_FOUND_NEXT_PICTURE;

#ifdef ST_smoothbackward
    if (((VIDDEC_Data_t *) DecodeHandle)->ControlMode == VIDDEC_MANUAL_CONTROL_MODE)
    {
        STEVT_Notify(((VIDDEC_Data_t *) DecodeHandle)->EventsHandle, ((VIDDEC_Data_t *) DecodeHandle)->RegisteredEventsID[VIDDEC_DECODER_IDLE_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
    }
#endif /* ST_smoothbackward */

    /* !!! To go try to decode next pic as soon as previous decode is over, while
     waiting for a command (otherwise blocked until timeout of command wait) */
    /* Signal controller that a new command is given */
    semaphore_signal(((VIDDEC_Data_t *) DecodeHandle)->DecodeOrder_p);

} /* End of viddec_HWDecoderFoundNextPicture() function */


/*******************************************************************************
Name        : viddec_HWDecoderStartOfDecode
Description : HW reports that decode has began
Parameters  : Video decode handle
Assumptions : To be called in decode interrupt
Limitations :
Returns     : Nothing
*******************************************************************************/
void viddec_HWDecoderStartOfDecode(const VIDDEC_Handle_t DecodeHandle)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
#ifdef TRACE_DECODE
    /*TraceBuffer(("EndOfDecode=%d \r\n", time_minus(time_now(), LaunchDecode[DecodeIndex])));*/
    if (DecodeIndex > 0)
    {
        InterruptPSD[DecodeIndex - 1] = time_now();
    }
#endif /* TRACE_DECODE */
#ifdef TRACE_DECODE
    /*TraceBuffer(("\r\n%d,StartDecode(%d)", time_now(), HALDEC_GetDecodeBitBufferLevel(((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle)));*/
#endif /* TRACE_DECODE */
    /* Now decode effectively starts: monitor real decode time. Time-out occuring from now means that the decoder is hanged without notice (error IT's) */
    switch (VIDDEC_Data_p->DecodingStream_p->SequenceExtension.profile_and_level_indication & PROFILE_AND_LEVEL_IDENTIFICATION_MASK)
    {
        case (PROFILE_MAIN | LEVEL_HIGH) : /* MPHL */
        case (PROFILE_MAIN | LEVEL_HIGH_1440) : /* MP HL1440 */
            VIDDEC_Data_p->HWDecoder.MaxDecodeOrSkipTime = time_plus(time_now(), MAX_DECODE_TIME_HD);
            break;

        case (PROFILE_MAIN | LEVEL_MAIN) : /* MPML */
        default :
            VIDDEC_Data_p->HWDecoder.MaxDecodeOrSkipTime = time_plus(time_now(), MAX_DECODE_TIME_SD);
            break;
    }
} /* End of viddec_HWDecoderStartOfDecode() function */


/*******************************************************************************
Name        : viddec_IsEventToBeNotified
Description : Test if the corresponding event is to be notified (enabled,  ...)
Parameters  : Decoder handle,
              event ID (defined in vid_dec.h).
Assumptions : Event_ID is supposed to be valid.
Limitations :
Returns     : Boolean, indicating if the event can be notified.
*******************************************************************************/
BOOL viddec_IsEventToBeNotified (const VIDDEC_Handle_t DecodeHandle, const U32 Event_ID)
{
    BOOL RetValue = FALSE;
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    STVID_ConfigureEventParams_t * ConfigureEventParams_p;
    U32                          * NotificationSkipped_p;

    /* Get the event configuration. */
    ConfigureEventParams_p = &VIDDEC_Data_p->EventNotificationConfiguration[Event_ID].ConfigureEventParam;
    NotificationSkipped_p  = &VIDDEC_Data_p->EventNotificationConfiguration[Event_ID].NotificationSkipped;

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

} /* End of viddec_IsEventToBeNotified() function. */


/*******************************************************************************
Name        : viddec_SetBitBufferThresholdToIdleValue
Description : Sets bit buffer threshold to its idle value, i.e. the value 'nearly full'
              of the bit buffer, used when not looking for vbv_level.
              This function is called at each softreset, and after the first BBF occurs
Parameters  : Decoder handle
Assumptions : Handle is valid
Limitations :
Returns     : Nothing
*******************************************************************************/
void viddec_SetBitBufferThresholdToIdleValue(const VIDDEC_Handle_t DecodeHandle)
{
    /* Set bit buffer threshold near the full size (just below) */
    HALDEC_SetDecodeBitBufferThreshold(((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle,
        ((VIDDEC_Data_t *) DecodeHandle)->BitBuffer.TotalSize - VIDDEC_BIT_BUFFER_THRESHOLD_IDLE_VALUE - 1);
} /* end of viddec_SetBitBufferThresholdToIdleValue() function */


/*******************************************************************************
Name        : viddec_UnMaskBitBufferFullInterrupt
Description : Unmask bit buffer full interrupt in interrupt mask
Parameters  : Decoder handle
Assumptions : Handle is valid
Limitations : Caution: as this modifies the VideoInterrupt.Mask variable, only call this function inside decode task, to avoid spoiling by multi-access !
Returns     : Nothing
*******************************************************************************/
void viddec_UnMaskBitBufferFullInterrupt(const VIDDEC_Handle_t DecodeHandle)
{
    /* If the flag was there, disable it. Otherwise no need to clear it, the less we read/write this variable, the best it is... */
#if defined TRACE_DECODE || defined STVID_DEBUG_GET_STATISTICS
    /* Still keep this IT for debug cases */
    return;
#endif /* TRACE_DECODE || STVID_DEBUG_GET_STATISTICS */
    if ((((VIDDEC_Data_t *) DecodeHandle)->VideoInterrupt.Mask & HALDEC_BIT_BUFFER_NEARLY_FULL_MASK) != 0)
    {
        /* If flag was there, disable it. Otherwise no need to clear it, the less we read/write this variable, the best it is... */
        ((VIDDEC_Data_t *) DecodeHandle)->VideoInterrupt.Mask &= ~HALDEC_BIT_BUFFER_NEARLY_FULL_MASK;
        HALDEC_SetInterruptMask(((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle, ((VIDDEC_Data_t *) DecodeHandle)->VideoInterrupt.Mask);
    }
} /* end of viddec_UnMaskBitBufferFullInterrupt() function */

/* Private Functions -------------------------------------------------------- */

/*******************************************************************************
Name        : CheckAndProcessInterrupt
Description : Process interrupt the sooner if there is one
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void CheckAndProcessInterrupt(const VIDDEC_Handle_t DecodeHandle)
{
    U32 InterruptStatus;

    while (PopInterruptCommand(DecodeHandle, &InterruptStatus) == ST_NO_ERROR)
    {
        viddec_ProcessInterrupt(DecodeHandle, InterruptStatus);
    }

#ifdef STVID_DEBUG_GET_STATISTICS
    if (((VIDDEC_Data_t *) DecodeHandle)->InterruptsBuffer.MaxUsedSize >= ((VIDDEC_Data_t *) DecodeHandle)->InterruptsBuffer.TotalSize)
    {
        /* If MaxUsedSize has gone over TotalSize, this is for ever. So don't increment for ever: increment only if 0 or if UsedSize is over TotalSize */
        if ((((VIDDEC_Data_t *) DecodeHandle)->Statistics.DecodePbInterruptQueueOverflow == 0) ||
            (((VIDDEC_Data_t *) DecodeHandle)->InterruptsBuffer.UsedSize >= ((VIDDEC_Data_t *) DecodeHandle)->InterruptsBuffer.TotalSize))
        {
            ((VIDDEC_Data_t *) DecodeHandle)->Statistics.DecodePbInterruptQueueOverflow ++;
        }
    }
#endif /* STVID_DEBUG_GET_STATISTICS */

} /* End of CheckAndProcessInterrupt() function */

/*******************************************************************************
Name        : ClearAllPendingStartCodes
Description : Clears start codes pending in queue of start code commands
              To be used at SoftReset, so that eventual previous start codes don't perturb new start
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void ClearAllPendingStartCodes(const VIDDEC_Handle_t DecodeHandle)
{
    U8   StartCode;

    while (PopStartCodeCommand(DecodeHandle, &StartCode) == ST_NO_ERROR)
    {
        ; /* Nothing to do, just pop all start code pushed... */
    }
} /* End of ClearAllPendingStartCodes() function */


/*******************************************************************************
Name        : ClearAllPendingInterrupt
Description : Clears interrupt pending in queue of interrupt status
              To be used at start or SoftReset, so that eventual previous interrupt don't perturb new start
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void ClearAllPendingInterrupt(const VIDDEC_Handle_t DecodeHandle)
{
    U32 InterruptStatus;

    while (PopInterruptCommand(DecodeHandle, &InterruptStatus) == ST_NO_ERROR)
    {
        ; /* Nothing to do, just pop all interrupt status pushed... */
    }
} /* End of ClearAllPendingInterrupt() function */


/*******************************************************************************
Name        : ClearAllPendingControllerCommands
Description : Clears commands pending in queue of controller commands
              To be used at SoftReset, so that eventual previous commandsdon't perturb new start
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void ClearAllPendingControllerCommands(const VIDDEC_Handle_t DecodeHandle)
{
    U8   ControllerCommand;

    while (PopControllerCommand(DecodeHandle, &ControllerCommand) == ST_NO_ERROR)
    {
        ; /* Nothing to do, just pop all start code pushed... */
    }
} /* End of ClearAllPendingControllerCommands() function */


/*******************************************************************************
Name        : DecodePicture
Description : Launch decode of a picture and start code search:
              - set quant matrix and set change flags to FALSE in DecodeParams_p
             - set decoder config
            - set decoder reference buffers
           - set decoder output buffers
          - launch decode
         - fill PictureInfos fields BitmapParams.Data1_p, Size1, Data2_p, Size2
        - update references
       - copy picture information
Parameters  : Video decode handle, decoding parameters
Assumptions : DecodeParams_p is not NULL, DecodeParams_p->OutputPicture_p is not NULL
              DecodeParams_p->PictureInfos_p is not NULL
              DecodeParams_p->PictureStreamInfos_p is not NULL
              DecodeParams_p->StreamInfoForDecode_p is not NULL
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static void DecodePicture(const VIDDEC_Handle_t DecodeHandle,
        VIDDEC_DecodePictureParams_t * const DecodeParams_p,
        BOOL * const OverWrittenDisplay_p)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
    const StreamInfoForDecode_t * const CurrentStreamInfoForDecode_p = DecodeParams_p->StreamInfoForDecode_p;
    VIDBUFF_FrameBuffer_t * CurrentFrame_p;
    VIDBUFF_PictureBuffer_t *   ForwardReference_p;  /* Earlier time reference picture */
    VIDBUFF_PictureBuffer_t *   BackwardReference_p; /* Future time reference picture */
    BOOL DecodingOnDisplayedFrameBuffer;
    HALDEC_Handle_t HALDecodeHandle;
    BOOL MainDecodeOverDisplay, SecondaryDecodeOverDisplay;
    STVID_DecimationFactor_t CurrentDecimatedFrameDecimationFactor = STVID_DECIMATION_FACTOR_NONE;
    BOOL OutputPicturePatched;
    STOS_Clock_t                InitialTime;

#ifdef DECODE_DEBUG /* debug help if three buffers only */
    STVID_MemoryProfile_t ThreeBuffersDebugMemoryProfile;
#endif /* DECODE_DEBUG */

#ifdef TRACE_DECODE
    TraceBuffer(("DecodePicture\r\n"));
#endif

    /* HAL decode handle */
    HALDecodeHandle = VIDDEC_Data_p->HALDecodeHandle;

    /* Load quant matrix before decode if necessary */
#ifdef STI7015_STI5528_WA_GNBvd10059
    if ((VIDDEC_Data_p->DeviceType != STVID_DEVICE_TYPE_7015_MPEG) && (VIDDEC_Data_p->DeviceType != STVID_DEVICE_TYPE_5528_MPEG))
    {
#endif /* STI7015_STI5528_WA_GNBvd10059 */
        if (CurrentStreamInfoForDecode_p->IntraQuantMatrixHasChanged)
        {
            /* Always load intra quant matrix */
            HALDEC_LoadIntraQuantMatrix(HALDecodeHandle, CurrentStreamInfoForDecode_p->LumaIntraQuantMatrix_p, CurrentStreamInfoForDecode_p->ChromaIntraQuantMatrix_p);
            DecodeParams_p->StreamInfoForDecode_p->IntraQuantMatrixHasChanged = FALSE;
        }
        if (CurrentStreamInfoForDecode_p->NonIntraQuantMatrixHasChanged)
        {
            /* Only load non-intra quant matrix if decoding a non-intra picture.
            This seems to be better for the hardware, because non-intra matrix are taken into account only for decode of non-I pictures */
            /* Normal case for everybody */
            if (DecodeParams_p->PictureInfos_p->VideoParams.MPEGFrame != STVID_MPEG_FRAME_I)
            {
                HALDEC_LoadNonIntraQuantMatrix(HALDecodeHandle, CurrentStreamInfoForDecode_p->LumaNonIntraQuantMatrix_p, CurrentStreamInfoForDecode_p->ChromaNonIntraQuantMatrix_p);
                DecodeParams_p->StreamInfoForDecode_p->NonIntraQuantMatrixHasChanged = FALSE;
            }
        }
#ifdef STI7015_STI5528_WA_GNBvd10059
    }
    else
    {
        /* For STi7015 bug: always load all quant matrixes */
        if ((CurrentStreamInfoForDecode_p->IntraQuantMatrixHasChanged) ||
            (CurrentStreamInfoForDecode_p->NonIntraQuantMatrixHasChanged))
        {
            /* Always load intra and non-intra quant matrix */
            HALDEC_LoadIntraQuantMatrix(HALDecodeHandle, CurrentStreamInfoForDecode_p->LumaIntraQuantMatrix_p, CurrentStreamInfoForDecode_p->ChromaIntraQuantMatrix_p);
            DecodeParams_p->StreamInfoForDecode_p->IntraQuantMatrixHasChanged = FALSE;
            HALDEC_LoadNonIntraQuantMatrix(HALDecodeHandle, CurrentStreamInfoForDecode_p->LumaNonIntraQuantMatrix_p, CurrentStreamInfoForDecode_p->ChromaNonIntraQuantMatrix_p);
            DecodeParams_p->StreamInfoForDecode_p->NonIntraQuantMatrixHasChanged = FALSE;
        }
    }
#endif /* STI7015_STI5528_WA_GNBvd10059 */

    /* Set decoder parameters */
    HALDEC_SetDecodeConfiguration(HALDecodeHandle, CurrentStreamInfoForDecode_p);

    /* Choose references */
    if (DecodeParams_p->PictureInfos_p->VideoParams.MPEGFrame == STVID_MPEG_FRAME_B)
    {
        /* References for a B picture: backward and forward prediction */
        BackwardReference_p = VIDDEC_Data_p->ReferencePicture.Last_p;
        ForwardReference_p  = VIDDEC_Data_p->ReferencePicture.LastButOne_p;
    }
    else /*(MPEGFrame != STVID_MPEG_FRAME_B)*/
    {
        /* References for a reference picture: only forward prediction,
            Also usefull for I for automatic error concealment decoder function */
        BackwardReference_p = NULL;
        if ((DecodeParams_p->PictureInfos_p->VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_FRAME) ||
            (CurrentStreamInfoForDecode_p->ExpectingSecondField))
        {
            /* Frame or first field: take last reference */
            ForwardReference_p = VIDDEC_Data_p->ReferencePicture.Last_p;
        }
        else
        {
            /* Second field: don't take first field as reference ! */
            ForwardReference_p = VIDDEC_Data_p->ReferencePicture.LastButOne_p;
        }
    }
    /* Set reference buffers */
    if (BackwardReference_p == NULL)
    {
        /* No reference: refer to itself rather to old last reference set... */
        if(DecodeParams_p->OutputPicture_p != NULL)
        {
            CurrentFrame_p = DecodeParams_p->OutputPicture_p->FrameBuffer_p;
        }
        else
        {
            CurrentFrame_p = DecodeParams_p->OutputDecimatedPicture_p->FrameBuffer_p;
        }
    }
    else
    {
        CurrentFrame_p = BackwardReference_p->FrameBuffer_p;
    }
    HALDEC_SetBackwardLumaFrameBuffer(HALDecodeHandle, CurrentFrame_p->Allocation.Address_p);
    HALDEC_SetBackwardChromaFrameBuffer(HALDecodeHandle, CurrentFrame_p->Allocation.Address2_p);

    if (ForwardReference_p == NULL)
    {
        /* No reference: refer to itself rather to old last reference set... */
        if(DecodeParams_p->OutputPicture_p != NULL)
        {
            CurrentFrame_p = DecodeParams_p->OutputPicture_p->FrameBuffer_p;
        }
        else
        {
            CurrentFrame_p = DecodeParams_p->OutputDecimatedPicture_p->FrameBuffer_p;
        }
    }
    else
    {
        CurrentFrame_p = ForwardReference_p->FrameBuffer_p;
    }
    HALDEC_SetForwardLumaFrameBuffer(HALDecodeHandle, CurrentFrame_p->Allocation.Address_p);
    HALDEC_SetForwardChromaFrameBuffer(HALDecodeHandle, CurrentFrame_p->Allocation.Address2_p);

    /* Set output buffers : Primary frame buffers. */
    if(DecodeParams_p->OutputPicture_p != NULL)
    {
        CurrentFrame_p = DecodeParams_p->OutputPicture_p->FrameBuffer_p;
#ifdef STVID_STVAPI_ARCHITECTURE
        if (DecodeParams_p->PictureInfos_p->VideoParams.MPEGFrame != STVID_MPEG_FRAME_B)
        {
#endif /* end of def STVID_STVAPI_ARCHITECTURE */
            HALDEC_SetDecodeLumaFrameBuffer(HALDecodeHandle, CurrentFrame_p->Allocation.Address_p);
            HALDEC_SetDecodeChromaFrameBuffer(HALDecodeHandle, CurrentFrame_p->Allocation.Address2_p);
            HALDEC_EnablePrimaryDecode(HALDecodeHandle);
#ifdef STVID_STVAPI_ARCHITECTURE
        }
        else
        {
            HALDEC_DisablePrimaryDecode(HALDecodeHandle);
        }
        /* if picture is a frame or a top, then only the top field picture parameters are enough to be written in the registers. */
        if (DecodeParams_p->PictureInfos_p->VideoParams.PictureStructure != STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)
        {
            /* the register wants half the value of the pitch */
            HALDEC_SetPrimaryRasterReconstructionLumaFrameBuffer(HALDecodeHandle,
                        VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainTopFieldPicture_p->Data1.pBaseAddress,
                        (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainTopFieldPicture_p->Data1.ulPitch / 2));
            HALDEC_SetPrimaryRasterReconstructionChromaFrameBuffer(HALDecodeHandle,
                        VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainTopFieldPicture_p->Data2.pBaseAddress);
        }
        else
        {
            /* retrieve pitch / 2 to the start address of the bottom frame buffers */
            HALDEC_SetPrimaryRasterReconstructionLumaFrameBuffer(HALDecodeHandle,
                        (void *)((U32)(VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainBottomFieldPicture_p->Data1.pBaseAddress)
                        - (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainBottomFieldPicture_p->Data1.ulPitch / 2)),
                        (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainBottomFieldPicture_p->Data1.ulPitch / 2));
            HALDEC_SetPrimaryRasterReconstructionChromaFrameBuffer(HALDecodeHandle,
                        (void *)((U32)VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainBottomFieldPicture_p->Data2.pBaseAddress
                        - (VIDDEC_Data_p->RasterBufManagerParams.ToBeDecodedRasterPicture.MainBottomFieldPicture_p->Data1.ulPitch / 2)));
        }
        HALDEC_EnablePrimaryRasterReconstruction(HALDecodeHandle);
#endif /* end of def STVID_STVAPI_ARCHITECTURE */

        /* Test if HD-PIP is to be activated.      */
        if (DecodeParams_p->PictureInfos_p->BitmapParams.BitmapType
                == STGXOBJ_BITMAP_TYPE_MB_HDPIP)
        {
            HALDEC_EnableHDPIPDecode(HALDecodeHandle,
                    &VIDDEC_Data_p->HDPIPParams);
        }
        else
        {
            HALDEC_DisableHDPIPDecode(HALDecodeHandle,
                    &VIDDEC_Data_p->HDPIPParams);
        }
    }
    else
    {
        HALDEC_DisablePrimaryDecode(HALDecodeHandle);
#ifdef STVID_STVAPI_ARCHITECTURE
        HALDEC_DisablePrimaryRasterReconstruction(DecodeHandle);
#endif /* end of def STVID_STVAPI_ARCHITECTURE */
    }
    /* Set output buffers : Secondary frame buffers (only if required). */
    if(DecodeParams_p->OutputDecimatedPicture_p != NULL)
    {
        CurrentFrame_p = DecodeParams_p->OutputDecimatedPicture_p->FrameBuffer_p;
        HALDEC_SetSecondaryDecodeLumaFrameBuffer(HALDecodeHandle, CurrentFrame_p->Allocation.Address_p);
        HALDEC_SetSecondaryDecodeChromaFrameBuffer(HALDecodeHandle, CurrentFrame_p->Allocation.Address2_p);
        CurrentDecimatedFrameDecimationFactor = CurrentFrame_p->DecimationFactor;

        if ((DecodeParams_p->PictureInfos_p->BitmapParams.Width > 720)||
                (DecodeParams_p->PictureInfos_p->BitmapParams.Height > 576))  /* HD, do not take into account the DEBLOCK_RECONSTRUCTION; */
        {
            CurrentDecimatedFrameDecimationFactor = CurrentFrame_p->DecimationFactor &~ STVID_MPEG_DEBLOCK_RECONSTRUCTION;
        }

#if defined(FORCE_SECONDARY_ON_AUX_DISPLAY)
        /* Force secondary reconstruction with H&V decim factor = 1 to support dual display when main decode is located in LMI_VID
         * (decimated frame buffers shall be allocated in LMI_SYS and sized to 720x576 minimum) */
        if (DecodeParams_p->StreamInfoForDecode_p->HorizontalSize <= 720)
        {
            CurrentDecimatedFrameDecimationFactor&=~(STVID_DECIMATION_FACTOR_H2|STVID_DECIMATION_FACTOR_H4);
        }
        if (DecodeParams_p->StreamInfoForDecode_p->VerticalSize <= 576)
        {
            CurrentDecimatedFrameDecimationFactor&=~(STVID_DECIMATION_FACTOR_V2|STVID_DECIMATION_FACTOR_V4);
        }
#endif /* defined(FORCE_SECONDARY_ON_AUX_DISPLAY) */

        /* Initialize the decoder to decode secondary buffers too. */
        if (CurrentStreamInfoForDecode_p->MPEG1BitStream)
        {
            /* Concerning MPEG1 streams, only PROGRESSIVE SCAN is supported. */
            HALDEC_EnableSecondaryDecode(HALDecodeHandle,
                    CurrentDecimatedFrameDecimationFactor & ~(STVID_DECIMATION_FACTOR_V2
                                                        |STVID_DECIMATION_FACTOR_V4),
                    CurrentDecimatedFrameDecimationFactor & ~(STVID_DECIMATION_FACTOR_H2
                                                        |STVID_DECIMATION_FACTOR_H4),
                    STVID_SCAN_TYPE_PROGRESSIVE);
#ifdef STVID_STVAPI_ARCHITECTURE
            HALDEC_EnableSecondaryRasterReconstruction(HALDecodeHandle,
                    CurrentDecimatedFrameDecimationFactor & ~(STVID_DECIMATION_FACTOR_V2
                                                        |STVID_DECIMATION_FACTOR_V4),
                    CurrentDecimatedFrameDecimationFactor & ~(STVID_DECIMATION_FACTOR_H2
                                                        |STVID_DECIMATION_FACTOR_H4),
                    STVID_SCAN_TYPE_PROGRESSIVE);
#endif /* end of def STVID_STVAPI_ARCHITECTURE */
        }
        else
        {
            /* Concerning MPEG2 streams, it depends on of picture coding extension. */
            HALDEC_EnableSecondaryDecode(HALDecodeHandle,
                    CurrentDecimatedFrameDecimationFactor & ~(STVID_DECIMATION_FACTOR_V2
                                                        |STVID_DECIMATION_FACTOR_V4),
                    CurrentDecimatedFrameDecimationFactor & ~(STVID_DECIMATION_FACTOR_H2
                                                        |STVID_DECIMATION_FACTOR_H4),
                    (CurrentStreamInfoForDecode_p->PictureCodingExtension.progressive_frame? STVID_SCAN_TYPE_PROGRESSIVE : STVID_SCAN_TYPE_INTERLACED));
#ifdef STVID_STVAPI_ARCHITECTURE
            HALDEC_EnableSecondaryRasterReconstruction(HALDecodeHandle,
                    CurrentDecimatedFrameDecimationFactor & ~(STVID_DECIMATION_FACTOR_V2
                                                        |STVID_DECIMATION_FACTOR_V4),
                    CurrentDecimatedFrameDecimationFactor & ~(STVID_DECIMATION_FACTOR_H2
                                                        |STVID_DECIMATION_FACTOR_H4),
                    (CurrentStreamInfoForDecode_p->PictureCodingExtension.progressive_frame? STVID_SCAN_TYPE_PROGRESSIVE : STVID_SCAN_TYPE_INTERLACED));
#endif /* end of def STVID_STVAPI_ARCHITECTURE */
        }
    }
    else
    {
        /* Otherwise, no need to decode in secondary frame buffers. */
        HALDEC_DisableSecondaryDecode(HALDecodeHandle);
#ifdef STVID_STVAPI_ARCHITECTURE
        HALDEC_DisableSecondaryRasterReconstruction(HALDecodeHandle);
#endif /* enf of def STVID_STVAPI_ARCHITECTURE */
    }

    /* Patch OutputPicture_p if was null to continue properly */
    OutputPicturePatched = FALSE;  /* Init */
    if(DecodeParams_p->OutputPicture_p == NULL)
    {
        DecodeParams_p->OutputPicture_p = DecodeParams_p->OutputDecimatedPicture_p;
        DecodeParams_p->OutputDecimatedPicture_p = NULL;
        OutputPicturePatched = TRUE;     /* To set the right DecimationFactor when patching had occured */
    }

    /* Determine if decode takes place on displayed picture */
    DecodingOnDisplayedFrameBuffer = FALSE; /* Init */
    SecondaryDecodeOverDisplay = FALSE;
    MainDecodeOverDisplay = FALSE;

    /* Has the Decode to overwrite the display (decode on a picture on screen). */
    /* TRUE if one picture buffer is on display between FirstPictureBuffer,     */
    /* SecondPictureBuffer for Primary decode and Secondary decode (4 cases).   */
    if(DecodeParams_p->OutputDecimatedPicture_p != NULL)
    {
        if (DecodeParams_p->OutputDecimatedPicture_p->FrameBuffer_p != NULL/*zxg 20090917 add NULL check*/ && DecodeParams_p->OutputDecimatedPicture_p->FrameBuffer_p->HasDecodeToBeDoneWhileDisplaying)
        {
            DecodingOnDisplayedFrameBuffer = TRUE;
            SecondaryDecodeOverDisplay = TRUE;
        }
    }
    if (DecodeParams_p->OutputPicture_p->FrameBuffer_p != NULL /*zxg 20090917 add NULL check*/&& DecodeParams_p->OutputPicture_p->FrameBuffer_p->HasDecodeToBeDoneWhileDisplaying)
    {
        DecodingOnDisplayedFrameBuffer = TRUE;
        MainDecodeOverDisplay = TRUE;
    }

#ifdef TRACE_DECODE
    TraceBuffer(("-DEC(%c%d%c) on %c%c OVW=%c, %c\n",(DecodeParams_p->PictureInfos_p->VideoParams.MPEGFrame == STVID_MPEG_FRAME_B?'B':
                             ((DecodeParams_p->PictureInfos_p->VideoParams.MPEGFrame == STVID_MPEG_FRAME_P?'P':'I'))),
                             DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                             DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                             (DecodeParams_p->PictureInfos_p->VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD?'T':
                             ((DecodeParams_p->PictureInfos_p->VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD?'B':'F'))),
                             (DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B?'B':
                             ((DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_P?'P':'I'))),
                             (DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD?'T':
                             ((DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD?'B':'F'))),
                             (DecodingOnDisplayedFrameBuffer?'y':'n')
    ));
#endif /* TRACE_DECODE */

    /* return the value */
    *OverWrittenDisplay_p = DecodingOnDisplayedFrameBuffer;

#ifdef TRACE_DECODE
    /* Debug traces */
    FrameBuffs[FrameBuffsIndex] = (U32) DecodeParams_p->OutputPicture_p->FrameBuffer_p->Allocation.Address_p;
    PicTypes[FrameBuffsIndex] = CurrentStreamInfoForDecode_p->Picture.picture_coding_type;
    if (FrameBuffsIndex < (sizeof(FrameBuffs) / sizeof(U32)) - 1)
    {
        FrameBuffsIndex ++;
    }
/*    if (DecodeParams_p->OutputPicture_p->FrameBuffer_p->Allocation.Address_p == CurrentFrameOnDisplay)*/
/*    {*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Decode B on same display field !"));*/
/*    }*/
#endif /* TRACE_DECODE */

    /* Suppose decode is OK, if not OK set in decode error interrupts */
    DecodeParams_p->OutputPicture_p->Decode.DecodingError = VIDCOM_ERROR_LEVEL_NONE;

    /* Attest that a decode is on-going */
    VIDDEC_Data_p->HWDecoder.DecodingPicture_p = DecodeParams_p->OutputPicture_p;

    /* Set address of decode */
    /* Empty function for STi5510/12, it enables to decode anywhere on STi5508/18/7015/20 */
    /* !!! */
/*    HALDEC_SetDecodeAddress(HALDecodeHandle, DecodeParams_p->DataInputAddress_p);*/

    /* Set the main buffer compression. */
    CurrentFrame_p = DecodeParams_p->OutputPicture_p->FrameBuffer_p;
    HALDEC_SetMainDecodeCompression(HALDecodeHandle, CurrentFrame_p->CompressionLevel);

    /* Monitor decode time: first monitor time before decoder effectively starts decode.
    Time-out occuring from now means that the hardware decoder is never scheduled to do our decode ! */
    InitialTime = time_now();
    VIDDEC_Data_p->HWDecoder.MaxDecodeOrSkipTime = time_plus(InitialTime, MAX_TIME_BEFORE_DECODE_START);

    /* The state of the decode is DECODING (rmq : in Smooth Backward, we are already in this mode since VIDDEC_DecodePicture has been called). */
    VIDDEC_Data_p->HWDecoder.HWState = VIDDEC_HARDWARE_STATE_DECODING;

#ifdef ST_smoothbackward
    if (VIDDEC_Data_p->ControlModeInsideHAL == VIDDEC_AUTOMATIC_CONTROL_MODE)
    {
#endif /*ST_smoothbackward*/
        /* Launching decode also launches SC detector in automatic mode ! */
        VIDDEC_Data_p->HWStartCodeDetector.IsIdle = FALSE;
        VIDDEC_Data_p->HWStartCodeDetector.MaxStartCodeSearchTime = time_plus(InitialTime, MAX_START_CODE_SEARCH_TIME);
#ifdef ST_smoothbackward
    }
#endif /*ST_smoothbackward*/

#ifdef DECODE_DEBUG /* debug help if three buffers only */
    VIDBUFF_GetMemoryProfile(VIDDEC_Data_p->BufferManagerHandle, &ThreeBuffersDebugMemoryProfile);
    if ((VIDDEC_Data_p->Debug.LastDecodedPictureMpegFrame == STVID_MPEG_FRAME_B) && (ThreeBuffersDebugMemoryProfile.PrimaryFrameBufferNumber == 3) &&
        (DecodeParams_p->PictureInfos_p->VideoParams.MPEGFrame == STVID_MPEG_FRAME_B))
    {
        if (!DecodingOnDisplayedFrameBuffer)
        {
            /* should not happen except when redecoding the same stream a second time : */
            /* a stream can finish by P2-B0-B1, and then becaus of CD fifo, display is B0-P2 */
            /* and B1 is decoded only when re-looping, and over an UNLOCKED buffer. */
            /* DecodeError(); */
        }
        if (DecodeParams_p->OutputPicture_p->PictureInfos_p->VideoParams.MPEGFrame != STVID_MPEG_FRAME_B)
        {
            /* Last minute test : panic error if the OutputPicture_p selected for the overwrite mode decode was not a B picture ! */
            DecodeError();
        }
    }

    VIDDEC_Data_p->Debug.LastDecodedPictureMpegFrame = DecodeParams_p->PictureInfos_p->VideoParams.MPEGFrame;
#endif /* DECODE_DEBUG */

#ifdef TRACE_DECODE
        /*TraceBuffer(("$$Dc:M%d Mo%d So%d$$", DecodeParams_p->PictureInfos_p->VideoParams.MPEGFrame, MainDecodeOverDisplay, SecondaryDecodeOverDisplay));*/
#endif
    /* Launch HW decode */
    HALDEC_StartDecodePicture(HALDecodeHandle, FALSE, MainDecodeOverDisplay, SecondaryDecodeOverDisplay);

#ifdef STVID_DEBUG_GET_STATISTICS
    VIDDEC_Data_p->Statistics.DecodePictureDecodeLaunched ++;
#endif /* STVID_DEBUG_GET_STATISTICS */

    /* Finish copy of picture information (info that is known when picture buffer is known) */
    DecodeParams_p->PictureInfos_p->BitmapParams.Data1_p = DecodeParams_p->OutputPicture_p->FrameBuffer_p->Allocation.Address_p;
    DecodeParams_p->PictureInfos_p->BitmapParams.Size1   = DecodeParams_p->OutputPicture_p->FrameBuffer_p->Allocation.TotalSize -
                                                           DecodeParams_p->OutputPicture_p->FrameBuffer_p->Allocation.Size2;
    DecodeParams_p->PictureInfos_p->BitmapParams.Data2_p = DecodeParams_p->OutputPicture_p->FrameBuffer_p->Allocation.Address2_p;
    DecodeParams_p->PictureInfos_p->BitmapParams.Size2   = DecodeParams_p->OutputPicture_p->FrameBuffer_p->Allocation.Size2;
    /* !!! ? Size2 = (Height * Width / 2)  i.e. size of chroma buffer (0.5 byte/pixel)  */
    DecodeParams_p->PictureInfos_p->VideoParams.CompressionLevel = CurrentFrame_p->CompressionLevel;
    /* GGi (04 Sept 2002) Multi-Layer management.                                       */
    /* Initialize the decimation factor of the currently decoded picture.               */
    /* NB: This field MUST NOT BE MODIFIED by any process, otherwise multi-layer feature*/
    /* won't work fine any more. In fact, this field is indicating the decimation       */
    /* allowed by the video memory profile, but not the decimation used to display this */
    /* picture. This field may change ONLY if the memory profile has changed.           */
    DecodeParams_p->PictureInfos_p->VideoParams.DecimationFactors = ( OutputPicturePatched == FALSE ?
        CurrentFrame_p->DecimationFactor : CurrentDecimatedFrameDecimationFactor );

    /* Set IsNonDecimExist flag */
    DecodeParams_p->PictureInfos_p->VideoParams.DoesNonDecimatedExist = (!OutputPicturePatched);
    DecodeParams_p->PictureInfos_p->PictureBufferHandle = (STVID_PictureBufferHandle_t) DecodeParams_p->OutputPicture_p;
    DecodeParams_p->PictureInfos_p->DecimatedPictureBufferHandle = (STVID_PictureBufferHandle_t) DecodeParams_p->OutputDecimatedPicture_p;

    /* Setting Decimated BitmapParams: */
    if(DecodeParams_p->OutputDecimatedPicture_p != NULL)
    {
        DecodeParams_p->PictureInfos_p->VideoParams.DecimatedBitmapParams.Data1_p
                = DecodeParams_p->OutputDecimatedPicture_p->FrameBuffer_p->Allocation.Address_p;
        DecodeParams_p->PictureInfos_p->VideoParams.DecimatedBitmapParams.Size1
                = DecodeParams_p->OutputDecimatedPicture_p->FrameBuffer_p->Allocation.TotalSize -
                  DecodeParams_p->OutputDecimatedPicture_p->FrameBuffer_p->Allocation.Size2;
        DecodeParams_p->PictureInfos_p->VideoParams.DecimatedBitmapParams.Data2_p
                = DecodeParams_p->OutputDecimatedPicture_p->FrameBuffer_p->Allocation.Address2_p;
        DecodeParams_p->PictureInfos_p->VideoParams.DecimatedBitmapParams.Size2
                = DecodeParams_p->OutputDecimatedPicture_p->FrameBuffer_p->Allocation.Size2;
    }

    /* Copy picture information into picture structure */
    DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos = *(DecodeParams_p->PictureInfos_p);
/*    DecodeParams_p->OutputPicture_p->AdditionalStreamInfo = *(DecodeParams_p->PictureStreamInfo_p);*/

    /* decimated picture infos... */
    if(DecodeParams_p->OutputDecimatedPicture_p != NULL)
    {
        DecodeParams_p->OutputDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos
                = *(DecodeParams_p->PictureInfos_p);
        /* Patch the decimation in the generic picture infos to keep the display chain consistent */
        DecodeParams_p->OutputDecimatedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.DecimationFactors
                = CurrentDecimatedFrameDecimationFactor;
    }
    /* Remember picture info for STVID_GetPictureParams() function */
    /* !!! ? point to picture buffer's PictureInfos, or to a static one ??? */
    VIDDEC_Data_p->HWDecoder.LastDecodedPictureInfos_p = &(DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos);
    VIDDEC_Data_p->HWDecoder.LastPresentationOrderPictureID = DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID;

    /* Update references list if picture is a new reference (can be done as soon as
    ReferencePicture.LastButOne_p and ReferencePicture.Last_p are not used any more in this function) */
    if (DecodeParams_p->PictureInfos_p->VideoParams.MPEGFrame != STVID_MPEG_FRAME_B)
    {
        /* New reference ! */
        VIDDEC_NewReferencePicture(DecodeHandle, DecodeParams_p->OutputPicture_p);
    }

    /* Notification of beginning of decode advise outside world.   */
    STEVT_Notify(VIDDEC_Data_p->EventsHandle, VIDDEC_Data_p->RegisteredEventsID[VIDDEC_DECODE_LAUNCHED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);

#ifdef TRACE_DECODE
    /*TraceBuffer(("\r\n-DEC(%c%d/%d%c) in %c%d%c",(DecodeParams_p->PictureInfos_p->VideoParams.MPEGFrame == STVID_MPEG_FRAME_B?'B':
                             ((DecodeParams_p->PictureInfos_p->VideoParams.MPEGFrame == STVID_MPEG_FRAME_P?'P':'I'))),
                             DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                             DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                             (DecodeParams_p->PictureInfos_p->VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD?'T':
                             ((DecodeParams_p->PictureInfos_p->VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD?'B':'F'))),
                             (DecodeParams_p->OutputPicture_p->PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B?'B':
                             ((DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_P?'P':'I'))),
                             DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                             (DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD?'T':
                             ((DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD?'B':'F')))
    ));*/
    /*TraceBuffer(("\r\n-DEC(%c%d/%d%c:%s[%4d;%4d])",(DecodeParams_p->PictureInfos_p->VideoParams.MPEGFrame == STVID_MPEG_FRAME_B?'B':
                             ((DecodeParams_p->PictureInfos_p->VideoParams.MPEGFrame == STVID_MPEG_FRAME_P?'P':'I'))),
                             DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                             DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                             (DecodeParams_p->PictureInfos_p->VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD?'T':
                             ((DecodeParams_p->PictureInfos_p->VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD?'B':'F'))),
                             (DecodeParams_p->PictureInfos_p->BitmapParams.BitmapType==STGXOBJ_BITMAP_TYPE_MB ? "MB" : "HDPIP"),
                             DecodeParams_p->PictureInfos_p->BitmapParams.Width,
                             DecodeParams_p->PictureInfos_p->BitmapParams.Height));*/
    /*TraceBuffer(("@=0x%x,bref=0x%x,fref=0x%x",
                             CurrentFrame_p->Allocation.Address_p,
                             (VIDDEC_Data_p->ReferencePicture.Last_p == NULL?0:VIDDEC_Data_p->ReferencePicture.Last_p->FrameBuffer_p->Allocation.Address_p),
                             (VIDDEC_Data_p->ReferencePicture.LastButOne_p == NULL?0:VIDDEC_Data_p->ReferencePicture.LastButOne_p->FrameBuffer_p->Allocation.Address_p)
    ));*/
#endif /* TRACE_DECODE */
#ifdef TRACE_DECODE
    /*if (!(DecodeParams_p->PictureInfos_p->VideoParams.PTS.Interpolated))
    {
        TraceBuffer((",PTS=%d", DecodeParams_p->PictureInfos_p->VideoParams.PTS.PTS));
    }
    else
    {
        TraceBuffer((",PTSInterpolated=%d", DecodeParams_p->PictureInfos_p->VideoParams.PTS.PTS));
    } */
#endif /* TRACE_DECODE */

#ifdef TRACE_DECODE
#ifdef TRACE_DECODE
/*    TraceBuffer(("\rpic=%x fra=%x tref=%d fwd=%x bwd=%x [time=%d]\r\n", DecodeParams_p->OutputPicture_p, DecodeParams_p->OutputPicture_p->FrameBuffer_p, DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID, ForwardReference_p, BackwardReference_p, InitialTime));*/
#endif
/*    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "pic=%x fra=%x tref=%d fwd=%x bwd=%x", DecodeParams_p->OutputPicture_p, DecodeParams_p->OutputPicture_p->FrameBuffer_p, DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID, ForwardReference_p, BackwardReference_p));*/
    LaunchDecode[DecodeIndex] = InitialTime;
    if (DecodeIndex < (sizeof(LaunchDecode) / sizeof(clock_t)))
    {
        DecodeIndex ++;
    }
    {
        if (DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID == StopAtPic)
        {
            /* For breakpoint */
            StopAtPic = 2 * StopAtPic - DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID;
/*            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Decoding picture %d !", PictureStreamInfo.ExtendedPresentationOrderPictureID.ID));*/
        }
    }
    PicTempRef[FrameBuffsIndex] = (U32) DecodeParams_p->OutputPicture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID;
#endif /* TRACE_DECODE */

#ifdef STVID_MEASURES
    DecodeCounter ++;
#endif /* STVID_MEASURES */
} /* End of DecodePicture() function */

#ifdef ST_smoothbackward
/*******************************************************************************
Name        : PrepareDecode
Description : Set the VLD with the address where to decode.
Parameters  : Video decode handle, decoding parameters
Assumptions : DecodeParams_p is not NULL
              Function only called in MANUAL.
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
static void PrepareDecode(const VIDDEC_Handle_t DecodeHandle, VIDDEC_DecodePictureParams_t * const DecodeParams_p)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
    HALDEC_Handle_t HALDecodeHandle;
    BOOL WaitVLD = TRUE;

    /* HAL decode handle */
    HALDecodeHandle = VIDDEC_Data_p->HALDecodeHandle;

    /* Prepare decode */
    HALDEC_PrepareDecode(HALDecodeHandle, DecodeParams_p->DataInputAddress_p, DecodeParams_p->pTemporalReference, &WaitVLD);

    if (!WaitVLD)
    {
        VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible = TRUE;
    }

} /* End of PrepareDecode() function */
#endif /* ST_smoothbackward */


/*******************************************************************************
Name        : EvaluateBitRateValue
Description : Compute Bit Rate Value
Parameters  : DecodeHanlde,
Assumptions : should be called every time a picture is to be decoded or skipped (when Trick mode is not requiring HDD jumps)
Limitations :
Returns     : Nothing
*******************************************************************************/
static void EvaluateBitRateValue(const VIDDEC_Handle_t DecodeHandle, STVID_PictureInfos_t * const PictureInfos_p)
{
    VIDDEC_Data_t * VIDDEC_Data_p;
    BitRateValueEvaluate_t * BitRate_p;
    U32 BitBufferOutputCounter, DifWithLastBitBufferOutputCounter;

    /* Find structures */
    VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
    BitRate_p = &(VIDDEC_Data_p->BitRateValueEvaluate);

    /* If Trick Mode is requiring HDD jumps, Bit Rate value is not computable */
    if (!(BitRate_p->BitRateValueIsComputable))
    {
        return;
    }

    /* Take bit buffer counter */
    BitBufferOutputCounter = HALDEC_GetBitBufferOutputCounter(VIDDEC_Data_p->HALDecodeHandle);
    if (PictureInfos_p->VideoParams.FrameRate != 0)
    {
        if(!BitRate_p->BitRateValueIsUnderComputing)
        {
            BitRate_p->BitRateValueIsUnderComputing = TRUE;
        }
        else
        {
            if (BitBufferOutputCounter < BitRate_p->LastBitBufferOutputCounter)
            {
                DifWithLastBitBufferOutputCounter = BitBufferOutputCounter + (0xFFFFFFFF - BitRate_p->LastBitBufferOutputCounter);
            }
            else
            {
                DifWithLastBitBufferOutputCounter = BitBufferOutputCounter - BitRate_p->LastBitBufferOutputCounter;
            }
            /* store the data in current index */
            BitRate_p->BitBufferSize[BitRate_p->BitRateIndex] = DifWithLastBitBufferOutputCounter;
            BitRate_p->Time[BitRate_p->BitRateIndex]          = (((VIDDEC_Data_p->NbFieldsDisplayNextDecode * 1000 * VIDDEC_TIME_FACTOR)
                                                               / PictureInfos_p->VideoParams.FrameRate) / 2);
                                                               /* * 1000 : Frame Rate: convert from mHz to Hz */
                                                               /* * VIDDEC_TIME_FACTOR : keep accuracy before div */
                                                                /* /2 : Frame->fields */

            /* index of stored values management */
            BitRate_p->BitRateIndex++;
            if (BitRate_p->BitRateIndex == VIDDEC_NB_PICTURES_USED_FOR_BITRATE)
            {
                BitRate_p->BitRateIndex = 0;
            }
            /* available stored values management */
            if (BitRate_p->BitRateValidValueNb < VIDDEC_NB_PICTURES_USED_FOR_BITRATE)
            {
                BitRate_p->BitRateValidValueNb++;
            }
        } /* endif Bitrate under computing */
    }/* endif  FrameRate != 0 */
    /* prepare next call */
    BitRate_p->LastBitBufferOutputCounter  = BitBufferOutputCounter;

} /* End of EvaluateBitRateValue() function */


/*******************************************************************************
Name        : DecodeTaskFunc
Description : Function fo the decode task
              Set VIDDEC_Data_p->DecodeTask.ToBeDeleted to end the task
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void DecodeTaskFunc(const VIDDEC_Handle_t DecodeHandle)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
    BOOL IsThereOrders = FALSE;

    STOS_TaskEnter(NULL);

    VIDDEC_Data_p->ForTask.PreviousStartCode = SEQUENCE_ERROR_CODE; /* Value should not be used in actions for previous start code */
    VIDDEC_Data_p->ForTask.TryingToRecoverFromMaxDecodeTimeOverflow = FALSE;
    VIDDEC_Data_p->ForTask.NoDisplayAfterNextDecode = FALSE;
    VIDDEC_Data_p->ForTask.SoftResetPending = FALSE;
    VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.HasProfileErrors = FALSE;
    VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.HasCriticalSequenceSyntaxErrors = FALSE;
    VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.HasRecoveredSequenceSyntaxErrors = FALSE;
    VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.FirstPictureOfSequence = FALSE;
    VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.HasPictureSyntaxErrors = FALSE;
    VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.NbOfPictureSCPerFirstSliceSC = 0;
    VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.PreviousPictureHasAMissingField = FALSE;
    VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.NbOfSyntaxError = 0;
    VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.NbOfCriticalError = 0;
    VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.NbOfMisalignmentError = 0;
    VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.DecodeErrorType = VIDDEC_DECODE_ERROR_TYPE_NONE;
    VIDDEC_Data_p->ForTask.SyntaxError.HasStartCodeParsingError = FALSE;
    VIDDEC_Data_p->ForTask.NewPictureDecodeOrSkipEventData.PictureInfos_p = NULL;
#ifdef ST_ordqueue
    VIDDEC_Data_p->ForTask.QueueParams.Picture_p = NULL;
    VIDDEC_Data_p->ForTask.QueueParams.CurrentDecodeForwardReferenceExtendedPresentationOrderPictureID.ID             = 0;
    VIDDEC_Data_p->ForTask.QueueParams.CurrentDecodeForwardReferenceExtendedPresentationOrderPictureID.IDExtension    = 0;
    VIDDEC_Data_p->ForTask.QueueParams.CurrentDecodeForwardReferenceExtendedPresentationOrderPictureIDIsValid         = FALSE;
    VIDDEC_Data_p->ForTask.QueueParams.CurrentDecodeExpectingSecondField                                              = FALSE;
    VIDDEC_Data_p->ForTask.QueueParams.LastParsedReferenceExtendedPresentationOrderPictureID.ID          = 0;
    VIDDEC_Data_p->ForTask.QueueParams.LastParsedReferenceExtendedPresentationOrderPictureID.IDExtension = 0;
    VIDDEC_Data_p->ForTask.QueueParams.LastParsedReferenceExtendedPresentationOrderPictureIDIsValid      = FALSE;
    VIDDEC_Data_p->ForTask.QueueParams.PictureToPushAfterCurrentDecodeExtendedPresentationOrderPictureID.ID          = 0;
    VIDDEC_Data_p->ForTask.QueueParams.PictureToPushAfterCurrentDecodeExtendedPresentationOrderPictureID.IDExtension = 0;
    VIDDEC_Data_p->ForTask.QueueParams.PictureToPushAfterCurrentDecodeExtendedPresentationOrderPictureIDIsValid      = FALSE;
    VIDDEC_Data_p->ForTask.QueueParams.InsertionOrder = VIDQUEUE_INSERTION_ORDER_INCREASING;
    VIDDEC_Data_p->ForTask.NewPictureDecodeOrSkipEventData.NbFieldsForDisplay = 0;
#endif  /* end ST_ordqueue */
    VIDDEC_Data_p->ForTask.TemporalReferenceOffset = 0;

    /* Big loop executed until ToBeDeleted is set --------------------------- */
    do
    { /* while not ToBeDeleted */

        /* Monitor HAL missing interrupts (problems) ! */
        MonitorAbnormallyMissingHALStatus(DecodeHandle);

        /* At each loop: first check if there are interrupts to process. */
        CheckAndProcessInterrupt(DecodeHandle);

        if ((VIDDEC_Data_p->DecoderState == VIDDEC_DECODER_STATE_STOPPED) && (!(VIDDEC_Data_p->DecodeTask.ToBeDeleted)))
        {
            /* Decoder stopped: there is no monitoring to do, so semaphore waits infinitely.
            This avoid task switching not really needed when video stopped. */

            semaphore_wait(VIDDEC_Data_p->DecodeOrder_p);
            /* VIDDEC_Data_p->DecodeOrder_p may has been woken up by StopDecodeTask. Thus VIDDEC_Data_p->DecodeTask.ToBeDeleted */
            /* is TRUE and IsThereOrders Automatic or manual Sub function should not be called any more. */
            if (!VIDDEC_Data_p->DecodeTask.ToBeDeleted)
            {
                /* Set flag to process SW decode orders as the semaphore was triggered by an order */
                IsThereOrders = TRUE;
            }
            /* signals on DecodeOrder_p may come from tasks, API or IT handlers, reset multiple signals as we have treated one */
            while (semaphore_wait_timeout(VIDDEC_Data_p->DecodeOrder_p, TIMEOUT_IMMEDIATE) == 0)
            {
                /* Nothing to do: just to decrement counter until 0, to avoid multiple looping on semaphore wait */;
            }
        }
        else
        {
            /* Decoder running: semaphore has normal time-out to do polling of actions even if semaphore was not signaled. */
            VIDDEC_Data_p->ForTask.MaxWaitOrderTime = time_plus(time_now(), MAX_WAIT_ORDER_TIME);

            /* Check if an order is available */
            if (semaphore_wait_timeout(VIDDEC_Data_p->DecodeOrder_p, &(VIDDEC_Data_p->ForTask.MaxWaitOrderTime)) == 0)
            {
                /* Set flag to process SW decode orders as the semaphore was triggered by an order */
                IsThereOrders = TRUE;
                /* signals on DecodeOrder_p may come from tasks, API or IT handlers, reset multiple signals as we have treated one */
                while (semaphore_wait_timeout(VIDDEC_Data_p->DecodeOrder_p, TIMEOUT_IMMEDIATE) == 0)
                {
                    /* Nothing to do: just to decrement counter until 0, to avoid multiple looping on semaphore wait */;
                }
            }
            else
            {
                /* Timeout: no SW decode orders. Monitor HAL missing interrupts (problems) ! */
                if (MonitorAbnormallyMissingHALStatus(DecodeHandle))
                {
                    /* At each loop: first check if there are interrupts to process. */
                    CheckAndProcessInterrupt(DecodeHandle);
                    /* Set flag to process SW decode orders as the semaphore was triggered by an order */
                    IsThereOrders = TRUE;
                }
                else
                {
                    /* Timeout and no missing interrupts: no SW decode orders */
                    IsThereOrders = FALSE;
                }
            }
        }

#ifdef ST_smoothbackward
        if ((VIDDEC_Data_p->ControlModeInsideHAL != VIDDEC_Data_p->ControlMode) && (VIDDEC_Data_p->DecoderState != VIDDEC_DECODER_STATE_STOPPED))
        {
            /* Control mode is changing: SoftReset needed to configure new mode */
            VIDDEC_Data_p->ForTask.SoftResetPending = TRUE;
            PushControllerCommand(DecodeHandle, CONTROLLER_COMMAND_RESET);
            /* Set flag to process SW decode orders as there is now an order */
            IsThereOrders = TRUE;
        }
#endif /* ST_smoothbackward */

        if (!VIDDEC_Data_p->DecodeTask.ToBeDeleted)
        {
#ifdef ST_smoothbackward
            if (VIDDEC_Data_p->ControlMode == VIDDEC_AUTOMATIC_CONTROL_MODE)
            {
#endif /* ST_smoothbackward */
                DecodeTaskFuncForAutomaticControlMode(DecodeHandle, IsThereOrders);
#ifdef ST_smoothbackward
            }
            if (VIDDEC_Data_p->ControlMode == VIDDEC_MANUAL_CONTROL_MODE) /*&& (VIDDEC_Data_p->HWDecoder.HWState == VIDDEC_HARDWARE_STATE_FOUND_NEXT_PICTURE))*/
            {
                DecodeTaskFuncForManualControlMode(DecodeHandle, IsThereOrders);
            }
#endif /* ST_smoothbackward */
        }   /* if CallTask */
    } while (!(VIDDEC_Data_p->DecodeTask.ToBeDeleted));

    STOS_TaskExit(NULL);
} /* End of DecodeTaskFunc() function */


/*******************************************************************************
Name        : DecodeTaskFuncForAutomaticControlMode
Description : Function fo the decode task
              Set VIDDEC_Data_p->DecodeTask.ToBeDeleted to end the task
Parameters  : Video decode handle
              IsThereOrders: Set TRUE to process all the decode orders
                             Set FALSE to just monitor HW (no SW orders processed)
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void DecodeTaskFuncForAutomaticControlMode(const VIDDEC_Handle_t DecodeHandle, const BOOL IsThereOrders)
{
    /* Overall variables of the task */
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
    ST_ErrorCode_t  ErrorCode;

    /* Variables used only in headers processing */
    U8  StartCode;
    U8  Tmp8;
    BOOL HasStartCodeCommands;
    HALDEC_DecodeSkipConstraint_t HALDecodeSkipConstraint;
    VIDBUFF_PictureBuffer_t * DecodedPicture_p = NULL;
    VIDBUFF_PictureBuffer_t * DecodedDecimPicture_p  = NULL;

#ifdef ST_SecondInstanceSharingTheSameDecoder
    VIDBUFF_GetUnusedPictureBufferParams_t  GetPictureBufferParams;
    STVID_SequenceInfo_t SequenceInfo;
    STVID_PictureInfos_t PictureInfos ;
    StreamInfoForDecode_t StreamInfoForDecode ;
    PictureStreamInfo_t  PictureStreamInfo ;
    VIDDEC_SyntaxError_t  SyntaxError ;
    U32 * TemporalReferenceOffset_p;
    void * PictureStartAddress_p ;
#endif /* ST_SecondInstanceSharingTheSameDecoder */

    /* LaunchDecodeWhenPossible: Flag to trigger the decode/display:
      Set to TRUE to launch the decode/display of the current picture.
      Warning: setting this flag to FALSE requires launch of the start code detector:
       - this is done automatically when launching decode
       - this must be done manually in other cases ! */

    /* Variables for picture data passed to launch decode and display (updated in headers processing, completly valid only after slice) */

    /* Variables used only in skip/decode/display/monitor part */
    VIDDEC_DecodePictureParams_t            DecodeParams;
    BOOL HasChanged;
    BOOL OverWrittenDisplay;

#ifdef STVID_HARDWARE_ERROR_EVENT
    STVID_HardwareError_t   HardwareErrorStatus;
#endif /* STVID_HARDWARE_ERROR_EVENT */

    /* Big loop executed until ToBeDeleted is set --------------------------- */

    if (IsThereOrders)
    {
        /* Take start code found orders first, then user commands if there was no start code order */
        HasStartCodeCommands = FALSE;

        /* Process all start code commands together... */
        while ((!(VIDDEC_Data_p->Stop.AfterCurrentPicture)) && (!(VIDDEC_Data_p->ForTask.SoftResetPending)) && (PopStartCodeCommand(DecodeHandle, &StartCode) == ST_NO_ERROR))
        {
            /* There are start code orders to process */
            HasStartCodeCommands = TRUE;

#ifdef STVID_DEBUG_GET_STATISTICS
            VIDDEC_Data_p->Statistics.DecodeStartCodeFound ++;
#endif /* STVID_DEBUG_GET_STATISTICS */

#ifdef TRACE_DECODE
            /*    TraceBuffer(("-sc:%d\n",StartCode));*/
#endif /* TRACE_DECODE */
#ifdef TRACE_DECODE
            StartCodes[StartCodeIndex] = StartCode;
            if (StartCodeIndex < (sizeof(StartCodes) / sizeof(U32)) - 1)
            {
                StartCodeIndex ++;
            }
#endif /* TRACE_DECODE */

            /* Do actions for previous start code: for those actions who can only be done after arrival of next start code */
            switch (VIDDEC_Data_p->ForTask.PreviousStartCode)
            {
                case PICTURE_START_CODE :
                    /* bitrate eval in automatic mode */
                    EvaluateBitRateValue(DecodeHandle, &VIDDEC_Data_p->ForTask.PictureInfos);
                    /* Error Recovery : we want to count the number of pictures before a First Slice. We must keep 1          */
                    /* First Slice SC for 1 Picture SC. Otherwise we consider the hw decoders will be lost in case of decode. */
                    VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.NbOfPictureSCPerFirstSliceSC++;
                    if (VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.NbOfPictureSCPerFirstSliceSC > 1)
                    {
                        /* Error Recovery Case n011 : */
                        /* Dummy Picture Start Code. Soft reset because we consider that the sequence is lost. */
                        if (IsSequenceLostInCaseOfDummyPictureStartCode(VIDDEC_Data_p->ErrorRecoveryMode))
                        {
                            VIDDEC_Data_p->ForTask.SoftResetPending = TRUE;
                            PushControllerCommand(DecodeHandle, CONTROLLER_COMMAND_RESET);
                            VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.NbOfPictureSCPerFirstSliceSC = 0;
                        }
                        else
                        {
                            STEVT_Notify(VIDDEC_Data_p->EventsHandle, VIDDEC_Data_p->RegisteredEventsID[VIDDEC_DATA_ERROR_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
                        }

#ifdef TRACE_DECODE
                        TraceBuffer(("-ERCn11-"));
#endif /* TRACE_DECODE */
                    }
                    break;

                case FIRST_SLICE_START_CODE :
#ifdef TRACE_DECODE
/*                    TraceBuffer(("Sli+1:"));*/
#endif /* TRACE_DECODE */
                    /* Caution ! After processing this start code, either LaunchDecodeWhenPossible or LaunchSkipWhenPossible
                       must be set, but not none of them ! (neither both of them, except if SoftReset is pending)
                       This is to ensure that the hardware is either skipping or decoding each picture in the bit buffer. */

                    /* Check if stop is to be done */
                    /* CL, test splitted because sh4gcc -O2 mishandle the original complex condition and lead to an 'Illegal delay slot instruction' exception  */
                    if (((VIDDEC_Data_p->Stop.WhenNextReferencePicture) && (VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame != STVID_MPEG_FRAME_B)))
                    {
                        VIDDEC_Data_p->Stop.AfterCurrentPicture = TRUE;
                    }
                    else if ((VIDDEC_Data_p->Stop.WhenNextIPicture) && (VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I))
                    {
                        VIDDEC_Data_p->Stop.AfterCurrentPicture = TRUE;
                    }
                    else if ((!VIDDEC_Data_p->Stop.DiscontinuityStartCodeInserted) && (VIDDEC_Data_p->Stop.WhenNoData))
                    {
                        VIDDEC_Data_p->Stop.AfterCurrentPicture = TRUE;
                    }

                    /* Counter of Picture SC to detect several Picture SC between First Slice SC. */
                    /* should be NULL after that (1P - 1 FS). */
                    if (VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.NbOfPictureSCPerFirstSliceSC > 0)
                    {
                        VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.NbOfPictureSCPerFirstSliceSC--;
                    }

/*                    if (VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Found slice again before last decode was launched !"));
                    }*/

                    /* Check error actions to do: start by SoftReset, then Skip, then Notify(ERROR) */
                    if (VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.PreviousPictureHasAMissingField)
                    {
                        /* Risk of crash : to do for every recovery modes. */
                        /* Error_Recovery_Case_n04: Missing expected 2nd field   */
                        /* Caution: be sure a soft reset is always done after PreviousPictureHasAMissingField is set TRUE: so check it first ! */
                        VIDDEC_Data_p->StartCodeProcessing.PictureCounter ++;
                        VIDDEC_Data_p->ForTask.SoftResetPending = TRUE;
                        PushControllerCommand(DecodeHandle, CONTROLLER_COMMAND_RESET);
#ifdef TRACE_DECODE
                        TraceBuffer(("-ERCn4-"));
#endif /* TRACE_DECODE */
                    }
                    else if ((VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.FirstPictureOfSequence) &&
                        ((VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.HasProfileErrors) ||
                            (VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.HasCriticalSequenceSyntaxErrors)) &&
                        (IsSequenceLostInCaseOfSequenceOrProfileError(VIDDEC_Data_p->ErrorRecoveryMode)))
                    {
                        /* Error_Recovery_Case_n01: the frame rate of the sequence is not supported.                    */
                        /* Error_Recovery_Case_n02: the dimensions for the sequence don't match with the MPEG Profile.  */
                        /* Check the profile can support the Width and the Height of the sequence.  */
                        /* Automatically skip sequence when profile error (by doing a soft reset).  */
                        VIDDEC_Data_p->ForTask.SoftResetPending = TRUE;
                        PushControllerCommand(DecodeHandle, CONTROLLER_COMMAND_RESET);
                        /* Here no need to signal controller that a new command is given ! */
#ifdef TRACE_DECODE
                        TraceBuffer(("-ERCn1n2-"));
#endif /* TRACE_DECODE */
                    }
                    else if ((VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.HasPictureSyntaxErrors) &&
                                (IsPictureToBeSkippedInCaseOfStartCodePictureSyntaxError(VIDDEC_Data_p->ErrorRecoveryMode)))
                    {
                        /* Error_Recovery_Case_n03: No picture coding extension in MPEG2   */
                        /* Automatically skip picture with errors or not entirely present. */
                        VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible = TRUE;
#ifdef TRACE_DECODE
                        TraceBuffer(("-ERCn3-"));
#endif /* TRACE_DECODE */
                    }
                    else if ((VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.FirstPictureOfSequence) &&
                                ((VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.HasProfileErrors) ||
                                (VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.HasCriticalSequenceSyntaxErrors)) &&
                                (IsEventToBeSentInCaseOfSequenceOrProfileError(VIDDEC_Data_p->ErrorRecoveryMode))) /* idem case but for Error Recovery Medium or Low */
                    {
                        /* Error_Recovery_Case_n01: the frame rate of the sequence is not supported. */
                        /* Error_Recovery_Case_n02: the dimensions for the sequence don't match with the MPEG Profile*/
                        /* Check the profile can support the Width and the Height of the sequence.  */
                        /* Automatically skip sequence when profile error (by doing a soft reser).  */
                        STEVT_Notify(VIDDEC_Data_p->EventsHandle, VIDDEC_Data_p->RegisteredEventsID[VIDDEC_DATA_ERROR_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
#ifdef TRACE_DECODE
                        TraceBuffer(("-ERCn1n2-"));
#endif /* TRACE_DECODE */
                    }
                    else if ((VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.HasPictureSyntaxErrors) &&
                                (IsEventToBeSentInCaseOfStartCodePictureSyntaxError(VIDDEC_Data_p->ErrorRecoveryMode))) /* same case but error recovery mode is medium or low */
                    {
                        /* Error_Recovery_Case_n03: No picture coding extension in MPEG2   */
                        /* Automatically skip picture with errors or not entirely present. */
                        STEVT_Notify(VIDDEC_Data_p->EventsHandle, VIDDEC_Data_p->RegisteredEventsID[VIDDEC_DATA_ERROR_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
#ifdef TRACE_DECODE
                        TraceBuffer(("-ERCn3-"));
#endif /* TRACE_DECODE */
                    }
                    else if (VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.HasRecoveredSequenceSyntaxErrors)
                    {
                        STEVT_Notify(VIDDEC_Data_p->EventsHandle, VIDDEC_Data_p->RegisteredEventsID[VIDDEC_DATA_ERROR_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
                    }
                    else /* No error: check if picture is to be skipped for normal reasons */
                         if (((VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B) && /* Cases of skip for B pictures : */
                              ((VIDDEC_Data_p->Skip.AllBPictures) || /* Mode skip B */
                               (VIDDEC_Data_p->BrokenLink.Active) || /* broken link after an open GOP */
                               (VIDDEC_Data_p->Skip.NextBPicture) || /* Skip of just one B requested */
                               (VIDDEC_Data_p->Skip.UntilNextIPicture) || /* Skip all B and P until next I */
                               (VIDDEC_Data_p->Skip.UntilNextSequence) || /* Skip all pictures until next sequence */
                               (((VIDDEC_Data_p->ReferencePicture.MissingPrevious) ||
                               ((VIDDEC_Data_p->ReferencePicture.LastButOne_p == NULL) && (!VIDDEC_Data_p->DecodingStream_p->GroupOfPictures.closed_gop))) &&  /* skip B without reference except for closed gop */
                                (!(IsBPictureToBeDecodedIfMissingReference(VIDDEC_Data_p->ErrorRecoveryMode)))))) || /* no reference present, in some error recovery modes */
                             ((VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_P) && /* Cases of skip for P pictures : */
                              ((VIDDEC_Data_p->Skip.AllPPictures) || /* Mode skip P */
                               (VIDDEC_Data_p->Skip.UntilNextIPicture) || /* Skip all B and P until next I */
                               (VIDDEC_Data_p->Skip.UntilNextSequence) || /* Skip all pictures until next sequence */
                               ((VIDDEC_Data_p->ReferencePicture.MissingPrevious) &&
                               (!(IsPPictureToBeDecodedIfMissingReference(VIDDEC_Data_p->ErrorRecoveryMode)))))) || /* no reference present, in some error recovery modes */
                             ((VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I) && /* Cases of skip for I pictures : */
                              ((VIDDEC_Data_p->Skip.AllIPictures) || /* Mode skip I */
                               (VIDDEC_Data_p->Skip.UntilNextSequence) || /* Skip all pictures until next sequence */
                               ((!VIDDEC_Data_p->SearchFirstI) && (VIDDEC_Data_p->Skip.AllExceptFirstI)))) /* Skip all pictures except first I in current bit buffer */
                            )
                            /* Question: ? Don't skip 2nd field if first was not skipped ? (complicated mechanism) */
                    {
                        /* Error or skip: skip picture */
                        VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible = TRUE;
#ifdef TRACE_DECODE
/*                        TraceBuffer(("-WeSkip"));*/
#endif /* TRACE_DECODE */
                    }

                    /* Ensure that the picture is either skipped or decoded: if no skip nor decode is
                       already flagged, nor a soft reset pending, be ready to trigger decode. */
                    if ((!(VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible)) &&
                        (!(VIDDEC_Data_p->ForTask.SoftResetPending)) &&
                        (!(VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible))
                       )
                    {
                        /* Ready to trigger decode: inform outside world, they may request special action */
                        VIDDEC_Data_p->DecodeAction = VIDDEC_DECODE_ACTION_NORMAL;
                        VIDDEC_Data_p->ForTask.NewPictureDecodeOrSkipEventData.PictureInfos_p = &(VIDDEC_Data_p->ForTask.PictureInfos);
                        VIDDEC_Data_p->ForTask.NewPictureDecodeOrSkipEventData.NbFieldsForDisplay = VIDDEC_Data_p->NbFieldsDisplayNextDecode;
                        VIDDEC_Data_p->ForTask.NewPictureDecodeOrSkipEventData.TemporalReference = VIDDEC_Data_p->ForTask.StreamInfoForDecode.Picture.temporal_reference;
                        VIDDEC_Data_p->ForTask.NewPictureDecodeOrSkipEventData.VbvDelay = VIDDEC_Data_p->ForTask.StreamInfoForDecode.Picture.vbv_delay;
                        PICTUREID_FROM_EXTTEMPREF(VIDDEC_Data_p->ForTask.NewPictureDecodeOrSkipEventData.ExtendedPresentationOrderPictureID,
                                                  VIDDEC_Data_p->ForTask.PictureStreamInfo.ExtendedTemporalReference,
                                                  VIDDEC_Data_p->ForTask.TemporalReferenceOffset);
                        VIDDEC_Data_p->ForTask.NewPictureDecodeOrSkipEventData.IsExtendedPresentationOrderIDValid = TRUE;

                        STEVT_Notify(VIDDEC_Data_p->EventsHandle, VIDDEC_Data_p->RegisteredEventsID[VIDDEC_READY_TO_DECODE_NEW_PICTURE_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(VIDDEC_Data_p->ForTask.NewPictureDecodeOrSkipEventData));
                        switch (VIDDEC_Data_p->DecodeAction)
                        {
                            case VIDDEC_DECODE_ACTION_SKIP :
                                VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible = TRUE;
#ifdef TRACE_DECODE
                                TraceBuffer(("-TrickmodSkip"));
#endif /* TRACE_DECODE */
                                break;

                            case VIDDEC_DECODE_ACTION_NO_DISPLAY :
                                /* Decode but don't display: picture is ready for decode, launch decode when possible */
                                VIDDEC_Data_p->ForTask.NoDisplayAfterNextDecode = TRUE;
#ifdef TRACE_DECODE
                                    TraceBuffer(("\rRequested not to display picture at decode.\r\n"));
#endif /* TRACE_DECODE */
/*                                    break;*/ /* no break */

/*                                case VIDDEC_DECODE_ACTION_NORMAL :*/
                            default :
                                /* Normal decode: picture is ready for decode, launch decode when possible */
#ifdef TRACE_DECODE
                                /*TraceBuffer(("-WeDec"));*/
#endif /* TRACE_DECODE */
                                VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible = TRUE;
#ifdef LIMIT_TRY_GET_BUFFER_TIME
                                VIDDEC_Data_p->ForTask.MaxTryGetBufferTime = time_plus(time_now(), MAX_TRY_GET_BUFFER_TIME);
#endif /* LIMIT_TRY_GET_BUFFER_TIME */
                                break;
                        }
                    } /* End ready to decode */

                    /* Detect end of delayed skips requested */
                    if (VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B)
                    {
                        /* Having a B: forget flag asking to skip next B, because this one must have been skipped ! */
                        VIDDEC_Data_p->Skip.NextBPicture = FALSE;
                    }
                    if (VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)
                    {
                        /* Having a I picture: forget flag asking to skip until next I */
                        VIDDEC_Data_p->Skip.UntilNextIPicture = FALSE;
                    }

#ifdef ST_ordqueue
                    /* Check if we can push a picture to the display now, as a new reference is here */
                    if (VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame != STVID_MPEG_FRAME_B)
                    {
                        VIDCOM_PictureID_t CurrentParsedReferenceExtendedPresentationOrderPictureID;

                        PICTUREID_FROM_EXTTEMPREF(CurrentParsedReferenceExtendedPresentationOrderPictureID,
                                                  VIDDEC_Data_p->ForTask.PictureStreamInfo.ExtendedTemporalReference,
                                                  VIDDEC_Data_p->ForTask.TemporalReferenceOffset);


                        if(  (VIDDEC_Data_p->ForTask.QueueParams.LastParsedReferenceExtendedPresentationOrderPictureIDIsValid)
                           &&(vidcom_ComparePictureID(&CurrentParsedReferenceExtendedPresentationOrderPictureID,
                                                      &VIDDEC_Data_p->ForTask.QueueParams.LastParsedReferenceExtendedPresentationOrderPictureID) != 0))
                        {
                            if(!VIDDEC_Data_p->NextAction.LaunchDisplayWhenPossible)
                            {
                                /* Try to push the last reference to the display, if it is still in the ordering queue.
                                 * Avoid to do that while decode is on-going because in case of a stream
                                 * I0 P3 B1 B2 P6 B4 B5, we might push P3 to the display while B2 is being decoded,
                                 * and then P3 will be pushed before B2 */
                                ErrorCode = VIDQUEUE_PushPicturesToDisplay(VIDDEC_Data_p->OrderingQueueHandle,
                                                &(VIDDEC_Data_p->ForTask.QueueParams.LastParsedReferenceExtendedPresentationOrderPictureID));
                            }
                            else
                            {
                                /* Push only on the decode end */
                                VIDDEC_Data_p->ForTask.QueueParams.PictureToPushAfterCurrentDecodeExtendedPresentationOrderPictureID = VIDDEC_Data_p->ForTask.QueueParams.LastParsedReferenceExtendedPresentationOrderPictureID;
                                VIDDEC_Data_p->ForTask.QueueParams.PictureToPushAfterCurrentDecodeExtendedPresentationOrderPictureIDIsValid = TRUE;
                            }
                        }
                        VIDDEC_Data_p->ForTask.QueueParams.LastParsedReferenceExtendedPresentationOrderPictureIDIsValid = TRUE;

                        /* Remember the last parsed reference presentation order ID */
                        PICTUREID_FROM_EXTTEMPREF(VIDDEC_Data_p->ForTask.QueueParams.LastParsedReferenceExtendedPresentationOrderPictureID,
                                                  VIDDEC_Data_p->ForTask.PictureStreamInfo.ExtendedTemporalReference,
                                                  VIDDEC_Data_p->ForTask.TemporalReferenceOffset);
                    }
#endif  /* end of def ST_ordqueue */

#ifdef STVID_DEBUG_GET_STATISTICS
                    VIDDEC_Data_p->Statistics.DecodePictureFound ++;
                    switch (VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame)
                    {
                        case STVID_MPEG_FRAME_I :
                            VIDDEC_Data_p->Statistics.DecodePictureFoundMPEGFrameI ++;
                            break;

                        case STVID_MPEG_FRAME_P :
                            VIDDEC_Data_p->Statistics.DecodePictureFoundMPEGFrameP ++;
                            break;

                        case STVID_MPEG_FRAME_B :
                            VIDDEC_Data_p->Statistics.DecodePictureFoundMPEGFrameB ++;
                            break;

                        default :
                            break;
                    }
#endif /* STVID_DEBUG_GET_STATISTICS */

                    break;

                case SEQUENCE_HEADER_CODE :
#ifdef TRACE_DECODE
/*    TraceBuffer(("\r\n%d,FirstSequence", time_now()));*/
#endif /* TRACE_DECODE */
                    VIDDEC_Data_p->Skip.UntilNextSequence = FALSE;
#ifdef STVID_DEBUG_GET_STATISTICS
                    VIDDEC_Data_p->Statistics.DecodeSequenceFound ++;
#endif /* STVID_DEBUG_GET_STATISTICS */
                    break;

                default :
                    break;
            } /* End of Switch Previous Start Code */

            /* Traces : Next Action */
#ifdef TRACE_DECODE
            if (VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible)
            {
              /*  TraceBuffer(("Skip"));*/
                    /* !!!! */
/*                        VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible = FALSE;*/
/*                        VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible = TRUE;*/
            }
            if (VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible)
            {
                /*TraceBuffer(("Dec"));*/
#ifdef TRACE_DECODE
                ReadyToDecode[DecodeIndex] = time_now();
#endif /* TRACE_DECODE */
            }
            if (VIDDEC_Data_p->ForTask.NoDisplayAfterNextDecode)
            {
                TraceBuffer(("Nodisp"));
            }
            if (VIDDEC_Data_p->ForTask.SoftResetPending)
            {
                TraceBuffer(("AskReset"));
            }
#endif /* TRACE_DECODE */

            if (StartCode == DISCONTINUITY_START_CODE)
            {
                if (VIDDEC_Data_p->Skip.AllExceptFirstI)
                {
                    VIDDEC_Data_p->SearchFirstI = TRUE;
                }

                /* Software start code inserted at flush (inject discontinuity) */
#ifdef TRACE_DECODE
                TraceBuffer(("***Discontinuity***"));
#endif /* TRACE_DECODE */
                /* Discontinuity in stream */
                if ( (VIDDEC_Data_p->ForTask.PreviousStartCode == FIRST_SLICE_START_CODE) && (!(VIDDEC_Data_p->DecodeOnce))
                      && (VIDDEC_Data_p->ErrorRecoveryMode == STVID_ERROR_RECOVERY_FULL) )
                {
                    /* Discontinuity after slice start code: picture is not entirely present, skip it */
                    /* Could we suppose on the opposite that the picture is good and decode it in error recovery mode != FULL ? */
                    VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible = FALSE;
                    VIDDEC_Data_p->ForTask.NoDisplayAfterNextDecode = FALSE;
                    VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible = TRUE;
#ifdef TRACE_DECODE
                    TraceBuffer(("DisSkip"));
#endif /* TRACE_DECODE */
                }

                if (VIDDEC_Data_p->Stop.WhenNoData)
                {
                    /* Discontinuity reached : this the end of the data. */
                    VIDDEC_Data_p->Stop.AfterCurrentPicture = TRUE;
                }
            }
            else
            {
                /* If start code is a slice but not first slice, this is invalid,
                except in MPEG1, where the first slice code can have any slice start code value. */
                if (((StartCode <= GREATEST_SLICE_START_CODE) && (StartCode > FIRST_SLICE_START_CODE)) && /* Slice start code but not first slice */
                    ((VIDDEC_Data_p->DecodingStream_p->MPEG1BitStream)) /* MPEG1 */
                    )
                {
                    /* Slice start code with any slice start code value in MPEG1: take as a first picture start code. */
                    StartCode = FIRST_SLICE_START_CODE;
                }

#if 0 /* Was used for errors like in test05.mpg but now detected by the NbOfPictureSCPerFirstSliceSC counter. */
                /* If picture and a picture passed with no slice, do as if a slice has come, and give picture for skip ! */
                if ((StartCode == PICTURE_START_CODE) &&
                    ((VIDDEC_Data_p->DecodingStream_p)->BitStreamProgress >= AFTER_PICTURE_HEADER) &&
                    ((VIDDEC_Data_p->DecodingStream_p)->BitStreamProgress < AFTER_PICTURE_DATA)
                    )
                {
                    /* Error_Recovery_Case_n06 : Picture coding type or picture structure invalid. */
                    /* Error_Recovery_Case_n11 : Overlapping picture start code. */
                    /* We set the flag so that when we will parse the first slice of this image, we will ask the decoder to skip. */
                    VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.HasPictureSyntaxErrors = TRUE;
#ifdef TRACE_DECODE
                    TraceBuffer(("-ERCn6n11-"));
#endif /* TRACE_DECODE */
                }
#endif /* 0 */
                /* Treat errors in start codes */
                /* CAUTION : we want to test if (SMALLEST_SYSTEM_START_CODE <= StartCode <= GREATEST_SYSTEM_START_CODE), but as
                comparing to GREATEST_SYSTEM_START_CODE causes a warning because it values 0xFF, the test is simplified. */
                if ((StartCode >= SMALLEST_SYSTEM_START_CODE) /*&& (StartCode <= GREATEST_SYSTEM_START_CODE)*/ )
                {
/*                        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "System start code !"));*/
#ifdef TRACE_DECODE
                    TraceBuffer(("****SystemSC****\a"));
#endif /* TRACE_DECODE */
#ifdef STVID_DEBUG_GET_STATISTICS
                    VIDDEC_Data_p->Statistics.DecodePbStartCodeFoundInvalid ++;
                    if ((StartCode >= SMALLEST_VIDEO_PACKET_START_CODE) && (StartCode <= GREATEST_VIDEO_PACKET_START_CODE))
                    {
                        VIDDEC_Data_p->Statistics.DecodePbStartCodeFoundVideoPES ++;
                    }
#endif /* STVID_DEBUG_GET_STATISTICS */
                    /* Error_Recovery_Case_n05 : Inconsistency in start code arrival.   */
                    /* System start code (should be managed by the PES Parser)          */
                    /* May also happen if bit buffer underflow : the function           */
                    /* viddec_HeaderDataGetStartCode may then return the error start    */
                    /* code GREATEST_SYSTEM_START_CODE !!!                              */
                    VIDDEC_Data_p->ForTask.SyntaxError.HasIllegalStartCodeError = TRUE;
#ifdef TRACE_DECODE
                    TraceBuffer(("-ERCn5-"));
#endif /* TRACE_DECODE */
                }
                /* We do not receive the start code following the first slice until something else than a slice. */
                else if ((StartCode > FIRST_SLICE_START_CODE) && (StartCode <= GREATEST_SLICE_START_CODE))
                {
/*                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Invalid slice (not first) start code !"));*/
#ifdef TRACE_DECODE
                    TraceBuffer(("****InvalidSli****\a"));
#endif /* TRACE_DECODE */
#ifdef STVID_DEBUG_GET_STATISTICS
                    VIDDEC_Data_p->Statistics.DecodePbStartCodeFoundInvalid ++;
#endif /* STVID_DEBUG_GET_STATISTICS */
                    /* Error_Recovery_Case_n05 : Inconsistency in start code arrival.    */
                    /* Invalid slice start code.                                         */
                    VIDDEC_Data_p->ForTask.SyntaxError.HasIllegalStartCodeError = TRUE;
                }
                else
                {
                    VIDDEC_Data_p->ForTask.SyntaxError.HasIllegalStartCodeError = FALSE;
                }
                if (VIDDEC_Data_p->ForTask.SyntaxError.HasIllegalStartCodeError)
                {
                    /* Error Recovery Case n05 : */
                    /* Illegal Start Code. Skip picture or do nothing.
                    If before a sequence, should have no effect (for example before first sequence !). */
                    if ((IsIllegalStartCodeErrorCausingPictureSyntaxError(VIDDEC_Data_p->ErrorRecoveryMode)) && (VIDDEC_Data_p->DecodingStream_p->BitStreamProgress != NO_BITSTREAM))
                    {
                        /* !!! Transform into picture syntax error ? */
                        VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.HasPictureSyntaxErrors = TRUE;
                    }
                    STEVT_Notify(VIDDEC_Data_p->EventsHandle, VIDDEC_Data_p->RegisteredEventsID[VIDDEC_DATA_ERROR_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
                }
                else
                {
                    if (StartCode == SEQUENCE_END_CODE)
                    {
                        STEVT_Notify(VIDDEC_Data_p->EventsHandle, VIDDEC_Data_p->RegisteredEventsID[VIDDEC_SEQUENCE_END_CODE_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
                    }
                    /* Parse start code */
                    ErrorCode = viddec_ParseStartCode(DecodeHandle, StartCode, VIDDEC_Data_p->DecodingStream_p, PARSER_MODE_STREAM_ANALYSER);
#ifdef TRACE_DECODE
                    if ( (ErrorCode == ST_NO_ERROR) && (StartCode == GROUP_START_CODE) )
                    {
                      /*  TraceBuffer (("\r\nGOP(%d/%d) ", VIDDEC_Data_p->DecodingStream_p->GroupOfPictures.time_code.seconds
                                                       , VIDDEC_Data_p->DecodingStream_p->GroupOfPictures.time_code.pictures));*/
                    }
#endif
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        /* Error_Recovery_Case_n05 : Inconsistency in start code arrival.    */
                        VIDDEC_Data_p->ForTask.SyntaxError.HasStartCodeParsingError = TRUE;
#ifdef TRACE_DECODE
                        TraceBuffer(("***Parsing Err***\a"));
                        TraceBuffer(("-ERCn5-"));
#endif /* TRACE_DECODE */
                    }

                    /* Process start code */
                    viddec_ProcessStartCode(DecodeHandle, StartCode, VIDDEC_Data_p->DecodingStream_p,
                            &(VIDDEC_Data_p->ForTask.PictureInfos), &(VIDDEC_Data_p->ForTask.StreamInfoForDecode), &(VIDDEC_Data_p->ForTask.PictureStreamInfo), &(VIDDEC_Data_p->ForTask.SyntaxError), &(VIDDEC_Data_p->ForTask.TemporalReferenceOffset));

#ifdef STVID_DEBUG_GET_STATUS
				    VIDDEC_Data_p->Status.SequenceInfo_p = &VIDDEC_Data_p->StartCodeProcessing.SequenceInfo;
#endif /* STVID_DEBUG_GET_STATUS */
                }

            } /* end not discontinuity start code */

            /* Store previous start code */
            VIDDEC_Data_p->ForTask.PreviousStartCode = StartCode;

            /* Check if there are interrupt to process after each start code process */
            CheckAndProcessInterrupt(DecodeHandle);

        } /* end while(start code) */

        if (HasStartCodeCommands)
        {
            /* Search next start code if not already done or about to be done:
                - if LaunchDecodeWhenPossible, next decode will launch next start code search
                - if start code USER_DATA_START_CODE, the start code after is found manually and it only will launch start code search */
            /* !!! debug *//*if (StartCode == USER_DATA_START_CODE)*/
/*                {*/
/*                    if (VIDDEC_Data_p->StartCodeParsing.HasUserDataProcessTriggeredStartCode)*/
/*                    {*/
/*                        VIDDEC_Data_p->StartCodeParsing.HasUserDataProcessTriggeredStartCode = TRUE;*/
/*                    }*/
/*                    else*/
/*                    {*/
/*                        VIDDEC_Data_p->StartCodeParsing.HasUserDataProcessTriggeredStartCode = FALSE;*/
/*                    }*/
/*                }*/

            if ((!(VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible)) &&
                ((StartCode != USER_DATA_START_CODE) ||
                    (!(VIDDEC_Data_p->StartCodeParsing.HasUserDataProcessTriggeredStartCode))/*FALSE*/) && /* Don't launch start code search if user data triggered start code */
                (!(VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible)) &&
                (!VIDDEC_Data_p->ForTask.SoftResetPending)   /* don't launch start code detector if soft reset is pending. */
                )
            {
                if (VIDDEC_Data_p->DecoderState == VIDDEC_DECODER_STATE_DECODING)
                {
                    VIDDEC_Data_p->HWStartCodeDetector.IsIdle = FALSE;
                    VIDDEC_Data_p->HWStartCodeDetector.MaxStartCodeSearchTime = time_plus(time_now(), MAX_START_CODE_SEARCH_TIME);
                    HALDEC_SearchNextStartCode(VIDDEC_Data_p->HALDecodeHandle, HALDEC_START_CODE_SEARCH_MODE_NORMAL, TRUE, NULL);
                    VIDDEC_Data_p->LaunchStartCodeSearchToResume = FALSE;
                }
                else
                {
                    VIDDEC_Data_p->LaunchStartCodeSearchToResume = TRUE;
                }
            }
            VIDDEC_Data_p->StartCodeParsing.HasUserDataProcessTriggeredStartCode = FALSE;
        }

        /* Process all user commands together only if no start code commands... */
        while (PopControllerCommand(DecodeHandle, &Tmp8) == ST_NO_ERROR)
        {
            switch ((ControllerCommandID_t) Tmp8)
            {
                case CONTROLLER_COMMAND_RESET :
                    VIDDEC_Data_p->ForTask.SoftResetPending = FALSE;
#ifdef ST_ordqueue
                    VIDDEC_Data_p->NextAction.LaunchDisplayWhenPossible = FALSE;
                    VIDDEC_Data_p->NextAction.NumberOfDisplayToLaunch = 0;
#endif  /* end of def ST_ordqueue */
#ifdef ST_smoothbackward
                    if (VIDDEC_Data_p->ControlModeInsideHAL != VIDDEC_AUTOMATIC_CONTROL_MODE)
                    {
                        /* Mode changing: coming to automatic mode */
                        HALDEC_SetSmoothBackwardConfiguration(VIDDEC_Data_p->HALDecodeHandle, FALSE);
                        HALDEC_SetDecodeBitBuffer(VIDDEC_Data_p->HALDecodeHandle, VIDDEC_Data_p->BitBuffer.Address_p, VIDDEC_Data_p->BitBuffer.TotalSize, HALDEC_BIT_BUFFER_HW_MANAGED_CIRCULAR);
                        viddec_SetBitBufferThresholdToIdleValue(DecodeHandle);
                        VIDDEC_Data_p->RequiredSearch.SCDetectorHasBeenLaunchedManually = FALSE;
                        /* Now mode has changed */
                        VIDDEC_Data_p->ControlModeInsideHAL = VIDDEC_AUTOMATIC_CONTROL_MODE;
                    }
#endif /* ST_smoothbackward */
                    /* Reset decoder to find first sequence */
                    SeekBeginningOfStream(DecodeHandle);

#ifdef STVID_STVAPI_ARCHITECTURE
                    /* if a decode of a picture was on going, the locked raster */
                    /* pictures have to be unlocked */
                    if (VIDDEC_Data_p->RasterBufManagerParams.RasterPicturesLocked)
                    {
                       viddec_UnuseRasterPictures(VIDDEC_Data_p);
                       VIDDEC_Data_p->RasterBufManagerParams.RasterPicturesLocked = FALSE;
                    }
#endif /* end of def STVID_STVAPI_ARCHITECTURE */

#if defined(TRACE_DECODE) || defined(TRACE_TAKE_RELEASE)
                    TraceBuffer(("***Reset*** \r\n"));
#endif /* TRACE_DECODE || TRACE_TAKE_RELEASE */
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Controller has been reset."));
                    break;

                case CONTROLLER_COMMAND_SWITCH_TO_BACKWARD_MODE:
#ifdef ST_smoothbackward
                    VIDDEC_SwitchToBackwardMode(DecodeHandle);
                    return;
#endif
                    break;

                /*case CONTROLLER_COMMAND_PIPELINE_RESET :
                case CONTROLLER_COMMAND_DECODING_ERROR :*/
                default :
                    break;
            } /* end switch(command) */
        } /* end while(controller command) */

        /* Check if there are interrupts to process after each command process */
        CheckAndProcessInterrupt(DecodeHandle);

    } /* end there's orders, start code or command */

    /* Do actions at each loop, not depending on orders (comes here at least each MAX_WAIT_ORDER_TIME ticks */

#ifdef STVID_HARDWARE_ERROR_EVENT
    /* Monitor HAL hardware error, and if required, notify it to application.   */
    HardwareErrorStatus = HALDEC_GetHardwareErrorStatus (VIDDEC_Data_p->HALDecodeHandle);
    if (HardwareErrorStatus != STVID_HARDWARE_NO_ERROR_DETECTED)
    {
#ifdef STVID_DEBUG_GET_STATISTICS
        /* Now check statistics info */
        if ((HardwareErrorStatus & STVID_HARDWARE_ERROR_MISSING_PID) != 0)
        {
            VIDDEC_Data_p->Statistics.DecodePbHardwareErrorMissingPID ++;
        }
        if ((HardwareErrorStatus & STVID_HARDWARE_ERROR_SYNTAX_ERROR) != 0)
        {
            VIDDEC_Data_p->Statistics.DecodePbHardwareErrorSyntaxError ++;
        }
        if ((HardwareErrorStatus & STVID_HARDWARE_ERROR_TOO_SMALL_PICTURE) != 0)
        {
            VIDDEC_Data_p->Statistics.DecodePbHardwareErrorTooSmallPicture ++;
        }
#endif /* STVID_DEBUG_GET_STATISTICS */

        VIDDEC_NotifyDecodeEvent(DecodeHandle, STVID_HARDWARE_ERROR_EVT, (const void *) &(HardwareErrorStatus));

    }
#endif /* STVID_HARDWARE_ERROR_EVENT */

    /* Monitor bit buffer level to send events overflow and underflow.
    Do that all the time, even if decode or skip on-going. Otherwise if decode
    lasts one VSync period, BB level won't be monitored for that time ! */
    HasChanged = MonitorBitBufferLevel(DecodeHandle);
    if (HasChanged)
    {
        VIDDEC_Data_p->ForTask.PreviousBitBufferLevelChangeTime = time_now();
    }
    else if ((!(VIDDEC_Data_p->HWStartCodeDetector.IsIdle)) && /* start code search on-going but no SC found yet ! */
             (!(VIDDEC_Data_p->DataUnderflowEventNotSatisfied)) && /* No underflow event already sent */
             (VIDDEC_Data_p->DataUnderflow.BitBufferFreeSize > (VIDDEC_Data_p->BitBuffer.TotalSize / 2)) && /* Ensure we don't have condition of bit buffer full ! */
             (STOS_time_after(time_now(), VIDDEC_Data_p->HWStartCodeDetector.MaxStartCodeSearchTime) != 0) && /* Start code not found since too long */
             (STOS_time_after(time_now(), time_plus(VIDDEC_Data_p->ForTask.PreviousBitBufferLevelChangeTime, MAX_BIT_BUFFER_LEVEL_UNCHANGED_WHEN_NO_SC_TIME)) != 0) && /* Bit buffer level didn't change for too long */
             (VIDDEC_Data_p->DecoderState == VIDDEC_DECODER_STATE_DECODING) /* Not stopped */
            )
    {
        /* Idle since too long and no change in BBL and no SC: must be underflow !!! */
        VIDDEC_Data_p->DataUnderflow.BitRateValue = 0;
        VIDDEC_GetBitRateValue(DecodeHandle, &(VIDDEC_Data_p->DataUnderflow.BitRateValue));
        VIDDEC_Data_p->DataUnderflow.RequiredTimeJump = 0;
        VIDDEC_Data_p->DataUnderflow.TransferRelatedToPrevious = TRUE;
        VIDDEC_Data_p->DataUnderflow.RequiredDuration = VIDDEC_Data_p->BitBufferRequiredDuration;

#ifdef ST_smoothbackward
        if (VIDDEC_Data_p->ControlMode != VIDDEC_MANUAL_CONTROL_MODE)/* ?????? */
#endif /* ST_smoothbackward */
        {
            VIDDEC_Data_p->DataUnderflowEventNotSatisfied = TRUE;
            /* Bit buffer free size is filled in MonitorBitBufferLevel() function */
            VIDDEC_NotifyDecodeEvent(DecodeHandle, STVID_DATA_UNDERFLOW_EVT, (const void *) &(VIDDEC_Data_p->DataUnderflow));
        }
    }

    /* Check if max decode time is over */
    if (!(IsHWDecoderIdle(DecodeHandle)))
    {
        if (STOS_time_after(time_now(), VIDDEC_Data_p->HWDecoder.MaxDecodeOrSkipTime) != 0)
        {
            if (!(VIDDEC_Data_p->ForTask.TryingToRecoverFromMaxDecodeTimeOverflow))
            {
                /* Too long time decode was launched, there must have been a problem */
#if defined(LCMPEGX1_WA_GNBvd45748)
                HALDEC_LCMPEGX1GNBvd45748WA(VIDDEC_Data_p->HALDecodeHandle);
#endif /* defined(LCMPEGX1_WA_GNBvd45748) */
#ifdef TRACE_DECODE
                clock_t Now = time_now();
                TraceBuffer(("\r\n-**********DEC/SKIP overtime[%d]**********\a\r\n", time_minus(Now, VIDDEC_Data_p->HWDecoder.MaxDecodeOrSkipTime)));
#endif /* TRACE_DECODE */
#ifdef STVID_DEBUG_GET_STATISTICS
                VIDDEC_Data_p->Statistics.DecodePbDecodeTimeOutError ++;
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef TRACE_DECODE
                if (DecodeIndex > 0)
                {
                    DecodeOvertime[DecodeIndex - 1] = time_now();
                }
#endif /* TRACE_DECODE */
/*                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Decode never ending, try to go on with next picture..."));*/

                /* TimeOut is a 'decode error' case. The actions to take are managed by the viddec_DecodeHasErrors functions. */
                viddec_DecodeHasErrors(DecodeHandle, VIDDEC_DECODE_ERROR_TYPE_TIMEOUT);

                /* Allow some time to recover */
                VIDDEC_Data_p->HWDecoder.MaxDecodeOrSkipTime = time_plus(VIDDEC_Data_p->HWDecoder.MaxDecodeOrSkipTime, MAX_RECOVER_TIME);
                VIDDEC_Data_p->ForTask.TryingToRecoverFromMaxDecodeTimeOverflow = TRUE;
            }
            else
            {
                /* Tried pipeline reset but no recovery: soft reset */
                VIDDEC_Data_p->ForTask.SoftResetPending = TRUE;
                PushControllerCommand(DecodeHandle, CONTROLLER_COMMAND_RESET);

                /* Signal controller that a new command is given */
                semaphore_signal(VIDDEC_Data_p->DecodeOrder_p);
            }
        }
    }
    else
    {
#ifdef ST_ordqueue
        /* Check if display is to be launched. */
        /* We manage NumberOfDisplayToLaunch only in STVID_ERROR_RECOVERY_FULL. It takes the value : */
        /* =1 for a frame or a I field or a B field or the 2nd P field.                              */
        /* =2 for the 1st P field.                                                                   */
        if ((VIDDEC_Data_p->NextAction.LaunchDisplayWhenPossible) &&
            ((!(IsDisplayFieldCounterTakenIntoAccount(VIDDEC_Data_p->ErrorRecoveryMode))) || (VIDDEC_Data_p->NextAction.NumberOfDisplayToLaunch > 0)))
        {
            /* In FULL and if the decode is bad : we don't insert the picture. The buffer has been unlocked by DecodeHasError. */
            /* In NONE and if the decode is bad, we insert even though. The buffer will be unlocked after the display.         */
            if ((VIDDEC_Data_p->ForTask.QueueParams.Picture_p->Decode.DecodingError == VIDCOM_ERROR_LEVEL_NONE) ||
                (IsSpoiledPictureToBeDisplayed(VIDDEC_Data_p->ErrorRecoveryMode)))
            {
#ifdef TRACE_DECODE
              /*  TraceBuffer(("-INS(%c%d/%d%c)",(VIDDEC_Data_p->ForTask.QueueParams.Picture_p->PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B?'B':
                                        ((VIDDEC_Data_p->ForTask.QueueParams.Picture_p->PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_P?'P':'I'))),
                                        VIDDEC_Data_p->ForTask.QueueParams.Picture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                                        VIDDEC_Data_p->ForTask.QueueParams.Picture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                                        (VIDDEC_Data_p->ForTask.QueueParams.Picture_p->PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD?'T':
                                        ((VIDDEC_Data_p->ForTask.QueueParams.Picture_p->PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD?'B':'F'))))
                );*/
#endif /* TRACE_DECODE */
#ifdef ST_ordqueue

#ifdef TRACE_TAKE_RELEASE
                    TraceBuffer((">> Insert 0x%x 0x%x (%c)\r\n",
                                    VIDDEC_Data_p->ForTask.QueueParams.Picture_p,
                                    VIDDEC_Data_p->ForTask.QueueParams.Picture_p->AssociatedDecimatedPicture_p,
                                    MPEGFrame2Char(VIDDEC_Data_p->ForTask.QueueParams.Picture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame) ));
#endif
                /* Insert picture in display queue as soon as possible: it may be urgent to have it for next VSync ! */
                ErrorCode = VIDQUEUE_InsertPictureInQueue(VIDDEC_Data_p->OrderingQueueHandle,
                                                          VIDDEC_Data_p->ForTask.QueueParams.Picture_p,
                                                          VIDDEC_Data_p->ForTask.QueueParams.InsertionOrder);

                if (ErrorCode == ST_NO_ERROR)
                {
                    MPEGSpecific_CheckPictureToSendToDisplay(DecodeHandle);
                }

                /* Check if we need also to push a reference */
                if(VIDDEC_Data_p->ForTask.QueueParams.PictureToPushAfterCurrentDecodeExtendedPresentationOrderPictureIDIsValid)
                {
                    ErrorCode = VIDQUEUE_PushPicturesToDisplay(VIDDEC_Data_p->OrderingQueueHandle,
                                    &(VIDDEC_Data_p->ForTask.QueueParams.PictureToPushAfterCurrentDecodeExtendedPresentationOrderPictureID));
                    VIDDEC_Data_p->ForTask.QueueParams.PictureToPushAfterCurrentDecodeExtendedPresentationOrderPictureIDIsValid = FALSE;
                }
#endif /* ST_ordqueue */

                /* DecodeOnce : pause or freeze after first picture is passed to display. */
                if (VIDDEC_Data_p->DecodeOnce)
                {
                    VIDDEC_Data_p->DecodeOnce = FALSE;
                    /* Inform upper layer (API) that first decode is done. */
                    STEVT_Notify(VIDDEC_Data_p->EventsHandle, VIDDEC_Data_p->RegisteredEventsID[VIDDEC_DECODE_ONCE_READY_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
                }

#ifdef TRACE_DECODE
#ifdef TRACE_DECODE
                    /*TraceBuffer(("\rQueueing picture. [Ready->Launch=%d, Launch->Queue=%d]\r\n", time_minus(LaunchDecode[QueueingPicIndex], ReadyToDecode[QueueingPicIndex]), time_minus(time_now(), LaunchDecode[QueueingPicIndex])));*/
#endif /* TRACE_DECODE */
                QueueingPic[QueueingPicIndex] = VIDDEC_Data_p->ForTask.QueueParams.Picture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID;
                if (QueueingPicIndex < (sizeof(QueueingPic) / sizeof(U32)) - 1)
                {
                    QueueingPicIndex ++;
                }
#endif /* TRACE_DECODE */
                if (VIDDEC_Data_p->NextAction.NumberOfDisplayToLaunch > 0)
                {
                    VIDDEC_Data_p->NextAction.NumberOfDisplayToLaunch--;
                }
            }

            VIDDEC_Data_p->NextAction.LaunchDisplayWhenPossible = FALSE;
        } /* end if(LaunchDisplayWhenPossible) */
#endif  /* end of def ST_ordqueue */

        /* The decoder is now idle (the last decode is completed or failed). The last decoded picture can now be released */
        VIDDEC_ReleaseLastDecodedPicture(DecodeHandle);

        /* No display to do and must stop after current picture: stop now */
        if ((VIDDEC_Data_p->Stop.AfterCurrentPicture) &&
            (!(VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible)) &&
            (!(VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible)))
        {
            /* Order stop now and send event STOPPED */
            DoStopNow(DecodeHandle, TRUE);
        }
        else
        {
            /* If we ask a stop_end_of_data and no picture is decoded after that for a long time, we conclude that
             * no picture will be decoded (because no picture data will be found in the bitbuffer) */
            if (   (VIDDEC_Data_p->Stop.WhenNoData)
                && (!VIDDEC_Data_p->Stop.IsPending)
                && (VIDDEC_Data_p->HWDecoder.LastDecodedPictureInfos_p == NULL)
                && (STOS_time_after(time_now(), VIDDEC_Data_p->Stop.TimeoutForSendingStoppedEvent)) )
            {
                DoStopNow(DecodeHandle, TRUE);
            }
        }

        /* Check if skip has to be done. */
        if ((IsHWDecoderReadyToDecode(DecodeHandle)) && (VIDDEC_Data_p->VbvStartCondition.IsDecodeEnable))
        {
            if ((VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible) || (VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible))
            {
                /* Ask the HALDEC which action (decode or skip) is possible. HALDEC can have constraints forcing it to skip */
                /* or to decode. For example for a 2nd field, we must have the same action we had had for the 1st field.    */
                HALDEC_GetDecodeSkipConstraint(VIDDEC_Data_p->HALDecodeHandle, &HALDecodeSkipConstraint);
                if ((HALDecodeSkipConstraint == HALDEC_DECODE_SKIP_CONSTRAINT_MUST_DECODE) && (VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible))
                {
                    VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible = TRUE;
                    VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible = FALSE;
                    VIDDEC_Data_p->ForTask.NoDisplayAfterNextDecode = TRUE;
                }
                else if ((HALDecodeSkipConstraint == HALDEC_DECODE_SKIP_CONSTRAINT_MUST_SKIP) && (VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible))
                {
                    VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible = TRUE;
                    VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible = FALSE;
                }
#ifdef ST_SecondInstanceSharingTheSameDecoder
                if (VIDDEC_Data_p->StillDecodeParams.DecodeStillRequested)
                {
                    VIDDEC_Data_p->StillDecodeParams.HeaderData.SearchThroughHeaderDataNotCPU = FALSE;
                    viddec_ResetStream(&(VIDDEC_Data_p->StillDecodeParams.MPEG2BitStream));
                    ErrorCode = viddec_FindOneSequenceAndPicture(&(VIDDEC_Data_p->StillDecodeParams.HeaderData),
                                                    FALSE,
                                                    (U8*) (VIDDEC_Data_p->StillDecodeParams.BitBufferAddress_p),
                                                    &(VIDDEC_Data_p->StillDecodeParams.MPEG2BitStream), &PictureInfos,
                                                    &StreamInfoForDecode,
                                                    &PictureStreamInfo,
                                                    &SyntaxError,
                                                    TemporalReferenceOffset_p,
                                                    &PictureStartAddress_p)  ;
                    ErrorCode = ComputeSequenceInformation(&(VIDDEC_Data_p->StillDecodeParams.MPEG2BitStream), &SequenceInfo, &SyntaxError);

                    VIDDEC_Data_p->StillDecodeParams.BroadcastProfile = VIDDEC_Data_p->BroadcastProfile ;
                    VIDDEC_Data_p->StillDecodeParams.StillStartCodeProcessing.TimeCode = VIDDEC_Data_p->StartCodeProcessing.TimeCode  ;
                    VIDDEC_Data_p->StillDecodeParams.StillStartCodeProcessing.ExpectingSecondField = VIDDEC_Data_p->StartCodeProcessing.ExpectingSecondField ;
                    VIDDEC_Data_p->StillDecodeParams.StillStartCodeProcessing.ExtendedTemporalReference = VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference;

                    ErrorCode = FillPictureParamsStructuresForStill( VIDDEC_Data_p->StillDecodeParams ,
                                                    &SequenceInfo,  &(VIDDEC_Data_p->StillDecodeParams.MPEG2BitStream), &PictureInfos, &StreamInfoForDecode,
                                                        &PictureStreamInfo);

                    GetPictureBufferParams.MPEGFrame            = PictureInfos.VideoParams.MPEGFrame;
                    GetPictureBufferParams.PictureStructure     = PictureInfos.VideoParams.PictureStructure;
                    GetPictureBufferParams.TopFieldFirst        = PictureInfos.VideoParams.TopFieldFirst;
                    GetPictureBufferParams.ExpectingSecondField = PictureStreamInfo.ExpectingSecondField;
                    GetPictureBufferParams.DecodingOrderFrameID = PictureStreamInfo.ExtendedTemporalReference;
                    GetPictureBufferParams.PictureWidth         = PictureInfos.BitmapParams.Width;
                    GetPictureBufferParams.PictureHeight        = PictureInfos.BitmapParams.Height;

                    ErrorCode = VIDBUFF_GetAndTakeUnusedPictureBuffer(
                                    VIDDEC_Data_p->StillDecodeParams.BufferManagerHandle,
                                    &GetPictureBufferParams,
                                    &DecodedPicture_p,
                                    VIDCOM_VIDDEC_MODULE_BASE);

                    HALDEC_StartDecodeStillPicture(VIDDEC_Data_p->HALDecodeHandle, VIDDEC_Data_p->StillDecodeParams.BitBufferAddress_p, DecodedPicture_p, &StreamInfoForDecode );

                    PictureInfos.BitmapParams.Data1_p = DecodedPicture_p->FrameBuffer_p->Allocation.Address_p;
                    PictureInfos.BitmapParams.Size1   = DecodedPicture_p->FrameBuffer_p->Allocation.TotalSize -
                                                        DecodedPicture_p->FrameBuffer_p->Allocation.Size2;
                    PictureInfos.BitmapParams.Data2_p = DecodedPicture_p->FrameBuffer_p->Allocation.Address2_p;
                    PictureInfos.BitmapParams.Size2   = DecodedPicture_p->FrameBuffer_p->Allocation.Size2;

                    PictureInfos.BitmapParams.Data1_p = Virtual(PictureInfos.BitmapParams.Data1_p);
                    PictureInfos.BitmapParams.Data2_p = Virtual(PictureInfos.BitmapParams.Data2_p);

                    /* OLO_TO_DO: Shall we Release the Picture after the notification? */

                    STEVT_Notify(VIDDEC_Data_p->StillDecodeParams.EventsHandle,
                                VIDDEC_Data_p->StillDecodeParams.RegisteredEventsID[VIDDEC_NEW_PICTURE_DECODED_EVT_ID],
                                STEVT_EVENT_DATA_TYPE_CAST &(PictureInfos));

                    VIDDEC_Data_p->StillDecodeParams.DecodeStillRequested = FALSE;
                }
#endif /* ST_SecondInstanceSharingTheSameDecoder */

                /* Launch Action */
                if (VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible)
                {
#ifdef TRACE_DECODE
                  /*  TraceBuffer(("-SKIP(%c%d)",(VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B?'B':
                    ((VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_P?'P':'I'))),
                    VIDDEC_Data_p->ForTask.StreamInfoForDecode.Picture.temporal_reference
                    ));*/
#endif /* TRACE_DECODE */
#ifdef TRACE_DECODE
                    if (!(VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.PTS.Interpolated))
                    {
                    /*   TraceBuffer(("-PTS=%d", VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.PTS.PTS));*/
                    }
#endif /* TRACE_DECODE */
#ifdef ST_ordqueue
                    if (VIDDEC_Data_p->NextAction.NumberOfDisplayToLaunch > 0)
                    {
                        /* We don't want to display this picture. So decrement the counter to keep it consistent. */
                        VIDDEC_Data_p->NextAction.NumberOfDisplayToLaunch--;
                    }

                    if (VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame != STVID_MPEG_FRAME_B)
                    {
#ifdef TRACE_TAKE_RELEASE
                        TraceBuffer(("Buffer status:\r\n"));
                        VIDBUFF_PrintPictureBuffersStatus(VIDDEC_Data_p->BufferManagerHandle);
                        TraceBuffer(("Push All Pictures to display and erase references\r\n"));
#endif
                        /* Flush all pictures in ordering queue to display */
                        VIDQUEUE_PushAllPicturesToDisplay(VIDDEC_Data_p->OrderingQueueHandle);

                        /* A reference picture is lost: skip every picture until next I */
                        VIDDEC_ReleaseReferencePictures(DecodeHandle);
                    }
#endif  /* end of def ST_ordqueue */

                    /* Skip the picture */
                    SkipPicture(DecodeHandle);

                    /* Inform layer above that we skip without its request */
                    if (VIDDEC_Data_p->DecodeAction != VIDDEC_DECODE_ACTION_SKIP)
                    {
                        VIDDEC_Data_p->ForTask.NewPictureDecodeOrSkipEventData.PictureInfos_p = &(VIDDEC_Data_p->ForTask.PictureInfos);
                        VIDDEC_Data_p->ForTask.NewPictureDecodeOrSkipEventData.NbFieldsForDisplay = VIDDEC_Data_p->NbFieldsDisplayNextDecode;
                        STEVT_Notify(VIDDEC_Data_p->EventsHandle, VIDDEC_Data_p->RegisteredEventsID[VIDDEC_NEW_PICTURE_SKIPPED_NOT_REQUESTED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(VIDDEC_Data_p->ForTask.NewPictureDecodeOrSkipEventData));
#ifdef STVID_DEBUG_GET_STATISTICS
                        VIDDEC_Data_p->Statistics.DecodePictureSkippedNotRequested ++;
#endif /* STVID_DEBUG_GET_STATISTICS */
                    }
                    else
                    {
                        /* Avoid taking it into account many times */
                        VIDDEC_Data_p->DecodeAction = VIDDEC_DECODE_ACTION_NORMAL;
#ifdef STVID_DEBUG_GET_STATISTICS
                        VIDDEC_Data_p->Statistics.DecodePictureSkippedRequested ++;
#endif /* STVID_DEBUG_GET_STATISTICS */
                    }
                    VIDDEC_Data_p->ForTask.TryingToRecoverFromMaxDecodeTimeOverflow = FALSE;

                    VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible = FALSE;
                } /* end skip was done */
                else if (VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible)
                {
#ifdef STVID_MEASURES
                    MeasuresRB[MEASURE_BEGIN][MeasureIndexRB] = time_now();
#endif /* STVID_MEASURES */
                    /* Before trying to get a buffer, check if there is not a reference that
                        we didn't allow the unlock, but that may now be unlocked */
                    if ((VIDDEC_Data_p->ReferencePicture.LastButOne_p != NULL) && /* Check that LastButOne_p is not already NULL, because if it is then no need to enter mutex section on ReferencePictureSemaphore_p below */
                        (VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame != STVID_MPEG_FRAME_B) && /* Decoding a reference */
                        ((VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_FRAME) || (VIDDEC_Data_p->ForTask.PictureStreamInfo.ExpectingSecondField)) /* Frame or first field */
                    )
                    {
                        /* Lock Access to Reference Pictures */
                        semaphore_wait(VIDDEC_Data_p->ReferencePicture.ReferencePictureSemaphore_p);

                        /* Release the oldest reference*/
                        VIDDEC_ReleaseLastButOneReference(DecodeHandle);

                        /* UnLock Access to Reference Pictures */
                        semaphore_signal(VIDDEC_Data_p->ReferencePicture.ReferencePictureSemaphore_p);
                    }

                    ErrorCode = GetBuffers(VIDDEC_Data_p,&DecodedPicture_p,&DecodedDecimPicture_p );
                    /* will update DecodedPicture_p and DecodedDecimPicture_p */
                    /* -> decode if ErrorCode=OK but disable one reconstruction if a picture returned is null */

#ifdef TRACE_TAKE_RELEASE
                    if (ErrorCode == ST_NO_ERROR)
                    {
                        TraceBuffer(("\r\nGet Buffers 0x%x 0x%x (%c)\r\n",
                                    DecodedPicture_p,
                                    DecodedDecimPicture_p,
                                    MPEGFrame2Char(DecodedPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame) ));
                    }
                    else
                    {
                        TraceBuffer(("-"));
                    }
#endif /* TRACE_TAKE_RELEASE */

#ifdef STVID_STVAPI_ARCHITECTURE
                    if (ErrorCode == ST_NO_ERROR)
                    {
                        ErrorCode = viddec_GetRasterPictures(VIDDEC_Data_p);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            ErrorCode = ST_ERROR_NO_MEMORY;
                        }
                        else
                        {
                            VIDDEC_Data_p->RasterBufManagerParams.RasterPicturesLocked = TRUE;
                        }
                    }
#endif /* end of def STVID_STVAPI_ARCHITECTURE */


                    if (ErrorCode != ST_ERROR_NO_MEMORY)
                    {
                        /*  Fix timeout for getting a new free buffer */
                        VIDDEC_Data_p->HWDecoder.MaxBufferWaitingTime = time_plus(time_now(), MAX_TIME_BEFORE_STOPPING_DECODER);
                    }
                    /* !!! Debug: test that returned picture is not one of the references ! */
                    if ((ErrorCode == ST_NO_ERROR) &&
                        (VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_P) &&
                        (DecodedPicture_p == VIDDEC_Data_p->ReferencePicture.Last_p)
                        )
                    {
    /*                  STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Re-allocating P picture but it is still reference !!! (BUG-BUG-BUG)"));*/
#ifdef TRACE_DECODE
                        TraceBuffer(("\rERROR: Re-allocating P picture but it is still reference !!! (BUG-BUG-BUG)\r\n\a"));
#endif /* TRACE_DECODE */
                    }

                    /* Check if picture is on display: if it is, and if first B after a reference: don't want it ! */
/* TODO_CL : problematic recovery case, how to handle with Take/Release
 * The topic is can we decode a B overwriting a frame buffer currently used by a previous B ?
 * This should be overcome as follows:
 * viddec knowns the last decode picture type. viddec must inform vid_buff (through a new parameter 'AllowOverWriteOnBOnDisplay' passed in VIDBUFF_GetAndTakeUnusedPictureBuffer call) that
 * vid_buff can propose a frame buffer which is currently displayed and containing a B picture
 * viddec will set this flag as follow:
 * AllowOverWriteOnBOnDisplay = (VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B) && (VIDDEC_Data_p->PreviousDecodedPictureMPEGFrame == STVID_MPEG_FRAME_B)
 * But there is still the risk of running out of frame buffer if there is bad temporal ref in the stream (i.e. VIDDEC_Data_p->ReferencePicture.LastButOne_p may be displayed prior to the B we want to decode) */

#if 0 /* !!! WARNING - WARNING - WARNING !!!
        This code cut off prevents vid_dec to support properly overwrite mode B decoding !!!
        We cut the current code waiting for a clean implementation as described upper */
                    if (ErrorCode == ST_NO_ERROR)
                    {
                        VIDDEC_Data_p->ForTask.PictureLockStatus = VIDBUFF_PICTURE_LOCK_STATUS_UNLOCKED;
                        VIDBUFF_GetPictureLockStatus(VIDDEC_Data_p->BufferManagerHandle, &(VIDDEC_Data_p->ForTask.PictureLockStatus),DecodedPicture_p );
                        VIDBUFF_GetPictureLockStatus(VIDDEC_Data_p->BufferManagerHandle, &LastButOnePictureLockStatus, VIDDEC_Data_p->ReferencePicture.LastButOne_p);
                        if (((VIDDEC_Data_p->ForTask.PictureLockStatus == VIDBUFF_PICTURE_LOCK_STATUS_ON_DISPLAY) ||
                            (VIDDEC_Data_p->ForTask.PictureLockStatus == VIDBUFF_PICTURE_LOCK_STATUS_LAST_FIELD_ON_DISPLAY)) && /* About to overwrite current display... */
                            ((VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B) &&  /* And want to decode a B... */
                             ((VIDDEC_Data_p->PreviousDecodedPictureMPEGFrame != STVID_MPEG_FRAME_B) /* ...and previous picture decoded was not a B */
                              &&(LastButOnePictureLockStatus != VIDBUFF_PICTURE_LOCK_STATUS_RESERVED)) /* but make an exception if backward reference has been already displayed (due to bad temporal ref error recovery for instance) else we'll run out of frame buffers because of one locked in the ordering queue */
                            )
                           )
                        {
                            /* Don't want this B picture while it is on display ! */
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                    }
#endif /* #if 0 */
                    if (ErrorCode != ST_NO_ERROR)
                    {
#ifdef TRACE_DECODE
                        /* TraceBuffer(("-NoBuff !")); */
#endif /* TRACE_DECODE */

                        /* Can't find a new buffer for decode and stop when end of data is asked */
                        /* Stop if force after timeout mode is selected */
                        if ((ErrorCode == ST_ERROR_NO_MEMORY) &&
                            (VIDDEC_Data_p->Stop.WhenNoData) &&
                            (VIDDEC_Data_p->Stop.StopAfterBuffersTimeOut) &&
                            (STOS_time_after(VIDDEC_Data_p->HWDecoder.MaxBufferWaitingTime, time_now()) == 0))
                        {
                            DoStopNow(DecodeHandle, TRUE);
                        }
                        if (ErrorCode == STVID_ERROR_NOT_AVAILABLE)
                        {
                            /* Impossible to allocate second field picture buffer: MPEG doesn't handle this case: cancel this decode and reset decoder (necessary on 55xx) */
                            VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible = FALSE;
                            VIDDEC_Data_p->ForTask.NoDisplayAfterNextDecode = FALSE;

                            /* Error_Recovery_Case_n10 : Impossible to get an unused frame buffer.*/
                            VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible = TRUE;
#ifdef TRACE_DECODE
                            TraceBuffer(("-Skipped field decode because missing field !"));
                            TraceBuffer(("-ERCn10-"));
#endif /* TRACE_DECODE */
                        }
                        if (ErrorCode == STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE)
                        {
                            /* Picture too big for profile: skip all pictures until next sequence. */
                            VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible = FALSE;
                            VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible = TRUE;
                            VIDDEC_Data_p->Skip.UntilNextSequence = TRUE;
#ifdef STVID_DEBUG_GET_STATISTICS
                            VIDDEC_Data_p->Statistics.DecodePbSequenceNotInMemProfileSkipped ++;
#endif /* STVID_DEBUG_GET_STATISTICS */
                        }
#ifdef LIMIT_TRY_GET_BUFFER_TIME
                        /* Can't decode yet: no frame free ! */
                        if (STOS_time_after(time_now(), VIDDEC_Data_p->ForTask.MaxTryGetBufferTime) != 0)
                        {
                            clock_t Now = time_now();
                            /* Too long time trying to get a buffer to decode in: there must have been a problem, abort this decode */
    /*                            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Could not find a frame buffer before time-out !"));*/
#ifdef TRACE_DECODE
                            TraceBuffer(("\rCould not find a frame buffer before time-out ! [overtime=%d]\r\n\a", time_minus(Now, VIDDEC_Data_p->ForTask.MaxTryGetBufferTime)));
#endif /* TRACE_DECODE */
                            VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible = FALSE;
                            VIDDEC_Data_p->ForTask.NoDisplayAfterNextDecode = FALSE;
                            VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible = TRUE;
                        }
#endif /* end of LIMIT_TRY_GET_BUFFER_TIME */
                    }
                    else
                    {
                        /* Picture buffer found: now can decode ! */
                        /* To avoid filling an empty buffer */
                        if (DecodedPicture_p != NULL)
                        {
                            viddec_FillPictureBuffer(DecodedPicture_p, VIDDEC_Data_p);
                        }

                        if (DecodedDecimPicture_p != NULL)
                        {
                            viddec_FillPictureBuffer(DecodedDecimPicture_p, VIDDEC_Data_p);
                        }
#ifdef ST_ordqueue
                        if (VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame != STVID_MPEG_FRAME_B)
                        {
                            /* Remember the current decode forward reference */
                            if(  (VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
                               ||(VIDDEC_Data_p->ForTask.PictureStreamInfo.ExpectingSecondField))  /* First field */
                            {
                                if(VIDDEC_Data_p->ReferencePicture.Last_p != NULL)
                                {
                                    /* Frame or first field: take last reference */
                                    VIDDEC_Data_p->ForTask.QueueParams.CurrentDecodeForwardReferenceExtendedPresentationOrderPictureID =
                                            VIDDEC_Data_p->ReferencePicture.Last_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID;
                                    VIDDEC_Data_p->ForTask.QueueParams.CurrentDecodeForwardReferenceExtendedPresentationOrderPictureIDIsValid = TRUE;
                                }
                                else
                                {
                                    VIDDEC_Data_p->ForTask.QueueParams.CurrentDecodeForwardReferenceExtendedPresentationOrderPictureIDIsValid = FALSE;
                                }
                            }
                            else
                            {
                                if(VIDDEC_Data_p->ReferencePicture.LastButOne_p != NULL)
                                {
                                    /* Second field: don't take first field as reference ! */
                                    VIDDEC_Data_p->ForTask.QueueParams.CurrentDecodeForwardReferenceExtendedPresentationOrderPictureID =
                                            VIDDEC_Data_p->ReferencePicture.LastButOne_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID;
                                    VIDDEC_Data_p->ForTask.QueueParams.CurrentDecodeForwardReferenceExtendedPresentationOrderPictureIDIsValid = TRUE;
                                }
                                else
                                {
                                    VIDDEC_Data_p->ForTask.QueueParams.CurrentDecodeForwardReferenceExtendedPresentationOrderPictureIDIsValid = FALSE;
                                }
                            }
                        }
                        /* Remember if the current decode is expecting another field */
                        VIDDEC_Data_p->ForTask.QueueParams.CurrentDecodeExpectingSecondField = VIDDEC_Data_p->ForTask.PictureStreamInfo.ExpectingSecondField;
#endif  /* end of def ST_ordqueue */

#ifdef TRACE_DECODE
                        /* TraceBuffer(("-GetBuff(OK)")); */

#endif /* TRACE_DECODE */

                        /* Picture buffer available for decode: launch decode ! */
                        VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible = FALSE;

                        /* Prepare decode parameters */
                        DecodeParams.DataInputAddress_p = VIDDEC_Data_p->BitBuffer.Address_p;
                        DecodeParams.OutputPicture_p = DecodedPicture_p;
                        DecodeParams.OutputDecimatedPicture_p = DecodedDecimPicture_p;
                        DecodeParams.StreamInfoForDecode_p = &VIDDEC_Data_p->ForTask.StreamInfoForDecode;
                        DecodeParams.PictureInfos_p = &VIDDEC_Data_p->ForTask.PictureInfos;
                        DecodeParams.PictureStreamInfo_p = &VIDDEC_Data_p->ForTask.PictureStreamInfo;
                        /* DecodeParams.PictureSyntaxError_p = &(VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError); */

                        /* initialize the decode error flags*/
                        VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.NbOfSyntaxError = 0;
                        VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.NbOfCriticalError = 0;
                        VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.NbOfMisalignmentError = 0;
                        VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.DecodeErrorType = VIDDEC_DECODE_ERROR_TYPE_NONE;

                        /* Launch decode now ! */
                        if (((VIDDEC_Data_p->SearchFirstI) && (VIDDEC_Data_p->Skip.AllExceptFirstI)) && (VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I))
                        {
                            VIDDEC_Data_p->SearchFirstI = FALSE;
                        }
                        DecodePicture(DecodeHandle, &DecodeParams, &OverWrittenDisplay);

                        /* Update parameters */
                        VIDDEC_Data_p->PreviousDecodedPictureMPEGFrame = DecodeParams.PictureInfos_p->VideoParams.MPEGFrame;

                        VIDDEC_Data_p->ForTask.TryingToRecoverFromMaxDecodeTimeOverflow = FALSE;
#ifdef TRACE_DECODE
                        if (DecodeIndex > 0)
                        {
                            DecodeMaxTime[DecodeIndex - 1] = VIDDEC_Data_p->HWDecoder.MaxDecodeOrSkipTime;
                        }
#endif /* TRACE_DECODE */

#ifdef ST_ordqueue
                        if ((!(VIDDEC_Data_p->ForTask.NoDisplayAfterNextDecode)) &&
                            (VIDDEC_Data_p->CurrentUpdateDisplayQueueMode != VIDPROD_UPDATE_DISPLAY_QUEUE_NEVER))
                        {
                            /* Get ready to put picture in display queue */
                            if(DecodedPicture_p != NULL)
                            {
                                VIDDEC_Data_p->ForTask.QueueParams.Picture_p = DecodedPicture_p;
                            }
                            else
                            {
                                VIDDEC_Data_p->ForTask.QueueParams.Picture_p = DecodedDecimPicture_p;
                            }

                            VIDDEC_Data_p->ForTask.QueueParams.InsertionOrder = VIDQUEUE_INSERTION_ORDER_INCREASING; /* Set decreasing in backward */

                            if ((VIDDEC_Data_p->CurrentUpdateDisplayQueueMode == VIDPROD_UPDATE_DISPLAY_QUEUE_AT_DECODE_START) ||
                                (OverWrittenDisplay) ||
                                ((VIDDEC_Data_p->ForTask.PictureInfos.BitmapParams.Height * VIDDEC_Data_p->ForTask.PictureInfos.BitmapParams.Width) > VIDDEC_LIMIT_PICTURE_SIZE_TO_DISPLAY_AT_DECODE_LAUNCH)
                               )
                            {
                                /* Either the mode is that pictures are always inserted in queue at decode start,
                                or this is made necessary by the length of the decodes:
                                  * when having OVW hardware mode
                                 * when decoding big HD pictures (note: this depends on the DeviceType and could need to be removed in the future !) */
#ifdef TRACE_TAKE_RELEASE
                                    TraceBuffer((">> Insert at decode start 0x%x 0x%x (%c)\r\n",
                                    VIDDEC_Data_p->ForTask.QueueParams.Picture_p,
                                    VIDDEC_Data_p->ForTask.QueueParams.Picture_p->AssociatedDecimatedPicture_p,
                                    MPEGFrame2Char(VIDDEC_Data_p->ForTask.QueueParams.Picture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame) ));
#endif
                                ErrorCode = VIDQUEUE_InsertPictureInQueue(VIDDEC_Data_p->OrderingQueueHandle,
                                                                          VIDDEC_Data_p->ForTask.QueueParams.Picture_p,
                                                                          VIDDEC_Data_p->ForTask.QueueParams.InsertionOrder);

                                if (ErrorCode == ST_NO_ERROR)
                                {
                                    MPEGSpecific_CheckPictureToSendToDisplay(DecodeHandle);
                                }

                                /* DecodeOnce : pause or freeze after first picture is passed to display.*/
                                /* should occur only if I over I picture, with only oe frame buffer and video has been stopped ans restarted */
                                if (VIDDEC_Data_p->DecodeOnce)
                                {
                                    VIDDEC_Data_p->DecodeOnce = FALSE;
                                    /* Inform upper layer (API) that first decode is done. */
                                    STEVT_Notify(VIDDEC_Data_p->EventsHandle, VIDDEC_Data_p->RegisteredEventsID[VIDDEC_DECODE_ONCE_READY_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
                                }

                                if (VIDDEC_Data_p->NextAction.NumberOfDisplayToLaunch > 0)
                                {
                                    VIDDEC_Data_p->NextAction.NumberOfDisplayToLaunch--;
                                }
                            }
                            else
                            {
                                /* Give picture to display when decoded */
                                VIDDEC_Data_p->NextAction.LaunchDisplayWhenPossible = TRUE;
                            }
                        }
                        else
                        {
                            /* Don't give it to the display. */

                            /* VIDDEC_Data_p->ForTask.QueueParams.Picture_p has not been set to DecodedPicture_p or DecodedDecimPicture_p
                               so it will not be possible to use these picture buffers anymore. Thus we should release then now */
                            if (DecodedDecimPicture_p != NULL)
                            {
#ifdef TRACE_TAKE_RELEASE
                                TraceBuffer(("Release Decim Pict: 0x%x\r\n", DecodedDecimPicture_p));
#endif
                                RELEASE_PICTURE_BUFFER(VIDDEC_Data_p->BufferManagerHandle, DecodedDecimPicture_p, VIDCOM_VIDDEC_MODULE_BASE);
                                /* Reset DecodedDecimPicture_p to ensure that vid_dec doesn't use this pointer now that we have released this picture buffer */
                                DecodedDecimPicture_p = NULL;
                            }
                            if (DecodedPicture_p != NULL)
                            {
#ifdef TRACE_TAKE_RELEASE
                                TraceBuffer(("Release Pict: 0x%x\r\n", DecodedPicture_p));
#endif
                                RELEASE_PICTURE_BUFFER(VIDDEC_Data_p->BufferManagerHandle, DecodedPicture_p, VIDCOM_VIDDEC_MODULE_BASE);
                                /* Reset DecodedPicture_p to ensure that vid_dec doesn't use this pointer now that we have released this picture buffer */
                                DecodedPicture_p = NULL;
                            }

                            if (VIDDEC_Data_p->NextAction.NumberOfDisplayToLaunch > 0)
                            {
                                VIDDEC_Data_p->NextAction.NumberOfDisplayToLaunch--;
                            }

                            /* Don't give picture to display this time, but suppose to do it next time */
                            VIDDEC_Data_p->ForTask.NoDisplayAfterNextDecode = FALSE;
                        }
#endif  /* end of def ST_ordqueue */


                    } /* end picture buffer available for decode */
#ifdef STVID_MEASURES
                    MeasuresRB[MEASURE_END][MeasureIndexRB] = time_now();
                    MeasuresRB[MEASURE_ID][MeasureIndexRB]  = MEASURE_ID_PROCESS_DECODE;
                    if (MeasureIndexRB < NB_OF_MEASURES - 1)
                    {
                        MeasureIndexRB++;
                    }
#endif /* STVID_MEASURES */
                } /* end if(LaunchDecodeWhenPossible) */
            } /* if LaunchDecodeWhenPossible || LaunchSkipWhenPossible */
            else /* if LaunchDecodeWhenPossible == FALSE &&  LaunchSkipWhenPossible == FALSE */
            {
                /* if a start code search is not succesful (detection by MaxStartCodeSearchTime time out) and if */
                /* bit buffer level high, then abnormal. launch soft reset */
                if ((IsTimeOutOnStartCodeSearchTakenIntoAccount(VIDDEC_Data_p->ErrorRecoveryMode)) &&
                        (STOS_time_after(time_now(), VIDDEC_Data_p->HWStartCodeDetector.MaxStartCodeSearchTime) != 0) &&
                        (!VIDDEC_Data_p->HWStartCodeDetector.IsIdle))
                {
                    /* start code search success should occur immediately after its launch if LastBitBufferLevel is */
                    /* above the max picture size */
                    if (VIDDEC_Data_p->HWDecoder.LastBitBufferLevel > VIDDEC_Data_p->HWStartCodeDetector.MaxBitbufferLevelForFindingStartCode)
                    {
                        /* Tried pipeline reset but no recovery: soft reset */
                        VIDDEC_Data_p->ForTask.SoftResetPending = TRUE;
                        PushControllerCommand(DecodeHandle, CONTROLLER_COMMAND_RESET);

                        /* Signal controller that a new command is given */
                        semaphore_signal(VIDDEC_Data_p->DecodeOrder_p);
                    }
                }
            } /* if LaunchDecodeWhenPossible == FALSE &&  LaunchSkipWhenPossible == FALSE */
        } /* end ready to decode */
    } /* end not decoding */
} /* End of DecodeTaskFuncForAutomaticControlMode() function */

#ifdef ST_ordqueue
/*******************************************************************************
Name        : MPEGSpecific_CheckPictureToSendToDisplay
Description : Decide according to MPEG protocol to allow the sending to display of pictures
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void MPEGSpecific_CheckPictureToSendToDisplay(const VIDDEC_Handle_t DecodeHandle)
{
    VIDDEC_Data_t             * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    if (VIDDEC_Data_p->ForTask.QueueParams.Picture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B)
    {
        VIDCOM_PictureID_t NextPictureID;

        /* B picture : always push the picture to the display */
        VIDQUEUE_PushPicturesToDisplay(VIDDEC_Data_p->OrderingQueueHandle,
                            &(VIDDEC_Data_p->ForTask.QueueParams.Picture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID));

        if(  (VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
           ||(  (VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.PictureStructure != STVID_PICTURE_STRUCTURE_FRAME)
              &&(!VIDDEC_Data_p->ForTask.QueueParams.CurrentDecodeExpectingSecondField))) /* Second field */
        {
            /* If the stream is I0 P3 B1 B2, P3 should be pushed just after decoding B2, so we always try to push
            * the picture with ID = B piture ID + 1 */
            NextPictureID = VIDDEC_Data_p->ForTask.QueueParams.Picture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID;
            NextPictureID.ID++;
            VIDQUEUE_PushPicturesToDisplay(VIDDEC_Data_p->OrderingQueueHandle,
                                &NextPictureID);
        }

    }
    else if(VIDDEC_Data_p->Skip.AllBPictures)
    {
        VIDQUEUE_PushPicturesToDisplay(VIDDEC_Data_p->OrderingQueueHandle,
                            &(VIDDEC_Data_p->ForTask.QueueParams.Picture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID));
    }
    else
    {
        /* If the stream is I0 P1 P2 P3, each reference should be immediatly pushed, so we check
         * if the reference ID = last reference ID + 1 and then we push the picture and also we have to push
         * the reference if it is the first of the GOP (ID == 0) */
        if (  (  (VIDDEC_Data_p->ForTask.QueueParams.CurrentDecodeForwardReferenceExtendedPresentationOrderPictureIDIsValid)
               &&(VIDDEC_Data_p->ForTask.QueueParams.CurrentDecodeForwardReferenceExtendedPresentationOrderPictureID.IDExtension ==
                        VIDDEC_Data_p->ForTask.QueueParams.Picture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension)
               &&((VIDDEC_Data_p->ForTask.QueueParams.CurrentDecodeForwardReferenceExtendedPresentationOrderPictureID.ID + 1) ==
                        VIDDEC_Data_p->ForTask.QueueParams.Picture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID))
            ||(VIDDEC_Data_p->ForTask.QueueParams.Picture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID == 0))
        {
            /* If existing , push last reference picture to display                     */
            /* since we know now that no more picture preceding this picture will come  */
            VIDQUEUE_PushPicturesToDisplay(VIDDEC_Data_p->OrderingQueueHandle,
                             &(VIDDEC_Data_p->ForTask.QueueParams.Picture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID));
        }
    }
}
#endif  /* end of def ST_ordqueue */

/*******************************************************************************
Name        : GetBuffers
Description : Buffers configuration to decode
Parameters  : Video decode handle
            OUT: DecodedPicture_p_p,
            OUT: DecodedDecimPicture_p_p
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static ST_ErrorCode_t GetBuffers(VIDDEC_Data_t * VIDDEC_Data_p,
                                VIDBUFF_PictureBuffer_t ** DecodedPicture_p_p,
                                VIDBUFF_PictureBuffer_t ** DecodedDecimPicture_p_p )
{
    VIDBUFF_GetUnusedPictureBufferParams_t  GetPictureBufferParams;
    ST_ErrorCode_t                          ErrorMain;
    ST_ErrorCode_t                          ErrorSecondary;
    VIDCOM_InternalProfile_t                Profile;
    VIDDEC_DisplayedReconstruction_t        NecessaryReconstruction;

    /* Get a picture buffer to decode in , and another for decimated */
    /* reconstruction */
    GetPictureBufferParams.MPEGFrame            = VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame;
    GetPictureBufferParams.PictureStructure     = VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.PictureStructure;
    GetPictureBufferParams.TopFieldFirst        = VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.TopFieldFirst;
    GetPictureBufferParams.ExpectingSecondField = VIDDEC_Data_p->ForTask.PictureStreamInfo.ExpectingSecondField;
    GetPictureBufferParams.DecodingOrderFrameID = VIDDEC_Data_p->ForTask.PictureStreamInfo.ExtendedTemporalReference;
    GetPictureBufferParams.PictureWidth         = VIDDEC_Data_p->ForTask.PictureInfos.BitmapParams.Width;
    GetPictureBufferParams.PictureHeight        = VIDDEC_Data_p->ForTask.PictureInfos.BitmapParams.Height;

    *DecodedDecimPicture_p_p    = NULL;
    *DecodedPicture_p_p         = NULL;

    VIDBUFF_GetMemoryProfile(VIDDEC_Data_p->BufferManagerHandle, &Profile);

   if (Profile.ApplicableDecimationFactor != STVID_DECIMATION_FACTOR_NONE)
   {
       NecessaryReconstruction = VIDDEC_DISPLAYED_BOTH;
   }
   else
   {
       NecessaryReconstruction = VIDDEC_DISPLAYED_PRIMARY_ONLY;
   }

    switch(NecessaryReconstruction)
    {
        case VIDDEC_DISPLAYED_UNKNOWN:
        case VIDDEC_DISPLAYED_NONE:
            return(ST_ERROR_NO_MEMORY);
        break;
        case VIDDEC_DISPLAYED_BOTH: /* we must get the two buffers */
            ErrorMain = VIDBUFF_GetAndTakeUnusedPictureBuffer(
                                   VIDDEC_Data_p->BufferManagerHandle,
                                   &GetPictureBufferParams,
                                   DecodedPicture_p_p,
                                   VIDCOM_VIDDEC_MODULE_BASE);
            ErrorSecondary = VIDBUFF_GetAndTakeUnusedDecimatedPictureBuffer(
                                       VIDDEC_Data_p->BufferManagerHandle,
                                       &GetPictureBufferParams,
                                       DecodedDecimPicture_p_p,
                                       VIDCOM_VIDDEC_MODULE_BASE);
            if((ErrorMain != ST_NO_ERROR)||(ErrorSecondary != ST_NO_ERROR))
            {
                /* Can't continue but restore particular cases: */
                if(ErrorMain == ST_NO_ERROR)
                {
                    if((GetPictureBufferParams.PictureStructure != STVID_PICTURE_STRUCTURE_FRAME)
                        &&(!GetPictureBufferParams.ExpectingSecondField))
                    /* it was a second field of a field: still expected */
                    {
                        (*DecodedPicture_p_p)->ParsedPictureInformation.PictureGenericData.IsFirstOfTwoFields = TRUE;
                    }
#ifdef TRACE_TAKE_RELEASE
                   TraceBuffer(("Untake Main 0x%x\r\n", *DecodedPicture_p_p));
#endif
                    /* The main picture has been taken, we must release it before to exit */
                    UNTAKE_PICTURE_BUFFER(VIDDEC_Data_p->BufferManagerHandle, (*DecodedPicture_p_p), VIDCOM_VIDDEC_MODULE_BASE);
                }
                if(ErrorSecondary == ST_NO_ERROR)
                {
                    if((GetPictureBufferParams.PictureStructure != STVID_PICTURE_STRUCTURE_FRAME)
                        &&(!GetPictureBufferParams.ExpectingSecondField))
                    /* it was a second field of a field: still expected */
                    {
                        (*DecodedDecimPicture_p_p)->ParsedPictureInformation.PictureGenericData.IsFirstOfTwoFields = TRUE;
                    }
#ifdef TRACE_TAKE_RELEASE
                   TraceBuffer(("Untake Decim 0x%x\r\n", *DecodedDecimPicture_p_p));
#endif
                    /* The decimated picture has been taken, we must release it before to exit */
                    UNTAKE_PICTURE_BUFFER(VIDDEC_Data_p->BufferManagerHandle, (*DecodedDecimPicture_p_p), VIDCOM_VIDDEC_MODULE_BASE);
                }
                if(ErrorMain != ST_NO_ERROR)
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

        case VIDDEC_DISPLAYED_PRIMARY_ONLY: /* we must get at least the primary buffer */
            ErrorMain = VIDBUFF_GetAndTakeUnusedPictureBuffer(
                                   VIDDEC_Data_p->BufferManagerHandle,
                                   &GetPictureBufferParams,
                                   DecodedPicture_p_p,
                                   VIDCOM_VIDDEC_MODULE_BASE);
            if(ErrorMain != ST_NO_ERROR)
            {
                return(ErrorMain);
            }
            ErrorSecondary = VIDBUFF_GetAndTakeUnusedDecimatedPictureBuffer(
                                       VIDDEC_Data_p->BufferManagerHandle,
                                       &GetPictureBufferParams,
                                       DecodedDecimPicture_p_p,
                                       VIDCOM_VIDDEC_MODULE_BASE);
            /* don't check ErrorSecondary, continue anyway */
        break;

        case VIDDEC_DISPLAYED_SECONDARY_ONLY:
            ErrorMain = VIDBUFF_GetAndTakeUnusedPictureBuffer(
                                   VIDDEC_Data_p->BufferManagerHandle,
                                   &GetPictureBufferParams,
                                   DecodedPicture_p_p,
                                   VIDCOM_VIDDEC_MODULE_BASE);
#ifdef DONT_DECODE_NON_REF_OPTIM
            /* optim : check error-code only if P or I pictures */
            if(VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame
                    != STVID_MPEG_FRAME_B)
#endif /* DONT_DECODE_NON_REF_OPTIM */
            {
                if(ErrorMain != ST_NO_ERROR)
                {
                    return(ErrorMain);
                }
            }
            ErrorSecondary = VIDBUFF_GetAndTakeUnusedDecimatedPictureBuffer(
                                       VIDDEC_Data_p->BufferManagerHandle,
                                       &GetPictureBufferParams,
                                       DecodedDecimPicture_p_p,
                                       VIDCOM_VIDDEC_MODULE_BASE);
            if(ErrorSecondary != ST_NO_ERROR)
            {
                if(ErrorMain == ST_NO_ERROR)
                {
                    if((GetPictureBufferParams.PictureStructure != STVID_PICTURE_STRUCTURE_FRAME)
                        &&(!GetPictureBufferParams.ExpectingSecondField))
                    /* it was a second field of a field: still expected */
                    {
                        (*DecodedPicture_p_p)->ParsedPictureInformation.PictureGenericData.IsFirstOfTwoFields = TRUE;
                    }
#ifdef TRACE_TAKE_RELEASE
                   TraceBuffer(("Untake main 0x%x\r\n", *DecodedPicture_p_p));
#endif
                    /* The main picture has been taken, we must release it before to exit */
                    UNTAKE_PICTURE_BUFFER(VIDDEC_Data_p->BufferManagerHandle, (*DecodedPicture_p_p), VIDCOM_VIDDEC_MODULE_BASE);
                }
                return(ErrorSecondary);
            }
            /* continue to linkage/reconstruction if OK */
        break;
    }

    /* Patch the reconstruction because of possible substitution */
    if(*DecodedPicture_p_p != NULL)
    {
        (*DecodedPicture_p_p)->AssociatedDecimatedPicture_p = NULL;
        (*DecodedPicture_p_p)->FrameBuffer_p->AvailableReconstructionMode = VIDBUFF_MAIN_RECONSTRUCTION;
        (*DecodedPicture_p_p)->FrameBuffer_p->DecimationFactor = STVID_DECIMATION_FACTOR_NONE;
    }

    if(*DecodedDecimPicture_p_p != NULL)
    {
        (*DecodedDecimPicture_p_p)->AssociatedDecimatedPicture_p = NULL;
    }
   /* linkage picture-decimated */
   if((*DecodedDecimPicture_p_p != NULL)
    &&(*DecodedPicture_p_p != NULL))
   {
       (*DecodedDecimPicture_p_p)->AssociatedDecimatedPicture_p = *DecodedPicture_p_p;
       (*DecodedPicture_p_p)->AssociatedDecimatedPicture_p = *DecodedDecimPicture_p_p;
   }

   return(ST_NO_ERROR);
} /* End of GetBuffers() function */

#ifdef ST_smoothbackward
/*******************************************************************************
Name        : DecodeTaskFuncForManualControlMode
Description : Function called by the decode task in Manual.
Parameters  : Video decode handle
              IsThereOrders: Set TRUE to process all the decode orders
                             Set FALSE to just monitor HW (no SW orders processed)
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void DecodeTaskFuncForManualControlMode(const VIDDEC_Handle_t DecodeHandle, const BOOL IsThereOrders)
{
    /* Overall variables of the task */
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
    ST_ErrorCode_t  ErrorCode;
    U32 LinearBitBufferFreeSize;

    /* Variables used only in headers processing */
    U8   StartCode;
    U8   Tmp8;
    /* Used to search next start code. */
    VIDDEC_StartCodeSearchMode_t    SearchMode;
    BOOL                            FirstSliceDetect;
    void *                          SearchAddress_p;

    BOOL                            TmpBool;

    /* LaunchDecodeWhenPossible: Flag to trigger the decode/display:
      Set to TRUE to launch the decode/display of the current picture.
      Warning: setting this flag to FALSE requires launch of the start code detector:
       - this is done automatically when launching decode
       - this must be done manually in other cases ! */

    /* Variables for picture data passed to launch decode and display (updated in headers processing, completly valid only after slice) */
    VIDDEC_StartCodeFoundInfo_t     StartCodeFoundInfo;
    VIDDEC_PictureInfoFromDecode_t  PictureInfoFromDecode;

#ifdef STVID_HARDWARE_ERROR_EVENT
    STVID_HardwareError_t   HardwareErrorStatus;
#endif /* STVID_HARDWARE_ERROR_EVENT */

#ifdef ST_SecondInstanceSharingTheSameDecoder
    VIDBUFF_PictureBuffer_t * DecodedPicture_p;
    VIDBUFF_GetUnusedPictureBufferParams_t  GetPictureBufferParams;
    STVID_SequenceInfo_t SequenceInfo;
    STVID_PictureInfos_t PictureInfos ;
    StreamInfoForDecode_t StreamInfoForDecode ;
    PictureStreamInfo_t  PictureStreamInfo ;
    VIDDEC_SyntaxError_t  SyntaxError ;
    U32 * TemporalReferenceOffset_p;
    void * PictureStartAddress_p ;
#endif /* ST_SecondInstanceSharingTheSameDecoder */

    /* Check if an order is available */
    if (IsThereOrders)
    {
        /* Take start code found orders first, then user commands if there was no start code order */

        /* Process all start code commands together... */
        while (PopStartCodeCommand(DecodeHandle, &StartCode) == ST_NO_ERROR)
        {

#ifdef STVID_DEBUG_GET_STATISTICS
            VIDDEC_Data_p->Statistics.DecodeStartCodeFound ++;
#endif /* STVID_DEBUG_GET_STATISTICS */

#ifdef TRACE_DECODE
            StartCodes[StartCodeIndex] = StartCode;
            if (StartCodeIndex < (sizeof(StartCodes) / sizeof(U32)) - 1)
            {
                StartCodeIndex ++;
            }
#endif /* TRACE_DECODE */
            if (VIDDEC_Data_p->RequiredSearch.SCDetectorHasBeenLaunchedManually)
            {
#ifdef TRACE_DECODE
/*              TraceBuffer(("-sc"));*/
#endif /* TRACE_DECODE */

                /* Retrieve the address of the start code. */
                ErrorCode = GetStartCodeAddress(DecodeHandle, &(StartCodeFoundInfo.StartCodeAddress_p) );
                if (ErrorCode != ST_NO_ERROR)
                {
#ifdef DECODE_DEBUG
                    DecodeError();
#endif /* DECODE_DEBUG */
                }

                StartCodeFoundInfo.StartCodeId = StartCode;
                StartCodeFoundInfo.RelevantData = FALSE;
                VIDDEC_Data_p->StartCodeParsing.DiscardStartCode = FALSE;
                STEVT_Notify(VIDDEC_Data_p->EventsHandle, VIDDEC_Data_p->RegisteredEventsID[VIDDEC_START_CODE_FOUND_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &StartCodeFoundInfo);

                if (StartCode == SEQUENCE_END_CODE)
                {
                    STEVT_Notify(VIDDEC_Data_p->EventsHandle, VIDDEC_Data_p->RegisteredEventsID[VIDDEC_SEQUENCE_END_CODE_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
                }
                /* Flag DiscardStartCode just updated by the CallBack TrickMode which has intercepted the event START_CODE_FOUND.*/

                if (!(VIDDEC_Data_p->StartCodeParsing.DiscardStartCode))
                {
                    /* Parse start codes in STREAM PARSER mode */
                    ErrorCode = viddec_ParseStartCode(DecodeHandle, StartCode, VIDDEC_Data_p->DecodingStream_p, PARSER_MODE_STREAM_PARSER);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        /* Error_Recovery_Case_n05 : Inconsistency in start code arrival.    */
                        VIDDEC_Data_p->ForTask.SyntaxError.HasStartCodeParsingError = TRUE;
#ifdef TRACE_DECODE
                        TraceBuffer(("-ERCn5-"));
#endif /* TRACE_DECODE */
#ifdef DECODE_DEBUG
                        DecodeError();
                        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Parse Start Code Error !"));
#endif /* DECODE_DEBUG */
                    }

                    /* Process start code */
                    viddec_ProcessStartCode(DecodeHandle, StartCode, VIDDEC_Data_p->DecodingStream_p, &VIDDEC_Data_p->ForTask.PictureInfos, &VIDDEC_Data_p->ForTask.StreamInfoForDecode, &VIDDEC_Data_p->ForTask.PictureStreamInfo, &VIDDEC_Data_p->ForTask.SyntaxError, &(VIDDEC_Data_p->ForTask.TemporalReferenceOffset));

                    if (StartCode == FIRST_SLICE_START_CODE)
                    {
                        /* Notify we have finished to process a start code. */
                        PictureInfoFromDecode.StreamInfoForDecode  = VIDDEC_Data_p->ForTask.StreamInfoForDecode;
                        PictureInfoFromDecode.PictureInfos         = VIDDEC_Data_p->ForTask.PictureInfos;
                        PictureInfoFromDecode.AdditionalStreamInfo = VIDDEC_Data_p->ForTask.PictureStreamInfo;
                        PictureInfoFromDecode.NbFieldsToDisplay    = VIDDEC_Data_p->NbFieldsDisplayNextDecode;
                        PICTUREID_FROM_EXTTEMPREF(PictureInfoFromDecode.ExtendedPresentationOrderPictureID,
                                                  VIDDEC_Data_p->ForTask.PictureStreamInfo.ExtendedTemporalReference,
                                                  VIDDEC_Data_p->ForTask.TemporalReferenceOffset);

                        StartCodeFoundInfo.RelevantData             = TRUE;
                        StartCodeFoundInfo.PictureInfoFromDecode_p  = &PictureInfoFromDecode;
                        StartCodeFoundInfo.SyntaxError_p            = &(VIDDEC_Data_p->ForTask.SyntaxError);
                        StartCodeFoundInfo.StartCodeId              = StartCode;

                 #ifdef STVID_DEBUG_GET_STATISTICS
                        VIDDEC_Data_p->Statistics.DecodePictureFound ++;
                        switch (VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.MPEGFrame)
                        {
                            case STVID_MPEG_FRAME_I :
                                VIDDEC_Data_p->Statistics.DecodePictureFoundMPEGFrameI ++;
                                break;

                            case STVID_MPEG_FRAME_P :
                                VIDDEC_Data_p->Statistics.DecodePictureFoundMPEGFrameP ++;
                                break;

                            case STVID_MPEG_FRAME_B :
                                VIDDEC_Data_p->Statistics.DecodePictureFoundMPEGFrameB ++;
                                break;

                            default :
                                break;
                        }
                  #endif /* STVID_DEBUG_GET_STATISTICS */


                        STEVT_Notify(VIDDEC_Data_p->EventsHandle, VIDDEC_Data_p->RegisteredEventsID[VIDDEC_START_CODE_SLICE_FOUND_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &StartCodeFoundInfo);
                    }
                    /* Bit rate eval in manual mode */
                    if (StartCode == PICTURE_START_CODE)
                    {
                        EvaluateBitRateValue(DecodeHandle, &VIDDEC_Data_p->ForTask.PictureInfos);
                    }

#ifdef STVID_DEBUG_GET_STATISTICS
                     if ((StartCode >= SMALLEST_SYSTEM_START_CODE))
                     {
                            VIDDEC_Data_p->Statistics.DecodePbStartCodeFoundInvalid ++;

                            if ((StartCode >= SMALLEST_VIDEO_PACKET_START_CODE) && (StartCode <= GREATEST_VIDEO_PACKET_START_CODE))
                            {
                                VIDDEC_Data_p->Statistics.DecodePbStartCodeFoundVideoPES ++;
                            }
                     }
                     else if ((StartCode > FIRST_SLICE_START_CODE) && (StartCode <= GREATEST_SLICE_START_CODE))
                     {
                           VIDDEC_Data_p->Statistics.DecodePbStartCodeFoundInvalid ++;
                     }
#endif /* STVID_DEBUG_GET_STATISTICS */
                 }
                /* Check if there are interrupt to process after each start code process */
                CheckAndProcessInterrupt(DecodeHandle);

                /* if Start code found is a user data, then all the datas will be parsed until the next */
                /* start code which flags the end of these datas. These start code has to be treated  */
                /* as if it was found by a start code detector launched manually ! */
                /* ==> keep SCDetectorHasBeenLaunchedManually TRUE. */
                if (StartCode != USER_DATA_START_CODE )
                {
                    VIDDEC_Data_p->RequiredSearch.SCDetectorHasBeenLaunchedManually = FALSE;
                }
            } /* end if Manually */
        } /* end while(start code) */

        /* Process all user commands together only if no start code commands... */
        while (PopControllerCommand(DecodeHandle, &Tmp8) == ST_NO_ERROR)
        {
            switch ((ControllerCommandID_t) Tmp8)
            {
                case CONTROLLER_COMMAND_RESET :
                    VIDDEC_Data_p->ForTask.SoftResetPending = FALSE;
                    VIDDEC_Data_p->NextAction.LaunchDisplayWhenPossible = FALSE;
                    VIDDEC_Data_p->NextAction.NumberOfDisplayToLaunch = 0;

                    /* Reset decoder  */
                    SeekBeginningOfStream(DecodeHandle);

                    if (VIDDEC_Data_p->ControlModeInsideHAL != VIDDEC_MANUAL_CONTROL_MODE)
                    {
                        /* Mode changing: coming to manual mode */
                        HALDEC_SetSmoothBackwardConfiguration(VIDDEC_Data_p->HALDecodeHandle, TRUE);
                        VIDDEC_Data_p->RequiredSearch.SCDetectorHasBeenLaunchedManually = FALSE;
                        /* Now mode has changed */
                        VIDDEC_Data_p->ControlModeInsideHAL = VIDDEC_MANUAL_CONTROL_MODE;
                    }

#if defined(TRACE_DECODE) || defined(TRACE_TAKE_RELEASE)
                    TraceBuffer(("***Reset*** \r\n"));
#endif /* TRACE_DECODE || TRACE_TAKE_RELEASE */
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Controller has been reset."));
                    break;

/*                case CONTROLLER_COMMAND_PIPELINE_RESET :*/
                    /* Caution: pipeline reset skips decoder to next picture ! */
/*                    HALDEC_PipelineReset(VIDDEC_Data_p->HALDecodeHandle);*/
/*                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Pipeline reset."));*/
/*                    break;*/

/*                case CONTROLLER_COMMAND_DECODING_ERROR :*/
/*                    DecodeHasErrors(DecodeHandle);*/
/*                    break;*/

                default :
                    break;
            } /* end switch(command) */
        } /* end while(controller command) */

        /* Check if there are interrupts to process after each command process */
        CheckAndProcessInterrupt(DecodeHandle);

    } /* end process controller commands */

#ifdef STVID_HARDWARE_ERROR_EVENT
    /* Monitor HAL hardware error, and if required, notify it to application.   */
    HardwareErrorStatus = HALDEC_GetHardwareErrorStatus (VIDDEC_Data_p->HALDecodeHandle);
    if (HardwareErrorStatus != STVID_HARDWARE_NO_ERROR_DETECTED)
    {
        VIDDEC_NotifyDecodeEvent(DecodeHandle, STVID_HARDWARE_ERROR_EVT, (const void *) &(HardwareErrorStatus));
    }
#endif /* STVID_HARDWARE_ERROR_EVENT */

    /* Are we in the DECODING state (VIDDEC_DecodePicture asked by TM) ?    */
    /* - Is TO reached ? or Is LaunchDecode to be done ?                    */
    /* Check if max decode time is over */
    if (!(IsHWDecoderIdle(DecodeHandle)))
    {
        if (STOS_time_after(time_now(), VIDDEC_Data_p->HWDecoder.MaxDecodeOrSkipTime) != 0)
        {
           if (!(VIDDEC_Data_p->ForTask.TryingToRecoverFromMaxDecodeTimeOverflow))
           {
             /* Too long time decode was launched, there must have been a problem */
#ifdef TRACE_DECODE
             clock_t Now = time_now();
             TraceBuffer(("-**********DEC/SKIP overtime[%d]**********\a", time_minus(Now, VIDDEC_Data_p->HWDecoder.MaxDecodeOrSkipTime)));
#endif /* TRACE_DECODE */
#ifdef STVID_DEBUG_GET_STATISTICS
               VIDDEC_Data_p->Statistics.DecodePbDecodeTimeOutError ++;
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef TRACE_DECODE
               if (DecodeIndex > 0)
               {
                  DecodeOvertime[DecodeIndex - 1] = time_now();
               }
#endif

               /* TimeOut is a 'decode error' case. The actions to take are managed by the viddec_DecodeHasErrors functions. */
               viddec_DecodeHasErrors(DecodeHandle, VIDDEC_DECODE_ERROR_TYPE_TIMEOUT);

              /* Allow some time to recover */
              VIDDEC_Data_p->HWDecoder.MaxDecodeOrSkipTime = time_plus(VIDDEC_Data_p->HWDecoder.MaxDecodeOrSkipTime, MAX_RECOVER_TIME);
              VIDDEC_Data_p->ForTask.TryingToRecoverFromMaxDecodeTimeOverflow = TRUE;
         }
         else
         {
            /* Tried pipeline reset but no recovery: soft reset */
            VIDDEC_Data_p->ForTask.SoftResetPending = TRUE;
            PushControllerCommand(DecodeHandle, CONTROLLER_COMMAND_RESET);
            /* Signal controller that a new command is given */
            semaphore_signal(VIDDEC_Data_p->DecodeOrder_p);
         }
      }

      /* Check if decode is to be launched, but don't do it if display is to be launched: QueueParams would be overwritten !
      Check this last, so that IsHWDecoderIdle(), which returns FALSE after launch of decode, doesn't trigger display launch too early */
      else if ( (VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible) && (VIDDEC_Data_p->DecodeParams.StreamInfoForDecode_p != NULL) )
      {
#ifdef STVID_MEASURES
          MeasuresRB[MEASURE_BEGIN][MeasureIndexRB] = time_now();
#endif /* STVID_MEASURES */

          /* Picture buffer available for decode: launch decode ! */
          VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible = FALSE;

          /* Launch decode */
          DecodePicture(DecodeHandle, &VIDDEC_Data_p->DecodeParams, &TmpBool);
#ifdef TRACE_DECODE
          /*  TraceBuffer(("Decode Picture \r\n"));*/
#endif /* TRACE_DECODE */

          /* Launching decode does NOT launch the SC detector in manual ! */
          /* VIDDEC_Data_p->HWStartCodeDetector.IsIdle = TRUE; */ /* HG: now done in DecodePicture() only if automatic ! */

#ifdef STVID_MEASURES
          MeasuresRB[MEASURE_END][MeasureIndexRB] = time_now();
          MeasuresRB[MEASURE_ID][MeasureIndexRB]  = MEASURE_ID_PROCESS_DECODE;
          if (MeasureIndexRB < NB_OF_MEASURES - 1)
          {
              MeasureIndexRB++;
          }
#endif /* STVID_MEASURES */
      } /* end if(VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible) */
    }
    else
    {
#ifdef ST_SecondInstanceSharingTheSameDecoder
            if (VIDDEC_Data_p->StillDecodeParams.DecodeStillRequested)
            {
                VIDDEC_Data_p->StillDecodeParams.HeaderData.SearchThroughHeaderDataNotCPU = FALSE;
                viddec_ResetStream(&(VIDDEC_Data_p->StillDecodeParams.MPEG2BitStream));
                ErrorCode = viddec_FindOneSequenceAndPicture(&(VIDDEC_Data_p->StillDecodeParams.HeaderData),
                                                FALSE,
                                                (U8*) (VIDDEC_Data_p->StillDecodeParams.BitBufferAddress_p),
                                                &(VIDDEC_Data_p->StillDecodeParams.MPEG2BitStream), &PictureInfos,
                                                &StreamInfoForDecode,
                                                &PictureStreamInfo,
                                                &SyntaxError,
                                                TemporalReferenceOffset_p,
                                                &PictureStartAddress_p)  ;
                ErrorCode = ComputeSequenceInformation(&(VIDDEC_Data_p->StillDecodeParams.MPEG2BitStream), &SequenceInfo, &SyntaxError);

                VIDDEC_Data_p->StillDecodeParams.BroadcastProfile = VIDDEC_Data_p->BroadcastProfile ;
                VIDDEC_Data_p->StillDecodeParams.StillStartCodeProcessing.TimeCode = VIDDEC_Data_p->StartCodeProcessing.TimeCode  ;
                VIDDEC_Data_p->StillDecodeParams.StillStartCodeProcessing.ExpectingSecondField = VIDDEC_Data_p->StartCodeProcessing.ExpectingSecondField ;
                VIDDEC_Data_p->StillDecodeParams.StillStartCodeProcessing.ExtendedTemporalReference = VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference;

                ErrorCode = FillPictureParamsStructuresForStill( VIDDEC_Data_p->StillDecodeParams ,
                                                &SequenceInfo,  &(VIDDEC_Data_p->StillDecodeParams.MPEG2BitStream), &PictureInfos, &StreamInfoForDecode,
                                                    &PictureStreamInfo);

                GetPictureBufferParams.MPEGFrame            = PictureInfos.VideoParams.MPEGFrame;
                GetPictureBufferParams.PictureStructure     = PictureInfos.VideoParams.PictureStructure;
                GetPictureBufferParams.TopFieldFirst        = PictureInfos.VideoParams.TopFieldFirst;
                GetPictureBufferParams.ExpectingSecondField = PictureStreamInfo.ExpectingSecondField;
                GetPictureBufferParams.DecodingOrderFrameID = PictureStreamInfo.ExtendedTemporalReference;
                GetPictureBufferParams.PictureWidth         = PictureInfos.BitmapParams.Width;
                GetPictureBufferParams.PictureHeight        = PictureInfos.BitmapParams.Height;

                ErrorCode = VIDBUFF_GetAndTakeUnusedPictureBuffer(
                                VIDDEC_Data_p->StillDecodeParams.BufferManagerHandle,
                                &GetPictureBufferParams,
                                &DecodedPicture_p,
                                VIDCOM_VIDDEC_MODULE_BASE);

                HALDEC_StartDecodeStillPicture(VIDDEC_Data_p->HALDecodeHandle, VIDDEC_Data_p->StillDecodeParams.BitBufferAddress_p, DecodedPicture_p, &StreamInfoForDecode );

                PictureInfos.BitmapParams.Data1_p = DecodedPicture_p->FrameBuffer_p->Allocation.Address_p;
                PictureInfos.BitmapParams.Size1   = DecodedPicture_p->FrameBuffer_p->Allocation.TotalSize -
                                                    DecodedPicture_p->FrameBuffer_p->Allocation.Size2;
                PictureInfos.BitmapParams.Data2_p = DecodedPicture_p->FrameBuffer_p->Allocation.Address2_p;
                PictureInfos.BitmapParams.Size2   = DecodedPicture_p->FrameBuffer_p->Allocation.Size2;

                PictureInfos.BitmapParams.Data1_p = Virtual(PictureInfos.BitmapParams.Data1_p);
                PictureInfos.BitmapParams.Data2_p = Virtual(PictureInfos.BitmapParams.Data2_p);

                /* OLO_TO_DO: Shall we Release the Picture after the notification? */

                STEVT_Notify(VIDDEC_Data_p->StillDecodeParams.EventsHandle,
                            VIDDEC_Data_p->StillDecodeParams.RegisteredEventsID[VIDDEC_NEW_PICTURE_DECODED_EVT_ID],
                            STEVT_EVENT_DATA_TYPE_CAST &(PictureInfos));

                VIDDEC_Data_p->StillDecodeParams.DecodeStillRequested = FALSE;
            }
#endif /* ST_SecondInstanceSharingTheSameDecoder */

    }
    /* Do actions at each loop, not depending on orders (comes here at least each MAX_WAIT_ORDER_TIME ticks */
    if (VIDDEC_Data_p->RequiredSearch.StartCodeDetectorToLaunch)
    {
        SearchMode = VIDDEC_Data_p->RequiredSearch.SearchMode;
        FirstSliceDetect = VIDDEC_Data_p->RequiredSearch.FirstSliceDetect;
        SearchAddress_p = VIDDEC_Data_p->RequiredSearch.SearchAddress_p;
        ErrorCode = VIDDEC_SearchNextStartCode(DecodeHandle, SearchMode, FirstSliceDetect, SearchAddress_p);
        if (ErrorCode != ST_NO_ERROR)
        {
#ifdef DECODE_DEBUG
            DecodeError();
#endif /* DECODE_DEBUG */
        }
        VIDDEC_Data_p->RequiredSearch.StartCodeDetectorToLaunch = FALSE;
        VIDDEC_Data_p->RequiredSearch.SCDetectorHasBeenLaunchedManually = TRUE;
        /* VIDDEC_AskToSearchNextStartCode */
    }

    /* Monitor linear Bit Buffer Level injection */
    if ((VIDDEC_Data_p->DataUnderflowEventNotSatisfied) && (!VIDDEC_Data_p->OverFlowEvtAlreadySent))
    {
        VIDDEC_GetBitBufferFreeSize(DecodeHandle, &LinearBitBufferFreeSize);
        if (LinearBitBufferFreeSize < (VIDDEC_Data_p->LastLinearBitBufferFlushRequestSize / LINEAR_BIT_BUFFER_OVERFLOW_RATIO))
        {
            VIDDEC_NotifyDecodeEvent(DecodeHandle, STVID_DATA_OVERFLOW_EVT, (const void *) NULL);
        }
    }

}/* End of DecodeTaskFuncForManualControlMode() function */
#endif /* ST_smoothbackward */


/*******************************************************************************
Name        : DoStopNow
Description : Stop the decoder
Parameters  :
Assumptions :
Limitations : Caution: this function can be called from API level, i.e. from user direct call !
Limitations : Caution: This function must never be called with SendStopEvent = TRUE inside a VIDDEC_Xxxx() function, as notification of event is forbidden there !
Returns     : ST_NO_ERROR
*******************************************************************************/
static void DoStopNow(const VIDDEC_Handle_t DecodeHandle, const BOOL SendStopEvent)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
    STVID_PictureParams_t LastDecodedPictureParams;
    STVID_PictureParams_t * LastDecodedPictureParams_p;
    BOOL StopWhenNoData;

#ifdef TRACE_TAKE_RELEASE
    TraceBuffer(("DoStopNow\r\n"));
#endif

    /* Disable interrupts, we don't need them any more now that we are stopped...
    Do that as soon as possible, so that no IT changes the variables we set below ! */
    VIDDEC_Data_p->VideoInterrupt.Mask = 0;
#ifndef STI7710_STI5105_WA_GNBvd50512
    /* Stop the current decode to avoid writing to a released frame buffer */
    HALDEC_PipelineReset(VIDDEC_Data_p->HALDecodeHandle);
#endif /* STI7710_STI5105_WA_GNBvd50512 */
    /* Disable interrupts */
    /* Don't set the mask but directly set 0, as this can be called from API call, so the decode task may overwrite the value of VIDDEC_Data_p->VideoInterrupt.Mask */
    HALDEC_SetInterruptMask(VIDDEC_Data_p->HALDecodeHandle, /*VIDDEC_Data_p->VideoInterrupt.Mask*/ 0);
    VIDDEC_Data_p->VideoInterrupt.Mask = 0;
    /* Clear all interrupts already queued */
    ClearAllPendingInterrupt(DecodeHandle);
#ifdef STI7710_STI5105_WA_GNBvd50512
    /* Stop the current decode to avoid writing to a released frame buffer */
    HALDEC_PipelineReset(VIDDEC_Data_p->HALDecodeHandle);
#endif /* STI7710_STI5105_WA_GNBvd50512 */

    /* Store Stop End Of Data Request */
    StopWhenNoData = VIDDEC_Data_p->Stop.WhenNoData;

    VIDDEC_Data_p->Stop.WhenNextReferencePicture = FALSE;
    VIDDEC_Data_p->Stop.WhenNoData = FALSE;
    VIDDEC_Data_p->Stop.StopAfterBuffersTimeOut = FALSE;
    VIDDEC_Data_p->Stop.DiscontinuityStartCodeInserted = FALSE;
    VIDDEC_Data_p->Stop.WhenNextIPicture = FALSE;
    VIDDEC_Data_p->Stop.AfterCurrentPicture = FALSE;
    VIDDEC_Data_p->Stop.IsPending = FALSE;

    /* Caution ! We may not detect the last DEI because mask was set to 0. So force end of decode manually. */
    VIDDEC_Data_p->HWDecoder.HWState = VIDDEC_HARDWARE_STATE_IDLE;
    VIDDEC_Data_p->HWDecoder.DecodingPicture_p = NULL;

#ifdef ST_ordqueue
    /* Flush all pictures in ordering queue to display */
    VIDQUEUE_PushAllPicturesToDisplay(VIDDEC_Data_p->OrderingQueueHandle);
#endif /* ST_ordqueue */

    /* Release pictures kept locked by decode */
    VIDDEC_ReleaseReferencePictures(DecodeHandle);

    /* Reset the state do not do actions between a stop and a start. */
    VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible = FALSE;
#ifdef ST_ordqueue
    VIDDEC_Data_p->NextAction.LaunchDisplayWhenPossible = FALSE;
    VIDDEC_Data_p->NextAction.NumberOfDisplayToLaunch = 0;
#endif  /* end of def ST_ordqueue */
    VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible = FALSE;
    VIDDEC_Data_p->ForTask.NoDisplayAfterNextDecode = FALSE;
    if (SendStopEvent)
    {
        if ((VIDDEC_Data_p->HWDecoder.LastDecodedPictureInfos_p!=NULL) && (!StopWhenNoData))
        {
            vidcom_PictureInfosToPictureParams(VIDDEC_Data_p->HWDecoder.LastDecodedPictureInfos_p,
                &LastDecodedPictureParams,
                VIDDEC_Data_p->HWDecoder.LastPresentationOrderPictureID);
            LastDecodedPictureParams_p = &LastDecodedPictureParams;
        }
        else
        {
            LastDecodedPictureParams_p = NULL;

        }
        STEVT_Notify(VIDDEC_Data_p->EventsHandle, VIDDEC_Data_p->RegisteredEventsID[VIDDEC_STOPPED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST LastDecodedPictureParams_p);
    }

    /* Set state to stopped */
    VIDDEC_Data_p->DecoderState = VIDDEC_DECODER_STATE_STOPPED;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Decoder stopped."));

    /* A picture decode was maybe in progress when the Stop command arrived.
       so we should wake up the VID_DEC task in order to release this picture */
    semaphore_signal(((VIDDEC_Data_t *) DecodeHandle)->DecodeOrder_p);

} /* End of DoStopNow() function */

/*******************************************************************************
Name        : ExtractBitRateValueFromStream
Description : Extract the Bit Rate Value from the stream
Parameters  :
Assumptions :
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER, STVID_ERROR_NOT_AVAILABLE, ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t ExtractBitRateValueFromStream(const VIDDEC_Handle_t DecodeHandle, U32 * const BitRateValue_p)
{
    VIDDEC_Data_t * VIDDEC_Data_p;

    if (BitRateValue_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Find structure */
    VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    /* init */
    *BitRateValue_p = 0;

    /* If Sequence header & Sequence extension have not been  */
    /* still received (MPEG2), return STVID_ERROR_NOT_AVAILABLE */
    if ((VIDDEC_Data_p->DecodingStream_p)->BitStreamProgress < AFTER_SEQUENCE_EXTENSION)
    {
        return(STVID_ERROR_NOT_AVAILABLE);
    }

    /* 18 less significant bits from sequence infos */
    *BitRateValue_p = (U32)((VIDDEC_Data_p->DecodingStream_p)->Sequence.bit_rate_value);

    /* 12 most significant bits from sequence extension infos */
    if (!(VIDDEC_Data_p->DecodingStream_p)->MPEG1BitStream)
    {
        *BitRateValue_p = *BitRateValue_p | ((U32)((VIDDEC_Data_p->DecodingStream_p)->SequenceExtension.bit_rate_extension) << 18);
    }

    return(ST_NO_ERROR);
} /* End of ExtractBitRateValueFromStream() function */


/*******************************************************************************
Name        : InitDecodeStructure
Description : Initialise decode structure
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void InitDecodeStructure(const VIDDEC_Handle_t DecodeHandle)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
    U32 LoopCounter;

    VIDDEC_Data_p->BitBufferAllocatedByUs = FALSE;

    VIDDEC_Data_p->DecoderState = VIDDEC_DECODER_STATE_STOPPED;
    VIDDEC_Data_p->DecoderIsFreezed = FALSE;
    VIDDEC_Data_p->DecodedPictures = STVID_DECODED_PICTURES_ALL;
    VIDDEC_Data_p->DecodedPicturesBeforeLastFreeze = VIDDEC_Data_p->DecodedPictures;
    VIDDEC_Data_p->DecodeAction = VIDDEC_DECODE_ACTION_NORMAL;
    VIDDEC_Data_p->SearchFirstI = FALSE;
    VIDDEC_Data_p->Skip.AllExceptFirstI = FALSE;
    VIDDEC_Data_p->Skip.AllIPictures = FALSE;
    VIDDEC_Data_p->Skip.AllPPictures = FALSE;
    VIDDEC_Data_p->Skip.AllBPictures = FALSE;
    VIDDEC_Data_p->Skip.NextBPicture = FALSE;
    VIDDEC_Data_p->Skip.UntilNextIPicture = FALSE;
    VIDDEC_Data_p->Skip.UntilNextSequence = FALSE;
    VIDDEC_Data_p->NbFieldsDisplayNextDecode = 0;

    VIDDEC_Data_p->HDPIPParams.Enable          = FALSE;
    VIDDEC_Data_p->HDPIPParams.WidthThreshold  = 0;
    VIDDEC_Data_p->HDPIPParams.HeightThreshold = 0;

    InitUnderflowEvent(DecodeHandle);

    VIDDEC_Data_p->LastStartCode = DISCONTINUITY_START_CODE;

    /* Decode events notification configuration ----------------------------- */

    /* Initialise the event notification configuration. */
    for (LoopCounter = 0; LoopCounter < VIDDEC_NB_REGISTERED_EVENTS_IDS; LoopCounter++)
    {
        VIDDEC_Data_p->EventNotificationConfiguration[LoopCounter].NotificationSkipped                     = 0;
        VIDDEC_Data_p->EventNotificationConfiguration[LoopCounter].ConfigureEventParam.NotificationsToSkip = 0;
        VIDDEC_Data_p->EventNotificationConfiguration[LoopCounter].ConfigureEventParam.Enable              = TRUE;
    }
    /* Set the default value of specific event. */
    VIDDEC_Data_p->EventNotificationConfiguration[VIDDEC_NEW_PICTURE_DECODED_EVT_ID].ConfigureEventParam.Enable = FALSE;

    /* Controller ----------------------------------------------------------- */

    VIDDEC_Data_p->DecodeTask.IsRunning  = FALSE;
#ifdef STVID_MEASURES
    VIDDEC_Data_p->DecodeOrder_p = STOS_SemaphoreCreateFifo(VIDDEC_Data_p->CPUPartition_p,0);
#else /* STVID_MEASURES */
    VIDDEC_Data_p->DecodeOrder_p = STOS_SemaphoreCreateFifoTimeOut(VIDDEC_Data_p->CPUPartition_p,0);
#endif /* not STVID_MEASURES */

	#ifdef BENCHMARK_WINCESTAPI
		P_ADDSEMAPHORE(VIDDEC_Data_p->DecodeOrder_p , "VIDDEC_Data_p->DecodeOrder_p");
	#endif

    VIDDEC_Data_p->VideoInterrupt.Mask      = 0;
    VIDDEC_Data_p->VideoInterrupt.Status    = 0;
    VIDDEC_Data_p->VideoInterrupt.InstallHandler = FALSE;
    VIDDEC_Data_p->VideoInterrupt.Level     = 0;
    VIDDEC_Data_p->VideoInterrupt.Number    = 0;

    /* Initialise commands queue */
    VIDDEC_Data_p->CommandsBuffer.Data_p         = VIDDEC_Data_p->CommandsData;
    VIDDEC_Data_p->CommandsBuffer.TotalSize      = sizeof(VIDDEC_Data_p->CommandsData);
    VIDDEC_Data_p->CommandsBuffer.BeginPointer_p = VIDDEC_Data_p->CommandsBuffer.Data_p;
    VIDDEC_Data_p->CommandsBuffer.UsedSize       = 0;
    VIDDEC_Data_p->CommandsBuffer.MaxUsedSize    = 0;

    /* Initialise start codes queue */
    VIDDEC_Data_p->StartCodeBuffer.Data_p         = VIDDEC_Data_p->StartCodeData;
    VIDDEC_Data_p->StartCodeBuffer.TotalSize      = sizeof(VIDDEC_Data_p->StartCodeData);
    VIDDEC_Data_p->StartCodeBuffer.BeginPointer_p = VIDDEC_Data_p->StartCodeBuffer.Data_p;
    VIDDEC_Data_p->StartCodeBuffer.UsedSize       = 0;
    VIDDEC_Data_p->StartCodeBuffer.MaxUsedSize    = 0;

    /* Initialise interrupts queue */
    VIDDEC_Data_p->InterruptsBuffer.Data_p        = VIDDEC_Data_p->InterruptsData;
    VIDDEC_Data_p->InterruptsBuffer.TotalSize     = sizeof(VIDDEC_Data_p->InterruptsData) / sizeof(U32);
    VIDDEC_Data_p->InterruptsBuffer.BeginPointer_p = VIDDEC_Data_p->InterruptsBuffer.Data_p;
    VIDDEC_Data_p->InterruptsBuffer.UsedSize      = 0;
    VIDDEC_Data_p->InterruptsBuffer.MaxUsedSize   = 0;

#ifdef ST_ordqueue
    VIDDEC_Data_p->CurrentUpdateDisplayQueueMode = VIDPROD_UPDATE_DISPLAY_QUEUE_NEVER;
    VIDDEC_Data_p->UpdateDisplayQueueMode        = VIDPROD_UPDATE_DISPLAY_QUEUE_NEVER;
#endif  /* end of def ST_ordqueue */
    VIDDEC_Data_p->DecodeOnce    = FALSE;
    VIDDEC_Data_p->RealTime      = FALSE;
    VIDDEC_Data_p->LaunchStartCodeSearchToResume = FALSE;
    VIDDEC_Data_p->SendSequenceInfoEventOnceAfterStart = FALSE;
    VIDDEC_Data_p->ComputeSequenceInfoOnNextPictureOrGroup = FALSE;
    VIDDEC_Data_p->PreviousDecodedPictureMPEGFrame = STVID_MPEG_FRAME_I;

    VIDDEC_Data_p->Stop.WhenNextReferencePicture = FALSE;
    VIDDEC_Data_p->Stop.WhenNoData = FALSE;
    VIDDEC_Data_p->Stop.StopAfterBuffersTimeOut = FALSE;
    VIDDEC_Data_p->Stop.DiscontinuityStartCodeInserted = FALSE;
    VIDDEC_Data_p->Stop.WhenNextIPicture = FALSE;
    VIDDEC_Data_p->Stop.AfterCurrentPicture = FALSE;
    VIDDEC_Data_p->Stop.IsPending = FALSE;
    VIDDEC_Data_p->UnsuccessfullDecodeCount = 0;

    /* Initialize Bit Rate computing Params */
    VIDDEC_Data_p->BitRateValueEvaluate.LastBitBufferOutputCounter      = 0;
    VIDDEC_Data_p->BitRateValueEvaluate.BitRateValueIsComputable        = TRUE;
    VIDDEC_Data_p->BitRateValueEvaluate.BitRateValidValueNb             = 0;
    VIDDEC_Data_p->BitRateValueEvaluate.BitRateIndex                    = 0;
    VIDDEC_Data_p->BitRateValueEvaluate.BitRateValueIsUnderComputing    = FALSE;
    for (LoopCounter = 0; LoopCounter < VIDDEC_NB_PICTURES_USED_FOR_BITRATE; LoopCounter++)
    {
        VIDDEC_Data_p->BitRateValueEvaluate.Time[LoopCounter]           = 0;
        VIDDEC_Data_p->BitRateValueEvaluate.BitBufferSize[LoopCounter]  = 0;
    }

    /* Decoder -------------------------------------------------------------- */

    VIDDEC_Data_p->ReferencePicture.LastButOne_p = NULL;
    VIDDEC_Data_p->ReferencePicture.Last_p = NULL;
    VIDDEC_Data_p->ReferencePicture.MissingPrevious = TRUE;
    VIDDEC_Data_p->ReferencePicture.KeepLastReference = FALSE;
    VIDDEC_Data_p->ReferencePicture.ReferencePictureSemaphore_p = STOS_SemaphoreCreatePriority(VIDDEC_Data_p->CPUPartition_p,1);

#ifdef BENCHMARK_WINCESTAPI
	P_ADDSEMAPHORE(VIDDEC_Data_p->ReferencePicture.ReferencePictureSemaphore_p , " VIDDEC_Data_p->ReferencePicture.ReferencePictureSemaphore_p");
#endif

    VIDDEC_Data_p->HWDecoder.HWState            = VIDDEC_HARDWARE_STATE_IDLE;
    VIDDEC_Data_p->HWDecoder.DecodingPicture_p  = NULL; /* Not currently decoding */
    VIDDEC_Data_p->HWDecoder.LastDecodedPictureInfos_p = NULL;
    VIDDEC_Data_p->HWDecoder.LastPresentationOrderPictureID = 0;
    VIDDEC_Data_p->HWDecoder.LastBitBufferLevel = 0;

    VIDDEC_Data_p->HWStartCodeDetector.IsIdle   = TRUE;

    /* Parser --------------------------------------------------------------- */

    VIDDEC_Data_p->BroadcastProfile = STVID_BROADCAST_DVB; /* no sense, just not to have 0... */

    VIDDEC_Data_p->HeaderData.Bits                = 0;
    VIDDEC_Data_p->HeaderData.NumberOfBits        = 0;
    VIDDEC_Data_p->HeaderData.SearchThroughHeaderDataNotCPU = TRUE;
    VIDDEC_Data_p->HeaderData.WaitForEverWhenHeaderDataEmpty = FALSE;
    VIDDEC_Data_p->HeaderData.IsHeaderDataSpoiled = FALSE;
    VIDDEC_Data_p->HeaderData.CPUSearchLastReadAddressPlusOne_p = NULL;
#ifdef STVID_CONTROL_HEADER_BITS
    VIDDEC_Data_p->HeaderData.BitsCounter         = 0;
    VIDDEC_Data_p->HeaderData.MissingBitsCounter  = 0;
#endif /* STVID_CONTROL_HEADER_BITS */

    VIDDEC_Data_p->StartCodeParsing.IsOnGoing   = FALSE;
    VIDDEC_Data_p->StartCodeParsing.IsGettingUserData = FALSE;
    VIDDEC_Data_p->StartCodeParsing.IsOK        = TRUE;
    VIDDEC_Data_p->StartCodeParsing.HasBadMarkerBit = FALSE;
    VIDDEC_Data_p->StartCodeParsing.HasUserDataProcessTriggeredStartCode = FALSE;
#ifdef ST_smoothbackward
    VIDDEC_Data_p->StartCodeParsing.DiscardStartCode = FALSE;
#endif /* ST_smoothbackward */

    /* Initialize the Manual Mode. */
#ifdef ST_smoothbackward
    VIDDEC_Data_p->RequiredSearch.StartCodeDetectorToLaunch = FALSE;
    VIDDEC_Data_p->RequiredSearch.SCDetectorHasBeenLaunchedManually = FALSE;
#endif /* ST_smoothbackward */

    VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.NbOfPictureSCPerFirstSliceSC = 0;
    VIDDEC_Data_p->StartCodeProcessing.PreviousStartCode = SEQUENCE_ERROR_CODE; /* Value should not be used in actions for previous start code */
    VIDDEC_Data_p->StartCodeProcessing.FirstPictureEver = TRUE;
    VIDDEC_Data_p->StartCodeProcessing.ExpectingSecondField = FALSE;
    VIDDEC_Data_p->StartCodeProcessing.TemporalReferenceOffset = 0;
    VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference = 0;
    VIDDEC_Data_p->StartCodeProcessing.PreviousExtendedTemporalReference = (U32) (-1); /* To start offset at 0 and not 1 (if detects that 0 is used then takes 1) */
    VIDDEC_Data_p->StartCodeProcessing.PreviousGreatestReferenceExtendedTemporalReference = 0;
    VIDDEC_Data_p->StartCodeProcessing.PreviousPictureMPEGFrame = STVID_MPEG_FRAME_I;
    VIDDEC_Data_p->StartCodeProcessing.PreviousPictureNbFieldsDisplay = 0; /* Used for PTS interpolation */
    VIDDEC_Data_p->StartCodeProcessing.PreviousReferenceTotalNbFieldsDisplay = 0; /* Used for PTS interpolation */
    VIDDEC_Data_p->StartCodeProcessing.SupposedPicturesDisplayDuration = 0; /* Used for PTS interpolation */
    VIDDEC_Data_p->StartCodeProcessing.PreviousExtendedTemporalReferenceForPTS = (U32) (-1); /* Used for PTS interpolation */
    VIDDEC_Data_p->StartCodeProcessing.OneTemporalReferenceOverflowed = FALSE;
    VIDDEC_Data_p->StartCodeProcessing.TimeCode.Interpolated = TRUE;
    VIDDEC_Data_p->StartCodeProcessing.PictureCounter = 0;
    VIDDEC_Data_p->StartCodeProcessing.PTS.Interpolated = TRUE;
    VIDDEC_Data_p->StartCodeProcessing.PTS.IsValid = FALSE;
    VIDDEC_Data_p->StartCodeProcessing.PreviousTemporalReference = IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE; /* Impossible value */
    VIDDEC_Data_p->StartCodeProcessing.LastReferenceTemporalReference = IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE; /* Impossible value */
    VIDDEC_Data_p->StartCodeProcessing.LastButOneReferenceTemporalReference = IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE; /* Impossible value */
    VIDDEC_Data_p->StartCodeProcessing.tempRefReachedZero = FALSE;

    VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.NbOfSyntaxError = 0;
    VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.NbOfMisalignmentError = 0;
    VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.NbOfCriticalError = 0;
    VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.DecodeErrorType = VIDDEC_DECODE_ERROR_TYPE_NONE;

    VIDDEC_Data_p->BrokenLink.Active = FALSE;
    VIDDEC_Data_p->BrokenLink.GotFirstReferencePicture = FALSE; /* Not necessary */

    VIDDEC_Data_p->UserDataEventEnabled = FALSE;
    /* Completly set each time, so init is not mandatory */
    VIDDEC_Data_p->UserData.Length = 0;
    VIDDEC_Data_p->UserData.Buff_p = NULL;
    VIDDEC_Data_p->UserData.IsRegistered = FALSE;
    VIDDEC_Data_p->UserData.itu_t_t35_country_code = 0;
    VIDDEC_Data_p->UserData.itu_t_t35_country_code_extension_byte = 0;
    VIDDEC_Data_p->UserData.itu_t_t35_provider_code = 0;

    /* Initialize the Next Actions. */
    VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible = FALSE;
    VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible = FALSE;
#ifdef ST_ordqueue
    VIDDEC_Data_p->NextAction.LaunchDisplayWhenPossible = FALSE;
    VIDDEC_Data_p->NextAction.NumberOfDisplayToLaunch = 0;
#endif  /* end of def ST_ordqueue */
    VIDDEC_Data_p->ErrorRecoveryMode = STVID_ERROR_RECOVERY_FULL;

#ifdef ST_smoothbackward
    VIDDEC_Data_p->ControlMode = VIDDEC_AUTOMATIC_CONTROL_MODE;
    VIDDEC_Data_p->ControlModeInsideHAL = VIDDEC_AUTOMATIC_CONTROL_MODE;
#endif /* ST_smoothbackward */

    VIDDEC_Data_p->DecodeParams.DataInputAddress_p = NULL;
    VIDDEC_Data_p->DecodeParams.OutputPicture_p = NULL;
    VIDDEC_Data_p->DecodeParams.StreamInfoForDecode_p = NULL;
    VIDDEC_Data_p->DecodeParams.PictureInfos_p = NULL;
    VIDDEC_Data_p->DecodeParams.PictureStreamInfo_p = NULL;
#ifdef ST_smoothbackward
    VIDDEC_Data_p->DecodeParams.pTemporalReference = 0;
#endif /* ST_smoothbackward */

    VIDDEC_Data_p->DecoderOperatingMode.IsLowDelay = FALSE;

    /* Start Condition flags init */
    VIDDEC_Data_p->VbvStartCondition.FirstSequenceSinceLastSoftReset = TRUE;
    VIDDEC_Data_p->VbvStartCondition.FirstPictureAfterFirstSequence = FALSE;
    VIDDEC_Data_p->VbvStartCondition.VbvSizeToReachBeforeStartInBytes = 0;
    VIDDEC_Data_p->VbvStartCondition.VbvSizeReachedAccordingToInterruptBBT = FALSE;
    VIDDEC_Data_p->VbvStartCondition.StartWithVbvBufferSize = FALSE;
    VIDDEC_Data_p->VbvStartCondition.StartWithVbvDelay = FALSE;
#ifdef ST_avsync
    VIDDEC_Data_p->VbvStartCondition.StartWithPTSTimeComparison = FALSE;
    VIDDEC_Data_p->VbvStartCondition.FirstPicturePTSAfterFirstSequence.PTS   = 0;
    VIDDEC_Data_p->VbvStartCondition.FirstPicturePTSAfterFirstSequence.PTS33 = FALSE;
#endif /* ST_avsync */
    VIDDEC_Data_p->VbvStartCondition.IsDecodeEnable = FALSE;
    /* VIDDEC_Data_p->VbvStartCondition.VbvBufferSizeStarted = FALSE; */

#ifdef STVID_STVAPI_ARCHITECTURE
    /* Raster initialization ------------------------------------------------ */
    VIDDEC_Data_p->RasterBufManagerParams.BufferPoolId = 0;
    VIDDEC_Data_p->RasterBufManagerParams.BufferPoolIdIsAvailable = FALSE;
    VIDDEC_Data_p->RasterBufManagerParams.RasterPicturesLocked = FALSE;
#endif /* end of def STVID_STVAPI_ARCHITECTURE */

#ifdef STVID_DEBUG_GET_STATISTICS
    /* Reset statistics parameters */
    VIDDEC_Data_p->Statistics.DecodeHardwareSoftReset = 0;
    VIDDEC_Data_p->Statistics.DecodeStartCodeFound = 0;
    VIDDEC_Data_p->Statistics.DecodeSequenceFound = 0;
    VIDDEC_Data_p->Statistics.DecodeUserDataFound = 0;
    VIDDEC_Data_p->Statistics.DecodePictureFound = 0;
    VIDDEC_Data_p->Statistics.DecodePictureFoundMPEGFrameI = 0;
    VIDDEC_Data_p->Statistics.DecodePictureFoundMPEGFrameP = 0;
    VIDDEC_Data_p->Statistics.DecodePictureFoundMPEGFrameB = 0;
    VIDDEC_Data_p->Statistics.DecodePictureSkippedRequested = 0;
    VIDDEC_Data_p->Statistics.DecodePictureSkippedNotRequested = 0;
    VIDDEC_Data_p->Statistics.DecodePictureDecodeLaunched = 0;
    VIDDEC_Data_p->Statistics.DecodeStartConditionVbvDelay = 0;
    VIDDEC_Data_p->Statistics.DecodeStartConditionPtsTimeComparison = 0;
    VIDDEC_Data_p->Statistics.DecodeStartConditionVbvBufferSize = 0;
    VIDDEC_Data_p->Statistics.DecodeInterruptStartDecode = 0;
    VIDDEC_Data_p->Statistics.DecodeInterruptPipelineIdle = 0;
    VIDDEC_Data_p->Statistics.DecodeInterruptDecoderIdle = 0;
    VIDDEC_Data_p->Statistics.DecodeInterruptBitBufferEmpty = 0;
    VIDDEC_Data_p->Statistics.DecodeInterruptBitBufferFull = 0;
    VIDDEC_Data_p->Statistics.DecodePbStartCodeFoundInvalid = 0;
    VIDDEC_Data_p->Statistics.DecodePbStartCodeFoundVideoPES = 0;
    VIDDEC_Data_p->Statistics.DecodePbMaxNbInterruptSyntaxErrorPerPicture = 0;
    VIDDEC_Data_p->Statistics.DecodePbInterruptSyntaxError = 0;
    VIDDEC_Data_p->Statistics.DecodePbInterruptDecodeOverflowError = 0;
    VIDDEC_Data_p->Statistics.DecodePbInterruptDecodeUnderflowError = 0;
    VIDDEC_Data_p->Statistics.DecodePbDecodeTimeOutError = 0;
    VIDDEC_Data_p->Statistics.DecodePbInterruptMisalignmentError = 0;
    VIDDEC_Data_p->Statistics.DecodePbInterruptQueueOverflow = 0;
    VIDDEC_Data_p->Statistics.DecodePbVbvSizeGreaterThanBitBuffer = 0;
    VIDDEC_Data_p->Statistics.DecodeMinBitBufferLevelReached = 0;
    VIDDEC_Data_p->Statistics.DecodeMaxBitBufferLevelReached = 0;
    VIDDEC_Data_p->Statistics.DecodePbSequenceNotInMemProfileSkipped = 0;
    VIDDEC_Data_p->HeaderData.StatisticsDecodePbHeaderFifoEmpty = 0;
    VIDDEC_Data_p->Statistics.MaxBitRate = 0;
    VIDDEC_Data_p->Statistics.MinBitRate = 0xFFFFFFFF;
    VIDDEC_Data_p->Statistics.LastBitRate = 0;
#ifdef STVID_HARDWARE_ERROR_EVENT
    VIDDEC_Data_p->Statistics.DecodePbHardwareErrorMissingPID = 0;
    VIDDEC_Data_p->Statistics.DecodePbHardwareErrorSyntaxError = 0;
    VIDDEC_Data_p->Statistics.DecodePbHardwareErrorTooSmallPicture = 0;
#endif /* STVID_HARDWARE_ERROR_EVENT */
#endif /* STVID_DEBUG_GET_STATISTICS */

#ifdef DECODE_DEBUG
    VIDDEC_Data_p->Debug.LastDecodedPictureMpegFrame = STVID_MPEG_FRAME_I;
#endif /* DECODE_DEBUG */

#ifdef ST_SecondInstanceSharingTheSameDecoder
    VIDDEC_Data_p->StillDecodeParams.DecodeStillRequested = FALSE; /* Useless ?! */
    VIDDEC_Data_p->StillDecodeParams.GlobalExternalBitBufferAllocated = FALSE;
    VIDDEC_Data_p->StillDecodeParams.StartCodeParsing = VIDDEC_Data_p->StartCodeParsing;
    VIDDEC_Data_p->StillDecodeParams.HeaderData = VIDDEC_Data_p->HeaderData;
    VIDDEC_Data_p->StillDecodeParams.BroadcastProfile = STVID_BROADCAST_DVB; /* no sense, just not to have 0... */
    VIDDEC_Data_p->StillDecodeParams.NbFieldsDisplayNextDecode = 0;
    VIDDEC_Data_p->StillDecodeParams.StillStartCodeProcessing.ExpectingSecondField = FALSE;
    VIDDEC_Data_p->StillDecodeParams.StillStartCodeProcessing.ExtendedTemporalReference = 0;
    VIDDEC_Data_p->StillDecodeParams.StillStartCodeProcessing.TimeCode.Interpolated = TRUE;
#endif /* ST_SecondInstanceSharingTheSameDecoder */


} /* end of InitDecodeStructure() */


/*******************************************************************************
Name        : InitStream
Description : Init stream structure
Parameters  : Pointer to stream, to init, values for user data buffer (address, size)
Assumptions : After that, stream is expected to begin with a sequence start code
Limitations :
Returns     : Nothing
*******************************************************************************/
static void InitStream(MPEG2BitStream_t * const Stream_p,
                       void * const UserDataBuffer_p, const U32 UserDataBufferSize)
{
    /* Initialise default matrix pointers (same for all, never changes) */
    Stream_p->DefaultIntraQuantMatrix_p    = &DefaultIntraQuantMatrix;
    Stream_p->DefaultNonIntraQuantMatrix_p = &DefaultNonIntraQuantMatrix;

    /* Initialise tables of user data to tables allocated by the user */
    Stream_p->UserData.Data_p = UserDataBuffer_p;
    Stream_p->UserData.TotalSize = UserDataBufferSize;

    /* Set values to defaults (same for all at reset, but real-time changing) */
    viddec_ResetStream(Stream_p);

} /* End of InitStream() function */

/*******************************************************************************
Name        : InitUnderflowEvent
Description : Initialise the data associated to the Underflow event.
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void InitUnderflowEvent(const VIDDEC_Handle_t DecodeHandle)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    VIDDEC_Data_p->DataUnderflow.BitBufferFreeSize = 0;
    VIDDEC_Data_p->DataUnderflow.BitRateValue = 0;
    VIDDEC_Data_p->DataUnderflow.RequiredTimeJump = 0;
    VIDDEC_Data_p->DataUnderflow.RequiredDuration =
        (STVID_MPEG_CLOCKS_ONE_SECOND_DURATION / VIDCOM_MIN_FRAME_RATE_MPEG) * VIDCOM_DECODER_SPEED_MPEG;
    VIDDEC_Data_p->DataUnderflow.TransferRelatedToPrevious = TRUE;
    VIDDEC_Data_p->DataUnderflowEventNotSatisfied = FALSE;

} /* end of InitUnderflowEvent() */

/*******************************************************************************
Name        : MonitorAbnormallyMissingHALStatus
Description : Monitor abnormally missing interrupt status of the HAL
              This is the polling mechanism for the HAL to raise interrupts which
              don't occur, of other problems like that.
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : TRUE if there was something raised by the HAL
              FALSE if there was noting raised by the HAL (normal)
*******************************************************************************/
static BOOL MonitorAbnormallyMissingHALStatus(const VIDDEC_Handle_t DecodeHandle)
{
    U32 MissingStatus;

    MissingStatus = HALDEC_GetAbnormallyMissingInterruptStatus(((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle);

    /* Post interrupt command only if there is one valid interrupt */
    if (MissingStatus != 0)
    {
        PushInterruptCommand(DecodeHandle, MissingStatus);

        /* Signal controller that a new command is given */
        semaphore_signal(((VIDDEC_Data_t *) DecodeHandle)->DecodeOrder_p);

        return(TRUE);
    }

    return(FALSE);
} /* End of MonitorAbnormallyMissingHALStatus */


/*******************************************************************************
Name        : MonitorBitBufferLevel
Description : Monitor bit buffer level, and eventually send events if after thresholds
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : TRUE if the bit buffer level has changed since previous Monitor
              FALSE if the bit buffer level is the same as it was when previous Monitor was done
*******************************************************************************/
static BOOL MonitorBitBufferLevel(const VIDDEC_Handle_t DecodeHandle)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
    U32 BitBufferLevel;
    BOOL HasChanged;
#ifdef ST_avsync
    STCLKRV_ExtendedSTC_t   ExtendedSTC;
    STVID_PTS_t             CurrentSTC;
    ST_ErrorCode_t          ErrorCode;
#endif /* ST_avsync */
    U32 OverflowReactionThreshold,OverflowRealTimeReactionThreshold;

    BitBufferLevel = HALDEC_GetDecodeBitBufferLevel(VIDDEC_Data_p->HALDecodeHandle);

    if (BitBufferLevel == VIDDEC_Data_p->HWDecoder.LastBitBufferLevel)
    {
        HasChanged = FALSE;
    }
    else
    {
#if defined TRACE_DECODE || defined TRACE_BBL_TRACKING
       TraceBuffer(("\r\n%d,%d", time_now(), BitBufferLevel));
#endif /* TRACE_DECODE || TRACE_BBL_TRACKING */
        HasChanged = TRUE;
    }

#if defined (STVID_DEBUG_GET_STATISTICS)
    if (BitBufferLevel < VIDDEC_Data_p->Statistics.DecodeMinBitBufferLevelReached)
    {
        VIDDEC_Data_p->Statistics.DecodeMinBitBufferLevelReached = BitBufferLevel;
    }
    if (BitBufferLevel > VIDDEC_Data_p->Statistics.DecodeMaxBitBufferLevelReached)
    {
        VIDDEC_Data_p->Statistics.DecodeMaxBitBufferLevelReached = BitBufferLevel;
    }
#endif /* STVID_DEBUG_GET_STATISTICS */
#if defined TRACE_DECODE_ADDITIONAL_IT || defined STVID_DEBUG_GET_STATISTICS
    if (((VIDDEC_Data_p->VideoInterrupt.Mask & HALDEC_PIPELINE_BIT_BUFFER_EMPTY_MASK) == 0) &&
        (VIDDEC_Data_p->DecodingStream_p->BitStreamProgress != NO_BITSTREAM) &&
        (VIDDEC_Data_p->VideoInterrupt.Mask != 0) &&
        (BitBufferLevel != 0))
    {
        /* BBE interrupt disabled and level is not 0: enable it again for when it will go back to 0. */
        /* If flag was there, disable it. Otherwise no need to clear it, the less we read/write this variable, the best it is... */
        VIDDEC_Data_p->VideoInterrupt.Mask |= HALDEC_PIPELINE_BIT_BUFFER_EMPTY_MASK;
        HALDEC_SetInterruptMask(VIDDEC_Data_p->HALDecodeHandle, VIDDEC_Data_p->VideoInterrupt.Mask);
    }
    if (((VIDDEC_Data_p->VideoInterrupt.Mask & HALDEC_BIT_BUFFER_NEARLY_FULL_MASK) == 0) &&
        (HasChanged) &&
        (VIDDEC_Data_p->VideoInterrupt.Mask != 0) &&
        (BitBufferLevel < (VIDDEC_Data_p->BitBuffer.TotalSize - VIDDEC_BIT_BUFFER_THRESHOLD_IDLE_VALUE - 1)))
    {
        /* BBE interrupt disabled and level is not 0: enable it again for when it will go back to 0. */
        /* If flag was there, disable it. Otherwise no need to clear it, the less we read/write this variable, the best it is... */
        VIDDEC_Data_p->VideoInterrupt.Mask |= HALDEC_BIT_BUFFER_NEARLY_FULL_MASK;
        HALDEC_SetInterruptMask(VIDDEC_Data_p->HALDecodeHandle, VIDDEC_Data_p->VideoInterrupt.Mask);
    }
#endif /* TRACE_DECODE_ADDITIONAL_IT || STVID_DEBUG_GET_STATISTICS */

    /* Remember last bit buffer level for next time... */
    VIDDEC_Data_p->HWDecoder.LastBitBufferLevel = BitBufferLevel;
    /* Update data underflow event parameter with new level */
    VIDDEC_Data_p->DataUnderflow.BitBufferFreeSize = VIDDEC_Data_p->BitBuffer.TotalSize - BitBufferLevel;

    if (!VIDDEC_Data_p->VbvStartCondition.IsDecodeEnable)
    {
        if (VIDDEC_Data_p->VbvStartCondition.StartWithVbvDelay)
        {
            if ((STOS_time_after(time_now(), VIDDEC_Data_p->VbvStartCondition.VbvDelayTimerExpiredDate) != 0))
            {
                VIDDEC_Data_p->VbvStartCondition.IsDecodeEnable = TRUE;
#if defined (STVID_DEBUG_GET_STATISTICS)
                VIDDEC_Data_p->Statistics.DecodeStartConditionVbvDelay++;
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef TRACE_DECODE
               /* TraceBuffer(("\r\n%d,FirstEnableDecode by vbv_delay - BBF=%d", time_now(), BitBufferLevel));*/
#endif /* TRACE_DECODE */
            }
        }
        else
        {
#ifdef ST_avsync
            if (VIDDEC_Data_p->VbvStartCondition.StartWithPTSTimeComparison)
            {
                /* Compare the current STC to the first picture's PTS.  */
                /* Get the current STC time and compare it the the picture's PTS.       */
                ErrorCode = STCLKRV_GetExtendedSTC(VIDDEC_Data_p->VbvStartCondition.STClkrvHandle, &ExtendedSTC);
                /* Convert STCLKRV STC to our PTS format */
                CurrentSTC.PTS      = ExtendedSTC.BaseValue;
                CurrentSTC.PTS33    = (ExtendedSTC.BaseBit32 != 0);
                if (ErrorCode == ST_NO_ERROR)
                {
                    /* STC got with no error. Add a VSYNC duration and test its validity.   */
                    vidcom_AddU32ToPTS(CurrentSTC,
                            ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION * 1000) / STVID_DEFAULT_VALID_VTG_FRAME_RATE));
                    /* Test a too big difference.                                           */
                    if (!IsThereHugeDifferenceBetweenPTSandSTC(CurrentSTC,
                            VIDDEC_Data_p->VbvStartCondition.FirstPicturePTSAfterFirstSequence))
                    {
                        /* No, test if it's time to launch first decode.                    */
                        if (vidcom_IsPTSGreaterThanPTS(CurrentSTC, VIDDEC_Data_p->VbvStartCondition.FirstPicturePTSAfterFirstSequence))
                        {
                            /* OK, enable first decode.                                     */
                            VIDDEC_Data_p->VbvStartCondition.IsDecodeEnable = TRUE;
                            VIDDEC_Data_p->VbvStartCondition.StartWithVbvBufferSize = FALSE;
                            VIDDEC_Data_p->VbvStartCondition.StartWithPTSTimeComparison = FALSE;
                            /* Reset threshold to init level                                */
                            viddec_SetBitBufferThresholdToIdleValue(DecodeHandle);
                            /* Now don't need any more the BBF interrupt: disable it        */
                            viddec_UnMaskBitBufferFullInterrupt(DecodeHandle);
#if defined (STVID_DEBUG_GET_STATISTICS)
                            VIDDEC_Data_p->Statistics.DecodeStartConditionPtsTimeComparison++;
#endif /* STVID_DEBUG_GET_STATISTICS */

#ifdef TRACE_DECODE
                           /* TraceBuffer(("\r\n%d,FirstEnableDecode by PTS (%u) time comparison - BBF=%d"
                                    , VIDDEC_Data_p->StartCodeProcessing.PTS.PTS, time_now(), BitBufferLevel));*/
#endif /* TRACE_DECODE */
                        }
                    }
                } /* Valid STC */
            } /* Start with PTS */
#endif /* ST_avsync */
            if (VIDDEC_Data_p->VbvStartCondition.StartWithVbvBufferSize)
            {
                if (VIDDEC_Data_p->VbvStartCondition.VbvSizeReachedAccordingToInterruptBBT) /* TRUE if the vbv has been reached */
                {
                    /* We start the decode on the vbv_buffer_size. IsDecodeEnable becomes true only when the Bit Buffer Level goes up to vbv_buffer_size */
                    /* Convert VBVSizeToReachBeforeStart in unit of bit ==> x2 x1024 */
                    /* Decode disabled if waiting for vbv_size in bit buffer */
                    /* vbv_buffer_size reached in bit buffer: enable decodes. */
                    VIDDEC_Data_p->VbvStartCondition.IsDecodeEnable = TRUE;
#if defined (STVID_DEBUG_GET_STATISTICS)
                    VIDDEC_Data_p->Statistics.DecodeStartConditionVbvBufferSize++;
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef TRACE_DECODE
                   /* TraceBuffer(("\r\n%d,FirstEnableDecode by vbv_buffer_size - BBF=%d", time_now(), BitBufferLevel));*/
#endif /* TRACE_DECODE */
                }
            }
        }
    }
    /* debug in order to compare vbv_buffer_size start and vbv_delay start */
    /* if (VIDDEC_Data_p->VbvStartCondition.VbvSizeReachedAccordingToInterruptBBT) */
    /* { */
        /* if (!VIDDEC_Data_p->VbvStartCondition.VbvBufferSizeStarted) */
        /* { */
            /* TraceBuffer(("%d,vbv_buffer_size would have started now - BBF=%d\r\n", time_now(), BitBufferLevel)); */
            /* VIDDEC_Data_p->VbvStartCondition.VbvBufferSizeStarted = TRUE; */
        /* } */
    /* } */

    switch (VIDDEC_Data_p->DecodingStream_p->SequenceExtension.profile_and_level_indication & PROFILE_AND_LEVEL_IDENTIFICATION_MASK)
    {
        case (PROFILE_MAIN | LEVEL_HIGH) : /* MPHL */
        case (PROFILE_MAIN | LEVEL_HIGH_1440) : /* MP HL1440 */
            OverflowReactionThreshold  = VIDDEC_Data_p->BitBuffer.TotalSize - VIDDEC_BIT_BUFFER_OVERFLOW_HD_MARGIN;
            OverflowRealTimeReactionThreshold = VIDDEC_Data_p->BitBuffer.TotalSize - VIDDEC_BIT_BUFFER_REAL_TIME_OVERFLOW_HD_MARGIN;
            break;

        case (PROFILE_MAIN | LEVEL_MAIN) : /* MPML */
        default :
            OverflowReactionThreshold  = VIDDEC_Data_p->BitBuffer.TotalSize - VIDDEC_BIT_BUFFER_OVERFLOW_SD_MARGIN;
            OverflowRealTimeReactionThreshold = VIDDEC_Data_p->BitBuffer.TotalSize - VIDDEC_BIT_BUFFER_REAL_TIME_OVERFLOW_SD_MARGIN;
            break;
    }

    if (VIDDEC_Data_p->DecoderState == VIDDEC_DECODER_STATE_DECODING)
    {
        /* Preform action after monitor only if decoder started */
        if(BitBufferLevel <= OverflowRealTimeReactionThreshold)
        {
            VIDDEC_Data_p->NeedToDecreaseBBL = FALSE;
            /* note : even if the flag is false, the decode may be */
            /* continuing skipping until next I ...                */
        }

        if (BitBufferLevel > OverflowReactionThreshold)
        {
            if (!VIDDEC_Data_p->OverFlowEvtAlreadySent)
            {
                /* Data overflow ! */
                VIDDEC_NotifyDecodeEvent(DecodeHandle, STVID_DATA_OVERFLOW_EVT, (const void *) NULL);
            }
            /* Re-start parsing data */
            VIDDEC_Data_p->DataUnderflowEventNotSatisfied = FALSE;
            if((VIDDEC_Data_p->RealTime)
               &&(BitBufferLevel > OverflowRealTimeReactionThreshold))
            {
                /* Prevent bitbuffer overflow during a real-time injection */
                /* = try to decrease bitbuffer level -> skip picture       */
                /* Need to skip if it was a decode, else none              */
                VIDDEC_Data_p->Skip.UntilNextIPicture = TRUE;
                /* force action for the first skip */
                if(!(VIDDEC_Data_p->NeedToDecreaseBBL))
                {
                    VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible = TRUE;
                    VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible = FALSE;
                }
                VIDDEC_Data_p->NeedToDecreaseBBL = TRUE;
            } /* end  if BitBufferLevel > OverflowRealTimeReactionThreshold*/
        } /* end  if BitBufferLevel > OverflowReactionThreshold */
        else
        {
            VIDDEC_Data_p->OverFlowEvtAlreadySent = FALSE;
            if (BitBufferLevel < VIDDEC_Data_p->BitBufferUnderflowMargin)
            {
                /* Data undeflow ! */
                if (!(VIDDEC_Data_p->DataUnderflowEventNotSatisfied))
                {
                    VIDDEC_Data_p->DataUnderflow.BitRateValue = 0;
                    VIDDEC_GetBitRateValue(DecodeHandle, &(VIDDEC_Data_p->DataUnderflow.BitRateValue));
                    VIDDEC_Data_p->DataUnderflow.RequiredTimeJump = 0;
                    VIDDEC_Data_p->DataUnderflow.TransferRelatedToPrevious = TRUE;
                    VIDDEC_Data_p->DataUnderflow.RequiredDuration = VIDDEC_Data_p->BitBufferRequiredDuration;
#ifdef ST_smoothbackward
                    if (VIDDEC_Data_p->ControlMode != VIDDEC_MANUAL_CONTROL_MODE)
#endif /* ST_smoothbackward */
                    {
                        VIDDEC_Data_p->DataUnderflowEventNotSatisfied = TRUE;
                        /* Bit buffer free size is filled in MonitorBitBufferLevel() function */
                        VIDDEC_NotifyDecodeEvent(DecodeHandle, STVID_DATA_UNDERFLOW_EVT, (const void *) &(VIDDEC_Data_p->DataUnderflow));
                    }
                }
            } /* end if BitBufferLevel < UnderflowReactionThreshold */
        }
    }/* endif DecoderState == VIDDEC_DECODER_STATE_DECODING */

    return(HasChanged);
} /* End of MonitorBitBufferLevel() function */


/*******************************************************************************
Name        : NewPictureBufferAvailable
Description : Function to subscribe the VIDBUFF event new picture buffer available
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void NewPictureBufferAvailable(STEVT_CallReason_t Reason,
    const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event,
    const void *EventData_p,
    const void *SubscriberData_p
)
{
    VIDDEC_Handle_t DecodeHandle   = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDDEC_Handle_t, SubscriberData_p);
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
#ifdef TRACE_TAKE_RELEASE
    VIDBUFF_PictureBuffer_t * PictureBufferJustReleased_p;
#endif
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

#ifdef TRACE_TAKE_RELEASE
    /* "PictureBufferJustReleased_p" is provided FOR INFORMATION ONLY so it should not be used because this Picture Buffer is not valid anymore */
    PictureBufferJustReleased_p = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDBUFF_PictureBuffer_t *, EventData_p);
    TraceBuffer(("NewPictBuffAvail 0x%x\r\n", PictureBufferJustReleased_p));
#else
    UNUSED_PARAMETER(EventData_p);
#endif

    /* Vid_tric is now notified directly by VID_BUFF so we don't need to do that here */

    /* !!! To go try to decode next pic as soon as previous decode is over, while
     waiting for a command (otherwise blocked until timeout of command wait) */
    /* Signal controller that a new command is given */
    semaphore_signal(VIDDEC_Data_p->DecodeOrder_p);

} /* End of NewPictureBufferAvailable() function */


/*******************************************************************************
Name        : SeekBeginningOfStream
Description : Seek beginning of stream (sequence start code)
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SeekBeginningOfStream(const VIDDEC_Handle_t DecodeHandle)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

#ifdef TRACE_TAKE_RELEASE
        TraceBuffer(("SeekBeginningOfStream\r\n"));
#endif

    /* Disable interrupts */
    HALDEC_SetInterruptMask(VIDDEC_Data_p->HALDecodeHandle, 0);
    /* Clear all interrupts already queued */
    ClearAllPendingInterrupt(DecodeHandle);
    /* Clear all start codes already queued */
    ClearAllPendingStartCodes(DecodeHandle);
    /* Clear all controller commands already queued */
    ClearAllPendingControllerCommands(DecodeHandle);

    /* Reset stream structure */
    viddec_ResetStream(VIDDEC_Data_p->DecodingStream_p);

    /* Reset task variables that need to be reset after a hardware SoftReset (next start code will be a new sequence). */
    VIDDEC_Data_p->NextAction.LaunchDecodeWhenPossible = FALSE;
    VIDDEC_Data_p->NextAction.LaunchSkipWhenPossible = FALSE;
    VIDDEC_Data_p->ForTask.NoDisplayAfterNextDecode = FALSE;
    VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.NbOfSyntaxError = 0;
    VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.NbOfMisalignmentError = 0;
    VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.NbOfCriticalError = 0;
    VIDDEC_Data_p->ForTask.SyntaxError.PictureDecodeError.DecodeErrorType = VIDDEC_DECODE_ERROR_TYPE_NONE;
    VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.NbOfPictureSCPerFirstSliceSC = 0;
    VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.HasPictureSyntaxErrors = FALSE;
    VIDDEC_Data_p->ForTask.PreviousStartCode = SEQUENCE_ERROR_CODE;
    VIDDEC_Data_p->StartCodeProcessing.PreviousStartCode = SEQUENCE_ERROR_CODE; /* Value here should not be used in actions for previous start code */
    VIDDEC_Data_p->StartCodeProcessing.ExpectingSecondField = FALSE;
    VIDDEC_Data_p->ForTask.SyntaxError.PictureSyntaxError.PreviousPictureHasAMissingField = FALSE;
    VIDDEC_Data_p->LaunchStartCodeSearchToResume = FALSE;
    VIDDEC_Data_p->ComputeSequenceInfoOnNextPictureOrGroup = FALSE;
    VIDDEC_Data_p->PreviousDecodedPictureMPEGFrame = STVID_MPEG_FRAME_I;

    /* Lock Access to Reference Pictures */
    semaphore_wait(VIDDEC_Data_p->ReferencePicture.ReferencePictureSemaphore_p);

    /* LastButOne_p Last_p have already been initialised. Then, they can not be reseted to NULL without being UNLOCKED, */
    /* it their status are RESERVED. Otherwise, one of the frame buffers may be stay definitively set to RESERVED status  */
    /* and then unusable. */
    VIDDEC_ReleaseLastButOneReference(DecodeHandle);

    if (VIDDEC_Data_p->ReferencePicture.Last_p != NULL)
    {
        if (VIDDEC_Data_p->ReferencePicture.KeepLastReference)
        {
            /* STVID_Clear was called, so keep the last reference */
#ifdef TRACE_TAKE_RELEASE
            TraceBuffer(("Keep the last reference\r\n"));
#endif
            VIDDEC_Data_p->ReferencePicture.KeepLastReference = FALSE;
        }
        else
        {
            VIDDEC_ReleaseLastReference(DecodeHandle);
        }
    }
    VIDDEC_Data_p->ReferencePicture.MissingPrevious = TRUE;

    /* Lock Access to Reference Pictures */
    semaphore_signal(VIDDEC_Data_p->ReferencePicture.ReferencePictureSemaphore_p);

    VIDDEC_Data_p->StartCodeParsing.HasUserDataProcessTriggeredStartCode = FALSE;

    /* To be verified */
/*    VIDDEC_Data_p->HWDecoder.LastBitBufferLevel = 0;*/
#ifdef ST_smoothbackward
    VIDDEC_Data_p->RequiredSearch.StartCodeDetectorToLaunch = FALSE;
    VIDDEC_Data_p->RequiredSearch.SCDetectorHasBeenLaunchedManually = FALSE;
#endif /* ST_smoothbackward */

    /* soft reset sets to 0 SCDCount register : thus current BitRateValue Evaluating is spoiled */
    VIDDEC_Data_p->BitRateValueEvaluate.BitRateValueIsUnderComputing = FALSE;

    /* After Soft reset: as this flushes the bit buffer, we need to wait again for vbv_delay level to be reached in bit buffer
       exactly as after Start(). Otherwise, if there are some errors, the decode starts again with a bit buffer
       at a bad level, and underflow or overflow quickly happens causing more errors on some streams or synchronisation
       difficulties. In ATSC it is very very bad. (This is the same problem as when video didn't take vbv_delay level into account.) */
    /* By default the decode is enabled at start */
    if (VIDDEC_Data_p->RealTime)
    {
        VIDDEC_Data_p->VbvStartCondition.IsDecodeEnable = FALSE;
    }
    else
    {
        VIDDEC_Data_p->VbvStartCondition.IsDecodeEnable = TRUE;
    }

    /* Start Condition flags init */
    VIDDEC_Data_p->VbvStartCondition.FirstSequenceSinceLastSoftReset = TRUE;
    VIDDEC_Data_p->VbvStartCondition.FirstPictureAfterFirstSequence = FALSE;
    VIDDEC_Data_p->VbvStartCondition.VbvSizeReachedAccordingToInterruptBBT = FALSE;
    VIDDEC_Data_p->VbvStartCondition.VbvSizeToReachBeforeStartInBytes = 0;
    VIDDEC_Data_p->VbvStartCondition.StartWithVbvBufferSize = FALSE;
    VIDDEC_Data_p->VbvStartCondition.StartWithVbvDelay = FALSE;
#ifdef ST_avsync
    VIDDEC_Data_p->VbvStartCondition.StartWithPTSTimeComparison = FALSE;
    VIDDEC_Data_p->VbvStartCondition.FirstPicturePTSAfterFirstSequence.PTS   = 0;
    VIDDEC_Data_p->VbvStartCondition.FirstPicturePTSAfterFirstSequence.PTS33 = FALSE;
#endif /* ST_avsync */
    /* VIDDEC_Data_p->VbvStartCondition.VbvBufferSizeStarted = FALSE; */

    /* While no stream, reset threshold to init level */
    viddec_SetBitBufferThresholdToIdleValue(DecodeHandle);

    /* Reseting decoder also launches start code detection ! */
    VIDDEC_Data_p->HWStartCodeDetector.IsIdle = FALSE;
    VIDDEC_Data_p->HWStartCodeDetector.MaxStartCodeSearchTime = time_plus(time_now(), MAX_START_CODE_SEARCH_TIME);

    /* About to SoftReset the hardware: any decode on-going will be aborted ! */
    VIDDEC_Data_p->HWDecoder.HWState = VIDDEC_HARDWARE_STATE_IDLE;
    if (VIDDEC_Data_p->HWDecoder.DecodingPicture_p != NULL)
    {
        VIDDEC_Data_p->HWDecoder.DecodingPicture_p->Decode.DecodingError = VIDCOM_ERROR_LEVEL_RESET;
        VIDDEC_Data_p->HWDecoder.DecodingPicture_p = NULL;
    }
    VIDDEC_Data_p->UnsuccessfullDecodeCount = 0;

    /* Prepare interrupt mask */
    VIDDEC_Data_p->VideoInterrupt.Mask =
#ifdef ST_smoothbackward
                                     HALDEC_VLD_READY_MASK |
#endif /* ST_smoothbackward */
                                     HALDEC_PIPELINE_START_DECODING_MASK |
                                     HALDEC_START_CODE_HIT_MASK |
                                     HALDEC_DECODER_IDLE_MASK |
                                     HALDEC_PIPELINE_IDLE_MASK |
                                     HALDEC_DECODING_SYNTAX_ERROR_MASK |
                                     HALDEC_DECODING_UNDERFLOW_MASK |
                                     HALDEC_DECODING_OVERFLOW_MASK |
                                     HALDEC_MISALIGNMENT_PIPELINE_AHEAD_MASK |
                                     HALDEC_MISALIGNMENT_START_CODE_DETECTOR_AHEAD_MASK;
    /* Allow BBF once at start for vbv_size reach detection */
    VIDDEC_Data_p->VideoInterrupt.Mask |= HALDEC_BIT_BUFFER_NEARLY_FULL_MASK;

    /**************************************************************************/
    /* CAUTION: interrupts below are NOT enabled by default, but only when some*/
    /* particular compilation flag are active. So careful: they should not be */
    /* intrusive not to change the driver's behaviour. Also, they should be   */
    /* modified together at all places: here, in IT handler, and anywhere in  */
    /* VIDDEC where the corresponding masks are modified in VideoInterrupt.Mask */
    /**************************************************************************/
#if defined (STVID_DEBUG_GET_STATISTICS)
    VIDDEC_Data_p->VideoInterrupt.Mask |=
                                     HALDEC_PIPELINE_BIT_BUFFER_EMPTY_MASK |
                                     HALDEC_BIT_BUFFER_NEARLY_FULL_MASK;
#endif /* STVID_DEBUG_GET_STATISTICS */
#if defined TRACE_DECODE_ADDITIONAL_IT
    VIDDEC_Data_p->VideoInterrupt.Mask |=
/*                                     HALDEC_START_CODE_DETECTOR_BIT_BUFFER_EMPTY_MASK |*/ /* occurs too often on STi7015. */
/*                                     HALDEC_BITSTREAM_FIFO_FULL_MASK |*/ /* not used */
/*                                     HALDEC_INCONSISTENCY_ERROR_MASK |*/ /* not used */
                                     HALDEC_PIPELINE_BIT_BUFFER_EMPTY_MASK |
                                     HALDEC_BIT_BUFFER_NEARLY_FULL_MASK |
                                     HALDEC_OVERTIME_ERROR_MASK;
#endif /* TRACE_DECODE_ADDITIONAL_IT */
    /**************************************************************************/

    /* Dummy read of interrupt status to clear it: this will clear all interrupts accumulated
    in interrupt status since last VIDDEC_Stop() if any (and particularly the BBF that problably
    occured if the time between stop and start was long enough and leaded to a fill of bit buffer !) */
    /*InterruptStatus =*/ HALDEC_GetInterruptStatus(VIDDEC_Data_p->HALDecodeHandle);

    /* Reset decoder, flush bit buffer ,and look for next sequence */
    HALDEC_DecodingSoftReset(VIDDEC_Data_p->HALDecodeHandle, VIDDEC_Data_p->RealTime);
    /* Set interrupt mask the soonest after soft reset, to enable interrupts.
    There's the risk not to have the first interrupts (like first SCH) that could occur between
    real hardware SoftReset and real hardware enable of interrupts, but they will still be
    accumulated in interrupt status, so not lost, just they won't have raised an IT and will be processed with next IT.
    Note: To be sure no interrupt is accumulated from before the SoftReset, the HAL
    must also clear the interrupt status just after the SoftReset, but before any
    action that can cause interrupts to occur (launch SC detector for example) */
    HALDEC_SetInterruptMask(VIDDEC_Data_p->HALDecodeHandle, VIDDEC_Data_p->VideoInterrupt.Mask);
#ifdef STVID_DEBUG_GET_STATISTICS
    VIDDEC_Data_p->Statistics.DecodeHardwareSoftReset ++;
#endif /* STVID_DEBUG_GET_STATISTICS */

    /* Notification of this soft reset to advise outside world. */
    STEVT_Notify(VIDDEC_Data_p->EventsHandle, VIDDEC_Data_p->RegisteredEventsID[VIDDEC_RESTARTED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);

    /* !!! */
/*    viddec_EnableVideoInterrupt(DecodeHandle);*/

/*    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Searching sequence..."));*/

} /* End of SeekBeginningOfStream() function */

/*******************************************************************************
Name        : SelfInstalledVideoInterruptHandler
Description : When installing interrupt handler ourselves, interrupt function
Parameters  : Pointer to video decode handle
Assumptions :
Limitations :
Comment     : STOS_INTERRUPT_DECLARE is used in order to be free from
            : OS dependent interrupt function prototype
Returns     : Nothing on OS20
              OS21_SUCCESS or OS21_FAILURE on OS21 (type int)
*******************************************************************************/
#if defined STVID_STVAPI_ARCHITECTURE || !defined ST_OSLINUX
static STOS_INTERRUPT_DECLARE(SelfInstalledVideoInterruptHandler, Param)
{
    VIDDEC_Data_t * const VIDDEC_Data_p = (VIDDEC_Data_t *) Param;
    ST_ErrorCode_t ErrorCode;

#if defined (ST_OS21) && defined (STVID_STVAPI_ARCHITECTURE) && defined (INTERRUPT_INSTALL_WORKAROUND)
    int   pollStatus;

    /* Check that the device reall has a pending interrupt */
    pollStatus = 0;
    interrupt_poll(interrupt_handle(VIDDEC_Data_p->VideoInterrupt.Number), &pollStatus);
    if (pollStatus == 0)
    {
        STOS_INTERRUPT_EXIT(STOS_FAILURE);
    }
#endif

    ErrorCode = STEVT_Notify(VIDDEC_Data_p->EventsHandle, VIDDEC_Data_p->VideoInterrupt.EventID, STEVT_EVENT_DATA_TYPE_CAST NULL);

    STOS_INTERRUPT_EXIT(STOS_SUCCESS);
} /* End of SelfInstalledVideoInterruptHandler() function */
#endif /* STVID_STVAPI_ARCHITECTURE | !LINUX */

/*******************************************************************************
Name        : SkipPicture
Description : Launch skip of a picture
              To behave like a decode in automatic mode, also launch start code detector
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SkipPicture(const VIDDEC_Handle_t DecodeHandle)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    /* Also launch start code detector when skipping ! */
#ifdef ST_smoothbackward
    if (VIDDEC_Data_p->ControlModeInsideHAL != VIDDEC_AUTOMATIC_CONTROL_MODE)
    {
#ifdef DECODE_DEBUG
        DecodeError();
#endif /* DECODE_DEBUG */
        return;
    }
#endif /* ST_smoothbackward */

    /* Launching skip also launches SC detector in automatic mode ! */
    VIDDEC_Data_p->HWStartCodeDetector.IsIdle = FALSE;
    VIDDEC_Data_p->HWStartCodeDetector.MaxStartCodeSearchTime = time_plus(time_now(), MAX_START_CODE_SEARCH_TIME);
    HALDEC_SearchNextStartCode(VIDDEC_Data_p->HALDecodeHandle, HALDEC_START_CODE_SEARCH_MODE_NORMAL, TRUE, NULL);

    /* Monitor skip time */
    VIDDEC_Data_p->HWDecoder.MaxDecodeOrSkipTime = time_plus(time_now(), MAX_SKIP_TIME);

    /* Attest that a skip is on-going */
    VIDDEC_Data_p->HWDecoder.HWState = VIDDEC_HARDWARE_STATE_SKIPPING;

    /* Launch skip */
    HALDEC_SetSkipConfiguration(VIDDEC_Data_p->HALDecodeHandle, &(VIDDEC_Data_p->ForTask.PictureInfos.VideoParams.PictureStructure));
    HALDEC_SkipPicture(VIDDEC_Data_p->HALDecodeHandle, FALSE, HALDEC_SKIP_PICTURE_1_THEN_STOP);

    /* Notification of beginning of decode advise outside world.   */
    STEVT_Notify(VIDDEC_Data_p->EventsHandle, VIDDEC_Data_p->RegisteredEventsID[VIDDEC_SKIP_LAUNCHED_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);

} /* End of SkipPicture() function */


/*******************************************************************************
Name        : StartDecodeTask
Description : Start the task of decode control
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_ALREADY_INITIALIZED if task already running
              ST_ERROR_BAD_PARAMETER if problem of creation
*******************************************************************************/
static ST_ErrorCode_t StartDecodeTask(const VIDDEC_Handle_t DecodeHandle)
{
    VIDCOM_Task_t * const Task_p = &(((VIDDEC_Data_t *) DecodeHandle)->DecodeTask);
    char TaskName[20] = "STVID[?].DecodeTask";
    ST_ErrorCode_t ErrorCode ;
    if (Task_p->IsRunning)
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* In TaskName string, replace '?' with decoder number */
    TaskName[6] = (char) ((U8) '0' + ((VIDDEC_Data_t *) DecodeHandle)->DecoderNumber);
    Task_p->ToBeDeleted = FALSE;

#ifdef STVID_MEASURES
    ErrorCode = STOS_TaskCreate((void (*) (void*)) DecodeTaskFunc, (void *) DecodeHandle,
                                        ((HALDEC_Properties_t *)((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle)->CPUPartition_p,
                                        STVID_TASK_STACK_SIZE_DECODE,
                                        &(Task_p->TaskStack),
                                        ((HALDEC_Properties_t *)((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle)->CPUPartition_p,
                                        &(Task_p->Task_p),
                                        &(Task_p->TaskDesc),
                                        STVID_TASK_PRIORITY_DECODE,
                                        TaskName, task_flags_high_priority_process | task_flags_no_min_stack_size);
#else /* STVID_MEASURES */
    ErrorCode = STOS_TaskCreate((void (*) (void*)) DecodeTaskFunc, (void *) DecodeHandle,
                                        ((HALDEC_Properties_t *)((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle)->CPUPartition_p,
                                        STVID_TASK_STACK_SIZE_DECODE,
                                        &(Task_p->TaskStack),
                                        ((HALDEC_Properties_t *)((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle)->CPUPartition_p,
                                        &(Task_p->Task_p),
                                        &(Task_p->TaskDesc),
                                        STVID_TASK_PRIORITY_DECODE,
                                        TaskName, task_flags_no_min_stack_size);
#endif /* not STVID_MEASURES */
    if (ErrorCode != ST_NO_ERROR)
    {
        return(ErrorCode);
    }

    Task_p->IsRunning  = TRUE;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Decode task created."));
    return(ST_NO_ERROR);
} /* End of StartDecodeTask() function */

/*******************************************************************************
Name        : StopDecodeTask
Description : Stop the task of decode control
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if no problem,
              ST_ERROR_ALREADY_INITIALIZED if task not running
*******************************************************************************/
static ST_ErrorCode_t StopDecodeTask(const VIDDEC_Handle_t DecodeHandle)
{
    VIDCOM_Task_t * const Task_p = &(((VIDDEC_Data_t *) DecodeHandle)->DecodeTask);
    task_t * TaskList_p = Task_p->Task_p;
    if (!(Task_p->IsRunning))
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* End the function of the task here... */
    Task_p->ToBeDeleted = TRUE;
    /* Signal semaphore to release task that may wait infinitely if stopped */
    semaphore_signal(((VIDDEC_Data_t *) DecodeHandle)->DecodeOrder_p);

    STOS_TaskWait(&TaskList_p, TIMEOUT_INFINITY);
    STOS_TaskDelete ( Task_p->Task_p,
                                  ((HALDEC_Properties_t *)((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle)->CPUPartition_p,
                                  Task_p->TaskStack,
                                  ((HALDEC_Properties_t *)((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle)->CPUPartition_p);
    Task_p->IsRunning  = FALSE;

    return(ST_NO_ERROR);
} /* End of StopDecodeTask() function */


/*******************************************************************************
Name        : TermDecodeStructure
Description : Terminate decode structure
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void TermDecodeStructure(const VIDDEC_Handle_t DecodeHandle)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    STOS_SemaphoreDelete(VIDDEC_Data_p->CPUPartition_p, VIDDEC_Data_p->DecodeOrder_p);
    STOS_SemaphoreDelete(VIDDEC_Data_p->CPUPartition_p, VIDDEC_Data_p->ReferencePicture.ReferencePictureSemaphore_p);

} /* end of TermDecodeStructure() */


#ifdef DECODE_DEBUG
/*******************************************************************************
Name        : DecodeError
Description : Function to help in debug : put a breakpoint here and you will know by the callers where it has been called
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void DecodeError(void)
{
    U8 Error = 0;
    Error ++;
#ifdef TRACE_DECODE
    TraceBuffer(("Error -"));
#endif /* TRACE_DECODE */

}
#endif /* DECODE_DEBUG */


/*******************************************************************************
Name        : viddec_FillPictureBuffer
Description : Function to fill new VIDBUFF_PictureBuffer_t structure with old VIDDEC_Data_t structure content
Parameters  :
Assumptions : both passed pointers are not NULL
Limitations :
Returns     :
*******************************************************************************/
static void viddec_FillPictureBuffer(VIDBUFF_PictureBuffer_t *PB_p, VIDDEC_Data_t *viddec_p)
{
    U32     DispHorSize;
    U32     DispVerSize;
    U8      panandscan;
#if defined(ST_multicodec)
    U32     counter;
#endif /* ST_multicodec */

    #ifdef IRDETOCA
    extern U32 gu_PictureDecodeCount;
    #endif

    #ifdef IRDETOCA
    gu_PictureDecodeCount ++;	
    #endif

    PB_p->ParsedPictureInformation.PictureDecodingContext_p = &(PB_p->PictureDecodingContext);
    PB_p->ParsedPictureInformation.SizeOfPictureSpecificDataInByte = 0; /* see next what to put inside specfic to MPEG parser/decoder */
    PB_p->ParsedPictureInformation.PictureSpecificData_p = PB_p->PictureSpecificData;
    PB_p->PictureDecodingContext.GlobalDecodingContext_p = &(PB_p->GlobalDecodingContext);

    PB_p->ParsedPictureInformation.PictureGenericData.PictureInfos = viddec_p->ForTask.PictureInfos;
    PB_p->ParsedPictureInformation.PictureGenericData.IsFirstOfTwoFields = viddec_p->ForTask.PictureStreamInfo.ExpectingSecondField;
    if (viddec_p->ForTask.PictureStreamInfo.RepeatFieldCounter != 0)
    {
        PB_p->ParsedPictureInformation.PictureGenericData.RepeatFirstField = TRUE;
    }
    else
    {
        PB_p->ParsedPictureInformation.PictureGenericData.RepeatFirstField = FALSE;
    }

    PB_p->ParsedPictureInformation.PictureGenericData.RepeatProgressiveCounter = (U8) viddec_p->ForTask.PictureStreamInfo.RepeatProgressiveCounter;

    if (viddec_p->ForTask.PictureStreamInfo.HasSequenceDisplay)
    {
        DispHorSize = viddec_p->ForTask.PictureStreamInfo.SequenceDisplayExtension.display_horizontal_size;
        DispVerSize = viddec_p->ForTask.PictureStreamInfo.SequenceDisplayExtension.display_vertical_size;
    }
    else
    {
        DispHorSize = 0;
        DispVerSize = 0;
    }
    for (panandscan = 0; panandscan < VIDCOM_MAX_NUMBER_OF_PAN_AND_SCAN; panandscan++)
    {
        PB_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[panandscan].FrameCentreHorizontalOffset = 0;
        PB_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[panandscan].FrameCentreVerticalOffset = 0;
        PB_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[panandscan].DisplayHorizontalSize = DispHorSize * 16;
        PB_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[panandscan].DisplayVerticalSize = DispVerSize * 16;
        PB_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[panandscan].HasDisplaySizeRecommendation =
                    viddec_p->ForTask.PictureStreamInfo.HasSequenceDisplay;
    }
    for (panandscan = 0; panandscan < viddec_p->ForTask.PictureStreamInfo.PictureDisplayExtension.number_of_frame_centre_offsets; panandscan++)
    {
        PB_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[panandscan].FrameCentreHorizontalOffset =
            viddec_p->ForTask.PictureStreamInfo.PictureDisplayExtension.FrameCentreOffsets[panandscan].frame_centre_horizontal_offset;
        PB_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[panandscan].FrameCentreVerticalOffset =
            viddec_p->ForTask.PictureStreamInfo.PictureDisplayExtension.FrameCentreOffsets[panandscan].frame_centre_vertical_offset;
    }
    PB_p->ParsedPictureInformation.PictureGenericData.NumberOfPanAndScan = viddec_p->ForTask.PictureStreamInfo.PictureDisplayExtension.number_of_frame_centre_offsets;

    PB_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID = viddec_p->ForTask.PictureStreamInfo.ExtendedTemporalReference;
    PICTUREID_FROM_EXTTEMPREF(PB_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID,
                              viddec_p->ForTask.PictureStreamInfo.ExtendedTemporalReference,
                              viddec_p->ForTask.TemporalReferenceOffset);

    /*--------------------------------------------------------------------------*/
    /*    not used in old viddec (taken from old decode viddec_p structure)     */
    /*--------------------------------------------------------------------------*/
    PB_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PTS.IsValid = FALSE;
    if (viddec_p->ForTask.PictureInfos.VideoParams.MPEGFrame != STVID_MPEG_FRAME_B)
    {
        PB_p->ParsedPictureInformation.PictureGenericData.IsReference = TRUE;
    }
    else
    {
        PB_p->ParsedPictureInformation.PictureGenericData.IsReference = FALSE;
    }
    PB_p->ParsedPictureInformation.PictureGenericData.IsDisplayBoundPictureIDValid = FALSE;
    PB_p->ParsedPictureInformation.PictureGenericData.DisplayBoundPictureID.ID = 0;
    PB_p->ParsedPictureInformation.PictureGenericData.DisplayBoundPictureID.IDExtension = 0;

#if defined(ST_multicodec)
    for (counter = 0; counter < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; counter++)
    {
        PB_p->ParsedPictureInformation.PictureGenericData.FullReferenceFrameList[counter] = 0;
        PB_p->ParsedPictureInformation.PictureGenericData.IsValidIndexInReferenceFrame[counter] = FALSE;
    }
    for (counter = 0; counter < VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS; counter++)
    {
        PB_p->ParsedPictureInformation.PictureGenericData.ReferenceListP0[counter] = 0;
        PB_p->ParsedPictureInformation.PictureGenericData.ReferenceListB0[counter] = 0;
        PB_p->ParsedPictureInformation.PictureGenericData.ReferenceListB1[counter] = 0;
    }
    PB_p->ParsedPictureInformation.PictureGenericData.UsedSizeInReferenceListP0 = 0;
    PB_p->ParsedPictureInformation.PictureGenericData.UsedSizeInReferenceListB0 = 0;
    PB_p->ParsedPictureInformation.PictureGenericData.UsedSizeInReferenceListB1 = 0;
    PB_p->ParsedPictureInformation.PictureGenericData.BitBufferPictureStartAddress = NULL;
    PB_p->ParsedPictureInformation.PictureGenericData.BitBufferPictureStopAddress = NULL;
    PB_p->ParsedPictureInformation.PictureGenericData.AsynchronousCommands.FlushPresentationQueue = FALSE;
    PB_p->ParsedPictureInformation.PictureGenericData.AsynchronousCommands.FreezePresentationOnThisFrame = FALSE;
    PB_p->ParsedPictureInformation.PictureGenericData.AsynchronousCommands.ResumePresentationOnThisFrame = FALSE;
#endif /* ST_multicodec */

/*    PB_p->ParsedPictureInformation.PictureGenericData.DecodeStartTime;*/
    PB_p->ParsedPictureInformation.PictureGenericData.IsDecodeStartTimeValid = FALSE;
/*    PB_p->ParsedPictureInformation.PictureGenericData.PresentationStartTime;*/
    PB_p->ParsedPictureInformation.PictureGenericData.IsPresentationStartTimeValid = FALSE;

    PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.SequenceInfo = viddec_p->StartCodeProcessing.SequenceInfo;
    /*--------------------------------------------------------------------------*/
    /* End of not used in old viddec (taken from old decode viddec_p structure) */
    /*--------------------------------------------------------------------------*/

    PB_p->ParsedPictureInformation.PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_NONE;

    PB_p->GlobalDecodingContext.SizeOfGlobalDecodingContextSpecificDataInByte = 0;
    PB_p->GlobalDecodingContext.GlobalDecodingContextSpecificData_p = PB_p->GlobalDecodingContextSpecificData;

    if ((viddec_p->ForTask.PictureStreamInfo.HasSequenceDisplay) && (viddec_p->ForTask.PictureStreamInfo.SequenceDisplayExtension.colour_description))
    {
        PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.ColourPrimaries =
                viddec_p->ForTask.PictureStreamInfo.SequenceDisplayExtension.colour_primaries;
        PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.TransferCharacteristics =
                viddec_p->ForTask.PictureStreamInfo.SequenceDisplayExtension.transfer_characteristics;
        PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.MatrixCoefficients =
                viddec_p->ForTask.PictureStreamInfo.SequenceDisplayExtension.matrix_coefficients;
    }
    else
    {
        /* MPEG default values set according to is138182 normative document */
        PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.ColourPrimaries = COLOUR_PRIMARIES_ITU_R_BT_709;
        PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.TransferCharacteristics = TRANSFER_ITU_R_BT_709;
        PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.MatrixCoefficients = MATRIX_COEFFICIENTS_ITU_R_BT_709;
    }

    PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.FrameCropInPixel.LeftOffset = 0;
    PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.FrameCropInPixel.RightOffset = 0;
    PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.FrameCropInPixel.TopOffset = 0;

    /* Test that display height is no bigger than 1080 lines. If it is, crop it from bottom */
    if (PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.SequenceInfo.Height > 1080)
    {
        PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.FrameCropInPixel.BottomOffset =
                PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.SequenceInfo.Height - 1080;
    }
    else
    {
        PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.FrameCropInPixel.BottomOffset = 0;
    }

    PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.NumberOfReferenceFrames = 2;
    PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.MaxDecFrameBuffering = 2;

    PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.ParsingError = VIDCOM_ERROR_LEVEL_NONE;

} /* end of viddec_FillPictureBuffer() */

#ifdef ST_smoothbackward
/*******************************************************************************
Name        : VIDDEC_WriteStartCode
Description : Writes a startcode in the bitbuffer at the given address
Parameters  : Video decode handle, startcode value, startcode address
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t VIDDEC_WriteStartCode(const VIDDEC_Handle_t DecodeHandle, const U32 SCVal, void * const SCAdd_p)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    HALDEC_WriteStartCode(VIDDEC_Data_p->HALDecodeHandle, SCVal, SCAdd_p);
    return(ST_NO_ERROR);
} /* end of VIDDEC_WriteStartCode() */

/*******************************************************************************
Name        : VIDDEC_GetCircularBitBufferParams
Description : Gives the circular bitbuffer address and size
Parameters  : Video decode handle, a pointer on a circular bitbuffer params
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t VIDDEC_GetCircularBitBufferParams(const VIDDEC_Handle_t DecodeHandle, VIDDEC_CircularBitbufferParams_t * const CircularBitbufferParams_p)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    CircularBitbufferParams_p->Address = VIDDEC_Data_p->BitBuffer.Address_p;
    CircularBitbufferParams_p->Size    = VIDDEC_Data_p->BitBuffer.TotalSize;

    return(ST_NO_ERROR);
} /* end of VIDDEC_GetCircularBitBufferParams() */

#endif

/*******************************************************************************
Name        : VIDDEC_SetBitBufferUnderflowMargin
Description : Sets the bitbuffer level that when reached, the underflow event
              must be notified
Parameters  : Video decode handle, the bitbuffer level
Assumptions :
Limitations :
Returns     : ST_NO_ERROR when the level is more little than the bitbuffer size
              STVID_ERROR_NOT_AVAILABLE otherwise
*******************************************************************************/
ST_ErrorCode_t VIDDEC_SetBitBufferUnderflowMargin(const VIDDEC_Handle_t DecodeHandle,
                                                  const U32 Level)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    if(Level <= VIDDEC_Data_p->BitBuffer.TotalSize)
    {
        VIDDEC_Data_p->BitBufferUnderflowMargin = Level;
        return(ST_NO_ERROR);
    }
    else
    {
        return(STVID_ERROR_NOT_AVAILABLE);
    }
} /* end of VIDDEC_SetBitBufferUnderflowMargin() */

/*******************************************************************************
Name        : VIDDEC_SetBitBufferUnderflowRequiredDuration
Description : Sets the injection duration required by the driver when the next
              underflow event will be notified
Parameters  : Video decode handle, the bitbuffer level
Assumptions :
Limitations :
Returns     : ST_NO_ERROR when the level is more little than the bitbuffer size
              STVID_ERROR_NOT_AVAILABLE otherwise
*******************************************************************************/
ST_ErrorCode_t VIDDEC_SetBitBufferUnderflowRequiredDuration(const VIDDEC_Handle_t DecodeHandle,
                                                            const U32 Duration)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    VIDDEC_Data_p->BitBufferRequiredDuration = Duration;
    return(ST_NO_ERROR);
} /* end of VIDDEC_SetBitBufferUnderflowRequiredDuration() */

/*******************************************************************************
Name        :  VIDDEC_GetBitBufferParams
Description : Get bit buffer info
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t  VIDDEC_GetBitBufferParams(const VIDDEC_Handle_t DecodeHandle,  void ** const Address_p,  U32 * const Size_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;


   if ( (Address_p == NULL) || (Size_p == NULL) )
    {
        Error = ST_ERROR_BAD_PARAMETER;
    }

   *Address_p = VIDDEC_Data_p->BitBuffer.Address_p;
   *Size_p    = VIDDEC_Data_p->BitBuffer.TotalSize;

   return(Error);
} /* End of  VIDDEC_GetBitBufferParams() */

/* End of vid_dec.c */
