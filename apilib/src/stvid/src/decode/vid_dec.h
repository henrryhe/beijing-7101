/*******************************************************************************

File name   : vid_dec.h

Description : Video decode common header file

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
 7 Mar 2000        Created                                          HG
16 Mar 2001        Decode Event notification configuration.         GG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_DECODE_COMMON_H
#define __VIDEO_DECODE_COMMON_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "stvid.h"

#include "buffers.h"
#include "multidec.h"
#include "decode.h"
#include "vid_ctcm.h"
#include "halv_mult.h"
#include "halv_dec.h"

#include "stevt.h"

#ifdef ST_avsync
#include "stclkrv.h"
#endif /* ST_avsync */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define CONTROLLER_COMMANDS_BUFFER_SIZE 10
/* Max 16 start code between sequence and slice... */
#define START_CODE_BUFFER_SIZE          16
/* Max 25 IT's before any of them are processed ? */
#define INTERRUPTS_BUFFER_SIZE          25

/* Impossible value for temporal reference (defined on 10 bits). Needed at init. */
#define IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE 2000
/* maximum difference between too succesive temporal differences */
#define MAX_DIFF_BETWEEN_TEMPORAL_REFERENCES 18
/* Maximum frames per GOP: 18 (NTSC) / 15 (PAL), i.e. 0.6 seconds */

enum
{
    VIDDEC_SEQUENCE_INFO_EVT_ID,
    VIDDEC_USER_DATA_EVT_ID,
    VIDDEC_DATA_OVERFLOW_EVT_ID,
    VIDDEC_DATA_UNDERFLOW_EVT_ID,
    VIDDEC_READY_TO_DECODE_NEW_PICTURE_EVT_ID,
    VIDDEC_DATA_ERROR_EVT_ID,
    VIDDEC_PICTURE_DECODING_ERROR_EVT_ID,
    VIDDEC_STOPPED_EVT_ID,
    VIDDEC_NEW_PICTURE_SKIPPED_NOT_REQUESTED_EVT_ID,
    VIDDEC_NEW_PICTURE_DECODED_EVT_ID,
    VIDDEC_DECODE_ONCE_READY_EVT_ID,
    VIDDEC_RESTARTED_EVT_ID,
    VIDDEC_DECODE_LAUNCHED_EVT_ID,
    VIDDEC_SKIP_LAUNCHED_EVT_ID,

#ifdef ST_smoothbackward
    VIDDEC_START_CODE_FOUND_EVT_ID,
    VIDDEC_START_CODE_SLICE_FOUND_EVT_ID,
    VIDDEC_DECODER_IDLE_EVT_ID,
    VIDDEC_READY_TO_START_BACKWARD_MODE_EVT_ID,
#endif /* ST_smoothbackward */

#ifdef STVID_HARDWARE_ERROR_EVENT
    VIDDEC_HARDWARE_ERROR_EVT_ID,
#endif /* STVID_HARDWARE_ERROR_EVENT */
    VIDDEC_SEQUENCE_END_CODE_EVT_ID,
    VIDDEC_NEW_PICTURE_PARSED_EVT_ID,
    VIDDEC_NB_REGISTERED_EVENTS_IDS
};

/* No more than 128 commands: it needs to be casted to U8 */
typedef enum ControllerCommandID_e {
    CONTROLLER_COMMAND_TERMINATE,
    CONTROLLER_COMMAND_RESET,
    CONTROLLER_COMMAND_SWITCH_TO_BACKWARD_MODE,
    NB_CONTROLLER_COMMANDS   /* Keep this one as the last one */
} ControllerCommandID_t;

typedef enum VIDDEC_DecoderState_e
{
    VIDDEC_DECODER_STATE_DECODING,
/*    VIDDEC_DECODER_STATE_PAUSED,*/
    VIDDEC_DECODER_STATE_STOPPED
} VIDDEC_DecoderState_t;

typedef enum VIDDEC_HardwareState_e
{
    /* DEI occured : Hardware decoder is locked on next picture and ready to decode. */
    VIDDEC_HARDWARE_STATE_FOUND_NEXT_PICTURE,

    /* PEI occured : Hardware decoder has finished decoding but is not ready to decode. */
    VIDDEC_HARDWARE_STATE_IDLE,

    /* Hardware decoder is decoding. */
    VIDDEC_HARDWARE_STATE_DECODING,

    /* Hardware decoder is skipping. */
    VIDDEC_HARDWARE_STATE_SKIPPING
} VIDDEC_HardwareState_t;

/* Exported Types ----------------------------------------------------------- */

typedef struct HeaderData_s {
    U32     Bits;           /* Remaining data bits. Only the NbRemainingBits MSB of this are valid bits */
    U32     NumberOfBits;   /* Available remaining bits in RemainingBits */
    BOOL    SearchThroughHeaderDataNotCPU; /* TRUE  : search through HW Header Data FIFO (standard HW data retrieval functions through HALDEC access)
                                              FALSE : CPU search through memory parsing (so called manual parsing) */
    HALDEC_Handle_t HALDecodeHandle; /* For HW Header Data FIFO search : HAL handle to call the HALDEC */
    BOOL    WaitForEverWhenHeaderDataEmpty; /* For HW HeaderData FIFO search : behaviour when HW HeaderData FIFO is empty, i.e. wait for ever or quit after a few trys */
    BOOL    IsHeaderDataSpoiled; /* For HW Header Data FIFO search : FALSE normally, TRUE when HW header data FIFO becomes empty and returns bad data. */
    const U8 * CPUSearchLastReadAddressPlusOne_p; /*  For CPU search, last read address + 1 in memory (byte aligned), i.e. address to read next time.
                Write an address there so that next viddec_HeaderDataGetXXX() starts parsing from this address.
                So the value saved in this parameter by previous viddec_HeaderDataGetXXX() can be used directly by next viddec_HeaderDataGetXXX() */
#ifdef STVID_DEBUG_GET_STATISTICS
    U32     StatisticsDecodePbHeaderFifoEmpty; /* Counts number of times the header fifo was empty when poping start code or bits. */
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_CONTROL_HEADER_BITS
    U32     BitsCounter;    /* Counts number of bits retrieved */
    U32     MissingBitsCounter;  /* 0 in normal behaviour, set to number of missing bits if data FIFO empty */
#endif
} HeaderData_t; /* Data concerning the header data */

typedef struct StartCodeParsing_s
{
    BOOL        IsOnGoing;              /* True while parsing start code data: caution if a SCH interrupt occurs while TRUE ! */
    BOOL        IsGettingUserData;      /* To know what to do with start code while parsing user data */
    BOOL        IsOK;                   /* Set TRUE at the beginning of parsing, checked at the end. In between, it may set FALSE by anybody detecting parsing error. */
    BOOL        HasBadMarkerBit;        /* Set FALSE at the beginning of parsing, checked at the end. In between, it may set TRUE when detecting marker bit at 0. */
    BOOL        HasUserDataProcessTriggeredStartCode;
#ifdef ST_smoothbackward
    BOOL        DiscardStartCode;       /* Action to take when a start code is found. */
#endif /* ST_smoothbackward */
} StartCodeParsing_t;

typedef struct BitRateValueEvaluate_s
{
    /* flags */
    BOOL BitRateValueIsComputable;                   /* If Trick mode is requiring jumps on HDD, Bit Rate Value can not be calculated */
    BOOL BitRateValueIsUnderComputing;
    /* bit rate value calculated by hand */
    U32  Time[VIDDEC_NB_PICTURES_USED_FOR_BITRATE];
    U32  BitBufferSize[VIDDEC_NB_PICTURES_USED_FOR_BITRATE];
    U32  LastBitBufferOutputCounter;                 /* in bytes units */
    /* counters */
    U8   BitRateIndex;
    U8   BitRateValidValueNb;
} BitRateValueEvaluate_t;

#ifdef ST_SecondInstanceSharingTheSameDecoder
typedef struct StillDecodeParams_s
{
    BOOL                    DecodeStillRequested;
    void *                  BitBufferAddress_p;
    U32                     BitBufferSize;  /* in byte */
    BOOL                    GlobalExternalBitBufferAllocated;
    VIDBUFF_Handle_t        BufferManagerHandle;
    StartCodeParsing_t      StartCodeParsing;
    HeaderData_t            HeaderData;
    MPEG2BitStream_t        MPEG2BitStream;
    STVID_BroadcastProfile_t                    BroadcastProfile;
    U32                     NbFieldsDisplayNextDecode;
    STEVT_EventID_t         RegisteredEventsID[VIDDEC_NB_REGISTERED_EVENTS_IDS];
    STEVT_Handle_t          EventsHandle;
    struct
    {
        BOOL ExpectingSecondField;
        U32 ExtendedTemporalReference;
        STVID_TimeCode_t TimeCode;
    }StillStartCodeProcessing;
} StillDecodeParams_t;
#endif /* ST_SecondInstanceSharingTheSameDecoder */

/* Type for internal structures for the decoder */
typedef struct VIDDEC_Data_s {
    ST_Partition_t *    CPUPartition_p;
    STEVT_Handle_t      EventsHandle;
    STEVT_EventID_t     RegisteredEventsID[VIDDEC_NB_REGISTERED_EVENTS_IDS];
    VIDBUFF_Handle_t    BufferManagerHandle;
#ifdef ST_ordqueue
    VIDQUEUE_Handle_t   OrderingQueueHandle;
#endif /* ST_ordqueue */
    HALDEC_Handle_t     HALDecodeHandle;
#ifdef ST_inject
    VIDINJ_Handle_t     InjecterHandle;
#endif /* ST_inject */
    STVID_DeviceType_t  DeviceType;
    U8                  DecoderNumber;  /* As defined by HALDEC. See definition there. */
    U32                 ValidityCheck;

    void *              MappedRegistersBaseAddress_p;
    void *              MappedCompressedDataInputBaseAddress_p;

    VIDBUFF_BufferAllocationParams_t BitBuffer; /* Used to allocate bit buffer, or to store Size and Address_p given by user at init ! */
    BOOL                BitBufferAllocatedByUs;
    U32                 BitBufferUnderflowMargin; /* Used to know when to notify
                                                   * the underflow event */
    U32                 BitBufferRequiredDuration; /* Used to know how much data
                                                    * to inject */

    VIDDEC_DecoderState_t   DecoderState;
    BOOL                    DecoderIsFreezed;
    STVID_DecodedPictures_t DecodedPictures;
    STVID_DecodedPictures_t DecodedPicturesBeforeLastFreeze;
    VIDDEC_DecodeAction_t   DecodeAction;

    struct
    {
        BOOL        AllIPictures;       /* Normally always FALSE. Could be true when streams with no I detected ? */
        BOOL        AllPPictures;       /* TRUE when decode mode requires slip of all P pictures */
        BOOL        AllBPictures;       /* TRUE when decode mode requires slip of all B pictures */
        BOOL        NextBPicture;       /* TRUE to skip next B picture */
        BOOL        UntilNextIPicture;  /* TRUE to skip all pictures until next I picture */
        BOOL        UntilNextSequence;  /* TRUE to skip all pictures until next sequence */
        BOOL        AllExceptFirstI;
    } Skip;
    BOOL                         SearchFirstI;
    U32 NbFieldsDisplayNextDecode;

    BOOL                    NeedToDecreaseBBL;  /* 2 states automaton used to prevent Bit Buffer Level Overflow */
    STVID_DataUnderflow_t   DataUnderflow; /* Updated when monitoring bit buffer level, uesd to send data underflow event */
    BOOL                    DataUnderflowEventNotSatisfied; /* Used not to send data underflow event continuously but only when detecting underflow:
                                                               TRUE when sending data underflow event, FALSE when user calls STVID_DataInjectionCompleted() */
                                                            /* for automatic mode, used to check if there is a data injection in progress */
                                                            /* for 2 reasons : first : when switching from positive to negative speed, */
                                                            /* trickmode can not require a linear bit buffer flush until the last */
                                                            /* UNDERFLOW event has been completed. Second, when playing backward, */
                                                            /* bit buffer free size must be returned to 0 by VIDDEC_GetBitBufferFreeSize */
                                                            /* if no injection in progress. */
    BOOL                    OverFlowEvtAlreadySent; /* used to send only one OVERFLOW evt once the overflow threshold has been reached */

#ifdef ST_smoothbackward
    /*BOOL                DataInjectionInProgress;*/
    U32                 LastLinearBitBufferFlushRequestSize;
#endif
    U8                  LastStartCode;          /* Last start code found by the SC detector */

    /* Decode events notification configuration ----------------------------- */
    struct
    {
        STVID_ConfigureEventParams_t ConfigureEventParam;    /* Contains informations : Event enable/disable notification
                                                            and Number of event notification to skip before notifying the event. */
        U32                          NotificationSkipped;    /* Number of notification skipped since the last one. */

    } EventNotificationConfiguration [VIDDEC_NB_REGISTERED_EVENTS_IDS];

    /* Controller ----------------------------------------------------------- */
    VIDCOM_Task_t       DecodeTask;
    semaphore_t  *      DecodeOrder_p;      /* To synchronise orders in the task */
    VIDCOM_Interrupt_t  VideoInterrupt;                 /* Data concerning the video interrupt */
#ifdef STVID_STVAPI_ARCHITECTURE
    VIDCOM_Interrupt_t  MB2RInterrupt;
#endif /* end of def STVID_STVAPI_ARCHITECTURE */

    /* Buffers of commands: 2 for each decoder */
    U8 CommandsData[CONTROLLER_COMMANDS_BUFFER_SIZE];
    U8 StartCodeData[START_CODE_BUFFER_SIZE];
    U32 InterruptsData[INTERRUPTS_BUFFER_SIZE];
    CommandsBuffer_t    CommandsBuffer;                 /* Data concerning the decode controller commands queue */
    CommandsBuffer_t    StartCodeBuffer;                /* Data concerning the start code queue */
    CommandsBuffer32_t  InterruptsBuffer;               /* Data concerning the interrupts commands queue */

    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping; /* Needed to translate data1 and data2 in NEW_PICTURE_DECODED_EVT */

#ifdef ST_ordqueue
    VIDDEC_UpdateDisplayQueue_t UpdateDisplayQueueMode; /* The mode to pass picture to display when display update starts */
    VIDDEC_UpdateDisplayQueue_t CurrentUpdateDisplayQueueMode; /* The mode currently used (should be never when display
                                                                  update is not yet started) */
#endif  /* end of def ST_ordqueue */

    BOOL DecodeOnce;
    BOOL RealTime;
    BOOL LaunchStartCodeSearchToResume;

    /* Used to send STVID_SEQUENCE_INFO_EVT */
    BOOL                    SendSequenceInfoEventOnceAfterStart;
    BOOL                    ComputeSequenceInfoOnNextPictureOrGroup;
    STVID_SequenceInfo_t    PreviousSequenceInfo;
    STVID_MPEGFrame_t       PreviousDecodedPictureMPEGFrame;

    struct
    {
        BOOL        WhenNextIPicture;
        BOOL        WhenNextReferencePicture;
        BOOL        WhenNoData;
        BOOL        StopAfterBuffersTimeOut; /* Stop decoder if no free buffer found after a timeout */
        BOOL        DiscontinuityStartCodeInserted;
        BOOL        AfterCurrentPicture;
        BOOL        IsPending;
        STOS_Clock_t   TimeoutForSendingStoppedEvent;
    } Stop;

    U32                 UnsuccessfullDecodeCount;

    BitRateValueEvaluate_t BitRateValueEvaluate;

    /* Decoder -------------------------------------------------------------- */
    struct
    {
        VIDBUFF_PictureBuffer_t *   LastButOne_p;   /* Last but one reference */
        VIDBUFF_PictureBuffer_t *   Last_p;         /* Last reference */
        BOOL                        MissingPrevious; /* Set TRUE when the previous reference is missing */
        BOOL                        KeepLastReference; /* Set TRUE when the previous reference musn't be reset on the next start */
        semaphore_t *               ReferencePictureSemaphore_p; /* semaphore to protect LastButOne_p && Last_p use */
    } ReferencePicture;

    struct
    {
        VIDDEC_HardwareState_t      HWState;
        VIDBUFF_PictureBuffer_t *   DecodingPicture_p; /* Set to the picture buffer currently being decoded, or NULL when no decode is on-going */
        STVID_PictureInfos_t *      LastDecodedPictureInfos_p; /* Remembers the last decoded picture infos */
        U32                         LastBitBufferLevel; /* Always contains the last bit buffer level monitored */
        S32                         LastPresentationOrderPictureID; /* Remembers the last decoded picture PresentationOrderPictureID */
        STOS_Clock_t                     MaxDecodeOrSkipTime; /* Max allowed end time for a decode or skip */
        STOS_Clock_t                     MaxBufferWaitingTime; /* Max allowed end time for getting a free buffer for decode when STOP_END_OF_DATA */
    } HWDecoder;

    struct
    {
        BOOL                        IsIdle; /* TRUE when idle, FALSE when launching SC detector, TRUE when SCH occurs (i.e. back idle) */
        STOS_Clock_t                MaxStartCodeSearchTime;
        U32                         MaxBitbufferLevelForFindingStartCode;
    } HWStartCodeDetector;

    /* Parser --------------------------------------------------------------- */
    STVID_BroadcastProfile_t BroadcastProfile;
    MPEG2BitStream_t * DecodingStream_p; /* Pointer to a stream structure (allocated somewhere else like at init) */
    HeaderData_t HeaderData;
    StartCodeParsing_t  StartCodeParsing;

#ifdef ST_smoothbackward
    struct
    {
        BOOL                            StartCodeDetectorToLaunch;
        VIDDEC_StartCodeSearchMode_t    SearchMode;
        BOOL                            FirstSliceDetect;
        void *                          SearchAddress_p;
        BOOL                            SCDetectorHasBeenLaunchedManually;
        BOOL                            StartCodeWasFoundManually;
	void *                          StartCodeAddress_p;
    } RequiredSearch;
#endif /* ST_smoothbackward */

    struct
    {
        U8   PreviousStartCode;
        STVID_SequenceInfo_t SequenceInfo;
        BOOL FirstPictureEver;
        BOOL ExpectingSecondField;

        U32 TemporalReferenceOffset;
        U32 ExtendedTemporalReference;
        U32 PreviousExtendedTemporalReference;
        U32 PreviousGreatestReferenceExtendedTemporalReference;
        STVID_MPEGFrame_t PreviousPictureMPEGFrame;
        U32 PreviousPictureNbFieldsDisplay;
        U32 PreviousReferenceTotalNbFieldsDisplay;
        U32 SupposedPicturesDisplayDuration;
        U32 PreviousExtendedTemporalReferenceForPTS;
        BOOL OneTemporalReferenceOverflowed;
        STVID_TimeCode_t TimeCode;
        U32 PictureCounter;
        STVID_PTS_t PTS;
        U16 PreviousTemporalReference;
        U16 LastReferenceTemporalReference;
        U16 LastButOneReferenceTemporalReference;
        BOOL tempRefReachedZero;
        BOOL OverflowingTemporalReferenceDetected;
        BOOL GetSecondReferenceAfterOverflowing;
    } StartCodeProcessing;

    struct
    {
        BOOL        Active;
        BOOL        GotFirstReferencePicture;
    } BrokenLink;

    void *                      UserDataTable_p; /* (allocated somewhere else like at init) */
    BOOL                        UserDataEventEnabled;
    STVID_UserData_t            UserData;

    struct
    {
#ifdef ST_ordqueue
        BOOL LaunchDisplayWhenPossible;
#endif  /* end of def ST_ordqueue */
        BOOL LaunchDecodeWhenPossible;
        BOOL LaunchSkipWhenPossible;
#ifdef ST_ordqueue
        U8 NumberOfDisplayToLaunch;  /* NumberOfDisplayToLaunch is there to check the 2 fields of a P are existing before allowing     */
                                     /* the insertion on display. The reason comes from the MPEG Standard : the 2nd field of P has its */
                                     /* 1st field as 2nd field of reference. Otherwise when the 1st field of a P is missing (ex: PT)   */
                                     /* we insert a macroblocked picture (ex: PB). Macroblocked because decoded from a bad reference.  */
#endif  /* end of def ST_ordqueue */
    } NextAction;

    STVID_ErrorRecoveryMode_t       ErrorRecoveryMode;
#ifdef ST_smoothbackward
    VIDDEC_ControlMode_t            ControlModeInsideHAL;   /* Current control mode as set in hardware, different from ControlMode only when changing, between call to SetControlMode and real change at SoftReset(). */
    VIDDEC_ControlMode_t            ControlMode;            /* Current control mode, or about to be this one (when new setting) */
#endif
    VIDDEC_DecodePictureParams_t    DecodeParams;

    struct
    {
        U8                                      PreviousStartCode;
        STOS_Clock_t                                 MaxWaitOrderTime;
        STOS_Clock_t                                 PreviousBitBufferLevelChangeTime;
#ifdef LIMIT_TRY_GET_BUFFER_TIME
        STOS_Clock_t                                 MaxTryGetBufferTime;
#endif /* LIMIT_TRY_GET_BUFFER_TIME */
        BOOL                                    TryingToRecoverFromMaxDecodeTimeOverflow ;
        BOOL                                    NoDisplayAfterNextDecode;
        BOOL                                    SoftResetPending;
        STVID_PictureInfos_t                    PictureInfos;
        StreamInfoForDecode_t                   StreamInfoForDecode;
        U32                                     TemporalReferenceOffset;
        PictureStreamInfo_t                     PictureStreamInfo;
        VIDDEC_SyntaxError_t                    SyntaxError ;
#ifdef ST_ordqueue
        struct
        {
            VIDBUFF_PictureBuffer_t *   Picture_p;    /* Picture to queue on display */
            BOOL                        CurrentDecodeExpectingSecondField;
            VIDCOM_PictureID_t          CurrentDecodeForwardReferenceExtendedPresentationOrderPictureID;
            BOOL                        CurrentDecodeForwardReferenceExtendedPresentationOrderPictureIDIsValid;
            VIDCOM_PictureID_t          LastParsedReferenceExtendedPresentationOrderPictureID;
            BOOL                        LastParsedReferenceExtendedPresentationOrderPictureIDIsValid;
            VIDCOM_PictureID_t          PictureToPushAfterCurrentDecodeExtendedPresentationOrderPictureID;
            BOOL                        PictureToPushAfterCurrentDecodeExtendedPresentationOrderPictureIDIsValid;
            VIDQUEUE_InsertionOrder_t   InsertionOrder;
        } QueueParams;
#endif  /* end of def ST_ordqueue */
        VIDDEC_NewPictureDecodeOrSkip_t         NewPictureDecodeOrSkipEventData;
    } ForTask;

    VIDDEC_DecoderOperatingMode_t               DecoderOperatingMode;
    struct
    {
        BOOL                  IsDecodeEnable; /* used to delay the first decode after VID_Start while the bit buffer level has not reached the vbv size */
                                              /* set FALSE at SoftReset in real time and TRUE when bit buffer level has reached its vbv size. otherwise set TRUE */
        BOOL                  FirstPictureAfterFirstSequence;
        BOOL                  FirstSequenceSinceLastSoftReset;
        /* Vbv_delay flags */
        BOOL                  StartWithVbvDelay;
        U32                   VbvDelayTimerExpiredDate;
        /* vbv_buffer_size flags */
        BOOL                  StartWithVbvBufferSize;
        U32                   VbvSizeToReachBeforeStartInBytes;      /* it is the vbv_buffer_size in the stream minor an amount of data (threshold to guarantee we don't reach the BBF */
        BOOL                  VbvSizeReachedAccordingToInterruptBBT; /* Set FALSE at SoftReset, and TRUE when the first BBF interrupt occurs. So checking */
                                                                     /* that it is FALSE tells that we are still waiting for vbv_size to be reached i bit buffer */
        /* PTS comparison time flag */
#ifdef ST_avsync
        BOOL                  StartWithPTSTimeComparison;
        STVID_PTS_t           FirstPicturePTSAfterFirstSequence;
        STCLKRV_Handle_t      STClkrvHandle;
#endif /* ST_avsync */

        /* BOOL                  VbvBufferSizeStarted; */ /* only used to debug to compare vbv delay start and vbv buffer size start */
    } VbvStartCondition;

    STVID_HDPIPParams_t     HDPIPParams;    /* HDPIP parameters.    */

#ifdef STVID_DEBUG_GET_STATISTICS
    STVID_Statistics_t                          Statistics;
#endif /* STVID_DEBUG_GET_STATISTICS */

#ifdef STVID_DEBUG_GET_STATUS
	struct
    {
    	STVID_SequenceInfo_t * SequenceInfo_p;  /* Last sequence information, NULL if none or stopped */
    } Status;
#endif /* STVID_DEBUG_GET_STATUS */

#ifdef DECODE_DEBUG
    struct
    {
        STVID_MPEGFrame_t                       LastDecodedPictureMpegFrame;
    } Debug;
#endif

#ifdef ST_SecondInstanceSharingTheSameDecoder
    StillDecodeParams_t StillDecodeParams;
#endif /* ST_SecondInstanceSharingTheSameDecoder */
#ifdef STVID_STVAPI_ARCHITECTURE
    struct
    {
        DVRBM_BufferPoolId_t BufferPoolId;
        BOOL                 BufferPoolIdIsAvailable;
        /* the RasterPicturesLocked are aimed to remember that some main or */
        /* decimated pictures are locked and have to be unlocked */
        BOOL                 MainRasterPicturesLocked;
        BOOL                 DecimatedRasterPicturesLocked;
        /* flag storing wich one of the top or bottom field of a decoded frame */
        /* is supposed to be displayed in first */
        BOOL                 FirstFieldIsTop;
        /* parmaters for the main & secondary raster reconstruction */
        BOOL                 MainRasterReconstructionIsEnabled;
        STGVOBJ_DecimationFactor_t CurrentRasterDecimation;
        /* structure storing all the identifiers of the pictures */
        struct
        {
            STGVOBJ_PictureId_t MainTopFieldPictureId;
            STGVOBJ_PictureId_t MainBottomFieldPictureId;
            STGVOBJ_PictureId_t MainRepeatFieldPictureId;
            STGVOBJ_PictureId_t DecimatedTopFieldPictureId;
            STGVOBJ_PictureId_t DecimatedBottomFieldPictureId;
            STGVOBJ_PictureId_t DecimatedRepeatFieldPictureId;
        } ToBeDecodedRasterPictureId;
        /* structure storing all the pointers of the pictures allocated by RBM, */
        /* that are in use in STVID */
        struct
        {
            STGVOBJ_Picture_t * MainTopFieldPicture_p;
            STGVOBJ_Picture_t * MainBottomFieldPicture_p;
            STGVOBJ_Picture_t * MainRepeatFieldPicture_p;
            STGVOBJ_Picture_t * DecimatedTopFieldPicture_p;
            STGVOBJ_Picture_t * DecimatedBottomFieldPicture_p;
            STGVOBJ_Picture_t * DecimatedRepeatFieldPicture_p;
        } ToBeDecodedRasterPicture;

        /* raster buffers semaphore */
        /* mutex_t *               RasterBuffersMutex_p; */

    } RasterBufManagerParams;
#endif /* end of def STVID_STVAPI_ARCHITECTURE */

} VIDDEC_Data_t;

/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */

/* StartCode should be U8 ! */
#define PushStartCodeCommand(DecodeHandle, StartCode) vidcom_PushCommand(&(((VIDDEC_Data_t *) (DecodeHandle))->StartCodeBuffer), ((U8) (StartCode)))
#define PopStartCodeCommand(DecodeHandle, StartCode_p) vidcom_PopCommand(&(((VIDDEC_Data_t *) (DecodeHandle))->StartCodeBuffer), ((U8 *) (StartCode_p)))

#define PushControllerCommand(DecodeHandle, U8Var) vidcom_PushCommand(&(((VIDDEC_Data_t *) (DecodeHandle))->CommandsBuffer), ((U8) (U8Var)))
#define PopControllerCommand(DecodeHandle, U8Var_p) vidcom_PopCommand(&(((VIDDEC_Data_t *) (DecodeHandle))->CommandsBuffer), ((U8 *) (U8Var_p)))

#define PushInterruptCommand(DecodeHandle, U32Var) vidcom_PushCommand32DoOrValueIfFull(&(((VIDDEC_Data_t *) (DecodeHandle))->InterruptsBuffer), ((U32) (U32Var)))
#define PopInterruptCommand(DecodeHandle, U32Var_p) vidcom_PopCommand32(&(((VIDDEC_Data_t *) (DecodeHandle))->InterruptsBuffer), ((U32 *) (U32Var_p)))


/* Exported Functions ------------------------------------------------------- */

void viddec_HWDecoderEndOfDecode(const VIDDEC_Handle_t DecodeHandle);
void viddec_HWDecoderFoundNextPicture(const VIDDEC_Handle_t DecodeHandle);
void viddec_HWDecoderStartOfDecode(const VIDDEC_Handle_t DecodeHandle);

void viddec_DecodeHasErrors(const VIDDEC_Handle_t DecodeHandle, const VIDDEC_DecodeErrorType_t DecodeErrorType);

BOOL viddec_IsEventToBeNotified(const VIDDEC_Handle_t DecodeHandle, const U32 Event_ID);

void viddec_SetBitBufferThresholdToIdleValue(const VIDDEC_Handle_t DecodeHandle);
void viddec_UnMaskBitBufferFullInterrupt(const VIDDEC_Handle_t DecodeHandle);


/* Private Comment -------------------------------------------------------------------- */
/* Error_Recovery_Case_n01                                                              */
/* Description  : The frame rate of the sequence is not supported.                      */
/* Flag         : HasCriticalSequenceSyntaxErrors                                       */
/* Action       : CONTROLLER_COMMAND_RESET after the process of start code.             */

/* Error_Recovery_Case_n02                                                              */
/* Description  : The dimensions for the sequence don't correspond to the MPEG Profile. */
/* Flag         : HasProfileErrors                                                      */
/* Action       : CONTROLLER_COMMAND_RESET after the process of start code.             */

/* Error_Recovery_Case_n03                                                              */
/* Description  : No picture coding extension in MPEG2.                                 */
/* Flag         : HasPictureSyntaxErrors                                                */
/* Action       : LaunchSkipWhenPossible when previous start code is the first slice.   */

/* Error_Recovery_Case_n04                                                              */
/* Description  : Missing expected 2nd field.                                           */
/* Flag         : PreviousPictureHasAMissingField                                       */
/* Action       : LaunchSkipWhenPossible when previous start code is the first slice.   */

/* Error_Recovery_Case_n05                                                              */
/* Description  : Inconsistency in start code arrival.                                  */
/* Flag         : HasStartCodeParsingError                                              */
/* Action       : HasPictureSyntaxErrors during process of start code.                  */

/* Error_Recovery_Case_n06                                                              */
/* Description  : Picture coding type or picture structure invalid.                     */
/* Flag         : HasPictureSyntaxErrors                                                */
/* Action       : LaunchSkipWhenPossible when previous start code is the first slice.   */

/* Error_Recovery_Case_n07                                                              */
/* Description  : Check Marker bits.                                                    */
/* Flag         : none                                                                  */
/* Action       : BadMarkerBit                                                          */

/* Error_Recovery_Case_n08                                                              */
/* Description  : Corruption in macroblock.                                             */
/* Flag         : Hardware interrupt (DECODING_SYNTAX_ERROR).                           */
/* Action       : CONTROLLER_COMMAND_DECODING_ERROR under IT.                           */

/* Error_Recovery_Case_n09                                                              */
/* Description  : Wrong number of slice .                                               */
/* Flag         : Hardware interrupt (DECODING_OVERFLOW or DECODING_UNDERFLOW).         */
/* Action       :                                                                       */

/* Error_Recovery_Case_n10                                                              */
/* Description  : Impossible to get an unused frame buffer.                             */
/* Flag         :                                                                       */
/* Action       : Skip the picture.                                                     */

/* Error_Recovery_Case_n11                                                              */
/* Description  : Overlapping pictue start codes.                                       */
/* Flag         :                                                                       */
/* Action       :                                                                       */


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_DECODE_COMMON_H */

/* End of vid_dec.h */
