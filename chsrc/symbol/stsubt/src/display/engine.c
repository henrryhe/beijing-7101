/******************************************************************************\
 *
 * File Name   : engine.c
 *  
 * Description : 
 *     Engine task is in charge to prepare displays copying regions' pixels 
 *     from Pixel Buffer into reserved locations in the real presentation 
 *     hardware (still picture, OSD...) in the appropriate format. 
 *     After the display has been prepared, the request of displaying is 
 *     inserted into the Display Buffer.
 *  
 *     See SDD document for more details.
 *  
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author : Marino Congiu - Feb 1999
 *
\******************************************************************************/
#include <stdlib.h>
#include <time.h>

#include <stddefs.h>
#include <stlite.h>
#include <stevt.h>

#include <stsubt.h>
#include <subtdev.h>
#include <common.h>
#include <dispserv.h>
#include <subttask.h>

#include <decoder.h>
#include "dispctrl.h"
#include "engine.h"

#define MAX_DECODING_DELAYS	3
#define MAX_RECOVERING_ATTEMPTS	3


/******************************************************************************\
 * Function: prepare_display
 * Parameters: prepare a display 
 *           That is achieved through the STSUBT_PrepareDisplay callback 
 *     display_descriptor:
 *           Device dependent structure which describe a display
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/
 
static __inline 
ST_ErrorCode_t prepare_display(void *ServiceHandle,
                               STSUBT_PrepareCallback_t STSUBT_PrepareDisplay,
                               PCS_t *pcs, void **display_descriptor)
{
  ST_ErrorCode_t res;
 
  STTBX_Report((STTBX_REPORT_LEVEL_USER1,"## Engine: Preparing Display ##\n"));
  
  if  (STSUBT_PrepareDisplay != NULL)
  {
      res = (*STSUBT_PrepareDisplay) (pcs, ServiceHandle, display_descriptor);
      return (res);
  }
  else
  {
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"## Engine: Prepare Display callback is NULL ##\n"));
      return (ST_NO_ERROR);
  }
}


/******************************************************************************\
 * Macro   : STOP_ENGINE_IF_REQUIRED
 * Purpose : No OS20 routines allow to externally stop or delete a running
 *           task: this macro is to force the Engine task (by itself) to stop
 *           and then either restart or suicide.
 * Note    : (status = Stopped) is set by Engine_Stop to force task stopping
 *           running. The task tests Status value and when it is Stopped
 *           it first "signal" the calling task that it is going to suspend 
 *           itself (EngineTaskSyncSem) and then suspends itself on semaphore 
 *           EngineTaskControl.  
 * Note    : (status = Suicide) is set during Engine_Delete. 
 *           Engine must be stopped before suiciding; in that case the 
 *           engine task was suspended in the EngineTaskControl queue. 
 *           Task is temporary restarted during Engine_Delete in order to 
 *           allow the engine to suicide.
 *           Caller task is signalled when engine is suspending.
\******************************************************************************/

#define STOP_ENGINE_IF_REQUIRED(handle,command)			\
  {								\
      if (handle->EngineStatus == STSUBT_TaskStopped)		\
      {								\
          STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Engine: Stopping ##\n"));		\
          semaphore_signal(handle->EngineTaskSyncSem); 		\
          semaphore_wait(handle->EngineTaskControl);  		\
          if (handle->EngineStatus == STSUBT_TaskSuicide)	\
          {							\
              STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Engine: Suiciding ##\n"));		\
              semaphore_signal(handle->EngineTaskSyncSem); 	\
              return ST_NO_ERROR; 				\
          }							\
          display_descriptor = NULL;				\
          ServiceHandle = (handle->DisplayService)->STSUBT_ServiceHandle; \
          STSUBT_PrepareDisplay = 			  	\
                          (handle->DisplayService)->STSUBT_PrepareDisplay;\
          delay_counter = 0;					\
          recovering_attempts = 0;				\
          STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Engine: Starting ##\n"));		\
          command;						\
      }								\
  }

/******************************************************************************\
 * Function: Engine_Body
 * Purpose : Function which is executed by Engine task after created
 * Parameters:
 *     DecoderHandle: Returned pointer to Engine handle
 * Return  : ST_NO_ERROR on success, 
\******************************************************************************/

ST_ErrorCode_t Engine_Body (STSUBT_InternalHandle_t * DecoderHandle)
{
    
  ST_ErrorCode_t res;
  STSUBT_DisplayItem_t DisplayItem;
  STSUBT_PrepareCallback_t STSUBT_PrepareDisplay;
  void   *ServiceHandle;
  PCS_t   pcs; 
  void   *display_descriptor = NULL;
  clock_t presentation_time;
  clock_t display_timeout;
  U32     delay_counter = 0;
  U32     recovering_attempts = 0;
  VisibleRegion_t               *PCS_Regions;
  RCS_t                         *RCS_Regions;
  BOOL                          FlagFound;
  
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Engine: I exist ! ##\n"));

  /* --- Engine will stop at first call of STOP_ENGINE_IF_REQUIRED --- */

  while (TRUE) 
  {
     STOP_ENGINE_IF_REQUIRED(DecoderHandle, continue);

     /* --- waits for a PCS to be deposed into PCS Buffer --- */

     STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Engine: waiting for a PCS ##\n"));
     res = fcBuffer_Remove (DecoderHandle->PCSBuffer, &pcs);

     STOP_ENGINE_IF_REQUIRED(DecoderHandle, continue);

     if (res != ST_NO_ERROR)
     {
         STTBX_Report((STTBX_REPORT_LEVEL_USER1,"## Engine: PCS Buffer Invalidated ##\n"));
         continue;
     }

     STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Engine: got a PCS ##\n"));
     

     /* --- get display presentation time and display timeout --- */
     /* --- They were already converted to clock ticks by pcs processor --- */
     presentation_time = pcs.presentation_time;
     display_timeout   = pcs.page_time_out;

     if (time_after(presentation_time, time_now()))
     {
         /* Wait until presentation time if necessary  */
         semaphore_wait_timeout(DecoderHandle->EngineWaitSemaphore, &presentation_time);         
         STOP_ENGINE_IF_REQUIRED(DecoderHandle, continue);
     }
     else
     {         
         /* if decoder is late, skip current display, increment delay_counter,
          * and in case, signal an event
          */
         
         STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Engine: display is late\n"));
         
         if (++delay_counter == MAX_DECODING_DELAYS)
         {
             /* restart the decoder, increment number of recovery attempts, 
              * and in case and signal a events
              */
             
             STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"## Engine: decoder is running slow\n"));
             /* signal decoder is running late */
             if (STEVT_Notify(
                 DecoderHandle->stevt_handle,
                 DecoderHandle->stevt_event_table[STSUBT_EVENT_DECODER_SLOW & 0x0000FFFF],
                 DecoderHandle))
                 {
                     STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"## Engine: Error notifying event**\n"));
                 }
             
             delay_counter = 0;
             
             /* force decoder to restart */
             
             DecoderHandle->AcquisitionMode = STSUBT_Starting;
             
             if (++recovering_attempts == MAX_RECOVERING_ATTEMPTS)
             {
                 STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"## Engine: decoder is definitely too slow\n"));
                 if (STEVT_Notify(
                     DecoderHandle->stevt_handle,
                     DecoderHandle->stevt_event_table
                     [STSUBT_EVENT_DEFINITELY_SLOW & 0x0000FFFF], 
                     DecoderHandle))
                 {
                     STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"## Engine: Error notifying event**\n"));
                 }
                 recovering_attempts = 0;
             }
         }
 
         /* ignore current display */
         continue;
         
     } /* if time_after */

         
     /* Got all objects ? */
     while(DecoderHandle->ObjectNumber > 0)
     {
         task_delay(DecoderHandle->clocks_per_sec/100);
         STTBX_Report((STTBX_REPORT_LEVEL_USER1,
                       "EpochN. %d, PcsNumber %d, ObjectNum %d",i
                       DisplayItem.EpochNumber,DecoderHandle->PcsNumber,
                       DecoderHandle->ObjectNumber));
         
     }

     /* Setup the epoch region list to epoch at creation time */
     pcs.EpochRegionList = (pcs.CompositionBuffer_p)->EpochRegionList_p;


      
#ifndef STSUBT_ERROR_CHECK_REMOVAL
 
     /* Indroduced by R. Nunzio for ddts: GNBvd07316 27/04/2001 START */          
     if(pcs.acquisition_mode == STSUBT_Acquisition ||
        pcs.acquisition_mode == STSUBT_Change_Case)
     {
         
         PCS_Regions = pcs.VisibleRegion_p;
         
         while (PCS_Regions != NULL)
         {
             RCS_Regions  = pcs.EpochRegionList;
             FlagFound = FALSE;
             
             while (RCS_Regions != NULL)
             {
                 if(PCS_Regions->region_id == RCS_Regions->region_id)
                 {
                     FlagFound = TRUE;
                     break;
                 }
                 
                 RCS_Regions = RCS_Regions->next_region_p;
             }
             
             if(FlagFound == FALSE)
             {
                 STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                               "## Timer: Error Region not present in RCS ##\n"));
                 
                 if (STEVT_Notify( DecoderHandle->stevt_handle,
                                   DecoderHandle->stevt_event_table[STSUBT_EVENT_UNKNOWN_REGION & 0x0000FFFF],
                                   DecoderHandle))
                 {
                     STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                                   "## Timer: Error notifying event**\n"));
                 }
             }
             PCS_Regions = PCS_Regions->next_VisibleRegion_p;
         }
     }
          
     /* Indroduced by R. Nunzio for ddts: GNBvd07316 27/04/2001 END */
#endif     

     
     /* Processor cannot invoke Processor_Pause() by itself.
      * Since Display Engine is idle when operating in
      * overlay mode, it can be used to make that call.
      * If a PCS is got, it means that Engine has to pause the Processor.
      * Got PCS is not processed at all.
      */

     if (DecoderHandle->WorkingMode == STSUBT_OverlayMode)
     {
         if (DecoderHandle->DecoderStatus==STSUBT_Driver_Running_And_Displaying)
         {
             STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Engine: Suspending Processor task ##\n"));
             if (Processor_Pause(DecoderHandle) != ST_NO_ERROR)
             {
                 STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"## Engine: Processor not Suspended!\n"));
                 continue;
             }
             /* set decoder status to suspended */
             DecoderHandle->DecoderStatus = STSUBT_Driver_Suspended;
         }
         continue;
     }
     
     
    /* --- since this display is in time, delay counters are reset --- */
     delay_counter = 0;
     recovering_attempts = 0;
     
     STOP_ENGINE_IF_REQUIRED(DecoderHandle, continue);

     /* Fill PCS with visible region data */
     res = CompBuffer_BoundVisibleRegions(&pcs);
     if (res != ST_NO_ERROR)
     {
         STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"## Engine: Cannot bind visible regions\n"));
         continue;
     }

     
     /* Protect operation and prepare display */
     semaphore_wait(DecoderHandle->DisplayMutex); 
     res = prepare_display (ServiceHandle, STSUBT_PrepareDisplay, &pcs, &display_descriptor);
     semaphore_signal(DecoderHandle->DisplayMutex);     
     if (res != ST_NO_ERROR)
     {
         STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"## Engine: Error preparing display ##\n"));
         if (STEVT_Notify(
               DecoderHandle->stevt_handle,
               DecoderHandle->stevt_event_table[STSUBT_EVENT_DISPLAY_ERROR & 0x0000FFFF],
               DecoderHandle))
         {
             STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"## Engine: Error notifying event**\n"));
         }
         continue;
     }

     /* --- Fill in DisplayItem structure --- */
     DisplayItem.display_descriptor = display_descriptor;
     DisplayItem.presentation_time = presentation_time;
     DisplayItem.display_timeout = display_timeout;
     DisplayItem.EpochNumber = DecoderHandle->EpochNumber;

     /* --- insert DisplayItem into Display Buffer --- */
     STTBX_Report((STTBX_REPORT_LEVEL_USER1,"## Engine: Inserting DisplayItem ##\n"));
     res = fcBuffer_Insert (DecoderHandle->DisplayBuffer, (void*)&DisplayItem);
     if (res != ST_NO_ERROR)
     {
         STTBX_Report((STTBX_REPORT_LEVEL_USER1,"## Engine: Display Buffer Invalidated ##\n"));
         continue;
     }
     STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Engine: DisplayItem notified ##\n"));

  } /* End infinite loop */

} /* End Engine_Body */


/******************************************************************************\
 * Function: Engine_Create
 * Purpose : Create a engine task
 * Parameters:
 *     DecoderHandle:
 *           Decoder handle
 * Return  : ST_NO_ERROR on success
\******************************************************************************/

ST_ErrorCode_t Engine_Create (STSUBT_InternalHandle_t *DecoderHandle)
{
  STSUBT_Task_t *EngineTask;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Engine_Create: Creating Display Engine ##\n"));

  /* --- Allocate semaphores --- */
 
  if ((DecoderHandle->EngineTaskControl = memory_allocate(
                                                DecoderHandle->MemoryPartition,
                                                sizeof(semaphore_t))) == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"Engine_Create: impossible to create EngineTaskControl!\n"));
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }
  semaphore_init_fifo(DecoderHandle->EngineTaskControl,0);
 
  if ((DecoderHandle->EngineTaskSyncSem = memory_allocate(
                                                DecoderHandle->MemoryPartition,
                                                sizeof(semaphore_t))) == NULL) 
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL, "Engine_Create: impossible to create EngineTaskSyncSem!\n"));
      /* Release allocated resources */
      semaphore_delete (DecoderHandle->EngineTaskControl);
      memory_deallocate(DecoderHandle->MemoryPartition,
                        DecoderHandle->EngineTaskControl);
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }
  semaphore_init_fifo_timeout(DecoderHandle->EngineTaskSyncSem,0);


  if ((DecoderHandle->EngineWaitSemaphore = memory_allocate(
      DecoderHandle->MemoryPartition,
      sizeof(semaphore_t))) == NULL) 
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL, "Engine_Create: impossible to create EngineTaskSyncSem!\n"));
      /* Release allocated resources */
      semaphore_delete (DecoderHandle->EngineTaskControl);
      memory_deallocate(DecoderHandle->MemoryPartition,
                        DecoderHandle->EngineTaskControl);
      semaphore_delete (DecoderHandle->EngineTaskSyncSem);
      memory_deallocate(DecoderHandle->MemoryPartition,
                        DecoderHandle->EngineTaskSyncSem);      
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }
  semaphore_init_fifo_timeout(DecoderHandle->EngineWaitSemaphore,0);
  

  /* Engine is created stopped */
 
  DecoderHandle->EngineStatus = STSUBT_TaskStopped;
 
  /* --- create the engine task --- */

  EngineTask = SubtTaskCreate((void (*)(void *))Engine_Body, 
                              (void *)DecoderHandle,
                              ENGINE_STACK_SIZE, 
                              ENGINE_TASK_PRIORITY,
                              "Engine Task", 0,
                              DecoderHandle->MemoryPartition);
  if (EngineTask == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"Engine_Create: impossible to create  engine task!\n"));
      /* Release allocated resources */
      semaphore_delete (DecoderHandle->EngineTaskControl);      
      memory_deallocate(DecoderHandle->MemoryPartition,
                        DecoderHandle->EngineTaskControl);
      semaphore_delete (DecoderHandle->EngineTaskSyncSem);            
      memory_deallocate(DecoderHandle->MemoryPartition,
                        DecoderHandle->EngineTaskSyncSem);
      semaphore_delete (DecoderHandle->EngineWaitSemaphore);            
      memory_deallocate(DecoderHandle->MemoryPartition,
                        DecoderHandle->EngineWaitSemaphore);      
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }

  /* wait for the engine task to signal semaphore */
 
  semaphore_wait_timeout(DecoderHandle->EngineTaskSyncSem, TIMEOUT_INFINITY);

  /* Register EngineTask to be used by creator */

  DecoderHandle->EngineTask = EngineTask;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Engine_Create: Display Engine created ##\n"));

  return (ST_NO_ERROR);
}

 
/******************************************************************************\
 * Function: Engine_Start
 * Purpose : Start the engine task
 * Parameters:
 * Return  : ST_NO_ERROR on success, non zero on error
\******************************************************************************/

ST_ErrorCode_t Engine_Start (STSUBT_InternalHandle_t *DecoderHandle)
{
  /* --- Start the engine task --- */
 
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Engine_Start: starting Display Engine ##\n"));

  DecoderHandle->EngineStatus = STSUBT_TaskRunning;

  semaphore_signal(DecoderHandle->EngineTaskControl);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Engine_Start: Display Engine Started ##\n"));

  return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: Engine_Stop
 * Purpose : Stop the engine task
 * Parameters:
 * Return  : ST_NO_ERROR on success, 
 *           STSUBT_ERROR_DRIVER_NOT_RUNNING
 *           STSUBT_ERROR_STOPPING_ENGINE on error
 * Note    : 
\******************************************************************************/

ST_ErrorCode_t Engine_Stop (STSUBT_InternalHandle_t *DecoderHandle)
{
  clock_t wait_time;
  S32 res;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Engine_Stop: Stopping Display Engine ##\n"));

  if (DecoderHandle->EngineStatus != STSUBT_TaskRunning)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"** Engine_Stop: Error: Engine is not running **\n"));
      return (STSUBT_ERROR_DRIVER_NOT_RUNNING);
  }

  DecoderHandle->EngineStatus = STSUBT_TaskStopped;

  /* kick off engine task if waiting for PTS to arrive */
  semaphore_signal(DecoderHandle->EngineWaitSemaphore);

  /* Invalidate data in PCS Buffer and Display Buffer, and force the engine 
   * task out of the semaphore queues in case it is suspended there
   */
  fcBuffer_FlushoutConsumer(DecoderHandle->PCSBuffer);
  fcBuffer_FlushoutProducer(DecoderHandle->DisplayBuffer);

  STTBX_Report((STTBX_REPORT_LEVEL_USER1,"## Engine_Stop: Buffers flushed ##\n"));

  /* --- wait for the engine task to stop --- */

  STTBX_Report((STTBX_REPORT_LEVEL_USER1,"## Engine_Stop: waiting for engine to stop ##\n"));

  wait_time = time_plus(time_now(), 5*DecoderHandle->clocks_per_sec);
  res = semaphore_wait_timeout(DecoderHandle->EngineTaskSyncSem, &wait_time); 
  if (res == - 1)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"## Engine_Stop: Engine Did Not Stop ##\n"));
      return (STSUBT_ERROR_STOPPING_ENGINE);
  }

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Engine_Stop: Display Engine Stopped ##\n"));

  return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: Engine_Delete
 * Purpose : Delete a engine task
 * Parameters:
 * Return  : ST_NO_ERROR on success, else ...
 *           STSUBT_ERROR_DRIVER_DISPLAYING
 *           STSUBT_ERROR_DELETING_ENGINE
\******************************************************************************/
 
ST_ErrorCode_t Engine_Delete (STSUBT_InternalHandle_t *DecoderHandle)
{
  clock_t wait_time;
  S32 res;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Engine_Delete: Deleting Display Engine ##\n"));

  if (DecoderHandle->EngineStatus == STSUBT_TaskRunning)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"Engine_Delete: Task is still running\n"));
      return (STSUBT_ERROR_DRIVER_DISPLAYING);
  }
 
  /* Force engine task to suicide 
   * Engine is temporary made running signalling the semaphore EngineTaskControl
   * in order to allow it to procede suiciding itself.
   */
 
  DecoderHandle->EngineStatus = STSUBT_TaskSuicide;

  /* kick off engine task if waiting for PTS to arrive */
  semaphore_signal(DecoderHandle->EngineWaitSemaphore);
 
  semaphore_signal(DecoderHandle->EngineTaskControl);                

  /* wait for the engine task to suicide */
 
  STTBX_Report((STTBX_REPORT_LEVEL_USER1,"## Engine_Delete: waiting for engine to suicide ##\n"));

  wait_time = time_plus(time_now(), 5 * DecoderHandle->clocks_per_sec);
  res = semaphore_wait_timeout(DecoderHandle->EngineTaskSyncSem, &wait_time); 
  if (res == - 1)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL, "## Engine_Delete: Display Engine not suicided.\n"));
      return (STSUBT_ERROR_DELETING_ENGINE);
  }
 
  /* delete task (always succeeds since task is already terminated) */

  SubtTaskDelete(DecoderHandle->EngineTask);
 
  /* Release all allocated resources */

  semaphore_delete (DecoderHandle->EngineTaskControl);  
  memory_deallocate(DecoderHandle->MemoryPartition,
                    DecoderHandle->EngineTaskControl);
  semaphore_delete (DecoderHandle->EngineTaskSyncSem);              
  memory_deallocate(DecoderHandle->MemoryPartition,
                    DecoderHandle->EngineTaskSyncSem);
  semaphore_delete (DecoderHandle->EngineWaitSemaphore);            
  memory_deallocate(DecoderHandle->MemoryPartition,
                    DecoderHandle->EngineWaitSemaphore);        

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Engine_Delete: Display Engine Deleted ##\n"));

  return (ST_NO_ERROR);
}
