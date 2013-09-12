/*******************************************************************************

File name   : vid_trqueue.h

Description : Video Trancoder queue header file

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
19 June 2007       Created                                          LM
*******************************************************************************/
/* Define to prevent recursive inclusion */

#ifndef __TRANSCODE_QUEUE_H
#define __TRANSCODE_QUEUE_H

/* Private preliminary definitions (internal use only) ---------------------- */

/* #define TRACE_TRQUEUE */ /* define or uncomment in ./makefile to set traces */
/* #define TRACE_TRQUEUE_VERBOSE */
#define STVID_DEBUG_TRQUEUE

#define STTBX_REPORT

#ifdef ST_OS21
    #define FORMAT_SPEC_OSCLOCK "ll"
    #define OSCLOCK_T_MILLE     1000LL
    #define OSCLOCK_T_MILLION   1000000LL
#else
    #define FORMAT_SPEC_OSCLOCK ""
    #define OSCLOCK_T_MILLE     1000
    #define OSCLOCK_T_MILLION   1000000
#endif

/* Includes ----------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
#include <limits.h>
#include <stdio.h>
#include <string.h>
#endif

#include "stddefs.h"
#include "stvid.h"

#include "vid_com.h"
#include "vid_ctcm.h"

#include "buffers.h"
#include "transcoder.h"
#include "transcodec.h"
#include "vid_tran.h"

#ifdef ST_OSLINUX
#include "compat.h"
#else
#include "sttbx.h"
#endif /* ST_OSLINUX */

#include "stevt.h"

#include "stdevice.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Default bit buffer sizes */

/* Private Variables (static)------------------------------------------------ */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */


/* Exported Constants ------------------------------------------------------- */

/* Defines the size of the transcode queue */
#define TRANSCODE_QUEUE_MAX_PICTURE       8

/* ------------------------------------------------------------ */
/* Definition of Levels for Transcode Queue & Late Picture List */
/* Transcode Queue:                                             */
/* TrQEL: Transcode Queue Empty Level                           */
/* TrQDL: Transcode Queue Decision Level                        */
/* TrQFL: Transocde Queue Full Level                            */
/* Late Picture List:                                           */
/* LPEL: Late Picture Empty Level                               */
/* LPAL: late Picture Adjust Level                              */
/* LPSL: Late Picture Skip Picture                              */
/* LPFL: Late Picture Full Level                                */
/* ------------------------------------------------------------ */
#define TRANSCODE_QUEUE_EMPTY_LEVEL       0   /* Not used */
#define TRANSCODE_QUEUE_DECISION_LEVEL    3   /* More pictures in trancode Queue -> Display is monitored */
#define TRANSCODE_QUEUE_FULL_LEVEL        5   /* Not used */
#define LATE_PICTURE_EMPTY_LEVEL          0   /* Not used */
#define LATE_PICTURE_ADJUST_LEVEL         3   /* More pictures late -> Adjust transcode parameters to speed up */
#define LATE_PICTURE_SKIP_LEVEL           4   /* More pictures late -> Skip a picture (no transcode */
#define LATE_PICTURE_FULL_LEVEL           5   /* Not used */

/* Exported Types ----------------------------------------------------------- */

/* Exported Function prototypes ---------------------------------------------- */
void vidtrqueue_InitTranscoderQueue(VIDTRAN_Data_t * const VIDTRAN_Data_p);
void vidtrqueue_InsertPictureInTranscoderQueue(VIDTRAN_Data_t * const VIDTRAN_Data_p, VIDTRAN_Picture_t  * const TrPictures_p);
void vidtrqueue_RemovePicturesFromTranscoderQueue(VIDTRAN_Data_t * const VIDTRAN_Data_p, VIDTRAN_Picture_t  * const TrPictures_p);
void vidtrqueue_ReleasePicturesFromTranscoderQueue(VIDTRAN_Data_t * const VIDTRAN_Data_p);
void vidtrqueue_InformationOnPicturesInTranscoderQueue(const VIDTRAN_Data_t * VIDTRAN_Data_p, VIDTRQUEUE_QueueInfo_t * const Queue_Info_p);
void vidtrqueue_TakePicturesBeforeInsertInTranscoderQueue(VIDTRAN_Data_t * const VIDTRAN_Data_p, VIDTRAN_Picture_t  * const TrPictures_p);
VIDTRAN_Picture_t * vidtrqueue_GetTrPictureElement(VIDTRAN_Data_t * VIDTRAN_Data_p);
#if 0
VIDTRAN_Picture_t * vidtrqueue_FindLastTrPictureElement(const VIDTRAN_Data_t * VIDTRAN_Data_p);
#endif

#ifdef STVID_DEBUG_TRQUEUE
void vidtrqueue_HistoryOfPicturesInTranscoderQueue(const VIDTRAN_Data_t * VIDTRAN_Data_p);
#endif /* STVID_DEBUG_TRQUEUE */

/* Global Variables --------------------------------------------------------- */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __TRANSCODE_QUEUE_H */
/* End of vid_trqueue.h */

