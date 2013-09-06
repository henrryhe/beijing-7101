/******************************************************************************\
 *  
 * Description : 
 *    Display Timer is responsible to show and hide processed displays at the 
 *    appropriate time (PTS).  
 *  
 *    The Display Timer uses the displayer interface (STSUBT_DisplayService_t) 
 *    to show/hide displays at the given pts/timeout.  
 *  
 *    See SDD document for more details.
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

#include <stsubt.h>
#include <subtdev.h>
#include <common.h>
#include <dispserv.h>
#include <subttask.h>

#include "dispctrl.h"
#include "timer.h"

#define PLUS_INFINITY	time_plus(time_now(), 0xfffffff)


/******************************************************************************\
 * Function: wait_until
 * Purpose : cause the Timer to wait for the specified number of ticks
 *           The semaphore TimerWaitSemaphore is signalled when Timer is stopped
\******************************************************************************/
 
static __inline
ST_ErrorCode_t wait_until (STSUBT_InternalHandle_t *DecoderHandle, 
                           clock_t *ticks)
{
    if (time_after(*ticks, time_now()))
    {
        semaphore_wait_timeout(DecoderHandle->TimerWaitSemaphore, ticks); 
        return (ST_NO_ERROR);
    }
    else return (!ST_NO_ERROR);
}
 

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

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Timer: Preparing Display ##\n"));

  if  (STSUBT_PrepareDisplay != NULL)
  {
      res = (*STSUBT_PrepareDisplay) (pcs, ServiceHandle, display_descriptor);
      return (res);
  }
  else
  {
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"## Timer: Prepare Display callback is NULL ##\n"));
      return (ST_NO_ERROR);
  }
}


/******************************************************************************\
 * Function: show_display
 * Purpose : Make a display visible on the screen;
 *           That is achived through the STSUBT_ShowDisplay callback function
 *     display_descriptor:
 *           Device dependent structure which describe a display
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/
 
static __inline 
ST_ErrorCode_t show_display(STSUBT_InternalHandle_t *DecoderHandle,
                            void *ServiceHandle,
                            STSUBT_ShowCallback_t STSUBT_ShowDisplay,
                            void *display_descriptor)
{
  ST_ErrorCode_t res;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Timer: Showing Display ##\n\n"));
 
  if  (display_descriptor == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"## Timer: No Display to Show ##\n\n"));
      return (1);
  }

  if  (STSUBT_ShowDisplay != NULL)
  {
      res = (*STSUBT_ShowDisplay) (ServiceHandle, display_descriptor);
      if (res != ST_NO_ERROR)
      {
         STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"## Timer: Error showing Display ##\n"));
         if (STEVT_Notify(
             DecoderHandle->stevt_handle,
             DecoderHandle->stevt_event_table[STSUBT_EVENT_DISPLAY_ERROR & 0x0000FFFF],
             DecoderHandle))
         {
             STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"## Timer: Error notifying event**\n"));
         }
      }
      return (res);
  }
  else
  {
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"## Timer: Show Display callback is NULL ##\n\n"));
      return (ST_NO_ERROR);
  }
}


/******************************************************************************\
 * Function: hide_display
 * Purpose : Hide current display;
 *           That is achived through the STSUBT_HideDisplay callback function
 * Parameters:
 *     display_descriptor:
 *           Device dependent structure which describe a display
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/
 
static __inline 
ST_ErrorCode_t hide_display (STSUBT_InternalHandle_t *DecoderHandle,
                             void *ServiceHandle,
                             STSUBT_HideCallback_t STSUBT_HideDisplay,
                             void *display_descriptor)
{
  ST_ErrorCode_t res;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Timer: Hiding Display  ##\n"));
 
  if  (display_descriptor == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"## Timer: No Display is Active ##\n\n"));
      return (ST_NO_ERROR);
  }

  if  (STSUBT_HideDisplay != NULL)
  {
      res = (*STSUBT_HideDisplay) (ServiceHandle, display_descriptor);
      if (res != ST_NO_ERROR)
      {
         STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"## Timer: Error Hiding Display ##\n"));
         if (STEVT_Notify(
             DecoderHandle->stevt_handle,
             DecoderHandle->stevt_event_table[STSUBT_EVENT_DISPLAY_ERROR & 0x0000FFFF],
             DecoderHandle))
         {
             STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"## Timer: Error notifying event**\n"));
         }
      }
      return (res);
  }
  else
  {
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"## Timer: Hide Display callback is NULL ##\n\n"));
      return (ST_NO_ERROR);
  }
}

/******************************************************************************\
 * Macro   : STOP_TIMER_IF_REQUIRED
 * Purpose : Force timer task to stop and then either restart or suicide.
 * Note    : (status = Stopped) is set by Timer_Stop to force task stopping
 *           running. The task tests Status value and when it is Stopped
 *           it first "signal" the calling task that it is going to suspend 
 *           itself (TimerTaskSyncSem) and then suspends itself on semaphore 
 *           TimerTaskControl.  
 * Note    : (status = Suicide) is set during Timer_Delete. 
 *           Timer must be stopped before suiciding; in that case the 
 *           timer task was suspended in the TimerTaskControl queue. 
 *           Task is temporary restarted during Timer_Delete in order to 
 *           allow the timer to suicide.
 *           Caller task is signalled when timer is suspending.
\******************************************************************************/

#define STOP_TIMER_IF_REQUIRED(handle,command)			\
  {								\
      if (handle->TimerStatus == STSUBT_TaskStopped)		\
      {								\
          STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Timer: Stopping ##\n"));		\
          hide_display (handle, ServiceHandle, 			\
                        STSUBT_HideDisplay,     		\
                        active_display_descriptor);		\
          semaphore_signal(handle->TimerTaskSyncSem); 		\
          semaphore_wait(handle->TimerTaskControl);  		\
          if (handle->TimerStatus == STSUBT_TaskSuicide)	\
          {							\
              STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Timer: Suiciding ##\n"));		\
              semaphore_signal(handle->TimerTaskSyncSem); 	\
              return ST_NO_ERROR; 				\
          }							\
          STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Timer: Starting ##\n"));		\
          ServiceHandle = (handle->DisplayService)->STSUBT_ServiceHandle;       \
          STSUBT_PrepareDisplay=(handle->DisplayService)->STSUBT_PrepareDisplay;\
          STSUBT_ShowDisplay = (handle->DisplayService)->STSUBT_ShowDisplay;    \
          STSUBT_HideDisplay = (handle->DisplayService)->STSUBT_HideDisplay;    \
          active_display_descriptor = NULL;			\
          ActiveTimeOut = PLUS_INFINITY;			\
          command;						\
      }								\
  }


/******************************************************************************\
 * Function: Timer_Body
 *
 * Purpose : Function which is executed by Timer task after created
 *
 * Description: 
 *    Within an infinite loop, it first waits for new entries to be deposed
 *    into the Display Buffer. Generally, when a new entry is got, it waits 
 *    for the corresponding presentation time to happen; then the displaying 
 *    of new picture is executed.
 *
 * Parameters:
 *     DecoderHandle: Pointer to Timer handle
 *
 * Return  : ST_NO_ERROR on success, 
\******************************************************************************/

ST_ErrorCode_t Timer_Body (STSUBT_InternalHandle_t * DecoderHandle)
{
  ST_ErrorCode_t 	   res;
  STSUBT_PrepareCallback_t STSUBT_PrepareDisplay = NULL;
  STSUBT_ShowCallback_t    STSUBT_ShowDisplay = NULL;
  STSUBT_HideCallback_t    STSUBT_HideDisplay = NULL;
  S32            	   wait_time;
  clock_t        	   ActiveTimeOut=0;
  clock_t             	   display_timeout;
  clock_t         	   presentation_time;
  void                    *active_display_descriptor = NULL;
  void                    *pending_display_descriptor = NULL;
  void                    *ServiceHandle = NULL;
  STSUBT_DisplayItem_t 	   DisplayItem; 
  VisibleRegion_t          *PCS_Regions;
  RCS_t                    *RCS_Regions;
  BOOL                     FlagFound;


  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Timer: I exist ! ##\n"));

  /* --- Timer will stop at first call of STOP_TIMER_IF_REQUIRED --- */
  while (TRUE) 
  {

      STOP_TIMER_IF_REQUIRED(DecoderHandle, continue);

      /* --- waits for a Display Item to be deposed into Display Buffer --- */

      /* It may happen that active display times out a while before a new 
       * display description arrives, so timer waits for a new display
       * for at most the active display timeout
       */

      wait_time = time_minus(ActiveTimeOut, time_now());
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"## Timer: waiting for a Display Item (%d) ##\n", wait_time));

      if (wait_time > 0)
      {
           res = fcBuffer_Remove_Timeout (DecoderHandle->DisplayBuffer, 
                                          &DisplayItem, &ActiveTimeOut);
      }
      else
      {
          res = FCBUFFER_WAIT_TIMEDOUT;
      }
 
      if (res != ST_NO_ERROR)
      {
          /* display timed out (buffer data invalidated), display is hidden */
          STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Timer: active display timed out ##\n"));

          res = hide_display (DecoderHandle, 
                              ServiceHandle, STSUBT_HideDisplay, 
                              active_display_descriptor); 
          active_display_descriptor = NULL;
          ActiveTimeOut = PLUS_INFINITY;
          continue;
      }


      /* If in overlay or DTG mode (descriptor is NULL in both cases)....*/
      if (DisplayItem.display_descriptor == NULL)
      {
          PCS_t PCS = DisplayItem.PCS;      
      
#ifndef STSUBT_ERROR_CHECK_REMOVAL
          /* Indroduced by R. Nunzio for ddts: GNBvd07316 27/04/2001 START */
          if(DisplayItem.PCS.acquisition_mode == STSUBT_Acquisition ||
             DisplayItem.PCS.acquisition_mode == STSUBT_Change_Case)
          {

              PCS_Regions = DisplayItem.PCS.VisibleRegion_p;
              
              while (PCS_Regions != NULL)
              {
                  RCS_Regions  = DisplayItem.PCS.EpochRegionList;
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
                        
          STOP_TIMER_IF_REQUIRED(DecoderHandle, continue);
          
          STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Timer: got Display Item ##\n"));

          /* --- Check PCS epoch number --- */
          
          if (DecoderHandle->EpochNumber > DisplayItem.EpochNumber)
          {
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"## Timer: Obsolete Display %d",DisplayItem.EpochNumber)); 
              continue;
          }
          

          /* Get display descriptor, timeout and presentation time */
                    
          pending_display_descriptor = DisplayItem.display_descriptor;
          
          if (DecoderHandle->WorkingMode == STSUBT_OverlayMode)
          {
              /* In overlay working mode, display are processed immediately. */
              presentation_time = time_now();
              display_timeout = time_plus (presentation_time,
                                           DisplayItem.display_timeout *
                                           DecoderHandle->clocks_per_sec);
          }
          else
          {
              presentation_time =  DisplayItem.presentation_time; 
              display_timeout = DisplayItem.display_timeout;
          }

          /* After a DisplayItem is got, its timeout value is compared with the 
           * current ActiveTimeOut. If (ActiveTimeOut < presentation_time)
           * that means the old picture have to be removed from screen a while 
           * before the presentation of the new display.
           */
          
          if (time_after(presentation_time, ActiveTimeOut) == 1)
          {
              STTBX_Report((STTBX_REPORT_LEVEL_USER1,"\n## Timer: waiting for display timeout ##\n\n"));
              
              wait_until (DecoderHandle, &ActiveTimeOut);
              
              res = hide_display (DecoderHandle, 
                                  ServiceHandle, STSUBT_HideDisplay, 
                                  active_display_descriptor);
              
              active_display_descriptor = NULL;
              ActiveTimeOut = PLUS_INFINITY;
          }
          
          STOP_TIMER_IF_REQUIRED(DecoderHandle, continue);
          
          /* --- wait until the presentation time is reached --- */
          
          STTBX_Report((STTBX_REPORT_LEVEL_INFO,"\n## Timer: waiting for presentation time ##\n\n"));
          
          res = wait_until (DecoderHandle, &presentation_time);
          
          while(DecoderHandle->ObjectNumber > 0)
          {
              task_delay(DecoderHandle->clocks_per_sec/100);
              STTBX_Report((STTBX_REPORT_LEVEL_USER1,
                            "EpochN. %d, PcsNumber %d, ObjectNum %d",i
                            DisplayItem.EpochNumber,DecoderHandle->PcsNumber,
                            DecoderHandle->ObjectNumber));
          }
          
          if (PCS.acquisition_mode == STSUBT_Acquisition )
          {
              /* Each visible region need to be associated to 
               * corresponding epoch region.
               */              
              res = CompBuffer_BoundVisibleRegions(&PCS);
              if (res != ST_NO_ERROR)
              {
                  STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"## Timer: Error bounding regions **\n"));
                  continue;
              }
          }
          
          /* Protect operation and prepare a page composition display */
          semaphore_wait(DecoderHandle->DisplayMutex);
	  res = prepare_display (ServiceHandle, STSUBT_PrepareDisplay,
                                 &PCS, &pending_display_descriptor); 
          semaphore_signal(DecoderHandle->DisplayMutex);         
          
          if (res != ST_NO_ERROR)
          {
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"## Timer: Error preparing display ##\n"));
              if (STEVT_Notify(
                  DecoderHandle->stevt_handle,
                  DecoderHandle->stevt_event_table[STSUBT_EVENT_DISPLAY_ERROR & 0x0000FFFF],
                  DecoderHandle))
              {
                  STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"## Timer: Error notifying event**\n"));
              }
              
              ActiveTimeOut = PLUS_INFINITY;
              continue;
          }
      }
      else /* we must in DVB normal mode...*/
      {
          /* Check to see if timeout necessary */
          presentation_time =  DisplayItem.presentation_time; 
          display_timeout = DisplayItem.display_timeout;


          /* After a DisplayItem is got, its timeout value is compared with the 
           * current ActiveTimeOut. If (ActiveTimeOut < presentation_time)
           * that means the old picture have to be removed from screen a while 
           * before the presentation of the new display.
           */
          
          if (time_after(presentation_time, ActiveTimeOut) == 1)
          {
              STTBX_Report((STTBX_REPORT_LEVEL_USER1,"\n## Timer: waiting for display timeout ##\n\n"));
              
              wait_until (DecoderHandle, &ActiveTimeOut);
              
              res = hide_display (DecoderHandle, 
                                  ServiceHandle, STSUBT_HideDisplay, 
                                  active_display_descriptor);
              
              active_display_descriptor = NULL;
              ActiveTimeOut = PLUS_INFINITY;
          }
          
          /* All preparation done by Engine task, so just load display descriptor */
          pending_display_descriptor = DisplayItem.display_descriptor;
          
      } /* endif overlay or DTG  mode */

      res = show_display(DecoderHandle,
                         ServiceHandle, STSUBT_ShowDisplay, 
                         pending_display_descriptor);

      
      active_display_descriptor = pending_display_descriptor;

      ActiveTimeOut = display_timeout;

  } /* End infinite loop */

} /* End Timer_Body */


/******************************************************************************\
 * Function: Timer_Create
 * Purpose : Create a timer task
 * Parameters:
 *     DecoderHandle:
 *           decoder handle
 * Return  : ST_NO_ERROR on success
\******************************************************************************/

ST_ErrorCode_t Timer_Create (STSUBT_InternalHandle_t *DecoderHandle)
{
  STSUBT_Task_t *TimerTask;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Timer_Create: Creating Display Timer ##\n"));

  /* --- Allocate semaphores --- */
 
  if ((DecoderHandle->TimerTaskControl = memory_allocate(
                                                DecoderHandle->MemoryPartition,
                                                sizeof(semaphore_t))) == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"Timer_Create: Impossible to create TimerTaskControl!\n"));
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }
  semaphore_init_fifo(DecoderHandle->TimerTaskControl,0);
 
  if ((DecoderHandle->TimerTaskSyncSem = memory_allocate(
                                                DecoderHandle->MemoryPartition,
                                                sizeof(semaphore_t))) == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL, "Timer_Create: Impossible to create TimerTaskSyncSem!\n"));
      /* Release allocated resources */
      semaphore_delete (DecoderHandle->TimerTaskControl);
      memory_deallocate(DecoderHandle->MemoryPartition,
                        DecoderHandle->TimerTaskControl);
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }
  semaphore_init_fifo_timeout(DecoderHandle->TimerTaskSyncSem,0);

  if ((DecoderHandle->TimerWaitSemaphore = memory_allocate(
                                                DecoderHandle->MemoryPartition,
                                                sizeof(semaphore_t))) == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL, "Timer_Create: Impossible to create TimerWaitSemaphore!\n"));
      /* Release allocated resources */ 
      semaphore_delete (DecoderHandle->TimerTaskControl);     
      memory_deallocate(DecoderHandle->MemoryPartition,
                        DecoderHandle->TimerTaskControl);
      semaphore_delete (DecoderHandle->TimerTaskSyncSem);           
      memory_deallocate(DecoderHandle->MemoryPartition,
                        DecoderHandle->TimerTaskSyncSem);
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }
  semaphore_init_fifo_timeout(DecoderHandle->TimerWaitSemaphore,0);

  /* Timer is created stopped */
 
  DecoderHandle->TimerStatus = STSUBT_TaskStopped;
 
  /* --- create the timer task --- */

  TimerTask = SubtTaskCreate((void (*)(void *))Timer_Body, 
                           (void *)DecoderHandle,
                           TIMER_STACK_SIZE, 
                           TIMER_TASK_PRIORITY, 
                           "Timer Task", 0,
                           DecoderHandle->MemoryPartition);
  if (TimerTask == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"Timer_Create: Impossible to create Timer task!\n"));
      /* Release allocated resources */
      semaphore_delete (DecoderHandle->TimerTaskControl);      
      memory_deallocate(DecoderHandle->MemoryPartition,
                        DecoderHandle->TimerTaskControl);
      semaphore_delete (DecoderHandle->TimerTaskSyncSem);                 
      memory_deallocate(DecoderHandle->MemoryPartition,
                        DecoderHandle->TimerTaskSyncSem);
      semaphore_delete (DecoderHandle->TimerWaitSemaphore); 
      memory_deallocate(DecoderHandle->MemoryPartition,
                        DecoderHandle->TimerWaitSemaphore);
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }
 

  /* wait for the timer task to signal semaphore */
 
  semaphore_wait_timeout(DecoderHandle->TimerTaskSyncSem, TIMEOUT_INFINITY);

  /* Register TimerTask to be used by creator */

  DecoderHandle->TimerTask = TimerTask;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Timer_Create: Display Timer created ##\n"));

  return (ST_NO_ERROR);
}

 
/******************************************************************************\
 * Function: Timer_Start
 * Purpose : Start the timer task
 * Parameters:
 * Return  : ST_NO_ERROR on success
\******************************************************************************/

ST_ErrorCode_t Timer_Start (STSUBT_InternalHandle_t *DecoderHandle)
{
  /* --- Start the timer task --- */
 
  DecoderHandle->TimerStatus = STSUBT_TaskRunning;

  semaphore_signal(DecoderHandle->TimerTaskControl);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Timer_Start: Display Timer started ##\n"));

  return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: Timer_Stop
 * Purpose : Stop the timer task
 * Parameters:
 * Return  : ST_NO_ERROR on success, 
 *           STSUBT_ERROR_DRIVER_NOT_RUNNING
 *           STSUBT_ERROR_STOPPING_TIMER on error
\******************************************************************************/

ST_ErrorCode_t Timer_Stop (STSUBT_InternalHandle_t *DecoderHandle)
{
  clock_t wait_time;
  S32 res;

  STTBX_Report((STTBX_REPORT_LEVEL_USER1,"## Timer_Stop: Stopping Display Timer ##\n"));

  if (DecoderHandle->TimerStatus != STSUBT_TaskRunning)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"** Timer_Stop: Error: Display Timer is not running **\n"));
      return (STSUBT_ERROR_DRIVER_NOT_RUNNING);
  }

  DecoderHandle->TimerStatus = STSUBT_TaskStopped;

  /* Invalidate data in Display Buffer, and force the timer 
   * task out of the semaphore queue in case it is suspended there
   */

  fcBuffer_FlushoutConsumer(DecoderHandle->DisplayBuffer);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Timer_Stop: Buffer flushed ##\n"));

  /* --- signal the Timer in case it is waiting --- */

  semaphore_signal(DecoderHandle->TimerWaitSemaphore);

  /* --- wait for the timer task to stop --- */

  STTBX_Report((STTBX_REPORT_LEVEL_USER1,"## Timer_Stop: waiting for timer to stop ##\n"));

  wait_time = time_plus(time_now(), 5*DecoderHandle->clocks_per_sec);
  res = semaphore_wait_timeout(DecoderHandle->TimerTaskSyncSem, &wait_time); 
  if (res == - 1)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"## Timer_Stop: Display Timer Not Stopped ##\n"));
      return (STSUBT_ERROR_STOPPING_TIMER);
  }

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Timer_Stop: Display Timer Stopped ##\n"));


  return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: Timer_Delete
 * Purpose : Delete a timer task
 * Parameters:
 * Return  : ST_NO_ERROR on success, else ...
 *           STSUBT_ERROR_DRIVER_DISPLAYING (timer is running)
 *           STSUBT_ERROR_DELETING_TIMER 
\******************************************************************************/
 
ST_ErrorCode_t Timer_Delete (STSUBT_InternalHandle_t *DecoderHandle)
{
  clock_t wait_time;
  S32 res;

  if (DecoderHandle->TimerStatus == STSUBT_TaskRunning)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"Timer_Delete: Display Timer Task is still running\n"));
      return (STSUBT_ERROR_DRIVER_DISPLAYING);
  }
 
  /* Force timer task to suicide 
   * Timer is temporary made running signalling the semaphore TimerTaskControl
   * in order to allow it to procede suiciding itself.
   */
 
  DecoderHandle->TimerStatus = STSUBT_TaskSuicide;
 
  semaphore_signal(DecoderHandle->TimerTaskControl);                

  /* wait for the timer task to suicide */
 
  STTBX_Report((STTBX_REPORT_LEVEL_USER1,"## Timer_Delete: waiting for timer to suicide ##\n"));

  wait_time = time_plus(time_now(), 5 * DecoderHandle->clocks_per_sec);
  res = semaphore_wait_timeout(DecoderHandle->TimerTaskSyncSem, &wait_time);
  if (res == - 1)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL, "## Timer_Delete: Display Timer not suicided.\n"));
      return (STSUBT_ERROR_DELETING_TIMER);
  }
 
  /* delete task (always succeeds since task is already terminated) */

  SubtTaskDelete(DecoderHandle->TimerTask);
 
  /* Release all allocated resources */

  semaphore_delete (DecoderHandle->TimerTaskControl);  
  memory_deallocate(DecoderHandle->MemoryPartition,
                    DecoderHandle->TimerTaskControl);
  semaphore_delete (DecoderHandle->TimerTaskSyncSem);             
  memory_deallocate(DecoderHandle->MemoryPartition,                    
                    DecoderHandle->TimerTaskSyncSem);
  semaphore_delete (DecoderHandle->TimerWaitSemaphore);   
  memory_deallocate(DecoderHandle->MemoryPartition,
                    DecoderHandle->TimerWaitSemaphore);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## Timer_Delete: Display Timer Deleted ##\n"));

  return (ST_NO_ERROR);
}

