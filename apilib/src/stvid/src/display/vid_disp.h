/*******************************************************************************

File name   : vid_disp.h

Description : Video display common header file

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
14 Mar 2000        Created                                          HG
09 Mar 2001        New ReconstructionMode field in VIDDISP_Data_t   GG
16 Mar 2001        Display Event notification configuration.        GG
   Sep 2002        Dual Display                                     MB
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_DISPLAY_COMMON_H
#define __VIDEO_DISPLAY_COMMON_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stvtg.h"
#include "stevt.h"
#include "stvid.h"
#include "buffers.h"
#include "display.h"
#include "vid_ctcm.h"
#include "halv_dis.h"

#ifdef ST_crc
    #include "crc.h"
#endif /* ST_crc */

#ifdef ST_XVP_ENABLE_FGT
  #include "vid_fgt.h"
#endif /* ST_XVP_ENABLE_FGT */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Options ------------------------------------------------------------------ */

/* Flag to allow any frame rate conversion :    */
/* - 3:2 pull down MPEG-1 and MPEG-2.           */
/* - Same input and output frame rate.          */
/* - PAL to NTSC or NTSC to PAL.                */
#define STVID_FRAME_RATE_CONVERSION

/* Exported Macros ---------------------------------------------------------- */

/* Returns TRUE if picture is actually on display */
#define IsOnDisplay(Picture) (((Picture).Counters[0].NextFieldOnDisplay   \
                                    != VIDDISP_FIELD_DISPLAY_NONE)        \
                            ||((Picture).Counters[1].NextFieldOnDisplay   \
                                    != VIDDISP_FIELD_DISPLAY_NONE))

#define PushDisplayCommand(DisplayHandle, U8Var)                \
    vidcom_PushCommand(&(((VIDDISP_Data_t *) DisplayHandle)     \
                ->DisplayCommandsBuffer), ((U8) (U8Var)))

#define PopDisplayCommand(DisplayHandle, U8Var_p)               \
    vidcom_PopCommand(&(((VIDDISP_Data_t *) DisplayHandle)      \
                ->DisplayCommandsBuffer), ((U8 *) (U8Var_p)))

/* Reads the number of used display HALs for the device. If the chip uses only 1 display HAL, returns 0 */
/* Else, if the layer type is STLAYER_GAMMA_GDP, returns (DisplayHALNumber - 1), */
/* Else, returns the value passed in parameter. */
#define GetHALIndex(DisplayHandle, DualHALIndex)   \
    ((((VIDDISP_Data_t *) DisplayHandle)->DisplayHALNumber == 1)? 0: \
    ((((VIDDISP_Data_t *) DisplayHandle)->LayerDisplay[DualHALIndex].LayerType == STLAYER_GAMMA_GDP)? \
    (U32)((((VIDDISP_Data_t *) DisplayHandle)->DisplayHALNumber) - 1): DualHALIndex))

#define LAYER_NEEDS_MAIN_PICTURE(Layer)             (VIDDISP_Data_p->LayerDisplay[Layer].ReconstructionMode != VIDBUFF_SECONDARY_RECONSTRUCTION)
#define LAYER_NEEDS_DECIMATED_PICTURE(Layer)        (VIDDISP_Data_p->LayerDisplay[Layer].ReconstructionMode == VIDBUFF_SECONDARY_RECONSTRUCTION)

/* Exported Constants ------------------------------------------------------- */

typedef enum VIDDISP_FieldReference_e
{
    VIDDISP_PREVIOUS_FIELD,             /* Field 'N-1' = Previous Field */
    VIDDISP_CURRENT_FIELD,              /* Field  'N'  = Current Field */
    VIDDISP_NEXT_FIELD,                 /* Field 'N+1' = Next Field */
    VIDDISP_NBR_OF_FIELD_REFERENCES     /*  Fields are handled as triplets */
} VIDDISP_FieldReference_t;

/* We currently support 2 layers: Main and Aux */
#define MAX_SUPPORTED_LAYERS    2

#define DISPLAY_COMMANDS_BUFFER_SIZE 10

#define NB_QUEUE_ELEM   (2 * (VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES + 2 + MAX_EXTRA_FRAME_BUFFER_NBR)) * 2
/* # of ref frames + 2 (decode, display) + MAX_EXTRA_FRAME_BUFFER_NBR = 32 ref fields + 4 + (MAX_EXTRA_FRAME_BUFFER_NBR*2) pictures */
/* Add again the same amount of entries to deal with the decimated pictures which are recorded in the same queue */

typedef enum VIDDISP_FieldDisplay_e
{
    VIDDISP_FIELD_DISPLAY_NONE,
    VIDDISP_FIELD_DISPLAY_TOP,
    VIDDISP_FIELD_DISPLAY_BOTTOM,
    VIDDISP_FIELD_DISPLAY_REPEAT,
    VIDDISP_FIELD_DISPLAY_SAME
} VIDDISP_FieldDisplay_t;

enum
{
    VIDDISP_ASPECT_RATIO_CHANGE_EVT_ID,
    VIDDISP_FRAME_RATE_CHANGE_EVT_ID,
    VIDDISP_SCAN_TYPE_CHANGE_EVT_ID,
    VIDDISP_RESOLUTION_CHANGE_EVT_ID,
    VIDDISP_DISPLAY_NEW_FRAME_EVT_ID,
    VIDDISP_NEW_PICTURE_PREPARED_EVT_ID,
    VIDDISP_NEW_PICTURE_TO_BE_DISPLAYED_EVT_ID,
    VIDDISP_PICTURE_CHARACTERISTICS_CHANGE_EVT_ID,
    VIDDISP_VSYNC_DISPLAY_NEW_PICTURE_EVT_ID,
    VIDDISP_VSYNC_EVT_ID,
    VIDDISP_PRE_DISPLAY_VSYNC_EVT_ID,
    VIDDISP_POST_DISPLAY_VSYNC_EVT_ID,
    VIDDISP_VTG_SCANTYPE_CHANGE_EVT_ID,
    VIDDISP_OUT_OF_PICTURES_EVT_ID,
#ifdef ST_smoothbackward
    VIDDISP_NO_NEXT_PICTURE_TO_DISPLAY_EVT_ID,
#endif
    VIDDISP_FIELDS_REPEATED_EVT_ID,
    VIDDISP_NEW_PICTURE_SKIPPED_NOT_REQUESTED_EVT_ID,
    VIDDISP_COMMIT_NEW_PICTURE_PARAMS_EVT_ID,
	VIDDISP_NEW_FIELD_TO_BE_DISPLAYED_EVT_ID,
	VIDDISP_NB_REGISTERED_EVENTS_IDS
};

/* Exported Types ----------------------------------------------------------- */

typedef struct VIDDISP_VsyncLayer_s
{
    STVTG_VSYNC_t VSync;
    U32           LayerIdent;
}VIDDISP_VsyncLayer_t;

typedef struct VIDDISP_PictDisplayCounters_s
{
    U32     TopFieldCounter;
    U32     BottomFieldCounter;
    U32     RepeatFieldCounter;
    U8      RepeatProgressiveCounter;
    VIDDISP_FieldDisplay_t NextFieldOnDisplay;
    VIDDISP_FieldDisplay_t CurrentFieldOnDisplay;
    U32     CounterInDisplay;
    BOOL    IsFRConversionActivated; /* The counters have been patched and
                                      thus introduces a new display duration. */
#ifdef ST_crc           /* Counters to detect repeated fields/frames */
    VIDDISP_FieldDisplay_t PreviousFieldOnDisplay;
    U32     TopFieldDisplayedCounter;
    U32     BottomFieldDisplayedCounter;
    U32     FrameDisplayDisplayedCounter;
#endif /* ST_crc */
}VIDDISP_PictDisplayCounters_t;

typedef struct VIDDISP_Picture_s
{
    struct  VIDDISP_Picture_s *         Next_p;
    VIDBUFF_PictureBuffer_t *           PrimaryPictureBuffer_p;
    VIDBUFF_PictureBuffer_t *           SecondaryPictureBuffer_p;
    VIDBUFF_PictureBuffer_t *           PictureInformation_p;     /* Picture Buffer used only to access picture information (ParsedPictureInformation, GlobalDecodingContext...) */
    U32                                 PictureIndex;
    VIDDISP_PictDisplayCounters_t       Counters[2]; /* elem 0 : main, elem 1 : aux */
}VIDDISP_Picture_t;

typedef struct LayerDisplayParams_s
{
    BOOL                                IsOpened;
    VIDBUFF_AvailableReconstructionMode_t ReconstructionMode; /* queue select */
    STLAYER_Layer_t                     LayerType;
    STLAYER_Handle_t                    LayerHandle;
    BOOL                                ManualPresentation;
    U32                                 PresentationDelay;
    ST_DeviceName_t                     VTGName;
    U32                                 VTGVSyncFrequency;
    /* Layer parameters updated from UpdateParametersFromLayer() callback */
    STLAYER_LayerParams_t               LayerParams;
    osclock_t                           TimeAtVSyncNotif;
    /* Last VSync polarity sent to the display process.             */
    BOOL                                CurrentVSyncTop;
    BOOL                                IsNextOutputVSyncTop;
    /* Memory of the last VSync polarity occured.                   */
    U8                                  VSyncMemoryArray;
    VIDDISP_Picture_t *                 PicturePreparedForNextVSync_p;
    VIDDISP_Picture_t *                 CurrentlyDisplayedPicture_p;
    VIDDISP_Picture_t *                 PictDisplayedWhenDisconnect_p;
    U8                                  DisplayLatency;
    U8                                  FrameBufferHoldTime;
    BOOL                                NewPictureDisplayedOnCurrentVSync;
    BOOL                                IsCurrentFieldOnDisplayTop;
    BOOL                                IsFieldPreparedTop;
    /* Variable indicating if the last picture prepared for this layer was Main or Decimated */
    BOOL                                IsLastPicturePreparedDecimated;
    /* Number Of Extra Frame Buffers that can be used as reference for Deinterlacing */
    U32                                 NbrOfExtraPrimaryFrameBuffers;
    U32                                 NbrOfExtraSecondaryFrameBuffers;
    struct
    {
        /* To send STVID_ASPECT_RATIO_CHANGE_EVT when displaying picture */
        /* if aspect_ratio_information is changing */
        STGXOBJ_AspectRatio_t           AspectRatio;
        /* To send STVID_SCAN_TYPE_CHANGE_EVT when displaying picture if */
        /*  ScanType is changing */
        STGXOBJ_ScanType_t              ScanType;
        /* To send STVID_FRAME_RATE_CHANGE_EVT */
        U32                             PictureFrameRate;
        /* To send STVID_RESOLUTION_CHANGE_EVT when displaying picture */
        /* if height is changing */
        U32                             Height;
        U32                             Width;
        /* To send VIDDISP_PICTURE_CHARACTERISTICS_CHANGE_EVT when displaying */
        /*  picture if it is changing */
        VIDCOM_PanAndScanIn16thPixel_t PanAndScanIn16thPixel;
        /* To send VIDDISP_PICTURE_CHARACTERISTICS_CHANGE_EVT when displaying */
        /* picture if it is changing */
        STVID_DecimationFactor_t        DecimationFactor;
        VIDBUFF_AvailableReconstructionMode_t  Reconstruction; /* picture */
        /* To send VIDDISP_PICTURE_CHARACTERISTICS_CHANGE_EVT when displaying */
        /* picture if ColorSpaceConversion is changing */
        STGXOBJ_ColorSpaceConversionMode_t  ColorSpaceConversion;
#ifdef ST_crc
        /* To send VIDDISP_PICTURE_CHARACTERISTICS_CHANGE_EVT when displaying */
        /* picture if CRCCheckMode or CRCScanType are changing */
        /* CRCScanType is mandatory because it is not allowed to change original*/
        /* ScanType from parsed picture */
        BOOL                            CRCCheckMode;
        STGXOBJ_ScanType_t              CRCScanType;
#endif /* ST_crc */
        /* To send VIDDISP_VTG_SCANTYPE_CHANGE_EVT when a new VTG
           progressive/interlaced scan type is detected */
        STGXOBJ_ScanType_t              VTGScanType;
    }DisplayCaract;

    /* Allocate 3 Fields arrays used to store the references to the Fields currently used for deinterlacing */
    HALDISP_Field_t             Fields1[VIDDISP_NBR_OF_FIELD_REFERENCES];
    HALDISP_Field_t             Fields2[VIDDISP_NBR_OF_FIELD_REFERENCES];
    HALDISP_Field_t             Fields3[VIDDISP_NBR_OF_FIELD_REFERENCES];
    /* Pointer to the Field array containing the Fields N, N-1 and N+1 that will be used at the Next VSync */
    HALDISP_Field_t *           FieldsUsedAtNextVSync_p;
    /* Pointer to the Field array containing the Fields N, N-1 and N+1 currently used by the hardware */
    HALDISP_Field_t *           FieldsUsedByHw_p;
    /* Pointer to the Field array currently available (we need one extra array to perform a circular rotation) */
    HALDISP_Field_t *           FieldsNotUsed_p;
    /* Pointer to the Field array which was used by the hardware. It is going to be Unlocked */
    HALDISP_Field_t *           FieldsToUnlock_p;

} LayerDisplayParams_t;

typedef struct VIDDISP_Data_s
{
    ST_Partition_t *    CPUPartition_p;
    STAVMEM_PartitionHandle_t   AvmemPartitionHandle;
    STEVT_Handle_t      EventsHandle;
    STEVT_Handle_t      AuxEventsHandle;
    STEVT_EventID_t     RegisteredEventsID[VIDDISP_NB_REGISTERED_EVENTS_IDS];
    VIDBUFF_Handle_t    BufferManagerHandle;
#ifdef ST_crc
    VIDCRC_Handle_t     CRCHandle;
    BOOL AreCRCParametersAvailable;
    struct
    {
        BOOL IsRepeatedLastPicture;
        BOOL  IsRepeatedLastButOnePicture;
        STVID_PictureInfos_t PictureInfos;
        VIDCOM_PictureID_t ExtendedPresentationOrderPictureID;
        U32 DecodingOrderFrameID;
        U32  PictureNumber;
#if defined(STVID_CRC_ALLOW_FRAME_DUMP) || defined(STVID_SOFTWARE_CRC_MODE)
        VIDDISP_Picture_t      * Picture_p;
#endif
        STVID_CRCDataMessage_t CRCDataToEnqueue;
    } PictureUsedForCRCParameters;
    U32 PreviousCRCPictureNumber;
#endif /* ST_crc */
#ifdef ST_XVP_ENABLE_FGT
    VIDFGT_Handle_t     FGTHandle;
#endif /* ST_XVP_ENABLE_FGT */
    /* Table containing handles to video display HALs */
    HALDISP_Handle_t    HALDisplayHandle[DISPLAY_HAL_MAX_NUMBER];
    /* Initialized at VIDDISP_Init and used in VIDDISP_Term */
    U8                  DisplayHALNumber;
    STVID_DeviceType_t  DeviceType;
    U32                 ValidityCheck;

    void *              MappedRegistersBaseAddress_p[DISPLAY_HAL_MAX_NUMBER];

    /* As defined by HALDEC. See definition there. */
    U8                  DecoderNumber;
    U8                  DisplayCommandsData[DISPLAY_COMMANDS_BUFFER_SIZE];
    /* To synchronise orders in the task */
    semaphore_t *       DisplayOrder_p;
    /* Data concerning the display commands queue */
    CommandsBuffer_t    DisplayCommandsBuffer;
    VIDCOM_Task_t       DisplayTask;
    mutex_t *       ContextLockingMutex_p;
    VIDDISP_DisplayState_t      DisplayState;
    U32                 LockState;
    BOOL                ForceNextPict;
    /* set TRUE when informed that a discontinuity in MPEG display order */
    /* is occuring.set FALSE when discontinuity is gone.  Used to unlock */
    /* display if it was locked because of DisplayStart */
    /*  == VIDDISP_DISPLAY_START_CARE_MPEG_FRAME. Indeed, when a reference */
    /* picture is decoded but not passed to display, there's no need to wait */
    /* for a coming B picture, display should go on to the next picture */
    /* after the discontinuity. */
    BOOL                HasDiscontinuityInMPEGDisplay;
    /* Display parameters */
    BOOL                DisplayForwardNotBackward;
    BOOL                LastDisplayDirectionIsForward;        /* Memorize the last display direction */
    /* TRUE : care about the polarity of the first picture's field displayed*/
    /* FALSE: don't care, eventually displays top on bottom and vice versa  */
    /* Remember the needed value of DisplayWithCareOfFieldsPolarity the     */
    /* trickmod module wants                                                */
    BOOL                DisplayWithCareOfFieldsPolarity;
    BOOL                FrameRateConversionNeedsCareOfFieldsPolarity;
    BOOL                NotifyStopWhenFinish;
    BOOL                DoReleaseDeiReferencesOnFreeze;
    struct  /* Pause and Freeze Params */
    {
        STVID_Freeze_t  Freeze;
        /* During Freeze/Pause state, indicate a step action is to be perf   */
        BOOL            SteppingOrderReceived;
        /* Pending cmd */
        BOOL            PauseRequestIsPending;
        BOOL            FreezeRequestIsPending;
    } FreezeParams;
    struct /* Stop */
    {
        BOOL            StopRequestIsPending;
        VIDDISP_Stop_t  StopMode;
        STVID_PTS_t     RequestStopPicturePTS;
        semaphore_t *   ActuallyStoppedSemaphore_p; /* semaphore to synchronize VIDDISP_Stop() function exit and actual stop in display */
    } StopParams;
    LayerDisplayParams_t  LayerDisplay[MAX_SUPPORTED_LAYERS];/* elem 0 is main, elem 1 is aux */
    U32                 DisplayRef;      /* the master one */
    VIDDISP_Picture_t * DisplayQueue_p; /* pointer to 1st main picture */
    VIDDISP_Picture_t   DisplayQueueElements[NB_QUEUE_ELEM];/* pool containing both pictures entries */
    struct /* Display events notification configuration */
    {
        /* Contains informations : Event enable/disable notification and */
        /* Number of event notification to skip before notifying the event. */
        STVID_ConfigureEventParams_t ConfigureEventParam;
        /* Number of notification skipped since the last one. */
        U32             NotificationSkipped;
    } EventNotificationConfiguration [VIDDISP_NB_REGISTERED_EVENTS_IDS];
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
    /* Flag to remember VTG changes, in order to recalculate output.*/
    BOOL IsVTGChangedSinceLastVSync;
#ifdef STVID_DEBUG_GET_STATISTICS
    /* Counts number of pictures inserted in display queue          */
    U32                 StatisticsPictureInsertedInQueue;
    /* Counts number of pictures displayed (not decimated)          */
    U32                 StatisticsPictureDisplayedByMain;
    /* Counts number of decimated pictures displayed                */
    U32                 StatisticsPictureDisplayedDecimatedByMain;
    /* Counts number of time the display queue is locked by lack of */
    /* picture to display */
    U32                 StatisticsPbPictureTooLateRejectedByMain;
    /* Counts number of pictures rejected by display because too late */
    U32                 StatisticsPictureDisplayedByAux;
    /* Counts number of decimated pictures displayed                */
    U32                 StatisticsPictureDisplayedDecimatedByAux;
    /* Counts number of time the display queue is locked by lack of */
    /* picture to display */
    U32                 StatisticsPbPictureTooLateRejectedByAux;
    U32                 StatisticsPbQueueLockedByLackOfPicture;
    U32                 StatisticsPbDisplayQueueOverflow;
    U32                 StatisticsPictureDisplayedBySec;
    U32                 StatisticsPictureDisplayedDecimatedBySec;
    U32                 StatisticsPbPictureTooLateRejectedBySec;
    U32                 StatisticsPbPicturePreparedAtLastMinuteRejected;
#ifdef ST_speed
    U32                 StatisticsSpeedDisplayIFramesNb;
    U32                 StatisticsSpeedDisplayPFramesNb;
    U32                 StatisticsSpeedDisplayBFramesNb;
#endif /*ST_speed*/
#endif /* STVID_DEBUG_GET_STATISTICS */

    /* Temporal referencce of the first frame of a 30 frames/sec NTSC stream,
     * when frame rate conversion is activated to PAL display */
    U32                 InitialTemporalReferenceForSkipping;
    /* Temporal referencce of the first frame of a 24 frames/sec NTSC stream,
    * when frame rate conversion is activated to PAL display */
    U32                 InitialTemporalReferenceForRepeating;
    /* Temporal referencce of the last received frame that have the flag Repeat
     * field counter */
    U32                 LastFlaggedPicture;
    /* Flag used to say that as stream is 24 frames/sec */
    BOOL                StreamIs24Hz;
    U32                 FrameRatePatchCounterTemporalReference;
    /*-----------------28-07-04 10:10-------------------
     * Flag used to enable or disable frame rate convertion */
    BOOL                EnableFrameRateConversion;
    /* Flag used to bypass the SequenceSlave() call until all viewport parameters are completely initialized. */
    BOOL                IsSlaveSequencerAllowedToRun;

    /* The Picture Index is an integer assigned to each picture put in the Display Queues */
    U32                         QueuePictureIndex;
#ifdef STVID_FRAME_RATE_CONVERSION
    U32                         FrameNumber;       /** Indicate if the Number of Frame that should be repeated is reached or not **/
    U32                         FrameTobeAdded;    /** Indicate the Number of picture that should be divided in equitable way on all patched picture **/
    U32                         PreviousFrameRate;  /** Used to store the Frame rate of the picture before the display **/
#endif /* STVID_FRAME_RATE_CONVERSION */
    BOOL                        PeriodicFieldInversionDueToFRC; /* Flag indicating that periodic field inversions are normal because of FRC */
} VIDDISP_Data_t;

/* Exported Functions ------------------------------------------------------- */

void viddisp_VSyncDisplayMain(STEVT_CallReason_t Reason,
                    const ST_DeviceName_t RegistrantName,
                    STEVT_EventConstant_t Event,
                    const void *EventData_p, const void *SubscriberData_p);

void viddisp_VSyncDisplayAux(STEVT_CallReason_t Reason,
                    const ST_DeviceName_t RegistrantName,
                    STEVT_EventConstant_t Event,
                    const void *EventData_p, const void *SubscriberData_p);

void viddisp_NewPictureToBeDisplayed(STEVT_CallReason_t Reason,
                            const ST_DeviceName_t RegistrantName,
                            STEVT_EventConstant_t Event,
                            const void *EventData_p,
                            const void *SubscriberData_p);

void viddisp_PictureCandidateToBeRemovedFromDisplayQueue(
                                        STEVT_CallReason_t Reason,
                                        const ST_DeviceName_t RegistrantName,
                                        STEVT_EventConstant_t Event,
                                        const void *EventData_p,
                                        const void *SubscriberData_p);
void viddisp_DisplayTaskFunc(const VIDDISP_Handle_t DisplayHandle);

void viddisp_InitMultiFieldPresentation(const VIDDISP_Handle_t DisplayHandle,
                                        U32 LayerIdent);
void viddisp_UnlockAllFields(const VIDDISP_Handle_t DisplayHandle, U32 LayerIdent);

BOOL viddisp_IsDeinterlacingPossible(const VIDDISP_Handle_t DisplayHandle, U32 LayerIdent);

#ifdef ST_crc
    void viddisp_UnlockCRCBuffer(VIDDISP_Data_t *  VIDDISP_Data_p);
#endif

VIDBUFF_PictureBuffer_t * GetAppropriatePictureBufferForLayer(const VIDDISP_Handle_t    DisplayHandle,
                                                              VIDDISP_Picture_t *       Picture_p,
                                                              U32                       Layer);

/* -------------------------------------------------------------------------- */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_DISPLAY_COMMON_H */

/* End of vid_disp.h */
