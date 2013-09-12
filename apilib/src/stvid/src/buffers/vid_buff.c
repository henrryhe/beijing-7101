/*******************************************************************************

File name   : vid_buff.c

Description : frame buffer management source file

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
21 Mar 2000        Created                                           HG
21 Feb 2001        Memory Profile management                         GG
03 Mar 2001        Secondary frame buffer management (decimation
                   feature) and VIDBUFF_AllocateMemoryForProfile
                   return values.                                    GG
25 Jul 2001        FrameBuffer allocated with height and width
                   insted of a buffer size.                          GG
29 Jan 2002        Add picture to list of refs only if not of dis-
                   -play, correcting bug in oldest reference         HG
20 may 2002        Removed the whole reference list management.      PLe
*******************************************************************************/
/*#define DEBUG_FB_MAPPING */

/* Private preliminary definitions (internal use only) ---------------------- */

/* Usual traces */
/*#define TRACE_BUFFER*/
/*#define STTBX_REPORT*/

/* Traces for Visual Debugger */
/*#define TRACE_FRAME_BUFFER_LOCK_STATUS */

#ifdef TRACE_FRAME_BUFFER_LOCK_STATUS
#define TraceFrameBufferLockStatus(x) TraceState x
#else
#define TraceFrameBufferLockStatus(x)
#endif  /* TRACE_FRAME_BUFFER_LOCK_STATUS */

/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <stdio.h>
#include <string.h>
#endif

#include "stddefs.h"
#include "stos.h"

#include "vid_buff.h"
#include "vid_ctcm.h"

#include "halv_buf.h"

#include "stavmem.h"
#include "sttbx.h"
#include "stevt.h"

/* Trace System activation ---------------------------------------------- */

/* Enable this if you want to see the Traces... */
/*#define TRACE_UART*/

/* Select the Traces you want */
#define TrMain                   TraceOff
#define TrMemAlloc               TraceOff
#define TrTakeRelease            TraceOff
#define TrPPB                    TraceOff
#define TrDealloc                TraceOff
#define TrFrameBuffer            TraceOff
#define TrDecFrameBuffer         TraceOff
#define TrExtraFrameBuffer       TraceOff
#define TrLock                   TraceOff
#define TrStatus                 TraceOff
#define TrPictureStatus          TraceOff
#define TrBitBuffer              TraceOff
#define TrSmoothBackwrd          TraceOff
#define TrOverwrite              TraceOff
#define TrImageType              TraceOff

#define TrWarning                TraceOn
/* TrError is now defined in vid_trace.h */

/* "vid_trace.h" should be included AFTER the definition of the traces wanted */
#include "vid_trace.h"

/* Enable this if you want to see which Frame Buffers are used after a stop */
/*#define SHOW_FRAME_BUFFER_USED*/

/* Private Types ------------------------------------------------------------ */

/* Defines which reconstruction mode is available
    Warning: There is an other structure called "VIDBUFF_AvailableReconstructionMode_t" with similar names */
typedef enum VIDBUFF_ReconstructionMode_e
{
    VIDBUFF_ONLY_MAIN_RECONSTRUCTION,
    VIDBUFF_ONLY_SECONDARY_RECONSTRUCTION,
    VIDBUFF_MAIN_AND_SECONDARY_RECONSTRUCTION
} VIDBUFF_ReconstructionMode_t;


/* Private Constants -------------------------------------------------------- */

#define     EXTRA_FRAME_BUFFER_MAX_WIDTH        720
#define     EXTRA_FRAME_BUFFER_MAX_HEIGHT       (576+32)                 /* HEIGHT+32 to avoid hardware failure */

#define     FILENAME_MAX_LENGTH                 10

#define MAX_DECODING_ORDER_FRAME_ID             0xffffffff

/* Private Variables (static)------------------------------------------------ */

/* Dummy value that we set to check that a handle is valid, only one chance out of 2^32 to get it by accident ! */
#define VIDBUFF_VALID_HANDLE 0x01451450

/* This is the number of added frame buffer in the allocated frame buffer array. It's because when
 * a vid_stop occurs, 1 (or 2) frame buffer(s) may stay reserved by the display, so if the memory profile change
 * the video driver must be able to deallocated them but keep the right number of frame buffers !!! */
#define VIDBUF_ADDED_FRAME_BUFFER 2

/* when getting an unused buffer, if a reserved buffer has a extended temp VIDBUF_EXT_TEMP_DIF_MAX */
/* more than the current displayed one, unlock it. */
#ifdef ST_smoothbackward
#define VIDBUF_EXT_TEMP_DIF_MAX   30
#endif

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */
#define  MPEGFrame2Char(Type)  (                                        \
               (  (Type) == STVID_MPEG_FRAME_I)                         \
                ? 'I' : (  ((Type) == STVID_MPEG_FRAME_P)               \
                         ? 'P' : (  ((Type) == STVID_MPEG_FRAME_B)      \
                                  ? 'B'  : 'U') ) )

#define  PictureStructure2String(PictureStructure)  (                                        \
               (  (PictureStructure) == STVID_PICTURE_STRUCTURE_FRAME)                         \
                ? "frame" : (  ((PictureStructure) == STVID_PICTURE_STRUCTURE_TOP_FIELD)               \
                         ? "top field" : (  ((PictureStructure) == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)      \
                                  ? "bot field"  : "Unknown") ) )

#define IsLocked(Picture_p)            	        ((Picture_p)->Buffers.PictureLockCounter > 0)
#define IsUnlocked(Picture_p)                   (!IsLocked(Picture_p))

/*  A Picture is considered as Last Field On Display if:
    - DisplayStatus == VIDBUFF_LAST_FIELD_ON_DISPLAY
    AND
    - the picture is locked only by the display (VIDCOM_VIDQUEUE_MODULE_BASE or VIDCOM_DEI_MODULE_BASE) */
#define IsLockedLastFieldOnDisplay(Picture_p)       ( ( (Picture_p)->Buffers.PictureLockCounter -                                       \
                                                        (Picture_p)->Buffers.Module[VIDCOM_VIDQUEUE_MODULE_BASE].TakeReleaseCounter -   \
                                                        (Picture_p)->Buffers.Module[VIDCOM_DEI_MODULE_BASE].TakeReleaseCounter == 0) && \
                                                      ( (Picture_p)->Buffers.DisplayStatus == VIDBUFF_LAST_FIELD_ON_DISPLAY) )

/* Macros checking if one of the 2 Picture Buffers is locked */
#define HasFrameOrFirstFieldPictureLocked(Frame)    ( ((Frame).FrameOrFirstFieldPicture_p != NULL) && (IsLocked((Frame).FrameOrFirstFieldPicture_p) ) )
#define HasSecondFieldPictureLocked(Frame)          ( ((Frame).NothingOrSecondFieldPicture_p != NULL) && (IsLocked((Frame).NothingOrSecondFieldPicture_p) ) )
#define HasAPictureLocked(Frame)                    ( HasFrameOrFirstFieldPictureLocked(Frame) || HasSecondFieldPictureLocked(Frame) )

#define HasNoPicture(Frame)                         (((Frame).FrameOrFirstFieldPicture_p) == NULL)
#define HasFrameNotFields(Frame)                    (((Frame).NothingOrSecondFieldPicture_p) == NULL)

#define FrameBufferToBeKilledAsSoonAsPossible(Frame) ((Frame).ToBeKilledAsSoonAsPossible)

#define IsAPrimaryFrameBuffer(Frame_p)                         (Frame_p->FrameBufferType != VIDBUFF_SECONDARY_FRAME_BUFFER)

/* Private Function prototypes ---------------------------------------------- */

#ifdef TRACE_BUFFER
static void PrintFrameBuffers(const VIDBUFF_Handle_t BuffersHandle);
static void PrintOneFrameBuffer(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_FrameBuffer_t * FrameBuffer_p);
#endif
static void LeavePicture(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const Picture_p);
static VIDBUFF_PictureBuffer_t * UsePicture(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_FrameBuffer_t * const Frame_p);
static void ResetPictureBufferParams(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const Picture_p);

static ST_ErrorCode_t RemoveFrameBufferFromFrameBufferLoop(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_FrameBuffer_t * const FrameBuffer_p);
static ST_ErrorCode_t AllocateFrameBuffers(const VIDBUFF_Handle_t BuffersHandle,
        const VIDBUFF_AllocateBufferParams_t * const AllocParams_p,
        const U32 NumberOfBuffers,
        const U32 NumberOfDecimatedBuffers);
static void VIDBUFF_TagEveryBuffersAsToBeKilledAsap(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_FrameBuffer_t * FrameBufferPool_p);
static void CheckIfToBeKilledAsSoonAsPossibleAndRemove(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const Picture_p);
static ST_ErrorCode_t DeAllocateFrameBuffers(const VIDBUFF_Handle_t BuffersHandle);
static void InitBufferManagerStructure(const VIDBUFF_Handle_t BuffersHandle);
static void TermBufferManagerStructure(const VIDBUFF_Handle_t BuffersHandle);
static ST_ErrorCode_t GetUnusedPictureBuffer(const VIDBUFF_Handle_t BuffersHandle,
        const VIDBUFF_GetUnusedPictureBufferParams_t * const Params_p,
        VIDBUFF_PictureBuffer_t ** const PictureBuffer_p,
        VIDBUFF_FrameBuffer_t * FrameBufferPool_p);
static ST_ErrorCode_t VIDBUFF_CheckAndUnlockFrameBuffers(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_FrameBuffer_t * FrameBuffer_p);
static ST_ErrorCode_t VIDBUFF_CheckAndUnlockPictureBuffer(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * PictureBuffer_p);

static ST_ErrorCode_t AllocateNewFrameBuffer(const VIDBUFF_Handle_t BuffersHandle,
        const VIDBUFF_AllocateBufferParams_t * const AllocParams_p, const VIDCOM_InternalProfile_t Profile, const U8 MaxFramesNumber,
        VIDBUFF_FrameBuffer_t ** const AllocatedFrameBuffersLoop_p, const HALBUFF_AllocateBufferParams_t HALAllocParams,
        const U32 MaxTotalFBSize, U32 * const NumberOfAllocatedFrameBuffers_p, const VIDBUFF_AvailableReconstructionMode_t RequiredAvailableReconstructionMode,
        const HALBUFF_FrameBufferType_t FrameBufferReconstructionMode, BOOL * const NeedForExtraAllocation_p, BOOL * const MaxSizedReached_p);

static ST_ErrorCode_t DeAllocateFrameBuffer(const VIDBUFF_Handle_t BuffersHandle,
                                            VIDBUFF_FrameBuffer_t *    FrameBufferToRemove_p);

#ifdef STVID_DEBUG_TAKE_RELEASE
static void VIDBUFF_PictureBufferShouldBeUnused(const VIDBUFF_Handle_t BuffersHandle,
                VIDBUFF_PictureBuffer_t * const Picture_p);
#endif

#ifdef ST_OSLINUX
static ST_ErrorCode_t NotifyRemovedFrameBuffers(const VIDBUFF_Handle_t BuffersHandle,
                                                FrameBuffers_t * FrameBuffers_p,
                                                U32              NbFrameBuffers);
static ST_ErrorCode_t NotifyNewFrameBuffers(const VIDBUFF_Handle_t BuffersHandle,
                            FrameBuffers_t * FrameBuffers_p,
                            U32              NbFrameBuffers);
#endif

#ifdef ST_smoothbackward
ST_ErrorCode_t GetSmoothBackwardGenericUnusedPictureBuffer(
        const VIDBUFF_Handle_t BuffersHandle,
        const VIDBUFF_GetSmoothBackwardUnusedPictureBufferParams_t * const Params_p,
        VIDBUFF_PictureBuffer_t ** const PictureBuffer_p,
        VIDBUFF_FrameBuffer_t * FrameBufferPool_p);
#endif /* ST_smoothbackward */

#ifdef USE_EXTRA_FRAME_BUFFERS
    /* Extra Frame Buffers Management */
    static ST_ErrorCode_t AllocateExtraPrimaryFrameBufferPool(const VIDBUFF_Handle_t BuffersHandle,
                                                       VIDBUFF_AllocateBufferParams_t * const AllocParams_p,
                                                       const U32 NbrOfExtraFrameBuffersRequested);
    static ST_ErrorCode_t DeAllocateExtraPrimaryFrameBufferPool(const VIDBUFF_Handle_t BuffersHandle);

    static ST_ErrorCode_t AllocateExtraPrimaryFrameBuffer(const VIDBUFF_Handle_t BuffersHandle,
                                                   const VIDBUFF_AllocateBufferParams_t * const AllocParams_p,
                                                   VIDBUFF_FrameBuffer_t **                     AllocatedFrameBuffer_p_p);
    static U32 GetNbrOfFrameBufferAllocatedInAPool(const VIDBUFF_Handle_t BuffersHandle,
                                                   VIDBUFF_FrameBuffer_t * FrameBufferLoop_p);
    /* Extra secondary Frame Buffers Management */
    static ST_ErrorCode_t AllocateExtraSecondaryFrameBufferPool(const VIDBUFF_Handle_t BuffersHandle,
                                                       VIDBUFF_AllocateBufferParams_t * const AllocParams_p,
                                                       const U32 NbrOfExtraFrameBuffersRequested);
    static ST_ErrorCode_t DeAllocateExtraSecondaryFrameBufferPool(const VIDBUFF_Handle_t BuffersHandle);

    static ST_ErrorCode_t AllocateExtraSecondaryFrameBuffer(const VIDBUFF_Handle_t BuffersHandle,
                                                   const VIDBUFF_AllocateBufferParams_t * const AllocParams_p,
                                                   VIDBUFF_FrameBuffer_t **                     AllocatedFrameBuffer_p_p);
#endif /* USE_EXTRA_FRAME_BUFFERS */

#if defined (TRACE_UART) || defined (VIDEO_DEBUG)
    static void PrintPictureBufferStatus(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const Picture_p);
    static void PrintPoolPictureBuffersStatus(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_FrameBuffer_t * FrameBufferPool_p);
#endif /* TRACE_UART || VIDEO_DEBUG */

static void vidbuff_FreeAdditionnalData(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const Picture_p, const BOOL Force);

/* Public functions --------------------------------------------------------- */

/*******************************************************************************
Name        : VIDBUFF_ChangeFrameBufferParams
Description : Change frame buffers # and size needs (due to stream properties changes)
Parameters  : Video buffer manager handle, requested Width, Height & NumberOfFrames
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if bad parameter
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_ChangeFrameBufferParams(const VIDBUFF_Handle_t BuffersHandle, const U32 Width, const U32 Height, const U8 NumberOfFrames)
{
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
    VIDBUFF_FrameBuffer_t * PreviousFrame_p;
    VIDBUFF_FrameBuffer_t * CurrentFrame_p;
#ifdef ST_OSLINUX
    FrameBuffers_t            * FrameBuffers_p = NULL;
    U32                         NbFrameBuffers = 0;
#endif

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Change FB params from %dx%d %d FBs to %dx%d %d FBs",
            VIDBUFF_Data_p->FrameBufferDynAlloc.Width,
            VIDBUFF_Data_p->FrameBufferDynAlloc.Height,
            VIDBUFF_Data_p->FrameBufferDynAlloc.NumberOfFrames,
            Width,
            Height,
            NumberOfFrames));
    /* We must flag all allocated frame buffers to make them deallocated ASAP */
    /* Initialize the frame pointer to the first one of the allocated loop.
        * And flag all frame buffers. */

    TrMain(("\r\nVIDBUFF_ChangeFrameBufferParams"));

#ifdef ST_OSLINUX
    /* Allocates max possible FB entries (primary + secondary) */
    FrameBuffers_p = (FrameBuffers_t *)STOS_MemoryAllocate(NULL, (VIDBUFF_Data_p->MaxFrameBuffers + VIDBUFF_Data_p->MaxSecondaryFrameBuffers)*sizeof(FrameBuffers_t));
#endif

    /* Lock access to allocated frame buffers loop */
    semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    VIDBUFF_TagEveryBuffersAsToBeKilledAsap(BuffersHandle, VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p);
    VIDBUFF_TagEveryBuffersAsToBeKilledAsap(BuffersHandle, VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p);

    /* primary pool */
    /*--------------*/
    PreviousFrame_p = VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p;
    if (PreviousFrame_p != NULL)
    {
        CurrentFrame_p = PreviousFrame_p->NextAllocated_p;
    }
    else
    {
        CurrentFrame_p = NULL;
    }
    while ((CurrentFrame_p != VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p) && (CurrentFrame_p != NULL))
    {
        if (HasNoPicture(*CurrentFrame_p) || (!HasAPictureLocked(*(CurrentFrame_p))))
        {
            /* Frame Buffer not used: de-allocate it */

            /* Remove it from the Pool */
            PreviousFrame_p->NextAllocated_p = CurrentFrame_p->NextAllocated_p;
            VIDBUFF_Data_p->NumberOfAllocatedFrameBuffers --;

#ifdef ST_OSLINUX
            /* Store this FB has being removed */
            FrameBuffers_p[NbFrameBuffers].KernelAddress_p = CurrentFrame_p->Allocation.Address_p;
            FrameBuffers_p[NbFrameBuffers].Size = CurrentFrame_p->Allocation.TotalSize;
            NbFrameBuffers++;
#endif
            DeAllocateFrameBuffer(BuffersHandle, CurrentFrame_p);
        }
        else
        {
            PreviousFrame_p = CurrentFrame_p;
        }
        CurrentFrame_p = PreviousFrame_p->NextAllocated_p;
    }
    /* Here CurrentFrame_p == VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p: check if it is to be removed */
    if (CurrentFrame_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unexpected CurrentFrame_p == NULL"));
    }
    else
    {
        if (HasNoPicture(*CurrentFrame_p) || (!HasAPictureLocked(*(CurrentFrame_p))))
        {
            /* Not locked : de-allocate it */
            if (PreviousFrame_p == CurrentFrame_p)
            {
                /* Picture is alone in loop */
                VIDBUFF_Data_p->NumberOfAllocatedFrameBuffers = 0;
                VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p = NULL;
            }
            else
            {
                /* Picture is not alone in loop */
                PreviousFrame_p->NextAllocated_p = CurrentFrame_p->NextAllocated_p;
                VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p = PreviousFrame_p;
                VIDBUFF_Data_p->NumberOfAllocatedFrameBuffers --;
            }

#ifdef ST_OSLINUX
            /* Store this FB has being removed */
            FrameBuffers_p[NbFrameBuffers].KernelAddress_p = CurrentFrame_p->Allocation.Address_p;
            FrameBuffers_p[NbFrameBuffers].Size = CurrentFrame_p->Allocation.TotalSize;
            NbFrameBuffers++;
#endif

            /* Deallocate the Frame Buffer*/
            DeAllocateFrameBuffer(BuffersHandle, CurrentFrame_p);

        }
    }

    /* secondary pool */
    /*--------------*/
    PreviousFrame_p = VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p;
    if (PreviousFrame_p != NULL)
    {
        CurrentFrame_p = PreviousFrame_p->NextAllocated_p;
    }
    else
    {
        CurrentFrame_p = NULL;
    }
    while ((CurrentFrame_p != VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p) && (CurrentFrame_p != NULL))
    {
        if (HasNoPicture(*CurrentFrame_p) || (!HasAPictureLocked(*(CurrentFrame_p))))
        {
            /* Frame Buffer not used: de-allocate it */
            /* Remove it from the Pool */
            PreviousFrame_p->NextAllocated_p = CurrentFrame_p->NextAllocated_p;
            VIDBUFF_Data_p->NumberOfSecondaryAllocatedFrameBuffers --;

#ifdef ST_OSLINUX
            /* Store this FB has being removed */
            FrameBuffers_p[NbFrameBuffers].KernelAddress_p = CurrentFrame_p->Allocation.Address_p;
            FrameBuffers_p[NbFrameBuffers].Size = CurrentFrame_p->Allocation.TotalSize;
            NbFrameBuffers++;
#endif
            DeAllocateFrameBuffer(BuffersHandle, CurrentFrame_p);
        }
        else
        {
            PreviousFrame_p = CurrentFrame_p;
        }
        CurrentFrame_p = PreviousFrame_p->NextAllocated_p;
    }
    /* Here CurrentFrame_p == VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p: check if it is to be removed */
    if (CurrentFrame_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unexpected CurrentFrame_p == NULL"));
    }
    else
    {
        if (HasNoPicture(*CurrentFrame_p) || (!HasAPictureLocked(*(CurrentFrame_p))))
        {
            /* Not locked : de-allocate it */
            if (PreviousFrame_p == CurrentFrame_p)
            {
                /* Picture is alone in loop */
                VIDBUFF_Data_p->NumberOfSecondaryAllocatedFrameBuffers = 0;
                VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p = NULL;
            }
            else
            {
                /* Picture is not alone in loop */
                PreviousFrame_p->NextAllocated_p = CurrentFrame_p->NextAllocated_p;
                VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p = PreviousFrame_p;
                VIDBUFF_Data_p->NumberOfSecondaryAllocatedFrameBuffers --;
            }

#ifdef ST_OSLINUX
            /* Store this FB has being removed */
            FrameBuffers_p[NbFrameBuffers].KernelAddress_p = CurrentFrame_p->Allocation.Address_p;
            FrameBuffers_p[NbFrameBuffers].Size = CurrentFrame_p->Allocation.TotalSize;
            NbFrameBuffers++;
#endif

            /* Deallocate the Frame Buffer*/
            DeAllocateFrameBuffer(BuffersHandle, CurrentFrame_p);

        }
    }

#ifdef ST_OSLINUX
    /* Notify removed secondary FB */
    NotifyRemovedFrameBuffers(BuffersHandle, FrameBuffers_p, NbFrameBuffers);
    if (FrameBuffers_p != NULL)
    {
        STOS_MemoryDeallocate(NULL, FrameBuffers_p);
    }
#endif

    /* Un-lock access to allocated frame buffers loop */
    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    /* Save the new requested size for next frame buffers allocation */
    VIDBUFF_Data_p->FrameBufferDynAlloc.Width = Width;
    VIDBUFF_Data_p->FrameBufferDynAlloc.Height = Height;
    VIDBUFF_Data_p->FrameBufferDynAlloc.NumberOfFrames = NumberOfFrames;
    VIDBUFF_Data_p->Profile.ApiProfile.MaxWidth = Width;
    VIDBUFF_Data_p->Profile.ApiProfile.MaxHeight = Height;

    /* Allow to send IMPOSSIBLE_WITH_PROFILE_EVT again: structure proposed may be different with new profile */
    VIDBUFF_Data_p->IsNotificationOfImpossibleWithProfileAllowed = TRUE;

    return(ST_NO_ERROR);
} /* End of VIDBUFF_ChangeFrameBufferParams() functions */

/*******************************************************************************
Name        : VIDBUFF_NeedToChangeFrameBufferParams
Description : Check frame buffers # and size needs (due to stream properties changes)
Parameters  : Video buffer manager handle, requested Width, Height & NumberOfFrames
Assumptions :
Limitations :
Returns     : TRUE if changes are needed in size or # of frame buffers, else FALSE
*******************************************************************************/
BOOL VIDBUFF_NeedToChangeFrameBufferParams(const VIDBUFF_Handle_t BuffersHandle, const U32 Width, const U32 Height, const U8 NumberOfFrames)
{
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;

    /*
     * if (StreamWidth * StreamHeight) <= (CurrentWidth * CurrentHeight) and NumberOfFrames <= CurrentMaxFrames there is no need to change the currently allocated frame buffers.
     * else, we've got to mark all currently allocated frame buffers (i.e. frame buffers with CurrentWidth * CurrentHeight resolution) as 'ToBeKilledAsSoonAsPossible' and
     * allow buffer module to allocate a new frame buffer upon producer requests
     * up to min(NumberofFrameBuffersComputedFrom("space available" or "max size passed by API"), NumberOfFrames).
     */
    if (((Width * Height) > (VIDBUFF_Data_p->FrameBufferDynAlloc.Width * VIDBUFF_Data_p->FrameBufferDynAlloc.Height))
        || (NumberOfFrames > VIDBUFF_Data_p->FrameBufferDynAlloc.NumberOfFrames))
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }

} /* End of VIDBUFF_NeedToChangeFrameBufferParams() function */

/*******************************************************************************
Name        : VIDBUFF_AllocateBitBuffer
Description :
Parameters  : Video buffer manager handle, alloc params, buffer params
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if bad parameter
              ST_ERROR_NO_MEMORY if allocation failed
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_AllocateBitBuffer(const VIDBUFF_Handle_t BuffersHandle, const VIDBUFF_AllocateBufferParams_t * const AllocParams_p, VIDBUFF_BufferAllocationParams_t * const BufferParams_p)
{
    ST_ErrorCode_t ErrorCode;

    if ((AllocParams_p == NULL) || (BufferParams_p == NULL) || (AllocParams_p->BufferSize == 0))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    ErrorCode = HALBUFF_AllocateBitBuffer(((VIDBUFF_Data_t *) BuffersHandle)->HALBuffersHandle, AllocParams_p, BufferParams_p);

    if (ErrorCode == ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Bit buffer allocated at address 0x%x", BufferParams_p->Address_p));
    }

    return(ErrorCode);
} /* End of VIDBUFF_AllocateBitBuffer() function */

/*******************************************************************************
Name        : VIDBUFF_ReAllocateBitBuffer
Description : Reallocates bit buffer that was allocated by driver
Parameters  : Video buffer manager handle, alloc params, buffer params
Assumptions : Driver in STOPPED state
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if bad parameter
              ST_ERROR_NO_MEMORY if allocation failed
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_ReAllocateBitBuffer(const VIDBUFF_Handle_t BuffersHandle,
                                           const VIDBUFF_AllocateBufferParams_t * const AllocParams_p,
                                           VIDBUFF_BufferAllocationParams_t * const BufferParams_p,
                                           const STAVMEM_PartitionHandle_t AVMEMPartitionHandle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (AVMEMPartitionHandle == (STAVMEM_PartitionHandle_t)NULL)   /* STAVMEM_INVALID_PARTITION_HANDLE may not exist yet! */
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* De-allocate bit buffer */
    if (VIDBUFF_DeAllocateBitBuffer(BuffersHandle, BufferParams_p) == ST_NO_ERROR)
    {
        ErrorCode = HALBUFF_SetAvmemPartitionForBitBuffer(((VIDBUFF_Data_t *) BuffersHandle)->HALBuffersHandle,
                                                          AVMEMPartitionHandle);
        if (ErrorCode == ST_NO_ERROR)
        {
            ErrorCode = VIDBUFF_AllocateBitBuffer(BuffersHandle, AllocParams_p, BufferParams_p);
            if (ErrorCode != ST_NO_ERROR)
            {
                /* Error: cannot allocate bit buffer */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Could not allocate new bit buffer !"));
                ErrorCode = ST_ERROR_NO_MEMORY;
            }
        }
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Could not Set Bit Buffer partition"));
        }
    }
    else
    {
        /* Could not deallocate original bit buffer */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Could not deallocate original bit buffer!"));
    }

    return(ErrorCode);
}


#if defined(DVD_SECURED_CHIP)
/*******************************************************************************
Name        : VIDBUFF_AllocateESCopyBuffer
Description : This function is called when video driver needs to allocate the ES Copy buffer.
Parameters  : Video buffer manager handle, alloc params, buffer params
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if bad parameter
              ST_ERROR_NO_MEMORY if allocation failed
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_AllocateESCopyBuffer(const VIDBUFF_Handle_t BuffersHandle, const VIDBUFF_AllocateBufferParams_t * const AllocParams_p, VIDBUFF_BufferAllocationParams_t * const BufferParams_p)
{
    ST_ErrorCode_t ErrorCode;

    if ((AllocParams_p == NULL) || (BufferParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (AllocParams_p->BufferSize == 0)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    ErrorCode = HALBUFF_AllocateESCopyBuffer(((VIDBUFF_Data_t *) BuffersHandle)->HALBuffersHandle, AllocParams_p, BufferParams_p);

    if (ErrorCode == ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ES Copy buffer allocated at address 0x%x", BufferParams_p->Address_p));
    }

    return(ErrorCode);
} /* End of VIDBUFF_AllocateESCopyBuffer() function */

/*******************************************************************************
Name        : VIDBUFF_DeAllocateESCopyBuffer
Description :
Parameters  : Video buffer manager handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if problem
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_DeAllocateESCopyBuffer(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_BufferAllocationParams_t * const BufferParams_p)
{
    ST_ErrorCode_t ErrorCode;

    if ((BufferParams_p == NULL) || (BufferParams_p->TotalSize == 0))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    ErrorCode = HALBUFF_DeAllocateESCopyBuffer(((VIDBUFF_Data_t *) BuffersHandle)->HALBuffersHandle, BufferParams_p);

    if (ErrorCode == ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ES Copy buffer de-allocated from address 0x%x", BufferParams_p->Address_p));
    }

    return(ErrorCode);
} /* End of VIDBUFF_DeAllocateESCopyBuffer() function */

#endif /* DVD_SECURED_CHIP */


/*******************************************************************************
Name        : VIDBUFF_AllocateMemoryForProfile
Description : Allocate frame buffers (primary and secondary if required).
              The number of the buffers is allocated is specified in
              the memory profile or can be overriden by setting
              ManualNumberOfBuffers to TRUE.
Parameters  : Video buffer manager handle
Assumptions : The memory profile is supposed to be good (MaxWeight, MaxHeight
              and NbFrameStore not null).
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER if too many buffers required
              ST_ERROR_NO_MEMORY if STAVMEM allocation failed
              ST_NO_ERROR if everything OK
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_AllocateMemoryForProfile(const VIDBUFF_Handle_t BuffersHandle, const BOOL ManualNumberOfBuffers, const U8 NumberOfBuffers)
{
    VIDBUFF_AllocateBufferParams_t  AllocParams;
    ST_ErrorCode_t                  ErrorCode;

    AllocParams.AllocationMode  = VIDBUFF_ALLOCATION_MODE_SHARED_MEMORY; /* !!! Should be passed */
    AllocParams.PictureWidth    = ((VIDBUFF_Data_t *) BuffersHandle)->Profile.ApiProfile.MaxWidth;
    AllocParams.PictureHeight   = ((VIDBUFF_Data_t *) BuffersHandle)->Profile.ApiProfile.MaxHeight;
    AllocParams.BufferType      = ((VIDBUFF_Data_t *) BuffersHandle)->FrameBuffersType;

#ifdef TRACE_BUFFER
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Before Allocating :"));
    PrintFrameBuffers(BuffersHandle);
#endif

    /* Here, allocate the memory for the frame buffers (primary and secondary (if required). */
    if(!ManualNumberOfBuffers)
    {
    ErrorCode = AllocateFrameBuffers(BuffersHandle, &AllocParams,
            ((VIDBUFF_Data_t *) BuffersHandle)->Profile.NbMainFrameStore,
            ((VIDBUFF_Data_t *) BuffersHandle)->Profile.NbSecondaryFrameStore);
    }
    else
    {
        ErrorCode = AllocateFrameBuffers(BuffersHandle, &AllocParams,
              NumberOfBuffers,
            (((VIDBUFF_Data_t *) BuffersHandle)->Profile.NbSecondaryFrameStore > 0) ? NumberOfBuffers : 0 );
    }

#ifdef USE_EXTRA_FRAME_BUFFERS
        ((VIDBUFF_Data_t *) BuffersHandle)->IsMemoryAllocatedForProfile = TRUE;

        /* Check if extra frame buffers needs to be allocated or if they are already allocated */
        /* Extra frame buffers can be allocated in 2 ways: */
        /* - when viewport is opened (call to VIDBUFF_SetNbrOfExtraPrimaryFrameBuffers) if memory has already been allocated for profile */
        /* - when memory is being allocated (call to VIDBUFF_AllocateMemoryForProfile)for profile if viewport has already been opened */
        if (((VIDBUFF_Data_t *) BuffersHandle)->NumberOfExtraPrimaryFrameBuffersRequested !=  0)
        {
            if(GetNbrOfFrameBufferAllocatedInAPool(BuffersHandle, ((VIDBUFF_Data_t *) BuffersHandle)->AllocatedExtraPrimaryFrameBuffersLoop_p) !=
                ((VIDBUFF_Data_t *) BuffersHandle)->NumberOfExtraPrimaryFrameBuffersRequested)
            {
                /* Reserve the specified number of SD frame buffers */
                VIDBUFF_SetNbrOfExtraPrimaryFrameBuffers(BuffersHandle, ((VIDBUFF_Data_t *) BuffersHandle)->NumberOfExtraPrimaryFrameBuffersRequested);
            }
        }
        if (((VIDBUFF_Data_t *) BuffersHandle)->NumberOfExtraSecondaryFrameBuffersRequested != 0)
        {
            /* Reserve the specified number of SD frame buffers */
            if(GetNbrOfFrameBufferAllocatedInAPool(BuffersHandle, ((VIDBUFF_Data_t *) BuffersHandle)->AllocatedExtraSecondaryFrameBuffersLoop_p) !=
                ((VIDBUFF_Data_t *) BuffersHandle)->NumberOfExtraSecondaryFrameBuffersRequested)
            {
                VIDBUFF_SetNbrOfExtraSecondaryFrameBuffers(BuffersHandle, ((VIDBUFF_Data_t *) BuffersHandle)->NumberOfExtraSecondaryFrameBuffersRequested);
            }
        }
#endif /* USE_EXTRA_FRAME_BUFFERS */

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Allow to send IMPOSSIBLE_WITH_PROFILE_EVT after re-allocation. */
        ((VIDBUFF_Data_t *) BuffersHandle)->IsNotificationOfImpossibleWithProfileAllowed = TRUE;
    }

#ifdef TRACE_BUFFER
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "After Allocating :"));
    PrintFrameBuffers(BuffersHandle);
#endif

    return(ErrorCode);

} /* End of VIDBUFF_AllocateMemoryForProfile() function */


/*******************************************************************************
Name        : VIDBUFF_DeAllocateBitBuffer
Description :
Parameters  : Video buffer manager handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if problem
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_DeAllocateBitBuffer(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_BufferAllocationParams_t * const BufferParams_p)
{
    ST_ErrorCode_t ErrorCode;

    if ((BufferParams_p == NULL) || (BufferParams_p->TotalSize == 0))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    ErrorCode = HALBUFF_DeAllocateBitBuffer(((VIDBUFF_Data_t *) BuffersHandle)->HALBuffersHandle, BufferParams_p);

    if (ErrorCode == ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Bit buffer de-allocated from address 0x%x", BufferParams_p->Address_p));
    }

    return(ErrorCode);
} /* End of VIDBUFF_DeAllocateBitBuffer() function */


/*******************************************************************************
Name        : VIDBUFF_DeAllocateUnusedMemoryOfProfile
Description : Deallocation of allocated frame buffers used for the decode/display(/D1)
              process.
Parameters  : Video buffer manager handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if deallocation done successfully.
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_DeAllocateUnusedMemoryOfProfile(const VIDBUFF_Handle_t BuffersHandle)
{
    VIDBUFF_FrameBuffer_t * CurrentFrame_p;
    VIDBUFF_FrameBuffer_t * PreviousFrame_p;
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
#ifdef ST_OSLINUX
    FrameBuffers_t            * FrameBuffers_p = NULL;
    U32                         NbFrameBuffers = 0;
#endif

    if (VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p == NULL) /* Nothing to de-allocate ! */
    {
        return(ST_NO_ERROR);
    }

    TrMain(("\r\nVIDBUFF_DeAllocateUnusedMemoryOfProfile"));

#ifdef TRACE_BUFFER
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Before deAllocating :"));
    PrintFrameBuffers(BuffersHandle);
#endif

#ifdef ST_OSLINUX
    /* Allocates max possible FB entries (primary + secondary) */
    FrameBuffers_p = (FrameBuffers_t *)STOS_MemoryAllocate(NULL, (VIDBUFF_Data_p->MaxFrameBuffers + VIDBUFF_Data_p->MaxSecondaryFrameBuffers)*sizeof(FrameBuffers_t));
#endif

    /* Lock access to allocated frame buffers loop */
    semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    /* primary pool */
    /*--------------*/
    PreviousFrame_p = VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p;
    CurrentFrame_p = PreviousFrame_p->NextAllocated_p;

    while ((CurrentFrame_p != VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p) && (CurrentFrame_p != NULL))
    {
        if (!(HasAPictureLocked(*CurrentFrame_p)))
        {
            /* Remove it from the Pool */
            PreviousFrame_p->NextAllocated_p = CurrentFrame_p->NextAllocated_p;
            VIDBUFF_Data_p->NumberOfAllocatedFrameBuffers --;

#ifdef ST_OSLINUX
            /* Store this FB has being removed */
            FrameBuffers_p[NbFrameBuffers].KernelAddress_p = CurrentFrame_p->Allocation.Address_p;
            FrameBuffers_p[NbFrameBuffers].Size = CurrentFrame_p->Allocation.TotalSize;
            NbFrameBuffers++;
#endif
            /* Deallocate the Frame Buffer*/
            DeAllocateFrameBuffer(BuffersHandle, CurrentFrame_p);
        }
        else
        {
            /* Change picture's DecodingOrderFrameID to avoid to match fields not properly in buffer manager as this picture buffer is no longer used but may still be locked as last field on display */
            if (CurrentFrame_p->FrameOrFirstFieldPicture_p != NULL)
            {
                CurrentFrame_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID = MAX_DECODING_ORDER_FRAME_ID;
            }
            if (CurrentFrame_p->NothingOrSecondFieldPicture_p != NULL)
            {
                CurrentFrame_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID = MAX_DECODING_ORDER_FRAME_ID;
            }
            /* Go to next one... */
            PreviousFrame_p = CurrentFrame_p;
        }
        CurrentFrame_p = PreviousFrame_p->NextAllocated_p;
    }

    if (CurrentFrame_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unexpected CurrentFrame_p == NULL"));
    }
    else
    {
        /* Here CurrentFrame_p == VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p: check if it is to be removed */
        if (!(HasAPictureLocked(*CurrentFrame_p)))
        {
            /* No picture locked */
            if (PreviousFrame_p == CurrentFrame_p)
            {
                /* Picture is alone in loop */
                VIDBUFF_Data_p->NumberOfAllocatedFrameBuffers = 0;
                VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p = NULL;
            }
            else
            {
                /* Picture is not alone in loop: remove it from the queue */
                PreviousFrame_p->NextAllocated_p = CurrentFrame_p->NextAllocated_p;
                VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p = PreviousFrame_p;
                VIDBUFF_Data_p->NumberOfAllocatedFrameBuffers --;
            }

#ifdef ST_OSLINUX
            /* Store this FB has being removed */
            FrameBuffers_p[NbFrameBuffers].KernelAddress_p = CurrentFrame_p->Allocation.Address_p;
            FrameBuffers_p[NbFrameBuffers].Size = CurrentFrame_p->Allocation.TotalSize;
            NbFrameBuffers++;
#endif

            /* Deallocate the Frame Buffer*/
            DeAllocateFrameBuffer(BuffersHandle, CurrentFrame_p);
        }
        else
        {
            /* Change picture's DecodingOrderFrameID to avoid to match fields not properly in buffer manager as this picture buffer is no longer used but may still be locked as last field on display */
            if (CurrentFrame_p->FrameOrFirstFieldPicture_p != NULL)
            {
                CurrentFrame_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID = MAX_DECODING_ORDER_FRAME_ID;
            }
            if (CurrentFrame_p->NothingOrSecondFieldPicture_p != NULL)
            {
                CurrentFrame_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID = MAX_DECODING_ORDER_FRAME_ID;
            }
        }
    }

    /* And now, the secondary pool... */
    /*--------------------------------*/
    if (VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p == NULL)
    {
        /* No Secondary Frame Buffer: Go to extra frame buffers */
        goto check_extra_pool;
    }

    /* Some Secondary Frame Buffer are present*/
    PreviousFrame_p = VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p;
    CurrentFrame_p = PreviousFrame_p->NextAllocated_p;

    while ((CurrentFrame_p != VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p) && (CurrentFrame_p != NULL))
    {
        if (!(HasAPictureLocked(*CurrentFrame_p)))
        {
            /* No picture locked */

            /* Remove it from the Pool */
            PreviousFrame_p->NextAllocated_p = CurrentFrame_p->NextAllocated_p;
            VIDBUFF_Data_p->NumberOfSecondaryAllocatedFrameBuffers --;

#ifdef ST_OSLINUX
            /* Store this FB has being removed */
            FrameBuffers_p[NbFrameBuffers].KernelAddress_p = CurrentFrame_p->Allocation.Address_p;
            FrameBuffers_p[NbFrameBuffers].Size = CurrentFrame_p->Allocation.TotalSize;
            NbFrameBuffers++;
#endif

            /* Deallocate the Frame Buffer*/
            DeAllocateFrameBuffer(BuffersHandle, CurrentFrame_p);
        }
        else
        {
            /* Change picture's DecodingOrderFrameID to avoid to match fields not properly in buffer manager as this picture buffer is no longer used but may still be locked as last field on display */
            if (CurrentFrame_p->FrameOrFirstFieldPicture_p != NULL)
            {
                CurrentFrame_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID = MAX_DECODING_ORDER_FRAME_ID;
            }
            if (CurrentFrame_p->NothingOrSecondFieldPicture_p != NULL)
            {
                CurrentFrame_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID = MAX_DECODING_ORDER_FRAME_ID;
            }
            /* Go to next one... */
            PreviousFrame_p = CurrentFrame_p;
        }
        CurrentFrame_p = PreviousFrame_p->NextAllocated_p;
    }

    if (CurrentFrame_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unexpected CurrentFrame_p == NULL"));
    }
    else
    {
        /* Here CurrentFrame_p == VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p: check if it is to be removed */
        if (!(HasAPictureLocked(*CurrentFrame_p)))
        {
            /* No picture locked */
            if (PreviousFrame_p == CurrentFrame_p)
            {
                /* Picture is alone in loop */
                VIDBUFF_Data_p->NumberOfSecondaryAllocatedFrameBuffers= 0;
                VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p= NULL;
            }
            else
            {
                /* Picture is not alone in loop */
                PreviousFrame_p->NextAllocated_p = CurrentFrame_p->NextAllocated_p;
                VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p= PreviousFrame_p;
                VIDBUFF_Data_p->NumberOfSecondaryAllocatedFrameBuffers--;
            }

#ifdef ST_OSLINUX
            /* Store this FB has being removed */
            FrameBuffers_p[NbFrameBuffers].KernelAddress_p = CurrentFrame_p->Allocation.Address_p;
            FrameBuffers_p[NbFrameBuffers].Size = CurrentFrame_p->Allocation.TotalSize;
            NbFrameBuffers++;
#endif

            /* Deallocate the Frame Buffer*/
            DeAllocateFrameBuffer(BuffersHandle, CurrentFrame_p);
        }
        else
        {
            /* Change picture's DecodingOrderFrameID to avoid to match fields not properly in buffer manager as this picture buffer is no longer used but may still be locked as last field on display */
            if (CurrentFrame_p->FrameOrFirstFieldPicture_p != NULL)
            {
                CurrentFrame_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID = MAX_DECODING_ORDER_FRAME_ID;
            }
            if (CurrentFrame_p->NothingOrSecondFieldPicture_p != NULL)
            {
                CurrentFrame_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID = MAX_DECODING_ORDER_FRAME_ID;
            }
        }
    }

check_extra_pool:

#ifdef USE_EXTRA_FRAME_BUFFERS
    /* And now, the extra pool... */
    /*--------------------------------*/
    if (VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p == NULL)
    {
        /* No Extra Frame Buffer: Go to end */
        goto no_more_checking;
    }

    /* Some Extra Frame Buffers are present*/
    PreviousFrame_p = VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p;
    CurrentFrame_p = PreviousFrame_p->NextAllocated_p;

    while ((CurrentFrame_p != VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p) && (CurrentFrame_p != NULL))
    {
        if (!(HasAPictureLocked(*CurrentFrame_p)))
        {
            /* Remove it from the Pool */
            PreviousFrame_p->NextAllocated_p = CurrentFrame_p->NextAllocated_p;
            VIDBUFF_Data_p->NumberOfAllocatedExtraPrimaryFrameBuffers --;

#ifdef ST_OSLINUX
            /* Store this FB has being removed */
            FrameBuffers_p[NbFrameBuffers].KernelAddress_p = CurrentFrame_p->Allocation.Address_p;
            FrameBuffers_p[NbFrameBuffers].Size = CurrentFrame_p->Allocation.TotalSize;
            NbFrameBuffers++;
#endif
            /* Deallocate the Frame Buffer*/
            DeAllocateFrameBuffer(BuffersHandle, CurrentFrame_p);
        }
        else
        {
            /* Change picture's DecodingOrderFrameID to avoid to match fields not properly in buffer manager as this picture buffer is no longer used but may still be locked as last field on display */
            if (CurrentFrame_p->FrameOrFirstFieldPicture_p != NULL)
            {
                CurrentFrame_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID = MAX_DECODING_ORDER_FRAME_ID;
            }
            if (CurrentFrame_p->NothingOrSecondFieldPicture_p != NULL)
            {
                CurrentFrame_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID = MAX_DECODING_ORDER_FRAME_ID;
            }
            /* Go to next one... */
            PreviousFrame_p = CurrentFrame_p;
        }
        CurrentFrame_p = PreviousFrame_p->NextAllocated_p;
    }

    if (CurrentFrame_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unexpected CurrentFrame_p == NULL"));
    }
    else
    {
        /* Here CurrentFrame_p == VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p: check if it is to be removed */
        if (!(HasAPictureLocked(*CurrentFrame_p)))
        {
            /* No picture locked */
            if (PreviousFrame_p == CurrentFrame_p)
            {
                /* Picture is alone in loop */
                VIDBUFF_Data_p->NumberOfAllocatedExtraPrimaryFrameBuffers = 0;
                VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p = NULL;
            }
            else
            {
                /* Picture is not alone in loop: remove it from the queue */
                PreviousFrame_p->NextAllocated_p = CurrentFrame_p->NextAllocated_p;
                VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p = PreviousFrame_p;
                VIDBUFF_Data_p->NumberOfAllocatedExtraPrimaryFrameBuffers --;
            }

#ifdef ST_OSLINUX
            /* Store this FB has being removed */
            FrameBuffers_p[NbFrameBuffers].KernelAddress_p = CurrentFrame_p->Allocation.Address_p;
            FrameBuffers_p[NbFrameBuffers].Size = CurrentFrame_p->Allocation.TotalSize;
            NbFrameBuffers++;
#endif

            /* Deallocate the Frame Buffer*/
            DeAllocateFrameBuffer(BuffersHandle, CurrentFrame_p);
        }
        else
        {
            /* Change picture's DecodingOrderFrameID to avoid to match fields not properly in buffer manager as this picture buffer is no longer used but may still be locked as last field on display */
            if (CurrentFrame_p->FrameOrFirstFieldPicture_p != NULL)
            {
                CurrentFrame_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID = MAX_DECODING_ORDER_FRAME_ID;
            }
            if (CurrentFrame_p->NothingOrSecondFieldPicture_p != NULL)
            {
                CurrentFrame_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID = MAX_DECODING_ORDER_FRAME_ID;
            }
        }
    }

    /* And now, the extra secondary pool... */
    /*--------------------------------*/
    if (VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p == NULL)
    {
        /* No Extra Frame Buffer: Go to end */
        goto no_more_checking;
    }

    /* Some Extra Secondary Frame Buffers are present*/
    PreviousFrame_p = VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p;
    CurrentFrame_p = PreviousFrame_p->NextAllocated_p;

    while ((CurrentFrame_p != VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p) && (CurrentFrame_p != NULL))
    {
        if (!(HasAPictureLocked(*CurrentFrame_p)))
        {
            /* Remove it from the Pool */
            PreviousFrame_p->NextAllocated_p = CurrentFrame_p->NextAllocated_p;
            VIDBUFF_Data_p->NumberOfAllocatedExtraSecondaryFrameBuffers --;

#ifdef ST_OSLINUX
            /* Store this FB has being removed */
            FrameBuffers_p[NbFrameBuffers].KernelAddress_p = CurrentFrame_p->Allocation.Address_p;
            FrameBuffers_p[NbFrameBuffers].Size = CurrentFrame_p->Allocation.TotalSize;
            NbFrameBuffers++;
#endif
            /* Deallocate the Frame Buffer*/
            DeAllocateFrameBuffer(BuffersHandle, CurrentFrame_p);
        }
        else
        {
            /* Change picture's DecodingOrderFrameID to avoid to match fields not properly in buffer manager as this picture buffer is no longer used but may still be locked as last field on display */
            if (CurrentFrame_p->FrameOrFirstFieldPicture_p != NULL)
            {
                CurrentFrame_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID = MAX_DECODING_ORDER_FRAME_ID;
            }
            if (CurrentFrame_p->NothingOrSecondFieldPicture_p != NULL)
            {
                CurrentFrame_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID = MAX_DECODING_ORDER_FRAME_ID;
            }
            /* Go to next one... */
            PreviousFrame_p = CurrentFrame_p;
        }
        CurrentFrame_p = PreviousFrame_p->NextAllocated_p;
    }

    if (CurrentFrame_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unexpected CurrentFrame_p == NULL"));
    }
    else
    {
        /* Here CurrentFrame_p == VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p: check if it is to be removed */
        if (!(HasAPictureLocked(*CurrentFrame_p)))
        {
            /* No picture locked */
            if (PreviousFrame_p == CurrentFrame_p)
            {
                /* Picture is alone in loop */
                VIDBUFF_Data_p->NumberOfAllocatedExtraSecondaryFrameBuffers = 0;
                VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p = NULL;
            }
            else
            {
                /* Picture is not alone in loop: remove it from the queue */
                PreviousFrame_p->NextAllocated_p = CurrentFrame_p->NextAllocated_p;
                VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p = PreviousFrame_p;
                VIDBUFF_Data_p->NumberOfAllocatedExtraSecondaryFrameBuffers --;
            }

#ifdef ST_OSLINUX
            /* Store this FB has being removed */
            FrameBuffers_p[NbFrameBuffers].KernelAddress_p = CurrentFrame_p->Allocation.Address_p;
            FrameBuffers_p[NbFrameBuffers].Size = CurrentFrame_p->Allocation.TotalSize;
            NbFrameBuffers++;
#endif

            /* Deallocate the Frame Buffer*/
            DeAllocateFrameBuffer(BuffersHandle, CurrentFrame_p);
        }
        else
        {
            /* Change picture's DecodingOrderFrameID to avoid to match fields not properly in buffer manager as this picture buffer is no longer used but may still be locked as last field on display */
            if (CurrentFrame_p->FrameOrFirstFieldPicture_p != NULL)
            {
                CurrentFrame_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID = MAX_DECODING_ORDER_FRAME_ID;
            }
            if (CurrentFrame_p->NothingOrSecondFieldPicture_p != NULL)
            {
                CurrentFrame_p->NothingOrSecondFieldPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID = MAX_DECODING_ORDER_FRAME_ID;
            }
        }
    }

no_more_checking:
    /* Every Pools have been checked */

    ((VIDBUFF_Data_t *) BuffersHandle)->IsMemoryAllocatedForProfile = FALSE;
#endif /* USE_EXTRA_FRAME_BUFFERS */

#ifdef ST_OSLINUX
    /* Notify removed FB */
    NotifyRemovedFrameBuffers(BuffersHandle, FrameBuffers_p, NbFrameBuffers);
    if (FrameBuffers_p != NULL)
    {
        STOS_MemoryDeallocate(NULL, FrameBuffers_p);
    }
#endif

    /* Un-lock access to allocated frame buffers loop */
    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

#ifdef TRACE_BUFFER
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "After deAllocating :"));
    PrintFrameBuffers(BuffersHandle);
#endif

#ifdef SHOW_FRAME_BUFFER_USED
    TrError(("\r\nAfter deallocatingUnusedMemoryOfProfile:"));
    VIDBUFF_PrintPictureBuffersStatus(BuffersHandle);
#endif

    return(ST_NO_ERROR);
} /* End of VIDBUFF_DeAllocateUnusedMemoryOfProfile() function */

#ifdef STVID_DEBUG_TAKE_RELEASE
/*******************************************************************************
Name        : VIDBUFF_PictureBufferShouldBeUnused
Description : This function check that a PictureLockCounter is not used
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void VIDBUFF_PictureBufferShouldBeUnused(const VIDBUFF_Handle_t BuffersHandle,
                VIDBUFF_PictureBuffer_t * const Picture_p)
{
    UNUSED_PARAMETER(BuffersHandle);

    if ( (Picture_p->Buffers.IsInDisplayQueue) ||
         (Picture_p->Buffers.PictureLockCounter != 0) ||
         (Picture_p->Buffers.DisplayStatus != VIDBUFF_NOT_ON_DISPLAY) )
    {
        TrError(("\r\nError! Pict Buff 0x%x still in use", Picture_p));
    }
}

/*******************************************************************************
Name        : VIDBUFF_CheckPictureBuffer
Description : This function check that the PictureLockCounter is not Null and that the Frame Buffer
              pointer is not Null neither.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_CheckPictureBuffer(const VIDBUFF_Handle_t BuffersHandle,
                VIDBUFF_PictureBuffer_t * const Picture_p)
{
    UNUSED_PARAMETER(BuffersHandle);

    if ( (Picture_p == NULL)                            ||
         (Picture_p->Buffers.PictureLockCounter == 0)   ||
         (Picture_p->FrameBuffer_p == NULL)             ||
         (Picture_p->Buffers.IsInUse == FALSE) )
    {
        TrError(("\r\nError! Invalid Pict Buf 0x%x!!! (%d %d)",
                    Picture_p,
                    Picture_p->Buffers.PictureLockCounter,
                    Picture_p->Buffers.IsInUse));
    }

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : VIDBUFF_DbgTakePictureBuffer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_DbgTakePictureBuffer(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const Picture_p, VIDCOM_MODULE_ID_t ModId, const char *FileName, U32 Line)
{
    ST_ErrorCode_t ErrorCode;
    UNUSED_PARAMETER(Line);
    UNUSED_PARAMETER(ModId);

    /* Trunc the filename to the last 10 characters */
    if (strlen(FileName) > FILENAME_MAX_LENGTH)
    {
        FileName += (strlen(FileName) - FILENAME_MAX_LENGTH);
    }

    TrTakeRelease(("\r\n%s take Pict 0x%x %d", FileName, Picture_p, Picture_p->Buffers.PictureLockCounter+1));
    ErrorCode = VIDBUFF_TakePictureBuffer(BuffersHandle, Picture_p, ModId);

    return ErrorCode;
}

/*******************************************************************************
Name        : VIDBUFF_DbgReleasePictureBuffer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_DbgReleasePictureBuffer(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const Picture_p, VIDCOM_MODULE_ID_t ModId, const char *FileName, U32 Line, const BOOL IsNotificationNeeded)
{
    ST_ErrorCode_t ErrorCode;
    UNUSED_PARAMETER(Line);
    UNUSED_PARAMETER(ModId);

    /* Check Picture Buffer validity */
    VIDBUFF_CheckPictureBuffer(BuffersHandle, Picture_p);

    /* Trunc the filename to the last 10 characters */
    if (strlen(FileName) > FILENAME_MAX_LENGTH)
    {
        FileName += (strlen(FileName) - FILENAME_MAX_LENGTH);
    }

    /* Rem: PictureLockCounter is not yet decremented but we can not put this code after VIDBUFF_ReleasePictureBuffer because the Picture Buffer may be freed */
    if (IsNotificationNeeded)
    {
        TrTakeRelease(("\r\n%s release Pict 0x%x %d", FileName, Picture_p, Picture_p->Buffers.PictureLockCounter-1));
    }
    else
    {
        TrTakeRelease(("\r\n%s UnTake Pict 0x%x %d", FileName, Picture_p, Picture_p->Buffers.PictureLockCounter-1));
    }

    ErrorCode = VIDBUFF_ReleasePictureBuffer(BuffersHandle, Picture_p, ModId, IsNotificationNeeded);

    return ErrorCode;
}
#endif /* STVID_DEBUG_TAKE_RELEASE */

/*******************************************************************************
Name        : VIDBUFF_ReleasePictureBuffer
Description : Release the picture buffer
Parameters  :
Assumptions :
Limitations : Caution, when called with IsNotificationNeeded==FALSE this function
              will not be suitable if we plan in a near future to share one buffer
              manager between several producer instances.
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_ReleasePictureBuffer(const VIDBUFF_Handle_t      BuffersHandle,
                                            VIDBUFF_PictureBuffer_t *   const Picture_p,
                                            VIDCOM_MODULE_ID_t          ModId,
                                            const BOOL                  IsNotificationNeeded)
{
    BOOL                        NewPictureBufferAvailable = FALSE;
    VIDBUFF_FrameBuffer_t   *   CurrentFrame_p;
    VIDBUFF_Data_t *            VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;

    if (Picture_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (ModId >= VIDCOM_MAX_MODULE_NBR)
    {
        TrError(("\r\nError! Invalid Mod %d!", ModId));
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Lock semaphore protecting access to the Lock counters */
    STOS_SemaphoreWait(Picture_p->Buffers.LockCounterSemaphore_p);

    /* Check if this module is allowed to release this Picture Buffer */
    if (Picture_p->Buffers.Module[ModId].TakeReleaseCounter ==  0)
    {
        /* This module is not allowed to release this picture buffer because he didn't take it before */

        /* Release the semaphore protecting access to the Lock counters */
        STOS_SemaphoreSignal(Picture_p->Buffers.LockCounterSemaphore_p);

        TrError(("\r\nError! Pict Buff 0x%x FB 0x%x PPB@ 0x%x released too many times by Mod %d!", Picture_p, Picture_p->FrameBuffer_p, Picture_p->PPB.Address_p, ModId));

        return ST_ERROR_BAD_PARAMETER;
    }

    /* Here we know that this module is allowed to release this picture Buffer so we can decrement the counters: */

    /* Decrement the module counter */
    Picture_p->Buffers.Module[ModId].TakeReleaseCounter--;
    /* Decrement the global counter */
    Picture_p->Buffers.PictureLockCounter--;

    if (Picture_p->Buffers.PictureLockCounter ==  0)
    {
        NewPictureBufferAvailable = TRUE;
        if (Picture_p->AssociatedDecimatedPicture_p != NULL)
        {
            Picture_p->AssociatedDecimatedPicture_p->AssociatedDecimatedPicture_p = NULL;
            Picture_p->AssociatedDecimatedPicture_p = NULL;
        }
        /* Try to deallocate PPB if exists */
        if ((Picture_p->PPB.Address_p != NULL) && (Picture_p->PPB.Size > 0))
        {
            TrPPB(("\r\nNormal attempt to free PPB @ 0x%x for 0x%x FB 0x%x", Picture_p->PPB.Address_p, Picture_p, Picture_p->FrameBuffer_p));
            vidbuff_FreeAdditionnalData(BuffersHandle, Picture_p, FALSE);
        }
    }

    /* Release the semaphore protecting access to the Lock counters */
    STOS_SemaphoreSignal(Picture_p->Buffers.LockCounterSemaphore_p);

    TrTakeRelease(("\r\nMod %d releases Pict 0x%x (%d)", ModId, Picture_p, Picture_p->Buffers.PictureLockCounter));

    if (NewPictureBufferAvailable)
    {
        if (IsNotificationNeeded)
        {
            /* Notify that a Picture Buffer is now available */
            STEVT_Notify(VIDBUFF_Data_p->EventsHandle,
                         VIDBUFF_Data_p->RegisteredEventsID[VIDBUFF_NEW_PICTURE_BUFFER_AVAILABLE_EVT_ID],
                         STEVT_EVENT_DATA_TYPE_CAST Picture_p);
        }

        if (Picture_p->Buffers.IsLinkToFrameBufferToBeRemoved)
        {
            /* The flag "IsLinkToFrameBufferToBeRemoved" indicates that this picture buffer has been overwritten by a new picture buffer.
                It should be freed now that it is not used anymore by the display ("LeavePicture" will remove the link to the frame buffer) */
            TrOverwrite(("\r\nLink removed on Pict Buff 0x%x", Picture_p));
            LeavePicture(BuffersHandle, Picture_p);
        }
        else
        {
            /* Retrieve the frame buffer corresponding to the picture to remove. */
            CurrentFrame_p = Picture_p->FrameBuffer_p;

            if ( (CurrentFrame_p != NULL) && (CurrentFrame_p->ToBeKilledAsSoonAsPossible) )
            {
                /* Lock access to allocated frame buffers loop */
                semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

                /* Check and remove eventual picture of previous profile. */
                CheckIfToBeKilledAsSoonAsPossibleAndRemove(BuffersHandle, Picture_p);

                /* Un-lock access to allocated frame buffers loop */
                semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);
            }
        }
    }

    return(ST_NO_ERROR);
} /* end of VIDBUFF_ReleasePictureBuffer() function */

/*******************************************************************************
Name        : VIDBUFF_TakePictureBuffer
Description : Locks the picture buffer
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_TakePictureBuffer(const VIDBUFF_Handle_t     BuffersHandle,
                                         VIDBUFF_PictureBuffer_t *  const Picture_p,
                                         VIDCOM_MODULE_ID_t         ModId)
{
    UNUSED_PARAMETER(BuffersHandle);

    if (Picture_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (ModId >= VIDCOM_MAX_MODULE_NBR)
    {
        TrError(("\r\nError! Invalid Mod %d!", ModId));
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Lock semaphore protecting access to the Lock counters */
    STOS_SemaphoreWait(Picture_p->Buffers.LockCounterSemaphore_p);

     /* Increment the module counter */
    Picture_p->Buffers.Module[ModId].TakeReleaseCounter++;

    /* Increment the global counter */
    Picture_p->Buffers.PictureLockCounter++;

    /* Release the semaphore protecting access to the Lock counters */
    STOS_SemaphoreSignal(Picture_p->Buffers.LockCounterSemaphore_p);

    TrTakeRelease(("\r\nMod %d takes Pict 0x%x (%d)", ModId, Picture_p, Picture_p->Buffers.PictureLockCounter));

    return(ST_NO_ERROR);
} /* end of VIDBUFF_TakePictureBuffer() function */

/*******************************************************************************
Name        : VIDBUFF_RecoverFrameBuffers
Description : This functions recovers the frame buffers that may not have been released.
              It should be called when the video is stopped. Every modules should have released the
              frame buffers used. The only one allowed to keep frame buffers locked is the display because
              a picture might still be on screen.
Parameters  :
Assumptions : This function should only be called when the video is STOPPED.
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_RecoverFrameBuffers(const VIDBUFF_Handle_t BuffersHandle)
{
    VIDBUFF_Data_t *     VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;

    /* Lock access to allocated frame buffers loop */
    semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    TrMain(("\r\nVIDBUFF_RecoverFrameBuffers"));

    /* Recover Frame Buffers associated to each buffer loop */
    VIDBUFF_CheckAndUnlockFrameBuffers(BuffersHandle, VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p);
    VIDBUFF_CheckAndUnlockFrameBuffers(BuffersHandle, VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p);
#ifdef USE_EXTRA_FRAME_BUFFERS
    VIDBUFF_CheckAndUnlockFrameBuffers(BuffersHandle, VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p);
    VIDBUFF_CheckAndUnlockFrameBuffers(BuffersHandle, VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p);
#endif

    /* Un-lock access to allocated frame buffers loop */
    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : VIDBUFF_CheckAndUnlockFrameBuffers
Description : This functions performs the frame buffer recovery for a given frame buffer loop.
              Every frame buffers that are locked but not in the Display queue are considered
              as erroneous so they will be released.
Parameters  :
Assumptions : This function should only be called when the video is STOPPED.
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t VIDBUFF_CheckAndUnlockFrameBuffers(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_FrameBuffer_t * FrameBuffer_p)
{
    /*VIDBUFF_Data_t *     VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;*/
    VIDBUFF_FrameBuffer_t * CurrentFrame_p;

    if (FrameBuffer_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    CurrentFrame_p = FrameBuffer_p->NextAllocated_p;

    /* Loop until it comes back to the begining of the loop or the pointer becomes NULL*/
    while ( (CurrentFrame_p != FrameBuffer_p) && (CurrentFrame_p != NULL) )
    {
        VIDBUFF_CheckAndUnlockPictureBuffer(BuffersHandle, CurrentFrame_p->FrameOrFirstFieldPicture_p);
        VIDBUFF_CheckAndUnlockPictureBuffer(BuffersHandle, CurrentFrame_p->NothingOrSecondFieldPicture_p);

        /* Move to next frame buffer */
        CurrentFrame_p = CurrentFrame_p->NextAllocated_p;
    }

    /* Manage the case of the 1st element of the loop */
    if (CurrentFrame_p == FrameBuffer_p)
    {
        VIDBUFF_CheckAndUnlockPictureBuffer(BuffersHandle, CurrentFrame_p->FrameOrFirstFieldPicture_p);
        VIDBUFF_CheckAndUnlockPictureBuffer(BuffersHandle, CurrentFrame_p->NothingOrSecondFieldPicture_p);
    }

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : VIDBUFF_CheckAndUnlockPictureBuffer
Description : This function check that only the Display Queue or the application still lock this picture buffer.
              Other locks are discarded.
Parameters  :
Assumptions : This function should only be called when the video is STOPPED.
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t VIDBUFF_CheckAndUnlockPictureBuffer(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * PictureBuffer_p)
{
    U32     ModId;
    UNUSED_PARAMETER(BuffersHandle);

    if (PictureBuffer_p == NULL)
    {
        /* Nothing to do */
        return(ST_NO_ERROR);
    }

    /* Lock semaphore protecting access to the Lock counters */
    STOS_SemaphoreWait(PictureBuffer_p->Buffers.LockCounterSemaphore_p);

    for(ModId=0; ModId<VIDCOM_MAX_MODULE_NBR; ModId++)
    {
        /* After a stop, only the following modules can keep some picture buffers locked:
           - The Display Queue
           - The application (which can be an external display)
           - The DEI (which can still lock the current field on display) */
        if ( (ModId != VIDCOM_VIDQUEUE_MODULE_BASE)     &&
             (ModId != VIDCOM_APPLICATION_MODULE_BASE)  &&
             (ModId != VIDCOM_DEI_MODULE_BASE) )
        {
            if (PictureBuffer_p->Buffers.Module[ModId].TakeReleaseCounter > 0)
            {
                /* This Picture buffer is still locked by a module not allowed to do so */
                TrError(("\n\rRecover Pict Buff 0x%x FB 0x%x still locked by Mod %d", PictureBuffer_p, PictureBuffer_p->FrameBuffer_p, ModId));
                PictureBuffer_p->Buffers.PictureLockCounter -= PictureBuffer_p->Buffers.Module[ModId].TakeReleaseCounter;
                PictureBuffer_p->Buffers.Module[ModId].TakeReleaseCounter = 0;
            }
        }
    }

    /* Try to deallocate PPB if exists */
    if ((PictureBuffer_p->PPB.Address_p != NULL) && (PictureBuffer_p->PPB.Size > 0))
    {
        TrPPB(("\r\nTry to free PPB @ 0x%x for Recovered Pict Buff 0x%x FB 0x%x", PictureBuffer_p->PPB.Address_p, PictureBuffer_p, PictureBuffer_p->FrameBuffer_p));
        /* Force to free PPB as we are currently recovering locked picture buffers after the stop */
        vidbuff_FreeAdditionnalData(BuffersHandle, PictureBuffer_p, TRUE);
    }

    /* Release the semaphore protecting access to the Lock counters */
    STOS_SemaphoreSignal(PictureBuffer_p->Buffers.LockCounterSemaphore_p);

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : VIDBUFF_GetAdditionnalDataBuffer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_GetAdditionnalDataBuffer(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const PictureBuffer_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (PictureBuffer_p == NULL)
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        ErrorCode = HALBUFF_GetAdditionnalDataBuffer(((VIDBUFF_Data_t *) BuffersHandle)->HALBuffersHandle, PictureBuffer_p);

        if (ErrorCode == ST_NO_ERROR)
        {
            /*STTBX_Report((STTBX_REPORT_LEVEL_INFO, "PPB allocated at address 0x%x size %d", PictureBuffer_p->PPB.Address_p, PictureBuffer_p->PPB.Size));*/
            TrPPB(("\r\nPPB @ 0x%x (%d) for 0x%x %c", PictureBuffer_p->PPB.Address_p, PictureBuffer_p->PPB.Size, PictureBuffer_p, MPEGFrame2Char(PictureBuffer_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame)));
        }
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Failed to allocated PPB for picture 0x%x FB 0x%x", PictureBuffer_p, PictureBuffer_p->FrameBuffer_p));
            TrPPB(("\r\nFailed alloc PPB for 0x%x FB 0x%x", PictureBuffer_p, PictureBuffer_p->FrameBuffer_p));
        }
    }

    return(ErrorCode);
} /* end of VIDBUFF_GetAdditionnalDataBuffer() function */

/*******************************************************************************
Name        : VIDBUFF_FreeAdditionnalDataBuffer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_FreeAdditionnalDataBuffer(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const PictureBuffer_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if ((PictureBuffer_p->PPB.Address_p == NULL) || (PictureBuffer_p->PPB.Size == 0))
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        TrPPB(("\r\nAlready Freed PPB for 0x%x FB 0x%x", PictureBuffer_p, PictureBuffer_p->FrameBuffer_p));
    }
    else
    {
        ErrorCode = HALBUFF_FreeAdditionnalDataBuffer(((VIDBUFF_Data_t *) BuffersHandle)->HALBuffersHandle, PictureBuffer_p);
        if (ErrorCode == ST_NO_ERROR)
        {
            /*STTBX_Report((STTBX_REPORT_LEVEL_INFO, "PPB de-allocated from address 0x%x", PictureBuffer_p->PPB.Address_p));*/
            TrPPB(("\r\nFree PPB @ 0x%x for 0x%x FB 0x%x", PictureBuffer_p->PPB.Address_p, PictureBuffer_p, PictureBuffer_p->FrameBuffer_p));
        }
    }

    return(ErrorCode);
} /* end of VIDBUFF_FreeAdditionnalDataBuffer() function */

/*******************************************************************************
Name        : VIDBUFF_GetMemoryProfile
Description : Get memory profile
Parameters  : Video buffer manager handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if NULL pointer
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_GetMemoryProfile(const VIDBUFF_Handle_t BuffersHandle, VIDCOM_InternalProfile_t * const Profile_p)
{
    if (Profile_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Lock access to allocated frame buffers loop */
    semaphore_wait(((VIDBUFF_Data_t *) BuffersHandle)->AllocatedFrameBuffersLoopLockingSemaphore_p);

    *Profile_p = ((VIDBUFF_Data_t *) BuffersHandle)->Profile;

    /* Unlock access to allocated frame buffers loop */
    semaphore_signal(((VIDBUFF_Data_t *) BuffersHandle)->AllocatedFrameBuffersLoopLockingSemaphore_p);

    return(ST_NO_ERROR);
} /* End of VIDBUFF_GetMemoryProfile() function */

/*******************************************************************************
Name        : VIDBUFF_GetMemoryProfileDecimationFactor
Description : Get the decimation factor from the memory profile
Parameters  : Video buffer manager handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if NULL pointer
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_GetMemoryProfileDecimationFactor(const VIDBUFF_Handle_t BuffersHandle, STVID_DecimationFactor_t * const DecimationFactor_p)
{
    if (DecimationFactor_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    *DecimationFactor_p = ((VIDBUFF_Data_t *) BuffersHandle)->Profile.ApiProfile.DecimationFactor;

    return(ST_NO_ERROR);
} /* End of VIDBUFF_GetMemoryProfileDecimationFactor() function */

/*******************************************************************************
Name        : VIDBUFF_GetApplicableDecimationFactor
Description : Get the applicable decimation factor
Parameters  : Video buffer manager handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if NULL pointer
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_GetApplicableDecimationFactor(const VIDBUFF_Handle_t BuffersHandle, STVID_DecimationFactor_t * const DecimationFactor_p)
{
    if (DecimationFactor_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    *DecimationFactor_p = ((VIDBUFF_Data_t *) BuffersHandle)->Profile.ApplicableDecimationFactor;

    return(ST_NO_ERROR);
} /* End of VIDBUFF_GetApplicableDecimationFactor() function */

/*******************************************************************************
Name        : VIDBUFF_GetPictureAllocParams
Description : Get size and alignement required to allocate picture of given size
Parameters  : Video buffer manager handle,
              PictureWidth is the width of the picture in pixels,
              PictureHeight is the height of the picture in pixels,
              pointer to return data
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
              ST_ERROR_BAD_PARAMETER if problem
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_GetPictureAllocParams(const VIDBUFF_Handle_t BuffersHandle, const U32 PictureWidth, const U32 PictureHeight, U32 * const TotalSize_p, U32 * const Alignment_p)
{
    if ((TotalSize_p == NULL) || (Alignment_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    HALBUFF_GetFrameBufferParams(((VIDBUFF_Data_t *) BuffersHandle)->HALBuffersHandle, PictureWidth, PictureHeight, TotalSize_p, Alignment_p);
    return(ST_NO_ERROR);
} /* End of VIDBUFF_GetPictureAllocParams() function */

/*******************************************************************************
Name        : VIDBUFF_GetAndTakeUnusedPictureBuffer
Description : Get a primary picture buffer available for decoding
Parameters  : Video buffer manager handle, parameters
Assumptions :
Limitations : Caution: the picture returned may be the one currently displayed if wanting buffer for a B picture
Returns     : ST_NO_ERROR if allocated, then *PictureBuffer_p_p is not NULL (as UsePicture() never returns NULL)
              ST_ERROR_BAD_PARAMETER if bad parameter (nothing is changed)
              STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE if picture proposed is too big for the current profile (*PictureBuffer_p_p is then NULL)
              ST_ERROR_NO_MEMORY if cannot find a buffer to allocate (*PictureBuffer_p_p is then NULL)
              STVID_ERROR_NOT_AVAILABLE if, for a second field picture, it is impossible
              to allocate it because no corresponding first field picture exist (*PictureBuffer_p_p is then NULL)
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_GetAndTakeUnusedPictureBuffer(
        const VIDBUFF_Handle_t                          BuffersHandle,
        const VIDBUFF_GetUnusedPictureBufferParams_t *  const Params_p,
        VIDBUFF_PictureBuffer_t **                      const PictureBuffer_p_p,
        VIDCOM_MODULE_ID_t                              ModId)
{
    ST_ErrorCode_t          Error;
    VIDBUFF_FrameBuffer_t * BufferPool_p;
    VIDBUFF_Data_t *                VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;

 #ifdef USE_EXTRA_FRAME_BUFFERS
    /* Set returned picture buffer to NULL, so quitting with error will return a NULL picture buffer */
    *PictureBuffer_p_p = NULL;

    /* If Extra SD Frame Buffers are available we try to allocate a Frame Buffer there first
         (we reserve the standard frame buffers in case of HD Frame Buffer request) */
    if (VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p != NULL)
    {
        BufferPool_p = VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p;
        Error = GetUnusedPictureBuffer(BuffersHandle,
                                                                     Params_p,
                                                                     PictureBuffer_p_p,
                                                                     BufferPool_p);
        if (Error == ST_NO_ERROR)
        {
            TrFrameBuffer(("\r\nGetUnusedPictureBuffer => (Extra) 0x%08x", *PictureBuffer_p_p));
        }
    }


    /* If no PictureBuffer as been found in the extra loop we look in the standard loop... */
    if (*PictureBuffer_p_p == NULL)
#endif /* USE_EXTRA_FRAME_BUFFERS */
    {
        BufferPool_p = VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p;
        Error = GetUnusedPictureBuffer(BuffersHandle,
                                       Params_p,
                                       PictureBuffer_p_p,
                                       BufferPool_p);
        if (Error == ST_NO_ERROR)
        {
            TrFrameBuffer(("\r\nGetUnusedPictureBuffer => 0x%08x", *PictureBuffer_p_p));
        }
        else
        {
            TrFrameBuffer(("\r\nNo PB avail"));
        }
    }

    if (Error == ST_NO_ERROR)
    {
        TrTakeRelease(("\r\nGetAndTake Pict 0x%x FB 0x%x", *PictureBuffer_p_p, (*PictureBuffer_p_p)->FrameBuffer_p));
        TrPPB(("\r\nGetAndTake Pict 0x%x FB 0x%x for %c picture (%s)",
                    *PictureBuffer_p_p,
                    (*PictureBuffer_p_p)->FrameBuffer_p,
                    MPEGFrame2Char(Params_p->MPEGFrame),
                    PictureStructure2String(Params_p->PictureStructure) ));

        /* The Picture Buffer request was OK: Save the kind of picture that is going to be stored in this buffer */
        VIDBUFF_Data_p->LastMPEGFrameType = Params_p->MPEGFrame;

        ResetPictureBufferParams(BuffersHandle, (*PictureBuffer_p_p));
        TAKE_PICTURE_BUFFER(BuffersHandle, (*PictureBuffer_p_p), ModId);
    }

    return(Error);
}

/*******************************************************************************
Name        : VIDBUFF_GetAndTakeUnusedDecimatedPictureBuffer
Description : Get a secondary picture buffer available for decoding
Parameters  : Video buffer manager handle, parameters
Assumptions :
Limitations : Caution: the picture returned may be the one currently displayed if wanting buffer for a B picture
Returns     : ST_NO_ERROR if allocated, then *PictureBuffer_p_p is not NULL (as UsePicture() never returns NULL)
              ST_ERROR_BAD_PARAMETER if bad parameter (nothing is changed)
              STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE if picture proposed is too big for the current profile (*PictureBuffer_p_p is then NULL)
              ST_ERROR_NO_MEMORY if cannot find a buffer to allocate (*PictureBuffer_p_p is then NULL)
              STVID_ERROR_NOT_AVAILABLE if, for a second field picture, it is impossible
              to allocate it because no corresponding first field picture exist (*PictureBuffer_p_p is then NULL)
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_GetAndTakeUnusedDecimatedPictureBuffer(
        const VIDBUFF_Handle_t BuffersHandle,
        const VIDBUFF_GetUnusedPictureBufferParams_t * const Params_p,
        VIDBUFF_PictureBuffer_t ** const PictureBuffer_p_p,
        VIDCOM_MODULE_ID_t ModId)
{
    ST_ErrorCode_t          Error;
    VIDBUFF_FrameBuffer_t * BufferPool_p;
    VIDBUFF_Data_t *                VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;

#ifdef USE_EXTRA_FRAME_BUFFERS
    /* Set returned picture buffer to NULL, so quitting with error will return a NULL picture buffer */
    *PictureBuffer_p_p = NULL;

    /* If Extra SD Frame Buffers are available we try to allocate a Frame Buffer there first
       (we reserve the standard frame buffers in case of HD Frame Buffer request) */
    if (VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p != NULL)
    {
        BufferPool_p = VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p;
        Error = GetUnusedPictureBuffer(BuffersHandle,
                                       Params_p,
                                       PictureBuffer_p_p,
                                       BufferPool_p);
        if (Error == ST_NO_ERROR)
        {
            TrFrameBuffer(("\r\nGetUnusedPictureBuffer => (Decim Extra) 0x%08x", *PictureBuffer_p_p));
        }
    }

    /* If no PictureBuffer as been found in the extra loop we look in the standard loop... */
    if (*PictureBuffer_p_p == NULL)
#endif /* USE_EXTRA_FRAME_BUFFERS */
    {
        BufferPool_p = VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p;
        Error = GetUnusedPictureBuffer(BuffersHandle,Params_p,
                                       PictureBuffer_p_p,BufferPool_p);
    }
    if (Error == ST_NO_ERROR)
    {
        ResetPictureBufferParams(BuffersHandle, (*PictureBuffer_p_p));
        TrTakeRelease(("\r\nGetAndTake Decim Pict 0x%x", *PictureBuffer_p_p));
        TAKE_PICTURE_BUFFER(BuffersHandle, (*PictureBuffer_p_p), ModId);

        /* Apply forced decimation factor */
        (*PictureBuffer_p_p)->FrameBuffer_p->DecimationFactor = VIDBUFF_Data_p->Profile.ApplicableDecimationFactor;
    }
    else
    {
        TrDecFrameBuffer(("\r\nNo Decim Pict Buffer avail"));
    }

    return(Error);
}

/*******************************************************************************
Name        : GetUnusedPictureBuffer
Description : Get a picture buffer available for decoding
Parameters  : Video buffer manager handle, parameters
Assumptions :
Limitations : Caution: the picture returned may be the one currently displayed if wanting buffer for a B picture
Returns     : ST_NO_ERROR if allocated, then *PictureBuffer_p is not NULL (as UsePicture() never returns NULL)
              ST_ERROR_BAD_PARAMETER if bad parameter (nothing is changed)
              STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE if picture proposed is too big for the current profile (*PictureBuffer_p is then NULL)
              ST_ERROR_NO_MEMORY if cannot find a buffer to allocate (*PictureBuffer_p is then NULL)
              STVID_ERROR_NOT_AVAILABLE if, for a second field picture, it is impossible
              to allocate it because no corresponding first field picture exist (*PictureBuffer_p is then NULL)
*******************************************************************************/
/* Pending: !!!
 * field pic on frame pic: ? care polarity otherwise first field may OVW last field of the frame
   with 4 buffs & more: no, another free buff should be available before
   with 3 buffs: no, next field matches polarity.
*/
static ST_ErrorCode_t GetUnusedPictureBuffer(const VIDBUFF_Handle_t BuffersHandle,
        const VIDBUFF_GetUnusedPictureBufferParams_t * const Params_p,
        VIDBUFF_PictureBuffer_t ** const PictureBuffer_p,
        VIDBUFF_FrameBuffer_t * FrameBufferPool_p)
{
    VIDBUFF_FrameBuffer_t * CurrentFrame_p;
    VIDBUFF_PictureBuffer_t * CurrentPicture_p = NULL;
    VIDBUFF_PictureBuffer_t * PictureUnlocked_p = NULL;
    VIDBUFF_PictureBuffer_t * FieldNotUsed_p = NULL;
    VIDBUFF_PictureBuffer_t * PictureCandidateForOverwrite_p = NULL; /* Pointer to the picture currently "Last Field On Display" candidate for overwrite */
    BOOL ReturningCurrentDisplay = FALSE;
    BOOL FoundMatchingFirstField;
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
    STVID_MemoryProfile_t  CommonMemoryProfile;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U32 * NumberOfAllocatedFrame_p;
    VIDBUFF_PictureBuffer_t * NewFrameOrFirstFieldPicture_p, * NewNothingOrSecondFieldPicture_p;
    VIDBUFF_PictureBuffer_t * OldFrameOrFirstFieldPicture_p, * OldNothingOrSecondFieldPicture_p;
#ifdef USE_EXTRA_FRAME_BUFFERS
    BOOL IsExtraPool = FALSE;
#endif

    if ((Params_p == NULL) || (PictureBuffer_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Set returned picture buffer to NULL, so quitting with error will return a NULL picture buffer */
    *PictureBuffer_p = NULL;

    if (FrameBufferPool_p == NULL) /* No frame allocated for decoder: can't allocate any ! */
    {
        return(ST_ERROR_NO_MEMORY);
    }

    /* pool validity */
    /*---------------*/
    if(FrameBufferPool_p == VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p)
    {
        /* primary pool, OK */
        NumberOfAllocatedFrame_p = &(VIDBUFF_Data_p->NumberOfAllocatedFrameBuffers);
    }
    else if(FrameBufferPool_p == VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p)
    {
        /* decimated pool, OK */
        NumberOfAllocatedFrame_p = &(VIDBUFF_Data_p->NumberOfSecondaryAllocatedFrameBuffers);
    }
#ifdef USE_EXTRA_FRAME_BUFFERS
    else if(FrameBufferPool_p == VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p)
    {
        /* extra pool, OK */
        NumberOfAllocatedFrame_p = &(VIDBUFF_Data_p->NumberOfAllocatedExtraPrimaryFrameBuffers);
        IsExtraPool = TRUE;
    }
    else if(FrameBufferPool_p == VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p)
    {
        /* extra pool, OK */
        NumberOfAllocatedFrame_p = &(VIDBUFF_Data_p->NumberOfAllocatedExtraSecondaryFrameBuffers);
        IsExtraPool = TRUE;
    }
#endif /* USE_EXTRA_FRAME_BUFFERS */
    else
    {
        /* not valid pool */
        return(ST_ERROR_BAD_PARAMETER);
    }

#ifdef USE_EXTRA_FRAME_BUFFERS
    if (IsExtraPool)
    {
        /* Check if picture proposed for buffer fits inside extra buffers */
        if ( (Params_p->PictureWidth * Params_p->PictureHeight) >
              (EXTRA_FRAME_BUFFER_MAX_WIDTH * EXTRA_FRAME_BUFFER_MAX_HEIGHT) )
        {
            /* Picture too big for extra buffers.
            (No notification is necessary because this notification will be sent when looking at Primary Pool) */
            return(STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE);
        }
    }
    else
#endif  /* USE_EXTRA_FRAME_BUFFERS */
    {
        /* Check if picture proposed for buffer fits inside profile */
        if ((Params_p->PictureWidth * Params_p->PictureHeight) >
            (VIDBUFF_Data_p->Profile.ApiProfile.MaxWidth * VIDBUFF_Data_p->Profile.ApiProfile.MaxHeight))
        {
            /* Profile too small for this picture: if first time, notify IMPOSSIBLE_WITH_PROFILE_EVT */
            if (VIDBUFF_Data_p->IsNotificationOfImpossibleWithProfileAllowed)
            {
                /* Notify the event only once, until it is re-allowed */
                VIDBUFF_Data_p->IsNotificationOfImpossibleWithProfileAllowed = FALSE;

                /* Notify event, so that API level notifies the application. Propose larger buffers. */
                CommonMemoryProfile = VIDBUFF_Data_p->Profile.ApiProfile;
                CommonMemoryProfile.MaxWidth = Params_p->PictureWidth;
                CommonMemoryProfile.MaxHeight = Params_p->PictureHeight;
                STEVT_Notify(((VIDBUFF_Data_t *) BuffersHandle)->EventsHandle,
                    ((VIDBUFF_Data_t *) BuffersHandle)->RegisteredEventsID[VIDBUFF_IMPOSSIBLE_WITH_MEM_PROFILE_EVT_ID],
                    STEVT_EVENT_DATA_TYPE_CAST &CommonMemoryProfile);
            }
            /* Picture won't fit inside the profile: refuse to give buffers */
            return(STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE);
        }
    }

    /* Lock access to allocated frame buffers loop */
    semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    /* Consider allocated frame buffers */
    CurrentFrame_p = FrameBufferPool_p;

    if ((Params_p->PictureStructure == STVID_PICTURE_STRUCTURE_FRAME) ||
        (Params_p->ExpectingSecondField))
    {
        /* Wants to allocate a frame picture, or a field picture (first of 2 fields): look for best frame among all */
        do
        {
            if (!(FrameBufferToBeKilledAsSoonAsPossible(*CurrentFrame_p)))
            {
                if (HasNoPicture(*CurrentFrame_p))
                {
                    /* Frame buffer contains no picture: take it */
                    CurrentFrame_p->FrameOrFirstFieldPicture_p = UsePicture(BuffersHandle, CurrentFrame_p);
                    if (Params_p->PictureStructure != STVID_PICTURE_STRUCTURE_FRAME)
                    {
                        /* Field picture: 2 pictures for one frame */
                        CurrentFrame_p->NothingOrSecondFieldPicture_p = UsePicture(BuffersHandle, CurrentFrame_p);
                    }
                    *PictureBuffer_p = CurrentFrame_p->FrameOrFirstFieldPicture_p;
                    /* debug !!! */ if (*PictureBuffer_p == NULL)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Impossible to return ST_NO_ERROR and have (*PictureBuffer_p == NULL) !!!"));
                    }

                    /* Un-lock access to allocated frame buffers loop */
                    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

                    return(ErrorCode);
                }
                else
                {
                    /* Frame Buffer already has associated Picture Buffers */
                    if (!HasAPictureLocked(*CurrentFrame_p))
                    {
                        /* The Picture Buffers are unused: Use the FrameOrFirstFieldPicture  */
                        PictureUnlocked_p = CurrentFrame_p->FrameOrFirstFieldPicture_p;
                    }

                    /* Here manage the Overwrite Frame Buffer with a B on display where we decode inside. */
                    if ((VIDBUFF_Data_p->StrategyForTheBuffersFreeing.IsTheBuffersFreeingOptimized) &&
                    (((Params_p->MPEGFrame == STVID_MPEG_FRAME_B) && (CurrentFrame_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.MPEGFrame == STVID_MPEG_FRAME_B))
                        || (*NumberOfAllocatedFrame_p <= 1)))
                    {
    /*
        Synthesis of the overwrite cases : Frame over a Frame, Field over a Field, Frame over a Field and Field over a Frame.

        Coming Picture               Frame or Field
        Frame Buffer Picture         Frame LFD                   Frame Unlock
        Frame Buffer Availability    OVW                         X

        Coming Picture               Frame or Field
        Frame Buffer Picture         Field LFD + Field Unlock    Field Unlock + Field LFD    Field Unlock + Field Unlock
        Frame Buffer Availability    OVW                         OVW                         X

        OVW : Frame Buffer Available with overwrite.        X : Frame Buffer Available without Overwrite
    */
                        /* if frame buffer to decode in contains frame */
                        if (HasFrameNotFields(*CurrentFrame_p))
                        {
                            /* Frame or Field decoded over a Frame Picture in "Last Field On Display" state */
                            TrOverwrite(("\r\nOverwrite over a Frame"));
                            if (IsLockedLastFieldOnDisplay(CurrentFrame_p->FrameOrFirstFieldPicture_p))
                            {
                                /* Field over a Frame is not really good due to the HW behaviour. */
                                PictureCandidateForOverwrite_p = CurrentFrame_p->FrameOrFirstFieldPicture_p;
                            }
                        } /* End of Frame over a Frame OR Field over a Frame */

                        /* if frame buffer to decode in contains Fields */
                        else
                        {
                            if (Params_p->PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
                            {
                                /* frame over fields */

                                /* UNLOCKED + LFD */
                                if ( (IsUnlocked(CurrentFrame_p->FrameOrFirstFieldPicture_p)) &&
                                     (IsLockedLastFieldOnDisplay(CurrentFrame_p->NothingOrSecondFieldPicture_p)) )
                                {
                                    TrOverwrite(("\r\nOverwrite Frame over 2nd Field"));
                                    PictureCandidateForOverwrite_p = CurrentFrame_p->FrameOrFirstFieldPicture_p;
                                }
                                /* LFD + UNLOCKED */
                                else if ( (IsLockedLastFieldOnDisplay(CurrentFrame_p->FrameOrFirstFieldPicture_p)) &&
                                        (IsUnlocked(CurrentFrame_p->NothingOrSecondFieldPicture_p) ) )
                                {
                                    TrOverwrite(("\r\nOverwrite Frame over 1st Field"));
                                    PictureCandidateForOverwrite_p = CurrentFrame_p->FrameOrFirstFieldPicture_p;
                                }
                            }
                            else
                            {
                                /* fields over fields: Overwrite NOT necessary. We simply decode in the Field available (i.e the one corresponding to the Field "Last Field On Display" */

                                /* UNLOCKED + LFD: 1st Field is available */
                                if ( (IsUnlocked(CurrentFrame_p->FrameOrFirstFieldPicture_p)) &&
                                     (IsLockedLastFieldOnDisplay(CurrentFrame_p->NothingOrSecondFieldPicture_p)) )
                                {
                                    TrOverwrite(("\r\nField over 1st Field"));
                                    FieldNotUsed_p = CurrentFrame_p->FrameOrFirstFieldPicture_p;
                                }
                                /* LFD + UNLOCKED: 2nd Field is available */
                                else if ( (IsLockedLastFieldOnDisplay(CurrentFrame_p->FrameOrFirstFieldPicture_p)) &&
                                        (IsUnlocked(CurrentFrame_p->NothingOrSecondFieldPicture_p) ) )
                                {
                                    TrOverwrite(("\r\nField over 2nd Field"));
                                    FieldNotUsed_p = CurrentFrame_p->NothingOrSecondFieldPicture_p;
                                }

                                if (FieldNotUsed_p != NULL)
                                {
                                    /* WARNING:
                                        The Field available has a certain type (Top or Bottom).
                                        The Field requested has a certain type (Top or Bottom).
                                        If the types doesn't match we can not propose to use "FieldNotUsed_p" because this would consist to write in the part of the frame buffer
                                        currently on display so there will have display artefacts.
                                    */
                                    /* Check if the Field available has the right type */
                                    if (Params_p->PictureStructure != FieldNotUsed_p->ParsedPictureInformation.PictureGenericData.PictureInfos.VideoParams.PictureStructure)
                                    {
                                        /* The Field available doesn't have the right type so we should not return it. */
                                        TrOverwrite(("\r\nField of the wrong type"));
                                        FieldNotUsed_p = NULL;
                                    }
                                }
                            }
                        } /* End of Frame over a Field OR Field over a Field */
                    } /* End of Optimized 3 buffers */
                } /* end frame buffer contains a picture */

            }/* FrameBufferToBeKilledAsSoonAsPossible */

            /* Loop to search again for an unused frame */
            CurrentFrame_p = CurrentFrame_p->NextAllocated_p;
        } while (CurrentFrame_p != FrameBufferPool_p);

        /* If no Picture Unlocked has been found we take the Field not used found */
        if (PictureUnlocked_p == NULL)
        {
            PictureUnlocked_p = FieldNotUsed_p;
        }

        /* This is the porting of a code which was previously in VID_DEC (see "TODO_CL" comment):
            Overwrite should be done only if the last picture decoded was a B picture. For instance, if the sequence is "B2 P3 B4",
            B4 will not be decoded in overwrite over B2. It will be decoded when P3 will be on display. */
        if (VIDBUFF_Data_p->LastMPEGFrameType != STVID_MPEG_FRAME_B)
        {
            /* Last decoded picture was not a B => Overwrite not allowed */
            TrOverwrite(("\r\nOverwrite not allowed"));
            PictureCandidateForOverwrite_p = NULL;
        }

        /* Consider best picture found between Unlocked on the 2 fields, Unlocked on the field to come  */
        /* or LFD on the field to come:                                                                 */
        /* PictureUnlocked_p, PictureCandidateForOverwrite_p.   */
        CurrentPicture_p = PictureUnlocked_p;
        if (CurrentPicture_p == NULL)
        {
            /* No picture found yet: consider next search result */
            CurrentPicture_p = PictureCandidateForOverwrite_p;
            if (CurrentPicture_p == NULL)
            {
                /* No picture matching the conditions was found: return nothing */
                CurrentPicture_p = NULL;
            }
            else
            {
                if (!(FrameBufferToBeKilledAsSoonAsPossible(*(CurrentPicture_p->FrameBuffer_p))))
                {
                    CurrentPicture_p->FrameBuffer_p->HasDecodeToBeDoneWhileDisplaying = TRUE;
                    ReturningCurrentDisplay = TRUE;
                }
                else
                {
                    CurrentPicture_p = NULL;
                }
            }
        }
        else
        {
            CurrentPicture_p->FrameBuffer_p->HasDecodeToBeDoneWhileDisplaying = FALSE;
        }

        /* Return picture if found */
        if (CurrentPicture_p != NULL)
        {
            if (ReturningCurrentDisplay)
            {
                /****  Overwrite of a Frame Buffer currently on display ****/
                /* The new picture may have an encoding mode different than the one currently in the buffer (frame vs fields). This would require to change the picture
                   buffers associated to this frame buffer but this is not allowed while the picture is in the Display Queue.

                   New Picture Buffers are allocated to replace the ones currently used. The Frame Buffer will now be pointing to the new picture buffers.
                   The old picture buffers are still pointing to the frame buffer but this link will be removed when the picture buffers are released by the display
                   (this is the purpose of the flag "IsLinkToFrameBufferToBeRemoved").
                */

                TrOverwrite(("\r\nOverwrite in FB 0x%x", CurrentPicture_p->FrameBuffer_p));

                /* Create 1 (or 2 if stream is field-encoded) picture buffers to replace existing ones */
                if (Params_p->PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
                {
                    NewFrameOrFirstFieldPicture_p = UsePicture(BuffersHandle, CurrentPicture_p->FrameBuffer_p);
                    NewNothingOrSecondFieldPicture_p = NULL;
                }
                else
                {
                    NewFrameOrFirstFieldPicture_p = UsePicture(BuffersHandle, CurrentPicture_p->FrameBuffer_p);
                    NewNothingOrSecondFieldPicture_p = UsePicture(BuffersHandle, CurrentPicture_p->FrameBuffer_p);
                }

                /* Save original picture buffer references */
                OldFrameOrFirstFieldPicture_p = CurrentPicture_p->FrameBuffer_p->FrameOrFirstFieldPicture_p;
                OldNothingOrSecondFieldPicture_p = CurrentPicture_p->FrameBuffer_p->NothingOrSecondFieldPicture_p;

                /* This is where the replacement of the picture buffers is done */
                STOS_InterruptLock();     /* Start Critical Section */
                CurrentPicture_p->FrameBuffer_p->FrameOrFirstFieldPicture_p = NewFrameOrFirstFieldPicture_p;
                CurrentPicture_p->FrameBuffer_p->NothingOrSecondFieldPicture_p = NewNothingOrSecondFieldPicture_p;
                CurrentPicture_p = NewFrameOrFirstFieldPicture_p;
                STOS_InterruptUnlock();   /* End Critical Section */

                TrOverwrite(("\r\nNew 1st field: 0x%x, New 2nd field: 0x%x", NewFrameOrFirstFieldPicture_p, NewNothingOrSecondFieldPicture_p));

                /* Free the old picture buffers if allowed (picture not locked) */
                if (OldFrameOrFirstFieldPicture_p != NULL)
                {
                    if (IsLocked(OldFrameOrFirstFieldPicture_p))
                    {
                        /* This picture buffer is still used: It can not be released now. We set a flag to indicate that it should be released later */
                        TrOverwrite(("\r\nPict Buff 0x%x to be removed", OldFrameOrFirstFieldPicture_p));
                        OldFrameOrFirstFieldPicture_p->Buffers.IsLinkToFrameBufferToBeRemoved = TRUE;
                    }
                    else
                    {
                        TrOverwrite(("\r\nLeave Pict Buff 0x%x", OldFrameOrFirstFieldPicture_p));
                        LeavePicture(BuffersHandle, OldFrameOrFirstFieldPicture_p);
                        OldFrameOrFirstFieldPicture_p = NULL;
                    }
                }

                if (OldNothingOrSecondFieldPicture_p != NULL)
                {
                    if (IsLocked(OldNothingOrSecondFieldPicture_p))
                    {
                        /* This picture buffer is still used: It can not be released now. We set a flag to indicate that it should be released later  */
                        TrOverwrite(("\r\nPict Buff 0x%x to be removed", OldNothingOrSecondFieldPicture_p));
                        OldNothingOrSecondFieldPicture_p->Buffers.IsLinkToFrameBufferToBeRemoved = TRUE;
                    }
                    else
                    {
                        TrOverwrite(("\r\nLeave Pict Buff 0x%x", OldNothingOrSecondFieldPicture_p));
                        LeavePicture(BuffersHandle, OldNothingOrSecondFieldPicture_p);
                        OldNothingOrSecondFieldPicture_p = NULL;
                    }
                }
            }
            else
            {
                /* Found a picture in a frame buffer matching the conditions: update frame-picture links before returning the picture */
                if (Params_p->PictureStructure == STVID_PICTURE_STRUCTURE_FRAME)
                {
                    /* Wants to allocate a frame picture */
                    if (!(HasFrameNotFields(*(CurrentPicture_p->FrameBuffer_p))))
                    {
                        /* Frame buffer actually contains fields: release second field */
                        LeavePicture(BuffersHandle, CurrentPicture_p->FrameBuffer_p->NothingOrSecondFieldPicture_p);
                    CurrentPicture_p->FrameBuffer_p->NothingOrSecondFieldPicture_p = NULL;
                }
            }
            else
            {
                /* Wants to allocate a field picture */
                if (HasFrameNotFields(*(CurrentPicture_p->FrameBuffer_p)))
                {
                    /* Frame buffer actually contains a frame: get a picture for second field */
                    CurrentPicture_p->FrameBuffer_p->NothingOrSecondFieldPicture_p = UsePicture(BuffersHandle, CurrentPicture_p->FrameBuffer_p);
                }
            }
            }

            /* Return picture found */
            *PictureBuffer_p = CurrentPicture_p;
        } /* End picture found */
        else
        {
            /* No picture found */
            ErrorCode = ST_ERROR_NO_MEMORY;
        }
    } /* End of wanting to allocate a frame picture or a first field picture */
    else
    {
        FoundMatchingFirstField = FALSE;
        /* Wants to allocate a field picture, second of 2 fields: look for the matching first field */
        do
        {
            CurrentPicture_p = CurrentFrame_p->NothingOrSecondFieldPicture_p;
            if (!(FrameBufferToBeKilledAsSoonAsPossible(*CurrentFrame_p)))
            {
                if ( (CurrentFrame_p->FrameOrFirstFieldPicture_p != NULL) &&
                    (CurrentPicture_p != NULL) && /* Second field link valid (security, but should not happen that it is NULL if ExpectingSecondField is TRUE) */
                    (CurrentFrame_p->FrameOrFirstFieldPicture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID == Params_p->DecodingOrderFrameID)
                )
                {
                    FoundMatchingFirstField = TRUE;
                    /* Frame buffer contains 2 fields picture and only first field is there (or second field of previous picture is there) and temporal_reference match:
                    take picture if second field (from last image) not in display queue, or if displayed and first field too... */
                    if (IsUnlocked(CurrentPicture_p))
                    {
                        if (CurrentFrame_p->FrameOrFirstFieldPicture_p->Buffers.DisplayStatus == VIDBUFF_LAST_FIELD_ON_DISPLAY)
                        {
                            TrOverwrite(("\r\nHasDecodeToBeDoneWhileDisplaying set to FALSE!!!!"));
                            CurrentPicture_p->FrameBuffer_p->HasDecodeToBeDoneWhileDisplaying = FALSE;
                        }

                        /* Return picture found */
                        *PictureBuffer_p = CurrentPicture_p;
                    }
                    else
                    {
                        /* Found the picture but conditions to take it not met: end search */
                        TrOverwrite(("\r\nFound matching picture but conditions to take it not met"));
                        ErrorCode = ST_ERROR_NO_MEMORY;
                    }
                    /* Found the picture corresponding to second field: end search */
                    break;
                } /* end frame buffer contains a 2 fields picture */
            } /* FrameBufferToBeKilledAsSoonAsPossible */

            /* Loop to search again for an unused frame */
            CurrentFrame_p = CurrentFrame_p->NextAllocated_p;
        } while (CurrentFrame_p != FrameBufferPool_p);
        /* Scanned all frame buffers */

        if (!(FoundMatchingFirstField))
        {
            /* No first field picture was found for second field: error */
            ErrorCode = STVID_ERROR_NOT_AVAILABLE;
        }
    } /* End of wanting to allocate a second field of 2 fields picture */

    /* Un-lock access to allocated frame buffers loop */
    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    /* debug !!! */ if ((ErrorCode == ST_NO_ERROR) && (*PictureBuffer_p == NULL))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Impossible to return ST_NO_ERROR and have (*PictureBuffer_p == NULL) !!!"));
        TrError(("\r\nImpossible to return ST_NO_ERROR and have (*PictureBuffer_p == NULL) !!!"));
    }

    return(ErrorCode);
} /* End of GetUnusedPictureBuffer() function */

#ifdef ST_smoothbackward
/*******************************************************************************
Name        : VIDBUFF_GetAndTakeSmoothBackwardUnusedPictureBuffer
Description : Get a picture buffer available for decoding
Parameters  : Video buffer manager handle, parameters
Assumptions :
Limitations : Caution: the picture returned may be the one currently displayed if wanting buffer for a B picture
Returns     :
              STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE if picture proposed is too big for the current profile (*PictureBuffer_p_p is then NULL)
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_GetAndTakeSmoothBackwardUnusedPictureBuffer(const VIDBUFF_Handle_t BuffersHandle,
        const VIDBUFF_GetSmoothBackwardUnusedPictureBufferParams_t * const Params_p,
                     VIDBUFF_PictureBuffer_t ** const PictureBuffer_p_p,
                     VIDCOM_MODULE_ID_t ModId)
{
    ST_ErrorCode_t          Error;
    VIDBUFF_FrameBuffer_t * BufferPool_p;
    VIDBUFF_Data_t *        VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;

#ifdef USE_EXTRA_FRAME_BUFFERS
    /* Set returned picture buffer to NULL, so quitting with error will return a NULL picture buffer */
    *PictureBuffer_p_p = NULL;

    /* If Extra SD Frame Buffers are available we try to allocate a Frame Buffer there first */
    if (VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p != NULL)
    {
        BufferPool_p = VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p;
        Error = GetSmoothBackwardGenericUnusedPictureBuffer(BuffersHandle,Params_p,
                PictureBuffer_p_p,BufferPool_p);

        if (Error == ST_NO_ERROR)
        {
            TrSmoothBackwrd(("\r\nGetSmoothBackward => 0x%08x", *PictureBuffer_p_p));
        }
    }
    /* If no PictureBuffer as been found in the extra loop, we look in the standard loop... */
    if (*PictureBuffer_p_p == NULL)
#endif /* USE_EXTRA_FRAME_BUFFERS */
    {

        BufferPool_p = VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p;
        Error = GetSmoothBackwardGenericUnusedPictureBuffer(BuffersHandle,Params_p,
                PictureBuffer_p_p,BufferPool_p);

        if (Error == ST_NO_ERROR)
        {
            TrSmoothBackwrd(("\r\nGetSmoothBackward => 0x%08x", *PictureBuffer_p_p));
        }
        else
        {
            TrSmoothBackwrd(("\r\nSmoothBackward: No Pict Buffer avail"));
        }
    }

    if (Error == ST_NO_ERROR)
    {
        ResetPictureBufferParams(BuffersHandle, (*PictureBuffer_p_p));
        TrTakeRelease(("\r\nGetAndTakeBackward Pict 0x%x FB 0x%x @ 0x%x",
                *PictureBuffer_p_p,
                (*PictureBuffer_p_p)->FrameBuffer_p,
                (*PictureBuffer_p_p)->FrameBuffer_p->Allocation.Address_p));
        TAKE_PICTURE_BUFFER(BuffersHandle, (*PictureBuffer_p_p), ModId);
    }

    return(Error);
}

/*******************************************************************************
Name        : VIDBUFF_GetSmoothBackwardDecimatedUnusedPictureBuffer
Description : Get a picture buffer available for decoding a decimated picture
Parameters  : Video buffer manager handle, parameters
Assumptions :
Limitations : Caution: the picture returned may be the one currently displayed if wanting buffer for a B picture
Returns     :
              STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE if picture proposed is too big for the current profile (*PictureBuffer_p_p is then NULL)
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_GetAndTakeSmoothBackwardDecimatedUnusedPictureBuffer(const VIDBUFF_Handle_t BuffersHandle,
        const VIDBUFF_GetSmoothBackwardUnusedPictureBufferParams_t * const Params_p,
                     VIDBUFF_PictureBuffer_t ** const PictureBuffer_p_p,
                     VIDCOM_MODULE_ID_t ModId)
{
    ST_ErrorCode_t          Error;
    VIDBUFF_FrameBuffer_t * BufferPool_p;
#ifdef USE_EXTRA_FRAME_BUFFERS
	VIDBUFF_Data_t *                VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
#endif /* USE_EXTRA_FRAME_BUFFERS */

#ifdef USE_EXTRA_FRAME_BUFFERS
    /* Set returned picture buffer to NULL, so quitting with error will return a NULL picture buffer */
    *PictureBuffer_p_p = NULL;

    /* If Extra SD Frame Buffers are available we try to allocate a Frame Buffer there first */
    if (VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p != NULL)
    {
        BufferPool_p = VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p;
        Error = GetSmoothBackwardGenericUnusedPictureBuffer(BuffersHandle,Params_p,
                PictureBuffer_p_p,BufferPool_p);

        if (Error == ST_NO_ERROR)
        {
            TrSmoothBackwrd(("\r\nGetSmoothBackward => 0x%08x", *PictureBuffer_p_p));
        }
    }
    /* If no PictureBuffer as been found in the extra loop, we look in the standard loop... */
    if (*PictureBuffer_p_p == NULL)
#endif /* USE_EXTRA_FRAME_BUFFERS */
    {
        BufferPool_p = ((VIDBUFF_Data_t *) BuffersHandle)->AllocatedSecondaryFrameBuffersLoop_p;
        Error = GetSmoothBackwardGenericUnusedPictureBuffer(BuffersHandle,Params_p,
                PictureBuffer_p_p,BufferPool_p);
    }

    if (Error == ST_NO_ERROR)
    {
        ResetPictureBufferParams(BuffersHandle, (*PictureBuffer_p_p));
        TrTakeRelease(("\r\nGetAndTakeBackward Decim Pict 0x%x FB 0x%x @ 0x%x",
                *PictureBuffer_p_p,
                (*PictureBuffer_p_p)->FrameBuffer_p,
                (*PictureBuffer_p_p)->FrameBuffer_p->Allocation.Address_p));
        TAKE_PICTURE_BUFFER(BuffersHandle, (*PictureBuffer_p_p), ModId);
    }

    return(Error);
}

/*******************************************************************************
Name        : GetSmoothBackwardGenericUnusedPictureBuffer
Description : Get a picture buffer available for decoding
Parameters  : Video buffer manager handle, parameters
Assumptions :
Limitations : Caution: the picture returned may be the one currently displayed if wanting buffer for a B picture
Returns     :
              STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE if picture proposed is too big for the current profile (*PictureBuffer_p is then NULL)
*******************************************************************************/
ST_ErrorCode_t GetSmoothBackwardGenericUnusedPictureBuffer(
        const VIDBUFF_Handle_t BuffersHandle,
        const VIDBUFF_GetSmoothBackwardUnusedPictureBufferParams_t * const Params_p,
        VIDBUFF_PictureBuffer_t ** const PictureBuffer_p,
        VIDBUFF_FrameBuffer_t * FrameBufferPool_p)
{
    VIDBUFF_FrameBuffer_t * CurrentFrame_p;
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVID_MemoryProfile_t  CommonMemoryProfile;
#ifdef USE_EXTRA_FRAME_BUFFERS
    BOOL IsExtraPool = FALSE;
#endif

    if ((Params_p == NULL) || (PictureBuffer_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Set returned picture buffer to NULL, so quitting with error will return a NULL picture buffer */
    *PictureBuffer_p = NULL;

    /* pool validity */
    /*---------------*/
    if(FrameBufferPool_p == VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p)
    {
        /* primary pool, OK */
    }
    else if(FrameBufferPool_p == VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p)
    {
        /* decimated pool, OK */
    }
#ifdef USE_EXTRA_FRAME_BUFFERS
    else if(FrameBufferPool_p == VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p)
    {
        /* extra pool, OK */
        IsExtraPool = TRUE;
    }
    else if(FrameBufferPool_p == VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p)
    {
        /* extra secondary pool, OK */
        IsExtraPool = TRUE;
    }
#endif /* USE_EXTRA_FRAME_BUFFERS */
    else
    {
        /* not valid pool */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (FrameBufferPool_p == NULL) /* No frame allocated for decoder: can't allocate any ! */
    {
        return(ST_ERROR_NO_MEMORY);
    }

#ifdef USE_EXTRA_FRAME_BUFFERS
    if (IsExtraPool)
    {
        /* Check if picture proposed for buffer fits inside extra buffers */
        if ( (Params_p->PictureWidth * Params_p->PictureHeight) >
              (EXTRA_FRAME_BUFFER_MAX_WIDTH * EXTRA_FRAME_BUFFER_MAX_HEIGHT) )
        {
            /* Picture too big for extra buffers.
            (No notification is necessary because this notification will be sent when looking at Primary Pool) */
            return(STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE);
        }
    }
    else
#endif  /* USE_EXTRA_FRAME_BUFFERS */
    {
        /* Check if picture proposed for buffer fits inside profile */
        if ((Params_p->PictureWidth * Params_p->PictureHeight) >
            (VIDBUFF_Data_p->Profile.ApiProfile.MaxWidth * VIDBUFF_Data_p->Profile.ApiProfile.MaxHeight))
        {
            /* Profile too small for this picture: if first time, notify IMPOSSIBLE_WITH_PROFILE_EVT */
            if (VIDBUFF_Data_p->IsNotificationOfImpossibleWithProfileAllowed)
            {
                /* Notify the event only once, until it is re-allowed */
                VIDBUFF_Data_p->IsNotificationOfImpossibleWithProfileAllowed = FALSE;

                /* Notify event, so that API level notifies the application. Propose larger buffers. */
                CommonMemoryProfile = VIDBUFF_Data_p->Profile.ApiProfile;
                CommonMemoryProfile.MaxWidth = Params_p->PictureWidth;
                CommonMemoryProfile.MaxHeight = Params_p->PictureHeight;
                STEVT_Notify(((VIDBUFF_Data_t *) BuffersHandle)->EventsHandle,
                    ((VIDBUFF_Data_t *) BuffersHandle)->RegisteredEventsID[VIDBUFF_IMPOSSIBLE_WITH_MEM_PROFILE_EVT_ID],
                    STEVT_EVENT_DATA_TYPE_CAST &CommonMemoryProfile);
            }
            /* Picture won't fit inside the profile: refuse to give buffers */
            return(STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE);
        }
    }

    /* Lock access to allocated frame buffers loop */
    semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    /* Consider allocated frame buffers */
    CurrentFrame_p = FrameBufferPool_p;

    if ((Params_p->PictureStructure == STVID_PICTURE_STRUCTURE_FRAME) ||
        (Params_p->ExpectingSecondField))
    {
        /* Wants to allocate a frame picture, or a field picture (first of 2 fields): look for best frame among all */
        do
        {
            if (!(FrameBufferToBeKilledAsSoonAsPossible(*CurrentFrame_p)))
            {
                if (HasNoPicture(*CurrentFrame_p))
                {
                    /* Frame buffer contains no picture: take it */
                    CurrentFrame_p->FrameOrFirstFieldPicture_p = UsePicture(BuffersHandle, CurrentFrame_p);
                    CurrentFrame_p->FrameOrFirstFieldPicture_p->SmoothBackwardPictureInfo_p = NULL;

                    if (Params_p->PictureStructure != STVID_PICTURE_STRUCTURE_FRAME)
                    {
                        /* Field picture: 2 pictures for one frame */
                        CurrentFrame_p->NothingOrSecondFieldPicture_p = UsePicture(BuffersHandle, CurrentFrame_p);
                        CurrentFrame_p->NothingOrSecondFieldPicture_p->SmoothBackwardPictureInfo_p = NULL;
                    }
                    *PictureBuffer_p = CurrentFrame_p->FrameOrFirstFieldPicture_p;
                    /* debug !!! */ if (*PictureBuffer_p == NULL)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Impossible to return ST_NO_ERROR and have (*PictureBuffer_p == NULL) !!!"));
                    }
                    /* Un-lock access to allocated frame buffers loop */
                    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);
                    return(ErrorCode);
                }
                else
                {
                    if (Params_p->PictureStructure != STVID_PICTURE_STRUCTURE_FRAME)
                    {
                        if (CurrentFrame_p->NothingOrSecondFieldPicture_p == NULL)
                        {
                            /* Field picture: 2 pictures for one frame */
                            CurrentFrame_p->NothingOrSecondFieldPicture_p = UsePicture(BuffersHandle, CurrentFrame_p);
                            if (CurrentFrame_p->NothingOrSecondFieldPicture_p != NULL)
                            {
                                CurrentFrame_p->NothingOrSecondFieldPicture_p->SmoothBackwardPictureInfo_p = NULL;
                            }
                        }
                    }

                    /* Frame Buffer already has associated Picture Buffers */
                    if (!HasAPictureLocked(*CurrentFrame_p))
                    {
                        /* The associated Picture Buffers are unused: Use them */
                        *PictureBuffer_p = CurrentFrame_p->FrameOrFirstFieldPicture_p;
                        /* Un-lock access to allocated frame buffers loop */
                        semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);
                        return(ErrorCode);
                    }
                }
                /* Loop to search again for an unused frame */
                CurrentFrame_p = CurrentFrame_p->NextAllocated_p;
            }
        } while (CurrentFrame_p != FrameBufferPool_p);
    }

#ifdef TRACE_UART_SMOOTH
    TraceBuffer(("\n\r"));
 #endif

    /* Un-lock access to allocated frame buffers loop */
    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    /* if no buffer has been found */
    if (*PictureBuffer_p == NULL)
    {
        return (ST_ERROR_NO_MEMORY);
    }

    return (ErrorCode);
} /* end of GetSmoothBackwardGenericUnusedPictureBuffer*/
#endif /* ST_smoothbackward */

/*******************************************************************************
Name        : VIDBUFF_Init
Description : Initialise buffer manager
Parameters  : Video buffer manager handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_NO_MEMORY if can't allocate
              STVID_ERROR_EVENT_REGISTRATION if registration problem
              ST_ERROR_BAD_PARAMETER if bad parameter
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_Init(const VIDBUFF_InitParams_t * const InitParams_p, VIDBUFF_Handle_t * const BuffersHandle_p)
{
    U8 k;
    STEVT_OpenParams_t STEVT_OpenParams;
    VIDBUFF_Data_t * VIDBUFF_Data_p;
    HALBUFF_InitParams_t HALInitParams;
    ST_ErrorCode_t ErrorCode;

    if ((InitParams_p == NULL) || (BuffersHandle_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Allocate a HALBUFF structure */
    SAFE_MEMORY_ALLOCATE(VIDBUFF_Data_p, VIDBUFF_Data_t *, InitParams_p->CPUPartition_p, sizeof(VIDBUFF_Data_t));
    if (VIDBUFF_Data_p == NULL)
    {
        /* Error: allocation failed */
        return(ST_ERROR_NO_MEMORY);
    }

    /* Remember partition */
    VIDBUFF_Data_p->CPUPartition_p  = InitParams_p->CPUPartition_p;

    /* Allocations succeeded: make handle point on structure */
    *BuffersHandle_p = (VIDBUFF_Handle_t *) VIDBUFF_Data_p;
    VIDBUFF_Data_p->ValidityCheck = VIDBUFF_VALID_HANDLE;
    VIDBUFF_Data_p->HALBuffersHandle = NULL;

    InitBufferManagerStructure(*BuffersHandle_p);

    /* Allocate pool of frame buffer structures */
    VIDBUFF_Data_p->MaxFrameBuffers = InitParams_p->MaxFrameBuffersInProfile + VIDBUF_ADDED_FRAME_BUFFER;
    VIDBUFF_Data_p->MaxSecondaryFrameBuffers= InitParams_p->MaxFrameBuffersInProfile + VIDBUF_ADDED_FRAME_BUFFER;
    SAFE_MEMORY_ALLOCATE(VIDBUFF_Data_p->FrameBuffers_p, VIDBUFF_FrameBuffer_t *,
                         InitParams_p->CPUPartition_p,
                         VIDBUFF_Data_p->MaxFrameBuffers * sizeof(VIDBUFF_FrameBuffer_t));
    if (VIDBUFF_Data_p->FrameBuffers_p == NULL)
    {
        /* Error: allocation failed, deallocate previous allocations */
        VIDBUFF_Term(*BuffersHandle_p);
        return(ST_ERROR_NO_MEMORY);
    }
    SAFE_MEMORY_ALLOCATE(VIDBUFF_Data_p->SecondaryFrameBuffers_p, VIDBUFF_FrameBuffer_t *,
                         InitParams_p->CPUPartition_p, VIDBUFF_Data_p->MaxSecondaryFrameBuffers * sizeof(VIDBUFF_FrameBuffer_t));

    if (VIDBUFF_Data_p->SecondaryFrameBuffers_p== NULL)
    {
        /* Error: allocation failed, deallocate previous allocations */
        VIDBUFF_Term(*BuffersHandle_p);
        return(ST_ERROR_NO_MEMORY);
    }
    /* Initialise frame buffer structures */
    for (k = 0; k < VIDBUFF_Data_p->MaxFrameBuffers; k++)
    {
        (VIDBUFF_Data_p->FrameBuffers_p)[k].AllocationIsDone                    = FALSE;
        (VIDBUFF_Data_p->FrameBuffers_p)[k].FrameBufferType                    = VIDBUFF_UNKNOWN_FRAME_BUFFER_TYPE;
        (VIDBUFF_Data_p->FrameBuffers_p)[k].ToBeKilledAsSoonAsPossible          = FALSE;
        (VIDBUFF_Data_p->FrameBuffers_p)[k].CompressionLevel                    = STVID_COMPRESSION_LEVEL_NONE;
        (VIDBUFF_Data_p->FrameBuffers_p)[k].DecimationFactor                    = STVID_DECIMATION_FACTOR_NONE;
        (VIDBUFF_Data_p->FrameBuffers_p)[k].AvailableReconstructionMode         = VIDBUFF_NONE_RECONSTRUCTION;
        (VIDBUFF_Data_p->FrameBuffers_p)[k].HasDecodeToBeDoneWhileDisplaying    = FALSE;
    }
    for (k = 0; k < VIDBUFF_Data_p->MaxSecondaryFrameBuffers; k++)
    {
        (VIDBUFF_Data_p->SecondaryFrameBuffers_p)[k].AllocationIsDone                   = FALSE;
        (VIDBUFF_Data_p->SecondaryFrameBuffers_p)[k].FrameBufferType                   = VIDBUFF_UNKNOWN_FRAME_BUFFER_TYPE;
        (VIDBUFF_Data_p->SecondaryFrameBuffers_p)[k].ToBeKilledAsSoonAsPossible         = FALSE;
        (VIDBUFF_Data_p->SecondaryFrameBuffers_p)[k].CompressionLevel                   = STVID_COMPRESSION_LEVEL_NONE;
        (VIDBUFF_Data_p->SecondaryFrameBuffers_p)[k].DecimationFactor                   = STVID_DECIMATION_FACTOR_NONE;
        (VIDBUFF_Data_p->SecondaryFrameBuffers_p)[k].AvailableReconstructionMode        = VIDBUFF_NONE_RECONSTRUCTION;
        (VIDBUFF_Data_p->SecondaryFrameBuffers_p)[k].HasDecodeToBeDoneWhileDisplaying   = FALSE;
    }

    /* Allocate pool of picture buffer structures: Up to 2 pictures for each frame buffer (field pictures are 2 per frame) */
    SAFE_MEMORY_ALLOCATE(VIDBUFF_Data_p->AllocatedPictureBuffersTable_p,
                         VIDBUFF_PictureBuffer_t *,
                         InitParams_p->CPUPartition_p,
                         2 * (VIDBUFF_Data_p->MaxFrameBuffers + VIDBUFF_Data_p->MaxSecondaryFrameBuffers + MAX_EXTRA_FRAME_BUFFER_NBR) * sizeof(VIDBUFF_PictureBuffer_t));
    if (VIDBUFF_Data_p->AllocatedPictureBuffersTable_p == NULL)
    {
        /* Error: allocation failed, deallocate previous allocations */
        VIDBUFF_Term(*BuffersHandle_p);
        return(ST_ERROR_NO_MEMORY);
    }
    /* Initialise picture buffer structures */
    for (k = 0; k < (2 * (VIDBUFF_Data_p->MaxFrameBuffers + VIDBUFF_Data_p->MaxSecondaryFrameBuffers + MAX_EXTRA_FRAME_BUFFER_NBR)); k++)
    {
        (VIDBUFF_Data_p->AllocatedPictureBuffersTable_p)[k].Buffers.IsInUse = FALSE;
        (VIDBUFF_Data_p->AllocatedPictureBuffersTable_p)[k].Buffers.DisplayStatus = VIDBUFF_NOT_ON_DISPLAY;
        (VIDBUFF_Data_p->AllocatedPictureBuffersTable_p)[k].Buffers.IsInDisplayQueue = FALSE;
        (VIDBUFF_Data_p->AllocatedPictureBuffersTable_p)[k].Buffers.PictureLockCounter = 0;
        (VIDBUFF_Data_p->AllocatedPictureBuffersTable_p)[k].Buffers.IsLinkToFrameBufferToBeRemoved = FALSE;
        memset((VIDBUFF_Data_p->AllocatedPictureBuffersTable_p)[k].Buffers.Module, 0, sizeof(VIDBUFF_ModuleCounter_t) * VIDCOM_MAX_MODULE_NBR);
        (VIDBUFF_Data_p->AllocatedPictureBuffersTable_p)[k].Buffers.LockCounterSemaphore_p = STOS_SemaphoreCreatePriority(VIDBUFF_Data_p->CPUPartition_p, 1);
        (VIDBUFF_Data_p->AllocatedPictureBuffersTable_p)[k].FrameBuffer_p = NULL;
        (VIDBUFF_Data_p->AllocatedPictureBuffersTable_p)[k].PPB.Address_p = NULL;
        (VIDBUFF_Data_p->AllocatedPictureBuffersTable_p)[k].PPB.Size = 0;
        (VIDBUFF_Data_p->AllocatedPictureBuffersTable_p)[k].PPB.FieldCounter = 0;
    }

    /* Store parameters */
    VIDBUFF_Data_p->AvmemPartitionHandle    = InitParams_p->AvmemPartitionHandle;
    VIDBUFF_Data_p->DeviceType              = InitParams_p->DeviceType;
    VIDBUFF_Data_p->FrameBuffersType        = InitParams_p->FrameBuffersType;

    /* Init information for Overwrite */
    VIDBUFF_Data_p->LastMPEGFrameType = STVID_MPEG_FRAME_I;

    /* Open EVT handle */
    ErrorCode = STEVT_Open(InitParams_p->EventsHandlerName, &STEVT_OpenParams, &(VIDBUFF_Data_p->EventsHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module buffer manager init: could not open EVT !"));
        VIDBUFF_Term(*BuffersHandle_p);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    /* Register events */
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDBUFF_Data_p->EventsHandle, InitParams_p->VideoName, VIDBUFF_PICTURE_OUT_OF_DISPLAY_EVT, &(VIDBUFF_Data_p->RegisteredEventsID[VIDBUFF_PICTURE_OUT_OF_DISPLAY_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDBUFF_Data_p->EventsHandle, InitParams_p->VideoName, VIDBUFF_NEW_PICTURE_BUFFER_AVAILABLE_EVT, &(VIDBUFF_Data_p->RegisteredEventsID[VIDBUFF_NEW_PICTURE_BUFFER_AVAILABLE_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDBUFF_Data_p->EventsHandle, InitParams_p->VideoName, VIDBUFF_IMPOSSIBLE_WITH_MEM_PROFILE_EVT, &(VIDBUFF_Data_p->RegisteredEventsID[VIDBUFF_IMPOSSIBLE_WITH_MEM_PROFILE_EVT_ID]));
    }
#ifdef ST_OSLINUX
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDBUFF_Data_p->EventsHandle, InitParams_p->VideoName, VIDBUFF_NEW_ALLOCATED_FRAMEBUFFERS_EVT, &(VIDBUFF_Data_p->RegisteredEventsID[VIDBUFF_NEW_ALLOCATED_FRAMEBUFFERS_EVT_ID]));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDBUFF_Data_p->EventsHandle, InitParams_p->VideoName, VIDBUFF_DEALLOCATED_FRAMEBUFFERS_EVT, &(VIDBUFF_Data_p->RegisteredEventsID[VIDBUFF_DEALLOCATED_FRAMEBUFFERS_EVT_ID]));
    }
#endif
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module buffer manager init: could not register events !"));
        VIDBUFF_Term(*BuffersHandle_p);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    HALInitParams.CPUPartition_p    = InitParams_p->CPUPartition_p;
    HALInitParams.DeviceType        = InitParams_p->DeviceType;
    HALInitParams.SharedMemoryBaseAddress_p = InitParams_p->SharedMemoryBaseAddress_p;
    HALInitParams.AvmemPartitionHandle = InitParams_p->AvmemPartitionHandle;
    HALInitParams.FrameBuffersType  = InitParams_p->FrameBuffersType;
#ifdef ST_XVP_ENABLE_FGT
    HALInitParams.FGTHandle  = InitParams_p->FGTHandle;
#endif /* ST_XVP_ENABLE_FGT */

    ErrorCode = HALBUFF_Init(&HALInitParams, &(VIDBUFF_Data_p->HALBuffersHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: HAL init failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module buffer manager init: could not initialize HAL buffer manager !"));
        VIDBUFF_Term(*BuffersHandle_p);
        return(ErrorCode);
    }

    return(ErrorCode);
} /* End of VIDBUFF_Init() function */


/*******************************************************************************
Name        : VIDBUFF_PictureRemovedFromDisplayQueue
Description : Inform VIDBUFF that a picture has been removed from display queue.
Parameters  : Video buffer manager handle, picture removed from the queue.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if problem
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_PictureRemovedFromDisplayQueue(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const Picture_p)
{
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;

    /* Simply notify that this picture buffer has been removed from display queue */
    STEVT_Notify(VIDBUFF_Data_p->EventsHandle,
                 VIDBUFF_Data_p->RegisteredEventsID[VIDBUFF_PICTURE_OUT_OF_DISPLAY_EVT_ID],
                 STEVT_EVENT_DATA_TYPE_CAST Picture_p);

    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : VIDBUFF_SetAvmemPartitionHandleForFrameBuffers
Description :
Parameters  : Buffer manager handle and New Avmem partition to be used
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if everything OK
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_SetAvmemPartitionForFrameBuffers(const VIDBUFF_Handle_t BuffersHandle, const STAVMEM_PartitionHandle_t AvmemPartitionHandle)
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    VIDBUFF_Data_t *    VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
    BOOL                AvmemPartitionHandleHasChanged;

    if ((VIDBUFF_Data_p == NULL) || (AvmemPartitionHandle == (STAVMEM_PartitionHandle_t)NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    ErrorCode = HALBUFF_SetAvmemPartitionForFrameBuffers(VIDBUFF_Data_p->HALBuffersHandle, AvmemPartitionHandle, &AvmemPartitionHandleHasChanged);

    if ( (ErrorCode == ST_NO_ERROR) && AvmemPartitionHandleHasChanged)
    {
        /* Lock access to allocated frame buffers loop */
        semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

        VIDBUFF_TagEveryBuffersAsToBeKilledAsap(BuffersHandle, VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p);
#ifdef USE_EXTRA_FRAME_BUFFERS
        VIDBUFF_TagEveryBuffersAsToBeKilledAsap(BuffersHandle, VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p);
#endif

        /* Un-lock access to allocated frame buffers loop */
        semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);
    }

    return(ErrorCode);
} /* End of VIDBUFF_SetAvmemPartitionForFrameBuffers() function */

/*******************************************************************************
Name        : VIDBUFF_SetAvmemPartitionForDecimatedFrameBuffers
Description :
Parameters  : Buffer manager handle and New Avmem partition to be used
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if everything OK
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_SetAvmemPartitionForDecimatedFrameBuffers(const VIDBUFF_Handle_t BuffersHandle, const STAVMEM_PartitionHandle_t AvmemPartitionHandle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
    BOOL                AvmemPartitionHandleHasChanged;

    if ((VIDBUFF_Data_p == NULL) || (AvmemPartitionHandle == (STAVMEM_PartitionHandle_t)NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    if ((VIDBUFF_Data_p->Profile.ApplicableDecimationFactor == STVID_DECIMATION_FACTOR_NONE)
        && (VIDBUFF_Data_p->DeviceType != STVID_DEVICE_TYPE_7100_MPEG4P2)
        && (VIDBUFF_Data_p->DeviceType != STVID_DEVICE_TYPE_7109_MPEG4P2)
        && (VIDBUFF_Data_p->DeviceType != STVID_DEVICE_TYPE_7109_AVS)
        && (VIDBUFF_Data_p->DeviceType != STVID_DEVICE_TYPE_7200_MPEG4P2))
    {
        return(STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE);
    }

    ErrorCode = HALBUFF_SetAvmemPartitionForDecimatedFrameBuffers(VIDBUFF_Data_p->HALBuffersHandle, AvmemPartitionHandle, &AvmemPartitionHandleHasChanged);

    if ( (ErrorCode == ST_NO_ERROR) && AvmemPartitionHandleHasChanged)
    {
        /* Lock access to allocated frame buffers loop */
        semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

        VIDBUFF_TagEveryBuffersAsToBeKilledAsap(BuffersHandle, VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p);
#ifdef USE_EXTRA_FRAME_BUFFERS
        VIDBUFF_TagEveryBuffersAsToBeKilledAsap(BuffersHandle, VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p);
#endif

        /* Un-lock access to allocated frame buffers loop */
        semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);
    }

    return(ErrorCode);
} /* End of VIDBUFF_SetAvmemPartitionForDecimatedFrameBuffers() function */

/*******************************************************************************
Name        : VIDBUFF_SetAvmemPartitionForAdditionnalData
Description :
Parameters  : Buffer manager handle and New Avmem partition to be used
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if everything OK
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_SetAvmemPartitionForAdditionnalData(const VIDBUFF_Handle_t BuffersHandle, const STAVMEM_PartitionHandle_t AvmemPartitionHandle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;

    if ((VIDBUFF_Data_p == NULL) || (AvmemPartitionHandle == (STAVMEM_PartitionHandle_t)NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    ErrorCode = HALBUFF_SetAvmemPartitionForAdditionnalData(VIDBUFF_Data_p->HALBuffersHandle, AvmemPartitionHandle);

    return(ErrorCode);
} /* End of VIDBUFF_SetAvmemPartitionForAdditionnalData() function */

#if defined(DVD_SECURED_CHIP)
/*******************************************************************************
Name        : VIDBUFF_SetAvmemPartitionForESCopyBuffer
Description :
Parameters  : Buffer manager handle and New Avmem partition to be used
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if everything OK
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_SetAvmemPartitionForESCopyBuffer(const VIDBUFF_Handle_t BuffersHandle, const STAVMEM_PartitionHandle_t AvmemPartitionHandle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;

    if ((VIDBUFF_Data_p == NULL) || (AvmemPartitionHandle == (STAVMEM_PartitionHandle_t)NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    ErrorCode = HALBUFF_SetAvmemPartitionForESCopyBuffer(VIDBUFF_Data_p->HALBuffersHandle, AvmemPartitionHandle);

    return(ErrorCode);
} /* End of VIDBUFF_SetAvmemPartitionForESCopyBuffer() function */
#endif /* DVD_SECURED_CHIP */

/*******************************************************************************
Name        : VIDBUFF_SetInfos
Description : Set informations comming from appli or other modules
Parameters  : Video buffer manager handle

Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if NULL pointer
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_SetInfos(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_Infos_t * const Infos_p)
{
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
    HALBUFF_Infos_t HALBUFF_Infos;
    ST_ErrorCode_t ErrorCode;

/* TODO (PC) : Check if change is allowed. Could be useful to avoid it if frame buffers are already allocated ? */
    VIDBUFF_Data_p->FrameBufferAdditionalDataBytesPerMB = Infos_p->FrameBufferAdditionalDataBytesPerMB;
    HALBUFF_Infos.FrameBufferAdditionalDataBytesPerMB = Infos_p->FrameBufferAdditionalDataBytesPerMB;

    ErrorCode = HALBUFF_SetInfos(VIDBUFF_Data_p->HALBuffersHandle, &HALBUFF_Infos);
/*    ErrorCode = HALBUFF_SetInfos(VIDBUFF_Data_p->HALBuffersHandle, (HALBUFF_Infos_t *)Infos_p); */

    return(ErrorCode);
} /* End of VIDBUFF_SetInfos() function */


/*******************************************************************************
Name        : VIDBUFF_SetStrategyForTheBuffersFreeing
Description : Set the strategy for the buffers freeing
Parameters  : Video buffers handle, Buffer freeing strategy
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_SetStrategyForTheBuffersFreeing(
        const VIDBUFF_Handle_t BuffersHandle,
        const VIDBUFF_StrategyForTheBuffersFreeing_t * const StrategyForTheBuffersFreeing_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* No need to protect access to anything here. The variable StrategyForTheBuffersFreeing is read-only everywhere else while running. */

    /* Valid mode: adopt it */
    ((VIDBUFF_Data_t *) BuffersHandle)->StrategyForTheBuffersFreeing = *StrategyForTheBuffersFreeing_p;

    return(ErrorCode);
} /* End of VIDBUFF_SetStrategyForTheBuffersFreeing() function */


/*******************************************************************************
Name        : VIDBUFF_SetHDPIPParams
Description : Set HDPIP buffers parameters.
Parameters  : Video buffer manager handle, HDPIP parameters.
Assumptions : HDPPIP params pointer is not NULL
                (checked before called).
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_SetHDPIPParams(const VIDBUFF_Handle_t BuffersHandle,
        const STVID_HDPIPParams_t * const HDPIPParams_p)
{
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
    BOOL             isAnyChangeDetected = FALSE;

    /* Lock access to allocated frame buffers loop */
    semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    /* Test if any change since last use of HDPIP.  */
    if (HDPIPParams_p->Enable != VIDBUFF_Data_p->HDPIPParams.Enable)
    {
        isAnyChangeDetected = TRUE;
    }
    else
    {
        if ( (HDPIPParams_p->Enable) &&
            ((HDPIPParams_p->WidthThreshold  != VIDBUFF_Data_p->HDPIPParams.WidthThreshold) ||
             (HDPIPParams_p->HeightThreshold != VIDBUFF_Data_p->HDPIPParams.HeightThreshold)) )
        {

            /* HDPIP is active and activation threshold are changing.   */
            isAnyChangeDetected = TRUE;
        }
    }

    if (isAnyChangeDetected)
    {
        /* HDPIP properties are changing. Flag all allocated buffers so */
        /* that next allocated ones will be done according to new       */
        /* HDPIP parameters.                                            */
        VIDBUFF_TagEveryBuffersAsToBeKilledAsap(BuffersHandle, VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p);

        /* Memorise the new FrameBuffersType.       */
        if (HDPIPParams_p->Enable)
        {
            VIDBUFF_Data_p->FrameBuffersType = VIDBUFF_BUFFER_FRAME_16BITS_PER_PIXEL;
        }
        else
        {
            VIDBUFF_Data_p->FrameBuffersType = VIDBUFF_BUFFER_FRAME_12BITS_PER_PIXEL;
        }
        /* And the HD-PIP properties.               */
        VIDBUFF_Data_p->HDPIPParams = *HDPIPParams_p;
    }

    /* Un-lock access to allocated frame buffers loop */
    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    return(ST_NO_ERROR);

} /* End of VIDBUFF_SetHDPIPParams() function. */

/*******************************************************************************
Name        : VIDBUFF_SetMemoryProfile
Description : Set memory profile
Parameters  : Video buffer manager handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if NULL pointer, or bad *Profile_p .
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_SetMemoryProfile(const VIDBUFF_Handle_t BuffersHandle, VIDCOM_InternalProfile_t * const Profile_p)
{
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;

    /* Check for bad parameters... */
    if (Profile_p == NULL)
    {
        /* Input parameter Profile_p incorrect. */
        return(ST_ERROR_BAD_PARAMETER);
    }
    if ( (Profile_p->NbMainFrameStore + VIDBUF_ADDED_FRAME_BUFFER) > VIDBUFF_Data_p->MaxFrameBuffers)
    {
        /* Too much frame buffers to take into account.  */
        return(ST_ERROR_BAD_PARAMETER);
    }
    if ( (Profile_p->NbSecondaryFrameStore + VIDBUF_ADDED_FRAME_BUFFER) > VIDBUFF_Data_p->MaxSecondaryFrameBuffers)
    {
        /* Too much frame buffers to take into account (Secondary).  */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Lock access to allocated frame buffers loop */
    semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    /* Test if the memory profile of previous allocated frame buffer has changed. */
    if ( (((Profile_p->ApiProfile.MaxWidth*Profile_p->ApiProfile.MaxHeight) != (VIDBUFF_Data_p->Profile.ApiProfile.MaxWidth*VIDBUFF_Data_p->Profile.ApiProfile.MaxHeight))
            || (Profile_p->ApiProfile.CompressionLevel != VIDBUFF_Data_p->Profile.ApiProfile.CompressionLevel)
            || (Profile_p->ApplicableDecimationFactor != VIDBUFF_Data_p->Profile.ApplicableDecimationFactor))
            && (VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p != NULL) )
    {
        /* Yes, we must flag all allocated frame buffers. */
        VIDBUFF_TagEveryBuffersAsToBeKilledAsap(BuffersHandle, VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p);
        VIDBUFF_TagEveryBuffersAsToBeKilledAsap(BuffersHandle, VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p);
#ifdef USE_EXTRA_FRAME_BUFFERS
        VIDBUFF_TagEveryBuffersAsToBeKilledAsap(BuffersHandle, VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p);
        VIDBUFF_TagEveryBuffersAsToBeKilledAsap(BuffersHandle, VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p);
#endif
    }

    /* Save the new memory profile. */
    ((VIDBUFF_Data_t *) BuffersHandle)->Profile = *Profile_p;

    VIDBUFF_Data_p->FrameBufferDynAlloc.Width = Profile_p->ApiProfile.MaxWidth;
    VIDBUFF_Data_p->FrameBufferDynAlloc.Height = Profile_p->ApiProfile.MaxHeight;
    VIDBUFF_Data_p->FrameBufferDynAlloc.NumberOfFrames = Profile_p->NbMainFrameStore;

    /* Allow to send IMPOSSIBLE_WITH_PROFILE_EVT again: structure proposed may be different with new profile */
    VIDBUFF_Data_p->IsNotificationOfImpossibleWithProfileAllowed = TRUE;

    /* Unlock access to allocated frame buffers loop */
    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    return(ST_NO_ERROR);
} /* End of VIDBUFF_SetMemoryProfile() function */

/*******************************************************************************
Name        : VIDBUFF_ForceDecimationFactor
Description :
Parameters  :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_ForceDecimationFactor(const VIDBUFF_Handle_t BuffersHandle, const STVID_DecimationFactor_t DecimationFactor)
{
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;

    VIDBUFF_Data_p->Profile.ApplicableDecimationFactor = DecimationFactor;

    return(ST_NO_ERROR);
} /* End of VIDBUFF_ForceDecimationFactor() function */

/*******************************************************************************
Name        : VIDBUFF_SetDisplayStatus
Description : Change display status of a picture
Parameters  : Video buffer manager handle, status, picture to update
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_SetDisplayStatus(const VIDBUFF_Handle_t BuffersHandle, const VIDBUFF_DisplayStatus_t DisplayStatus, VIDBUFF_PictureBuffer_t * const Picture_p)
{
    UNUSED_PARAMETER(BuffersHandle);

    if ((Picture_p == NULL) || (DisplayStatus > VIDBUFF_LAST_FIELD_ON_DISPLAY) )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Set status */
    Picture_p->Buffers.DisplayStatus = DisplayStatus;
    TrOverwrite(("\r\nPB 0x%x: Set Display Status %d", Picture_p, DisplayStatus));

    /* WARNING: When using the Overwrite, it would be wise to send a notification to VID_DEC in order to inform it that
                a picture buffer is "Last Field On Display" and thus available for Overwrite.
                Otherwise the buffer will be requested only at the next periodical wake up of VID_DEC which is a waste of time. */

    return(ST_NO_ERROR);
} /* End of VIDBUFF_SetDisplayStatus() function */


/* #ifdef ST_smoothbackward */
/* #if 0 */
/*******************************************************************************
Name        : VIDBUFF_UnlockNotDisplayedPictures
Description : Set all pictures of the buffer manager to UNLOCKED status
Parameters  : Should be used when switching from a positive to a negative speed
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
/* ST_ErrorCode_t VIDBUFF_UnlockNotDisplayedPictures(const VIDBUFF_Handle_t BuffersHandle) */
/* { */
    /* VIDBUFF_FrameBuffer_t * CurrentFrame_p; */
    /* VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle; */

    /* if (VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p == NULL) */ /* No frame allocated for decoder: can't allocate any ! */
    /* { */
        /* return(ST_ERROR_NO_MEMORY); */
    /* } */

    /* Lock access to allocated frame buffers loop */
    /* semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p); */

    /* Consider allocated frame buffers */
    /* CurrentFrame_p = VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p; */

    /* do */
    /* { */
        /* if ((CurrentFrame_p->FrameOrFirstFieldPicture_p->Buffers.PictureLockStatus != VIDBUFF_PICTURE_LOCK_STATUS_ON_DISPLAY) && */
                /* (CurrentFrame_p->FrameOrFirstFieldPicture_p->Buffers.PictureLockStatus != VIDBUFF_PICTURE_LOCK_STATUS_LAST_FIELD_ON_DISPLAY)) */
        /* { */
             /* CurrentFrame_p->FrameOrFirstFieldPicture_p = NULL; */
        /* } */

        /* Loop to search again for an unused frame */
        /* CurrentFrame_p = CurrentFrame_p->NextAllocated_p; */
    /* } while (CurrentFrame_p != VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p); */

    /* Un-lock access to allocated frame buffers loop */
    /* semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p); */

    /* return(ST_NO_ERROR); */

/* } */
/* #endif */
/* #endif */

/*******************************************************************************
Name        : VIDBUFF_Term
Description : Terminate buffer manager, undo all what was done at init
Parameters  :
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_INVALID_HANDLE if not initialised
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_Term(const VIDBUFF_Handle_t BuffersHandle)
{
    VIDBUFF_Data_t * VIDBUFF_Data_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U8 k;

    /* Find structure */
    VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;

    if (VIDBUFF_Data_p->ValidityCheck != VIDBUFF_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef USE_EXTRA_FRAME_BUFFERS
    /* De-allocate frame buffers which were not de-allocated by VIDBUFF_DeAllocateUnusedExtraBuffers() */
    DeAllocateExtraPrimaryFrameBufferPool(BuffersHandle);
    DeAllocateExtraSecondaryFrameBufferPool(BuffersHandle);
#endif /* USE_EXTRA_FRAME_BUFFERS */

    /* De-allocate frame buffers which were not de-allocated by VIDBUFF_DeAllocateUnusedMemoryOfProfile() */
    DeAllocateFrameBuffers(BuffersHandle);

    /* Terminate HAL */
    if (VIDBUFF_Data_p->HALBuffersHandle != NULL)
    {
        HALBUFF_Term(VIDBUFF_Data_p->HALBuffersHandle);
    }

    /* Un-subscribe and un-register events: this is done automatically by STEVT_Close() */

    /* Close instances opened at init */
    ErrorCode = STEVT_Close(VIDBUFF_Data_p->EventsHandle);

    /* De-allocate memory allocated at init */
    if (((void *) VIDBUFF_Data_p->FrameBuffers_p) != NULL)
    {
        SAFE_MEMORY_DEALLOCATE(VIDBUFF_Data_p->FrameBuffers_p, VIDBUFF_Data_p->CPUPartition_p,
                               VIDBUFF_Data_p->MaxFrameBuffers * sizeof(VIDBUFF_FrameBuffer_t));
    }
    if (((void *) VIDBUFF_Data_p->SecondaryFrameBuffers_p) != NULL)
    {
        SAFE_MEMORY_DEALLOCATE(VIDBUFF_Data_p->SecondaryFrameBuffers_p, VIDBUFF_Data_p->CPUPartition_p,
                               VIDBUFF_Data_p->MaxSecondaryFrameBuffers * sizeof(VIDBUFF_FrameBuffer_t));
    }

    if (((void *) VIDBUFF_Data_p->AllocatedPictureBuffersTable_p) != NULL)
    {
        /* Deallocate semaphores associated to each picture buffer
            (Warning: The number of semaphore deleted should be consistent with the number created by VIDBUFF_Init) */
        for (k = 0; k < (2 * (VIDBUFF_Data_p->MaxFrameBuffers + VIDBUFF_Data_p->MaxSecondaryFrameBuffers + MAX_EXTRA_FRAME_BUFFER_NBR)); k++)
        {
            if ((VIDBUFF_Data_p->AllocatedPictureBuffersTable_p)[k].Buffers.LockCounterSemaphore_p != NULL)
            {
                STOS_SemaphoreDelete(VIDBUFF_Data_p->CPUPartition_p, (VIDBUFF_Data_p->AllocatedPictureBuffersTable_p)[k].Buffers.LockCounterSemaphore_p);
            }
        }
        SAFE_MEMORY_DEALLOCATE(VIDBUFF_Data_p->AllocatedPictureBuffersTable_p, VIDBUFF_Data_p->CPUPartition_p,
                               2 * (VIDBUFF_Data_p->MaxFrameBuffers + VIDBUFF_Data_p->MaxSecondaryFrameBuffers + MAX_EXTRA_FRAME_BUFFER_NBR)* sizeof(VIDBUFF_PictureBuffer_t));
    }

    /* Lock access to data */
    semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    TermBufferManagerStructure(BuffersHandle);

    /* De-validate structure */
    VIDBUFF_Data_p->ValidityCheck = 0; /* not VIDBUFF_VALID_HANDLE */

    /* De-allocate last: data inside cannot be used after ! */
    SAFE_MEMORY_DEALLOCATE(VIDBUFF_Data_p, VIDBUFF_Data_p->CPUPartition_p, sizeof(VIDBUFF_Data_t));

    return(ErrorCode);
} /* End of VIDBUFF_Term() function */






/* Private Functions -------------------------------------------------------- */

/*******************************************************************************
Name        : UsePicture
Description : Take a picture structure from the table of allocacted ones
Parameters  : Buffer manager handle, frame buffer to associate to the picture structure
Assumptions : In frame buffer, make link to this picture just after calling this
              function (FrameOrFirstFieldPicture_p or NothingOrSecondFieldPicture_p pointer)
              Frame_p should not be NULL (otherwise links are spoiled)
Limitations :
Returns     : pointer to a picture structure (never NULL !)
*******************************************************************************/
static VIDBUFF_PictureBuffer_t * UsePicture(const VIDBUFF_Handle_t BuffersHandle,
                                           VIDBUFF_FrameBuffer_t * const Frame_p)
{
    U8 k = 0;
    VIDBUFF_PictureBuffer_t * Picture_p = ((VIDBUFF_Data_t *) BuffersHandle)->AllocatedPictureBuffersTable_p;

    if (Picture_p == NULL)
    {
        /* No picture exist */
        return(NULL);
    }

    do
    {
        if (!Picture_p->Buffers.IsInUse)
        {
            /* This Picture Buffer is available: We take it */
#ifdef STVID_DEBUG_TAKE_RELEASE
            VIDBUFF_PictureBufferShouldBeUnused(BuffersHandle, Picture_p);
#endif
            Picture_p->Buffers.IsInUse = TRUE;
            /* Picture structure free */
            Picture_p->FrameBuffer_p = Frame_p;
            /* Initialisations */
            ResetPictureBufferParams(BuffersHandle, Picture_p);

            return(Picture_p);
        }
        k ++;
        Picture_p ++;
    } while (k < (2 * (  ((VIDBUFF_Data_t *) BuffersHandle)->MaxFrameBuffers
                        +((VIDBUFF_Data_t *) BuffersHandle)->MaxSecondaryFrameBuffers)));

    /* Should not happen because enough pictures are allocated (2 by frame buffer) */
    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Running out of pictures: should be impossible because allocation is large enough !"));
    TrError(("\r\nError: No Picture buffer available!"));

    return(NULL);
} /* End fo UsePicture() function */


/*******************************************************************************
Name        : LeavePicture
Description : Release a picture structure
Parameters  : Buffer manager handle
Assumptions : In frame buffer, destroy link to this picture just after calling this function
Limitations :
Returns     : Nothing
*******************************************************************************/
static void LeavePicture(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const Picture_p)
{
    UNUSED_PARAMETER(BuffersHandle);

    if (Picture_p == NULL)
    {
        /* Error case */
        return;
    }

    /* Reset structure */
    Picture_p->Buffers.IsInUse = FALSE;
    Picture_p->FrameBuffer_p = NULL;
    ResetPictureBufferParams(BuffersHandle, Picture_p);

} /* End of LeavePicture() function */

/*******************************************************************************
Name        : ResetPictureBufferParams
Description : This function reset the Picture Buffer params to their default value.
              The fields "FrameBuffer_p" and "IsInUse" are not changed.
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void ResetPictureBufferParams(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const Picture_p)
{
    UNUSED_PARAMETER(BuffersHandle);

    if (Picture_p == NULL)
    {
        /* Error case */
        return;
    }

    /* Lock semaphore protecting access to the Lock counters */
    STOS_SemaphoreWait(Picture_p->Buffers.LockCounterSemaphore_p);

    Picture_p->Buffers.DisplayStatus = VIDBUFF_NOT_ON_DISPLAY;
    Picture_p->Buffers.IsInDisplayQueue = FALSE;
    Picture_p->Buffers.PictureLockCounter = 0 ;
    Picture_p->Buffers.IsLinkToFrameBufferToBeRemoved = FALSE;
    memset(Picture_p->Buffers.Module, 0, sizeof(VIDBUFF_ModuleCounter_t) * VIDCOM_MAX_MODULE_NBR);
    Picture_p->AssociatedDecimatedPicture_p = NULL;
#ifdef ST_smoothbackward
    Picture_p->SmoothBackwardPictureInfo_p = NULL;
#endif

    /* Release the semaphore protecting access to the Lock counters */
    STOS_SemaphoreSignal(Picture_p->Buffers.LockCounterSemaphore_p);

    /* Decimation is by default supposed to be "standard" */
    if (Picture_p->FrameBuffer_p != NULL)
    {
        Picture_p->FrameBuffer_p->AdvancedHDecimation = FALSE;
    }

} /* End of ResetPictureBufferParams() function */

/*******************************************************************************
Name        : NotifyFrameBufferList
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
#ifdef ST_OSLINUX
void NotifyFrameBufferList(STEVT_Handle_t EventsHandle, STEVT_EventID_t EventId, FrameBuffers_t * FrameBuffers_p, U32 NbFrameBuffers)
{
    VIDBUFF_FrameBufferList_t * FlattenedFBList_p;
    U32                         StructAlignedSize = ((sizeof(VIDBUFF_FrameBufferList_t) + 3)/4)*4;         /* Flattened data are aligned on U32 */

    if (   (FrameBuffers_p == NULL)
        || (NbFrameBuffers == 0))
    {
        return;
    }

    /* Allocates area for VIDBUFF_FrameBufferList_t + array content flattened after structrure, flattened data are aligned on U32 */
    FlattenedFBList_p = (VIDBUFF_FrameBufferList_t *)STOS_MemoryAllocate(NULL, StructAlignedSize + NbFrameBuffers*sizeof(FrameBuffers_t));
    if (FlattenedFBList_p != NULL)
    {
        FlattenedFBList_p->FrameBuffers_p = FrameBuffers_p;
        FlattenedFBList_p->NbFrameBuffers = NbFrameBuffers;
        memcpy((void *)((U32)FlattenedFBList_p + StructAlignedSize),
                        FrameBuffers_p,
                        NbFrameBuffers*sizeof(FrameBuffers_t));     /* Flatten data after structure content */

#ifdef DEBUG_FB_MAPPING
        {
            U32  i;
            for (i=0 ; i<NbFrameBuffers ; i++)
               STTBX_Report((STTBX_REPORT_LEVEL_INFO, "0x%.8x-> ", (U32)FrameBuffers_p[i].KernelAddress_p));
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "NULL\n"));
        }
#endif

        STEVT_NotifyWithSize(EventsHandle, EventId,
                             STEVT_EVENT_DATA_TYPE_CAST FlattenedFBList_p,
                             StructAlignedSize + NbFrameBuffers*sizeof(FrameBuffers_t));
        STOS_MemoryDeallocate(NULL, FlattenedFBList_p);
    }
}
#endif

/*******************************************************************************
Name        : NotifyNewFrameBuffers
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
#ifdef ST_OSLINUX
static ST_ErrorCode_t NotifyNewFrameBuffers(const VIDBUFF_Handle_t BuffersHandle,
                            FrameBuffers_t * FrameBuffers_p,
                            U32              NbFrameBuffers)
{
    VIDBUFF_Data_t    * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    U32                 i;

    if (   (FrameBuffers_p == NULL)
        || (NbFrameBuffers == 0))
    {
        return ErrorCode;
    }
    /* Map frame buffers regions */
    for (i=0 ; i<NbFrameBuffers ; i++)
    {
        /* Request region only for information while doing 'cat /proc/iomem' which lists all mapped regions */
        request_mem_region((U32)FrameBuffers_p[i].KernelAddress_p | REGION2,
                           FrameBuffers_p[i].Size,
                           "Frame buffer");

        /* Now that we have control over the memory region, remap it to */
        /* something safe for us to use in in writel/readl accesses. */
        FrameBuffers_p[i].MappedAddress_p = (void *)ioremap_nocache((U32)FrameBuffers_p[i].KernelAddress_p,
                                                                    FrameBuffers_p[i].Size);
        if (FrameBuffers_p[i].MappedAddress_p == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s: ioremap: failed to remap\n", __FUNCTION__));

            /* Modify NbFrameBuffers just to free the right number of mem region */
            NbFrameBuffers = i;
            ErrorCode = ST_ERROR_NO_MEMORY;
            break;
        }
    }

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Notify the new frame buffers */
#ifdef DEBUG_FB_MAPPING
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Notify New List: "));
#endif
        NotifyFrameBufferList(VIDBUFF_Data_p->EventsHandle,
                              VIDBUFF_Data_p->RegisteredEventsID[VIDBUFF_NEW_ALLOCATED_FRAMEBUFFERS_EVT_ID],
                              FrameBuffers_p,
                              NbFrameBuffers);
    }
    else
    {
        /* Error: unmap and release each region */
        for (i=0 ; i<NbFrameBuffers ; i++ )
        {
            if (FrameBuffers_p[i].KernelAddress_p == NULL)
            {
                /* This frame buffer field has not been set, continue to next one */
                continue;
            }

            /* unmap & release region */
            iounmap((void *)((U32)FrameBuffers_p[i].KernelAddress_p | REGION2));
            release_mem_region((U32)FrameBuffers_p[i].KernelAddress_p | REGION2, FrameBuffers_p[i].Size);
        }
    }

    return(ErrorCode);
}
#endif  /* LINUX */

/*******************************************************************************
Name        : NotifyRemovedFrameBuffers
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
#ifdef ST_OSLINUX
static ST_ErrorCode_t NotifyRemovedFrameBuffers(const VIDBUFF_Handle_t BuffersHandle,
                                                FrameBuffers_t * FrameBuffers_p,
                                                U32              NbFrameBuffers)
{
    VIDBUFF_Data_t    * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
    U32                 i;

    if (   (FrameBuffers_p == NULL)
        || (NbFrameBuffers == 0))
    {
        return ST_NO_ERROR;
    }

    /* Unmap frame buffers regions */
    for (i=0 ; i<NbFrameBuffers ; i++)
    {
        if (FrameBuffers_p[i].KernelAddress_p == NULL)
        {
            /* This frame buffer field has not been set, continue to next one */
            continue;
        }

        /* unmap & release region */
        iounmap((void *)((U32)FrameBuffers_p[i].KernelAddress_p | REGION2));
        release_mem_region((U32)FrameBuffers_p[i].KernelAddress_p | REGION2, FrameBuffers_p[i].Size);
    }

    /* Notify the new frame buffers */
#ifdef DEBUG_FB_MAPPING
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Notify Removed List: "));
#endif
    NotifyFrameBufferList(VIDBUFF_Data_p->EventsHandle,
                          VIDBUFF_Data_p->RegisteredEventsID[VIDBUFF_DEALLOCATED_FRAMEBUFFERS_EVT_ID],
                          FrameBuffers_p,
                          NbFrameBuffers);

    return(ST_NO_ERROR);
}
#endif  /* LINUX */

/*******************************************************************************
Name        : AllocateFrameBuffers
Description : Allocates frame buffers for a decoder, and links them into a linked
              loop of allocated frame buffers, pointed by AllocatedFrameBuffersLoop_p.
              It allocates secondary frame buffers if required too (according to the
              memory profile of video driver).
Parameters  : Buffer manager handle, allocation parameters, number of primary frame buffers.
Assumptions : Decimation modes and Compression modes are MUTUALLY EXCLUSIVE.
Limitations : The BufferSize field from AllocParams_p is not used.
Returns     : ST_ERROR_BAD_PARAMETER if too many buffers required
              ST_ERROR_NO_MEMORY if STAVMEM allocation failed
              ST_NO_ERROR if everything OK
*******************************************************************************/
static ST_ErrorCode_t AllocateFrameBuffers(const VIDBUFF_Handle_t BuffersHandle,
        const VIDBUFF_AllocateBufferParams_t * const AllocParams_p,
        const U32 NumberOfPrimary, const U32 NumberOfSecondary)
{
    HALBUFF_AllocateBufferParams_t HALPrimaryAllocParams;
    HALBUFF_AllocateBufferParams_t HALSecondaryAllocParams;
    U8 NumberToAllocate;
    U8 NumberAlreadyAllocated;
    U8 FrameBufferIndex;
    BOOL AllocationError;
    VIDBUFF_FrameBuffer_t * CurrentFrame_p, * FirstOfNewAllocated_p = NULL;
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
    U32 HeightAfterDecimationFactor, WidthAfterDecimationFactor;
    VIDBUFF_ReconstructionMode_t    ReconstructionMode = VIDBUFF_ONLY_MAIN_RECONSTRUCTION;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
#ifdef ST_OSLINUX
    FrameBuffers_t                * FrameBuffers_p = NULL;
    U32                             NbFrameBuffers = 0;
#endif

    if ((AllocParams_p == NULL) || (NumberOfPrimary == 0))
    {
        /* Errors */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Save of width and height of the picture size, to calculate the corresponding size of frame buffers.        */
    /* Those valuse are rounded up to the nearest modulo 16 (macroblock size in pixels).                          */

    WidthAfterDecimationFactor  = (((AllocParams_p->PictureWidth + 15) / 16) * 16);
    HeightAfterDecimationFactor = (((AllocParams_p->PictureHeight + 15) / 16) * 16);

    if ((NumberOfPrimary) > (U32)(VIDBUFF_Data_p->MaxFrameBuffers - VIDBUF_ADDED_FRAME_BUFFER))
    {
        /* Too much buffers to allocate */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if ((NumberOfSecondary) > (U32)(VIDBUFF_Data_p->MaxSecondaryFrameBuffers - VIDBUF_ADDED_FRAME_BUFFER))
    {
        /* Too much buffers to allocate */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Apply the compression level and decimation factor to the buffer size. It's assumed that if value are
       the defaults one, nothing will be done to those buffer sizes. */
    if (HALBUFF_ApplyCompressionLevelToPictureSize(VIDBUFF_Data_p->HALBuffersHandle,
            VIDBUFF_Data_p->Profile.ApiProfile.CompressionLevel,
            &WidthAfterDecimationFactor, &HeightAfterDecimationFactor) != ST_NO_ERROR)
    {
        /* Error in input parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (((VIDBUFF_Data_p->Profile.ApiProfile.DecimationFactor & STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2) == STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2) &&
        (VIDBUFF_Data_p->Profile.ApiProfile.MaxWidth < 1920))
    {
        /* Error in input parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (HALBUFF_ApplyDecimationFactorToPictureSize(VIDBUFF_Data_p->HALBuffersHandle,
                                                   VIDBUFF_Data_p->Profile.ApiProfile.DecimationFactor,
                                                   &WidthAfterDecimationFactor, &HeightAfterDecimationFactor) != ST_NO_ERROR)
    {
        /* Error in input parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check case of Picture(s) of this frame has been removed from display queue since last deallocation of unused memory of profile: remove it now */
    VIDBUFF_DeAllocateUnusedMemoryOfProfile(BuffersHandle);

    /* Lock access to allocated frame buffers loop */
    semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);


    /* Determine the number of buffers already allocated */
    NumberAlreadyAllocated = 0;
    CurrentFrame_p = VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p;
    if (CurrentFrame_p != NULL)
    {
        /* There are buffers already allocated: count them */
        do
        {
            /* Test if the allocated frame buffer has got the good memory profile. */
            if (!(CurrentFrame_p->ToBeKilledAsSoonAsPossible))
            {
                /* Yes, we can count it. */
                NumberAlreadyAllocated ++;
            }
            /* Get the next frame buffer in the loop. */
            CurrentFrame_p = CurrentFrame_p->NextAllocated_p;

        } while ((CurrentFrame_p != VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p) && (CurrentFrame_p != NULL));
    }

    /* Check if there is not too much buffers ! This is the case only if display
    stopped locking 2 buffers, and new profile has only 1 buffer. */
    if (NumberOfPrimary < NumberAlreadyAllocated)
    {
        /* Problem of too much buffers: cancel with error... */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else if (NumberOfPrimary == NumberAlreadyAllocated)
    {
        /* All buffers already allocated: nothing to do */
        ErrorCode = ST_NO_ERROR;
    }
    else
    {
        NumberToAllocate = NumberOfPrimary - NumberAlreadyAllocated;

        /* Initialization of allocation parameters for primary frame buffers. */
        HALPrimaryAllocParams.AllocationMode = AllocParams_p->AllocationMode;
        HALPrimaryAllocParams.BufferType     = AllocParams_p->BufferType;
        if ((VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2) ||
            (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2) ||
            (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2))
        {
            /* For MPEG4-P2, we need 64 bits alligned buffers */
            HALPrimaryAllocParams.PictureWidth   = ((AllocParams_p->PictureWidth + 32 + 63) / 64) * 64;
            HALPrimaryAllocParams.PictureHeight  = AllocParams_p->PictureHeight + 32;
        }
        else if (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS )
        {
            /* Width : once its MB boundary size is calculated, the size is increased to the second 64 bytes boundary value */
            HALPrimaryAllocParams.PictureWidth = (((((AllocParams_p->PictureWidth+15)/16)*16+64)+63)/64)*64;
            /* Height : once its MB boundary size is calculated, the size is increased of 64 bytes */
            HALPrimaryAllocParams.PictureHeight = (((AllocParams_p->PictureHeight+15)/16))*16+64;
        }
        else
        {
        HALPrimaryAllocParams.PictureWidth   = AllocParams_p->PictureWidth;
        HALPrimaryAllocParams.PictureHeight  = AllocParams_p->PictureHeight;
        }

        TrMemAlloc(("\r\nAllocate %d Frame buffers [%d;%d]",
                NumberToAllocate, HALPrimaryAllocParams.PictureWidth, HALPrimaryAllocParams.PictureHeight));
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Allocate %d Frame buffers [%d;%d]",
                NumberToAllocate, HALPrimaryAllocParams.PictureWidth, HALPrimaryAllocParams.PictureHeight));

        if (VIDBUFF_Data_p->Profile.ApiProfile.DecimationFactor == STVID_DECIMATION_FACTOR_NONE)
        {
            /* No decimation is allowed. Only MAIN reconstruction is available. */
            ReconstructionMode = VIDBUFF_ONLY_MAIN_RECONSTRUCTION;
        }
        else
        {
            if ( (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7015_DIGITAL_INPUT) ||
                 (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7020_DIGITAL_INPUT) ||
                 (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7710_DIGITAL_INPUT) ||
                 (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7100_DIGITAL_INPUT) ||
                 (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_DIGITAL_INPUT) ||
                 (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7200_DIGITAL_INPUT) ||
                 (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT) )

            {
                /* Decimation is allowed for D1 device. Only SECONDARY              */
                /* reconstruction mode is allowed.                                  */
                ReconstructionMode = VIDBUFF_ONLY_SECONDARY_RECONSTRUCTION;
            }
            else
            {
                /* Decimation is allowed for general case. Both MAIN and SECONDARY  */
                /* reconstruction mode are allowed.                                 */
                ReconstructionMode = VIDBUFF_MAIN_AND_SECONDARY_RECONSTRUCTION;
            }
        }
        if ((VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2) ||
            (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2) ||
            (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS) ||
            (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2))
        {
            ReconstructionMode = VIDBUFF_MAIN_AND_SECONDARY_RECONSTRUCTION;
        }


        if (VIDBUFF_Data_p->HDPIPParams.Enable)
        {
            /* HD-PIP is enable and buffers will be (re-)allocated. Check if any    */
            /* optimization can be done (thanks to HD-PIP use).                     */
            if ( (VIDBUFF_Data_p->HDPIPParams.HeightThreshold < VIDBUFF_Data_p->Profile.ApiProfile.MaxHeight) ||
                 (VIDBUFF_Data_p->HDPIPParams.WidthThreshold  < VIDBUFF_Data_p->Profile.ApiProfile.MaxWidth ) )
            {
                /* HD-PIP Threshold are aboce memort profile. It may be active.     */
                if ( (VIDBUFF_Data_p->HDPIPParams.WidthThreshold * VIDBUFF_Data_p->HDPIPParams.HeightThreshold) <
                     ((VIDBUFF_Data_p->Profile.ApiProfile.MaxWidth * VIDBUFF_Data_p->Profile.ApiProfile.MaxHeight) / 4) )
                {
                    /* As HD-PIP can be active, Frame buffers need less memory than */
                    /* allowed memory profile. 1/4 will be sufficient.              */
                    HALPrimaryAllocParams.PictureWidth >>= 2;
                }
                else
                {
                    /* Use HD-PIP threshold as frame buffer size.                   */
                    if ( (VIDBUFF_Data_p->HDPIPParams.WidthThreshold * VIDBUFF_Data_p->HDPIPParams.HeightThreshold) <
                         (VIDBUFF_Data_p->Profile.ApiProfile.MaxHeight * VIDBUFF_Data_p->Profile.ApiProfile.MaxWidth ) )
                    {
                        HALPrimaryAllocParams.PictureWidth   = VIDBUFF_Data_p->HDPIPParams.WidthThreshold;
                        HALPrimaryAllocParams.PictureHeight  = VIDBUFF_Data_p->HDPIPParams.HeightThreshold;
                    }
                }

                TrMemAlloc(("\r\n  Alloc modified -> [%d;%d]",
                        HALPrimaryAllocParams.PictureWidth, HALPrimaryAllocParams.PictureHeight));

                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Alloc modified -> [%d;%d]",
                        HALPrimaryAllocParams.PictureWidth, HALPrimaryAllocParams.PictureHeight));
            }
        }

        /* Allocate new frame buffers in the list FirstOfNewAllocated_p */
        if ( (ReconstructionMode == VIDBUFF_ONLY_MAIN_RECONSTRUCTION) ||
             (ReconstructionMode == VIDBUFF_MAIN_AND_SECONDARY_RECONSTRUCTION) )
        {
#ifdef ST_OSLINUX
            NbFrameBuffers = 0;
            FrameBuffers_p = (FrameBuffers_t *)STOS_MemoryAllocate(NULL, NumberToAllocate*sizeof(FrameBuffers_t));
#endif

            do
            {
                /* Search a frame buffer structure */
                FrameBufferIndex = 0;
                while ((FrameBufferIndex < VIDBUFF_Data_p->MaxFrameBuffers) && (VIDBUFF_Data_p->FrameBuffers_p[FrameBufferIndex].AllocationIsDone))
                {
                    FrameBufferIndex++;
                }
                if (FrameBufferIndex == VIDBUFF_Data_p->MaxFrameBuffers)
                {
                    /* Error: No frame buffer structure free ! */
                    AllocationError = TRUE;
                }
                else
                {
                    CurrentFrame_p = VIDBUFF_Data_p->FrameBuffers_p + FrameBufferIndex;

                    /* Allocate required frame buffers */
                    AllocationError = FALSE;
                    /* Remember now the current decimation.                                 */
                    /*CurrentFrame_p->DecimationFactor = VIDBUFF_Data_p->Profile.ApplicableDecimationFactor;*/
                    /* And the available reconstruction mode.                               */
                    /*CurrentFrame_p->AvailableReconstructionMode = AvailableReconstructionMode;*/
                    if (HALBUFF_AllocateFrameBuffer(VIDBUFF_Data_p->HALBuffersHandle, &HALPrimaryAllocParams, &(CurrentFrame_p->Allocation), HALBUFF_FRAMEBUFFER_PRIMARY) != ST_NO_ERROR)
                    {
                        /* Error: Allocation could not be performed ! */
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Allocation of a frame buffer failed: de-allocating all and exiting..."));
                        TrError(("\r\nAllocation of a frame buffer failed: de-allocating all and exiting..."));
                        AllocationError = TRUE;
                    }
                    else
                    {
                        TrMemAlloc(("\r\nAllocated frame buffer 0x%x at address 0x%08x", CurrentFrame_p, CurrentFrame_p->Allocation.Address_p));
                        TrMemAlloc(("\t  Total: 0x%08x, Y: 0x%08x, C: 0x%08x",
                                                    CurrentFrame_p->Allocation.TotalSize,
                                                    CurrentFrame_p->Allocation.TotalSize-CurrentFrame_p->Allocation.Size2,
                                                    CurrentFrame_p->Allocation.Size2));
                        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Allocated frame buffer 0x%x at address 0x%08x", CurrentFrame_p, CurrentFrame_p->Allocation.Address_p));
                        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "\tTotal: 0x%08x, Y: 0x%08x, C: 0x%08x",
                                                    CurrentFrame_p->Allocation.TotalSize,
                                                    CurrentFrame_p->Allocation.TotalSize-CurrentFrame_p->Allocation.Size2,
                                                    CurrentFrame_p->Allocation.Size2));
                        CurrentFrame_p->AvailableReconstructionMode = VIDBUFF_MAIN_RECONSTRUCTION;

#ifdef ST_OSLINUX
                        FrameBuffers_p[NbFrameBuffers].KernelAddress_p = CurrentFrame_p->Allocation.Address_p;
                        FrameBuffers_p[NbFrameBuffers].Size = CurrentFrame_p->Allocation.TotalSize;
                        NbFrameBuffers ++;
#endif
                    }
                }

                if (AllocationError)
                {
#ifdef ST_OSLINUX
                    if (FrameBuffers_p != NULL)
                    {
                        /* Error occured, newly allocated FB will not be notified, free memory ... */
                        STOS_MemoryDeallocate(NULL, FrameBuffers_p);
                    }
#endif
                    /* De-allocate ALL previously allocated buffers and quit with error */
                    CurrentFrame_p = FirstOfNewAllocated_p;
                    while (CurrentFrame_p != NULL)
                    {
                        /* De-allocate frame */
                        CurrentFrame_p->AllocationIsDone = FALSE;
                        CurrentFrame_p->FrameBufferType = VIDBUFF_UNKNOWN_FRAME_BUFFER_TYPE;

                        TrError(("\r\nDeAllocating primary frame buffer from address 0x%08x", CurrentFrame_p->Allocation.Address_p));
                        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DeAllocating primary frame buffer from address 0x%08x", CurrentFrame_p->Allocation.Address_p));
                        HALBUFF_DeAllocateFrameBuffer(VIDBUFF_Data_p->HALBuffersHandle, &(CurrentFrame_p->Allocation), HALBUFF_FRAMEBUFFER_PRIMARY);

                        CurrentFrame_p->CompressionLevel                    = STVID_COMPRESSION_LEVEL_NONE;
                        CurrentFrame_p->DecimationFactor                    = STVID_DECIMATION_FACTOR_NONE;
                        CurrentFrame_p->AvailableReconstructionMode         = VIDBUFF_NONE_RECONSTRUCTION;
                        CurrentFrame_p->HasDecodeToBeDoneWhileDisplaying    = FALSE;
                        CurrentFrame_p->AdvancedHDecimation = FALSE;

                        /* Get the next frame buffer. */
                        FirstOfNewAllocated_p = CurrentFrame_p->NextAllocated_p;
                        CurrentFrame_p->NextAllocated_p = NULL; /* facultative */
                        CurrentFrame_p = FirstOfNewAllocated_p;
                    }
                    /* Un-lock access to allocated frame buffers loop */
                    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);
                    return(ST_ERROR_NO_MEMORY);
                }

                /* Allocation successful: build new bank of frames */
                CurrentFrame_p->AllocationIsDone = TRUE;
                CurrentFrame_p->FrameBufferType = VIDBUFF_PRIMARY_FRAME_BUFFER;
                CurrentFrame_p->FrameOrFirstFieldPicture_p    = NULL;
                CurrentFrame_p->NothingOrSecondFieldPicture_p = NULL;
                CurrentFrame_p->CompressionLevel = STVID_COMPRESSION_LEVEL_NONE;
                CurrentFrame_p->DecimationFactor = STVID_DECIMATION_FACTOR_NONE;
                CurrentFrame_p->AdvancedHDecimation = FALSE;

                CurrentFrame_p->HasDecodeToBeDoneWhileDisplaying    = FALSE;

                /* Append allocated buffer to decoder buffer bank */
                CurrentFrame_p->NextAllocated_p = FirstOfNewAllocated_p;
                FirstOfNewAllocated_p = CurrentFrame_p;

                NumberToAllocate--;

            } while (NumberToAllocate != 0);

            /* New allocations successful: add list of new allocated frames to AllocatedFrameBuffersLoop_p */
            while (CurrentFrame_p->NextAllocated_p != NULL)
            {
                /* Look for last frame of list */
                CurrentFrame_p = CurrentFrame_p->NextAllocated_p;
            }
            /* Last frame of the new list found: insert it into AllocatedFrameBuffersLoop_p */
            if (VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p == NULL)
            {
                /* AllocatedFrameBuffersLoop_p was empty: make list a loop */
                CurrentFrame_p->NextAllocated_p = FirstOfNewAllocated_p;
                VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p = FirstOfNewAllocated_p;
            }
            else
            {
                /* AllocatedFrameBuffersLoop_p was not empty: make new list point to loop, open loop and make it again */
                CurrentFrame_p->NextAllocated_p = VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p->NextAllocated_p;
                VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p->NextAllocated_p = FirstOfNewAllocated_p;
            }
            /* Update number fo allocated frames */
            VIDBUFF_Data_p->NumberOfAllocatedFrameBuffers += (NumberOfPrimary - NumberAlreadyAllocated);

#ifdef ST_OSLINUX
            /* Notify only newly primary allocated FB */
            NotifyNewFrameBuffers(BuffersHandle, FrameBuffers_p, NbFrameBuffers);
            if (FrameBuffers_p != NULL)
            {
                STOS_MemoryDeallocate(NULL, FrameBuffers_p);
            }
#endif
        } /* NumberOfBuffers >= NumberAlreadyAllocated */
    }


    /* And now, the decimated buffers */
    /*--------------------------------*/
    FirstOfNewAllocated_p = NULL;
    /* Determine the number of buffers already allocated */
    NumberAlreadyAllocated = 0;
    CurrentFrame_p = VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p;
    if (CurrentFrame_p != NULL)
    {
        /* There are buffers already allocated: count them */
        do
        {
            /* Test if the allocated frame buffer has got the good memory profile. */
            if (!(CurrentFrame_p->ToBeKilledAsSoonAsPossible))
            {
                /* Yes, we can count it. */
                NumberAlreadyAllocated ++;
            }
            /* Get the next frame buffer in the loop. */
            CurrentFrame_p = CurrentFrame_p->NextAllocated_p;

        } while ((CurrentFrame_p != VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p)
                && (CurrentFrame_p != NULL));
    }

    /* Check if there is not too much buffers ! This is the case only if display
    stopped locking 2 buffers, and new profile has only 1 buffer. */
    if (NumberOfSecondary < NumberAlreadyAllocated)
    {
        /* Problem of too much buffers: cancel with error... */
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else if (NumberOfSecondary == NumberAlreadyAllocated)
    {
        /* All buffers already allocated: nothing to do */
        ErrorCode = ST_NO_ERROR;
    }
    else
    {
        NumberToAllocate = NumberOfSecondary - NumberAlreadyAllocated;

        /* Initialization of allocation parameters for secondary frame buffers. */
        HALSecondaryAllocParams.AllocationMode = AllocParams_p->AllocationMode;
        HALSecondaryAllocParams.BufferType     = AllocParams_p->BufferType;

        if ((VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2) ||
            (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2) ||
            (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2))
        {
            /* For MPEG4-P2, we need even number of macroblocks */
            HALSecondaryAllocParams.PictureWidth   = (((WidthAfterDecimationFactor + 15)/16 + 1) / 2) * 32;
            HALSecondaryAllocParams.PictureHeight  = HeightAfterDecimationFactor;
        }
        else if (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS)
        {
            /* PLE_TO_DO AVS needs a specific allocation because of the specific AVS firware needs */
            /* Current allocation for decimated is for example 720*576 compatible */
            HALSecondaryAllocParams.PictureWidth   = WidthAfterDecimationFactor ;
            HALSecondaryAllocParams.PictureHeight  = HeightAfterDecimationFactor;
        }
        else
        {
        HALSecondaryAllocParams.PictureWidth   = WidthAfterDecimationFactor;
        HALSecondaryAllocParams.PictureHeight  = HeightAfterDecimationFactor;
        }

        TrMemAlloc(("\r\nAllocate %d Secondary Frame buffers [%d;%d]",
                NumberToAllocate, HALSecondaryAllocParams.PictureWidth, HALSecondaryAllocParams.PictureHeight));

        if ( (ReconstructionMode == VIDBUFF_ONLY_SECONDARY_RECONSTRUCTION) ||
             (ReconstructionMode == VIDBUFF_MAIN_AND_SECONDARY_RECONSTRUCTION) )
        {
#ifdef ST_OSLINUX
            NbFrameBuffers = 0;
            FrameBuffers_p = (FrameBuffers_t *)STOS_MemoryAllocate(NULL, NumberToAllocate*sizeof(FrameBuffers_t));
#endif

            /* Allocate new frame buffers in the list FirstOfNewAllocated_p */
            do
            {
                /* Search a frame buffer structure */
                FrameBufferIndex = 0;
                while ((FrameBufferIndex < VIDBUFF_Data_p->MaxSecondaryFrameBuffers)
                && (VIDBUFF_Data_p->SecondaryFrameBuffers_p[FrameBufferIndex].AllocationIsDone))
                {
                    FrameBufferIndex++;
                }
                if (FrameBufferIndex == VIDBUFF_Data_p->MaxSecondaryFrameBuffers)
                {
                    /* Error: No frame buffer structure free ! */
                    AllocationError = TRUE;
                }
                else
                {
                    CurrentFrame_p = VIDBUFF_Data_p->SecondaryFrameBuffers_p + FrameBufferIndex;

                    /* Allocate required frame buffers */
                    AllocationError = FALSE;
                    /* Remember now the current decimation.                                 */
                    /*CurrentFrame_p->DecimationFactor = VIDBUFF_Data_p->Profile.ApplicableDecimationFactor;*/
                    /* And the available reconstruction mode.                               */
                    /*CurrentFrame_p->AvailableReconstructionMode = AvailableReconstructionMode;*/

                    if (HALBUFF_AllocateFrameBuffer(VIDBUFF_Data_p->HALBuffersHandle, &HALSecondaryAllocParams, &(CurrentFrame_p->Allocation), HALBUFF_FRAMEBUFFER_DECIMATED) != ST_NO_ERROR)
                    {
                        /* Error: Allocation could not be performed ! */
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Allocation of a frame buffer failed: de-allocating all and exiting..."));
                        TrError(("\r\nAllocation of a frame buffer failed: de-allocating all and exiting..."));
                        AllocationError = TRUE;
                    }
                    else
                    {
                        TrMemAlloc(("\r\nAllocated secondary frame buffer at address 0x%08x", CurrentFrame_p->Allocation.Address_p));
                        TrMemAlloc(("\t  Total: 0x%08x, Y: 0x%08x, C: 0x%08x",
                                                    CurrentFrame_p->Allocation.TotalSize,
                                                    CurrentFrame_p->Allocation.TotalSize-CurrentFrame_p->Allocation.Size2,
                                                    CurrentFrame_p->Allocation.Size2));
                        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Allocated secondary frame buffer at address 0x%08x", CurrentFrame_p->Allocation.Address_p));
                        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "\tTotal: 0x%08x, Y: 0x%08x, C: 0x%08x",
                                                    CurrentFrame_p->Allocation.TotalSize,
                                                    CurrentFrame_p->Allocation.TotalSize-CurrentFrame_p->Allocation.Size2,
                                                    CurrentFrame_p->Allocation.Size2));
                        CurrentFrame_p->AvailableReconstructionMode = VIDBUFF_SECONDARY_RECONSTRUCTION;

#ifdef ST_OSLINUX
                        FrameBuffers_p[NbFrameBuffers].KernelAddress_p = CurrentFrame_p->Allocation.Address_p;
                        FrameBuffers_p[NbFrameBuffers].Size = CurrentFrame_p->Allocation.TotalSize;
                        NbFrameBuffers ++;
#endif
                    }
                }

                if (AllocationError)
                {
#ifdef ST_OSLINUX
                    if (FrameBuffers_p != NULL)
                    {
                        /* Error occured, newly allocated FB will not be notified, free memory ... */
                        STOS_MemoryDeallocate(NULL, FrameBuffers_p);
                    }
#endif
                    /* De-allocate previously allocated buffers (decimated) and quit with error */
                    CurrentFrame_p = FirstOfNewAllocated_p;
                    while (CurrentFrame_p != NULL)
                    {
                        /* De-allocate frame */
                        CurrentFrame_p->AllocationIsDone = FALSE;
                        CurrentFrame_p->FrameBufferType = VIDBUFF_UNKNOWN_FRAME_BUFFER_TYPE;

                        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DeAllocating decimated frame buffer from address 0x%08x", CurrentFrame_p->Allocation.Address_p));
                        TrMemAlloc(("\r\nDeAllocating decimated frame buffer from address 0x%08x", CurrentFrame_p->Allocation.Address_p));
                        HALBUFF_DeAllocateFrameBuffer(VIDBUFF_Data_p->HALBuffersHandle, &(CurrentFrame_p->Allocation), HALBUFF_FRAMEBUFFER_DECIMATED);
                        CurrentFrame_p->CompressionLevel                    = STVID_COMPRESSION_LEVEL_NONE;
                        CurrentFrame_p->DecimationFactor                    = STVID_DECIMATION_FACTOR_NONE;
                        CurrentFrame_p->AvailableReconstructionMode         = VIDBUFF_NONE_RECONSTRUCTION;
                        CurrentFrame_p->HasDecodeToBeDoneWhileDisplaying    = FALSE;
                        CurrentFrame_p->AdvancedHDecimation = FALSE;

                        /* Get the next frame buffer. */
                        FirstOfNewAllocated_p = CurrentFrame_p->NextAllocated_p;
                        CurrentFrame_p->NextAllocated_p = NULL; /* facultative */
                        CurrentFrame_p = FirstOfNewAllocated_p;
                    }
/*                    VIDBUFF_Data_p->NumberOfSecondaryAllocatedFrameBuffers = 0;*/
                    /* Un-lock access to allocated frame buffers loop */
                    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);
                    /* De-allocate previously allocated buffers (Primary) and quit with error */
                    /* We need to call VIDBUFF_DeAllocateUnusedMemoryOfProfile for this because at this step we need to separate newly allocated and already existing frame buffers */
                    VIDBUFF_DeAllocateUnusedMemoryOfProfile(BuffersHandle);
                    return(ST_ERROR_NO_MEMORY);
                }

                /* Allocation successful: build new bank of frames */
                CurrentFrame_p->AllocationIsDone                = TRUE;
                CurrentFrame_p->FrameBufferType                 = VIDBUFF_SECONDARY_FRAME_BUFFER;
                CurrentFrame_p->FrameOrFirstFieldPicture_p      = NULL;
                CurrentFrame_p->NothingOrSecondFieldPicture_p   = NULL;

                CurrentFrame_p->CompressionLevel = VIDBUFF_Data_p->Profile.ApiProfile.CompressionLevel;
                CurrentFrame_p->DecimationFactor = VIDBUFF_Data_p->Profile.ApplicableDecimationFactor;

                CurrentFrame_p->HasDecodeToBeDoneWhileDisplaying    = FALSE;
                  CurrentFrame_p->AdvancedHDecimation = FALSE;

                /* Append allocated buffer to decoder buffer bank */
                CurrentFrame_p->NextAllocated_p = FirstOfNewAllocated_p;
                FirstOfNewAllocated_p = CurrentFrame_p;

                NumberToAllocate--;

            } while (NumberToAllocate != 0);

            /* New allocations successful: add list of new allocated frames to AllocatedFrameBuffersLoopLockingSemaphore_p*/
            while (CurrentFrame_p->NextAllocated_p != NULL)
            {
                /* Look for last frame of list */
                CurrentFrame_p = CurrentFrame_p->NextAllocated_p;
            }
            /* Last frame of the new list found: insert it into AllocatedFrameBuffersLoopLockingSemaphore_p*/
            if (VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p== NULL)
            {
                /* AllocatedFrameBuffersLoopLockingSemaphore_pwas empty: make list a loop */
                CurrentFrame_p->NextAllocated_p = FirstOfNewAllocated_p;
                VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p= FirstOfNewAllocated_p;
            }
            else
            {
                /* AllocatedFrameBuffersLoopLockingSemaphore_pwas not empty: make new list point to loop, open loop and make it again */
                CurrentFrame_p->NextAllocated_p = VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p->NextAllocated_p;
                VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p->NextAllocated_p = FirstOfNewAllocated_p;
            }
            /* Update number fo allocated frames */
            VIDBUFF_Data_p->NumberOfSecondaryAllocatedFrameBuffers+= (NumberOfSecondary - NumberAlreadyAllocated);

#ifdef ST_OSLINUX
            /* Notify only newly secondary allocated FB */
            NotifyNewFrameBuffers(BuffersHandle, FrameBuffers_p, NbFrameBuffers);
            if (FrameBuffers_p != NULL)
            {
                STOS_MemoryDeallocate(NULL, FrameBuffers_p);
            }
#endif
        } /* NumberOfBuffers >= NumberAlreadyAllocated */
    }

    /* Un-lock access to allocated frame buffers loop */
    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    /* Initialise picture structures */

    return(ErrorCode);
} /* End of AllocateFrameBuffers() function */

/*******************************************************************************
Name        : VIDBUFF_ClearFrameBuffer
Description : Fills one frame buffer with a pattern
Parameters  : Video buffer manager handle
Assumptions : The memory profile is supposed to be good (MaxWeight, MaxHeight
              and NbFrameStore not null).
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER if too many buffers required
              STVID_ERROR_MEMORY_ACCESS if the buffer can't be cleared
              ST_NO_ERROR if everything OK
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_ClearFrameBuffer(const VIDBUFF_Handle_t BuffersHandle,
                                        VIDBUFF_BufferAllocationParams_t * const BufferParams_p,
                                        const STVID_ClearParams_t * const ClearFrameBufferParams_p)
{
    return(HALBUFF_ClearFrameBuffer(((VIDBUFF_Data_t *) BuffersHandle)->HALBuffersHandle, BufferParams_p, ClearFrameBufferParams_p));
} /* End of VIDBUFF_ClearFrameBuffer() function */

/*******************************************************************************
Name        : VIDBUFF_TagEveryBuffersAsToBeKilledAsap
Description : Tag every Frame Buffers of a given loop as to be killed asap.
Parameters  : Buffer manager handle, frame buffer loop
Assumptions :
Limitations : Call to this function must be protected by AllocatedFrameBuffersLoopLockingSemaphore_p
Returns     : Nothing
*******************************************************************************/
static void VIDBUFF_TagEveryBuffersAsToBeKilledAsap(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_FrameBuffer_t * FrameBufferPool_p)
{
    VIDBUFF_FrameBuffer_t   * CurrentFrame_p;
    UNUSED_PARAMETER(BuffersHandle);

    if (FrameBufferPool_p == NULL)
    {
        /* Nothing to do */
        return;
    }

    /* Initialize the frame buffer pointer to the first one of the allocated loop */
    CurrentFrame_p = FrameBufferPool_p;

    do
    {
        /* Flag all frame buffers of the loop */
        CurrentFrame_p->ToBeKilledAsSoonAsPossible = TRUE;
        TrMain(("\r\nMark FB 0x%x ToBeKilledASAP", CurrentFrame_p));

        /* A FB should never be tagged as "to be killed asap" if it is not in use (otherwise it will never be destroyed)
        if (!(HasAPictureLocked(*CurrentFrame_p)))
        {
            TrError(("\r\nError! FB 0x%x unused but tagged ToBeKilledASAP!", CurrentFrame_p));
        }*/

        CurrentFrame_p = CurrentFrame_p->NextAllocated_p;
    } while ((CurrentFrame_p != FrameBufferPool_p) && (CurrentFrame_p != NULL));

    if (CurrentFrame_p == NULL)
    {
        TrError(("\r\nCritical Error! A Frame Buffer loop seems to be broken!"));
    }

}/* End of VIDBUFF_TagEveryBuffersAsToBeKilledAsap() function */

/*******************************************************************************
Name        : CheckIfToBeKilledAsSoonAsPossibleAndRemove
Description : Check if picture has frame buffer to be removed and remove it if YES
Parameters  : Buffer manager handle, frame buffer
Assumptions :
Limitations : Call to this function must be protected by AllocatedFrameBuffersLoopLockingSemaphore_p
Returns     : Nothing
*******************************************************************************/
static void CheckIfToBeKilledAsSoonAsPossibleAndRemove(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const Picture_p)
{
    VIDBUFF_FrameBuffer_t   * CurrentFrame_p;
    ST_ErrorCode_t  ErrorCode;
#ifdef ST_OSLINUX
    VIDBUFF_Data_t *            VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
#endif

    if ((Picture_p == NULL) || (Picture_p->FrameBuffer_p == NULL))
    {
        /* Nothing to do */
        return;
    }

    /* Retrieve the frame buffer corresponding to this picture */
    CurrentFrame_p = Picture_p->FrameBuffer_p;

    /* Check if this picture belongs to a frame buffer that should be deleted as soon as possible. */
    if (!CurrentFrame_p->ToBeKilledAsSoonAsPossible)
    {
        /* This Frame Buffer doesn't need to be killed: Nothing to do */
        return;
    }

    /* The Frame Buffer can be deallocated only if no associated Picture Buffer is in use */
   if (HasAPictureLocked(*CurrentFrame_p) )
   {
        TrDealloc(("\r\nFB 0x%08x can not be killed yet (a Picture Buffer is still used)", CurrentFrame_p));
   }
   else
   {
#ifdef ST_OSLINUX
        FrameBuffers_t    * FrameBuffers_p = NULL;
        U32                 NbFrameBuffers = 0;
#endif

        TrDealloc(("\r\nFB 0x%08x can be killed now", CurrentFrame_p));

#ifdef ST_OSLINUX
        /* Allocates max possible FB entries (primary + secondary) */
        FrameBuffers_p = (FrameBuffers_t *)STOS_MemoryAllocate(NULL, (VIDBUFF_Data_p->MaxFrameBuffers + VIDBUFF_Data_p->MaxSecondaryFrameBuffers)*sizeof(FrameBuffers_t));
#endif

        ErrorCode = RemoveFrameBufferFromFrameBufferLoop(BuffersHandle, CurrentFrame_p);
        if (ErrorCode != ST_NO_ERROR)
        {
            TrError(("\r\nCritical Error! Frame Buffer 0x%x could not be removed from the loop!!!", CurrentFrame_p));
            /* We should deallocate it anyway */
        }

#ifdef ST_OSLINUX
        /* Store this FB has being removed */
        FrameBuffers_p[NbFrameBuffers].KernelAddress_p = CurrentFrame_p->Allocation.Address_p;
        FrameBuffers_p[NbFrameBuffers].Size = CurrentFrame_p->Allocation.TotalSize;
        NbFrameBuffers++;
#endif

        DeAllocateFrameBuffer(BuffersHandle, CurrentFrame_p);

#ifdef ST_OSLINUX
        /* Notify all removed FB */
        NotifyRemovedFrameBuffers(BuffersHandle, FrameBuffers_p, NbFrameBuffers);
        if (FrameBuffers_p != NULL)
        {
            STOS_MemoryDeallocate(NULL, FrameBuffers_p);
        }
#endif
   }

} /* end of CheckIfToBeKilledAsSoonAsPossibleAndRemove() function */

/*******************************************************************************
Name        : DeAllocateFrameBuffers
Description : De-allocate all frame buffers of the decoder
Parameters  : Buffer manager handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if everything OK
*******************************************************************************/
static ST_ErrorCode_t DeAllocateFrameBuffers(const VIDBUFF_Handle_t BuffersHandle)
{
    VIDBUFF_FrameBuffer_t * CurrentFrame_p;
    VIDBUFF_FrameBuffer_t * FrameBufferToRemove_p;
    VIDBUFF_Data_t *            VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
#ifdef ST_OSLINUX
    FrameBuffers_t    * FrameBuffers_p = NULL;
    U32                 NbFrameBuffers = 0;
#endif

     /* Nothing to de-allocate ! */
    if ( (VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p == NULL) &&
          (VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p == NULL) )
    {
        return(ST_NO_ERROR);
    }

    TrMain(("\r\nDeAllocateFrameBuffers"));
#ifdef ST_OSLINUX
    /* Allocates max possible FB entries (primary + secondary) */
    FrameBuffers_p = (FrameBuffers_t *)STOS_MemoryAllocate(NULL, (((VIDBUFF_Data_t *) BuffersHandle)->MaxFrameBuffers + ((VIDBUFF_Data_t *) BuffersHandle)->MaxSecondaryFrameBuffers)*sizeof(FrameBuffers_t));
#endif

    /* Lock access to allocated frame buffers loop */
    semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    /**********************/
    /* primary frame buffers  */
    /**********************/

    /* De-allocate previously allocated buffers and quit with error */
    CurrentFrame_p = VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p;
    while (CurrentFrame_p != NULL)
    {
        FrameBufferToRemove_p = CurrentFrame_p;
        CurrentFrame_p = CurrentFrame_p->NextAllocated_p;

#ifdef ST_OSLINUX
        /* Store this FB has being removed */
        FrameBuffers_p[NbFrameBuffers].KernelAddress_p = FrameBufferToRemove_p->Allocation.Address_p;
        FrameBuffers_p[NbFrameBuffers].Size = FrameBufferToRemove_p->Allocation.TotalSize;
        NbFrameBuffers++;
#endif

        DeAllocateFrameBuffer(BuffersHandle, FrameBufferToRemove_p);

        if (CurrentFrame_p == VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p)
        {
            /* Finished going through all buffers */
            CurrentFrame_p = NULL;
        }
    } /* while CurrentFrame_p != NULL */

    VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p = NULL;
    VIDBUFF_Data_p->NumberOfAllocatedFrameBuffers = 0;


    /***********************/
    /* secondary frame buffers */
    /***********************/
    CurrentFrame_p = VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p;

    while (CurrentFrame_p != NULL)
    {
        FrameBufferToRemove_p = CurrentFrame_p;
        CurrentFrame_p = CurrentFrame_p->NextAllocated_p;

#ifdef ST_OSLINUX
        /* Store this FB has being removed */
        FrameBuffers_p[NbFrameBuffers].KernelAddress_p = FrameBufferToRemove_p->Allocation.Address_p;
        FrameBuffers_p[NbFrameBuffers].Size = FrameBufferToRemove_p->Allocation.TotalSize;
        NbFrameBuffers++;
#endif

        DeAllocateFrameBuffer(BuffersHandle, FrameBufferToRemove_p);

        if (CurrentFrame_p == VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p)
        {
            /* Finished going through all buffers */
            CurrentFrame_p = NULL;
        }
    } /* while CurrentFrame_p != NULL */

    VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p= NULL;
    VIDBUFF_Data_p->NumberOfSecondaryAllocatedFrameBuffers = 0;

#ifdef ST_OSLINUX
    /* Notify all removed FB (primary + secondary) */
    NotifyRemovedFrameBuffers(BuffersHandle, FrameBuffers_p, NbFrameBuffers);
    if (FrameBuffers_p != NULL)
    {
        STOS_MemoryDeallocate(NULL, FrameBuffers_p);
    }
#endif

    /* Unlock access to allocated frame buffers loop */
    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    return(ST_NO_ERROR);
} /* End of DeAllocateFrameBuffers() function */


/*******************************************************************************
Name        : InitBufferManagerStructure
Description : Initialise buffer manager structure
Parameters  : Buffer manager handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void InitBufferManagerStructure(const VIDBUFF_Handle_t BuffersHandle)
{
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;

    /* Initialise loops and tables of structures (nothing allocated yet for the user point of view) */
    /* Frame buffers not allocated yet */
    VIDBUFF_Data_p->NumberOfAllocatedFrameBuffers = 0;
    VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p = NULL;
    VIDBUFF_Data_p->NumberOfSecondaryAllocatedFrameBuffers = 0;
    VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p = NULL;
#ifdef USE_EXTRA_FRAME_BUFFERS
    VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p = NULL;
    VIDBUFF_Data_p->NumberOfExtraPrimaryFrameBuffersRequested = 0;
    VIDBUFF_Data_p->NumberOfAllocatedExtraPrimaryFrameBuffers = 0;
    VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p = NULL;
    VIDBUFF_Data_p->NumberOfExtraSecondaryFrameBuffersRequested = 0;
    VIDBUFF_Data_p->NumberOfAllocatedExtraSecondaryFrameBuffers = 0;
#endif
    /* No list of reference yet */

    /* Initialise the freeing of buffers strategy */
    VIDBUFF_Data_p->StrategyForTheBuffersFreeing.IsTheBuffersFreeingOptimized = FALSE;

    /* Initialise current profile to empty profile */
    VIDBUFF_Data_p->Profile.ApiProfile.MaxWidth          = 0;
    VIDBUFF_Data_p->Profile.ApiProfile.MaxHeight         = 0;
    VIDBUFF_Data_p->Profile.ApiProfile.NbFrameStore      = 0;
    VIDBUFF_Data_p->Profile.NbMainFrameStore           = 0;
    VIDBUFF_Data_p->Profile.NbSecondaryFrameStore      = 0;
    VIDBUFF_Data_p->Profile.ApiProfile.CompressionLevel  = STVID_COMPRESSION_LEVEL_NONE;
    VIDBUFF_Data_p->Profile.ApiProfile.DecimationFactor  = STVID_DECIMATION_FACTOR_NONE;
    VIDBUFF_Data_p->Profile.ApplicableDecimationFactor   = STVID_DECIMATION_FACTOR_NONE;
    VIDBUFF_Data_p->Profile.ApiProfile.FrameStoreIDParams.OptimisedNumber.Main = 0;
    VIDBUFF_Data_p->Profile.ApiProfile.FrameStoreIDParams.OptimisedNumber.Decimated = 0;
    VIDBUFF_Data_p->Profile.ApiProfile.FrameStoreIDParams.VariableInFixedSize.Main = 0;
    VIDBUFF_Data_p->Profile.ApiProfile.FrameStoreIDParams.VariableInFixedSize.Decimated = 0;
    VIDBUFF_Data_p->Profile.ApiProfile.FrameStoreIDParams.VariableInFullPartition.Main = (STAVMEM_PartitionHandle_t) NULL;
    VIDBUFF_Data_p->Profile.ApiProfile.FrameStoreIDParams.VariableInFullPartition.Decimated = (STAVMEM_PartitionHandle_t) NULL;

    VIDBUFF_Data_p->HDPIPParams.Enable                   = FALSE;
    VIDBUFF_Data_p->HDPIPParams.WidthThreshold           = 0;
    VIDBUFF_Data_p->HDPIPParams.HeightThreshold          = 0;

    /* Allow to send IMPOSSIBLE_WITH_PROFILE_EVT */
    VIDBUFF_Data_p->IsNotificationOfImpossibleWithProfileAllowed = TRUE;

    VIDBUFF_Data_p->FrameBufferAdditionalDataBytesPerMB  = 0;

#ifdef ST_smoothbackward
    VIDBUFF_Data_p->ControlMode = VIDBUFF_AUTOMATIC_CONTROL_MODE;
#endif

    VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p = STOS_SemaphoreCreatePriority(VIDBUFF_Data_p->CPUPartition_p,1);
#ifdef BENCHMARK_WINCESTAPI
	P_ADDSEMAPHORE(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p , "VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p");
#endif

} /* End of InitBufferManagerStructure() function */


/*******************************************************************************
Name        : TermBufferManagerStructure
Description : Terminate buffer manager structure
Parameters  : Buffer manager handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void TermBufferManagerStructure(const VIDBUFF_Handle_t BuffersHandle)
{
#ifndef REAL_OS40 /* semaphores are statics so they can not be deleted on OS40. */
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;

     STOS_SemaphoreDelete(VIDBUFF_Data_p->CPUPartition_p,VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);
#endif /* REAL_OS40 */

} /* End of TermBufferManagerStructure() function */


/*******************************************************************************
Name        : RemoveFrameBufferFromFrameBufferLoop
Description : Remove a frame buffer from the frame buffer loop (pointed by
              AllocatedFrameBuffersLoop_p).
Parameters  : Buffer manager handle, FrameBuffer to remove from the loop.
Assumptions : The frameBuffer is assumed to be in the loop.
              Call to this function must be protected by AllocatedFrameBuffersLoopLockingSemaphore_p
Limitations : None
Returns     : ST_NO_ERROR if everything OK
*******************************************************************************/
static ST_ErrorCode_t RemoveFrameBufferFromFrameBufferLoop(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_FrameBuffer_t * const FrameBuffer_p)
{
    VIDBUFF_FrameBuffer_t * PreviousFrame_p;
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
    BOOL TrySecondary;

    TrMain(("\r\nRemoveFrameBufferFromFrameBufferLoop 0x%08x", FrameBuffer_p));

    if (FrameBuffer_p == NULL)
    {
        /* Nothing to do ! */
        return ST_ERROR_BAD_PARAMETER;
    }

    switch (FrameBuffer_p->FrameBufferType)
    {
        case    VIDBUFF_PRIMARY_FRAME_BUFFER :
            if (VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p == NULL)
            {
                TrError(("\r\nError can't find FrameBuffer_p 0x%08x in Primary Loop (Loop empty)", FrameBuffer_p));
                return ST_ERROR_BAD_PARAMETER;
            }
            break;
        case    VIDBUFF_SECONDARY_FRAME_BUFFER :
            if (VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p == NULL)
            {
                TrError(("\r\nError can't find FrameBuffer_p 0x%08x in secondary Loop (Loop empty)", FrameBuffer_p));
                return ST_ERROR_BAD_PARAMETER;
            }
            break;
#ifdef USE_EXTRA_FRAME_BUFFERS
        case    VIDBUFF_EXTRA_PRIMARY_FRAME_BUFFER :
            if (VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p == NULL)
            {
                TrError(("\r\nError can't find FrameBuffer_p 0x%08x in Extra Loop (Loop empty)", FrameBuffer_p));
                return ST_ERROR_BAD_PARAMETER;
            }
            break;
        case    VIDBUFF_EXTRA_SECONDARY_FRAME_BUFFER :
            if (VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p == NULL)
            {
                TrError(("\r\nError can't find FrameBuffer_p 0x%08x in Extra Loop (Loop empty)", FrameBuffer_p));
                return ST_ERROR_BAD_PARAMETER;
            }
            break;
#endif /* USE_EXTRA_FRAME_BUFFERS */
        case    VIDBUFF_UNKNOWN_FRAME_BUFFER_TYPE :
        default :
            TrError(("\r\nError unexpected type %d for FrameBuffer_p 0x%08x", FrameBuffer_p->FrameBufferType, FrameBuffer_p));
            return ST_ERROR_BAD_PARAMETER;
    }


#ifdef USE_EXTRA_FRAME_BUFFERS
    if (FrameBuffer_p->FrameBufferType == VIDBUFF_EXTRA_PRIMARY_FRAME_BUFFER)
    {
        /************************/
        /* Remove from Extra Pool  */
        /************************/

        if (VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p == NULL)
        {
            TrError(("\r\nError can't find FrameBuffer_p 0x%08x in Extra Loop (Loop empty)", FrameBuffer_p));
            return ST_ERROR_BAD_PARAMETER;
        }

       /* Look for the frame just before the current one. */
        PreviousFrame_p = VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p;
        while (PreviousFrame_p->NextAllocated_p != FrameBuffer_p)
        {
            PreviousFrame_p = PreviousFrame_p->NextAllocated_p;
            if ((PreviousFrame_p == NULL) || (PreviousFrame_p == VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p))
            {
                TrError(("\r\nError can't find FrameBuffer_p 0x%08x in Extra Loop", FrameBuffer_p));
                return ST_ERROR_BAD_PARAMETER;
            }
        }

        /* Here we have (PreviousFrame_p->NextAllocated_p == FrameBuffer_p) */
        if (PreviousFrame_p == FrameBuffer_p)
        {
            /* There is only one frame buffer in the loop. Remove it. */
            VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p = NULL;
        }
        else
        {
            if (FrameBuffer_p == VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p)
            {
                /* The current frame buffer is the first of the loop. So, the loop address must be
                * set with the previous frame buffer of the loop (no to be NULL). */
                VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p = PreviousFrame_p;
            }
            /* Remove the current frame buffer from the loop. */
            PreviousFrame_p->NextAllocated_p = FrameBuffer_p->NextAllocated_p;
        }
        /* facultative */
        FrameBuffer_p->NextAllocated_p = NULL;

        VIDBUFF_Data_p->NumberOfAllocatedExtraPrimaryFrameBuffers--;
        return ST_NO_ERROR;
    }
    if (FrameBuffer_p->FrameBufferType == VIDBUFF_EXTRA_SECONDARY_FRAME_BUFFER)
    {
        /************************/
        /* Remove from Extra Pool  */
        /************************/

        if (VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p == NULL)
        {
            TrError(("\r\nError can't find FrameBuffer_p 0x%08x in Extra Loop (Loop empty)", FrameBuffer_p));
            return ST_ERROR_BAD_PARAMETER;
        }

       /* Look for the frame just before the current one. */
        PreviousFrame_p = VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p;
        while (PreviousFrame_p->NextAllocated_p != FrameBuffer_p)
        {
            PreviousFrame_p = PreviousFrame_p->NextAllocated_p;
            if ((PreviousFrame_p == NULL) || (PreviousFrame_p == VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p))
            {
                TrError(("\r\nError can't find FrameBuffer_p 0x%08x in Extra Loop", FrameBuffer_p));
                return ST_ERROR_BAD_PARAMETER;
            }
        }

        /* Here we have (PreviousFrame_p->NextAllocated_p == FrameBuffer_p) */
        if (PreviousFrame_p == FrameBuffer_p)
        {
            /* There is only one frame buffer in the loop. Remove it. */
            VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p = NULL;
        }
        else
        {
            if (FrameBuffer_p == VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p)
            {
                /* The current frame buffer is the first of the loop. So, the loop address must be
                * set with the previous frame buffer of the loop (no to be NULL). */
                VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p = PreviousFrame_p;
            }
            /* Remove the current frame buffer from the loop. */
            PreviousFrame_p->NextAllocated_p = FrameBuffer_p->NextAllocated_p;
        }
        /* facultative */
        FrameBuffer_p->NextAllocated_p = NULL;

        VIDBUFF_Data_p->NumberOfAllocatedExtraSecondaryFrameBuffers--;
        return ST_NO_ERROR;
    }
#endif /* USE_EXTRA_FRAME_BUFFERS */

    /* The loops below try to identify if this frame buffer belongs to the Primary or Secondary loop.
       Rem: We could now get this information from "FrameBufferType"! */

    TrySecondary=FALSE;
    /* Look for the frame just before the current one. */
    PreviousFrame_p = VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p;
    while ((PreviousFrame_p->NextAllocated_p != FrameBuffer_p)&&(!TrySecondary))
    {
        PreviousFrame_p = PreviousFrame_p->NextAllocated_p;
        if ((PreviousFrame_p == NULL) || (PreviousFrame_p == VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p))
        {
            /* Error, can't find FrameBuffer_p,  ! */
            TrySecondary=TRUE;
        }
    }
    if(!TrySecondary)
    {
        /**************************/
        /* Remove from Primary Pool  */
        /**************************/

        /* Here we have (PreviousFrame_p->NextAllocated_p == FrameBuffer_p) */
        if (PreviousFrame_p == FrameBuffer_p)
        {
            /* There is only one frame buffer in the loop. Remove it. */
            VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p = NULL;
        }
        else
        {
            if (FrameBuffer_p == VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p)
            {
                /* The current frame buffer is the first of the loop. So, the loop address must be
                * set with the previous frame buffer of the loop (no to be NULL). */
                VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p = PreviousFrame_p;
            }
            /* Remove the current frame buffer from the loop. */
            PreviousFrame_p->NextAllocated_p = FrameBuffer_p->NextAllocated_p;
        }
        /* facultative */
        FrameBuffer_p->NextAllocated_p = NULL;

        VIDBUFF_Data_p->NumberOfAllocatedFrameBuffers --;
    }
    else /* must try in secondary pool */
    {
        /****************************/
        /* Remove from Secondary Pool  */
        /****************************/

        if (VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p == NULL)
        {
            TrError(("\r\nError can't find FrameBuffer_p 0x%08x in Secondary Loop (Loop empty)", FrameBuffer_p));
            return ST_ERROR_BAD_PARAMETER;
        }

        /* Look for the frame just before the current one. */
        PreviousFrame_p = VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p;
        while (PreviousFrame_p->NextAllocated_p != FrameBuffer_p)
        {
            PreviousFrame_p = PreviousFrame_p->NextAllocated_p;
            if ((PreviousFrame_p == NULL) || (PreviousFrame_p == VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p))
            {
                TrError(("\r\nError can't find FrameBuffer_p 0x%08x in Secondary Loop", FrameBuffer_p));
                return ST_ERROR_BAD_PARAMETER;
            }
        }
        /* Here we have (PreviousFrame_p->NextAllocated_p == FrameBuffer_p) */
        if (PreviousFrame_p == FrameBuffer_p)
        {
            /* There is only one frame buffer in the loop. Remove it. */
            VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p= NULL;
        }
        else
        {
            if (FrameBuffer_p == VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p)
            {
                /* The current frame buffer is the first of the loop. So, the loop address must be
                * set with the previous frame buffer of the loop (no to be NULL). */
                VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p= PreviousFrame_p;
            }
            /* Remove the current frame buffer from the loop. */
            PreviousFrame_p->NextAllocated_p = FrameBuffer_p->NextAllocated_p;
        }
        /* facultative */
        FrameBuffer_p->NextAllocated_p = NULL;

        VIDBUFF_Data_p->NumberOfSecondaryAllocatedFrameBuffers --;
    }

    return ST_NO_ERROR;
} /* End of RemoveFrameBufferFromFrameBufferLoop() function */

/*******************************************************************************
Name        : VIDBUFF_PrintPictureBuffersStatus
Description : Print the Status of every Picture Buffers.
              This function does nothing if the Debug Print are not activated!
Parameters  : Buffer manager handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void VIDBUFF_PrintPictureBuffersStatus(const VIDBUFF_Handle_t BuffersHandle)
{
#if defined (TRACE_UART) || defined (VIDEO_DEBUG)
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;

    /* Lock access to allocated frame buffers loop */
    semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    TrError(("\n\r***************"));
    TrError(("\n\rPrimary:"));
    PrintPoolPictureBuffersStatus(BuffersHandle, VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p);
    TrError(("\n\rSecondary:"));
    PrintPoolPictureBuffersStatus(BuffersHandle, VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p);
#ifdef USE_EXTRA_FRAME_BUFFERS
    TrError(("\n\rExtra Primary:"));
    PrintPoolPictureBuffersStatus(BuffersHandle, VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p);
    TrError(("\n\rExtra Secondary:"));
    PrintPoolPictureBuffersStatus(BuffersHandle, VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p);
#endif
    TrError(("\n\r***************\r\n"));

    /* Un-lock access to allocated frame buffers loop */
    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);
#else
    UNUSED_PARAMETER(BuffersHandle);
#endif /* TRACE_UART || VIDEO_DEBUG */
}

#if defined (TRACE_UART) || defined (VIDEO_DEBUG)
/*******************************************************************************
Name        : PrintPoolPictureBuffersStatus
Description : Print the Status of Picture Buffers associated to a given Pool of
              Frame Buffers (Primary, Secondary or Extra).
Parameters  : Buffer manager handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void PrintPoolPictureBuffersStatus(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_FrameBuffer_t * FrameBufferPool_p)
{
    VIDBUFF_FrameBuffer_t * CurrentFrame_p;

    CurrentFrame_p = FrameBufferPool_p;

    if (CurrentFrame_p != NULL)
    {
        do
        {
            TrError(("\n\rFB 0x%x:", CurrentFrame_p));

            if (CurrentFrame_p->FrameOrFirstFieldPicture_p != NULL)
            {
                PrintPictureBufferStatus(BuffersHandle, CurrentFrame_p->FrameOrFirstFieldPicture_p);
            }

            if (CurrentFrame_p->NothingOrSecondFieldPicture_p != NULL)
            {
                PrintPictureBufferStatus(BuffersHandle, CurrentFrame_p->NothingOrSecondFieldPicture_p);
            }

            if ( (CurrentFrame_p->FrameOrFirstFieldPicture_p == NULL) &&
                 (CurrentFrame_p->NothingOrSecondFieldPicture_p == NULL) )
            {
                TrError(("\n\r  No PB associated"));
            }

            CurrentFrame_p = CurrentFrame_p->NextAllocated_p;
        }while (CurrentFrame_p != FrameBufferPool_p);
    }

}

/*******************************************************************************
Name        : PrintPictureBufferStatus
Description : Print the Status of One Picture Buffer
Parameters  : Buffer manager handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void PrintPictureBufferStatus(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const Picture_p)
{
    U32 index = 0;
    UNUSED_PARAMETER(BuffersHandle);

    if (Picture_p->Buffers.PictureLockCounter == 0)
    {
        /* If the PictureLockCounter is null then the picture buffer is not in use */
        TrError(("\n\r  PB 0x%08x not used (Assoc 0x%x)",
                    Picture_p,
                    Picture_p->AssociatedDecimatedPicture_p));
    }
    else
    {
        TrError(("\n\r  PB 0x%08x DisplayStatus=%d, Counter=%d (Assoc 0x%x)",
                    Picture_p,
                    Picture_p->Buffers.DisplayStatus,
                    Picture_p->Buffers.PictureLockCounter,
                    Picture_p->AssociatedDecimatedPicture_p));
        /* Show the modules who have taken this buffer: */
        for (index=0; index<VIDCOM_MAX_MODULE_NBR; index++)
        {
            if (Picture_p->Buffers.Module[index].TakeReleaseCounter != 0)
            {
                char moduleName[30];

                switch (index)
                {
                    case VIDCOM_APPLICATION_MODULE_BASE :
                        strcpy(moduleName, "Application");
                        break;
                    case VIDCOM_VIDORDQUEUE_MODULE_BASE :
                        strcpy(moduleName, "VID_ORDQUEUE");
                        break;
                    case VIDCOM_VIDDECO_MODULE_BASE :
                        strcpy(moduleName, "VID_DECO");
                        break;
                    case VIDCOM_VIDDEC_REF_MODULE_BASE :
                        strcpy(moduleName, "VIDDEC_REF");
                        break;
                    case VIDCOM_VIDBUFF_MODULE_BASE :
                        strcpy(moduleName, "VID_BUFF");
                        break;
                    case VIDCOM_VIDDEC_MODULE_BASE :
                        strcpy(moduleName, "VID_DEC");
                        break;
                    case VIDCOM_VIDDISP_MODULE_BASE :
                        strcpy(moduleName, "VID_DISP");
                        break;
                    case VIDCOM_VIDSPEED_MODULE_BASE :
                        strcpy(moduleName, "VID_SPEED-TRICK");
                        break;
                    case VIDCOM_VIDPROD_MODULE_BASE :
                        strcpy(moduleName, "VID_PROD");
                        break;
                    case VIDCOM_VIDQUEUE_MODULE_BASE :
                        strcpy(moduleName, "VID_QUEUE");
                        break;
                    case VIDCOM_DEI_MODULE_BASE :
                        strcpy(moduleName, "DEI");
                        break;
                    default :
                        strcpy(moduleName, "Unknown Module");
                }
                TrError(("\n\r  %s: %d take(s)", moduleName, Picture_p->Buffers.Module[index].TakeReleaseCounter));
            }
        }
    }
}
#endif /* TRACE_UART || VIDEO_DEBUG */

#ifdef TRACE_BUFFER
/*******************************************************************************
Name        : PrintFrameBuffers
Description : Print a status of ALL Frame Buffers
Parameters  : Buffer manager handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void PrintFrameBuffers(const VIDBUFF_Handle_t BuffersHandle)
{
    VIDBUFF_FrameBuffer_t * CurrentFrame_p;
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;

    /* Lock access to allocated frame buffers loop */
    semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    /**************************/
    /* Print Primary Pool     */
    /**************************/
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "\r\n*** Primary ***"));
    if (VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "No frame buffer."));
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Allocated frame buffers >>>>%d<<<<",
                VIDBUFF_Data_p->NumberOfAllocatedFrameBuffers));

        CurrentFrame_p = VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p;
        do
        {
            PrintOneFrameBuffer(BuffersHandle, CurrentFrame_p);
            CurrentFrame_p = CurrentFrame_p->NextAllocated_p;

        } while (CurrentFrame_p != VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p);
    }

    /**************************/
    /* Print Secondary Pool   */
    /**************************/
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "\r\n*** Secondary ***"));
    if (VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "No frame buffer."));
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Allocated frame buffers >>>>%d<<<<",
                VIDBUFF_Data_p->NumberOfSecondaryAllocatedFrameBuffers));

        CurrentFrame_p = VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p;
        do
        {
            PrintOneFrameBuffer(BuffersHandle, CurrentFrame_p);
            CurrentFrame_p = CurrentFrame_p->NextAllocated_p;

        } while (CurrentFrame_p != VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p);
    }

#ifdef USE_EXTRA_FRAME_BUFFERS
    /**************************/
    /* Print Extra Pool       */
    /**************************/
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "\r\n*** Extra ***"));
    if (VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "No frame buffer."));
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Allocated frame buffers >>>>%d<<<<",
                VIDBUFF_Data_p->NumberOfAllocatedExtraPrimaryFrameBuffers));

        CurrentFrame_p = VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p;
        do
        {
            PrintOneFrameBuffer(BuffersHandle, CurrentFrame_p);
            CurrentFrame_p = CurrentFrame_p->NextAllocated_p;

        } while (CurrentFrame_p != VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p);
    }
    /**************************/
    /* Print Extra Secondary Pool       */
    /**************************/
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "\r\n*** Extra Secondary ***"));
    if (VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "No frame buffer."));
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Allocated frame buffers >>>>%d<<<<",
                VIDBUFF_Data_p->NumberOfAllocatedExtraSecondaryFrameBuffers));

        CurrentFrame_p = VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p;
        do
        {
            PrintOneFrameBuffer(BuffersHandle, CurrentFrame_p);
            CurrentFrame_p = CurrentFrame_p->NextAllocated_p;

        } while (CurrentFrame_p != VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p);
    }
#endif /* USE_EXTRA_FRAME_BUFFERS */

    /* Un-lock access to allocated frame buffers loop */
    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

} /* End of PrintFrameBuffers() function */

/*******************************************************************************
Name        : PrintOneFrameBuffer
Description : Print a status of ONE Frame Buffers
Parameters  : Buffer manager handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void PrintOneFrameBuffer(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_FrameBuffer_t * FrameBuffer_p)
{
    if (FrameBuffer_p->ToBeKilledAsSoonAsPossible)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Frame 0x%08x (to be removed)", FrameBuffer_p->Allocation.Address_p));
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Frame 0x%08x", FrameBuffer_p->Allocation.Address_p));
    }

    if (FrameBuffer_p->FrameOrFirstFieldPicture_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "     no pic1."));
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "     pic1=0x%08x, count=%d, disp_stat=%d",
        	FrameBuffer_p->FrameOrFirstFieldPicture_p,
            FrameBuffer_p->FrameOrFirstFieldPicture_p->Buffers.PictureLockCounter,
            FrameBuffer_p->FrameOrFirstFieldPicture_p->Buffers.DisplayStatus));
    }

    if (FrameBuffer_p->NothingOrSecondFieldPicture_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "     no pic2."));
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "     pic2=0x%08x, count=%d, disp_stat=%d",
        	FrameBuffer_p->NothingOrSecondFieldPicture_p,
            FrameBuffer_p->NothingOrSecondFieldPicture_p->Buffers.PictureLockCounter,
            FrameBuffer_p->NothingOrSecondFieldPicture_p->Buffers.DisplayStatus));
    }

} /* End of PrintOneFrameBuffer() function */

#endif


/*******************************************************************************
Name        : ComputeFrameBufferSize
Description :
Parameters  : Buffer manager handle, allocation parameters
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void ComputeFrameBufferSize(const HALBUFF_Handle_t BufferHandle, const HALBUFF_AllocateBufferParams_t * const AllocParams_p, U32 * const Size)
{
    U32 LumaBufferSize, ChromaBufferSize;
    U32 ChromaOffset;
    U32 Alignment;

    /* Get alignment depending on the HAL */
    Alignment = ((HALBUFF_Data_t *) BufferHandle)->FunctionsTable_p->GetFrameBufferAlignment(BufferHandle);
    /* Compute chroma and luma buffer sizes, according to width, height and buffer type.    */
    ((HALBUFF_Data_t *) BufferHandle)->FunctionsTable_p->ComputeBufferSizes(BufferHandle,
            AllocParams_p, &LumaBufferSize, &ChromaBufferSize);

    /* Calculate the chroma position.                   */
    ChromaOffset = ((LumaBufferSize + Alignment - 1) & (~(Alignment - 1)));
    /* and its size according to memory granularity.    */
    ChromaBufferSize = ((ChromaBufferSize + Alignment - 1) & (~(Alignment - 1)));

    /* Calculate the total buffer size.                 */
    *Size = ChromaOffset + ChromaBufferSize;
}

/*******************************************************************************
Name        : VIDBUFF_GetAllocatedFrameNumbers
Description :
Parameters  : Buffer manager handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void VIDBUFF_GetAllocatedFrameNumbers(const VIDBUFF_Handle_t BuffersHandle, U32 * const MainBuffers, U32 * const DecimBuffers)
{
    U8 NumberAlreadyAllocated;
    VIDBUFF_FrameBuffer_t * CurrentFrame_p;
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;

    /* Lock access to allocated frame buffers loop */
    semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    /* Determine the number of buffers already allocated */
    NumberAlreadyAllocated = 0;
    CurrentFrame_p = VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p;
    if (CurrentFrame_p != NULL)
    {
        /* There are buffers already allocated: count them */
        do
        {
            /* Test if the allocated frame buffer has got the good memory profile. */
            if (!(CurrentFrame_p->ToBeKilledAsSoonAsPossible))
            {
                /* Yes, we can count it. */
                NumberAlreadyAllocated ++;
            }
            /* Get the next frame buffer in the loop. */
            CurrentFrame_p = CurrentFrame_p->NextAllocated_p;
        } while ((CurrentFrame_p != VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p) && (CurrentFrame_p != NULL));
    }
    *MainBuffers = NumberAlreadyAllocated;
    /* Determine the number of decimated buffers already allocated */
    NumberAlreadyAllocated = 0;
    CurrentFrame_p = VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p;
    if (CurrentFrame_p != NULL)
    {
        /* There are buffers already allocated: count them */
        do
        {
            /* Test if the allocated frame buffer has got the good memory profile. */
            if (!(CurrentFrame_p->ToBeKilledAsSoonAsPossible))
            {
                /* Yes, we can count it. */
                NumberAlreadyAllocated ++;
            }
            /* Get the next frame buffer in the loop. */
            CurrentFrame_p = CurrentFrame_p->NextAllocated_p;
        } while ((CurrentFrame_p != VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p) && (CurrentFrame_p != NULL));
    }
    *DecimBuffers = NumberAlreadyAllocated;

    /* Un-lock access to allocated frame buffers loop */
    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);
} /* end of VIDBUFF_GetAllocatedFrameNumbers */

/*******************************************************************************
Name        : AllocateNewFrameBuffer
Description : Allocates one frame buffer and add it into the linked
              loop of allocated frame buffers, pointed by AllocatedFrameBuffersLoop_p.
              It allocates secondary frame buffer if required too (according to the
              memory profile of video driver).
Parameters  : Buffer manager handle, allocation parameters
Assumptions : Decimation modes and Compression modes are MUTUALLY EXCLUSIVE.
              The dynamic frame buffer allocation is used for H264 instances _only_
Limitations : The BufferSize field from AllocParams_p is not used.
Returns     : ST_ERROR_NO_MEMORY if STAVMEM allocation failed
              ST_NO_ERROR if everything OK
*******************************************************************************/
static ST_ErrorCode_t AllocateNewFrameBuffer(const VIDBUFF_Handle_t BuffersHandle,
        const VIDBUFF_AllocateBufferParams_t * const AllocParams_p, const VIDCOM_InternalProfile_t Profile, const U8 MaxFramesNumber,
        VIDBUFF_FrameBuffer_t ** const AllocatedFrameBuffersLoop_p, const HALBUFF_AllocateBufferParams_t HALAllocParams,
        const U32 MaxTotalFBSize, U32 * const NumberOfAllocatedFrameBuffers_p, const VIDBUFF_AvailableReconstructionMode_t RequiredAvailableReconstructionMode,
        const HALBUFF_FrameBufferType_t FrameBufferReconstructionMode, BOOL * const NeedForExtraAllocation_p, BOOL * const MaxSizedReached_p)
{
    U8 NumberAlreadyAllocated;
    U8 FrameBufferIndex;
    BOOL AllocationError;
    VIDBUFF_FrameBuffer_t * CurrentFrame_p, * FirstOfNewAllocated_p = NULL;
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    U32 TotalSizeInUse, FrameBufferSize;
    STVID_MemoryProfile_t  CommonMemoryProfile;
    U32 MaxFrameBufferIndex = 0;
    BOOL IsAPrimaryFrameBuffer = TRUE;
#ifdef ST_OSLINUX
    FrameBuffers_t                * FrameBuffers_p = NULL;
    U32                             NbFrameBuffers = 0;
#endif

    /* Check pool validity */
    /*---------------*/
    if ( (*AllocatedFrameBuffersLoop_p) == VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p)
    {
        /* primary pool, OK */
        IsAPrimaryFrameBuffer = TRUE;
    }
    else
    {
        if ( (*AllocatedFrameBuffersLoop_p) == VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p)
        {
            /* secondary pool, OK */
            IsAPrimaryFrameBuffer = FALSE;
        }
        else
        {
            /* not valid pool */
            TrError(("\r\nError! Invalid Frame Buffer pool!!!"));
            return(ST_ERROR_BAD_PARAMETER);
        }
    }

    /* Lock access to allocated frame buffers loop */
    semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    /* Determine the number of buffers already allocated */
    NumberAlreadyAllocated = 0;
    TotalSizeInUse = 0;
    *MaxSizedReached_p = FALSE;

    CurrentFrame_p = *AllocatedFrameBuffersLoop_p;
    if (CurrentFrame_p != NULL)
    {
        /* There are buffers already allocated: count them */
        do
        {
            /* Test if the allocated frame buffer has got the good memory profile. */
            if (!(CurrentFrame_p->ToBeKilledAsSoonAsPossible))
            {
                /* Yes, we can count it. */
                NumberAlreadyAllocated ++;
            }
            /* Get the next frame buffer in the loop. */
            CurrentFrame_p = CurrentFrame_p->NextAllocated_p;
            TotalSizeInUse = TotalSizeInUse + CurrentFrame_p->Allocation.TotalSize;
        } while ((CurrentFrame_p != *AllocatedFrameBuffersLoop_p) && (CurrentFrame_p != NULL));
    }

    /* Check if there is not too much buffers ! */
    if (NumberAlreadyAllocated >= MaxFramesNumber)
    {
        *NeedForExtraAllocation_p = FALSE;
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "# of allocated (%d) reaches MaxDecFrame (%d)", NumberAlreadyAllocated, MaxFramesNumber));
        ErrorCode = ST_NO_ERROR;
    }
    else
    {
        ComputeFrameBufferSize(VIDBUFF_Data_p->HALBuffersHandle, &HALAllocParams, &FrameBufferSize);
        TotalSizeInUse = TotalSizeInUse + FrameBufferSize;
        /* Allocate new frame buffer  */
        /* Search a frame buffer structure */
        FrameBufferIndex = 0;
        CurrentFrame_p = NULL;
        if (RequiredAvailableReconstructionMode == VIDBUFF_SECONDARY_RECONSTRUCTION)
        {
            MaxFrameBufferIndex = VIDBUFF_Data_p->MaxSecondaryFrameBuffers;
            while ((FrameBufferIndex < MaxFrameBufferIndex) && (VIDBUFF_Data_p->SecondaryFrameBuffers_p[FrameBufferIndex].AllocationIsDone))
            {
                FrameBufferIndex++;
            }
            CurrentFrame_p = VIDBUFF_Data_p->SecondaryFrameBuffers_p + FrameBufferIndex;
        }
        else
        {
            MaxFrameBufferIndex = VIDBUFF_Data_p->MaxFrameBuffers;
            while ((FrameBufferIndex < MaxFrameBufferIndex) && (VIDBUFF_Data_p->FrameBuffers_p[FrameBufferIndex].AllocationIsDone))
            {
                FrameBufferIndex++;
            }
            CurrentFrame_p = VIDBUFF_Data_p->FrameBuffers_p + FrameBufferIndex;
        }

        if (FrameBufferIndex == MaxFrameBufferIndex)
        {
            /* Error: No frame buffer structure free ! */
            AllocationError = TRUE;
        }
        else
        /* Check Total size used in partition for MAIN buffers < Profile.ApiProfile.FrameStoreIDParams.VariableInFixedSize.Main */
        if ((Profile.ApiProfile.NbFrameStore == STVID_VARIABLE_IN_FIXED_SIZE_NB_FRAME_STORE_ID) && (TotalSizeInUse > MaxTotalFBSize))
        {
            /* Error: Allocation could not be performed within passed partition size ! */
            AllocationError = TRUE;
            *MaxSizedReached_p = TRUE;
        }
        else
        {
#ifdef TRACE_UART
            /*TraceBuffer(("\n\rAllocate one Frame buffer [%d;%d]",
                    HALAllocParams.PictureWidth, HALAllocParams.PictureHeight));*/
#endif
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Allocate one Frame buffer [%d;%d]",
                    HALAllocParams.PictureWidth, HALAllocParams.PictureHeight));

            /* Allocate required frame buffers */
            AllocationError = FALSE;

            if (HALBUFF_AllocateFrameBuffer(VIDBUFF_Data_p->HALBuffersHandle, &HALAllocParams, &(CurrentFrame_p->Allocation), FrameBufferReconstructionMode) != ST_NO_ERROR)
            {
                /* Error: Allocation could not be performed ! */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Allocation of a frame buffer failed: exiting..."));
                AllocationError = TRUE;
                if (Profile.ApiProfile.NbFrameStore == STVID_VARIABLE_IN_FULL_PARTITION_NB_FRAME_STORE_ID)
                {
                    /* Error: Allocation could not be performed within the passed partition (out of memory)! */
                    *MaxSizedReached_p = TRUE;
                }
            }
            else
            {
#ifdef ST_OSLINUX
                NbFrameBuffers = 0;
                FrameBuffers_p = (FrameBuffers_t *)STOS_MemoryAllocate(NULL, sizeof(FrameBuffers_t));
#endif

#ifdef STTBX_REPORT
                if (RequiredAvailableReconstructionMode == VIDBUFF_SECONDARY_RECONSTRUCTION)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Allocated secondary frame buffer at address 0x%08x", CurrentFrame_p->Allocation.Address_p));
                }
                else
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Allocated frame buffer at address 0x%08x", CurrentFrame_p->Allocation.Address_p));
                }
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "\tTotal: 0x%08x, Y: 0x%08x, C: 0x%08x",
                                                                                            CurrentFrame_p->Allocation.TotalSize,
                                                                                            CurrentFrame_p->Allocation.TotalSize-CurrentFrame_p->Allocation.Size2,
                                                                                            CurrentFrame_p->Allocation.Size2));
#endif /* ifdef STTBX_REPORT */
                CurrentFrame_p->AvailableReconstructionMode = RequiredAvailableReconstructionMode;
#ifdef ST_OSLINUX
                FrameBuffers_p[NbFrameBuffers].KernelAddress_p = CurrentFrame_p->Allocation.Address_p;
                FrameBuffers_p[NbFrameBuffers].Size = CurrentFrame_p->Allocation.TotalSize;
                NbFrameBuffers ++;
#endif
            }
        }

        if (AllocationError)
        {
#ifdef ST_OSLINUX
            if (FrameBuffers_p != NULL)
            {
                /* Error occured, newly allocated FB will not be notified, free memory ... */
                STOS_MemoryDeallocate(NULL, FrameBuffers_p);
            }
#endif
            /* Un-lock access to allocated frame buffers loop */
            semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);
            /* Profile too small: if first time, notify IMPOSSIBLE_WITH_PROFILE_EVT */
            if (VIDBUFF_Data_p->IsNotificationOfImpossibleWithProfileAllowed)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Not enough memory, Notify VIDBUFF_IMPOSSIBLE_WITH_MEM_PROFILE_EVT_ID"));
                /* Notify the event only once, until it is re-allowed */
                VIDBUFF_Data_p->IsNotificationOfImpossibleWithProfileAllowed = FALSE;

                /* Notify event, so that API level notifies the application. Propose larger buffers. */
                CommonMemoryProfile = VIDBUFF_Data_p->Profile.ApiProfile;
                if (CommonMemoryProfile.NbFrameStore == STVID_VARIABLE_IN_FIXED_SIZE_NB_FRAME_STORE_ID)
                {
                    if (RequiredAvailableReconstructionMode == VIDBUFF_SECONDARY_RECONSTRUCTION)
                    {
                        /* We failed to allocate a decimated frame buffer, recommend the current profile size for Main */
                        CommonMemoryProfile.FrameStoreIDParams.VariableInFixedSize.Main = VIDBUFF_Data_p->Profile.ApiProfile.FrameStoreIDParams.VariableInFixedSize.Main;
                        CommonMemoryProfile.FrameStoreIDParams.VariableInFixedSize.Decimated = TotalSizeInUse;
                    }
                    else
                    {
                        CommonMemoryProfile.FrameStoreIDParams.VariableInFixedSize.Main = TotalSizeInUse;
                        CommonMemoryProfile.FrameStoreIDParams.VariableInFixedSize.Decimated = TotalSizeInUse;
                        if (VIDBUFF_Data_p->Profile.ApplicableDecimationFactor & STVID_DECIMATION_FACTOR_H2)
                        {
                            CommonMemoryProfile.FrameStoreIDParams.VariableInFixedSize.Decimated = CommonMemoryProfile.FrameStoreIDParams.VariableInFixedSize.Decimated / 2;
                        }
                        if (VIDBUFF_Data_p->Profile.ApplicableDecimationFactor & STVID_DECIMATION_FACTOR_V2)
                        {
                            CommonMemoryProfile.FrameStoreIDParams.VariableInFixedSize.Decimated = CommonMemoryProfile.FrameStoreIDParams.VariableInFixedSize.Decimated / 2;
                        }
                        if (VIDBUFF_Data_p->Profile.ApplicableDecimationFactor & STVID_DECIMATION_FACTOR_H4)
                        {
                            CommonMemoryProfile.FrameStoreIDParams.VariableInFixedSize.Decimated = CommonMemoryProfile.FrameStoreIDParams.VariableInFixedSize.Decimated / 4;
                        }
                        if (VIDBUFF_Data_p->Profile.ApplicableDecimationFactor & STVID_DECIMATION_FACTOR_V4)
                        {
                            CommonMemoryProfile.FrameStoreIDParams.VariableInFixedSize.Decimated = CommonMemoryProfile.FrameStoreIDParams.VariableInFixedSize.Decimated / 4;
                        }
                    }
                }
                CommonMemoryProfile.MaxWidth = AllocParams_p->PictureWidth;
                CommonMemoryProfile.MaxHeight = AllocParams_p->PictureHeight;
                STEVT_Notify(((VIDBUFF_Data_t *) BuffersHandle)->EventsHandle,
                    ((VIDBUFF_Data_t *) BuffersHandle)->RegisteredEventsID[VIDBUFF_IMPOSSIBLE_WITH_MEM_PROFILE_EVT_ID],
                    STEVT_EVENT_DATA_TYPE_CAST &CommonMemoryProfile);
            }
            return(ST_ERROR_NO_MEMORY);
        }

        /* Allocation successful: build new bank of frames */
        CurrentFrame_p->AllocationIsDone              = TRUE;
        if (IsAPrimaryFrameBuffer)
        {
            CurrentFrame_p->FrameBufferType = VIDBUFF_PRIMARY_FRAME_BUFFER;
        }
        else
        {
            CurrentFrame_p->FrameBufferType = VIDBUFF_SECONDARY_FRAME_BUFFER;
        }
        CurrentFrame_p->ToBeKilledAsSoonAsPossible    = FALSE;
        CurrentFrame_p->FrameOrFirstFieldPicture_p    = NULL;
        CurrentFrame_p->NothingOrSecondFieldPicture_p = NULL;

        if (RequiredAvailableReconstructionMode == VIDBUFF_SECONDARY_RECONSTRUCTION)
        {
            CurrentFrame_p->CompressionLevel = Profile.ApiProfile.CompressionLevel;
            CurrentFrame_p->DecimationFactor = Profile.ApplicableDecimationFactor;
        }
        else
        {
            CurrentFrame_p->CompressionLevel = STVID_COMPRESSION_LEVEL_NONE;
            CurrentFrame_p->DecimationFactor = STVID_DECIMATION_FACTOR_NONE;
        }

        CurrentFrame_p->HasDecodeToBeDoneWhileDisplaying    = FALSE;

        /* Append allocated buffer to decoder buffer bank */
        CurrentFrame_p->NextAllocated_p = FirstOfNewAllocated_p;
        FirstOfNewAllocated_p = CurrentFrame_p;

        /* New allocations successful: add list of new allocated frames to AllocatedFrameBuffersLoop_p */
        while (CurrentFrame_p->NextAllocated_p != NULL)
        {
            /* Look for last frame of list */
            CurrentFrame_p = CurrentFrame_p->NextAllocated_p;
        }
        /* Last frame of the new list found: insert it into AllocatedFrameBuffersLoop_p */
        if (*AllocatedFrameBuffersLoop_p == NULL)
        {
            /* AllocatedFrameBuffersLoop_p was empty: make list a loop */
            CurrentFrame_p->NextAllocated_p = FirstOfNewAllocated_p;
            *AllocatedFrameBuffersLoop_p = FirstOfNewAllocated_p;
        }
        else
        {
            /* AllocatedFrameBuffersLoop_p was not empty: make new list point to loop, open loop and make it again */
            CurrentFrame_p->NextAllocated_p = (*AllocatedFrameBuffersLoop_p)->NextAllocated_p;
            (*AllocatedFrameBuffersLoop_p)->NextAllocated_p = FirstOfNewAllocated_p;
        }
        /* Update number fo allocated frames */
        *NumberOfAllocatedFrameBuffers_p += 1;

#ifdef ST_OSLINUX
        /* Notify only newly primary allocated FB */
        NotifyNewFrameBuffers(BuffersHandle, FrameBuffers_p, NbFrameBuffers);
        if (FrameBuffers_p != NULL)
        {
            STOS_MemoryDeallocate(NULL, FrameBuffers_p);
        }
#endif
    }

    /* Un-lock access to allocated frame buffers loop */
    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    return(ErrorCode);
} /* End of AllocateNewFrameBuffer() function */


/*******************************************************************************
Name        : VIDBUFF_AllocateNewDecimatedFrameBuffer
Description :
Parameters  : Video buffer manager handle
Assumptions : The dynamic frame buffer allocation is used for H264 instances _only_
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER if too many buffers required
              ST_ERROR_NO_MEMORY if STAVMEM allocation failed
              ST_NO_ERROR if everything OK
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_AllocateNewDecimatedFrameBuffer(const VIDBUFF_Handle_t BuffersHandle, BOOL * const NeedForExtraAllocationDecimated_p, BOOL * const MaxSizedReached_p)
{
    VIDBUFF_AllocateBufferParams_t  AllocParams;
    HALBUFF_AllocateBufferParams_t  HALSecondaryAllocParams;
    ST_ErrorCode_t                  ErrorCode;
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
    U8 MaxDecimFramesNumber;
    U32 HeightAfterDecimationFactor, WidthAfterDecimationFactor;

    if ((VIDBUFF_Data_p->DeviceType != STVID_DEVICE_TYPE_7100_H264) &&
        (VIDBUFF_Data_p->DeviceType != STVID_DEVICE_TYPE_7109_H264) &&
        (VIDBUFF_Data_p->DeviceType != STVID_DEVICE_TYPE_7200_H264))
    {
        /* Errors */
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (VIDBUFF_Data_p->Profile.ApplicableDecimationFactor == STVID_DECIMATION_FACTOR_NONE)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

#ifdef USE_EXTRA_FRAME_BUFFERS
        VIDBUFF_Data_p->IsMemoryAllocatedForProfile = TRUE;

        /* Check if extra frame buffers needs to be allocated or if they are already allocated */
        /* Extra frame buffers can be allocated in 2 ways: */
        /* - when viewport is opened (call to VIDBUFF_SetNbrOfExtraPrimaryFrameBuffers) if memory has already been allocated for profile */
        /* - when memory is being allocated for profile if viewport has already been opened */
        if (VIDBUFF_Data_p->NumberOfExtraSecondaryFrameBuffersRequested != 0)
        {
            /* Reserve the specified number of SD frame buffers */
            if(GetNbrOfFrameBufferAllocatedInAPool(BuffersHandle, VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p) !=
                VIDBUFF_Data_p->NumberOfExtraSecondaryFrameBuffersRequested)
            {
                VIDBUFF_SetNbrOfExtraSecondaryFrameBuffers(BuffersHandle, VIDBUFF_Data_p->NumberOfExtraSecondaryFrameBuffersRequested);
            }
        }
#endif /* USE_EXTRA_FRAME_BUFFERS */

    /* Allow one more decimated buffer allocation by default to handle dual display */
    MaxDecimFramesNumber = VIDBUFF_Data_p->FrameBufferDynAlloc.NumberOfFrames + 1;

    AllocParams.AllocationMode  = VIDBUFF_ALLOCATION_MODE_SHARED_MEMORY; /* !!! Should be passed */
    AllocParams.PictureWidth    = VIDBUFF_Data_p->FrameBufferDynAlloc.Width;
    AllocParams.PictureHeight   = VIDBUFF_Data_p->FrameBufferDynAlloc.Height;
    AllocParams.BufferType      = ((VIDBUFF_Data_t *) BuffersHandle)->FrameBuffersType;

    /* Save of width and height of the picture size, to calculate the corresponding size of frame buffers.        */
    /* Those valuse are rounded up to the nearest modulo 16 (macroblock size in pixels).                          */


    WidthAfterDecimationFactor  = (((AllocParams.PictureWidth + 15) / 16) * 16);
    HeightAfterDecimationFactor = (((AllocParams.PictureHeight + 15) / 16) * 16);

    /* Apply the compression level and decimation factor to the buffer size. It's assumed that if value are
       the defaults one, nothing will be done to those buffer sizes. */
    if (HALBUFF_ApplyCompressionLevelToPictureSize(VIDBUFF_Data_p->HALBuffersHandle,
            VIDBUFF_Data_p->Profile.ApiProfile.CompressionLevel,
            &WidthAfterDecimationFactor, &HeightAfterDecimationFactor) != ST_NO_ERROR)
    {
        /* Error in input parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }
    if (HALBUFF_ApplyDecimationFactorToPictureSize(VIDBUFF_Data_p->HALBuffersHandle,
            VIDBUFF_Data_p->Profile.ApplicableDecimationFactor,
            &WidthAfterDecimationFactor, &HeightAfterDecimationFactor) != ST_NO_ERROR)
    {
        /* Error in input parameters */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Initialization of allocation parameters for secondary frame buffers. */
    HALSecondaryAllocParams.AllocationMode = AllocParams.AllocationMode;
    HALSecondaryAllocParams.BufferType     = AllocParams.BufferType;
    HALSecondaryAllocParams.PictureWidth   = WidthAfterDecimationFactor;
    HALSecondaryAllocParams.PictureHeight  = HeightAfterDecimationFactor;

    ErrorCode = AllocateNewFrameBuffer(BuffersHandle, &AllocParams, VIDBUFF_Data_p->Profile, MaxDecimFramesNumber,
        &(VIDBUFF_Data_p->AllocatedSecondaryFrameBuffersLoop_p), HALSecondaryAllocParams, VIDBUFF_Data_p->Profile.ApiProfile.FrameStoreIDParams.VariableInFixedSize.Decimated,
        &(VIDBUFF_Data_p->NumberOfSecondaryAllocatedFrameBuffers), VIDBUFF_SECONDARY_RECONSTRUCTION, HALBUFF_FRAMEBUFFER_DECIMATED, NeedForExtraAllocationDecimated_p, MaxSizedReached_p);

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Allow to send IMPOSSIBLE_WITH_PROFILE_EVT after successful allocation. */
        VIDBUFF_Data_p->IsNotificationOfImpossibleWithProfileAllowed = TRUE;
    }

    return(ErrorCode);
} /* End of VIDBUFF_AllocateNewDecimatedFrameBuffer() function */

/*******************************************************************************
Name        : VIDBUFF_AllocateNewFrameBuffer
Description :
Parameters  : Video buffer manager handle
Assumptions : The dynamic frame buffer allocation is used for H264 instances _only_
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER if too many buffers required
              ST_ERROR_NO_MEMORY if STAVMEM allocation failed
              ST_NO_ERROR if everything OK
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_AllocateNewFrameBuffer(const VIDBUFF_Handle_t BuffersHandle, BOOL * const NeedForExtraAllocation_p, BOOL * const MaxSizedReached_p)
{
    VIDBUFF_AllocateBufferParams_t  AllocParams;
    HALBUFF_AllocateBufferParams_t  HALPrimaryAllocParams;
    ST_ErrorCode_t                  ErrorCode;
    VIDBUFF_Data_t * VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;

    if ((VIDBUFF_Data_p->DeviceType != STVID_DEVICE_TYPE_7100_H264) &&
        (VIDBUFF_Data_p->DeviceType != STVID_DEVICE_TYPE_7109_H264) &&
        (VIDBUFF_Data_p->DeviceType != STVID_DEVICE_TYPE_7200_H264))
    {
        /* Errors */
        return(ST_ERROR_BAD_PARAMETER);
    }

#ifdef USE_EXTRA_FRAME_BUFFERS
        VIDBUFF_Data_p->IsMemoryAllocatedForProfile = TRUE;

        /* Check if extra frame buffers needs to be allocated or if they are already allocated */
        /* Extra frame buffers can be allocated in 2 ways: */
        /* - when viewport is opened (call to VIDBUFF_SetNbrOfExtraPrimaryFrameBuffers) if memory has already been allocated for profile */
        /* - when memory is being allocated for profile if viewport has already been opened */
        if (VIDBUFF_Data_p->NumberOfExtraPrimaryFrameBuffersRequested !=  0)
        {
            if(GetNbrOfFrameBufferAllocatedInAPool(BuffersHandle, VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p) !=
                VIDBUFF_Data_p->NumberOfExtraPrimaryFrameBuffersRequested)
            {
                /* Reserve the specified number of SD frame buffers */
                VIDBUFF_SetNbrOfExtraPrimaryFrameBuffers(BuffersHandle, VIDBUFF_Data_p->NumberOfExtraPrimaryFrameBuffersRequested);
            }
        }
#endif /* USE_EXTRA_FRAME_BUFFERS */

    AllocParams.AllocationMode  = VIDBUFF_ALLOCATION_MODE_SHARED_MEMORY; /* !!! Should be passed */
    AllocParams.PictureWidth    = VIDBUFF_Data_p->FrameBufferDynAlloc.Width;
    AllocParams.PictureHeight   = VIDBUFF_Data_p->FrameBufferDynAlloc.Height;
    AllocParams.BufferType      = ((VIDBUFF_Data_t *) BuffersHandle)->FrameBuffersType;

    /* Initialization of allocation parameters for primary frame buffers. */
    HALPrimaryAllocParams.AllocationMode = AllocParams.AllocationMode;
    HALPrimaryAllocParams.BufferType     = AllocParams.BufferType;
    HALPrimaryAllocParams.PictureWidth   = AllocParams.PictureWidth;
    HALPrimaryAllocParams.PictureHeight  = AllocParams.PictureHeight;

    ErrorCode = AllocateNewFrameBuffer(BuffersHandle, &AllocParams, VIDBUFF_Data_p->Profile, VIDBUFF_Data_p->FrameBufferDynAlloc.NumberOfFrames,
        &(VIDBUFF_Data_p->AllocatedFrameBuffersLoop_p), HALPrimaryAllocParams, VIDBUFF_Data_p->Profile.ApiProfile.FrameStoreIDParams.VariableInFixedSize.Main,
        &(VIDBUFF_Data_p->NumberOfAllocatedFrameBuffers), VIDBUFF_MAIN_RECONSTRUCTION, HALBUFF_FRAMEBUFFER_PRIMARY, NeedForExtraAllocation_p, MaxSizedReached_p);

    if (ErrorCode == ST_NO_ERROR)
    {
        /* Allow to send IMPOSSIBLE_WITH_PROFILE_EVT after successful allocation. */
        VIDBUFF_Data_p->IsNotificationOfImpossibleWithProfileAllowed = TRUE;
    }

    return(ErrorCode);
} /* End of VIDBUFF_AllocateNewFrameBuffer() function */

/*******************************************************************************
Name        : GetNbrOfFrameBufferAllocatedInAPool
Description : This function counts the number of Frame Buffers allocated in the specified Frame
                    Buffer Pool.
                    The ones tagged 'ToBeKilledAsSoonAsPossible' are not count.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
#ifdef USE_EXTRA_FRAME_BUFFERS
static U32 GetNbrOfFrameBufferAllocatedInAPool(const VIDBUFF_Handle_t BuffersHandle,
                                                                                           VIDBUFF_FrameBuffer_t * FrameBufferLoop_p)
{
    U32     NumberAlreadyAllocated = 0;
    VIDBUFF_FrameBuffer_t * CurrentFrame_p;
    UNUSED_PARAMETER(BuffersHandle);

    /* Init CurrentFrame_p at the begining of the Pool */
    CurrentFrame_p = FrameBufferLoop_p;

    if (CurrentFrame_p != NULL)
    {
        /* Warning: The Frame Buffer pool is a Loop */
        do
        {
            if (!(CurrentFrame_p->ToBeKilledAsSoonAsPossible))
            {
                NumberAlreadyAllocated++;
            }

            /* Get the next frame buffer in the loop. */
            CurrentFrame_p = CurrentFrame_p->NextAllocated_p;
        }
        while ((CurrentFrame_p != NULL) && (CurrentFrame_p != FrameBufferLoop_p));
    }

    return (NumberAlreadyAllocated);
}
#endif  /* USE_EXTRA_FRAME_BUFFERS */

/*******************************************************************************
Name        : DeAllocateFrameBuffer
Description : Deallocate the specified Frame Buffer (from Primary or Secondary or Extra queue)
Parameters  :
Assumptions : Call to this function must be protected by AllocatedFrameBuffersLoopLockingSemaphore_p
Limitations : This function is not in charge of removing the frame buffer from the Pool!
Returns     :
*******************************************************************************/
static ST_ErrorCode_t DeAllocateFrameBuffer(const VIDBUFF_Handle_t BuffersHandle,
                                            VIDBUFF_FrameBuffer_t *    FrameBufferToRemove_p)
{
    VIDBUFF_Data_t *    VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
#ifdef USE_EXTRA_FRAME_BUFFERS
    BOOL                IsAnExtraBuffer;
#endif

    TrDealloc(("\r\nDeAllocateFrameBuffer 0x%08x", FrameBufferToRemove_p));

    if (FrameBufferToRemove_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

#ifdef USE_EXTRA_FRAME_BUFFERS
    if ((FrameBufferToRemove_p->FrameBufferType == VIDBUFF_EXTRA_PRIMARY_FRAME_BUFFER)
            ||(FrameBufferToRemove_p->FrameBufferType == VIDBUFF_EXTRA_SECONDARY_FRAME_BUFFER))
    {
        IsAnExtraBuffer = TRUE;
    }
    else
    {
        IsAnExtraBuffer = FALSE;
    }
#endif /* USE_EXTRA_FRAME_BUFFERS */

    FrameBufferToRemove_p->AllocationIsDone = FALSE;
    FrameBufferToRemove_p->FrameBufferType = VIDBUFF_UNKNOWN_FRAME_BUFFER_TYPE;
    FrameBufferToRemove_p->ToBeKilledAsSoonAsPossible = FALSE;

    /* Release pictures attached to buffer */
    if (FrameBufferToRemove_p->FrameOrFirstFieldPicture_p != NULL)
    {
        TrDealloc(("\r\nRelease Picture Frameor1stField 0x%x from FB @ 0x%x, ",
                FrameBufferToRemove_p->FrameOrFirstFieldPicture_p,
                FrameBufferToRemove_p->Allocation.Address_p));
        LeavePicture(BuffersHandle, FrameBufferToRemove_p->FrameOrFirstFieldPicture_p);
        FrameBufferToRemove_p->FrameOrFirstFieldPicture_p = NULL;
    }

    if (FrameBufferToRemove_p->NothingOrSecondFieldPicture_p != NULL)
    {
        TrDealloc(("\r\nRelease Picture 2ndfield 0x%x from FB @ 0x%x, ",
                FrameBufferToRemove_p->NothingOrSecondFieldPicture_p,
                FrameBufferToRemove_p->Allocation.Address_p));
        LeavePicture(BuffersHandle, FrameBufferToRemove_p->NothingOrSecondFieldPicture_p);
        FrameBufferToRemove_p->NothingOrSecondFieldPicture_p = NULL;
    }

#ifdef USE_EXTRA_FRAME_BUFFERS
    if (IsAnExtraBuffer)
    {
        switch (FrameBufferToRemove_p->AvailableReconstructionMode)
        {
            case VIDBUFF_MAIN_RECONSTRUCTION:
                TrMemAlloc(("\r\nDeAllocating extra frame buffer from address 0x%08x", FrameBufferToRemove_p->Allocation.Address_p));
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DeAllocating extra frame buffer from address 0x%08x", FrameBufferToRemove_p->Allocation.Address_p));
                HALBUFF_DeAllocateFrameBuffer(VIDBUFF_Data_p->HALBuffersHandle, &(FrameBufferToRemove_p->Allocation), HALBUFF_FRAMEBUFFER_PRIMARY);
                break;

            case VIDBUFF_SECONDARY_RECONSTRUCTION:
                TrMemAlloc(("\r\nDeAllocating extra secondary frame buffer from address 0x%08x", FrameBufferToRemove_p->Allocation.Address_p));
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DeAllocating extra frame buffer from address 0x%08x", FrameBufferToRemove_p->Allocation.Address_p));
                HALBUFF_DeAllocateFrameBuffer(VIDBUFF_Data_p->HALBuffersHandle, &(FrameBufferToRemove_p->Allocation), HALBUFF_FRAMEBUFFER_DECIMATED);
                break;

            default:
                TrError(("\r\nError DeAllocate extraFrameBuffer! Invalid ReconstructionMode (%d)", FrameBufferToRemove_p->AvailableReconstructionMode));
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Error DeAllocateExtraFrameBuffer! Invalid ReconstructionMode (%d)", FrameBufferToRemove_p->AvailableReconstructionMode));
                break;

        }
    }
    else
#endif /* USE_EXTRA_FRAME_BUFFERS */
    {
        /* This is an ordinary Frame Buffer*/
        switch (FrameBufferToRemove_p->AvailableReconstructionMode)
        {
            case VIDBUFF_MAIN_RECONSTRUCTION:
                TrMemAlloc(("\r\nDeAllocating primary frame buffer from address 0x%08x", FrameBufferToRemove_p->Allocation.Address_p));
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DeAllocating primary frame buffer 0x%x from address 0x%08x", FrameBufferToRemove_p, FrameBufferToRemove_p->Allocation.Address_p));
                HALBUFF_DeAllocateFrameBuffer(VIDBUFF_Data_p->HALBuffersHandle, &(FrameBufferToRemove_p->Allocation), HALBUFF_FRAMEBUFFER_PRIMARY);
                break;

            case VIDBUFF_SECONDARY_RECONSTRUCTION:
                TrMemAlloc(("\r\nDeAllocating decimated frame buffer from address 0x%08x", FrameBufferToRemove_p->Allocation.Address_p));
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DeAllocating decimated frame buffer from address 0x%08x", FrameBufferToRemove_p->Allocation.Address_p));
                HALBUFF_DeAllocateFrameBuffer(VIDBUFF_Data_p->HALBuffersHandle, &(FrameBufferToRemove_p->Allocation), HALBUFF_FRAMEBUFFER_DECIMATED);
                break;

            default:
            case VIDBUFF_NONE_RECONSTRUCTION:
            case VIDBUFF_BOTH_RECONSTRUCTION:
                TrError(("\r\nError DeAllocateFrameBuffer! Invalid ReconstructionMode (%d)", FrameBufferToRemove_p->AvailableReconstructionMode));
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Error DeAllocateFrameBuffer! Invalid ReconstructionMode (%d)", FrameBufferToRemove_p->AvailableReconstructionMode));
                break;
        }
    }

    FrameBufferToRemove_p->CompressionLevel                    = STVID_COMPRESSION_LEVEL_NONE;
    FrameBufferToRemove_p->DecimationFactor                    = STVID_DECIMATION_FACTOR_NONE;
    FrameBufferToRemove_p->AvailableReconstructionMode         = VIDBUFF_NONE_RECONSTRUCTION;
    FrameBufferToRemove_p->HasDecodeToBeDoneWhileDisplaying    = FALSE;
    FrameBufferToRemove_p->NextAllocated_p = NULL;

#ifdef USE_EXTRA_FRAME_BUFFERS
    /* There is a major difference between 'Primary/Secondary' Frame Buffers and Extra Frame Buffers:
        - For Primary and Secondary Frame Buffers, the Frame Buffer structure is taken from tables allocated at init
            ('FrameBuffers_p' and 'SecondaryFrameBuffers_p').
        - For Extra Frame Buffers, the Frame Buffer structure is allocated fully dynamically.
        So we should Deallocate this Frame Buffer Structure */
    if (IsAnExtraBuffer)
    {
        SAFE_MEMORY_DEALLOCATE( FrameBufferToRemove_p, VIDBUFF_Data_p->CPUPartition_p, sizeof(VIDBUFF_FrameBuffer_t));
    }
#endif /* USE_EXTRA_FRAME_BUFFERS */

    return (ST_NO_ERROR);
}


#ifdef USE_EXTRA_FRAME_BUFFERS
/*******************************************************************************
Name        : VIDBUFF_SetNbrOfExtraPrimaryFrameBuffers
Description : Set the number of extra SD Frame Buffers.
                    These SD Frame Buffers are for instance used as picture reference in case of Deinterlacing.
Parameters  : Video buffer manager handle
Assumptions : This function should be called BEFORE calling VIDBUFF_AllocateMemoryForProfile()
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER if too many buffers required
              ST_NO_ERROR if everything OK
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_SetNbrOfExtraPrimaryFrameBuffers(const VIDBUFF_Handle_t BuffersHandle, const U32 NumberOfBuffers)
{
    VIDBUFF_Data_t *                            VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
    VIDBUFF_AllocateBufferParams_t       AllocParams;
    ST_ErrorCode_t                                ErrorCode;

    VIDBUFF_Data_p->NumberOfExtraPrimaryFrameBuffersRequested = NumberOfBuffers;
    TrExtraFrameBuffer(("\r\n%d extra SD Frame Buffers requested", NumberOfBuffers));

    /* Check if extra frame buffers needs to be allocated or if they are already allocated */
    /* Extra frame buffers can be allocated in 2 ways: */
    /* - when viewport is opened (call to VIDBUFF_SetNbrOfExtraPrimaryFrameBuffers) if memory has already been allocated for profile */
    /* - when memory is being allocated (call to VIDBUFF_AllocateMemoryForProfile)for profile if viewport has already been opened */
    if(((VIDBUFF_Data_t *) BuffersHandle)->IsMemoryAllocatedForProfile == TRUE)
    {
        /* Memory allocation of Extra SD Frame Buffers*/
        if (VIDBUFF_Data_p->NumberOfExtraPrimaryFrameBuffersRequested != 0)
        {
            AllocParams.AllocationMode  = VIDBUFF_ALLOCATION_MODE_SHARED_MEMORY;
            AllocParams.PictureWidth    = EXTRA_FRAME_BUFFER_MAX_WIDTH;
            AllocParams.PictureHeight   = EXTRA_FRAME_BUFFER_MAX_HEIGHT;
            AllocParams.BufferType      = ((VIDBUFF_Data_t *) BuffersHandle)->FrameBuffersType;  /* The Extra Frame Buffers are from the same type than the main Frame Buffers */

            ErrorCode = AllocateExtraPrimaryFrameBufferPool(BuffersHandle, &AllocParams, VIDBUFF_Data_p->NumberOfExtraPrimaryFrameBuffersRequested);
        }
        else
        {
        }
    }
    return (ST_NO_ERROR);
}

/*******************************************************************************
Name        : AllocateExtraPrimaryFrameBufferPool
Description : Allocates Extra primary SD frame buffers and links them into a linked
                   loop of allocated extra frame buffers, pointed by AllocatedExtraPrimaryFrameBuffersLoop_p.
Parameters  : Buffer manager handle, allocation parameters, number of extra frame buffers.
Assumptions : Decimation modes and Compression modes are MUTUALLY EXCLUSIVE.
Limitations : The BufferSize field from AllocParams_p is not used.
Returns     : ST_ERROR_BAD_PARAMETER if too many buffers required
              ST_ERROR_NO_MEMORY if STAVMEM allocation failed
              ST_NO_ERROR if everything OK
*******************************************************************************/
static ST_ErrorCode_t AllocateExtraPrimaryFrameBufferPool(const VIDBUFF_Handle_t BuffersHandle,
                                            VIDBUFF_AllocateBufferParams_t * const AllocParams_p,
                                            const U32   NbrOfExtraFrameBuffersRequested)
{
    VIDBUFF_FrameBuffer_t *         AllocatedFrameBuffers_p;
    VIDBUFF_FrameBuffer_t *         LastFrameBuffer_p;
    VIDBUFF_FrameBuffer_t *         FirstFrameBuffer_p;
    VIDBUFF_FrameBuffer_t *         OldNextAllocated_p;
    VIDBUFF_Data_t *                VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
#ifdef ST_OSLINUX
    FrameBuffers_t *                FrameBuffers_p = NULL;
    U32                             NbFrameBuffers = 0;
#endif

    if ( (AllocParams_p == NULL) ||
          (NbrOfExtraFrameBuffersRequested == 0) ||
          (NbrOfExtraFrameBuffersRequested > MAX_EXTRA_FRAME_BUFFER_NBR) )
    {
        /* Errors */
        return(ST_ERROR_BAD_PARAMETER);
    }

#ifdef ST_OSLINUX
    NbFrameBuffers = 0;
    FrameBuffers_p = (FrameBuffers_t *) STOS_MemoryAllocate(NULL, MAX_EXTRA_FRAME_BUFFER_NBR * sizeof(FrameBuffers_t) );
#endif

    /* Lock access to allocated frame buffers loop */
    semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    /* NB: We need to re-count the remaining number of extra Frame Buffers because some of them may be tagged
                "ToBeKilledAsSoonAsPossible" (so we can not simply use "VIDBUFF_Data_p->NumberOfAllocatedExtraPrimaryFrameBuffers") */
    VIDBUFF_Data_p->NumberOfAllocatedExtraPrimaryFrameBuffers = GetNbrOfFrameBufferAllocatedInAPool(BuffersHandle,
                                                                                                       VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p);

      if (NbrOfExtraFrameBuffersRequested < VIDBUFF_Data_p->NumberOfAllocatedExtraPrimaryFrameBuffers)
    {
        /* We have more Extra Frame Buffers than necessary: We keep them anyway */
        ErrorCode = ST_NO_ERROR;
        goto end;
    }

    if (NbrOfExtraFrameBuffersRequested == VIDBUFF_Data_p->NumberOfAllocatedExtraPrimaryFrameBuffers)
    {
        /* There are enough Extra Frame Buffer allocated: Nothing to do */
        ErrorCode = ST_NO_ERROR;
        goto end;
    }

    /***********************************/
    /* We should allocate more Extra Buffers */

    /* NB: It is not necessary to handle HDPIPParams because these extra frame buffers will contain only SD */

    if (    (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2) ||
            (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2) ||
            (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2))
    {
        /* For MPEG4-P2, we need 64 bits alligned buffers */
        AllocParams_p->PictureWidth = ((AllocParams_p->PictureWidth + 32 + 63) / 64) * 64;
        AllocParams_p->PictureHeight = AllocParams_p->PictureHeight + 32;
    }
    else if (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_AVS)
    {
        /* PLE_TO_DO :  For AVS, the extra frame buffers usage is DEI. so the allocation constraints should */
        /* be the same than the normal omega2 buffers allocation. To be updated */
        AllocParams_p->PictureWidth = ((AllocParams_p->PictureWidth + 64 + 63) / 64) *64;
        AllocParams_p->PictureHeight = AllocParams_p->PictureHeight + 64;
    }


    TrMemAlloc(("\r\nAllocate %d Extra Frame buffers [%d;%d]",
            NbrOfExtraFrameBuffersRequested - VIDBUFF_Data_p->NumberOfAllocatedExtraPrimaryFrameBuffers,
            AllocParams_p->PictureWidth,
            AllocParams_p->PictureHeight));

    LastFrameBuffer_p = NULL;
    FirstFrameBuffer_p = NULL;

    /******************************/
    /* Start the Frame Buffer allocation */
    while ( (VIDBUFF_Data_p->NumberOfAllocatedExtraPrimaryFrameBuffers < NbrOfExtraFrameBuffersRequested) &&
                (ErrorCode == ST_NO_ERROR) )
    {
        ErrorCode = AllocateExtraPrimaryFrameBuffer(BuffersHandle,
                                                                       AllocParams_p,
                                                                       &AllocatedFrameBuffers_p);

        if (ErrorCode == ST_NO_ERROR)
        {
            if (LastFrameBuffer_p == NULL)
            {
                /* This is the First Frame Buffer allocated */
                LastFrameBuffer_p = AllocatedFrameBuffers_p;
                FirstFrameBuffer_p = AllocatedFrameBuffers_p;
            }
            else
            {
                /* Append the new Frame Buffer to the tail of the queue */
                LastFrameBuffer_p->NextAllocated_p = AllocatedFrameBuffers_p;
                LastFrameBuffer_p = AllocatedFrameBuffers_p;
            }
            VIDBUFF_Data_p->NumberOfAllocatedExtraPrimaryFrameBuffers++;
#ifdef ST_OSLINUX
            FrameBuffers_p[NbFrameBuffers].KernelAddress_p = AllocatedFrameBuffers_p->Allocation.Address_p;
            FrameBuffers_p[NbFrameBuffers].Size = AllocatedFrameBuffers_p->Allocation.TotalSize;
            NbFrameBuffers ++;
#endif

        }
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Not enough memory to allocate the required nbr of extra Frame Buffer (%d instead of %d)",
                        VIDBUFF_Data_p->NumberOfAllocatedExtraPrimaryFrameBuffers,
                        NbrOfExtraFrameBuffersRequested));
            /* NB: We keep the one allocated anyway*/
        }
    }

    if (LastFrameBuffer_p != NULL)
    {
        /* Some Frame Buffers have been allocated in a row. We should now put them in
             AllocatedExtraPrimaryFrameBuffersLoop_p and make it a loop */
        if (VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p == NULL)
        {
            /* The list was empty so we create it and make it a loop */
            VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p = FirstFrameBuffer_p;
            LastFrameBuffer_p->NextAllocated_p = FirstFrameBuffer_p;
        }
        else
        {
            /* We cut the existing loop to insert the new frame buffers */
            OldNextAllocated_p = VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p->NextAllocated_p;
            VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p->NextAllocated_p = FirstFrameBuffer_p;
            LastFrameBuffer_p->NextAllocated_p = OldNextAllocated_p;
        }
    }

#ifdef ST_OSLINUX
    /* Notify newly extra FB allocated */
    NotifyNewFrameBuffers(BuffersHandle, FrameBuffers_p, NbFrameBuffers);
#endif


end:

#ifdef ST_OSLINUX
    if (FrameBuffers_p != NULL)
    {
        /* Free memory before leaving... */
        STOS_MemoryDeallocate(NULL, FrameBuffers_p);
    }
#endif

    /* Un-lock access to allocated frame buffers loop */
    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    return(ErrorCode);
}/* End of AllocateExtraPrimaryFrameBufferPool() function */

/*******************************************************************************
Name        : DeAllocateExtraPrimaryFrameBufferPool
Description : This function deallocates the extra primary Frame Buffers.
Parameters  :
Assumptions : At this stage the extra Frame Buffers are all considered as unused.
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t DeAllocateExtraPrimaryFrameBufferPool(const VIDBUFF_Handle_t BuffersHandle)
{
    VIDBUFF_FrameBuffer_t *  CurrentFrame_p;
    VIDBUFF_FrameBuffer_t *  FrameToRemove_p;
    VIDBUFF_Data_t *         VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
#ifdef ST_OSLINUX
    FrameBuffers_t *         FrameBuffers_p = NULL;
    U32                      NbFrameBuffers = 0;
#endif

     /* Nothing to de-allocate ! */
    if (VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p == NULL)
    {
        return(ST_NO_ERROR);
    }

    TrMain(("\r\nDeAllocateExtraPrimaryFrameBufferPool"));

#ifdef ST_OSLINUX
    /* Allocates max possible FB entries (extra only) */
    FrameBuffers_p = (FrameBuffers_t *) STOS_MemoryAllocate(NULL, MAX_EXTRA_FRAME_BUFFER_NBR * sizeof(FrameBuffers_t) );

    if (FrameBuffers_p == NULL)
    {
        TrError(("\r\nError! Not enough memory for %d deallocations", MAX_EXTRA_FRAME_BUFFER_NBR));
        return(ST_ERROR_NO_MEMORY);
    }
#endif

    /* Lock access to allocated frame buffers loop */
    semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    /* De-allocate previously allocated buffers and quit with error */
    CurrentFrame_p = VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p;

    while (CurrentFrame_p != NULL)
    {
        /* NB: In case of extra Frame Buffer, the structure "VIDBUFF_FrameBuffer_t" is fully deallocated. */
        FrameToRemove_p = CurrentFrame_p;
        CurrentFrame_p = CurrentFrame_p->NextAllocated_p;

#ifdef ST_OSLINUX
        if (NbFrameBuffers < MAX_EXTRA_FRAME_BUFFER_NBR)
        {
            /* Store this FB has being removed */
            FrameBuffers_p[NbFrameBuffers].KernelAddress_p = FrameToRemove_p->Allocation.Address_p;
            FrameBuffers_p[NbFrameBuffers].Size = FrameToRemove_p->Allocation.TotalSize;
            NbFrameBuffers++;
        }
        else
        {
            TrError(("\r\nError! Too many entries in FrameBuffers_p (%d)", NbFrameBuffers));
        }
#endif

        DeAllocateFrameBuffer(BuffersHandle, FrameToRemove_p);

        if (CurrentFrame_p == VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p)
        {
            /* Finished going through all buffers */
            CurrentFrame_p = NULL;
        }
    } /* while CurrentFrame_p != NULL */

    VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p = NULL;
    VIDBUFF_Data_p->NumberOfAllocatedExtraPrimaryFrameBuffers = 0;

#ifdef ST_OSLINUX
    /* Notify removed extra FB */
    NotifyRemovedFrameBuffers(BuffersHandle, FrameBuffers_p, NbFrameBuffers);
    if (FrameBuffers_p != NULL)
    {
        STOS_MemoryDeallocate(NULL, FrameBuffers_p);
    }
#endif

    /* Lock access to allocated frame buffers loop */
    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    return(ST_NO_ERROR);
}/* End of DeAllocateExtraPrimaryFrameBufferPool() function */

/*******************************************************************************
Name        : AllocateExtraPrimaryFrameBuffer
Description : Allocate ONE extra primary SD Frame Buffer
Parameters  :
Assumptions : Call to this function must be protected by AllocatedFrameBuffersLoopLockingSemaphore_p
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t AllocateExtraPrimaryFrameBuffer(const VIDBUFF_Handle_t BuffersHandle,
                        const VIDBUFF_AllocateBufferParams_t * const AllocParams_p,
                        VIDBUFF_FrameBuffer_t **                     AllocatedFrameBuffer_p_p)
{
    VIDBUFF_Data_t *                VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
    VIDBUFF_FrameBuffer_t *         FrameBuffers_p;
    HALBUFF_AllocateBufferParams_t  HALAllocParams;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    /* Set AllocatedFrameBuffer_p_p to NULL (in case of error) */
    *AllocatedFrameBuffer_p_p = NULL;

    /* FIRST: Allocate a Frame Buffer Structure */
    SAFE_MEMORY_ALLOCATE(FrameBuffers_p,
                                               VIDBUFF_FrameBuffer_t *,
                                               VIDBUFF_Data_p->CPUPartition_p,
                                               sizeof(VIDBUFF_FrameBuffer_t) );

    if (FrameBuffers_p == NULL)
    {
        ErrorCode = ST_ERROR_NO_MEMORY;
        goto end;
    }

    /* Change VIDBUFF Params to HALBUFF Params... */
    HALAllocParams.AllocationMode = AllocParams_p->AllocationMode;
    HALAllocParams.BufferType      = AllocParams_p->BufferType;
    HALAllocParams.PictureWidth    = AllocParams_p->PictureWidth;
    HALAllocParams.PictureHeight   = AllocParams_p->PictureHeight;


    /* Then allocate the memory associated to this Frame Buffer */
    if (HALBUFF_AllocateFrameBuffer(VIDBUFF_Data_p->HALBuffersHandle,
                                                            &HALAllocParams,
                                                            &(FrameBuffers_p->Allocation),
                                                            HALBUFF_FRAMEBUFFER_PRIMARY) != ST_NO_ERROR)
    {
        /* Error: Allocation could not be performed ! */
        TrError(("\r\nAllocation of a extra frame buffer failed"));

        /* We should Deallocate the Frame Buffer Structure */
        SAFE_MEMORY_DEALLOCATE( FrameBuffers_p, VIDBUFF_Data_p->CPUPartition_p, sizeof(VIDBUFF_FrameBuffer_t));
        ErrorCode = ST_ERROR_NO_MEMORY;
        goto end;
    }

    /* The Frame Buffer allocation was successful */
    TrMemAlloc(("\r\nAllocated extra frame buffer at address 0x%08x", FrameBuffers_p->Allocation.Address_p));
    TrMemAlloc(("\t  Total: 0x%08x, Y: 0x%08x, C: 0x%08x",
                        FrameBuffers_p->Allocation.TotalSize,
                        FrameBuffers_p->Allocation.TotalSize-FrameBuffers_p->Allocation.Size2,
                        FrameBuffers_p->Allocation.Size2));

    FrameBuffers_p->AvailableReconstructionMode = VIDBUFF_MAIN_RECONSTRUCTION;
    FrameBuffers_p->AllocationIsDone = TRUE;
    FrameBuffers_p->FrameBufferType = VIDBUFF_EXTRA_PRIMARY_FRAME_BUFFER;
    FrameBuffers_p->FrameOrFirstFieldPicture_p    = NULL;
    FrameBuffers_p->NothingOrSecondFieldPicture_p = NULL;
    FrameBuffers_p->CompressionLevel = STVID_COMPRESSION_LEVEL_NONE;
    FrameBuffers_p->DecimationFactor = STVID_DECIMATION_FACTOR_NONE;
    FrameBuffers_p->HasDecodeToBeDoneWhileDisplaying    = FALSE;
    FrameBuffers_p->NextAllocated_p = NULL;
    FrameBuffers_p->ToBeKilledAsSoonAsPossible = FALSE;

    *AllocatedFrameBuffer_p_p = FrameBuffers_p;

end:

    return(ErrorCode);
}

/*******************************************************************************
Name        : VIDBUFF_DeAllocateUnusedExtraBuffers
Description : Deallocation of allocated extra frame buffers not used for the decode/display process.
              If the Frame Buffer is in use, it is tagged "ToBeKilledAsSoonAsPossible"

              Warning: This function can be called while the Decoder is running.
Parameters  : Video buffer manager handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if deallocation done successfully.
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_DeAllocateUnusedExtraBuffers(const VIDBUFF_Handle_t BuffersHandle)
{
    VIDBUFF_FrameBuffer_t *     CurrentFrame_p;
    VIDBUFF_FrameBuffer_t *     PreviousFrame_p;
    VIDBUFF_Data_t *            VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
#ifdef ST_OSLINUX
    FrameBuffers_t *            FrameBuffers_p = NULL;
    U32                         NbFrameBuffers = 0;
#endif

    if (VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p == NULL) /* Nothing to de-allocate ! */
    {
        return(ST_NO_ERROR);
    }

    TrMain(("\r\nVIDBUFF_DeAllocateUnusedExtraBuffers"));

#ifdef ST_OSLINUX
    /* Allocates max possible FB entries (extra only) */
    FrameBuffers_p = (FrameBuffers_t *) STOS_MemoryAllocate(NULL, MAX_EXTRA_FRAME_BUFFER_NBR * sizeof(FrameBuffers_t) );

    if (FrameBuffers_p == NULL)
    {
        TrError(("\r\nError! Not enough memory for %d deallocations", MAX_EXTRA_FRAME_BUFFER_NBR));
        return(ST_ERROR_NO_MEMORY);
    }
#endif

    /* Lock access to allocated frame buffers loop */
    semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    /* De-allocate previously allocated buffers and quit with error */
    /*--------------*/
    PreviousFrame_p = VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p;
    CurrentFrame_p = PreviousFrame_p->NextAllocated_p;

    while ((CurrentFrame_p != VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p) && (CurrentFrame_p != NULL))
    {
        if (!(HasAPictureLocked(*CurrentFrame_p)))
        {
            /* Frame Buffer not used: de-allocate it */

            /* Remove it from the Pool */
            PreviousFrame_p->NextAllocated_p = CurrentFrame_p->NextAllocated_p;
            VIDBUFF_Data_p->NumberOfAllocatedExtraPrimaryFrameBuffers --;

#ifdef ST_OSLINUX
           if (NbFrameBuffers < MAX_EXTRA_FRAME_BUFFER_NBR)
           {
                /* Store this FB has being removed */
                FrameBuffers_p[NbFrameBuffers].KernelAddress_p = CurrentFrame_p->Allocation.Address_p;
                FrameBuffers_p[NbFrameBuffers].Size = CurrentFrame_p->Allocation.TotalSize;
                NbFrameBuffers++;
           }
           else
           {
               TrError(("\r\nError! Too many entries in FrameBuffers_p (%d)", NbFrameBuffers));
           }
#endif
            DeAllocateFrameBuffer(BuffersHandle, CurrentFrame_p);
        }
        else
        {
            /* Mark this Frame Buffer to be killed ASAP and move to next one */
            CurrentFrame_p->ToBeKilledAsSoonAsPossible = TRUE;
            TrMain(("\r\nMark Extra FB 0x%x ToBeKilledASAP (Picture 0x%x, %d)",
                        CurrentFrame_p,
                        CurrentFrame_p->FrameOrFirstFieldPicture_p,
                        CurrentFrame_p->FrameOrFirstFieldPicture_p->Buffers.PictureLockCounter));
            TrMemAlloc(("\r\nMark Extra FB @ 0x%x ToBeKilledASAP",  CurrentFrame_p->Allocation.Address_p));
            PreviousFrame_p = CurrentFrame_p;
        }
        CurrentFrame_p = PreviousFrame_p->NextAllocated_p;
    }

    if (CurrentFrame_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unexpected CurrentFrame_p == NULL"));
    }
    else
    {
    /* Here CurrentFrame_p == VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p: check if it is to be removed */
    if (!(HasAPictureLocked(*CurrentFrame_p)))
    {
        /* Frame Buffer not used: de-allocate it */
        if (PreviousFrame_p == CurrentFrame_p)
        {
            /* Picture is alone in loop */
            VIDBUFF_Data_p->NumberOfAllocatedExtraPrimaryFrameBuffers = 0;
            VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p = NULL;
        }
        else
        {
            /* Picture is not alone in loop */
            PreviousFrame_p->NextAllocated_p = CurrentFrame_p->NextAllocated_p;
            VIDBUFF_Data_p->AllocatedExtraPrimaryFrameBuffersLoop_p = PreviousFrame_p;
            VIDBUFF_Data_p->NumberOfAllocatedExtraPrimaryFrameBuffers --;
        }

#ifdef ST_OSLINUX
        if (NbFrameBuffers < MAX_EXTRA_FRAME_BUFFER_NBR)
        {
             /* Store this FB has being removed */
             FrameBuffers_p[NbFrameBuffers].KernelAddress_p = CurrentFrame_p->Allocation.Address_p;
             FrameBuffers_p[NbFrameBuffers].Size = CurrentFrame_p->Allocation.TotalSize;
             NbFrameBuffers++;
        }
        else
        {
            TrError(("\r\nError! Too many entries in FrameBuffers_p (%d)", NbFrameBuffers));
        }
#endif

        DeAllocateFrameBuffer(BuffersHandle, CurrentFrame_p);
    }
    else
    {
        /* Mark this Frame Buffer to be killed ASAP */
        CurrentFrame_p->ToBeKilledAsSoonAsPossible = TRUE;
        TrMain(("\r\nMark Extra FB 0x%x ToBeKilledASAP (Picture 0x%x, %d)",
                    CurrentFrame_p,
                    CurrentFrame_p->FrameOrFirstFieldPicture_p,
                    CurrentFrame_p->FrameOrFirstFieldPicture_p->Buffers.PictureLockCounter));
        TrMemAlloc(("\r\nMark Extra FB @ 0x%x ToBeKilledASAP",  CurrentFrame_p->Allocation.Address_p));
        }
    }

#ifdef ST_OSLINUX
    /* Notify removed extra FB */
    NotifyRemovedFrameBuffers(BuffersHandle, FrameBuffers_p, NbFrameBuffers);
    if (FrameBuffers_p != NULL)
    {
        STOS_MemoryDeallocate(NULL, FrameBuffers_p);
    }
#endif

    /* Un-lock access to allocated frame buffers loop */
    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

#ifdef SHOW_FRAME_BUFFER_USED
    TrError(("\r\nAfter deallocating Unused Extra Primary Frame Buffers:"));
    VIDBUFF_PrintPictureBuffersStatus(BuffersHandle);
#endif

    return(ST_NO_ERROR);
} /* End of VIDBUFF_DeAllocateUnusedExtraBuffers() function */

/*******************************************************************************
Name        : VIDBUFF_SetNbrOfExtraSecondaryFrameBuffers
Description : Set the number of extra SD secondary Frame Buffers.
                    These SD Frame Buffers are for instance used as picture reference in case of Deinterlacing.
Parameters  : Video buffer manager handle
Assumptions : This function should be called BEFORE calling VIDBUFF_AllocateMemoryForProfile()
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER if too many buffers required
              ST_NO_ERROR if everything OK
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_SetNbrOfExtraSecondaryFrameBuffers(const VIDBUFF_Handle_t BuffersHandle, const U32 NumberOfBuffers)
{
    VIDBUFF_Data_t *                            VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
    VIDBUFF_AllocateBufferParams_t       AllocParams;
    ST_ErrorCode_t                                ErrorCode;

    VIDBUFF_Data_p->NumberOfExtraSecondaryFrameBuffersRequested = NumberOfBuffers;
    TrExtraFrameBuffer(("\r\n%d extra SD Secondary Frame Buffers requested", NumberOfBuffers));

    /* Check if extra frame buffers needs to be allocated or if they are already allocated */
    /* Extra frame buffers can be allocated in 2 ways: */
    /* - when viewport is opened (call to VIDBUFF_SetNbrOfExtraPrimaryFrameBuffers) if memory has already been allocated for profile */
    /* - when memory is being allocated (call to VIDBUFF_AllocateMemoryForProfile)for profile if viewport has already been opened */
    if(((VIDBUFF_Data_t *) BuffersHandle)->IsMemoryAllocatedForProfile == TRUE)
    {
        if (((VIDBUFF_Data_t *) BuffersHandle)->Profile.NbSecondaryFrameStore != 0)
        {
            /* Memory allocation of Extra secondary SD Frame Buffers*/
            if (VIDBUFF_Data_p->NumberOfExtraSecondaryFrameBuffersRequested != 0)
            {
                AllocParams.AllocationMode  = VIDBUFF_ALLOCATION_MODE_SHARED_MEMORY;
                AllocParams.PictureWidth    = EXTRA_FRAME_BUFFER_MAX_WIDTH;
                AllocParams.PictureHeight   = EXTRA_FRAME_BUFFER_MAX_HEIGHT;
                AllocParams.BufferType      = VIDBUFF_Data_p->FrameBuffersType;  /* All frame buffers in this instance have the same type */

                ErrorCode = AllocateExtraSecondaryFrameBufferPool(BuffersHandle, &AllocParams, VIDBUFF_Data_p->NumberOfExtraSecondaryFrameBuffersRequested);
            }
            else
            {
            }
        }
    }

    return (ST_NO_ERROR);
}

/*******************************************************************************
Name        : AllocateExtraSecondaryFrameBufferPool
Description : Allocates Extra Secondary SD frame buffers and links them into a linked
                   loop of allocated extra frame buffers, pointed by AllocatedExtraSecondaryFrameBuffersLoop_p.
Parameters  : Buffer manager handle, allocation parameters, number of extra frame buffers.
Assumptions : Decimation modes and Compression modes are MUTUALLY EXCLUSIVE.
Limitations : The BufferSize field from AllocParams_p is not used.
Returns     : ST_ERROR_BAD_PARAMETER if too many buffers required
              ST_ERROR_NO_MEMORY if STAVMEM allocation failed
              ST_NO_ERROR if everything OK
*******************************************************************************/
static ST_ErrorCode_t AllocateExtraSecondaryFrameBufferPool(const VIDBUFF_Handle_t BuffersHandle,
                                            VIDBUFF_AllocateBufferParams_t * const AllocParams_p,
                                            const U32   NbrOfExtraSecondaryFrameBuffersRequested)
{
    VIDBUFF_FrameBuffer_t *         AllocatedFrameBuffers_p;
    VIDBUFF_FrameBuffer_t *         LastFrameBuffer_p;
    VIDBUFF_FrameBuffer_t *         FirstFrameBuffer_p;
    VIDBUFF_FrameBuffer_t *         OldNextAllocated_p;
    VIDBUFF_Data_t *                VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
#ifdef ST_OSLINUX
    FrameBuffers_t *                FrameBuffers_p = NULL;
    U32                             NbFrameBuffers = 0;
#endif

    if ( (AllocParams_p == NULL) ||
          (NbrOfExtraSecondaryFrameBuffersRequested == 0) ||
          (NbrOfExtraSecondaryFrameBuffersRequested > MAX_EXTRA_FRAME_BUFFER_NBR) )
    {
        /* Errors */
        return(ST_ERROR_BAD_PARAMETER);
    }

#ifdef ST_OSLINUX
    NbFrameBuffers = 0;
    FrameBuffers_p = (FrameBuffers_t *) STOS_MemoryAllocate(NULL, MAX_EXTRA_FRAME_BUFFER_NBR * sizeof(FrameBuffers_t) );
#endif

    /* Lock access to allocated frame buffers loop */
    semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    /* NB: We need to re-count the remaining number of extra Frame Buffers because some of them may be tagged
                "ToBeKilledAsSoonAsPossible" (so we can not simply use "VIDBUFF_Data_p->NumberOfAllocatedExtraSecondaryFrameBuffers") */
    VIDBUFF_Data_p->NumberOfAllocatedExtraSecondaryFrameBuffers = GetNbrOfFrameBufferAllocatedInAPool(BuffersHandle,
                                                                                                       VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p);

      if (NbrOfExtraSecondaryFrameBuffersRequested < VIDBUFF_Data_p->NumberOfAllocatedExtraSecondaryFrameBuffers)
    {
        /* We have more Extra Frame Buffers than necessary: We keep them anyway */
        ErrorCode = ST_NO_ERROR;
        goto end;
    }

    if (NbrOfExtraSecondaryFrameBuffersRequested == VIDBUFF_Data_p->NumberOfAllocatedExtraSecondaryFrameBuffers)
    {
        /* There are enough Extra Frame Buffer allocated: Nothing to do */
        ErrorCode = ST_NO_ERROR;
        goto end;
    }

    /***********************************/
    /* We should allocate more Extra Buffers */

    /* NB: It is not necessary to handle HDPIPParams because these extra frame buffers will contain only SD */

    if (    (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG4P2) ||
            (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG4P2) ||
            (VIDBUFF_Data_p->DeviceType == STVID_DEVICE_TYPE_7200_MPEG4P2))
    {
        /* For MPEG4-P2, we need 64 bits alligned buffers */
        AllocParams_p->PictureWidth = ((AllocParams_p->PictureWidth + 32 + 63) / 64) * 64;
        AllocParams_p->PictureHeight = AllocParams_p->PictureHeight + 32;
    }

    TrMemAlloc(("\r\nAllocate %d Extra Frame buffers [%d;%d]",
            NbrOfExtraFrameBuffersRequested - VIDBUFF_Data_p->NumberOfAllocatedExtraSecondaryFrameBuffers,
            AllocParams_p->PictureWidth,
            AllocParams_p->PictureHeight));

    LastFrameBuffer_p = NULL;
    FirstFrameBuffer_p = NULL;

    /******************************/
    /* Start the Frame Buffer allocation */
    while ( (VIDBUFF_Data_p->NumberOfAllocatedExtraSecondaryFrameBuffers < NbrOfExtraSecondaryFrameBuffersRequested) &&
                (ErrorCode == ST_NO_ERROR) )
    {
        ErrorCode = AllocateExtraSecondaryFrameBuffer(BuffersHandle,
                                                                       AllocParams_p,
                                                                       &AllocatedFrameBuffers_p);

        if (ErrorCode == ST_NO_ERROR)
        {
            if (LastFrameBuffer_p == NULL)
            {
                /* This is the First Frame Buffer allocated */
                LastFrameBuffer_p = AllocatedFrameBuffers_p;
                FirstFrameBuffer_p = AllocatedFrameBuffers_p;
            }
            else
            {
                /* Append the new Frame Buffer to the tail of the queue */
                LastFrameBuffer_p->NextAllocated_p = AllocatedFrameBuffers_p;
                LastFrameBuffer_p = AllocatedFrameBuffers_p;
            }
            VIDBUFF_Data_p->NumberOfAllocatedExtraSecondaryFrameBuffers++;
#ifdef ST_OSLINUX
            FrameBuffers_p[NbFrameBuffers].KernelAddress_p = AllocatedFrameBuffers_p->Allocation.Address_p;
            FrameBuffers_p[NbFrameBuffers].Size = AllocatedFrameBuffers_p->Allocation.TotalSize;
            NbFrameBuffers ++;
#endif

        }
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Not enough memory to allocate the required nbr of extra Frame Buffer (%d instead of %d)",
                        VIDBUFF_Data_p->NumberOfAllocatedExtraSecondaryFrameBuffers,
                        NbrOfExtraSecondaryFrameBuffersRequested));
            /* NB: We keep the one allocated anyway*/
        }
    }

    if (LastFrameBuffer_p != NULL)
    {
        /* Some Frame Buffers have been allocated in a row. We should now put them in
             AllocatedExtraSecondaryFrameBuffersLoop_p and make it a loop */
        if (VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p == NULL)
        {
            /* The list was empty so we create it and make it a loop */
            VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p = FirstFrameBuffer_p;
            LastFrameBuffer_p->NextAllocated_p = FirstFrameBuffer_p;
        }
        else
        {
            /* We cut the existing loop to insert the new frame buffers */
            OldNextAllocated_p = VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p->NextAllocated_p;
            VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p->NextAllocated_p = FirstFrameBuffer_p;
            LastFrameBuffer_p->NextAllocated_p = OldNextAllocated_p;
        }
    }

#ifdef ST_OSLINUX
    /* Notify newly extra FB allocated */
    NotifyNewFrameBuffers(BuffersHandle, FrameBuffers_p, NbFrameBuffers);
#endif


end:

#ifdef ST_OSLINUX
    if (FrameBuffers_p != NULL)
    {
        /* Free memory before leaving... */
        STOS_MemoryDeallocate(NULL, FrameBuffers_p);
    }
#endif

    /* Un-lock access to allocated frame buffers loop */
    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    return(ErrorCode);
}/* End of AllocateExtraSecondaryFrameBufferPool() function */

/*******************************************************************************
Name        : DeAllocateExtraSecondaryFrameBufferPool
Description : This function deallocates the extra secondary Frame Buffers.
Parameters  :
Assumptions : At this stage the extra Frame Buffers are all considered as unused.
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t DeAllocateExtraSecondaryFrameBufferPool(const VIDBUFF_Handle_t BuffersHandle)
{
    VIDBUFF_FrameBuffer_t *  CurrentFrame_p;
    VIDBUFF_FrameBuffer_t *  FrameToRemove_p;
    VIDBUFF_Data_t *         VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
#ifdef ST_OSLINUX
    FrameBuffers_t *         FrameBuffers_p = NULL;
    U32                      NbFrameBuffers = 0;
#endif

     /* Nothing to de-allocate ! */
    if (VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p == NULL)
    {
        return(ST_NO_ERROR);
    }

    TrMain(("\r\nDeAllocateExtraSecondaryFrameBufferPool"));

#ifdef ST_OSLINUX
    /* Allocates max possible FB entries (extra only) */
    FrameBuffers_p = (FrameBuffers_t *) STOS_MemoryAllocate(NULL, MAX_EXTRA_FRAME_BUFFER_NBR * sizeof(FrameBuffers_t) );

    if (FrameBuffers_p == NULL)
    {
        TrError(("\r\nError! Not enough memory for %d deallocations", MAX_EXTRA_FRAME_BUFFER_NBR));
        return(ST_ERROR_NO_MEMORY);
    }
#endif

    /* Lock access to allocated frame buffers loop */
    semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    /* De-allocate previously allocated buffers and quit with error */
    CurrentFrame_p = VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p;

    while (CurrentFrame_p != NULL)
    {
        /* NB: In case of extra Frame Buffer, the structure "VIDBUFF_FrameBuffer_t" is fully deallocated. */
        FrameToRemove_p = CurrentFrame_p;
        CurrentFrame_p = CurrentFrame_p->NextAllocated_p;

#ifdef ST_OSLINUX
        if (NbFrameBuffers < MAX_EXTRA_FRAME_BUFFER_NBR)
        {
            /* Store this FB has being removed */
            FrameBuffers_p[NbFrameBuffers].KernelAddress_p = FrameToRemove_p->Allocation.Address_p;
            FrameBuffers_p[NbFrameBuffers].Size = FrameToRemove_p->Allocation.TotalSize;
            NbFrameBuffers++;
        }
        else
        {
            TrError(("\r\nError! Too many entries in FrameBuffers_p (%d)", NbFrameBuffers));
        }
#endif

        DeAllocateFrameBuffer(BuffersHandle, FrameToRemove_p);

        if (CurrentFrame_p == VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p)
        {
            /* Finished going through all buffers */
            CurrentFrame_p = NULL;
        }
    } /* while CurrentFrame_p != NULL */

    VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p = NULL;
    VIDBUFF_Data_p->NumberOfAllocatedExtraSecondaryFrameBuffers = 0;

#ifdef ST_OSLINUX
    /* Notify removed extra FB */
    NotifyRemovedFrameBuffers(BuffersHandle, FrameBuffers_p, NbFrameBuffers);
    if (FrameBuffers_p != NULL)
    {
        STOS_MemoryDeallocate(NULL, FrameBuffers_p);
    }
#endif

    /* Lock access to allocated frame buffers loop */
    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    return(ST_NO_ERROR);
}/* End of DeAllocateExtraSecondaryFrameBufferPool() function */

/*******************************************************************************
Name        : AllocateExtraSecondaryFrameBuffer
Description : Allocate ONE extra SD decondary Frame Buffer
Parameters  :
Assumptions : Call to this function must be protected by AllocatedFrameBuffersLoopLockingSemaphore_p
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t AllocateExtraSecondaryFrameBuffer(const VIDBUFF_Handle_t BuffersHandle,
                        const VIDBUFF_AllocateBufferParams_t * const AllocParams_p,
                        VIDBUFF_FrameBuffer_t **                     AllocatedFrameBuffer_p_p)
{
    VIDBUFF_Data_t *                VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
    VIDBUFF_FrameBuffer_t *         FrameBuffers_p;
    HALBUFF_AllocateBufferParams_t  HALAllocParams;
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;

    /* Set AllocatedFrameBuffer_p_p to NULL (in case of error) */
    *AllocatedFrameBuffer_p_p = NULL;

    /* FIRST: Allocate a Frame Buffer Structure */
    SAFE_MEMORY_ALLOCATE(FrameBuffers_p,
                                               VIDBUFF_FrameBuffer_t *,
                                               VIDBUFF_Data_p->CPUPartition_p,
                                               sizeof(VIDBUFF_FrameBuffer_t) );

    if (FrameBuffers_p == NULL)
    {
        ErrorCode = ST_ERROR_NO_MEMORY;
        goto end;
    }

    /* Change VIDBUFF Params to HALBUFF Params... */
    HALAllocParams.AllocationMode = AllocParams_p->AllocationMode;
    HALAllocParams.BufferType      = AllocParams_p->BufferType;
    HALAllocParams.PictureWidth    = AllocParams_p->PictureWidth;
    HALAllocParams.PictureHeight   = AllocParams_p->PictureHeight;


    /* Then allocate the memory associated to this Frame Buffer */
    if (HALBUFF_AllocateFrameBuffer(VIDBUFF_Data_p->HALBuffersHandle,
                                                            &HALAllocParams,
                                                            &(FrameBuffers_p->Allocation),
                                                            HALBUFF_FRAMEBUFFER_DECIMATED) != ST_NO_ERROR)
    {
        /* Error: Allocation could not be performed ! */
        TrError(("\r\nAllocation of a extra secondary frame buffer failed"));

        /* We should Deallocate the Frame Buffer Structure */
        SAFE_MEMORY_DEALLOCATE( FrameBuffers_p, VIDBUFF_Data_p->CPUPartition_p, sizeof(VIDBUFF_FrameBuffer_t));
        ErrorCode = ST_ERROR_NO_MEMORY;
        goto end;
    }

    /* The Frame Buffer allocation was successful */
    TrMemAlloc(("\r\nAllocated extra frame buffer at address 0x%08x", FrameBuffers_p->Allocation.Address_p));
    TrMemAlloc(("\t  Total: 0x%08x, Y: 0x%08x, C: 0x%08x",
                        FrameBuffers_p->Allocation.TotalSize,
                        FrameBuffers_p->Allocation.TotalSize-FrameBuffers_p->Allocation.Size2,
                        FrameBuffers_p->Allocation.Size2));

    FrameBuffers_p->AvailableReconstructionMode = VIDBUFF_SECONDARY_RECONSTRUCTION;
    FrameBuffers_p->AllocationIsDone = TRUE;
    FrameBuffers_p->FrameBufferType = VIDBUFF_EXTRA_SECONDARY_FRAME_BUFFER;
    FrameBuffers_p->FrameOrFirstFieldPicture_p    = NULL;
    FrameBuffers_p->NothingOrSecondFieldPicture_p = NULL;
    FrameBuffers_p->CompressionLevel = VIDBUFF_Data_p->Profile.ApiProfile.CompressionLevel;
    FrameBuffers_p->DecimationFactor = VIDBUFF_Data_p->Profile.ApplicableDecimationFactor;
    FrameBuffers_p->HasDecodeToBeDoneWhileDisplaying    = FALSE;
    FrameBuffers_p->NextAllocated_p = NULL;
    FrameBuffers_p->ToBeKilledAsSoonAsPossible = FALSE;

    *AllocatedFrameBuffer_p_p = FrameBuffers_p;

end:

    return(ErrorCode);
}

/*******************************************************************************
Name        :   VIDBUFF_DeAllocateUnusedExtraSecondaryBuffers
Description : Deallocation of allocated extra Secondary frame buffers not used for the decode/display process.
              If the Frame Buffer is in use, it is tagged "ToBeKilledAsSoonAsPossible"

              Warning: This function can be called while the Decoder is running.
Parameters  : Video buffer manager handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if deallocation done successfully.
*******************************************************************************/
ST_ErrorCode_t VIDBUFF_DeAllocateUnusedExtraSecondaryBuffers(const VIDBUFF_Handle_t BuffersHandle)
{
    VIDBUFF_FrameBuffer_t *     CurrentFrame_p;
    VIDBUFF_FrameBuffer_t *     PreviousFrame_p;
    VIDBUFF_Data_t *            VIDBUFF_Data_p = (VIDBUFF_Data_t *) BuffersHandle;
#ifdef ST_OSLINUX
    FrameBuffers_t *            FrameBuffers_p = NULL;
    U32                         NbFrameBuffers = 0;
#endif

    if (VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p == NULL) /* Nothing to de-allocate ! */
    {
        return(ST_NO_ERROR);
    }

    TrMain(("\r\nVIDBUFF_DeAllocateUnusedExtraBuffers"));

#ifdef ST_OSLINUX
    /* Allocates max possible FB entries (extra only) */
    FrameBuffers_p = (FrameBuffers_t *) STOS_MemoryAllocate(NULL, MAX_EXTRA_FRAME_BUFFER_NBR * sizeof(FrameBuffers_t) );

    if (FrameBuffers_p == NULL)
    {
        TrError(("\r\nError! Not enough memory for %d deallocations", MAX_EXTRA_FRAME_BUFFER_NBR));
        return(ST_ERROR_NO_MEMORY);
    }
#endif

    /* Lock access to allocated frame buffers loop */
    semaphore_wait(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

    /* De-allocate previously allocated buffers and quit with error */
    /*--------------*/
    PreviousFrame_p = VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p;
    CurrentFrame_p = PreviousFrame_p->NextAllocated_p;

    while ((CurrentFrame_p != VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p) && (CurrentFrame_p != NULL))
    {
        if (!(HasAPictureLocked(*CurrentFrame_p)))
        {
            /* Frame Buffer not used: de-allocate it */

            /* Remove it from the Pool */
            PreviousFrame_p->NextAllocated_p = CurrentFrame_p->NextAllocated_p;
            VIDBUFF_Data_p->NumberOfAllocatedExtraSecondaryFrameBuffers --;

#ifdef ST_OSLINUX
           if (NbFrameBuffers < MAX_EXTRA_FRAME_BUFFER_NBR)
           {
                /* Store this FB has being removed */
                FrameBuffers_p[NbFrameBuffers].KernelAddress_p = CurrentFrame_p->Allocation.Address_p;
                FrameBuffers_p[NbFrameBuffers].Size = CurrentFrame_p->Allocation.TotalSize;
                NbFrameBuffers++;
           }
           else
           {
               TrError(("\r\nError! Too many entries in FrameBuffers_p (%d)", NbFrameBuffers));
           }
#endif
            DeAllocateFrameBuffer(BuffersHandle, CurrentFrame_p);
        }
        else
        {
            /* Mark this Frame Buffer to be killed ASAP and move to next one */
            CurrentFrame_p->ToBeKilledAsSoonAsPossible = TRUE;
            TrMain(("\r\nMark Extra FB 0x%x ToBeKilledASAP (Picture 0x%x, %d)",
                        CurrentFrame_p,
                        CurrentFrame_p->FrameOrFirstFieldPicture_p,
                        CurrentFrame_p->FrameOrFirstFieldPicture_p->Buffers.PictureLockCounter));
            TrMemAlloc(("\r\nMark Extra FB @ 0x%x ToBeKilledASAP",  CurrentFrame_p->Allocation.Address_p));
            PreviousFrame_p = CurrentFrame_p;
        }
        CurrentFrame_p = PreviousFrame_p->NextAllocated_p;
    }

    if (CurrentFrame_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unexpected CurrentFrame_p == NULL"));
    }
    else
    {
    /* Here CurrentFrame_p == VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p: check if it is to be removed */
    if (!(HasAPictureLocked(*CurrentFrame_p)))
    {
        /* Frame Buffer not used: de-allocate it */
        if (PreviousFrame_p == CurrentFrame_p)
        {
            /* Picture is alone in loop */
            VIDBUFF_Data_p->NumberOfAllocatedExtraSecondaryFrameBuffers = 0;
            VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p = NULL;
        }
        else
        {
            /* Picture is not alone in loop */
            PreviousFrame_p->NextAllocated_p = CurrentFrame_p->NextAllocated_p;
            VIDBUFF_Data_p->AllocatedExtraSecondaryFrameBuffersLoop_p = PreviousFrame_p;
            VIDBUFF_Data_p->NumberOfAllocatedExtraSecondaryFrameBuffers --;
        }

#ifdef ST_OSLINUX
        if (NbFrameBuffers < MAX_EXTRA_FRAME_BUFFER_NBR)
        {
             /* Store this FB has being removed */
             FrameBuffers_p[NbFrameBuffers].KernelAddress_p = CurrentFrame_p->Allocation.Address_p;
             FrameBuffers_p[NbFrameBuffers].Size = CurrentFrame_p->Allocation.TotalSize;
             NbFrameBuffers++;
        }
        else
        {
            TrError(("\r\nError! Too many entries in FrameBuffers_p (%d)", NbFrameBuffers));
        }
#endif

        DeAllocateFrameBuffer(BuffersHandle, CurrentFrame_p);
    }
    else
    {
        /* Mark this Frame Buffer to be killed ASAP */
        CurrentFrame_p->ToBeKilledAsSoonAsPossible = TRUE;
        TrMain(("\r\nMark Extra FB 0x%x ToBeKilledASAP (Picture 0x%x, %d)",
                    CurrentFrame_p,
                    CurrentFrame_p->FrameOrFirstFieldPicture_p,
                    CurrentFrame_p->FrameOrFirstFieldPicture_p->Buffers.PictureLockCounter));
        TrMemAlloc(("\r\nMark Extra FB @ 0x%x ToBeKilledASAP",  CurrentFrame_p->Allocation.Address_p));
        }
    }

#ifdef ST_OSLINUX
    /* Notify removed extra FB */
    NotifyRemovedFrameBuffers(BuffersHandle, FrameBuffers_p, NbFrameBuffers);
    if (FrameBuffers_p != NULL)
    {
        STOS_MemoryDeallocate(NULL, FrameBuffers_p);
    }
#endif

    /* Un-lock access to allocated frame buffers loop */
    semaphore_signal(VIDBUFF_Data_p->AllocatedFrameBuffersLoopLockingSemaphore_p);

#ifdef SHOW_FRAME_BUFFER_USED
    TrError(("\r\nAfter deallocating Unused Extra Secondary Frame Buffers:"));
    VIDBUFF_PrintPictureBuffersStatus(BuffersHandle);
#endif

    return(ST_NO_ERROR);
} /* End of VIDBUFF_DeAllocateUnusedExtraSecondaryBuffers() function */


#endif /* USE_EXTRA_FRAME_BUFFERS */


/*******************************************************************************
Name        : vidbuff_FreeAdditionnalData
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void vidbuff_FreeAdditionnalData(const VIDBUFF_Handle_t BuffersHandle, VIDBUFF_PictureBuffer_t * const Picture_p, const BOOL Force)
{
	ST_ErrorCode_t        ErrorCode;
    VIDBUFF_FrameBuffer_t * CurrentFrame_p;
    BOOL AllowToFreePPB = TRUE;

    ErrorCode = ST_NO_ERROR;

    if (Picture_p->FrameBuffer_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Unexpected NULL FB associated to picture %d %d-%d/%d 0x%x PPB @0x%x",
                        Picture_p->Decode.CommandId,
                        Picture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                        Picture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                        Picture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID,
                        Picture_p,
                        Picture_p->PPB.Address_p));
    }
    else
    {
        CurrentFrame_p = Picture_p->FrameBuffer_p;
        if (HasAPictureLocked(*CurrentFrame_p))
        {
            if (!Force)
            {
                TrPPB(("\r\nStill one locked picture in FB, Can't free PPB @ 0x%x for 0x%x FB 0x%x", Picture_p->PPB.Address_p, Picture_p, Picture_p->FrameBuffer_p));
                AllowToFreePPB = FALSE;
            }
            else
            {
                TrPPB(("\r\nStill one locked picture in FB, but forced PPB freeing asked @ 0x%x for 0x%x FB 0x%x", Picture_p->PPB.Address_p, Picture_p, Picture_p->FrameBuffer_p));
            }
#if defined (TRACE_UART)
            if (CurrentFrame_p->ToBeKilledAsSoonAsPossible)
            {
                TraceTakeRelease(("\n\r  FB ToBeKillASAP"));
            }
            if (CurrentFrame_p->FrameOrFirstFieldPicture_p != NULL)
            {
                PrintPictureBufferStatus(BuffersHandle, CurrentFrame_p->FrameOrFirstFieldPicture_p);
            }

            if (CurrentFrame_p->NothingOrSecondFieldPicture_p != NULL)
            {
                PrintPictureBufferStatus(BuffersHandle, CurrentFrame_p->NothingOrSecondFieldPicture_p);
            }

            if ( (CurrentFrame_p->FrameOrFirstFieldPicture_p == NULL) &&
                 (CurrentFrame_p->NothingOrSecondFieldPicture_p == NULL) )
            {
                TraceTakeRelease(("\n\r  No PB associated"));
            }
#endif /* TRACE_UART */
        }
        if (AllowToFreePPB)
        {
            ErrorCode = VIDBUFF_FreeAdditionnalDataBuffer(BuffersHandle, Picture_p);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDBUFF_FreeAdditionnalDataBuffer Error (0x%x) while freeing buffer 0x%x for %d %d-%d/%d!!",
                    ErrorCode,
                    Picture_p->PPB.Address_p,
                    Picture_p->Decode.CommandId,
                    Picture_p->ParsedPictureInformation.PictureGenericData.DecodingOrderFrameID,
                    Picture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.IDExtension,
                    Picture_p->ParsedPictureInformation.PictureGenericData.ExtendedPresentationOrderPictureID.ID));
            }
            Picture_p->PPB.FieldCounter = 0;
            /* Resynchronize first & second field PPB structure content */
            if (Picture_p->FrameBuffer_p->FrameOrFirstFieldPicture_p != NULL)
            {
                Picture_p->FrameBuffer_p->FrameOrFirstFieldPicture_p->PPB = Picture_p->PPB;
            }
            if (Picture_p->FrameBuffer_p->NothingOrSecondFieldPicture_p != NULL)
            {
                Picture_p->FrameBuffer_p->NothingOrSecondFieldPicture_p->PPB = Picture_p->PPB;
            }
        }
    }
} /* End of vidbuff_FreeAdditionnalData */

/*******************************************************************************/

/* End of vid_buff.c */
