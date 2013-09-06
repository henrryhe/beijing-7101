/******************************************************************************\
 *
 * File Name   : extpes.c
 *  
 * Description : For link only, extracts pes data from pti_buffer into
 *               the circular FilterBuffer, using a temporary 8k buffer
 *               in case of FitlerBuffer wrap.
 *               Upon data reception the Filter Task is signalled to
 *               commence start code search and parsing.
 *               Added to overcome DMA overflow problems when using
 *               Link with 8k PTI buffer.
 *  
 * Copyright 2002 STMicroelectronics. All Rights Reserved.
 *
 * Author : Anthony Kemble 2002
 *  
\******************************************************************************/
#if (defined DVD_TRANSPORT_LINK) && (!defined PTI_EMULATION_MODE)

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <stddefs.h>
#include <stlite.h>

#include <common.h>
#include <stevt.h>
#include <stsubt.h>
#include <subtitle.h>
#include <compbuf.h>	
#include <subttask.h>	
#include "extpes.h"

#define FIVE_SECONDS		(5  * DecoderHandle->clocks_per_sec)

/******************************************************************************\
 * Macro   : STOP_EXTRACTPES_IF_REQUIRED
 * Purpose : No OS20 routines allow to externally stop or delete a running
 *           task: this macro is to force the ExtractPes task (by itself) to stop
 *           and then either restart or suicide.
 * Note    : (status = Stopped) is set by ExtractPes_Stop to force task stopping
 *           running. The task tests Status value and when it is Stopped
 *           it first "signal" the calling task that it is going to suspend
 *           itself (ExtractPesTaskSyncSem) and then suspends itself on semaphore
 *           ExtractPesTaskControl.
 * Note    : (status = Suicide) is set during ExtractPes_Delete.
 *           ExtractPes must be stopped before suiciding; in that case the
 *           ExtractPes task was suspended in the ExtractPesTaskControl queue.
 *           Task is temporary restarted during ExtractPes_Delete in order to
 *           allow the ExtractPes to suicide.
 *           Caller task is signalled when ExtractPes is suspending.
\******************************************************************************/
 
#define STOP_EXTRACTPES_IF_REQUIRED(handle,command)			\
  {								\
      if (handle->ExtractPesStatus == STSUBT_TaskStopped)		\
      {                                                             \
          STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** ExtractPes: Stopping **\n"));  \
          semaphore_signal(handle->ExtractPesTaskSyncSem); 		\
           semaphore_wait(handle->ExtractPesTaskControl);     \
          if (handle->ExtractPesStatus == STSUBT_TaskSuicide)	\
          {							\
              STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** ExtractPes: Suiciding **\n")); \
              semaphore_signal(handle->ExtractPesTaskSyncSem); 	\
              return ST_NO_ERROR; 				\
          }							\
          STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** ExtractPes: Starting **\n"));   \
          command;						\
      }								\
  }



/******************************************************************************\
 * Function: ResetExtPesBufferControl
 * Purpose : Utility functions that clears the pes extraction buffers and
             resets control variables.
 * Parameters: DecoderHandle
 * Return  : none
\******************************************************************************/
void ResetExtPesBufferControl (STSUBT_InternalHandle_t *DecoderHandle)
{
    /* Flush pti buffer */
    pti_get_producer(DecoderHandle->pti_slot, &DecoderHandle->pti_producer);
    pti_set_consumer_ptr(DecoderHandle->pti_slot, DecoderHandle->pti_producer);

    /* Flush FitlerBuffer */
    DecoderHandle->FilterBufferProducer = DecoderHandle->FilterBuffer;
    DecoderHandle->FilterBufferPesConsumer = DecoderHandle->FilterBuffer;
    DecoderHandle->FilterBufferSegmentConsumer = DecoderHandle->FilterBuffer;
    DecoderHandle->FilterBufferNextSegmentConsumer = DecoderHandle->FilterBuffer;

    /* Reset extraction control variables */
    DecoderHandle->PartialPesOffset = 0;
    DecoderHandle->FilterBufferPesAmount = 0;
    DecoderHandle->PesPacketLength = 0;

}


/******************************************************************************\
 * Function: ExtractPes_Body
 * Purpose : PES extraction task body
 *           Extracts PES to from the PTI buffer to the circular FilterBuffer
             making use of the temporary 8k buffer in case Filterbuffer
             needs to wrap.
 * Parameters:
 * Return  : 
\******************************************************************************/
ST_ErrorCode_t ExtractPes_Body(STSUBT_InternalHandle_t * DecoderHandle)
{

    U8 *ExtPesTempBuffPtr;
    U32 delaytime =  (DecoderHandle->clocks_per_sec /1000); 
    U32 PTIBufferBytes;
    U32 AmountFromPTI;
    U32 FilterBufferUpperFree;
    BOOL Wrap;    
    BOOL PTICopyErr = FALSE;
    BOOL PTIConsErr = FALSE;

    while (TRUE)
    {
        STOP_EXTRACTPES_IF_REQUIRED(DecoderHandle, continue);
        
        while (DecoderHandle->ExtractPesStatus == STSUBT_TaskRunning)
        {
            /* necessary to force rechedule of other tasks, Consider changing extpes task priority */
            task_delay(delaytime);

            STOP_EXTRACTPES_IF_REQUIRED(DecoderHandle, break);
                        
            pti_get_written_size(DecoderHandle->pti_slot, &PTIBufferBytes);

            if (PTIBufferBytes > 6)
            {
                
                /* Got enough space in local buffer? */
                FilterBufferUpperFree = (U32)((DecoderHandle->FilterBuffer + DecoderHandle->FilterBufferSize)
                                               - DecoderHandle->FilterBufferProducer);
                
                /* refresh producer and number of bytes obtained */
                pti_get_producer(DecoderHandle->pti_slot, &DecoderHandle->pti_producer);
                pti_get_written_size(DecoderHandle->pti_slot, &PTIBufferBytes);
                                
                /* If buffer needs to wrap */
                if (PTIBufferBytes > FilterBufferUpperFree)
                {
                
                    /* copy pti buffer to local buffer */
                    PTICopyErr = pti_copy_circular_to_linear(DecoderHandle->pti_buffer,
                                                DecoderHandle->pti_buffer_size,
                                                &DecoderHandle->pti_consumer,
                                                DecoderHandle->pti_producer,
                                                DecoderHandle->ExtractPesTempBuffer,
                                                EXTRACT_PES_TEMP_BUFFER_SIZE,
                                                &AmountFromPTI);

                    /* Update pti_buffer consumer pointer */
                    PTIConsErr = pti_set_consumer_ptr(DecoderHandle->pti_slot, DecoderHandle->pti_consumer);

                    /* PTI successful? */
                    if ((!PTICopyErr) && (!PTIConsErr))
                    {
                        /* If amount taken won't fit into ExtractPesBuffer upper free space */
                        if (AmountFromPTI >  FilterBufferUpperFree) 
                        {
                            /* Look to start of local buffer */
                            ExtPesTempBuffPtr = DecoderHandle->ExtractPesTempBuffer;
                            
                            /* copy one chunk to upper */                    
                            memcpy(DecoderHandle->FilterBufferProducer, ExtPesTempBuffPtr, FilterBufferUpperFree);
                            
                            /* Move through ExtPesTmpBuff to next chunk to copy */
                            ExtPesTempBuffPtr += FilterBufferUpperFree;
                            
                            /* Copy one chunk to lower */
                            memcpy(DecoderHandle->FilterBuffer, ExtPesTempBuffPtr, (AmountFromPTI - FilterBufferUpperFree));
                            
                            Wrap = TRUE;                        
                        }       
                        else   /* the amount obtained will actually fit....*/
                        {
                            memcpy(DecoderHandle->FilterBufferProducer, DecoderHandle->ExtractPesTempBuffer, AmountFromPTI);
                            
                            Wrap = FALSE;                            
                        }
                    }
                    
                        
                }
                else /* No space issues, just copy data into buffer */
                {
                    PTICopyErr = pti_copy_circular_to_linear(DecoderHandle->pti_buffer,
                                                DecoderHandle->pti_buffer_size,
                                                &DecoderHandle->pti_consumer,
                                                DecoderHandle->pti_producer,
                                                DecoderHandle->FilterBufferProducer,
                                                DecoderHandle->FilterBufferSize,
                                                &AmountFromPTI);

                    /* Update pti_buffer consumer pointer */
                    PTIConsErr = pti_set_consumer_ptr(DecoderHandle->pti_slot, DecoderHandle->pti_consumer);
                    
                    Wrap = FALSE;

                }

                STOP_EXTRACTPES_IF_REQUIRED(DecoderHandle, break);

                /* PTI copy error notification */
                if (PTICopyErr == TRUE)
                {
#ifndef  IGNORE_LOST_PES_ERROR
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                                  "** Filter: error copying pes **\n"));
                    if (STEVT_Notify(
                        DecoderHandle->stevt_handle, 
                        DecoderHandle->stevt_event_table[STSUBT_EVENT_LOST_PES & 0x0000FFFF],
                        DecoderHandle))
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                                      "** Filter: Error notifying event**\n"));
                    }
#endif
                    AmountFromPTI = 0;
                    PTICopyErr = FALSE;
                }

                /* PTI consumer update error notification */                
                if (PTIConsErr == TRUE)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                                  "** Filter: error setting pti pointer **\n"));
                    if (STEVT_Notify(
                        DecoderHandle->stevt_handle, 
                        DecoderHandle->stevt_event_table[STSUBT_EVENT_PES_NOT_CONSUMED & 0x0000FFFF],
                        DecoderHandle))
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                                      "** Filter: Error notifying event**\n"));
                    }                    
                    AmountFromPTI = 0;
                    PTIConsErr = FALSE;                    
                }


                /* If some bytes have been extracted from pti_buffer, commence parse */
                if (AmountFromPTI != 0)
                {
                    /* Update count and producer pointer */
                    if (Wrap == TRUE)
                    {
                        semaphore_wait(DecoderHandle->ExtractPesMutex);
                        DecoderHandle->FilterBufferPesAmount += AmountFromPTI;
                        DecoderHandle->FilterBufferProducer = DecoderHandle->FilterBuffer + (AmountFromPTI- FilterBufferUpperFree);
                        semaphore_signal(DecoderHandle->ExtractPesMutex);
                    }
                    else
                    {
                        semaphore_wait(DecoderHandle->ExtractPesMutex);
                        DecoderHandle->FilterBufferPesAmount += AmountFromPTI;
                        DecoderHandle->FilterBufferProducer += AmountFromPTI;                        
                        semaphore_signal(DecoderHandle->ExtractPesMutex);

                    }
                    
                    /* Trigger filtering task */
                    semaphore_signal(DecoderHandle->ExtractPesGotData);                    
                }
                
                STOP_EXTRACTPES_IF_REQUIRED(DecoderHandle, break);
                
            } /* if > 6 */
            
        } /* while running */
        
    } /* while TRUE */

}


/******************************************************************************\
 * Function: ExtractPes_Create
 * Purpose : Create a ExtractPes task
 * Parameters:
 *     DecoderHandle
 * Return  : ST_NO_ERROR on success
\******************************************************************************/

ST_ErrorCode_t ExtractPes_Create (STSUBT_InternalHandle_t *DecoderHandle)
{
  STSUBT_Task_t  *ExtractPesTask;
  ST_Partition_t *MemoryPartition = DecoderHandle->MemoryPartition;

  /* --- Allocate a linear buffer to store the temp PES data for wrap condition --- */
  
  if ((DecoderHandle->ExtractPesTempBuffer = 
      (U8*)memory_allocate(MemoryPartition, EXTRACT_PES_TEMP_BUFFER_SIZE)) == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"ExtractPes_Create: unable to allocate!\n"));
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }
  
  /* --- Allocate semaphores --- */
 
  if ((DecoderHandle->ExtractPesTaskControl = 
       memory_allocate(MemoryPartition, sizeof(semaphore_t))) == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
		"ExtractPes_Create: error creating semaphore ExtractPesTaskControl!\n"));
      /* delete allocated resources */
      memory_deallocate(MemoryPartition, DecoderHandle->ExtractPesTempBuffer);
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }
  semaphore_init_fifo(DecoderHandle->ExtractPesTaskControl,0);
  

  if ((DecoderHandle->ExtractPesMutex = 
       memory_allocate(MemoryPartition, sizeof(semaphore_t))) == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
		"ExtractPes_Create: error creating semaphore ExtractPesMutex!\n"));
      /* delete allocated resources */
      memory_deallocate(MemoryPartition, DecoderHandle->ExtractPesTempBuffer);
      semaphore_delete (DecoderHandle->ExtractPesTaskControl);
      memory_deallocate(MemoryPartition, DecoderHandle->ExtractPesTaskControl);
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }
  semaphore_init_fifo(DecoderHandle->ExtractPesMutex,1);
  
 
  if ((DecoderHandle->ExtractPesTaskSyncSem = 
       memory_allocate(MemoryPartition, sizeof(semaphore_t))) == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
		"ExtractPes_Create: error creating semaphore ExtractPesTaskSyncSem!\n"));

      /* delete allocated resources */
      memory_deallocate(MemoryPartition, DecoderHandle->ExtractPesTempBuffer);      
      semaphore_delete (DecoderHandle->ExtractPesTaskControl);
      memory_deallocate(MemoryPartition, DecoderHandle->ExtractPesTaskControl);
      semaphore_delete (DecoderHandle->ExtractPesMutex);
      memory_deallocate(MemoryPartition, DecoderHandle->ExtractPesMutex);
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }
  semaphore_init_fifo_timeout(DecoderHandle->ExtractPesTaskSyncSem,0);
  
  /* create data semap */
  if ((DecoderHandle->ExtractPesGotData = 
       memory_allocate(MemoryPartition, sizeof(semaphore_t))) == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
		"ExtractPes_Create: error creating semaphore ExtractPesTaskSyncSem!\n"));

      /* delete allocated resources */
      semaphore_delete (DecoderHandle->ExtractPesTaskControl);
      semaphore_delete(DecoderHandle->ExtractPesTaskSyncSem);
      semaphore_delete (DecoderHandle->ExtractPesMutex);
      memory_deallocate(MemoryPartition, DecoderHandle->ExtractPesMutex);
      memory_deallocate(MemoryPartition,DecoderHandle->ExtractPesTaskSyncSem);
      memory_deallocate(MemoryPartition, DecoderHandle->ExtractPesTaskControl);
      memory_deallocate(MemoryPartition, DecoderHandle->ExtractPesTempBuffer);
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }
  semaphore_init_fifo_timeout(DecoderHandle->ExtractPesGotData,0);
  

  /* ExtractPes is created stopped */ 
  DecoderHandle->ExtractPesStatus = STSUBT_TaskStopped;
 
  /* --- create the ExtractPes task --- */
  ExtractPesTask = SubtTaskCreate((void (*)(void *))ExtractPes_Body, 
                                  (void *)DecoderHandle,
                                  EXTRACTPES_STACK_SIZE, 
                                  EXTRACTPES_TASK_PRIORITY,
                                  "ExtractPes Task", 0,
                                  MemoryPartition);
  if (ExtractPesTask == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"ExtractPes_Create: error creating ExtractPes task!\n"));
      /* delete allocated resources */
      semaphore_delete(DecoderHandle->ExtractPesGotData);
      memory_deallocate(MemoryPartition,DecoderHandle->ExtractPesGotData);      
      semaphore_delete (DecoderHandle->ExtractPesTaskControl);      
      memory_deallocate(MemoryPartition, DecoderHandle->ExtractPesTaskControl);
      semaphore_delete (DecoderHandle->ExtractPesTaskSyncSem);
      memory_deallocate(MemoryPartition, DecoderHandle->ExtractPesTaskSyncSem);
      semaphore_delete (DecoderHandle->ExtractPesMutex);
      memory_deallocate(MemoryPartition, DecoderHandle->ExtractPesMutex);
      memory_deallocate(MemoryPartition, DecoderHandle->ExtractPesTempBuffer);
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }

  /* wait for the ExtractPes task to signal semaphore */
  semaphore_wait_timeout(DecoderHandle->ExtractPesTaskSyncSem, TIMEOUT_INFINITY);

  /* Register ExtractPesTask to be used by creator */
  DecoderHandle->ExtractPesTask = ExtractPesTask;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                "** ExtractPes_Create: ExtractPes created **\n"));

  return (ST_NO_ERROR);
}

/******************************************************************************\
 * Function: ExtractPes_Start
 * Purpose : Start the ExtractPes task
 * Parameters:
 * Return  : ST_NO_ERROR on success, 
 *           STSUBT_ERROR_DRIVER_RUNNING if currently running
\******************************************************************************/

ST_ErrorCode_t ExtractPes_Start (STSUBT_InternalHandle_t *DecoderHandle)
{
  /* ExtractPes should not be already running */

  if (DecoderHandle->ExtractPesStatus == STSUBT_TaskRunning)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"** ExtractPes_Start: Error: already running **\n"));
      return (STSUBT_ERROR_DRIVER_RUNNING);
  }

  /* Clear circular filter buffer pointers and variables */
  ResetExtPesBufferControl(DecoderHandle);
  
  /* --- Start the ExtractPes --- */
  DecoderHandle->ExtractPesStatus = STSUBT_TaskRunning; 
  semaphore_signal(DecoderHandle->ExtractPesTaskControl);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** ExtractPes_Start: ExtractPes started **\n"));

  return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: ExtractPes_Stop
 * Purpose : Stop the ExtractPes task
 * Parameters:
 * Return  : ST_NO_ERROR on success,
 *           STSUBT_ERROR_DRIVER_NOT_RUNNING
\******************************************************************************/

ST_ErrorCode_t ExtractPes_Stop (STSUBT_InternalHandle_t *DecoderHandle)
{
    clock_t wait_time;
  S32 res;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** ExtractPes_Stop: Stopping ExtractPes **\n"));

  if (DecoderHandle->ExtractPesStatus != STSUBT_TaskRunning)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
                    "** ExtractPes_Start: Error: not running **\n"));
      return (STSUBT_ERROR_DRIVER_NOT_RUNNING);
  }

  DecoderHandle->ExtractPesStatus = STSUBT_TaskStopped;

  wait_time = time_plus(time_now(),FIVE_SECONDS);
  res = semaphore_wait_timeout(DecoderHandle->ExtractPesTaskSyncSem, &wait_time); 
  if (res == - 1)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"** ExtractPes_Stop: ExtractPes Not Stopped **\n"));
      return (STSUBT_ERROR_STOPPING_EXTPES);
  }

  /* Clear circular filter buffer pointers and variables */
  ResetExtPesBufferControl(DecoderHandle);

  /* trigger filter task */
  semaphore_signal(DecoderHandle->ExtractPesGotData);
  
  return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: ExtractPes_Delete
 * Purpose : Delete a ExtractPes task
 * Parameters:
 * Return  : ST_NO_ERROR on success, else ...
 *           STSUBT_ERROR_DRIVER_RUNNING,
 *           STSUBT_ERROR_DELETING_EXTPES
\******************************************************************************/
 
ST_ErrorCode_t ExtractPes_Delete (STSUBT_InternalHandle_t *DecoderHandle)
{
  clock_t wait_time;
  S32 res;

  if (DecoderHandle->ExtractPesStatus == STSUBT_TaskRunning)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"ExtractPes_Delete: Error Task is still running\n"));
      return (STSUBT_ERROR_DRIVER_RUNNING);
  }
 
  /* Force ExtractPes task to suicide 
   * ExtractPes is temporary made running signalling the semaphore ExtractPesTaskControl
   * in order to allow it to procede suiciding itself.
   */
 
  DecoderHandle->ExtractPesStatus = STSUBT_TaskSuicide;

  semaphore_signal(DecoderHandle->ExtractPesTaskControl);                

  /* wait for the ExtractPes task to suicide */
 
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
		"** ExtractPes_Delete: waiting for ExtractPes to suicide **\n"));

  wait_time = time_plus(time_now(), FIVE_SECONDS);
  res = semaphore_wait_timeout(DecoderHandle->ExtractPesTaskSyncSem, &wait_time); 
  if (res == -1)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"** ExtractPes_Delete: task not suicided.\n"));
      return (STSUBT_ERROR_DELETING_EXTPES); 
  }
 
  /* delete task (always succeeds since task is already terminated) */
  SubtTaskDelete(DecoderHandle->ExtractPesTask); 

  /* delete allocated resources */
  semaphore_delete(DecoderHandle->ExtractPesGotData);
  memory_deallocate(DecoderHandle->MemoryPartition,
                    DecoderHandle->ExtractPesGotData);      
  semaphore_delete (DecoderHandle->ExtractPesTaskControl);
  memory_deallocate(DecoderHandle->MemoryPartition,
                    DecoderHandle->ExtractPesTaskControl);
  semaphore_delete (DecoderHandle->ExtractPesTaskSyncSem);  
  memory_deallocate(DecoderHandle->MemoryPartition,
                    DecoderHandle->ExtractPesTaskSyncSem);
  semaphore_delete (DecoderHandle->ExtractPesMutex);
  memory_deallocate(DecoderHandle->MemoryPartition,
                    DecoderHandle->ExtractPesMutex);  
  memory_deallocate(DecoderHandle->MemoryPartition,
                    DecoderHandle->ExtractPesTempBuffer);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
			"** ExtractPes_Delete: Suicided **\n"));

  return (ST_NO_ERROR);
}



#endif /* _TRANSPORT_LINK */
