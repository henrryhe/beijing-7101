/*******************************************************************************

File name   : vid_tric.h

Description : Video trick mode common header file

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
 7 Sept 2000       Created                                          PLe
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_TRICKMOD_COMMON_H
#define __VIDEO_TRICKMOD_COMMON_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "decode.h"
#include "display.h"
#include "stevt.h"
#include "stavmem.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

enum
{
    VIDTRICK_SPEED_DRIFT_THRESHOLD_EVT_ID = 0,
    VIDTRICK_DISPLAY_PARAMS_CHANGE_EVT_ID,  /* Careful: this event should not be notified inside a VIDTRICK API call from API module, because API module subscribes to this event and this would lead to a block because of semaphore lock ! */
    VIDTRICK_NB_REGISTERED_EVENTS_IDS /* always keep as last one to count event ID's */
};

/* Exported Types ----------------------------------------------------------- */
/******************************************************************************************/
/******* Start of the Stub to compile trickmode while DECODE not fully implemented. *******/
/******************************************************************************************/
#ifdef ST_smoothbackward

/* Trickmode different video control modes */
typedef enum VIDTRICK_ControlMode_e
{
    VIDTRICK_CONTROLMODE_AUTOMATIC,
    VIDTRICK_CONTROLMODE_MANUAL
} VIDTRICK_ControlMode_t;

/* 3 manners for having a bit buffer flushed  */
typedef enum VIDTRICK_FushBitBufferManner_e
{
    VIDTRICK_FLUSH_WITH_NORMAL_OVERLAP,       /* normal bit buffer flush  */
    VIDTRICK_FLUSH_BBFSIZE_BACKWARD_JUMP,     /* flush the bit buffer with a jump corresponding to the size of the bit buffer */
                                              /* happens when first flush required after a negative speed or if */
                                              /* spurious hardware reporting for example a wrong size of injected datas */
    VIDTRICK_FLUSH_BECAUSE_OF_NO_I_PICTURE    /* flush the same bit buffer with a jump half of bit buffer */
}  VIDTRICK_FushBitBufferManner_t;

typedef struct VIDTRICK_FirstTemporalRefFound_s
{
    U16           TemporalRef;
    BOOL          TheNextFirstSliceIsTheFirstOne;
} VIDTRICK_FirstTemporalRefFound_t;

typedef struct VIDTRICK_MatchingPicture_s
{
   U32                                  ParsedBufferMatchingPictureNumber;
   U32                                  LastPictureIndex;
   struct VIDTRICK_PictureInfo_s     *  MatchingPicture_p;
} VIDTRICK_MatchingPicture_t;

typedef struct VIDTRICK_QuantiserMatrix_s
{
   QuantiserMatrix_t                    LumaIntraQuantMatrix;
   QuantiserMatrix_t                    LumaNonIntraQuantMatrix;
   QuantiserMatrix_t                    ChromaIntraQuantMatrix;
   QuantiserMatrix_t                    ChromaNonIntraQuantMatrix;
} VIDTRICK_QuantiserMatrix_t;


typedef struct VIDTRICK_PictureInfo_s
{
   /* necessary parameters  */
   void *                               AssociatedStartCodeAddress_p;
   BOOL                                 IsDecoded;
   BOOL                                 IsDecodable;
   BOOL                                 IsInDisplayQueue;
   BOOL                                 IsGoneOutOfDisplay;
   BOOL                                 HasBeenInsertedInDisplayQueue;
   /* 2 flags to notice that a reference picture  */
   BOOL                                 DisplayUnlockHasBeenRequired;
   BOOL                                 AnyBToDecodeUnlockHasBeenRequired;
   VIDDEC_PictureInfoFromDecode_t       PictureInfoFromDecode;
   /* PictureBuffer_p is the address of the VIDBUFF_PictureBuffer_t structure given by the */
   /* GetUnusedPictureBuffer function. It must be kept until the picture is no more decoded */
   VIDBUFF_PictureBuffer_t *            PictureBuffer_p;
   /* References pictures infos */
   struct VIDTRICK_PictureInfo_s *      BackwardOrLastBeforeIReferencePicture_p;
   struct VIDTRICK_PictureInfo_s *      ForwardOrNextReferencePicture_p;
   /* SecondFieldPicture */
   struct VIDTRICK_PictureInfo_s *      SecondFieldPicture_p;

   struct VIDTRICK_PictureInfo_s *      FirstDisplayablePictureAfterJump_p;

   /* Pictures around in the stream order (list). */
   struct VIDTRICK_PictureInfo_s *      PreviousInStreamPicture_p;
   U32                                  IndiceInBuffer;
   /* Linear bit buffer whom the picture belongs to */
   struct VIDTRICK_LinearBitBuffer_s *  LinearBitBufferStoredInfo_p;
} VIDTRICK_PictureInfo_t;


/* Structure with characteristic datas of every smooth backward linear bit buffer */
typedef struct VIDTRICK_LinearBitBuffer_s
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
   struct VIDTRICK_LinearBitBuffer_s *  AssociatedLinearBitBuffer_p;
   /* False while relevant informations have not been found during the parsing of the buffer. */
   BOOL                                 ParsingBeginOfRelevantDataFound;
   /* Allocation address of PictureInfoTableStartAddress_p array, must only be used for deallocation  */
   VIDTRICK_PictureInfo_t *             PictureInfoTableAllocationAddress_p;
   /* Start address of the array which contains all the characteristics about each picture found in the linear bit buffer */
   VIDTRICK_PictureInfo_t *             PictureInfoTableStartAddress_p;
   /* flag telling if first picture of bit buffer has been parsed. */
   /* It allows first picture discard if it is a first field */
   BOOL                                 FirstPictureOfLBBFFound;
   /* Number of picture in the bit buffer */
   U32                                  NbPicturesInside;
   U32                                  NbIFieldInside;
   U32                                  NbIPFieldInside;
   U32                                  NumberOfBytesToDiscardAtTheEndOfTheBuffer;
   struct VIDTRICK_PictureInfo_s *      LastDisplayablePicture_p;              /* first I picture of the bit buffer */
   /* Discontinuity with the previous linear bitbuffer buffer */
   BOOL                                 DiscontinuityWithPreviousBuffer;
   U32                                  BeginningOfInjectionBitrateValue;
   U32                                  InjectionDuration;
   U32                                  NbFieldsInside;
   U32                                  OneFieldAverageSize;
} VIDTRICK_LinearBitBuffer_t;
#endif /* #ifdef ST_smoothbackward */

typedef struct VIDTRICK_Data_s
{
    U32                                 ValidityCheck;
    ST_Partition_t *                    CPUPartition_p;
    U8                                  DecoderNumber;  /* As defined in vid_com.c. See definition there. */
    /* Handles */
    STEVT_Handle_t                      EventsHandle;
    VIDDEC_Handle_t                     DecodeHandle;
    VIDDISP_Handle_t                    DisplayHandle;
#ifdef ST_smoothbackward
    VIDBUFF_Handle_t                    BufferManagerHandle;
#endif /* #ifdef ST_smoothbackward */
    STAVMEM_PartitionHandle_t           AvmemPartitionHandle;
#ifdef ST_ordqueue
    VIDQUEUE_Handle_t                   OrderingQueueHandle;
#endif /* ST_ordqueue */
    /* Real Time Constraints */
    VIDCOM_Task_t                       TrickModeTask;
    semaphore_t *                       TrickModeParamsSemaphore_p;
    mutex_t *                           TrickModeParamsMutex_p;
    semaphore_t *                       WakeTrickModeTaskUp_p;

    /* Display params */
    VIDDISP_DisplayParams_t             DisplayParams;
    /* Registered events and data to reach asked speed if necessary */
    STEVT_EventID_t                     RegisteredEventsID[VIDTRICK_NB_REGISTERED_EVENTS_IDS];

    /* Trick mode management params */
    struct
    {
        S32                             VirtualClockCounter;
        S32                             NbFieldsIfPosToSkipIfNegToRepeat;
        S32                             Speed;
        S32                             RequiredSpeed;
        U32                             BitRateValue;   /* Unit used 400bits/s */
        U32                             FrameRateValue; /* Unit used = mHz     */
        STVID_SequenceInfo_t            LastSequenceInfo;
    } MainParams;

    /* Backward I only infos */
    struct
    {
        U32                             NbVsyncSinceLastSentDriftEvt;
    } BackwardIOnly;

    struct
    {
        BOOL                            RequiredSpeedTakenInAccount;
        BOOL                            MustSkipAllPPictures;
        BOOL                            SpeedControlEnabled;
        BOOL                            SecondFieldHasToBeSkipped;
        BOOL                            WaitingForASecondField;
#ifdef ST_smoothbackward
        BOOL                            DoNotTryToRestartOnTheLastDisplayedPicture;
        BOOL                            SmoothBackwardIsInitializing;
        BOOL                            BackwardToForwardInitializing;
        BOOL                            AtLeastOnePictureHasBeenInserted;
        BOOL                            DoNotMatchBufAWithBufB;
        BOOL                            LinearBitBuffersAreAllocated;
        BOOL                            SmoothBackwardSpeedHasBeenChanged;  /* remembers smooth backward speed changes. */
        BOOL                            FirstSliceStartCodeFoundInUnvalidArea;
        BOOL                            BackwardModeInitIsRequiredInTask;
#endif /* #ifdef ST_smoothbackward */
    } Flags;

#ifdef ST_smoothbackward
    struct
    {
        BOOL                            OnGoing;
        BOOL                            MustDecodeSecondField;
        STVID_PictureStructure_t        FailingDecodePictureStructure;
    } BackwardWarmReset;

    struct
    {
        BOOL                            TrickmodIsStopped;
        BOOL                            TrickmodStopIsPending;
        BOOL                            LinearBitBufferFlushRequirementAvailable;
        STVID_Freeze_t                  SmoothBackwardRequiredStopFreezeParams;
        STVID_Stop_t                    StopMode;
    } Stop;
#endif /* #ifdef ST_smoothbackward */

    /* Others infos */
    /* if I pictures are very near in the stream, then skipping some I picture is authorized */
    U32                                 NbTreatedPicturesSinceLastDecodedI;
#ifdef ST_smoothbackward
    U32                                 SCDAlignmentInBytes;
    U32                                 NbOfPicturesInToBeInsertedInDQ;
    U32                                 NbFieldsToBeInDisplayQueue; /* nb of pictures(fields) smooth backward has to have
                                                                     * continuously in display queue */
    /* array of last pictures present in display queue. */
    VIDTRICK_PictureInfo_t **           LastPictureInDisplayQueue_p;
    /* Picture To stop on if END OF DATA Stop is required */
    VIDTRICK_PictureInfo_t *            EndOfDataPicture_p;
    /* under Decoding picture infos */
    VIDTRICK_PictureInfo_t *            PictureUnderDecoding_p;
    VIDTRICK_MatchingPicture_t          MatchingPicture;

    /* Linear Bit Buffers */
    struct
    {
        VIDTRICK_LinearBitBuffer_t      BufferA;
        VIDTRICK_LinearBitBuffer_t      BufferB;
        VIDTRICK_LinearBitBuffer_t *    BufferUnderTransfer_p;
        VIDTRICK_LinearBitBuffer_t *    BufferUnderParsing_p;
        VIDTRICK_LinearBitBuffer_t *    NextBufferToBeTransfered_p;
    } LinearBitBuffers;

    /* when no matching picture is found or when speed is big : DisplayContinuity params */
    struct
    {
        U32                             NbFieldsToRecover;
        S32                             OverlapDuration;
    } Discontinuity;

    /* Error recovery */
    struct
    {
        U32                             NbVsyncElapsedSinceLastDecodeHasBeenLaunched;
        U32                             NbVSyncElapsedSinceLastSCDHasBeenLaunched;
        U32                             SuccessiveDecodeSecondFieldErrorsNb;
        U32                             SuccessiveDecodeFrameOrFirstFieldErrorsNb;
        U32                             NbVsyncElapsedWithoutAnyDecode;
        BOOL                            ReInitSmoothIsPending;
    } ErrorRecovery;

    /* Fast Backward Fluidity : flags in order to fluidify fast backward speeds */
    struct
    {
        clock_t                         TransferBeginDate;
        U32                             NbFieldsToBeDisplayedToBeFluent;
        U32                             NbFieldsToBeDisplayedPerPicture;
        U32                             NbFieldsToBeDisplayed;
        BOOL                            BackwardFluentDisplayProcessIsRunning;
        U32                             LastBackwardFieldsSkippedNb;
        U32                             LastInsertedInDQPicRepeatedFieldsNb;
        U32                             MaxJumpeableFields;
        U32                             VSyncEventNb;
        U32                             DisplayQueueEventEmptyNb;
    } BackwardFluidity;

    struct
    {
        /* Forward to Backward */
        BOOL                            FirstGOPFound;
        VIDTRICK_PictureInfo_t *        FirstNullTempRefPicture_p;

        /* Backward to Forward */
        BOOL                            ForwardDisplayQueueInsertionIsEnabled;
        U32                             LastBackwardDisplayedPictureTimeCode;
        U16                             LastBackwardDisplayedPictureTemporalReference;
        U16                             LastBackwardDisplayedPictureVbvDelay;
        U32                             MatchingRecognitionTryNumber;
        BOOL                            NextReferenceFound;
        STVID_MPEGFrame_t               MPEGFrame;
    } NegativeToPositiveSpeedParams;

    struct
    {
        BOOL                            IsRequiredInTask;
        VIDDEC_StartCodeSearchMode_t    SearchMode;
        BOOL                            FirstSliceDetect;
        void *                          SearchAddress_p;
        clock_t                         DontStartBeforeThatTime;    /* Next start should not be done before that time ! */
        BOOL                            MustWaitBecauseOfVSync;
    } NewStartCodeSearch;

    struct
    {
        struct VIDTRICK_LinearBitBuffer_s * PreviousBitBufferUsedForDisplay;
        clock_t PreviousBitBufferBeginningOfDisplay;
        S32 PreviousSkip;
        S32 NBFieldsLate;
    } SpeedAccuracy;

    VIDTRICK_LinearBitBuffer_t *        BitBufferFullyParsedToBeProcessedInTask_p;
#endif /* #ifdef ST_smoothbackward */
    BOOL                                EventDisplayChangeToBeNotifiedInTask;
    BOOL                                IsWaitOrderTimeWatchDog; /* TRUE: wait infinity for orders or watch dog, FALSE: wait the time as specified in GiveOtherTasksCPUMaxWaitOrderTime */
    clock_t                             GiveOtherTasksCPUMaxWaitOrderTime;

    U32                                 NumberOfFlushesToRestartFromTheSamePicture;
    BOOL                                ForwardInjectionDiscontinuity;
    STVID_DecodedPictures_t             DecodedPictures;
} VIDTRICK_Data_t;

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_TRICKMOD_COMMON_H*/

/* End of vid_tric.h */
