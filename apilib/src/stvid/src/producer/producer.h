/*******************************************************************************

File name   : producer.h

Description : Video producer header file

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
26 Jan 2004        Created from decoder module                       HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_PRODUCER_H
#define __VIDEO_PRODUCER_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "stvid.h"

#include "buffers.h"

#ifdef ST_XVP_ENABLE_FGT
#include "vid_fgt.h"
#endif /* ST_XVP_ENABLE_FGT */

#ifdef ST_ordqueue
#include "ordqueue.h"
#endif /* ST_ordqueue */

#ifdef ST_inject
#include "inject.h"
#endif /* ST_inject */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define VIDPROD_MODULE_BASE   0x900
#define VIDPROD_NB_PICTURES_USED_FOR_BITRATE        30

enum
{
    /* This event passes a (VIDPROD_NewPictureDecodeOrSkip_t *) as parameter */
    /* This event can be used to modify the decode module normal behaviour by calling
    VIDPROD_SetNextDecodeAction(), which requests an action before decode. */
    VIDPROD_READY_TO_DECODE_NEW_PICTURE_EVT = STVID_DRIVER_BASE + VIDPROD_MODULE_BASE,
    /* This event passes a (VIDPROD_NewPictureDecodeOrSkip_t *) as parameter */
    /* This event is notified when skipping a picture, but not if requested by
    VIDPROD_SetNextDecodeAction(). This is to inform upper layers that a skip is
    done when they can't be aware of it because VIDPROD decides it (error recovery). */
    VIDPROD_NEW_PICTURE_SKIPPED_NOT_REQUESTED_EVT,

    /* This event returns a (STVID_PictureParams_t *) as parameter */
    VIDPROD_STOPPED_EVT,

    /* This event returns nothing as parameter */
    VIDPROD_DECODE_ONCE_READY_EVT,

    /* This event returns nothing as parameter */
    VIDPROD_RESTARTED_EVT,

    /* This event returns nothing as parameter */
    VIDPROD_DECODE_LAUNCHED_EVT,

    /* This event returns nothing as parameter */
    VIDPROD_SKIP_LAUNCHED_EVT,

    /* Reserved: should not be subscribed ! Just here for reservation of the event constant ! */
    VIDPROD_SELF_INSTALLED_INTERRUPT_EVT,
#ifdef STVID_TRICKMODE_BACKWARD
    VIDPROD_BUFFER_FULLY_PARSED_EVT,
    VIDPROD_DECODE_FIRST_PICTURE_BACKWARD_EVT,
#endif
    /* This event returns a (STVID_PictureParams_t *) as parameter */
    VIDPROD_NEW_PICTURE_PARSED_EVT,

    /* This event returns the number of fields of the skipped picture */
    VIDPROD_PICTURE_SKIPPED_EVT
};

typedef enum VIDPROD_DisplayedReconstruction_e
{
    VIDPROD_DISPLAYED_NONE,
    VIDPROD_DISPLAYED_UNKNOWN,
    VIDPROD_DISPLAYED_PRIMARY_ONLY,
    VIDPROD_DISPLAYED_SECONDARY_ONLY,
    VIDPROD_DISPLAYED_BOTH
} VIDPROD_DisplayedReconstruction_t;

typedef enum
{
    VIDPROD_SKIP_NEXT_B_PICTURE,
#ifdef ST_speed
    VIDPROD_SKIP_ALL_B_PICTURES,
    VIDPROD_SKIP_ALL_P_PICTURES,
    VIDPROD_SKIP_NO_PICTURE,
    VIDPROD_SKIP_NEXT_I,
	VIDPROD_SKIP_UNTIL_NEXT_I_SPEED,
#endif /* ST_speed */
    VIDPROD_SKIP_UNTIL_NEXT_I,
    VIDPROD_SKIP_UNTIL_NEXT_SEQUENCE
} VIDPROD_SkipMode_t;

#ifdef STVID_TRICKMODE_BACKWARD
typedef enum DecodeDirection_e
{
	TRICKMODE_NOCHANGE,
    TRICKMODE_CHANGING_FORWARD,
    TRICKMODE_CHANGING_BACKWARD
} DecodeDirection_t;
#endif /*STVID_TRICKMODE_BACKWARD*/

/* Exported Types ----------------------------------------------------------- */

typedef void * VIDPROD_Handle_t;

typedef struct
{
    ST_Partition_t *    CPUPartition_p;
    ST_DeviceName_t     EventsHandlerName;
    VIDBUFF_Handle_t    BufferManagerHandle;
#ifdef ST_XVP_ENABLE_FGT
    VIDFGT_Handle_t     FGTHandle;
#endif /* ST_XVP_ENABLE_FGT */
#ifdef ST_ordqueue
    VIDQUEUE_Handle_t   OrderingQueueHandle;
#endif /* ST_ordqueue */
    STEVT_EventConstant_t InterruptEvent;
    ST_DeviceName_t     InterruptEventName;
    void *              SharedMemoryBaseAddress_p;
    STAVMEM_PartitionHandle_t AvmemPartitionHandle;
    BOOL                InstallVideoInterruptHandler;
    U32                 InterruptNumber;
    U32                 InterruptLevel;
#ifdef STVID_STVAPI_ARCHITECTURE
    U32                 MB2RInterruptNumber;
    U32                 MB2RInterruptLevel;
#endif /* end of def STVID_STVAPI_ARCHITECTURE */
    ST_DeviceName_t     VideoName;              /* Name of the video instance */
    void *              RegistersBaseAddress;   /* For the HAL */
#if defined(ST_7200)
    char                  MMETransportName[32];
#endif /* #if defined(ST_7200) ...  */
    void *              CompressedDataInputBaseAddress_p;
    BOOL                BitBufferAllocated;
    void *              BitBufferAddress_p;
    U32                 BitBufferSize;
    U32                 UserDataSize;
    STVID_DeviceType_t  DeviceType;
    U8                  DecoderNumber;  /* As defined in vid_com.c. See definition there. */
    ST_DeviceName_t     ClockRecoveryName;
#ifdef ST_inject
    VIDINJ_Handle_t     InjecterHandle;
#endif /* ST_inject */
#ifdef ST_multidecodeswcontroller
    VIDPROD_MultiDecodeHandle_t MultiDecodeHandle;
#endif /* ST_multidecodeswcontroller */
    STVID_CodecMode_t   CodecMode;
} VIDPROD_InitParams_t;

typedef struct
{
    STVID_PictureInfos_t *  PictureInfos_p; /* Pointer to picture infos of the picture (cannot be NULL) */
    VIDCOM_PictureID_t      ExtendedPresentationOrderPictureID;
    BOOL                    IsExtendedPresentationOrderIDValid;
    U32                     NbFieldsForDisplay; /* Number of fields the picture to decode or skip is supposed to be displayed, according to the MPEG norm */
    U16                     TemporalReference;
    U16                     VbvDelay;
#ifdef ST_speed
    BOOL                    UnderflowOccured;
#endif /*ST_speed*/
} VIDPROD_NewPictureDecodeOrSkip_t;

#ifdef STVID_TRICKMODE_BACKWARD
typedef struct
{
    U32                     NbIPicturesInBuffer;
    U32                     NbNotIPicturesInBuffer;
    U32                     TotalNbIFields;
    U32                     TotalNbNotIFields;
    U32                     BufferTreatmentTime;
    U32                     BufferDecodeTime;
    U32                     TimeToDecodeIPicture;
} VIDPROD_BitBUfferInfos_t;
#endif
/* The default mode to pass picture to display when relevant: at decode end or at decode start. */
typedef enum VIDPROD_UpdateDisplayQueue_e
{
    VIDPROD_UPDATE_DISPLAY_QUEUE_NEVER,
    VIDPROD_UPDATE_DISPLAY_QUEUE_AT_DECODE_START,
    VIDPROD_UPDATE_DISPLAY_QUEUE_AT_DECODE_END,
    VIDPROD_UPDATE_DISPLAY_QUEUE_DEFAULT_MODE = VIDPROD_UPDATE_DISPLAY_QUEUE_AT_DECODE_END
} VIDPROD_UpdateDisplayQueue_t;

typedef struct
{
    BOOL        RealTime;
    STVID_StreamType_t StreamType;
    STVID_BroadcastProfile_t BroadcastProfile;
    U8          StreamID;
    BOOL        UpdateDisplay;
    BOOL        DecodeOnce;
    VIDPROD_UpdateDisplayQueue_t UpdateDisplayQueueMode;
} VIDPROD_StartParams_t;

typedef struct {
    BOOL IsLowDelay;
} VIDPROD_DecoderOperatingMode_t;

typedef struct VIDPROD_Infos_s
{
  U32   MaximumDecodingLatencyInSystemClockUnit; /* System Clock = 90kHz */
  U32   FrameBufferAdditionalDataBytesPerMB; /* Number of additional bytes to allocate for each macroblock */
} VIDPROD_Infos_t;

/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */

typedef struct
{
    ST_ErrorCode_t (*ConfigureEvent)(const VIDPROD_Handle_t ProducerHandle, const STEVT_EventID_t Event, const STVID_ConfigureEventParams_t * const Params_p);
    ST_ErrorCode_t (*DataInjectionCompleted)(const VIDPROD_Handle_t ProducerHandle, const STVID_DataInjectionCompletedParams_t * const Params_p);
    ST_ErrorCode_t (*FlushInjection)(const VIDPROD_Handle_t ProducerHandle);
    ST_ErrorCode_t (*Freeze)(const VIDPROD_Handle_t ProducerHandle);
    ST_ErrorCode_t (*GetBitBufferFreeSize)(const VIDPROD_Handle_t ProducerHandle, U32 * const FreeSize_p);
    ST_ErrorCode_t (*GetDecodedPictures)(const VIDPROD_Handle_t ProducerHandle, STVID_DecodedPictures_t * const DecodedPictures_p);
    ST_ErrorCode_t (*GetDecoderSpecialOperatingMode)(const VIDPROD_Handle_t ProducerHandle, VIDPROD_DecoderOperatingMode_t * const DecoderOperatingMode_p);
    ST_ErrorCode_t (*GetErrorRecoveryMode)(const VIDPROD_Handle_t ProducerHandle, STVID_ErrorRecoveryMode_t * const Mode_p);
    ST_ErrorCode_t (*GetHDPIPParams)(const VIDPROD_Handle_t ProducerHandle, STVID_HDPIPParams_t * const HDPIPParams_p);
    ST_ErrorCode_t (*GetLastDecodedPictureInfos)(const VIDPROD_Handle_t ProducerHandle, STVID_PictureInfos_t * const PictureInfos_p);
    ST_ErrorCode_t (*GetInfos)(const VIDPROD_Handle_t ProducerHandle, VIDPROD_Infos_t * const Infos_p);
#ifdef STVID_DEBUG_GET_STATISTICS
    ST_ErrorCode_t (*GetStatistics)(const VIDPROD_Handle_t ProducerHandle, STVID_Statistics_t * const Statistics_p);
    ST_ErrorCode_t (*ResetStatistics)(const VIDPROD_Handle_t ProducerHandle, const STVID_Statistics_t * const Statistics_p);
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
    ST_ErrorCode_t (*GetStatus)(const VIDPROD_Handle_t ProducerHandle, STVID_Status_t * const Status_p);
#endif /* STVID_DEBUG_GET_STATUS */
    ST_ErrorCode_t (*Init)(const VIDPROD_InitParams_t * const InitParams_p, VIDPROD_Handle_t * const ProducerHandle_p);
    ST_ErrorCode_t (*NewReferencePicture)(const VIDPROD_Handle_t ProducerHandle, VIDBUFF_PictureBuffer_t * const Picture_p);
    ST_ErrorCode_t (*NotifyDecodeEvent)(const VIDPROD_Handle_t ProducerHandle, STEVT_EventID_t EventID, const void * EventData);
    ST_ErrorCode_t (*Resume)(const VIDPROD_Handle_t ProducerHandle);
    ST_ErrorCode_t (*SetDecodedPictures)(const VIDPROD_Handle_t ProducerHandle, const STVID_DecodedPictures_t DecodedPictures);
    ST_ErrorCode_t (*SetErrorRecoveryMode)(const VIDPROD_Handle_t ProducerHandle, const STVID_ErrorRecoveryMode_t ErrorRecoveryMode);
    ST_ErrorCode_t (*SetHDPIPParams)(const VIDPROD_Handle_t ProducerHandle, const STVID_HDPIPParams_t * const HDPIPParams_p);
    ST_ErrorCode_t (*Setup)(const VIDPROD_Handle_t ProducerHandle, const STVID_SetupParams_t * const SetupParams_p); /* Only STVID_SetupObject_t STVID_SETUP_DECODER_INTERMEDIATE_BUFFER_PARTITION supported */
    ST_ErrorCode_t (*SkipData)(const VIDPROD_Handle_t ProducerHandle, const VIDPROD_SkipMode_t Mode);
    ST_ErrorCode_t (*Start)(const VIDPROD_Handle_t ProducerHandle, const VIDPROD_StartParams_t * const StartParams_p);
    ST_ErrorCode_t (*StartUpdatingDisplay)(const VIDPROD_Handle_t ProducerHandle);
    ST_ErrorCode_t (*Stop)(const VIDPROD_Handle_t ProducerHandle, const STVID_Stop_t StopMode,const BOOL StopAfterBuffersTimeOut);
    ST_ErrorCode_t (*Term)(const VIDPROD_Handle_t ProducerHandle);
#ifdef STVID_STVAPI_ARCHITECTURE
    void (*SetExternalRasterBufferManagerPoolId)(const VIDPROD_Handle_t ProducerHandle, const DVRBM_BufferPoolId_t BufferPoolId);
    ST_ErrorCode_t (*SetRasterReconstructionMode)(const VIDPROD_Handle_t ProducerHandle, const BOOL EnableMainRasterRecontruction, const STGVOBJ_DecimationFactor_t Decimation);
#endif /* end of def STVID_STVAPI_ARCHITECTURE */
#ifdef ST_deblock
    void (*SetDeblockingMode)(const VIDPROD_Handle_t ProducerHandle, const BOOL EnableDeblocking);
#ifdef VIDEO_DEBLOCK_DEBUG
    void (*SetDeblockingStrength)(const VIDPROD_Handle_t ProducerHandle, const int DeblockingStrength);
#endif /* VIDEO_DEBLOCK_DEBUG  */
#endif /* ST_deblock */
/* extension for injection controle */
    ST_ErrorCode_t (*SetDataInputInterface)(const VIDPROD_Handle_t ProducerHandle,
            ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle, void ** const Address_p),
            void (*InformReadAddress)(void * const Handle, void * const Address),
            void * const Handle ); /* Handle to pass to the 2 function calls */
    ST_ErrorCode_t (*DeleteDataInputInterface)(const VIDPROD_Handle_t ProducerHandle);
    ST_ErrorCode_t (*GetDataInputBufferParams)(const VIDPROD_Handle_t ProducerHandle, void ** const BaseAddress_p, U32 * const Size_p);
    ST_ErrorCode_t (*GetBitBufferParams)(const VIDPROD_Handle_t ProducerHandle, void ** const Address_p, U32 * const Size_p);
} VIDPROD_FunctionsTable_t;

typedef struct {
    const VIDPROD_FunctionsTable_t * FunctionsTable_p;
    ST_Partition_t *        CPUPartition_p; /* Where the module can allocate memory for its internal usage */
    VIDPROD_Handle_t        InternalProducerHandle;
} VIDPROD_Properties_t;

#ifndef ST_multicodec /* if ST_oldmpeg2codec only then export VIDDEC_Init and VIDDEC_Term functions and call them directly */

ST_ErrorCode_t VIDDEC_Init(const VIDPROD_InitParams_t * const InitParams_p, VIDPROD_Handle_t * const DecodeHandle_p);
ST_ErrorCode_t VIDDEC_Term(const VIDPROD_Handle_t DecodeHandle);

#define VIDPROD_Init(InitParams_p, DecodeHandle_p)  VIDDEC_Init((InitParams_p), (DecodeHandle_p))
#define VIDPROD_Term(DecodeHandle)                  VIDDEC_Term((DecodeHandle))

#ifdef ST_SecondInstanceSharingTheSameDecoder
ST_ErrorCode_t VIDDEC_InitSecond(const VIDPROD_InitParams_t * const InitParams_p, const VIDPROD_Handle_t DecodeHandle);
ST_ErrorCode_t VIDDEC_StopSecond(const VIDPROD_Handle_t ProducerHandle, const STVID_Stop_t StopMode, const BOOL StopAfterBuffersTimeOut);
ST_ErrorCode_t VIDDEC_TermSecond(const VIDPROD_Handle_t DecodeHandle);
#define VIDPROD_InitSecond(InitParams_p, DecodeHandle)  VIDDEC_InitSecond((InitParams_p), (((VIDPROD_Properties_t *)(DecodeHandle))->InternalProducerHandle))
#define VIDPROD_StopSecond(DecodeHandle, StopMode, StopAfterBuffersTimeOut) VIDDEC_StopSecond((((VIDPROD_Properties_t *)(DecodeHandle))->InternalProducerHandle), (StopMode), (StopAfterBuffersTimeOut))
#define VIDPROD_TermSecond(DecodeHandle)            VIDDEC_TermSecond((((VIDPROD_Properties_t *)(DecodeHandle))->InternalProducerHandle))
#endif /* ST_SecondInstanceSharingTheSameDecoder*/

#else /* not ST_multicodec */

ST_ErrorCode_t VIDPROD_Init(const VIDPROD_InitParams_t * const InitParams_p, VIDPROD_Handle_t * const ProducerHandle_p);
ST_ErrorCode_t VIDPROD_Term(const VIDPROD_Handle_t ProducerHandle);
#ifdef VALID_TOOLS
ST_ErrorCode_t VIDPROD_RegisterTrace(void);
#endif /* VALID_TOOLS */

#endif /* ST_multicodec */

#define VIDPROD_ConfigureEvent(ProducerHandle, Event, Params_p)                                           ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->ConfigureEvent(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (Event), (Params_p))
#define VIDPROD_DataInjectionCompleted(ProducerHandle, Params_p)                                          ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->DataInjectionCompleted(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (Params_p))
#define VIDPROD_FlushInjection(ProducerHandle)                                                            ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->FlushInjection(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle)
#define VIDPROD_Freeze(ProducerHandle)                                                                    ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->Freeze(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle)
#define VIDPROD_GetBitBufferFreeSize(ProducerHandle, FreeSize_p)                                          ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->GetBitBufferFreeSize(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (FreeSize_p))
#define VIDPROD_GetDecodedPictures(ProducerHandle, DecodedPictures_p)                                     ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->GetDecodedPictures(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (DecodedPictures_p))
#define VIDPROD_GetDecoderSpecialOperatingMode(ProducerHandle, DecoderOperatingMode_p)                    ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->GetDecoderSpecialOperatingMode (((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (DecoderOperatingMode_p))
#define VIDPROD_GetErrorRecoveryMode(ProducerHandle, Mode_p)                                              ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->GetErrorRecoveryMode(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (Mode_p))
#define VIDPROD_GetHDPIPParams(ProducerHandle, HDPIPParams_p)                                             ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->GetHDPIPParams(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (HDPIPParams_p))
#define VIDPROD_GetLastDecodedPictureInfos(ProducerHandle, PictureInfos_p)                                ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->GetLastDecodedPictureInfos(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (PictureInfos_p))
#define VIDPROD_GetInfos(ProducerHandle, Infos_p)                                                         ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->GetInfos(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (Infos_p))
#ifdef STVID_DEBUG_GET_STATISTICS
#define VIDPROD_GetStatistics(ProducerHandle, Statistics_p)                                               ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->GetStatistics(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (Statistics_p))
#define VIDPROD_ResetStatistics(ProducerHandle, Statistics_p)                                             ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->ResetStatistics(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (Statistics_p))
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
#define VIDPROD_GetStatus(ProducerHandle, Status_p)                                                       ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->GetStatus(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (Status_p))
#endif /* STVID_DEBUG_GET_STATUS */
#define VIDPROD_NewReferencePicture(ProducerHandle, Picture_p)                                            ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->NewReferencePicture(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (Picture_p))
#define VIDPROD_NotifyDecodeEvent(ProducerHandle, EventID, EventData)                                     ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->NotifyDecodeEvent(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (EventID), (EventData))
#define VIDPROD_Resume(ProducerHandle)                                                                    ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->Resume(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle)
#define VIDPROD_SetDecodedPictures(ProducerHandle, DecodedPictures)                                       ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->SetDecodedPictures(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (DecodedPictures))
#define VIDPROD_SetErrorRecoveryMode(ProducerHandle, ErrorRecoveryMode)                                   ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->SetErrorRecoveryMode(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (ErrorRecoveryMode))
#define VIDPROD_SetHDPIPParams(ProducerHandle, HDPIPParams_p)                                             ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->SetHDPIPParams(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (HDPIPParams_p))
#define VIDPROD_Setup(ProducerHandle, SetupParams_p)                                                      ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->Setup(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (SetupParams_p))
#define VIDPROD_SkipData(ProducerHandle, Mode)                                                            ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->SkipData(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (Mode))
#define VIDPROD_Start(ProducerHandle, StartParams_p)                                                      ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->Start(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (StartParams_p))
#define VIDPROD_StartUpdatingDisplay(ProducerHandle)                                                      ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->StartUpdatingDisplay(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle)
#define VIDPROD_Stop(ProducerHandle, StopMode,StopAfterBuffersTimeOut)                                    ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->Stop(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (StopMode),(StopAfterBuffersTimeOut))
#ifdef STVID_STVAPI_ARCHITECTURE
#define VIDPROD_SetExternalRasterBufferManagerPoolId(ProducerHandle,BufferPoolId)                         ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->SetExternalRasterBufferManagerPoolId(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle,(BufferPoolId))
#define VIDPROD_SetRasterReconstructionMode(ProducerHandle, EnableMainRasterRecontruction, Decimation)    ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->SetRasterReconstructionMode(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle,(EnableMainRasterRecontruction),(Decimation))
#endif /* end of def STVID_STVAPI_ARCHITECTURE */
#ifdef ST_deblock
#define VIDPROD_SetDeblockingMode(ProducerHandle, EnableDeblocking)                                       ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->SetDeblockingMode(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle,(EnableDeblocking))
#ifdef VIDEO_DEBLOCK_DEBUG
#define VIDPROD_SetDeblockingStrength(ProducerHandle, DeblockingStrength)                                 ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->SetDeblockingStrength(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle,(DeblockingStrength))
#endif /* VIDEO_DEBLOCK_DEBUG  */
#endif /* ST_deblock */
/* extension for injection control */
#define VIDPROD_SetDataInputInterface(ProducerHandle, GetWriteAddress, InformReadAddress,Handle )         ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->SetDataInputInterface(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (GetWriteAddress), (InformReadAddress), (Handle))
#define VIDPROD_DeleteDataInputInterface(ProducerHandle)                                                  ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->DeleteDataInputInterface(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle)
#define VIDPROD_GetDataInputBufferParams(ProducerHandle, BaseAddress_p, Size_p)                           ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->GetDataInputBufferParams(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (BaseAddress_p), (Size_p))
#define VIDPROD_GetBitBufferParams(ProducerHandle, Address_p, Size_p)                                     ((VIDPROD_Properties_t *)(ProducerHandle))->FunctionsTable_p->GetBitBufferParams(((VIDPROD_Properties_t *)(ProducerHandle))->InternalProducerHandle, (Address_p), (Size_p))

#if defined(DVD_SECURED_CHIP)
ST_ErrorCode_t VIDPROD_SetupForH264PreprocWA_GNB42332(const VIDPROD_Handle_t ProducerHandle, const STAVMEM_PartitionHandle_t AVMEMPartition);
#endif /* DVD_SECURED_CHIP */

#ifdef ST_speed
ST_ErrorCode_t VIDPROD_GetBitRateValue(const VIDPROD_Handle_t ProducerHandle, U32 * const BitRateValue_p);
void VIDPROD_SetDecodingMode(const VIDPROD_Handle_t ProducerHandle, const U8 Mode);
#ifdef STVID_TRICKMODE_BACKWARD
void VIDPROD_ChangeDirection(const VIDPROD_Handle_t ProducerHandle, const DecodeDirection_t Change);
#endif
void VIDPROD_UpdateDriftTime(const VIDPROD_Handle_t ProducerHandle, const S32 DriftTime);
void VIDPROD_ResetDriftTime(const VIDPROD_Handle_t ProducerHandle);
#endif /*ST_speed*/

ST_ErrorCode_t VIDPROD_UpdateBitBufferParams(const VIDPROD_Handle_t ProducerHandle, void * const Address_p, const U32 Size);
/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_PRODUCER_H */

/* End of producer.h */
