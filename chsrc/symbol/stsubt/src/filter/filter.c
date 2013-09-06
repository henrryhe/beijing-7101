/******************************************************************************\
 *
 * File Name   : filter.c
 *  
 * Description : 
 *  
 * Copyright 1998, 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author : Raciti 2001
 *  
\******************************************************************************/
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
#include <subtdev.h>
#include "filter.h"
#include "extpes.h"
/*#include "..\main\report.h"*/
#define DELAY_1_SEC		    (1000)
#define SEC_TO_WAIT_TS		10
#define DELAY_20_SEC		(20 * DecoderHandle->clocks_per_sec)
#define DELAY_10_SEC		(10 * DecoderHandle->clocks_per_sec)
#define FIVE_SECONDS		(5  * DecoderHandle->clocks_per_sec)


/* Private Variables ------------------------------------------------------ */

#if !defined (PTI_EMULATION_MODE) && defined (DVD_TRANSPORT_LINK) 

static __inline int FindNextStartCode(U32 *StartAddress,
                                      U32 ByteCount,
                                      U32 *BufferBase,
                                      U32 BufferSize,
                                      U8 **StartCodeAddr,
                                      U8 **NextConsumerPtr,
                                      U32 *Index);


static __inline void MoveConsumerPtrsToNextPes(STSUBT_InternalHandle_t *DecoderHandle);
                                     
#endif


/******************************************************************************\
 * Macro   : STOP_FILTER_IF_REQUIRED
 * Purpose : No OS20 routines allow to externally stop or delete a running
 *           task: this macro is to force the Filter task (by itself) to stop
 *           and then either restart or suicide.
 * Note    : (status = Stopped) is set by Filter_Stop to force task stopping
 *           running. The task tests Status value and when it is Stopped
 *           it first "signal" the calling task that it is going to suspend
 *           itself (FilterTaskSyncSem) and then suspends itself on semaphore
 *           FilterTaskControl.
 * Note    : (status = Suicide) is set during Filter_Delete.
 *           Filter must be stopped before suiciding; in that case the
 *           filter task was suspended in the FilterTaskControl queue.
 *           Task is temporary restarted during Filter_Delete in order to
 *           allow the filter to suicide.
 *           Caller task is signalled when filter is suspending.
\******************************************************************************/
 
#define STOP_FILTER_IF_REQUIRED(handle,command)			\
  {								\
      if (handle->FilterStatus == STSUBT_TaskStopped)		\
      {								\
          STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Filter: Stopping **\n"));		\
          semaphore_signal(handle->FilterTaskSyncSem); 		\
          semaphore_wait(handle->FilterTaskControl);  		\
          if (handle->FilterStatus == STSUBT_TaskSuicide)	\
          {							\
              STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Filter: Suiciding **\n"));		\
              semaphore_signal(handle->FilterTaskSyncSem); 	\
              return ST_NO_ERROR; 				\
          }							\
          STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Filter: Starting **\n"));		\
          waiting_for_data = TRUE;				\
          waiting_for_segment = TRUE;				\
          command;						\
      }								\
  }
/*zxg add get one pes packet*/
/******************************************************************************\
 * Function: get_ts_packet
 * Purpose : copy a TS packet into a linear buffer
 *           The function will take the TS data from the buffer and copy 
 *           it to a linear buffer. Buffer pointers are updated as well.
 * Return  : ST_NO_ERROR on success
 *         : 1  No TS arrived.
 *         : 2  pti_extract_partial_pes_data() Failed.
 *         : 3  pti_set_consumer_ptr() Failed.
\******************************************************************************/
 
#if !defined PTI_EMULATION_MODE && (defined DVD_TRANSPORT_STPTI || defined DVD_TRANSPORT_PTI)

#ifdef DVD_TRANSPORT_STPTI
static __inline ST_ErrorCode_t get_ts_packet (
			STSUBT_InternalHandle_t *DecoderHandle,
			BOOL *payload_unit_start,
			U8  *cc,
			BOOL *cc_discontinuity)
{

  STPTI_Buffer_t SignaledBuffer;
  ST_ErrorCode_t ErrorCode;
  U32 data_size;
  U8 NumberSec=0;
  U8  TsBuffer[188] = { 0 };

#endif


#if defined (DVD_TRANSPORT_PTI)
static __inline ST_ErrorCode_t get_ts_packet (
			STSUBT_InternalHandle_t *DecoderHandle,
			boolean *payload_unit_start,
			unsigned int *cc,
			boolean *cc_discontinuity)
{
   clock_t wait_time;
   boolean    exit_res_copy = FALSE;
   boolean    exit_res_set = FALSE;
   boolean    buffer_wrap;
   unsigned int     block0_size;
   unsigned char     *block0;
   unsigned int      block1_size;
   unsigned char     *block1;
   U32     data_size;
#endif

#if defined (DVD_TRANSPORT_STPTI)

   /* Wait for a TS packet to be deposed by demux --- */
 
  /*STTBX_Report((STTBX_REPORT_LEVEL_INFO,
				"** Filter: waiting for ts data **\n"));

  */
   STTBX_Print(("** Filter: waiting for ts data **\n"));
   do
   {
   	ErrorCode=STPTI_SignalWaitBuffer(DecoderHandle->stpti_signal,
				&SignaledBuffer,DELAY_1_SEC );

	if(ErrorCode!=ST_NO_ERROR && ErrorCode!=ST_ERROR_TIMEOUT)
	{
  		/*STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
				"STPTI_SignalWaitBuffer 0x%x\n",ErrorCode));*/
		  STTBX_Print(("STPTI_SignalWaitBuffer Error\n"));
		return (WAIT_ERROR);
	}

	NumberSec++;

   } while(NumberSec < SEC_TO_WAIT_TS && ErrorCode==ST_ERROR_TIMEOUT &&
	DecoderHandle->FilterStatus == STSUBT_TaskRunning);


   if(ErrorCode != ST_NO_ERROR ||
		 DecoderHandle->FilterStatus != STSUBT_TaskRunning)
   {
	   STTBX_Print(("STPTI WAIT_NO_TS\n"));
		return (WAIT_NO_TS);
   }
   if((ErrorCode=STPTI_BufferReadPartialPesPacket(
			DecoderHandle->stpti_buffer,
            TsBuffer,
			sizeof(TsBuffer),
			NULL,
			0,
 			payload_unit_start,
			cc_discontinuity,
			cc,
			&data_size,
			STPTI_COPY_TRANSFER_BY_MEMCPY))!=ST_NO_ERROR)
   {
  	/*	STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
				"STPTI_BufferReadPartialPesPacket: Error 0x%x\n",ErrorCode));
	*/
	      STTBX_Print(("STPTI_BufferReadPartialPesPacket: Error \n"));

        return (READ_ERROR);
   }
   else
   {
   STTBX_Print(("STPTI_BufferReadPartialPesPacket: OK \n"));
   }


   /* --- */

   if( *payload_unit_start == 1)
    	DecoderHandle->pes_buffer=DecoderHandle->FilterBuffer;


   if(FILTER_BUFFER_SIZE <
				 (DecoderHandle->pes_buffer-DecoderHandle->FilterBuffer))
   {
  		STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
			" PES too large for filter buffer\n"));
        return (READ_ERROR);
   }

   /* --- Perform copies --- */

   memcpy(DecoderHandle->pes_buffer, TsBuffer, data_size );

   DecoderHandle->pes_buffer+= data_size;

#endif 
#if defined (DVD_TRANSPORT_PTI) 
 
   /* Wait for a PES packet to be deposed by demux --- */
 
   STTBX_Report((STTBX_REPORT_LEVEL_INFO,
					"** Filter: waiting for ts data **\n"));
 
   wait_time = time_plus(time_now(), DELAY_10_SEC);
 

   if (semaphore_wait_timeout(DecoderHandle->pti_semaphore,
							 &wait_time) == -1)
   {
       return (1);
   }

   exit_res_copy = pti_extract_partial_pes_data(
				        DecoderHandle->pti_buffer,
                        DecoderHandle->pti_buffer_size,
                        &DecoderHandle->pti_consumer,
                        DecoderHandle->pti_producer,
 						payload_unit_start,
						cc_discontinuity,
						cc,
						&buffer_wrap, 
						&block0,
						&block0_size,
						&block1,
						&block1_size);

    data_size = block0_size + block1_size;

    /* --- */

    if(*payload_unit_start== 1)
    	DecoderHandle->pes_buffer=DecoderHandle->FilterBuffer;


    if(FILTER_BUFFER_SIZE < (block0_size + block1_size) )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
			"pti_extract_partial_pes_data - ts_packet too large for buffer\n" ));
        return (2);
    }

    /* --- Perform copies --- */

    memcpy( DecoderHandle->pes_buffer, block0, block0_size );

    if( buffer_wrap )
    {
        memcpy( DecoderHandle->pes_buffer+block0_size, block1, block1_size );
    }


   if (exit_res_copy == TRUE)
   {
       DecoderHandle->pti_consumer = DecoderHandle->pti_producer;
   }

   exit_res_set = pti_set_consumer_ptr (DecoderHandle->pti_slot,
                                        DecoderHandle->pti_consumer);

   if (exit_res_set == TRUE)
   {
       return (3);
   }

   if (exit_res_copy == TRUE)
   {
       return (2);
   }

   DecoderHandle->pes_buffer+= data_size;

   
#endif

   return (ST_NO_ERROR);

} 
#endif /* not def PTI_EMULATION_MODE */



/**********************************************************************\
 * Function: get_pes_packet (Link only)
 * Purpose : Searches the circular FilterBuffer for a PES start code
 *           and packet length.
 * Return  : ST_NO_ERROR on success
 *         : 1  No PES arrived.
\**********************************************************************/

#if !defined (PTI_EMULATION_MODE) && defined (DVD_TRANSPORT_LINK) 
static __inline
ST_ErrorCode_t get_pes_packet (STSUBT_InternalHandle_t *DecoderHandle)
{
    U32     ParseAmount = 0;
    S8  GotStartCode = 0;
    clock_t WaitTime;
    U32 PacketLength = 0;
    BOOL GotWholePacket = FALSE;
    U32 consumer_index;
    U32 producer_index;
    U32 StartCodeSrch =0;
    U32 StartCodeOffset = 0;
                                                         
    while ((GotWholePacket == FALSE) && (DecoderHandle->FilterStatus == STSUBT_TaskRunning))
    {

        /* Check data status.... */
        if ((DecoderHandle->PartialPesOffset != 0)  && (PacketLength == 0))
        {
            /* Parse data remaining from previous acquisition */
            ParseAmount = DecoderHandle->PartialPesOffset;
            DecoderHandle->PartialPesOffset = 0;
        }
        else 
        {
            /* Wait for data to arrive in exraction buffer */
            WaitTime = time_plus(time_now(), DELAY_10_SEC);
            if (semaphore_wait_timeout(DecoderHandle->ExtractPesGotData, &WaitTime) == -1)
            {
                GotWholePacket = FALSE;
                DecoderHandle->PartialPesOffset = 0;
                DecoderHandle->PesPacketLength = 0;
                break;
            }
            ParseAmount = DecoderHandle->FilterBufferPesAmount;            
            semaphore_wait(DecoderHandle->ExtractPesMutex);
            DecoderHandle->FilterBufferPesAmount -= ParseAmount;                        
            semaphore_signal(DecoderHandle->ExtractPesMutex);
            producer_index = (U32)(DecoderHandle->FilterBufferProducer - DecoderHandle->FilterBuffer);
            
        }
       
        consumer_index = (U32)(DecoderHandle->FilterBufferPesConsumer - DecoderHandle->FilterBuffer);
      
        /* Got some data to search and haven't got a start code yet...search */
        if ((GotStartCode == 0) && (ParseAmount != 0))
        {
            /* No known start code, more than 6 bytes */
            if ((DecoderHandle->FilterBuffer[(consumer_index)] != 0) ||
                (DecoderHandle->FilterBuffer[(consumer_index + 1) % DecoderHandle->FilterBufferSize] != 0) ||
                (DecoderHandle->FilterBuffer[(consumer_index + 2) % DecoderHandle->FilterBufferSize] != 1) ||
                (DecoderHandle->FilterBuffer[(consumer_index + 3) % DecoderHandle->FilterBufferSize] != 0xbd))
            {

                U8 *startcode;
                /* Not on a start code, trigger a search and wind on PesConsumer to suite */
                StartCodeSrch = FindNextStartCode((U32 *)DecoderHandle->FilterBufferPesConsumer,
                                                  ParseAmount,
                                                  (U32 *)DecoderHandle->FilterBuffer,
                                                  DecoderHandle->FilterBufferSize,
                                                  &startcode,
                                                  &DecoderHandle->FilterBufferPesConsumer,
                                                  &StartCodeOffset);
                    
                if (StartCodeSrch == 1)                    
                {
                    /* Remove redundant bytes from parse count and re-asses consumer index */
                    ParseAmount -= StartCodeOffset;
                    GotStartCode = 1;                    
                    consumer_index = (U32)(DecoderHandle->FilterBufferPesConsumer - DecoderHandle->FilterBuffer);

                    
                    PacketLength = (U32)(DecoderHandle->FilterBuffer[(consumer_index + 4) %
                                             DecoderHandle->FilterBufferSize] << 8) |
                                             (DecoderHandle->FilterBuffer[(consumer_index + 5) %
                                             DecoderHandle->FilterBufferSize]);
                    PacketLength += 6;

                    /* Recovery code left in, although it shouldn't
                     * be reached.
                     */                    
                    if ((PacketLength >  FILTER_BUFFER_SIZE) || (PacketLength == 6))
                    {
                        if (consumer_index >= DecoderHandle->FilterBufferSize)
                            DecoderHandle->FilterBufferPesConsumer =  (U8*)(&(DecoderHandle->FilterBuffer[producer_index]));
                        else
                            DecoderHandle->FilterBufferPesConsumer++;
                        
                        GotStartCode = 0;
                    }                   
                }
                else
                {                   
                    /* No start code found after search, skip data */
                    DecoderHandle->FilterBufferPesConsumer =  (U8*)(&(DecoderHandle->FilterBuffer[producer_index]));
                    DecoderHandle->PartialPesOffset = 0;
                    continue;  /* start again */
                }
                
            }
            else
            {
                PacketLength = (U32)(DecoderHandle->FilterBuffer[(consumer_index + 4) % DecoderHandle->FilterBufferSize] << 8) |
                    (DecoderHandle->FilterBuffer[(consumer_index + 5) % DecoderHandle->FilterBufferSize]);
                
                PacketLength += 6;
                GotStartCode = 1;

                /* Recovery code left in, although it shouldn't
                 * be reached.
                 */                
                if ((PacketLength > FILTER_BUFFER_SIZE) || (PacketLength == 6))
                {
                    if (consumer_index >= DecoderHandle->FilterBufferSize)
                        DecoderHandle->FilterBufferPesConsumer =  (U8*)(&(DecoderHandle->FilterBuffer[producer_index]));       
                    else
                        DecoderHandle->FilterBufferPesConsumer++;
                    DecoderHandle->PartialPesOffset = 0;
                    GotStartCode = 0;
                }
            }
        }
        

        /* Already got start code.....*/
        if (GotStartCode == 1)
        {
            /* Store progress through packet */
            DecoderHandle->PartialPesOffset += ParseAmount;
            
            /* check on progress */
            if ((DecoderHandle->PartialPesOffset >= PacketLength) ||
                (DecoderHandle->PartialPesOffset >= FILTER_BUFFER_SIZE))
            {
                /* Note number of bytes of next packet already obtained */
                DecoderHandle->PartialPesOffset -= PacketLength;
                
                /* Set all conumer pointers to the new packet */
                DecoderHandle->FilterBufferSegmentConsumer = DecoderHandle->FilterBufferPesConsumer;
                DecoderHandle->FilterBufferNextSegmentConsumer = DecoderHandle->FilterBufferPesConsumer;
                
                DecoderHandle->PesPacketLength = PacketLength;
                GotWholePacket = TRUE;
                GotStartCode = 0;
            }                
        }
    }

    if (GotWholePacket != TRUE)
        return (1);
    
    return (ST_NO_ERROR);
}

#endif  /* !PTI_EMU_MODE && DVD_TRANS_LINK */

#if (defined PTI_EMULATION_MODE || defined DVD_TRANSPORT_PTI || defined DVD_TRANSPORT_STPTI)
/******************************************************************************\
 * Function: parse_pes_packet
 * Purpose : parses the pes packet header and extracts pts. 
 *           Checks pes packet to contain subtitling data and determines 
 *           starting segment data in pes.
 * Argument :
 *     pes_packet:       A pointer to the begining of pes packet
 *     pts:              Returned pts structure
 *     segment_p:        Returned pointer to first segment in pes payload.
 *     pes_payload_size: Returned size of pes_payload
 * Return  : ST_NO_ERROR on success, STSUBT_ERROR_BAD_PES in case of error
\******************************************************************************/
 
static __inline
ST_ErrorCode_t parse_pes_packet (U8 *pes_packet, pts_t *pts, U8 **segment_p, U16 *pes_payload_size)
{
  U16 pes_packet_size;
  U8  pes_header_size;
  U8 *pes_payload_p;
  pts_t new_pts;

 
  /* --- check pes --- */
 
  /* check packet_start_prefix_code (0x000001) */
  if ((pes_packet[0] != 0) && (pes_packet[1] != 0) && (pes_packet[2] != 1))
  {
     return (STSUBT_ERROR_BAD_PES);
  }
 
  /* check private_stream_id (0xbd) */
  if (pes_packet[3] != PRIVATE_STREAM_ID_1) 
  {
     return (STSUBT_ERROR_BAD_PES);
  }
 
  /* check data_allignment_indicator (0x1) */
  if (! ((pes_packet[6] >> 2) & 1))
  {
     return (STSUBT_ERROR_BAD_PES);
  }
 
  /* --- get pes packet length --- */
 
  pes_packet_size = ((((U16)(pes_packet[4]))<< 8) + ((U16)(pes_packet[5]))) + 6;
 
  /* --- extract pts --- */


  if (! (pes_packet[7] >> 7) & 1 )          /* there is no pts */
  {
      return (STSUBT_ERROR_BAD_PES);
  }
  else
  {

      new_pts.high = (U32)(pes_packet[9] & 0x8) >> 3;

      new_pts.low  = (((U32)(pes_packet[9] & 0x6)) << 29)
                   +  ((U32)pes_packet[10] << 22)
                   + (((U32)pes_packet[11] & 0xFE) << 14)
                   +  ((U32)pes_packet[12] << 7)
                   + (((U32)pes_packet[13]) >> 1);

  }
  *pts = new_pts;


  /* --- get pes header length --- */
 
  pes_header_size = (*(U8*)(pes_packet + 8)) + 9;

 
  /* --- compute begining of pes payload --- */
 
  pes_payload_p = pes_packet + pes_header_size;
 
  /* --- compute size of pes payload --- */
 
  *pes_payload_size = pes_packet_size - pes_header_size;
 
  /* --- check pes data identifier --- */

  /* check packet_header_data_identifier (0x02) */
  if (pes_payload_p[0] != 0x20)
  {
     return (STSUBT_ERROR_BAD_PES);
  }

  /* check subtitle_stream_id (0x00) */
  if (pes_payload_p[1] != 0)
  {
     return (STSUBT_ERROR_BAD_PES);
  }

  /* --- compute begining of segments --- */

  *segment_p = pes_payload_p + 2;

  return (ST_NO_ERROR);
}

#endif  /* !PTI_EMU_MODE && (TRANS_PTI) || (TRANS_STPTI) */


#if !defined (PTI_EMULATION_MODE) && defined (DVD_TRANSPORT_LINK) 
/******************************************************************************\
 * Function:  ParseCircularPesPacket 
 * Purpose : Parses the Pes packet in the circular Filterbuffer.
 * Argument :
 * Return  : ST_NO_ERROR on success, STSUBT_ERROR_BAD_PES in case of error
\******************************************************************************/
 
static __inline
ST_ErrorCode_t ParseCircularPesPacket (STSUBT_InternalHandle_t *DecoderHandle, pts_t *pts, U16 *pes_payload_size)
{
  U16 pes_packet_size;
  U8  pes_header_size;
  pts_t new_pts;
  U32 consumer_index;

  consumer_index = (U32)(DecoderHandle->FilterBufferPesConsumer - DecoderHandle->FilterBuffer);
  
  /* --- check pes --- */
 
  /* check packet_start_prefix_code (0x000001) */
  if ((DecoderHandle->FilterBuffer[consumer_index] != 0) &&
      (DecoderHandle->FilterBuffer[(consumer_index + 1) % DecoderHandle->FilterBufferSize] != 0) &&
      (DecoderHandle->FilterBuffer[(consumer_index + 2) % DecoderHandle->FilterBufferSize] != 1))
  {
     return (STSUBT_ERROR_BAD_PES);
  }
 
  /* check private_stream_id (0xbd) */
  if ( (DecoderHandle->FilterBuffer[(consumer_index + 3) % DecoderHandle->FilterBufferSize]) != PRIVATE_STREAM_ID_1) 
  {
     return (STSUBT_ERROR_BAD_PES);
  }
 
  /* check data_allignment_indicator (0x1) */
  if (! (((DecoderHandle->FilterBuffer[(consumer_index + 6) % DecoderHandle->FilterBufferSize]) >> 2) & 1))
  {
     return (STSUBT_ERROR_BAD_PES);
  }
 
  /* --- get pes packet length --- */
 
  pes_packet_size = ((((U16)((DecoderHandle->FilterBuffer[(consumer_index + 4) % DecoderHandle->FilterBufferSize]))) << 8)
                     + ((U16)((DecoderHandle->FilterBuffer[(consumer_index + 5) % DecoderHandle->FilterBufferSize])))) + 6;
 

  /* --- extract pts --- */
  if (! (((DecoderHandle->FilterBuffer[(consumer_index + 7) % DecoderHandle->FilterBufferSize]) >> 7) & 1) )  
  {
      return (STSUBT_ERROR_BAD_PES);
  }
  else
  {

      new_pts.high = (U32)((DecoderHandle->FilterBuffer[(consumer_index + 9) % DecoderHandle->FilterBufferSize]) & 0x8) >> 3;

      new_pts.low  = (((U32)((DecoderHandle->FilterBuffer[(consumer_index + 9) % DecoderHandle->FilterBufferSize]) & 0x6)) << 29)
                   +  ((U32)(DecoderHandle->FilterBuffer[(consumer_index + 10) % DecoderHandle->FilterBufferSize]) << 22)
                   + (((U32)(DecoderHandle->FilterBuffer[(consumer_index + 11) % DecoderHandle->FilterBufferSize]) & 0xFE) << 14)
                   +  ((U32)(DecoderHandle->FilterBuffer[(consumer_index + 12) % DecoderHandle->FilterBufferSize]) << 7)
                   + (((U32)(DecoderHandle->FilterBuffer[(consumer_index + 13) % DecoderHandle->FilterBufferSize])) >> 1);

  }
  *pts = new_pts;


  /* --- get pes header length --- */ 
  pes_header_size = ((U8)(DecoderHandle->FilterBuffer[(consumer_index + 8) % DecoderHandle->FilterBufferSize]) + 9);
 
 
  /* --- compute size of pes payload --- */
  *pes_payload_size = pes_packet_size - pes_header_size;
 
  /* --- check pes data identifier --- */
  /* check packet_header_data_identifier (0x02) */
  if (DecoderHandle->FilterBuffer[(consumer_index + pes_header_size) % DecoderHandle->FilterBufferSize] != 0x20)
  {
     return (STSUBT_ERROR_BAD_PES);
  }

  /* check subtitle_stream_id (0x00) */
  if ((DecoderHandle->FilterBuffer[(consumer_index + pes_header_size + 1) % DecoderHandle->FilterBufferSize]) != 0)
  {
     return (STSUBT_ERROR_BAD_PES);
  }

  /* --- compute begining of segments --- */
     DecoderHandle->FilterBufferSegmentConsumer =
         (U8*)(&(DecoderHandle->FilterBuffer[(consumer_index + pes_header_size + 2)
                                            % DecoderHandle->FilterBufferSize]));

     DecoderHandle->FilterBufferNextSegmentConsumer =  DecoderHandle->FilterBufferSegmentConsumer;
   
  return (ST_NO_ERROR);
}
#endif

/******************************************************************************\
 * Function: pack_pts_segment
 * Purpose : Pack a fake pts segment
 *           To pts segment is given segment_type = 0x09 (must be different
 *           from any type used by dvb subtitling).
 * Arguments:
 *      segment_p:
 *           A pointer to created pts segment
\******************************************************************************/
 
static __inline
void pack_pts_segment (pts_t pts, U16 page_id,
                       segment_t *segment_p, U16 *size)
{
  const U16 segment_length = PTS_SEGMENT_LENGHT;
  U32 s;
 
  segment_p[0] = SEGMENT_SYNC_BYTE;
  segment_p[1] = PTS_SEGMENT_TYPE;
  segment_p[2] = (U8)(page_id >> 8);
  segment_p[3] = ((U8)(page_id));
  segment_p[4] = (U8)(segment_length >> 8);
  segment_p[5] = (U8)(segment_length);
  for (s=0; s<4; s++)
  {
     segment_p[s+6] = (U8)(pts.high >> ((3-s) *8));
  }
  for (s=0; s<4; s++)
  {
     segment_p[s+10] = (U8)(pts.low >> ((3-s) *8));
  }
 
  *size = PTS_SEGMENT_LENGHT;
}

#if defined PTI_EMULATION_MODE
/*****************************************************************************\
 * Function: get_next_segment_from_pes
 * Purpose : get next segment of current stream from pes payload
 * Return  : A pointer to segment, if any, NULL otherwise
\******************************************************************************/

static __inline
segment_t *get_next_segment_from_pes(U16 composition_page_id,
                                     U16 ancillary_page_id,
                                     U8 **segment_data_p, U16 *pes_payload_size,
                                     U16 *segment_size)
{
  U16 page_id;
  U16 segment_length;
  segment_t *segment_p;
  BOOL found_segment = FALSE;


  /* find starting of next segment of selected stream in pes */

  do {

    if (*pes_payload_size < 6) break;  /* empty payload or bad pes */

    segment_p = *segment_data_p;

    /* parse segment header */

    if (*segment_p != SEGMENT_SYNC_BYTE) break; /* bad pes */

    /* get page_id and segment_length of segment */

    page_id = (((U16)segment_p[2]) << 8) + ((U16)segment_p[3]);
    segment_length = ((((U16)segment_p[4]) << 8) + ((U16)segment_p[5]) + 6);

    /* check page_id and segment_length of segment */

    if (segment_length > *pes_payload_size) break; /* bad pes */

    if ((page_id == composition_page_id) || (page_id == ancillary_page_id))
       found_segment = TRUE;

    /* increment payload pointer to skip current segment */

    *segment_data_p += segment_length;

  } while (found_segment == FALSE);


  if (found_segment == FALSE)
  {
     *segment_size = 0;
      return (NULL);
  }

  *segment_size = segment_length;
  *pes_payload_size -= segment_length;
  return segment_p;
}

#endif /* PTI_EMU_MODE */

#if (defined DVD_TRANSPORT_LINK) && (!defined PTI_EMULATION_MODE) 
/*****************************************************************************\
 * Function: GetNextSegmentFromCircularPes
 * Purpose : get next segment of current stream from pes payload in the
             circular filter buffer.
 * Return  : A pointer to segment, if any, NULL otherwise
 \******************************************************************************/
static __inline
BOOL GetNextSegmentFromCircularPes(STSUBT_InternalHandle_t *DecoderHandle,
                                         U16 composition_page_id,
                                         U16 ancillary_page_id,
                                         U16 *pes_payload_size,
                                         U16 *segment_size)
{
  U16 page_id;
  U16 segment_length;
  BOOL found_segment = FALSE;
  U32 consumer_index;


  /* find starting of next segment of selected stream in pes */

  do {
      
      if (*pes_payload_size < 6)
      {
          break;  /* empty payload or bad pes */
      }

      /* Take index of payload consumer ptr */
      consumer_index = (U32)(DecoderHandle->FilterBufferNextSegmentConsumer - DecoderHandle->FilterBuffer);

      /* point to start of processable segment */
      DecoderHandle->FilterBufferSegmentConsumer = (U8*)(&DecoderHandle->FilterBuffer[consumer_index]);
      
      /* parse segment header */      
      if (DecoderHandle->FilterBuffer[consumer_index] != SEGMENT_SYNC_BYTE)
      {
          break; /* bad pes */
      }
      
      /* get page_id and segment_length of segment */
      
      page_id = ((((U16)(DecoderHandle->FilterBuffer[(consumer_index + 2) % DecoderHandle->FilterBufferSize])) << 8)
                 + ((U16)(DecoderHandle->FilterBuffer[(consumer_index + 3) % DecoderHandle->FilterBufferSize])));

      segment_length = ((((U16)(DecoderHandle->FilterBuffer[(consumer_index + 4) % DecoderHandle->FilterBufferSize])) << 8)
                        + ((U16)(DecoderHandle->FilterBuffer[(consumer_index + 5) % DecoderHandle->FilterBufferSize]) + 6));
      
      /* check page_id and segment_length of segment */
      
      if (segment_length > *pes_payload_size)
      {
          break; /* bad pes */
      }
      
      if ((page_id == composition_page_id) || (page_id == ancillary_page_id))
      {
          found_segment = TRUE;
      }
      
      /* increment payload pointer to skip current segment */
      DecoderHandle->FilterBufferNextSegmentConsumer = (U8*)(&(DecoderHandle->FilterBuffer[(consumer_index + segment_length)
                                                                                          % DecoderHandle->FilterBufferSize]));

  } while (found_segment == FALSE);


  if (found_segment == FALSE)
  {
     *segment_size = 0;
     return (FALSE);
  }

  *segment_size = segment_length;
  *pes_payload_size -= segment_length;
  return (TRUE);

}

#endif /* DVD_TRANSPORT_LINK */


#if ((!defined PTI_EMULATION_MODE) &&  ((defined DVD_TRANSPORT_PTI || defined DVD_TRANSPORT_STPTI)))
/******************************************************************************\
 * Function: get_next_segment_info
 * Purpose : get next segment of current stream from pes payload
 * Return  : A pointer to segment, if any, NULL otherwise
\******************************************************************************/
 
static __inline
segment_t  *get_next_segment_info(U16 composition_page_id,
                                     U16 ancillary_page_id,
                                     U8 **segment_data_p, U16 *pes_payload_size,
                                     U16 *segment_size)
{
  U16 page_id;
  U16 segment_length;
  segment_t *segment_p;
  BOOL found_segment = FALSE;

	/* find starting of next segment of selected stream in pes */


    if (*pes_payload_size < 6)   /* empty payload or bad pes */
  		found_segment = FALSE;

    segment_p = *segment_data_p;

    /* parse segment header */

    if (*segment_p != SEGMENT_SYNC_BYTE)  /* bad pes */
  		found_segment = FALSE;

    /* get page_id and segment_length of segment */

    page_id = (((U16)segment_p[2]) << 8) + ((U16)segment_p[3]);
    segment_length = ((((U16)segment_p[4]) << 8) + ((U16)segment_p[5])+ 6);

    /* check page_id and segment_length of segment */

    if (segment_length > *pes_payload_size)  /* bad pes */
  		found_segment = FALSE;

    if ((page_id == composition_page_id) || (page_id == ancillary_page_id))
       found_segment = TRUE;

    /* increment payload pointer to skip current segment */

     *segment_data_p += segment_length; 


  if (found_segment == FALSE)
  {
      *segment_size = 0;
    return (NULL);
  }

  *segment_size = segment_length;
  *pes_payload_size -= segment_length; 
  return segment_p;
}
#endif


#if defined PTI_EMULATION_MODE || defined DVD_TRANSPORT_PTI || defined DVD_TRANSPORT_STPTI

/******************************************************************************\
 * Function: Filter_Body
 * Purpose : Filter task Body
 * Parameters:
 * Return  : 
\******************************************************************************/
#if 1/*zxg change*/
ST_ErrorCode_t Filter_Body(STSUBT_InternalHandle_t * DecoderHandle)
{
   
  pts_t	PTS;
  U8    *segment_data_p;
  U16   pes_payload_size;
  segment_t *segment_p;
  U8    pts_segment[PTS_SEGMENT_LENGHT];
  U16   segment_size;
  ST_ErrorCode_t res;
  BOOL  waiting_for_data = TRUE;
  BOOL  waiting_for_segment = TRUE;
  BOOL    payload_unit_start;
  BOOL    cc_discontinuity;
  U8  cc;
  BOOL Start_SEG;
  BOOL First_PES;


  int ParsedLen;

  /* --- Filter will stop at first call of STOP_FILTER_IF_REQUIRED --- */


  First_PES=FALSE;


  while (TRUE) 
  {
     STOP_FILTER_IF_REQUIRED(DecoderHandle, continue);

     

     /* --- get a PES packet from pti buffer into the linear buffer  --- */

#if 0
     res = get_ts_packet (DecoderHandle,&payload_unit_start,
		&cc,&cc_discontinuity);
#else
	 {		 
		 res=CH_GetOnepes(DecoderHandle);		
		 payload_unit_start=1;
	 }
#endif

     STOP_FILTER_IF_REQUIRED(DecoderHandle, continue);
#if 1
if(res!=ST_NO_ERROR)
{
	waiting_for_data = TRUE;
	waiting_for_segment = TRUE;
	continue;
}
#else
     switch (res)
     {
         case ST_NO_ERROR:
			 /*do_report(0,"OK **\n");*/
             break;

         case WAIT_NO_TS:

             STTBX_Print((
					"** Filter: no pes data received **\n"));
              /*do_report(0,"no pes data received **\n");*/
              if (STEVT_Notify(DecoderHandle->stevt_handle,
                 DecoderHandle->stevt_event_table[STSUBT_EVENT_NO_PES_RECEIVED
					 & 0x0000FFFF],
                 DecoderHandle))
             {
                 STTBX_Print((
						"** Filter: Error notifying event**\n"));
             }
             waiting_for_data = TRUE;
             waiting_for_segment = TRUE;
             continue;
             break;

         case READ_ERROR:
			 /*do_report(0,"read error **\n");*/

             STTBX_Print((
					"** Filter: error copying pes **\n"));
             if (STEVT_Notify( DecoderHandle->stevt_handle, 
                        DecoderHandle->stevt_event_table[STSUBT_EVENT_LOST_PES
							 & 0x0000FFFF], DecoderHandle))
             {
                 STTBX_Print((
						"** Filter: Error notifying event**\n"));
             }
             waiting_for_data = TRUE;
             waiting_for_segment = TRUE;

             continue;
             break;

         case WAIT_ERROR:
			 /*do_report(0,"wait error **\n");*/
             STTBX_Print((						"** Filter: error setting pti pointer **\n"));
             if (STEVT_Notify(
                DecoderHandle->stevt_handle, 
                DecoderHandle->stevt_event_table[STSUBT_EVENT_PES_NOT_CONSUMED 
						& 0x0000FFFF], DecoderHandle))
             {
                 STTBX_Print((
							"** Filter: Error notifying event**\n"));
             }


  			STPTI_BufferFlush(DecoderHandle->stpti_buffer);

             waiting_for_data = TRUE;
             waiting_for_segment = TRUE;
             continue;
             break;

         default:
             continue;
             break;
     }
#endif
     /*STTBX_Print(("** Filter: got ts **\n"));*/

     if (waiting_for_data == TRUE)
     {
         if (STEVT_Notify( DecoderHandle->stevt_handle,
                   DecoderHandle->stevt_event_table[STSUBT_EVENT_PES_AVAILABLE
							 & 0x0000FFFF], DecoderHandle))
         {
             STTBX_Print((					"** Filter: Error notifying event**\n"));
         }
         waiting_for_data = FALSE;
     }

     /* Parse pes packet, extract pts mark from pes header,
      * check pes packet data to contatin subtitling data,
      * and determine starting of segment data in pes packet.
      */

     if( payload_unit_start==1) /* first TS */
     {
     	Start_SEG=TRUE;
     	First_PES=TRUE;
	
     	res = parse_pes_packet(DecoderHandle->FilterBuffer, &PTS,
                               &segment_data_p, &pes_payload_size);

 
     	if (res != ST_NO_ERROR)		/* bad pes */
     	{
         	STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** Filter: Bad PES\n"));
         	if (STEVT_Notify(
                   DecoderHandle->stevt_handle,
                   DecoderHandle->stevt_event_table[STSUBT_EVENT_BAD_PES
							 & 0x0000FFFF], DecoderHandle))
         	{
             	STTBX_Print((
						"** Filter: Error notifying event**\n"));
         	}
         	waiting_for_data = TRUE;
         	waiting_for_segment = TRUE;
         	continue;
     	}

     	/* --- Pack fake pts segment --- */

     /*	STTBX_Print((
				"** Filter: PTS = %.1x.%.8x\n", PTS.high, PTS.low));*/
	
     	pack_pts_segment(PTS, DecoderHandle->composition_page_id, 
                      pts_segment, &segment_size);


     	res = cBuffer_Insert(DecoderHandle->CodedDataBuffer,
                           pts_segment, segment_size);

     	STOP_FILTER_IF_REQUIRED(DecoderHandle, continue);
    
     	if (res != ST_NO_ERROR)
     	{
         	if (res == CBUFFER_ERROR_INVALID_DATA) continue;

         	STTBX_Print((
					"** Filter: error inserting data in CodedDataBuffer\n"));
         	waiting_for_data = TRUE;
         	waiting_for_segment = TRUE;
         	continue;
     	}

     }
      
     STOP_FILTER_IF_REQUIRED(DecoderHandle, continue);

     if(Start_SEG==TRUE)
     {
	 	if (pes_payload_size < 6)
  		{	
			STTBX_Print((					"pes_payload_size [%d]< 6",pes_payload_size));
			continue;
		}
 		if( segment_data_p[0] != SEGMENT_SYNC_BYTE)
  		{	
			STTBX_Print((					"segment_data_p[0] != SEGMENT_SYNC_BYTE"));
			continue;
		}

    	segment_size = ((((U16)segment_data_p[4]) << 8) + 
					((U16)segment_data_p[5]));
     }

     if(segment_size >= (DecoderHandle->pes_buffer - segment_data_p))
     {
		Start_SEG=FALSE;
		continue;
     }

     Start_SEG=TRUE;
#if 1
	 ParsedLen=0;
     do
     {
	 /* 
	 --- Parse PES payload to get segments of selected stream ---
		 */
		 /*zxg20050826 add*/
		 if(DecoderHandle->FilterStatus != STSUBT_TaskRunning)
		 {
			 break;
		 }
         /******************/
        	segment_p = get_next_segment_info(
				DecoderHandle->composition_page_id,
				DecoderHandle->ancillary_page_id,
				&segment_data_p, &pes_payload_size,
				&segment_size);
			
			/*zxg20050826 add*/
			if(DecoderHandle->FilterStatus != STSUBT_TaskRunning)
			{
				break;
			}
			/******************/
			/* receiving segment for the first time: event is raised */
			
			if (waiting_for_segment == TRUE)
			{
				if (STEVT_Notify(
					DecoderHandle->stevt_handle,
					DecoderHandle->
					stevt_event_table[STSUBT_EVENT_SEGMENT_AVAILABLE & 0x0000FFFF], 
					DecoderHandle))
				{
					STTBX_Print((
						"** Filter: Error notifying event**\n"));
				}
				waiting_for_segment = FALSE;
			}
#if 1
			
			/*ParsedLen=(DecoderHandle->pes_buffer - segment_data_p)+segment_size;*/
			
			/*zxg20050826 add*/
			if(DecoderHandle->FilterStatus != STSUBT_TaskRunning)
			{
				break;
			}
			/******************/
			
			if(First_PES==TRUE && segment_p != NULL/*&&ParsedLen<188*/)
			{
				res = cBuffer_Insert (DecoderHandle->CodedDataBuffer, 
					segment_p, segment_size);
				
				if (res != ST_NO_ERROR)
				{
					if (res == CBUFFER_ERROR_INVALID_DATA)
						break;
					STTBX_Print((
						"**Filter: error inserting data in CodedDataBuffer\n"));
					continue;
				}
			}
#endif
			if (pes_payload_size < 6)
				break;
			
			if( segment_data_p[0] != SEGMENT_SYNC_BYTE)
				break;
			
			segment_size = ((((U16)segment_data_p[4]) << 8) + ((U16)segment_data_p[5]));
			
			
			STOP_FILTER_IF_REQUIRED(DecoderHandle, break);
			
	 }while(segment_size <  (DecoderHandle->pes_buffer - segment_data_p));
#endif
 
  }  /* end while */

}
#else
ST_ErrorCode_t Filter_Body(STSUBT_InternalHandle_t * DecoderHandle)
{
   
  pts_t	PTS;
  U8    *segment_data_p;
  U16   pes_payload_size;
  segment_t *segment_p;
  U8    pts_segment[PTS_SEGMENT_LENGHT];
  U16   segment_size;
  ST_ErrorCode_t res;
  BOOL  waiting_for_data = TRUE;
  BOOL  waiting_for_segment = TRUE;
#ifndef PTI_EMULATION_MODE
#ifdef DVD_TRANSPORT_STPTI
  BOOL    payload_unit_start;
  BOOL    cc_discontinuity;
  U8  cc;
  BOOL Start_SEG;
  BOOL First_PES;
#endif
#if defined DVD_TRANSPORT_PTI 
  boolean    payload_unit_start;
  boolean    cc_discontinuity;
  unsigned int cc;
  boolean Start_SEG;
  boolean First_PES;
#endif
#endif


  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Filter: I exist ! **\n"));

  /* --- Filter will stop at first call of STOP_FILTER_IF_REQUIRED --- */

#ifndef PTI_EMULATION_MODE
  First_PES=FALSE;
#endif

  while (TRUE) 
  {
     STOP_FILTER_IF_REQUIRED(DecoderHandle, continue);

     STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Filter: waiting for a pes **\n"));

#ifndef PTI_EMULATION_MODE

     /* --- get a PES packet from pti buffer into the linear buffer  --- */

#if 0
     res = get_ts_packet (DecoderHandle,&payload_unit_start,
		&cc,&cc_discontinuity);
#else
	 {
		 
		 res=CH_GetOnepes(DecoderHandle);
		
		payload_unit_start=1;
	 }
#endif
	 continue;
     STOP_FILTER_IF_REQUIRED(DecoderHandle, continue);

     switch (res)
     {
         case ST_NO_ERROR:
			 /*do_report(0,"OK **\n");*/
             break;

         case WAIT_NO_TS:

             STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
					"** Filter: no pes data received **\n"));
              /*do_report(0,"no pes data received **\n");*/
              if (STEVT_Notify(DecoderHandle->stevt_handle,
                 DecoderHandle->stevt_event_table[STSUBT_EVENT_NO_PES_RECEIVED
					 & 0x0000FFFF],
                 DecoderHandle))
             {
                 STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
						"** Filter: Error notifying event**\n"));
             }
             waiting_for_data = TRUE;
             waiting_for_segment = TRUE;
             continue;
             break;

         case READ_ERROR:
			 /*do_report(0,"read error **\n");*/
#ifndef  IGNORE_LOST_PES_ERROR
             STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
					"** Filter: error copying pes **\n"));
             if (STEVT_Notify( DecoderHandle->stevt_handle, 
                        DecoderHandle->stevt_event_table[STSUBT_EVENT_LOST_PES
							 & 0x0000FFFF], DecoderHandle))
             {
                 STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
						"** Filter: Error notifying event**\n"));
             }
             waiting_for_data = TRUE;
             waiting_for_segment = TRUE;
#endif
             continue;
             break;

         case WAIT_ERROR:
			 /*do_report(0,"wait error **\n");*/
             STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
						"** Filter: error setting pti pointer **\n"));
             if (STEVT_Notify(
                DecoderHandle->stevt_handle, 
                DecoderHandle->stevt_event_table[STSUBT_EVENT_PES_NOT_CONSUMED 
						& 0x0000FFFF], DecoderHandle))
             {
                 STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
							"** Filter: Error notifying event**\n"));
             }

#ifdef DVD_TRANSPORT_STPTI
  			STPTI_BufferFlush(DecoderHandle->stpti_buffer);
#endif
#if defined DVD_TRANSPORT_PTI 
  			pti_flush_stream (DecoderHandle->pti_slot);
#endif
             waiting_for_data = TRUE;
             waiting_for_segment = TRUE;
             continue;
             break;

         default:
             continue;
             break;
     }

     STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Filter: got ts **\n"));

     if (waiting_for_data == TRUE)
     {
         if (STEVT_Notify( DecoderHandle->stevt_handle,
                   DecoderHandle->stevt_event_table[STSUBT_EVENT_PES_AVAILABLE
							 & 0x0000FFFF], DecoderHandle))
         {
             STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
					"** Filter: Error notifying event**\n"));
         }
         waiting_for_data = FALSE;
     }

     /* Parse pes packet, extract pts mark from pes header,
      * check pes packet data to contatin subtitling data,
      * and determine starting of segment data in pes packet.
      */

     if( payload_unit_start==1) /* first TS */
     {
     	Start_SEG=TRUE;
     	First_PES=TRUE;
	
     	res = parse_pes_packet(DecoderHandle->FilterBuffer, &PTS,
                            &segment_data_p, &pes_payload_size);
 
     	if (res != ST_NO_ERROR)		/* bad pes */
     	{
         	STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** Filter: Bad PES\n"));
         	if (STEVT_Notify(
                   DecoderHandle->stevt_handle,
                   DecoderHandle->stevt_event_table[STSUBT_EVENT_BAD_PES
							 & 0x0000FFFF], DecoderHandle))
         	{
             	STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
						"** Filter: Error notifying event**\n"));
         	}
         	waiting_for_data = TRUE;
         	waiting_for_segment = TRUE;
         	continue;
     	}

     	/* --- Pack fake pts segment --- */

     	STTBX_Report((STTBX_REPORT_LEVEL_USER1,
				"** Filter: PTS = %.1x.%.8x\n", PTS.high, PTS.low));
	
     	pack_pts_segment(PTS, DecoderHandle->composition_page_id, 
                      pts_segment, &segment_size);


     	res = cBuffer_Insert(DecoderHandle->CodedDataBuffer,
                           pts_segment, segment_size);

     	STOP_FILTER_IF_REQUIRED(DecoderHandle, continue);
    
     	if (res != ST_NO_ERROR)
     	{
         	if (res == CBUFFER_ERROR_INVALID_DATA) continue;

         	STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
					"** Filter: error inserting data in CodedDataBuffer\n"));
         	waiting_for_data = TRUE;
         	waiting_for_segment = TRUE;
         	continue;
     	}

     }
      
     STOP_FILTER_IF_REQUIRED(DecoderHandle, continue);

     if(Start_SEG==TRUE)
     {
	 	if (pes_payload_size < 6)
  		{	
			STTBX_Report((STTBX_REPORT_LEVEL_USER1,
					"pes_payload_size [%d]< 6",pes_payload_size));
			continue;
		}
 		if( segment_data_p[0] != SEGMENT_SYNC_BYTE)
  		{	
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
					"segment_data_p[0] != SEGMENT_SYNC_BYTE"));
			continue;
		}

    	segment_size = ((((U16)segment_data_p[4]) << 8) + 
					((U16)segment_data_p[5]));
     }

     if(segment_size >= (DecoderHandle->pes_buffer - segment_data_p))
     {
		Start_SEG=FALSE;
		continue;
     }

     Start_SEG=TRUE;

     do
     {
         	/* 
		    --- Parse PES payload to get segments of selected stream ---
		*/

        	segment_p = get_next_segment_info(
                              DecoderHandle->composition_page_id,
                              DecoderHandle->ancillary_page_id,
                              &segment_data_p, &pes_payload_size,
                              &segment_size);

         	/* receiving segment for the first time: event is raised */

         	if (waiting_for_segment == TRUE)
         	{
           		if (STEVT_Notify(
              		DecoderHandle->stevt_handle,
              		DecoderHandle->
                    	stevt_event_table[STSUBT_EVENT_SEGMENT_AVAILABLE & 0x0000FFFF], 
              		DecoderHandle))
           		{
              		   STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
							"** Filter: Error notifying event**\n"));
           		}
             		waiting_for_segment = FALSE;
         	}
	
		if(First_PES==TRUE && segment_p != NULL)
		{
        	res = cBuffer_Insert (DecoderHandle->CodedDataBuffer, 
                         segment_p, segment_size);

         		if (res != ST_NO_ERROR)
         		{
           			if (res == CBUFFER_ERROR_INVALID_DATA) break;
           			STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
						"**Filter: error inserting data in CodedDataBuffer\n"));
           			continue;
         		}
		}

    		if (pes_payload_size < 6)
			break;

 		if( segment_data_p[0] != SEGMENT_SYNC_BYTE)
			break;

      		segment_size = ((((U16)segment_data_p[4]) << 8) + 
					((U16)segment_data_p[5]));

	}while(segment_size <  (DecoderHandle->pes_buffer - segment_data_p));

#else  /* PTI_EMULATION_MODE defined */

     {
         U32 data_size;

         res = cBuffer_Copy (DecoderHandle->pti_buffer, &data_size,
                             DecoderHandle->FilterBuffer, 
                             FILTER_BUFFER_SIZE);

         STOP_FILTER_IF_REQUIRED(DecoderHandle, continue);

         if (res != ST_NO_ERROR)
         {
             waiting_for_data = TRUE;
             waiting_for_segment = TRUE;
             if (res == CBUFFER_ERROR_INVALID_DATA) continue;
             STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
						"** Filter: error copying pes **\n"));
             if (STEVT_Notify(
                       DecoderHandle->stevt_handle,
                       DecoderHandle->stevt_event_table[STSUBT_EVENT_LOST_PES
						 & 0x0000FFFF],
                       DecoderHandle))
             {
                 STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
							"** Filter: Error notifying event**\n"));
             }
             continue;
         } 
     }


     STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Filter: got pes **\n"));

     if (waiting_for_data == TRUE)
     {
         if (STEVT_Notify(
                   DecoderHandle->stevt_handle,
                   DecoderHandle->stevt_event_table[STSUBT_EVENT_PES_AVAILABLE
							 & 0x0000FFFF ],
                   DecoderHandle))
         {
             STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
						"** Filter: Error notifying event**\n"));
         }
         waiting_for_data = FALSE;
     }

     /* Parse pes packet, extract pts mark from pes header,
      * check pes packet data to contatin subtitling data,
      * and determine starting of segment data in pes packet.
      */

     res = parse_pes_packet(DecoderHandle->FilterBuffer, &PTS,
                            &segment_data_p, &pes_payload_size);

     
     if (res != ST_NO_ERROR)            /* bad pes */
     {
         STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** Filter: Bad PES\n"));
         if (STEVT_Notify(
                   DecoderHandle->stevt_handle,
                   DecoderHandle->stevt_event_table[STSUBT_EVENT_BAD_PES 
							& 0x0000FFFF],
                   DecoderHandle))
         {
             STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
						"** Filter: Error notifying event**\n"));
         }
         waiting_for_data = TRUE;
         waiting_for_segment = TRUE;
         continue;
     }

     /* --- Pack fake pts segment --- */

     STTBX_Report((STTBX_REPORT_LEVEL_USER1,
			"** Filter: PTS = %.1x.%.8x\n", PTS.high, PTS.low));
     pack_pts_segment(PTS, DecoderHandle->composition_page_id,
                      pts_segment, &segment_size);
     
     res = cBuffer_Insert(DecoderHandle->CodedDataBuffer, pts_segment, segment_size);
     
     STOP_FILTER_IF_REQUIRED(DecoderHandle, continue);
     if (res != ST_NO_ERROR)
     {
         if (res == CBUFFER_ERROR_INVALID_DATA) continue;

         STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
			"** Filter: error inserting data in CodedDataBuffer\n"));
         waiting_for_data = TRUE;
         waiting_for_segment = TRUE;
         continue;
     }


     /* --- Parse PES payload to get segments of selected stream --- */

     while (TRUE)
     {
         segment_p = get_next_segment_from_pes(
                              DecoderHandle->composition_page_id,
                              DecoderHandle->ancillary_page_id,
                              &segment_data_p, &pes_payload_size,
                              &segment_size);

         /* no more segments in PES */

         if (segment_p == NULL)
         {
             STTBX_Report((STTBX_REPORT_LEVEL_USER1,
					"** Filter: no more segment in pes\n"));
             break;
         }

         /* receiving segment for the first time: event is raised */

         if (waiting_for_segment == TRUE)
         {
             if (STEVT_Notify(
                 DecoderHandle->stevt_handle,
                 DecoderHandle->stevt_event_table[STSUBT_EVENT_SEGMENT_AVAILABLE 
						& 0x0000FFFF], DecoderHandle))
             {
                 STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
						"** Filter: Error notifying event**\n"));
             }
             waiting_for_segment = FALSE;
         }

         
         res = cBuffer_Insert (DecoderHandle->CodedDataBuffer, segment_p, segment_size);
         
         STOP_FILTER_IF_REQUIRED(DecoderHandle, break);
         if (res != ST_NO_ERROR)
         {
             if (res == CBUFFER_ERROR_INVALID_DATA) break;
             STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
					"**Filter: error inserting data in CodedDataBuffer\n"));
             continue;
         }
     }

#endif
 
  }  /* end while */

} /* End Filter_Body */
#endif

#endif /* #if !defined DVD_TRANSPORT_LINK  */



#if !defined PTI_EMULATION_MODE && defined DVD_TRANSPORT_LINK
/******************************************************************************\
 * Function: Filter_Body For link
 * Purpose : Filter task Body
 * Parameters:
 * Return  : 
\******************************************************************************/

ST_ErrorCode_t Filter_Body(STSUBT_InternalHandle_t * DecoderHandle)
{
  pts_t	PTS;
  U16   pes_payload_size;
  U8    pts_segment[PTS_SEGMENT_LENGHT];
  U16   segment_size;
  ST_ErrorCode_t res;
  BOOL  waiting_for_data = TRUE;
  BOOL  waiting_for_segment = TRUE;
  BOOL GotSegment = FALSE;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Filter: I exist ! **\n"));

  /* --- Filter will stop at first call of STOP_FILTER_IF_REQUIRED --- */

  while (TRUE) 
  {
      
      MoveConsumerPtrsToNextPes(DecoderHandle);      

     STOP_FILTER_IF_REQUIRED(DecoderHandle, continue);

     STTBX_Report((STTBX_REPORT_LEVEL_INFO, "** Filter: waiting for a pes **\n"));

     /* --- get a PES packet from pti buffer into the linear buffer  --- */     
     res = get_pes_packet (DecoderHandle);     

     STOP_FILTER_IF_REQUIRED(DecoderHandle, continue);

     /* Pes packet obtained ?*/
     if (res == 1)
     {
         STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                       "** Filter: no pes data received **\n"));
         if (STEVT_Notify(
             DecoderHandle->stevt_handle,
             DecoderHandle->stevt_event_table[STSUBT_EVENT_NO_PES_RECEIVED & 0x0000FFFF],
             DecoderHandle))
             {
                 STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                               "** Filter: Error notifying event**\n"));
             }
         waiting_for_data = TRUE;
         waiting_for_segment = TRUE;
         continue;
     }

     STTBX_Report((STTBX_REPORT_LEVEL_INFO, "** Filter: got pes **\n"));

     if (waiting_for_data == TRUE)
     {
         if (STEVT_Notify(
                   DecoderHandle->stevt_handle,
                   DecoderHandle->stevt_event_table[STSUBT_EVENT_PES_AVAILABLE & 0x0000FFFF],
                   DecoderHandle))
         {
             STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                        "** Filter: Error notifying event**\n"));
         }
         waiting_for_data = FALSE;
     }

     
     /* Parse pes packet, extract pts mark from pes header,
      * check pes packet data to contatin subtitling data,
      * and determine starting of segment data in pes packet.
      */
     res = ParseCircularPesPacket(DecoderHandle,
                                  &PTS,
                                  &pes_payload_size);
     
     /* bad pes */
     if (res != ST_NO_ERROR)	
     {
         STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "** Filter: Bad PES\n"));
         if (STEVT_Notify(
                   DecoderHandle->stevt_handle,
                   DecoderHandle->stevt_event_table[STSUBT_EVENT_BAD_PES & 0x0000FFFF],
                   DecoderHandle))
         {
             STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                        "** Filter: Error notifying event**\n"));
         }
         waiting_for_data = TRUE;
         waiting_for_segment = TRUE;
         continue;
     }

     
     /* --- Pack fake pts segment --- */
     STTBX_Report((STTBX_REPORT_LEVEL_INFO,  "** Filter: PTS = %.1x.%.8x\n", PTS.high, PTS.low));

     pack_pts_segment(PTS, DecoderHandle->composition_page_id, pts_segment, &segment_size);

     res = cBuffer_Insert(DecoderHandle->CodedDataBuffer, &pts_segment, segment_size);     
     if (res != ST_NO_ERROR)
     {
         if (res == CBUFFER_ERROR_INVALID_DATA)
         {
             continue;
         }

         STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                "** Filter: error inserting data in CodedDataBuffer\n"));
         waiting_for_data = TRUE;
         waiting_for_segment = TRUE;
         continue;
     }
     
     STOP_FILTER_IF_REQUIRED(DecoderHandle, continue);     

     /* --- Parse PES payload to get segments of selected stream --- */
     while (TRUE)
     {
         GotSegment = GetNextSegmentFromCircularPes(DecoderHandle,
                                                    DecoderHandle->composition_page_id, 
                                                    DecoderHandle->ancillary_page_id, 
                                                    &pes_payload_size, 
                                                    &segment_size);
         /* no more segments in PES */
         if (GotSegment == FALSE)
         {
             STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Filter: no more segment in pes\n"));
             break;
         }

         /* receiving segment for the first time: event is raised */
         if (waiting_for_segment == TRUE)
         {
             if (STEVT_Notify(
                 DecoderHandle->stevt_handle,
                 DecoderHandle->
                 stevt_event_table[STSUBT_EVENT_SEGMENT_AVAILABLE & 0x0000FFFF], 
                 DecoderHandle))                 
             {
                 STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                               "** Filter: Error notifying event**\n"));

             }
             waiting_for_segment = FALSE;
         }

         /* Copy from circular FitlerBuffer to CodedDataBuffer */
         res = cBuffer_InsertSegmentFromCircular((void*)DecoderHandle, segment_size);
         
         if (res != ST_NO_ERROR)
         {
             if (res == CBUFFER_ERROR_INVALID_DATA) break;
             STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                    "**Filter: error inserting data in CodedDataBuffer\n"));
             
             continue;
         }
         STOP_FILTER_IF_REQUIRED(DecoderHandle, break);
     } /* end while */

  }  /* end while */

} /* End Filter_Body */



/******************************************************************************\
 * Function: MoveConsumerPtrsToNextPes, only for link
 * Purpose : Wind on the pes and segment consumer pointers to the next pes
             packet in the circular filterbuffer
 * Parameters: DecoderHandle
 * Return  : None
\******************************************************************************/
static __inline
void MoveConsumerPtrsToNextPes(STSUBT_InternalHandle_t *DecoderHandle)
{
    /* Advance filter buffer consumer pointer to next pes */
     DecoderHandle->FilterBufferPesConsumer = (U8*)(&(DecoderHandle->FilterBuffer[
         ((((U32)(DecoderHandle->FilterBufferPesConsumer - DecoderHandle->FilterBuffer)) +
           DecoderHandle->PesPacketLength) % DecoderHandle->FilterBufferSize)]));

     /* Align segment consumer pointers to pes start */
     DecoderHandle->FilterBufferNextSegmentConsumer = DecoderHandle->FilterBufferPesConsumer;
     DecoderHandle->FilterBufferSegmentConsumer = DecoderHandle->FilterBufferPesConsumer;
     
}


#endif  /* !defined PTI_EMULATION_MODE && defined DVD_TRANSPORT_LINK */


/******************************************************************************\
 * Function: Filter_Create
 * Purpose : Create a filter task
 * Parameters:
 *     DecoderHandle:
 *           IN/OUT Decoder handle structure
 *           The following parameters need to be set in DecoderHandle in case
 *           the filter is started separately by decoder:
 *             pti_slot:
 *             pti_buffer:
 *             pti_buffer_size:
 *             pti_producer:
 *             pti_consumer:
 *             pti_semaphore:
 *             CodedDataBuffer:
 * Return  : ST_NO_ERROR on success
\******************************************************************************/

ST_ErrorCode_t Filter_Create (STSUBT_InternalHandle_t *DecoderHandle)
{
  STSUBT_Task_t  *FilterTask;
  ST_Partition_t *MemoryPartition = DecoderHandle->MemoryPartition;


  
#if (defined DVD_TRANSPORT_LINK) && (!defined PTI_EMULATION_MODE)
  /* create pes extraction first */
   if (ExtractPes_Create(DecoderHandle) != ST_NO_ERROR)
   {
       STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "** Filter: Error creating PES extract task**\n"));
       return (STSUBT_ERROR_ALLOCATING_MEMORY);
   }
#endif

  /* --- Allocate a linear buffer to store the PES data --- */
 
  if ((DecoderHandle->FilterBuffer = 
      (U8*)memory_allocate(MemoryPartition, FILTER_BUFFER_SIZE)) == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"Filter_Create: unable to allocate!\n"));


#if (defined DVD_TRANSPORT_LINK) && (!defined PTI_EMULATION_MODE)      
      if (ExtractPes_Delete(DecoderHandle) != ST_NO_ERROR)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "** Filter: Error deleting PES extract task**\n"));          
      }
#endif
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }
  DecoderHandle->FilterBufferSize = FILTER_BUFFER_SIZE;

  
  /* --- Allocate semaphores --- */
 
  if ((DecoderHandle->FilterTaskControl = 
       memory_allocate(MemoryPartition, sizeof(semaphore_t))) == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
		"Filter_Create: error creating semaphore FilterTaskControl!\n"));
      /* delete allocated resources */
      memory_deallocate(MemoryPartition, DecoderHandle->FilterBuffer);
#if (defined DVD_TRANSPORT_LINK) && (!defined PTI_EMULATION_MODE)            
      if (ExtractPes_Delete(DecoderHandle) != ST_NO_ERROR)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "** Filter: Error deleting PES extract task**\n"));
      }
#endif
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }
  semaphore_init_fifo(DecoderHandle->FilterTaskControl,0);

  
  if ((DecoderHandle->FilterTaskSyncSem = 
       memory_allocate(MemoryPartition, sizeof(semaphore_t))) == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
		"Filter_Create: error creating semaphore FilterTaskSyncSem!\n"));

      /* delete allocated resources */
      semaphore_delete (DecoderHandle->FilterTaskControl);
      memory_deallocate(MemoryPartition, DecoderHandle->FilterTaskControl);
      memory_deallocate(MemoryPartition, DecoderHandle->FilterBuffer);
#if (defined DVD_TRANSPORT_LINK) && (!defined PTI_EMULATION_MODE)            
      if (ExtractPes_Delete(DecoderHandle) != ST_NO_ERROR)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "** Filter: Error deleting PES extract task**\n"));
      }
#endif
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }
  semaphore_init_fifo_timeout(DecoderHandle->FilterTaskSyncSem,0);

  /* no stream is actually set */

  DecoderHandle->StreamInitialized = FALSE;

  /* Filter is created stopped */ 
  DecoderHandle->FilterStatus = STSUBT_TaskStopped;
 
  /* --- create the filter task --- */
  FilterTask = SubtTaskCreate((void (*)(void *))Filter_Body, 
                              (void *)DecoderHandle,
                              FILTER_STACK_SIZE, 
                              FILTER_TASK_PRIORITY,
                              "Filter Task", 0,
                              MemoryPartition);
  if (FilterTask == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"Filter_Create: error creating filter task!\n"));
      /* delete allocated resources */
      semaphore_delete (DecoderHandle->FilterTaskControl);      
      memory_deallocate(MemoryPartition, DecoderHandle->FilterTaskControl);
      semaphore_delete (DecoderHandle->FilterTaskSyncSem);
      memory_deallocate(MemoryPartition, DecoderHandle->FilterTaskSyncSem);
      memory_deallocate(MemoryPartition, DecoderHandle->FilterBuffer);
#if (defined DVD_TRANSPORT_LINK) && (!defined PTI_EMULATION_MODE) 
      if (ExtractPes_Delete(DecoderHandle) != ST_NO_ERROR)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "** Filter: Error deleting PES extract task**\n"));          
      }
#endif
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }

  /* wait for the filter task to signal semaphore */ 
  semaphore_wait_timeout(DecoderHandle->FilterTaskSyncSem, TIMEOUT_INFINITY);

  /* Register FilterTask to be used by creator */
  DecoderHandle->FilterTask = FilterTask;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO, "** Filter_Create: Filter created **\n"));
  
  return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: Filter_SetStream
 * Purpose : Set subtitle stream
 * Parameters:
 * Return  : ST_NO_ERROR on success, 
 *           STSUBT_ERROR_DRIVER_RUNNING on error
 * Note    : Filter must be in the Filter_Stopped status when stream is set,
 *           an error occurs if the filter is running.
\******************************************************************************/
 
ST_ErrorCode_t Filter_SetStream (U16 composition_page_id, 
                                 U16 ancillary_page_id,
                                 STSUBT_InternalHandle_t *DecoderHandle)
{
  if (DecoderHandle->FilterStatus == STSUBT_TaskRunning)
  {
      return (STSUBT_ERROR_DRIVER_RUNNING);
  }
 
  DecoderHandle->composition_page_id = composition_page_id;
  DecoderHandle->ancillary_page_id   = ancillary_page_id;
  DecoderHandle->StreamInitialized   = TRUE;
 
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Filter_SetStream: stream set **\n"));

  return (ST_NO_ERROR);
}

 
/******************************************************************************\
 * Function: Filter_Start
 * Purpose : Start the filter task
 * Parameters:
 * Return  : ST_NO_ERROR on success, 
 *           STSUBT_ERROR_DRIVER_RUNNING or
 *           STSUBT_ERROR_NO_STREAM_SET on error
\******************************************************************************/

ST_ErrorCode_t Filter_Start (STSUBT_InternalHandle_t *DecoderHandle)
{
  /* filter should not be already running */

  if (DecoderHandle->FilterStatus == STSUBT_TaskRunning)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"** Filter_Start: Error: already running **\n"));
      return (STSUBT_ERROR_DRIVER_RUNNING);
  }

  /* Stream should be already initialized */
 
  if (DecoderHandle->StreamInitialized == FALSE)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"** Filter_Start: error Stream not initialized **\n"));
      return (STSUBT_ERROR_NO_STREAM_SET);
  }

#ifndef PTI_EMULATION_MODE

  /* --- Flush the contents of the pti buffer, this is   --- */
  /* --- essential since the buffer may contain old data --- */

#ifdef DVD_TRANSPORT_STPTI
  STPTI_BufferFlush(DecoderHandle->stpti_buffer);
#endif
#if defined DVD_TRANSPORT_PTI || defined DVD_TRANSPORT_LINK
  pti_flush_stream (DecoderHandle->pti_slot);
#endif


#if (defined DVD_TRANSPORT_LINK) 
  /* start the pes extraction */
  if (ExtractPes_Start(DecoderHandle) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"** Filter_Start: Extract PES task failed to start **\n"));
      return (STSUBT_ERROR_STARTING_EXTPES);
  }
#endif /* LINK */


#endif  /* PTI_EMU */
  

  /* --- Start the Filter --- */ 
  DecoderHandle->FilterStatus = STSUBT_TaskRunning;

  semaphore_signal(DecoderHandle->FilterTaskControl);
      

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Filter_Start: filter started **\n"));

  return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: Filter_Stop
 * Purpose : Stop the filter task
 * Parameters:
 * Return  : ST_NO_ERROR on success,
 *           STSUBT_ERROR_DRIVER_NOT_RUNNING or
 *           STSUBT_ERROR_STOPPING_FILTER on error
\******************************************************************************/

ST_ErrorCode_t Filter_Stop (STSUBT_InternalHandle_t *DecoderHandle)
{
  clock_t wait_time;
  S32 res;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Filter_Stop: Stopping Filter **\n"));

  if (DecoderHandle->FilterStatus != STSUBT_TaskRunning)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"** Filter_Stop: Error: not running **\n"));
      return (STSUBT_ERROR_DRIVER_NOT_RUNNING);
  }
  

  DecoderHandle->FilterStatus = STSUBT_TaskStopped;

#if (defined DVD_TRANSPORT_LINK) && (!defined PTI_EMULATION_MODE)   
  /* stop the pes extraction */
  if (ExtractPes_Stop(DecoderHandle) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"** Filter_Stop: Error top PES extraction **\n"));      
      return (STSUBT_ERROR_STOPPING_EXTPES);
  }

#endif


#ifndef PTI_EMULATION_MODE

  /* Force the filter task out of the pti semaphore queue 
   * in case it is suspended there
   */

#if defined DVD_TRANSPORT_PTI || defined DVD_TRANSPORT_LINK
  semaphore_signal(DecoderHandle->pti_semaphore);
#endif

#else

  /* Invalidate data in pti buffer, and force the filter task
   * out of the semaphore queue in case it is suspended there
   */
 
  cBuffer_FlushOutConsumer(DecoderHandle->pti_buffer);

#endif

  /* Invalidate data in Coded Data Buffer, and force the filter task
   * out of the semaphore queue in case it is suspended there
   */

  cBuffer_FlushOutProducer(DecoderHandle->CodedDataBuffer);

  /* wait for the filter task to stop */

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
				"** Filter_Stop: waiting for filter to stop **\n"));

  wait_time = time_plus(time_now(), DELAY_10_SEC); /* akem: was 5secs */
  res = semaphore_wait_timeout(DecoderHandle->FilterTaskSyncSem, &wait_time); 
  if (res == - 1)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"** Filter_Stop: Filter Not Stopped **\n"));
      return (STSUBT_ERROR_STOPPING_FILTER);
  }

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** Filter_Stop: Stopped **\n"));

  return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: Filter_Delete
 * Purpose : Delete a filter task
 * Parameters:
 * Return  : ST_NO_ERROR on success, else ...
 *           STSUBT_ERROR_DRIVER_RUNNING,
 *           STSUBT_ERROR_DELETING_FILTER
\******************************************************************************/
 
ST_ErrorCode_t Filter_Delete (STSUBT_InternalHandle_t *DecoderHandle)
{
  clock_t wait_time;
  S32 res;

  if (DecoderHandle->FilterStatus == STSUBT_TaskRunning)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"Filter_Delete: Error Task is still running\n"));
      return (STSUBT_ERROR_DRIVER_RUNNING);
  }
 
  /* Force filter task to suicide 
   * Filter is temporary made running signalling the semaphore FilterTaskControl
   * in order to allow it to procede suiciding itself.
   */
 
  DecoderHandle->FilterStatus = STSUBT_TaskSuicide;
 
  semaphore_signal(DecoderHandle->FilterTaskControl);                

  /* wait for the filter task to suicide */
 
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
		"** Filter_Delete: waiting for filter to suicide **\n"));

  wait_time = time_plus(time_now(), FIVE_SECONDS);
  res = semaphore_wait_timeout(DecoderHandle->FilterTaskSyncSem, &wait_time); 
  if (res == - 1)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"** Filter_Delete: task not suicided.\n"));
      return (STSUBT_ERROR_DELETING_FILTER);
  }
 
  /* delete task (always succeeds since task is already terminated) */

  SubtTaskDelete(DecoderHandle->FilterTask); 

  /* delete allocated resources */
  semaphore_delete (DecoderHandle->FilterTaskControl);
  memory_deallocate(DecoderHandle->MemoryPartition,
                    DecoderHandle->FilterTaskControl);
  semaphore_delete (DecoderHandle->FilterTaskSyncSem);  
  memory_deallocate(DecoderHandle->MemoryPartition,
                    DecoderHandle->FilterTaskSyncSem);
  memory_deallocate(DecoderHandle->MemoryPartition,
                    DecoderHandle->FilterBuffer);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
			"** Filter_Delete: Suicided **\n"));

#if (defined DVD_TRANSPORT_LINK) && (!defined PTI_EMULATION_MODE)   
  /* kill PES extraction task */
  if (ExtractPes_Delete(DecoderHandle) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"** Filter_Delete: ExtractPes task not suicided.\n"));
      return (STSUBT_ERROR_DELETING_EXTPES);      

  }
#endif


  return (ST_NO_ERROR);
}

#if !defined (PTI_EMULATION_MODE) && defined DVD_TRANSPORT_LINK 
/***************************************************************
Name :  FindNextStartCode
Description : Find next start code in memory (CPU search). Assumes
              it's working on a circular buffer.
Parameters:
 * EndAddress can not be 0
 * returns 0 if no start code found, 1 otherwise
 * returns the exact byte address of the first byte of the start code
 * EndAddress - StarAddress must be at least 1
*****************************************************************/
static __inline int FindNextStartCode(U32 *StartAddress,
                                      U32 ByteCount,
                                      U32 *BufferBase,
                                      U32 BufferSize,
                                      U8 **StartCodeAddr,
                                      U8 **NextConsumerPtr,
                                      U32 *Index)
              
{

    U32 WrapRemaining, BytesRemaining;
    U32 WrapStore, ByteStore;
    register U8 *Ptr;
    U8 CompareArray[] = { 0x00, 0x00, 0x01, 0xBD }; 
    register U8 *ComparePtr = CompareArray;
    U8 *StartCodePtr = NULL;

    /* Setup */
    *NextConsumerPtr = (U8 *)StartAddress;
    BytesRemaining = ByteCount;
    WrapRemaining = BufferSize - (U32)((U32)StartAddress - (U32)BufferBase);


    for (Ptr = (U8 *)StartAddress; BytesRemaining > 0; WrapRemaining--,
                                                       BytesRemaining--)
    {
        if (*Ptr == *ComparePtr)
        {
            /* Keep track of where the possible start code begins */
            if (StartCodePtr == NULL)
            {
                StartCodePtr = Ptr;
                WrapStore = WrapRemaining;
                ByteStore = BytesRemaining;
            }
            ComparePtr++;
            if (ComparePtr == &CompareArray[4])
                break;
        }
        else
        {
            ComparePtr = CompareArray;
            /* Have we started matching...? */
            if (StartCodePtr != NULL)
            {
                /* Yes, so reset back to the start of it (incremented
                 * below so we don't keep looping). We don't just stay
                 * at the current pos. for reasons detailed in GNBvd13784.
                 */
                Ptr = StartCodePtr;
                StartCodePtr = NULL;
                WrapRemaining = WrapStore;
                BytesRemaining = ByteStore;
            }
        }

        Ptr++;
        if (WrapRemaining == 1)
            Ptr = (U8 *)BufferBase;

        /* Ensure the consumer always stays at least 6 bytes back from
         * the end.
         */
        if (BytesRemaining >= 6)
            (*NextConsumerPtr) = Ptr;
    }
    
    /* We cannot return 'true' if the caller doesn't have the
     * length-bytes yet. Since the NextConsumerPtr stopped being updated
     * some time back (if we're this short of data), we just don't
     * update. We'll find it next time we're called.
     */
    if (StartCodePtr != NULL && BytesRemaining >= 2)
    {
        *StartCodeAddr = StartCodePtr;
        *NextConsumerPtr = StartCodePtr;
        
        /* Return number of bytes searched upto start of start code */
        *Index = ((ByteCount - BytesRemaining) - 3);
        return 1;
    }

    return 0;
    
}

#endif

#if 1/*zxg add*/
ST_ErrorCode_t CH_GetOnepes(STSUBT_InternalHandle_t *DecoderHandle)
{
	ST_ErrorCode_t ErrorCode;
	

	STPTI_Buffer_t SignaledBuffer;
	U8 PesFlags, TrickMode;
	STPTI_TimeStamp_t PTS, DTS;
	U32 CurrentPacketSize = 0;
	U32 DataSize;
    int NumberSec=0;
	
	/*STTBX_Print(("** Filter: waiting for ts data **\n"));*/
	
   do
   {
   	ErrorCode=STPTI_SignalWaitBuffer(DecoderHandle->stpti_signal,
				&SignaledBuffer,DELAY_1_SEC );

	if(ErrorCode!=ST_NO_ERROR && ErrorCode!=ST_ERROR_TIMEOUT)
	{
  		/*STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
				"STPTI_SignalWaitBuffer 0x%x\n",ErrorCode));*/
		  STTBX_Print(("STPTI_SignalWaitBuffer Error\n"));
		return (WAIT_ERROR);
	}

	NumberSec++;

   } while(NumberSec < SEC_TO_WAIT_TS && ErrorCode==ST_ERROR_TIMEOUT &&
	DecoderHandle->FilterStatus == STSUBT_TaskRunning);
						
	if(ErrorCode != ST_NO_ERROR ||DecoderHandle->FilterStatus != STSUBT_TaskRunning)
	{
		/*STTBX_Print(("STPTI WAIT_NO_TS\n"));*/
		return (WAIT_NO_TS);
	}
	
	ErrorCode = STPTI_BufferExtractPesPacketData(DecoderHandle->stpti_buffer, &PesFlags,
		&TrickMode,
		&CurrentPacketSize, &PTS, &DTS);

	if (ErrorCode == ST_NO_ERROR)
	{
		CurrentPacketSize += 6;		   
		if (CurrentPacketSize < 8192&&CurrentPacketSize>6)
		{
			/*得到一个PES完整包*/
			DecoderHandle->pes_buffer=DecoderHandle->FilterBuffer;
			
			ErrorCode = STPTI_BufferReadPes(DecoderHandle->stpti_buffer,
				DecoderHandle->pes_buffer, 8192, 
				NULL, 0, &DataSize,
				STPTI_COPY_TRANSFER_BY_MEMCPY);	
			
			/*STTBX_Print(("Get date size: %d\n", DataSize));*/
		}	
		else
		{
			STTBX_Print(("Erro too large or small CurrentPacketSize=%d\n",CurrentPacketSize));
			STPTI_BufferFlush(DecoderHandle->stpti_buffer);
			return (READ_ERROR);
		}
	}
	else
	{
		STTBX_Print(("Erro BufferExtractPesPacketData: %s\n", GetErrorText(ErrorCode)));
		STPTI_BufferFlush(DecoderHandle->stpti_buffer);
		return (READ_ERROR);
	}

	DecoderHandle->pes_buffer+= DataSize;

	/******************/
	return (ST_NO_ERROR);		
}
#endif

