/*******************************************************************************

File name   : vid_buff.h

Description : frame buffer management header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
21 Mar 2000        Created                                           HG
14 Feb 2001        Comment completion                                GG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_BUFFER_MANAGER_COMMON_H
#define __VIDEO_BUFFER_MANAGER_COMMON_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "buffers.h"

#include "halv_buf.h"

#ifdef USE_EVT
#include "stevt.h"
#endif

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

enum
{
    /* Always set first element as 0 as it will be used for a table */
    VIDBUFF_PICTURE_OUT_OF_DISPLAY_EVT_ID = 0,      /* Evt called when a Picture is removed from the Display Queue */
    VIDBUFF_IMPOSSIBLE_WITH_MEM_PROFILE_EVT_ID,
#ifdef ST_OSLINUX
    VIDBUFF_NEW_ALLOCATED_FRAMEBUFFERS_EVT_ID,
    VIDBUFF_DEALLOCATED_FRAMEBUFFERS_EVT_ID,
#endif
    VIDBUFF_NEW_PICTURE_BUFFER_AVAILABLE_EVT_ID,    /* Evt called when a Picture Buffer becomes available */
    VIDBUFF_NB_REGISTERED_EVENTS_IDS /* always keep as last one to count event ID's */
};

/* Exported Types ----------------------------------------------------------- */

typedef struct VIDBUFF_Data_s {
    U32                 ValidityCheck;
    ST_Partition_t *    CPUPartition_p;
    STEVT_Handle_t      EventsHandle;
    STEVT_EventID_t     RegisteredEventsID[VIDBUFF_NB_REGISTERED_EVENTS_IDS];
    HALBUFF_Handle_t    HALBuffersHandle;
    STAVMEM_PartitionHandle_t AvmemPartitionHandle;
    STVID_DeviceType_t  DeviceType;

    U8                      MaxFrameBuffers;                /* Maximum number of frame buffer that the USER can allocate minus 1.
                                                             * This value is set at init(). It's assumed that other Frame Buffers
                                                             * are allocated to manage possible memory profile incompatibility */

    U8                      MaxSecondaryFrameBuffers;      /* Maximum number of frame buffer that the USER can allocate minus 1.
                                                             * This value is set at init(). It's assumed that other Frame Buffers
                                                             * are allocated to manage possible memory profile incompatibility */

    VIDBUFF_FrameBuffer_t   *FrameBuffers_p;                /* Where frame buffers structures are allocated at init (size MaxFrameBuffersInProfile) */
    VIDBUFF_FrameBuffer_t   *SecondaryFrameBuffers_p;       /* Where frame buffers structures are allocated at init (size MaxFrameBuffersInProfile) */
    VIDBUFF_PictureBuffer_t *AllocatedPictureBuffersTable_p;/* Where picture structures are allocated at init (size 2 * MaxFrameBuffersInProfile) */

    U32                     NumberOfAllocatedFrameBuffers;  /* Number of allocated frame buffers */
    VIDBUFF_FrameBuffer_t * AllocatedFrameBuffersLoop_p;    /* NULL if no frames allocated, loop linked list of allocated frames otherwise */
    U32                     NumberOfSecondaryAllocatedFrameBuffers;  /* Number of allocated frame buffers */
    VIDBUFF_FrameBuffer_t * AllocatedSecondaryFrameBuffersLoop_p;    /* NULL if no frames allocated, loop linked list of allocated frames otherwise */
    semaphore_t *           AllocatedFrameBuffersLoopLockingSemaphore_p; /* To lock access to list of references */
#ifdef USE_EXTRA_FRAME_BUFFERS
    BOOL                    IsMemoryAllocatedForProfile;    /* Set to TRUE if frame buffers have been allocated for profile (used to allocated extra buffers)*/
    VIDBUFF_FrameBuffer_t * AllocatedExtraPrimaryFrameBuffersLoop_p;    /* NULL if no frames allocated, loop linked list of allocated frames otherwise (SD only) */
    U32                     NumberOfExtraPrimaryFrameBuffersRequested; /* Number of extra SD frame buffers requested by the upper layers */
    U32                     NumberOfAllocatedExtraPrimaryFrameBuffers; /* Number of extra SD frame buffers that can be used (usually as picture reference) */
    VIDBUFF_FrameBuffer_t * AllocatedExtraSecondaryFrameBuffersLoop_p;    /* NULL if no frames allocated, loop linked list of allocated frames otherwise (SD only) */
    U32                     NumberOfExtraSecondaryFrameBuffersRequested; /* Number of extra SD frame buffers requested by the upper layers */
    U32                     NumberOfAllocatedExtraSecondaryFrameBuffers; /* Number of extra SD frame buffers that can be used (usually as picture reference) */
#endif

    VIDCOM_InternalProfile_t Profile;           /* Current memory profile */
    VIDBUFF_StrategyForTheBuffersFreeing_t StrategyForTheBuffersFreeing;    /* In 3 buffers we free the buffers as soon as 1 of the 2 fields is on display. */
    VIDBUFF_BufferType_t    FrameBuffersType;    /* Type  of all frame buffers in this instance */
    U32                     FrameBufferAdditionalDataBytesPerMB;  /* Size of additionnal data accociated to FB in bytes/MB (H264 DeltaPhy IP MB structure storage) */

    STVID_HDPIPParams_t     HDPIPParams;        /* HDPIP parametres for this instance.          */

    BOOL                    IsNotificationOfImpossibleWithProfileAllowed; /* Used to notify event IMPOSSIBLE_WITH_PROFILE not all the time but just once each time it is new that a picture doesn't fit in the profile
                                                                             Set TRUE to allow to notify the event, set FASLE after it has been notified once. */

    STVID_MPEGFrame_t       LastMPEGFrameType; /* Variable indicating the type of the last Picture Buffer requested ('I', 'P' or 'B'). This information is necessary because Overwrite
                                                  should be done only if the last picture decoded was a B picture. */

#ifdef ST_smoothbackward
    VIDBUFF_ControlMode_t   ControlMode;
#endif

    struct
    {
        U32                 Width;
        U32                 Height;
        U32                 NumberOfFrames;
    } FrameBufferDynAlloc;

#ifdef ST_XVP_ENABLE_FGT
    VIDFGT_Handle_t         FGTHandle;
#endif /* ST_XVP_ENABLE_FGT */
} VIDBUFF_Data_t;


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_BUFFER_MANAGER_COMMON_H */

/* End of vid_buff.h */
