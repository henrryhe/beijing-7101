/*******************************************************************************

File name   : vid_prod.h

Description : Video producer private header file

COPYRIGHT (C) STMicroelectronics 2004.

Date               Modification                                     Name
----               ------------                                     ----
26 Jan 2004        Created from decoder module                       HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_PRODUCER_COMMON_H
#define __VIDEO_PRODUCER_COMMON_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "stvid.h"

#include "vid_ctcm.h"
#include "buffers.h"
#include "producer.h"
#include "vidcodec.h"
#include "stevt.h"

#ifdef ST_avsync
#include "stclkrv.h"
#endif /* ST_avsync */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
#if defined(VALID_TOOLS) && defined(TRACE_UART)

#define TRACE_PRODUCER_COLOR (TRC_ATTR_BACKGND_BLACK | TRC_ATTR_FORGND_RED | TRC_ATTR_BRIGHT)

#define VIDPROD_TRACE_BLOCKID   (STVID_DRIVER_ID)
enum
{
    VIDPROD_TRACE_GLOBAL = 0,
    VIDPROD_TRACE_INIT_TERM,
    VIDPROD_TRACE_RESET,
    VIDPROD_TRACE_DISPLAY_INSERT,
    VIDPROD_TRACE_PRESENTATION,
    VIDPROD_TRACE_PARSER_START,
    VIDPROD_TRACE_PARSER_JOB_END,
    VIDPROD_TRACE_PARSING_ERRORS,
    VIDPROD_TRACE_BBL,
    VIDPROD_TRACE_INFORM_READ_ADDR,
    VIDPROD_TRACE_DECODER_START,
    VIDPROD_TRACE_DECODER_CONFIRM,
    VIDPROD_TRACE_DECODER_JOB_END,
    VIDPROD_TRACE_DECODER_PROCESSING,
    VIDPROD_TRACE_DECODING_TIMEOUT,
    VIDPROD_TRACE_DECODING_ERRORS,
    VIDPROD_TRACE_PROCESSING_TIME,
    VIDPROD_TRACE_DPB_MANAGEMENT,
    VIDPROD_TRACE_DPB_STATUS
};
#endif /* defined(VALID_TOOLS) && defined(TRACE_UART) */
#ifdef STVID_TRICKMODE_BACKWARD
#define MAX_BACKWARD_PICTURES                   50 /*((21170320 / 2)/ 960000)*/ /*960000b/pict = 24Mb/s /25pict./s*/
#endif /*STVID_TRICKMODE_BACKWARD*/

#define CONTROLLER_COMMANDS_BUFFER_SIZE         10
#define MAX_NUMBER_OF_DECODE_PICTURES           (VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS + 4)

/* TODO_CL : this limit should be a variable coming from codecinfo, not hard coded */
#define MAX_NUMBER_OF_PREPROCESSING_PICTURES    3
#ifdef ST_speed
#define MAX_NUMBER_OF_PARSED_PICTURES           10
#endif /* ST_speed*/
#define MAX_NUMBER_OF_COMPLETED_PICTURES        4 /* Size of decoder job completed queue */

#ifdef STVID_TRICKMODE_BACKWARD
#define MAX_CURRENT_PICTURES                    (MAX_NUMBER_OF_DECODE_PICTURES+MAX_NUMBER_OF_PREPROCESSING_PICTURES+(2*MAX_BACKWARD_PICTURES))
#else
#define MAX_CURRENT_PICTURES                    (MAX_NUMBER_OF_DECODE_PICTURES+MAX_NUMBER_OF_PREPROCESSING_PICTURES)
#endif /* STVID_TRICKMODE_BACKWARD */

/* This value is the max number of references that we will store in a table in
 * order to compute later the picture ID of each picture */
#define STORED_PICTURE_BUFFER_SIZE              10

enum
{
    VIDPROD_SEQUENCE_INFO_EVT_ID,
    VIDPROD_USER_DATA_EVT_ID,
    VIDPROD_DATA_OVERFLOW_EVT_ID,
    VIDPROD_DATA_UNDERFLOW_EVT_ID,
    VIDPROD_READY_TO_DECODE_NEW_PICTURE_EVT_ID,
    VIDPROD_DATA_ERROR_EVT_ID,
    VIDPROD_PICTURE_DECODING_ERROR_EVT_ID,
    VIDPROD_STOPPED_EVT_ID,
    VIDPROD_NEW_PICTURE_SKIPPED_NOT_REQUESTED_EVT_ID,
    VIDPROD_NEW_PICTURE_DECODED_EVT_ID,
    VIDPROD_DECODE_ONCE_READY_EVT_ID,
    VIDPROD_RESTARTED_EVT_ID,
    VIDPROD_DECODE_LAUNCHED_EVT_ID,
    VIDPROD_SKIP_LAUNCHED_EVT_ID,
    VIDPROD_BUFFER_FULLY_PARSED_EVT_ID,
    VIDPROD_DECODE_FIRST_PICTURE_BACKWARD_EVT_ID,
#ifdef STVID_HARDWARE_ERROR_EVENT
    VIDPROD_HARDWARE_ERROR_EVT_ID,
#endif /* STVID_HARDWARE_ERROR_EVENT */
    VIDPROD_PICTURE_SKIPPED_EVT_ID,
    VIDPROD_NB_REGISTERED_EVENTS_IDS
};

/* No more than 128 commands: it needs to be casted to U8 */
typedef enum ControllerCommandID_e {
    CONTROLLER_COMMAND_RESET,               /* Ask controller to perform reset (error concealment, channel change ...) */
    CONTROLLER_COMMAND_SEARCH_PICTURE,      /* Ask controller to launch search for next picture (or first one if after reset) */
    CONTROLLER_COMMAND_NEW_PARSED_PICTURE,  /* New parsed picture to process received from PARSER */
    CONTROLLER_COMMAND_NEW_DECODED_PICTURE, /* Decoded picture received from DECODER */
    CONTROLLER_COMMAND_CONFIRM_PICTURE_BUFFER,  /* DECODER asked to confirm a frame buffer */
    CONTROLLER_COMMAND_PARSER_PICTURE_SKIPPED, /* A picture has been skipped by the parser */
    CONTROLLER_COMMAND_DECODER_READ_ADDRESS_UPDATED, /* DECODER sent its last read address, transmit it to PARSER if needed */
    CONTROLLER_COMMAND_STOP,                /* Ask controller to perform stop (halts the parser and finish current decode according to stop mode requested) */
    CONTROLLER_COMMAND_CHANGE_DIRECTION,
    NB_CONTROLLER_COMMANDS                  /* Keep this one as the last one */
} ControllerCommandID_t;

typedef enum VIDPROD_ProducerState_e
{
    VIDPROD_DECODER_STATE_DECODING,
    VIDPROD_DECODER_STATE_STOPPED
} VIDPROD_ProducerState_t;

typedef enum VIDPROD_ErrorType_e
{
    VIDPROD_DECODE_ERROR_TYPE_NONE,         /* Default. */
    VIDPROD_DECODE_ERROR_TYPE_SYNTAX,       /* inferred by DECODER_ERROR_PICTURE_SYNTAX error */
    VIDPROD_DECODE_ERROR_TYPE_UNDERFLOW,    /* inferred by DECODER_ERROR_MISSING_SLICE error */
    VIDPROD_DECODE_ERROR_TYPE_CORRUPTED,    /* inferred by DECODER_ERROR_CORRUPTED error */
    VIDPROD_PARSE_ERROR_TYPE_SYNTAX,        /* inferred by PARSER_ERROR_STREAM_SYNTAX error */
    VIDPROD_DECODE_ERROR_TYPE_TIMEOUT       /* inferred by pre processing or decode timeout detected by the producer */
} VIDPROD_ErrorType_t;

/* Exported Constants ------------------------------------------------------- */

extern const VIDPROD_FunctionsTable_t   VIDPROD_Functions;

/* Exported Types ----------------------------------------------------------- */

typedef struct BitRateValueEvaluate_s
{
    /* flags */
    BOOL BitRateValueIsComputable;                   /* If Trick mode is requiring jumps on HDD, Bit Rate Value can not be calculated */
    BOOL BitRateValueIsUnderComputing;
    /* bit rate value calculated by hand */
    U32  Time[VIDPROD_NB_PICTURES_USED_FOR_BITRATE];
    U32  BitBufferSize[VIDPROD_NB_PICTURES_USED_FOR_BITRATE];
    U32  LastBitBufferOutputCounter;                 /* in bytes units */
    /* counters */
    U8   BitRateIndex;
    U8   BitRateValidValueNb;
} BitRateValueEvaluate_t;
#ifdef STVID_TRICKMODE_BACKWARD
/* 3 manners for having a bit buffer flushed  */
typedef enum VIDPROD_FlushBitBufferManner_e
{
    VIDPROD_FLUSH_WITH_NORMAL_OVERLAP,       /* normal bit buffer flush  */
    VIDPROD_FLUSH_BBFSIZE_BACKWARD_JUMP,     /* flush the bit buffer with a jump corresponding to the size of the bit buffer */
                                              /* happens when first flush required after a negative speed or if */
                                              /* spurious hardware reporting for example a wrong size of injected datas */
    VIDPROD_FLUSH_BECAUSE_OF_NO_I_PICTURE    /* flush the same bit buffer with a jump half of bit buffer */
}  VIDPROD_FlushBitBufferManner_t;

typedef struct VIDPROD_Parsedpictures_s
{
    VIDBUFF_PictureBuffer_t     PreprocessingPictureBufferTable[MAX_BACKWARD_PICTURES];
    BOOL                        IsPictureBufferUsed[MAX_BACKWARD_PICTURES];
    U32                         NbUsedPictureBuffers;
    /* PC : Next table has been added to avoid multiple timeout messages on console/trace for the same picture */
    BOOL                        IsPictureTimedOut[MAX_BACKWARD_PICTURES];
    U16                          NbParsedPicturesLeft;
    BOOL                        BitBufferFullyParsed;
}VIDPROD_Parsedpictures_t;

typedef struct VIDPROD_LinearBitBuffer_s
{
   /* whole buffer */
   void *                               WholeBufferStartAddress_p;
   /*U32                                  WholeBufferSize;*/
   void *                               WholeBufferEndAddress_p;
   /* Data transfers limits */
   U32                                  TransferDataMaxSize;
   void *                               TransferDataMaxEndAddress_p;
   /* about last tranfert */
   U32                                  TransferedDataSize;
   void *                               TransferedDataEndAddress_p;
   /* Valid Data buffer */
   void *                               ValidDataBufferStartAddress_p;
   U32                                  ValidDataBufferSize;
   void *                               ValidDataBufferEndAddress_p;
   /* the other linear bit buffer */
   struct VIDPROD_LinearBitBuffer_s   * AssociatedLinearBitBuffer_p;
   VIDPROD_Parsedpictures_t             PreprocessingPicture;
   /* False while relevant informations have not been found during the parsing of the buffer. */
   BOOL                                 ParsingBeginOfRelevantDataFound;
   /* flag telling if first picture of bit buffer has been parsed. */
   /* It allows first picture discard if it is a first field */
   BOOL                                 FirstPictureOfLBBFFound;
   /* Number of picture in the bit buffer */
   U32                                  NbPicturesInside;
   U32                                  NumberOfBytesToDiscardAtTheEndOfTheBuffer;
   /* Discontinuity with the previous linear bitbuffer buffer */
   BOOL                                 DiscontinuityWithPreviousBuffer;
   U32                                  BeginningOfInjectionBitrateValue;
   U32                                  InjectionDuration;
   VIDBUFF_PictureBuffer_t *			FirstIPictureInBuffer_p;
   U32									Index;
   U32									NbIPicturesInBuffer;
   U32									NbNotIPicturesInBuffer;
   U32									AverageNbPicturesBetweenTwoI;
   BOOL                                 IsFirstPictureDecoded;
   U32                                  TotalNbIFields;
   U32                                  TotalNbNotIFields;
} VIDPROD_LinearBitBuffer_t;

#endif  /*STVID_TRICKMODE_BACKWARD*/

/* Type for internal structures for the decoder */
typedef struct VIDPROD_Data_s {
    ST_Partition_t *    CPUPartition_p;
    STEVT_Handle_t      EventsHandle;
    STEVT_EventID_t     RegisteredEventsID[VIDPROD_NB_REGISTERED_EVENTS_IDS];
    VIDBUFF_Handle_t    BufferManagerHandle;
    VIDQUEUE_Handle_t   OrderingQueueHandle;
    PARSER_Handle_t     ParserHandle;
    DECODER_Handle_t    DecoderHandle;
    DECODER_CodecInfo_t CodecInfo;
    STVID_CodecMode_t   CodecMode;

#ifdef ST_Mpeg2CodecFromOldDecodeModule /* Temporary for quick mpeg2 codec implementation based on viddec */
    VIDPROD_Handle_t    VIDDEC_Handle;
#endif

#ifdef ST_inject
    VIDINJ_Handle_t     InjecterHandle;
#endif /* ST_inject */
    STVID_DeviceType_t  DeviceType;
    U8                  DecoderNumber;  /* As defined by DECODER API. */
    U32                 ValidityCheck;

    VIDBUFF_BufferAllocationParams_t BitBuffer; /* Used to allocate bit buffer, or to store Size and Address_p given by user at init ! */
    BOOL                BitBufferAllocatedByUs;
    U32                 LastBitBufferLevel;

#if defined(DVD_SECURED_CHIP)
    VIDBUFF_BufferAllocationParams_t ESCopyBuffer; /* Used to allocate ES Copy buffer, or to store Size and Address_p given by user at init ! */
    BOOL                ESCopyBufferAllocatedByUs;
#endif /* DVD_SECURED_CHIP */

    VIDPROD_ProducerState_t ProducerState;
    BOOL                    DecoderIsFreezed;
    STVID_DecodedPictures_t DecodedPictures;
    STVID_DecodedPictures_t DecodedPicturesBeforeLastFreeze;
#ifdef STVID_TRICKMODE_BACKWARD
    /* Linear Bit Buffers */
    struct
    {
        VIDPROD_LinearBitBuffer_t      BufferA;
        VIDPROD_LinearBitBuffer_t      BufferB;
        VIDPROD_LinearBitBuffer_t *    NextBufferToBeTransfered_p;
        VIDPROD_LinearBitBuffer_t *    BufferUnderTransferandParsing_p;
        VIDPROD_LinearBitBuffer_t *    BufferUnderDecode_p;
        semaphore_t *                  PreprocessingPictureSemaphore_p; /* semaphore to protect PreprocessingPictureBufferTable use */
    } LinearBitBuffers;
    struct
    {
        BOOL                         IsBackward;
        BOOL                         InjectDiscontinuityCanBeAccepted;
        BOOL                         DoNotTreatParserJobCompletedEvt;
        U32                          LevelAndProfile;
        BOOL                         LastPictureDecoded;
        BOOL                         WaitingProducerReadyToSwitchBackward;
        BOOL                         WaitingProducerReadyToSwitchForward;
        DecodeDirection_t            ChangingDirection;
        BOOL                         JustSwitchForward;
        U32                          NbDecodedPictures;
        osclock_t                    DecodeStartTime;
        osclock_t                    InjectionStartTime;
        U32                          BufferDecodeTime;
        U32                          InjectionTime;
        U32                          TimeToDecodeIPicture;
        BOOL                         IsPreviousInjectionFine;
        BOOL                         IsInjectionNeeded;
    } Backward;
#endif /*STVID_TRICKMODE_BACKWARD*/
#ifdef ST_speed
    S32                          DriftTime;
    BOOL                    IsBitRateComputable;
#endif
    BOOL                    NeedToDecreaseBBL;  /* 2 states automaton used to prevent Bit Buffer Level Overflow */
    STVID_DataUnderflow_t   DataUnderflow; /* Updated when monitoring bit buffer level, uesd to send data underflow event */
    BOOL                    DataUnderflowEventNotSatisfied; /* Used not to send data underflow event continuously but only when detecting underflow */
    BOOL                    OverFlowEvtAlreadySent; /* used to send only one OVERFLOW evt once the overflow threshold has been reached */

    /* Decode events notification configuration ----------------------------- */
    struct
    {
        STVID_ConfigureEventParams_t ConfigureEventParam;    /* Contains informations : Event enable/disable notification
                                                            and Number of event notification to skip before notifying the event. */
        U32                          NotificationSkipped;    /* Number of notification skipped since the last one. */
    } EventNotificationConfiguration [VIDPROD_NB_REGISTERED_EVENTS_IDS];

    /* Controller ----------------------------------------------------------- */
    VIDCOM_Task_t       ProducerTask;
    semaphore_t *       ProducerOrder_p;      /* To synchronise orders in the task */

    /* Buffers of commands: 2 for each decoder */
    U8 CommandsData[CONTROLLER_COMMANDS_BUFFER_SIZE];
    CommandsBuffer_t    CommandsBuffer;                 /* Data concerning the decode controller commands queue */

    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping; /* Needed to translate data1 and data2 in NEW_PICTURE_DECODED_EVT */

    VIDPROD_UpdateDisplayQueue_t UpdateDisplayQueue;
    BOOL DecodeOnce;
    BOOL RealTime;

    /* Used to send STVID_SEQUENCE_INFO_EVT */
    BOOL                    SendSequenceInfoEventOnceAfterStart;
    STVID_SequenceInfo_t    PreviousSequenceInfo;

    struct
    {
        BOOL        WhenNextIPicture;
        BOOL        WhenNextReferencePicture;
        BOOL        WhenNoData;
        BOOL        StopAfterBuffersTimeOut; /* Stop decoder if no free buffer found after a timeout */
        BOOL        DiscontinuityStartCodeInserted;
        BOOL        AfterCurrentPicture;
        BOOL        IsPending;
        semaphore_t * ActuallyStoppedSemaphore_p; /* semaphore to synchronize VIDPROD_Stop() function exit and actual stop in producer */
        STOS_Clock_t MaxWaitStopTime;
    } Stop;

    U32                 UnsuccessfullDecodeCount;

    /* Decoder -------------------------------------------------------------- */

    struct
    {
        U32                         NbUsedPictureBuffers;  /* Number of used entries in DecodePictureBuffer table */
        VIDBUFF_PictureBuffer_t *   DecodePictureBuffer[MAX_NUMBER_OF_DECODE_PICTURES]; /* Contains pointers on pictures used by decoder */
                                                                                        /* Currently decoding pictures and reference pictures */
                                                                                        /* these pointers are retrieved from VIDBUFF */
        semaphore_t *               DecodePictureSemaphore_p; /* semaphore to protect DecodePictureBuffer use */
/* PC : Next table has been added to avoid multiple timeout messages on console/trace for the same picture */
        BOOL                        IsPictureTimedOut[MAX_NUMBER_OF_DECODE_PICTURES];
    } DecodePicture;

    struct
    {
/* These are allocated by VIDPROD and are used to store picture descriptions between */
/* PARSER_JOB_COMPLETED_EVT and DECODER_CONFIRM_BUFFER_EVT, at the latter event, */
/* an other descriptor is retrieved from VIDBUFF along with an available FrameBuffer */
/* NOTE : In final implementation, VIDBUFF should be called at decoder start to get a */
/* PictureBuffer with available FrameBuffer and then confirmed if DECODER_CONFIRM_BUFFER_EVT */
/* occurs before DECODER_JOB_COMPLETED_EVT */
        VIDBUFF_PictureBuffer_t     PreprocessingPictureBufferTable[MAX_NUMBER_OF_PREPROCESSING_PICTURES];
        BOOL                        IsPictureBufferUsed[MAX_NUMBER_OF_PREPROCESSING_PICTURES];
        U32                         NbUsedPictureBuffers;
        semaphore_t *               PreprocessingPictureSemaphore_p; /* semaphore to protect PreprocessingPictureBufferTable use */
/* PC : Next table has been added to avoid multiple timeout messages on console/trace for the same picture */
        BOOL                        IsPictureTimedOut[MAX_NUMBER_OF_PREPROCESSING_PICTURES];
    } PreprocessingPicture;
#ifdef ST_speed
    struct
    {
        VIDBUFF_PictureBuffer_t     ParsedPictureTable[MAX_NUMBER_OF_PARSED_PICTURES];
        U32                         NbUsedPictureBuffers;
        U32                         ReadTableIndex;
        U32                         WriteTableIndex;
        BOOL                        PreviousPictureWasI;
        U8                          Loop;
    } ParsedPicture;
#endif

    struct
    {
        void *                      LastPictureSkippedStopAddress_p;
        BOOL                        IsPictureSkipJustHappened ;
        BOOL                        IsSearchingPicture;
        osclock_t                   MaxPictureSearchTime;
        STVID_StreamType_t          StreamType;       /* PES Stream Type (ES/PES ...) */
        U8                          StreamID;       /* PES Stream ID */
        BOOL                        SearchNextRandomAccessPoint;
    } ParserStatus;

    struct
    {
        U32                         NbPicturesInQueue;
        void *                      LastReadAddress_p;          /* Last address read by decoder (end of last decoded picture in bit buffer)*/
        VIDBUFF_PictureBuffer_t *   LastDecodedPicture_p;       /* Remembers the last decoded picture */
        osclock_t                   MaxBufferWaitingTime;       /* Max allowed end time for getting a free buffer for decode when STOP_END_OF_DATA */
        osclock_t                   MaxErrorRecoveryBufferWaitingTime; /* Max allowed end time for getting a free buffer for normal (real time & non real time) decode */
        VIDBUFF_PictureBuffer_t     *CurrentPictures_p[MAX_CURRENT_PICTURES];   /* List (FIFO) of currently decoding pictures in decoding order (pointers on PictureBuffers) */
        U32                         NextOutputIndex;                                        /* Index of next picture to be removed from list */
        U32                         NbIndices;                                              /* Number of pictures in list */
                                                                                            /* input at Parser job completed, output at decoder job completed */
        BOOL                        IsPicturePushStarted;       /* Indicates that first picture after DPB reset has been pushed to display */
        U32                         FrameBufferUsedSinceLastRAP;/* Used to start the display process in combination with IsPicturePushStarted */
        U32                         CurrentMaxDecFrameBuffering;
    } DecoderStatus;

    /* Parser --------------------------------------------------------------- */
    STVID_BroadcastProfile_t    BroadcastProfile;

    BOOL                        UserDataEventEnabled;

    struct
    {
        BOOL LaunchDisplayWhenPossible;
        U32  LaunchDecodeWhenPossible;
        BOOL LaunchSkipWhenPossible;
        U32  ConfirmBufferWhenPossible;
        U8 NumberOfDisplayToLaunch;  /* NumberOfDisplayToLaunch is there to check the 2 fields of a P are existing before allowing     */
                                     /* the insertion on display. The reason comes from the MPEG Standard : the 2nd field of P has its */
                                     /* 1st field as 2nd field of reference. Otherwise when the 1st field of a P is missing (ex: PT)   */
                                     /* we insert a macroblocked picture (ex: PB). Macroblocked because decoded from a bad reference.  */
    } NextAction;

    STVID_ErrorRecoveryMode_t       ErrorRecoveryMode;

    struct
    {
        osclock_t                               MaxWaitOrderTime;
        BOOL                                    NoDisplayAfterNextDecode;
        BOOL                                    SoftResetPending;
        BOOL                                    SoftResetPosted;
        VIDBUFF_PictureBuffer_t *               NewParsedPicture_p;
        VIDBUFF_PictureBuffer_t *               PictureToDecode_p;
        DECODER_DecodingJobResults_t            PictureToConfirmResult[MAX_NUMBER_OF_PREPROCESSING_PICTURES];
        U32                                     NextIndexToConfirm;
        U32                                     NbIndexToConfirm;
        U32                                     CurrentCommandIDToConfirm;
        VIDBUFF_PictureBuffer_t *               PictureToConfirm_p;
        U32                                     PreprocessingPictureNumber;
        DECODER_DecodingJobResults_t            CompletedPictureResults[MAX_NUMBER_OF_COMPLETED_PICTURES];
        U32                                     NextCompletedIndex;
        U32                                     NbCompletedIndex;
        DECODER_DecodingJobResults_t            NewPictureDecodeResults;
        VIDBUFF_PictureBuffer_t *               LastDecodedPicture_p;
        BOOL                                    NeedForUpdateReferenceList;
        VIDPROD_NewPictureDecodeOrSkip_t        NewPictureDecodeOrSkipEventData;
        S32                                     NbPicturesOutOfDisplay;
        BOOL                                    NeedForExtraAllocation; /* used for dynamic reallocation in H264 in due to dynamic changes in stream resolution or profile */
        BOOL                                    NeedForExtraAllocationDecimated; /* used for dynamic reallocation in H264 in due to dynamic changes in stream resolution or profile */
        BOOL                                    MaxSizeReachedForExtraAllocation; /* used for dynamic reallocation in H264 in due to dynamic changes in stream resolution or profile */
        BOOL                                    MaxSizeReachedForExtraAllocationDecimated; /* used for dynamic reallocation in H264 in due to dynamic changes in stream resolution or profile */
        U8                                      DynamicAllocatedMainFrameBuffers; /* Number of actually allocated main frame buffers with dynamic allocation. Neede for trigger picture push to display */
        BOOL                                    OnePictureHasAMissingField;
        U32                                     MissingFieldPictureDecodingOrderFrameID;

#ifdef STVID_TRICKMODE_BACKWARD
		BOOL									NewLinearBB;
		U32										NbIPicturesInBuffer;
#endif
        VIDCOM_InternalProfile_t                CurrentProfile;
	} ForTask;
    struct
    {
        BOOL    PresentationFreezed;
    } AsynchronousStatus;

    VIDPROD_DecoderOperatingMode_t              DecoderOperatingMode;

    struct
    {
        BOOL                  IsDecodeEnable; /* used to delay the first decode after VID_Start while the bit buffer level has not reached the vbv size */
                                              /* set FALSE at SoftReset in real time and TRUE when bit buffer level has reached its vbv size. otherwise set TRUE */

        BOOL                  FirstPictureAfterReset;

        /* Start on picture delay flags */
        BOOL                  StartOnPictureDelay;
        U32                   PictureDelayTimerExpiredDate;
        /* Start on Bit buffer fullness flags */
        BOOL                  StartWithBufferFullness;
        STOS_Clock_t          MaxWaitForBufferLevel;
        U32                   BufferFullnessToReachBeforeStartInBytes;      /* it is the vbv_buffer_size in the stream minor an amount of data (threshold to guarantee we don't reach the BBF */
/*        BOOL                  VbvSizeReachedAccordingToInterruptBBT; *//* Set FALSE at SoftReset, and TRUE when the first BBF interrupt occurs. So checking */
                                                                     /* that it is FALSE tells that we are still waiting for vbv_size to be reached i bit buffer */
        /* PTS comparison time flag */
        BOOL                  StartWithPTSTimeComparison;
        STVID_PTS_t           FirstPicturePTSAfterReset;
#ifdef ST_avsync
        STCLKRV_Handle_t      STClkrvHandle;
#endif /* ST_avsync */
    } StartCondition;

    STVID_HDPIPParams_t     HDPIPParams;    /* HDPIP parameters.    */

#ifdef STVID_DEBUG_GET_STATISTICS
    STVID_Statistics_t      Statistics;
#endif /* STVID_DEBUG_GET_STATISTICS */
    U32 CurrentPictureNumber;

    void                  * VidMappedBaseAddress_p; /* Address where will be mapped the driver registers */

	BitRateValueEvaluate_t BitRateValueEvaluate;
	/*MPEG2BitStream_t * DecodingStream_p; *//* Pointer to a stream structure (allocated somewhere else like at init) */

    VIDBUFF_PictureBuffer_t     * StoredPictureBuffer[STORED_PICTURE_BUFFER_SIZE];
    U32                           StoredPictureCounter;
    U32                           PreviousPictureCount;
    U8                            NumOfPictToWaitBeforePush;
    VIDCOM_PictureID_t            CurrentPresentationOrderID;

    #ifdef ST_speed
    BOOL                         SearchFirstI;
#endif
    struct
    {
        BOOL        AllIPictures;                /* Normally always FALSE. Could be true when streams with no I detected ? */
        BOOL        AllPPictures;                /* TRUE when decode mode requires slip of all P pictures */
        BOOL        AllBPictures;                /* TRUE when decode mode requires slip of all B pictures */
        BOOL        NextBPicture;                /* TRUE to skip next B picture */
#ifdef ST_speed
        BOOL        AllPPicturesSpeed;                /* TRUE to skip all P picture */
        BOOL        AllBPicturesSpeed;                /* TRUE to skip  all B picture */
        BOOL        NextIPicture;                     /* TRUE to skip next I picture */
		U32			AllPicturesNb;
		BOOL		UntilNextIPictureSpeed;
        BOOL        AllExceptFirstI;
        BOOL        UnderflowOccured;
#endif /* ST_speed */
        BOOL        ParserAutoUpdatePointeur;
        BOOL        UntilNextIPicture;          /* TRUE to skip all pictures until next I picture */
        BOOL        UntilNextSequence;  /* TRUE to skip all pictures until next sequence */
        BOOL        UntilNextRandomAccessPoint;  /* TRUE to skip all pictures until next DBP flush */
        BOOL        UntilNextRandomAccessPointJustRequested;
        BOOL        RecoveringNextEntryPoint;    /* TRUE while we are skipping until the next recovery point due to producer's error recovery actions */
        VIDPROD_SkipMode_t SkipMode;
    } Skip;

    VIDBUFF_PictureBuffer_t *   ReplacementReference_p;    /* Used as a remplacement for a missing reference. It is created in STVID_Clear */
    BOOL                        ReplacementReference_IsAvailable;
    U32                         LastParsedPictureDecodingOrderFrameID;
    VIDCOM_PictureID_t          LastParsedPictureExtendedPresentationOrderPictureID;
} VIDPROD_Data_t;

/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */

#define PushControllerCommand(ProducerHandle, U8Var) vidcom_PushCommand(&(((VIDPROD_Data_t *) (ProducerHandle))->CommandsBuffer), ((U8) (U8Var)))
#define PopControllerCommand(ProducerHandle, U8Var_p) vidcom_PopCommand(&(((VIDPROD_Data_t *) (ProducerHandle))->CommandsBuffer),((U8 *) (U8Var_p)))

/* Exported Functions ------------------------------------------------------- */

#ifdef VALID_TOOLS
ST_ErrorCode_t vidprod_RegisterTrace(void);
#endif /* VALID_TOOLS */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_PRODUCER_COMMON_H */

/* End of vid_prod.h */
