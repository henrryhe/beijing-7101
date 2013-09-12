/*******************************************************************************

File name   : display.h

Description : Video display module header file

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
21 Mar 2000        Created                                           HG
01 Feb 2001        Digital Input merge                               JA
09 Mar 2001        new VIDDISP_SetReconstructionMode() function      GG
16 Mar 2001        new VIDDISP_ConfigureEvent() function             GG
   Sep 2002        Dual Display                                      MB
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_DISPLAY_H
#define __VIDEO_DISPLAY_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stvid.h"
#include "buffers.h"
#include "vid_com.h"

#ifdef ST_crc
#include "crc.h"
#endif /* ST_crc */

#ifdef ST_XVP_ENABLE_FGT
#include "vid_fgt.h"
#endif /* ST_XVP_ENABLE_FGT */

/* C++ support -------------------------------------------------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define DISPLAY_HAL_MAX_NUMBER      3

#define VIDDISP_MODULE_BASE   0x700

enum
{
    /* This event passes a (VIDDISP_PictureCharacteristicsChangeParams_t *) */
    VIDDISP_PICTURE_CHARACTERISTICS_CHANGE_EVT
                        = STVID_DRIVER_BASE + VIDDISP_MODULE_BASE,
    /* This event passes a (VIDDISP_VsyncDisplayNewPictureParams_t *) */
    VIDDISP_VSYNC_DISPLAY_NEW_PICTURE_EVT,
    /* This event passes a (VIDDISP_NewPicturePreparedParams_t *) */
    VIDDISP_NEW_PICTURE_PREPARED_EVT,
    /* This event passes a (STVTG_VSYNC_t *) as parameter. It is used by the */
    /* display HAL instead of using VTG's VSync. */
    VIDDISP_VSYNC_EVT,
    /* This event passes a (STGXOBJ_ScanType_t *) as parameter. It is used by the */
    /* trickmode to know if the display is either progressive or interlaced. */
    VIDDISP_VTG_SCANTYPE_CHANGE_EVT,
    /* This event is notified just before the display task beginings processing
     * a new Vsync to allow trickmode patch display counters and also to lock
     * access to the trickmode params */
    VIDDISP_PRE_DISPLAY_VSYNC_EVT,
    /* This event is notified just after the display task has finished processing
     * a new Vsync to allow trickmode unlock the access to the trickmode params */
    VIDDISP_POST_DISPLAY_VSYNC_EVT,
    VIDDISP_OUT_OF_PICTURES_EVT,
    /* This event passes a (VIDDISP_LayerUpdateParams_t *) parameter. Used by */
    /* API layer to update gfx layer source pointer */
    /* Only used in case of Gfx Layer display (generic digital input)*/
    VIDDISP_UPDATE_GFX_LAYER_SRC_EVT,
#ifdef ST_smoothbackward
    /* This event passes a NULL parameter. Used to inform smooth backward */
    /* trickmod management that display could not display */
    /* next picture of display queue because it is missing. */
    VIDDISP_NO_NEXT_PICTURE_TO_DISPLAY_EVT,
#endif
    /* This evt passes an U32 as parameter. It's the num of repeated fields. */
    VIDDISP_FIELDS_REPEATED_EVT,
    /* This event passes a (VIDDISP_NewPictureSkippedWithNoRequest_t)*/
    /* parameter. It's the picture currently skipped by  */
    /* display without request.       */
    VIDDISP_NEW_PICTURE_SKIPPED_NOT_REQUESTED_EVT,
    /* This event passes a (STVID_PictureInfos_t) parameter. It's notified just after video display
     * params commit */
    VIDDISP_COMMIT_NEW_PICTURE_PARAMS_EVT
};

typedef enum VIDDISP_DisplayStart_e
{
    /* Care I, P, B type to display, with possible wait for reference */
    /* pictures after stop */
    VIDDISP_DISPLAY_START_CARE_MPEG_FRAME,
    /* Display picture as soon as they are available */
    VIDDISP_DISPLAY_START_AS_AVAILABLE
} VIDDISP_DisplayStart_t;

typedef enum VIDDISP_PictureCharacteristicsChanging_e
{
    /* Only MPEGFrameCentreOffsets has changed */
    VIDDISP_CHANGING_ONLY_FRAME_CENTER_OFFSETS,
    /* Everything has changed */
    VIDDISP_CHANGING_EVERYTHING
} VIDDISP_PictureCharacteristicsChanging_t;

typedef enum VIDDISP_Stop_e
{
    VIDDISP_STOP_NOW,
    VIDDISP_STOP_ON_PTS,
    VIDDISP_STOP_LAST_PICTURE_IN_QUEUE
} VIDDISP_Stop_t;

/* Update reasons. Use 2^n values to be able to OR reasons */
typedef enum VIDDISP_LayerUpdateReason_e
{
    VIDDISP_LAYER_UPDATE_TOP_FIELD_ADDRESS      = 1,
    VIDDISP_LAYER_UPDATE_BOTTOM_FIELD_ADDRESS   = 2
} VIDDISP_LayerUpdateReason_t;

/* Exported Types ----------------------------------------------------------- */

typedef void * VIDDISP_Handle_t;

typedef struct VIDDISP_DisplayParams_s
{
    VIDDISP_DisplayStart_t  DisplayStart;
    /* TRUE: display fields of a picture in forward order */
    /* FALSE: display fields of a picture in backward order */
    BOOL                    DisplayForwardNotBackward;
    /* TRUE: care about the polarity of the first field displayed of the */
    /* picture */
    /* FALSE: don't care, eventually displays top on bottom and vice versa */
    BOOL                    DisplayWithCareOfFieldsPolarity;
} VIDDISP_DisplayParams_t;

typedef struct VIDDISP_InitParams_s
{
    ST_Partition_t *    CPUPartition_p;
    ST_DeviceName_t     EventsHandlerName;
    VIDBUFF_Handle_t    BufferManagerHandle;
    STAVMEM_PartitionHandle_t   AvmemPartitionHandle;
#ifdef ST_crc
    VIDCRC_Handle_t     CRCHandle;
#endif /* ST_crc */
#ifdef ST_XVP_ENABLE_FGT
    VIDFGT_Handle_t     FGTHandle;
#endif /* ST_XVP_ENABLE_FGT */
    void *              RegistersBaseAddress[DISPLAY_HAL_MAX_NUMBER];   /* For the HALs */
    STVID_DeviceType_t  DeviceType;
    ST_DeviceName_t     VideoName;
    U8                  DecoderNumber;
    U8                  MaxDisplayHAL;  /* Number of display HALs to initialize */
} VIDDISP_InitParams_t;

typedef struct VIDDISP_NewPictureSkippedWithNoRequest_s
{
    U32                 NbFieldsSkipped;
} VIDDISP_NewPictureSkippedWithNoRequest_t;

typedef struct VIDDISP_PictureCharacteristicsChangeParams_s
{
    VIDDISP_PictureCharacteristicsChanging_t CharacteristicsChanging;
    /* NULL if VIDDISP_CHANGING_ONLY_FRAME_CENTER_OFFSETS (should never be NULL else) */
    STVID_PictureInfos_t *              PictureInfos_p;
    VIDCOM_PanAndScanIn16thPixel_t      PanAndScanIn16thPixel;
    VIDCOM_FrameCropInPixel_t           FrameCropInPixel;
    STLAYER_Layer_t                     LayerType;
#ifdef ST_crc
    BOOL                                CRCCheckMode;
    /* CRCScanType is mandatory because it is not allowed to change original*/
    /* ScanType from parsed picture */
    STGXOBJ_ScanType_t                  CRCScanType;
#endif /* ST_crc */
} VIDDISP_PictureCharacteristicsChangeParams_t;

typedef struct VIDDISP_ShowParams_s
{
    void                      * Data_p;
    void                      * ChromaData_p;
    U32                         Height;
    U32                         Width;
    STVID_DecimationFactor_t    HDecimation;
    STVID_DecimationFactor_t    VDecimation;
    STVID_CompressionLevel_t    Compression;
    STGXOBJ_ScanType_t          ScanType;
    STGXOBJ_BitmapType_t        BitmapType;
} VIDDISP_ShowParams_t;

typedef struct VIDDISP_BlitterDisplayUpdateParams_s
{
    void *  LumaBuff_p;
    void *  ChromaBuff_p;
    BOOL    FieldInverted;
}VIDDISP_BlitterDisplayUpdateParams_t;

typedef enum VIDDISP_DisplayState_e
{
    /* Normal use of the Display process.*/
    VIDDISP_DISPLAY_STATE_DISPLAYING,
    /* The Display process is stopped. The Decode process is stopped too. */
    VIDDISP_DISPLAY_STATE_PAUSED,
    /* The Display process is stopped. The Decode process is still running. */
    VIDDISP_DISPLAY_STATE_FREEZED
} VIDDISP_DisplayState_t;


typedef enum
{
    VIDDISP_TOP_FIELD,
    VIDDISP_BOTTOM_FIELD
} VIDDISP_FieldType_t;

typedef struct VIDDISP_Field_s
{
    BOOL                            FieldAvailable;
    U32                             PictureIndex;
    VIDDISP_FieldType_t             FieldType;
} VIDDISP_Field_t;

typedef struct VIDDISP_LayerUpdateParams_s
{
    VIDDISP_LayerUpdateReason_t    UpdateReason;
    BOOL                           PresentedFieldInverted;
	STLAYER_Layer_t				   LayerType;
    /* Address of top field : Meaningfull only if Top address update */
    void *                         Data1_p;
    /* Address of bottom field : Meaningfull only if Bottom address update */
    void *                         Data2_p;
    VIDDISP_Field_t                PreviousField;
    VIDDISP_Field_t                CurrentField;
    VIDDISP_Field_t                NextField;
    BOOL                           AdvancedHDecimation ;
    STVID_DecimationFactor_t       DecimationFactors;
    BOOL                           IsDisplayStopped;
    BOOL                           PeriodicFieldInversionDueToFRC;
}VIDDISP_LayerUpdateParams_t;

typedef struct VIDDISP_VsyncDisplayNewPictureParams_s
{
    STVID_PictureInfos_t *  PictureInfos_p;
    /* In units of PTS/STC, i.e. 90kHz period */
    U32                     TimeElapsedSinceVSyncAtNotification;
    /* Set TRUE if the currently displayed picture has been */
    BOOL                    IsAnyFrameRateConversionActivated;
    /* modified for any frame rate conversion.              */
    BOOL                    IsDisplayedWithCareOfPolarity;
    U32                     OutputFrameRate;
    U32                     DisplayDelay;               /* Latency introduced by the Display (in 90kHz units) */
} VIDDISP_VsyncDisplayNewPictureParams_t;

typedef VIDDISP_VsyncDisplayNewPictureParams_t VIDDISP_NewPicturePreparedParams_t;

typedef struct VIDDISP_LayerConnectionParams_s
{
    STLAYER_Handle_t    LayerHandle;
    STLAYER_Layer_t     LayerType;
    U8                  DisplayLatency;
    U8                  FrameBufferHoldTime;
} VIDDISP_LayerConnectionParams_t;

typedef enum VIDDISP_SkipMode_e
{
    VIDDISP_SKIP_STARTING_FROM_PREPARED_PICTURE_FOR_NEXT_VSYNC,
    VIDDISP_SKIP_STARTING_FROM_CURRENTLY_DISPLAYED_PICTURE
} VIDDISP_SkipMode_t;

typedef enum
{
    VIDDISP_SHOW_VIDEO,                 /* Usual case: No picture show requested by the application */
    VIDDISP_SHOW_PICTURE,               /* The application requests to show a picture */
    VIDDISP_RESTORE_ORIGINAL_PICTURE    /* The application request to hide the picture previously shown */
} VIDDISP_ShowPicture_t;

/* Exported Functions ------------------------------------------------------- */

/* Display Commands Api */
/*----------------------*/
void VIDDISP_GetState(const VIDDISP_Handle_t DisplayHandle,
                      VIDDISP_DisplayState_t* DisplayState);
ST_ErrorCode_t VIDDISP_Freeze(const VIDDISP_Handle_t DisplayHandle,
                            const STVID_Freeze_t * const Freeze_p);
ST_ErrorCode_t VIDDISP_Pause(const VIDDISP_Handle_t DisplayHandle,
                            const STVID_Freeze_t * const Freeze_p);
ST_ErrorCode_t VIDDISP_RepeatFields(const VIDDISP_Handle_t DisplayHandle,
                                    const U32 NumberOfFields,
                                    U32 * const NumberDone_p);
ST_ErrorCode_t VIDDISP_DisplayBitmap(const VIDDISP_Handle_t DisplayHandle,
                                    BOOL MainNotAux, STGXOBJ_Bitmap_t * BitmapParams_p);
ST_ErrorCode_t VIDDISP_Resume(const VIDDISP_Handle_t DisplayHandle);
ST_ErrorCode_t VIDDISP_Skip(const VIDDISP_Handle_t DisplayHandle);
ST_ErrorCode_t VIDDISP_SkipFields(const VIDDISP_Handle_t DisplayHandle,
                                  const U32 NumberOfFields,
                                  U32 * const NumberDone_p,
                                  const BOOL SkipAllFieldsOfSamePictureAllowed,
                                  const VIDDISP_SkipMode_t SkipMode);
ST_ErrorCode_t VIDDISP_Start(const VIDDISP_Handle_t DisplayHandle, const BOOL KeepLastValuesOfChangeEVT);
ST_ErrorCode_t VIDDISP_Step(const VIDDISP_Handle_t DisplayHandle);
ST_ErrorCode_t VIDDISP_Stop(const VIDDISP_Handle_t DisplayHandle,
                            const STVID_PTS_t * const LastPicturePTS_p,
                            const VIDDISP_Stop_t StopMode);
/* Init-Config-Term Api */
/*----------------------*/
ST_ErrorCode_t VIDDISP_Init(const VIDDISP_InitParams_t * const InitParams_p,
                            VIDDISP_Handle_t * const DisplayHandle_p);
ST_ErrorCode_t VIDDISP_Reset(const VIDDISP_Handle_t DisplayHandle);
ST_ErrorCode_t VIDDISP_LayerConnection(const VIDDISP_Handle_t DisplayHandle,
                                        const VIDDISP_LayerConnectionParams_t* LayerConnectionParams,
                                        const BOOL IsOpened);
ST_ErrorCode_t VIDDISP_NewVTG(const VIDDISP_Handle_t DisplayHandle,
                              BOOL MainNotAux,
                              const STLAYER_VTGParams_t * const VTGParams_p);
ST_ErrorCode_t VIDDISP_ConfigureEvent(const VIDDISP_Handle_t DecodeHandle,
                           const STEVT_EventID_t Event,
                           const STVID_ConfigureEventParams_t * const Params_p);
ST_ErrorCode_t VIDDISP_FreeNotDisplayedPicturesFromDisplayQueue(
                                          const VIDDISP_Handle_t DisplayHandle);
ST_ErrorCode_t VIDDISP_Term(const VIDDISP_Handle_t DisplayHandle);

ST_ErrorCode_t VIDDISP_DisableDisplay(const VIDDISP_Handle_t DisplayHandle, const ST_DeviceName_t deviceName);
ST_ErrorCode_t VIDDISP_EnableDisplay(const VIDDISP_Handle_t DisplayHandle, const ST_DeviceName_t deviceName);

/* Settings Api */
/*--------------*/
void VIDDISP_DisableFrameRateConversion(const VIDDISP_Handle_t DisplayHandle);
void VIDDISP_EnableFrameRateConversion(const VIDDISP_Handle_t DisplayHandle);
ST_ErrorCode_t VIDDISP_SetDiscontinuityInMPEGDisplay(
                                        const VIDDISP_Handle_t DisplayHandle);
ST_ErrorCode_t VIDDISP_SetDisplayParams(const VIDDISP_Handle_t DisplayHandle,
                                const VIDDISP_DisplayParams_t * const Params_p);
ST_ErrorCode_t VIDDISP_SetReconstructionMode(
                         const VIDDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t LayerType,
                const VIDBUFF_AvailableReconstructionMode_t ReconstructionMode);
ST_ErrorCode_t VIDDISP_GetDisplayParams(const VIDDISP_Handle_t DisplayHandle,
                                    VIDDISP_DisplayParams_t * const Params_p);
ST_ErrorCode_t VIDDISP_GetReconstructionMode(
                     const VIDDISP_Handle_t DisplayHandle,
                     const STLAYER_Layer_t LayerType,
            VIDBUFF_AvailableReconstructionMode_t * const ReconstructionMode_p);
ST_ErrorCode_t VIDDISP_GetDisplayedPictureInfos(
                            const VIDDISP_Handle_t DisplayHandle,
                            STVID_PictureInfos_t * const PictureInfos_p);
ST_ErrorCode_t VIDDISP_GetDisplayedPictureExtendedInfos(
                                const VIDDISP_Handle_t DisplayHandle,
                                VIDBUFF_PictureBuffer_t * const PictureExtendedInfos_p);
ST_ErrorCode_t VIDDISP_NewMixerConnection(
                            const VIDDISP_Handle_t DisplayHandle,
                            const U32 DisplayLatency,
                            const U32 FrameBufferHoldTime);
ST_ErrorCode_t VIDDISP_GetFrameBufferHoldTime(
                            const VIDDISP_Handle_t DisplayHandle,
                            U8* const FrameBufferHoldTime_p);
void VIDDISP_SetSlaveDisplayUpdate(const VIDDISP_Handle_t DisplayHandle,
                                   const BOOL IsViewportInitialized);
void VIDDISP_UpdateLayerParams(const VIDDISP_Handle_t DisplayHandle,
                               const STLAYER_LayerParams_t * LayerParams_p,
                               const U32 LayerId);

/* Manual Mode Api */
/*-----------------*/

ST_ErrorCode_t VIDDISP_ShowPicture(const VIDDISP_Handle_t DisplayHandle,
                   VIDBUFF_PictureBuffer_t * const Picture_p,
                   const U32 Layer,
                   VIDDISP_ShowPicture_t ShowPict);
ST_ErrorCode_t VIDDISP_ShowPictureLayer(const VIDDISP_Handle_t DisplayHandle,
                                     VIDBUFF_PictureBuffer_t * const Picture_p,
                                     U32 LayerIdent,
                                     VIDDISP_ShowPicture_t ShowPict);
/* Optional Api */
/*--------------*/
#ifdef ST_smoothbackward
ST_ErrorCode_t VIDDISP_GetDisplayedPictureParams(
                            const VIDDISP_Handle_t DisplayHandle,
                            STVID_PictureParams_t * const PictureParams_p);
#endif
#ifdef STVID_DEBUG_GET_STATISTICS
ST_ErrorCode_t VIDDISP_GetStatistics(const VIDDISP_Handle_t DisplayHandle,
                                STVID_Statistics_t * const Statistics_p);
ST_ErrorCode_t VIDDISP_ResetStatistics(const VIDDISP_Handle_t DisplayHandle,
                                const STVID_Statistics_t * const Statistics_p);
#endif /* STVID_DEBUG_GET_STATISTICS */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_DISPLAY_H */

/* End of display.h */
