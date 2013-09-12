/*******************************************************************************

File name   : vid_tric.c

Description : Video Trick modes source file

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
14 Sept 2000       Created                                          PLe
29 Jan 2002        VITRICK now notifies DISPLAY_PARAMS_CHANGE_EVT
                   instead of calling VIDDISP_SetDisplayParams()    HG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#endif

#include "stddefs.h"
#include "stos.h"

#include "stsys.h"
#include "sttbx.h"
#include "vid_ctcm.h"
#include "trickmod.h"
#include "decode.h"
#include "display.h"
#ifdef ST_smoothbackward
#include "buffers.h"
#endif /* ST_smoothbackward */

#include "vid_tric.h"

#include "stevt.h"
#include "stvtg.h"
#include "stavmem.h"
#include "stos.h"

/* #define OLD_MAX_JUMP_FIELDS*/
/* #define OLD_OVERLAP_COMPUTATION*/
/* #define OLD_BACKWARD_FLUENT_DISPLAY_PROCESS */

/*#define TRACE_UART_MAIN */                      /* Activate this to get the main Debug information */
/*#define TRACE_UART_DEBUG    */                  /* Activate this to get more Debug information */
/* #define TRACE_UART_SPEEDSWITCH */
/* #define TRACE_UART_EXT_TEMP_REF */
/* #define TRACE_UART_TOPBOT_ALTERNATING */
/* #define TRACE_UART_DISPLAY_SKIP */
/*#define TRICKMOD_DEBUG */                       /* Activate this to see only error messages */
/*#define DEBUG_DISPLAY_QUEUE  */                   /* Activate this to Debug LastPictureInDisplayQueue_p[] content */

#if defined(TRACE_UART_MAIN) || defined(TRACE_UART_DEBUG) || defined(TRICKMOD_DEBUG) || defined(TRACE_UART_SPEEDSWITCH) || defined(TRACE_UART_EXT_TEMP_REF) || defined(TRACE_UART_TOPBOT_ALTERNATING) || defined(TRACE_UART_DISPLAY_SKIP)
    #define TRACE_UART
    #include "trace.h"
#else
    #define TraceBuffer(x)
#endif /* TRACE_UART_MAIN or TRICKMOD_DEBUG or ... */

#define TraceSpeedAccuracy(x) TraceBuffer(x)

/* Public variables --------------------------------------------------------- */

/* static U32 State = 0;  ...inside function TrickModeTraces() at the end of this file. This is a global ! (it is in TRACE_UART_DISPLAY_SKIP #ifdef only ! */

/* Private Constants -------------------------------------------------------- */

/* default bitrate if none has been computed */
#define VIDTRICK_DEFAULT_BITRATE                          37500
#define VIDTRICK_MAX_BITRATE                              37500

/* speed under which counter of fields to skip is reseted */
#define VIDTRICK_SPEED_MIN_SPEED_FOR_NO_NULL_COUNTER      200
/* max speed to skip 0,1,2 or 3 fields */
#define VIDTRICK_MAX_SPEED_FOR_SMALL_SKIP                 400

/* Decode Skipping pictures */
#define  VIDTRICK_LEVEL_SKIP_CURRENT_PICTURE              4
#define  VIDTRICK_LEVEL_SKIP_ALL_B_AND_DISP_P             6
#define  VIDTRICK_LEVEL_SKIP_ALL_P                        14
#define  VIDTRICK_LEVEL_SKIP_ON_HDD                       80
#define  VIDTRICK_LEVEL_TO_REACH_AFTER_SKIP_ON_HDD        40
#define  VIDTRICK_BETWEEN_2_I_MIN_PIC_NB                  2

/* Max time between two consecutive I pictures */
#define VIDTRICK_MAX_TIME_BETWEEN_TWO_CONSECUTIVE_I_PICTURES   (STVID_MPEG_CLOCKS_ONE_SECOND_DURATION / 2)

/* HG: trying to return to mode CARE of display with an hysteresis, not to toggle
premanentely between care and as_available. This first try works until a certain
extent: changes occurs each 10-20 pics instead of every 2... */
#define  VIDTRICK_LEVEL_BACK_TO_DISPLAY_CARE_FRAME        2

/* Decode Skipping pictures for backward */
/* Offset to substract to the new jump computed value to compute fields to jump min value*/
#define  VIDTRICK_FIELDS_JUMP_NB_DECREASE                 0

/* smooth backward : if fast backward speed required with if many pictures inside a same bit buffer */
/* several pictures of the bit buffer should be displayed in order to give a good backward fluidity impression */
/* these 4 values are used to compute the maximum homes allowed between 2 pictures of the same bit buffer */
/* which have to be displayed */
#define  VIDTRICK_MAX_JUMPEABLE_UNDER_200                 2
#define  VIDTRICK_MAX_JUMPEABLE_FIELDS_MIN                6
#define  VIDTRICK_MAX_JUMPEABLE_FIELDS_MAX                10
#define  VIDTRICK_MAX_JUMPEABLE_FIELDS_SPEED_MIN          300
#define  VIDTRICK_MAX_JUMPEABLE_FIELDS_SPEED_MAX          500

/* from this value of Number of fields to skip, smooth has to require HDD discontinuities */
#define  VIDTRICK_LEVEL_SPACE_BUFFERS                     34
/* as soon as the counter of fields to skip is over VIDTRICK_LEVEL_SKIP_BACKWARD_PICTURE */
/* smooth backward stops decoding all the pictures (stops decode all B pictures) */
#define  VIDTRICK_LEVEL_SKIP_BACKWARD_PICTURE             4
/* smooth backward may not be able to provide enough pictures to display even if the speed */
/* is between 0 and -100. Then as soon as the counter of fields to skip (which can only be increased */
/* by display queue locked event with such a speed) is over this value, smooth backward  */
/* stops decoding all the pictures (stops decode all B pictures), and increases the counter */
/* of fields to be displayed of each pictures it inserts in display queue in order */
/* to compensate for the not decoded pictures */
#define  VIDTRICK_LEVEL_STOP_SKIPPING_DISPLAY             3

/* Backward Mode characteristics */
/* bit buffer size(no more used because now the linear bitbuffer are allocated
 * as 2 halves of the circular bitbuffer memory area. */
/*#define  VIDTRICK_LINEAR_BIT_BUFFER_SIZE                  (1024 * 1024)*/
/* max number of linear bitbuffers to search in for the displayed picture before we restart
 * the display after a forward->backward transition */
#define  VIDTRICK_MAX_FLUSHES_TO_RESTART_FROM_THE_SAME_PICTURE  1
/* bit buffer max storeable pictures */
#define  VIDTRICK_BACKWARD_BUFFER_MAX_PICTURES_NB         200
/* if bitstream bit rate is very small, underflows event tries to require only by bit buffer */
#define  VIDTRICK_BACKWARD_PICTURES_NB_BY_BUFFER          150
/* max number of pictures to drift backward when switching from negative to positive speed on in order */
/* start new forward display from the lat backward displayed picture */
#define  VIDTRICK_MAX_BACKWARD_DRIFT_PICTURE_NB           40
/* required pictures nb for overlap */
#define  VIDTRICK_BACKWARD_OVERLAP_PICTURES               8
/* if big bitstream bit rate value underflow event VIDTRICK_BACKWARD_OVERLAP_BIT_BUFFER_RATIO % */
/* of bit buffer size  for overlap */
#define  VIDTRICK_BACKWARD_OVERLAP_BIT_BUFFER_RATIO       20
/* minimum size in bytes to be injected in a bit buffer.  */
#define  VIDTRICK_MIN_DATA_SIZE_TO_BE_INJECTED            (2 * 1024)
/* nb of pictures(fields) smooth backward has to have continuously in display queue */
#define  VIDTRICK_PICTURES_NB_TO_BE_IN_DISPLAY_QUEUE      3
/* additional nb of pictures(fields) smooth backward has to have continuously in display queue with blitter display platforms */
#define  VIDTRICK_ADDITONAL_PICTURES_NB_TO_BE_IN_DISPLAY_QUEUE_FOR_BLIT_DISP      4


/* others Backward defines */
/* max start code datas size */
#define  VIDTRICK_START_CODE_HEADER_MAX_SIZE              257
/* start code size */
#define  VIDTRICK_START_CODE_SIZE                         4
/* min pictures to be found by bit buffer */
#define  VIDTRICK_NB_PIC_MIN_TO_BE_FOUND_BY_BUFFER        4
/* smooth backward error recovery : a picture decode duration can not be over VIDTRICK_NB_VSYNC_MAX_FOR_A_DECODE */
#define  VIDTRICK_NB_VSYNC_MAX_FOR_A_DECODE               20
/* smooth backward error recovery : if VIDTRICK_MAX_UNSUCCESSFULLY_DECODE_TRY unsuccessfull consecutive picture decode */
/* stop and a reset of the whole smooth backward should be done */
#define  VIDTRICK_MAX_UNSUCCESSFULLY_DECODE_TRY           3
/* smooth backward error recovery : a SC search duration can not be over VIDTRICK_NB_VSYNC_MAX_TO_FIND_SC */
#define  VIDTRICK_NB_VSYNC_MAX_TO_FIND_SC                 50
/* smooth backward error recovery : a picture has to be decoded every VIDTRICK_NB_VSYNC_WITHOUT_ANY_DECODE vsyncs */
#define  VIDTRICK_NB_VSYNC_WITHOUT_ANY_DECODE             300
/* when setting positive speed just after negative one, number of tries of recognition of the last displayed */
/* by backward process must have a max number security, in order to ensure the start of the forward decode. */
#define  VIDTRICK_MAX_UNSUCCESSFULL_PICTURE_RECOGNITION_TRY (2 * VIDTRICK_BACKWARD_BUFFER_MAX_PICTURES_NB)
/* first extended temporal ref in backward */
#define  VIDTRICK_SMOOTH_FIRST_EXTENDED_TEMP_REF          50000000
/* default frame rate */
#define  VIDTRICK_DEFAULT_FRAME_RATE                      25
/* if buffer are dicontinued extended temporal ref hole maust be over VIDTRICK_EXT_TEMP_REF_OFFSET_DISCONTINUITY_MIN */
/* in order to ensure all the pictures ext temp ref of the new bit buffer to be under the last displayable pictures */
/* extended temp ref of the associated bit buffer */
#define  VIDTRICK_EXT_TEMP_REF_OFFSET_DISCONTINUITY_MIN   20
/* time the smooth backward task should be waked up if no event occurs, in order to avoid of forgotten posted commmand */
#define  MAX_WAIT_ORDER_TIME_WATCHDOG                     (3 * STVID_MAX_VSYNC_DURATION)
/* smooth backward fluidity process has to be set only if too many display queue empty events */
/* occur a too little time. thats why, every VIDTRICK_VSYNC_EVTS_DQE_MAX_NB Vsyncs Display */
/* queue empty event numbre has to be decreased in order to allow this event to occur sometimes */
#define VIDTRICK_VSYNC_EVTS_DQE_MAX_NB                    300

/* Value below determines the time during which, during bit buffer parsing in
smooth backward mode, no start code search will be launched after each VSync.
This is to give the hand to the rest of the system for some time at each VSync,
as otherwise (the parsing being a higher priority than a lot of tasks) those
other tasks would not be scheduled for a too long time. The time has been
calculated to give enough time in particular to the display task, but it should
not be too big otherwise the parsing will last for too long, and so the higher
negative speeds will suffer of problems of fluidity. */
#define VIDTRICK_DEFAULT_TIME_TO_WAIT_AT_VSYNC_BEFORE_START_CODE_SEARCH_IN_TICKS (STVID_MAX_VSYNC_DURATION / 5)

/* Number of bytes to compare between the data of two pictures when trying to
find the matching picture of one buffer in the associated buffer */
#define VIDTRICK_NUMBER_OF_BYTES_TO_COMPARE_WHEN_MATCHING_TWO_LINEAR_BITBUFFERS          100

#ifdef TRICKMOD_DEBUG
#define  EXTENDED_DECODE_COPY_VERIFY    10
#endif /* TRICKMOD_DEBUG */

/* Dummy value that we set to check that a handle is valid, only one chance out of 2^32 to get it by accident ! */
#define  VIDTRICK_VALID_HANDLE 0x01851850

#define INVALID_ID          (0xFFFFFFFF)

/* Private Macros ----------------------------------------------------------- */
#define  Min(Val1, Val2) (((Val1) < (Val2))?(Val1):(Val2))
#define  Max(Val1, Val2) (((Val1) > (Val2))?(Val1):(Val2))
#define  IsPictureSecondField(Picture_p) (                 \
        (   ((Picture_p)->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure != STVID_PICTURE_STRUCTURE_FRAME) \
         && (!((Picture_p)->PictureInfoFromDecode.AdditionalStreamInfo.ExpectingSecondField))))
#define  IsPictureField(Picture_p) ((Picture_p)->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure != STVID_PICTURE_STRUCTURE_FRAME)
#define  HasPictureASecondField(Picture_p)      ((Picture_p)->PictureInfoFromDecode.AdditionalStreamInfo.ExpectingSecondField)
#define  IsPictureFrame(Picture_p)              ((Picture_p)->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
#define  PictureMpegTypeIsB(Picture_p)          ((Picture_p)->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B)
#define  PictureMpegTypeIsI(Picture_p)          ((Picture_p)->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)
#define  PictureMpegTypeIsP(Picture_p)          ((Picture_p)->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_P)
#define  PictureMpegTypeIsReference(Picture_p)  ((Picture_p)->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame != STVID_MPEG_FRAME_B)

#define  IsPictureBufferAFrameOrFirstField(PictureBuffer_p)     ((PictureBuffer_p) == (PictureBuffer_p)->FrameBuffer_p->FrameOrFirstFieldPicture_p)
#define  IsAPrimaryPictureBuffer(PictureBuffer_p)             ((PictureBuffer_p)->FrameBuffer_p->FrameBufferType != VIDBUFF_SECONDARY_FRAME_BUFFER)

#define  IsStvidPictureInfosExpectingSecondField(PictureInfos_p) \
           ((((PictureInfos_p)->VideoParams.TopFieldFirst) && ((PictureInfos_p)->VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD))  || \
            ((!((PictureInfos_p)->VideoParams.TopFieldFirst)) && ((PictureInfos_p)->VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)))

#define IsPictureAuthorizedToBeDecoded(handle,Picture_p)  \
        (       (Picture_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)     ||  \
            (   (Picture_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_P)&&       \
                (   (((VIDTRICK_Data_t *) handle)->DecodedPictures == STVID_DECODED_PICTURES_ALL)||                 \
                    (((VIDTRICK_Data_t *) handle)->DecodedPictures == STVID_DECODED_PICTURES_IP )                   \
                )                                                                                                   \
            )                                                                                                   ||  \
            (   (Picture_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B)&&       \
                (                                                                                                   \
                        ((VIDTRICK_Data_t *) handle)->DecodedPictures == STVID_DECODED_PICTURES_ALL                 \
                )                                                                                                   \
            )                                                                                                       \
        )

#define  MPEGFrame2Char(Type)  (                                        \
               (  (Type) == STVID_MPEG_FRAME_I)                         \
                ? 'I' : (  ((Type) == STVID_MPEG_FRAME_P)               \
                         ? 'P' : (  ((Type) == STVID_MPEG_FRAME_B)      \
                                  ? 'B'  : 'U') ) )
#define  PictureStruct2String(PicStruct)  (                                                            \
               (  (PicStruct) == STVID_PICTURE_STRUCTURE_FRAME)                                        \
                ? "Frm" : (  ((PicStruct) == STVID_PICTURE_STRUCTURE_TOP_FIELD)                        \
                           ? "Top" : (  ((PicStruct) == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)          \
                                      ? "Bot"  : "---") ) )


/* Private Variables (static)------------------------------------------------ */

/* Private Function prototypes ---------------------------------------------- */
static void vidtrick_FillPictureBuffer(VIDBUFF_PictureBuffer_t *PB_p, VIDTRICK_PictureInfo_t *vidtric_p);

static ST_ErrorCode_t GetBitRateValue(const VIDTRICK_Handle_t TrickModeHandle, U32 * const BitRateValue_p);
static void NewPictureSkippedByDecodeWhithoutTrickmodeRequest(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
static void NewPictureSkippedByDisplayWhithoutTrickmodeRequest(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
static void ReadyToDecodeNewPictureTrickMode(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
static void SpeedInitialize(const VIDTRICK_Handle_t TrickModeHandle);
static ST_ErrorCode_t StartTrickModeTask(const VIDTRICK_Handle_t TrickModeHandle);
static ST_ErrorCode_t StopTrickModeTask(const VIDTRICK_Handle_t TrickModeHandle);
static void TrickModeTaskFunc(const VIDTRICK_Handle_t TrickModeHandle);
static void UpdateUnderflowEventParams(const VIDTRICK_Handle_t TrickModeHandle);
static void BitbufferUnderflowEventOccured(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);

#ifdef ST_smoothbackward
static void AffectReferencesPictures(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const CurrentBuffer_p);
static void AffectTheBufferSpacing(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const BufferToBeFilled_p);
static ST_ErrorCode_t AllocateLinearBitBuffers(const VIDTRICK_Handle_t TrickModeHandle);
static void AskForBackwardDrift(const VIDTRICK_Handle_t TrickModeHandle);
static void AskForLinearBitBufferFlush(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * BufferToBeFilled_p, const VIDTRICK_FushBitBufferManner_t FlushManner);
static ST_ErrorCode_t BackwardModeInit(const VIDTRICK_Handle_t TrickModeHandle, const BOOL ReInit);
static ST_ErrorCode_t BackwardWarmReset(const VIDTRICK_Handle_t TrickModeHandle);
static BOOL CheckIfSecondFieldIsAvailable(VIDTRICK_PictureInfo_t * const CurrentPicture_p);
static BOOL CheckIfDisplayHasToBeEnabled(const VIDTRICK_Handle_t TrickModeHandle, STVID_PictureInfos_t * const PictureInfos_p, const U16 TemporalReference, const U16 VbvDelay);
static BOOL CheckIfPictureCanBeDisplayed(const VIDTRICK_Handle_t TrickModeHandle, STVID_PictureInfos_t * const PictureInfos_p);
static void CheckIfLinearBufferHasToBeFlushed(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t const * CurrentlyDisplayedPicture_p);
static BOOL CheckIfPictureNotLinkedToLinearBuffer(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t const * CurrentlyDisplayedPicture_p);
static void ComputeMaxJumpeableFields(const VIDTRICK_Handle_t TrickModeHandle, const S32 Speed, const U32 NumberOfPicturesInTheLinearBitbuffer);
static void DisplayDiscontinuityAffectPictures(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const JustParsedBuffer_p);
static void FindFirstIPictureOfLinearBitBuffer(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const LinearBitBuffer_p, VIDTRICK_PictureInfo_t ** FirstIpicture_p, U32* const Index_p);
static void FindFirstDisplayablePictureOfLinearBitBuffer(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const LinearBitBuffer_p, VIDTRICK_PictureInfo_t ** FirstDisplayablePicture_p);
static BOOL FindFirstPictureToBeDisplayed(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const LinearBitBuffer_p, VIDTRICK_PictureInfo_t ** FirstDisplayablePicture_p);
static void FindFirstReferencePictureOfLinearBitBuffer(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const LinearBitBuffer_p, VIDTRICK_PictureInfo_t ** FirstReferencePicture_p);
static BOOL FindMatchingPictureWithTemporalReference(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const CurrentBuffer_p);
static void FindPictureToComputeFromAndInsertInDisplayQueue(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t ** const PictureToComputeFrom_p);
static void FreeUnnecessaryFrameBuffers(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t * const PictureToBeDisplayed_p, VIDTRICK_PictureInfo_t * const FollowingPicture_p);
static ST_ErrorCode_t ForwardModeInit(const VIDTRICK_Handle_t TrickModeHandle);
static ST_ErrorCode_t ForwardModeRequested(const VIDTRICK_Handle_t TrickModeHandle);
static U32 GetMaxAlignmentBetweenCDFifoAndSCD(const VIDDEC_Handle_t DecodeHandle);
static BOOL HasPictureReferenceForDecode(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t const * PictureToBeDecodedInfo_p, VIDTRICK_PictureInfo_t const * PictureInfoInFrameBuffer_p);
static void LaunchPictureDecode(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t * PictureToBeDecodedInfo_p, VIDBUFF_PictureBuffer_t * PictureBufferToDecodeIn_p, VIDTRICK_PictureInfo_t * const PictureInfoInFrameBuffer_p);
static void LinearBitBufferFullyParsed(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const JustParsedBuffer_p);
static void DecoderReadyForBackwardMode(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
static void NewDecodedPicture(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
static void NewPictureDecodeIsRequired(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t * PictureToBeDecodedInfo_p, VIDBUFF_PictureBuffer_t * PictureBufferToDecodeIn_p, VIDTRICK_PictureInfo_t * const PictureInfoInFrameBuffer_p);
static void NewPictureFoundInBitBuffer(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, STEVT_EventConstant_t Event, const void * EventData_p, const void * SubscriberData_p);
static void NewSequenceFoundInBitBuffer(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, STEVT_EventConstant_t Event, const void * EventData_p, const void * SubscriberData_p);
static void NewStartCodeSearchIsRequired(const VIDTRICK_Handle_t TrickModeHandle, const VIDDEC_StartCodeSearchMode_t SearchMode, const BOOL FirstSliceDetect, void * const SearchAddress_p);
static void PictureOutOfDisplay(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
static void NoNextPictureToDisplay(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
static void PictureDecodeHasErrors(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t * const BadlyDecodedPicture_p);
static BOOL PutPictureStartCodeAndComputeValidDataBuffer(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const  LinearBitBuffer_p, const BOOL Overlap);
static void PutSequenceStartCodeAndComputeLinearBitBuffers(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const  LinearBitBuffer_p);
static void RenumberExtendedTemporalReferences(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const CurrentBuffer_p);
static void RenumberTimeCodes(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const CurrentBuffer_p);
static S32 GetExtendedTempRefAccordingToPreviousBuffer(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const CurrentBuffer_p);
static void RepeatDisplayFieldsOfToBeInsertedPictureIfNecessary(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t * LastInsertedPictureInDQ_p, VIDTRICK_PictureInfo_t * const PictureToInsertInDQ_p);
static void ResetReferencesAndFreeNotDisplayedPictures(const VIDTRICK_Handle_t TrickModeHandle);
static BOOL SearchForMostRecentDecodableReferencePictureRecord(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t * const CurrentPictureInfo_p, VIDTRICK_PictureInfo_t ** const FoundPictureInfo_p);
static BOOL SearchForNormalRevSpeedPreviousPicToDisplay(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t * const CurrentPictureInfo_p, VIDTRICK_PictureInfo_t ** const NextInDisplayOrder_pPictureInfo_p);
static void SmoothBackwardInitStructure(const VIDTRICK_Handle_t TrickModeHandle);
static void StartCodeFoundInBitBuffer(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, STEVT_EventConstant_t Event, const void * EventData_p, const void * SubscriberData_p);
static ST_ErrorCode_t StopSmoothBackward(const VIDTRICK_Handle_t TrickModeHandle, const BOOL FreeNotDisplayedPictures);
static void UnlinkBufferToBeFLushedPictures(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const BufferToBeFilled_p);
static void UnlockPicture(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t * const PictureInfo_p, const BOOL UnlockDefinitively, const BOOL HasBeenDisplayed, const BOOL UnusefulToDecAnyB);
static void ResetPictureInfoStatus(const VIDTRICK_Handle_t     TrickModeHandle, VIDTRICK_PictureInfo_t *    PictureInfo_p);
static void ReleasePictureInfoElement(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t * const PictureInfoElemToRelease_p);
static void UpdateFirstPicturesInDisplayQueue(const VIDTRICK_Handle_t TrickModeHandle);
static void UpdateLastPictureInDisplayQueueList(const VIDTRICK_Handle_t TrickModeHandle);
static BOOL vidtrick_SearchAndGetNextStartCode(const U8*  const BeginSearchFrom_p,
                                               const U8*  const MaxSearchAddress_p,
                                               U8*  const StartCode_p,
                                               U8** StartCodeAddress_p);
static ST_ErrorCode_t AssociatePictureBuffers(const VIDTRICK_Handle_t     TrickModeHandle,
                                        VIDTRICK_PictureInfo_t *    PictureInfo_p);
static ST_ErrorCode_t ReusePictureBuffer(const VIDTRICK_Handle_t     TrickModeHandle,
                                        VIDTRICK_PictureInfo_t *    PictureInfo_p);
static VIDTRICK_PictureInfo_t * LookForPictureBufferToReuse(const VIDTRICK_Handle_t     TrickModeHandle,
                                         VIDTRICK_LinearBitBuffer_t *  LinearBitBuffer_p);
static ST_ErrorCode_t ReleaseAllPictureInfoElements(const VIDTRICK_Handle_t     TrickModeHandle);
#endif /* ST_smoothbackward */

static void PurgeFrameBuffers(const VIDTRICK_Handle_t TrickModeHandle);

#ifdef ST_smoothbackward
#ifdef TRACE_UART_EXT_TEMP_REF
static void TraceAllExtendedTemporalReferenceOfTheBuffer(const VIDTRICK_Handle_t TrickModeHandle,  VIDTRICK_LinearBitBuffer_t * const LinearBitBuffer_p, const U32 NumberOfPictureToTrace);
#endif  /* TRACE_UART_EXT_TEMP_REF */
#endif /* ST_smoothbackward */

#ifdef TRICKMOD_DEBUG
static void TrickModeErrorLine(U32 Line);
#define TrickModeError()       TrickModeErrorLine(__LINE__)
#endif /* TRICKMOD_DEBUG */

#ifdef TRACE_UART_DISPLAY_SKIP
static void TrickModeTraces(const S32 NbFieldsIfPosToSkipIfNegToRepeat, const BOOL SkipAllPPictures);
#endif

#ifdef DEBUG_DISPLAY_QUEUE
static ST_ErrorCode_t DbgCheckPicturesInDisplayQueue(const VIDTRICK_Handle_t     TrickModeHandle);
#endif

/* Public Functions --------------------------------------------------------- */
#ifdef TRACE_UART_MAIN
extern void PrintFrameBuffers(const VIDBUFF_Handle_t BuffersHandle);
#endif /* TRACE_UART_MAIN */

/*******************************************************************************
 * Name        : VIDTRICK_DisableSpeedControl
 * Description : Disables Speed control when video is stopped or paused
 * Parameters  : TrickMode handle
 * Assumptions : MUST be called by VID_Stop !
 * Limitations :
 * Returns     : ST_NO_ERROR
 * *******************************************************************************/
ST_ErrorCode_t VIDTRICK_DisableSpeedControl(const VIDTRICK_Handle_t TrickModeHandle)
{
    VIDTRICK_Data_t * VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;

    /* disable control speed */
    VIDTRICK_Data_p->Flags.SpeedControlEnabled = FALSE;

    return(ST_NO_ERROR);
} /* end of VIDTRICK_DisableSpeedControl() function */

/*******************************************************************************
* Name        : VIDTRICK_EnableSpeedControl
* Description : Enables Speed control when video is started
* Parameters  : TrickMode handle
* Assumptions : MUST be called by VID_Start !
* Limitations :
* Returns     : ST_NO_ERROR
* *******************************************************************************/
ST_ErrorCode_t VIDTRICK_EnableSpeedControl(const VIDTRICK_Handle_t TrickModeHandle)
{
    VIDTRICK_Data_t * VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;

    /* enable control speed */
    VIDTRICK_Data_p->Flags.SpeedControlEnabled = TRUE;

    return(ST_NO_ERROR);
} /* end of VIDTRICK_EnableSpeedControl() function */


/*******************************************************************************
* Name        : VIDTRICK_GetSpeed
* Description : Gets speed of video
* Parameters  : Video TrickMode handle, Speed
* Assumptions :
* Limitations :
* Returns     : ST_NO_ERROR if success,
* *******************************************************************************/
ST_ErrorCode_t VIDTRICK_GetSpeed(const VIDTRICK_Handle_t TrickModeHandle, S32 * const Speed_p)
{
    VIDTRICK_Data_t * VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;

    *Speed_p = VIDTRICK_Data_p->MainParams.RequiredSpeed;

    return(ST_NO_ERROR);
} /* end of VIDTRICK_GetSpeed() function */


/*******************************************************************************
Name        : VIDTRICK_Init
Description :
Parameters  : Init params, returns TrickMode handle
Assumptions : InitParams_p is not NULL
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_ALREADY_INITIALIZED if already initialised
              ST_ERROR_BAD_PARAMETER if bad parameter
*******************************************************************************/
ST_ErrorCode_t VIDTRICK_Init(const VIDTRICK_InitParams_t * const InitParams_p, VIDTRICK_Handle_t * const TrickModeHandle_p)
{
    VIDTRICK_Data_t *                   VIDTRICK_Data_p;

#ifdef ST_oldmpeg2codec /* if ST_oldmpeg2codec only */
    VIDPROD_Properties_t * VIDPROD_Data_p;
#endif /* ST_oldmpeg2codec */

    STEVT_OpenParams_t                  STEVT_OpenParams;
    STEVT_DeviceSubscribeParams_t       STEVT_SubscribeParams;
    ST_ErrorCode_t                      ErrorCode;
#ifdef ST_smoothbackward
    U32                                 SCDAlignmentInBytes;
    U32                                 i;
    BOOL                                IsHardwareSuppportingSmoothBackward = FALSE;
#endif /* ST_smoothbackward */

    /* Allocate a HALTRICK structure */
    SAFE_MEMORY_ALLOCATE(VIDTRICK_Data_p, VIDTRICK_Data_t * ,InitParams_p->CPUPartition_p, sizeof(VIDTRICK_Data_t));
    if (VIDTRICK_Data_p == NULL)
    {
#ifdef ST_smoothbackward
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("VIDTRICK_Data_p allocation failed!\n\r"));
#endif /* TRICKMOD_DEBUG */
#endif /* ST_smoothbackward */

        /* Error: allocation failed */
        return(ST_ERROR_NO_MEMORY);
    }

#ifdef TRACE_UART_MAIN
    TraceBuffer(("\n\n\r**************************\n\rTrickmod UART initializing.\n\r**************************\n\n\n\r"));
#endif

    /* Remember partition */
    VIDTRICK_Data_p->CPUPartition_p = InitParams_p->CPUPartition_p;

    /* Allocation succeeded: make handle point on structure */
    *TrickModeHandle_p = (VIDTRICK_Handle_t *) VIDTRICK_Data_p;
    VIDTRICK_Data_p->ValidityCheck = VIDTRICK_VALID_HANDLE;

    /* Store parameters */
    VIDTRICK_Data_p->DecoderNumber = InitParams_p->DecoderNumber;
    VIDTRICK_Data_p->DisplayHandle = InitParams_p->DisplayHandle;
/*    VIDTRICK_Data_p->DecodeHandle = InitParams_p->DecodeHandle;*/
#ifdef ST_oldmpeg2codec /* if ST_oldmpeg2codec only */
    VIDPROD_Data_p = (VIDPROD_Properties_t *) InitParams_p->DecodeHandle;
    VIDTRICK_Data_p->DecodeHandle = (VIDDEC_Handle_t *) VIDPROD_Data_p->InternalProducerHandle;
#else /* ST_oldmpeg2codec */
    VIDTRICK_Data_p->DecodeHandle = (VIDDEC_Handle_t *) InitParams_p->DecodeHandle;
#endif /* ST_oldmpeg2codec */
#ifdef ST_smoothbackward
    VIDTRICK_Data_p->BufferManagerHandle = InitParams_p->BufferManagerHandle;
#endif /* ST_smoothbackward */

    VIDTRICK_Data_p->AvmemPartitionHandle = InitParams_p->AvmemPartitionHandle;
#ifdef ST_ordqueue
    VIDTRICK_Data_p->OrderingQueueHandle = InitParams_p->OrderingQueueHandle;
#endif /* ST_ordqueue */
    VIDTRICK_Data_p->MainParams.VirtualClockCounter = 0;
    VIDTRICK_Data_p->MainParams.Speed = 100;
    VIDTRICK_Data_p->MainParams.RequiredSpeed = 100;
    VIDTRICK_Data_p->MainParams.BitRateValue = VIDTRICK_DEFAULT_BITRATE;
    VIDTRICK_Data_p->MainParams.FrameRateValue = VIDTRICK_DEFAULT_FRAME_RATE * 1000 ; /* Hz -> mHz */
    memset(&VIDTRICK_Data_p->MainParams.LastSequenceInfo, 0, sizeof(STVID_SequenceInfo_t));
    VIDTRICK_Data_p->Flags.RequiredSpeedTakenInAccount = TRUE;
    VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat = 0;
    VIDTRICK_Data_p->Flags.MustSkipAllPPictures = FALSE;
    VIDTRICK_Data_p->Flags.SpeedControlEnabled = FALSE;
    VIDTRICK_Data_p->Flags.SecondFieldHasToBeSkipped = FALSE;
    VIDTRICK_Data_p->Flags.WaitingForASecondField = FALSE;
    VIDTRICK_Data_p->Flags.DoNotTryToRestartOnTheLastDisplayedPicture = FALSE;

    /* statistics parameters */
    VIDTRICK_Data_p->NbTreatedPicturesSinceLastDecodedI = 0;

    /* Display parameters init */
    VIDTRICK_Data_p->DisplayParams.DisplayStart = VIDDISP_DISPLAY_START_CARE_MPEG_FRAME;
    VIDTRICK_Data_p->DisplayParams.DisplayForwardNotBackward = TRUE;
    VIDTRICK_Data_p->DisplayParams.DisplayWithCareOfFieldsPolarity = TRUE;

    /* Backward I only parameters */
    VIDTRICK_Data_p->BackwardIOnly.NbVsyncSinceLastSentDriftEvt = 0;

    VIDTRICK_Data_p->EventDisplayChangeToBeNotifiedInTask = FALSE;
    VIDTRICK_Data_p->IsWaitOrderTimeWatchDog = TRUE;

#ifdef ST_smoothbackward
    VIDTRICK_Data_p->NewStartCodeSearch.IsRequiredInTask = FALSE;
    VIDTRICK_Data_p->NewStartCodeSearch.SearchMode = VIDDEC_START_CODE_SEARCH_MODE_NORMAL;
    VIDTRICK_Data_p->NewStartCodeSearch.FirstSliceDetect = TRUE;
    VIDTRICK_Data_p->NewStartCodeSearch.SearchAddress_p = (void *) 0;
    VIDTRICK_Data_p->NewStartCodeSearch.MustWaitBecauseOfVSync = FALSE;
    VIDTRICK_Data_p->NewStartCodeSearch.DontStartBeforeThatTime = time_now();
    VIDTRICK_Data_p->DecodedPictures = STVID_DECODED_PICTURES_ALL;
    VIDTRICK_Data_p->BitBufferFullyParsedToBeProcessedInTask_p = NULL;
    VIDTRICK_Data_p->NbFieldsToBeInDisplayQueue = VIDTRICK_PICTURES_NB_TO_BE_IN_DISPLAY_QUEUE +
                                                  InitParams_p->DisplayDeviceAdditionnalFieldLatency;
    SAFE_MEMORY_ALLOCATE(VIDTRICK_Data_p->LastPictureInDisplayQueue_p,
                         VIDTRICK_PictureInfo_t ** ,
                         InitParams_p->CPUPartition_p,
                         sizeof(VIDTRICK_PictureInfo_t*) * (VIDTRICK_Data_p->NbFieldsToBeInDisplayQueue + 2));

    if (VIDTRICK_Data_p->LastPictureInDisplayQueue_p == NULL)
    {
        SAFE_MEMORY_DEALLOCATE(VIDTRICK_Data_p, VIDTRICK_Data_p->CPUPartition_p, sizeof(VIDTRICK_Data_t));
        /* Error: allocation failed */
        return(ST_ERROR_NO_MEMORY);
    }

    memset(VIDTRICK_Data_p->LastPictureInDisplayQueue_p, 0, sizeof(VIDTRICK_PictureInfo_t*) * (VIDTRICK_Data_p->NbFieldsToBeInDisplayQueue + 2));
#endif /* ST_smoothbackward */

    /* Semaphore protection init */
    VIDTRICK_Data_p->TrickModeParamsMutex_p = STOS_MutexCreatePriority();
    VIDTRICK_Data_p->WakeTrickModeTaskUp_p = semaphore_create_fifo_timeout(0);

#ifdef ST_smoothbackward
    /* Init to NULL while not initialised */
    VIDTRICK_Data_p->LinearBitBuffers.BufferA.PictureInfoTableStartAddress_p = NULL;
    VIDTRICK_Data_p->LinearBitBuffers.BufferA.PictureInfoTableAllocationAddress_p = NULL;
    VIDTRICK_Data_p->LinearBitBuffers.BufferA.NbPicturesInside = 0;
    VIDTRICK_Data_p->LinearBitBuffers.BufferB.PictureInfoTableStartAddress_p = NULL;
    VIDTRICK_Data_p->LinearBitBuffers.BufferB.PictureInfoTableAllocationAddress_p = NULL;
    VIDTRICK_Data_p->LinearBitBuffers.BufferB.NbPicturesInside = 0;

    /* Initialize for the smooth backward feature if the hardward supports it. */
    if (VIDDEC_GetSmoothBackwardCapability(VIDTRICK_Data_p->DecodeHandle) == ST_NO_ERROR)
    {
        /* Hardware supports smooth backward: initialise the feature */
        IsHardwareSuppportingSmoothBackward = TRUE;

        /* The tables which contain all the start code infos of both backward buffers have to be allocated here */
        /* Buffer A */
        SAFE_MEMORY_ALLOCATE(VIDTRICK_Data_p->LinearBitBuffers.BufferA.PictureInfoTableAllocationAddress_p,
                             VIDTRICK_PictureInfo_t *,
                             InitParams_p->CPUPartition_p,
                             VIDTRICK_BACKWARD_BUFFER_MAX_PICTURES_NB * sizeof(VIDTRICK_PictureInfo_t));
        if (VIDTRICK_Data_p->LinearBitBuffers.BufferA.PictureInfoTableAllocationAddress_p == NULL)
        {
            /* Error: allocation failed */
#ifdef TRICKMOD_DEBUG
            TrickModeError();
            TraceBuffer(("PictureInfoTable allocation failed!\n\r"));
#endif /* TRICKMOD_DEBUG */
            VIDTRICK_Term(*TrickModeHandle_p);
            return(ST_ERROR_NO_MEMORY);
        }

        /* Buffer B */
        SAFE_MEMORY_ALLOCATE(VIDTRICK_Data_p->LinearBitBuffers.BufferB.PictureInfoTableAllocationAddress_p,
                             VIDTRICK_PictureInfo_t *,
                             InitParams_p->CPUPartition_p,
                             VIDTRICK_BACKWARD_BUFFER_MAX_PICTURES_NB * sizeof(VIDTRICK_PictureInfo_t));
        if (VIDTRICK_Data_p->LinearBitBuffers.BufferB.PictureInfoTableAllocationAddress_p == NULL)
        {
            /* Error: allocation failed */
#ifdef TRICKMOD_DEBUG
            TrickModeError();
            TraceBuffer(("PictureInfoTable allocation failed!\n\r"));
#endif /* TRICKMOD_DEBUG */
            VIDTRICK_Term(*TrickModeHandle_p);
            return(ST_ERROR_NO_MEMORY);
        }

        VIDTRICK_Data_p->LinearBitBuffers.BufferA.PictureInfoTableStartAddress_p = VIDTRICK_Data_p->LinearBitBuffers.BufferA.PictureInfoTableAllocationAddress_p;
        VIDTRICK_Data_p->LinearBitBuffers.BufferB.PictureInfoTableStartAddress_p = VIDTRICK_Data_p->LinearBitBuffers.BufferB.PictureInfoTableAllocationAddress_p;

        /* buffer Appartenance */
        for (i = 0 ; i < VIDTRICK_BACKWARD_BUFFER_MAX_PICTURES_NB ; i++)
        {
            VIDTRICK_Data_p->LinearBitBuffers.BufferA.PictureInfoTableStartAddress_p[i].LinearBitBufferStoredInfo_p = &VIDTRICK_Data_p->LinearBitBuffers.BufferA;
            VIDTRICK_Data_p->LinearBitBuffers.BufferA.PictureInfoTableStartAddress_p[i].PictureBuffer_p = NULL;
            VIDTRICK_Data_p->LinearBitBuffers.BufferB.PictureInfoTableStartAddress_p[i].LinearBitBufferStoredInfo_p = &VIDTRICK_Data_p->LinearBitBuffers.BufferB;
            VIDTRICK_Data_p->LinearBitBuffers.BufferB.PictureInfoTableStartAddress_p[i].PictureBuffer_p = NULL;
        }

        /* Associated informations for BufferA & BufferB */

        /* Cross references for buffers A and B. */
        VIDTRICK_Data_p->LinearBitBuffers.BufferA.AssociatedLinearBitBuffer_p = &(VIDTRICK_Data_p->LinearBitBuffers.BufferB);
        VIDTRICK_Data_p->LinearBitBuffers.BufferB.AssociatedLinearBitBuffer_p = &(VIDTRICK_Data_p->LinearBitBuffers.BufferA);

        /* init */
        VIDTRICK_Data_p->Stop.TrickmodIsStopped = TRUE;
        VIDTRICK_Data_p->Flags.SmoothBackwardIsInitializing = FALSE;
        VIDTRICK_Data_p->Flags.BackwardToForwardInitializing = FALSE;
        VIDTRICK_Data_p->BackwardWarmReset.OnGoing = FALSE;
        VIDTRICK_Data_p->BackwardWarmReset.MustDecodeSecondField= FALSE;

        SmoothBackwardInitStructure(*TrickModeHandle_p);

        /* Get the SCD alignment */
        ErrorCode = VIDDEC_GetSCDAlignmentInBytes(VIDTRICK_Data_p->DecodeHandle, &SCDAlignmentInBytes);
        VIDTRICK_Data_p->SCDAlignmentInBytes = SCDAlignmentInBytes;
#ifdef TRICKMOD_DEBUG
        if ((ErrorCode != ST_NO_ERROR) || (SCDAlignmentInBytes == 0))
        {
            TrickModeError();
            TraceBuffer(("VIDDEC_GetSCDAlignmentInBytes failed with ErrorCode=%d.\n\r",ErrorCode));
        }
#endif /* TRICKMOD_DEBUG */
        if (ErrorCode == ST_NO_ERROR)
        {
            /* Find the number of blocks in SCD unit where the Start Codes found     */
            /* are discarded.                                                        */
            VIDTRICK_Data_p->LinearBitBuffers.BufferA.NumberOfBytesToDiscardAtTheEndOfTheBuffer =
                    SCDAlignmentInBytes * (U8)(((VIDTRICK_START_CODE_HEADER_MAX_SIZE + VIDTRICK_START_CODE_SIZE + 1) / SCDAlignmentInBytes) + 1);

            /* Find the number of blocks in SCD unit where the Start Codes found     */
            /* are discarded.                                                        */
            VIDTRICK_Data_p->LinearBitBuffers.BufferB.NumberOfBytesToDiscardAtTheEndOfTheBuffer =
                    VIDTRICK_Data_p->LinearBitBuffers.BufferA.NumberOfBytesToDiscardAtTheEndOfTheBuffer;
        }
        /* Backward specific. Allocate 2 buffers A and B in shared memory.       */
        /* This module could be done only when backward is asked.                */
        /* CAUTION : A & B buffers should allocated with an alignement wich is the same than */
        /* The programmable Start code dectector alignement so that the start code detector can */
        /* be launched exactly from the begenning of the buffer */

        /*ErrorCode =*/AllocateLinearBitBuffers(*TrickModeHandle_p);
    } /* end smooth baxkward supported by hardware */
#endif /* ST_smoothbackward */

    /* Open EVT handle */
    ErrorCode = STEVT_Open(InitParams_p->EventsHandlerName, &STEVT_OpenParams, &(VIDTRICK_Data_p->EventsHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        VIDTRICK_Term(*TrickModeHandle_p);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    /* Register event to reached asked speed if necessary (skip on HDD) */
    ErrorCode = STEVT_RegisterDeviceEvent(VIDTRICK_Data_p->EventsHandle, InitParams_p->VideoName, STVID_SPEED_DRIFT_THRESHOLD_EVT, &(VIDTRICK_Data_p->RegisteredEventsID[VIDTRICK_SPEED_DRIFT_THRESHOLD_EVT_ID]));
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDTRICK_Data_p->EventsHandle, InitParams_p->VideoName, VIDTRICK_DISPLAY_PARAMS_CHANGE_EVT, &(VIDTRICK_Data_p->RegisteredEventsID[VIDTRICK_DISPLAY_PARAMS_CHANGE_EVT_ID]));
    }
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: registration failed, undo initialisations done */
        VIDTRICK_Term(*TrickModeHandle_p);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    /* Start Trick Mode task */
    VIDTRICK_Data_p->TrickModeTask.IsRunning = FALSE;
    ErrorCode = StartTrickModeTask(*TrickModeHandle_p);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: task creation failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Trick mode init: could not start Trick mode task ! (%s)", (ErrorCode == ST_ERROR_ALREADY_INITIALIZED?"already exists":"failed")));
        VIDTRICK_Term(*TrickModeHandle_p);
        if (ErrorCode == ST_ERROR_ALREADY_INITIALIZED)
        {
            /* Change error code because ST_ERROR_ALREADY_INITIALIZED is not allowed for _Init() functions */
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        return(ErrorCode);
    }

    /* Prepare subscribe parameters for all subscribes */
    STEVT_SubscribeParams.SubscriberData_p = (void *) (*TrickModeHandle_p);

    /* Subscribe to ready to decode new picture */
    STEVT_SubscribeParams.NotifyCallback = ReadyToDecodeNewPictureTrickMode;
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDTRICK_Data_p->EventsHandle, InitParams_p->VideoName, VIDDEC_READY_TO_DECODE_NEW_PICTURE_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        VIDTRICK_Term(*TrickModeHandle_p);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Subscribe to new sequence event */
    STEVT_SubscribeParams.NotifyCallback = NewSequenceFoundInBitBuffer;
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDTRICK_Data_p->EventsHandle, InitParams_p->VideoName, STVID_SEQUENCE_INFO_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        VIDTRICK_Term(*TrickModeHandle_p);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Subscribe to the underflow event */
    STEVT_SubscribeParams.NotifyCallback = BitbufferUnderflowEventOccured;
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDTRICK_Data_p->EventsHandle, InitParams_p->VideoName, STVID_DATA_UNDERFLOW_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        VIDTRICK_Term(*TrickModeHandle_p);
        return(ST_ERROR_BAD_PARAMETER);
    }


    /* Subscribe to new picture skipped by decode   */
    STEVT_SubscribeParams.NotifyCallback = NewPictureSkippedByDecodeWhithoutTrickmodeRequest;
    /* STEVT_SubscribeParams.SubscriberData_p = (void *) (*TrickModeHandle_p); */
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDTRICK_Data_p->EventsHandle, InitParams_p->VideoName, VIDDEC_NEW_PICTURE_SKIPPED_NOT_REQUESTED_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        VIDTRICK_Term(*TrickModeHandle_p);
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Subscribe to new picture skipped by display      */
    STEVT_SubscribeParams.NotifyCallback = NewPictureSkippedByDisplayWhithoutTrickmodeRequest;
    /* STEVT_SubscribeParams.SubscriberData_p = (void *) (*TrickModeHandle_p); */
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDTRICK_Data_p->EventsHandle, InitParams_p->VideoName, VIDDISP_NEW_PICTURE_SKIPPED_NOT_REQUESTED_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        VIDTRICK_Term(*TrickModeHandle_p);
        return(ST_ERROR_BAD_PARAMETER);
    }

#ifdef ST_smoothbackward
    /* Subscribe to more events if smooth backward */
    if (IsHardwareSuppportingSmoothBackward)
    {
        /* Subscribe to StartCodeFoundInBitBuffer */
        STEVT_SubscribeParams.NotifyCallback = StartCodeFoundInBitBuffer;
        /* STEVT_SubscribeParams.SubscriberData_p = (void *) (*TrickModeHandle_p); */
        ErrorCode = STEVT_SubscribeDeviceEvent(VIDTRICK_Data_p->EventsHandle, InitParams_p->VideoName, VIDDEC_START_CODE_FOUND_EVT, &STEVT_SubscribeParams);
#ifdef TRICKMOD_DEBUG
        if (ErrorCode != ST_NO_ERROR)
        {
            TrickModeError();
            TraceBuffer(("Subscription to VIDDEC_START_CODE_FOUND_EVT failed with ErrorCode=%d.\n\r", ErrorCode));
        }
#endif /* TRICKMOD_DEBUG */

        /* Subscribe to VIDDEC_START_CODE_SLICE_FOUND_EVT */
        STEVT_SubscribeParams.NotifyCallback = NewPictureFoundInBitBuffer;
        /* STEVT_SubscribeParams.SubscriberData_p = (void *) (*TrickModeHandle_p); */
        ErrorCode = STEVT_SubscribeDeviceEvent(VIDTRICK_Data_p->EventsHandle, InitParams_p->VideoName, VIDDEC_START_CODE_SLICE_FOUND_EVT, &STEVT_SubscribeParams);
#ifdef TRICKMOD_DEBUG
        if (ErrorCode != ST_NO_ERROR)
        {
            TrickModeError();
            TraceBuffer(("Subscription to VIDDEC_START_CODE_SLICE_FOUND_EVT failed with ErrorCode=%d.\n\r", ErrorCode));
        }
#endif /* TRICKMOD_DEBUG */

        /* Subscribe to DecoderIdle */
        STEVT_SubscribeParams.NotifyCallback = NewDecodedPicture;
        /* STEVT_SubscribeParams.SubscriberData_p = (void *) (*TrickModeHandle_p); */
        ErrorCode = STEVT_SubscribeDeviceEvent(VIDTRICK_Data_p->EventsHandle, InitParams_p->VideoName, VIDDEC_DECODER_IDLE_EVT, &STEVT_SubscribeParams);
#ifdef TRICKMOD_DEBUG
        if (ErrorCode != ST_NO_ERROR)
        {
            TrickModeError();
            TraceBuffer(("Subscription to VIDDEC_DECODER_IDLE_EVT failed with ErrorCode=%d.\n\r", ErrorCode));
        }
#endif /* TRICKMOD_DEBUG */

        /* Subscribe to VIDDEC_READY_TO_START_BACKWARD_MODE_EVT */
        STEVT_SubscribeParams.NotifyCallback = DecoderReadyForBackwardMode;
        /* STEVT_SubscribeParams.SubscriberData_p = (void *) (*TrickModeHandle_p); */
        ErrorCode = STEVT_SubscribeDeviceEvent(VIDTRICK_Data_p->EventsHandle, InitParams_p->VideoName, VIDDEC_READY_TO_START_BACKWARD_MODE_EVT, &STEVT_SubscribeParams);
#ifdef TRICKMOD_DEBUG
        if (ErrorCode != ST_NO_ERROR)
        {
            TrickModeError();
            TraceBuffer(("Subscription to VIDDEC_READY_TO_START_BACKWARD_MODE_EVT failed with ErrorCode=%d.\n\r", ErrorCode));
        }
#endif /* TRICKMOD_DEBUG */

        /* Subscribe to picture out of display */
        STEVT_SubscribeParams.NotifyCallback = PictureOutOfDisplay;
        STEVT_SubscribeParams.SubscriberData_p = (void *) (*TrickModeHandle_p);
        ErrorCode = STEVT_SubscribeDeviceEvent(VIDTRICK_Data_p->EventsHandle, InitParams_p->VideoName, VIDBUFF_PICTURE_OUT_OF_DISPLAY_EVT, &STEVT_SubscribeParams);
#ifdef TRICKMOD_DEBUG
        if (ErrorCode != ST_NO_ERROR)
        {
             TrickModeError();
             TraceBuffer(("Subscription to VIDBUFF_PICTURE_OUT_OF_DISPLAY_EVT failed with ErrorCode=%d.\n\r", ErrorCode));
        }
#endif /* TRICKMOD_DEBUG */

        /* Subscribe to VIDDISP_NO_NEXT_PICTURE_TO_DISPLAY_EVT */
        STEVT_SubscribeParams.NotifyCallback = NoNextPictureToDisplay;
        /* STEVT_SubscribeParams.SubscriberData_p = (void *) (*TrickModeHandle_p); */
        ErrorCode = STEVT_SubscribeDeviceEvent(VIDTRICK_Data_p->EventsHandle, InitParams_p->VideoName, VIDDISP_NO_NEXT_PICTURE_TO_DISPLAY_EVT, &STEVT_SubscribeParams);
#ifdef TRICKMOD_DEBUG
        if (ErrorCode != ST_NO_ERROR)
        {
            TrickModeError();
            TraceBuffer(("Subscription to VIDDISP_NO_NEXT_PICTURE_TO_DISPLAY_EVT failed with ErrorCode=%d.\n\r", ErrorCode));
        }
#endif /* TRICKMOD_DEBUG */
    } /* end smooth baxkward supported by hardware */
#endif /* ST_smoothbackward */

    return(ErrorCode);
} /* end of VIDTRICK_Init() */

/*******************************************************************************
Name        : VIDTRICK_SetDecodedPictures
Description : Set decoded pictures
Parameters  : Video decode handle, decoded pictures
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if unsupported mode
*******************************************************************************/
ST_ErrorCode_t VIDTRICK_SetDecodedPictures(const VIDTRICK_Handle_t TrickModeHandle, const STVID_DecodedPictures_t DecodedPictures)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VIDTRICK_Data_t   * VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;

    switch (DecodedPictures)
    {
        case STVID_DECODED_PICTURES_I :
            break;

        case STVID_DECODED_PICTURES_IP :
            break;

        case STVID_DECODED_PICTURES_ALL :
            break;

        default :
            /* Unrecognised mode: don't take it into account ! */
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

     /* Un lock Access to some parameters of Trick mode datas */
    STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p);

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Valid mode: adopt it */
        VIDTRICK_Data_p->DecodedPictures = DecodedPictures;
    }
    /* Un lock Access to some parameters of Trick mode data */
    STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);


    return(ErrorCode);
} /* End of VIDTRICK_SetDecodedPictures() function */


/*******************************************************************************
Name        : VIDTRICK_SetSpeed
Description : Sets speed of video
Parameters  : Video TrickMode handle, Speed
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t VIDTRICK_SetSpeed(const VIDTRICK_Handle_t TrickModeHandle, const S32 Speed)
{
    VIDTRICK_Data_t   * VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    BOOL                SkipInitSpeed = FALSE;

    /* Required Speed init */
    VIDTRICK_Data_p->MainParams.RequiredSpeed = Speed;
    VIDTRICK_Data_p->Flags.RequiredSpeedTakenInAccount = FALSE;

#ifdef TRACE_UART_MAIN
    TraceBuffer(("\n\r------------------\n\r"));
    TraceBuffer(("--- Speed %4d ---\n\r", Speed));
    TraceBuffer(("------------------\n\r"));
#endif

    /* Un lock Access to some parameters of Trick mode datas */
    STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p);

    /* ------------- Speed < 0 ----------------- */
    if (Speed < 0)  /* should be done only if the speed before was > 0 */
    {
        /* If previous speed was a positive speed */

#ifdef ST_smoothbackward
        /* ----- previous Speed >= 0 ------------ */
        if (VIDTRICK_Data_p->MainParams.Speed >= 0)
        {
#ifdef OLD_BACKWARD_FLUENT_DISPLAY_PROCESS
            /* change speed has to reset backward fluidity process ! */
            VIDTRICK_Data_p->BackwardFluidity.BackwardFluentDisplayProcessIsRunning = FALSE;
#endif /* OLD_BACKWARD_FLUENT_DISPLAY_PROCESS */

            /* Request Vid_dec to switch to Backward Mode */
            VIDTRICK_Data_p->Flags.SmoothBackwardIsInitializing = TRUE;
            ErrorCode = VIDDEC_BackwardModeRequested(VIDTRICK_Data_p->DecodeHandle);
        } /* end of if (VIDTRICK_Data_p->MainParams.Speed >= 0) */
        else
        {
            /* reset all previous next temporal jump between last flushed linear bit buffer and next */
            /* linear bit buffer to be flushed */
            VIDTRICK_Data_p->Discontinuity.NbFieldsToRecover = 0;

            /* If changing from fast backward to slow backward speed, the table of pictures to be inserted */
            /* in display queue has to be recomputed */
            VIDTRICK_Data_p->Flags.SmoothBackwardSpeedHasBeenChanged = TRUE;
            /* if new speed > ols speed, reset fast fluidity flags */
            if (Speed >= VIDTRICK_Data_p->MainParams.Speed)
            {
                /* change speed has to reset backward fuidity process ! */
#ifdef OLD_BACKWARD_FLUENT_DISPLAY_PROCESS
                VIDTRICK_Data_p->BackwardFluidity.BackwardFluentDisplayProcessIsRunning = FALSE;
#endif /* OLD_BACKWARD_FLUENT_DISPLAY_PROCESS */
                VIDTRICK_Data_p->BackwardFluidity.LastBackwardFieldsSkippedNb = 0;
            }
        }
#endif /* ST_smoothbackward */
    }
    /* ------------- Speed >= 0 ----------------- */
    else
    {
         /* Inits Trick mode params compliant with the asked speed  */
         /* If P Pictures were skipped the new required speed should need P & B pictures to be displayed */
         /* it is better to wait for next I picture to avoid a slow jump between 2 I pictures or to avoid */
         /* to display P & B pictures because the previous speed was a I Only backward speed  */
        /* ----- previous Speed < 0 ------------ */
         if (VIDTRICK_Data_p->MainParams.Speed < 0)
         {
#ifdef ST_smoothbackward
             /* Forward Init */
             /* speed drift event should be sent in order to re-start from where we are. */
             ErrorCode = ForwardModeRequested(TrickModeHandle);
#endif /* ST_smoothbackward */

         }
         /* ----- previous Speed >=0 ------------ */
         else
         {
             /* If must skip all P pictures   */
             /* then do not initialize speed  */
             /* else do speed init            */
             SkipInitSpeed = VIDTRICK_Data_p->Flags.MustSkipAllPPictures;
         }
    }

    if (ErrorCode != ST_NO_ERROR)
    {
        /* Speed change was not possible, restore the context */
        VIDTRICK_Data_p->MainParams.RequiredSpeed = VIDTRICK_Data_p->MainParams.Speed;
    }
    else if (!(SkipInitSpeed))
    {
        SpeedInitialize(TrickModeHandle);
    }

    /* Un lock Access to some parameters of Trick mode data */
    STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);

    /* If previous speed was a positive speed */
#ifdef ST_smoothbackward
#ifdef TRICKMOD_DEBUG
    if (   (Speed < 0)
        && (VIDTRICK_Data_p->MainParams.Speed >= 0)
        && (VIDTRICK_Data_p->Flags.SmoothBackwardIsInitializing))
    {
        TrickModeError();
    }
#endif /* TRICKMOD_DEBUG */
#endif /* ST_smoothbackward */

    return (ErrorCode);

} /* End of VIDTRICK_SetSpeed() function */

/*******************************************************************************
Name        : VIDTRICK_GetBitBufferFreeSize
Description : Get the bit buffer free size.
Parameters  : Trickmode handle, pointer on U32 to fill
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_BAD_PARAMETER if bad parameter
              Caution : no other error allowed ! API STVID_GetBitBufferFreeSize() depends on that.
*******************************************************************************/
ST_ErrorCode_t VIDTRICK_GetBitBufferFreeSize(const VIDTRICK_Handle_t TrickModeHandle, U32 * const FreeSize_p)
{
    VIDTRICK_Data_t         * VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;
    VIDTRICK_PictureInfo_t  * Picture_p;
    U32                       NextPictureIdx;

    if (FreeSize_p == NULL)
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* By default, no free size */
    *FreeSize_p = 0;

    /* Un lock Access to some parameters of Trick mode datas */
    STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p);

    if (VIDTRICK_Data_p->LinearBitBuffers.BufferUnderTransfer_p == NULL)
    {
      /* No buffer under transfer, free size is calculated with the currently displayed picture */
      Picture_p = VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0];

      if (   (! VIDTRICK_Data_p->Flags.SmoothBackwardIsInitializing)
          && (! VIDTRICK_Data_p->Stop.TrickmodIsStopped)
          && (Picture_p != NULL))
      {
        if (PictureMpegTypeIsReference(Picture_p))
        {
          /* If picture is reference, get its next reference to avoid */
          /* to have a fluctuating buffer free size */
          if (   (HasPictureASecondField(Picture_p))
              && (Picture_p->SecondFieldPicture_p != NULL))
          {
            Picture_p = Picture_p->SecondFieldPicture_p;
          }

          if (Picture_p->ForwardOrNextReferencePicture_p != NULL)
          {
            Picture_p = Picture_p->ForwardOrNextReferencePicture_p;
          }
        }

        if (Picture_p->LinearBitBufferStoredInfo_p != VIDTRICK_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p)
        {
          /* Currently displayed picture has switched of linear bit buffer */
          if (CheckIfPictureNotLinkedToLinearBuffer(TrickModeHandle, Picture_p))
          {
            /* Picture is no more linked to old linear bit buffer */
            /* get remaining size from old linear bit buffer to be coherent in the remaining size evolution */
            *FreeSize_p = VIDTRICK_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p->TransferDataMaxSize;
          }
          else
          {
             /* Picture is still linked to old linear bit buffer */
             /* We are in the case I0 P3 B1 B2 P6 B4 B5 P9 B7 B8 */
             /* with I0 P3 B1 B2 P6 B4 into BufferA and B5 P9 B7 B8 into BufferB */
             /* and displayed picture is P6 */
             /* Interpolate free size with next reference (P9) which is in the old linear buffer */
             /* and which has a temporal reference between the 2 previous dsiplayed B pictures */
                 while (   (Picture_p != NULL)
                        && (Picture_p->LinearBitBufferStoredInfo_p != VIDTRICK_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p))
                 {
                    Picture_p = Picture_p->ForwardOrNextReferencePicture_p;
                 }

                 if (Picture_p == NULL)
                 {
#ifdef TRICKMOD_DEBUG
                   /* Should never happen, but we never know ...                   */
                   TrickModeError();
#endif /* TRICKMOD_DEBUG */
                   *FreeSize_p = VIDTRICK_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p->TransferDataMaxSize;
                 }
              }
            }

            if (*FreeSize_p == 0)
            {
              /* Currently displayed picture is still in the same linear bit buffer  */
          /* or Currently displayed picture is linked with old linear bit buffer */

          /* Get Next picture index to get start address of following picture in linear bit buffer */
          NextPictureIdx = Picture_p->IndiceInBuffer + 1;

          if (   (NextPictureIdx < VIDTRICK_BACKWARD_BUFFER_MAX_PICTURES_NB)
              && (NextPictureIdx < Picture_p->LinearBitBufferStoredInfo_p->NbPicturesInside))
          {
            if (   (U32)Picture_p->LinearBitBufferStoredInfo_p->TransferDataMaxEndAddress_p
                >= (U32)Picture_p->LinearBitBufferStoredInfo_p->PictureInfoTableStartAddress_p[ NextPictureIdx ].AssociatedStartCodeAddress_p)
            {
              /* Free size is the difference between address of max transferable data     */
              /* and start address of following picture in linear buffer                  */
                  *FreeSize_p =
                     (U32)Picture_p->LinearBitBufferStoredInfo_p->TransferDataMaxEndAddress_p
                   - (U32)Picture_p->LinearBitBufferStoredInfo_p->PictureInfoTableStartAddress_p[ NextPictureIdx ].AssociatedStartCodeAddress_p;
                }
#ifdef TRICKMOD_DEBUG
                else
                {
                  /* Should never happen, but we never know ...                   */
                  TrickModeError();
                }
#endif /* TRICKMOD_DEBUG */
              }
            }
          }
          else
          {
            /* If SmoothBackward is initializing or trickmod is stopped                     */
        /* => only one linear bit buffer is used, get BufferA TransferDataMaxSize       */
        /* If no picture is available yet                                               */
        /* => get TransferDataMaxSize from a linear buffer (here BufferA)               */
        *FreeSize_p = VIDTRICK_Data_p->LinearBitBuffers.BufferA.TransferDataMaxSize;
      }
    }
    else
    {
        /* Buffer is under transfer, the totality of linear bit buffer is free */
        *FreeSize_p = VIDTRICK_Data_p->LinearBitBuffers.BufferUnderTransfer_p->TransferDataMaxSize;
    }

    /* Un lock Access to some parameters of Trick mode datas */
    STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);

    return (ST_NO_ERROR);
}

/*******************************************************************************
Name        : VIDTRICK_Start
Description : Starts Trickmode
Parameters  :
Assumptions : Useful only if smooth backward feature : Linear bit buffers may have to be allocated if speed < 0
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t VIDTRICK_Start(const VIDTRICK_Handle_t TrickModeHandle)
{
    VIDTRICK_Data_t   * VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

#ifdef TRACE_UART_MAIN
    TraceBuffer(("\n\r  ---- START (%d) ----\n\r\n\r", VIDTRICK_Data_p->MainParams.Speed));
#endif

#ifdef ST_smoothbackward
    /* Lock */
    STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p);

    if (VIDTRICK_Data_p->Stop.TrickmodIsStopped)
    {
        VIDTRICK_Data_p->Stop.TrickmodIsStopped = FALSE;

        if (VIDTRICK_Data_p->MainParams.Speed < 0)
        {
            /* Don't try to restart on the picture that is already on the display */
            VIDTRICK_Data_p->Flags.DoNotTryToRestartOnTheLastDisplayedPicture = TRUE;
            /* Init backward mode since it has been reseted at stop time */
            ErrorCode = BackwardModeInit(TrickModeHandle, FALSE);
        }
        else
        {
            /* Don't try to restart on the picture that is already on the display */
            VIDTRICK_Data_p->NegativeToPositiveSpeedParams.ForwardDisplayQueueInsertionIsEnabled = TRUE;
            /* Forward is already initialised because we need to restart on the first picture */
            VIDTRICK_Data_p->Flags.BackwardToForwardInitializing = FALSE;
        }

        if (ErrorCode != ST_NO_ERROR)
        {
            VIDTRICK_Data_p->Stop.TrickmodIsStopped = TRUE;
        }
    }
    else
    {
        ErrorCode = STVID_ERROR_DECODER_RUNNING;
    }

    /* Unlock */
    STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
#endif /* #ifdef ST_smoothbackward */

    return(ErrorCode);
} /* end of VIDTRICK_Start() function */

/*******************************************************************************
Name        : VIDTRICK_Stop
Description : Stops Trickmode
Parameters  :
Assumptions : should be called if STVID_Stop required when playing smooth backward. Linear bit buffers should
              be deallocated
Limitations :
Returns     : STAVMEM deallocation Errors, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t VIDTRICK_Stop(const VIDTRICK_Handle_t TrickModeHandle, const STVID_Stop_t StopMode, BOOL * const TrickModeWillStopDecode_p, const STVID_Freeze_t * const Freeze_p)
{
    VIDTRICK_Data_t    * VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;
    ST_ErrorCode_t       ErrorCode = ST_NO_ERROR;
#ifdef ST_smoothbackward
    STVID_Freeze_t       EndOfdataForceFreeze;
    BOOL                 PictureToBeStoppedOnIsFound = FALSE;
    U32                  i;
#endif /* #ifdef ST_smoothbackward */

#ifdef TRACE_UART_MAIN
    TraceBuffer(("\n\r  ---- STOP (%d) ----\n\r\n\r", VIDTRICK_Data_p->MainParams.Speed));
#endif

    /* if stop now, disable control speed now */
    if (StopMode == STVID_STOP_NOW)
    {
        ErrorCode = VIDTRICK_DisableSpeedControl(TrickModeHandle);
    }

    if (VIDTRICK_Data_p->MainParams.Speed >= 0)
    {
        /* if speed > 0, Nothing to do. */
        *TrickModeWillStopDecode_p = FALSE;
#ifdef ST_smoothbackward
        VIDTRICK_Data_p->Stop.TrickmodIsStopped = TRUE;
#endif

        return(ST_NO_ERROR);
    }

#ifdef ST_smoothbackward
    if (VIDDEC_GetSmoothBackwardCapability(VIDTRICK_Data_p->DecodeHandle) != ST_NO_ERROR)
    {
        /* if no smooth backward feature, No Trickmod Stop to do */
        *TrickModeWillStopDecode_p = FALSE;
        VIDTRICK_Data_p->Stop.TrickmodIsStopped = TRUE;
        return(ST_NO_ERROR);
    }

    /* Lock trickmode params */
    STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p);

    /* No linear Bit buffer flush must be required */
    VIDTRICK_Data_p->Stop.LinearBitBufferFlushRequirementAvailable = FALSE;

    if (StopMode != STVID_STOP_NOW)
    {
        for (i = 1 ; ((i < VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ) && (!PictureToBeStoppedOnIsFound)); i++)
        {
            if (   ((StopMode == STVID_STOP_WHEN_END_OF_DATA) && (VIDTRICK_Data_p->LastPictureInDisplayQueue_p[i] == VIDTRICK_Data_p->EndOfDataPicture_p))
                || ((StopMode == STVID_STOP_WHEN_NEXT_REFERENCE) && (PictureMpegTypeIsReference(VIDTRICK_Data_p->LastPictureInDisplayQueue_p[i])))
                || ((StopMode == STVID_STOP_WHEN_NEXT_I) && (PictureMpegTypeIsI(VIDTRICK_Data_p->LastPictureInDisplayQueue_p[i]))))
            {
                /* Picture to stop on is found */
                PictureToBeStoppedOnIsFound = TRUE;

                if (StopMode == STVID_STOP_WHEN_END_OF_DATA)
                {
                    /* the picture to be stopped on is on display, but Stop mow can not be used : StvidOuOfPictures evt */
                    /* wouldnt be sent and frame buffers desallocation would be done */
                    VIDDISP_Stop(VIDTRICK_Data_p->DisplayHandle, &(VIDTRICK_Data_p->LastPictureInDisplayQueue_p[i-1]->PictureInfoFromDecode.PictureInfos.VideoParams.PTS), VIDDISP_STOP_ON_PTS);

                    /* if Stop end of data picture is the second picture on the display queue : we have to freeze on the previous one */
                    /* but we may not be able to freeze on the required one because only Stop Now display manages to re-display */
                    /* the right field to freeze if necessary */
                    if (i == 1)
                    {
                        /* to ensure freeze correclty, freeze on current field */
                        EndOfdataForceFreeze.Mode = STVID_FREEZE_MODE_FORCE;
                        EndOfdataForceFreeze.Field = STVID_FREEZE_FIELD_CURRENT;
                        VIDDISP_Freeze(VIDTRICK_Data_p->DisplayHandle, &EndOfdataForceFreeze);
                    }
                    else
                    {
                        /* Freeze display normally */
                        VIDDISP_Freeze(VIDTRICK_Data_p->DisplayHandle, Freeze_p);
                    }
                }
                else
                {
                    /* Stop */
                    VIDDISP_Stop(VIDTRICK_Data_p->DisplayHandle, &(VIDTRICK_Data_p->LastPictureInDisplayQueue_p[i]->PictureInfoFromDecode.PictureInfos.VideoParams.PTS), VIDDISP_STOP_ON_PTS);
                    /* Freeze display normally */
                    VIDDISP_Freeze(VIDTRICK_Data_p->DisplayHandle, Freeze_p);
                }

                /* Stop */
                ErrorCode = StopSmoothBackward(TrickModeHandle, TRUE);
            }
        }
        /* if PictureToBeStoppedOnIsFound is not found, let trickmod manage when it will be inserrted in display queue */
        if (!PictureToBeStoppedOnIsFound)
        {
            /* remember that a Stop different than the stop mode has been required */
            VIDTRICK_Data_p->Stop.TrickmodStopIsPending = TRUE;
            VIDTRICK_Data_p->Stop.SmoothBackwardRequiredStopFreezeParams = *Freeze_p;
            VIDTRICK_Data_p->Stop.StopMode = StopMode;
        }
    }
    else /* Stop Now */
    {
        /* display Stop and freeze are done by STVID_Stop : no need to do it here */
        /* Stop backward */
        ErrorCode = StopSmoothBackward(TrickModeHandle, TRUE);
    }

    /* Un Lock trickmode params */
    STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);

#endif /* ST_smoothbackward */

    /* return */
    *TrickModeWillStopDecode_p = TRUE;
    return(ErrorCode);
} /* end of VIDTRICK_Stop() function */

/*******************************************************************************
Name        : VIDTRICK_Term
Description : Terminate TrickMode, undo all what was done at init
Parameters  : TrickMode handle
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_INVALID_HANDLE if not initialised
*******************************************************************************/
ST_ErrorCode_t VIDTRICK_Term(const VIDTRICK_Handle_t TrickModeHandle)
{
    VIDTRICK_Data_t  * VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;
    ST_ErrorCode_t     ErrorCode = ST_NO_ERROR;

    if (VIDTRICK_Data_p->ValidityCheck != VIDTRICK_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef ST_smoothbackward
    SAFE_MEMORY_DEALLOCATE(VIDTRICK_Data_p->LastPictureInDisplayQueue_p, VIDTRICK_Data_p->CPUPartition_p, sizeof(VIDTRICK_PictureInfo_t*) * (VIDTRICK_Data_p->NbFieldsToBeInDisplayQueue + 2));
    if (VIDDEC_GetSmoothBackwardCapability(VIDTRICK_Data_p->DecodeHandle) == ST_NO_ERROR)
    {
        if (VIDTRICK_Data_p->LinearBitBuffers.BufferA.PictureInfoTableAllocationAddress_p != NULL)
        {
            SAFE_MEMORY_DEALLOCATE(VIDTRICK_Data_p->LinearBitBuffers.BufferA.PictureInfoTableAllocationAddress_p,
                                   VIDTRICK_Data_p->CPUPartition_p,
                                   VIDTRICK_BACKWARD_BUFFER_MAX_PICTURES_NB * sizeof(VIDTRICK_PictureInfo_t));
        }
        if (VIDTRICK_Data_p->LinearBitBuffers.BufferB.PictureInfoTableAllocationAddress_p != NULL)
        {
            SAFE_MEMORY_DEALLOCATE(VIDTRICK_Data_p->LinearBitBuffers.BufferB.PictureInfoTableAllocationAddress_p,
                                   VIDTRICK_Data_p->CPUPartition_p,
                                   VIDTRICK_BACKWARD_BUFFER_MAX_PICTURES_NB * sizeof(VIDTRICK_PictureInfo_t));
        }
    }
#endif /* ST_smoothbackward */

    /* Stop Trick Mode Task */
    StopTrickModeTask(TrickModeHandle);

    /* Close instances opened at init */
    /* Un-subscribe and un-register events: this is done automatically by STEVT_Close() */
    ErrorCode = STEVT_Close(VIDTRICK_Data_p->EventsHandle);

    /* semaphore delete */
    if (STOS_MutexDelete(VIDTRICK_Data_p->TrickModeParamsMutex_p) != STOS_SUCCESS)
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("Impossible to delete TrickModeParamsMutex_p!!!\n\r"));
#endif /* TRICKMOD_DEBUG */
    }
    VIDTRICK_Data_p->TrickModeParamsMutex_p = NULL;         /* For safety only */

    semaphore_delete(VIDTRICK_Data_p->WakeTrickModeTaskUp_p);
    VIDTRICK_Data_p->WakeTrickModeTaskUp_p = NULL;          /* For safety only */

    /* De-validate structure */
    VIDTRICK_Data_p->ValidityCheck = 0; /* not VIDTRICK_VALID_HANDLE */

    /* De-allocate last: data inside cannot be used after ! */
    SAFE_MEMORY_DEALLOCATE(VIDTRICK_Data_p, VIDTRICK_Data_p->CPUPartition_p, sizeof(VIDTRICK_Data_t));

    return(ErrorCode);

} /* end of VIDTRICK_Term() */

#ifdef ST_smoothbackward
/*******************************************************************************
Name        : VIDTRICK_DataInjectionCompleted
Description : Function which should be called by the data feeder to signal the end of a required transfer by data underflow evt
Parameters  : TrickModeHandle  DataInjectionCompletedParams_p
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDTRICK_DataInjectionCompleted(const VIDTRICK_Handle_t TrickModeHandle, const STVID_DataInjectionCompletedParams_t * const DataInjectionCompletedParams_p)
{
    VIDTRICK_Data_t                *  VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    VIDTRICK_LinearBitBuffer_t     *  LinearBitBufferJustTransfered_p;

#if defined(TRACE_UART_DISPLAY_SKIP) || defined(TRACE_UART_EXT_TEMP_REF)
    TraceBuffer(("DataInjectionCompleted\n\r"));
#endif

    /* If speed > 0, decode sends underflow events by itself, and trick mode does not need */
    /* to know when the injection is completed */
    if (VIDTRICK_Data_p->MainParams.Speed >= 0)
    {
        UpdateUnderflowEventParams(TrickModeHandle);
        return(ST_NO_ERROR);
    }

    /* if linear bit buffer flushes have been forbidden by a pending stop       */
    /* or because of a stop an eventual previous reponse has to be not computed */
    if (   (!VIDTRICK_Data_p->Stop.LinearBitBufferFlushRequirementAvailable)
        || (VIDTRICK_Data_p->Stop.TrickmodIsStopped))
    {
         return(ST_NO_ERROR);
    }

    /* if too many errors, the decision of fully re-initialize smooth may has been done waiting a response */
    /* of a previous linear bit buffer flush request. Then do not compute it and reflush buf A. */
    if (VIDTRICK_Data_p->ErrorRecovery.ReInitSmoothIsPending)
    {
        VIDTRICK_Data_p->ErrorRecovery.ReInitSmoothIsPending = FALSE;
        AskForLinearBitBufferFlush(TrickModeHandle, &(VIDTRICK_Data_p->LinearBitBuffers.BufferA), VIDTRICK_FLUSH_BBFSIZE_BACKWARD_JUMP);
        return (ST_NO_ERROR);
    }

    /* lock Access to some parameters of Trick mode datas.  */
     STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p);

    /* Find the buffer which just has been transfered */
    LinearBitBufferJustTransfered_p = VIDTRICK_Data_p->LinearBitBuffers.BufferUnderTransfer_p;
    if (LinearBitBufferJustTransfered_p == NULL)
    {
        if (! VIDTRICK_Data_p->Flags.SmoothBackwardIsInitializing)
        {
#ifdef TRICKMOD_DEBUG
            TrickModeError();
            TraceBuffer(("Data inj completed but no buffer under transfer\n\r"));
#endif /* #ifdef TRICKMOD_DEBUG */
            STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
            return (ST_ERROR_BAD_PARAMETER);
        }

        /* if LinearBitBufferJustTransfered_p == NULL it means that the current data injection is an injection */
        /* required by decode by itself and that smooth backward is under initializing. No more UNDERFLOW events */
        /* should be required by decode. we can ask buffer A to be flushed */
        AskForLinearBitBufferFlush(TrickModeHandle, &(VIDTRICK_Data_p->LinearBitBuffers.BufferA), VIDTRICK_FLUSH_BBFSIZE_BACKWARD_JUMP);
        STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
        return (ST_NO_ERROR);
     }

     /* Add a picture start code just above the end address of Transfered Data buffer */
     /* if PutPictureStartCodeAndComputeValidDataBuffer returns FALSE, there is a problem */
     /* reading size of just flushed buffer */
     if (! PutPictureStartCodeAndComputeValidDataBuffer( TrickModeHandle,
                                                         LinearBitBufferJustTransfered_p,
                                                         DataInjectionCompletedParams_p->TransferRelatedToPrevious))
     {
         STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
         return(ST_ERROR_BAD_PARAMETER);
     }

     /* Initialize the number of picture before parsing.                                                   */
     /* and the picture table start address since it could have been changed by AffectReferencePictures(). */
     LinearBitBufferJustTransfered_p->NbPicturesInside = 0;
     LinearBitBufferJustTransfered_p->PictureInfoTableStartAddress_p = LinearBitBufferJustTransfered_p->PictureInfoTableAllocationAddress_p;

     LinearBitBufferJustTransfered_p->LastDisplayablePicture_p = NULL;

#ifdef TRICKMOD_DEBUG
     if (VIDTRICK_Data_p->Flags.SmoothBackwardIsInitializing)
     {
          /* for the moment smooth backward is initialized sequencially : */
          /* Transfer A , Parse A , Transfer B and Parse B ==> A can not be under parsing */
          if (VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p != NULL)
          {
              TrickModeError();
          }
     }
#endif /* #ifdef TRICKMOD_DEBUG */

     /* launch parsing ok just transfered buffer */
     /* Start Code detector has to be re-programmed */
     VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p = LinearBitBufferJustTransfered_p;
     NewStartCodeSearchIsRequired(TrickModeHandle, VIDDEC_START_CODE_SEARCH_MODE_FROM_ADDRESS, TRUE, LinearBitBufferJustTransfered_p->WholeBufferStartAddress_p);

     /* if the volontary overlap with previous buffer */
     /* TraceBuffer(("TransferRelatedToPrevious = %d\r\n", DataInjectionCompletedParams_p->TransferRelatedToPrevious)); */

     /* VIDTRICK_Data_p->Discontinuity.DiscontinuityWithPreviousBuffer is TRUE, it means that is hes been set to TRUE */
     /* by AffectTheBufferSpacing because the required speed needs no overlap on HDD. Thus, Do not mind of TransferRelatedToPrevious*/
     if (!LinearBitBufferJustTransfered_p->DiscontinuityWithPreviousBuffer)
     {
         if (!DataInjectionCompletedParams_p->TransferRelatedToPrevious)
         {
              /* TraceBuffer(("No Relation with previous LBBF. looping?\r\n")); */
              LinearBitBufferJustTransfered_p->DiscontinuityWithPreviousBuffer = TRUE;
         }
     }

     /* No buffer under transfer now */
     VIDTRICK_Data_p->LinearBitBuffers.BufferUnderTransfer_p = NULL;

    /* Unlock Access to some parameters of Trick mode datas */
    STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);

    return(ST_NO_ERROR);
} /* end of VIDTRICK_DataInjectionCompleted() function */
#endif  /* ST_smoothbackward */


/* Private Functions -------------------------------------------------------- */

#ifdef ST_smoothbackward
/*******************************************************************************
Name        : FreeUnnecessaryFrameBuffers
Description : Clean all the Frame Buffers containing unecessary forward references. Will be called when skiping.
              in fast backward mode, a lot of references pictures have only been decoded in order to decode the
              chosen pictures to be displayed. Once a chosen picture to be dissplayed is decoded, all its
              references pictures which have been decoded without having being given to the display queue,
              have to have their lock Status set to UNLOCKED.
                      ex :   P3 - P5 - P7 - P9   P9 & P3 given to display queue. P5 & P7 only decoded to have P9 decoded
                             ^              ^    FreeUnnecessaryFrameBuffers unlocks P5 and P7
Parameters  : Video TrickMode handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
static void FreeUnnecessaryFrameBuffers(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t * const PictureToBeDisplayed_p, VIDTRICK_PictureInfo_t * const FollowingPicture_p)
{
    VIDTRICK_PictureInfo_t * Picture_p;
    VIDTRICK_PictureInfo_t * TmpPicture_p;
    VIDTRICK_PictureInfo_t * JustFoundPictureInfo_p;
    BOOL   FollowingPictureReached = FALSE;
    BOOL   FoundPictureIsOK;

    if ((PictureToBeDisplayed_p == NULL) || (FollowingPicture_p == NULL))
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("null pointers FreeUnnecessaryFrameBuffers \r\n"));
#endif
        return;
    }

#ifdef TRICKMOD_DEBUG
    /* just for test : two following picture are not supposed to have the same Extended Temp ref if next to be inserted is a frame */
    if (   (FollowingPicture_p->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
        && (FollowingPicture_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference == PictureToBeDisplayed_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference))
    {
        TrickModeError();
    }
#endif

    /* Set local variables for a better reading. */
    FoundPictureIsOK = SearchForNormalRevSpeedPreviousPicToDisplay(TrickModeHandle, PictureToBeDisplayed_p, &Picture_p);
    while (   (FoundPictureIsOK)
           && (! CheckIfSecondFieldIsAvailable(Picture_p)))
    {
        /* Second field is not available, search next previous picture */
        TmpPicture_p = Picture_p;
        FoundPictureIsOK = SearchForNormalRevSpeedPreviousPicToDisplay( TrickModeHandle,
                                                                        TmpPicture_p,
                                                                        &Picture_p);
    }

    if (! FoundPictureIsOK)
    {
#ifdef TRICKMOD_DEBUG
        /* TrickModeError(); */
        TraceBuffer(("SearchFromPl should not return FALSE !\r\n"));
#endif /* TRICKMOD_DEBUG */
        return;
    }

    if (Picture_p != FollowingPicture_p)
    {
#ifdef TRICKMOD_DEBUG
        if (PictureMpegTypeIsB(FollowingPicture_p))
        {
            TrickModeError();
            TraceBuffer(("Error in FreeUnnecessaryFrameBuffers \r\n"));
        }
#endif /* TRICKMOD_DEBUG */

        /* there are at least one picture between PictureToBeDisplayed_p and FollowingPicture_p */
        /* means that all Bs between PictureToBeDisplayed_p and its FollowingPicture_p */
        /* are skipped ==> PictureToBeDisplayed_p is unuseful to decode any B pictures */
        /* Unlock it partially */
        if (PictureMpegTypeIsReference(PictureToBeDisplayed_p))
        {
#ifdef TRICKMOD_DEBUG
            if (IsPictureSecondField(PictureToBeDisplayed_p))
            {
                TrickModeError();
            }
#endif /* TRICKMOD_DEBUG */

            /* FreeUnnecessaryFrameBuffers can be called several times for the same both following pictures in the table. */
            /* then, do not unlock two times partially first picture, otherwise PictureToBeDisplayed_p could be definitively unlocked */
            /* if (!PictureToBeDisplayed_p->FirstUnlockedChangeStatusHasBeenRequired) */
            /* { */
                /* Unlock  */
                /* UnlockPicture(TrickModeHandle, PictureToBeDisplayed_p, FALSE); */
            /* } */
        }
        else
        /* just inserted picture is B and all previous B pictures between this just insetred picture */
        /* are skipped : unlock partially forward ==> if previous in the stream is B : means than this B */
        /* has been skipped to reach the required speed : Unlock forward. If not a B Forward has already been */
        /* UNLOCKED when this B PictureToBeDisplayed_p has been decoded */
        {
            if (!IsPictureSecondField(PictureToBeDisplayed_p))
            {
                if (PictureMpegTypeIsB(PictureToBeDisplayed_p->PreviousInStreamPicture_p))
                {
                    /* Unlock  */
                    /* TraceBuffer(("FreeUn - Unlock %d\r\n, ", */
                            /* PictureToBeDisplayed_p->ForwardOrNextReferencePicture_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference)); */
                    UnlockPicture(TrickModeHandle, PictureToBeDisplayed_p->ForwardOrNextReferencePicture_p, FALSE, FALSE, TRUE);
                }
            }
        }

        /* All references pictures between just inserted picture and next to be inserted have to be unlocked */
        while (! FollowingPictureReached)
        {
            /* if picture to skip is a reference */
            if (PictureMpegTypeIsReference(Picture_p))
            {
                if (!IsPictureSecondField(Picture_p))
                {
                     UnlockPicture(TrickModeHandle, Picture_p, TRUE, FALSE, FALSE);
                }
            }

            FoundPictureIsOK = SearchForNormalRevSpeedPreviousPicToDisplay(TrickModeHandle, Picture_p, &JustFoundPictureInfo_p);
            if (! FoundPictureIsOK)
            {
#ifdef TRICKMOD_DEBUG
                TrickModeError();
                TraceBuffer((" should not return FALSE !\r\n"));
#endif /* TRICKMOD_DEBUG */

                return;
            }

            if (CheckIfSecondFieldIsAvailable(JustFoundPictureInfo_p))
            {
                /* Second field is available, check if equal to following picture */
                if (JustFoundPictureInfo_p == FollowingPicture_p)
                {
                    FollowingPictureReached = TRUE;
                }
            }

            /* Search for next previous picture */
            Picture_p = JustFoundPictureInfo_p;
        } /* while (! FollowingPictureReached) */
    } /* (Picture_p != FollowingPicture_p) */

#ifdef TRICKMOD_DEBUG
    /* Picture_p = PictureToBeDisplayed_p; */
    /* FoundPictureIsOK = SearchForNormalRevSpeedPreviousPicToDisplay(TrickModeHandle, PictureToBeDisplayed_p, &Picture_p); */

    /* while (Picture_p != FollowingPicture_p) */
    /* { */
        /* if (Picture_p->PictureBuffer_p->Buffers.PictureLockStatus !=  VIDBUFF_PICTURE_LOCK_STATUS_UNLOCKED) */
        /* { */
            /* TrickModeError(); */
        /* } */
        /* Picture_p = Picture_p->BackwardOrLastBeforeIReferencePicture_p; */
    /* } */
#endif /* TRICKMOD_DEBUG */

} /* end of FreeUnnecessaryFrameBuffers(FreeUnnecessaryFrameBuffers) function */
#endif /* ST_smoothbackward */

#ifdef ST_smoothbackward
/*******************************************************************************
Name        : BackwardModeInit
Description : init Backward mode, when switching from positive to negative speed, or
              when smooth backward has to be re-initialzed.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t BackwardModeInit(const VIDTRICK_Handle_t TrickModeHandle, const BOOL ReInit)
{
    VIDTRICK_Data_t   * VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    VIDDEC_ControlMode_t    ControlMode;

#ifdef TRICKMOD_DEBUG
    TraceBuffer(("---BackwardModeInit (%d)---\r\n", ReInit));
#endif

    ErrorCode = VIDDEC_GetSmoothBackwardCapability(VIDTRICK_Data_p->DecodeHandle);
    /* Useless to ask for the smooth backward feature if the hardware doesn't support it. */
    if (ErrorCode == ST_NO_ERROR)
    {
        /* Reinit the table that will contain all the inserted pictures in backward playback */
        memset(VIDTRICK_Data_p->LastPictureInDisplayQueue_p, 0, sizeof(VIDTRICK_PictureInfo_t*) * (VIDTRICK_Data_p->NbFieldsToBeInDisplayQueue + 2));
        /* Smooth Backward enters a initializing phase */
        VIDTRICK_Data_p->Flags.SmoothBackwardIsInitializing = TRUE;
        VIDTRICK_Data_p->Flags.BackwardToForwardInitializing = FALSE;
        VIDTRICK_Data_p->ErrorRecovery.ReInitSmoothIsPending = FALSE;

#ifdef TRACE_UART_MAIN
        if (ReInit)
        {
            TraceBuffer(("--------- Re-InitSmooth -------------- !!!\r\n"));
        }
#endif

        /* Control Mode should already be in manual mode */
        VIDDEC_GetControlMode(VIDTRICK_Data_p->DecodeHandle, &ControlMode);

        if (ControlMode != VIDDEC_MANUAL_CONTROL_MODE)
        {
#ifdef TRICKMOD_DEBUG
            TrickModeError();
            TraceBuffer(("Error! Not in Manual mode!\r\n"));
#endif
        }

        if (!ReInit)
        {
            /* Reset reference pictures and free the not displayed pictures */
            ResetReferencesAndFreeNotDisplayedPictures(TrickModeHandle);
        }

#ifdef TRICKMOD_DEBUG
        /* Dump the frame buffers status in order to check that no FB remains locked */
        TraceBuffer(("\r\nFB status after InitSmooth:\r\n"));
        VIDBUFF_PrintPictureBuffersStatus(VIDTRICK_Data_p->BufferManagerHandle);
#endif

        /* If trickmode is stopped, there is no need to allocate the 2 linear bitbuffer and to
         * send an underflow event */
        if((VIDTRICK_Data_p->Stop.TrickmodIsStopped)||(VIDTRICK_Data_p->Stop.TrickmodStopIsPending))
        {
            /* Init Structure */
            SmoothBackwardInitStructure(TrickModeHandle);
        }
        else /* Trickmode is not stopped */
        {
            if (ReInit)
            {
                if (VIDTRICK_Data_p->LinearBitBuffers.BufferUnderTransfer_p != NULL)
                {
                    /* there is a running data injection. wait for the end of this one. */
                    VIDTRICK_Data_p->ErrorRecovery.ReInitSmoothIsPending = TRUE;
                }
            }

            /* Init Structure */
            SmoothBackwardInitStructure(TrickModeHandle);

            if (!ReInit)
            {
                /* Buffer A can be flushed only if an injection required by decode is not running !!! */
                if (!(VIDDEC_IsThereARunningDataInjection(VIDTRICK_Data_p->DecodeHandle)))
                {
                    /* TraceBuffer(("Ask for bufferA to be flushed in live.\r\n")); */
                    AskForLinearBitBufferFlush(TrickModeHandle, &(VIDTRICK_Data_p->LinearBitBuffers.BufferA), VIDTRICK_FLUSH_BBFSIZE_BACKWARD_JUMP);
                }
            }
            else if (!VIDTRICK_Data_p->ErrorRecovery.ReInitSmoothIsPending)
            {
                AskForLinearBitBufferFlush(TrickModeHandle, &(VIDTRICK_Data_p->LinearBitBuffers.BufferA), VIDTRICK_FLUSH_BBFSIZE_BACKWARD_JUMP);
            }
        }
    } /* end of if
       * VIDDEC_GetSmoothBackwardCapability(VIDTRICK_Data_p->DecodeHandle) */
    VIDTRICK_Data_p->SpeedAccuracy.PreviousBitBufferUsedForDisplay = NULL;
    VIDTRICK_Data_p->SpeedAccuracy.NBFieldsLate = 0;

    return(ErrorCode);
} /* end of BackwardModeInit() function */

/*******************************************************************************
Name        : BackwardWarmReset
Description : Not yet.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t BackwardWarmReset(const VIDTRICK_Handle_t TrickModeHandle)
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    VIDTRICK_Data_t *   VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;

    VIDTRICK_Data_p->BackwardWarmReset.OnGoing = TRUE;

#ifdef TRICKMOD_DEBUG
    TraceBuffer(("BackwardWarmReset\n\r"));
#endif

    if (HasPictureASecondField(VIDTRICK_Data_p->PictureUnderDecoding_p))
    {
        VIDTRICK_Data_p->BackwardWarmReset.MustDecodeSecondField= TRUE;
        VIDTRICK_Data_p->BackwardWarmReset.FailingDecodePictureStructure= VIDTRICK_Data_p->PictureUnderDecoding_p->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure;
    }
    else
    {
        VIDTRICK_Data_p->BackwardWarmReset.MustDecodeSecondField= FALSE;
    }

    BackwardModeInit(TrickModeHandle, TRUE);

    return (ErrorCode);
} /* end of BackwardWarmReset() function */

#endif /* ST_smoothbackward */


/*******************************************************************************
Name        : GetBitRateValue
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
static ST_ErrorCode_t GetBitRateValue(const VIDTRICK_Handle_t TrickModeHandle, U32 * const BitRateValue_p)
{
    VIDTRICK_Data_t  * VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    ST_ErrorCode_t     ErrorCode;

    /* Get Bit Rate Value */
    ErrorCode = VIDDEC_GetBitRateValue(VIDTRICK_Data_p->DecodeHandle, BitRateValue_p);

    if (ErrorCode != ST_NO_ERROR)
    {
#ifdef TRACE_UART_MAIN
        TraceBuffer(("VIDDEC_GetBitRateValue failed value assumed : %d\n\r", VIDTRICK_Data_p->MainParams.BitRateValue));
#endif
        *BitRateValue_p = VIDTRICK_Data_p->MainParams.BitRateValue;
        return (ErrorCode);
    }

    VIDTRICK_Data_p->MainParams.BitRateValue = *BitRateValue_p;
    return (ST_NO_ERROR);
} /* end of GetBitRateValue() function */


/*******************************************************************************
Name        : NewPictureSkippedByDecodeWhithoutTrickmodeRequest
Description : Function to subscribe VIDDEC_NEW_PICTURE_SKIPPED_NOT_REQUESTED_EVT.
              Removes the number of fields from NbFieldsIfPosToSkipIfNegToRepeat
              when decode Error recovery skips one picture.
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void NewPictureSkippedByDecodeWhithoutTrickmodeRequest(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                      STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p)
{
    VIDTRICK_Handle_t  		            TrickModeHandle;
    VIDTRICK_Data_t *                   VIDTRICK_Data_p;
    U32                                 NbFieldsShouldBeDisplayed;
    STVID_PictureInfos_t *              PictureInfos_p;
    VIDDEC_NewPictureDecodeOrSkip_t *   NewPictureDecodeOrSkipEventData_p;
#ifdef ST_smoothbackward
    BOOL                                WaitingForFirstPictureToBeDisplayed;
#endif /* ST_smoothbackward */

    TrickModeHandle  = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDTRICK_Handle_t, SubscriberData_p);
    VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    NewPictureDecodeOrSkipEventData_p = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDDEC_NewPictureDecodeOrSkip_t *, EventData_p);
    PictureInfos_p = NewPictureDecodeOrSkipEventData_p->PictureInfos_p;
    NbFieldsShouldBeDisplayed = NewPictureDecodeOrSkipEventData_p->NbFieldsForDisplay;
#ifdef ST_smoothbackward
    WaitingForFirstPictureToBeDisplayed = FALSE;
#endif /* ST_smoothbackward */

    /* unused parameters */
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    if (VIDTRICK_Data_p->MainParams.Speed == 100)
    {
        return;
    }
#ifdef ST_smoothbackward
    if (!VIDTRICK_Data_p->NegativeToPositiveSpeedParams.ForwardDisplayQueueInsertionIsEnabled)
    {
        /* in order to re start display from the same picture, in case of just set positive speed, */
        /* the picture to be decoded may have to be skipped */
        WaitingForFirstPictureToBeDisplayed = !( CheckIfDisplayHasToBeEnabled(
                                                            TrickModeHandle,
                                                            PictureInfos_p,
                                                            NewPictureDecodeOrSkipEventData_p->TemporalReference,
                                                            NewPictureDecodeOrSkipEventData_p->VbvDelay) );
    }
#endif /* ST_smoothbackward */

    if ((VIDTRICK_Data_p->MainParams.Speed != 100) && (VIDTRICK_Data_p->Flags.SpeedControlEnabled))
    {
        /* Lock mutex protecting Trick mode private params  */
        STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p);

        /* Test the current skip/repeat counter polarity.       */
        if (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat > 0)
        {
#ifdef ST_smoothbackward
            if (!WaitingForFirstPictureToBeDisplayed)
#endif /* ST_smoothbackward */
            {
                /* OK, it's positive. The skip action is then allowed, try it.  */
                /* remove it from MainParams.NbFieldsIfPosToSkipIfNegToRepeat   */
                VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat -= NbFieldsShouldBeDisplayed;

                /* except if MainParams.NbFieldsIfPosToSkipIfNegToRepeat turns to a negative value */
                /* To avoid to repeat a field is speed > 0, let it to 0 */
                if (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat < 0)
                {
                    VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat = 0;
                }
            }

            /* new picture treated */
            VIDTRICK_Data_p->NbTreatedPicturesSinceLastDecodedI ++;
        }

        /* Unlock mutex protecting Trick mode private params */
        STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
    }

#ifdef TRACE_UART_DISPLAY_SKIP
    TraceBuffer(("c:%d (Dec)NbForDisp: %d \r\n",
                     VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat,
                     NbFieldsShouldBeDisplayed));
#endif
} /* end of NewPictureSkippedByDecodeWhithoutTrickmodeRequest() function */

/*******************************************************************************
Name        : NewPictureSkippedByDisplayWhithoutTrickmodeRequest
Description : Function to subscribe VIDDISP_NEW_PICTURE_SKIPPED_NOT_REQUESTED_EVT.
              Removes the number of fields from NbFieldsIfPosToSkipIfNegToRepeat
              when display skips one too late picture because.
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void NewPictureSkippedByDisplayWhithoutTrickmodeRequest(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                      STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p)
{
    VIDTRICK_Handle_t        	TrickModeHandle;
    VIDTRICK_Data_t *           VIDTRICK_Data_p;
    U32                         NbFieldsShouldBeDisplayed;
    VIDDISP_NewPictureSkippedWithNoRequest_t *   VIDDISP_NewPictureSkippedWithNoRequest_p;

    TrickModeHandle  = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDTRICK_Handle_t, SubscriberData_p);
    VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    VIDDISP_NewPictureSkippedWithNoRequest_p = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDDISP_NewPictureSkippedWithNoRequest_t *, EventData_p);

    /* Get the number of field skipped. */
    NbFieldsShouldBeDisplayed = VIDDISP_NewPictureSkippedWithNoRequest_p->NbFieldsSkipped;

	/* unused parameters */
	UNUSED_PARAMETER(Reason);
	UNUSED_PARAMETER(RegistrantName);
	UNUSED_PARAMETER(Event);

    /*  */
    if ((VIDTRICK_Data_p->MainParams.Speed != 100) && (VIDTRICK_Data_p->Flags.SpeedControlEnabled))
    {
        /* Lock mutex protecting Trick mode private params */
        STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p);

        /* Test the current skip/repeat counter polarity.       */
        if (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat > 0)
        {
            /* OK, it's positive. The skip action is then allowed, try it.  */

            /* remove it from MainParams.NbFieldsIfPosToSkipIfNegToRepeat   */
            VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat -= NbFieldsShouldBeDisplayed;

            /* except if MainParams.NbFieldsIfPosToSkipIfNegToRepeat turns to a negative value */
            /* To avoid to repeat a field is speed > 0, let it to 0 */
            if (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat < 0)
            {
                VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat = 0;
            }
        }

        /* Unlock mutex protecting Trick mode private params */
        STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
    }

#ifdef TRACE_UART_DISPLAY_SKIP
    TraceBuffer(("c:%d (Dis)NbForDisp: %d \r\n",
                      VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat,
                      NbFieldsShouldBeDisplayed));
#endif
} /* end of NewPictureSkippedByDisplayWhithoutTrickmodeRequest() function */

/*******************************************************************************
Name        : ReadyToDecodeNewPictureTrickMode
Description : Function to subscribe the VIDDEC_READY_TO_DECODE_NEW_PICTURE_EVT event
Parameters  : PictureInfos_p, temporal_reference, vbv_delay.
              According to the NbOfFieldsToSkipOrToRepeat counter value, the decision-making of
              skipping or decoding it is made in this call-back.
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void ReadyToDecodeNewPictureTrickMode(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p)
{
    VIDTRICK_Handle_t       	  TrickModeHandle  = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDTRICK_Handle_t, SubscriberData_p);
    VIDDEC_NewPictureDecodeOrSkip_t *   NewPictureDecodeOrSkipEventData_p = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDDEC_NewPictureDecodeOrSkip_t *, EventData_p);
    STVID_PictureInfos_t *        PictureInfos_p = NewPictureDecodeOrSkipEventData_p->PictureInfos_p;
    U32                           NbFieldsShouldBeDisplayed = NewPictureDecodeOrSkipEventData_p->NbFieldsForDisplay;
    STVID_SpeedDriftThreshold_t   SpeedDriftThresholdData;
    VIDTRICK_Data_t *             VIDTRICK_Data_p;
    ST_ErrorCode_t                ErrorCode = ST_NO_ERROR;
    U32                           BitRateValue;
    S32                           NbDriftedFields;
    BOOL                          SkipB, SkipP;
#ifdef ST_smoothbackward
    BOOL                          DecodeButNotDisplay = FALSE;
    BOOL                          WaitingForFirstPictureToBeDisplayed = FALSE;
    BOOL                          PictureHasToBeDisplayed = TRUE;
#endif

    VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;

	/* unused parameters */
	UNUSED_PARAMETER(Reason);
	UNUSED_PARAMETER(RegistrantName);
	UNUSED_PARAMETER(Event);

    if (PictureInfos_p == NULL)
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
#endif
        return;
    }
    if (VIDTRICK_Data_p->MainParams.Speed == 100)
    {
        return;
    }
#ifdef ST_smoothbackward
    if (!VIDTRICK_Data_p->NegativeToPositiveSpeedParams.ForwardDisplayQueueInsertionIsEnabled)
    {
        WaitingForFirstPictureToBeDisplayed = !( CheckIfDisplayHasToBeEnabled(
                                                       TrickModeHandle,
                                                       PictureInfos_p,
                                                       NewPictureDecodeOrSkipEventData_p->TemporalReference,
                                                       NewPictureDecodeOrSkipEventData_p->VbvDelay) );
    }
    else
    {
        PictureHasToBeDisplayed = CheckIfPictureCanBeDisplayed(TrickModeHandle, PictureInfos_p);
    }
#endif
    if(VIDTRICK_Data_p->MainParams.LastSequenceInfo.FrameRate == 0)
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("Frame rate in last sequence info should not be 0 \n\r"));
#endif
        VIDTRICK_Data_p->MainParams.LastSequenceInfo.FrameRate = 30000; /* set default value*/
    }
    if (VIDTRICK_Data_p->MainParams.FrameRateValue == 0)
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("BFrame rate should not be 0\n\r"));
#endif
        VIDTRICK_Data_p->MainParams.FrameRateValue = 30000;
    }
    /* If video is started and not stopped */
    if (   (VIDTRICK_Data_p->Flags.SpeedControlEnabled)
        || (VIDTRICK_Data_p->Flags.SecondFieldHasToBeSkipped)
#ifdef ST_smoothbackward
        || (WaitingForFirstPictureToBeDisplayed)
        || (!PictureHasToBeDisplayed)
#endif /* ST_smoothbackward */
        )
    {
        /* Lock mutex protecting Trick mode private params */
        STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p);

        /* If First field has been skipped or decoded, 2nd field has to be skipped or decoded too */
        if (VIDTRICK_Data_p->Flags.WaitingForASecondField)
        {
            if (VIDTRICK_Data_p->Flags.SecondFieldHasToBeSkipped)
            {
                VIDDEC_SetNextDecodeAction(VIDTRICK_Data_p->DecodeHandle, VIDDEC_DECODE_ACTION_SKIP);
#ifdef ST_smoothbackward
                if (!WaitingForFirstPictureToBeDisplayed)
#endif /* ST_smoothbackward */
                {
                    VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat -= NbFieldsShouldBeDisplayed;
                }
                VIDTRICK_Data_p->Flags.SecondFieldHasToBeSkipped = FALSE;

#ifdef TRACE_UART_DISPLAY_SKIP
                TraceBuffer(("c:%d NbField=%d 2ndField:--\r\n",
                                 VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat,
                                 NbFieldsShouldBeDisplayed));
#endif
            }

#ifdef ST_smoothbackward
            else if (WaitingForFirstPictureToBeDisplayed)
            {
                VIDDEC_SetNextDecodeAction(VIDTRICK_Data_p->DecodeHandle, VIDDEC_DECODE_ACTION_NO_DISPLAY);
            }
#endif
            /* This picture is the waited second field */
            VIDTRICK_Data_p->Flags.WaitingForASecondField = FALSE;
        }
        else
        {
            VIDTRICK_Data_p->MainParams.FrameRateValue = PictureInfos_p->VideoParams.FrameRate;
            if (IsStvidPictureInfosExpectingSecondField(PictureInfos_p))
            {
                VIDTRICK_Data_p->Flags.WaitingForASecondField = TRUE;
            }

#ifdef TRACE_UART_DISPLAY_SKIP
            TrickModeTraces( VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat,
                             VIDTRICK_Data_p->Flags.MustSkipAllPPictures);
#endif

            /*------------------------------------------------------------------------*/
            /*- Forward --------------------------------------------------------------*/
            /*------------------------------------------------------------------------*/
            if (VIDTRICK_Data_p->MainParams.Speed >= 0)
            {
                VIDTRICK_Data_p->NbTreatedPicturesSinceLastDecodedI ++;
#ifdef TRACE_UART_DISPLAY_SKIP
                TraceBuffer(("-%d", VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat));
#endif

                /*------------------------------------------------------------------------*/
                /*- B Frame --------------------------------------------------------------*/
                /*------------------------------------------------------------------------*/
                if (PictureInfos_p->VideoParams.MPEGFrame == STVID_MPEG_FRAME_B)
                {
#ifdef ST_smoothbackward
                    SkipB = FALSE;
#else
                    SkipB = TRUE;
#endif /* ST_smoothbackward */

#ifdef ST_smoothbackward
                    if ((WaitingForFirstPictureToBeDisplayed)||(!PictureHasToBeDisplayed))
                    {
                        SkipB = TRUE;
                    }
                    else
#endif /* ST_smoothbackward */
                    {
                        /* Second level reached : Skip all Bs and set display in AS_AVAILABLE mode for fluidity  */
                        if (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat > VIDTRICK_LEVEL_SKIP_ALL_B_AND_DISP_P)
                        {
                            SkipB = TRUE;
                            if (VIDTRICK_Data_p->DisplayParams.DisplayStart != VIDDISP_DISPLAY_START_AS_AVAILABLE)
                            {
                                VIDTRICK_Data_p->DisplayParams.DisplayStart = VIDDISP_DISPLAY_START_AS_AVAILABLE;
                                /* Notify event so that API level can choose to change display settings or not. */
                                STEVT_Notify(VIDTRICK_Data_p->EventsHandle, VIDTRICK_Data_p->RegisteredEventsID[VIDTRICK_DISPLAY_PARAMS_CHANGE_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(VIDTRICK_Data_p->DisplayParams));
                            }
                        }
                        else /* Nb Fields <= VIDTRICK_LEVEL_SKIP_ALL_B_AND_DISP_P */
                        {
                            /* First level reached : Skip current B */
                            if (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat > VIDTRICK_LEVEL_SKIP_CURRENT_PICTURE)
                            {
                                SkipB = TRUE;
                            }
                            if ((VIDTRICK_Data_p->DisplayParams.DisplayStart != VIDDISP_DISPLAY_START_CARE_MPEG_FRAME)/*&& (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat <= VIDTRICK_LEVEL_BACK_TO_DISPLAY_CARE_FRAME)*/)
                            {
                                VIDTRICK_Data_p->DisplayParams.DisplayStart = VIDDISP_DISPLAY_START_CARE_MPEG_FRAME;
                                /* Notify event so that API level can choose to change display settings or not. */
                                STEVT_Notify(VIDTRICK_Data_p->EventsHandle, VIDTRICK_Data_p->RegisteredEventsID[VIDTRICK_DISPLAY_PARAMS_CHANGE_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(VIDTRICK_Data_p->DisplayParams));
                            }
                        } /* Nb Fields <= VIDTRICK_LEVEL_SKIP_ALL_B_AND_DISP_P */
                    }

                    if (SkipB)
                    {
                        VIDDEC_SetNextDecodeAction(VIDTRICK_Data_p->DecodeHandle, VIDDEC_DECODE_ACTION_SKIP);
                        /* Remember that if 2nd field : it has to be skipped */
                        if (IsStvidPictureInfosExpectingSecondField(PictureInfos_p))
                        {
                            VIDTRICK_Data_p->Flags.SecondFieldHasToBeSkipped = TRUE;
                        }

#ifdef ST_smoothbackward
                        if (!WaitingForFirstPictureToBeDisplayed)
#endif /* ST_smoothbackward */
                        {
                            VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat -= NbFieldsShouldBeDisplayed;
                        }

#ifdef TRACE_UART_DISPLAY_SKIP
                        TraceBuffer(("c:%d NbFields=%d B:--\r\n",
                                           VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat,
                                           NbFieldsShouldBeDisplayed));
#endif
                    } /* if (SkipB) */
                } /* if B picture  */
                /*------------------------------------------------------------------------*/
                /*- P Frame --------------------------------------------------------------*/
                /*------------------------------------------------------------------------*/
                else if (PictureInfos_p->VideoParams.MPEGFrame == STVID_MPEG_FRAME_P)
                {
                    SkipP = FALSE;
#ifdef ST_smoothbackward
                    if ((WaitingForFirstPictureToBeDisplayed)||(!PictureHasToBeDisplayed))
                    {
                        DecodeButNotDisplay = TRUE;
                    }
                    else
#endif /* ST_smoothbackward */
                    {
                        if (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat > VIDTRICK_LEVEL_SKIP_ALL_P)
                        {
                            SkipP = TRUE;
                            /* One P Picture is skipped. All P & B others have to been skipped until next I Picture */
                            VIDTRICK_Data_p->Flags.MustSkipAllPPictures = TRUE;
                            if (VIDTRICK_Data_p->DisplayParams.DisplayStart != VIDDISP_DISPLAY_START_AS_AVAILABLE)
                            {
                                /* Set Display Params */
                                VIDTRICK_Data_p->DisplayParams.DisplayStart = VIDDISP_DISPLAY_START_AS_AVAILABLE;
                                /* Notify event so that API level can choose to change display settings or not. */
                                STEVT_Notify(VIDTRICK_Data_p->EventsHandle, VIDTRICK_Data_p->RegisteredEventsID[VIDTRICK_DISPLAY_PARAMS_CHANGE_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(VIDTRICK_Data_p->DisplayParams));
                            }
                        }
                        else if (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat > VIDTRICK_LEVEL_SKIP_ALL_B_AND_DISP_P)
                        {
                            if (VIDTRICK_Data_p->DisplayParams.DisplayStart != VIDDISP_DISPLAY_START_AS_AVAILABLE)
                            {
                                /* Set Display Params */
                                VIDTRICK_Data_p->DisplayParams.DisplayStart = VIDDISP_DISPLAY_START_AS_AVAILABLE;
                                /* Notify event so that API level can choose to change display settings or not. */
                                STEVT_Notify(VIDTRICK_Data_p->EventsHandle, VIDTRICK_Data_p->RegisteredEventsID[VIDTRICK_DISPLAY_PARAMS_CHANGE_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(VIDTRICK_Data_p->DisplayParams));
                            }
                        }
                    }

                    if (SkipP)
                    {
                        /* Skip  */
                        VIDDEC_SetNextDecodeAction(VIDTRICK_Data_p->DecodeHandle,  VIDDEC_DECODE_ACTION_SKIP);
                        /* Remember that if 2nd field : it has to be skipped */
                        if (IsStvidPictureInfosExpectingSecondField(PictureInfos_p))
                        {
                            VIDTRICK_Data_p->Flags.SecondFieldHasToBeSkipped = TRUE;
                        }
                    }
#ifdef ST_smoothbackward
                    else if (DecodeButNotDisplay)
                    {
                        /* Decode but not display  */
                        VIDDEC_SetNextDecodeAction(VIDTRICK_Data_p->DecodeHandle,  VIDDEC_DECODE_ACTION_NO_DISPLAY);
                    }
#endif /* ST_smoothbackward */

#ifdef ST_smoothbackward
                    if (!WaitingForFirstPictureToBeDisplayed)
#endif /* ST_smoothbackward */
                    {
                        if (SkipP)
                        {
                            VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat -= NbFieldsShouldBeDisplayed ;

#ifdef TRACE_UART_DISPLAY_SKIP
                            TraceBuffer(("c:%d NbFields=%d P\r\n",
                                               VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat,
                                               NbFieldsShouldBeDisplayed));
#endif
                        }
                    } /* if (!WaitingForFirstPictureToBeDisplayed) */
                }
                /*------------------------------------------------------------------------*/
                /*- I Frame --------------------------------------------------------------*/
                /*------------------------------------------------------------------------*/
                else if (PictureInfos_p->VideoParams.MPEGFrame == STVID_MPEG_FRAME_I)
                {
#ifdef ST_smoothbackward
                     if ((WaitingForFirstPictureToBeDisplayed)||(!PictureHasToBeDisplayed))
                    {
                        /* Decode but not display  */
                        VIDDEC_SetNextDecodeAction(VIDTRICK_Data_p->DecodeHandle,  VIDDEC_DECODE_ACTION_NO_DISPLAY);
                    }
                    else
#endif
                    {
#ifdef TRACE_UART_DISPLAY_SKIP
                        TraceBuffer(("c:%d  I\r\n", VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat));
#endif
                        /* If Required Speed is still not taken under account */
                        if (!VIDTRICK_Data_p->Flags.RequiredSpeedTakenInAccount)
                        {
                            SpeedInitialize(TrickModeHandle);
                        }

                        /* Skip I pictures only if a I pictures are very near in the stream */
                        /* As there are only I Pictures current I is skippeable as B picture is skipeable in a normal stream */
                        if (   (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat > VIDTRICK_LEVEL_SKIP_CURRENT_PICTURE)
                            && (VIDTRICK_Data_p->NbTreatedPicturesSinceLastDecodedI < VIDTRICK_BETWEEN_2_I_MIN_PIC_NB))
                        {
                            /* Skip */
                            VIDDEC_SetNextDecodeAction(VIDTRICK_Data_p->DecodeHandle, VIDDEC_DECODE_ACTION_SKIP);
                            /* Remember that if 2nd field : it has to be skipped */
                            if (IsStvidPictureInfosExpectingSecondField(PictureInfos_p))
                            {
                                VIDTRICK_Data_p->Flags.SecondFieldHasToBeSkipped = TRUE;
                            }
                            /* If MainParams.NbFieldsIfPosToSkipIfNegToRepeat is not enough decreased I pictures may never be displayed */
                            VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat -= NbFieldsShouldBeDisplayed ;

#ifdef TRACE_UART_DISPLAY_SKIP
                            TraceBuffer(("SkipIifNear: NBFields=%d SkipRepeat=%d\r\n",
                                                  NbFieldsShouldBeDisplayed,
                                                  VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat));
#endif
                            /* I Picture has been skipped : All next P & B Pictures have to skipped */
                            VIDTRICK_Data_p->Flags.MustSkipAllPPictures = TRUE;
                            if (VIDTRICK_Data_p->DisplayParams.DisplayStart != VIDDISP_DISPLAY_START_AS_AVAILABLE)
                            {
                                VIDTRICK_Data_p->DisplayParams.DisplayStart = VIDDISP_DISPLAY_START_AS_AVAILABLE;
                                /* Notify event so that API level can choose to change display settings or not. */
                                STEVT_Notify(VIDTRICK_Data_p->EventsHandle, VIDTRICK_Data_p->RegisteredEventsID[VIDTRICK_DISPLAY_PARAMS_CHANGE_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(VIDTRICK_Data_p->DisplayParams));
                            }
                        }
                        else /* normal */
                        {
                            if (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat > VIDTRICK_LEVEL_SKIP_ALL_B_AND_DISP_P)
                            {
                                if (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat > VIDTRICK_LEVEL_SKIP_ALL_P)
                                {
                                    VIDTRICK_Data_p->Flags.MustSkipAllPPictures = TRUE;
                                }
                                else
                                {
                                    VIDTRICK_Data_p->Flags.MustSkipAllPPictures = FALSE;
                                }

                                if (VIDTRICK_Data_p->DisplayParams.DisplayStart != VIDDISP_DISPLAY_START_AS_AVAILABLE)
                                {
                                    /* Set display params */
                                    VIDTRICK_Data_p->DisplayParams.DisplayStart = VIDDISP_DISPLAY_START_AS_AVAILABLE;
                                    /* Notify event so that API level can choose to change display settings or not. */
                                    STEVT_Notify(VIDTRICK_Data_p->EventsHandle, VIDTRICK_Data_p->RegisteredEventsID[VIDTRICK_DISPLAY_PARAMS_CHANGE_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(VIDTRICK_Data_p->DisplayParams));
                                }
                            }
                            else
                            {
                                VIDTRICK_Data_p->Flags.MustSkipAllPPictures = FALSE;

                                /* I Picture has been decoded : Next P & B Pictures can be decoded */
                                if ((VIDTRICK_Data_p->DisplayParams.DisplayStart != VIDDISP_DISPLAY_START_CARE_MPEG_FRAME)/*&& (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat <= VIDTRICK_LEVEL_BACK_TO_DISPLAY_CARE_FRAME)*/)
                                {
                                    /* Set display params */
                                    VIDTRICK_Data_p->DisplayParams.DisplayStart = VIDDISP_DISPLAY_START_CARE_MPEG_FRAME;
                                    /* Notify event so that API level can choose to change display settings or not. */
                                    STEVT_Notify(VIDTRICK_Data_p->EventsHandle, VIDTRICK_Data_p->RegisteredEventsID[VIDTRICK_DISPLAY_PARAMS_CHANGE_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(VIDTRICK_Data_p->DisplayParams));
                                }
                            } /* else of if (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat > VIDTRICK_LEVEL_SKIP_ALL_P) */
                        } /* else of if stream contains only I pictures */
                    }

                    VIDTRICK_Data_p->NbTreatedPicturesSinceLastDecodedI = 0;

                    /* Skip on HDD must be done only if I picture ! */
                    if (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat > VIDTRICK_LEVEL_SKIP_ON_HDD)
                    {
                        ErrorCode = GetBitRateValue(TrickModeHandle, &BitRateValue);
                        if (ErrorCode == ST_NO_ERROR)
                        {
                            SpeedDriftThresholdData.BitRateValue = BitRateValue;
                            NbDriftedFields = VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat - VIDTRICK_LEVEL_TO_REACH_AFTER_SKIP_ON_HDD;

                            /* FrameRate is 24000, 25000 (progressive) or 50000, 60000 (interlaced) */
                            /* So, if progressive DriftTime final value has to divided by 2 */
                            SpeedDriftThresholdData.DriftTime = (NbDriftedFields * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) / (PictureInfos_p->VideoParams.FrameRate / 1000);
                            if (PictureInfos_p->VideoParams.FrameRate <= 30000)
                            {
                                SpeedDriftThresholdData.DriftTime = SpeedDriftThresholdData.DriftTime / 2;
                            }

                            /* Ask for a HDD skip */
                            STEVT_Notify(VIDTRICK_Data_p->EventsHandle, VIDTRICK_Data_p->RegisteredEventsID[VIDTRICK_SPEED_DRIFT_THRESHOLD_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST (&SpeedDriftThresholdData));
                            VIDTRICK_Data_p->ForwardInjectionDiscontinuity = TRUE;
                            UpdateUnderflowEventParams(TrickModeHandle);
                            VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat = VIDTRICK_LEVEL_TO_REACH_AFTER_SKIP_ON_HDD ;

#ifdef TRACE_UART_DISPLAY_SKIP
                            TraceBuffer(("Drift Forward on HDD: Field=%d, DriftTime=%d, SkipRepeat=%d\n\r",
                                    NbDriftedFields,
                                    SpeedDriftThresholdData.DriftTime,
                                    VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat));
#endif
                        }
                    }
                } /* if I frame */
            }/* If Speed >0 */

            /*------------------------------------------------------------------------*/
            /*- Backward I Only ------------------------------------------------------*/
            /*------------------------------------------------------------------------*/
            else /* If Speed < 0 */
            {
                if (PictureInfos_p->VideoParams.MPEGFrame != STVID_MPEG_FRAME_I)
                {
                    VIDDEC_SetNextDecodeAction(VIDTRICK_Data_p->DecodeHandle, VIDDEC_DECODE_ACTION_SKIP);
                    /* Remember that if 2nd field : it has to be skipped */
                    if (IsStvidPictureInfosExpectingSecondField(PictureInfos_p))
                    {
                        VIDTRICK_Data_p->Flags.SecondFieldHasToBeSkipped = TRUE;
                    }
                }

                /* If Backward I only, Speed Drift Treshold evts have to be sent */
                if (VIDTRICK_Data_p->BackwardIOnly.NbVsyncSinceLastSentDriftEvt > 0)
                {
                    /* Get bit rate value of the decoded stream in units of 400 bits/second */
                    ErrorCode = GetBitRateValue(TrickModeHandle, &BitRateValue);
                    if (ErrorCode == ST_NO_ERROR)
                    {
                        SpeedDriftThresholdData.BitRateValue = BitRateValue;
                        SpeedDriftThresholdData.DriftTime = (VIDTRICK_Data_p->BackwardIOnly.NbVsyncSinceLastSentDriftEvt * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) / (PictureInfos_p->VideoParams.FrameRate / 1000);
                        SpeedDriftThresholdData.DriftTime = (SpeedDriftThresholdData.DriftTime * VIDTRICK_Data_p->MainParams.Speed) / 100;
                        if (PictureInfos_p->VideoParams.FrameRate <= 30000)
                        {
                            SpeedDriftThresholdData.DriftTime = SpeedDriftThresholdData.DriftTime / 2;
                        }

                        /* Send drift treshold evt */
                        STEVT_Notify(VIDTRICK_Data_p->EventsHandle, VIDTRICK_Data_p->RegisteredEventsID[VIDTRICK_SPEED_DRIFT_THRESHOLD_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST (&SpeedDriftThresholdData));
                        VIDTRICK_Data_p->BackwardIOnly.NbVsyncSinceLastSentDriftEvt = 0;

#ifdef TRACE_UART_DISPLAY_SKIP
                        TraceBuffer(("**SpeedDrift: BitRate=%d, DriftTime=%d\n\r",
                                SpeedDriftThresholdData.BitRateValue,
                                SpeedDriftThresholdData.DriftTime));
#endif
                    }
                } /* if (VIDTRICK_Data_p->BackwardIOnly.NbVsyncSinceLastSentDriftEvt > 0) */
            } /* if speed < 0 */
        } /* if frame of first field */

        /* Un-Lock mutex protecting Trick mode private params */
        STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);

    } /* if (VIDTRICK_Data_p->Flags.SpeedControlEnabled) */
} /* end of ReadyToDecodeNewPictureTrickMode() function */


/*******************************************************************************
Name        : SpeedInitialize
Description : Sets real speed of video and a lot of parameters
Parameters  : Video TrickMode handle, Speed
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
static void SpeedInitialize(const VIDTRICK_Handle_t TrickModeHandle)
{
    VIDTRICK_Data_t   * VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;
    U32   PositiveOfRequiredSpeed;
    U32   PositiveOfCurrentSpeed;

    PositiveOfRequiredSpeed =  (VIDTRICK_Data_p->MainParams.RequiredSpeed >= 0)
                             ? (VIDTRICK_Data_p->MainParams.RequiredSpeed)
                             : (-VIDTRICK_Data_p->MainParams.RequiredSpeed);
    PositiveOfCurrentSpeed =  (VIDTRICK_Data_p->MainParams.Speed >= 0)
                            ? (VIDTRICK_Data_p->MainParams.Speed)
                            : (-VIDTRICK_Data_p->MainParams.Speed);

    /* Compute New value of MainParams.NbFieldsIfPosToSkipIfNegToRepeat */
    if (PositiveOfRequiredSpeed < VIDTRICK_SPEED_MIN_SPEED_FOR_NO_NULL_COUNTER)
    {
        VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat = 0;

#ifdef TRACE_UART_DISPLAY_SKIP
        TraceBuffer(("NULLCOUNT: "));
#endif
    }
    else if ((PositiveOfRequiredSpeed < VIDTRICK_MAX_SPEED_FOR_SMALL_SKIP) || (PositiveOfCurrentSpeed==0))
    {
        VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat = PositiveOfRequiredSpeed / 100;
#ifdef TRACE_UART_DISPLAY_SKIP
        TraceBuffer(("SMALLSKIP: "));
#endif
    }
    else
    {
        VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat = (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat * ((S32) PositiveOfRequiredSpeed)) / ((S32)PositiveOfCurrentSpeed);
#ifdef TRACE_UART_DISPLAY_SKIP
        TraceBuffer(("SKIP: "));
#endif
    }

    if ((VIDTRICK_Data_p->MainParams.RequiredSpeed > 0) && (VIDTRICK_Data_p->MainParams.Speed > 0))
    {
        /* if new speed is required, HDD jumps can not be allowed just now because */
        /* B & P pictures have firstly to be received to try to decrease MainParams.NbFieldsIfPosToSkipIfNegToRepeat */
        /* it is useful when switching for example from speed 900 to speed 400 */
        if (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat > VIDTRICK_LEVEL_SKIP_ALL_P)
        {
            VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat = VIDTRICK_LEVEL_SKIP_ALL_P;

#ifdef TRACE_UART_DISPLAY_SKIP
            TraceBuffer(("ALLP: "));
#endif
        }
    }

#ifdef TRACE_UART_DISPLAY_SKIP
    TraceBuffer(("SkipRepeat=%d\r\n",VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat));
#endif

    /* Initialize new Speed */
    VIDTRICK_Data_p->MainParams.Speed = VIDTRICK_Data_p->MainParams.RequiredSpeed;
    VIDTRICK_Data_p->Flags.RequiredSpeedTakenInAccount = TRUE;

    /* forward init params */
    if (VIDTRICK_Data_p->MainParams.Speed >= 0)
    {
        VIDTRICK_Data_p->DisplayParams.DisplayForwardNotBackward = TRUE;

        /* Since speed > 0, bit rate value is computable. */
        VIDDEC_EnableBitRateEvaluating(VIDTRICK_Data_p->DecodeHandle);

        VIDTRICK_Data_p->DisplayParams.DisplayWithCareOfFieldsPolarity = FALSE;
        /* Speed 100%: care fields polarity matching when displaying */
        if (VIDTRICK_Data_p->MainParams.Speed == 100)
        {
            VIDTRICK_Data_p->DisplayParams.DisplayWithCareOfFieldsPolarity = TRUE;
        }
        UpdateUnderflowEventParams(TrickModeHandle);
    }
    /* backward init params */
    else
    {
        VIDTRICK_Data_p->DisplayParams.DisplayForwardNotBackward = FALSE;
        VIDTRICK_Data_p->DisplayParams.DisplayWithCareOfFieldsPolarity = FALSE;
#ifdef ST_smoothbackward
        /* Speed 100%: care fields polarity matching when displaying only if VIDDEC_GetSmoothBackwardCapability OK */
        if (   (VIDTRICK_Data_p->MainParams.Speed == -100)
            && ((VIDDEC_GetSmoothBackwardCapability(VIDTRICK_Data_p->DecodeHandle) == ST_NO_ERROR)))
        {
            VIDTRICK_Data_p->DisplayParams.DisplayWithCareOfFieldsPolarity = TRUE;
        }
#endif /* ST_smoothbackward */

        VIDTRICK_Data_p->DisplayParams.DisplayStart = VIDDISP_DISPLAY_START_AS_AVAILABLE;
    }

    /* Set display params */
    /* Cannot notify event VIDTRICK_DISPLAY_PARAMS_CHANGE_EVT here because we may be in API call, notify it right after in task */
    VIDTRICK_Data_p->EventDisplayChangeToBeNotifiedInTask = TRUE;

    /* Trick Mode Task has to be waked up so that Fields have to be skipped if necessary. */
    semaphore_signal(VIDTRICK_Data_p->WakeTrickModeTaskUp_p);

} /* End of SpeedInitialize() function */


/*******************************************************************************
Name        : StartTrickModeTask
Description : Start the task of Trick Mode control
Parameters  : Trick Mode handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_ALREADY_INITIALIZED if task already running
              ST_ERROR_BAD_PARAMETER if problem of creation
*******************************************************************************/
static ST_ErrorCode_t StartTrickModeTask(const VIDTRICK_Handle_t TrickModeHandle)
{
    VIDCOM_Task_t * const Task_p = &(((VIDTRICK_Data_t *) TrickModeHandle)->TrickModeTask);
    VIDTRICK_Data_t * VIDTRICK_Data_p;
    ST_ErrorCode_t ErrorCode ;
    char TaskName[23] = "STVID[?].TrickModeTask";

    if (Task_p->IsRunning)
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* Find structure */
    VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;

    /* In TaskName string, replace '?' with decoder number */
    TaskName[6] = (char) ((U8) '0' + VIDTRICK_Data_p->DecoderNumber);
    Task_p->ToBeDeleted = FALSE;

    if ((ErrorCode = STOS_TaskCreate((void (*) (void*)) TrickModeTaskFunc,(void *) TrickModeHandle,
            VIDTRICK_Data_p->CPUPartition_p,
            STVID_TASK_STACK_SIZE_TRICKMODE,
            &(Task_p->TaskStack),
            VIDTRICK_Data_p->CPUPartition_p,
            &(Task_p->Task_p),
            &(Task_p->TaskDesc),
            STVID_TASK_PRIORITY_TRICKMODE,
            TaskName, task_flags_no_min_stack_size)) != ST_NO_ERROR)
    {
        return(ErrorCode);
    }

    Task_p->IsRunning  = TRUE;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "TrickMode task created."));
    return(ST_NO_ERROR);
} /* End of StartTrickModeTask() function */


/*******************************************************************************
Name        : StopTrickModeTask
Description : Stop the task of TrickMode control
Parameters  : TrickMode handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if no problem,
              ST_ERROR_ALREADY_INITIALIZED if task not running
*******************************************************************************/
static ST_ErrorCode_t StopTrickModeTask(const VIDTRICK_Handle_t TrickModeHandle)
{
    VIDCOM_Task_t * const Task_p = &(((VIDTRICK_Data_t *) TrickModeHandle)->TrickModeTask);
    VIDTRICK_Data_t * VIDTRICK_Data_p;
    task_t * TaskList_p;;

    if (!(Task_p->IsRunning))
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* Find structure */
    VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;

    TaskList_p = Task_p->Task_p;

    /* End the function of the task here... */
    Task_p->ToBeDeleted = TRUE;

    /* Wake the task up so that it can terminate */
    semaphore_signal(VIDTRICK_Data_p->WakeTrickModeTaskUp_p);

    STOS_TaskWait(&TaskList_p, TIMEOUT_INFINITY);
    STOS_TaskDelete ( Task_p->Task_p,
                                  VIDTRICK_Data_p->CPUPartition_p,
                                  Task_p->TaskStack,
                                  VIDTRICK_Data_p->CPUPartition_p);

    Task_p->IsRunning  = FALSE;

    return(ST_NO_ERROR);
} /* End of StopTrickModeTask() function */



/*******************************************************************************
Name        : TrickModeTaskFunc
Description : Function of the TrickMode task
              Set VIDTRICK_Data_p->TrickModeTask.ToBeDeleted to end the task
Parameters  : TrickMode handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void TrickModeTaskFunc(const VIDTRICK_Handle_t VideoTrickModeHandle)
{
    VIDTRICK_Data_t * VIDTRICK_Data_p;

    VIDTRICK_Handle_t TrickModeHandle;
    clock_t         MaxWaitOrderTimeWatchDog;

#ifdef ST_smoothbackward
    VIDBUFF_PictureBuffer_t *                               PictureBufferToDecodeIn_p;
    VIDTRICK_PictureInfo_t *                                PictureInfo_p;
    VIDTRICK_PictureInfo_t *                                MostRecentDecodablePictureInfo_p;
    VIDTRICK_PictureInfo_t *                                PictureInfoInFrameBuffer_p = NULL;
    BOOL            SearchForPicture;
    BOOL            StateIsOkToLaunchDecode;
    BOOL            DecodeCanBeLaunched;
    VIDCOM_PictureID_t    MostRecentDecodablePictureExtendedToBeCompared, PictureBufferToDecodeInExtendedToBeCompared;
    ST_ErrorCode_t  ErrorCode;
#endif /* ST_smoothbackward */

    STOS_TaskEnter(NULL);

#ifdef REAL_OS40
    TrickModeHandle = GlobalContext;
    semV(&ContextAccess);
#else /* REAL_OS40 */
    TrickModeHandle = VideoTrickModeHandle;
#endif /* not REAL_OS40 */

     VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;

    do
    {
        if (VIDTRICK_Data_p->MainParams.Speed == 100)
        {
            /* There is nothing to do, so semaphore waits infinitely. */
            STOS_SemaphoreWait(VIDTRICK_Data_p->WakeTrickModeTaskUp_p);
        }
        else
        {
        /* Trick Mode Task has to wait for VSYNC subscribe callback */
        if (VIDTRICK_Data_p->IsWaitOrderTimeWatchDog)
        {
            MaxWaitOrderTimeWatchDog = time_plus(time_now(), MAX_WAIT_ORDER_TIME_WATCHDOG);
            semaphore_wait_timeout(VIDTRICK_Data_p->WakeTrickModeTaskUp_p, &MaxWaitOrderTimeWatchDog);
        }
        else
        {
            /* Specific case of polling for launch start code search in smooth backward: wait timeout. */
            semaphore_wait_timeout(VIDTRICK_Data_p->WakeTrickModeTaskUp_p, &(VIDTRICK_Data_p->GiveOtherTasksCPUMaxWaitOrderTime));
            /* Now wait infinity (normal) */
            VIDTRICK_Data_p->IsWaitOrderTimeWatchDog = TRUE;
        }
        while (semaphore_wait_timeout(VIDTRICK_Data_p->WakeTrickModeTaskUp_p, TIMEOUT_IMMEDIATE) == 0)
        {
            /* Just pop all semaphore signals, so that in case of multiple signal there is only one wait, and not many for nothing... */;
        }

#ifdef ST_smoothbackward
        if(VIDTRICK_Data_p->Flags.BackwardModeInitIsRequiredInTask)
        {
            VIDTRICK_Data_p->Flags.BackwardModeInitIsRequiredInTask = FALSE;
            BackwardModeInit(TrickModeHandle, FALSE);
        }
#endif /* ST_smoothbackward */

        if (VIDTRICK_Data_p->EventDisplayChangeToBeNotifiedInTask)
        {
            /* Notify event so that API level can choose to change display settings or not. */
            VIDTRICK_Data_p->EventDisplayChangeToBeNotifiedInTask = FALSE;
            STEVT_Notify(VIDTRICK_Data_p->EventsHandle, VIDTRICK_Data_p->RegisteredEventsID[VIDTRICK_DISPLAY_PARAMS_CHANGE_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(VIDTRICK_Data_p->DisplayParams));
        }

#ifdef ST_smoothbackward
        if (VIDTRICK_Data_p->BitBufferFullyParsedToBeProcessedInTask_p != NULL)
        {
            LinearBitBufferFullyParsed(VideoTrickModeHandle, VIDTRICK_Data_p->BitBufferFullyParsedToBeProcessedInTask_p);
            VIDTRICK_Data_p->BitBufferFullyParsedToBeProcessedInTask_p = NULL;
        }
#endif /* ST_smoothbackward */

        /* Lock mutex protecting Trick mode private params */
        STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p);

















#ifdef ST_smoothbackward
        if ((VIDTRICK_Data_p->NewStartCodeSearch.MustWaitBecauseOfVSync) &&
            (STOS_time_after(time_now(), VIDTRICK_Data_p->NewStartCodeSearch.DontStartBeforeThatTime) != 0))
        {
            /* There has been a VSync: leave some time where no start code search takes place for other tasks.
            If many VSync occured, leave time corresponding to each VSync one by one. */
            VIDTRICK_Data_p->NewStartCodeSearch.MustWaitBecauseOfVSync = FALSE;
            VIDTRICK_Data_p->NewStartCodeSearch.DontStartBeforeThatTime = time_plus(time_now(), VIDTRICK_DEFAULT_TIME_TO_WAIT_AT_VSYNC_BEFORE_START_CODE_SEARCH_IN_TICKS);
        }
        if (VIDTRICK_Data_p->NewStartCodeSearch.IsRequiredInTask)
        {
/*            if (((VIDTRICK_Data_p->PercentageParsingBitBufferConsumed <= ((VIDTRICK_Data_p->PercentageDecodeBitBufferConsumed * 10) / 8)) &&*/
/*                 (STOS_time_after(time_now(), VIDTRICK_Data_p->NewStartCodeSearch.DontStartBeforeThatTime) != 0)) ||*/
            if (STOS_time_after(time_now(), VIDTRICK_Data_p->NewStartCodeSearch.DontStartBeforeThatTime) != 0)
            {
                /* OK to launch start code search now... */
                VIDTRICK_Data_p->NewStartCodeSearch.IsRequiredInTask = FALSE;
                /* Do that as latest : this triggers start code detector, and all variables must be set before ! */
                NewStartCodeSearchIsRequired(TrickModeHandle, VIDTRICK_Data_p->NewStartCodeSearch.SearchMode, VIDTRICK_Data_p->NewStartCodeSearch.FirstSliceDetect, VIDTRICK_Data_p->NewStartCodeSearch.SearchAddress_p);
            }
            else
            {
                /* Not OK to launch start code search now, must wait: For next loop in trick mode task, wait with timeout */
                VIDTRICK_Data_p->IsWaitOrderTimeWatchDog = FALSE;
                VIDTRICK_Data_p->GiveOtherTasksCPUMaxWaitOrderTime = VIDTRICK_Data_p->NewStartCodeSearch.DontStartBeforeThatTime;
            }
        }

        /* if speed >= 0 stop here */
        if (   (VIDTRICK_Data_p->MainParams.Speed < 0)
            && (! VIDTRICK_Data_p->Flags.BackwardToForwardInitializing))  /* Not in Backward to Forward mode init phase */
        {
            if (VIDDEC_GetSmoothBackwardCapability(VIDTRICK_Data_p->DecodeHandle) == ST_NO_ERROR)
            {
                if (! VIDTRICK_Data_p->Flags.SmoothBackwardIsInitializing)
                {
                    /* try to fill the table of pictures to be inserted in display queue as soon as possible */
                    if (   (VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ < (VIDTRICK_Data_p->NbFieldsToBeInDisplayQueue + 2))
                        && (! VIDTRICK_Data_p->Stop.TrickmodIsStopped))
                    {
                        UpdateLastPictureInDisplayQueueList(TrickModeHandle);
                    }

                    /* Find a picture to insert in display queue */
                    FindPictureToComputeFromAndInsertInDisplayQueue(TrickModeHandle, &PictureInfo_p);

                    /* If all pictures of the table of pictures to insert are decoded : returned PictureInfo_p is NULL */
                    if (PictureInfo_p != NULL)
                    {
                        /* if a picture is under decoding : can not decode an other picture */
                        if (   (VIDTRICK_Data_p->PictureUnderDecoding_p == NULL)
                            && (! VIDTRICK_Data_p->Stop.TrickmodIsStopped))
                        {
                            /* if B picture and 2nd field : decode first field before !! */
                            if (   (IsPictureSecondField(PictureInfo_p))
                                && (PictureMpegTypeIsB(PictureInfo_p))
                                && (! PictureInfo_p->IsDecoded))
                            {
                                /* previous in the stream is the first field B picture */
                                if  (PictureInfo_p->PreviousInStreamPicture_p != NULL)
                                {
                                    PictureInfo_p = PictureInfo_p->PreviousInStreamPicture_p;
                                }
#ifdef TRICKMOD_DEBUG
                                else
                                {
                                    TrickModeError();
                                }
                                if (   (! PictureMpegTypeIsB(PictureInfo_p))
                                    || (HasPictureASecondField(PictureInfo_p->BackwardOrLastBeforeIReferencePicture_p))
                                    || (IsPictureSecondField(PictureInfo_p->ForwardOrNextReferencePicture_p)))
                                {
                                    TrickModeError();
                                }
#endif /* TRICKMOD_DEBUG */
                            }

                            if (   (! PictureInfo_p->IsDecoded)
                                && (! IsPictureSecondField(PictureInfo_p)))
                            {
                                StateIsOkToLaunchDecode = TRUE;  /* initialized to TRUE : first supposed to be possible */

                                /*------------------------------------------------------------------------*/
                                /*- Trying to decode picture to be inserted in display queue -------------*/
                                /*------------------------------------------------------------------------*/
#ifdef TRACE_UART_DEBUG
                                TraceBuffer (("\r\nTrying to decode picture to be inserted in display queue 0x%x %c%d\r\n",
                                           PictureInfo_p,
                                           MPEGFrame2Char(PictureInfo_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame),
                                           PictureInfo_p->PictureInfoFromDecode.pTemporalReference));
#endif

                                ErrorCode = AssociatePictureBuffers(TrickModeHandle, PictureInfo_p);

                                if (ErrorCode == ST_NO_ERROR)
                                {
                                    PictureBufferToDecodeIn_p = PictureInfo_p->PictureBuffer_p;

                                    /* One buffer has been found (should always be true) */
                                    PictureInfoInFrameBuffer_p = (VIDTRICK_PictureInfo_t *)(PictureBufferToDecodeIn_p->SmoothBackwardPictureInfo_p);

                                    /* the picture is not decoded. Try to decode it */
                                    /* here we dont mind of the value of IsStillUsedToDecodeOtherPictures because */
                                    /* the decode of the next picture to display is necessary */
                                    if (HasPictureReferenceForDecode(TrickModeHandle, PictureInfo_p, PictureInfoInFrameBuffer_p))
                                    {
                                        /* TraceBuffer(("For Disp %d, %d, %d/%d   ", */
                                        /* PictureInfo_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame, */
                                        /* PictureInfo_p->PictureInfoFromDecode.pTemporalReference, */
                                        /* PictureInfo_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference)); */
                                        /* launch the decode of the picture */
                                        if (PictureInfo_p->IsDecodable)
                                        {
                                            NewPictureDecodeIsRequired(TrickModeHandle, PictureInfo_p, PictureBufferToDecodeIn_p, PictureInfoInFrameBuffer_p);
                                        }
#ifdef TRICKMOD_DEBUG
                                        else
                                        {
                                            TrickModeError();
                                        }
#endif /* TRICKMOD_DEBUG */
                                    }
                                    else
                                    {
#ifdef TRACE_UART_DEBUG
                                        TraceBuffer(("Ref needed: Release Pict 0x%x, PB 0x%x, PB Decim 0x%x\n\r",
                                                    PictureInfo_p,
                                                    PictureInfo_p->PictureBuffer_p,
                                                    PictureInfo_p->PictureBuffer_p->AssociatedDecimatedPicture_p));
#endif
                                        if (PictureInfo_p->PictureBuffer_p->AssociatedDecimatedPicture_p != NULL)
                                        {
                                            RELEASE_PICTURE_BUFFER(VIDTRICK_Data_p->BufferManagerHandle, PictureInfo_p->PictureBuffer_p->AssociatedDecimatedPicture_p, VIDCOM_VIDTRICK_MODULE_BASE);
                                            /* It is not allowed to set AssociatedDecimatedPicture_p to NULL because the link between Main and Decimated Picture Buffers is managed by VID_BUFF */
                                        }
                                        RELEASE_PICTURE_BUFFER(VIDTRICK_Data_p->BufferManagerHandle, PictureInfo_p->PictureBuffer_p, VIDCOM_VIDTRICK_MODULE_BASE);
                                        PictureInfo_p->PictureBuffer_p = NULL;
                                    }
                                }
                            } /* if (!PictureInfo_p->IsDecoded) && (Frame or First field !) */

                            /* the decode of the picture to be inserted in display queue has not been launched or */
                            /* the table of the pictures to be inserted in display queue as soon as possible is full */
                            /* Try to decode as many recent previous references as possible to anticipate next decodes */
                            /* of next pictures */
                            if (VIDTRICK_Data_p->PictureUnderDecoding_p == NULL)
                            {
                                /*------------------------------------------------------------------------*/
                                /*- Trying to decode most recent not decoded reference -------------------*/
                                /*------------------------------------------------------------------------*/
                                SearchForPicture = SearchForMostRecentDecodableReferencePictureRecord(TrickModeHandle, PictureInfo_p, &MostRecentDecodablePictureInfo_p);
                                DecodeCanBeLaunched = FALSE;
                                if (   (SearchForPicture)
                                    && (!IsPictureSecondField(MostRecentDecodablePictureInfo_p)))
                                {
#ifdef TRACE_UART_DEBUG
                                    TraceBuffer (("\r\nTry to decode most recent not decoded reference 0x%x %c%d\r\n",
                                               MostRecentDecodablePictureInfo_p,
                                               MPEGFrame2Char(MostRecentDecodablePictureInfo_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame),
                                               MostRecentDecodablePictureInfo_p->PictureInfoFromDecode.pTemporalReference));
#endif

                                    /* SearchForPicture is TRUE : The given reference is decodable : but it is not useful */
                                    /* to decode it over an other most recent reference and if P, forbidden to decode it  */
                                    /* over its backward */
                                    ErrorCode = AssociatePictureBuffers(TrickModeHandle, MostRecentDecodablePictureInfo_p);

                                    if (ErrorCode == ST_NO_ERROR)
                                    {
                                        PictureBufferToDecodeIn_p = MostRecentDecodablePictureInfo_p->PictureBuffer_p;

                                        PictureInfoInFrameBuffer_p = (VIDTRICK_PictureInfo_t *)(PictureBufferToDecodeIn_p->SmoothBackwardPictureInfo_p);

                                        /* Nothing in the frame buffer we get. */
                                        if (HasPictureReferenceForDecode(TrickModeHandle, MostRecentDecodablePictureInfo_p, PictureInfoInFrameBuffer_p))
                                        {
#ifdef TRICKMOD_DEBUG
                                            /* Should be decodable ! */
                                            if (!MostRecentDecodablePictureInfo_p->IsDecodable)
                                            {
                                                TrickModeError();
                                                TraceBuffer (("MostRecentDecodablePictureInfo_p is not decodable!\r\n"));
                                            }
#endif /* TRICKMOD_DEBUG */

                                            if (PictureInfoInFrameBuffer_p != NULL)
                                            {
                                                MostRecentDecodablePictureExtendedToBeCompared.ID = MostRecentDecodablePictureInfo_p->PictureInfoFromDecode.pTemporalReference;
                                                MostRecentDecodablePictureExtendedToBeCompared.IDExtension = MostRecentDecodablePictureInfo_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference
                                                            - MostRecentDecodablePictureInfo_p->PictureInfoFromDecode.pTemporalReference;
                                                PictureBufferToDecodeInExtendedToBeCompared = PictureBufferToDecodeIn_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID;
                                                if ( vidcom_ComparePictureID(&MostRecentDecodablePictureExtendedToBeCompared, &PictureBufferToDecodeInExtendedToBeCompared) > 0 )
                                                {
                                                    /* TraceBuffer(("For Ref %d, %d, %d   ", */
                                                    /* MostRecentDecodablePictureInfo_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame, */
                                                    /* MostRecentDecodablePictureInfo_p->PictureInfoFromDecode.pTemporalReference, */
                                                    /* MostRecentDecodablePictureInfo_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference)); */
                                                    /* launch the decode of the picture */
                                                    NewPictureDecodeIsRequired(TrickModeHandle, MostRecentDecodablePictureInfo_p, PictureBufferToDecodeIn_p, PictureInfoInFrameBuffer_p);

                                                } /* MostRecentDecodablePictureExtendedToBeCompared > PictureBufferToDecodeInExtendedToBeCompared */
                                            } /* PictureInfoInFrameBuffer_p != NULL */
                                            else
                                            {
                                                /* TraceBuffer(("For Ref %d, %d, %d   ", */
                                                /* MostRecentDecodablePictureInfo_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame, */
                                                /* MostRecentDecodablePictureInfo_p->PictureInfoFromDecode.pTemporalReference, */
                                                /* MostRecentDecodablePictureInfo_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference)); */
                                                /* launch the decode of the picture */
                                                NewPictureDecodeIsRequired(TrickModeHandle, MostRecentDecodablePictureInfo_p, PictureBufferToDecodeIn_p, PictureInfoInFrameBuffer_p);

                                            } /* PictureInfoInFrameBuffer_p == NULL */
                                        } /* Has Picture Reference For Decode */
                                    } /* if AssociatePictureBuffers did not return any error */
                                } /* if a most recent reference picture to decode has been found */
                            } /* if no picure under decoding */
                        }   /* if no picture under decoding */
                    } /* if (PictureInfo_p != NULL) */

                    /* VLD Error Recovery */
                    /* decoder crashed */
                    if (VIDTRICK_Data_p->PictureUnderDecoding_p != NULL)
                    {
                        if (VIDTRICK_Data_p->ErrorRecovery.NbVsyncElapsedSinceLastDecodeHasBeenLaunched >= VIDTRICK_NB_VSYNC_MAX_FOR_A_DECODE)
                        {
#ifdef TRICKMOD_DEBUG
                            TraceBuffer(("Time Out picture under decoding elapsed : re-decode it\n\r"));
#endif
                            BackwardWarmReset(TrickModeHandle);
                        }
                    }
                    /* else */
                    /* { */
                        /* if ((VIDTRICK_Data_p->ErrorRecovery.NbVsyncElapsedWithoutAnyDecode >= VIDTRICK_NB_VSYNC_WITHOUT_ANY_DECODE) && */
                                /* (VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0] == VIDTRICK_Data_p->Discontinuity.LastDisplayablePictureBeforeJump_p)) */
                        /* { */
                            /* if trickmod manager is KO and requires no more decodes, re-initialize it only picture on display */
                            /* is different : it may mean that HDD manager do not inject anything any more because the beginning */
                            /* of the stream has been reached */
                            /* BackwardModeInit(TrickModeHandle, TRUE); */
                        /* } */
                    /* } */
                } /* end if smoothBackwardIsIninitializing */

                /* SCD Error recovery crashed */
                if (VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p != NULL)
                {
                    if (VIDTRICK_Data_p->ErrorRecovery.NbVSyncElapsedSinceLastSCDHasBeenLaunched >= VIDTRICK_NB_VSYNC_MAX_TO_FIND_SC)
                    {
#ifdef TRICKMOD_DEBUG
                        TraceBuffer(("Time Out searching SC : re-parse buffer under parsing\n\r"));
#endif
                        if (VIDTRICK_Data_p->PictureUnderDecoding_p != NULL)
                        {
                            BackwardWarmReset(TrickModeHandle);
                        }
                        else
                        {
                            BackwardModeInit(TrickModeHandle, TRUE);
                        }
                    }
                }
            }
            else
            {
                VIDTRICK_Data_p->BackwardIOnly.NbVsyncSinceLastSentDriftEvt ++;
            } /* end of GetSmoothBackwardCapability */
        } /* end of if Speed < 0 */
        else
        {
            if (VIDTRICK_Data_p->Flags.BackwardToForwardInitializing)
            {
                ForwardModeInit(TrickModeHandle);
            }
        }
#endif /* ST_smoothbackward */

        /* unlock mutex protecting Trick mode private params */
        STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
        }
     } while (!(((VIDTRICK_Data_t *) TrickModeHandle)->TrickModeTask.ToBeDeleted));

    STOS_TaskExit(NULL);

} /* End of TrickModeTaskFunc() function */

/*******************************************************************************
Name        : VIDTRICK_VsyncActionSpeed
Description : Function which has to be called every Vsync occurence. It computes the number of fileds to
              be skipped or repeated, Skipd or Repeats fields in display module in order to reach teh required speed.
Parameters  :
Assumptions : None
Limitations : None
Returns     : None
*******************************************************************************/
ST_ErrorCode_t VIDTRICK_VsyncActionSpeed(const VIDTRICK_Handle_t TrickModeHandle, BOOL IsReferenceDisplay)
{
    VIDTRICK_Data_t  * VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    BOOL   ComputeFieldsToSkipOrToRepeat = FALSE;
    S32    VirtualClockCounter;
    S32    TimeElapsed = 0;
    /* slow motion */
    S32    NbFieldsToRepeat;
    U32    NbFieldsRepeated;
    /* Fast forward */
    U32    NbFieldsToSkip = 0;
    U32    NbFieldsSkipped = 0;

    if (VIDTRICK_Data_p->ValidityCheck != VIDTRICK_VALID_HANDLE)
    {
#ifdef TRICKMOD_DEBUG
       TrickModeError();
        TraceBuffer(("Bad Validity ckeck\n\r"));
#endif /* TRICKMOD_DEBUG */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Lock mutex protecting Trick mode private params (it will be released by VIDTRICK_PostVsyncAction) */
    STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p);

    if ((!VIDTRICK_Data_p->Flags.SpeedControlEnabled) || (VIDTRICK_Data_p->MainParams.Speed == 100))
    {
        return(ST_NO_ERROR);
    }

    /* Speed actions are done only on the reference display (but TrickModeParamsMutex_p should be locked anyway) */
    if (!IsReferenceDisplay)
    {
        return(ST_NO_ERROR);
    }

    if (VIDTRICK_Data_p->MainParams.Speed < 0)
    {
#ifdef ST_smoothbackward
        if(  (!VIDTRICK_Data_p->Flags.BackwardToForwardInitializing)     /* Not Backward to Forward mode init phase */
           ||(VIDTRICK_Data_p->Stop.TrickmodIsStopped))                  /* Trickmode is stopped, but needs to continue
                                                                          * display computing because when we have done a
                                                                          * stop_end_of_data, the display will continue after
                                                                          * the trickmode is stopped */
        {
       if (VIDDEC_GetSmoothBackwardCapability(VIDTRICK_Data_p->DecodeHandle) == ST_NO_ERROR)
       {
            /* virtual Time Elapsed since last VSync is speed */
            TimeElapsed = -VIDTRICK_Data_p->MainParams.Speed;

                /* speed action can be done only if Smooth backward initialisation process is up, or if the trickmode is stopped */
                if (  ((!VIDTRICK_Data_p->Flags.SmoothBackwardIsInitializing)
                       &&(VIDTRICK_Data_p->Flags.AtLeastOnePictureHasBeenInserted))
                    ||(VIDTRICK_Data_p->Stop.TrickmodIsStopped))
            {
               ComputeFieldsToSkipOrToRepeat = TRUE;
               /* process smooth backward fluidity flags. */
               /* smooth backward fluidity process has to be set only if too many display queue empty events */
               /* occur a too little time. */
               VIDTRICK_Data_p->BackwardFluidity.VSyncEventNb ++;
               if (   (VIDTRICK_Data_p->BackwardFluidity.DisplayQueueEventEmptyNb > 0)
                   && (VIDTRICK_Data_p->BackwardFluidity.VSyncEventNb > VIDTRICK_VSYNC_EVTS_DQE_MAX_NB))
               {
                   VIDTRICK_Data_p->BackwardFluidity.VSyncEventNb = 0;
                   VIDTRICK_Data_p->BackwardFluidity.DisplayQueueEventEmptyNb --;
               }
            }

            /* Error Recovery */
            if (VIDTRICK_Data_p->PictureUnderDecoding_p != NULL)
            {
                VIDTRICK_Data_p->ErrorRecovery.NbVsyncElapsedSinceLastDecodeHasBeenLaunched++;
            }
            else
            {
                VIDTRICK_Data_p->ErrorRecovery.NbVsyncElapsedWithoutAnyDecode ++;
            }

            if (VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p != NULL)
            {
                VIDTRICK_Data_p->ErrorRecovery.NbVSyncElapsedSinceLastSCDHasBeenLaunched++;
            }
       }
        }
#else
       /* Backward I only  */
       VIDTRICK_Data_p->BackwardIOnly.NbVsyncSinceLastSentDriftEvt ++;
#endif /* ST_smoothbackward */
    }
    else /* Speed >= 0 */
    {
#ifdef ST_smoothbackward
        if (VIDTRICK_Data_p->NegativeToPositiveSpeedParams.ForwardDisplayQueueInsertionIsEnabled)
#endif /* ST_smoothbackward */
        {
            /* virtual Time Elapsed since last VSync is speed */
            TimeElapsed = VIDTRICK_Data_p->MainParams.Speed ;
            ComputeFieldsToSkipOrToRepeat = TRUE;
        }
    }

    if (ComputeFieldsToSkipOrToRepeat)
    {
        /* Compute MainParams.NbFieldsIfPosToSkipIfNegToRepeat and skip fields if necessary */
        VirtualClockCounter = VIDTRICK_Data_p->MainParams.VirtualClockCounter;

        /* Compute slow motion (backward or forward) */
        /* MAJ of Trick Mode Fields to skip or to repeat counter */
        if (TimeElapsed < 100)
        {
            VirtualClockCounter = VirtualClockCounter - 100 + TimeElapsed;
            if (VirtualClockCounter < 0)
            {
                /* the current field displayed has to be displayed one more time */
                VirtualClockCounter += 100;
                VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat --;

#ifdef TRACE_UART_DISPLAY_SKIP
                TraceBuffer(("SlowSpeed: SkipRepeat=%d\r\n", VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat));
#endif
            }
        }
        /* If Fast (forward or backward) */
        else
        {
            /* Compute number of fields to skip */
            NbFieldsToSkip = (TimeElapsed + VirtualClockCounter) / 100 - 1;

            /* MAJ Virtual clock */
            VirtualClockCounter = (TimeElapsed + VirtualClockCounter) % 100;
            if (VirtualClockCounter > TimeElapsed)
            {
                VirtualClockCounter = VirtualClockCounter - TimeElapsed;
            }

            if (NbFieldsToSkip)
            {
                VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat += (S32) NbFieldsToSkip;

#ifdef TRACE_UART_DISPLAY_SKIP
                TraceBuffer(("FastSpeed: SkipRepeat=%d\r\n", VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat));
#endif
            }
        }
        VIDTRICK_Data_p->MainParams.VirtualClockCounter = VirtualClockCounter ;
        /* End of computing MainParams.NbFieldsIfPosToSkipIfNegToRepeat against VirtualClockCounter */

        /* if special case of decode I or IP only in backward, do not skip on BB */
        if((((VIDTRICK_Data_p->DecodedPictures == STVID_DECODED_PICTURES_I)||(VIDTRICK_Data_p->DecodedPictures == STVID_DECODED_PICTURES_IP))
                &&(VIDTRICK_Data_p->MainParams.Speed < 0)&&(VIDTRICK_Data_p->BackwardFluidity.NbFieldsToBeDisplayed>0))&&
                (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat > 0))
        {
            NbFieldsToRepeat = VIDTRICK_Data_p->BackwardFluidity.NbFieldsToBeDisplayed;

            /* Repeat displayed field */
            VIDDISP_RepeatFields(VIDTRICK_Data_p->DisplayHandle, (U32)NbFieldsToRepeat, &NbFieldsRepeated);
            VIDTRICK_Data_p->BackwardFluidity.NbFieldsToBeDisplayed-=NbFieldsRepeated;
            VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat += (S32) NbFieldsRepeated;
        }

        /* If Fields to repeat */
        else if (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat < 0)
        {
            NbFieldsToRepeat = -(VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat);

            /* NbFieldsToRepeat should not be > 1 ! */
            NbFieldsToRepeat = 1;

            /* Repeat displayed field */
            VIDDISP_RepeatFields(VIDTRICK_Data_p->DisplayHandle, (U32)NbFieldsToRepeat, &NbFieldsRepeated);

            VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat += (S32) NbFieldsRepeated;

#ifdef TRACE_UART_DISPLAY_SKIP
            TraceBuffer(("Repeat: SkipRepeat=%d\r\n", VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat));
#endif
        }
        /* if Fields to skip */
        else
        {
            NbFieldsToSkip = (U32)(VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat);
#ifdef ST_smoothbackward
            if (VIDTRICK_Data_p->MainParams.Speed < 0)
            {
                /* Limitation of MainParams.NbFieldsIfPosToSkipIfNegToRepeat because we need at list 2 pictures per buffer to work. */
                /* If it is outside of our limit, we will recover it by asking a spacing buffer.                         */
                if (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat  > VIDTRICK_LEVEL_SPACE_BUFFERS)
                {
                    /* Try to recover during the next flush (by spacing the buffers) the late we have not taken into account. */
                    VIDTRICK_Data_p->Discontinuity.NbFieldsToRecover += VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat - VIDTRICK_LEVEL_SPACE_BUFFERS;

                    /* Limitation of the NbFieldsToSkip in order to avoid 1 picture per buffer (doesn't work). */
                    VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat = VIDTRICK_LEVEL_SPACE_BUFFERS;

#ifdef TRACE_UART_DISPLAY_SKIP
                    TraceBuffer(("Limit: SkipRepeat=%d\r\n", VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat));
#endif
                }

#ifdef OLD_BACKWARD_FLUENT_DISPLAY_PROCESS
                if  (   (NbFieldsToSkip > 0)
                     && (VIDTRICK_Data_p->BackwardFluidity.BackwardFluentDisplayProcessIsRunning))
                {
                    /* if counter of fields to skip is too much, we'll be obliged to skip many */
                    /* pictures in a linear bit buffer and display a few pictures of the same */
                    /* linear bit buffer : to increase fluidity it is better to stop skipping fields */
                    NbFieldsToSkip = 0;
                }
#endif /* OLD_BACKWARD_FLUENT_DISPLAY_PROCESS */
            }
#endif /* ST_smoothbackward */
            if (NbFieldsToSkip != 0)
            {
                VIDDISP_SkipFields(VIDTRICK_Data_p->DisplayHandle, (U32)NbFieldsToSkip, &NbFieldsSkipped, FALSE, VIDDISP_SKIP_STARTING_FROM_CURRENTLY_DISPLAYED_PICTURE);

#ifdef TRICKMOD_DEBUG
                if (NbFieldsSkipped > (U32)NbFieldsToSkip)
                {
                    TrickModeError();
                    TraceBuffer(("Too many Fields skipped!\r\n"));
                }
#endif
                VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat -= (S32) NbFieldsSkipped;

#ifdef TRACE_UART_DISPLAY_SKIP
                TraceBuffer(("ToSkip=%d Skipped=%d SkipRepeat=%d\r\n",
                       NbFieldsToSkip ,
                       NbFieldsSkipped,
                       VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat));
#endif
            }
        }
    }
    /* else */
    /* { */
        /* TraceBuffer(("speed not computed !\r\n")); */
    /* } */

#ifdef ST_smoothbackward
    /* If video is started and not stopped. While Buffer A and B are parsed for the first time, we don't want the task to be waked up. */
    if (   (VIDTRICK_Data_p->Flags.SpeedControlEnabled)
        && (VIDTRICK_Data_p->MainParams.Speed < 100)
        && (VIDDEC_GetSmoothBackwardCapability(VIDTRICK_Data_p->DecodeHandle) == ST_NO_ERROR))
    {
        VIDTRICK_Data_p->NewStartCodeSearch.MustWaitBecauseOfVSync = TRUE;
    }
#endif

    return(ST_NO_ERROR);

} /* end of VIDTRICK_VsyncActionSpeed() function */

/*******************************************************************************
Name        : VIDTRICK_PostVsyncAction
Description : Function which has to be called every Vsync occurence. It computes the number of fileds to
              be skipped or repeated, Skipd or Repeats fields in display module in order to reach teh required speed.
Parameters  :
Assumptions : None
Limitations : None
Returns     : None
*******************************************************************************/
ST_ErrorCode_t VIDTRICK_PostVsyncAction(const VIDTRICK_Handle_t TrickModeHandle, BOOL IsReferenceDisplay)
{
    VIDTRICK_Data_t  * VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    UNUSED_PARAMETER(IsReferenceDisplay);

    if (VIDTRICK_Data_p->ValidityCheck != VIDTRICK_VALID_HANDLE)
    {
#ifdef TRICKMOD_DEBUG
       TrickModeError();
       TraceBuffer(("Bad Validity ckeck\n\r"));
#endif /* TRICKMOD_DEBUG */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* unlock mutex protecting Trick mode private params */
    STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);

    /* Speed actions are done only on the reference display
    if (!IsReferenceDisplay)
    {
        return(ST_NO_ERROR);
    }
    *** Not needed for the moment because no action is done on Post VSync Event ***/

    return(ST_NO_ERROR);
} /* end of VIDTRICK_PostVsyncAction() function */

#ifdef ST_smoothbackward
/*******************************************************************************
Name        : AllocateLinearBitBuffers
Description : Allocated two linear bit buffers to store MPEG stream
Parameters  :
Assumptions :
Limitations :
Returns     : ST_ERROR_NO_MEMORY, ST_NO_ERROR
*******************************************************************************/
static ST_ErrorCode_t AllocateLinearBitBuffers(VIDTRICK_Handle_t TrickModeHandle)
{
    VIDTRICK_Data_t                     *VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;
    VIDDEC_CircularBitbufferParams_t    CircularBitbufferParams;

    /* Get the circular bitbuffer address and size */
    /*ErrorCode =*/ VIDDEC_GetCircularBitBufferParams(VIDTRICK_Data_p->DecodeHandle, &CircularBitbufferParams);
    /* Return params of the buffer A */
    VIDTRICK_Data_p->LinearBitBuffers.BufferA.WholeBufferStartAddress_p = (void*)
                                                                          (( (U32)CircularBitbufferParams.Address
                                                                           + (GetMaxAlignmentBetweenCDFifoAndSCD(VIDTRICK_Data_p->DecodeHandle) - 1))
                                                                          & (~(GetMaxAlignmentBetweenCDFifoAndSCD(VIDTRICK_Data_p->DecodeHandle) - 1)));
    VIDTRICK_Data_p->LinearBitBuffers.BufferA.WholeBufferEndAddress_p   = (void *)
                                                                          ( ( ( (U32)CircularBitbufferParams.Address
                                                                              + ((CircularBitbufferParams.Size + 1)/ 2))
                                                                            & (~(GetMaxAlignmentBetweenCDFifoAndSCD(VIDTRICK_Data_p->DecodeHandle) - 1)))
                                                                          - 1 );
    /* Return params of the buffer B */
    VIDTRICK_Data_p->LinearBitBuffers.BufferB.WholeBufferStartAddress_p = (void *)
                                                                          ( ( (U32)CircularBitbufferParams.Address
                                                                            + ((CircularBitbufferParams.Size + 1)/ 2))
                                                                          & (~(GetMaxAlignmentBetweenCDFifoAndSCD(VIDTRICK_Data_p->DecodeHandle) - 1)));
    VIDTRICK_Data_p->LinearBitBuffers.BufferB.WholeBufferEndAddress_p   = (void *)
                                                                          ( ( ( (U32)CircularBitbufferParams.Address
                                                                              + CircularBitbufferParams.Size)
                                                                            & (~(GetMaxAlignmentBetweenCDFifoAndSCD(VIDTRICK_Data_p->DecodeHandle) - 1)))
                                                                          - 1);
    /* A & B bufers have been allocated, the first bytes of both buffers have to be filled with a sequence start code */
    /* to allow start code dectector to find it and so all the other start codes every new parsing of one of the two buffer */
    /* But CD fifo registers are ? bits aligned, stuffing bytes have to be put in the beginning of this buffer */
    PutSequenceStartCodeAndComputeLinearBitBuffers(TrickModeHandle, &(VIDTRICK_Data_p->LinearBitBuffers.BufferA));
    PutSequenceStartCodeAndComputeLinearBitBuffers(TrickModeHandle, &(VIDTRICK_Data_p->LinearBitBuffers.BufferB));

    return(ST_NO_ERROR);
} /* End of AllocateLinearBitBuffers() function */

/*******************************************************************************
Name        : AskForBackwardDrift
Description : Function requiring HDD application to drift its position backward. this function is called only
              when the application sets a positive speed after having set a negative speed : In order to ensure

Parameters  : TrickModeHandle.
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void AskForBackwardDrift(const VIDTRICK_Handle_t TrickModeHandle)
{
    VIDTRICK_Data_t             * VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;
    STVID_SpeedDriftThreshold_t   SpeedDriftThresholdData;
    ST_ErrorCode_t                ErrorCode = ST_NO_ERROR;
    U32                           BitRateValue;

    ErrorCode = GetBitRateValue(TrickModeHandle, &BitRateValue);
    if ((ErrorCode == ST_NO_ERROR) && (BitRateValue != 0))
    {
        /* BitRateValue has to be * 400/8 to have bytes/s */
        /* has to be * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION to have tile in units of 90Khz as required */
        SpeedDriftThresholdData.BitRateValue = BitRateValue;
        /* Drift time is the jump duration to ask to go back to the beginning of the last injection*/
        if(VIDTRICK_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p != NULL)
        {
            SpeedDriftThresholdData.DriftTime =  - VIDTRICK_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p->AssociatedLinearBitBuffer_p->InjectionDuration;
        }
        else
        {
            SpeedDriftThresholdData.DriftTime = - (VIDTRICK_MAX_BACKWARD_DRIFT_PICTURE_NB * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) / VIDTRICK_DEFAULT_FRAME_RATE;
        }

#ifdef TRACE_UART_MAIN
        TraceBuffer(("DriftTime =%d\r\n", SpeedDriftThresholdData.DriftTime));
#endif

        STEVT_Notify(VIDTRICK_Data_p->EventsHandle, VIDTRICK_Data_p->RegisteredEventsID[VIDTRICK_SPEED_DRIFT_THRESHOLD_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST (&SpeedDriftThresholdData));
        /* Ask the decode to notify this event by itself */

#ifdef TRACE_UART_DISPLAY_SKIP
        TraceBuffer(("**ThrEVT BR=%d DriftTime=%d **\r\n",
               SpeedDriftThresholdData.BitRateValue,
               SpeedDriftThresholdData.DriftTime ));
#endif
    }
}

/*******************************************************************************
Name        : AskForLinearBitBufferFlush
Description : Ask the decode part to send Data Underfloaw event
Parameters  : TrickModeHandle, BufferToBeFilled, ReFlush : if FALSE, same tansfert as the last done is required.
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void AskForLinearBitBufferFlush(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * BufferToBeFilled_p, const VIDTRICK_FushBitBufferManner_t FlushManner)
{
    VIDTRICK_Data_t              * VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;
    STVID_DataUnderflow_t          DataUnderflowData;
    U32                            MaxLinearBBFContenanceDuration;
    U32                 i;

    /* a Stop has been required. All Linear Bit Buffer Flushes have to be forbidden */
    if (!VIDTRICK_Data_p->Stop.LinearBitBufferFlushRequirementAvailable)
    {
        return;
    }

    /* The STi5508 and STi5518 supports the following trick-mode features: */
    /* - Programmable video CD (Compressed Data) FIFO pointer */
    /* - Programmable SCD (Start Code Detector) pointer */
    /* - Programmable VLD (Variable Length Decoder) pointer */
    /* They have to be Initialized if necessary */

#ifdef TRICKMOD_DEBUG
    /* Problem : apparently a linear bit buffer requirement is launching start code detector. */
    /* It shouldn't ! And if there is a buffer under parsing (which shouldn't happen) it is a big problem */
    if (VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p != NULL)
    {
        TrickModeError();
        TraceBuffer(("Asking For a Linear Bit Buffer Flush and a buffer is under parsing !\n\r"));
    }
#endif /* TRICKMOD_DEBUG */

    if (   (BufferToBeFilled_p == NULL)
        || (BufferToBeFilled_p->ValidDataBufferStartAddress_p == NULL))
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("Critical ERROR: NULL pointer !!!!\n\r"));
#endif /* TRICKMOD_DEBUG */
        return;
    }

    /* Set CD fifo for buffer to be filled  */
    VIDDEC_SetDecodeBitBufferInLinearMode(VIDTRICK_Data_p->DecodeHandle, BufferToBeFilled_p->ValidDataBufferStartAddress_p, BufferToBeFilled_p->TransferDataMaxSize);

    /* Write the Sequence Start Code */
    VIDDEC_WriteStartCode(VIDTRICK_Data_p->DecodeHandle, SEQUENCE_HEADER_CODE, (void*)(BufferToBeFilled_p->WholeBufferStartAddress_p));

    /* Use the bitrate value computed in the last linear bitbuffer */
#ifdef OLD_OVERLAP_COMPUTATION
    GetBitRateValue(TrickModeHandle, &DataUnderflowData.BitRateValue);
#else /* !OLD_OVERLAP_COMPUTATION */
    DataUnderflowData.BitRateValue      = BufferToBeFilled_p->AssociatedLinearBitBuffer_p->BeginningOfInjectionBitrateValue;
#endif /* OLD_OVERLAP_COMPUTATION */
    DataUnderflowData.BitBufferFreeSize = BufferToBeFilled_p->TransferDataMaxSize;
    /* Take care to not have pictures in the bitbuffer more than the number of entries in the pictures table */
    MaxLinearBBFContenanceDuration      = (VIDTRICK_BACKWARD_PICTURES_NB_BY_BUFFER * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) / ((VIDTRICK_Data_p->MainParams.FrameRateValue + 999) / 1000);

    /* BitRateValue has to be * 400/8 to have bytes/s */
    /* has to be * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION to have tile in units of 90Khz as required */
    if (DataUnderflowData.BitRateValue == 0)
    {
        DataUnderflowData.BitRateValue = VIDTRICK_DEFAULT_BITRATE;
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("Bit rate should not be 0 \n\r"));
#endif

    }
    DataUnderflowData.RequiredDuration = Min(((BufferToBeFilled_p->TransferDataMaxSize / DataUnderflowData.BitRateValue) * STVID_DURATION_COMPUTE_FACTOR) ,  MaxLinearBBFContenanceDuration);
    switch (FlushManner)
    {
        case VIDTRICK_FLUSH_WITH_NORMAL_OVERLAP :
#ifdef TRACE_UART_DISPLAY_SKIP
            TraceBuffer(("**undflow: NORMAL,\r\n"));
#endif
#ifdef OLD_OVERLAP_COMPUTATION
            DataUnderflowData.RequiredTimeJump = - 2 * DataUnderflowData.RequiredDuration + VIDTRICK_Data_p->Discontinuity.OverlapDuration;
#else /* !OLD_OVERLAP_COMPUTATION */
            DataUnderflowData.RequiredTimeJump = - BufferToBeFilled_p->AssociatedLinearBitBuffer_p->InjectionDuration - DataUnderflowData.RequiredDuration + VIDTRICK_Data_p->Discontinuity.OverlapDuration;
            VIDTRICK_Data_p->SpeedAccuracy.PreviousSkip = ((- BufferToBeFilled_p->AssociatedLinearBitBuffer_p->InjectionDuration - DataUnderflowData.RequiredTimeJump) / STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) * ((VIDTRICK_Data_p->MainParams.FrameRateValue + 999) / 1000) * 2;
            if(BufferToBeFilled_p->AssociatedLinearBitBuffer_p->DiscontinuityWithPreviousBuffer)
            {
                DataUnderflowData.RequiredTimeJump += (VIDTRICK_Data_p->SpeedAccuracy.NBFieldsLate * (STVID_MPEG_CLOCKS_ONE_SECOND_DURATION / (((VIDTRICK_Data_p->MainParams.FrameRateValue + 999) / 1000) * 2)));
                VIDTRICK_Data_p->SpeedAccuracy.NBFieldsLate = 0;
            }

#endif /* OLD_OVERLAP_COMPUTATION */
#ifdef TRACE_UART_DISPLAY_SKIP
            TraceBuffer(("Jump -%lu-%lu+%lu\r\n",
                BufferToBeFilled_p->AssociatedLinearBitBuffer_p->InjectionDuration,
                DataUnderflowData.RequiredDuration,
                VIDTRICK_Data_p->Discontinuity.OverlapDuration));
#endif
            DataUnderflowData.TransferRelatedToPrevious = (VIDTRICK_Data_p->Discontinuity.OverlapDuration > 0) ? TRUE : FALSE;
            break;

        case VIDTRICK_FLUSH_BBFSIZE_BACKWARD_JUMP :
#ifdef TRACE_UART_DISPLAY_SKIP
            TraceBuffer(("**undflow: BBFSIZE, "));
#endif
            DataUnderflowData.RequiredTimeJump = - (DataUnderflowData.RequiredDuration);
            VIDTRICK_Data_p->SpeedAccuracy.PreviousSkip = (DataUnderflowData.RequiredDuration / STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) * ((VIDTRICK_Data_p->MainParams.FrameRateValue + 999) / 1000) * 2;
            DataUnderflowData.TransferRelatedToPrevious = TRUE;
            break;

        case VIDTRICK_FLUSH_BECAUSE_OF_NO_I_PICTURE :
#ifdef TRACE_UART_DISPLAY_SKIP
            TraceBuffer(("**undflow: NO_I_PIC,\r\n"));
#endif
            DataUnderflowData.RequiredTimeJump = - (3 * DataUnderflowData.RequiredDuration / 2) + VIDTRICK_Data_p->Discontinuity.OverlapDuration;
            VIDTRICK_Data_p->SpeedAccuracy.PreviousSkip = ((- BufferToBeFilled_p->AssociatedLinearBitBuffer_p->InjectionDuration - DataUnderflowData.RequiredTimeJump) / STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) * ((VIDTRICK_Data_p->MainParams.FrameRateValue + 999) / 1000) * 2;
            if(BufferToBeFilled_p->AssociatedLinearBitBuffer_p->DiscontinuityWithPreviousBuffer)
            {
                DataUnderflowData.RequiredTimeJump += (VIDTRICK_Data_p->SpeedAccuracy.NBFieldsLate * (STVID_MPEG_CLOCKS_ONE_SECOND_DURATION / (((VIDTRICK_Data_p->MainParams.FrameRateValue + 999) / 1000) * 2)));
                VIDTRICK_Data_p->SpeedAccuracy.NBFieldsLate = 0;
            }
            DataUnderflowData.TransferRelatedToPrevious = FALSE;
            break;

        default :
#ifdef TRACE_UART_DISPLAY_SKIP
            TraceBuffer(("**undflow: UNKNOWN,\r\n"));
#endif
            DataUnderflowData.RequiredDuration = 0;
            DataUnderflowData.RequiredTimeJump = 0;
            DataUnderflowData.TransferRelatedToPrevious = FALSE;
            break;
    }

    /* Buffer is now under transfer */
    VIDTRICK_Data_p->LinearBitBuffers.BufferUnderTransfer_p = BufferToBeFilled_p;

    /* next to be transfered is the associated buffer */
    VIDTRICK_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p = BufferToBeFilled_p->AssociatedLinearBitBuffer_p;

    /* reset flags */
    BufferToBeFilled_p->FirstPictureOfLBBFFound = FALSE;
    BufferToBeFilled_p->NbFieldsInside = 0;
    VIDTRICK_Data_p->NegativeToPositiveSpeedParams.FirstGOPFound = FALSE;
    VIDTRICK_Data_p->NegativeToPositiveSpeedParams.FirstNullTempRefPicture_p = NULL;
    VIDTRICK_Data_p->MatchingPicture.MatchingPicture_p = NULL;

#ifdef TRACE_UART_DEBUG
    TraceBuffer(("Reset Bit Buffer content"));
#endif
    for (i = 0 ; i < BufferToBeFilled_p->NbPicturesInside ; i++)
    {
        if (BufferToBeFilled_p->PictureInfoTableStartAddress_p[i].PictureBuffer_p != NULL)
        {
            ReleasePictureInfoElement(TrickModeHandle, &(BufferToBeFilled_p->PictureInfoTableStartAddress_p[i]) );
        }
    }

    /* Ask the decode to notify this event by itself */
#if defined(TRACE_UART_DISPLAY_SKIP) || defined(TRACE_UART_EXT_TEMP_REF)
    TraceBuffer(("BR=%d Jump=%d Dur=%d Ovlap=%d Dst=0x%.8x\r\n",
            DataUnderflowData.BitRateValue,
            DataUnderflowData.RequiredTimeJump,
            DataUnderflowData.RequiredDuration,
            VIDTRICK_Data_p->Discontinuity.OverlapDuration,
            BufferToBeFilled_p->WholeBufferStartAddress_p ));
#endif
    VIDDEC_NotifyDecodeEvent(VIDTRICK_Data_p->DecodeHandle, STVID_DATA_UNDERFLOW_EVT, (const void *) &DataUnderflowData);

    /* store date in order to know, for BackwardFluidity feature */
    /* how long will last flush + parse  of the linear bit buffer */
    VIDTRICK_Data_p->BackwardFluidity.TransferBeginDate = time_now();
} /* end of AskForLinearBitBufferFlush() function */

/*******************************************************************************
Name        : NewStartCodeSearchIsRequired
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void NewStartCodeSearchIsRequired(const VIDTRICK_Handle_t TrickModeHandle, const VIDDEC_StartCodeSearchMode_t SearchMode, const BOOL FirstSliceDetect, void * const SearchAddress_p)
{
    VIDTRICK_Data_t * VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;

    /* error recovery reset */
    VIDTRICK_Data_p->ErrorRecovery.NbVSyncElapsedSinceLastSCDHasBeenLaunched = 0;

    VIDDEC_AskToSearchNextStartCode(VIDTRICK_Data_p->DecodeHandle, SearchMode, FirstSliceDetect, SearchAddress_p);
    if (SearchMode == VIDDEC_START_CODE_SEARCH_MODE_FROM_ADDRESS)
    {
        /* bit rate eval */
        VIDDEC_EnableBitRateEvaluating(VIDTRICK_Data_p->DecodeHandle);
    }
} /* end of NewStartCodeSearchIsRequired() function */

/*******************************************************************************
Name        : CheckIfDisplayHasToBeEnabled
Description : this function is called if a postive speed has just been set after a negative one. Depending
              of the chronological position of the picture, it allows the display to be enabled  in order
              to start the display from the last displayed picture in backward mode.
Parameters  :
Assumptions :
Limitations :
Returns     : TRUE, if the searched picture was found in the current buffer
*******************************************************************************/
static BOOL CheckIfDisplayHasToBeEnabled(const VIDTRICK_Handle_t TrickModeHandle, STVID_PictureInfos_t * const PictureInfos_p, const U16 TemporalReference, const U16 VbvDelay)
{
    VIDTRICK_Data_t  * VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;
    U32                ToBeDecodedPictureTimeCode;

#ifdef TRACE_UART_SPEEDSWITCH
    TraceBuffer(("Chk: TimeCod=%.2x %.2x %.2x %.2x TemplRef= %d Vbv=%d  ",
            PictureInfos_p->VideoParams.TimeCode.Hours,
            PictureInfos_p->VideoParams.TimeCode.Minutes,
            PictureInfos_p->VideoParams.TimeCode.Seconds,
            PictureInfos_p->VideoParams.TimeCode.Frames,
            TemporalReference,
            VbvDelay
            ));
#endif

    ToBeDecodedPictureTimeCode =  (((U32)PictureInfos_p->VideoParams.TimeCode.Hours)   << 24)
                                | (((U32)PictureInfos_p->VideoParams.TimeCode.Minutes) << 16)
                                | (((U32)PictureInfos_p->VideoParams.TimeCode.Seconds) << 8 )
                                | (((U32)PictureInfos_p->VideoParams.TimeCode.Frames));
    if (   (ToBeDecodedPictureTimeCode == VIDTRICK_Data_p->NegativeToPositiveSpeedParams.LastBackwardDisplayedPictureTimeCode)
        && (TemporalReference == VIDTRICK_Data_p->NegativeToPositiveSpeedParams.LastBackwardDisplayedPictureTemporalReference)
        && (VbvDelay == VIDTRICK_Data_p->NegativeToPositiveSpeedParams.LastBackwardDisplayedPictureVbvDelay))
    {
        /* unlock display update */
        VIDTRICK_Data_p->NegativeToPositiveSpeedParams.ForwardDisplayQueueInsertionIsEnabled = TRUE;
        VIDTRICK_Data_p->Flags.BackwardToForwardInitializing = FALSE;
#ifdef TRACE_UART_SPEEDSWITCH
        TraceBuffer((" -> FDis OK\r\n"));
#endif
        return(TRUE);
    }

    /* If the picture we are searching for is located before the current
     * picture, so we won't be able to restart from it, so unlock the display from now. */
    if ( ToBeDecodedPictureTimeCode > VIDTRICK_Data_p->NegativeToPositiveSpeedParams.LastBackwardDisplayedPictureTimeCode)
    {
        /* unlock display update */
        VIDTRICK_Data_p->NegativeToPositiveSpeedParams.ForwardDisplayQueueInsertionIsEnabled = TRUE;
        VIDTRICK_Data_p->Flags.BackwardToForwardInitializing = FALSE;
#ifdef TRACE_UART_SPEEDSWITCH
        TraceBuffer((" -> FDis KO : Bigger timecode found !\r\n"));
#endif
        return(TRUE);
    }

    /* Don't search undefinetly for the picture to restart from.
     * Prefer to start the display, after a few tries although the same
     * picture wasn't found. */
    if (VIDTRICK_Data_p->NegativeToPositiveSpeedParams.MatchingRecognitionTryNumber >= VIDTRICK_MAX_UNSUCCESSFULL_PICTURE_RECOGNITION_TRY)
    {
        /* unlock display update */
        VIDTRICK_Data_p->NegativeToPositiveSpeedParams.ForwardDisplayQueueInsertionIsEnabled = TRUE;
        VIDTRICK_Data_p->Flags.BackwardToForwardInitializing = FALSE;
#ifdef TRACE_UART_SPEEDSWITCH
        TraceBuffer((" -> FDis KO: max try reached !!!\r\n"));
#endif
        return(TRUE);
    }

#ifdef TRACE_UART_SPEEDSWITCH
    TraceBuffer((" -> NoDis \r\n"));
#endif

    VIDTRICK_Data_p->NegativeToPositiveSpeedParams.MatchingRecognitionTryNumber ++;
    return(FALSE);
}

/*******************************************************************************
Name        : CheckIfPictureCanBeDisplayed
Description : This function is called if the display has just been enabled after changing direction
              from BW to FW . It checks that the new requested for decode picture can be displayed or not
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static BOOL CheckIfPictureCanBeDisplayed(const VIDTRICK_Handle_t TrickModeHandle, STVID_PictureInfos_t * const PictureInfos_p)
{
    VIDTRICK_Data_t  * VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;

    /* - If we restart with a B frame all next frames can be inserted in display queue and they will be surely displayed in the
     * correct order
     * - If we restart with a reference frame we don't allow B frames to be inserted in display queue until we reach the next
     * reference picture */

    if ((VIDTRICK_Data_p->NegativeToPositiveSpeedParams.NextReferenceFound)||
        ((VIDTRICK_Data_p->MainParams.Speed >= 0)&&
         ((VIDTRICK_Data_p->NegativeToPositiveSpeedParams.MPEGFrame == STVID_MPEG_FRAME_B)||
          ((VIDTRICK_Data_p->NegativeToPositiveSpeedParams.MPEGFrame != STVID_MPEG_FRAME_B)&&
           (PictureInfos_p->VideoParams.MPEGFrame != STVID_MPEG_FRAME_B)
       ))))
    {
        if (PictureInfos_p->VideoParams.MPEGFrame != STVID_MPEG_FRAME_B)
        {
            VIDTRICK_Data_p->NegativeToPositiveSpeedParams.NextReferenceFound = TRUE;
        }
        return(TRUE);
    }

#ifdef TRACE_UART_SPEEDSWITCH
/*    TraceBuffer((" -> NoDis \r\n"));*/
#endif

    return(FALSE);
}


/*******************************************************************************
Name        : CheckIfPictureNotLinkedToLinearBuffer
Description : Detects that a given picture is still linked or not with the other linear bit buffer
Parameters  :
Assumptions :
Limitations :
Returns     : TRUE if picture is only dependent of pictures contained in the current linear bit buffer (BB)
              FALSE if picture is needed to decode pictures in both linear bit buffer (for example
              P picture at the end of first BB and B pictures depending to this P in the second BB)
*******************************************************************************/
static BOOL CheckIfPictureNotLinkedToLinearBuffer(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t const * CurrentlyDisplayedPicture_p)
{
    VIDTRICK_PictureInfo_t  * NextFrameOrFirstField_p = NULL;
    VIDTRICK_PictureInfo_t  * FirstDisplayablePicture_p = NULL;
    BOOL                      PictureNotLinked = FALSE;         /* By default, picture is linked to other linear bit buffer */

    /* if B picture , no more picture of associated buffer can have a extended temp ref before this B picture */
    /* even if its second field picture is the first picture of the associated buffer !!! */
    if (!(PictureMpegTypeIsReference(CurrentlyDisplayedPicture_p)))
    {
        PictureNotLinked = TRUE;
    }
    else
    {
        if (!(HasPictureASecondField(CurrentlyDisplayedPicture_p)))
        {
            NextFrameOrFirstField_p = CurrentlyDisplayedPicture_p->ForwardOrNextReferencePicture_p;
        }
        else
        {
            if (CurrentlyDisplayedPicture_p->SecondFieldPicture_p != NULL)
            {
                NextFrameOrFirstField_p = CurrentlyDisplayedPicture_p->SecondFieldPicture_p->ForwardOrNextReferencePicture_p;
            }
            /* else, NextFrameOrFirstField_p is NULL because of its initial value */
        }

        /* if no forward, then currently displayed is the last reference of the buffer */
        /* and its buffer has no continuity with the associated */
        if (NextFrameOrFirstField_p == NULL)
        {
            PictureNotLinked = TRUE;
        }
        else /* if currently and its forward are in the same linear bit buffer, their associated is unuseful */
        if (CurrentlyDisplayedPicture_p->LinearBitBufferStoredInfo_p == NextFrameOrFirstField_p->LinearBitBufferStoredInfo_p)
        {
            PictureNotLinked = TRUE;
        }
        else /* test if currently displayed picture is at the same time last and first displayabale picture */
             /* of its bit buffer */
        {
            /* if currently displayed picture is the only picture choosen to be displayed of its bit buffer */
            /* its associated linear bit buffer has to be re-flushed */

            if (   (CurrentlyDisplayedPicture_p == CurrentlyDisplayedPicture_p->LinearBitBufferStoredInfo_p->LastDisplayablePicture_p)
                || (   (IsPictureSecondField(CurrentlyDisplayedPicture_p))
                    && (CurrentlyDisplayedPicture_p->BackwardOrLastBeforeIReferencePicture_p == CurrentlyDisplayedPicture_p->LinearBitBufferStoredInfo_p->LastDisplayablePicture_p)))
            {
                FindFirstDisplayablePictureOfLinearBitBuffer(TrickModeHandle, CurrentlyDisplayedPicture_p->LinearBitBufferStoredInfo_p, &FirstDisplayablePicture_p);
                if (FirstDisplayablePicture_p == CurrentlyDisplayedPicture_p)
                {
                    PictureNotLinked = TRUE;
                }
            }
        }
    }

    return PictureNotLinked;
} /* CheckIfPictureNotLinkedToLinearBuffer */

/*******************************************************************************
Name        : CheckIfLinearBufferHasToBeFlushed
Description : Detects that a linear bit buffer is unuseful and has to be flushed. this tests is done each time a picture is out of display
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void CheckIfLinearBufferHasToBeFlushed(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t const * CurrentlyDisplayedPicture_p)
{
    VIDTRICK_Data_t         * VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;
    BOOL                      BitBufferHasToBeFlushed = FALSE;

    /* CheckIfLinearBufferHasToBeFlushed */
    /* if buffer of currently displayed id the associated one of the next one to be transfered, study.. */
    if (   (VIDTRICK_Data_p->LinearBitBuffers.BufferUnderTransfer_p == NULL)
        && (VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p == NULL)
        && (CurrentlyDisplayedPicture_p->LinearBitBufferStoredInfo_p != VIDTRICK_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p))
    {
        BitBufferHasToBeFlushed = CheckIfPictureNotLinkedToLinearBuffer(TrickModeHandle, CurrentlyDisplayedPicture_p);
    }

    /* if just out of diplay picture come from buffer to be transfered, and that next to display is in the other buffer */
    if (BitBufferHasToBeFlushed)
    {
        /* Affect the buffer spacing according to the speed. Sets Discontinutiy flag if necessary. */
        AffectTheBufferSpacing(TrickModeHandle, VIDTRICK_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p);

        /* Unlink all dependencies pictures of buffer to be flushed. */
        UnlinkBufferToBeFLushedPictures(TrickModeHandle, VIDTRICK_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p);

        /* Ask for flush. */
        AskForLinearBitBufferFlush(TrickModeHandle, VIDTRICK_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p, VIDTRICK_FLUSH_WITH_NORMAL_OVERLAP);
    }
} /* CheckIfLinearBufferHasToBeFlushed */

/*******************************************************************************
Name        : ComputeMaxJumpeableFields
Description : Computes the MaxJumpeableFields which should grow if the decreases.
              it used to display backward fluently, but with a display fluidity depending of the required speed.
Parameters  : Speed
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void ComputeMaxJumpeableFields(const VIDTRICK_Handle_t TrickModeHandle, const S32 Speed, const U32 NumberOfPicturesInTheLinearBitbuffer)
{
#ifndef OLD_MAX_JUMP_FIELDS
    VIDTRICK_Data_t    * VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    U32 MaxJump1, MaxJump2;

    MaxJump1 = (- Speed) / 100;

    /* Try to have at least a minimal number of pictures displayed per new linear bitbuffer */
    MaxJump2 = 2 * (NumberOfPicturesInTheLinearBitbuffer / (((VIDTRICK_Data_p->MainParams.FrameRateValue + 999) / 1000))) ;

    VIDTRICK_Data_p->BackwardFluidity.MaxJumpeableFields = Min(MaxJump1, MaxJump2);
#if 0
    TraceBuffer(("MaxJump1=%lu MaxJump2=%lu MaxJump=%lu\r\n",
            MaxJump1,
            MaxJump2,
            VIDTRICK_Data_p->BackwardFluidity.MaxJumpeableFields));
#endif
#else /* !OLD_MAX_JUMP_FIELDS */
    VIDTRICK_Data_t    * VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    U32   CoefDirY;
    U32   CoefDirX;

    CoefDirY = VIDTRICK_MAX_JUMPEABLE_FIELDS_MAX - VIDTRICK_MAX_JUMPEABLE_FIELDS_MIN;
    CoefDirX = VIDTRICK_MAX_JUMPEABLE_FIELDS_SPEED_MAX - VIDTRICK_MAX_JUMPEABLE_FIELDS_SPEED_MIN;

    if (Speed > (-VIDTRICK_MAX_JUMPEABLE_FIELDS_SPEED_MIN))
    {
        VIDTRICK_Data_p->BackwardFluidity.MaxJumpeableFields = VIDTRICK_MAX_JUMPEABLE_UNDER_200;
    }
    else if (Speed < (-VIDTRICK_MAX_JUMPEABLE_FIELDS_SPEED_MAX))
    {
        VIDTRICK_Data_p->BackwardFluidity.MaxJumpeableFields = VIDTRICK_MAX_JUMPEABLE_FIELDS_MAX;
    }
    else
    {
        if (CoefDirX != 0)
        {
            VIDTRICK_Data_p->BackwardFluidity.MaxJumpeableFields = CoefDirY * ((-Speed) - VIDTRICK_MAX_JUMPEABLE_FIELDS_SPEED_MIN) / CoefDirX + VIDTRICK_MAX_JUMPEABLE_FIELDS_MIN;
        }
    }
#endif /* OLD_MAX_JUMP_FIELDS */

#ifdef TRACE_UART_DISPLAY_SKIP
    TraceBuffer(("MaxJumpeableFields=%d\r\n", VIDTRICK_Data_p->BackwardFluidity.MaxJumpeableFields));
#endif
}

/*******************************************************************************
Name        : ForwardModeInit
Description : Stops Smooth Trick mode management and gives decode and buffer management
              commands to reinitialize frame buffers and to be ready to decode forward.
              It is a synchronous function so it should be called only from the VID_TRIC's task.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t ForwardModeInit(const VIDTRICK_Handle_t TrickModeHandle)
{
    VIDTRICK_Data_t  * VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    ST_ErrorCode_t     ErrorCode = ST_NO_ERROR;

#ifdef TRACE_UART_DEBUG
    TraceBuffer(("---ForwardModeInit---\r\n"));
#endif

    /* Stop Smooth backward processing. */
    SmoothBackwardInitStructure(TrickModeHandle);

    if(VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0] != NULL)
    {
        /* If the backward->forward speed switch was done while the driver is running,
         * store last backward displayed picture params in order to enable display from the same picture
         * in order to restart the display again from the same picture */
        VIDTRICK_Data_p->NegativeToPositiveSpeedParams.LastBackwardDisplayedPictureTimeCode =
              (((U32)VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0]->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode.Hours)   << 24)
            | (((U32)VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0]->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode.Minutes) << 16)
            | (((U32)VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0]->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode.Seconds) << 8 )
            | (((U32)VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0]->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode.Frames));

        VIDTRICK_Data_p->NegativeToPositiveSpeedParams.LastBackwardDisplayedPictureTemporalReference =
                 VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0]->PictureInfoFromDecode.pTemporalReference;
        VIDTRICK_Data_p->NegativeToPositiveSpeedParams.LastBackwardDisplayedPictureVbvDelay =
                VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0]->PictureInfoFromDecode.StreamInfoForDecode.Picture.vbv_delay;
            /* Picture insertion in the display are not allowed until the first picture to display is found */
        VIDTRICK_Data_p->NegativeToPositiveSpeedParams.ForwardDisplayQueueInsertionIsEnabled = FALSE;
        VIDTRICK_Data_p->NegativeToPositiveSpeedParams.MatchingRecognitionTryNumber = 0;
        VIDTRICK_Data_p->NegativeToPositiveSpeedParams.NextReferenceFound = FALSE;
        VIDTRICK_Data_p->NegativeToPositiveSpeedParams.MPEGFrame = VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0]->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame;

#ifdef TRACE_UART_SPEEDSWITCH
        TraceBuffer(("Last Displayed: TimeCod=0x%.8x, TmplRef=%d, Vbv=%d\r\n",
            VIDTRICK_Data_p->NegativeToPositiveSpeedParams.LastBackwardDisplayedPictureTimeCode,
            VIDTRICK_Data_p->NegativeToPositiveSpeedParams.LastBackwardDisplayedPictureTemporalReference,
            VIDTRICK_Data_p->NegativeToPositiveSpeedParams.LastBackwardDisplayedPictureVbvDelay));
#endif
    }
    else
    {
        /* Don't try to restart on the picture that is already on the display
         * because there's no picture on display ! (This is not an error but it can happen) */
        VIDTRICK_Data_p->NegativeToPositiveSpeedParams.ForwardDisplayQueueInsertionIsEnabled = TRUE;
    }

    /* is a positive speed is required by appli, the HDD manager status may be at the beginning of */
    /* a stream and linear bit buffers flush request may be pending, Consequently */
    /* SmoothBackwardIsInitializing flag may be TRUE */
    VIDTRICK_Data_p->Flags.SmoothBackwardIsInitializing = FALSE;

    /* init fast forward speed flags */
    VIDTRICK_Data_p->Flags.MustSkipAllPPictures = FALSE;

    /* init forward discontinuity flag */
    VIDTRICK_Data_p->ForwardInjectionDiscontinuity = TRUE;

    /* BackwardToForwardInitializing is finished */
    VIDTRICK_Data_p->Flags.BackwardToForwardInitializing = FALSE;

    /* Reset reference pictures and free the not displayed picture buffers */
    ResetReferencesAndFreeNotDisplayedPictures(TrickModeHandle);

#ifdef TRICKMOD_DEBUG
    /* Dump the frame buffers status in order to check that no FB remains locked */
    TraceBuffer(("\r\nFB status when leaving BW mode:\r\n"));
    VIDBUFF_PrintPictureBuffersStatus(VIDTRICK_Data_p->BufferManagerHandle);
#endif

    /* Init Control Mode back to automatic mode */
    /* Trickmode leaves masterchip back to decoder */
    VIDDEC_SetControlMode(VIDTRICK_Data_p->DecodeHandle, VIDDEC_AUTOMATIC_CONTROL_MODE);

    return(ErrorCode);
}

/*******************************************************************************
Name        : ForwardModeRequested
Description : This function informs VID_TRIC that it should stop and switch to forward mode.
              This operation should be done in VID_TRIC context in order to ensure that no decoding
              occurs while we stop VID_TRIC.
Parameters  : Video decode handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
static ST_ErrorCode_t ForwardModeRequested(const VIDTRICK_Handle_t TrickModeHandle)
{
    VIDTRICK_Data_t  * VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    ST_ErrorCode_t     ErrorCode = ST_NO_ERROR;

#ifdef TRACE_UART_DEBUG
    TraceBuffer(("ForwardModeRequested\r\n"));
#endif

    if (VIDDEC_GetSmoothBackwardCapability(VIDTRICK_Data_p->DecodeHandle) == ST_NO_ERROR)
    {
        /* Drift backward in order to re-launch forward injection from a backward position */
        AskForBackwardDrift(TrickModeHandle);

        /* Set a flag indicating that a switch to forward mode is necessary */
        VIDTRICK_Data_p->Flags.BackwardToForwardInitializing = TRUE;
        /* Reset backward flags to avoid problems when a quick transition
         * happens. */
#ifdef ST_smoothbackward
        VIDTRICK_Data_p->Flags.BackwardModeInitIsRequiredInTask = FALSE;
#endif /* ST_smoothbackward */
    }
    else
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }

    return(ErrorCode);
} /* end of ForwardModeRequested() function */


/*******************************************************************************
Name        : GetMaxAlignmentBetweenCDFifoAndSCD
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : the max alignment between SCD and CD ones.
*******************************************************************************/
static U32 GetMaxAlignmentBetweenCDFifoAndSCD(const VIDDEC_Handle_t DecodeHandle)
{
    U32             AlignmentCDFifo, AlignmentSCD;
    ST_ErrorCode_t  ErrorCode;

    ErrorCode = VIDDEC_GetCDFifoAlignmentInBytes(DecodeHandle, &AlignmentCDFifo);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error getting address of buffer !"));
    }
    else
    {
        ErrorCode = VIDDEC_GetSCDAlignmentInBytes(DecodeHandle, &AlignmentSCD);
        if (ErrorCode != ST_NO_ERROR)
        {
           STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error getting address of buffer !"));
        }
        else
        {
            if (AlignmentCDFifo > AlignmentSCD)
            {
                return (AlignmentCDFifo);
            }
            else
            {
               return (AlignmentSCD);
            }
        }
    }
    return (0);
} /* end of GetMaxAlignmentBetweenCDFifoAndSCD() function */


/*******************************************************************************
Name        : HasPictureReferenceForDecode
Description : Verifies that References pictures are decoded.
Parameters  :
Assumptions :
Limitations :
Returns     : TRUE if YES, FALSE otherwise
*******************************************************************************/
static BOOL HasPictureReferenceForDecode(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t const * PictureToBeDecodedInfo_p, VIDTRICK_PictureInfo_t const * PictureInfoInFrameBuffer_p)
{
    VIDTRICK_Data_t *             VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    VIDTRICK_PictureInfo_t      * BackwardPredictionPicture_p;
    VIDTRICK_PictureInfo_t      * ForwardPredictionPicture_p;
    VIDTRICK_PictureInfo_t      * BackwardPicture_p = NULL;
    VIDTRICK_PictureInfo_t      * Backward2Picture_p = NULL;

    /* I Picture */
    if (PictureMpegTypeIsI(PictureToBeDecodedInfo_p))
    {
        /* Always TRUE for I Pictures */
        return(TRUE);
    }

    /* P or B : Backward prediction is needed */
    BackwardPicture_p = PictureToBeDecodedInfo_p->BackwardOrLastBeforeIReferencePicture_p;
    if (BackwardPicture_p != NULL)
    {
        Backward2Picture_p = BackwardPicture_p->BackwardOrLastBeforeIReferencePicture_p;
    }

    /* if frame picture or first field of two fields picture */
    if (!IsPictureSecondField(PictureToBeDecodedInfo_p))
    {
        /*  */
        BackwardPredictionPicture_p = BackwardPicture_p;
    }
    else
    {
        if  ((BackwardPicture_p != NULL) && (PictureMpegTypeIsI(BackwardPicture_p)))
        {
            BackwardPredictionPicture_p = BackwardPicture_p;
        }
        else
        {
            BackwardPredictionPicture_p = Backward2Picture_p;
        }
    }

    if ((BackwardPredictionPicture_p == NULL) || (!BackwardPredictionPicture_p->IsDecoded))
    {
        return(FALSE);
    }

    if (PictureInfoInFrameBuffer_p != NULL)
    {
        /* PictureInfoInFrameBuffer_p may be the frame or first field of the backward prediction */
        if (IsPictureSecondField(BackwardPredictionPicture_p))
        {
            if (BackwardPredictionPicture_p->BackwardOrLastBeforeIReferencePicture_p == PictureInfoInFrameBuffer_p)
            {
                return(FALSE);
            }
        }
        else
        {
            if (BackwardPredictionPicture_p == PictureInfoInFrameBuffer_p)
            {
                return(FALSE);
            }
        }
    }

    /* Set Backward reference to decode It will be considered as the forward reference. Thus give a null pointer */
    /* so that Backward reference will be considered as the backward reference */
#ifdef TRACE_UART_DEBUG
    TraceBuffer(("** Set BW ref 0x%x 0x%x (%c%d) **\n\r",
               BackwardPredictionPicture_p,
               BackwardPredictionPicture_p->PictureBuffer_p,
               MPEGFrame2Char(BackwardPredictionPicture_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame),
               BackwardPredictionPicture_p->PictureInfoFromDecode.pTemporalReference));
#endif
    VIDDEC_NewReferencePicture(VIDTRICK_Data_p->DecodeHandle, BackwardPredictionPicture_p->PictureBuffer_p);

    /* B Picture */
    if (PictureMpegTypeIsB(PictureToBeDecodedInfo_p))
    {
        /* B : Forward prediction is needed */
        ForwardPredictionPicture_p = PictureToBeDecodedInfo_p->ForwardOrNextReferencePicture_p;

        if (   (ForwardPredictionPicture_p == NULL)
            || (! ForwardPredictionPicture_p->IsDecoded)
            || (   (PictureInfoInFrameBuffer_p != NULL)
                && (PictureInfoInFrameBuffer_p == ForwardPredictionPicture_p)))
        {
            return(FALSE);
        }

        /* Set Forward reference to decode */
#ifdef TRACE_UART_DEBUG
        TraceBuffer(("** Set FW ref 0x%x 0x%x (%c%d) **\n\r",
                   ForwardPredictionPicture_p,
                   ForwardPredictionPicture_p->PictureBuffer_p,
                   MPEGFrame2Char(ForwardPredictionPicture_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame),
                   ForwardPredictionPicture_p->PictureInfoFromDecode.pTemporalReference));
#endif
        VIDDEC_NewReferencePicture(VIDTRICK_Data_p->DecodeHandle, ForwardPredictionPicture_p->PictureBuffer_p);
    }

    return(TRUE);

} /* end Of HasPictureReferenceForDecode() function */


/*******************************************************************************
Name        : LaunchPictureDecode
Description : Asks the decode to decode a picture
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void LaunchPictureDecode(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t * PictureToBeDecodedInfo_p, VIDBUFF_PictureBuffer_t * PictureBufferToDecodeIn_p, VIDTRICK_PictureInfo_t * const PictureInfoInFrameBuffer_p)
{
    VIDTRICK_Data_t *             VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;
    VIDDEC_DecodePictureParams_t  DecodePictureParams;
    BOOL                          WarmResetDone= FALSE;

#ifdef TRICKMOD_DEBUG
    /* U8 i = 0; */
    /* VIDTRICK_PictureInfo_t * NextInTheList_p = NULL; */
    /* static STVID_MPEGFrame_t  LastDecodedPictureMPEGFrame = STVID_MPEG_FRAME_B; */
#endif  /* TRICKMOD_DEBUG */

#ifdef TRICKMOD_DEBUG
    if  (VIDTRICK_Data_p->Flags.SmoothBackwardIsInitializing)
    {
        TrickModeError();
        TraceBuffer(("smooth backward is under initializing !\n\r"));
    }

    /* Find First picture to insert in Display queue */
    /* run the table while picture found is in display queue */
    /* i = 0; */
    /* do */
    /* { */
        /* NextInTheList_p = VIDTRICK_Data_p->LastPictureInDisplayQueue_p[i]; */
        /* i++; */
    /* } while ((i <= VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ) && (NextInTheList_p->IsInDisplayQueue)); */
    /* i is thus the number of the first picture not in DQ in the table */

    /* if (NextInTheList_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference < */
            /* PictureToBeDecodedInfo_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference) */
    /* { */
        /* TrickModeError(); */
    /* } */
#endif  /* TRICKMOD_DEBUG */

    if (VIDTRICK_Data_p->BackwardWarmReset.OnGoing)
    {
#ifdef TRACE_UART_MAIN
        TraceBuffer(("WARM reset on going\r\n"));
#endif

        if (   (VIDTRICK_Data_p->BackwardWarmReset.MustDecodeSecondField)
            && (VIDTRICK_Data_p->BackwardWarmReset.FailingDecodePictureStructure == PictureToBeDecodedInfo_p->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure))
        {
            if (HasPictureASecondField(PictureToBeDecodedInfo_p))
            {
              WarmResetDone= TRUE;

              /* Find Second field picture to decode */
              if (PictureToBeDecodedInfo_p->SecondFieldPicture_p != NULL)
              {
                  PictureToBeDecodedInfo_p= PictureToBeDecodedInfo_p->SecondFieldPicture_p;
              }
              else
              {
#ifdef TRICKMOD_DEBUG
                  TrickModeError();
                  TraceBuffer(("Second field Picture is NULL !!!\r\n"));
#endif /* TRICKMOD_DEBUG */
              }
            }
            else
            {
#ifdef TRICKMOD_DEBUG
                  TrickModeError();
                  TraceBuffer(("Has no second field\r\n"));
#endif /* TRICKMOD_DEBUG */
            }

            if (VIDTRICK_Data_p->BackwardWarmReset.FailingDecodePictureStructure == PictureToBeDecodedInfo_p->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure)
            {
#ifdef TRICKMOD_DEBUG
                  TrickModeError();
                  TraceBuffer(("Picture Structure still the same !!!\r\n"));
#endif /* TRICKMOD_DEBUG */
            }
        }

        VIDTRICK_Data_p->BackwardWarmReset.OnGoing = FALSE;
        VIDTRICK_Data_p->BackwardWarmReset.MustDecodeSecondField = FALSE;
    }

    /* do not decode over its backward ref */
    if ( PictureToBeDecodedInfo_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame != STVID_MPEG_FRAME_I)
    {
#ifdef TRICKMOD_DEBUG
        if(PictureToBeDecodedInfo_p->BackwardOrLastBeforeIReferencePicture_p == NULL)
        {
            TrickModeError();
            TraceBuffer(("Trying to decode a %s picture while the backward reference is not available !\r\n",
                (PictureToBeDecodedInfo_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B) ?
                "B" : "P"));
        }
#endif /* TRICKMOD_DEBUG */
        if (PictureBufferToDecodeIn_p == PictureToBeDecodedInfo_p->BackwardOrLastBeforeIReferencePicture_p->PictureBuffer_p)
        {
#ifdef TRICKMOD_DEBUG
            TrickModeError();
            TraceBuffer(("Want to re-decode on its backward\n\r"));
#endif /* TRICKMOD_DEBUG */
            /* Error Computing */
            BackwardModeInit(TrickModeHandle, TRUE);
        }
    }

    if (VIDTRICK_Data_p->PictureUnderDecoding_p != NULL)
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("Decode already in progress\n\r"));
#endif /* TRICKMOD_DEBUG */
        return;
    }

    /* if first field or frame */
    if (   (! IsPictureSecondField(PictureToBeDecodedInfo_p))
        || (WarmResetDone))
    {
        if (PictureInfoInFrameBuffer_p != NULL)
        {
            UnlockPicture(TrickModeHandle, PictureInfoInFrameBuffer_p, TRUE, FALSE, FALSE);
        }

        /* "PictureBufferToDecodeIn_p" is already locked: Nothing to do */
    }
#ifdef TRICKMOD_DEBUG
    else
    {
        /* decode of second field should be launched directly. */
        TrickModeError();
        TraceBuffer(("Launch second field decode, should be launched directly\n\r"));
    }
#endif /* TRICKMOD_DEBUG */

    DecodePictureParams.DataInputAddress_p = (void *)((U32)(PictureToBeDecodedInfo_p->AssociatedStartCodeAddress_p));
    DecodePictureParams.OutputPicture_p = PictureBufferToDecodeIn_p;
    DecodePictureParams.OutputDecimatedPicture_p = PictureBufferToDecodeIn_p->AssociatedDecimatedPicture_p;
    DecodePictureParams.StreamInfoForDecode_p = &(PictureToBeDecodedInfo_p->PictureInfoFromDecode.StreamInfoForDecode);
    DecodePictureParams.PictureInfos_p = &(PictureToBeDecodedInfo_p->PictureInfoFromDecode.PictureInfos);
    DecodePictureParams.PictureStreamInfo_p = &(PictureToBeDecodedInfo_p->PictureInfoFromDecode.AdditionalStreamInfo);
    DecodePictureParams.pTemporalReference = PictureToBeDecodedInfo_p->PictureInfoFromDecode.pTemporalReference;

    /* smooth inits */
    PictureToBeDecodedInfo_p->PictureBuffer_p = PictureBufferToDecodeIn_p;
    PictureBufferToDecodeIn_p->SmoothBackwardPictureInfo_p = NULL; /* SmoothBackwardPictureInfo_p will be initialized in NewDecodedPicture function */
    VIDTRICK_Data_p->PictureUnderDecoding_p = PictureToBeDecodedInfo_p;

#ifdef TRACE_UART_TOPBOT_ALTERNATING
    TraceBuffer(("\n\rW(%d) %d %c %s\r\n", __LINE__,
                   PictureToBeDecodedInfo_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference,
                   MPEGFrame2Char(PictureToBeDecodedInfo_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame),
                   PictureStruct2String(PictureToBeDecodedInfo_p->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure)));
#endif

    /* Decode the picture */
    VIDDEC_DecodePicture(VIDTRICK_Data_p->DecodeHandle, &DecodePictureParams);

} /* end of LaunchPictureDecode() function */


/*******************************************************************************
Name        : NewPictureDecodeIsRequired
Description : Asks the decode to decode a picture
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void NewPictureDecodeIsRequired(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t * PictureToBeDecodedInfo_p, VIDBUFF_PictureBuffer_t * PictureBufferToDecodeIn_p, VIDTRICK_PictureInfo_t * const PictureInfoInFrameBuffer_p)
{
    VIDTRICK_Data_t * VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;

    if (   (PictureToBeDecodedInfo_p == NULL)
        || (PictureBufferToDecodeIn_p == NULL))
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("Null pointers in NewPictureDecodeIsRequired ! \n\r"));
#endif /* TRICKMOD_DEBUG */
        return;
    }

#ifdef TRACE_UART_DEBUG
    TraceBuffer(("New picture decode in 0x%x, PB 0x%x, 0x%x\n\r", PictureToBeDecodedInfo_p,
                    PictureBufferToDecodeIn_p,
                    PictureInfoInFrameBuffer_p));
#endif

#ifdef TRICKMOD_DEBUG
     if (   (PictureMpegTypeIsB(PictureToBeDecodedInfo_p))
         && (   (PictureToBeDecodedInfo_p->BackwardOrLastBeforeIReferencePicture_p == NULL)
             || (PictureToBeDecodedInfo_p->BackwardOrLastBeforeIReferencePicture_p->PictureBuffer_p == NULL)
             || (PictureToBeDecodedInfo_p->ForwardOrNextReferencePicture_p == NULL)
             || (PictureToBeDecodedInfo_p->ForwardOrNextReferencePicture_p->PictureBuffer_p == NULL)))
     {
        TrickModeError();
     }
     if (   (PictureMpegTypeIsP(PictureToBeDecodedInfo_p))
         && (   (PictureToBeDecodedInfo_p->BackwardOrLastBeforeIReferencePicture_p == NULL)
             || (PictureToBeDecodedInfo_p->BackwardOrLastBeforeIReferencePicture_p->PictureBuffer_p == NULL)))
     {
        TrickModeError();
     }
     if (   (IsPictureField(PictureToBeDecodedInfo_p))
         && (PictureToBeDecodedInfo_p->SecondFieldPicture_p == NULL))
     {
        TrickModeError();
     }
#endif /* TRICKMOD_DEBUG */

    /* Error recovery reset */
    VIDTRICK_Data_p->ErrorRecovery.NbVsyncElapsedSinceLastDecodeHasBeenLaunched = 0;
    VIDTRICK_Data_p->ErrorRecovery.NbVsyncElapsedWithoutAnyDecode = 0;

    /* update generic structure VIDBUFF_PictureBuffer_t  */
    vidtrick_FillPictureBuffer(PictureBufferToDecodeIn_p, PictureToBeDecodedInfo_p);
    if (PictureBufferToDecodeIn_p->AssociatedDecimatedPicture_p != NULL)
    {
        vidtrick_FillPictureBuffer(PictureBufferToDecodeIn_p->AssociatedDecimatedPicture_p, PictureToBeDecodedInfo_p);
    }

    /* launch decode */
    LaunchPictureDecode(TrickModeHandle, PictureToBeDecodedInfo_p, PictureBufferToDecodeIn_p, PictureInfoInFrameBuffer_p);
} /* end of NewPictureDecodeIsRequired() function */

/*******************************************************************************
Name        : CheckIfSecondFieldIsAvailable
Description : Checks the picture has its second field picture
              in the linear bit buffer for decode.
Parameters  : CurrentPicture_p: current picture
Assumptions :
Limitations :
Returns     : TRUE, FALSE
*******************************************************************************/
static BOOL CheckIfSecondFieldIsAvailable(VIDTRICK_PictureInfo_t * const CurrentPicture_p)
{
    BOOL  PictureOK = TRUE;

    if (! IsPictureFrame(CurrentPicture_p))
    {
        if (HasPictureASecondField(CurrentPicture_p))       /* First field */
        {
            /* Picture has its second field picture */
            /* and second field structure is effectively different from current one */
            /* => the picture is OK, otherwise it is KO */
            if (   (CurrentPicture_p->SecondFieldPicture_p == NULL)
                || (CurrentPicture_p->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure ==
                         CurrentPicture_p->SecondFieldPicture_p->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure))
            {
                PictureOK = FALSE;
            }
        }
        else                                                /* Second Field */
        {
            /* Picture has its first field picture */
            /* and first field points effectively on current picture */
            /* and first field structure is effectively different from current one */
            /* => the picture is OK, otherwise it is KO */
            if (   (CurrentPicture_p->PreviousInStreamPicture_p == NULL)
                || (CurrentPicture_p->PreviousInStreamPicture_p->SecondFieldPicture_p != CurrentPicture_p)
                || (CurrentPicture_p->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure ==
                         CurrentPicture_p->PreviousInStreamPicture_p->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure))
            {
                PictureOK = FALSE;
            }
        }
    }

    return (PictureOK);
} /* end of CheckIfSecondFieldIsAvailable() function */

/*******************************************************************************
Name        : FindFirstPictureToBeDisplayed
Description : This function is called every time trickmod speed is changed from poisitve to negative speed :
              is searches for the last picture displayed in automatic mode in order to start backward display
              from tha same picture
Parameters  :
Assumptions : The same picture may not be decodable, then the FirstDisplayable is returned.
Limitations :
Returns     : TRUE if the searched picture was found in the current buffer
*******************************************************************************/
static BOOL FindFirstPictureToBeDisplayed(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const LinearBitBuffer_p, VIDTRICK_PictureInfo_t ** FirstDisplayablePicture_p)
{
    VIDTRICK_Data_t             * VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;
    VIDTRICK_PictureInfo_t      * Picture_p;
    STVID_PictureParams_t         CurrentDisplayedPictureParams;
    U32                           CurrentlyDisplayedTimeCode, ParsedPictureTimeCode, LastDisplayablePictureTimeCode;

    /* init displayable picture */
    *FirstDisplayablePicture_p= NULL;

    /* init picture index */
    Picture_p = &(VIDTRICK_Data_p->LinearBitBuffers.BufferA.PictureInfoTableStartAddress_p[VIDTRICK_Data_p->LinearBitBuffers.BufferA.NbPicturesInside-1]);

    /* The current displayed picture has to be recognized to start smooth backward from this one */
    VIDDISP_GetDisplayedPictureParams(VIDTRICK_Data_p->DisplayHandle, &CurrentDisplayedPictureParams);

    CurrentlyDisplayedTimeCode =  (((U32)CurrentDisplayedPictureParams.TimeCode.Hours)   << 24)
                                | (((U32)CurrentDisplayedPictureParams.TimeCode.Minutes) << 16)
                                | (((U32)CurrentDisplayedPictureParams.TimeCode.Seconds) << 8 )
                                | (((U32)CurrentDisplayedPictureParams.TimeCode.Frames));

#ifdef TRACE_UART_SPEEDSWITCH
    TraceBuffer(("Last Displayed: TmplRef=%d, TimeCod=%.8x\r\n",
        CurrentDisplayedPictureParams.pTemporalReference,
        CurrentlyDisplayedTimeCode
          ));
#endif

    LastDisplayablePictureTimeCode =  (((U32)LinearBitBuffer_p->LastDisplayablePicture_p->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode.Hours)   << 24)
                           | (((U32)LinearBitBuffer_p->LastDisplayablePicture_p->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode.Minutes) << 16)
                           | (((U32)LinearBitBuffer_p->LastDisplayablePicture_p->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode.Seconds) << 8 )
                           | (((U32)LinearBitBuffer_p->LastDisplayablePicture_p->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode.Frames));

    /* If the timecode of the last displable picture is less than the currently display picture time code,
     * all the pictures in the bitbuffer will have timecodes less than the currently display. So
     * no need to go through all the pictures.  */
    if(LastDisplayablePictureTimeCode < CurrentlyDisplayedTimeCode)
    {
        while (   (*FirstDisplayablePicture_p == NULL)
            && (Picture_p != VIDTRICK_Data_p->LinearBitBuffers.BufferA.PictureInfoTableStartAddress_p))
        {
            ParsedPictureTimeCode =  (((U32)Picture_p->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode.Hours)   << 24)
                                | (((U32)Picture_p->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode.Minutes) << 16)
                                | (((U32)Picture_p->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode.Seconds) << 8 )
                                | (((U32)Picture_p->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode.Frames));
#ifdef TRACE_UART_SPEEDSWITCH
            TraceBuffer(("Parsed picture TimeCod=%.8x temp=%d\r\n",
                ParsedPictureTimeCode,
                Picture_p->PictureInfoFromDecode.pTemporalReference
                ));
#endif
            if ( (  (ParsedPictureTimeCode < CurrentlyDisplayedTimeCode)
                ||    (ParsedPictureTimeCode == CurrentlyDisplayedTimeCode) )
                    && (CurrentDisplayedPictureParams.pTemporalReference == Picture_p->PictureInfoFromDecode.pTemporalReference))
            {
                if (PictureMpegTypeIsB(Picture_p))
                {
                    if (   (Picture_p->BackwardOrLastBeforeIReferencePicture_p != NULL)
                        && (Picture_p->BackwardOrLastBeforeIReferencePicture_p->IsDecodable)
                        && (CheckIfSecondFieldIsAvailable(Picture_p->BackwardOrLastBeforeIReferencePicture_p)))
                    {
                        *FirstDisplayablePicture_p = Picture_p;
                    }
                }
                else
                {
                    if (   (Picture_p->IsDecodable)
                        && (CheckIfSecondFieldIsAvailable(Picture_p)))
                    {
                        *FirstDisplayablePicture_p = Picture_p;
                    }
                }
            }
            Picture_p--;
        }
    }
#ifdef TRACE_UART_SPEEDSWITCH
    else
    {
        TraceBuffer(("Last displayable TimeCod=%.8x : Searched picture is not in this buffer\r\n",
            ParsedPictureTimeCode
            ));
    }
#endif

    if (*FirstDisplayablePicture_p == NULL)
    {
#ifdef TRICKMOD_DEBUG
        TraceBuffer(("Could not find 1st Picture to be displayed !!\r\n"));
#endif
        /* Find First Displayable picture  */
        FindFirstDisplayablePictureOfLinearBitBuffer(TrickModeHandle, &VIDTRICK_Data_p->LinearBitBuffers.BufferA, FirstDisplayablePicture_p);
        return(FALSE);
    }
#ifdef TRACE_UART_SPEEDSWITCH
    else
    {
        TraceBuffer(("Picture Found\r\n"));
    }
#endif

#ifdef TRACE_UART_SPEEDSWITCH
    TraceBuffer(("1st Displayable: TmplRef=%d, TimeCod=%.2x %.2x %.2x %.2x, ExtTmplRef=%d, Idx=%d \r\n",
        (*FirstDisplayablePicture_p)->PictureInfoFromDecode.pTemporalReference,
        (*FirstDisplayablePicture_p)->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode.Hours,
        (*FirstDisplayablePicture_p)->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode.Minutes,
        (*FirstDisplayablePicture_p)->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode.Seconds,
        (*FirstDisplayablePicture_p)->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode.Frames,
        (*FirstDisplayablePicture_p)->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference,
        (*FirstDisplayablePicture_p)->IndiceInBuffer
          ));
#endif
    return(TRUE);
} /* End of functions FindFirstPictureToBeDisplayed() */

/*******************************************************************************
Name        : LinearBitBufferFullyParsed
Description : This function prepares the just parsed linear bit buffer : it links all the references pictures,
              and links it with the associated linear bit buffer.
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void LinearBitBufferFullyParsed(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const JustParsedBuffer_p)
{
    VIDTRICK_Data_t               * VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;
    VIDTRICK_PictureInfo_t        * FirstIpicture_p = NULL;
    U32                             FirstIPictureIndex;
    VIDTRICK_PictureInfo_t        * FirstPictureToBeDisplayed_p = NULL;
    clock_t                         LinearBitBufferTransferBeginDate;
    clock_t                         FullyParsedLinearBitBufferDate;
    clock_t                         TicksPerSecond;
    BOOL                            MatchingPictureHasBeenFound;
#ifdef TRACE_UART_EXT_TEMP_REF
    BOOL                            DoNotTraceExtTempRef;

    /* Store value of DoNotMatchBufAWithBufB since it is changed into this very function */
    DoNotTraceExtTempRef= VIDTRICK_Data_p->Flags.DoNotMatchBufAWithBufB;

    TraceBuffer(("\n\nBuf=0x%.8x parsed, Link=%d\r\n", JustParsedBuffer_p->WholeBufferStartAddress_p,
                                                       (   (!VIDTRICK_Data_p->Flags.DoNotMatchBufAWithBufB)
                                                        && (!JustParsedBuffer_p->Discontinuity.DiscontinuityWithPreviousBuffer))));
#endif /* TRACE_UART_EXT_TEMP_REF */

#ifdef TRACE_UART_DEBUG
    TraceBuffer(("LinearBitBufferFullyParsed \n\r"));
#endif

    /* Renumber all time codes of picture before first GOP. */
    RenumberTimeCodes(TrickModeHandle, JustParsedBuffer_p);

    /* Reorganize pictures in the buffer */
    RenumberExtendedTemporalReferences(TrickModeHandle, JustParsedBuffer_p);

    /* If parsing buffer A for the first time buffer A does not need to be matched with buffer B */
    /* The pictures have to be reorganized all the other times.                                  */
    if (!VIDTRICK_Data_p->Flags.DoNotMatchBufAWithBufB)
    {
        /* Buffer fully parsed detected by this way means that all PTS comparisons done before have been unsuccessfull.   */
        /* An other way to recognize at least one same picture in both linear buffers is to take the latest picture in the*/
        /* current buffer which has the same temporal reference as the one of the first picture of the other buffer.      */
        if (!JustParsedBuffer_p->DiscontinuityWithPreviousBuffer)
        {
            /* no matching picture have been found by PTS. find it with temporal refences and size */
            MatchingPictureHasBeenFound = FindMatchingPictureWithTemporalReference(TrickModeHandle, JustParsedBuffer_p);
            if (MatchingPictureHasBeenFound)
            {
                /* Matching picture has been found */
#ifdef TRACE_UART_MAIN
                 TraceBuffer(("Parsed buf Pic nb = %d  Matching Pic Found, nb common picts = %d \n\r" ,JustParsedBuffer_p->NbPicturesInside, JustParsedBuffer_p->NbPicturesInside -
                       VIDTRICK_Data_p->MatchingPicture.ParsedBufferMatchingPictureNumber + 1));
#endif
                /* Number of pictures of just parsed buffer                             */
                /* LastPictureIndex is the index of first picture of associated buffer  */
                /* must then substract 1 to get last picture of current buffer          */
                JustParsedBuffer_p->NbPicturesInside = VIDTRICK_Data_p->MatchingPicture.LastPictureIndex - 1;
            }
            else
            {
#ifdef TRICKMOD_DEBUG
                TraceBuffer(("Parsed buf Pic nb = %d, Matching Pic not found !!! \n\r", JustParsedBuffer_p->NbPicturesInside));
#endif
                JustParsedBuffer_p->DiscontinuityWithPreviousBuffer = TRUE;
            }
        }
        else
        {
#ifdef TRACE_UART_DEBUG
              TraceBuffer(("Parsed buf Pic nb = %d, Discontinuity between both buffers !!! \n\r", JustParsedBuffer_p->NbPicturesInside));
#endif
        }
    }
    else
    {
#ifdef TRACE_UART_DEBUG
        TraceBuffer(("No matching\n\r"));
#endif
    }


    /* find First I picture of just parsed buffer. if returned FirstIpicture_p is NULL it means that there is not */
    /* frame or first field I picture in the bit buffer ==> unesable : ask for a new bit buffer flush */
    FindFirstIPictureOfLinearBitBuffer(TrickModeHandle, JustParsedBuffer_p, &FirstIpicture_p, &FirstIPictureIndex);
    if (FirstIpicture_p == NULL)
    {
#ifdef TRACE_UART_MAIN
        TraceBuffer(("No I Picture in just flushed bit buffer ! \r\n"));
#endif
        JustParsedBuffer_p->DiscontinuityWithPreviousBuffer = TRUE;
        VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p = NULL;
        AskForLinearBitBufferFlush(TrickModeHandle, JustParsedBuffer_p, VIDTRICK_FLUSH_WITH_NORMAL_OVERLAP);
        return;
    }
    else
    {
        /* !!!! if first field I picture is the last picture of the buffer */
        /* this buffer may have to be reflushed */
        VIDTRICK_Data_p->EndOfDataPicture_p = FirstIpicture_p;
        JustParsedBuffer_p->LastDisplayablePicture_p = FirstIpicture_p;
    }

    /* Choose the pictures that will be displayed according to the current speed and according to number of pictures in the bitbuffer */
    ComputeMaxJumpeableFields(TrickModeHandle,
                              VIDTRICK_Data_p->MainParams.Speed,
                              JustParsedBuffer_p->NbPicturesInside - FirstIPictureIndex);

    /* Affect all references */
    AffectReferencesPictures(TrickModeHandle, JustParsedBuffer_p);

    /* must be done after AffectReferencesPictures because DisplayDiscontinuityAffectPictures need Pictures to be linked */
    if (   (JustParsedBuffer_p->DiscontinuityWithPreviousBuffer)
        && (! VIDTRICK_Data_p->Flags.DoNotMatchBufAWithBufB))
    {
        DisplayDiscontinuityAffectPictures(TrickModeHandle, JustParsedBuffer_p);
    }

    /* Fast Backward fluidity */
    /* Get transfer begin date before it is eventually reset by AskForLinearBitBufferFlush(). */
    LinearBitBufferTransferBeginDate = VIDTRICK_Data_p->BackwardFluidity.TransferBeginDate;

    /* if Smooth backward Initializing */
    if (VIDTRICK_Data_p->Flags.SmoothBackwardIsInitializing)
    {
        /* If buffer under parsing is buffer A */
        if (VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p == &VIDTRICK_Data_p->LinearBitBuffers.BufferA)
        {
            /* Set to False definitvely (until a new negative speed with a positive previous speed is asked) */
            VIDTRICK_Data_p->Flags.DoNotMatchBufAWithBufB = FALSE;

            if(VIDTRICK_Data_p->Flags.DoNotTryToRestartOnTheLastDisplayedPicture)
            {
                VIDTRICK_Data_p->Flags.DoNotTryToRestartOnTheLastDisplayedPicture = FALSE;
                FindFirstDisplayablePictureOfLinearBitBuffer(TrickModeHandle, VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p, &FirstPictureToBeDisplayed_p);
            }
            else
            {
                BOOL    FirstPictureToBeDisplayedWasFound = FALSE;
                /* Find First Displayable picture */
                FirstPictureToBeDisplayedWasFound = FindFirstPictureToBeDisplayed(TrickModeHandle, VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p, &FirstPictureToBeDisplayed_p);
                /* If we don't find the picture in the current injection, ask for another injection with overlap, until we reach the searched picture. */
                if(   (!FirstPictureToBeDisplayedWasFound)
                   && (VIDTRICK_Data_p->NumberOfFlushesToRestartFromTheSamePicture < (U32)VIDTRICK_MAX_FLUSHES_TO_RESTART_FROM_THE_SAME_PICTURE))
                {
                    VIDTRICK_Data_p->NumberOfFlushesToRestartFromTheSamePicture++;
                    JustParsedBuffer_p->DiscontinuityWithPreviousBuffer = TRUE;
                    VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p = NULL;
                    JustParsedBuffer_p->AssociatedLinearBitBuffer_p->InjectionDuration = JustParsedBuffer_p->InjectionDuration;
                    JustParsedBuffer_p->AssociatedLinearBitBuffer_p->BeginningOfInjectionBitrateValue      = JustParsedBuffer_p->BeginningOfInjectionBitrateValue;
                    AskForLinearBitBufferFlush(TrickModeHandle, JustParsedBuffer_p, VIDTRICK_FLUSH_WITH_NORMAL_OVERLAP);
                    return;
                }
            }
            /* no more buffer under parsing, but buffer B has to be parsed */
            VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p = NULL;

#ifdef TRICKMOD_DEBUG
            if (   (HasPictureASecondField(FirstPictureToBeDisplayed_p))
                && (FirstPictureToBeDisplayed_p->SecondFieldPicture_p == NULL))
            {
                /* Should only insert field pictures with their second field */
                TrickModeError();
            }
#endif
            /* temporary : let the task compute all the pictures to be displayed */
            VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0] = FirstPictureToBeDisplayed_p;
            VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ = 1;
#ifdef DEBUG_DISPLAY_QUEUE
            DbgCheckPicturesInDisplayQueue(TrickModeHandle);
#endif

            /* Ask for buffer B to be flushed */
            AskForLinearBitBufferFlush(TrickModeHandle, &(VIDTRICK_Data_p->LinearBitBuffers.BufferB), VIDTRICK_FLUSH_WITH_NORMAL_OVERLAP);

            /* The lines below were commented because if the picture to restart on is at the begginng of the linear bitbuffer,
             * we will display a few pictures and then wait for the next bitbuffer to be filled. So, we prefer to wait for the
             * next linear bitbuffer to be filled also, before starting the display. */

            /* buffer A has been fully parsed : display process can begin */
            VIDTRICK_Data_p->Flags.SmoothBackwardIsInitializing = FALSE;
        }

        /* if buffer under parsing is buffer B */
        else if (VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p == &VIDTRICK_Data_p->LinearBitBuffers.BufferB)
        {
            /* buffer A and B have been fully parsed parsed : display process can begin */
            /*VIDTRICK_Data_p->Flags.SmoothBackwardIsInitializing = FALSE;*/

            /* wake up of the smooth backward task */
            semaphore_signal(VIDTRICK_Data_p->WakeTrickModeTaskUp_p);

        } /* if (VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p == &VIDTRICK_Data_p->LinearBitBuffers.BufferB) */
#ifdef TRICKMOD_DEBUG
        else
        {
            TrickModeError();
            TraceBuffer(("Unknown buffer under parsing\n\r"));
        }
#endif /* TRICKMOD_DEBUG */
    } /* if (VIDTRICK_Data_p->Flags.SmoothBackwardIsInitializing) */

    /* Fast Backward fluidity */
    /* Transfer + Parse duration evaluation. */
    FullyParsedLinearBitBufferDate = time_now();
    TicksPerSecond = ST_GetClocksPerSecond();

    /* NbFieldsToBeDisplayedToBeFluent_p = ((transfer + parse durationin ticks ) / (nb ticks per second)) * FrameRate */
    if (TicksPerSecond != 0)
    {
        VIDTRICK_Data_p->BackwardFluidity.NbFieldsToBeDisplayedToBeFluent = (time_minus(FullyParsedLinearBitBufferDate, LinearBitBufferTransferBeginDate) * ((VIDTRICK_Data_p->MainParams.FrameRateValue + 999) / 1000) * 2) / TicksPerSecond;
    }

#ifdef TRACE_UART_DISPLAY_SKIP
    TraceBuffer(("Ticks=%d Time=%d\n\r",
                      TicksPerSecond,
                      FullyParsedLinearBitBufferDate - LinearBitBufferTransferBeginDate));
#endif

    /* Stop the parsing of the current buffer.                   */
    /* must be done AFTER affect functions because of CheckIfLinearBufferHasToBeFlushed test in smooth backward management */
    VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p = NULL;

#ifdef TRACE_UART_EXT_TEMP_REF
    /* traces of reorganized pictures */
    TraceAllExtendedTemporalReferenceOfTheBuffer( TrickModeHandle,
                                                  JustParsedBuffer_p,
                                                  JustParsedBuffer_p->NbPicturesInside);
    if (! DoNotTraceExtTempRef)
    {
        TraceAllExtendedTemporalReferenceOfTheBuffer( TrickModeHandle,
                                                      JustParsedBuffer_p->AssociatedLinearBitBuffer_p,
                                                      JustParsedBuffer_p->AssociatedLinearBitBuffer_p->NbPicturesInside );
    }
#endif /* TRACE_UART_EXT_TEMP_REF */
} /* end of LinearBitBufferFullyParsed() function */

/*******************************************************************************
Name        : DecoderReadyForBackwardMode
Description : Callback function called when VIDDEC_READY_TO_START_BACKWARD_MODE_EVT occurs.
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void DecoderReadyForBackwardMode(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p)
{
    VIDTRICK_Handle_t       TrickModeHandle  = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDTRICK_Handle_t, SubscriberData_p);
    VIDTRICK_Data_t *             VIDTRICK_Data_p;

    /* Find structure */
    VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData_p);

#ifdef TRACE_UART_DEBUG
    TraceBuffer(("DecoderReadyForBackwardMode\n\r"));
#endif

#ifdef ST_smoothbackward
    VIDTRICK_Data_p->Flags.BackwardModeInitIsRequiredInTask = TRUE;
    semaphore_signal(VIDTRICK_Data_p->WakeTrickModeTaskUp_p);
#endif /* ST_smoothbackward */

}

/*******************************************************************************
Name        : NewDecodedPicture
Description : Function to subscribe the VIDDEC event picture decoder is idle
Parameters  : EVT API
Assumptions : if B picture decoded its forward may be now unuseful to decode any other B pictures.
              thus, unlock it partially.
Limitations :
Returns     : nothing
*******************************************************************************/
static void NewDecodedPicture(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p)
{
    VIDTRICK_Handle_t       TrickModeHandle = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDTRICK_Handle_t, SubscriberData_p);
    VIDDEC_DecodePictureParams_t  DecodePictureParams;
    VIDTRICK_PictureInfo_t *      PictureJustDecoded_p = NULL;
    VIDTRICK_PictureInfo_t *      FrameOrFirstFieldPicture_p = NULL;
    VIDTRICK_PictureInfo_t *      SecondFieldPictureToBeDecoded_p = NULL;
    VIDTRICK_Data_t *             VIDTRICK_Data_p;

/* #define DEBUG_SHOW_DECODED_PICTURE */
#ifdef DEBUG_SHOW_DECODED_PICTURE  /* debug only */
    U8 DcfHigh;
    static BOOL DebugFlag = FALSE;
#endif

    /* Find structure */
    VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;

    /* unused parameters */
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData_p);

    /* if Smooth Backward Is Initializing, it means that a decode manual mode  has been selected */
    /* between a picture decode beginning and its end. Should only happen one time. */
    if (   (VIDTRICK_Data_p->Flags.SmoothBackwardIsInitializing)
        || (VIDTRICK_Data_p->Flags.BackwardToForwardInitializing)
        || (VIDTRICK_Data_p->Stop.TrickmodIsStopped)
        || (VIDTRICK_Data_p->MainParams.Speed >= 0))
    {
#ifdef TRACE_UART_MAIN
        TraceBuffer(("New decoded picture but "));
        if (VIDTRICK_Data_p->Stop.TrickmodIsStopped)
        {
            TraceBuffer(("smooth backward is stopped \r\n"));
        }
        if (VIDTRICK_Data_p->Flags.SmoothBackwardIsInitializing)
        {
            TraceBuffer(("smooth backward is still initializing \r\n"));
        }
        if (VIDTRICK_Data_p->Flags.BackwardToForwardInitializing)
        {
            TraceBuffer(("forward is initializing \r\n"));
        }
        if (VIDTRICK_Data_p->MainParams.Speed >= 0)
        {
            TraceBuffer(("speed > 0\r\n"));
        }
#endif /* TRACE_UART_MAIN */
       return;
    }

    /* Lock Trick mode params */
    STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p);

    /* Test if speed is switching from backward to forward */
    /* in that case speed is still negative whereas ForwardDisplayQueueInsertionIsEnabled is not enabled */
    if (VIDTRICK_Data_p->Flags.BackwardToForwardInitializing)
    {
        /* Switching from backward to forward : ignore event and exit */
        STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
        return;
    }

    /* Set local variable */
    PictureJustDecoded_p = VIDTRICK_Data_p->PictureUnderDecoding_p;

#ifdef TRACE_UART_MAIN
        TraceBuffer(("NewDecodedPicture  0x%x 0x%x\n\r", PictureJustDecoded_p, PictureJustDecoded_p->PictureBuffer_p));
#endif /* TRACE_UART_MAIN */

#ifdef TRACE_UART_TOPBOT_ALTERNATING
    TraceBuffer(("R(%d) %d %c %s\r\n", __LINE__,
                   PictureJustDecoded_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference,
                   MPEGFrame2Char(PictureJustDecoded_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame),
                   PictureStruct2String(PictureJustDecoded_p->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure)));
#endif

    /* no more picture is under decoding */
    VIDTRICK_Data_p->PictureUnderDecoding_p = NULL;

#ifdef TRACE_UART_TOPBOT_ALTERNATING
    TraceBuffer(("%d W=NULL\n\r", __LINE__));
#endif

    /* should never happen */
    if (PictureJustDecoded_p == NULL)
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("NULL pointer for the picture just decoded.\n\r"));
#endif /* TRICKMOD_DEBUG */

        STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
        return;
    }

    if (PictureJustDecoded_p->IsDecoded)
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("Picture 0x%x (PB 0x%x) already decoded\n\r", PictureJustDecoded_p, PictureJustDecoded_p->PictureBuffer_p));
#endif /* TRICKMOD_DEBUG */
        STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
        return;
    }

#ifdef TRICKMOD_DEBUG
    if (PictureJustDecoded_p->PictureBuffer_p == NULL)
    {
        TrickModeError();
        TraceBuffer(("Error! PictureBuffer_p == NULL !!!\n\r"));
        STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
        return;
    }
#endif /* TRICKMOD_DEBUG */

    if ( vidcom_ComparePictureID(&PictureJustDecoded_p->PictureInfoFromDecode.ExtendedPresentationOrderPictureID,
            &PictureJustDecoded_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID) != 0 )
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("New Decoded Picture : Incoherency between PictureInFrameBuffer and its record (%d/%d vs %d/%d) !\n\r",
           PictureJustDecoded_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
           PictureJustDecoded_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
           PictureJustDecoded_p->PictureInfoFromDecode.ExtendedPresentationOrderPictureID.ID,
           PictureJustDecoded_p->PictureInfoFromDecode.ExtendedPresentationOrderPictureID.IDExtension));
#endif /* TRICKMOD_DEBUG */
        /* SPURIOUS ! the decode seem to not have duplicated the given parameters when */
        /* the decode has been required....  */
        /* Try to decode one more time only if process is not too late */
        /* Indeed, the first not decoded picture has to be removed from the table */
        /* of the pictures to be inserted in display queue as soon as possible */

        /* Error Computing */
        PictureDecodeHasErrors(TrickModeHandle, PictureJustDecoded_p);
    }
    else if (PictureJustDecoded_p->PictureBuffer_p->Decode.DecodingError != VIDCOM_ERROR_LEVEL_NONE)
    {
#ifdef TRICKMOD_DEBUG
         /* TrickModeError(); */
#endif /* TRICKMOD_DEBUG */

#ifdef TRICKMOD_DEBUG
        TraceBuffer(("Decode %d number %d Is Not OK !!\n\r",
                PictureJustDecoded_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference,
                PictureJustDecoded_p->IndiceInBuffer));
#endif

        /* Error Computing */
        PictureDecodeHasErrors(TrickModeHandle, PictureJustDecoded_p);
    }
    else if (   (IsPictureSecondField(PictureJustDecoded_p))
             && (PictureJustDecoded_p->PreviousInStreamPicture_p != NULL)
             && (! (PictureJustDecoded_p->PreviousInStreamPicture_p->IsDecoded)))
    {
        /* Error Computing */
        PictureDecodeHasErrors(TrickModeHandle, PictureJustDecoded_p);
    }
    else
    {
#ifdef DEBUG_SHOW_DECODED_PICTURE
/* this sequence allows to display the just decode picture : only for 5508-18 DEBUG ! */
if (DebugFlag)
{
        /* chroma lunma */
        STSYS_WriteRegDev16BE(0x0C , ((U32) (PictureJustDecoded_p->PictureBuffer_p->FrameBuffer_p->Allocation.Address_p) / 256));
        STSYS_WriteRegDev16BE(0x58 , ((U32) (PictureJustDecoded_p->PictureBuffer_p->FrameBuffer_p->Allocation.Address2_p) / 256));

        /* Top / Bot */
        if (PictureJustDecoded_p->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD)
        {
            /* freeze top  */
            DcfHigh = STSYS_ReadRegDev8(0x74);
            /* Enable freeze on polarity. */
            DcfHigh |= 0x01;
            /* Freeze on TOP field. */
            DcfHigh &= ~0x40;
            STSYS_WriteRegDev8(0x74 , DcfHigh);
            DcfHigh = 0;
        }
        else
        {
            /* freeze bot */
            DcfHigh = STSYS_ReadRegDev8(0x74);
            DcfHigh |= 0x01;
            /* Freeze on BOTTOM field. */
            DcfHigh |= 0x40;
            STSYS_WriteRegDev8(0x74 , DcfHigh);
            DcfHigh = 0;
        }
}
#endif /* 0 */

        /* error recovery. 2 counters Frame or first field and second field. The reason is that if a second field is badly decoded */
        /* it has to be re-decoded but after the re-decode of its first field which can possibly be always TRUE ! */
        if (IsPictureSecondField(PictureJustDecoded_p))
        {
            VIDTRICK_Data_p->ErrorRecovery.SuccessiveDecodeSecondFieldErrorsNb = 0;
        }
        else
        {
            VIDTRICK_Data_p->ErrorRecovery.SuccessiveDecodeFrameOrFirstFieldErrorsNb = 0;
        }

        /* this picture is decoded ! */
#ifdef TRACE_UART_DEBUG
        TraceBuffer(("Picture 0x%x is decoded (%c%d)\n\r", PictureJustDecoded_p,
                                           MPEGFrame2Char(PictureJustDecoded_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame),
                                           PictureJustDecoded_p->PictureInfoFromDecode.pTemporalReference));
#endif

        PictureJustDecoded_p->IsDecoded = TRUE;
        /* This picture buffer does not contain the required VIDTRICK_PictureInfo any more */
        PictureJustDecoded_p->PictureBuffer_p->SmoothBackwardPictureInfo_p = PictureJustDecoded_p;

        /* if a second field has to be decoded, lauch the decode of second field */
        if (HasPictureASecondField(PictureJustDecoded_p))
        {
            /* decode second field */

#ifdef TRACE_UART_MAIN
            TraceBuffer(("Expecting a second field \n\r"));
#endif /* TRACE_UART_MAIN */

            if (PictureJustDecoded_p->PictureBuffer_p->FrameBuffer_p->NothingOrSecondFieldPicture_p == NULL)
            {
#ifdef TRICKMOD_DEBUG
                TrickModeError();
                 TraceBuffer(("Error! This picture needs a second field but NothingOrSecondFieldPicture_p == NULL !!!\n\r"));
#endif /* TRICKMOD_DEBUG */
                STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
                return;
            }

            /* Find Second field picture to decode */
            SecondFieldPictureToBeDecoded_p = PictureJustDecoded_p->SecondFieldPicture_p;
            if (SecondFieldPictureToBeDecoded_p == NULL)
            {
#ifdef TRICKMOD_DEBUG
                TrickModeError();
                TraceBuffer(("Error! SecondFieldPictureToBeDecoded_p == NULL !!!\n\r"));
#endif /* TRICKMOD_DEBUG */
                STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
                return;
            }

            /* inits */
            ResetPictureInfoStatus(TrickModeHandle, SecondFieldPictureToBeDecoded_p);
            SecondFieldPictureToBeDecoded_p->PictureBuffer_p = PictureJustDecoded_p->PictureBuffer_p->FrameBuffer_p->NothingOrSecondFieldPicture_p;
            VIDTRICK_Data_p->PictureUnderDecoding_p = SecondFieldPictureToBeDecoded_p;

#ifdef TRACE_UART_DEBUG
            TraceBuffer(("Take 2nd field 0x%x\r\n", SecondFieldPictureToBeDecoded_p->PictureBuffer_p));
#endif
            TAKE_PICTURE_BUFFER(VIDTRICK_Data_p->BufferManagerHandle, SecondFieldPictureToBeDecoded_p->PictureBuffer_p, VIDCOM_VIDTRICK_MODULE_BASE);

#ifdef TRACE_UART_TOPBOT_ALTERNATING
            TraceBuffer(("\n\rW(%d) %d %c %s\r\n", __LINE__,
                           SecondFieldPictureToBeDecoded_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference,
                           MPEGFrame2Char(SecondFieldPictureToBeDecoded_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame),
                           PictureStruct2String(SecondFieldPictureToBeDecoded_p->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure)));
#endif

#ifdef TRACE_UART_DEBUG
            TraceBuffer(("Launch 2nd field decode\r\n"));
#endif

            /* launch second field picture decode */
            DecodePictureParams.DataInputAddress_p = (void *)((U32)SecondFieldPictureToBeDecoded_p->AssociatedStartCodeAddress_p);
            DecodePictureParams.OutputPicture_p = PictureJustDecoded_p->PictureBuffer_p->FrameBuffer_p->NothingOrSecondFieldPicture_p;
            DecodePictureParams.OutputDecimatedPicture_p = DecodePictureParams.OutputPicture_p->AssociatedDecimatedPicture_p;
            DecodePictureParams.StreamInfoForDecode_p = &(SecondFieldPictureToBeDecoded_p->PictureInfoFromDecode.StreamInfoForDecode);
            DecodePictureParams.PictureInfos_p = &(SecondFieldPictureToBeDecoded_p->PictureInfoFromDecode.PictureInfos);
            DecodePictureParams.PictureStreamInfo_p = &(SecondFieldPictureToBeDecoded_p->PictureInfoFromDecode.AdditionalStreamInfo);
            DecodePictureParams.pTemporalReference = SecondFieldPictureToBeDecoded_p->PictureInfoFromDecode.pTemporalReference;

            vidtrick_FillPictureBuffer(DecodePictureParams.OutputPicture_p, SecondFieldPictureToBeDecoded_p);

            /* Decode the second field picture */
            VIDDEC_DecodePicture(VIDTRICK_Data_p->DecodeHandle, &DecodePictureParams);
        }
        else /* frame or 2nd field just decoded */
        {
            /* if 2nd field just decoded, all status are made in First Field structure */
            if (IsPictureSecondField(PictureJustDecoded_p))
            {
                FrameOrFirstFieldPicture_p = PictureJustDecoded_p->PreviousInStreamPicture_p;
            }
            else
            {
                FrameOrFirstFieldPicture_p = PictureJustDecoded_p;
            }

            /*------------------------------------------------------------------------*/
            /*- B picture ------------------------------------------------------------*/
            /*------------------------------------------------------------------------*/
            /* unlock if first field or if frame */
            /* if (FrameOrFirstFieldPicture_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B) */
            if (PictureMpegTypeIsB(FrameOrFirstFieldPicture_p))
            {
                /* unlock forward references */
                /* if previous chronological picture is a reference, forward ref is unuseful to decode any B pictures : it can be unlocked ! */
                if (PictureMpegTypeIsReference(FrameOrFirstFieldPicture_p->PreviousInStreamPicture_p))
                {
#ifdef TRICKMOD_DEBUG
                    if (   (FrameOrFirstFieldPicture_p->PictureBuffer_p == NULL)
                        || (!FrameOrFirstFieldPicture_p->ForwardOrNextReferencePicture_p->IsDecoded))
                    {
                        TrickModeError();
                        TraceBuffer(("New Decoded Picture Error\n\r"));
                    }
#endif /* TRICKMOD_DEBUG */

#ifdef TRACE_UART_MAIN
                    TraceBuffer(("Unlock forward reference \n\r"));
#endif /* TRACE_UART_MAIN */

                    /* Unlock forward reference */
                    UnlockPicture(TrickModeHandle, FrameOrFirstFieldPicture_p->ForwardOrNextReferencePicture_p, FALSE, FALSE, TRUE);

#ifdef TRICKMOD_DEBUG
                    if (IsPictureSecondField(FrameOrFirstFieldPicture_p->ForwardOrNextReferencePicture_p))
                    {
                        TrickModeError();
                    }
#endif /* TRICKMOD_DEBUG */
                }
            }
        }
     } /* if decode is OK */

     /* UnLock */
     STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);

     /* new picture decode : smooth backward process has to be woke up */
     semaphore_signal(VIDTRICK_Data_p->WakeTrickModeTaskUp_p);
} /* end of NewDecodedPicture() function */

/*******************************************************************************
Name        : NoNextPictureToDisplay
Description : Call back of NO_NEXT_PICTURE_TO_DISPLAY evt. This event is notified
              from the display task context, so the trickmode context is already
              protected (in VIDTRICK_VsyncActionSpeed).
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void NoNextPictureToDisplay(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p)
{
    VIDTRICK_Handle_t  		 TrickModeHandle;
    VIDTRICK_Data_t *        VIDTRICK_Data_p;

    TrickModeHandle = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDTRICK_Handle_t, SubscriberData_p);
    /* Find structure */
    VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;

    /* unused parameters */
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData_p);

    /* increment fields to skip counter */
    if (   (VIDTRICK_Data_p->Flags.SpeedControlEnabled)
        && (VIDTRICK_Data_p->MainParams.Speed != 100))
    {
#ifdef ST_smoothbackward
        if (   (VIDTRICK_Data_p->Flags.SmoothBackwardIsInitializing)
            || (VIDTRICK_Data_p->Flags.BackwardToForwardInitializing))  /* Forward mode is in initializing phase */
        {
            /* Ignore Event */
            return;
        }

        if (VIDTRICK_Data_p->MainParams.Speed < 0)
        {
            /* smooth backward is not able to provide enough pictures to display */
            /* display should not be fluent. A corrective action has to be done */

            /* This corrective action can only be done when at least one picture has been inserted in display queue */
            /* <==> first picture of table is decoded */
            if (!VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0]->IsDecoded)
            {
                return;
            }

            /* increment the counter of DisplayQueueEventEmptyNb and counter of fields to skip */
            VIDTRICK_Data_p->BackwardFluidity.DisplayQueueEventEmptyNb ++;
            VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat++;

#ifdef TRACE_UART_DISPLAY_SKIP
            TraceBuffer(("NoPic: SkipRepeat=%d\r\n",VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat));
#endif


#ifdef OLD_BACKWARD_FLUENT_DISPLAY_PROCESS
            if (   (VIDTRICK_Data_p->MainParams.Speed < 0)
                && (VIDTRICK_Data_p->BackwardFluidity.DisplayQueueEventEmptyNb > VIDTRICK_LEVEL_STOP_SKIPPING_DISPLAY))
            {
                VIDTRICK_Data_p->BackwardFluidity.BackwardFluentDisplayProcessIsRunning = TRUE;
            }
#endif /* OLD_BACKWARD_FLUENT_DISPLAY_PROCESS */

#ifdef TRICKMOD_DEBUG
            if (   (VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0]->PictureBuffer_p == NULL)
                || (VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ == 0))
            {
                TrickModeError();
            }
            /* else */
            /* { */
                /* TraceBuffer(("DisQueue empty. OnD: M=%d E=%d ind=%d\r\n" , */
                        /* VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0]->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame, */
                        /* VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0]->PictureBuffer_p->AdditionalStreamInfo.ExtendedTemporalReference, */
                        /* VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0]->IndiceInBuffer)); */
            /* } */
#endif
        }
        else if (VIDTRICK_Data_p->NegativeToPositiveSpeedParams.ForwardDisplayQueueInsertionIsEnabled)
#endif  /* ST_smoothbackward */
        {
            /* one field display has been repeated by display beacuse of a lack of pictures */
            /* in display queue. Compensatin must be done by incrmenting counter of fields to skip */
            VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat++;

#ifdef TRACE_UART_DISPLAY_SKIP
            TraceBuffer(("Neg2Pos: SkipRepeat=%d\r\n",VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat));
#endif
        }
    } /* if ((VIDTRICK_Data_p->Flags.SpeedControlEnabled) && (VIDTRICK_Data_p->MainParams.Speed != 100)) */

} /* end of NoNextPictureToDisplay() function */


/*******************************************************************************
Name        : PictureOutOfDisplay
Description : Function to subscribe the VIDBUFF event picture out of display.
              This event is notified from the display task context, so
              the trickmode context is already protected (in VIDTRICK_VsyncActionSpeed).
              1- Free buffers according to the picture which is signaled here
              out of display.
              2- Update the LastPictureInDisplayQueue list by taking into
              account the number of fields to skip.
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void PictureOutOfDisplay(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p)
{
    VIDTRICK_Handle_t     		TrickModeHandle;
    VIDBUFF_PictureBuffer_t   * JustOutOfDisplayPictureBuffer_p;
    VIDBUFF_PictureBuffer_t   * FrameFirstFieldPictureBuffer_p;
    VIDTRICK_PictureInfo_t    * JustOutOfDisplayPicture_p;
    VIDTRICK_Data_t           * VIDTRICK_Data_p;
    VIDDEC_ControlMode_t    ControlMode;

    TrickModeHandle = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDTRICK_Handle_t, SubscriberData_p);

    /* Find structure */
    VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;
    JustOutOfDisplayPictureBuffer_p = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDBUFF_PictureBuffer_t *, EventData_p);
    FrameFirstFieldPictureBuffer_p = NULL;

    /* if no smooth backward computed picture in the buffer : unlock it */
    JustOutOfDisplayPicture_p = (VIDTRICK_PictureInfo_t *)(JustOutOfDisplayPictureBuffer_p->SmoothBackwardPictureInfo_p);

    /* unused parameters */
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    VIDDEC_GetControlMode(VIDTRICK_Data_p->DecodeHandle, &ControlMode);

    if (   (JustOutOfDisplayPicture_p == NULL)
        || (VIDTRICK_Data_p->Stop.TrickmodIsStopped)
        || (ControlMode != VIDDEC_MANUAL_CONTROL_MODE) )
    {
        /* Nothing to do because we are not in Manual Mode or the Picture Buffer was not allocated by vid_tric */
        return;
    }

    /* can happen until smooth backward management process is not launched when current picture in frame buffers are skipped. */
    if (VIDTRICK_Data_p->Flags.SmoothBackwardIsInitializing)
    {
        /* "PictureOutOfDisplay" event while SmoothBackward is Initializing. This can happen for exemple in case of stop/start event */
#ifdef TRICKMOD_DEBUG
         TrickModeError();
         TraceBuffer(("PictureOutOfDisplay event while SmoothBackwardIsInitializing!\n\r"));
#endif /* TRICKMOD_DEBUG */
        return;
    }
    /* WARNING: This is temporary code while mutex is not supported in STOS for Linux. This is to solve a crash issue seen by SA */
#if defined(ST_OS21) || defined(ST_OS20)
    /* Lock TrickMode params */
    STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p);
#endif

#ifdef TRACE_UART_DEBUG
    TraceBuffer(("PB 0x%x out of display\n\r", JustOutOfDisplayPictureBuffer_p));
#endif

    /* Check that we began displaying from a new bitbuffer */
    if((JustOutOfDisplayPicture_p->LinearBitBufferStoredInfo_p != VIDTRICK_Data_p->SpeedAccuracy.PreviousBitBufferUsedForDisplay)

       &&((JustOutOfDisplayPictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B)
          ||(JustOutOfDisplayPicture_p->ForwardOrNextReferencePicture_p == NULL)
          ||(JustOutOfDisplayPicture_p->ForwardOrNextReferencePicture_p->LinearBitBufferStoredInfo_p == JustOutOfDisplayPicture_p->LinearBitBufferStoredInfo_p)))
    {
        if(VIDTRICK_Data_p->SpeedAccuracy.PreviousBitBufferUsedForDisplay == NULL)
        {
            /* This is the first picture displayed in backward trickmodes, so no
             * way to check for accuracy at this time. */

            /* Remember for next time that a display from a new bitbuffer began */
            VIDTRICK_Data_p->SpeedAccuracy.PreviousBitBufferBeginningOfDisplay = time_now();
        }
        else
        {
            clock_t PreviousBitBufferDisplayDuration;
            U32 NBFieldsDisplayed, SkipThatShouldHaveBeenDone;
            U32 FieldRate = ((VIDTRICK_Data_p->MainParams.FrameRateValue * 2) + 999) / 1000;
            S32 Speed = -VIDTRICK_Data_p->MainParams.Speed;
            U32 SkipDone = VIDTRICK_Data_p->SpeedAccuracy.PreviousSkip;
            clock_t PreviousBitBufferBeginningOfDisplay = VIDTRICK_Data_p->SpeedAccuracy.PreviousBitBufferBeginningOfDisplay;

            /* Compute the time duration that elapsed from the beginning of the
             * display of pictures from the previous bitbuffer */
            PreviousBitBufferDisplayDuration = time_minus(time_now(), PreviousBitBufferBeginningOfDisplay);
            NBFieldsDisplayed = (PreviousBitBufferDisplayDuration * FieldRate) / ST_GetClocksPerSecond();
#ifdef TRACE_UART_MAIN
            TraceSpeedAccuracy(("Fields Displayed from the last bitbuffer = %d during %lu ms\r\n",
                                NBFieldsDisplayed,
                                (PreviousBitBufferDisplayDuration * 1000) / ST_GetClocksPerSecond() ));
#endif

            /* To have an accurate speed, at speed xY, we must display A fields from
               each injection, and we skip A(Y-1) fields.
               Here we compute the late we have because we haven't followed the
               previous rule */
            SkipThatShouldHaveBeenDone = (NBFieldsDisplayed * Speed) / 100;
            VIDTRICK_Data_p->SpeedAccuracy.NBFieldsLate += SkipDone - SkipThatShouldHaveBeenDone ;

#ifdef TRACE_UART_MAIN
            TraceSpeedAccuracy(("Skip : Done %d Shouldbe %d Diff %d\r\n", SkipDone, SkipThatShouldHaveBeenDone, VIDTRICK_Data_p->SpeedAccuracy.NBFieldsLate));
#endif
            /* Remember for next time when a display from a new bitbuffer began */
            VIDTRICK_Data_p->SpeedAccuracy.PreviousBitBufferBeginningOfDisplay = time_now();
        }

        /* Remember the bitbuffer the current picture belongs to */
        VIDTRICK_Data_p->SpeedAccuracy.PreviousBitBufferUsedForDisplay = JustOutOfDisplayPicture_p->LinearBitBufferStoredInfo_p;
    }


    /* TraceBuffer(("      OutOfQ: M=%d E=%d ind=%d Buf=0x%x exp2ndf=%d\r\n", */
            /* JustOutOfDisplayPicture_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame, */
            /* JustOutOfDisplayPicture_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference, */
            /* JustOutOfDisplayPicture_p->IndiceInBuffer, */
            /* JustOutOfDisplayPicture_p->LinearBitBufferStoredInfo_p, */
            /* JustOutOfDisplayPicture_p->PictureInfoFromDecode.AdditionalStreamInfo.ExpectingSecondField)); */

    /* Picture has just been out of display */
    JustOutOfDisplayPicture_p->IsGoneOutOfDisplay = TRUE;

    if (JustOutOfDisplayPicture_p != VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0])
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("OutOfDisp pic not first pic of the table (0x%x)!\r\n", VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0]));
#endif /* TRICKMOD_DEBUG */
    }
    PurgeFrameBuffers(TrickModeHandle);
    /* WARNING: This is temporary code while mutex is not supported in STOS for Linux. This is to solve a crash issue seen by SA */
#if defined(ST_OS21) || defined(ST_OS20)
    /* Release access to TrickMode params */
    STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
#endif
} /* End of PictureOutOfDisplay() function */

/*******************************************************************************
Name        : PurgeFrameBuffers
Description : This function checks which framebuffers could be unlocked. It is
              called each time a new picture goes out of the display. The first
              implementation of this functionality was to check only the current
              picture out of display, supposing that the pictures goes out of
              the display in the same order they goes in (FIFO). This created a
              few problems because pictures might be rejected from the display
              on their insertion, and then framebuffers are never unlocked. The
              current implementation tries to consider the first pictures of the
              LastPictureInDisplayQueue_p table that went out of the display,
              and tries to unlock them. In that way, if a picture goes out of
              display not in the display order, it will be checked later for
              unlock, when all the previous pictures are no more displayed.
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void PurgeFrameBuffers(const VIDTRICK_Handle_t TrickModeHandle)
{
    VIDTRICK_PictureInfo_t    * JustOutOfDisplayPicture_p = NULL;
    VIDTRICK_PictureInfo_t    * CurrentlyDisplayedPicture_p = NULL;
    VIDTRICK_Data_t           * VIDTRICK_Data_p;
    VIDBUFF_PictureBuffer_t   * JustOutOfDisplayPictureBuffer_p = NULL;

    /* Find structure */
    VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;

#ifdef TRACE_UART_DEBUG
    TraceBuffer(("PurgeFrameBuffers\r\n"));
#endif

    while((VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ > 1)
            && (VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0]->HasBeenInsertedInDisplayQueue)
             && (VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0]->IsGoneOutOfDisplay ))
    {
        JustOutOfDisplayPicture_p = VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0];
        JustOutOfDisplayPictureBuffer_p = JustOutOfDisplayPicture_p->PictureBuffer_p;
        /* Now we can consider that the picture is no more on display, to allow
         * unlocking the framebuffer */
        JustOutOfDisplayPicture_p->IsInDisplayQueue = FALSE;

        /* Currently Displayed picture is VIDTRICK_Data_p->LastPictureInDisplayQueue_p[1] */
        CurrentlyDisplayedPicture_p = VIDTRICK_Data_p->LastPictureInDisplayQueue_p[1];

#ifdef TRICKMOD_DEBUG
        if (CurrentlyDisplayedPicture_p == NULL)
        {
            TrickModeError();
        }
#endif /* TRICKMOD_DEBUG */

        /* if first field or frame out of display, unlock convenient pictures. */
        if (!IsPictureSecondField(JustOutOfDisplayPicture_p))
        {
            /*------------------------------------------------------------------------*/
            /*- I or P picture -------------------------------------------------------*/
            /*------------------------------------------------------------------------*/
            if (JustOutOfDisplayPictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame != STVID_MPEG_FRAME_B)
            {
                /* if Currently displayed is a reference : just out of diplay picture unuseful to decode any B pictures */
                if (PictureMpegTypeIsReference(CurrentlyDisplayedPicture_p))
                {
                    /* unlock if necessary this picture */
                    UnlockPicture(TrickModeHandle, JustOutOfDisplayPicture_p, TRUE, FALSE, FALSE);
                }
                else
                {
                    /* JustOutOfDisplayPictureBuffer_p may still be useful to decode some B pictures so we don't release it */

                    /* unlock if necessary this picture  : it will be definitively unlocked */
                    UnlockPicture(TrickModeHandle, JustOutOfDisplayPicture_p, FALSE, TRUE, FALSE);
                }
            }
            /*------------------------------------------------------------------------*/
            /*- B picture ------------------------------------------------------------*/
            /*------------------------------------------------------------------------*/
            else
            {
                /* Unlock the frame buffer containg the just decoded B picture */
                UnlockPicture(TrickModeHandle, JustOutOfDisplayPicture_p, TRUE, FALSE, FALSE);
            }

            /* check if next buffer to be transfered is fully unused and ready to receive new datas */
            CheckIfLinearBufferHasToBeFlushed(TrickModeHandle, CurrentlyDisplayedPicture_p);
        }
        else
        {
            /* If Seconf field : Unlock (Second field status is only used by display and GetUnusedBuffer function */
            ReleasePictureInfoElement(TrickModeHandle, JustOutOfDisplayPicture_p);
        }

        /* MAJ of the table of next pictures to be inserted in display queue */
        UpdateFirstPicturesInDisplayQueue(TrickModeHandle);

#ifdef TRICKMOD_DEBUG
        /* The out of diplay picture should has been removed from the table. */
        if (VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ == (VIDTRICK_Data_p->NbFieldsToBeInDisplayQueue + 2))
        {
            TrickModeError();
        }
#endif /* TRICKMOD_DEBUG */
    }

    /* new picture out of display : smooth backward process has to be woke up */
    semaphore_signal(VIDTRICK_Data_p->WakeTrickModeTaskUp_p);
} /* End of PurgeFrameBuffers() function */

/*******************************************************************************
Name        : PutPictureStartCodeAndComputeValidDataBuffer
Description : Put a picture start code at the end of the buffer. It will be used
              in order to stop the parsing.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static BOOL PutPictureStartCodeAndComputeValidDataBuffer( const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const LinearBitBuffer_p, const BOOL Overlap)
{
    VIDTRICK_Data_t *   VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    U32                 MinDataInjected;
    U32                 SCDAlignment;
    U8 *                BufferPointer_p;

	UNUSED_PARAMETER(Overlap);
    /* U32 i; */
    /* static U32 size = 20000; */
    /* static BOOL DebugFlag = FALSE; */

    /* CAUTION : DataEndAddress_p must have the same alignement than SCD_RD  */
    /* register! As the size of the data of a Start Code is less or egal     */
    /* than 257 bytes, we will discard as many blocks to fill 257 + SC + 1.  */
    /* Indeed there is a risk that a SC in this area could be not complete.  */
    /* Although there is no risk with a size greater of 1.                   */

    /* Get the size of the data transfered in the buffer                     */
    ErrorCode = VIDDEC_GetLinearBitBufferTransferedDataSize(VIDTRICK_Data_p->DecodeHandle, &(LinearBitBuffer_p->TransferedDataSize));
#ifdef TRICKMOD_DEBUG
    if (ErrorCode != ST_NO_ERROR)
    {
        TrickModeError();
       /* TraceBuffer(("VIDDEC_GetLinearBitBufferTransferedDataSize failed with ErrorCode=%d.\n\r", ErrorCode)) ;*/
    }
#endif /* TRICKMOD_DEBUG */

    /* Computing MinDataInjected with Bit rate may not be incoherent if various bit rate ... */
    /* we prefer a fixed size */
    MinDataInjected = VIDTRICK_MIN_DATA_SIZE_TO_BE_INJECTED;

    /* Workaround for an eventual failing of the hardware (seen on 5518, 5517). */
    if (LinearBitBuffer_p->TransferedDataSize > ((U32)LinearBitBuffer_p->WholeBufferEndAddress_p - (U32)LinearBitBuffer_p->WholeBufferStartAddress_p + 1))
    {
#ifdef TRICKMOD_DEBUG
       TrickModeError();
       TraceBuffer(("Bad Transfer Size (%d). Re-flush just flushed buffer \r\n", LinearBitBuffer_p->TransferedDataSize));
#endif /* TRICKMOD_DEBUG */
       /* Size is not convenient : ReFlush the just flushed buffer */
       AskForLinearBitBufferFlush(TrickModeHandle, LinearBitBuffer_p, VIDTRICK_FLUSH_BBFSIZE_BACKWARD_JUMP);
       return (FALSE);
    }
    else if (LinearBitBuffer_p->TransferedDataSize < MinDataInjected)
    {
#ifdef TRICKMOD_DEBUG
       TrickModeError();
       TraceBuffer(("Not enough data injected (%d). \r\n", LinearBitBuffer_p->TransferedDataSize));
#endif /* TRICKMOD_DEBUG */
       AskForLinearBitBufferFlush(TrickModeHandle, LinearBitBuffer_p, VIDTRICK_FLUSH_WITH_NORMAL_OVERLAP);
       return (FALSE);
    }

    /* End Address of the valid data buffer */
    LinearBitBuffer_p->TransferedDataEndAddress_p = (void *)((U32)(LinearBitBuffer_p->ValidDataBufferStartAddress_p) + LinearBitBuffer_p->TransferedDataSize);

    /* Put a Picture Start Code at the end of the buffer in unit of the SCD  */
    /* pointer. It will be useful to find the end of the buffer.             */
    ErrorCode = VIDDEC_GetSCDAlignmentInBytes(VIDTRICK_Data_p->DecodeHandle, &SCDAlignment);
#ifdef TRICKMOD_DEBUG
    if (ErrorCode != ST_NO_ERROR)
    {
        TrickModeError();
       /* TraceBuffer(("VIDDEC_GetSCDAlignmentInBytes failed with ErrorCode=%d.\n\r", ErrorCode)) ;*/
    }
#endif /* TRICKMOD_DEBUG */


    /* !!! temporary debug !!! */
    /* if (DebugFlag) */
    /* { */
        /* BufferPointer_p = (U8*)((U32)LinearBitBuffer_p->ValidDataBufferStartAddress_p); */
        /* for (i=0 ; i<size; i++) */
        /* { */
            /* *(BufferPointer_p + i) = 0xCC; */
        /* } */
    /* } */
    /* !!! end of temporary debug !!! */

    /* Write the Picture Start Code.                                        */
    /* DG TO DO: For secure platforms, address of SC in ES Copy buffer to pass as parameter also */
    BufferPointer_p = (U8*)(((U32)LinearBitBuffer_p->TransferedDataEndAddress_p) & (U32)(~(SCDAlignment - 1)));
    VIDDEC_WriteStartCode(VIDTRICK_Data_p->DecodeHandle, 0x0, (void*)BufferPointer_p);

    /* Useful data are stored before the cut data.                           */
    LinearBitBuffer_p->ValidDataBufferEndAddress_p =
            (void *)
            ((U32)BufferPointer_p -
             LinearBitBuffer_p->NumberOfBytesToDiscardAtTheEndOfTheBuffer);

    /* Data buffer size compute.                                             */
    LinearBitBuffer_p->ValidDataBufferSize =
            (U32)(LinearBitBuffer_p->ValidDataBufferEndAddress_p) -
            (U32)(LinearBitBuffer_p->ValidDataBufferStartAddress_p);

    return (TRUE);
} /* end of PutPictureStartCodeAndComputeValidDataBuffer() function */

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
        const VIDTRICK_Handle_t             TrickModeHandle,
        VIDTRICK_LinearBitBuffer_t * const  LinearBitBuffer_p)
{
    VIDTRICK_Data_t *   VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;
    U32                 AlignmentInBytes, i;
    U8 *                BufferPointer_p;

    /* CAUTION : DataStartAddress_p must have the same alignement than       */
    /* CDFifo registers ! So the space between BufferstartAddress_p and      */
    /* DataStartAddress_p must be filled with stuffing bytes and a sequence  */
    /* start code.                                                           */

    AlignmentInBytes = /*VIDDEC_GetSCDAlignmentInBytes*/  GetMaxAlignmentBetweenCDFifoAndSCD(VIDTRICK_Data_p->DecodeHandle);

    /* Put Sequence start code at the begin of the buffer.                   */
    BufferPointer_p = (U8 *)(LinearBitBuffer_p->WholeBufferStartAddress_p);

#ifdef TRICKMOD_DEBUG
    if (BufferPointer_p == NULL)
    {
        TrickModeError();
        TraceBuffer(("NULL pointer.\n\r"));
    }
#endif /* TRICKMOD_DEBUG */

    /* stuffing */
    if (AlignmentInBytes > VIDTRICK_START_CODE_SIZE)
    {
        for (i = VIDTRICK_START_CODE_SIZE; i < AlignmentInBytes; i++)
        {
            STSYS_WriteRegMemUncached8((BufferPointer_p + i), 0xFF);
        }
    }

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
Name        : DisplayDiscontinuityAffectPictures
Description : When no matching picture found or for high speed :
              - affect the first displayable picture of the just parsed buffer,
              - affect links between the 2 buffers through the first and the last
              displayable pictures.
Parameters  : TrickModeHandle, JustParsedBuffer_p
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void DisplayDiscontinuityAffectPictures(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const JustParsedBuffer_p)
{
    VIDTRICK_PictureInfo_t   * FirstDisplayablePicture_p;
    VIDTRICK_PictureInfo_t   * LastDisplayablePicture_p;

    /* Find Last Displayable picture of Associated buffer now */
    LastDisplayablePicture_p = JustParsedBuffer_p->AssociatedLinearBitBuffer_p->LastDisplayablePicture_p;
    if (LastDisplayablePicture_p != NULL)
    {
        /* find first displayable picture of just parsed buffer */
        FindFirstDisplayablePictureOfLinearBitBuffer(TrickModeHandle, JustParsedBuffer_p, &FirstDisplayablePicture_p);
#ifdef TRICKMOD_DEBUG
        if (FirstDisplayablePicture_p == NULL)
        {
            TrickModeError();
        }
#endif
        LastDisplayablePicture_p->FirstDisplayablePictureAfterJump_p = FirstDisplayablePicture_p;
    }
#ifdef TRICKMOD_DEBUG
    else
    {
        TrickModeError();
    }
#endif
} /* end of DisplayDiscontinuityAffectPictures() function */

/*******************************************************************************
Name        : FindMatchingPictureWithTemporalReference
Description : Find matching picture if not found by PTS
Parameters  : TrickModeHandle, CurrentBuffer_p
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static BOOL FindMatchingPictureWithTemporalReference(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const CurrentBuffer_p)
{
    VIDTRICK_Data_t             * VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;
    VIDTRICK_PictureInfo_t      * AssociatedBufferPictureInfoToBeCompared_p;
    VIDTRICK_PictureInfo_t      * CurrentBufferPictureInfo_p;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    BOOL    MatchingPictureHasBeenFound = FALSE;
    U32     SizeOfPictureInCurrentBuffer;
    U32     SizeOfPictureInAssociatedBuffer;
    U32     idx;
    S32     DifBetweenBothSize;
    U8      StartCode;
    U8*     CurrentBufferFirstSliceStartCodeAddress_p, *CurrentBufferFirstSliceStartCodeAddressCPU_p;
    U8*     AssociatedBufferFirstSliceStartCodeAddress_p, *AssociatedBufferFirstSliceStartCodeAddressCPU_p;
    U32     i = 0;
    BOOL    CurrentBufferFoundStartCode;
    BOOL    AssociatedBufferFoundStartCode = 0;

    /* Set local variables to ease code reading */
    AssociatedBufferPictureInfoToBeCompared_p = CurrentBuffer_p->AssociatedLinearBitBuffer_p->PictureInfoTableStartAddress_p;
    SizeOfPictureInAssociatedBuffer =  (U32)(AssociatedBufferPictureInfoToBeCompared_p[1].AssociatedStartCodeAddress_p)
                                     - (U32)(AssociatedBufferPictureInfoToBeCompared_p[0].AssociatedStartCodeAddress_p);

    /* Initialize Matching picture info */
    VIDTRICK_Data_p->MatchingPicture.ParsedBufferMatchingPictureNumber = CurrentBuffer_p->NbPicturesInside;
    VIDTRICK_Data_p->MatchingPicture.LastPictureIndex = CurrentBuffer_p->NbPicturesInside;
    VIDTRICK_Data_p->MatchingPicture.MatchingPicture_p = AssociatedBufferPictureInfoToBeCompared_p;

    /* Compare Temporal Reference */
    idx = CurrentBuffer_p->NbPicturesInside - 1;          /* First tested picture is last but one picture of current buffer */
    CurrentBufferPictureInfo_p = &CurrentBuffer_p->PictureInfoTableStartAddress_p[idx - 1];
    while ((!MatchingPictureHasBeenFound) && (idx > 0))
    {
        if (   (PictureMpegTypeIsI(CurrentBufferPictureInfo_p))
            && (!(IsPictureSecondField(CurrentBufferPictureInfo_p))))
        {
            VIDTRICK_Data_p->MatchingPicture.LastPictureIndex = idx;
        }

        /* First : compare temporal references, Mpeg types, and picture structures */
        if (   (AssociatedBufferPictureInfoToBeCompared_p->PictureInfoFromDecode.pTemporalReference == CurrentBufferPictureInfo_p->PictureInfoFromDecode.pTemporalReference)
            && (AssociatedBufferPictureInfoToBeCompared_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame == CurrentBufferPictureInfo_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame)
            && (AssociatedBufferPictureInfoToBeCompared_p->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure == CurrentBufferPictureInfo_p->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure))
        {
            /* Get size by substracting next picture address with current picture address. The size value is not accurate for some
             * chips like 55x because startcodes'addresses are SCDAligned */
            SizeOfPictureInCurrentBuffer = (U32)((CurrentBufferPictureInfo_p+1)->AssociatedStartCodeAddress_p)
                                         - (U32)(CurrentBufferPictureInfo_p->AssociatedStartCodeAddress_p);

            /* TraceBuffer(("Size1 :%d  Size2 :%d Nb :%d TR =%d\n\r",SizeOfPictureInCurrentBuffer, SizeOfPictureInAssociatedBuffer, i-1,CurrentBufferPictureInfo_p->PictureInfoFromDecode.pTemporalReference));*/
            /* Search for the first slice address in the current buffer */

            /* Second : Compare sizes. The size value is not accurate because of the SCDAlignment.
             * size = realsize +- (SCDAlignement-1).
             * Difference = size1 - size2 = realsize1 - realsize2 +- 2*(SCAlignement-1)
             * So if realsize1 = realsize2 (same picture) then difference = +- 2*(SCAlignement-1) */
            DifBetweenBothSize = (S32)SizeOfPictureInCurrentBuffer - (S32)SizeOfPictureInAssociatedBuffer;

            if (   (DifBetweenBothSize >= -(S32)(2 * (VIDTRICK_Data_p->SCDAlignmentInBytes-1)))
                && (DifBetweenBothSize <=  (S32)(2 * (VIDTRICK_Data_p->SCDAlignmentInBytes-1))))
            {
                /* Third : Compare x bytes from the two pictures data */

                /* Find manually the real address of the first slice startcode because with 55xx the returned startcode address
                 * is SCD aligned. We begin searching the startcode from the address - 3 because I don't know if the startcode
                 * address is the address of the first 0 in the sequence (00) 00 01 or the address of the byte that gives the
                 * startcode value 00 00 01 (xx). */

                /* Find it for the current picture buffer */
                CurrentBufferFirstSliceStartCodeAddress_p = (U8*) CurrentBufferPictureInfo_p->AssociatedStartCodeAddress_p - 3;
                do
                {
                    CurrentBufferFoundStartCode =
                            vidtrick_SearchAndGetNextStartCode(CurrentBufferFirstSliceStartCodeAddress_p,                         /* search from */
                                                               (U8*) ((U32)CurrentBufferPictureInfo_p->AssociatedStartCodeAddress_p
                                                                     + VIDTRICK_Data_p->SCDAlignmentInBytes),                     /* Max search address */
                                                               &StartCode,                                                        /* Startcode value */
                                                               &CurrentBufferFirstSliceStartCodeAddress_p);                       /* Startcode address */
                } while((CurrentBufferFoundStartCode) && (!((StartCode <= GREATEST_SLICE_START_CODE) && (StartCode >= FIRST_SLICE_START_CODE))));
#ifdef TRICKMOD_DEBUG
                if(!CurrentBufferFoundStartCode)
                {
                    TraceBuffer(("Haven't found the slice startcode in the Current buffer !\r\n"));
                }
#endif
                /* Find it for the associated picture buffer */
                if(CurrentBufferFoundStartCode)
                {
                    AssociatedBufferFirstSliceStartCodeAddress_p = (U8*) AssociatedBufferPictureInfoToBeCompared_p->AssociatedStartCodeAddress_p - 4;
                    do
                    {
                        AssociatedBufferFoundStartCode =
                                vidtrick_SearchAndGetNextStartCode(AssociatedBufferFirstSliceStartCodeAddress_p,                      /* search from */
                                                                (U8*) ((U32)AssociatedBufferPictureInfoToBeCompared_p->AssociatedStartCodeAddress_p
                                                                        + VIDTRICK_Data_p->SCDAlignmentInBytes),                     /* Max search address */
                                                                &StartCode,                                                        /* Startcode value */
                                                                &AssociatedBufferFirstSliceStartCodeAddress_p);                    /* Startcode address */
                    }
                    while((AssociatedBufferFoundStartCode) && (!((StartCode <= GREATEST_SLICE_START_CODE) && (StartCode >= FIRST_SLICE_START_CODE))));
                }
#ifdef TRICKMOD_DEBUG
                if(!AssociatedBufferFoundStartCode)
                {
                    TraceBuffer(("Haven't found the slice startcode in the associated buffer !\r\n"));
                }
#endif

                /* Now compare the VIDTRICK_NUMBER_OF_BYTES_TO_COMPARE_WHEN_MATCHING_TWO_LINEAR_BITBUFFERS bytes of the
                 * picture's data, to ensure that the current picture and the associated buffer are the same */
                STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );
                AssociatedBufferFirstSliceStartCodeAddressCPU_p =
                                    STAVMEM_DeviceToCPU(AssociatedBufferFirstSliceStartCodeAddress_p,&VirtualMapping);
                            CurrentBufferFirstSliceStartCodeAddressCPU_p =
                                    STAVMEM_DeviceToCPU(CurrentBufferFirstSliceStartCodeAddress_p,&VirtualMapping);
                if((CurrentBufferFoundStartCode)&&(AssociatedBufferFoundStartCode))
                {
                    /* Compare the first x bytes after */
                     if ((AssociatedBufferFirstSliceStartCodeAddressCPU_p != NULL) &&
                            (CurrentBufferFirstSliceStartCodeAddressCPU_p !=NULL))
                     {
                        for(i = 0; i < VIDTRICK_NUMBER_OF_BYTES_TO_COMPARE_WHEN_MATCHING_TWO_LINEAR_BITBUFFERS; i++)
                        {
                            if(AssociatedBufferFirstSliceStartCodeAddressCPU_p[i+1] != CurrentBufferFirstSliceStartCodeAddressCPU_p[i+1])
                            {
                                break;
                            }
                        }
                    }
                    /* Are all the x first bytes equal ? */
                    if(i == VIDTRICK_NUMBER_OF_BYTES_TO_COMPARE_WHEN_MATCHING_TWO_LINEAR_BITBUFFERS)
                    {
                        U32 PTSDiff;    /* PTS diff between the new and the old PTS of the matching picture */
                        MatchingPictureHasBeenFound = TRUE;
                        VIDTRICK_Data_p->MatchingPicture.ParsedBufferMatchingPictureNumber = idx;

                        PTSDiff = AssociatedBufferPictureInfoToBeCompared_p->PictureInfoFromDecode.PictureInfos.VideoParams.PTS.PTS -
                         CurrentBuffer_p->PictureInfoTableStartAddress_p[idx-1].PictureInfoFromDecode.PictureInfos.VideoParams.PTS.PTS;
                        idx--;
                        while (idx < CurrentBuffer_p->NbPicturesInside)
                        {
                            /* Here copy the old value of PTS of matched pictures */
                            if (CurrentBuffer_p->PictureInfoTableStartAddress_p[idx].PictureInfoFromDecode.PictureInfos.VideoParams.PTS.Interpolated)
                            {
                                CurrentBuffer_p->PictureInfoTableStartAddress_p[idx].PictureInfoFromDecode.PictureInfos.VideoParams.PTS.PTS =
                                AssociatedBufferPictureInfoToBeCompared_p->PictureInfoFromDecode.PictureInfos.VideoParams.PTS.PTS;
                            }
                            AssociatedBufferPictureInfoToBeCompared_p ++;
                            idx++;
                        }
                        idx = VIDTRICK_Data_p->MatchingPicture.ParsedBufferMatchingPictureNumber -1;
                        while (idx > 0)
                        {
                            /* Here translate the new picture PTSs' by the GAP between the matched picture PTS and the matching picture PTS */
                            idx--;
                            if (CurrentBuffer_p->PictureInfoTableStartAddress_p[idx].PictureInfoFromDecode.PictureInfos.VideoParams.PTS.Interpolated)
                            {
                                CurrentBuffer_p->PictureInfoTableStartAddress_p[idx].PictureInfoFromDecode.PictureInfos.VideoParams.PTS.PTS +=
                                 PTSDiff;
                            }
                        }
                        idx = VIDTRICK_Data_p->MatchingPicture.ParsedBufferMatchingPictureNumber;
                    }
                }
#ifdef TRICKMOD_DEBUG
                else
                {
                    if(!CurrentBufferFoundStartCode)
                    {
                        TrickModeError();
                        TraceBuffer(("Can't find the current picturebuffer's startcode\n\r"));
                    }
                    if(!AssociatedBufferFoundStartCode)
                    {
                        TrickModeError();
                        TraceBuffer(("Can't find the associated picturebuffer's startcode\n\r"));
                    }
                }
#endif /* TRICKMOD_DEBUG */
            }
        }  /* temporal references, picture structures, and mpeg types are the same */
        idx--;
        CurrentBufferPictureInfo_p--;
    }

    return(MatchingPictureHasBeenFound);
} /* end of FindMatchingPictureWithTemporalReference() function */

/*******************************************************************************
Name        : FindPictureToComputeFromAndInsertInDisplayQueue
Description : This function inserts in display queue the decoded pictures chosen to be displayed by
              backward algorithm. It also returns the first not decoded picture of the table
              in order to have it decoded
Parameters  : TrickModeHandle, CurrentBuffer_p
Assumptions :
Limitations :
Returns     : Nothing.
*******************************************************************************/
static void FindPictureToComputeFromAndInsertInDisplayQueue(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t ** const PictureToComputeFrom_p)
{
    VIDTRICK_Data_t                      *  VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;
    VIDTRICK_PictureInfo_t *                PictureToInsertInDQ_p = NULL;
    VIDTRICK_PictureInfo_t *                PictureInfo_p = NULL;
    VIDTRICK_PictureInfo_t *                LastInsertedPictureInDQ_p = NULL;
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;
    U32                                     FirstPic;
    U32                                     NbFieldsToBeInDisplayQueue;

#ifdef TRACE_UART_DEBUG
    /*TraceBuffer(("FindPictureToComputeFromAndInsertInDisplayQueue\n\r"));*/
#endif
    /* Display queue should be filled with VIDTRICK_PICTURES_NB_TO_BE_IN_DISPLAY_QUEUE frame picutres, so */
    /* NbFieldsToBeInDisplayQueue is initialised to VIDTRICK_PICTURES_NB_TO_BE_IN_DISPLAY_QUEUE and incremented */
    /* for each Second field in display queue */
    NbFieldsToBeInDisplayQueue = VIDTRICK_Data_p->NbFieldsToBeInDisplayQueue;

    /* Find First picture to insert in Display queue : it is the first decoded but not in display queue picture. */
    /* The display queue must be filled with only 2 pictures */
    FirstPic = 0;
    do
    {
        PictureInfo_p = VIDTRICK_Data_p->LastPictureInDisplayQueue_p[FirstPic];
            if (IsPictureSecondField(PictureInfo_p))
            {
                NbFieldsToBeInDisplayQueue ++;
            }
            FirstPic++;
    } while (   (FirstPic < VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ)
             && (FirstPic < NbFieldsToBeInDisplayQueue)
             && (PictureInfo_p->IsInDisplayQueue)
             && (PictureInfo_p->HasBeenInsertedInDisplayQueue));
    /* FirstPic is thus the index of the first picture not in DQ in the table */

    /* if found a picture to insert in display queue. */
        if (   (!PictureInfo_p->IsInDisplayQueue)
            && (PictureInfo_p->IsDecoded))
        {
            PictureToInsertInDQ_p = PictureInfo_p;
            if (FirstPic > 1)  /* should always be true except if no picture still displayed or if only one picture */
                                    /* in a bit buffer choosen to be displayed */
            {
                LastInsertedPictureInDQ_p = VIDTRICK_Data_p->LastPictureInDisplayQueue_p[FirstPic-2];
    #ifdef TRICKMOD_DEBUG
                if (LastInsertedPictureInDQ_p == NULL)
                {
                    TrickModeError();
                }
    #endif
            }
    }
    else
    {
#ifdef TRACE_UART_DEBUG
        TraceBuffer(("No pict ready for display (0x%x %d %d)\n\r", PictureInfo_p, PictureInfo_p->IsInDisplayQueue, PictureInfo_p->IsDecoded));
#endif
    }

    /* Speed may has been changed : then all the not still in display queue pictures are to be re-computed */
    /* in order avoid jumps once the speed has been changed. we can verify it with the nd of fields to skip */
    if (VIDTRICK_Data_p->Flags.SmoothBackwardSpeedHasBeenChanged)
    {
#ifdef TRACE_UART_DEBUG
        TraceBuffer(("Speed has changed\n\r"));
#endif
        /* smoth backward speed change taken under account */
        VIDTRICK_Data_p->Flags.SmoothBackwardSpeedHasBeenChanged = FALSE;

        /* NbFieldsToBeInDisplayQueue != VIDTRICK_PICTURES_NB_TO_BE_IN_DISPLAY_QUEUE means that field pictures */
        /* are stored in display queue. following feature is not applicated only for frame streams */
        if (NbFieldsToBeInDisplayQueue == VIDTRICK_Data_p->NbFieldsToBeInDisplayQueue)
        {
            if (   (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat < VIDTRICK_LEVEL_SKIP_BACKWARD_PICTURE)
                && (VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ >= VIDTRICK_Data_p->NbFieldsToBeInDisplayQueue))
            {
                /* a little value of this counter is a good way to detect a speed chenge : if inferior of VIDTRICK_LEVEL_SKIP_BACKWARD_PICTURE */
                /* should be enough to re compute pictures to be inserted. Reduce table of pictures to only those which are in display queue */
                VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ = VIDTRICK_Data_p->NbFieldsToBeInDisplayQueue;

                /* wake task up to re-compute the whole table. */
                *PictureToComputeFrom_p = NULL;
                semaphore_signal(VIDTRICK_Data_p->WakeTrickModeTaskUp_p);
                return;
            }
        }
    }

    /* find the first computable picture : the first no decoded picture */
    FirstPic = 0;
    do
    {
        PictureInfo_p = VIDTRICK_Data_p->LastPictureInDisplayQueue_p[FirstPic];
            if (   (PictureInfo_p->IsDecoded)
                && (FirstPic+1 < VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ))
            {
                /* Clean all the Frame Buffers containing unecessary forward references. */
                FreeUnnecessaryFrameBuffers( TrickModeHandle,
                                            VIDTRICK_Data_p->LastPictureInDisplayQueue_p[FirstPic],
                                            VIDTRICK_Data_p->LastPictureInDisplayQueue_p[FirstPic+1] );
            }
            FirstPic++;
    }  while (   (FirstPic < VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ)
              && (PictureInfo_p->IsDecoded));

    /* decoded or not picture to compute from is found */
        if (!PictureInfo_p->IsDecoded)
        {
            *PictureToComputeFrom_p = PictureInfo_p;
        }
        else
        {
            *PictureToComputeFrom_p = NULL;
        }
    /* if found a picture to insert in display queue : */
    if (PictureToInsertInDQ_p != NULL)
    {
        /* debug test */
#ifdef TRICKMOD_DEBUG
        if (   (!PictureToInsertInDQ_p->IsDecoded)
            || (PictureToInsertInDQ_p->IsInDisplayQueue)
            || (PictureToInsertInDQ_p->PictureBuffer_p->SmoothBackwardPictureInfo_p == NULL))
        {
            TrickModeError();
        }
#endif /* TRICKMOD_DEBUG */

        if (LastInsertedPictureInDQ_p != NULL)
        {
            /* Between LastInsertedPictureInDQ_p and PictureToInsertInDQ_p a lot of picture may be skipped : so to avoid */
            /* to display too few pictures and that last one of a bit buffer is too long displayed because */
            /* the other buffer is under transfer and parsing (==> so not ready), inserted pictures must */
            /* have their display counters incremented to give an fluidity display impression */
            /* if pictures to be inserted do not follow : means that speed < -100 */
            RepeatDisplayFieldsOfToBeInsertedPictureIfNecessary(TrickModeHandle, LastInsertedPictureInDQ_p, PictureToInsertInDQ_p);
        }

        /* insert in display queue */
        PictureToInsertInDQ_p->IsInDisplayQueue = TRUE;
        PictureToInsertInDQ_p->HasBeenInsertedInDisplayQueue = TRUE;

        /* TraceBuffer(("Ins E=%d \r\n", */
                                    /* PictureToInsertInDQ_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference, */

        /* lock the buffer into the RESERVED status */
        if (IsPictureSecondField(PictureToInsertInDQ_p))
        {
            /* 2nd field status is not used by trickmode smooth but is by display. Keep it locked to avoid display to believe a 3 buffers behaviour */
        }

        /* Insert In DQ */
        if (! VIDTRICK_Data_p->Stop.TrickmodIsStopped)
        {
#ifdef TRACE_UART_DISPLAY_SKIP
            TraceBuffer(("DisplayQ: %c %d  %d/%d  ind=%d\r\n" ,
                MPEGFrame2Char(PictureToInsertInDQ_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame),
                PictureToInsertInDQ_p->PictureInfoFromDecode.pTemporalReference,
                PictureToInsertInDQ_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                PictureToInsertInDQ_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                PictureToInsertInDQ_p->IndiceInBuffer ));

            /* TraceBuffer(("InsInDQ ind=%d TiC=%d %d %d %d vbv=%d\r\n", */
                /* PictureToInsertInDQ_p->IndiceInBuffer, */
                /* PictureToInsertInDQ_p->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode.Hours, */
                /* PictureToInsertInDQ_p->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode.Minutes, */
                /* PictureToInsertInDQ_p->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode.Seconds, */
                /* PictureToInsertInDQ_p->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode.Frames, */
                /* PictureToInsertInDQ_p->PictureInfoFromDecode.StreamInfoForDecode.Picture.vbv_delay */
                /* )); */
#endif
            VIDTRICK_Data_p->Flags.AtLeastOnePictureHasBeenInserted = TRUE;

#ifdef ST_ordqueue
    #ifdef TRACE_UART_DEBUG
            TraceBuffer((">>Insert picture %c%d in Queue\n\r",
                                       MPEGFrame2Char(PictureToInsertInDQ_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame),
                                       PictureToInsertInDQ_p->PictureInfoFromDecode.pTemporalReference));
    #endif

            ErrorCode = VIDQUEUE_InsertPictureInQueue(VIDTRICK_Data_p->OrderingQueueHandle,
                                                      PictureToInsertInDQ_p->PictureBuffer_p,
                                                      VIDQUEUE_INSERTION_ORDER_LAST_PLACE);
            if (ErrorCode == ST_NO_ERROR)
            {
                /* Push directly picture to display */
                ErrorCode = VIDQUEUE_PushPicturesToDisplay(VIDTRICK_Data_p->OrderingQueueHandle,
                                &(PictureToInsertInDQ_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID));
            }
            if (ErrorCode == ST_NO_ERROR)
            {
                 if(((VIDTRICK_Data_p->DecodedPictures == STVID_DECODED_PICTURES_I)||(VIDTRICK_Data_p->DecodedPictures == STVID_DECODED_PICTURES_IP))
                                    &&(VIDTRICK_Data_p->MainParams.Speed < 0))
                {
                    VIDTRICK_Data_p->BackwardFluidity.NbFieldsToBeDisplayed = VIDTRICK_Data_p->BackwardFluidity.NbFieldsToBeDisplayedPerPicture;
                }
            }
#endif /* ST_ordqueue */
        }

        /* Is there a Stop pending ? */
        if (VIDTRICK_Data_p->Stop.TrickmodStopIsPending)
        {
            if  (   ((VIDTRICK_Data_p->Stop.StopMode == STVID_STOP_WHEN_END_OF_DATA) && (PictureToInsertInDQ_p == VIDTRICK_Data_p->EndOfDataPicture_p))
                 || ((VIDTRICK_Data_p->Stop.StopMode == STVID_STOP_WHEN_NEXT_REFERENCE) && (PictureMpegTypeIsReference(PictureToInsertInDQ_p)))
                 || ((VIDTRICK_Data_p->Stop.StopMode == STVID_STOP_WHEN_NEXT_I) && (PictureMpegTypeIsI(PictureToInsertInDQ_p))))
            {
                /* this is the last picture */
                VIDDISP_Stop(VIDTRICK_Data_p->DisplayHandle, &(PictureToInsertInDQ_p->PictureInfoFromDecode.PictureInfos.VideoParams.PTS), VIDDISP_STOP_ON_PTS);
                VIDDISP_Freeze(VIDTRICK_Data_p->DisplayHandle, &VIDTRICK_Data_p->Stop.SmoothBackwardRequiredStopFreezeParams);

                /* Stop without freeing not displayed pictures */
                ErrorCode = StopSmoothBackward(TrickModeHandle, FALSE);
            }
        }
    } /* if (PictureToInsertInDQ_p != NULL) */
} /* FindPictureToComputeFromAndInsertInDisplayQueue */

/*******************************************************************************
Name        : RenumberExtendedTemporalReferences
Description : Affect extended temporal references of a liner bit buffer
Parameters  : TrickModeHandle, CurrentBuffer_p
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void RenumberExtendedTemporalReferences(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const CurrentBuffer_p)
{
    U32                          index;
    U32                          Offset;
    VIDTRICK_PictureInfo_t *     CurrentPicture_p;
    VIDTRICK_PictureInfo_t *     PreviousPicture_p;

    CurrentPicture_p = CurrentBuffer_p->PictureInfoTableStartAddress_p;
    PreviousPicture_p = CurrentPicture_p;
    CurrentPicture_p ++;
    Offset = 0;
	UNUSED_PARAMETER(TrickModeHandle);

    PreviousPicture_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference = PreviousPicture_p->PictureInfoFromDecode.pTemporalReference;
    for (index = 1; index < CurrentBuffer_p->NbPicturesInside; index++)
    {
        if (CurrentPicture_p->PictureInfoFromDecode.pTemporalReference < PreviousPicture_p->PictureInfoFromDecode.pTemporalReference)
        {
            /* can not happen if B-B */
            /* can happen if ref-B ==> do nothing */
            /* can happen if B-ref or ref-ref */
            if (PictureMpegTypeIsReference(CurrentPicture_p))
            {
                if (PictureMpegTypeIsB(PreviousPicture_p)) /* B-ref */
                {
                    Offset += PreviousPicture_p->PictureInfoFromDecode.pTemporalReference + 2;
                }
                else /* ref-ref */
                {
                    Offset += PreviousPicture_p->PictureInfoFromDecode.pTemporalReference + 1;
                }
            }
        }
        else if (CurrentPicture_p->PictureInfoFromDecode.pTemporalReference == PreviousPicture_p->PictureInfoFromDecode.pTemporalReference + 1)
        {
            /* case B-ref with Ref temp = B ref + 1 : case gop has only 1 reference */
            if ((PictureMpegTypeIsReference(CurrentPicture_p)) && (PictureMpegTypeIsB(PreviousPicture_p)))
            {
                Offset += PreviousPicture_p->PictureInfoFromDecode.pTemporalReference + 2;
            }
        }
        CurrentPicture_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference = CurrentPicture_p->PictureInfoFromDecode.pTemporalReference + Offset;
        CurrentPicture_p++;
        PreviousPicture_p++;
    }
}

/**********************************************************************************
Name        : RenumberTimeCodes
Description : first pictures of linear bit buffer can only have their time code values
              correctly computed only if a GOP has been parsed previously. These function
              interpolates all the time code of the pictures which are placed before
              the fist GOP in a linear bit buffer.
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void RenumberTimeCodes(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const CurrentBuffer_p)
{
    VIDTRICK_Data_t           * VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    VIDTRICK_PictureInfo_t    * Picture_p;
    U32                         MaxFrameNumber;
    STVID_TimeCode_t            TimeCode;

    Picture_p = VIDTRICK_Data_p->NegativeToPositiveSpeedParams.FirstNullTempRefPicture_p;

    /* If First picture of buffer is the one with null temp ref after the GOP : nothing to do */
    if (   (Picture_p == NULL)
        || (Picture_p->IndiceInBuffer == 0))
    {
        return;
    }

    TimeCode = Picture_p->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode;
    MaxFrameNumber = (Picture_p->PictureInfoFromDecode.PictureInfos.VideoParams.FrameRate + 60) / 1000 - 1;

    while (Picture_p != CurrentBuffer_p->PictureInfoTableStartAddress_p)
    {
        Picture_p--;

        /* if second field or frame picture : decrement Time code */
        if (!Picture_p->PictureInfoFromDecode.AdditionalStreamInfo.ExpectingSecondField)
        {
            if (TimeCode.Frames == 0)
            {
                TimeCode.Frames =  MaxFrameNumber;
                if (TimeCode.Seconds == 0)
                {
                    TimeCode.Seconds = 59;
                    if (TimeCode.Minutes == 0)
                    {
                        TimeCode.Minutes = 59;
                        if (TimeCode.Hours == 0)
                        {
                            TimeCode.Hours = 23;
                        } /* if (TimeCode.Hours == 0) */
                        else
                        {
                            TimeCode.Hours--;
                        }
                    } /* if (TimeCode.Minutes == 0) */
                    else
                    {
                        TimeCode.Minutes--;
                    }
                } /* if (TimeCode.Seconds == 0) */
                else
                {
                    TimeCode.Seconds--;
                }
            } /* if (TimeCode.Frames == 0) */
            else
            {
                TimeCode.Frames--;
            }
        } /* if (!Picture_p->PictureInfoFromDecode.AdditionalStreamInfo.ExpectingSecondField) */

        Picture_p->PictureInfoFromDecode.PictureInfos.VideoParams.TimeCode = TimeCode;

    } /* while (Picture_p != CurrentBuffer_p->PictureInfoTableStartAddress_p) */
}

/*******************************************************************************
Name        : GetExtendedTempRefAccordingToPreviousBuffer
Description : Returns extended temporal references of a linear bit buffer
Parameters  : TrickModeHandle, CurrentBuffer_p
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static S32 GetExtendedTempRefAccordingToPreviousBuffer(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const CurrentBuffer_p)
{
    /* In this function we do 2 actions :                                    */
    /* - Apply a continue Extended Temporal Reference on each picture of the */
    /*   buffer. i.e. we take into account the Extended Temporal Reference   */
    /*   of the 2 matching pictures each in a buffer.                        */

    /* Conversion table Temporal Reference -> Extended Temporal Reference    */
    /* --------------------------------------------------------------------- */
    /* Temporal Reference          |  2 -> 0 (GOP) |  2 -> 5   |  3 -> 2   | */
    /* Extended Temporal reference |  n  ->  n+1   |  n -> n+3 |  n -> n-1 | */
    /* --------------------------------------------------------------------- */
    /* ------------------------------------------------------------------------------------------ */
    /* Temporal Reference          | k (near 1024) ->  i (near 0)  | k (near 0) ->  i (near 1024) */
    /* Extended Temporal reference | n             ->  n+i+1024-k  | n          ->  n+i-1024-k    */
    /* ------------------------------------------------------------------------------------------ */

    /* We suppose that the Extended Temporal Reference has been calculated   */
    /* by the software decoder. We expected from it that the first Extended  */
    /* Temporal Reference of the buffer is the real Temporal Reference.      */
    /* By the way here we just have to align the value according to the      */
    /* Extended Temporal Reference of the matching picture.                  */

    VIDTRICK_Data_t *            VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    VIDTRICK_LinearBitBuffer_t * AssociatedBuffer_p = CurrentBuffer_p->AssociatedLinearBitBuffer_p;
    U32                          MatchingPictureNumberInParsedBuffer;       /* Picture Number of the matching picture in current buffer. */
    S32                          ConstForBuffer;                            /* Constant variable for these buffer. */

    if (VIDTRICK_Data_p->Flags.DoNotMatchBufAWithBufB)
    {
        ConstForBuffer = VIDTRICK_SMOOTH_FIRST_EXTENDED_TEMP_REF;
    }
    else if (   (!CurrentBuffer_p->DiscontinuityWithPreviousBuffer)
             && (VIDTRICK_Data_p->MatchingPicture.MatchingPicture_p != NULL))
    {
        /* For a easier reading, set local variables.                            */
        MatchingPictureNumberInParsedBuffer = VIDTRICK_Data_p->MatchingPicture.ParsedBufferMatchingPictureNumber;

        ConstForBuffer =   VIDTRICK_Data_p->MatchingPicture.MatchingPicture_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference
                         - CurrentBuffer_p->PictureInfoTableStartAddress_p[MatchingPictureNumberInParsedBuffer - 1].
                                             PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference;
    }
    else
    {
        /* difficult to estimate extended temporal references of a discontinuited buffer */
        ConstForBuffer =
                  AssociatedBuffer_p->PictureInfoTableStartAddress_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference
                - CurrentBuffer_p->PictureInfoTableStartAddress_p[CurrentBuffer_p->NbPicturesInside-1].PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference
                - VIDTRICK_EXT_TEMP_REF_OFFSET_DISCONTINUITY_MIN
                - VIDTRICK_Data_p->Discontinuity.NbFieldsToRecover / 2;
        VIDTRICK_Data_p->Discontinuity.NbFieldsToRecover = 0;
    }

    return ConstForBuffer;
} /* end of the GetExtendedTempRefAccordingToPreviousBuffer function. */

/*******************************************************************************
Name        : AffectReferencesPictures
Description : TrickModeHandle, CurrentBuffer_p
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void AffectReferencesPictures(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const CurrentBuffer_p)
{
    /* - Apply a Backward and a Forward Reference picture on the pictures of */
    /*   the end of the current buffer and on the pictures of the beginning  */
    /*   of the associated buffer.                                           */

    VIDTRICK_Data_t *            VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    VIDTRICK_LinearBitBuffer_t * AssociatedBuffer_p = CurrentBuffer_p->AssociatedLinearBitBuffer_p;
    VIDTRICK_PictureInfo_t *     CurrentPicture_p;
    VIDTRICK_PictureInfo_t *     LatestBackwardReference_p = NULL;
    VIDTRICK_PictureInfo_t *     LatestForwardReference_p = NULL;
    VIDTRICK_PictureInfo_t *     LastBFirstPicture_p = NULL;
    BOOL                         FirstReferenceFound = FALSE;
    BOOL                         FirstIPictureFound = FALSE;
    BOOL                         FirstRefAfterFirstIFound = FALSE;
    S32                          OffsetConstant;
    U32                          index= 0;

    /*-------------------------------------------------------------------------------*/
    /*- first part : affect references pictures of the just parsed linear bit buffer */
    /*-------------------------------------------------------------------------------*/

    /* Get Extended Temporal Reference Constant to add to each picture */
    OffsetConstant = GetExtendedTempRefAccordingToPreviousBuffer(TrickModeHandle, CurrentBuffer_p);

    CurrentBuffer_p->NbIFieldInside = 0;
    CurrentBuffer_p->NbIPFieldInside = 0;

    /* Find the number minimum between the first I and the second P. Only    */
    /* from there we will be able to apply the references between pictures.  */
    CurrentPicture_p = CurrentBuffer_p->PictureInfoTableStartAddress_p;

    /* first expecting second field flag may be false : correct it */
    if (   (IsPictureFrame(CurrentPicture_p))
        || (CurrentPicture_p->PictureInfoFromDecode.pTemporalReference != CurrentBuffer_p->PictureInfoTableStartAddress_p[1].PictureInfoFromDecode.pTemporalReference)
        || (CurrentPicture_p->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure == CurrentBuffer_p->PictureInfoTableStartAddress_p[1].PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure))
    {
        CurrentPicture_p->PictureInfoFromDecode.AdditionalStreamInfo.ExpectingSecondField = FALSE;
    }
    else
    {
        CurrentPicture_p->PictureInfoFromDecode.AdditionalStreamInfo.ExpectingSecondField = TRUE;
    }

    for (index=0 ; index < CurrentBuffer_p->NbPicturesInside ; index++ , CurrentPicture_p++)
    {
      /* Search for 2 reference picture P-P, I-P or P-I .                               */
      /* The first one is the backward reference, the second is the forward reference.  */
      switch (CurrentPicture_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame)
      {
        case STVID_MPEG_FRAME_I :
        case STVID_MPEG_FRAME_P :
                /* compute number of I & P frames on the BB */
                if (CurrentPicture_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame==STVID_MPEG_FRAME_I)
                {
                    CurrentBuffer_p->NbIFieldInside +=
                    (   CurrentPicture_p->PictureInfoFromDecode.AdditionalStreamInfo.TopFieldCounter
                      + CurrentPicture_p->PictureInfoFromDecode.AdditionalStreamInfo.BottomFieldCounter
                      + CurrentPicture_p->PictureInfoFromDecode.AdditionalStreamInfo.RepeatFieldCounter)
                    * ( CurrentPicture_p->PictureInfoFromDecode.AdditionalStreamInfo.RepeatProgressiveCounter + 1);
                }
                CurrentBuffer_p->NbIPFieldInside +=
                    (   CurrentPicture_p->PictureInfoFromDecode.AdditionalStreamInfo.TopFieldCounter
                      + CurrentPicture_p->PictureInfoFromDecode.AdditionalStreamInfo.BottomFieldCounter
                      + CurrentPicture_p->PictureInfoFromDecode.AdditionalStreamInfo.RepeatFieldCounter)
                    * ( CurrentPicture_p->PictureInfoFromDecode.AdditionalStreamInfo.RepeatProgressiveCounter + 1);

                /* Compute decodable flag  */
                if (FirstIPictureFound)
                {
                    /* Reference picture (I or P)                       */
                    /* and First I picture has been previously found    */
                    /* then picture is decodable                        */

                    if (!IsPictureSecondField(CurrentPicture_p))
                    {
                        FirstRefAfterFirstIFound = TRUE;
                    }
                    CurrentPicture_p->IsDecodable = TRUE;
                }
                else if (   (PictureMpegTypeIsI(CurrentPicture_p))
                         && (!IsPictureSecondField(CurrentPicture_p)))
                {
                    /* First I picture has NOT already been found               */
                    /* and current picture is I                                 */
                    /* and current pic is a first field or a frame picture      */
                    /* then picture is decodable                                */

                    FirstIPictureFound = TRUE;
                    CurrentPicture_p->IsDecodable = TRUE;
                }

                /* compute references inter - dependencies */
                CurrentPicture_p->ForwardOrNextReferencePicture_p = NULL ;

                /*  */
                if (!FirstReferenceFound)
                {
                    CurrentPicture_p->BackwardOrLastBeforeIReferencePicture_p = NULL;
                    if (!IsPictureSecondField(CurrentPicture_p))
                    {
                        FirstReferenceFound = TRUE;
                    }
                }
                else
                {
                    CurrentPicture_p->BackwardOrLastBeforeIReferencePicture_p = LatestForwardReference_p;
                    LatestForwardReference_p->ForwardOrNextReferencePicture_p = CurrentPicture_p;
                }

                if (LatestForwardReference_p != NULL)
                {
                    if (HasPictureASecondField(LatestForwardReference_p))
                    {
                        /* For Reference picture, Second field is its Forward reference. */
                        LatestForwardReference_p->SecondFieldPicture_p = CurrentPicture_p;
                    }
                }

                LatestBackwardReference_p = LatestForwardReference_p;
                LatestForwardReference_p = CurrentPicture_p;
            break;

            case STVID_MPEG_FRAME_B :
                if (FirstRefAfterFirstIFound)
                {
                    CurrentPicture_p->IsDecodable = TRUE;
                }

                if  ((LatestForwardReference_p != NULL) && (IsPictureSecondField(LatestForwardReference_p)))
                {
                    CurrentPicture_p->ForwardOrNextReferencePicture_p = LatestForwardReference_p->BackwardOrLastBeforeIReferencePicture_p;
                    if (LatestBackwardReference_p != NULL)
                    {
                        CurrentPicture_p->BackwardOrLastBeforeIReferencePicture_p = LatestBackwardReference_p->BackwardOrLastBeforeIReferencePicture_p;
                    }
                    else
                    {
                        CurrentPicture_p->BackwardOrLastBeforeIReferencePicture_p = NULL;
                    }
                }
                else
                {
                    CurrentPicture_p->BackwardOrLastBeforeIReferencePicture_p = LatestBackwardReference_p;
                    CurrentPicture_p->ForwardOrNextReferencePicture_p = LatestForwardReference_p;
                }

                if (HasPictureASecondField(CurrentPicture_p))
                {
                    LastBFirstPicture_p = CurrentPicture_p;
                }
                else if (IsPictureSecondField(CurrentPicture_p))
                {
                    if (LastBFirstPicture_p != NULL)
                    {
                        LastBFirstPicture_p->SecondFieldPicture_p = CurrentPicture_p;
                    }
                    /* Found 2nd B field picture, first field picture pointer becomes useless */
                    /* Reinit it because first field cannot have another 2nd field */
                    LastBFirstPicture_p = NULL;
                }
                break;

        default :
            break;
      } /* end of switch */

      /* Affect the Previous Picture in the stream (link list). */
      if (index != 0)
      {
          CurrentPicture_p->PreviousInStreamPicture_p = CurrentPicture_p - 1;
      }

      /* Affect the Extended Temporal Reference. */
      CurrentPicture_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference += OffsetConstant;

    } /* end of for */

    /* Lock mutex protecting Trick mode private params */
    STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p);

    /* second part must not be done if buffer have not to be matched */
    if (   (! VIDTRICK_Data_p->Flags.DoNotMatchBufAWithBufB)
        && (! CurrentBuffer_p->DiscontinuityWithPreviousBuffer))
    {
        /* Skipp useless pictures in associated buffer table */
        if (VIDTRICK_Data_p->MatchingPicture.LastPictureIndex >= VIDTRICK_Data_p->MatchingPicture.ParsedBufferMatchingPictureNumber)
        {
            OffsetConstant =  VIDTRICK_Data_p->MatchingPicture.LastPictureIndex
                            - VIDTRICK_Data_p->MatchingPicture.ParsedBufferMatchingPictureNumber;
        }
        else
        {
            OffsetConstant = 0;
#ifdef TRICKMOD_DEBUG
            TrickModeError();
#endif
        }
        AssociatedBuffer_p->PictureInfoTableStartAddress_p += OffsetConstant;
        AssociatedBuffer_p->NbPicturesInside -= OffsetConstant;

        /*---------------------------------------------------------------------------------------*/
        /*- second part : affect references of the beginning of the associated linear bit buffer */
        /*---------------------------------------------------------------------------------------*/

        /* Loop for each picture for witch we can.  */
        CurrentPicture_p = AssociatedBuffer_p->PictureInfoTableStartAddress_p;
        FirstRefAfterFirstIFound = FALSE;
        FirstIPictureFound = FALSE;

        for (index=0 ; index < AssociatedBuffer_p->NbPicturesInside ; index++ , CurrentPicture_p++)
        {
            CurrentPicture_p->IndiceInBuffer -= OffsetConstant;
            if (FirstRefAfterFirstIFound)
            {
                /* Once First Reference after I ref has been found      */
                /* only affect IndiceInBuffer variable with new value   */
                /* since useless pictures have been skipped             */
                continue;
            }

            switch (CurrentPicture_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame)
            {
                case STVID_MPEG_FRAME_I :
                case STVID_MPEG_FRAME_P :
                    if (FirstIPictureFound)
                    {
                        /* Reference picture (I or P)                       */
                        /* and First I picture has been previously found    */
                        if (!IsPictureSecondField(CurrentPicture_p))
                        {
                            FirstRefAfterFirstIFound = TRUE;
                        }
                    }
                    else if (   (PictureMpegTypeIsI(CurrentPicture_p))
                             && (!IsPictureSecondField(CurrentPicture_p)))
                    {
                        /* First I picture has NOT already been found               */
                        /* and current picture is I                                 */
                        /* and current pic is a first field or a frame picture      */
                        FirstIPictureFound = TRUE;
                    }

                    /* Manage Interdependencies between pictures */
                    CurrentPicture_p->BackwardOrLastBeforeIReferencePicture_p = LatestForwardReference_p;
                    LatestForwardReference_p->ForwardOrNextReferencePicture_p = CurrentPicture_p;
                    /* CurrentPicture_p->ForwardOrNextReferencePicture_p = NULL ; */
                    if (HasPictureASecondField(LatestForwardReference_p))
                    {
                        /* For Reference picture, Second field is its Forward reference. */
                        LatestForwardReference_p->SecondFieldPicture_p = CurrentPicture_p;
                    }

                    /* Manage IsDecodable flag */
                    if (   (PictureMpegTypeIsP(CurrentPicture_p))
                        || (IsPictureSecondField(CurrentPicture_p)))
                    {
                        if (CurrentPicture_p->BackwardOrLastBeforeIReferencePicture_p != NULL)
                        {
                            CurrentPicture_p->IsDecodable = CurrentPicture_p->BackwardOrLastBeforeIReferencePicture_p->IsDecodable;
                        }
#ifdef TRICKMOD_DEBUG
                        else
                        {
                            TrickModeError();
                        }
#endif
                    }

                    LatestBackwardReference_p = LatestForwardReference_p;
                    LatestForwardReference_p = CurrentPicture_p;
                    break;
                case STVID_MPEG_FRAME_B :
                    /* Backward & Forward */
                    /* Manage Interdependencies between pictures */
                    if  (   (LatestForwardReference_p != NULL)
                         && (IsPictureSecondField(LatestForwardReference_p)))
                    {
                        CurrentPicture_p->ForwardOrNextReferencePicture_p = LatestForwardReference_p->BackwardOrLastBeforeIReferencePicture_p;
                        if (LatestBackwardReference_p != NULL)
                        {
                            CurrentPicture_p->BackwardOrLastBeforeIReferencePicture_p = LatestBackwardReference_p->BackwardOrLastBeforeIReferencePicture_p;
                        }
                        else
                        {
                            CurrentPicture_p->BackwardOrLastBeforeIReferencePicture_p = NULL;
                        }
                    }
                    else
                    {
                        CurrentPicture_p->BackwardOrLastBeforeIReferencePicture_p = LatestBackwardReference_p;
                        CurrentPicture_p->ForwardOrNextReferencePicture_p = LatestForwardReference_p;
                    }

                    /* Manage IsDecodable flag */
                    if (   (CurrentPicture_p->BackwardOrLastBeforeIReferencePicture_p != NULL)
                        && (CurrentPicture_p->ForwardOrNextReferencePicture_p != NULL))
                    {
                        if (   CurrentPicture_p->BackwardOrLastBeforeIReferencePicture_p->IsDecodable
                            && CurrentPicture_p->ForwardOrNextReferencePicture_p->IsDecodable)
                        {
                            CurrentPicture_p->IsDecodable = TRUE;
                        }
                        else
                        {
                            CurrentPicture_p->IsDecodable = FALSE;
                        }
                    }
#ifdef TRICKMOD_DEBUG
                    else
                    {
                        TrickModeError();
                    }
#endif

                    if (HasPictureASecondField(CurrentPicture_p))
                    {
                        LastBFirstPicture_p = CurrentPicture_p;
                    }
                    else if (IsPictureSecondField(CurrentPicture_p))
                    {
                        if (LastBFirstPicture_p != NULL)
                        {
                            LastBFirstPicture_p->SecondFieldPicture_p = CurrentPicture_p;
                        }
                        /* Found 2nd B field picture, first field picture pointer becomes useless */
                        /* Reinit it because first field cannot have another 2nd field */
                        LastBFirstPicture_p = NULL;
                    }
#ifdef TRICKMOD_DEBUG
                    /* if matched picture, all B pictures encountered must linked with their backwrad and thier forward */
                    if (   (CurrentPicture_p->BackwardOrLastBeforeIReferencePicture_p == NULL)
                        || (CurrentPicture_p->ForwardOrNextReferencePicture_p == NULL))
                    {
                        TrickModeError();
                    }
#endif
                    break;

                default :
                    break;
            } /* end of switch */

        } /* end of for : end for the reference for the associated buffer. */

        /* Affect Previous pointer between each Buffer */
        CurrentPicture_p = AssociatedBuffer_p->PictureInfoTableStartAddress_p;
        CurrentPicture_p->PreviousInStreamPicture_p = &(CurrentBuffer_p->PictureInfoTableStartAddress_p[CurrentBuffer_p->NbPicturesInside - 1]);

    } /* if (!CurrentBuffer_p->DiscontinuityWithPreviousBuffer) */

    /* Un Lock mutex protecting Trick mode private params */
    STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);

} /* end of the AffectReferencesPictures function. */


/*******************************************************************************
Name        : FindFirstIPictureOfLinearBitbuffer
Description : TrickModeHandle, AssociatedBuffer
Parameters  : Finds the last displayable picture of the parsed buffer. It is the first I picture of the buffer !
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void FindFirstIPictureOfLinearBitBuffer(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const LinearBitBuffer_p, VIDTRICK_PictureInfo_t ** FirstIpicture_p, U32* const Index_p )
{
    VIDTRICK_PictureInfo_t *     Picture_p = LinearBitBuffer_p->PictureInfoTableStartAddress_p;

    *Index_p = 0;
    *FirstIpicture_p = NULL;
	UNUSED_PARAMETER(TrickModeHandle);

    while ((*FirstIpicture_p == NULL) && (*Index_p < LinearBitBuffer_p->NbPicturesInside))
    {
        if (   (PictureMpegTypeIsI(Picture_p))
            && (!(IsPictureSecondField(Picture_p))))
        {
            *FirstIpicture_p = Picture_p;
        }
        Picture_p++;
        (*Index_p)++;
    }
} /* end of the FindFirstIPictureOfLinearBitBuffer() function. */


/*******************************************************************************
Name        : FindFirstReferencePictureOfLinearBitBuffer
Description : TrickModeHandle, AssociatedBuffer
Parameters  : Finds the first reference picture of the buffer
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void FindFirstDisplayablePictureOfLinearBitBuffer(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const LinearBitBuffer_p, VIDTRICK_PictureInfo_t ** FirstDisplayablePicture_p)
{
    VIDTRICK_PictureInfo_t   * JustParsedBufferLastPicture_p = NULL;
    VIDTRICK_PictureInfo_t   * LastDisplayablePicture_p = NULL;
	UNUSED_PARAMETER(TrickModeHandle);

    /* Find Last Displayable picture of Associated buffer now */
    LastDisplayablePicture_p = LinearBitBuffer_p->LastDisplayablePicture_p;
#ifdef TRICKMOD_DEBUG
    if (LastDisplayablePicture_p == NULL)
    {
        TrickModeError();
    }
#endif

    /* Last Picture Of The Buffer */
    JustParsedBufferLastPicture_p = &(LinearBitBuffer_p->PictureInfoTableStartAddress_p[LinearBitBuffer_p->NbPicturesInside - 1]);
    /* JustParsedBufferLastPicture_p can not be first field if second field is not in the linear bit buffer */
    if (HasPictureASecondField(JustParsedBufferLastPicture_p))
    {
        JustParsedBufferLastPicture_p = JustParsedBufferLastPicture_p->PreviousInStreamPicture_p;
    }

#ifdef TRICKMOD_DEBUG
    if (JustParsedBufferLastPicture_p == NULL)
    {
        TrickModeError();
    }
#endif

    /* if B picture */
    if (PictureMpegTypeIsB(JustParsedBufferLastPicture_p))
    {
        /* if end of buffer is for example  -P4-B2-B3-X7-B5 (X : I or P)  */
        if (JustParsedBufferLastPicture_p->ForwardOrNextReferencePicture_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference
                > (JustParsedBufferLastPicture_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference + 1))
        {
            /* if end of buffer is for example  -P4-B2-B3-I7-B5 (if I7 is the single I of the buffer) I7 is the first displayable */
            if (JustParsedBufferLastPicture_p->ForwardOrNextReferencePicture_p == LastDisplayablePicture_p)
            {
                *FirstDisplayablePicture_p = LastDisplayablePicture_p;
            }
            else /* if end of buffer is for example  -P4-B2-B3-P7-B5 : B5 should be the first displayable, but is in fact P4 ... */
            {
                /* !!!!! caution B5 will require to have its P7 forward decoded without giving it to display */
                /* taking P4 is better ! */
                *FirstDisplayablePicture_p = JustParsedBufferLastPicture_p->BackwardOrLastBeforeIReferencePicture_p;
            }
        }
        else /* if end of buffer is for example  -P4-B2-B3-X7-B5-B6 (X : I or P) X7 is the first displayable picture */
        {
            *FirstDisplayablePicture_p = JustParsedBufferLastPicture_p->ForwardOrNextReferencePicture_p;
        }
    }
    else /* reference picture */
    {
        if (IsPictureSecondField(JustParsedBufferLastPicture_p))
        {
            /* Get first field in order to process correctly interdependence between references ... */
            JustParsedBufferLastPicture_p = JustParsedBufferLastPicture_p->BackwardOrLastBeforeIReferencePicture_p;
        }

        /* if end of buffer is for example  -P6-B4-B5-I9 (if I9 is the single I of the buffer) I9 is the first displayable */
        if (JustParsedBufferLastPicture_p == LastDisplayablePicture_p)
        {
            *FirstDisplayablePicture_p = LastDisplayablePicture_p;
        }
        else
        {
            /* if end of buffer is for example  -P4-P5-P6-P7 : P7 is the first displayable picture */
            /* if end of buffer is for example  -P6-B4-B5-P7 : P7 is the first displayable picture  */
            if (JustParsedBufferLastPicture_p->BackwardOrLastBeforeIReferencePicture_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference
                    == JustParsedBufferLastPicture_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference - 1)
            {
                *FirstDisplayablePicture_p = JustParsedBufferLastPicture_p;
            }
            else /* if end of buffer is for example -P6-B4-B5-P9 : P6 is the first displayable */
            {
                *FirstDisplayablePicture_p = JustParsedBufferLastPicture_p->BackwardOrLastBeforeIReferencePicture_p;
            }
        }
    }

#ifdef TRICKMOD_DEBUG
    if (PictureMpegTypeIsB(*FirstDisplayablePicture_p))
    {
        /* *FirstDisplayablePicture_p should be a reference ! */
        TrickModeError();
    }
#endif

    /* If first field then return its second field */
    if (HasPictureASecondField((*FirstDisplayablePicture_p)))
    {
#ifdef TRICKMOD_DEBUG
        /* The second field should be found */
        if ((*FirstDisplayablePicture_p)->SecondFieldPicture_p == NULL)
        {
            TrickModeError();
        }
#endif
        *FirstDisplayablePicture_p = (*FirstDisplayablePicture_p)->SecondFieldPicture_p;
    }
}


/*******************************************************************************
Name        : FindFirstReferencePictureOfLinearBitBuffer
Description : TrickModeHandle, AssociatedBuffer
Parameters  : Finds the first reference picture of the buffer
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void FindFirstReferencePictureOfLinearBitBuffer(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const LinearBitBuffer_p, VIDTRICK_PictureInfo_t ** FirstReferencePicture_p)
{
    VIDTRICK_PictureInfo_t  * Picture_p;
    U32                       index = 0;

    *FirstReferencePicture_p = NULL;
	UNUSED_PARAMETER(TrickModeHandle);

    Picture_p = LinearBitBuffer_p->PictureInfoTableStartAddress_p;
    while ((*FirstReferencePicture_p == NULL) && (index < LinearBitBuffer_p->NbPicturesInside))
    {
        if (PictureMpegTypeIsReference(Picture_p))
        {
            *FirstReferencePicture_p = Picture_p;
        }
        Picture_p++;
        index++;
    }
} /* end of the FindFirstIPictureOfLinearBitBuffer function. */

/*******************************************************************************
Name        : AffectTheBufferSpacing
Description : If the number of fields to recover is null the behaviour is with
              matching picture else we are in the discontinuity state. Then,
              the space between the buffers is calculated according to this
              number of fields to recover.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void AffectTheBufferSpacing(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const BufferToBeFilled_p)
{
    VIDTRICK_Data_t *          VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    VIDTRICK_PictureInfo_t *   LastDisplayablePicture_p = NULL;
    U32                        OverlapPicturesNbDuration;
    U32                        BitBufferRatioDuration;

    /* Find Last Displayable picture of Associated buffer now */
    LastDisplayablePicture_p = BufferToBeFilled_p->AssociatedLinearBitBuffer_p->LastDisplayablePicture_p;
#ifdef TRICKMOD_DEBUG
    if (LastDisplayablePicture_p == NULL)
    {
        TrickModeError();
    }
#endif

    /* Conditioned by the number of fields to recover. */
    if (VIDTRICK_Data_p->Discontinuity.NbFieldsToRecover > 0)
    {
        U32 FieldsPerSecond = (((VIDTRICK_Data_p->MainParams.FrameRateValue + 999) / 1000) * 2);

        BufferToBeFilled_p->DiscontinuityWithPreviousBuffer = TRUE;

        /* We reduce by the number of fields of the lost not decodable pictures which are at the top of the */
        /* current buffer. 2 fields per pictures is assumed.                                                */
        VIDTRICK_Data_p->Discontinuity.NbFieldsToRecover += LastDisplayablePicture_p->IndiceInBuffer * 2;

        /* BitRateValue is in 400 bits/s. 1 field has a duration of 20ms. The space between the buffers is in bytes. */
        /* Space = BitRate * NbFields * DurationOfOneField = BitRateValue * 400 * NbFields * 0.02 / 8                */
        /* Space = BitRateValue * 400 * NbFields * 0.02 / 8 = BitRateValue * NbFields                                */
#ifdef TRACE_UART_DEBUG
        TraceBuffer(("Recover %lu fields Frames Per Sec %lu\r\n",
                VIDTRICK_Data_p->Discontinuity.NbFieldsToRecover,
                FieldsPerSecond));
#endif
        if (FieldsPerSecond == 0)
        {
            FieldsPerSecond = 60;
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("fields per second should not be 0 \n\r"));
#endif

        }
        VIDTRICK_Data_p->Discontinuity.OverlapDuration = - ((VIDTRICK_Data_p->Discontinuity.NbFieldsToRecover * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) / FieldsPerSecond) ;
    }
    else
    {
        /* compute a justified overlap : at least VIDTRICK_BACKWARD_OVERLAP_PICTURES if little bit rate, 10% of bit buffers for high bit rate streams. */
        BufferToBeFilled_p->DiscontinuityWithPreviousBuffer = FALSE;

        /* compute VIDTRICK_BACKWARD_OVERLAP_PICTURES overlap duration */
        OverlapPicturesNbDuration = (VIDTRICK_BACKWARD_OVERLAP_PICTURES * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) / ((VIDTRICK_Data_p->MainParams.FrameRateValue + 999) / 1000);

        /* compute VIDTRICK_BACKWARD_OVERLAP_BIT_BUFFER_RATIO % of bit buffer overlap duration */
        BitBufferRatioDuration = ((STVID_DURATION_COMPUTE_FACTOR / 100) * VIDTRICK_BACKWARD_OVERLAP_BIT_BUFFER_RATIO * ((U32)BufferToBeFilled_p->AssociatedLinearBitBuffer_p->WholeBufferEndAddress_p - (U32)BufferToBeFilled_p->AssociatedLinearBitBuffer_p->WholeBufferStartAddress_p)) / BufferToBeFilled_p->AssociatedLinearBitBuffer_p->BeginningOfInjectionBitrateValue;

        /* TraceBuffer (("OverlapPicturesNbDuration=%d  BitBufferRatioDuration=%d\r\n", OverlapPicturesNbDuration, BitBufferRatioDuration)); */

        /* min value of both ones has to be considered */
        VIDTRICK_Data_p->Discontinuity.OverlapDuration = Min(OverlapPicturesNbDuration, BitBufferRatioDuration);
    }
} /* end of the AffectTheBufferSpacing function. */


/*******************************************************************************
Name        : RepeatDisplayFieldsOfToBeInsertedPictureIfNecessary
Description : Between two following pictures to be inserted in display queue, a lot of picture may be skipped : so to avoid
            to display too few pictures and that last one of a bit buffer is too long displayed because
            the other buffer is under transfer and parsing (==> so not ready), inserted pictures must
            have their display counters incremented to give an fluidity display impression
            if pictures to be inserted do not follow : means that speed < -100
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void RepeatDisplayFieldsOfToBeInsertedPictureIfNecessary(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t * LastInsertedPictureInDQ_p, VIDTRICK_PictureInfo_t * const PictureToInsertInDQ_p)
{
    VIDTRICK_Data_t *              VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    VIDTRICK_LinearBitBuffer_t *   PictureToInsertBitBuffer_p;
    U32   NbOfFieldsToRepeatForThisPicture1 = 0;
    S32   NbOfFieldsToRepeatForThisPicture2 = 0;
    U32   NbOfFieldsToRepeatForThisPicture;
    U32   NbOfFieldsDisplayedOfTheBitBufferWithSuchJumps = 0;
    U32   TemporalRefJumpBetweenBothPic;
    S32   TemporalRefDifBetweenFirstAndLastPicOfLBBF;
    U32   NbFieldsRepeated;

    if ((LastInsertedPictureInDQ_p == NULL) || (PictureToInsertInDQ_p == NULL))
    {
#ifdef TRICKMOD_DEBUG
         TrickModeError();
#endif
         return;
    }

    /* look at the dif extended temp ref dif between both pictures */
    TemporalRefJumpBetweenBothPic = LastInsertedPictureInDQ_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference -
                            PictureToInsertInDQ_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference;
    /* if jump detected : a too big jump may need to display each picture a few time more to ensure a fluent display */
    if (TemporalRefJumpBetweenBothPic > 1)
    {
        if (PictureToInsertInDQ_p->LinearBitBufferStoredInfo_p == LastInsertedPictureInDQ_p->LinearBitBufferStoredInfo_p)
        {
#ifdef TRACE_UART_DISPLAY_SKIP
            TraceBuffer (("\r\nLast=%d ToBe=%d\n\r",
                     LastInsertedPictureInDQ_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference,
                     PictureToInsertInDQ_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference));
#endif

            /* find bit buffer of picture to insert */
            PictureToInsertBitBuffer_p = PictureToInsertInDQ_p->LinearBitBufferStoredInfo_p;

            /*------------------------------------------------------------------------------------------------------------------------*/
            /* 1- compute the nb of fieds to repeat to avoid long flush and parse bit buffer too big durations consequence on display */
            /*------------------------------------------------------------------------------------------------------------------------*/
            TemporalRefDifBetweenFirstAndLastPicOfLBBF =
                    PictureToInsertBitBuffer_p->PictureInfoTableStartAddress_p[PictureToInsertBitBuffer_p->NbPicturesInside - 1].PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference -
                    PictureToInsertBitBuffer_p->LastDisplayablePicture_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference;

            if (TemporalRefDifBetweenFirstAndLastPicOfLBBF > 0)
            {
                /* compare total of displayable fields of the bit buffer and calculated fields duration mini to be fluent */
                /* nb of fields to be displayed to be fluent <--> nb of pictures of the bit buffer with such jumps */

                /* the difference between extended temporal references is representative of the fields skipped */
                /* (available for frame and field streams but not for mixed ones !) */
                switch (VIDTRICK_Data_p->DecodedPictures)
                {
                        case STVID_DECODED_PICTURES_I :
                            NbOfFieldsDisplayedOfTheBitBufferWithSuchJumps = VIDTRICK_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p->NbIFieldInside;

                            break;

                        case STVID_DECODED_PICTURES_IP :
                            NbOfFieldsDisplayedOfTheBitBufferWithSuchJumps = VIDTRICK_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p->NbIPFieldInside;

                            break;

                        default :
                            NbOfFieldsDisplayedOfTheBitBufferWithSuchJumps = TemporalRefDifBetweenFirstAndLastPicOfLBBF / TemporalRefJumpBetweenBothPic;
                            break;
                }

                if (NbOfFieldsDisplayedOfTheBitBufferWithSuchJumps < VIDTRICK_Data_p->BackwardFluidity.NbFieldsToBeDisplayedToBeFluent)
                {
                    /* There are NbFieldsToBeDisplayedToBeFluent - NbOfFieldsDisplayedOfTheBitBufferWithSuchJumps */
                    /* to repeat to all the pictures supposed to be displayed of the bit buffer.  */
                    if (NbOfFieldsDisplayedOfTheBitBufferWithSuchJumps > 0)
                    {
                        /* So (NbFieldsToBeDisplayedToBeFluent - NbOfFieldsDisplayedOfTheBitBufferWithSuchJumps) / NbOfFieldsDisplayedOfTheBitBufferWithSuchJumps * 2 */
                        /* fields have to be added to each picture.  */
                        NbOfFieldsToRepeatForThisPicture1 = (VIDTRICK_Data_p->BackwardFluidity.NbFieldsToBeDisplayedToBeFluent - NbOfFieldsDisplayedOfTheBitBufferWithSuchJumps)
                                     / NbOfFieldsDisplayedOfTheBitBufferWithSuchJumps;

                        if(((VIDTRICK_Data_p->DecodedPictures == STVID_DECODED_PICTURES_I)||(VIDTRICK_Data_p->DecodedPictures == STVID_DECODED_PICTURES_IP))
                                    &&(VIDTRICK_Data_p->MainParams.Speed < 0))
                        {
                                VIDTRICK_Data_p->BackwardFluidity.NbFieldsToBeDisplayedPerPicture = NbOfFieldsToRepeatForThisPicture1+1;
                                if (VIDTRICK_Data_p->BackwardFluidity.NbFieldsToBeDisplayedPerPicture > 1)
                                {
                                        VIDTRICK_Data_p->BackwardFluidity.BackwardFluentDisplayProcessIsRunning = TRUE;
                                }
                        }

                        /* 1 is added to affect the first entire value over the calculated one */
                        NbOfFieldsToRepeatForThisPicture1 ++;
                    }
                }
            } /* if (TemporalRefDifBetweenFirstAndLastPicOfLBBF > 0) */

            /*--------------------------------------------------------------------------------------------------*/
            /* 2- compute the nb of fieds to repeat to avoid too long decode pictures duration (high bit rate!) */
            /*--------------------------------------------------------------------------------------------------*/
#ifdef OLD_BACKWARD_FLUENT_DISPLAY_PROCESS
            if (   (VIDTRICK_Data_p->MainParams.Speed != 0)
                && (VIDTRICK_Data_p->BackwardFluidity.BackwardFluentDisplayProcessIsRunning))
            {
                /* BackwardFluentDisplayProcessIsRunning must be TRUE before testing NbOfFieldsToRepeatForThisPicture2 */
                /* because of extended temp ref holes can occur between 2 consecutives pictures. */
                if (VIDTRICK_Data_p->MainParams.Speed == 0)
                {
                    NbOfFieldsToRepeatForThisPicture2 = 0
#ifdef TRICKMOD_DEBUG
                    TrickModeError();
                    TraceBuffer(("Speed should not be 0 \n\r"));
#endif

                }
                else
                {
                    NbOfFieldsToRepeatForThisPicture2 = (2 * TemporalRefJumpBetweenBothPic * 100 / (-VIDTRICK_Data_p->MainParams.Speed)) - 2;
                }
                if (NbOfFieldsToRepeatForThisPicture2 < 0)
                {
                    NbOfFieldsToRepeatForThisPicture2 = 0;
                }
            }
#endif /* OLD_BACKWARD_FLUENT_DISPLAY_PROCESS */

#ifdef TRACE_UART_DISPLAY_SKIP
            TraceBuffer (("DfBoth=%d DfFirstLast=%d Displayed=%d Fluent=%d NbToRt1=%d NbToRt2=%d\n\r",
                     TemporalRefJumpBetweenBothPic,
                     TemporalRefDifBetweenFirstAndLastPicOfLBBF,
                     NbOfFieldsDisplayedOfTheBitBufferWithSuchJumps,
                     VIDTRICK_Data_p->BackwardFluidity.NbFieldsToBeDisplayedToBeFluent,
                     NbOfFieldsToRepeatForThisPicture1,
                     NbOfFieldsToRepeatForThisPicture2));
#endif

            /* the max value of both has to be computed */
            NbOfFieldsToRepeatForThisPicture = Max(NbOfFieldsToRepeatForThisPicture1, ((U32)NbOfFieldsToRepeatForThisPicture2));

            if ((NbOfFieldsToRepeatForThisPicture > 0) &&(VIDTRICK_Data_p->DecodedPictures != STVID_DECODED_PICTURES_I)&&
                    (VIDTRICK_Data_p->DecodedPictures != STVID_DECODED_PICTURES_IP))
            {
                /* if PictureToInsertInDQ_p has a second field, this second field has been skipped. NbOfFieldsToRepeatForThisPicture */
                /* has to be updated consequently */
                if (HasPictureASecondField(PictureToInsertInDQ_p))
                {
                    NbOfFieldsToRepeatForThisPicture++;
                }

                VIDDISP_RepeatFields(VIDTRICK_Data_p->DisplayHandle, NbOfFieldsToRepeatForThisPicture, &NbFieldsRepeated);

                /* TheNumber NbOfFieldsToRepeatForThisPicture has to be added to the Nb of fields to skip counter */
                /* this counter should then rapidly become over VIDTRICK_LEVEL_SPACE_BUFFERS and added to NbFieldsToRecover */
                /* Next Vsync Trickmod computation should then  */
                VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat += (S32) NbFieldsRepeated;

#ifdef TRACE_UART_DISPLAY_SKIP
                TraceBuffer(("RepeatFieldsPic=%d SkipRepeat=%d\r\n",
                        NbOfFieldsToRepeatForThisPicture,
                        VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat));
#endif
                /* VIDTRICK_Data_p->BackwardFluidity.BackwardFluentDisplayProcessIsRunning = TRUE;*/
            }
        } /* if (PictureToInsertInDQ_p->LinearBitBufferStoredInfo_p == LastInsertedPictureInDQ_p->LinearBitBufferStoredInfo_p) */
    } /* if ((TemporalRefJumpBetweenBothPic > 1) */
}

/*******************************************************************************
Name        : ResetReferencesAndFreeNotDisplayedPictures
Description : This function removes the current references in the decoder and
              frees the not displayed pictures. Should be used when stopping
              smooth backward or when switching from backward to forward, or
              from forward to backward.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void  ResetReferencesAndFreeNotDisplayedPictures(const VIDTRICK_Handle_t TrickModeHandle)
{
    VIDTRICK_Data_t  *      VIDTRICK_Data_p = (VIDTRICK_Data_t *) TrickModeHandle;

    /* resets references of decode. */
#ifdef TRACE_UART_DEBUG
    TraceBuffer(("Reset all references\n\r"));
#endif
    VIDDEC_ReleaseReferencePictures(VIDTRICK_Data_p->DecodeHandle);

#ifdef ST_ordqueue
    /* Cancel all pictures waiting in ordering queue */
    VIDQUEUE_CancelAllPictures(VIDTRICK_Data_p->OrderingQueueHandle);
#endif /* ST_ordqueue */

    /* Frees all not displayed pictures */
    VIDDISP_FreeNotDisplayedPicturesFromDisplayQueue(VIDTRICK_Data_p->DisplayHandle);

    /* Ensure that every Picture Buffers are released by vid_tric */
    ReleaseAllPictureInfoElements(TrickModeHandle);

    /* Recover Frame Buffers still locked after change between forward and backward mode */
    VIDBUFF_RecoverFrameBuffers(VIDTRICK_Data_p->BufferManagerHandle);

} /* End of ResetReferencesAndFreeNotDisplayedPictures() function */

/*******************************************************************************
Name        : SearchForMostRecentDecodableReferencePictureRecord
Description : Find from the current extended temporal reference the most recent not decoded but decocable (means that its
              Backward reference is decoded )
Parameters  :
Assumptions :
Limitations :
Returns     : TRUE if found or FALSE can not find it.
*******************************************************************************/
static BOOL SearchForMostRecentDecodableReferencePictureRecord(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t * const CurrentPictureInfo_p, VIDTRICK_PictureInfo_t ** const FoundPictureInfo_p)
{
    VIDTRICK_PictureInfo_t  * PictureInfo_p;
    BOOL                      SequenceBeforeHasAlreadyParsed = FALSE;
    BOOL                      SearchedPictureIsFound = FALSE;
	UNUSED_PARAMETER(TrickModeHandle);

    if (CurrentPictureInfo_p == NULL)
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
       /* TraceBuffer(("Unexpected NULL pointer.\n\r")) ;*/
#endif /* TRICKMOD_DEBUG */
        return (FALSE);
    }

    /* Find first reference Picture from the given current one */
    if (PictureMpegTypeIsB(CurrentPictureInfo_p))
    {
        PictureInfo_p = CurrentPictureInfo_p->ForwardOrNextReferencePicture_p;
    }
    else
    {
        PictureInfo_p = CurrentPictureInfo_p;
    }

    /* While the searched picture record is not found */
    while (! SearchedPictureIsFound)
    {
        if (   (PictureInfo_p == NULL)
            || (!PictureInfo_p->IsDecodable))
        {
            /* backward references are still not parsed */
            /* TraceBuffer(("Back ref is missing Current ETR=%d! number in buffer=%d\n\r",
                    CurrentPictureInfo_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference,
                    CurrentPictureInfo_p->IndiceInBuffer));*/
            return (FALSE);
        }

        switch (PictureInfo_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame)
        {
            /* If I picture, if not decoded, it is the searched picture, else a search must be done in the previous sequence */
            case STVID_MPEG_FRAME_I :
                if (!IsPictureSecondField(PictureInfo_p))
                {
                    /* if not decoded, the searched picture is this one */
                    if (!PictureInfo_p->IsDecoded)
                    {
                        *FoundPictureInfo_p = PictureInfo_p;
                        return (TRUE);
                    }
                    else
                    /* The sequence before must be treated now */
                    {
                        if (SequenceBeforeHasAlreadyParsed)
                        {
                            /* If all picturee of previous sequence are already decoded */
                            /* do not try to search in previous sequence !!!! */
                            /* return FALSE but it is not a ERROR !!!!! */
                            return (FALSE);
                        }
                        /* remember that we are parsing the previous sequence */
                        SequenceBeforeHasAlreadyParsed = TRUE;
                        PictureInfo_p = PictureInfo_p->BackwardOrLastBeforeIReferencePicture_p;
                    }
                    break;
                } /* !IsPictureSecondField(PictureInfo_p) : if I picture && 2nd field its I backward has to be decoded before */

            case STVID_MPEG_FRAME_P :
                /* the searched picture is the first no decoded with its backward reference decoded */
                if (PictureInfo_p->BackwardOrLastBeforeIReferencePicture_p != NULL)
                {
                    /*  */
                    if (   (!PictureInfo_p->IsDecoded)
                        && (PictureInfo_p->BackwardOrLastBeforeIReferencePicture_p->IsDecoded))
                    {
                        *FoundPictureInfo_p = PictureInfo_p;
                        return (TRUE);
                    }
                    else
                    {
                        PictureInfo_p = PictureInfo_p->BackwardOrLastBeforeIReferencePicture_p;
                    }
                }
                else
                {
                    return (FALSE);
                }
                break;

            default :
#ifdef TRICKMOD_DEBUG
                TrickModeError();
#endif /* TRICKMOD_DEBUG */
                return(FALSE);
                break;
        }
    }

    /* Should never exit from here, but to avoid warnings ... */
    return (FALSE);
} /* end of SearchForMostRecentDecodableReferencePictureRecord() function */

/*******************************************************************************
Name        : SearchForNormalRevSpeedPreviousPicToDisplay
Description : Search for Picture Record from the Picture List (PL). It finds the
              VIDTRICK_PictureInfo_t structure before the given VIDTRICK_PictureInfo_t
              analysing the MPEG flow.
Parameters  :
Assumptions :
Limitations :
Returns     : TRUE or FALSE
*******************************************************************************/
static BOOL SearchForNormalRevSpeedPreviousPicToDisplay(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t * const CurrentPictureInfo_p, VIDTRICK_PictureInfo_t ** const PictureInfo_p)
{
#ifdef TRICKMOD_DEBUG
    /*VIDTRICK_Data_t        * VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;*/
#endif /* TRICKMOD_DEBUG */
    VIDTRICK_PictureInfo_t * NextFirstFieldOrFrame_p = NULL;

    *PictureInfo_p = NULL;
	UNUSED_PARAMETER(TrickModeHandle);

    if (CurrentPictureInfo_p == NULL)
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
         TraceBuffer((" Null pointer-1 ! \n\r"));
#endif /* TRICKMOD_DEBUG */
        return (FALSE);
    }

    /* Particular case : we have a discontinuity between the buffers (we don't match between the buffers). */
    /* The previous to display before the last displayable of this buffer is the first displayable of the  */
    /* other buffer.                                                                                       */
    if (CurrentPictureInfo_p->FirstDisplayablePictureAfterJump_p != NULL)
    {
        *PictureInfo_p = CurrentPictureInfo_p->FirstDisplayablePictureAfterJump_p;
#ifdef TRICKMOD_DEBUG
/*  The following code was commented because we don't know which linear bitbuffer has no continuity with
 * the previous. So how to change the following condition ?

        if (   (!VIDTRICK_Data_p->Discontinuity.DiscontinuityWithPreviousBuffer)
            || (IsPictureSecondField(CurrentPictureInfo_p)))
        {
            TrickModeError();
        } */
#endif /* TRICKMOD_DEBUG */
    }
    /*- Second field picture -------------------------------------------------*/
    else if (IsPictureSecondField(CurrentPictureInfo_p))
    {
        *PictureInfo_p = CurrentPictureInfo_p->PreviousInStreamPicture_p;
    }
    /*- normal cases: Frame or First field -----------------------------------*/
    /*- B picture ------------------------------------------------------------*/
    else if (PictureMpegTypeIsB(CurrentPictureInfo_p))
    {
        /* if reaching beginning of the buffer, Backward and previous in the stream */
        /* may still not be initialized : can not find searched picture : return FALSE */
        if (   (CurrentPictureInfo_p->PreviousInStreamPicture_p == NULL)
            || (CurrentPictureInfo_p->BackwardOrLastBeforeIReferencePicture_p == NULL))
        {
#ifdef TRICKMOD_DEBUG
            TrickModeError();
            TraceBuffer(("Null pointer-2 ! \n\r"));
#endif
            return (FALSE);
        }

        /* If previous Picture in the stream is a B picture : it is this one */
        if (PictureMpegTypeIsB(CurrentPictureInfo_p->PreviousInStreamPicture_p))
        {
            *PictureInfo_p = CurrentPictureInfo_p->PreviousInStreamPicture_p;
        }
        else
        {
            *PictureInfo_p = CurrentPictureInfo_p->BackwardOrLastBeforeIReferencePicture_p;
        }
    }
    /*- reference picture ----------------------------------------------------*/
    else
    {
        /* if reaching beginning of the buffer, Backward may still not be initialized : can not find */
        /* searched picture : return FALSE */
        if (CurrentPictureInfo_p->BackwardOrLastBeforeIReferencePicture_p == NULL)
        {
            /*
#ifdef TRICKMOD_DEBUG
            TrickModeError();
            TraceBuffer(("reaching beginning of the buffer!\n\r"));
#endif */
            return (FALSE);
        }

        /* if forward == NULL means this reference is the last reference of the buffer and its flush  */
        /* is discontinued with its previous */
        if (CurrentPictureInfo_p->ForwardOrNextReferencePicture_p == NULL)
        {
            if (CurrentPictureInfo_p->IndiceInBuffer < CurrentPictureInfo_p->LinearBitBufferStoredInfo_p->NbPicturesInside - 1)
            {
                *PictureInfo_p = &(CurrentPictureInfo_p->LinearBitBufferStoredInfo_p->PictureInfoTableStartAddress_p[CurrentPictureInfo_p->LinearBitBufferStoredInfo_p->NbPicturesInside - 1]);
#ifdef TRICKMOD_DEBUG
                if (!(PictureMpegTypeIsB(*PictureInfo_p)))
                {
                    TrickModeError();
                }
#endif /* TRICKMOD_DEBUG */
            }
            else
            {
                *PictureInfo_p = CurrentPictureInfo_p->BackwardOrLastBeforeIReferencePicture_p;
            }
        }
        else
        {
            /* searching the searched picture ... */
            if (IsPictureFrame(CurrentPictureInfo_p))
            {
                NextFirstFieldOrFrame_p = CurrentPictureInfo_p->ForwardOrNextReferencePicture_p;
            }
            else
            {
                NextFirstFieldOrFrame_p = CurrentPictureInfo_p->ForwardOrNextReferencePicture_p->ForwardOrNextReferencePicture_p;
            }

            /* Warning: NextFirstFieldOrFrame_p can be NULL! */
            if (NextFirstFieldOrFrame_p != NULL)
            {
                /* if the given picture and its forward first field or frame are separated by B pictures ... */
                if (PictureMpegTypeIsB(NextFirstFieldOrFrame_p->PreviousInStreamPicture_p))
                {
                    *PictureInfo_p = NextFirstFieldOrFrame_p->PreviousInStreamPicture_p;
                }
                else
                {
                    *PictureInfo_p = CurrentPictureInfo_p->BackwardOrLastBeforeIReferencePicture_p;
                }
            }
        }
    }

    if (*PictureInfo_p == NULL)
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
       TraceBuffer(("Previous in display not found ! \n\r"));
#endif /* TRICKMOD_DEBUG */
        return (FALSE);
    }

    /* picture may not be decodable : it must not be returned */
    if (! (*PictureInfo_p)->IsDecodable)
    {
       /*TraceBuffer(("Error Picture not decodable ETR=%d.\n\r", CurrentPictureInfo_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference));*/
       return (FALSE);
    }

    if (CurrentPictureInfo_p == *PictureInfo_p)
    {
        *PictureInfo_p = NULL;

#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("both pictures are the same !!!\n\r"));
#endif /* TRICKMOD_DEBUG */
        return (FALSE);
    }

#ifdef TRICKMOD_DEBUG
    /* just for test : two following picture are not supposed to have the same Extended Temp ref if next to be inserted is a frame */
    if (   ((*PictureInfo_p)->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
        && (CurrentPictureInfo_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference == (*PictureInfo_p)->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference))
    {
        TrickModeError();
    }
#endif /* TRICKMOD_DEBUG */

   return (TRUE);
} /* end of SearchForNormalRevSpeedPreviousPicToDisplay() function */

/*******************************************************************************
Name        : SmoothBackwardInitStructure
Description : parameters to be initialized when trickmode is initialized
              and when switching from positive to negative speed
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SmoothBackwardInitStructure(const VIDTRICK_Handle_t TrickModeHandle)
{
    VIDTRICK_Data_t   * VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    ST_ErrorCode_t      ErrorCode;

#ifdef TRACE_UART_TOPBOT_ALTERNATING
    TraceBuffer(("%d W=NULL\n\r", __LINE__));
#endif

    /* inits */
    VIDTRICK_Data_p->Flags.DoNotMatchBufAWithBufB = TRUE;
    VIDTRICK_Data_p->Flags.SmoothBackwardSpeedHasBeenChanged = FALSE;
    VIDTRICK_Data_p->Flags.FirstSliceStartCodeFoundInUnvalidArea = FALSE;
    VIDTRICK_Data_p->Flags.AtLeastOnePictureHasBeenInserted = FALSE;
    VIDTRICK_Data_p->NumberOfFlushesToRestartFromTheSamePicture = 1;
    VIDTRICK_Data_p->LinearBitBuffers.BufferA.DiscontinuityWithPreviousBuffer = FALSE;
    VIDTRICK_Data_p->LinearBitBuffers.BufferB.DiscontinuityWithPreviousBuffer = FALSE;
    VIDTRICK_Data_p->LinearBitBuffers.BufferA.InjectionDuration = (VIDTRICK_BACKWARD_PICTURES_NB_BY_BUFFER * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) / VIDTRICK_DEFAULT_FRAME_RATE;
    VIDTRICK_Data_p->LinearBitBuffers.BufferB.InjectionDuration = (VIDTRICK_BACKWARD_PICTURES_NB_BY_BUFFER * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) / VIDTRICK_DEFAULT_FRAME_RATE;
    ErrorCode = GetBitRateValue(TrickModeHandle, &VIDTRICK_Data_p->LinearBitBuffers.BufferA.BeginningOfInjectionBitrateValue);
    if(ErrorCode != ST_NO_ERROR)
    {
        VIDTRICK_Data_p->LinearBitBuffers.BufferA.BeginningOfInjectionBitrateValue = VIDTRICK_MAX_BITRATE;
    }
    VIDTRICK_Data_p->LinearBitBuffers.BufferB.BeginningOfInjectionBitrateValue = VIDTRICK_Data_p->LinearBitBuffers.BufferA.BeginningOfInjectionBitrateValue;
    VIDTRICK_Data_p->PictureUnderDecoding_p = NULL;
    VIDTRICK_Data_p->LinearBitBuffers.BufferUnderTransfer_p = NULL;
    VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p = NULL;
    VIDTRICK_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p = NULL;
    VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ = 0;
    VIDTRICK_Data_p->Discontinuity.NbFieldsToRecover = 0;
    VIDTRICK_Data_p->Discontinuity.OverlapDuration =  (VIDTRICK_BACKWARD_OVERLAP_PICTURES * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) / VIDTRICK_DEFAULT_FRAME_RATE;
#ifdef OLD_BACKWARD_FLUENT_DISPLAY_PROCESS
    VIDTRICK_Data_p->BackwardFluidity.BackwardFluentDisplayProcessIsRunning = FALSE;
#endif /* OLD_BACKWARD_FLUENT_DISPLAY_PROCESS */
    VIDTRICK_Data_p->BackwardFluidity.LastBackwardFieldsSkippedNb = 0;
    VIDTRICK_Data_p->BackwardFluidity.LastInsertedInDQPicRepeatedFieldsNb = 0;
    VIDTRICK_Data_p->BackwardFluidity.TransferBeginDate = 0;
    VIDTRICK_Data_p->BackwardFluidity.NbFieldsToBeDisplayedToBeFluent = 0;
    VIDTRICK_Data_p->BackwardFluidity.NbFieldsToBeDisplayedPerPicture = 1;
    VIDTRICK_Data_p->BackwardFluidity.NbFieldsToBeDisplayed = 1;

    VIDTRICK_Data_p->BackwardFluidity.VSyncEventNb = 0;
    VIDTRICK_Data_p->BackwardFluidity.DisplayQueueEventEmptyNb = 0;

    /* Error Recovery init */
    VIDTRICK_Data_p->ErrorRecovery.NbVsyncElapsedSinceLastDecodeHasBeenLaunched = 0;
    VIDTRICK_Data_p->ErrorRecovery.NbVSyncElapsedSinceLastSCDHasBeenLaunched = 0;
    VIDTRICK_Data_p->ErrorRecovery.NbVsyncElapsedWithoutAnyDecode = 0;
    VIDTRICK_Data_p->ErrorRecovery.SuccessiveDecodeSecondFieldErrorsNb = 0;
    VIDTRICK_Data_p->ErrorRecovery.SuccessiveDecodeFrameOrFirstFieldErrorsNb = 0;

    /* Smooth Stop parameters, NB: TrickmodIsStopped must not be set here but in the calling function */
    VIDTRICK_Data_p->Stop.TrickmodStopIsPending = FALSE;
    VIDTRICK_Data_p->Stop.LinearBitBufferFlushRequirementAvailable = TRUE;

    VIDTRICK_Data_p->NegativeToPositiveSpeedParams.ForwardDisplayQueueInsertionIsEnabled = TRUE;
    VIDTRICK_Data_p->NegativeToPositiveSpeedParams.NextReferenceFound = TRUE;          /* Don't care in BW play */

    VIDTRICK_Data_p->NewStartCodeSearch.MustWaitBecauseOfVSync = FALSE;
    VIDTRICK_Data_p->NewStartCodeSearch.DontStartBeforeThatTime = time_now();

#ifdef TRACE_UART_MAIN
    TraceBuffer(("SmoothBackward initialized\r\n"));
#endif
} /* end of SmoothBackwardInitStructure() function */


/*******************************************************************************
Name        : NewPictureFoundInBitBuffer
Description : Callback of the event VIDDEC_NEW_START_CODE_SLICE_FOUND_IN_BIT_BUFFER
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void NewPictureFoundInBitBuffer(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, STEVT_EventConstant_t Event, const void * EventData_p, const void * SubscriberData_p)
{
    VIDTRICK_Handle_t        		TrickModeHandle;
    VIDDEC_StartCodeFoundInfo_t *   StartCodeFound_p;
    VIDTRICK_LinearBitBuffer_t *    CurrentBuffer_p;
    VIDTRICK_Data_t *               VIDTRICK_Data_p;
    void *                          StartCodeAddress_p;
    BOOL                            StartCodeDetectorHasToBelaunched;
    U32                             PictID;

    TrickModeHandle = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDTRICK_Handle_t, SubscriberData_p);
    /* find structure */
    VIDTRICK_Data_p     = (VIDTRICK_Data_t *)TrickModeHandle;

    /* Lock mutex protecting Trick mode private params */
    /* No protection because no sharing of these data : the task is managing data for the other buffer. */
    /* STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p); */

    /* Retrieve the parameters given with the event. */
    StartCodeFound_p = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDDEC_StartCodeFoundInfo_t *, EventData_p);
    StartCodeAddress_p  = StartCodeFound_p->StartCodeAddress_p;

	/* unused parameters */
	UNUSED_PARAMETER(Reason);
	UNUSED_PARAMETER(RegistrantName);
	UNUSED_PARAMETER(Event);

    /*  */
    StartCodeDetectorHasToBelaunched = TRUE;
    CurrentBuffer_p = VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p;

/* TraceBuffer(("*pic SC=0x%x M=%d t=%d Add=0x%x\r\n", */
            /* StartCodeFound_p->StartCodeId, */
            /* StartCodeFound_p->PictureInfoFromDecode_p->PictureInfos.VideoParams.MPEGFrame, */
            /* StartCodeFound_p->PictureInfoFromDecode_p->StreamInfoForDecode.Picture.temporal_reference, */
            /* StartCodeFound_p->StartCodeAddress_p)); */

    /* TraceBuffer(("M=%d   H:%d M:%d S:%d F:%d \r\n", */
            /* StartCodeFound_p->PictureInfoFromDecode_p->PictureInfos.VideoParams.MPEGFrame, */
            /* StartCodeFound_p->PictureInfoFromDecode_p->PictureInfos.VideoParams.TimeCode.Hours, */
            /* StartCodeFound_p->PictureInfoFromDecode_p->PictureInfos.VideoParams.TimeCode.Minutes, */
            /* StartCodeFound_p->PictureInfoFromDecode_p->PictureInfos.VideoParams.TimeCode.Seconds, */
            /* StartCodeFound_p->PictureInfoFromDecode_p->PictureInfos.VideoParams.TimeCode.Frames */
            /* )); */

    /* if StartCodeId is placed in the valid data end and transfered data end area. */
    /* because FIRST_SLICE_START_CODE is the only start code which calls StartCodeFoundInBitBuffer AND */
    /* NewPictureFoundInBitBuffer functions */
    if (VIDTRICK_Data_p->Flags.FirstSliceStartCodeFoundInUnvalidArea)
    {
        VIDTRICK_Data_p->Flags.FirstSliceStartCodeFoundInUnvalidArea = FALSE;
        return;
    }

    if (CurrentBuffer_p == NULL)
    {
        /* should not occur only  */
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("Start code found but no buffer under parsing.\n\r")) ;
#endif
       return;
    }

    if (StartCodeFound_p->StartCodeId != FIRST_SLICE_START_CODE)
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("Bad start code ID.\n\r")) ;
#endif /* TRICKMOD_DEBUG */
    }

    if ((StartCodeAddress_p <= CurrentBuffer_p->WholeBufferStartAddress_p) || (StartCodeAddress_p >= CurrentBuffer_p->ValidDataBufferEndAddress_p))
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("Bad start code address.\n\r"));
#endif /* TRICKMOD_DEBUG */
        /* the start code detector is lost : reinitialze the parsing of the buffer */
/*        NewStartCodeSearchIsRequired(TrickModeHandle, VIDDEC_START_CODE_SEARCH_MODE_FROM_ADDRESS, TRUE, CurrentBuffer_p->WholeBufferStartAddress_p);*/
        /* Lock mutex protecting Trick mode private params */
        STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p);
        if (!(VIDTRICK_Data_p->NewStartCodeSearch.IsRequiredInTask))
        {
            VIDTRICK_Data_p->NewStartCodeSearch.IsRequiredInTask = TRUE;
#ifdef TRICKMOD_DEBUG
            if (VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p == NULL)
            {
                TrickModeError();
            }
#endif
            VIDTRICK_Data_p->NewStartCodeSearch.SearchMode = VIDDEC_START_CODE_SEARCH_MODE_FROM_ADDRESS;
            VIDTRICK_Data_p->NewStartCodeSearch.FirstSliceDetect = TRUE;
            VIDTRICK_Data_p->NewStartCodeSearch.SearchAddress_p = CurrentBuffer_p->WholeBufferStartAddress_p;
        }
#ifdef TRICKMOD_DEBUG
        else
        {
            TrickModeError();
        }
#endif /* TRICKMOD_DEBUG */

        /* Unlock mutex protecting Trick mode private params */
        STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
        CurrentBuffer_p->NbPicturesInside = 0;
        semaphore_signal(VIDTRICK_Data_p->WakeTrickModeTaskUp_p);
        return;
    }

    if (CurrentBuffer_p->ParsingBeginOfRelevantDataFound)
    {
        /* We decide to not discard the start code in order that the decode stored the parameters. */

        /* Temporary tip while not pTemporalReference replaced by StreamInfoForDecode_p->Picture.temporal_reference. */
        StartCodeFound_p->PictureInfoFromDecode_p->pTemporalReference = StartCodeFound_p->PictureInfoFromDecode_p->StreamInfoForDecode.Picture.temporal_reference;

        /* We have to find if the picture found is the same than the first picture in the other table */
        /* if TRUE : we have found the first picture which is also in the other table. Current Buffer */
        /* Is fully parsed. */
        if (VIDTRICK_Data_p->Flags.DoNotMatchBufAWithBufB)
        {
#ifdef TRICKMOD_DEBUG
            /* if BufferADoesNotNeedToBeMatchedWithBufferB is TRUE we are parsing buffer A ! */
            if (VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p != &(VIDTRICK_Data_p->LinearBitBuffers.BufferA))
            {
                TrickModeError();
                TraceBuffer(("Unexpected buffer\n\r")) ;
            }
#endif /* TRICKMOD_DEBUG */
            /* buffer A Does not need to be matched because it is the first time it is flused : Buffer B is empty */

            /* Every Found picture should be matched with the displayeed one, to avoid to begin smooth backward start with */
            /* than the displayed one. But VIDDISP_GetDisplayedPictures don't give enough infos. That's why we'll */
            /* begin by displaying the last picture in buffer A */
        }

        if (CurrentBuffer_p->NbPicturesInside < VIDTRICK_BACKWARD_BUFFER_MAX_PICTURES_NB)
        {
            /* Store the parameters of the whole picture fileds by   */
            /* fields in our list.                                   */
            /* For a easier reading, use a local variable.           */
            PictID = CurrentBuffer_p->NbPicturesInside;

#ifdef TRICKMOD_DEBUG
            if (((CurrentBuffer_p->PictureInfoTableStartAddress_p)[PictID].PictureBuffer_p != NULL) && (!VIDTRICK_Data_p->Flags.SmoothBackwardIsInitializing))
            {
               /* should not happen except during the initializing stage (data have not been cleared before) */
               /* TrickModeError(); */
            }
#endif  /* TRICKMOD_DEBUG */

            /* Copy of PictureInfoFromDecode.                                 */
            CurrentBuffer_p->PictureInfoTableStartAddress_p[PictID].PictureInfoFromDecode = *(StartCodeFound_p->PictureInfoFromDecode_p);

            /* Store the address of the start code.                  */
            (CurrentBuffer_p->PictureInfoTableStartAddress_p)[PictID].AssociatedStartCodeAddress_p = StartCodeAddress_p;

            /* some inits */
            (CurrentBuffer_p->PictureInfoTableStartAddress_p)[PictID].IsInDisplayQueue = FALSE;
            (CurrentBuffer_p->PictureInfoTableStartAddress_p)[PictID].IsGoneOutOfDisplay = FALSE;
            (CurrentBuffer_p->PictureInfoTableStartAddress_p)[PictID].HasBeenInsertedInDisplayQueue = FALSE;
            (CurrentBuffer_p->PictureInfoTableStartAddress_p)[PictID].IsDecoded = FALSE;
            (CurrentBuffer_p->PictureInfoTableStartAddress_p)[PictID].IsDecodable = FALSE;
            (CurrentBuffer_p->PictureInfoTableStartAddress_p)[PictID].DisplayUnlockHasBeenRequired = FALSE;
            (CurrentBuffer_p->PictureInfoTableStartAddress_p)[PictID].AnyBToDecodeUnlockHasBeenRequired = FALSE;
            (CurrentBuffer_p->PictureInfoTableStartAddress_p)[PictID].PictureBuffer_p = NULL;
            (CurrentBuffer_p->PictureInfoTableStartAddress_p)[PictID].BackwardOrLastBeforeIReferencePicture_p = NULL;
            (CurrentBuffer_p->PictureInfoTableStartAddress_p)[PictID].FirstDisplayablePictureAfterJump_p = NULL;
            (CurrentBuffer_p->PictureInfoTableStartAddress_p)[PictID].ForwardOrNextReferencePicture_p = NULL;
            (CurrentBuffer_p->PictureInfoTableStartAddress_p)[PictID].SecondFieldPicture_p = NULL;
            (CurrentBuffer_p->PictureInfoTableStartAddress_p)[PictID].PreviousInStreamPicture_p = NULL;
            (CurrentBuffer_p->PictureInfoTableStartAddress_p)[PictID].IndiceInBuffer = PictID;

            /* Store First available Time Code picture. It will be the starting point of */
            /* RenumberTimeCode function. */
            if (   (VIDTRICK_Data_p->NegativeToPositiveSpeedParams.FirstGOPFound)
                && (VIDTRICK_Data_p->NegativeToPositiveSpeedParams.FirstNullTempRefPicture_p == NULL)
                && (StartCodeFound_p->PictureInfoFromDecode_p->StreamInfoForDecode.Picture.temporal_reference == 0))
            {
                VIDTRICK_Data_p->NegativeToPositiveSpeedParams.FirstNullTempRefPicture_p = &(CurrentBuffer_p->PictureInfoTableStartAddress_p[PictID]);
            }

            /* if first picture found is a field picture, discard it because every field picture parsed first */
            /* is considerated by decode as excepting its second field ! */
            if (IsPictureField(CurrentBuffer_p->PictureInfoTableStartAddress_p) && (!CurrentBuffer_p->FirstPictureOfLBBFFound))
            {
                CurrentBuffer_p->NbPicturesInside = 0;
            }
            else
            {
                CurrentBuffer_p->NbPicturesInside++;
                /* Try to compute the bitrate on the first pictures of the bitbuffer, to have a bitrate value near to the value
                 * we would get on the next injection */
                if (VIDTRICK_Data_p->MainParams.FrameRateValue == 0)
                {
                    VIDTRICK_Data_p->MainParams.FrameRateValue = 30000;
#ifdef TRICKMOD_DEBUG
                    TrickModeError();
                    TraceBuffer(("Frame rate should not be 0 \n\r"));
#endif
                }
                if(CurrentBuffer_p->NbFieldsInside <= (((VIDTRICK_Data_p->MainParams.FrameRateValue * 2) + 999)/1000))
                {
                    CurrentBuffer_p->BeginningOfInjectionBitrateValue =
                        (((((U32)StartCodeAddress_p - (U32)(CurrentBuffer_p->PictureInfoTableStartAddress_p[0].AssociatedStartCodeAddress_p)) * 8) / 400) / (((VIDTRICK_Data_p->MainParams.FrameRateValue * 2) + 999)/1000)) * CurrentBuffer_p->NbFieldsInside;
                }
                if(CurrentBuffer_p->NbFieldsInside != 0)
                {
                    CurrentBuffer_p->OneFieldAverageSize = ((U32)StartCodeAddress_p - (U32)(CurrentBuffer_p->PictureInfoTableStartAddress_p[0].AssociatedStartCodeAddress_p)) / CurrentBuffer_p->NbFieldsInside;
                }
                /* Compute the number of fields inside the current bitbuffer */
                CurrentBuffer_p->NbFieldsInside +=
                    (   CurrentBuffer_p->PictureInfoTableStartAddress_p[PictID].PictureInfoFromDecode.AdditionalStreamInfo.TopFieldCounter
                      + CurrentBuffer_p->PictureInfoTableStartAddress_p[PictID].PictureInfoFromDecode.AdditionalStreamInfo.BottomFieldCounter
                      + CurrentBuffer_p->PictureInfoTableStartAddress_p[PictID].PictureInfoFromDecode.AdditionalStreamInfo.RepeatFieldCounter)
                    * ( CurrentBuffer_p->PictureInfoTableStartAddress_p[PictID].PictureInfoFromDecode.AdditionalStreamInfo.RepeatProgressiveCounter + 1);

            }
            CurrentBuffer_p->FirstPictureOfLBBFFound = TRUE;
        }
        else
        {
            /* if too many pictures */
            CurrentBuffer_p->DiscontinuityWithPreviousBuffer = TRUE;
        }
    } /* end of RelevantData */

    if (StartCodeDetectorHasToBelaunched)
    {
/*        NewStartCodeSearchIsRequired(TrickModeHandle, VIDDEC_START_CODE_SEARCH_MODE_NORMAL, FALSE, NULL);*/
        /* Lock mutex protecting Trick mode private params */
        STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p);
        if (!(VIDTRICK_Data_p->NewStartCodeSearch.IsRequiredInTask))
        {
            VIDTRICK_Data_p->NewStartCodeSearch.IsRequiredInTask = TRUE;
#ifdef TRICKMOD_DEBUG
            if (VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p == NULL)
            {
                TrickModeError();
            }
#endif
            VIDTRICK_Data_p->NewStartCodeSearch.SearchMode = VIDDEC_START_CODE_SEARCH_MODE_NORMAL;
            VIDTRICK_Data_p->NewStartCodeSearch.FirstSliceDetect = FALSE;
            VIDTRICK_Data_p->NewStartCodeSearch.SearchAddress_p = (void *) 0;
        }
#ifdef TRICKMOD_DEBUG
        else
        {
            TrickModeError();
        }
#endif /* TRICKMOD_DEBUG */
        /* Unlock mutex protecting Trick mode private params */
        STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
        /* TraceBuffer(("Search next Start Code \r\n")); */
    }

    /* Lock mutex protecting Trick mode private params */
    /* STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p);*/

    /* Trick Mode Task has to be waked up so that Fields have to be skipped if necessary. */
    semaphore_signal(VIDTRICK_Data_p->WakeTrickModeTaskUp_p);
} /* end of NewPictureFoundInBitBuffer() function */

/*******************************************************************************
Name        : NewSequenceFoundInBitBuffer
Description : Callback of the event STVID_SEQUENCE_INFO_EVT
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void NewSequenceFoundInBitBuffer(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, STEVT_EventConstant_t Event, const void * EventData_p, const void * SubscriberData_p)
{
    VIDTRICK_Handle_t               TrickModeHandle;
    VIDTRICK_Data_t *               VIDTRICK_Data_p;
    STVID_SequenceInfo_t *          SequenceInfo_p;

    TrickModeHandle = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDTRICK_Handle_t, SubscriberData_p);
    /* find structure */
    VIDTRICK_Data_p     = (VIDTRICK_Data_t *)TrickModeHandle;
    SequenceInfo_p = CAST_CONST_HANDLE_TO_DATA_TYPE(STVID_SequenceInfo_t *, EventData_p);

    /* unused parameters */
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    if((SequenceInfo_p->ProfileAndLevelIndication != VIDTRICK_Data_p->MainParams.LastSequenceInfo.ProfileAndLevelIndication) || (SequenceInfo_p->FrameRate != VIDTRICK_Data_p->MainParams.LastSequenceInfo.FrameRate))
    {
        VIDTRICK_Data_p->MainParams.LastSequenceInfo = (*SequenceInfo_p);
        UpdateUnderflowEventParams(TrickModeHandle);
    }
    else
    {
        VIDTRICK_Data_p->MainParams.LastSequenceInfo = (*SequenceInfo_p);
    }
} /* end of NewSequenceFoundInBitBuffer() function */

/*******************************************************************************
Name        : BitbufferUnderflowEventOccured
Description : Callback of the event STVID_DATA_UNDERFLOW_EVT, used to be make
              the trickmode aware about the underflow event sent by the decode
               module.
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void BitbufferUnderflowEventOccured(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p)
{
    VIDTRICK_Handle_t       TrickModeHandle = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDTRICK_Handle_t, SubscriberData_p);
    VIDTRICK_Data_t *               VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;

    /* unused parameters */
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData_p);

    /* Reset the discontinuity flag when a new underflow occurs */
    if (VIDTRICK_Data_p->MainParams.Speed >= 0)
    {
        VIDTRICK_Data_p->ForwardInjectionDiscontinuity = FALSE;
    }
} /* end of BitbufferUnderflowEventOccured() function */

/*******************************************************************************
Name        : UpdateUnderflowEventParams
Description : Changes the params for the next underflow event according to the
              current profile, level, framerate, discontinuity.
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void UpdateUnderflowEventParams(const VIDTRICK_Handle_t TrickModeHandle)
{
    U32                             PictureAverageSize;
    U32                             PictureDuration;
    VIDDEC_CircularBitbufferParams_t    CircularBitbufferParams;
    VIDTRICK_Data_t *               VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    U32                             MaxBitrate;
    U32                             UnderflowMargin;
    U32                             RequiredDuration;

    /* Set the bitbuffer underflow threshold according to the profile and level */
    switch (VIDTRICK_Data_p->MainParams.LastSequenceInfo.ProfileAndLevelIndication & PROFILE_AND_LEVEL_IDENTIFICATION_MASK)
    {
        case (PROFILE_MAIN | LEVEL_HIGH) : /* MPHL */
        case (PROFILE_MAIN | LEVEL_HIGH_1440) : /* MP HL1440 */
            MaxBitrate = VIDCOM_MAX_BITRATE_MPEG_HD;
            break;

        case (PROFILE_MAIN | LEVEL_MAIN) : /* MPML */
        default :
            MaxBitrate = VIDCOM_MAX_BITRATE_MPEG_SD;
            break;
    }
    PictureAverageSize = ((MaxBitrate * 400) / 8) / ((VIDTRICK_Data_p->MainParams.FrameRateValue + 999) / 1000);
#if 1
    /* CRE/WHE Teamlog & WINCE project, correct a bug of divide  by 0 when this function is called without FrameRate initialization*/
     /* to be confirmed*/

	if(VIDTRICK_Data_p->MainParams.LastSequenceInfo.FrameRate == 0)
	{
        VIDTRICK_Data_p->MainParams.LastSequenceInfo.FrameRate = 30000; /* set default value*/
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("Frame rate should not be 0 \n\r"));
#endif
	}
#endif
	PictureDuration = ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION * 1000) / VIDTRICK_Data_p->MainParams.LastSequenceInfo.FrameRate);

    if(VIDTRICK_Data_p->MainParams.Speed <= 100)
    {
        UnderflowMargin  = PictureAverageSize * VIDCOM_DECODER_SPEED_MPEG;
        RequiredDuration = PictureDuration * VIDCOM_DECODER_SPEED_MPEG;
    }
    else /* Speed > 100 */
    {
        UnderflowMargin  = PictureAverageSize * ((VIDTRICK_Data_p->MainParams.Speed + 99) / 100) * VIDCOM_DECODER_SPEED_MPEG;
        RequiredDuration = PictureDuration * ((VIDTRICK_Data_p->MainParams.Speed + 99) / 100) * VIDCOM_DECODER_SPEED_MPEG;
    }

    /* Compute max and min values for the underflow params */
    /*ErrorCode =*/ VIDDEC_GetCircularBitBufferParams(VIDTRICK_Data_p->DecodeHandle, &CircularBitbufferParams);

    /* First check : check overflow cases */
    if(UnderflowMargin + ((RequiredDuration + PictureDuration - 1) / PictureDuration) * PictureAverageSize > CircularBitbufferParams.Size)
    {
        RequiredDuration = PictureDuration * VIDCOM_DECODER_SPEED_MPEG;
        UnderflowMargin  = CircularBitbufferParams.Size - (((RequiredDuration + PictureDuration - 1) / PictureDuration) * PictureAverageSize);
    }

    /* Second check : jumps on HDD. In that case, we need to inject data that
       contain almost one I picture. Note that although if we can't require the
       whole duration VIDTRICK_MAX_TIME_BETWEEN_TWO_CONSECUTIVE_I_PICTURES
       during the following underflow, the driver will require more injections
       without jump, and then we are sur to find a new I picture after the
       discontinuity. */
    if(VIDTRICK_Data_p->ForwardInjectionDiscontinuity)
    {
        if(RequiredDuration < VIDTRICK_MAX_TIME_BETWEEN_TWO_CONSECUTIVE_I_PICTURES)
        {
            RequiredDuration = VIDTRICK_MAX_TIME_BETWEEN_TWO_CONSECUTIVE_I_PICTURES;
        }
        /* Check the bitbuffer size is enough to hold the required duration
         * otherwise we adjust again the required duration */
        if(CircularBitbufferParams.Size < (((RequiredDuration + PictureDuration - 1) / PictureDuration) * PictureAverageSize))
        {
            UnderflowMargin  = PictureAverageSize * VIDCOM_DECODER_SPEED_MPEG;
            RequiredDuration = ((CircularBitbufferParams.Size - UnderflowMargin) / PictureAverageSize) * PictureDuration;
        }
        else
        {
            U32 MaxUnderflowMargin;

            /* Adjust the underflow margin according to the required duration */
            MaxUnderflowMargin = CircularBitbufferParams.Size - (((RequiredDuration + PictureDuration - 1) / PictureDuration) * PictureAverageSize);
            if(UnderflowMargin > MaxUnderflowMargin)
            {
                UnderflowMargin = MaxUnderflowMargin;
            }
        }
    }

    /* Set new underflow params */
    VIDDEC_SetBitBufferUnderflowMargin(VIDTRICK_Data_p->DecodeHandle, UnderflowMargin);
    VIDDEC_SetBitBufferUnderflowRequiredDuration(VIDTRICK_Data_p->DecodeHandle, RequiredDuration);
} /* end of UpdateUnderflowEventParams() function */

/*******************************************************************************
Name        : StartCodeFoundInBitBuffer
Description : Callback of the event VIDDEC_NEW_START_CODE_FOUND_IN_BIT_BUFFER
Assumptions :
Limitations :
Returns     : Give in response the command of discard or not to the decoder.
*******************************************************************************/
static void StartCodeFoundInBitBuffer(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, STEVT_EventConstant_t Event, const void * EventData_p, const void * SubscriberData_p)
{
    VIDTRICK_Handle_t         		TrickModeHandle;
    VIDDEC_StartCodeFoundInfo_t *   StartCodeFound_p;
    VIDTRICK_LinearBitBuffer_t *    UnderParsingBuffer_p;
    ST_ErrorCode_t                  ErrorCode;
    VIDTRICK_Data_t *               VIDTRICK_Data_p;
    void *                          StartCodeAddress_p;
    BOOL                            DiscardStartCode, StartCodeDetectorHasToBelaunched;  /* To be set when start code detector has to be launched without being re-programmed */

    TrickModeHandle = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDTRICK_Handle_t, SubscriberData_p);
    /* find structure */
    VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    /* unused parameters */
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    if (VIDTRICK_Data_p->ValidityCheck != VIDTRICK_VALID_HANDLE)
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("Bad Validity ckeck\n\r"));
#endif /* TRICKMOD_DEBUG */
        return;
    }
    /* TraceBuffer(("-Sc(%d)", time_now()));*/

    /* Lock mutex protecting Trick mode private params */
    /* STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p); */

    /* Retrieve the parameters given with the event. */
    StartCodeFound_p = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDDEC_StartCodeFoundInfo_t *, EventData_p);

    if (StartCodeFound_p->StartCodeId == GROUP_START_CODE)
    {
        /* TraceBuffer(("-- GOP --\r\n")); */
        VIDTRICK_Data_p->NegativeToPositiveSpeedParams.FirstGOPFound = TRUE;
    }

/*     TraceBuffer(("SC 0x%x add=0x%x \r\n", StartCodeFound_p->StartCodeId, StartCodeFound_p->StartCodeAddress_p)); */

    /* Retrieve the current buffer under decode. */
    if (VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p != NULL)
    {
        UnderParsingBuffer_p = VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p;
    }
    else
    {
#ifdef TRICKMOD_DEBUG
        if (! VIDTRICK_Data_p->Flags.BackwardToForwardInitializing)
        {
          /* Speed is not switching from back to forward : ERROR case */
          TrickModeError();
          /* TraceBuffer(("No buffer in parsing process.\n\r"));*/
        }
#endif /* TRICKMOD_DEBUG */
        /* Should not occur. Except if a start code launch has been called in Automatic Mode just */
        /* before passing in Manual Mode. Then the start code found is not to take into account.   */
        /* It can also occur if speed is switching from Back to forward, all backward data are reset */
        /* including BufferUnderParsing_p which is then NULL */
        /* CAUTION !!!!! It is a workaround */
        VIDDEC_DiscardStartCode(VIDTRICK_Data_p->DecodeHandle, TRUE);
        /* Unlock */
        /*STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);*/
        return;
    }

    /* Error cases. */
    if ((StartCodeFound_p == NULL) || (StartCodeFound_p->StartCodeAddress_p == NULL))
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
       /* TraceBuffer(("The start Code found is NULL !!\n\r")) ;*/
#endif /* TRICKMOD_DEBUG */

        VIDDEC_DiscardStartCode(VIDTRICK_Data_p->DecodeHandle, TRUE);
        return;
    }

    StartCodeAddress_p = StartCodeFound_p->StartCodeAddress_p;
     /* TraceBuffer(("Start Code Found. Address = 0x%x\r\n", StartCodeAddress_p)); */
     /* TraceBuffer(("Start Code Found\n\r")); */

    /* Compare the address where the start code has been found  */

    /*---------------------------------------------*/
    /* first case : begin of the linear bit buffer */
    /*---------------------------------------------*/
    if (StartCodeAddress_p == UnderParsingBuffer_p->WholeBufferStartAddress_p)
    {

        /*TraceBuffer(("1--SC 0x%x add=0x%x \r\n", StartCodeFound_p->StartCodeId, StartCodeFound_p->StartCodeAddress_p));*/
        /* TraceBuffer(("SC found 1st case\r\n")); */
        /* The start code should be a sequence start code !!!!! */
        if ((StartCodeFound_p->StartCodeId) != SEQUENCE_HEADER_CODE)
        {
#ifdef TRICKMOD_DEBUG
             /* TrickModeError(); */
             TraceBuffer(("First Start code is not a sequence header one\n\r"));
#endif /* TRICKMOD_DEBUG */
            /* the start code detector is lost : reinitialze the parsing of the buffer */
            /* but  start code found means that Start code dectector is OK, and all following SC should be parsable !!! */
            /* ==> no need to re-launch it !!!!!! */
            /* NewStartCodeSearchIsRequired(TrickModeHandle, VIDDEC_START_CODE_SEARCH_MODE_FROM_ADDRESS, TRUE, UnderParsingBuffer_p->WholeBufferStartAddress_p); */
            /* UnderParsingBuffer_p->NbPicturesInside = 0; */
            /* return; */
        }

        /* We ask the software decoder to apply its counting of the Extended Temporal Reference but by starting from the */
        /* first picture of this new buffer (i.e. by reseting any kind of memory on the previous temporal rerferences.   */
        ErrorCode = VIDDEC_ResetTemporalRefCounters(VIDTRICK_Data_p->DecodeHandle);
#ifdef TRICKMOD_DEBUG
        if (ErrorCode != ST_NO_ERROR)
        {
            TrickModeError();
            /* TraceBuffer(("VIDDEC_ResetTemporalRefCounters failed with ErrorCode=%d\n\r", ErrorCode)) ;*/
        }
#endif /* TRICKMOD_DEBUG */

        /* The next data in the buffer will be relevant only from the first picture or sequence. i.e. if we find a slice */
        /* first we cannot do anything with it (no temporal ref for example).                                            */
        UnderParsingBuffer_p->ParsingBeginOfRelevantDataFound = FALSE;

        ErrorCode = VIDDEC_DiscardStartCode(VIDTRICK_Data_p->DecodeHandle, TRUE);
#ifdef TRICKMOD_DEBUG
        if (ErrorCode != ST_NO_ERROR)
        {
            TrickModeError();
            /* TraceBuffer(("VIDDEC_DiscardStartCode failed with ErrorCode=%d\n\r", ErrorCode)) ;*/
        }
#endif /* TRICKMOD_DEBUG */

/*        NewStartCodeSearchIsRequired(TrickModeHandle, VIDDEC_START_CODE_SEARCH_MODE_NORMAL, FALSE, NULL);*/
        /* Lock mutex protecting Trick mode private params */
        STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p);

        if (VIDTRICK_Data_p->Flags.BackwardToForwardInitializing)
        {
          /* Speed is switching from back to forward : Event should be ignored */
          STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
          return;
        }

        if (!(VIDTRICK_Data_p->NewStartCodeSearch.IsRequiredInTask))
        {
            VIDTRICK_Data_p->NewStartCodeSearch.IsRequiredInTask = TRUE;
#ifdef TRICKMOD_DEBUG
            if (VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p == NULL)
            {
                TrickModeError();
            }
#endif

            VIDTRICK_Data_p->NewStartCodeSearch.SearchMode = VIDDEC_START_CODE_SEARCH_MODE_NORMAL;
            VIDTRICK_Data_p->NewStartCodeSearch.FirstSliceDetect = FALSE;
            VIDTRICK_Data_p->NewStartCodeSearch.SearchAddress_p = (void *) 0;
        }
#ifdef TRICKMOD_DEBUG
        else
        {
            TrickModeError();
        }
#endif /* TRICKMOD_DEBUG */
        /* TraceBuffer(("-1>%d", time_now()));*/
        /* Unlock mutex protecting Trick mode private params */
        STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
    }

    /*----------------------------------------------------------------------------------------------------*/
    /* second case : Start code has been found in a valid data area. the header following can be computed */
    /* We never discard such start codes.                                                                 */
    /*----------------------------------------------------------------------------------------------------*/
    else if (StartCodeAddress_p < UnderParsingBuffer_p->ValidDataBufferEndAddress_p)
    {
/*        TraceBuffer(("2--SC 0x%x add=0x%x \r\n", StartCodeFound_p->StartCodeId, StartCodeFound_p->StartCodeAddress_p));*/
        /* TraceBuffer(("SC found 2nd case\r\n")); */

        if (StartCodeFound_p->StartCodeId != FIRST_SLICE_START_CODE)
        {
            if ( (!UnderParsingBuffer_p->ParsingBeginOfRelevantDataFound) &&
                ((StartCodeFound_p->StartCodeId == SEQUENCE_HEADER_CODE) ||
                (StartCodeFound_p->StartCodeId == GROUP_START_CODE) ||
                (StartCodeFound_p->StartCodeId == PICTURE_START_CODE)))
            {
                UnderParsingBuffer_p->ParsingBeginOfRelevantDataFound = TRUE;
            }

            if (StartCodeFound_p->StartCodeId != USER_DATA_START_CODE)
            {
                /* NewStartCodeSearchIsRequired(TrickModeHandle, VIDDEC_START_CODE_SEARCH_MODE_NORMAL, FALSE, NULL);*/
                /* Lock mutex protecting Trick mode private params */
                STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p);

                if (VIDTRICK_Data_p->Flags.BackwardToForwardInitializing)
                {
                  /* Speed is switching from back to forward : Event should be ignored */
                  STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
                  return;
                }

                if (!(VIDTRICK_Data_p->NewStartCodeSearch.IsRequiredInTask))
                {
                    VIDTRICK_Data_p->NewStartCodeSearch.IsRequiredInTask = TRUE;
#ifdef TRICKMOD_DEBUG
                    if (VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p == NULL)
                    {
                        TrickModeError();
                    }
#endif

                    VIDTRICK_Data_p->NewStartCodeSearch.SearchMode = VIDDEC_START_CODE_SEARCH_MODE_NORMAL;
                    VIDTRICK_Data_p->NewStartCodeSearch.FirstSliceDetect = FALSE;
                    VIDTRICK_Data_p->NewStartCodeSearch.SearchAddress_p = (void *) 0;
                }
#ifdef TRICKMOD_DEBUG
                else
                {
                    TrickModeError();
                }
#endif /* TRICKMOD_DEBUG */
                /* TraceBuffer(("-2>%d", time_now()));*/
                /* Unlock mutex protecting Trick mode private params */
                STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
            }
        }

        /* UnderParsingBuffer_p->LastStartCodeAddressFound_p = StartCodeAddress_p; */

        ErrorCode = VIDDEC_DiscardStartCode(VIDTRICK_Data_p->DecodeHandle, FALSE);
#ifdef TRICKMOD_DEBUG
        if (ErrorCode != ST_NO_ERROR)
        {
            TrickModeError();
            /* TraceBuffer(("VIDDEC_DiscardStartCode failed with ErrorCode=%d\n\r", ErrorCode)) ;*/
        }
#endif /* TRICKMOD_DEBUG */
    }

    /*---------------------------------------------------------------------------------------------------------*/
    /* third case : Start code has been found in a unvalid data area. the header following can NOT be computed */
    /*---------------------------------------------------------------------------------------------------------*/
    else  if (StartCodeAddress_p < UnderParsingBuffer_p->TransferDataMaxEndAddress_p)
            /*if (StartCodeAddress_p >= BufferValidDataEndAddress_p)*/
    {
/*        TraceBuffer(("3--SC 0x%x add=0x%x \r\n", StartCodeFound_p->StartCodeId, StartCodeFound_p->StartCodeAddress_p));*/
        DiscardStartCode = TRUE;
        StartCodeDetectorHasToBelaunched = FALSE;

        /* TraceBuffer(("         SC 0x%x 3rd case add=0x%x max=0x%x\r\n", StartCodeFound_p->StartCodeId, StartCodeAddress_p, UnderParsingBuffer_p->ValidDataBufferEndAddress_p)); */

        if (UnderParsingBuffer_p->NbPicturesInside < VIDTRICK_NB_PIC_MIN_TO_BE_FOUND_BY_BUFFER)
        {
#ifdef TRICKMOD_DEBUG
            TrickModeError();
            TraceBuffer(("Buffer fully parsed but not enough pictures found !\n\r"));
#endif /* TRICKMOD_DEBUG */

            /* the start code detector is lost or the transfered data have not been successfully flushed.*/
            /* re-flush the bit buffer */
            UnderParsingBuffer_p->NbPicturesInside = 0;
            /* VIDTRICK_Data_p->LinearBitBuffers.BufferUnderTransfer_p = UnderParsingBuffer_p; */
            VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p = NULL;
            AskForLinearBitBufferFlush(TrickModeHandle, UnderParsingBuffer_p, VIDTRICK_FLUSH_WITH_NORMAL_OVERLAP);
            return;
        }

        if (StartCodeFound_p->StartCodeId == PICTURE_START_CODE)
        {
            /* If Start code found is PICTURE_START_CODE, it should mean that last picture found in this buffer is not */
            /* available because the whole picture should not be in the buffer */
            UnderParsingBuffer_p->NbPicturesInside --;
            /* Disable the bit rate evaluation */
            VIDDEC_DisableBitRateEvaluating(VIDTRICK_Data_p->DecodeHandle);
        }
        else if (StartCodeFound_p->StartCodeId == FIRST_SLICE_START_CODE)
        {
            /* just found FIRST_SLICE_START_CODE will make have Next NewPictureFoundInBitBuffer function called */
            /* but next picture can not be computed P */
            VIDTRICK_Data_p->Flags.FirstSliceStartCodeFoundInUnvalidArea = TRUE;
        }

        /* New buffer is fully parsed : post a command to trickmod task in order to have this buffer */
        /* analysed by trickmod task */
        VIDTRICK_Data_p->BitBufferFullyParsedToBeProcessedInTask_p = UnderParsingBuffer_p;
        /* Compute the injection duration according to the number of pictures inside the bitbuffer */
        UnderParsingBuffer_p->InjectionDuration =
            ((UnderParsingBuffer_p->TransferedDataSize / UnderParsingBuffer_p->OneFieldAverageSize) * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) / (((VIDTRICK_Data_p->MainParams.FrameRateValue*2) + 999) / 1000);
#ifdef TRACE_UART_MAIN
        TraceBuffer(("\r\nCurrent injection : Fields %lu ", UnderParsingBuffer_p->NbFieldsInside));
        TraceBuffer(("correspond to a duration of %lu\r\n",
            (UnderParsingBuffer_p->NbFieldsInside * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) / (((VIDTRICK_Data_p->MainParams.FrameRateValue*2) + 999) / 1000)));
        TraceBuffer(("Current injection : Field average size %lu Totalsize %lu ",
            UnderParsingBuffer_p->OneFieldAverageSize,
            UnderParsingBuffer_p->TransferedDataSize
            ));
        TraceBuffer(("correspond to a duration of %lu\r\n", UnderParsingBuffer_p->InjectionDuration));
#endif
    }

    /*---------------------------------------------------------------------------------------------------------*/
    /* fourth case : Start code has been found outside the buffer area. */
    /*---------------------------------------------------------------------------------------------------------*/
    else if (   (StartCodeAddress_p >= UnderParsingBuffer_p->TransferDataMaxEndAddress_p)
             || (StartCodeAddress_p < UnderParsingBuffer_p->WholeBufferStartAddress_p))
    {
/*        TraceBuffer(("4--SC 0x%x add=0x%x \r\n", StartCodeFound_p->StartCodeId, StartCodeFound_p->StartCodeAddress_p));*/
        /* TraceBuffer(("         SC 0x%x 4th case add=0x%x \r\n", StartCodeFound_p->StartCodeId, StartCodeAddress_p)); */

#ifdef TRICKMOD_DEBUG
        TrickModeError();
        if (StartCodeAddress_p >= UnderParsingBuffer_p->TransferDataMaxEndAddress_p)
        {
            TraceBuffer(("Start Code over a valid range\n\r")) ;
        }
        else if (StartCodeAddress_p < UnderParsingBuffer_p->WholeBufferStartAddress_p)
        {
            TraceBuffer(("Start Code under a valid range\n\r")) ;
        }
#endif /* TRICKMOD_DEBUG */

        /* Try to recover this error by restarting the parsing from the last startcode which was not bad. */
        /* NewStartCodeSearchIsRequired(TrickModeHandle, VIDDEC_START_CODE_SEARCH_MODE_FROM_ADDRESS, TRUE, UnderParsingBuffer_p->LastStartCodeAddressFound_p); */

        /* the start code detector is lost : reinitialze the parsing of the buffer */
        UnderParsingBuffer_p->NbPicturesInside = 0;
        /*  NewStartCodeSearchIsRequired(TrickModeHandle, VIDDEC_START_CODE_SEARCH_MODE_FROM_ADDRESS, TRUE, UnderParsingBuffer_p->WholeBufferStartAddress_p);*/
        /* Lock mutex protecting Trick mode private params */
        if (StartCodeFound_p->StartCodeId != USER_DATA_START_CODE)
        {
            STOS_MutexLock(VIDTRICK_Data_p->TrickModeParamsMutex_p);

            if (VIDTRICK_Data_p->Flags.BackwardToForwardInitializing)
            {
              /* Speed is switching from back to forward : Event should be ignored */
              STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
              return;
            }

            if (!(VIDTRICK_Data_p->NewStartCodeSearch.IsRequiredInTask))
            {
                VIDTRICK_Data_p->NewStartCodeSearch.IsRequiredInTask = TRUE;
#ifdef TRICKMOD_DEBUG
                if (VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p == NULL)
                {
                    TrickModeError();
                }
#endif

                VIDTRICK_Data_p->NewStartCodeSearch.SearchMode = VIDDEC_START_CODE_SEARCH_MODE_FROM_ADDRESS;
                VIDTRICK_Data_p->NewStartCodeSearch.FirstSliceDetect = TRUE;
                VIDTRICK_Data_p->NewStartCodeSearch.SearchAddress_p = UnderParsingBuffer_p->WholeBufferStartAddress_p;
            }
#ifdef TRICKMOD_DEBUG
            else
            {
                TrickModeError();
            }
#endif /* TRICKMOD_DEBUG */
            /* TraceBuffer(("-4>%d", time_now()));*/
            /* Unlock mutex protecting Trick mode private params */
            STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p);
        }

        ErrorCode = VIDDEC_DiscardStartCode(VIDTRICK_Data_p->DecodeHandle, TRUE);
#ifdef TRICKMOD_DEBUG
        if (ErrorCode != ST_NO_ERROR)
        {
            TrickModeError();
            /* TraceBuffer(("Discard Start Code failed\n\r")) ;*/
        }
#endif /* TRICKMOD_DEBUG */
    }

    /* Unlock */
    /* STOS_MutexRelease(VIDTRICK_Data_p->TrickModeParamsMutex_p); */
    /* Trick Mode Task has to be waked up so that Fields have to be skipped if necessary. */
    semaphore_signal(VIDTRICK_Data_p->WakeTrickModeTaskUp_p);
} /* end of StartCodeFoundInBitBuffer() function */


/*******************************************************************************
Name        : StoreFirstPTSFound
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : TRUE or FALSE
*******************************************************************************/
/* static void StoreFirstPTSFound(VIDTRICK_LinearBitBuffer_t * const CurrentBuffer_p, const VIDDEC_PictureInfoFromDecode_t * PictureInfoFromDecode_p) */
/* { */
    /* Is There a real PTS attached to this picture ? */
    /* if (!(PictureInfoFromDecode_p->PictureInfos.VideoParams.PTS.Interpolated)) */ /* case yes */
    /* { */
        /* Is no PTS already found in this buffer */
        /* if (!CurrentBuffer_p->FirstPTSFound.HasBeenFound) */
        /* { */
            /* Store PTS charcateristics */
            /* CurrentBuffer_p->FirstPTSFound.HasBeenFound = TRUE; */
            /* CurrentBuffer_p->FirstPTSFound.PTSFoundPictureNumber = CurrentBuffer_p->NbPicturesInside; */
/* #ifdef TRICKMOD_DEBUG */
            /* CurrentBuffer_p->FirstPTSFound.pTemporalReference = PictureInfoFromDecode_p->pTemporalReference; */
/* #endif */
        /* } */
    /* } */
/* } */




/*******************************************************************************
Name        : StopSmoothBackward
Description : Stops smooth back and desallocates linear bit buffers
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static ST_ErrorCode_t StopSmoothBackward(const VIDTRICK_Handle_t TrickModeHandle, const BOOL FreeNotDisplayedPictures)
{
    VIDTRICK_Data_t  * VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    ST_ErrorCode_t     ErrorCode;

    /* Stop Decode Now ! */
    ErrorCode = VIDDEC_Stop(VIDTRICK_Data_p->DecodeHandle, STVID_STOP_NOW, FALSE);
#ifdef TRICKMOD_DEBUG
    if (ErrorCode != ST_NO_ERROR)
    {
        TrickModeError();
    }
#endif /* TRICKMOD_DEBUG */

    /* Trickmode is now stopped */
    VIDTRICK_Data_p->Stop.TrickmodIsStopped = TRUE;
    /* Prepare smooth Backward for next restart */
    VIDTRICK_Data_p->Flags.SmoothBackwardIsInitializing = TRUE;
    VIDTRICK_Data_p->Flags.BackwardToForwardInitializing = FALSE;
    VIDTRICK_Data_p->ErrorRecovery.ReInitSmoothIsPending = FALSE;

    /* Init Smooth backward structure. */
    SmoothBackwardInitStructure(TrickModeHandle);

    /* The not displayed pictures shouldn't be removed from the display queue, if the stop required is not a stop_now */
    if(FreeNotDisplayedPictures)
    {
        /* Reset reference pictures and free the not displayed picture buffers */
        ResetReferencesAndFreeNotDisplayedPictures(TrickModeHandle);
    }

    return(ErrorCode);
} /* end of StopSmoothBackward() function */

/*******************************************************************************
Name        : UnlinkBufferToBeFLushedPictures
Description : This function resets all dependancies of current linear bit buffer with its associated to be flushed buffer
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void UnlinkBufferToBeFLushedPictures(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const BufferToBeFilled_p)
{
    VIDTRICK_PictureInfo_t  * FirstIPicture_p = NULL;
    VIDTRICK_PictureInfo_t  * FirstReferencePicture_p = NULL;

    /* Find Last Displayable picture of Associated buffer now */
    FirstIPicture_p = BufferToBeFilled_p->LastDisplayablePicture_p;
#ifdef TRICKMOD_DEBUG
    if (FirstIPicture_p == NULL)
    {
        TrickModeError();
    }
#endif

    /* check discontinuity between both linear buffers */
    if (FirstIPicture_p->FirstDisplayablePictureAfterJump_p != NULL)
    {
        FirstIPicture_p->FirstDisplayablePictureAfterJump_p = NULL;
        /* FirstIPicture_p->PreviousInStreamPicture_p = NULL; */
        /* FirstIPicture_p->BackwardOrLastBeforeIReferencePicture_p->ForwardOrNextReferencePicture_p = NULL; */
        /* FirstIPicture_p->BackwardOrLastBeforeIReferencePicture_p = NULL; */
    }
    else
    {
        FindFirstReferencePictureOfLinearBitBuffer(TrickModeHandle, BufferToBeFilled_p, &FirstReferencePicture_p);
        if (FirstReferencePicture_p->BackwardOrLastBeforeIReferencePicture_p != NULL)
        {
            FirstReferencePicture_p->BackwardOrLastBeforeIReferencePicture_p->ForwardOrNextReferencePicture_p = NULL;
            FirstReferencePicture_p->BackwardOrLastBeforeIReferencePicture_p = NULL;
        }
#ifdef TRICKMOD_DEBUG
        else
        {
            /* no dicontinuity : First Reference of the buffer must have a Backward reference */
            TrickModeError();
        }
#endif /* TRICKMOD_DEBUG */
    }
}

/*******************************************************************************
Name        : UnlockPicture
Description : either sets the given picture status to UNLOCKED (first case),  or sets FirstUnlockedChangeStatusHasBeenRequired
              to TRUE (second case) if two conditions are necessary to set the given picture status to UNLOCKED
              - UnlockDefinitively is TRUE : (first case)
                -  when a new picture to be decoded over an other. This other one has to be
                  UNLOCKED. (may has been already done !)
                - if reference picture out display : its forward prediction is from now totally unuseful. set it
                  to UNLOCKED
              - UnlockDefinitively is FALSE : (first case or second case, depending on FirstUnlockedChangeStatusHasBeenRequired)
                - picture out if display
                - just decoded B picture : its forward prediction picture may be from now unuseful to decode any other Bs
Parameters  : PictureInfo_p : given picture
              UnlockDefinitively : if TRUE the given picture status has to be set to UNLOCKED whatever the
              FirstUnlockedChangeStatusHasBeenRequired value. if FALSE care of this flag as described above.
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void UnlockPicture(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t * const PictureInfo_p, const BOOL UnlockDefinitively, const BOOL HasBeenDisplayed, const BOOL UnusefulToDecAnyB)
{
#ifdef TRICKMOD_DEBUG
    VIDTRICK_Data_t         *  VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
#endif

#ifdef TRACE_UART_DEBUG
    TraceBuffer(("UnlockPicture 0x%x\n\r", PictureInfo_p));
#endif

    if (PictureInfo_p == NULL)
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("Trying to unlock an NULL VIDTRICK_PictureInfo_t * !\n\r"));
#endif /* TRICKMOD_DEBUG */
        return;
    }

    /* TraceBuffer(("Unlock : M=%d E=%d ind=%d Buf=0x%x defve=%d\r\n", */
            /* PictureInfo_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame, */
            /* PictureInfo_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference, */
            /* PictureInfo_p->IndiceInBuffer, */
            /* PictureInfo_p->LinearBitBufferStoredInfo_p, */
            /* UnlockDefinitively)); */

    if (IsPictureSecondField(PictureInfo_p))
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("Trying to unlock Second Field Picture !\n\r"));
#endif /* TRICKMOD_DEBUG */
        return;
    }

#ifdef TRICKMOD_DEBUG
    if (   (PictureInfo_p->LinearBitBufferStoredInfo_p == VIDTRICK_Data_p->LinearBitBuffers.BufferUnderTransfer_p)
        || (PictureInfo_p->LinearBitBufferStoredInfo_p == VIDTRICK_Data_p->LinearBitBuffers.BufferUnderParsing_p))
    {
        TrickModeError();
        TraceBuffer(("Trying to unlock a picture belonging to a BitBuffer under transfer or parsing!\n\r"));
    }
#endif  /* TRICKMOD_DEBUG */

    if (! UnlockDefinitively)
    {
        if (HasBeenDisplayed)
        {
            PictureInfo_p->DisplayUnlockHasBeenRequired = TRUE;
        }
        if (UnusefulToDecAnyB)
        {
            PictureInfo_p->AnyBToDecodeUnlockHasBeenRequired = TRUE;
        }

        /* if not both unlock have been required, do not unlock the picture */
        if (   (! PictureInfo_p->DisplayUnlockHasBeenRequired)
            || (! PictureInfo_p->AnyBToDecodeUnlockHasBeenRequired) )
        {
            /* This picture has not been displayed yet or is still useful to decode a B picture so we shall keep it */
            return;
        }
    }

    if (PictureInfo_p->IsInDisplayQueue)
    {
        /* This picture is currently in Display Queue: We keep it until "PictureOutOfDisplay" is notified */
        return;
    }

    /* Unlock Definitively */
    ReleasePictureInfoElement(TrickModeHandle, PictureInfo_p);

} /* end of UnlockPicture() function */

/*******************************************************************************
Name        : ResetPictureInfoStatus
Description : Reset status fields of a PictureInfo element.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void ResetPictureInfoStatus(const VIDTRICK_Handle_t     TrickModeHandle,
                                    VIDTRICK_PictureInfo_t *    PictureInfo_p)
{
    UNUSED_PARAMETER(TrickModeHandle);

    PictureInfo_p->IsInDisplayQueue = FALSE;
    PictureInfo_p->IsGoneOutOfDisplay = FALSE;
    PictureInfo_p->HasBeenInsertedInDisplayQueue = FALSE;
    PictureInfo_p->IsDecoded = FALSE;
    PictureInfo_p->DisplayUnlockHasBeenRequired = FALSE;
    PictureInfo_p->AnyBToDecodeUnlockHasBeenRequired = FALSE;
}

/*******************************************************************************
Name        : ReleasePictureInfoElement
Description : This function reset the content of a Picture Info Element and release
              the associated Picture Buffers.
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void ReleasePictureInfoElement(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t * const PictureInfoElemToRelease_p)
{
    VIDTRICK_Data_t         *  VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    VIDTRICK_PictureInfo_t  *  SecondFieldPictureInfo_p = NULL;

#ifdef TRACE_UART_DEBUG
    TraceBuffer(("Release 0x%x\n\r", PictureInfoElemToRelease_p));
#endif

    ResetPictureInfoStatus(TrickModeHandle, PictureInfoElemToRelease_p);

    /* if picture has a second field : its second field has to be unlocked too. */
    if (HasPictureASecondField(PictureInfoElemToRelease_p))
    {
        SecondFieldPictureInfo_p = PictureInfoElemToRelease_p->SecondFieldPicture_p;

        if (SecondFieldPictureInfo_p != NULL)
        {
            /* reset second field status */
            ResetPictureInfoStatus(TrickModeHandle, SecondFieldPictureInfo_p);

            if (SecondFieldPictureInfo_p->PictureBuffer_p != NULL)
            {
                /* unlink picture buffer */
                SecondFieldPictureInfo_p->PictureBuffer_p->SmoothBackwardPictureInfo_p = NULL;
#ifdef TRACE_UART_DEBUG
                TraceBuffer((" Release 2nd Field PB 0x%x\n\r", SecondFieldPictureInfo_p->PictureBuffer_p));
#endif
                if (SecondFieldPictureInfo_p->PictureBuffer_p->AssociatedDecimatedPicture_p != NULL)
                {
                    RELEASE_PICTURE_BUFFER(VIDTRICK_Data_p->BufferManagerHandle, SecondFieldPictureInfo_p->PictureBuffer_p->AssociatedDecimatedPicture_p, VIDCOM_VIDTRICK_MODULE_BASE);
                    /* It is not allowed to set AssociatedDecimatedPicture_p to NULL because the link between Main and Decimated Picture Buffers is managed by VID_BUFF */
                }
                RELEASE_PICTURE_BUFFER(VIDTRICK_Data_p->BufferManagerHandle, SecondFieldPictureInfo_p->PictureBuffer_p, VIDCOM_VIDTRICK_MODULE_BASE);
                SecondFieldPictureInfo_p->PictureBuffer_p = NULL;
            }
        }
        else
        {
#ifdef TRICKMOD_DEBUG
            TrickModeError();
#endif /* TRICKMOD_DEBUG */
        }
    }

    if (PictureInfoElemToRelease_p->PictureBuffer_p != NULL)
    {
        /* unlink picture buffer */
        PictureInfoElemToRelease_p->PictureBuffer_p->SmoothBackwardPictureInfo_p = NULL;
#ifdef TRACE_UART_DEBUG
        TraceBuffer((" Release PB 0x%x\n\r", PictureInfoElemToRelease_p->PictureBuffer_p));
#endif
        if (PictureInfoElemToRelease_p->PictureBuffer_p->AssociatedDecimatedPicture_p != NULL)
        {
            RELEASE_PICTURE_BUFFER(VIDTRICK_Data_p->BufferManagerHandle, PictureInfoElemToRelease_p->PictureBuffer_p->AssociatedDecimatedPicture_p, VIDCOM_VIDTRICK_MODULE_BASE);
            /* It is not allowed to set AssociatedDecimatedPicture_p to NULL because the link between Main and Decimated Picture Buffers is managed by VID_BUFF */
        }
        RELEASE_PICTURE_BUFFER(VIDTRICK_Data_p->BufferManagerHandle, PictureInfoElemToRelease_p->PictureBuffer_p, VIDCOM_VIDTRICK_MODULE_BASE);
        PictureInfoElemToRelease_p->PictureBuffer_p = NULL;
    }

    /* This Picture Info element is clean now!*/
}

/*******************************************************************************
Name        : UpdateFirstPicturesInDisplayQueue
Description : removes the just displayed picture from the array of the pictures to be inserted as soon as possible
              in display queue
Parameters  : must be called by pictureOutOfDisplay functon
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void UpdateFirstPicturesInDisplayQueue(const VIDTRICK_Handle_t TrickModeHandle)
{
    VIDTRICK_Data_t * VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    U32  i;

    if (VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ == 0)
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("No picture in Display Queue !\n\r"));
#endif
        VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0] = NULL;
        return;
    }

    /* MAJ of first Picture to insert in display queue */
    for (i = 0  ; i < VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ - 1; i++)
    {
        VIDTRICK_Data_p->LastPictureInDisplayQueue_p[i] = VIDTRICK_Data_p->LastPictureInDisplayQueue_p[i+1];
    }
    VIDTRICK_Data_p->LastPictureInDisplayQueue_p[VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ - 1] = NULL;
    VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ--;

#ifdef DEBUG_DISPLAY_QUEUE
      DbgCheckPicturesInDisplayQueue(TrickModeHandle);
#endif

#ifdef TRACE_UART_DEBUG
    TraceBuffer(("Elem 0 in DQ: 0x%x\r\n", VIDTRICK_Data_p->LastPictureInDisplayQueue_p[0] ));
#endif

} /* end of UpdateFirstPicturesInDisplayQueue() function */


/*******************************************************************************
Name        : UpdateLastPictureInDisplayQueueList
Description : Completes the array of the pictures which have to be inserted in display queue as soon as possible
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void UpdateLastPictureInDisplayQueueList(const VIDTRICK_Handle_t TrickModeHandle)
{
    VIDTRICK_Data_t          * VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    VIDTRICK_PictureInfo_t   * JustFoundPictureInfo_p = NULL;
    VIDTRICK_PictureInfo_t   * LastPictureInTableNextReferencePicture_p = NULL;
    VIDTRICK_PictureInfo_t   * LastFoundPictureInfo_p = NULL;
    VIDTRICK_PictureInfo_t   * LastPictureInTable_p = NULL;
    VIDTRICK_PictureInfo_t   * TmpPicture_p = NULL;
    BOOL  OnlyOnePictureOfBufferToBeDisplayedCase = FALSE;
    BOOL  JumpBetweenBothLBBFHasBeenDetected = FALSE;
    BOOL  FoundPictureIsOK;
    BOOL  UpdateTable = FALSE;
    BOOL  EndOfPL = FALSE;
    U32   NbFieldsSurelySkipped = 0;
    U32   NbFieldsSkipped = 0;
    U32   TheoricalNbOfFieldsToSkip1 = 0;
    U32   TheoricalNbOfFieldsToSkip2 = 0;
    U32   TheoricalNbOfFieldsToSkip = 0;
    BOOL  Authorized=FALSE;

    if (VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ == 0)
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("No picture ToBeInsertedInDQ !\n\r"));
#endif
        return;
    }

    /* init LastPictureInTable_p */
    LastPictureInTable_p = VIDTRICK_Data_p->LastPictureInDisplayQueue_p[VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ - 1];

    /* Setup the last item in the list of the pictures to deal with. */
    FoundPictureIsOK = SearchForNormalRevSpeedPreviousPicToDisplay(TrickModeHandle, LastPictureInTable_p, &JustFoundPictureInfo_p);


    if (FoundPictureIsOK)
    {
        Authorized = IsPictureAuthorizedToBeDecoded(TrickModeHandle, JustFoundPictureInfo_p);
        if (!Authorized)
        {
                VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat -=
                    ((  JustFoundPictureInfo_p->PictureInfoFromDecode.AdditionalStreamInfo.TopFieldCounter
                      +JustFoundPictureInfo_p->PictureInfoFromDecode.AdditionalStreamInfo.BottomFieldCounter
                      +JustFoundPictureInfo_p->PictureInfoFromDecode.AdditionalStreamInfo.RepeatFieldCounter)
                    * (JustFoundPictureInfo_p->PictureInfoFromDecode.AdditionalStreamInfo.RepeatProgressiveCounter + 1));
        }
    }

   while (   (FoundPictureIsOK)
           && ((! CheckIfSecondFieldIsAvailable(JustFoundPictureInfo_p))||(!Authorized)))
    {
        /* Second field is not available, search next previous picture */
        TmpPicture_p = JustFoundPictureInfo_p;
        FoundPictureIsOK = SearchForNormalRevSpeedPreviousPicToDisplay( TrickModeHandle,
                                                                        TmpPicture_p,
                                                                        &JustFoundPictureInfo_p);
        if (FoundPictureIsOK)
        {
            Authorized = IsPictureAuthorizedToBeDecoded(TrickModeHandle, JustFoundPictureInfo_p);
            if (!Authorized)
            {
                VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat-=
                    ((  JustFoundPictureInfo_p->PictureInfoFromDecode.AdditionalStreamInfo.TopFieldCounter
                      +JustFoundPictureInfo_p->PictureInfoFromDecode.AdditionalStreamInfo.BottomFieldCounter
                      +JustFoundPictureInfo_p->PictureInfoFromDecode.AdditionalStreamInfo.RepeatFieldCounter)
                    * (JustFoundPictureInfo_p->PictureInfoFromDecode.AdditionalStreamInfo.RepeatProgressiveCounter + 1));
            }
        }

    }

    if (! FoundPictureIsOK)
    {
        /* We have not find the previous picture of the last in the list of pictures. */
        /* It is not the normal behaviour ...                                         */
        return;
    }

    /* If last is a second field picture, table must be updated with its first field */
    /* FirstDisplayablePictureAfterJump_p is a 2nd or a frame picture : the whole picture (2fields or 1 frame )*/
    /* is in the bit buffer */
    if (   (IsPictureSecondField(LastPictureInTable_p))
        || (LastPictureInTable_p->FirstDisplayablePictureAfterJump_p == JustFoundPictureInfo_p)
        || (JustFoundPictureInfo_p == JustFoundPictureInfo_p->LinearBitBufferStoredInfo_p->LastDisplayablePicture_p))
    {
#ifdef TRICKMOD_DEBUG
        if (JustFoundPictureInfo_p == NULL )
        {
            TrickModeError();
        }

        if (   (HasPictureASecondField(JustFoundPictureInfo_p))
            && (JustFoundPictureInfo_p->SecondFieldPicture_p == NULL))
        {
            /* Should only insert field pictures with their second field */
            TrickModeError();
        }
#endif /* TRICKMOD_DEBUG */

        VIDTRICK_Data_p->LastPictureInDisplayQueue_p[VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ] = JustFoundPictureInfo_p;
        VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ++;
#ifdef DEBUG_DISPLAY_QUEUE
       DbgCheckPicturesInDisplayQueue(TrickModeHandle);
#endif
        return;
    }

    /* two reasons to skip pictures */
    /* 1- NbFieldsIfPosToSkipIfNegToRepeat > VIDTRICK_LEVEL_SKIP_BACKWARD_PICTURE  */
    /* 2- LastBackwardFieldsSkippedNb > VIDTRICK_FIELDS_JUMP_NB_DECREASE ==> at least */
    /*    LastBackwardFieldsSkippedNb - VIDTRICK_FIELDS_JUMP_NB_DECREASE fields have to be skipped to avoid */
    /*    inconsistency between 2 foolowing jumps (12/2, for example) */
    /* The number of fields to skip has to be the max between case 1 and 2 */

    /* Compute case 1 */
    if (VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat > VIDTRICK_LEVEL_SKIP_BACKWARD_PICTURE)
    {
        /* TheoricalNbOfFieldsToSkip1 takes the value NbFieldsIfPosToSkipIfNegToRepeat - the min of fields that the display */
        /* is supposed to skip by itself */
        TheoricalNbOfFieldsToSkip1 = VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat - VIDTRICK_LEVEL_SKIP_BACKWARD_PICTURE;
    }

#ifdef OLD_BACKWARD_FLUENT_DISPLAY_PROCESS
    /* Compute case 2 */
    if (VIDTRICK_Data_p->BackwardFluidity.LastBackwardFieldsSkippedNb > VIDTRICK_FIELDS_JUMP_NB_DECREASE)
    {
        TheoricalNbOfFieldsToSkip2 = VIDTRICK_Data_p->BackwardFluidity.LastBackwardFieldsSkippedNb - VIDTRICK_FIELDS_JUMP_NB_DECREASE;
    }
#endif /* OLD_BACKWARD_FLUENT_DISPLAY_PROCESS */

    /* compute the final Theorical Nb Of Fields To Skip */
    TheoricalNbOfFieldsToSkip = Max(TheoricalNbOfFieldsToSkip1, TheoricalNbOfFieldsToSkip2);

    /* if no fields to skip, do not process any skip, just update the table with the previous picture */
    if ((TheoricalNbOfFieldsToSkip == 0) ||(VIDTRICK_Data_p->DecodedPictures==STVID_DECODED_PICTURES_I)||(VIDTRICK_Data_p->DecodedPictures==STVID_DECODED_PICTURES_IP))
    {
        UpdateTable = TRUE;
    }
    else
    {
        /* TheoricalNbOfFieldsToSkip > 0 : test several conditions in order to require the associated buffer */
        /* to be flushed, which could avoid described case */
        /* Last Reference Of Current Buffer on display + jump to do in order to skip required nb of fields */
        /* over the last displayable picture of the current buffer. */
        if (VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ == 1)
        {
            LastPictureInTableNextReferencePicture_p = LastPictureInTable_p->ForwardOrNextReferencePicture_p;
            if (   (LastPictureInTableNextReferencePicture_p != NULL)
                && (IsPictureSecondField(LastPictureInTableNextReferencePicture_p)))
            {
                LastPictureInTableNextReferencePicture_p = LastPictureInTableNextReferencePicture_p->ForwardOrNextReferencePicture_p;
            }
            if (LastPictureInTableNextReferencePicture_p != NULL)
            {
                if (LastPictureInTable_p->LinearBitBufferStoredInfo_p != LastPictureInTableNextReferencePicture_p->LinearBitBufferStoredInfo_p)
                {
                    /* Affect the buffer spacing according to the speed. Sets Discontinutiy flag if necessary. */
                    AffectTheBufferSpacing(TrickModeHandle, VIDTRICK_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p);

                    /* Unlink all dependencies pictures of buffer to be flushed. */
                    UnlinkBufferToBeFLushedPictures(TrickModeHandle, VIDTRICK_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p);

                    /* ask for linear bit buffer to be flushed. */
                    AskForLinearBitBufferFlush(TrickModeHandle, VIDTRICK_Data_p->LinearBitBuffers.NextBufferToBeTransfered_p, VIDTRICK_FLUSH_WITH_NORMAL_OVERLAP);
                }
            }
        }

        /* if many pictures inside a same bit buffer : display several in order to give a good backward fluidity impression */
        /* TheoricalNbOfFieldsToSkip should be under a max level */
        if (TheoricalNbOfFieldsToSkip > VIDTRICK_Data_p->BackwardFluidity.MaxJumpeableFields)
        {
            TheoricalNbOfFieldsToSkip = VIDTRICK_Data_p->BackwardFluidity.MaxJumpeableFields;
        }

        LastFoundPictureInfo_p = JustFoundPictureInfo_p;
        /* PreviousPictureToDisplay_p = JustFoundPictureInfo_p; */
        while (! EndOfPL)
        {
            FoundPictureIsOK = SearchForNormalRevSpeedPreviousPicToDisplay(TrickModeHandle, LastFoundPictureInfo_p, &JustFoundPictureInfo_p);
            if (! FoundPictureIsOK)
            {
                EndOfPL = TRUE;
            }
            else
            {
                Authorized = IsPictureAuthorizedToBeDecoded(TrickModeHandle, JustFoundPictureInfo_p);
                if (! CheckIfSecondFieldIsAvailable(JustFoundPictureInfo_p))
                {
                    /* Second field is not available, search next previous picture */
                    LastFoundPictureInfo_p = JustFoundPictureInfo_p;
                }
                else
                {
#ifdef TRICKMOD_DEBUG
                    /* debug tests : JustFoundPictureInfo_p can not be null if FoundPictureIsOK is TRUE */
                    /* if LastFoundPictureInfo_p is a second field, JustFoundPictureInfo_p must be the first field and must be the previous in the stream */
                    if (   (IsPictureSecondField(LastFoundPictureInfo_p))
                        && (JustFoundPictureInfo_p != LastFoundPictureInfo_p->PreviousInStreamPicture_p))
                    {
                        TrickModeError();
                    }
#endif /* TRICKMOD_DEBUG */
                    NbFieldsSkipped += JustFoundPictureInfo_p->PictureInfoFromDecode.NbFieldsToDisplay;
                    /* choosing B pictures is some pictures are jumped, not optimising smmoth backward management : it is better */
                    /* to jump to a reference, to avoid to decode too many decodes. */
                    if (    PictureMpegTypeIsReference(JustFoundPictureInfo_p)
                        && (! IsPictureSecondField(JustFoundPictureInfo_p))&&(Authorized))
                    {
                        /* PreviousPictureToDisplay_p = JustFoundPictureInfo_p; */
                        NbFieldsSurelySkipped = NbFieldsSkipped;
                        if (NbFieldsSurelySkipped >= TheoricalNbOfFieldsToSkip)
                        {
                            UpdateTable = TRUE;
                            EndOfPL = TRUE;
                        }

                        /* if jump between Linear bit buffers detected : Update with First Displayable picture : This picture, */
                        /* because of HDD jump should be enough away from PreviousPictureToDisplay_p */
                        if (LastFoundPictureInfo_p->FirstDisplayablePictureAfterJump_p == JustFoundPictureInfo_p)
                        {
                            JumpBetweenBothLBBFHasBeenDetected = TRUE;
                            UpdateTable = TRUE;
                            EndOfPL = TRUE;
                        }

                        /* if last displayable picture of a buffer found. give it to display. */
                        if (   (JustFoundPictureInfo_p == JustFoundPictureInfo_p->LinearBitBufferStoredInfo_p->LastDisplayablePicture_p)
                            && (JustFoundPictureInfo_p->LinearBitBufferStoredInfo_p != LastPictureInTable_p->LinearBitBufferStoredInfo_p))
                        {
                            OnlyOnePictureOfBufferToBeDisplayedCase = TRUE;
                            UpdateTable = TRUE;
                            EndOfPL = TRUE;
                        }
                    }
                    /* update LastFoundPictureInfo_p */
                    LastFoundPictureInfo_p = JustFoundPictureInfo_p;
                }
            }
        }
    }

    /* NbFieldsSurelySkipped can not be too small regarding last computed value */
    /* necessary to avoid big and too small consecutive jumps */
    if (UpdateTable)
    {
#ifdef TRICKMOD_DEBUG
        if (   (HasPictureASecondField(JustFoundPictureInfo_p))
            && (JustFoundPictureInfo_p->SecondFieldPicture_p == NULL))
        {
            TrickModeError();
        }
#endif

        VIDTRICK_Data_p->LastPictureInDisplayQueue_p[ VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ] = JustFoundPictureInfo_p;
        VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ++;
#ifdef DEBUG_DISPLAY_QUEUE
       DbgCheckPicturesInDisplayQueue(TrickModeHandle);
#endif

        if (NbFieldsSurelySkipped > 0)
        {
            VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat -= NbFieldsSurelySkipped;

#ifdef TRACE_UART_DISPLAY_SKIP
            TraceBuffer(("\n\rSkipped=%d \r\n", NbFieldsSurelySkipped));
#endif
        }

#ifdef TRACE_UART_DISPLAY_SKIP
        TraceBuffer (("UpdDQ: %c %d  %d  ind=%d SkipRepeat=%d\r\n",
            MPEGFrame2Char(JustFoundPictureInfo_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame),
            JustFoundPictureInfo_p->PictureInfoFromDecode.pTemporalReference,
            JustFoundPictureInfo_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference,
            JustFoundPictureInfo_p->IndiceInBuffer,
            VIDTRICK_Data_p->MainParams.NbFieldsIfPosToSkipIfNegToRepeat));
#endif

        /* JustFoundPictureInfo_p->LinearBitBufferStoredInfo_p->OnePictureAtLeastChoosenToBeDisplayed = TRUE; */
        /* if JumpBetweenBothLBBFHasBeenDetected, NbFieldsSurelySkipped has no meaning because there is an HDD jump between */
        /* LastPictureInTable_p and JustFoundPictureInfo_p */
        if (   (!JumpBetweenBothLBBFHasBeenDetected)
            && (!OnlyOnePictureOfBufferToBeDisplayedCase))
        {
            VIDTRICK_Data_p->BackwardFluidity.LastBackwardFieldsSkippedNb = NbFieldsSurelySkipped;
        }

#ifdef TRICKMOD_DEBUG
        if (   (JustFoundPictureInfo_p == NULL)
            || (!JustFoundPictureInfo_p->IsDecodable))
        {
            TrickModeError();
        }
        if (JustFoundPictureInfo_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference >
                    LastPictureInTable_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference)
        {
            TrickModeError();
        }
        if ((TheoricalNbOfFieldsToSkip > 0) && (PictureMpegTypeIsB(JustFoundPictureInfo_p)))
        {
            TrickModeError();
        }
#endif /* TRICKMOD_DEBUG */
    }
} /* end of UpdateLastPictureInDisplayQueueList() function */


/*******************************************************************************
Name        : PictureDecodeHasErrors
Description : A Picture has can not be decoded : first not dcoded picture in table of
              pictures to be inserted as soon as possible has to be removed.
              Thus, UpdateLastPictureInDisplayQueueList, computing with the acculumated
              late, may update table with an other picture.
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void PictureDecodeHasErrors(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_PictureInfo_t * const BadlyDecodedPicture_p)
{
    VIDTRICK_Data_t               * VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    VIDDEC_DecodePictureParams_t    DecodePictureParams;
    VIDTRICK_PictureInfo_t        * SecondFieldPictureToBeDecoded_p = NULL;

#ifdef TRICKMOD_DEBUG
        TraceBuffer(("PictureDecodeHasErrors!\n\r"));
#endif

    if (HasPictureASecondField(BadlyDecodedPicture_p))
    {
        /* The second field has to be decoded anyway !!!!! */

        /* Find Second field picture to decode */
        SecondFieldPictureToBeDecoded_p = BadlyDecodedPicture_p->SecondFieldPicture_p;
        if (SecondFieldPictureToBeDecoded_p == NULL)
        {
#ifdef TRICKMOD_DEBUG
            TrickModeError();
#endif /* TRICKMOD_DEBUG */
            return;
        }

        /* launch second field picture decode */
        DecodePictureParams.DataInputAddress_p = (void *)((U32)SecondFieldPictureToBeDecoded_p->AssociatedStartCodeAddress_p);
        DecodePictureParams.OutputPicture_p = BadlyDecodedPicture_p->PictureBuffer_p->FrameBuffer_p->NothingOrSecondFieldPicture_p;
        DecodePictureParams.OutputDecimatedPicture_p = DecodePictureParams.OutputPicture_p->AssociatedDecimatedPicture_p;
        DecodePictureParams.StreamInfoForDecode_p = &(SecondFieldPictureToBeDecoded_p->PictureInfoFromDecode.StreamInfoForDecode);
        DecodePictureParams.PictureInfos_p = &(SecondFieldPictureToBeDecoded_p->PictureInfoFromDecode.PictureInfos);
        DecodePictureParams.PictureStreamInfo_p = &(SecondFieldPictureToBeDecoded_p->PictureInfoFromDecode.AdditionalStreamInfo);
        DecodePictureParams.pTemporalReference = SecondFieldPictureToBeDecoded_p->PictureInfoFromDecode.pTemporalReference;

        /* inits */
        SecondFieldPictureToBeDecoded_p->PictureBuffer_p = BadlyDecodedPicture_p->PictureBuffer_p->FrameBuffer_p->NothingOrSecondFieldPicture_p;
        VIDTRICK_Data_p->PictureUnderDecoding_p = SecondFieldPictureToBeDecoded_p;

#ifdef TRACE_UART_TOPBOT_ALTERNATING
        TraceBuffer(("\n\rW(%d) %d %c %s\r\n", __LINE__,
                       SecondFieldPictureToBeDecoded_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference,
                       MPEGFrame2Char(SecondFieldPictureToBeDecoded_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame),
                       PictureStruct2String(SecondFieldPictureToBeDecoded_p->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure)));
#endif

        vidtrick_FillPictureBuffer(DecodePictureParams.OutputPicture_p, SecondFieldPictureToBeDecoded_p);

        /* Decode The second field picture */
        VIDDEC_DecodePicture(VIDTRICK_Data_p->DecodeHandle, &DecodePictureParams);
        return;
    }

    /* Unlock buffer containig badly decoded picture */
    if (IsPictureSecondField(BadlyDecodedPicture_p))
    {
        UnlockPicture(TrickModeHandle, BadlyDecodedPicture_p->PreviousInStreamPicture_p, TRUE, FALSE, FALSE);
    }
    else
    {
        UnlockPicture(TrickModeHandle, BadlyDecodedPicture_p, TRUE, FALSE, FALSE);
    }

    /* error recovery */
    if (IsPictureSecondField(BadlyDecodedPicture_p))
    {
        VIDTRICK_Data_p->ErrorRecovery.SuccessiveDecodeSecondFieldErrorsNb ++;
    }
    else
    {
        VIDTRICK_Data_p->ErrorRecovery.SuccessiveDecodeFrameOrFirstFieldErrorsNb++;
    }

    if (   (VIDTRICK_Data_p->ErrorRecovery.SuccessiveDecodeFrameOrFirstFieldErrorsNb == VIDTRICK_MAX_UNSUCCESSFULLY_DECODE_TRY)
        || (VIDTRICK_Data_p->ErrorRecovery.SuccessiveDecodeSecondFieldErrorsNb == VIDTRICK_MAX_UNSUCCESSFULLY_DECODE_TRY))
    {
#ifdef TRICKMOD_DEBUG
        TraceBuffer (("Too many consecutive unsuccessfull decodes !!!\n\r"));
#endif
        /* re-init Smooth backward */
        BackwardModeInit(TrickModeHandle, TRUE);
    }
} /* end of PictureDecodeHasErrors() function */

/*******************************************************************************
Name        : AssociatePictureBuffers
Description : This function will associate a "Frame or 1st Field" picture buffer to a given picture info element.
              If no Picture Buffer is available it will try to find one to reuse.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t AssociatePictureBuffers(const VIDTRICK_Handle_t     TrickModeHandle,
                                              VIDTRICK_PictureInfo_t *    PictureInfo_p)
{
    VIDBUFF_GetSmoothBackwardUnusedPictureBufferParams_t    GetUnusedParams;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    ST_ErrorCode_t              ErrorMain = ST_NO_ERROR;
    ST_ErrorCode_t              ErrorSecond = ST_NO_ERROR;
    VIDTRICK_Data_t *           VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    VIDBUFF_PictureBuffer_t *   PrimaryPictureBuffer_p = NULL;
    VIDBUFF_PictureBuffer_t *   SecondaryPictureBuffer_p = NULL;
    VIDCOM_InternalProfile_t    Profile;
    BOOL                        IsDecimatedFrameBufferNeeded = FALSE;

#ifdef TRICKMOD_DEBUG
    if (PictureInfo_p->PictureBuffer_p != NULL)
    {

        TrickModeError();
        TraceBuffer(("PictureInfo 0x%x already have a pict buff (0x%x) \n\r", PictureInfo_p, PictureInfo_p->PictureBuffer_p));
    }
#endif

    /* Check if a decimated frame buffer is needed */
    VIDBUFF_GetMemoryProfile(VIDTRICK_Data_p->BufferManagerHandle, &Profile);
    if (Profile.ApplicableDecimationFactor != STVID_DECIMATION_FACTOR_NONE)
    {
        IsDecimatedFrameBufferNeeded = TRUE;
    }

    GetUnusedParams.PictureStructure = PictureInfo_p->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure;
    GetUnusedParams.TopFieldFirst = PictureInfo_p->PictureInfoFromDecode.PictureInfos.VideoParams.TopFieldFirst;
    GetUnusedParams.ExpectingSecondField = PictureInfo_p->PictureInfoFromDecode.AdditionalStreamInfo.ExpectingSecondField;
    GetUnusedParams.PictureWidth   = PictureInfo_p->PictureInfoFromDecode.PictureInfos.BitmapParams.Width;
    GetUnusedParams.PictureHeight  = PictureInfo_p->PictureInfoFromDecode.PictureInfos.BitmapParams.Height;

    /* Get Primary Picture Buffer */
    ErrorMain = VIDBUFF_GetAndTakeSmoothBackwardUnusedPictureBuffer(
                                    VIDTRICK_Data_p->BufferManagerHandle,
                                    &GetUnusedParams,
                                    &PrimaryPictureBuffer_p,
                                    VIDCOM_VIDTRICK_MODULE_BASE);

    if (ErrorMain == ST_NO_ERROR)
    {
#ifdef TRACE_UART_DEBUG
        TraceBuffer(("GetAndTakeSmooth 0x%x PB 0x%x\n\r", PictureInfo_p, PrimaryPictureBuffer_p));
#endif

        if (IsDecimatedFrameBufferNeeded)
        {
            /* Get Secondary Picture Buffer */
            ErrorSecond = VIDBUFF_GetAndTakeSmoothBackwardDecimatedUnusedPictureBuffer(
                                VIDTRICK_Data_p->BufferManagerHandle,
                                &GetUnusedParams,
                                &SecondaryPictureBuffer_p,
                                VIDCOM_VIDTRICK_MODULE_BASE);

            if (ErrorSecond == ST_NO_ERROR)
            {
#ifdef TRACE_UART_DEBUG
                TraceBuffer(("GetAndTakeSmooth Decim 0x%x PB 0x%x\n\r", PictureInfo_p, SecondaryPictureBuffer_p));
#endif
            }
            else
            {
                /* No Decimated Picture Buffer is available so we should release the Primary Picture Buffer */
#ifdef TRACE_UART_DEBUG
                TraceBuffer(("Untake 0x%x PB 0x%x\n\r", PictureInfo_p, PrimaryPictureBuffer_p));
#endif
                UNTAKE_PICTURE_BUFFER(VIDTRICK_Data_p->BufferManagerHandle, PrimaryPictureBuffer_p, VIDCOM_VIDTRICK_MODULE_BASE);
                PrimaryPictureBuffer_p = NULL;
            }
        }
        else
        {
            /* no secondary buffer is needed */
        }
        ErrorCode = ErrorSecond;

        if (ErrorSecond == ST_NO_ERROR)
        {
            /* Link Primary picture to PictureInfo_p */
            PictureInfo_p->PictureBuffer_p = PrimaryPictureBuffer_p;

            /* Warning: SecondaryPictureBuffer_p can be NULL */
            PictureInfo_p->PictureBuffer_p->AssociatedDecimatedPicture_p = SecondaryPictureBuffer_p;

            if (SecondaryPictureBuffer_p != NULL)
            {
                /* Link Secondary picture buffer to Primary Picture Buffer */
                SecondaryPictureBuffer_p->AssociatedDecimatedPicture_p = PictureInfo_p->PictureBuffer_p;
            }
        }
    }


    if ( (ErrorMain != ST_NO_ERROR) ||
         (ErrorSecond != ST_NO_ERROR) )
    {
        /* No Primary and/or Secondary Picture Buffer is available.
            In that case we should reuse a Picture Buffer already owned by vid_tric.
            We simply look for a Primary Picture Buffer because this picture buffer should already have an associated decimated picture buffer. */
        ErrorCode = ReusePictureBuffer(TrickModeHandle, PictureInfo_p);

        if (ErrorCode == ST_NO_ERROR)
        {
            /* A Primary Picture Buffer to reuse has been found. Check if a decimated picture buffer is needed: */
            if ( (IsDecimatedFrameBufferNeeded) &&
                 (PictureInfo_p->PictureBuffer_p->AssociatedDecimatedPicture_p == NULL) )
            {
                /* A decimated picture buffer is needed but this Picture Buffer doesn't have an associated decimated picture so it can not be used. */
#ifdef TRICKMOD_DEBUG
                TrickModeError();
                TraceBuffer(("Error! Can not reuse PB 0x%x because no decim pict is associated\n\r", PictureInfo_p->PictureBuffer_p));
#endif
                /* Release the picture buffer and return an error to indicate the no Picture Buffer to use has been found */
                RELEASE_PICTURE_BUFFER(VIDTRICK_Data_p->BufferManagerHandle, PictureInfo_p->PictureBuffer_p, VIDCOM_VIDTRICK_MODULE_BASE);
                PictureInfo_p->PictureBuffer_p = NULL;
                ErrorCode = ST_ERROR_NO_MEMORY;
            }
        }
    }

    return (ErrorCode);
}

/*******************************************************************************
Name        : ReusePictureBuffer
Description : This function should be called when no "Frame or 1st Field" Picture Buffer is available.
              It will parse the PictureInfo elements currently used by vid_tric and look for a
              Picture Buffer that could be reused.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t ReusePictureBuffer(const VIDTRICK_Handle_t     TrickModeHandle,
                                         VIDTRICK_PictureInfo_t *    PictureInfo_p)
{
    VIDTRICK_Data_t *           VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    ST_ErrorCode_t              ErrorCode = ST_ERROR_NO_MEMORY;
    VIDTRICK_PictureInfo_t *    PictureInfoToReuse_p = NULL;
    VIDTRICK_PictureInfo_t *    PictureInfoToReuseInBufferA_p = NULL;
    VIDTRICK_PictureInfo_t *    PictureInfoToReuseInBufferB_p = NULL;

    /* Look for a picture buffer to reuse in BitBuffer A */
    PictureInfoToReuseInBufferA_p = LookForPictureBufferToReuse(TrickModeHandle, &(VIDTRICK_Data_p->LinearBitBuffers.BufferA) );

    /* Look for a picture buffer to reuse in BitBuffer B */
    PictureInfoToReuseInBufferB_p = LookForPictureBufferToReuse(TrickModeHandle, &(VIDTRICK_Data_p->LinearBitBuffers.BufferB) );


    if (PictureInfoToReuseInBufferA_p != NULL)
    {
         if (PictureInfoToReuseInBufferB_p != NULL)
         {
            /*We've found a Picture Buffer to reuse in both Bit Buffers, we keep the one with the lower DecodingOrderFrameID */
            if (PictureInfoToReuseInBufferA_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID <
                 PictureInfoToReuseInBufferB_p->PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID)
            {
                /* Picture in Bit Buffer 'A' has a lower DecodingOrderFrameID: We reuse it */
                PictureInfoToReuse_p = PictureInfoToReuseInBufferA_p;
            }
            else
            {
                /* Picture in Bit Buffer 'B' has a lower DecodingOrderFrameID: We reuse it */
                PictureInfoToReuse_p = PictureInfoToReuseInBufferB_p;
            }
         }
         else
         {
            /* No Picture found in Bit BufferB => Use the one in Bit BufferA */
            PictureInfoToReuse_p = PictureInfoToReuseInBufferA_p;
         }
    }
    else
    {
        /* No Picture found in Bit BufferA => Use the one in Bit BufferB (it doesn't matter if PictureInfoToReuseInBufferA_p is NULL) */
        PictureInfoToReuse_p = PictureInfoToReuseInBufferB_p;
    }


    /* If we've found a PictureInfo element to reuse: */
    if (PictureInfoToReuse_p != NULL)
    {
#ifdef TRACE_UART_DEBUG
        TraceBuffer(("=> reuse PB 0x%x from 0x%x for 0x%x\r\n",
            PictureInfoToReuse_p->PictureBuffer_p,
            PictureInfoToReuse_p,
            PictureInfo_p));
#endif

        PictureInfo_p->PictureBuffer_p = PictureInfoToReuse_p->PictureBuffer_p;

        /* Reset Status of this PictureInfo element */
        ResetPictureInfoStatus(TrickModeHandle, PictureInfoToReuse_p);
        PictureInfoToReuse_p->PictureBuffer_p = NULL;

        if (PictureInfoToReuse_p->SecondFieldPicture_p != NULL)
        {
            /* We can not assume that PictureInfo_p will need this 2nd field so we should release it */
            ResetPictureInfoStatus(TrickModeHandle, PictureInfoToReuse_p->SecondFieldPicture_p);

            if (PictureInfoToReuse_p->SecondFieldPicture_p->PictureBuffer_p != NULL)
            {
                /* unlink picture buffer */
                PictureInfoToReuse_p->SecondFieldPicture_p->PictureBuffer_p->SmoothBackwardPictureInfo_p = NULL;
#ifdef TRACE_UART_DEBUG
                TraceBuffer((" Release 2nd Field PB 0x%x\n\r", PictureInfoToReuse_p->SecondFieldPicture_p->PictureBuffer_p));
#endif
                if (PictureInfoToReuse_p->SecondFieldPicture_p->PictureBuffer_p->AssociatedDecimatedPicture_p != NULL)
                {
                    RELEASE_PICTURE_BUFFER(VIDTRICK_Data_p->BufferManagerHandle, PictureInfoToReuse_p->SecondFieldPicture_p->PictureBuffer_p->AssociatedDecimatedPicture_p, VIDCOM_VIDTRICK_MODULE_BASE);
                    /* It is not allowed to set AssociatedDecimatedPicture_p to NULL because the link between Main and Decimated Picture Buffers is managed by VID_BUFF */
                }
                RELEASE_PICTURE_BUFFER(VIDTRICK_Data_p->BufferManagerHandle, PictureInfoToReuse_p->SecondFieldPicture_p->PictureBuffer_p, VIDCOM_VIDTRICK_MODULE_BASE);
                PictureInfoToReuse_p->SecondFieldPicture_p->PictureBuffer_p = NULL;
            }
        }

        ErrorCode = ST_NO_ERROR;
    }
    else
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("Error! No Pict Buff to reuse!\r\n"));
#endif
        /*VIDBUFF_PrintPictureBuffersStatus(VIDTRICK_Data_p->BufferManagerHandle); */
    }

    if  ( (ErrorCode == ST_NO_ERROR) &&
          (PictureInfo_p->PictureBuffer_p == NULL) )
    {
#ifdef TRICKMOD_DEBUG
        TrickModeError();
        TraceBuffer(("Error! No error but PB is a NULL pointer!"));
#endif
        ErrorCode = ST_ERROR_NO_MEMORY;
    }

    return (ErrorCode);
}

/*******************************************************************************
Name        : LookForPictureBufferToReuse
Description : This function parses a LinearBitBuffer structure and looks for a picture buffer to reuse.
              Only the Primary Picture Buffers containing a "Frame or 1st Field" are evaluated.
              It will return the one with oldest DecodingOrderFrameID.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static VIDTRICK_PictureInfo_t * LookForPictureBufferToReuse(const VIDTRICK_Handle_t     TrickModeHandle,
                                         VIDTRICK_LinearBitBuffer_t *  LinearBitBuffer_p)
{
    U32                         DecodingOrderFrameID = INVALID_ID;
    VIDTRICK_PictureInfo_t *    PictureInfoToReuse_p = NULL;
    VIDBUFF_PictureBuffer_t *   PictureBuffer_p;
    U32                         i;
    UNUSED_PARAMETER(TrickModeHandle);

    /* Go through all the elements referenced in this bit buffer */
    for (i = 0 ; i < LinearBitBuffer_p->NbPicturesInside; i++)
    {
        PictureBuffer_p = LinearBitBuffer_p->PictureInfoTableStartAddress_p[i].PictureBuffer_p;

        if ( (PictureBuffer_p != NULL) &&
             (IS_PICTURE_BUFFER_OWNED_ONLY_BY(PictureBuffer_p, VIDCOM_VIDTRICK_MODULE_BASE) ) &&
             (IsPictureBufferAFrameOrFirstField(PictureBuffer_p) ) &&
             (IsAPrimaryPictureBuffer(PictureBuffer_p) ) )
        {
            /* This element:
                - has a picture buffer associated
                - is owned only by vid_tric
                - contains a "Frame or 1st Field"
                - is a primary frame buffer
                so this is a good candidate for reuse */

            /* We want to reuse the oldest element (= the one with the lower DecodingOrderFrameID) */
            if (LinearBitBuffer_p->PictureInfoTableStartAddress_p[i].PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID <= DecodingOrderFrameID)
            {
                DecodingOrderFrameID = LinearBitBuffer_p->PictureInfoTableStartAddress_p[i].PictureBuffer_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID;
                PictureInfoToReuse_p = &(LinearBitBuffer_p->PictureInfoTableStartAddress_p[i]);
            }
        }
    }

    return(PictureInfoToReuse_p);
}


/*******************************************************************************
Name        : ReleaseAllPictureInfoElements
Description : This function releases every Picture Buffers currently used by vid_tric.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t ReleaseAllPictureInfoElements(const VIDTRICK_Handle_t     TrickModeHandle)
{
    VIDTRICK_Data_t *               VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    VIDTRICK_LinearBitBuffer_t *    LinearBitBuffer_p;
    U32                             i;

#ifdef TRACE_UART_DEBUG
    TraceBuffer(("ReleaseAllPictureInfoElements\n\r"));
#endif

    /* Look for a picture to reuse in BitBuffer A */
    LinearBitBuffer_p = &(VIDTRICK_Data_p->LinearBitBuffers.BufferA);

    for (i = 0 ; i < LinearBitBuffer_p->NbPicturesInside; i++)
    {
        if (LinearBitBuffer_p->PictureInfoTableStartAddress_p[i].PictureBuffer_p != NULL)
        {
            ReleasePictureInfoElement(TrickModeHandle, &(LinearBitBuffer_p->PictureInfoTableStartAddress_p[i]) );
        }
    }
    LinearBitBuffer_p->NbPicturesInside = 0;

    /* Look for a picture to reuse in BitBuffer B */
    LinearBitBuffer_p = &(VIDTRICK_Data_p->LinearBitBuffers.BufferB);

    for (i = 0 ; i < LinearBitBuffer_p->NbPicturesInside ; i++)
    {
        if (LinearBitBuffer_p->PictureInfoTableStartAddress_p[i].PictureBuffer_p != NULL)
        {
            ReleasePictureInfoElement(TrickModeHandle, &(LinearBitBuffer_p->PictureInfoTableStartAddress_p[i]) );
        }
    }
    LinearBitBuffer_p->NbPicturesInside = 0;

    return (ST_NO_ERROR);
}

#endif /* ST_smoothbackward */

#ifdef DEBUG_DISPLAY_QUEUE
/*******************************************************************************
Name        : DbgCheckPicturesInDisplayQueue
Description : This function checks that every pictures in LastPictureInDisplayQueue_p[] are valid
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static ST_ErrorCode_t DbgCheckPicturesInDisplayQueue(const VIDTRICK_Handle_t     TrickModeHandle)
{
    VIDTRICK_Data_t *               VIDTRICK_Data_p = (VIDTRICK_Data_t *)TrickModeHandle;
    U32         i;

    for(i=0; i<VIDTRICK_Data_p->NbOfPicturesInToBeInsertedInDQ; i++)
    {
        if (VIDTRICK_Data_p->LastPictureInDisplayQueue_p[i] == NULL)
        {
#ifdef TRICKMOD_DEBUG
            TrickModeError();
            TraceBuffer(("Invalid picture in DQ!\n\r"));
#endif /* TRICKMOD_DEBUG */
            return (ST_ERROR_BAD_PARAMETER);
        }
    }

    return (ST_NO_ERROR);
}
#endif /* DEBUG_DISPLAY_QUEUE*/

#ifdef TRACE_UART_DISPLAY_SKIP
/*******************************************************************************
Name        : TrickModeTraces
Description :
Parameters  : NbFieldsIfPosToSkipIfNegToRepeat, MustSkipAllPPictures
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void TrickModeTraces(const S32 NbFieldsIfPosToSkipIfNegToRepeat, const BOOL SkipAllPPictures)
{
    static U32 State = 0;

    if ((NbFieldsIfPosToSkipIfNegToRepeat <= VIDTRICK_LEVEL_SKIP_ALL_B_AND_DISP_P) && (!SkipAllPPictures))
    {
        if ((NbFieldsIfPosToSkipIfNegToRepeat <= VIDTRICK_LEVEL_SKIP_CURRENT_PICTURE) && (State != 0))
        {
            State = 0;
            TraceBuffer(("No Skip at all\n\r"));
        }
        else if ((NbFieldsIfPosToSkipIfNegToRepeat > VIDTRICK_LEVEL_SKIP_CURRENT_PICTURE) && (State != VIDTRICK_LEVEL_SKIP_CURRENT_PICTURE))
        {
            State = VIDTRICK_LEVEL_SKIP_CURRENT_PICTURE;
            TraceBuffer(("Skipping current\n\r"));
        }
    }
    else
    {
        if (SkipAllPPictures)
        {
            if (State != VIDTRICK_LEVEL_SKIP_ALL_P)
            {
                State = VIDTRICK_LEVEL_SKIP_ALL_P;
                TraceBuffer(("Skipping ALL P\n\r"));
            }
        }
        else if (NbFieldsIfPosToSkipIfNegToRepeat > VIDTRICK_LEVEL_SKIP_ALL_B_AND_DISP_P)
        {
            if (State != VIDTRICK_LEVEL_SKIP_ALL_B_AND_DISP_P)
            {
                State = VIDTRICK_LEVEL_SKIP_ALL_B_AND_DISP_P;
                TraceBuffer(("Skipping all B and not displaying P\n\r"));
            }
        }
    }
} /* end of TrickModeTraces() function */
#endif /* TRACE_UART_DISPLAY_SKIP */

#ifdef ST_smoothbackward
#ifdef TRACE_UART_EXT_TEMP_REF
/*******************************************************************************
Name        : TraceAllExtendedTemporalReferenceOfTheBuffer
Description : Function to help in debug : put a breakpoint here and you will know by the callers where it has been called
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void TraceAllExtendedTemporalReferenceOfTheBuffer(const VIDTRICK_Handle_t TrickModeHandle, VIDTRICK_LinearBitBuffer_t * const LinearBitBuffer_p, const U32 NumberOfPictureToTrace)
{
    VIDTRICK_PictureInfo_t   * Picture_p = LinearBitBuffer_p->PictureInfoTableStartAddress_p;
    U32                        index;

    if (LinearBitBuffer_p == &((VIDTRICK_Data_t *)TrickModeHandle)->LinearBitBuffers.BufferA)
    {
        TraceBuffer (("BUFFER A, %d\n\r", NumberOfPictureToTrace));
    }
    else
    if (LinearBitBuffer_p == &((VIDTRICK_Data_t *)TrickModeHandle)->LinearBitBuffers.BufferB)
    {
        TraceBuffer (("BUFFER B, %d\n\r", NumberOfPictureToTrace));
    }

    for (index=0 ; index < NumberOfPictureToTrace ; index++ , Picture_p++)
    {
        if ((index < 30) || (index > LinearBitBuffer_p->NbPicturesInside - 30))
        {
            TraceBuffer (("%c %2d  %d  Idx=%2d  ",
                   MPEGFrame2Char(Picture_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame),
                   Picture_p->PictureInfoFromDecode.pTemporalReference,
                   Picture_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference,
                   Picture_p->IndiceInBuffer));

            if (Picture_p->IsDecodable)
            {
                TraceBuffer (("Dec "));
            }
            else
            {
                TraceBuffer (("--- "));
            }

            TraceBuffer (("%s ", PictureStruct2String(Picture_p->PictureInfoFromDecode.PictureInfos.VideoParams.PictureStructure)));

 #define PRINT_ALL_PICTURES_INFO
#ifdef PRINT_ALL_PICTURES_INFO
            if (Picture_p->PictureInfoFromDecode.AdditionalStreamInfo.ExpectingSecondField)
            {
                TraceBuffer (("Exp "));
            }
            else
            {
                TraceBuffer (("--- "));
            }

            if (Picture_p->SecondFieldPicture_p != NULL)
            {
                TraceBuffer (("Scd=%2d  ", Picture_p->SecondFieldPicture_p->IndiceInBuffer));
            }
            else
            {
                TraceBuffer (("Scd=--  "));
            }

            if (Picture_p->BackwardOrLastBeforeIReferencePicture_p != NULL)
            {
                TraceBuffer (("Bck=%2d ", Picture_p->BackwardOrLastBeforeIReferencePicture_p->IndiceInBuffer));
            }
            else
            {
                TraceBuffer (("Bck=-- "));
            }

            if  (Picture_p->ForwardOrNextReferencePicture_p != NULL)
            {
                TraceBuffer (("Fwd=%2d  ", Picture_p->ForwardOrNextReferencePicture_p->IndiceInBuffer));
            }
            else
            {
                TraceBuffer (("Fwd=--  "));
            }
#endif
            TraceBuffer (("\n\r"));
        }
    }
} /* end of TraceAllExtendedTemporalReferenceOfTheBuffer() function */
#endif /* TRACE_UART_EXT_TEMP_REF */
#endif /* ST_smoothbackward */

#ifdef TRICKMOD_DEBUG
/*******************************************************************************
Name        : TrickModeError
Description : Function to help in debug : put a breakpoint here
              and you will know by the callers where it has been called
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void TrickModeErrorLine(U32 Line)
{
    U8 Error;
    Error++;
    TraceBuffer(("ERROR - %d - ", Line));       /* Informs user about Error line */
} /* end of TrickModeError() function */
#endif /* TRICKMOD_DEBUG */

/*******************************************************************************
Name        : vidtrick_FillPictureBuffer
Description : Function to fill new VIDBUFF_PictureBuffer_t structure with old VIDDEC_Data_t structure content
Parameters  :
Assumptions : both passed pointers are not NULL
Limitations :
Returns     :
*******************************************************************************/
static void vidtrick_FillPictureBuffer(VIDBUFF_PictureBuffer_t *PB_p, VIDTRICK_PictureInfo_t *vidtric_p)
{
    U32     DispHorSize;
    U32     DispVerSize;
    U8      panandscan;
#if defined(ST_multicodec)
    U32     counter;
#endif /* ST_multicodec */

    PB_p->ParsedPictureInformation.PictureDecodingContext_p = &(PB_p->PictureDecodingContext);
    PB_p->ParsedPictureInformation.SizeOfPictureSpecificDataInByte = 0; /* see next what to put inside specfic to MPEG parser/decoder */
    PB_p->ParsedPictureInformation.PictureSpecificData_p = &(PB_p->PictureSpecificData[0]);
    PB_p->PictureDecodingContext.GlobalDecodingContext_p = &(PB_p->GlobalDecodingContext);

    PB_p->ParsedPictureInformation.PictureGenericData.PictureInfos = vidtric_p->PictureInfoFromDecode.PictureInfos;
    PB_p->ParsedPictureInformation.PictureGenericData.IsFirstOfTwoFields = vidtric_p->PictureInfoFromDecode.AdditionalStreamInfo.ExpectingSecondField;
    if (vidtric_p->PictureInfoFromDecode.AdditionalStreamInfo.RepeatFieldCounter != 0)
    {
        PB_p->ParsedPictureInformation.PictureGenericData.RepeatFirstField = TRUE;
    }
    else
    {
        PB_p->ParsedPictureInformation.PictureGenericData.RepeatFirstField = FALSE;
    }

    PB_p->ParsedPictureInformation.PictureGenericData.RepeatProgressiveCounter = (U8) vidtric_p->PictureInfoFromDecode.AdditionalStreamInfo.RepeatProgressiveCounter;

    if (vidtric_p->PictureInfoFromDecode.AdditionalStreamInfo.HasSequenceDisplay)
    {
        DispHorSize = vidtric_p->PictureInfoFromDecode.AdditionalStreamInfo.SequenceDisplayExtension.display_horizontal_size;
        DispVerSize = vidtric_p->PictureInfoFromDecode.AdditionalStreamInfo.SequenceDisplayExtension.display_vertical_size;
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
                    vidtric_p->PictureInfoFromDecode.AdditionalStreamInfo.HasSequenceDisplay;
    }
    for (panandscan = 0; panandscan < vidtric_p->PictureInfoFromDecode.AdditionalStreamInfo.PictureDisplayExtension.number_of_frame_centre_offsets; panandscan++)
    {
        PB_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[panandscan].FrameCentreHorizontalOffset =
            vidtric_p->PictureInfoFromDecode.AdditionalStreamInfo.PictureDisplayExtension.FrameCentreOffsets[panandscan].frame_centre_horizontal_offset;
        PB_p->ParsedPictureInformation.PictureGenericData.PanAndScanIn16thPixel[panandscan].FrameCentreVerticalOffset =
            vidtric_p->PictureInfoFromDecode.AdditionalStreamInfo.PictureDisplayExtension.FrameCentreOffsets[panandscan].frame_centre_vertical_offset;
    }
    PB_p->ParsedPictureInformation.PictureGenericData.NumberOfPanAndScan = vidtric_p->PictureInfoFromDecode.AdditionalStreamInfo.PictureDisplayExtension.number_of_frame_centre_offsets;

    PB_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID = vidtric_p->PictureInfoFromDecode.AdditionalStreamInfo.ExtendedTemporalReference;
    PB_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID = vidtric_p->PictureInfoFromDecode.ExtendedPresentationOrderPictureID;

    /*--------------------------------------------------------------------------*/
    /*    not used in old viddec (taken from old decode viddec_p structure)     */
    /*--------------------------------------------------------------------------*/
    PB_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PTS.IsValid = FALSE;
    if (vidtric_p->PictureInfoFromDecode.PictureInfos.VideoParams.MPEGFrame != STVID_MPEG_FRAME_B)
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

/*    PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.SequenceInfo = not used in vid_tric;*/
    /*--------------------------------------------------------------------------*/
    /* End of not used in old viddec (taken from old decode viddec_p structure) */
    /*--------------------------------------------------------------------------*/

    PB_p->ParsedPictureInformation.PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_NONE;

    PB_p->GlobalDecodingContext.SizeOfGlobalDecodingContextSpecificDataInByte = 0;
    PB_p->GlobalDecodingContext.GlobalDecodingContextSpecificData_p = &(PB_p->GlobalDecodingContextSpecificData[0]);

    if ((vidtric_p->PictureInfoFromDecode.AdditionalStreamInfo.HasSequenceDisplay) &&
        (vidtric_p->PictureInfoFromDecode.AdditionalStreamInfo.SequenceDisplayExtension.colour_description))
    {
        PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.ColourPrimaries =
                vidtric_p->PictureInfoFromDecode.AdditionalStreamInfo.SequenceDisplayExtension.colour_primaries;
        PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.TransferCharacteristics =
                vidtric_p->PictureInfoFromDecode.AdditionalStreamInfo.SequenceDisplayExtension.transfer_characteristics;
        PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.MatrixCoefficients =
                vidtric_p->PictureInfoFromDecode.AdditionalStreamInfo.SequenceDisplayExtension.matrix_coefficients;
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
    PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.FrameCropInPixel.BottomOffset = 0;

    PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.NumberOfReferenceFrames = 2;
    PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.MaxDecFrameBuffering = 2;


    PB_p->GlobalDecodingContext.GlobalDecodingContextGenericData.ParsingError = VIDCOM_ERROR_LEVEL_NONE;

} /* end of vidtrick_FillPictureBuffer() */

/*********************************************************************************************
Name        : vidtrick_SearchAndGetNextStartCode
Description : Function to search for a startcode in the given memory area
              from BeginSearchFrom_p to MaxSearchAddress_p. The first byte X of
              the 00 00 01 xx sequence must verify BeginSearchFrom_p <= X < MaxSearchAddress_p
Parameters  :
              BeginSearchFrom_p  : IN start search address
              MaxSearchAddress_p : IN end search address
              StartCode_p        : OUT startcode value
              StartCodeAddress_p : OUT the address of the byte containing the startcode value
Assumptions : no null pointer
Limitations :
Returns     : TRUE if a startcode was found
**********************************************************************************************/
static BOOL vidtrick_SearchAndGetNextStartCode(const U8*  const BeginSearchFrom_p,
                                               const U8*  const MaxSearchAddress_p,
                                               U8*  const StartCode_p,
                                               U8** StartCodeAddress_p)
{
    U8      SearchPattern[] = {0x0, 0x0, 0x1};
    U8*     NextByte_p;
    U32     i = 0;
    BOOL    StartCodeFound;

    /* Begin searching for the 00 00 01 xx from the address (BeginSearchFrom_p) */
    NextByte_p = (U8*) ((U32)BeginSearchFrom_p);
    while((i != 3) && ((U32)NextByte_p < (U32)MaxSearchAddress_p))
    {
        /* Search for the 00 00 01 sequence from the NextByte_p address */
        for(i=0;i<3;i++)
        {
            if(STSYS_ReadRegMemUncached8(NextByte_p + i) != SearchPattern[i])
            {
                break;
            }
        }
        NextByte_p++;
    }
    /* Check if we have found the 00 00 01 pattern */
    if(i==3)
    {
        /* The pattern was found. */
        (*StartCode_p)         = STSYS_ReadRegMemUncached8(NextByte_p + 2);
        (*StartCodeAddress_p)  = NextByte_p + 2;
        StartCodeFound = TRUE;
    }
    else
    {
        StartCodeFound = FALSE;
    }
    return(StartCodeFound);
} /* End of vidtrick_SearchAndGetNextStartCode() function */

/*******************************************************************************/

/* End of vid_tric.c */
