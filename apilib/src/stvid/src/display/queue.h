/*******************************************************************************

File name   : queue.h

Description : Video display source file. Queue manager header file.

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
 02 Sep 2002        Created                                   Michel Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_DISPLAY_QUEUE_H
#define __VIDEO_DISPLAY_QUEUE_H


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Constants ---------------------------------------------------------------- */

#define IMPOSSIBLE_INDEX    0xffffffff

/* Exported Functions ------------------------------------------------------- */

VIDDISP_Picture_t * viddisp_GetPictureElement(const VIDDISP_Handle_t DisplayHandle);

void viddisp_InitQueueElementsPool(VIDDISP_Data_t * VIDDISP_Data_p);

void viddisp_ReleasePictureBuffer(const VIDDISP_Handle_t        DisplayHandle,
                                  VIDBUFF_PictureBuffer_t *     PictureBufferToRelease_p);

void viddisp_RemovePictureFromDisplayQueue(const VIDDISP_Handle_t DisplayHandle,
                                 VIDDISP_Picture_t * const Picture_p);

void viddisp_InsertPictureInDisplayQueue(const VIDDISP_Handle_t DisplayHandle,
                                 VIDDISP_Picture_t * const Picture_p,
                                 VIDDISP_Picture_t ** Queue_p_p);

void viddisp_PurgeQueues(U32 Layer, VIDDISP_Handle_t DisplayHandle);

void viddisp_ResetCountersInQueue(U32 Layer, VIDDISP_Handle_t DisplayHandle);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef  __VIDEO_DISPLAY_QUEUE_H*/

