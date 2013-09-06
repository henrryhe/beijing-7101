/******************************************************************************\
 *
 * File Name   : subtdev.h
 *  
 * Description : Internal types and functions of subtitling decoder
 *  
 * Copyright 1998, 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author : Marino Congiu - Dec 1998
 *  
\******************************************************************************/

#ifndef __SUBTDEV_H
#define __SUBTDEV_H

#include <stlite.h>
#include <stddefs.h>

#include <stclkrv.h>            /* for STCLKRV_Handle_t         */
#include <stevt.h>              /* for STEVT_Handle_t           */

#include <stsubt.h>
#include <common.h>
#include <subtitle.h>
#include <compbuf.h>
#include <pixbuf.h>
#include <dispserv.h>
#include <subttask.h>
#include <objcache.h>

#include "subtconf.h"
 
/* some internal errors */

#define STSUBT_ERROR 			1
#define STSUBT_ERROR_BAD_PES 		2
#define STSUBT_ERROR_BAD_SEGMENT 	3

/* --------------------------------------------------- */
/* --- Built-in Display Service for common Devices --- */
/* --------------------------------------------------- */
 
extern const STSUBT_DisplayService_t STSUBT_DispServDataBase[];

 
/* ------------------------------------------------ */
/* --- Define structures for Subtitling Decoder --- */
/* ------------------------------------------------ */


/* --- Define the task status --- */

typedef enum {
  STSUBT_TaskStopped,
  STSUBT_TaskRunning,
  STSUBT_TaskSuspended,
  STSUBT_TaskSuicide
} STSUBT_TaskStatus_t;

/* --- Define the status of the driver --- */
 
typedef enum {
  STSUBT_Driver_Stopped,
  STSUBT_Driver_Running_And_Displaying,
  STSUBT_Driver_Running_But_Not_Displaying,
  STSUBT_Driver_Suspended
} STSUBT_DecoderStatus_t;

/* --- Define the Processor acquisition mode --- */

typedef enum {
  STSUBT_Starting,
  STSUBT_Acquisition,
  STSUBT_Change_Case,
  STSUBT_Normal_Case
} STSUBT_AcquisitionMode_t;

/* --- Define the status insert APDU --- */
 
typedef enum {
  STSUBT_expecting_any,
  STSUBT_expecting_subtitle_segment,
  STSUBT_expecting_subtitle_download
} STSUBT_ExpectedAPDU_t;

/* --------------------------------------------- */
/* --- Define structures for Display Control --- */
/* --------------------------------------------- */

/* --- Define Display Buffer item content --- */

typedef struct DisplayItem_s {
  void   *display_descriptor;
  PCS_t   PCS;
  clock_t presentation_time;
  clock_t display_timeout;
  U32     EpochNumber;
  U8      PcsNumber;
} STSUBT_DisplayItem_t;


/* --- Define the status of the Display Controller --- */

typedef enum {
  STSUBT_Display_Ready,
  STSUBT_Display_Displaying
} STSUBT_DisplayStatus_t;


/* --------------------------------------------- */
/* --- Real Handle of the subtitling decoder --- */
/* --------------------------------------------- */

typedef struct STSUBT_InternalHandle_s {

  /* Control structures */

  U8 		    DeviceInstance;     /* n instance into the dictionary    */
  U32  	            MagicNumber;        /* Used by Init and Open         */
  ST_Partition_t   *MemoryPartition;    /* memory partition name         */
  ST_Partition_t   *NoCacheMemoryPartition;    /* memory partition name  */
  BOOL  	    StreamInitialized;  /* true when subt stream is set      */
  BOOL              AutoDetection;      /* decoder must identify standard    */
  U16               composition_page_id;/* page composition id               */
  U16               ancillary_page_id;  /* ancillary page id (if any)        */
  pts_t             LastPTS;            /* last received pts mark            */
  STSUBT_DecoderStatus_t DecoderStatus; /* status of subtitling driver       */
  STSUBT_Standard_t SubtitleStandard;   /* standard of current stream        */
  STSUBT_WorkingMode_t WorkingMode;     /* decoder working mode              */
  STSUBT_AcquisitionMode_t AcquisitionMode;/* decoder acquisition mode       */
  STSUBT_ExpectedAPDU_t ExpectingStatus;/* awaiting apdu status              */ 
  BOOL              SubtitleDisplayEnabled;/* display of subtitles is enabled*/
  BOOL  	    CommonInterfaceEnabled;/* true if CI has been enabled    */
  U32               clocks_per_sec;     /* precalculated num clocks per sec  */
  U32               EpochNumber;        /* progressive epoch number          */

  /* Decoder resources */

  cBuffer_t        *CodedDataBuffer;    /* address of coded data buffer      */
  CompBuffer_t     *CompositionBuffer;  /* address of Composition Buffer     */
  CompBuffer_t     *CompositionBuffer1;  /* address of Composition Buffer     */
  CompBuffer_t     *CompositionBuffer2;  /* address of Composition Buffer     */
  PixelBuffer_t    *PixelBuffer;        /* address of Pixel Buffer           */
  PixelBuffer_t    *PixelBuffer1;        /* address of Pixel Buffer           */
  PixelBuffer_t    *PixelBuffer2;        /* address of Pixel Buffer           */
  fcBuffer_t       *DisplayBuffer;      /* address of Display Buffer         */
  fcBuffer_t       *PCSBuffer;          /* address of PCS Buffer             */
  U8               *CI_WorkingBuffer;   /* address of segment buffer         */

  /* Demux stuff */

#if defined DVD_TRANSPORT_PTI || defined DVD_TRANSPORT_LINK
  slot_t            pti_slot;           /* assigned pti slot for subtitling  */
#endif

#ifndef PTI_EMULATION_MODE
#ifdef DVD_TRANSPORT_STPTI
  STPTI_Handle_t       stpti_handle;    /* STPTI driver handle    */
  STPTI_Signal_t	   stpti_signal;
  STPTI_Buffer_t	   stpti_buffer;
  STPTI_Slot_t	   	   stpti_slot;
#endif
#if defined DVD_TRANSPORT_PTI || defined DVD_TRANSPORT_LINK
  U8               *pti_buffer;         /* pti buffer for pes of subt stream */
  U32               pti_buffer_size;    /* size of pti buffer                */
  U8               *pti_producer;       /* pti producer pointer              */
  U8               *pti_consumer;       /* pti consumer pointer              */
#if defined DVD_TRANSPORT_LINK
    STSUBT_Task_t    *ExtractPesTask;   /* pti buffer pes extraction task */   
    U8               *ExtractPesTempBuffer; /* temp buffer for when fitler buffer wraps */
    semaphore_t      *ExtractPesTaskControl; /* task and access control semaps */
    semaphore_t      *ExtractPesTaskSyncSem;
    semaphore_t      *ExtractPesGotData;
    semaphore_t      *ExtractPesMutex;     
    STSUBT_TaskStatus_t ExtractPesStatus;   /* status of extraction task */
    U32              PartialPesOffset;      /* count of bytes for pes construction */
    U32              PesDataAvailable;      /* count of bytes for pes processing */    
    U32             FilterBufferPesAmount;  /* amount of data filterbuffer for processing */
    U32              PesPacketLength;       /* size of current pes packet */
    /* circular FilterBuffer handling */
    U8               *FilterBufferProducer;      /* new data insertion point */
    U8               *FilterBufferPesConsumer;   /* new pes data to process start point */
    U8               *FilterBufferSegmentConsumer;   /* current segment to process */
    U8               *FilterBufferNextSegmentConsumer;  /* start of next segment to process */
#endif

    
#endif

  U8        	   *pes_buffer;         /* pti pointer for pes  */

#else
  cBuffer_t        *pti_buffer;         /* emulation of pti buffer for test  */
#endif

  semaphore_t      *pti_semaphore;      /* semaphore to signal pes received  */

  /* Filter stuff */

  STSUBT_Task_t    *FilterTask;         /* pointer to the filter task struct */
  semaphore_t      *FilterTaskControl;  /* semaphore to start/stop task      */
  semaphore_t      *FilterTaskSyncSem;  /* sem to signal task is suspending  */
  U8               *FilterBuffer;       /* working buffer for pes parsing    */
  U32               FilterBufferSize;   /* working buffer size               */
  STSUBT_TaskStatus_t FilterStatus;     /* status of filter                  */

    semaphore_t      *DisplayMutex;       /* Display buffer critical section protection */

  /* Processor stuff */

  STSUBT_TaskStatus_t ProcessorStatus;  /* status of processor               */
  STSUBT_Task_t      *ProcessorTask;    /* pointer to Processor task struct  */
  semaphore_t        *ProcessorTaskControl; /* semaphore to start/stop task  */
  semaphore_t        *ProcessorTaskSyncSem; /* signal task is suspending     */
  U32                 STSUBT_Display_Depth; /* display device pixel depth    */
  PCS_t               PCS;              /* working PCS structure             */
  PCS_t              *ArrayPCS;         /* array of 16 PCS (overlay mode)    */
  U8      PcsNumber;			/* Number of current PCS */
  U8      ObjectNumber;			/* Number of current Obj */
  STSUBT_RecoveryLevel_t     RecoveryByte;	/* Level of recovery Obj */

  /* Display Controller stuff */

  STSUBT_DisplayStatus_t DisplayStatus; /* status of Displaying              */

  /* Engine Display stuff */

  STSUBT_Task_t      *EngineTask;       /* pointer to Engine task struct     */
  semaphore_t        *EngineTaskControl;/* semaphore to start/stop task      */
  semaphore_t        *EngineTaskSyncSem;/* sem to signal task is suspending  */
  semaphore_t        *EngineWaitSemaphore;/* sem to timeout task when waiting */    
  STSUBT_TaskStatus_t EngineStatus;     /* status of engine                  */

  /* Timer Display stuff */

  STSUBT_Task_t      *TimerTask;        /* pointer to Timer task struct      */
  semaphore_t        *TimerTaskControl; /* semaphore to start/stop task      */
  semaphore_t        *TimerTaskSyncSem; /* sem to signal task is suspending  */
  semaphore_t        *TimerWaitSemaphore;/* sem to timeout task when waiting */
  STSUBT_TaskStatus_t TimerStatus;      /* status of timer                   */

  /* Display Service */

  STSUBT_DisplayService_t *DisplayService; /* display device control struct    */

  /* Common Interface resources */

  cBuffer_t          *CI_ReplyBuffer;   /* address of CI Reply Buffer        */
  cBuffer_t          *CI_MessageBuffer; /* address of CI Message Buffer      */

  /* Object Cache resources */

  BOOL               ObjectCacheEnabled;/* TRUE if Object cache enabled      */
  ObjectCache_t     *ObjectCache;       /* address of Object Cache           */
  Object_t       *ReferredObjectInCache;/* Referred objects in a display set */

  /* external dependencies */

  STCLKRV_Handle_t     stclkrv_handle;  /* STCLKRV driver handle             */
  STEVT_Handle_t       stevt_handle;    /* STEVT driver handle               */
  STEVT_EventID_t      stevt_event_table[STSUBT_NUM_EVENTS];
                                        /* Table of registered events        */
#ifdef RDE_DISPLAY_SERVICE
   U32  RDEBufferSize;                  /* Size of DisplayServiceParams RDE Buffer size */
   U8  RDEStatusFlag;                  /* Flag of  RDE Status */
#endif
} STSUBT_InternalHandle_t;


/* ------------------------------------------------------------ */
/* --- Disctionary where instances of STSUBT are registered --- */
/* ------------------------------------------------------------ */
 
typedef struct STSUBT_DecoderInstance_s {
  ST_DeviceName_t  DeviceName;     	/* name of the device                */
  U32              DecoderHandle;  	/* address of SubT handle            */
  BOOL             HandleIsOpen;        /* TRUE if handle is open            */
} STSUBT_DecoderInstance_t;
 
#endif  /* #ifndef __SUBTDEV_H */

