/*******************************************************************************

File name   : buffers.h

Description : Buffer manager header file

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
21 Mar 2000        Created                                           HG
03 Mar 2001        Secondary frame buffer management (decimation)
                   and compression.                                  GG
26 Jan 2004        Moved picture and frame buffer objects to generic
                   standard independant objects (vid_com.h)          HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_BUFFER_MANAGER_H
#define __VIDEO_BUFFER_MANAGER_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "stvid.h"

#include "vid_com.h"

#include "stavmem.h"

#ifdef ST_XVP_ENABLE_FGT
#include "vid_fgt.h"
#endif /* ST_XVP_ENABLE_FGT */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

#if defined(ST_7109) || defined(ST_7200)
    #if !defined(STVID_MINIMIZE_MEMORY_USAGE_FOR_DEI)
    #define USE_EXTRA_FRAME_BUFFERS
    #endif /* STVID_MINIMIZE_MEMORY_USAGE_FOR_DEI */
#endif /* defined(ST_7109) || defined(ST_7200) */

/* Exported Constants ------------------------------------------------------- */
#ifdef USE_EXTRA_FRAME_BUFFERS
    #define     MAX_EXTRA_FRAME_BUFFER_NBR          10
#else
    #define     MAX_EXTRA_FRAME_BUFFER_NBR          0
#endif /* USE_EXTRA_FRAME_BUFFERS */

#define VIDBUFF_MODULE_BASE   0x400

enum
{
    /* This event passes a (VIDBUFF_PictureBuffer_t *) as parameter */
    VIDBUFF_PICTURE_OUT_OF_DISPLAY_EVT = STVID_DRIVER_BASE + VIDBUFF_MODULE_BASE,
    /* This event passes a (STVID_MemoryProfile_t *) as parameter */
    VIDBUFF_IMPOSSIBLE_WITH_MEM_PROFILE_EVT,
#ifdef ST_OSLINUX
    /* This event passes a (VIDBUFF_FrameBufferList_t *) as parameter */
    VIDBUFF_NEW_ALLOCATED_FRAMEBUFFERS_EVT,
    /* This event passes a (VIDBUFF_FrameBufferList_t *) as parameter */
    VIDBUFF_DEALLOCATED_FRAMEBUFFERS_EVT,
#endif
    /* This event passes a (VIDBUFF_FrameBufferList_t *) as parameter */
    VIDBUFF_NEW_PICTURE_BUFFER_AVAILABLE_EVT
};

typedef enum VIDBUFF_InsertionOrder_e
{
    VIDBUFF_INSERTION_ORDER_INCREASING, /* Insert picture at right place in ExtendedPresentationOrderPictureID increasing order */
    VIDBUFF_INSERTION_ORDER_DECREASING, /* Insert picture at right place in ExtendedPresentationOrderPictureID decreasing order */
    VIDBUFF_INSERTION_ORDER_LAST_PLACE  /* Insert picture at last place in queue */
} VIDBUFF_InsertionOrder_t;


#ifdef ST_smoothbackward
typedef enum VIDBUFF_ControlMode_e
{
    VIDBUFF_MANUAL_CONTROL_MODE,
    VIDBUFF_AUTOMATIC_CONTROL_MODE
} VIDBUFF_ControlMode_t;
#endif

/* Exported Types ----------------------------------------------------------- */

typedef void * VIDBUFF_Handle_t;

#ifdef ST_OSLINUX
typedef struct FrameBuffers_s {
    void          * KernelAddress_p;
    void          * MappedAddress_p;
    U32             Size;
} FrameBuffers_t;

typedef struct VIDBUFF_FrameBufferList_s
{
    FrameBuffers_t * FrameBuffers_p;
    U32              NbFrameBuffers;
} VIDBUFF_FrameBufferList_t;
#endif  /* LINUX */


/* Parameters required to allocate a buffer */
typedef struct VIDBUFF_AllocateBufferParams_s
{
    VIDBUFF_BufferType_t BufferType;    /* Buffer type */
    U32     PictureWidth;   /* Witdh (in pixels) of the picture (to allocate frame buffers). */
    U32     PictureHeight;  /* Height (in pixels) of the picture(to allocate frame buffers). */
    U32     BufferSize;     /* Buffer Size, used to allocate bitbuffer. */
    VIDBUFF_AllocationMode_t AllocationMode;
} VIDBUFF_AllocateBufferParams_t;

typedef struct VIDBUFF_Infos_s
{
    U32     FrameBufferAdditionalDataBytesPerMB;  /* Size of additionnal data accociated to FB in bytes/MB (H264 DeltaPhy IP MB structure storage) */
} VIDBUFF_Infos_t;

typedef struct VIDBUFF_GetUnusedPictureBufferParams_s
{
    STVID_MPEGFrame_t           MPEGFrame;     /* Video MPEG frame type: I, P or B*/
    STVID_PictureStructure_t    PictureStructure; /* Picture structure (Frame, Top field, Bottom field)*/
    BOOL                        TopFieldFirst; /* If PictureStructure is frame, indicates the first (decoded) field in the frame */
    BOOL                        ExpectingSecondField; /* Set TRUE if the picture is a field picture, the first of the 2 fields (FALSE if second field) */
    U32                         DecodingOrderFrameID; /* FrameID with same value for both fields of field picture */
    U32                         PictureWidth;
    U32                         PictureHeight;
} VIDBUFF_GetUnusedPictureBufferParams_t;

#ifdef ST_smoothbackward
typedef struct VIDBUFF_GetSmoothBackwardUnusedPictureBufferParams_s
{
    STVID_PictureStructure_t    PictureStructure; /* Picture structure (Frame, Top field, Bottom field)*/
    BOOL                        TopFieldFirst; /* If PictureStructure is frame, indicates the first (decoded) field in the frame */
    BOOL                        ExpectingSecondField; /* Set TRUE if the picture is a field picture, the first of the 2 fields (FALSE if second field) */
    U32                         PictureWidth;
    U32                         PictureHeight;
} VIDBUFF_GetSmoothBackwardUnusedPictureBufferParams_t;
#endif

typedef struct VIDBUFF_InitParams_s
{
    ST_Partition_t *    CPUPartition_p;
    ST_DeviceName_t     EventsHandlerName;
    void *              SharedMemoryBaseAddress_p;
    STAVMEM_PartitionHandle_t AvmemPartitionHandle;
    U8                  MaxFrameBuffersInProfile;
    STVID_DeviceType_t  DeviceType;
    ST_DeviceName_t     VideoName;              /* Name of the video instance */
    VIDBUFF_BufferType_t FrameBuffersType;      /* Type required for all the frame buffers in the instance:
                        * VIDBUFF_BUFFER_FRAME_12BITS_PER_PIXEL for decode which fills frame buffers in 4:2:0
                        * VIDBUFF_BUFFER_FRAME_16BITS_PER_PIXEL for digital input which fills frame buffers in 4:2:2
                                                 * or HD-PIP.*/
#ifdef ST_XVP_ENABLE_FGT
    VIDFGT_Handle_t         FGTHandle;
#endif /* ST_XVP_ENABLE_FGT */
} VIDBUFF_InitParams_t;

typedef struct VIDBUFF_StrategyForTheBuffersFreeing_s
{
    BOOL IsTheBuffersFreeingOptimized;
} VIDBUFF_StrategyForTheBuffersFreeing_t;

/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */

/* Enable this #define if you want to Debug Picture Buffer Take/Release */
/* #define STVID_DEBUG_TAKE_RELEASE */

#ifdef STVID_DEBUG_TAKE_RELEASE
    #define DEBUG_CHECK_PICTURE_BUFFER_VALIDITY(BuffersHandle, Picture_p)   VIDBUFF_CheckPictureBuffer(BuffersHandle, Picture_p)
    #define TAKE_PICTURE_BUFFER(BuffersHandle, Picture_p, ModId)            VIDBUFF_DbgTakePictureBuffer(BuffersHandle, Picture_p, ModId, __FILE__, __LINE__)
    #define RELEASE_PICTURE_BUFFER(BuffersHandle, Picture_p, ModId)         VIDBUFF_DbgReleasePictureBuffer(BuffersHandle, Picture_p, ModId, __FILE__, __LINE__, TRUE)
    #define UNTAKE_PICTURE_BUFFER(BuffersHandle, Picture_p, ModId)          VIDBUFF_DbgReleasePictureBuffer(BuffersHandle, Picture_p, ModId, __FILE__, __LINE__, FALSE)
#else
    #define DEBUG_CHECK_PICTURE_BUFFER_VALIDITY(BuffersHandle, Picture_p)
    #define TAKE_PICTURE_BUFFER(BuffersHandle, Picture_p, ModId)            VIDBUFF_TakePictureBuffer(BuffersHandle, Picture_p, ModId)
    #define RELEASE_PICTURE_BUFFER(BuffersHandle, Picture_p, ModId)         VIDBUFF_ReleasePictureBuffer(BuffersHandle, Picture_p, ModId, TRUE)
    #define UNTAKE_PICTURE_BUFFER(BuffersHandle, Picture_p, ModId)          VIDBUFF_ReleasePictureBuffer(BuffersHandle, Picture_p, ModId, FALSE)
#endif

#define IS_PICTURE_BUFFER_IN_USE(PictureBuffer_p)       (PictureBuffer_p->Buffers.PictureLockCounter >  0)

/* Macro indicating if a Picture buffer is used by a given Module */
#define IS_PICTURE_BUFFER_USED_BY(PictureBuffer_p, ModId)       (PictureBuffer_p->Buffers.Module[ModId].TakeReleaseCounter  > 0)

/* Macro indicating if a Picture buffer is used ONLY by a given Module */
#define IS_PICTURE_BUFFER_OWNED_ONLY_BY(PictureBuffer_p, ModId)       (PictureBuffer_p->Buffers.PictureLockCounter ==  PictureBuffer_p->Buffers.Module[ModId].TakeReleaseCounter)

/* Exported Functions ------------------------------------------------------- */

ST_ErrorCode_t VIDBUFF_Init(const VIDBUFF_InitParams_t * const InitParams_p, VIDBUFF_Handle_t * const Handle_p);
ST_ErrorCode_t VIDBUFF_Term(const VIDBUFF_Handle_t Handle);

ST_ErrorCode_t VIDBUFF_AllocateBitBuffer(const VIDBUFF_Handle_t BuffersHandle, const VIDBUFF_AllocateBufferParams_t * const AllocParams_p, VIDBUFF_BufferAllocationParams_t * const BufferParams_p);
ST_ErrorCode_t VIDBUFF_AllocateMemoryForProfile(const VIDBUFF_Handle_t BuffersHandle, const BOOL ManualNumberOfBuffers, const U8 NumberOfBuffers);
ST_ErrorCode_t VIDBUFF_ClearFrameBuffer(const VIDBUFF_Handle_t BuffersHandle,
                                        VIDBUFF_BufferAllocationParams_t * const BufferParams_p,
                                        const STVID_ClearParams_t * const ClearFrameBufferParams_p);
ST_ErrorCode_t VIDBUFF_DeAllocateBitBuffer(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_BufferAllocationParams_t * const BufferParams_p);
ST_ErrorCode_t VIDBUFF_DeAllocateUnusedMemoryOfProfile(const VIDBUFF_Handle_t Handle);
ST_ErrorCode_t VIDBUFF_RecoverFrameBuffers(const VIDBUFF_Handle_t BuffersHandle);
ST_ErrorCode_t VIDBUFF_SetInfos(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_Infos_t * const Infos_p);
ST_ErrorCode_t VIDBUFF_ForceDecimationFactor(const VIDBUFF_Handle_t BuffersHandle, const STVID_DecimationFactor_t DecimationFactor);
ST_ErrorCode_t VIDBUFF_GetMemoryProfile(const VIDBUFF_Handle_t Handle, VIDCOM_InternalProfile_t * const Profile_p);

ST_ErrorCode_t VIDBUFF_GetMemoryProfileDecimationFactor(const VIDBUFF_Handle_t Handle, STVID_DecimationFactor_t * const DecimationFactor_p);
ST_ErrorCode_t VIDBUFF_GetApplicableDecimationFactor(const VIDBUFF_Handle_t Handle, STVID_DecimationFactor_t * const DecimationFactor_p);

ST_ErrorCode_t VIDBUFF_GetPictureAllocParams(const VIDBUFF_Handle_t BuffersHandle, const U32 PictureWidth, const U32 PictureHeight, U32 * const TotalSize_p, U32 * const Aligment_p);
ST_ErrorCode_t VIDBUFF_GetAndTakeUnusedPictureBuffer(const VIDBUFF_Handle_t Handle, const VIDBUFF_GetUnusedPictureBufferParams_t * const Params_p,
                                              VIDBUFF_PictureBuffer_t ** const PictureBuffer_p,
                                              VIDCOM_MODULE_ID_t ModId);
ST_ErrorCode_t VIDBUFF_GetAndTakeUnusedDecimatedPictureBuffer(const VIDBUFF_Handle_t Handle, const VIDBUFF_GetUnusedPictureBufferParams_t * const Params_p,
                                              VIDBUFF_PictureBuffer_t ** const PictureBuffer_p,
                                              VIDCOM_MODULE_ID_t ModId);

ST_ErrorCode_t VIDBUFF_PictureRemovedFromDisplayQueue(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const Picture_p);
ST_ErrorCode_t VIDBUFF_ReAllocateBitBuffer(const VIDBUFF_Handle_t BuffersHandle, const VIDBUFF_AllocateBufferParams_t * const AllocParams_p,
                                           VIDBUFF_BufferAllocationParams_t * const BufferParams_p, const STAVMEM_PartitionHandle_t AVMEMPartitionHandle);
ST_ErrorCode_t VIDBUFF_SetStrategyForTheBuffersFreeing(const VIDBUFF_Handle_t BuffersHandle, const VIDBUFF_StrategyForTheBuffersFreeing_t * const StrategyForTheBuffersFreeing_p);
ST_ErrorCode_t VIDBUFF_SetHDPIPParams(const VIDBUFF_Handle_t BuffersHandle, const STVID_HDPIPParams_t * const HDPIPParams_p);
ST_ErrorCode_t VIDBUFF_SetMemoryProfile(const VIDBUFF_Handle_t Handle,VIDCOM_InternalProfile_t* const Profile_p);
ST_ErrorCode_t VIDBUFF_SetDisplayStatus(const VIDBUFF_Handle_t BuffersHandle, const VIDBUFF_DisplayStatus_t DisplayStatus, VIDBUFF_PictureBuffer_t * const Picture_p);
ST_ErrorCode_t VIDBUFF_SetAvmemPartitionForFrameBuffers(const VIDBUFF_Handle_t BuffersHandle, const STAVMEM_PartitionHandle_t AvmemPartitionHandle);
ST_ErrorCode_t VIDBUFF_SetAvmemPartitionForDecimatedFrameBuffers(const VIDBUFF_Handle_t BuffersHandle, const STAVMEM_PartitionHandle_t AvmemPartitionHandle);
ST_ErrorCode_t VIDBUFF_SetAvmemPartitionForAdditionnalData(const VIDBUFF_Handle_t BuffersHandle, const STAVMEM_PartitionHandle_t AvmemPartitionHandle);

/* Function to help picture buffer allocation debugging (you should also activate the Debug Print in "vid_buff.c") */
void           VIDBUFF_PrintPictureBuffersStatus(const VIDBUFF_Handle_t BuffersHandle);

#ifdef DEBUG_PICTURE_BUFFER
ST_ErrorCode_t VIDBUFF_DbgTakePictureBuffer(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const Picture_p, VIDCOM_MODULE_ID_t ModId, const char *FileName, U32 Line);
ST_ErrorCode_t VIDBUFF_DbgReleasePictureBuffer(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const Picture_p, VIDCOM_MODULE_ID_t ModId, const char *FileName, U32 Line, const BOOL IsNotificationNeeded);
#endif

#if defined(DVD_SECURED_CHIP)
ST_ErrorCode_t VIDBUFF_AllocateESCopyBuffer(const VIDBUFF_Handle_t BuffersHandle, const VIDBUFF_AllocateBufferParams_t * const AllocParams_p, VIDBUFF_BufferAllocationParams_t * const BufferParams_p);
ST_ErrorCode_t VIDBUFF_DeAllocateESCopyBuffer(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_BufferAllocationParams_t * const BufferParams_p);
ST_ErrorCode_t VIDBUFF_SetAvmemPartitionForESCopyBuffer(const VIDBUFF_Handle_t BuffersHandle, const STAVMEM_PartitionHandle_t AvmemPartitionHandle);
#endif /* DVD_SECURED_CHIP */

ST_ErrorCode_t VIDBUFF_CheckPictureBuffer(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const Picture_p);
ST_ErrorCode_t VIDBUFF_TakePictureBuffer(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const Picture_p, VIDCOM_MODULE_ID_t ModId);
ST_ErrorCode_t VIDBUFF_ReleasePictureBuffer(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const Picture_p, VIDCOM_MODULE_ID_t ModId, const BOOL IsNotificationNeeded);

ST_ErrorCode_t VIDBUFF_GetAdditionnalDataBuffer(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const PictureBuffer_p);
ST_ErrorCode_t VIDBUFF_FreeAdditionnalDataBuffer(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const PictureBuffer_p);

#ifdef USE_EXTRA_FRAME_BUFFERS
ST_ErrorCode_t VIDBUFF_SetNbrOfExtraPrimaryFrameBuffers(const VIDBUFF_Handle_t BuffersHandle, const U32 NumberOfBuffers);
ST_ErrorCode_t VIDBUFF_DeAllocateUnusedExtraBuffers(const VIDBUFF_Handle_t BuffersHandle);
ST_ErrorCode_t VIDBUFF_SetNbrOfExtraSecondaryFrameBuffers(const VIDBUFF_Handle_t BuffersHandle, const U32 NumberOfBuffers);
ST_ErrorCode_t VIDBUFF_DeAllocateUnusedExtraSecondaryBuffers(const VIDBUFF_Handle_t BuffersHandle);
#endif /* USE_EXTRA_FRAME_BUFFERS */

#ifdef ST_smoothbackward
ST_ErrorCode_t VIDBUFF_GetAndTakeSmoothBackwardUnusedPictureBuffer(const VIDBUFF_Handle_t BuffersHandle,
                                const VIDBUFF_GetSmoothBackwardUnusedPictureBufferParams_t * const Params_p,
                                VIDBUFF_PictureBuffer_t ** const PictureBuffer_p,
                                VIDCOM_MODULE_ID_t ModId);
ST_ErrorCode_t VIDBUFF_GetAndTakeSmoothBackwardDecimatedUnusedPictureBuffer(const VIDBUFF_Handle_t BuffersHandle,
                                const VIDBUFF_GetSmoothBackwardUnusedPictureBufferParams_t * const Params_p,
                                VIDBUFF_PictureBuffer_t ** const PictureBuffer_p,
                                VIDCOM_MODULE_ID_t ModId);
#endif

BOOL VIDBUFF_NeedToChangeFrameBufferParams(const VIDBUFF_Handle_t BuffersHandle, const U32 Width, const U32 Height, const U8 NumberOfFrames);
ST_ErrorCode_t VIDBUFF_ChangeFrameBufferParams(const VIDBUFF_Handle_t BuffersHandle, const U32 Width, const U32 Height, const U8 NumberOfFrames);
void VIDBUFF_GetAllocatedFrameNumbers(const VIDBUFF_Handle_t BuffersHandle, U32 * const MainBuffers, U32 * const DecimBuffers);
ST_ErrorCode_t VIDBUFF_AllocateNewFrameBuffer(const VIDBUFF_Handle_t BuffersHandle, BOOL * const NeedForExtraAllocation_p, BOOL * const MaxSizedReached_p);
ST_ErrorCode_t VIDBUFF_AllocateNewDecimatedFrameBuffer(const VIDBUFF_Handle_t BuffersHandle, BOOL * const NeedForExtraAllocationDecimated_p, BOOL * const MaxSizedReached_p);
/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_BUFFER_MANAGER_H */

/* End of buffers.h */

