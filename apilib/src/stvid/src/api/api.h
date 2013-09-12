/*******************************************************************************

File name   : api.h

Description : Video api common functions header file

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
 23 Jun 2000        Created                                         AN
 14 Feb 2001        MemoryProfile removed from VideoDevice_t        GG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_API_H
#define __VIDEO_API_H

/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <string.h>
#include <stdlib.h>
#endif

#include "vid_ctcm.h"
#include "vid_com.h"

#ifdef ST_avsync
#include "avsync.h"
#endif /* ST_avsync */

#if !defined(ST_oldbuffermanager)
#ifdef ST_ordqueue
#include "ordqueue.h"
#endif /* ST_ordqueue */
#endif

#ifdef ST_producer
#include "producer.h"
#endif /* ST_producer */

#ifdef ST_display
#include "display.h"
#endif /* ST_display */

#ifdef ST_he_disp
#include "he_disp.h"
#endif /* ST_he_disp */

#ifdef ST_crc
#include "crc.h"
#endif /* ST_crc */

#include "buffers.h"

#if defined (ST_diginput) && !defined (ST_MasterDigInput)
#include "diginput.h"
#endif/* #ifdef ST_diginput && !ST_MasterDigInput */

#ifdef ST_speed
#include "speed.h"
#endif /*ST_speed*/

#ifdef ST_trickmod
#include "trickmod.h"
#endif /* ST_trickmod */

#include "stgxobj.h"
#include "stvid.h"
#include "stlayer.h"

#include "stos.h"
#include "sttbx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

enum
{
        STVID_STOPPED_ID     = 0,
        STVID_IMPOSSIBLE_WITH_MEM_PROFILE_ID,
#ifdef ST_diginput
        STVID_DIGINPUT_WIN_CHANGE_ID,
#endif/* #ifdef ST_diginput */
        STVID_NB_REGISTERED_EVENTS /* keep in last position */
};

#define STVID_VALID_UNIT    0x01051050

/* Used only in Init/Open/Close/Term functions */
#define STVID_MAX_DEVICE    20      /* Max number of Init() allowed */
#define STVID_MAX_OPEN       5      /* Max number of Open() allowed per Init() */
#define STVID_MAX_UNIT  (STVID_MAX_OPEN * STVID_MAX_DEVICE)

#define STVID_INVALID_DECODER_NUMBER    ((U32) (-1))

/* Dummy value that we set to check that a handle is valid, only one chance out of 2^32 to get it by accident ! */
#define VIDAPI_VALID_VIEWPORT_HANDLE 0x01251250


/* Exported Types ----------------------------------------------------------- */
typedef enum VideoStatus_e
{
  VIDEO_STATUS_STOPPED,
  VIDEO_STATUS_RUNNING,
  VIDEO_STATUS_PAUSING,
  VIDEO_STATUS_FREEZING
} VideoStatus_t;

typedef enum LiveResetState_e
{
  WAIT_FOR_VIDEO_START,             /* Video is initialized, and we are waiting for a start command */
  WAIT_FOR_FIRST_PICTURE_DETECTED,  /* Decoder just started by STVID_Start() (or restarted).        */
                                    /* All first picture informations are expected (first parse).   */
  WAIT_FOR_FIRST_PICTURE_DECODE,    /* The first picture is parsed, and sufficient bit buffer level */
                                    /* is expected to launch the very first decode.                 */
  WAIT_FOR_NEXT_PICTURE_DECODE      /* Decode is going on. The next decode is expected.             */

} LiveResetState_t;

typedef enum ForceDecimationState_e
{
    /* A new decimation factor has been specified through API call */
    FORCE_PENDING,
    /* The driver has found the first picture to begin applying the new
       decimation factor */
    FORCE_ONGOING,
    /* No decimation factor forcing is done */
    FORCE_DISABLED
} ForceDecimationState_t;

typedef struct StreamParameters_s
{
  BOOL                          HasDisplaySizeRecommendation;
  /* Following variables are only valid if SequenceDisplayExtensionFound is true */
  U16                           DisplayHorizontalSize;
  U16                           DisplayVerticalSize;
  S16                           PanVector;
  S16                           ScanVector;
  VIDCOM_FrameCropInPixel_t     FrameCropInPixel;
} StreamParameters_t;

typedef struct PictureSave_s
{
  /* Layer/viewport informations.   */
  StreamParameters_t            StreamParameters;
  STLAYER_ViewPortParams_t      LayerViewport;
  VIDBUFF_AvailableReconstructionMode_t ReconstructionMode;
  BOOL                          WasViewPortEnabledDuringShow;   /* Flag to remember we have enable manually the */
                                                /* video viewport when processing STVID_ShowPicture().          */

  /* Source informations.           */
  STLAYER_StreamingVideo_t      VideoStreaming; /* remember streaming informations before STVID_ShowPicture.    */
  VIDBUFF_PictureBuffer_t       PictureBuffer;  /* Picture buffer info before the show.                         */
  VIDBUFF_FrameBuffer_t         FrameBuffer;    /* Frame buffer info before the show.                           */
} PictureSave_t;

typedef struct VideoLayer_s
{
    BOOL                    IsOpened;
    ST_DeviceName_t         LayerName;
    STLAYER_Handle_t        LayerHandle;
    STLAYER_Layer_t         LayerType;
    U32                     NumberVideoViewportsAssociated;
    U32                     DisplayLatency;
} VideoLayer_t;

typedef struct VideoViewPort_s
{
    struct
    {
        U32                       ViewPortOpenedAndValidityCheck; /* VIDAPI_VALID_VIEWPORT_HANDLE if used, anything otherwise */
        VideoLayer_t *            Layer_p;  /* Layer the viewport is associated to */
        STVID_Handle_t            VideoHandle;  /* Handle used to open the view port (not usefull for now) */
        struct VideoDevice_t *    Device_p;     /* Device concerned by the viewport */
        STLAYER_ViewPortHandle_t  LayerViewPortHandle;
    } Identity;

    struct
    {
        BOOL                                DisplayEnable;
#ifdef GNBvd33476_WA
        BOOL                                FirstTopDisplayed;
        BOOL                                SetViewportParamsReported;
#endif
        BOOL                                DisplayEnableDelayed;
        BOOL                                InputWinAutoMode;
        BOOL                                OutputWinAutoMode;
        STVID_WindowParams_t                InputWinParams;
        STVID_WindowParams_t                OutputWinParams;
                                                                    /* Last input/output windows set to layer       */
                                                                    /* viewport without adjustment.                 */
        STLAYER_ViewPortParams_t            ViewportParametersAskedToLayerViewport;
        STLAYER_ViewPortParams_t            VideoViewportParameters;    /* Current viewport parameters, including :     */
                                                                    /*  - input/output windows set by application   */
                                                                    /*    thanks to STVID_SetIOWindows().           */
                                                                    /*  - source characteristics.                   */
#ifdef ST_diginput
        U32                                 CurrentOutputWidthForDVP;
        U32                                 CurrentOutputHeightForDVP;
#endif/* #ifdef ST_diginput */
        STVID_DisplayAspectRatioConversion_t DispARConversion;
        StreamParameters_t                  StreamParameters;
        BOOL                                ShowingPicture;     /* Info show for each view port !? */
        PictureSave_t                       PictureSave;
        STLAYER_QualityOptimizations_t QualityOptimizations;
    } Params;
} VideoViewPort_t;

/* Structure containing dynamic data for initialised instances */
typedef struct VideoDeviceData_s
{
    /* Variables written only in init/termination/capability phase, and used as read-only after */
    /* All data in this section are protected with InterInstancesLockingSemaphore, used through
	 * macros stvid_EnterCriticalSection(NULL) and stvid_LeaveCriticalSection(NULL) */
	U32                     MaxOpen;

    ST_Partition_t*         CPUPartition_p;
    STAVMEM_PartitionHandle_t  AVMEMPartitionHandle;
/*    void*                   SharedMemoryBaseAddress_p;*/ /* not used */
    STEVT_Handle_t          EventsHandle;
    STEVT_EventID_t         EventID[STVID_NB_REGISTERED_EVENTS];
    STVID_DeviceType_t      DeviceType;
#ifdef ST_avsync
    VIDAVSYNC_Handle_t      AVSyncHandle;
#endif /* ST_avsync */
#ifdef ST_producer
#ifdef ST_multidecodeswcontroller
    VIDDEC_MultiDecodeHandle_t MultiDecodeHandle;
#endif /* ST_multidecodeswcontroller */
    VIDPROD_Handle_t        ProducerHandle;
    BOOL                    ProducerHandleIsValid;
#endif /* ST_producer */
#ifdef ST_display
    VIDDISP_Handle_t        DisplayHandle;
    BOOL                    DisplayHandleIsValid;
#endif /* ST_display */
#ifdef ST_he_disp
    VIDHEDISP_Handle_t      HEDisplayHandle;
    BOOL                    HEDisplayHandleIsValid;
#endif /* ST_he_disp */
#ifdef ST_crc
    VIDCRC_Handle_t         CRCHandle;
    BOOL                    CRCHandleIsValid;
#endif /* ST_crc */
#ifdef ST_XVP_ENABLE_FGT
    VIDFGT_Handle_t         FGTHandle;
    BOOL                    FGTHandleIsValid;
#endif /* ST_XVP_ENABLE_FGT */
#ifdef ST_ordqueue
    VIDQUEUE_Handle_t       OrderingQueueHandle;
    BOOL                    OrderingQueueHandleIsValid;
#endif /* ST_ordqueue */
#ifdef ST_diginput
#ifdef ST_MasterDigInput
    BOOL                    DigitalInputIsValid;
#else /* ST_MasterDigInput */
    VIDINP_Handle_t         DigitalInputHandle;
    BOOL                    DigitalInputHandleIsValid;
#endif /* not ST_MasterDigInput */
#endif /* ST_diginput */
#ifdef ST_trickmod
    VIDTRICK_Handle_t       TrickModeHandle;
    BOOL                    TrickModeHandleIsValid;
#endif /* ST_trickmod */
#ifdef ST_speed
	VIDSPEED_Handle_t       SpeedHandle;
    BOOL                    SpeedHandleIsValid;
#endif  /*ST_speed*/

    VIDBUFF_Handle_t        BuffersHandle;
    U32                     NumberOfOpenSinceInit; /* Just counts the number of successfull open's done */
    void *                  DeviceBaseAddress_p;    /* Base Adress of the device */
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping; /* AVMEM virtual mapping */
    U32                     MaxViewportNumberPerLayer;
    /* Structures dynamic in capabilities must be allocated at init, and not in the STVID_GetCapability() function.
     Otherwise they are freed when going out of this function ! */
    STVID_ScalingFactors_t  HorizontalFactors[2]; /* Size varies depending on chip (put max here) */
    STVID_ScalingFactors_t  VerticalFactors[7];   /* Size varies depending on chip (put max here) */

    /* All data in this section are protected with DeviceControlLockingSemaphore, used through
    macros stvid_EnterCriticalSection(Device_p) and stvid_LeaveCriticalSection(Device_p) */
    /* Status : should be a single variable, so that there is no need to protect it
    for simple read-only access (except of course if it is for a status transition) */
    VideoStatus_t           Status;             /* Status of the video decode (stopped/running...) */
    BOOL                    ExpectingEventStopped; /* Set TRUE when user calls STVID_Stop() with stop mode NOT now and when user
                                                      calls STVID_Clear.
                                                      Then a layer below (ex VIDDISP) is supposed to send a OUT_OF_PICTURES event,
                                                      which should finish stop in callback stvid_DisplayOutOfPictures and set this
                                                      flag FALSE.
                                                      But the flag is used in VID_Start() to detect that stop was not properly done */
    BOOL                    NotifyStopped;         /* Set TRUE when user calls STVID_Stop() with stop mode NOT now to say that
                                                      we have to notify the event STVID_STOPPED_EVT when stop is finished */
    STVID_StartParams_t     StartParams;        /* Start parameters given to the video decoder */
    STVID_Stop_t            StopMode;           /* Stop mode given to the video decoder */

    /* VTG parameters */
    /* All data in this section are protected with VTGParamsChangeLockingSemaphore */
    semaphore_t *           VTGParamsChangeLockingSemaphore_p;    /* Semaphore used to protect change in VTG name or params */
    U32                     VTGFrameRate;                       /* Frame rate of VTG in mHz, 0 if no VTG connected */

    /* Layers and viewports parameters */
    /* All data in this section are protected with LayersViewportsLockingSemaphore */
    semaphore_t *           LayersViewportsLockingSemaphore_p;    /* Semaphore used to protect layer and viewport data */
    /* Parameters for layers */
    U32                     MaxLayerNumber;         /* Fixed to a max number for each instance. This number of VideoLayer_t is allocated at address LayerProperties_p */
    VideoLayer_t *          LayerProperties_p;
    /* Parameters for viewports */
    U32                     MaxViewportNumber;      /* Fixed to a max number for each instance. This number of VideoViewPort_t is allocated at address ViewportProperties_p */
    VideoViewPort_t *       ViewportProperties_p;

    /* Error Recovery data      */
    semaphore_t *           ErrorRecoveryDataAccessSemaphore_p;   /* Semaphore used to protect error recovey data */
    VIDCOM_Task_t           ErrorRecoveryTask;
    STVID_ErrorRecoveryMode_t ErrorRecoveryMode;
    semaphore_t *           ErrorRecoveryTriggerSemaphore_p;      /* Semaphore used to awake up the task.         */
#ifdef STVID_STVAPI_ARCHITECTURE
    STOS_Clock_t            ErrorRecoveryTaskMaxWaitOrderTime;    /* Time out used to wake error recovery task in case
                                                                  of the sofware architecture can not connect any VTG */
#endif
    ST_DeviceName_t         ErrorRecoveryVTGName;
    U32                     MaxNumberOfVSyncToWaitBeforeLiveReset;
    LiveResetState_t        LiveResetState;         /* State of the live reset state machine                */
    U32                     CurrentAbsoluteSpeed;   /* Safe of the current absolute speed of video driver   */
    BOOL                    IsErrorRecoveryInRealTime;  /* Safe of the running mode of decoder              */

    /* Parameters for display (VIDDISP) */
    semaphore_t *           DisplayParamsAccessSemaphore_p;   /* Semaphore used to protect display parameters */
    STVID_Freeze_t          RequiredStopFreezeParams;       /* Used to store freeze params for delayed Stop() */
#ifdef ST_display
    VIDDISP_DisplayParams_t DisplayParamsForAPI;        /* Display params to set to VIDDISP according to API decisions (decoded pictures, speed, etc) */
#if defined ST_trickmod || defined ST_speed
    VIDDISP_DisplayParams_t DisplayParamsForTrickMode;  /* Display params to set to VIDDISP according to trickmode decisions (B skipped or not) */
#endif /* ST_trickmod || ST_speed */
#endif /* ST_display */

#ifdef STVID_DEBUG_GET_STATISTICS
    /* Statistics variables (not protected through semaphores) */
    U32 StatisticsApiPbLiveResetWaitForFirstPictureDetected;   /* Counts number of LiveReset while waiting for the 1st picture parsed (automatic Stop()/Start() when idle for too long in real-time) */
    U32 StatisticsApiPbLiveResetWaitForFirstPictureDecoded;   /* Counts number of LiveReset while waiting for the 1st decode (automatic Stop()/Start() when idle for too long in real-time) */
    U32 StatisticsApiPbLiveResetWaitForNextPicture;   /* Counts number of LiveReset while waiting for the next picture(automatic Stop()/Start() when idle for too long in real-time) */
#endif /* STVID_DEBUG_GET_STATISTICS */

#ifdef STVID_DEBUG_GET_STATUS
	BOOL VideoAlreadyStarted;                /* TRUE when STVID_Start is called */
	STVID_StartParams_t LastStartParams;     /* Last start parameters */
	BOOL VideoAlreadyFreezed;                /* TRUE when STVID_Freeze is called */
	STVID_Freeze_t LastFreezeParams;         /* Last freeze parameters */
#endif /* STVID_DEBUG_GET_STATUS */

#ifdef ST_SecondInstanceSharingTheSameDecoder
    BOOL SpecialCaseOfSecondInstanceSharingTheSameDecoder;
#endif /* ST_SecondInstanceSharingTheSameDecoder */

    /* Indicates that STVID_OpenViewPort() has finished opening the viewport with all the correct parameters. */
    BOOL AreViewportParametersInitialized;
#ifdef ST_producer
    /* The mode to pass picture to display when relevant */
    VIDPROD_UpdateDisplayQueue_t UpdateDisplayQueueMode;
#endif     /** ST_producer **/

#ifdef ST_producer
    /* Params required to force the decimation factor */
    struct
    {
        ForceDecimationState_t      State;
        VIDCOM_PictureID_t          FirstForcedPictureID;
        STVID_DecimationFactor_t    DecimationFactor;
        STVID_DecimationFactor_t    LastDecimationFactor;
    } ForceDecimationFactor;
    VIDCOM_PictureID_t HighestExtendedPresentationOrderPictureID;
#endif /* ST_producer */
} VideoDeviceData_t;

/* Structure containing static data for initialised instances */
typedef struct VideoDevice_t
{
    ST_DeviceName_t         DeviceName;
    VideoDeviceData_t *     DeviceData_p;   /* NULL when not initialised,
                                               allocated structure otherwise */
    U32                     DecoderNumber;  /* Contains STVID_INVALID_DECODER_NUMBER if the device is not initialised.
                                               Contains DecoderNumber as defined by vidcom when initialised. See definition there. */
    semaphore_t *           DeviceControlLockingSemaphore_p; /* Mutex object to access the device's fundamental data: Status, memory profile, */

} VideoDevice_t;

/* Structure containing static data for open handles */
typedef struct VideoUnit_s
{
    VideoDevice_t *             Device_p;
    U32                         UnitValidity;
} VideoUnit_t;

/* Exported Variables ------------------------------------------------------- */

/* Just exported so that macro IsValidHandle works ! Should never be used outside ! */
extern VideoUnit_t stvid_UnitArray[STVID_MAX_UNIT];

/* Exported Macros ---------------------------------------------------------- */

/* Macros used to know the status of the device */
#define SET_VIDEO_STATUS(d_p, s)    ((d_p)->DeviceData_p->Status = s)
#define GET_VIDEO_STATUS(d_p)       ((d_p)->DeviceData_p->Status)

/* Macro to know if a unit is valid */
/* Passing a (STVID_Handle_t), returns TRUE if the handle is valid, FALSE otherwise */
#define IsValidHandle(Handle) ((((VideoUnit_t *) (Handle)) >= ((VideoUnit_t *) stvid_UnitArray)) &&                    \
                               (((VideoUnit_t *) (Handle)) < (VideoUnit_t *) ((VideoUnit_t *) stvid_UnitArray + STVID_MAX_UNIT)) &&  \
                               (((VideoUnit_t *) (Handle))->UnitValidity == STVID_VALID_UNIT))

/*#define IS_VALID_VIDEO(d_p)         (((d_p) != NULL) ? ((d_p)->UnitValidity == STVID_VALID_UNIT) : FALSE)*/


#define DeviceToVirtual(DevAdd, Mapping) (void*)((U32)(DevAdd)\
        -(U32)((Mapping).PhysicalAddressSeenFromDevice_p)\
        +(U32)((Mapping).VirtualBaseAddress_p))

#define VirtualToDevice(Virtual, Mapping) (void*)((U32)(Virtual)\
        +(U32)((Mapping).PhysicalAddressSeenFromDevice_p)\
        -(U32)((Mapping).VirtualBaseAddress_p))

#if !defined(CAST_CONST_HANDLE_TO_DATA_TYPE)
#define CAST_CONST_HANDLE_TO_DATA_TYPE( DataType, Handle )  \
    ((DataType)(*(DataType*)((void*)&(Handle))))
#endif

/* This macro is to be called to have a same code to be executed whether this is VIDTRICK or VIDSPEED that is
  active. All this while supporting VIDSPEED or VIDTRICK or BOTH or NONE ! */
#if defined ST_trickmod && defined ST_speed
#define IsTrickModeHandleOrSpeedHandleValid(Device_p) (((Device_p)->DeviceData_p->TrickModeHandleIsValid) || ((Device_p)->DeviceData_p->SpeedHandleIsValid))
#elif defined ST_trickmod /* both VIDTRICK and VIDSPEED */
#define IsTrickModeHandleOrSpeedHandleValid(Device_p) ((Device_p)->DeviceData_p->TrickModeHandleIsValid)
#elif defined ST_speed /* only VIDTRICK */
#define IsTrickModeHandleOrSpeedHandleValid(Device_p) ((Device_p)->DeviceData_p->SpeedHandleIsValid)
#else /* only VIDSPPED */
#define IsTrickModeHandleOrSpeedHandleValid(Device_p) FALSE
#endif /* not VIDSPPED nor VIDTRICK */

/* Exported Functions ------------------------------------------------------- */

void stvid_EnterCriticalSection(VideoDevice_t * const VideoDevice_p);
void stvid_LeaveCriticalSection(VideoDevice_t * const VideoDevice_p);

ST_ErrorCode_t stvid_ActionDisableOutputWindow(VideoViewPort_t * const ViewPort_p);
ST_ErrorCode_t stvid_ActionEnableOutputWindow(VideoViewPort_t * const ViewPort_p);
ST_ErrorCode_t stvid_ActionCloseViewPort(VideoViewPort_t * const ViewPort_p, STLAYER_ViewPortParams_t * const ViewportParamsForReOpenOrNullToForceClose_p);
ST_ErrorCode_t stvid_ActionStart(VideoDevice_t * const Device_p, const STVID_StartParams_t * const Params_p);
ST_ErrorCode_t stvid_ActionStop(VideoDevice_t * const Device_p, const STVID_Stop_t StopMode, const STVID_Freeze_t * const Freeze_p);

void stvid_BuffersImpossibleWithProfile(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);

#ifdef ST_producer
void stvid_DecoderStopped(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
void stvid_DecodeOnceReady(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
void stvid_NewPictureSkippedWithoutRequest(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
void stvid_ReadyToDecodeNewPicture(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
#endif /* ST_producer */

#ifdef ST_display
void stvid_DisplayOutOfPictures(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
void stvid_PictureCharacteristicsChange(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
void stvid_GraphicsPictureCharacteristicsChange( VideoViewPort_t * const ViewPort_p,
        VIDDISP_PictureCharacteristicsChangeParams_t * const PictureCharacteristicsParams_p);
#endif /* ST_display */

#if defined ST_trickmod || defined ST_speed
void stvid_TrickmodeDisplayParamsChange(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
void stvid_TrickmodeVsyncActionSpeed(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
void stvid_TrickmodePostVsyncAction(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
#endif /* ST_trickmod || ST_speed */

#if defined (ST_diginput) && defined (ST_MasterDigInput)
void stvid_DvpPictureCharacteristicsChange(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);

ST_ErrorCode_t stvid_DvpSetIOWindows(const STVID_ViewPortHandle_t    ViewPortHandle,
                S32 const InputWinX,            S32 const InputWinY,
                U32 const InputWinWidth,        U32 const InputWinHeight,
                S32 const OutputWinX,           S32 const OutputWinY,
                U32 const OutputWinWidth,       U32 const OutputWinHeight);
#endif/* #ifdef ST_diginput && ST_MasterDigInput */

#ifdef ST_producer
ST_ErrorCode_t AffectDecodedPictures(VideoDevice_t * const Device_p,
                                     const STVID_MemoryProfile_t * const MemoryProfile_p,
                                     const U8 FrameBufferHoldTime,
                                     STVID_DecodedPictures_t* const  DecodedPictures_p);
ST_ErrorCode_t AffectTheBuffersFreeingStrategy(
        const STVID_DeviceType_t DeviceType,
        const U8 NbFrameStore,
        const STVID_DecodedPictures_t DecodedPictures,
        VIDBUFF_StrategyForTheBuffersFreeing_t * const StrategyForTheBuffersFreeing_p);
#endif /* ST_producer */

ST_ErrorCode_t stvid_GetDeviceNameArray(ST_DeviceName_t* DeviceNameArray, U32* NbDevices_p);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_API_H */

/* End of api.h */
