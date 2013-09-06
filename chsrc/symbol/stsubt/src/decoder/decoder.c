/******************************************************************************\
 *
 * File Name   : decoder.c
 *  
 * Description : The Processor processes subtitling data packets deposed in the
 *               Coded Data Buffer by the Filter (normal mode) or the 
 *               application (overlay mode). It parses the data of the
 *               different types of segments, or APDU, and organizes data in the
 *               Composition Buffer. Decoding of graphical objects results
 *               in pixel data that is stored in the Pixel Buffer.
 *  
 *               See SDD document for more details.
 *  
 * Author : Marino Congiu - Jan 1999 - Nov 1999
 *
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *  
 \******************************************************************************/
#include <stdlib.h>
#include <time.h>

#include <stddefs.h>
#include <stlite.h>
#include <stevt.h>
#include <stclkrv.h>

#include <subtdev.h>
#include <common.h>
#include <stsubt.h>
#include <subttask.h>
#include <compbuf.h>
#include <objcache.h>

#include "decoder.h"

static const pts_t PTS_ZERO = {0,0};

/******************************************************************************\
 * Function: get_segment_type
 * Return  : segment type
\******************************************************************************/
 
static __inline
U8 get_segment_type (STSUBT_InternalHandle_t * DecoderHandle, void *buffer)
{
  U8 segment_type;
  U8 *segment_p = (U8*)buffer;

  if (*segment_p != SEGMENT_SYNC_BYTE)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** processor: bad segment **\n"));
      if (STEVT_Notify(
                DecoderHandle->stevt_handle,
                DecoderHandle->stevt_event_table[STSUBT_EVENT_BAD_SEGMENT & 0x0000FFFF],
                DecoderHandle))
      {
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** processor: Error notifying event**\n"));
      }
      return (0);
  }
 
  segment_type = *(segment_p + 1);
 
  return segment_type;
}


/******************************************************************************\
 * Function: pack_download_reply
 * Return  : a download_reply() APDU
\******************************************************************************/

static 
void pack_download_reply (U16 ObjectId, U8 download_result, U32 *download_reply)
{
  U32  download_reply_apdu = 0;
  U8  *download_reply_apdu_p = (U8*)&download_reply_apdu;

  download_reply_apdu_p[0] = (ObjectId & 0xff00) >> 8;
  download_reply_apdu_p[1] =  ObjectId & 0x00ff;
  download_reply_apdu_p[2] =  download_result;

  *download_reply = download_reply_apdu;
  return;
}

/******************************************************************************\
 * Function: pack_scene_done_message
 * Return  : a scene_done_message() APDU
\******************************************************************************/

static 
void pack_scene_done_message (U8  decoder_continue_flag,
                              U8  scene_reveal_flag,
                              U8  scene_tag,
                              U8 *scene_done_message)
{
  U8 flags = 0;
  flags  = decoder_continue_flag << 7;
  flags |= scene_reveal_flag << 6;
  flags |= scene_tag & 0xf;
  *scene_done_message = flags;
  return;
}


/******************************************************************************\
 * Macro   : STOP_PROCESSOR_IF_REQUIRED
 * Purpose : No OS20 routines allow to externally stop or delete a running
 *           task: this macro is to force the processor task (by itself) to stop
 *           and then either restart or suicide.
 * Note    : (status = Suspended) is set by Processor_Pause to force task 
 *           pausing.
 * Note    : (status = Stopped) is set by Processor_Stop to force task stopping
 *           running. The task tests Status value and when it is Stopped
 *           it first "signal" the calling task that it is going to suspend 
 *           itself (TaskSyncSem) and then suspends itself on semaphore 
 *           TaskControl.  
 * Note    : (status = Suicide) is set during Processor_Delete. 
 *           Processor must be stopped before suiciding; in that case the 
 *           processor task was suspended in the TaskControl queue. 
 *           Task is temporary restarted during Processor_Delete in order to 
 *           allow the processor to suicide.
 *           Caller task is signalled when processor stops/pauses.
\******************************************************************************/

#define STOP_PROCESSOR_IF_REQUIRED(handle,command)			\
  {									\
      STSUBT_TaskStatus_t ProcessorStatus = handle->ProcessorStatus;	\
      if ((ProcessorStatus == STSUBT_TaskSuspended)			\
      ||  (ProcessorStatus == STSUBT_TaskStopped) )			\
      {                                                        		\
          if (ProcessorStatus == STSUBT_TaskStopped)			\
          {								\
              STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor: Stopping **\n"));		\
              semaphore_signal(handle->ProcessorTaskSyncSem); 		\
              semaphore_wait(handle->ProcessorTaskControl);  		\
              if (handle->ProcessorStatus == STSUBT_TaskSuicide)	\
              {								\
                  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor: Suiciding **\n"));		\
                  semaphore_signal(handle->ProcessorTaskSyncSem); 	\
                  return ST_NO_ERROR; 					\
              }								\
              STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor: Starting **\n"));		\
              handle->AcquisitionMode = STSUBT_Starting;		\
              BufferLookup = handle->CI_WorkingBuffer;	 		\
              WorkingDataSize = 0;		 			\
              handle->LastPTS = PTS_ZERO;				\
              command;							\
          }								\
          else								\
          {								\
              STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor: Suspending **\n"));		\
              semaphore_signal(handle->ProcessorTaskSyncSem); 		\
              semaphore_wait(handle->ProcessorTaskControl);  		\
              STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor: Resuming **\n"));		\
          }								\
      }									\
  }


/******************************************************************************\
 * Function: ProcessSegment
 * Purpose : Process segment depending on segment type.
 * Parameters:
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/

static __inline
ST_ErrorCode_t ProcessSegment (STSUBT_InternalHandle_t * DecoderHandle,
                               U8 segment_type,
                               Segment_t *Segment)
{
  ST_ErrorCode_t  res = ST_NO_ERROR;

  /* --- dispatch segment to corresponding processor --- */

    
 
  switch (segment_type) 
  {
     case PTS_SEGMENT_TYPE:
          if ((res = pts_processor(DecoderHandle, 
                                   Segment)) != ST_NO_ERROR)
          {
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** Processor: error processing PTS **\n"));
              DecoderHandle->AcquisitionMode = STSUBT_Starting;
          }
          break;
     case PCS_SEGMENT_TYPE:

          if ((res = pcs_processor(DecoderHandle, 
                                   Segment)) != ST_NO_ERROR)
          {
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** Processor: error processing PCS **\n"));
              DecoderHandle->AcquisitionMode = STSUBT_Starting;
          }
          /* Queue for display */
          if ((DecoderHandle->WorkingMode == STSUBT_NormalMode) &&
              (DecoderHandle->SubtitleStandard == STSUBT_DVBStandard))
          {              
              /* Insert to buffer */
              res = fcBuffer_Insert (DecoderHandle->PCSBuffer, &DecoderHandle->PCS);              
              if (res != ST_NO_ERROR)
              {
                  return (STSUBT_ERROR_PROCESSING_SEGMENT);
              }
          }      
          break;
     case RCS_SEGMENT_TYPE:
          if ((res = rcs_processor(DecoderHandle, 
                                   Segment)) != ST_NO_ERROR)
          {
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** Processor: error processing RCS **\n"));
              DecoderHandle->AcquisitionMode = STSUBT_Starting;
          }
          break;
     case CLUT_SEGMENT_TYPE:
         semaphore_wait(DecoderHandle->DisplayMutex);
          if ((res = clut_processor(DecoderHandle, 
                                    Segment)) != ST_NO_ERROR)
          {               
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** Processor: error processing CLUT **\n"));
              DecoderHandle->AcquisitionMode = STSUBT_Starting;
          }
          semaphore_signal(DecoderHandle->DisplayMutex);
          break;
     case OBJECT_SEGMENT_TYPE:
         semaphore_wait(DecoderHandle->DisplayMutex);
          if ((res = object_processor(DecoderHandle, 
                                      Segment)) != ST_NO_ERROR)
          {
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** Processor: error processing Object **\n"));
              DecoderHandle->AcquisitionMode = STSUBT_Starting;
          }
          DecoderHandle->ObjectNumber--;
          semaphore_signal(DecoderHandle->DisplayMutex);
          break;
     case EDS_SEGMENT_TYPE:
          if ((res = eds_processor(DecoderHandle, 
                                   Segment)) != ST_NO_ERROR)
          {
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** Processor: error processing EDS **\n"));
              DecoderHandle->AcquisitionMode = STSUBT_Starting;
          }
          break;
     default:
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** Processor: unknown segment type **\n"));
          DecoderHandle->AcquisitionMode = STSUBT_Starting;
          res = STSUBT_ERROR;
  }

  
  return (res);
}   

/******************************************************************************\
 * Function: ProcessApdu
 * Purpose : Process APDU packet.
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/

static 
ST_ErrorCode_t ProcessApdu (STSUBT_InternalHandle_t * DecoderHandle,
                           U32 apdu_tag,
                           U32 apdu_size,
                           Segment_t *Packet)
{
  ST_ErrorCode_t  res = ST_NO_ERROR;
  PCS_t          *PCS = &DecoderHandle->PCS;
  Segment_t       Segment;
  U8              segment_type;
  STSUBT_DisplayItem_t DisplayItem;

  switch (apdu_tag) 
  {
      case STSUBT_DECODER_PAUSE_TAG:
           /* Add delay to allow application to invoke Processor_Pause()
            * before Processor starts waiting for a new packet.
            */
           task_delay(DecoderHandle->clocks_per_sec);
           break;

      case STSUBT_SUBTITLE_SEGMENT_LAST_TAG:
           STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor: processing subtitle_segment()\n"));
           /* get segment type */              
           segment_type = get_segment_type(DecoderHandle, Packet->Data);

           /* process encapsulated subtitles segment */
           res = ProcessSegment(DecoderHandle, segment_type, Packet);
           break;

      case STSUBT_SCENE_END_MARK_TAG:
           STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor: processing scene_end_mark()\n"));
           {
               U8 data_field = *Packet->Data;
               U8 decoder_continue_flag = (data_field & 0x80) >> 7;
               U8 scene_reveal_flag = (data_field & 0x40) >> 6;
               U8 send_scene_done = (data_field & 0x20) >> 5;
               U8 scene_tag = data_field & 0xf;
               Object_t *ObjectList;

               /* --- check ReferredObjectInCache list --- */

               /* foreach object in the list, it is first retrieved
                * in the cache and then processed.
                */

               ObjectList = DecoderHandle->ReferredObjectInCache;
               while (ObjectList != NULL)
               {
                   U16 ObjectId = ObjectList->object_id;
                   U16 ObjectSize;

                   /* search object in the cache */
                   Segment.Data = ObjectCache_GetObject (
                                        DecoderHandle->ObjectCache,
                                        ObjectId, &ObjectSize);
                   if (Segment.Data == NULL)
                   {
                     STTBX_Report(
                     (STTBX_REPORT_LEVEL_ERROR,"** Processor: Referred obj in cache not found"));
                     ObjectList = ObjectList->next_object_p;
                     continue;
                   }
                   Segment.Size = ObjectSize;
             
                   /* process ODS segment */
                   object_processor(DecoderHandle, &Segment);

                   ObjectList = ObjectList->next_object_p;
               }
                
               /* if acquisition mode is acquisition, then
                * set epoch region list in the PCS, and 
                * set decoder acquisition mode to normal case 
                */

               if (DecoderHandle->AcquisitionMode == STSUBT_Acquisition)
               {
                   DecoderHandle->AcquisitionMode = STSUBT_Normal_Case;
                   SetEpochRegionList(DecoderHandle);
               }

               /* associate last PCS to scene_tag and store it */
               DecoderHandle->ArrayPCS[scene_tag] = *PCS;

               /* implement latest Page Composition display */
               if (scene_reveal_flag == 1)
               {
                   /* Notify last processed PCS to Display Timer */

                   DisplayItem.PCS = *PCS;
                   DisplayItem.display_descriptor = NULL;
                   DisplayItem.presentation_time = 0;
                   DisplayItem.display_timeout = PCS->page_time_out;
                   DisplayItem.EpochNumber = DecoderHandle->EpochNumber;

                   fcBuffer_Insert (DecoderHandle->DisplayBuffer, 
                                    (void*)&DisplayItem);
               }

               /* send a scene_done_message() APDU */
               if (send_scene_done == 1)
               {
                   /* pack a scene_done_message() APDU and insert it 
                    * in the CI Reply buffer.
                    */
                   U8 scene_done_message;
                   STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor: producing scene_done_message()"));
                   pack_scene_done_message(decoder_continue_flag,
                                           scene_reveal_flag,
                                           scene_tag,
                                           &scene_done_message);
                   cBuffer_InsertAPDU_ow(DecoderHandle->CI_ReplyBuffer,
                                         STSUBT_SCENE_DONE_MESSAGE_TAG,
                                         1,    
                                         &scene_done_message);
               }

               /* --- suspend the decoding process --- */

               /* Processor cannot invoke Processor_Pause() by itself.
                * Since Display Engine is idle when operating in 
                * overlay mode, it can be used to make that call.
                * A PCS just need to be included in the PCS Buffer.
                * PCS will be not processed at all.
                */
               if (decoder_continue_flag == 0)
               {
                   fcBuffer_Insert (DecoderHandle->PCSBuffer, PCS);
                   /* Add delay to allow Engine to invoke Processor_Pause()
                    * before Processor starts waiting for a new packet.
                    */
                   task_delay(DecoderHandle->clocks_per_sec);
               }
           }
           break;

      case STSUBT_SCENE_CONTROL_TAG:
           STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor: processing scene_control()\n"));
           {
               /* make visible the matching Page Composition Display */

               U8 scene_tag = (*Packet->Data) & 0xf;
               PCS_t MatchingPCS = DecoderHandle->ArrayPCS[scene_tag];

               MatchingPCS.EpochNumber = DecoderHandle->EpochNumber;
               DisplayItem.PCS = MatchingPCS;
               DisplayItem.display_descriptor = NULL;
               DisplayItem.display_timeout = MatchingPCS.page_time_out;
               DisplayItem.EpochNumber = DecoderHandle->EpochNumber;

               fcBuffer_Insert (DecoderHandle->DisplayBuffer,
                                (void*)&DisplayItem);
           }
           break;

      case STSUBT_SUBTITLE_DOWNLOAD_LAST_TAG:
           STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor: processing subtitle_download()\n"));
           {
               U16  ObjectId;
               U8  *ObjectId_p = (U8*)&ObjectId;
               U32  download_reply;
               U8   reply_res = 0;

               /* get segment type */
               segment_type = get_segment_type(DecoderHandle, Packet->Data);

               if (segment_type == OBJECT_SEGMENT_TYPE)
               {
                   /* extract object id */
                   ObjectId_p[1] = Packet->Data[6];
                   ObjectId_p[0] = Packet->Data[7];

                   reply_res = ObjectCache_StoreObject(
                                           DecoderHandle->ObjectCache,
                                           ObjectId,
                                           apdu_size,
                                           Packet->Data);
               }
               else 
               {
                   STTBX_Report(
                   (STTBX_REPORT_LEVEL_ERROR,"** Processor: not ODS in subtitle_download()\n"));
                   reply_res = 1;
               }

               /* pack an download_reply() APDU and insert it in the
                * CI Reply buffer.
                */

               STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor: producing download_reply() APDU"));
               pack_download_reply (ObjectId,reply_res,&download_reply);
               cBuffer_InsertAPDU_ow(DecoderHandle->CI_ReplyBuffer,
                                     STSUBT_DOWNLOAD_REPLY_TAG,
                                     3, /* object_id + download_reply_id */
                                     &download_reply);
           }
           break;

      case STSUBT_FLUSH_DOWNLOAD_TAG:
           STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor: processing flush_download() **\n"));
           ObjectCache_Flush (DecoderHandle->ObjectCache);
           break;

      default:
           STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"** Processor: unknown APDU received!\n"));
           return (STSUBT_ERROR_UNKNOWN_APDU);
  }

  return (res);
}


/******************************************************************************\
 * Function: Processor_Body
 * Purpose : Function which is executed by Processor task after created
 * Description: Within an infinite loop, it first waits for a segments/APDUs 
 *           to be deposed in the Coded Data Buffer, then, depending on the 
 *           segment/apsu type, it is processed by the corresponding processor.
 * Parameters:
 *     DecoderHandle: Returned pointer to STSUBT decoder handle
 * Return  : ST_NO_ERROR on success, 
\******************************************************************************/

ST_ErrorCode_t Processor_Body (STSUBT_InternalHandle_t * DecoderHandle)
{
  ST_ErrorCode_t  res;
  U8    	  segment_type;
  U32 		  data_size;
  U32 		  apdu_tag;
  U8              reset_flag;
  U8              packet_status;
  void           *data_pointer;
  Segment_t       Segment, Packet;  
  U8             *BufferLookup = DecoderHandle->CI_WorkingBuffer;
  U32             WorkingDataSize = 0;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor: I exist ! **\n"));

  /* --- Processor will stop at first call of STOP_PROCESSOR_IF_REQUIRED --- */

  while (TRUE) 
  {

      STOP_PROCESSOR_IF_REQUIRED(DecoderHandle, continue);

      if (DecoderHandle->WorkingMode == STSUBT_NormalMode)
      {
          /* --- waits for a segment to be deposed in Coded Data Buffer --- */

          STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor: waiting for a segment **\n"));
 
          res = cBuffer_Peek(DecoderHandle->CodedDataBuffer,
                             &data_size, 
                             &data_pointer);
     
          STOP_PROCESSOR_IF_REQUIRED(DecoderHandle, continue);
     
          if (res != ST_NO_ERROR)
          {
              /* Force a deschedule (ddts 13736) */
              task_delay(DecoderHandle->clocks_per_sec/1000);
              if (res == CBUFFER_ERROR_INVALID_DATA) continue;
     
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** Processor: error getting segment\n"));
              continue;
          }
    
          Segment.Data = (U8*)data_pointer;
          Segment.Size = data_size;
     
          /* --- get segment type --- */
          segment_type = get_segment_type(DecoderHandle, Segment.Data);

          task_delay(DecoderHandle->clocks_per_sec/1000);
          
	    if(DecoderHandle->SubtitleStandard == STSUBT_DTGStandard 
         && segment_type == PCS_SEGMENT_TYPE )
	    {
            clock_t running_time;

            running_time = time_plus(time_now(),DecoderHandle->clocks_per_sec*2); 

            while((fcBuffer_IsEmpty(DecoderHandle->DisplayBuffer)==FALSE)
			    && DecoderHandle->ProcessorStatus == STSUBT_TaskRunning )
 		    {
		        STTBX_Report((STTBX_REPORT_LEVEL_USER1,"Decoder: DisplayBuffer not Empty "));

		        task_delay(DecoderHandle->clocks_per_sec/50);

                if(time_after(running_time,time_now()))
                    break;

		    }
        }
     
          STTBX_Report((STTBX_REPORT_LEVEL_USER1,"\n** Processor: got segment type %d **\n", segment_type));
     
          /* --- process subtitles segment --- */

          /*     ProcessSegment(DecoderHandle, segment_type, &Segment);*/
          ProcessSegment(DecoderHandle, segment_type, &Segment);

    
          /* --- consume just processed data segment packet --- */
          cBuffer_Remove(DecoderHandle->CodedDataBuffer);

      }   
      else	/* **** overlay mode processing **** */
      {
          /* --- waits for an APDU to be deposed in the Coded Data Buffer --- */

          STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor: waiting for APDU **\n"));
 
          res = cBuffer_PeekAPDU(DecoderHandle->CodedDataBuffer,
                                 &apdu_tag,
                                 &data_size,
                                 &data_pointer,
                                 &packet_status,
                                 &reset_flag);
          STOP_PROCESSOR_IF_REQUIRED(DecoderHandle, continue);
          if (res != ST_NO_ERROR)
          {
              if (res == CBUFFER_ERROR_INVALID_DATA) continue;
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** Processor: error getting APDU\n"));
              continue;
          }
     
          /* --- check if data is split in more than one packet --- */
          /* --- if so APDU is copied in a linear buffer --- */

          if (reset_flag == TRUE) 
          {
              BufferLookup = DecoderHandle->CI_WorkingBuffer;
              WorkingDataSize = 0;
          }

          if (packet_status == CBUFFER_SINGLE)
          {
              if (apdu_tag == STSUBT_DECODER_PAUSE_TAG)
              {
                  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor: got decoder_pause apdu\n"));
              }
              else
              {
                  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor: APDU data continuous in buffer\n"));
                  Packet.Data = (U8*)data_pointer;
                  Packet.Size = data_size;
                  BufferLookup = DecoderHandle->CI_WorkingBuffer;
                  WorkingDataSize = 0;
              }
          }
          else
          {
              STTBX_Report((STTBX_REPORT_LEVEL_USER1,"\n** Processor: got last APDU portion\n"));
              res = cBuffer_CopyAPDU(DecoderHandle->CodedDataBuffer,
                                     &apdu_tag,
                                     &data_size,
                                     BufferLookup,
                                     CI_WORKING_BUFFER_SIZE-WorkingDataSize);
              STOP_PROCESSOR_IF_REQUIRED(DecoderHandle, continue);
              if (res != ST_NO_ERROR)
              {
                  BufferLookup = DecoderHandle->CI_WorkingBuffer;
                  WorkingDataSize = 0;
                  if (res == CBUFFER_ERROR_INVALID_DATA) continue;
                  STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** Processor: error getting APDU\n"));
                  continue;
              }

              if (packet_status == CBUFFER_MORE)
              {
                  STTBX_Report((STTBX_REPORT_LEVEL_USER1,"\n**Processor: packed data not complete yet\n"));
                  BufferLookup += data_size;
                  WorkingDataSize += data_size;
                  continue;
              }
              else /* if (packet_status == CBUFFER_LAST) */
              {
                  STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** Processor: APDU complete in Working buffer"));
                  WorkingDataSize += data_size;
                  data_size = WorkingDataSize;
                  Packet.Data = DecoderHandle->CI_WorkingBuffer;
                  Packet.Size = WorkingDataSize;

                  BufferLookup = DecoderHandle->CI_WorkingBuffer;
                  WorkingDataSize = 0;
              }
          }

          /* --- Process APDU Packet --- */
        
          ProcessApdu(DecoderHandle, apdu_tag, data_size, &Packet);

          /* --- consume just processed data packet --- */
    
          if (packet_status == CBUFFER_SINGLE)
              cBuffer_RemoveAPDU(DecoderHandle->CodedDataBuffer);
      }
  
  } /* End infinite loop */

} /* End Processor_Body */


/******************************************************************************\
 * Function: Processor_Create
 * Purpose : Create a processor task
 * Parameters:
 *     DecoderHandle:
 *           IN/OUT Decoder handle structure
 *           The following parameters need to be set in DecoderHandle in case
 *           the processor is started separately by decoder:
 *               CompositionBuffer;
 *               CodedDataBuffer;
 *               PixelBuffer;
 *               PCSBuffer;
 *               decoderStatus;
 *               SubtitleStandard;
 *               AutoDetection;
 *
 * Return  : ST_NO_ERROR on success
\******************************************************************************/

ST_ErrorCode_t Processor_Create (STSUBT_InternalHandle_t *DecoderHandle)
{
  STSUBT_Task_t  *ProcessorTask;
  ST_Partition_t *MemoryPartition = DecoderHandle->MemoryPartition;

  /* --- Allocate semaphores --- */

  if ((DecoderHandle->ProcessorTaskControl = 
       memory_allocate(MemoryPartition, sizeof(semaphore_t))) == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"Processor_Create: error creating semaphore TaskControl\n"));
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }
  semaphore_init_fifo(DecoderHandle->ProcessorTaskControl,0);
 
  if ((DecoderHandle->ProcessorTaskSyncSem = 
       memory_allocate(MemoryPartition, sizeof(semaphore_t))) == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"Processor_Create: error creating semaphore TaskSyncSem\n"));
      /* Release all allocated resources */
      semaphore_delete (DecoderHandle->ProcessorTaskControl);
      memory_deallocate(MemoryPartition, DecoderHandle->ProcessorTaskControl);
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }
  semaphore_init_fifo_timeout(DecoderHandle->ProcessorTaskSyncSem,0);

  /* --- create the processor task --- */

  /* processor task is created stopped */

  DecoderHandle->ProcessorStatus = STSUBT_TaskStopped;

  ProcessorTask = SubtTaskCreate((void (*)(void *))Processor_Body, 
                                 (void *)DecoderHandle,
                                 PROCESSOR_STACK_SIZE, 
                                 PROCESSOR_TASK_PRIORITY,
                                 "Processor Task", 0,
                                 MemoryPartition);
  if (ProcessorTask == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"Processor_Create: error creating processor task!\n"));
      /* Release all allocated resources */
      semaphore_delete (DecoderHandle->ProcessorTaskControl);      
      memory_deallocate(MemoryPartition, DecoderHandle->ProcessorTaskControl);
      semaphore_delete (DecoderHandle->ProcessorTaskSyncSem);      
      memory_deallocate(MemoryPartition, DecoderHandle->ProcessorTaskSyncSem);
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }

  /* wait for the processor task to signal semaphore */
 
  semaphore_wait_timeout(DecoderHandle->ProcessorTaskSyncSem, TIMEOUT_INFINITY);

  /* --- Fill in Decoder handle structure --- */
 
  DecoderHandle->ProcessorTask = ProcessorTask;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor_Create: Processor created **\n"));

  return (ST_NO_ERROR);
}

 
/******************************************************************************\
 * Function: Processor_Start
 * Purpose : Start the processor task
 * Parameters:
 * Return  : ST_NO_ERROR
 *           STSUBT_ERROR_DRIVER_RUNNING 
\******************************************************************************/

ST_ErrorCode_t Processor_Start (STSUBT_InternalHandle_t *DecoderHandle)
{
  /* Processor should not be already running */
 
  if (DecoderHandle->ProcessorStatus == STSUBT_TaskRunning)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"** Processor_Start: Error: already running **\n"));
      return (STSUBT_ERROR_DRIVER_RUNNING);
  }
 
  /* --- Start the processor task --- */
 
  DecoderHandle->ProcessorStatus = STSUBT_TaskRunning;

  semaphore_signal(DecoderHandle->ProcessorTaskControl);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor_Start: processor started **\n"));

  return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: Processor_Stop
 * Purpose : Stop the processor task
 * Parameters:
 * Return  : ST_NO_ERROR on success, 
 *           STSUBT_ERROR_DRIVER_NOT_RUNNING or
 *           STSUBT_ERROR_STOPPING_PROCESSOR on error
\******************************************************************************/

ST_ErrorCode_t Processor_Stop (STSUBT_InternalHandle_t *DecoderHandle)
{
  clock_t wait_time;
  S32 res;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor_Stop: Stopping Processor **\n"));

  if (DecoderHandle->ProcessorStatus != STSUBT_TaskRunning)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"** Processor_Stop: Error: not running **\n"));
      return (STSUBT_ERROR_DRIVER_NOT_RUNNING);
  }

  DecoderHandle->ProcessorStatus = STSUBT_TaskStopped;

  /* Invalidate data in Coded Data Buffer, and force the processor task
   * out of the semaphore queue in case it is suspended there
   */

  cBuffer_FlushOutConsumer(DecoderHandle->CodedDataBuffer);

  /* --- Flush PCS Buffer --- */

  fcBuffer_FlushoutProducer(DecoderHandle->PCSBuffer);

  /* --- wait for the processor task to stop --- */

  STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** Processor_Stop: waiting for processor to stop **\n"));

  wait_time = time_plus(time_now(), 5*DecoderHandle->clocks_per_sec);
  res = semaphore_wait_timeout(DecoderHandle->ProcessorTaskSyncSem, &wait_time); 
  if (res == - 1)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"** Processor_Stop: Processor Not Stopped **\n"));
      return (STSUBT_ERROR_STOPPING_PROCESSOR);
  }

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor_Stop: Stopped **\n"));

  return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: Processor_Pause
 * Purpose : Suspend the processor task
 * Parameters:
 * Return  : ST_NO_ERROR on success,
 *           STSUBT_ERROR_DRIVER_NOT_RUNNING or
 *           STSUBT_ERROR_STOPPING_PROCESSOR on error
\******************************************************************************/

ST_ErrorCode_t Processor_Pause (STSUBT_InternalHandle_t *DecoderHandle)
{
  clock_t wait_time;
  S32 res;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor_Pause: Suspending Processor **\n"));

  if (DecoderHandle->ProcessorStatus != STSUBT_TaskRunning)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"** Processor_Pause: Error: not running **\n"));
      return (STSUBT_ERROR_DRIVER_NOT_RUNNING);
  }

  DecoderHandle->ProcessorStatus = STSUBT_TaskSuspended;

  /* --- wait for the processor task to Pause --- */

  STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** Processor_Pause: waiting for processor to pause **\n"));

  wait_time = time_plus(time_now(), 5*DecoderHandle->clocks_per_sec);
  res = semaphore_wait_timeout(DecoderHandle->ProcessorTaskSyncSem, &wait_time);

  if (res == - 1)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"** Processor_Pause: Processor Not Suspended **\n"));
      return (STSUBT_ERROR_STOPPING_PROCESSOR);
  }

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor_Pause: Suspended **\n"));

  return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: Processor_Resume
 * Purpose : Resume the processor task
 * Parameters:
 * Return  : ST_NO_ERROR
 *           STSUBT_ERROR_DRIVER_NOT_SUSPENDED
\******************************************************************************/

ST_ErrorCode_t Processor_Resume (STSUBT_InternalHandle_t *DecoderHandle)
{
  /* Processor should not be already running */

  if (DecoderHandle->ProcessorStatus != STSUBT_TaskSuspended)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"** Processor_Resume: Error: not suspended **\n"));
      return (STSUBT_ERROR_DRIVER_NOT_SUSPENDED);
  }

  /* --- Start the processor task --- */

  DecoderHandle->ProcessorStatus = STSUBT_TaskRunning;

  semaphore_signal(DecoderHandle->ProcessorTaskControl);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor_Resume: processor resumed **\n"));

  return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: Processor_Delete
 * Purpose : Delete a processor task
 * Parameters:
 * Return  : ST_NO_ERROR on success, else ...
 *           STSUBT_ERROR_DRIVER_RUNNING,
 *           STSUBT_ERROR_DELETING_PROCESSOR
\******************************************************************************/
 
ST_ErrorCode_t Processor_Delete (STSUBT_InternalHandle_t *DecoderHandle)
{
  clock_t wait_time;
  S32 res;

  if (DecoderHandle->ProcessorStatus == STSUBT_TaskRunning)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"Processor_Delete: Error Task is still running\n"));
      return (STSUBT_ERROR_DRIVER_RUNNING);
  }
 
  /* Force processor task to suicide 
   * Processor is temporary made running signalling the semaphore TaskControl
   * in order to allow it to procede suiciding itself.
   */
 
  DecoderHandle->ProcessorStatus = STSUBT_TaskSuicide;
 
  semaphore_signal(DecoderHandle->ProcessorTaskControl);                

  /* wait for the processor task to suicide */
 
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor_Delete: waiting for processor to suicide **\n"));

  wait_time = time_plus(time_now(), 5 * DecoderHandle->clocks_per_sec);
  res = semaphore_wait_timeout(DecoderHandle->ProcessorTaskSyncSem, &wait_time);
  if (res == - 1)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"** Processor_Delete: task not suicided.\n"));
      return (STSUBT_ERROR_DELETING_PROCESSOR);
  }
 
  /* delete task (always succeeds since task is already terminated) */

  SubtTaskDelete(DecoderHandle->ProcessorTask);

  /* Release all allocated resources */

  semaphore_delete (DecoderHandle->ProcessorTaskControl);  
  memory_deallocate(DecoderHandle->MemoryPartition,
                    DecoderHandle->ProcessorTaskControl);
  semaphore_delete (DecoderHandle->ProcessorTaskSyncSem);  
  memory_deallocate(DecoderHandle->MemoryPartition,
                    DecoderHandle->ProcessorTaskSyncSem);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Processor_Delete: Suicided **\n"));

  return (ST_NO_ERROR);
}
