/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
/* 
Modifications of this software Copyright(C) 2008, Standard Microsystems Corporation
  All Rights Reserved.
  The modifications to this program code are proprietary to SMSC and may not be copied,
  distributed, or used without a license to do so.  Such license may have
  Limited or Restricted Rights. Please refer to the license for further
  clarification.
*/
/*
FILE: packet_manager.c
*/

#include "packet_manager.h"
#include "memory_pool.h"
#include "smsc_threading.h"

u8_t gSmallBufferMemory[
	MEMORY_POOL_SIZE(
		sizeof(PACKET_BUFFER)+SMALL_BUFFER_SIZE,
		SMALL_BUFFER_COUNT)];
MEMORY_POOL_HANDLE gSmallBufferPool=NULL;
u8_t gLargeBufferMemory[
	MEMORY_POOL_SIZE(
		sizeof(PACKET_BUFFER)+LARGE_BUFFER_SIZE,
		LARGE_BUFFER_COUNT)];
MEMORY_POOL_HANDLE gLargeBufferPool=NULL;

static SMSC_SEMAPHORE pPacketQueueLock = NULL;

static void SmallBufferInitializer(void * element)
{
	PPACKET_BUFFER packetBuffer=(PPACKET_BUFFER)element;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packetBuffer!=NULL);
	/* make sure we are memory aligned */
	SMSC_ASSERT((((mem_ptr_t)packetBuffer)&(MEMORY_ALIGNMENT-1))==0);
	memset(packetBuffer,0,sizeof(PACKET_BUFFER));
	ASSIGN_SIGNATURE(packetBuffer,PACKET_BUFFER_SIGNATURE);
	packetBuffer->mAllocatedLength=SMALL_BUFFER_SIZE;
}
static void LargeBufferInitializer(void * element)
{
	PPACKET_BUFFER packetBuffer=(PPACKET_BUFFER)element;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packetBuffer!=NULL);
	/* make sure we are memory aligned */
	SMSC_ASSERT((((mem_ptr_t)packetBuffer)&(MEMORY_ALIGNMENT-1))==0);
	memset(packetBuffer,0,sizeof(PACKET_BUFFER));
	ASSIGN_SIGNATURE(packetBuffer,PACKET_BUFFER_SIGNATURE);
	packetBuffer->mAllocatedLength=LARGE_BUFFER_SIZE;
}

void PacketManager_Initialize(void)
{
	SMSC_ASSERT((sizeof(PACKET_BUFFER)&(MEMORY_ALIGNMENT-1))==0);

	gSmallBufferPool=MemoryPool_Initialize(
		gSmallBufferMemory,
		sizeof(PACKET_BUFFER)+SMALL_BUFFER_SIZE,
		SMALL_BUFFER_COUNT,SmallBufferInitializer);
	gLargeBufferPool=MemoryPool_Initialize(
		gLargeBufferMemory,
		sizeof(PACKET_BUFFER)+LARGE_BUFFER_SIZE,
		LARGE_BUFFER_COUNT,LargeBufferInitializer);
	pPacketQueueLock = smsc_semaphore_create(1);
}

#if SMSC_ERROR_ENABLED
int PacketBuffer_SanityCheck(PPACKET_BUFFER packetBuffer)
{
	PPACKET_BUFFER buffer;
	smsc_semaphore_wait(pPacketQueueLock);
	for(buffer=packetBuffer;buffer!=NULL;buffer=buffer->mNext) {
		u32_t nextLength=0;
		SMSC_ASSERT(buffer!=NULL);
		CHECK_SIGNATURE(buffer,PACKET_BUFFER_SIGNATURE);
		if((buffer->mNext!=NULL)&&(buffer->mThisLength!=buffer->mTotalLength)) 
		{
			nextLength=buffer->mNext->mTotalLength;
		}
		SMSC_ASSERT(buffer->mTotalLength==(buffer->mThisLength+nextLength));
		SMSC_ASSERT(buffer->mAllocatedLength>=(buffer->mStartOffset+buffer->mThisLength));
		SMSC_ASSERT(buffer->mAllocatedLength>0);
	}
	smsc_semaphore_signal(pPacketQueueLock);
	return 1;
}
#endif

PPACKET_BUFFER PacketBuffer_Copy(PPACKET_BUFFER packetBuffer)
{
	PPACKET_BUFFER result=NULL;
	PPACKET_BUFFER lastNewBuffer=NULL;
	PPACKET_BUFFER thisBuffer=packetBuffer;
	while(thisBuffer!=NULL) {
		u16_t length;
		CHECK_SIGNATURE(thisBuffer,PACKET_BUFFER_SIGNATURE);
		length=thisBuffer->mStartOffset+thisBuffer->mThisLength;
		if(result==NULL) {
			SMSC_ASSERT(lastNewBuffer==NULL);
			lastNewBuffer=result=PacketBuffer_Create(length);
		} else {
			SMSC_ASSERT(lastNewBuffer!=NULL);
			SMSC_ASSERT(lastNewBuffer->mNext==NULL);
			lastNewBuffer->mNext=PacketBuffer_Create(length);
			lastNewBuffer=lastNewBuffer->mNext;
		}
		if(lastNewBuffer==NULL) {
			goto NOT_ENOUGH_MEMORY;
		}
		CHECK_SIGNATURE(lastNewBuffer,PACKET_BUFFER_SIGNATURE);
		if(PacketBuffer_MoveStartPoint(lastNewBuffer,thisBuffer->mStartOffset)!=ERR_OK) {
			/* should never happen because length included start offset */
			SMSC_ERROR(("PacketBuffer_Copy: unable to move start point"));
		}
		SMSC_ASSERT(lastNewBuffer->mThisLength==thisBuffer->mThisLength);
		lastNewBuffer->mTotalLength=thisBuffer->mTotalLength;
		memcpy(PacketBuffer_GetStartPoint(lastNewBuffer),
			PacketBuffer_GetStartPoint(thisBuffer),
			thisBuffer->mThisLength);
		thisBuffer=thisBuffer->mNext;
	}
	SMSC_ASSERT(PacketBuffer_SanityCheck(result));
	return result;
NOT_ENOUGH_MEMORY:
	if(result!=NULL) {
		PacketBuffer_DecreaseReference(result);
	}
	return NULL;
}

PPACKET_BUFFER PacketBuffer_Create(
	u16_t length)
{
	PPACKET_BUFFER result=NULL;
	smsc_semaphore_wait(pPacketQueueLock);
	if(length<=SMALL_BUFFER_SIZE) {
		result=(PPACKET_BUFFER)
			MemoryPool_Allocate(gSmallBufferPool);
	} else if(length<=LARGE_BUFFER_SIZE) {
		result=(PPACKET_BUFFER)
			MemoryPool_Allocate(gLargeBufferPool);
	} else {
		SMSC_ERROR(("PacketBuffer_Create: unhandled length = %"U16_F,length));
	}
	if(result!=NULL) {
		CHECK_SIGNATURE(result,PACKET_BUFFER_SIGNATURE);
		SMSC_ASSERT(result->mNext==NULL);
		result->mStartOffset=0;
		result->mThisLength=length;
		result->mTotalLength=length;
		result->mReferenceCount=1;
	}
	else
	{
            SMSC_ERROR(("PacketBuffer_Create Failed\n "));
	}
	 SMSC_TRACE(PACKET_MANAGER_DEBUG,("PacketBuffer_Create success[%p]\n ",result));
	smsc_semaphore_signal(pPacketQueueLock);
	return result;
}

#if !INLINE_SIMPLE_FUNCTIONS
u8_t * PacketBuffer_GetStartPoint(PPACKET_BUFFER packetBuffer)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(packetBuffer!=NULL,NULL);
	CHECK_SIGNATURE(packetBuffer,PACKET_BUFFER_SIGNATURE);
	return (&(((u8_t *)(((mem_ptr_t)packetBuffer)+
		sizeof(PACKET_BUFFER)))[packetBuffer->mStartOffset]));
}                
int PacketQueue_IsEmpty(PPACKET_QUEUE packetQueue)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(packetQueue!=NULL,ERR_ARG);
	CHECK_SIGNATURE(packetQueue,PACKET_QUEUE_SIGNATURE);
	return (packetQueue->mHead==NULL);
}
PPACKET_BUFFER PacketQueue_Peek(PPACKET_QUEUE packetQueue)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(packetQueue!=NULL,NULL);
	CHECK_SIGNATURE(packetQueue,PACKET_QUEUE_SIGNATURE);
	return (packetQueue->mHead);
}
#endif /*!INLINE_SIMPLE_FUNCTIONS*/

err_t PacketBuffer_RestoreStartPoint(PPACKET_BUFFER packetBuffer,u8_t * startPoint)
{
	s16_t offset=(s16_t)(startPoint-PacketBuffer_GetStartPoint(packetBuffer));
	return PacketBuffer_MoveStartPoint(packetBuffer,offset);
}

void PacketBuffer_ReduceTotalLength(PPACKET_BUFFER packetBuffer,u32_t newLength)
{
	PPACKET_BUFFER buffer;
	u32_t remainingLength;
	s32_t grow;

       if(packetBuffer== NULL)
       	{
                 return;
       	}
	/* desired length larger than current length? */
	if (newLength >= packetBuffer->mTotalLength) {
		/* can not increase length */
		SMSC_ASSERT(newLength==packetBuffer->mTotalLength);
		return;
	}

	smsc_semaphore_wait(pPacketQueueLock);
	/* the packet buffer chain grows by (newLength - packetBuffer->mTotalLength) bytes
	* (which may be negative in case of shrinking) */
	grow = newLength - packetBuffer->mTotalLength;

	/* first, step over any pbufs that should remain in the chain */
	remainingLength = newLength;
	buffer = packetBuffer;
	/* should this pbuf be kept? */
	while (buffer != NULL && remainingLength > buffer->mThisLength) {
		/* decrease remaining length by pbuf length */
		remainingLength -= buffer->mThisLength;
		/* decrease total length indicator */
		buffer->mTotalLength += grow;
		/* proceed to next pbuf in chain */
		buffer = buffer->mNext;
	}
	smsc_semaphore_signal(pPacketQueueLock);
	/* we have now reached the new last packet buffer (in buffer) */
	/* remainingLength == desired length for buffer */
        
	/* make sure the following typecast is acceptable */
	SMSC_ASSERT(!(remainingLength&0xFFFF0000));
        if(buffer != NULL)
        {
	/* adjust length fields for new last pbuf */
	buffer->mThisLength = ((u16_t)remainingLength);
	buffer->mTotalLength = buffer->mThisLength;

	if (buffer->mNext != NULL) {
		/* free remaining buffers in chain */
		PacketBuffer_DecreaseReference(buffer->mNext);
	}
	/* buffer is last packet in chain */
	buffer->mNext = NULL;
        }
}
/**
 * Concatenate two PACKET_BUFFERs (each may be a PACKET_BUFFER chain) and take over
 * the caller's reference of the tail PACKET_BUFFER.
 * 
 * @see PacketBuffer_JoinChains()
 */
void PacketBuffer_JoinChains(PPACKET_BUFFER headChain, PPACKET_BUFFER tailChain)
{
	PPACKET_BUFFER buffer;

	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(headChain != NULL);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(tailChain != NULL);
	if ((headChain == NULL) || (tailChain == NULL)) return;

	smsc_semaphore_wait(pPacketQueueLock);
	/* proceed to last packet buffer of chain */
	for (buffer = headChain; buffer->mNext != NULL; buffer = buffer->mNext) {
		/* add total length of second chain to all totals of first chain */
		buffer->mTotalLength += tailChain->mTotalLength;
		SMSC_NOTICE(TCP_DEBUG,("1BUFF[%p]->mNext[%p]",buffer,buffer->mNext));
	}
	/* { buffer is last packet buffer of first chain (headChain), buffer->mNext == NULL } */
	SMSC_ASSERT(buffer->mTotalLength == buffer->mThisLength);
	SMSC_ASSERT(buffer->mNext == NULL);
	/* add total length of second chain to last packet buffer total of first chain */
	buffer->mTotalLength += tailChain->mTotalLength;
	/* chain last packet buffer of head (buffer) with first of tail (tailChain) */
	buffer->mNext = tailChain;
	/* buffer->mNext now references tailChain, but the caller will drop its reference to tailChain,
	* so netto there is no change to the reference count of tailChain.
	*/	
	smsc_semaphore_signal(pPacketQueueLock);
	SMSC_NOTICE(TCP_DEBUG,("2BUFF[%p]->mNext[%p]",buffer,buffer->mNext));
}

void PacketBuffer_CopyFromExternalBuffer(PPACKET_BUFFER packet,u32_t offset, u8_t * data,u32_t length)
{
	u32_t remainingLength=length;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(data!=NULL);
	smsc_semaphore_wait(pPacketQueueLock);
	while((remainingLength>0)&&(packet!=NULL)) 
	{
		CHECK_SIGNATURE(packet,PACKET_BUFFER_SIGNATURE);
		SMSC_ASSERT((remainingLength+offset)<=packet->mTotalLength);
		if(offset>=packet->mThisLength) {
			offset-=packet->mThisLength;
		} else {
			u16_t thisLength=packet->mThisLength;
			memcpy(&(PacketBuffer_GetStartPoint(packet)[offset]),data,thisLength);
			data+=thisLength;
			remainingLength-=thisLength;
			offset=0;
		}
		packet=packet->mNext;
	}
	smsc_semaphore_signal(pPacketQueueLock);
	SMSC_ASSERT(remainingLength==0);
}

void PacketBuffer_CopyToExternalBuffer(PPACKET_BUFFER packet,u32_t offset,u8_t * data, u32_t length)
{
	u32_t remainingLength=length;
	smsc_semaphore_wait(pPacketQueueLock);
	while((remainingLength>0)&&(packet!=NULL)) 
	{
		CHECK_SIGNATURE(packet,PACKET_BUFFER_SIGNATURE);
		SMSC_ASSERT((remainingLength+offset)<=packet->mTotalLength);
		if(offset>=packet->mThisLength) {
			offset-=packet->mThisLength;
		} else {
			u16_t thisLength=packet->mThisLength;
			memcpy(data,&(PacketBuffer_GetStartPoint(packet)[offset]),thisLength);
			data+=thisLength;
			remainingLength-=thisLength;
			offset=0;
		}
		packet=packet->mNext;
	}
	smsc_semaphore_signal(pPacketQueueLock);
	SMSC_ASSERT(remainingLength==0);
}

err_t PacketBuffer_MoveStartPoint(
	PPACKET_BUFFER packetBuffer, s16_t offset)
{
	/* NOTE: If packetBuffer is a buffer chain
		then packetBuffer must be the first buffer in the chain.
		There is no way to check this in the code.
		Programmer must be careful.
		For example: If you have the following buffer list
			buffer1->buffer2->buffer3
			If you move the start point of buffer2
				then the mTotalLength of buffer1 
				will be in error as it is not updated 
	*/
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(packetBuffer!=NULL,NULL);
	CHECK_SIGNATURE(packetBuffer,PACKET_BUFFER_SIGNATURE);
	if(offset>0) {
		if(((u16_t)offset)>packetBuffer->mThisLength) 
			return ERR_VAL;
	} else {
		if(((u16_t)(-offset))>packetBuffer->mStartOffset)
			return ERR_VAL;
	}
	packetBuffer->mStartOffset+=offset;
	packetBuffer->mThisLength-=offset;
	packetBuffer->mTotalLength-=offset;
	return ERR_OK;
}
              
err_t PacketBuffer_MoveEndPoint(
	PPACKET_BUFFER packetBuffer,s16_t offset)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(packetBuffer!=NULL,ERR_ARG);
	CHECK_SIGNATURE(packetBuffer,PACKET_BUFFER_SIGNATURE);
	
	/* This packet must be the last packet in a buffer chain */
	SMSC_ASSERT(packetBuffer->mTotalLength==packetBuffer->mThisLength);
	
	if(offset>0) {
		if((packetBuffer->mThisLength+((u16_t)offset)+packetBuffer->mStartOffset)>packetBuffer->mAllocatedLength)
			return ERR_VAL;
	} else {
		if(packetBuffer->mThisLength<((u16_t)(-offset)))
			return ERR_VAL;
	}
	packetBuffer->mThisLength+=offset;
	packetBuffer->mTotalLength+=offset;
	return ERR_OK;
}
void PacketQueue_Initialize(PPACKET_QUEUE packetQueue)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packetQueue!=NULL);
	ASSIGN_SIGNATURE(packetQueue,PACKET_QUEUE_SIGNATURE);
	packetQueue->mHead=NULL;
	packetQueue->mTail=NULL;
}

void PacketQueue_Push(PPACKET_QUEUE packetQueue,PPACKET_BUFFER packet)
{
	PPACKET_BUFFER temp_packet = NULL;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packetQueue!=NULL);
	CHECK_SIGNATURE(packetQueue,PACKET_QUEUE_SIGNATURE);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packet!=NULL);
	SMSC_ASSERT(packet->mNext==NULL);/* ensure this is not a multifragment packet */
	/* TODO this function needs to be updated to support multifragment packets */
	/* TODO also update PacketQueue_Pop at the same time */
	/* TODO also update PacketQueue_UnPop at the same time */
	//smsc_semaphore_wait(pPacketQueueLock);
	for(temp_packet= packetQueue->mHead;temp_packet!= NULL;temp_packet = temp_packet->mNext){
		if(temp_packet == packet){
			SMSC_NOTICE(PACKET_MANAGER_DEBUG,("packet[%p] have been in queque[%p]",packet,packetQueue));
			//smsc_semaphore_signal(pPacketQueueLock);
			return;
		}
	} 
	
	if(packet->mNext != NULL){
		SMSC_NOTICE(1,("PacketQueue_Push:packetQueue[%p],packet[%p],packet->mNext[%p]",packetQueue,packet,packet->mNext));
	}
	if(packetQueue->mHead!=NULL)
	{
		SMSC_ASSERT(packetQueue->mTail!=NULL);
		SMSC_ASSERT(packetQueue->mTail->mNext==NULL);
		if(packetQueue->mTail->mNext != NULL){
			SMSC_NOTICE(1,("PacketQueue_Push:packetQueue[%p],packet[%p],packet->mTail[%p],packetQueue->mTail->mNext[%p]",packetQueue,packet,packetQueue->mTail,packetQueue->mTail->mNext));
		}
		packetQueue->mTail->mNext=packet;
	}
	else 
	{
		SMSC_ASSERT(packetQueue->mTail==NULL);
		packetQueue->mHead=packet;
	}
	packetQueue->mTail=packet;
	//smsc_semaphore_signal(pPacketQueueLock);
}

void PacketQueue_UnPop(PPACKET_QUEUE packetQueue,PPACKET_BUFFER packet)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packetQueue!=NULL);
	CHECK_SIGNATURE(packetQueue,PACKET_QUEUE_SIGNATURE);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packet!=NULL);
	SMSC_ASSERT(packet->mNext==NULL);/* ensure this is not a multifragment packet */
	/* TODO this function needs to be updated to support multifragment packets */
	//smsc_semaphore_wait(pPacketQueueLock);
	if(packetQueue->mHead!=NULL) {
		SMSC_ASSERT(packetQueue->mTail!=NULL);
		packet->mNext=packetQueue->mHead;
		packetQueue->mHead=packet;
	} else {
		SMSC_ASSERT(packetQueue->mTail==NULL);
		packetQueue->mHead=packet;
		packetQueue->mTail=packet;
	}
	//smsc_semaphore_signal(pPacketQueueLock);
}

PPACKET_BUFFER PacketQueue_Pop(PPACKET_QUEUE packetQueue)
{
	PPACKET_BUFFER result=NULL;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(packetQueue!=NULL,NULL);
	CHECK_SIGNATURE(packetQueue,PACKET_QUEUE_SIGNATURE);
	//smsc_semaphore_wait(pPacketQueueLock);
	if(packetQueue->mHead!=NULL) {
		SMSC_ASSERT(packetQueue->mTail!=NULL);
		result=packetQueue->mHead;
		packetQueue->mHead=result->mNext;
		if(packetQueue->mHead==NULL) {
			packetQueue->mTail=NULL;
		}
		result->mNext=NULL;
	} else {
		SMSC_ASSERT(packetQueue->mTail==NULL);
	}
	//smsc_semaphore_signal(pPacketQueueLock);
	return result;
}

void PacketBuffer_DecreaseReference(PPACKET_BUFFER packetBuffer)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packetBuffer!=NULL);
	CHECK_SIGNATURE(packetBuffer,PACKET_BUFFER_SIGNATURE);
	if(packetBuffer->mReferenceCount<=0) 
	{
		SMSC_ASSERT(packetBuffer->mReferenceCount>0);
		return ;
	}
	packetBuffer->mReferenceCount--;
	if(packetBuffer->mReferenceCount==0) {
		if(packetBuffer->mNext!=NULL) {
			PacketBuffer_DecreaseReference(packetBuffer->mNext);
			packetBuffer->mNext=NULL;
		}
		if(packetBuffer->mAllocatedLength==SMALL_BUFFER_SIZE) {
			MemoryPool_Free(gSmallBufferPool,packetBuffer);
		} else if(packetBuffer->mAllocatedLength==LARGE_BUFFER_SIZE) {
			MemoryPool_Free(gLargeBufferPool,packetBuffer);
		} else {
			SMSC_ERROR(("Unexpected allocation length=%"U16_F,
				packetBuffer->mAllocatedLength));
		}
	}
	SMSC_TRACE(PACKET_MANAGER_DEBUG,("PacketBuffer_DecreaseReference[%p]\n",packetBuffer));
}
void PacketBuffer_IncreaseReference(PPACKET_BUFFER packetBuffer)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packetBuffer!=NULL);
	CHECK_SIGNATURE(packetBuffer,PACKET_BUFFER_SIGNATURE);
	SMSC_ASSERT(packetBuffer->mReferenceCount>0);
	packetBuffer->mReferenceCount++;
}

void PacketBuffer_Dump(int enabled, PPACKET_BUFFER packet)
{
	if(packet!=NULL) {
		int packetIndex=0;
		while(packet!=NULL) {
			int bufferIndex=0;
			SMSC_TRACE(enabled,("Packet Index=%d, total_length=%"U32_F,packetIndex,packet->mTotalLength));
			while(packet!=NULL) {
				int byteIndex=0;
				mem_ptr_t byteAddress=(mem_ptr_t)PacketBuffer_GetStartPoint(packet);
				int offset=(int)(byteAddress&((mem_ptr_t)0x0F));
				int firstLine=1;
				byteAddress&=~((mem_ptr_t)(0x0F));
				SMSC_TRACE(enabled,("  Buffer index=%d, buffer_length=%"U16_F,bufferIndex,packet->mThisLength));
				for(byteIndex=-offset;byteIndex<packet->mThisLength;byteIndex++,byteAddress++) {
					if((byteAddress&((mem_ptr_t)0x0F))==0) {
						if(!firstLine) {
							SMSC_PRINT(enabled,("\n"));
						}
						firstLine=0;
						SMSC_PRINT(enabled,("  TRACE:     %p  ",(void *)byteAddress));
					}
					if((byteIndex>=0)&&(byteIndex<packet->mThisLength)) {
						SMSC_PRINT(enabled,("%02"X16_F" ",PacketBuffer_GetStartPoint(packet)[byteIndex]));
					} else {
						SMSC_PRINT(enabled,("   "));
					}
				}
				SMSC_PRINT(enabled,("\n"));
				bufferIndex++;
				if(packet->mTotalLength!=packet->mThisLength) {
					packet=packet->mNext;
				} else {
					break;
				}
			}
			packetIndex++;
			packet=packet->mNext;
		}
	} else {
		SMSC_TRACE(enabled,("packet==NULL"));
	}
}
