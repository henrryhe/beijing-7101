/*******************************************************************************
File Name   : vidcodec.h

Description : Video driver CODEC API header file

Copyright (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
26 Jan 2004        Created                                           HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STVIDCODEC_H
#define __STVIDCODEC_H

/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"
#include "stos.h"

#ifndef ST_OSLINUX
#include "stlite.h"
#endif

#include "stevt.h"
#include "stavmem.h"

#include "stgxobj.h"
#include "stlayer.h"

#include "stvid.h"

#include "vid_com.h"

#ifdef ST_XVP_ENABLE_FGT
#include "vid_fgt.h"
#endif /* ST_XVP_ENABLE_FGT */

#ifdef ST_inject
#include "inject.h"
#endif /* ST_inject */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Exported Constants ------------------------------------------------------- */

#define PARSER_MODULE_BASE    0xA00
#define DECODER_MODULE_BASE   0xB00

#ifdef ST_speed
enum
{
	NO_DEGRADED_MODE = 0,
	DEGRADED_BSLICES,
	DEGRADED_PSLICES
};

#ifdef STVID_TRICKMODE_BACKWARD
typedef enum DecodingDirection_e
{
    DECODING_FORWARD,
    DECODING_BACKWARD
} DecodingDirection_t;

typedef enum BitBufferType_e
{
    BIT_BUFFER_CIRCULAR,
    BIT_BUFFER_LINEAR
} BitBufferType_t;
#endif /* STVID_TRICKMODE_BACKWARD */
#endif

enum
{
    /* This event passes a (PARSER_ParsingJobResults_t) as parameter */
    PARSER_JOB_COMPLETED_EVT = STVID_DRIVER_BASE + PARSER_MODULE_BASE,
    /* This event passes a (VIDCOM_PictureGenericData_t) as parameter */
    PARSER_PICTURE_SKIPPED_EVT,
    PARSER_FIND_DISCONTINUITY_EVT,
#ifdef STVID_TRICKMODE_BACKWARD
    PARSER_BITBUFFER_FULLY_PARSED_EVT,
#endif
    /* This event passes a (void *) as parameter (StopAddress of the skipped picture) */
    PARSER_USER_DATA_EVT
};

enum
{
    /* This event passes a (DECODER_InformReadAddress_t) as parameter */
    DECODER_INFORM_READ_ADDRESS_EVT = STVID_DRIVER_BASE + DECODER_MODULE_BASE,

    /* This event passes a (DECODER_DecodingJobResults_t) as parameter */
    DECODER_JOB_COMPLETED_EVT,

    /* This event passes a (DECODER_DecodingJobResults_t) as parameter */
    DECODER_CONFIRM_BUFFER_EVT
};

typedef enum DECODER_ErrorCode_e
{
    DECODER_NO_ERROR,
    DECODER_ERROR_PICTURE_SYNTAX,
    DECODER_ERROR_MISSING_SLICE,
    DECODER_ERROR_CORRUPTED
} DECODER_ErrorCode_t;

typedef enum DECODER_ErrorRecoveryMode_e
{
    DECODER_ERROR_RECOVERY_MODE1 = STVID_ERROR_RECOVERY_FULL,
    DECODER_ERROR_RECOVERY_MODE2 = STVID_ERROR_RECOVERY_HIGH,
    DECODER_ERROR_RECOVERY_MODE3 = STVID_ERROR_RECOVERY_PARTIAL,
    DECODER_ERROR_RECOVERY_MODE4 = STVID_ERROR_RECOVERY_NONE
} DECODER_ErrorRecoveryMode_t;

typedef enum DECODER_SourceType_e
{
    DECODER_SOURCE_TYPE_MPEG2_COMPRESSED_PICTURE,
    DECODER_SOURCE_TYPE_H264_COMPRESSED_PICTURE
} DECODER_SourceType_t;

typedef enum DECODER_State_e
{
    DECODER_STATE_IDLE,
    DECODER_STATE_READY_TO_DECODE,
    DECODER_STATE_DECODING
} DECODER_State_t;

typedef enum PARSER_ErrorCode_e
{
    PARSER_NO_ERROR,
    PARSER_ERROR_STREAM_SYNTAX
} PARSER_ErrorCode_t;

typedef enum PARSER_ErrorRecoveryMode_e
{
    PARSER_ERROR_RECOVERY_MODE1 = STVID_ERROR_RECOVERY_FULL,
    PARSER_ERROR_RECOVERY_MODE2 = STVID_ERROR_RECOVERY_HIGH,
    PARSER_ERROR_RECOVERY_MODE3 = STVID_ERROR_RECOVERY_PARTIAL,
    PARSER_ERROR_RECOVERY_MODE4 = STVID_ERROR_RECOVERY_NONE
} PARSER_ErrorRecoveryMode_t;

typedef enum DECODER_JobState_e
{
    DECODER_JOB_UNDEFINED, /* reset value */
    DECODER_JOB_PREPROCESSING, /* Job has just been started (pre-process) */
    DECODER_JOB_WAITING_CONFIRM_BUFFER, /* decoder waits for DECODER_ConfirmBuffer() call (i.e. has notified DECODER_CONFIRM_BUFFER_EVT or doesn't require it)*/
    DECODER_JOB_DECODING,  /* Job is currently processed by the DECODER */
    DECODER_JOB_COMPLETED  /* Job has been entirely processed by the DECODER (DECODER_JOB_COMPLETED_EVT has been issued) */
} DECODER_JobState_t;

/* Exported Types ----------------------------------------------------------- */

typedef U32 CommandId_t;
typedef osclock_t GENERIC_Time_t;

typedef struct DECODER_DecodingJobStatus_s
{
    DECODER_JobState_t  JobState;
    GENERIC_Time_t  JobSubmissionTime;
    GENERIC_Time_t  JobCompletionTime;
#ifdef STVID_VALID_MEASURE_TIMING
    U32    LXDecodeTimeInMicros;
#endif /* STVID_VALID_MEASURE_TIMING */
} DECODER_DecodingJobStatus_t;

typedef struct DECODER_DecodingJobResults_s
{
    DECODER_DecodingJobStatus_t     DecodingJobStatus;
    DECODER_ErrorCode_t             ErrorCode;
    CommandId_t                     CommandId;
} DECODER_DecodingJobResults_t;

typedef struct DECODER_InformReadAddress_s
{
    CommandId_t         CommandId;
    void *              DecoderReadAddress_p;
} DECODER_InformReadAddress_t;

typedef void * DECODER_Handle_t;

typedef struct DECODER_InitParams_s
{
    ST_Partition_t *            CPUPartition_p;
 	STAVMEM_PartitionHandle_t   AvmemPartitionHandle;
	ST_DeviceName_t             EventsHandlerName;
    void *                      SharedMemoryBaseAddress_p;
    void *                      RegistersBaseAddress_p;   /* For the HAL */
#ifdef ST_XVP_ENABLE_FGT
    VIDFGT_Handle_t             FGTHandle;
#endif /* ST_XVP_ENABLE_FGT */
#if defined(ST_7200)
    char                          MMETransportName[32];
#endif /* #if defined(ST_7200) ... */
    ST_DeviceName_t             VideoName;              /* Name of the video instance */
    void *                      BitBufferAddress_p;
    U32                         BitBufferSize;
    STVID_DeviceType_t          DeviceType;
    U8                          DecoderNumber;  /* As defined in vid_com.c. See definition there. */
    DECODER_SourceType_t        SourceType;
    DECODER_ErrorRecoveryMode_t ErrorRecoveryMode;
    struct
    {
        U32                     Event;
        U32                     Level;
        U32                     Number;
    } SharedItParams;
    STVID_CodecMode_t           CodecMode;
} DECODER_InitParams_t;

typedef struct DECODER_DecodePictureParams_s
{
    VIDBUFF_PictureBuffer_t *           PictureInformation;
    DECODER_ErrorRecoveryMode_t ErrorRecoveryMode;
} DECODER_DecodePictureParams_t;

typedef struct DECODER_Status_s
{
    DECODER_State_t             State;
    BOOL                        IsJobSubmitted;
    CommandId_t                 CommandId;
    DECODER_DecodingJobStatus_t DecodingJobStatus;
} DECODER_Status_t;

typedef struct DECODER_ConfirmBufferParams_s
{
    CommandId_t               CommandId;
    BOOL                      ConfirmBuffer;      /* If true, buffer specified at start is OK, no change. Else change to "NewPictureBuffer" */
    VIDBUFF_PictureBuffer_t * NewPictureBuffer_p;
} DECODER_ConfirmBufferParams_t;

typedef struct DECODER_CodecInfo_s
{
  U32   MaximumDecodingLatencyInSystemClockUnit; /* System Clock = 90kHz */
  U32   FrameBufferAdditionalDataBytesPerMB; /* Number of additional bytes to allocate for each macroblock */
  U32   DecodePictureToConfirmBufferMaxTime; /* maximal duration duration between a DECODER_DecodePicture call and the corresponding confirm buffer request event raising <TODO which units, ms ?> */
} DECODER_CodecInfo_t;

typedef struct DECODER_SetupIntermediateBuffersParams_s
{
    STAVMEM_PartitionHandle_t   AvmemPartitionHandleForIntermediateBuffers; /* Partition in which to allocate the intermediate buffers */
} DECODER_SetupIntermediateBuffersParams_t;

typedef struct PARSER_ParsingJobStatus_s
{
    BOOL                 HasErrors;
    PARSER_ErrorCode_t   ErrorCode;
	GENERIC_Time_t       JobSubmissionTime;
    GENERIC_Time_t       JobCompletionTime;
} PARSER_ParsingJobStatus_t;

typedef struct PARSER_ParsingJobResults_s
{
    PARSER_ParsingJobStatus_t           ParsingJobStatus;
    VIDCOM_ParsedPictureInformation_t * ParsedPictureInformation_p;
#ifdef ST_speed
    U32                                 DiffPSC; /* Use to compute the bit rate in backward */
#endif /* speed */
} PARSER_ParsingJobResults_t;

typedef void * PARSER_Handle_t;

typedef struct
{
    void *              BufferAddress_p;
    U32                 BufferSize;
} PARSER_ExtraParams_t;

typedef struct PARSER_InitParams_s
{
    ST_Partition_t *    CPUPartition_p;
    ST_DeviceName_t     EventsHandlerName;
    ST_DeviceName_t     VideoName;              /* Name of the video instance */
    void *              BitBufferAddress_p;
    U32                 BitBufferSize;
    U32                 UserDataSize;
    U8                  DecoderNumber;  /* As defined in vid_com.c. See definition there. */
    STVID_DeviceType_t          DeviceType;
    void *                      SharedMemoryBaseAddress_p;  /* used for DeviceType which includes a FDMA */
    STAVMEM_PartitionHandle_t   AvmemPartitionHandle;       /* used for DeviceType which includes a FDMA */
    void *                      RegistersBaseAddress_p;     /* HW registers access, used for DeviceType  without FDMA  */
    void *                      CompressedDataInputBaseAddress_p; /* CDFIFO, used for DeviceType  without FDMA  */
#ifdef ST_inject
    VIDINJ_Handle_t     InjecterHandle;
#endif /* ST_inject */
#if defined(DVD_SECURED_CHIP)
    PARSER_ExtraParams_t *      SecurityInitParams_p;
#endif /* DVD_SECURED_CHIP */
} PARSER_InitParams_t;

typedef struct PARSER_StartParams_s
{
    STVID_StreamType_t              StreamType;
    STVID_BroadcastProfile_t        BroadcastProfile;
    U8                              StreamID;
    PARSER_ErrorRecoveryMode_t      ErrorRecoveryMode;
    BOOL                            OverwriteFullBitBuffer;
    void *                          BitBufferWriteAddress_p;
    VIDCOM_GlobalDecodingContext_t  *DefaultGDC_p;
    VIDCOM_PictureDecodingContext_t *DefaultPDC_p;
    BOOL                            RealTime;
    BOOL                            ResetParserContext;
} PARSER_StartParams_t;

typedef struct PARSER_GetPictureParams_s
{
	BOOL   GetRandomAccessPoint;
    PARSER_ErrorRecoveryMode_t      ErrorRecoveryMode;
    STVID_DecodedPictures_t         DecodedPictures;
} PARSER_GetPictureParams_t;

typedef struct PARSER_InformReadAddressParams_s
{
    void * DecoderReadAddress_p;
} PARSER_InformReadAddressParams_t;

typedef struct PARSER_GetBitBufferLevelParams_s
{
	U32 BitBufferLevelInBytes;
} PARSER_GetBitBufferLevelParams_t;

/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */

typedef struct
{
    ST_ErrorCode_t (*GetState)(const DECODER_Handle_t DecoderHandle, const CommandId_t CommandId, DECODER_Status_t * const DecoderStatus_p);
    ST_ErrorCode_t (*GetCodecInfo)(const DECODER_Handle_t DecoderHandle, DECODER_CodecInfo_t * const CodecInfo_p, const U32 ProfileAndLevelIndication, const STVID_CodecMode_t CodecMode);
#ifdef STVID_DEBUG_GET_STATISTICS
    ST_ErrorCode_t (*GetStatistics)(const DECODER_Handle_t DecoderHandle, STVID_Statistics_t * const Statistics_p);
    ST_ErrorCode_t (*ResetStatistics)(const DECODER_Handle_t DecoderHandle, const STVID_Statistics_t * const Statistics_p);
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
    ST_ErrorCode_t (*GetStatus)(const DECODER_Handle_t DecoderHandle, STVID_Status_t * const Status_p);
#endif /* STVID_DEBUG_GET_STATUS */
    ST_ErrorCode_t (*Init)(const DECODER_Handle_t DecoderHandle, const DECODER_InitParams_t * const InitParams_p);
    ST_ErrorCode_t (*Reset)(const DECODER_Handle_t DecoderHandle);
    ST_ErrorCode_t (*DecodePicture)(const DECODER_Handle_t DecoderHandle, const DECODER_DecodePictureParams_t * const DecodePictureParams_p, DECODER_Status_t * const DecoderStatus_p);
    ST_ErrorCode_t (*ConfirmBuffer)(const DECODER_Handle_t DecoderHandle, const DECODER_ConfirmBufferParams_t * const ConfirmBufferParams_p);
    ST_ErrorCode_t (*Abort)(const DECODER_Handle_t DecoderHandle, const CommandId_t CommandId);
    ST_ErrorCode_t (*Setup)(const DECODER_Handle_t DecoderHandle, const STVID_SetupParams_t * const SetupParams_p); /* Only STVID_SetupObject_t STVID_SETUP_DECODER_INTERMEDIATE_BUFFER_PARTITION supported */
    ST_ErrorCode_t (*FillAdditionnalDataBuffer)(const DECODER_Handle_t DecoderHandle, const VIDBUFF_PictureBuffer_t* const Picture_p);
    ST_ErrorCode_t (*Term)(const DECODER_Handle_t DecoderHandle);
#ifdef ST_speed
    void (*SetDecodingMode)(const DECODER_Handle_t DecoderHandle, const U8 Mode);
#ifdef STVID_TRICKMODE_BACKWARD
    void (*SetDecodingDirection)(const DECODER_Handle_t DecoderHandle, const DecodingDirection_t Direction);
#endif /* STVID_TRICKMODE_BACKWARD */
#endif
    ST_ErrorCode_t (*UpdateBitBufferParams)(const DECODER_Handle_t DecoderHandle, void * const Address_p, const U32 Size);
#if defined(DVD_SECURED_CHIP)
    ST_ErrorCode_t (*SetupForH264PreprocWA_GNB42332)(const DECODER_Handle_t DecoderHandle, const STAVMEM_PartitionHandle_t AVMEMPartition);
#endif /* DVD_SECURED_CHIP */
} DECODER_FunctionsTable_t;

typedef struct {
    const DECODER_FunctionsTable_t * FunctionsTable_p;
    ST_Partition_t *        CPUPartition_p; /* Where the module can allocate memory for its internal usage */
    STEVT_Handle_t          EventsHandle;
    STAVMEM_PartitionHandle_t AvmemPartitionHandle;
    ST_DeviceName_t         VideoName;              /* Name of the video instance */
    STVID_DeviceType_t      DeviceType;
    void *                  RegistersBaseAddress_p;  /* Base address of the CODEC registers */
#ifdef ST_XVP_ENABLE_FGT
    VIDFGT_Handle_t         FGTHandle;
#endif
#if defined(ST_7200)
    char                      MMETransportName[32];
#endif /* #if defined(ST_7200) ... */
    U32                     ValidityCheck;
    void *                  SDRAMMemoryBaseAddress_p;
    U8                      DecoderNumber;  /* As defined in vid_com.c. See definition there. */
    /* If It is handled by the hal : */
    U32                     InterruptEvt;
    U32                     InterruptLevel;
    U32                     InterruptNumber;
#ifdef ST_Mpeg2CodecFromOldDecodeModule /* Temporary for quick mpeg2 codec implementation based on viddec */
    void *                  VIDDEC_Handle;
#endif
    void *                  PrivateData_p;
    STVID_CodecMode_t       CodecMode;
} DECODER_Properties_t;

ST_ErrorCode_t DECODER_Init(const DECODER_InitParams_t * const InitParams_p, DECODER_Handle_t * const DecoderHandle_p);
ST_ErrorCode_t DECODER_Term(const DECODER_Handle_t DecoderHandle);
#ifdef VALID_TOOLS
ST_ErrorCode_t DECODER_RegisterTrace(void);
#endif /* VALID_TOOLS */

#define DECODER_GetState(DecodeHandle,CommandId,DecoderStatus_p)    ((DECODER_Properties_t *)(DecodeHandle))->FunctionsTable_p->GetState((DecodeHandle),(CommandId),(DecoderStatus_p))
#define DECODER_GetCodecInfo(DecodeHandle,CodecInfo_p,ProfileAndLevelIndication,CodecMode) ((DECODER_Properties_t *)(DecodeHandle))->FunctionsTable_p->GetCodecInfo((DecodeHandle),(CodecInfo_p), (ProfileAndLevelIndication),(CodecMode))
#ifdef STVID_DEBUG_GET_STATISTICS
#define DECODER_GetStatistics(DecodeHandle,Statistics_p)    ((DECODER_Properties_t *)(DecodeHandle))->FunctionsTable_p->GetStatistics((DecodeHandle),(Statistics_p))
#define DECODER_ResetStatistics(DecodeHandle,Statistics_p)  ((DECODER_Properties_t *)(DecodeHandle))->FunctionsTable_p->ResetStatistics((DecodeHandle),(Statistics_p))
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
#define DECODER_GetStatus(DecodeHandle,Status_p)    ((DECODER_Properties_t *)(DecodeHandle))->FunctionsTable_p->GetStatus((DecodeHandle),(Status_p))
#endif /* STVID_DEBUG_GET_STATUS */
#define DECODER_Reset(DecodeHandle)                                 ((DECODER_Properties_t *)(DecodeHandle))->FunctionsTable_p->Reset(DecodeHandle)
#define DECODER_DecodePicture(DecodeHandle,DecodePictureParams_p,DecoderStatus_p)   ((DECODER_Properties_t *)(DecodeHandle))->FunctionsTable_p->DecodePicture((DecodeHandle),(DecodePictureParams_p),(DecoderStatus_p))
#define DECODER_ConfirmBuffer(DecodeHandle,ConfirmBufferParams_p)   ((DECODER_Properties_t *)(DecodeHandle))->FunctionsTable_p->ConfirmBuffer((DecodeHandle),(ConfirmBufferParams_p))
#define DECODER_Abort(DecodeHandle,CommandId)                       ((DECODER_Properties_t *)(DecodeHandle))->FunctionsTable_p->Abort((DecodeHandle),(CommandId))
#define DECODER_Setup(DecodeHandle,SetupParams_p)                   ((DECODER_Properties_t *)(DecodeHandle))->FunctionsTable_p->Setup((DecodeHandle),(SetupParams_p))
#define DECODER_FillAdditionnalDataBuffer(DecodeHandle, Picture_p)   ((DECODER_Properties_t *)(DecodeHandle))->FunctionsTable_p->FillAdditionnalDataBuffer((DecodeHandle), (Picture_p))
#ifdef ST_speed
#define DECODER_SetDecodingMode(DecodeHandle, Mode)					((DECODER_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetDecodingMode((DecodeHandle),(Mode))
#define DECODER_SetDecodingDirection(DecodeHandle,Direction)       ((DECODER_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetDecodingDirection((DecodeHandle),(Direction))
#endif
#define DECODER_UpdateBitBufferParams(DecodeHandle, Address_p, Size)  ((DECODER_Properties_t *)(DecodeHandle))->FunctionsTable_p->UpdateBitBufferParams((DecodeHandle),(Address_p), (Size))
#if defined(DVD_SECURED_CHIP)
#define DECODER_SetupForH264PreprocWA_GNB42332(DecodeHandle, AVMEMPartition)   ((DECODER_Properties_t *)(DecodeHandle))->FunctionsTable_p->SetupForH264PreprocWA_GNB42332((DecodeHandle),(AVMEMPartition))
#endif /* DVD_SECURED_CHIP */

typedef struct
{
#ifdef STVID_DEBUG_GET_STATISTICS
    ST_ErrorCode_t (*ResetStatistics)(const PARSER_Handle_t ParserHandle, const STVID_Statistics_t * const Statistics_p);
    ST_ErrorCode_t (*GetStatistics)(const PARSER_Handle_t ParserHandle, STVID_Statistics_t * const Statistics_p);
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
    ST_ErrorCode_t (*GetStatus)(const PARSER_Handle_t ParserHandle, STVID_Status_t * const Status_p);
#endif /* STVID_DEBUG_GET_STATUS */
    ST_ErrorCode_t (*Init)(const PARSER_Handle_t ParserHandle, const PARSER_InitParams_t * const InitParams_p);
    ST_ErrorCode_t (*Stop)(const PARSER_Handle_t ParserHandle);
    ST_ErrorCode_t (*Start)(const PARSER_Handle_t ParserHandle, const PARSER_StartParams_t * const StartParams_p);
    ST_ErrorCode_t (*GetPicture)(const PARSER_Handle_t ParserHandle, const PARSER_GetPictureParams_t * const GetPictureParams_p);
    ST_ErrorCode_t (*Term)(const PARSER_Handle_t ParserHandle);
    ST_ErrorCode_t (*InformReadAddress)(const PARSER_Handle_t ParserHandle, const PARSER_InformReadAddressParams_t * const InformReadAddressParams_p);
    ST_ErrorCode_t (*GetBitBufferLevel)(const PARSER_Handle_t ParserHandle, PARSER_GetBitBufferLevelParams_t * const GetBitBufferLevelParams_p);
    ST_ErrorCode_t (*GetDataInputBufferParams)(const PARSER_Handle_t ParserHandle, void ** const BaseAddress_p, U32 * const Size_p);
    ST_ErrorCode_t (*SetDataInputInterface)(const PARSER_Handle_t ParserHandle,
                                            ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle, void ** const Address_p),
                                            void (*InformReadAddress)(void * const Handle, void * const Address),
                                            void * const Handle);
    ST_ErrorCode_t (*Setup)(const DECODER_Handle_t DecoderHandle, const STVID_SetupParams_t * const SetupParams_p);
    void (*FlushInjection)(const PARSER_Handle_t ParserHandle);
#ifdef ST_speed
    U32 (*GetBitBufferOutputCounter)(const PARSER_Handle_t ParserHandle);
#ifdef STVID_TRICKMODE_BACKWARD
    void (*SetBitBuffer)(const PARSER_Handle_t ParserHandle,
                         void * const BufferAddressStart_p,
                         const U32 BufferSize,
                         const BitBufferType_t BufferType,
                         const BOOL Stop);
    void (*WriteStartCode)(const PARSER_Handle_t ParserHandle, const U32 SCVal, const void * const SCAdd_p);
#endif /*STVID_TRICKMODE_BACKWARD*/
#endif /* ST_speed */
    ST_ErrorCode_t (*BitBufferInjectionInit) (const PARSER_Handle_t ParserHandle, const PARSER_ExtraParams_t * const BitBufferInjectionParams_p);
#if defined(DVD_SECURED_CHIP)
    ST_ErrorCode_t (*SecureInit) (const PARSER_Handle_t ParserHandle, const PARSER_ExtraParams_t * const SecureInitParams_p);
#endif /* Secure chip */
} PARSER_FunctionsTable_t;

typedef struct {
    const PARSER_FunctionsTable_t * FunctionsTable_p;
    ST_Partition_t *        CPUPartition_p; /* Where the module can allocate memory for its internal usage */
    STEVT_Handle_t          EventsHandle;
    STAVMEM_PartitionHandle_t AvmemPartitionHandle;
    ST_DeviceName_t         VideoName;              /* Name of the video instance */
    STVID_DeviceType_t      DeviceType;
    void *                  RegistersBaseAddress_p;  /* Base address of the CODEC registers */
    U32                     ValidityCheck;
    void *                  SDRAMMemoryBaseAddress_p;
    void *                  CompressedDataInputBaseAddress_p;
    U8                      DecoderNumber;  /* As defined in vid_com.c. See definition there. */
    /* If It is handled by the hal : */
    U32                     InterruptEvt;
    U32                     InterruptLevel;
    U32                     InterruptNumber;
#ifdef ST_Mpeg2CodecFromOldDecodeModule /* Temporary for quick mpeg2 codec implementation based on viddec */
    void *                  VIDDEC_Handle;
#endif
    void *                  PrivateData_p;
} PARSER_Properties_t;

ST_ErrorCode_t PARSER_Init(const PARSER_InitParams_t * const InitParams_p, PARSER_Handle_t * const ParserHandle_p);
ST_ErrorCode_t PARSER_Term(const PARSER_Handle_t ParserHandle);
#ifdef VALID_TOOLS
ST_ErrorCode_t PARSER_RegisterTrace(void);
#endif /* VALID_TOOLS */


#ifdef STVID_DEBUG_GET_STATISTICS
    #define PARSER_ResetStatistics(ParserHandle,Statistics_p)   ((PARSER_Properties_t *)(ParserHandle))->FunctionsTable_p->ResetStatistics((ParserHandle),(Statistics_p))
    #define PARSER_GetStatistics(ParserHandle,Statistics_p)     ((PARSER_Properties_t *)(ParserHandle))->FunctionsTable_p->GetStatistics((ParserHandle),(Statistics_p))
#endif /* STVID_DEBUG_GET_STATISTICS */
#ifdef STVID_DEBUG_GET_STATUS
#define PARSER_GetStatus(ParserHandle,Status_p)     ((PARSER_Properties_t *)(ParserHandle))->FunctionsTable_p->GetStatus((ParserHandle),(Status_p))
#endif /* STVID_DEBUG_GET_STATUS */

#define PARSER_Stop(ParserHandle)                               ((PARSER_Properties_t *)(ParserHandle))->FunctionsTable_p->Stop(ParserHandle)
#define PARSER_Start(ParserHandle,StartParams_p)                ((PARSER_Properties_t *)(ParserHandle))->FunctionsTable_p->Start((ParserHandle),(StartParams_p))
#define PARSER_GetPicture(ParserHandle, GetPictureParams_p)     ((PARSER_Properties_t *)(ParserHandle))->FunctionsTable_p->GetPicture((ParserHandle), (GetPictureParams_p))
#define PARSER_InformReadAddress(ParserHandle, InformReadAddressParams_p)   ((PARSER_Properties_t *)(ParserHandle))->FunctionsTable_p->InformReadAddress((ParserHandle), (InformReadAddressParams_p))
#define PARSER_GetBitBufferLevel(ParserHandle, GetBitBufferLevelParams_p)   ((PARSER_Properties_t *)(ParserHandle))->FunctionsTable_p->GetBitBufferLevel((ParserHandle), (GetBitBufferLevelParams_p))
#define PARSER_GetDataInputBufferParams(ParserHandle, BaseAddress_p, Size_p)((PARSER_Properties_t *)(ParserHandle))->FunctionsTable_p->GetDataInputBufferParams((ParserHandle), (BaseAddress_p), (Size_p))
#define PARSER_SetDataInputInterface(ParserHandle, GetWriteAddress, InformReadAddress, Handle) ((PARSER_Properties_t *)(ParserHandle))->FunctionsTable_p->SetDataInputInterface((ParserHandle), (GetWriteAddress), (InformReadAddress), (Handle))
#define PARSER_Setup(ParserHandle, SetupParams_p)               ((PARSER_Properties_t *)(ParserHandle))->FunctionsTable_p->Setup(ParserHandle, SetupParams_p)
#define PARSER_FlushInjection(ParserHandle)                     ((PARSER_Properties_t *)(ParserHandle))->FunctionsTable_p->FlushInjection((ParserHandle))

#define PARSER_BitBufferInjectionInit(ParserHandle, BufferParams_p) ((PARSER_Properties_t *)(ParserHandle))->FunctionsTable_p->BitBufferInjectionInit(ParserHandle, BufferParams_p)

#if defined(DVD_SECURED_CHIP)
#define PARSER_SecureInit(ParserHandle, BufferParams_p)             ((PARSER_Properties_t *)(ParserHandle))->FunctionsTable_p->SecureInit(ParserHandle, BufferParams_p)
#endif /* Secure chip */

#ifdef ST_speed
    #define PARSER_GetBitBufferOutputCounter(ParserHandle)      ((PARSER_Properties_t *)(ParserHandle))->FunctionsTable_p->GetBitBufferOutputCounter((ParserHandle))
    #ifdef STVID_TRICKMODE_BACKWARD
        #define PARSER_SetBitBuffer(ParserHandle, BufferAddressStart_p, BufferSize, BufferType, Stop) ((PARSER_Properties_t *)(ParserHandle))->FunctionsTable_p->SetBitBuffer((ParserHandle), (BufferAddressStart_p), (BufferSize), (BufferType), (Stop))
        #define PARSER_WriteStartCode(ParserHandle, SCVal, SCAdd_p) ((PARSER_Properties_t *)(ParserHandle))->FunctionsTable_p->WriteStartCode((ParserHandle), (SCVal), (SCAdd_p))
        #endif /*STVID_TRICKMODE_BACKWARD*/
    #endif /*ST_speed*/
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifndef __STVIDCODEC_H */

/* End of vidcodec.h */
