/*******************************************************************************

File name   : transcode.h

Description : Video Trancoder header file

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
17 Aug 2006        Created from producer module                       OD
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_TRANSCODE_H
#define __VIDEO_TRANSCODE_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "stvid.h"
#include "vid_xcode.h"

#include "buffers.h"

#ifdef ST_inject
#include "inject.h"
#endif /* ST_inject */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
/* TODO: to check value with STAPI team */
#define VIDTRAN_MODULE_BASE                 0xE00

#define STVID_INVALID_TRANSCODER_NUMBER     ((U32) (-1))

/* ------------------------------------------------------ */
/* Number of framebuffers reserved as an additional poll. */
/* WARNING: Until dynamic allocation is not fully         */
/* implemented   Number of framebuffers reserved as an additional poll. */
/* In realtime:                                           */
/* Between TrQE  and TrQHF:  3 Min                        */
/* Between TrQHF and TrQAF:  2 Min                        */
/* Between TrQAF and TrQF:   2 Min                        */
/* In offline:                                            */
/* 0 Min                                                  */
/* ------------------------------------------------------ */
#define NB_FRAME_BUFFER_REALTIME_TRANSCODE     7
#define NB_FRAME_BUFFER_OFFLINE_TRANSCODE      0
#define NB_FRAME_BUFFER_EXTRA_TRANSCODE        1

#define MAX_TRANSCODE_FRAME_BUFFERS            (NB_FRAME_BUFFER_REALTIME_TRANSCODE + NB_FRAME_BUFFER_EXTRA_TRANSCODE)

#define NB_TRQUEUE_ELEM                        ((2 * (VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES + 2 + MAX_EXTRA_FRAME_BUFFER_NBR + MAX_TRANSCODE_FRAME_BUFFERS)) * 2)

enum
{
    /* Reserved: should not be subscribed ! Just here for reservation of the event constant ! */
    VIDTRAN_SELF_INSTALLED_INTERRUPT_EVT = STVID_DRIVER_BASE + VIDTRAN_MODULE_BASE
};

/* Exported Types ----------------------------------------------------------- */

typedef void * VIDTRAN_Handle_t;

/* Global Init structure for Encode/Transcode */
typedef struct VIDTRAN_InitParams_s
{
    /* General parameters */
    ST_DeviceName_t         TranscoderName;      /* Name of the encoder instance */
    ST_Partition_t *        CPUPartition_p;
    STAVMEM_PartitionHandle_t AVMEMPartition;
    ST_DeviceName_t         EvtHandlerName;

    /* Source & Target parameters */
    ST_DeviceName_t         TargetDeviceName;
    STVID_EncoderOuputProfile_t OutputStreamProfile;

    /* Xcode */
    STVID_XcodeInitParams_t Xcode_InitParams;

    /* Output buffer */
    BOOL                    OutputBufferAllocated;
    void *                  OutputBufferAddress_p;
    U32                     OutputBufferSize;

} VIDTRAN_InitParams_t;

typedef STVID_TranscoderTermParams_t        VIDTRAN_TermParams_t;
typedef STVID_TranscoderOpenParams_t        VIDTRAN_OpenParams_t;
typedef STVID_TranscoderCloseParams_t       VIDTRAN_CloseParams_t;
typedef STVID_TranscoderStartParams_t       VIDTRAN_StartParams_t;
typedef STVID_TranscoderStopParams_t        VIDTRAN_StopParams_t;
typedef STVID_TranscoderSetParams_t         VIDTRAN_SetParams_t;
typedef STVID_TranscoderSetMode_t           VIDTRAN_SetMode_t;
typedef STVID_TranscoderErrorRecoveryMode_t VIDTRAN_ErrorRecoveryMode_t;

typedef struct VIDTRQUEUE_QueueInfo_s
{
    U32 NbOfPicturesFree;
    U32 NbOfPicturesValidInQueue;
    U32 NbOfPicturesStillInDisplayQueue;
    U32 NbOfPicturesReleaseRequested;
    U32 NbOfPicturesToBeTranscoded;
} VIDTRQUEUE_QueueInfo_t;

typedef enum VIDTRQUEUE_QueuePictureState_e
{
    VIDTRQUEUE_STATE_FREE,                    /* The entry is fully free */
    VIDTRQUEUE_STATE_RESERVED,                /* The entry is reserved for setting a picture */
    VIDTRQUEUE_STATE_VALID,                   /* The entry (thus the picture) is valid in queue */
    VIDTRQUEUE_STATE_RELEASE_REQUESTED        /* The entry is no more valid in queue and will be released ASAP (picture buffers are valid) */
} VIDTRQUEUE_QueuePictureState_t;

typedef struct TranscoderDeviceData_s
{
    /* Variables written only in init/termination/capability phase, and used as read-only after */
    /* All data in this section are protected with InterInstancesLockingSemaphore, used through
	 * macros stvid_EnterCriticalSection(NULL) and stvid_LeaveCriticalSection(NULL) */

	U32                      MaxOpen;
    ST_Partition_t*          CPUPartition_p;
/*    STVID_TranscoderType_t   TranscoderType;*/            /* LM: Not used at this time */
    STVID_TranscoderHandle_t TranscoderHandle;
    BOOL                     TranscoderHandleIsValid;
    U32                      NumberOfOpenSinceInit;     /* Just counts the number of successfull open's done */


} TranscoderDeviceData_t;

/* Structure containing static data for initialised instances of transcode */
typedef struct TranscoderDevice_s
{
    ST_DeviceName_t          DeviceName;
    TranscoderDeviceData_t * DeviceData_p;              /* NULL when not initialised,
                                                            allocated structure otherwise */
#if 0
    U32                      TranscoderNumber;          /* Contains STVID_INVALID_TRANSCODER_NUMBER if the device is not initialised.
                                                            Contains DecoderNumber as defined by vidcom when initialised. See definition there. */
#endif
    semaphore_t *            DeviceControlLockingSemaphore_p; /* Mutex object to access the device's fundamental data: Status, memory profile, */


} TranscoderDevice_t;

/* Structure containing static data for open handles */
typedef struct TranscoderUnit_s
{
    TranscoderDevice_t *     Device_p;
    U32                      UnitValidity;
} TranscoderUnit_t;

/* Exported Variables ------------------------------------------------------- */

/* Just exported so that macro IsValidHandle works ! Should never be used outside ! */
extern TranscoderUnit_t stvid_TranscoderUnitArray[];

/* Exported Macros ---------------------------------------------------------- */

/* Passing a (STVID_Handle_t), returns TRUE if the handle is valid, FALSE otherwise */
#define IsValidTranscoderHandle(Handle) ((((TranscoderUnit_t *) (Handle)) >= ((TranscoderUnit_t *) stvid_TranscoderUnitArray)) &&                    \
                               (((TranscoderUnit_t *) (Handle)) < (TranscoderUnit_t *) ((TranscoderUnit_t *) stvid_TranscoderUnitArray + STVID_MAX_UNIT)) &&  \
                               (((TranscoderUnit_t *) (Handle))->UnitValidity == STVID_VALID_UNIT))

void stvid_EnterTranscoderCriticalSection(TranscoderDevice_t * const TranscoderDevice_p);
void stvid_LeaveTranscoderCriticalSection(TranscoderDevice_t * const TranscoderDevice_p);

/* Exported Functions ------------------------------------------------------- */

typedef struct
{
#ifdef STVID_DEBUG_GET_STATISTICS
    ST_ErrorCode_t (*GetStatistics)(const VIDTRAN_Handle_t TranscoderHandle, STVID_EncoderStatistics_t * const Statistics_p);
    ST_ErrorCode_t (*ResetAllStatistics)(const VIDTRAN_Handle_t TranscoderHandle, STVID_EncoderStatistics_t * const Statistics_p);
#endif /* STVID_DEBUG_GET_STATISTICS */
    ST_ErrorCode_t (*Init)(const VIDTRAN_InitParams_t * const InitParams_p, VIDTRAN_Handle_t * const TranscoderHandle_p);
    ST_ErrorCode_t (*Term)(const VIDTRAN_Handle_t TranscoderHandle);
    ST_ErrorCode_t (*Open)(const VIDTRAN_Handle_t TranscoderHandle, const STVID_TranscoderOpenParams_t * const TranscoderOpenParams_p);
    ST_ErrorCode_t (*Close)(const VIDTRAN_Handle_t TranscoderHandle, const STVID_TranscoderCloseParams_t * const TranscoderCloseParams_p);
    ST_ErrorCode_t (*Start)(const VIDTRAN_Handle_t TranscoderHandle, const STVID_TranscoderStartParams_t * const StartParams_p);
    ST_ErrorCode_t (*Stop)(const VIDTRAN_Handle_t TranscoderHandle, const STVID_TranscoderStopParams_t * const StopParams_p);
    ST_ErrorCode_t (*SetParams)(const VIDTRAN_Handle_t TranscoderHandle, const STVID_TranscoderSetParams_t * const SetParams_p);
    ST_ErrorCode_t (*SetMode)(const VIDTRAN_Handle_t TranscoderHandle, const STVID_TranscoderSetMode_t * const SetMode_p);
    ST_ErrorCode_t (*Link)(const VIDTRAN_Handle_t TranscoderHandle, const STVID_TranscoderLinkParams_t * const TranscoderLinkParams_p);
    ST_ErrorCode_t (*Unlink)(const VIDTRAN_Handle_t TranscoderHandle);
    ST_ErrorCode_t (*InformReadAddress)(const VIDTRAN_Handle_t TranscoderHandle, void * const Read_p);
    ST_ErrorCode_t (*SetErrorRecoveryMode)(const VIDTRAN_Handle_t TranscoderHandle, const STVID_TranscoderErrorRecoveryMode_t Mode);
    ST_ErrorCode_t (*GetErrorRecoveryMode)(const VIDTRAN_Handle_t TranscoderHandle, STVID_TranscoderErrorRecoveryMode_t * const Mode_p);
} VIDTRAN_FunctionsTable_t;

typedef struct {
    const VIDTRAN_FunctionsTable_t * FunctionsTable_p;
    ST_Partition_t *        CPUPartition_p; /* Where the module can allocate memory for its internal usage */
    VIDTRAN_Handle_t        InternalTranscoderHandle;
} VIDTRAN_Properties_t;

ST_ErrorCode_t VIDTRAN_Init(const VIDTRAN_InitParams_t * const InitParams_p, VIDTRAN_Handle_t * const TranscoderHandle_p);
ST_ErrorCode_t VIDTRAN_Term(const VIDTRAN_Handle_t TranscoderHandle);

#ifdef STVID_DEBUG_GET_STATISTICS
#define VIDTRAN_GetStatistics(TranscoderHandle, Statistics_p)                         ((VIDTRAN_Properties_t *)(TranscoderHandle))->FunctionsTable_p->GetStatistics(((VIDTRAN_Properties_t *)(TranscoderHandle))->InternalTranscoderHandle, (Statistics_p))
#define VIDTRAN_ResetAllStatistics(TranscoderHandle, Statistics_p)                    ((VIDTRAN_Properties_t *)(TranscoderHandle))->FunctionsTable_p->ResetAllStatistics(((VIDTRAN_Properties_t *)(TranscoderHandle))->InternalTranscoderHandle, (Statistics_p))
#endif /* STVID_DEBUG_GET_STATISTICS */
#define VIDTRAN_Open(TranscoderHandle, OpenParams_p)                                  ((VIDTRAN_Properties_t *)(TranscoderHandle))->FunctionsTable_p->Open(((VIDTRAN_Properties_t *)(TranscoderHandle))->InternalTranscoderHandle, (OpenParams_p))
#define VIDTRAN_Close(TranscoderHandle, CloseParams_p)                                ((VIDTRAN_Properties_t *)(TranscoderHandle))->FunctionsTable_p->Close(((VIDTRAN_Properties_t *)(TranscoderHandle))->InternalTranscoderHandle, (CloseParams_p))
#define VIDTRAN_Start(TranscoderHandle, StartParams_p)                                ((VIDTRAN_Properties_t *)(TranscoderHandle))->FunctionsTable_p->Start(((VIDTRAN_Properties_t *)(TranscoderHandle))->InternalTranscoderHandle, (StartParams_p))
#define VIDTRAN_Stop(TranscoderHandle, StopParams_p)                                  ((VIDTRAN_Properties_t *)(TranscoderHandle))->FunctionsTable_p->Stop(((VIDTRAN_Properties_t *)(TranscoderHandle))->InternalTranscoderHandle, (StopParams_p))
#define VIDTRAN_SetParams(TranscoderHandle, SetParams_p)                              ((VIDTRAN_Properties_t *)(TranscoderHandle))->FunctionsTable_p->SetParams(((VIDTRAN_Properties_t *)(TranscoderHandle))->InternalTranscoderHandle, (SetParams_p))
#define VIDTRAN_SetMode(TranscoderHandle, SetMode_p)                                  ((VIDTRAN_Properties_t *)(TranscoderHandle))->FunctionsTable_p->SetMode(((VIDTRAN_Properties_t *)(TranscoderHandle))->InternalTranscoderHandle, (SetMode_p))
#define VIDTRAN_Link(TranscoderHandle, LinkParams_p)                                  ((VIDTRAN_Properties_t *)(TranscoderHandle))->FunctionsTable_p->Link(((VIDTRAN_Properties_t *)(TranscoderHandle))->InternalTranscoderHandle, (LinkParams_p))
#define VIDTRAN_Unlink(TranscoderHandle)                                              ((VIDTRAN_Properties_t *)(TranscoderHandle))->FunctionsTable_p->Unlink(((VIDTRAN_Properties_t *)(TranscoderHandle))->InternalTranscoderHandle)
#define VIDTRAN_InformReadAddress(TranscoderHandle, Read_p)                           ((VIDTRAN_Properties_t *)(TranscoderHandle))->FunctionsTable_p->InformReadAddress(((VIDTRAN_Properties_t *)(TranscoderHandle))->InternalTranscoderHandle, (Read_p))
#define VIDTRAN_SetErrorRecoveryMode(TranscoderHandle, Mode)                          ((VIDTRAN_Properties_t *)(TranscoderHandle))->FunctionsTable_p->SetErrorRecoveryMode(((VIDTRAN_Properties_t *)(TranscoderHandle))->InternalTranscoderHandle, (Mode))
#define VIDTRAN_GetErrorRecoveryMode(TranscoderHandle, Mode_p)                        ((VIDTRAN_Properties_t *)(TranscoderHandle))->FunctionsTable_p->GetErrorRecoveryMode(((VIDTRAN_Properties_t *)(TranscoderHandle))->InternalTranscoderHandle, (Mode_p))

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_TRANSCODE_H */

/* End of trancode.h */
