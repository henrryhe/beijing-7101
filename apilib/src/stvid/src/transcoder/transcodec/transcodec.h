/*******************************************************************************
File Name   : transcodec.h

Description : Video driver Transcoder transcodec header file

Copyright (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
18 Sep 2006        Created                                           OD
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __TRANSCODEC_H
#define __TRANSCODEC_H

/* Includes ----------------------------------------------------------------- */
#ifdef ST_OSLINUX
#include "compat.h"
#else
#include "stlite.h"
#endif
#include "stddefs.h"

#include "stevt.h"
#include "stavmem.h"

#include "stgxobj.h"
#include "stlayer.h"

#include "stvid.h"

#include "vid_com.h"

#ifdef ST_inject
#include "inject.h"
#endif /* ST_inject */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Exported Constants ------------------------------------------------------- */
typedef osclock_t GENERIC_Time_t;

#define TRANSCODEC_MODULE_BASE    0xF00
enum
{
    /* This event passes a (XCODER_TranscodingJobResults_t) as parameter */
    STVID_TRANSCODER_XCODER_JOB_COMPLETED_EVT = STVID_DRIVER_BASE + TRANSCODEC_MODULE_BASE
};

typedef enum XCODER_ErrorCode_e
{
    XCODER_NO_ERROR,
    XCODER_ERROR_CORRUPTED
} XCODER_ErrorCode_t;

typedef enum XCODER_JobState_e
{
    XCODER_JOB_UNDEFINED, /* reset value */
    XCODER_JOB_DECODING,  /* Job is currently processed by the XCODER */
    XCODER_JOB_COMPLETED  /* Job has been entirely processed by the XCODER (XCODER_JOB_COMPLETED_EVT has been issued) */
} XCODER_JobState_t;

typedef enum XCODER_SpeedMode_e
{
    XCODER_MODE_STANDARD = 0,
    XCODER_MODE_SPEED = 1,
    XCODER_MODE_COMPRESSION = 2
} XCODER_SpeedMode_t;

/* Exported Types ----------------------------------------------------------- */
typedef struct XCODER_TranscodingJobStatus_s
{
    XCODER_JobState_t  JobState;
    GENERIC_Time_t     JobSubmissionTime;
    GENERIC_Time_t     JobCompletionTime;
    U32                UsedOutputBufferSizeInByte;
} XCODER_TranscodingJobStatus_t;

typedef struct XCODER_TranscodingJobResults_s
{
    XCODER_TranscodingJobStatus_t   TranscodingJobStatus;
    XCODER_ErrorCode_t              ErrorCode;
} XCODER_TranscodingJobResults_t;

/* Generic */
typedef void * XCODEC_Handle_t;

/* Source Decoder */
typedef XCODEC_Handle_t SOURCE_Handle_t;

typedef struct SOURCE_InitParams_s
{
    ST_Partition_t *        CPUPartition_p;
    STVID_DeviceType_t      DeviceType;
    ST_DeviceName_t         EvtHandlerName;
} SOURCE_InitParams_t;

typedef struct SOURCE_StartParams_s
{
    void *Dummy;
} SOURCE_StartParams_t;

typedef struct SOURCE_UpdateAndGetRefListParams_s
{
    void *Dummy;
}  SOURCE_UpdateAndGetRefListParams_t;

/* Target Decoder */
typedef XCODEC_Handle_t TARGET_Handle_t;

typedef struct TARGET_InitParams_s
{
    ST_Partition_t *        CPUPartition_p;
    STVID_DeviceType_t      DeviceType;
    ST_DeviceName_t         EvtHandlerName;
/*    void *                  BuffersHandle;*/
} TARGET_InitParams_t;

typedef struct TARGET_StartParams_s
{
    void *Dummy;
} TARGET_StartParams_t;

/* Xcode transform */
typedef void * XCODE_Handle_t;

typedef struct XCODE_InitParams_s
{
  STVID_XcodeInitParams_t STVID_XcodeInitParams;
  /* Xcoder callback */
  void  (* XcoderJobCompletedCB) (void * TranscoderHandle, XCODER_TranscodingJobResults_t XCODER_TranscodingJobResults);
  void * TranscoderHandle;
} XCODE_InitParams_t;

typedef struct XCODE_TranscodePictureParams_s
{
  VIDBUFF_PictureBuffer_t           MainPictureBuffer;
  BOOL                              MainPictureBufferIsValid;
  VIDBUFF_PictureBuffer_t           DecimatedPictureBuffer;
  BOOL                              DecimatedPictureBufferIsValid;
  STVID_DecimationFactor_t          TargetDecimationFactor;
  U32                               TargetFrameRate;
  U32                               TargetBitRate;
  STVID_RateControlMode_t           RateControlMode;
  XCODER_SpeedMode_t                SpeedMode;
  U8                                * Write_p; /* last write pointer in Xcoder output buffer */
  U32                               XcoderOutputBufferAvailableSize;
} XCODE_TranscodePictureParams_t;

/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */

/* Source decoder */
typedef struct
{
    ST_ErrorCode_t (*Init)(const SOURCE_Handle_t SourceHandle, const SOURCE_InitParams_t * const InitParams_p);
    ST_ErrorCode_t (*Start)(const SOURCE_Handle_t SourceHandle, const SOURCE_StartParams_t * const StartParams_p);
    ST_ErrorCode_t (*UpdateAndGetRefList)(const SOURCE_Handle_t SourceHandle, SOURCE_UpdateAndGetRefListParams_t * const SOURCE_UpdateAndGetRefListParams_p);
    BOOL           (*IsPictureToBeSkipped)(const SOURCE_Handle_t SourceHandle,
                                           const U32 CurrentPictureDOFID,
                                           const STVID_MPEGFrame_t MPEGFrame,
                                           const U32 SourceFrameRate,
                                           const U32 TargetFrameRate);
    ST_ErrorCode_t (*Term)(const SOURCE_Handle_t SourceHandle);
} SOURCE_FunctionsTable_t;

typedef struct {
    const SOURCE_FunctionsTable_t * FunctionsTable_p;
    ST_Partition_t *                CPUPartition_p; /* Where the module can allocate memory for its internal usage */
    STEVT_Handle_t                  EventsHandle;
    STVID_DeviceType_t              DeviceType;
    U32                             ValidityCheck;
    U8                              DecoderNumber;  /* Might be useful ? */
    void *                          PrivateData_p;
} SOURCE_Properties_t;

ST_ErrorCode_t SOURCE_Init(const SOURCE_InitParams_t * const InitParams_p, SOURCE_Handle_t * const SourceTranscodecHandle_p);
ST_ErrorCode_t SOURCE_Term(const SOURCE_Handle_t SourceTranscodecHandle);

#define SOURCE_Start(SourceHandle,StartParams_p)    ((SOURCE_Properties_t *)(SourceHandle))->FunctionsTable_p->Start((SourceHandle),(StartParams_p))
#define SOURCE_UpdateAndGetRefList(SourceHandle,UpdateAndGetRefListParams_p)    ((SOURCE_Properties_t *)(SourceHandle))->FunctionsTable_p->UpdateAndGetRefList((SourceHandle),(UpdateAndGetRefListParams_p))
#define SOURCE_IsPictureToBeSkipped(SourceHandle, CurrentPictureDOFID, MPEGFrame, SourceFrameRate, TargetFrameRate)     ((SOURCE_Properties_t *)(SourceHandle))->FunctionsTable_p->IsPictureToBeSkipped((SourceHandle),(CurrentPictureDOFID), (MPEGFrame), (SourceFrameRate), (TargetFrameRate))

/* Target decoder */
typedef struct
{
    ST_ErrorCode_t (*Init)(const TARGET_Handle_t TargetHandle, const TARGET_InitParams_t * const InitParams_p);
    ST_ErrorCode_t (*Start)(const TARGET_Handle_t TargetHandle, const TARGET_StartParams_t * const StartParams_p);
    ST_ErrorCode_t (*Term)(const TARGET_Handle_t TargetHandle);
    U32            (*GetPictureMaxSizeInByte)(const U32 PictureWidth, const U32 PictureHeight, const U32 TargetFrameRate, const U32 TargetBitRate);
} TARGET_FunctionsTable_t;

typedef struct {
    const TARGET_FunctionsTable_t * FunctionsTable_p;
    ST_Partition_t *                CPUPartition_p; /* Where the module can allocate memory for its internal usage */
    STEVT_Handle_t                  EventsHandle;
    STVID_DeviceType_t              DeviceType;
    U32                             ValidityCheck;
    U8                              DecoderNumber;  /* Might be useful ? */
    void *                          PrivateData_p;
} TARGET_Properties_t;

ST_ErrorCode_t TARGET_Init(const TARGET_InitParams_t * const InitParams_p, TARGET_Handle_t * const TargetTranscodecHandle_p);
ST_ErrorCode_t TARGET_Term(const TARGET_Handle_t TargetTranscodecHandle);

#define TARGET_Start(TargetHandle,StartParams_p)    ((TARGET_Properties_t *)(TargetHandle))->FunctionsTable_p->Start((TargetHandle),(StartParams_p))
#define TARGET_GetPictureMaxSizeInByte(TargetHandle, PictureWidth, PictureHeight, TargetFrameRate, TargetBitRate) ((TARGET_Properties_t *)(TargetHandle))->FunctionsTable_p->GetPictureMaxSizeInByte((PictureWidth), (PictureHeight), (TargetFrameRate), (TargetBitRate))
/* Xcode transform */
typedef struct
{
    ST_ErrorCode_t (*Init)(const XCODE_Handle_t XcodeHandle, const XCODE_InitParams_t * const InitParams_p);
    ST_ErrorCode_t (*TranscodePicture)(const XCODE_Handle_t XcodeHandle, const XCODE_TranscodePictureParams_t * const TranscodePictureParams_p);
    ST_ErrorCode_t (*Term)(const XCODE_Handle_t XcodeHandle);
} XCODE_FunctionsTable_t;

typedef struct {
    const XCODE_FunctionsTable_t * FunctionsTable_p;
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
    void *                  PrivateData_p;
} XCODE_Properties_t;

ST_ErrorCode_t XCODE_Init(const XCODE_InitParams_t * const InitParams_p, XCODE_Handle_t * const XcodeHandle_p);
ST_ErrorCode_t XCODE_Term(const XCODE_Handle_t XcodeHandle);

#define XCODE_TranscodePicture(XcodeHandle,TranscodePictureParams_p)    ((XCODE_Properties_t *)(XcodeHandle))->FunctionsTable_p->TranscodePicture((XcodeHandle),(TranscodePictureParams_p))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifndef __TRANSCODEC_H */

/* End of transcodec.h */
