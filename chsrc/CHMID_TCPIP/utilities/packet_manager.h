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
/***************************************************************************** 
* Modifications of this software Copyright(C) 2008, Standard Microsystems Corporation
*  All Rights Reserved.
*  The modifications to this program code are proprietary to SMSC and may not be copied,
*  distributed, or used without a license to do so.  Such license may have
*  Limited or Restricted Rights. Please refer to the license for further
*  clarification.
******************************************************************************
* FILE: packet_manager.h
* PURPOSE:
*    This file declares functions for managing packet buffers.
*****************************************************************************/

#ifndef PACKET_MANAGER_H
#define PACKET_MANAGER_H

#include "smsc_environment.h"

#if SMSC_ERROR_ENABLED
#define PACKET_BUFFER_SIGNATURE	(0x63FCF752)
#endif

/*****************************************************************************
* STRUCTURE: PACKET_BUFFER
* DESCRIPTION: This structure maintains context information about each
*    packet buffer. The actual buffer region does not require a pointer
*    because the buffer region is expected to follow immediately after this
*    structure in memory. Refer to PacketBuffer_GetStartPoint to see how
*    a buffer pointer is obtained. With in the actual buffer there is a 
*    concept of an active region. The active region can be changed by calls to
*    PacketBuffer_MoveStartPoint, and PacketBuffer_MoveEndPoint.
*****************************************************************************/
typedef struct PACKET_BUFFER_ {
	DECLARE_SIGNATURE	/* used for run time type checking and memory corruption detection */
	struct PACKET_BUFFER_ * mNext; 
		/* points to the next packet buffer in a packet buffer list 
		   or points to the next packet buffer in a packet list.
		   
		   A packet buffer list is a list of packet buffers that
		   are concidered as one packet. The end of a packet buffer list
		   is detected by this condition (mThisLength==mTotalLength)
		   
		   A packet list is a list of packet buffer lists. The end
		   of a packet list is detected by this condition(mNext==NULL)
		*/

	u16_t mAllocatedLength;
		/* specifies the allocated length of the buffer region. */
		/* a packet buffer can not expand beyond this allocated length */

	u16_t mStartOffset;
		/* specifies the offset with in the packet buffer where the
		   start point of the active region can be found */

	u16_t mThisLength;
		/* specifies the length of the active region starting from 
		   from the mStartOffset */

	u16_t mReferenceCount;
		/* This is the reference count. It maintains the number of references 
		   that are held to this packet buffer. It is used for memory management
		   purposes. You can modify the reference count by these functions.
		       PacketBuffer_IncreaseReference
		       PacketBuffer_DecreaseReference
		*/

	u32_t mTotalLength;
		/* Specifies the total length of a list of packet buffers.
		   If you add mThisLength of each packet buffer in a packet
		   buffer list it should equal the mTotalLength of the first
		   packet buffer in the list. And the mTotalLength of every
		   packet buffer in a packet buffer list is equal to the sum of 
		   mThisLength for this packet buffer and every following
		   packet buffer in a packet buffer list. NOTE: the end of a
		   packet buffer list is detected by (mTotalLength==mThisLength).
		   You can not use (mNext==NULL) to detect the end of a packet
		   buffer list as mNext may point to the next packet buffer list.
		*/
			
} PACKET_BUFFER, *PPACKET_BUFFER;

#if SMSC_ERROR_ENABLED
#define PACKET_QUEUE_SIGNATURE	(0xECD732CF)
#endif

/*****************************************************************************
* STRUCTURE: PACKET_QUEUE
* Used to maintain packet queue context.
* Should not access this structure directly.
* Should use PacketQueue_ function for all accesses
*****************************************************************************/
typedef struct PACKET_QUEUE_ {
	DECLARE_SIGNATURE /* used for run time type checking and memory corruption detection */
	PPACKET_BUFFER mHead;
	PPACKET_BUFFER mTail;
} PACKET_QUEUE, * PPACKET_QUEUE;

/*****************************************************************************
* FUNCTION: PacketQueue_Initialize
* PURPOSE: Initializes a PACKET_QUEUE structure
*****************************************************************************/
void PacketQueue_Initialize(PPACKET_QUEUE packetQueue);

/*****************************************************************************
* FUNCTION: PacketQueue_Push
* PURPOSE: Pushes a packet buffer list on to the tail of a packet queue
*****************************************************************************/
void PacketQueue_Push(PPACKET_QUEUE packetQueue,PPACKET_BUFFER packet);

/*****************************************************************************
* FUNCTION: PacketQueue_UnPop
* PURPOSE: Pushes a packet buffer list on to the head of a packet queue
*****************************************************************************/
void PacketQueue_UnPop(PPACKET_QUEUE packetQueue,PPACKET_BUFFER packet);

/*****************************************************************************
* FUNCTION: PacketQueue_Pop
* PURPOSE: Pops a packet buffer list from the head of a packet queue
*****************************************************************************/
PPACKET_BUFFER PacketQueue_Pop(PPACKET_QUEUE packetQueue);

#if INLINE_SIMPLE_FUNCTIONS 
/* See functional description below for how these macros are to behave */
#define PacketQueue_IsEmpty(packetQueue) ((packetQueue)->mHead==NULL)
#define PacketQueue_Peek(packetQueue)	((packetQueue)->mHead)
#define PacketBuffer_GetStartPoint(packetBuffer)	\
	(&(((u8_t *)(((mem_ptr_t)packetBuffer)+sizeof(PACKET_BUFFER)))[packetBuffer->mStartOffset]))
#else

/*****************************************************************************
* FUNCTION: PacketQueue_IsEmpty
* RETURN VALUE: 
*    returns non zero if the packet queue is empty
*    returns zero if the packet queue is not empty
*****************************************************************************/
int PacketQueue_IsEmpty(PPACKET_QUEUE packetQueue);

/*****************************************************************************
* FUNCTION: PacketQueue_Peek
* DESCRIPTION:
*    Returns a pointer to the head packet buffer with out removing it
*    from the queue
*****************************************************************************/
PPACKET_BUFFER PacketQueue_Peek(PPACKET_QUEUE packetQueue);

/*****************************************************************************
* FUNCTION: PacketBuffer_GetStartPoint
* DESCRIPTION:
*    Returns the start point of the active region in a packet buffer
*****************************************************************************/
u8_t * PacketBuffer_GetStartPoint(PPACKET_BUFFER packetBuffer);
#endif /* INLINE_SIMPLE_FUNCTIONS */

/*****************************************************************************
* FUNCTION: PacketBuffer_RestoreStartPoint
* DESCRIPTION:
*    Moves the start point to an absolute location, and adjusts mThisLength
*    to maintain the same end point. This is typically used to restore a 
*    location that was previously returned by PacketBuffer_GetStartPoint. 
*    This is common with TCP when a segment may need to be retransmitted. 
*****************************************************************************/
err_t PacketBuffer_RestoreStartPoint(PPACKET_BUFFER packetBuffer,u8_t * startPoint);

/*****************************************************************************
* FUNCTION: PacketBuffer_MoveStartPoint
* DESCRIPTION: 
*    Moves the start point of the active region by an offset. A positive 
*    offset means that the start point moves to a more positive address.
*    This is used to remove header space from the active region. A negative 
*    offset means the the start point moves to a less positive address. 
*    This is used to add header space to the active region.
*****************************************************************************/
err_t PacketBuffer_MoveStartPoint(PPACKET_BUFFER packetBuffer,s16_t offset);

/*****************************************************************************
* FUNCTION: PacketBuffer_MoveEndPoint
* DESCRIPTION:
*    Move the end point of the active region by an offset. Should not
*    be used in any packet buffer in a packet buffer list.
*****************************************************************************/
err_t PacketBuffer_MoveEndPoint(PPACKET_BUFFER packetBuffer,s16_t offset);

/*****************************************************************************
* FUNCTION: PacketBuffer_DecreaseReference
* DESCRIPTION:
*    Decreases the reference count of a packet buffer. If the
*    mReferenceCount becomes zero then it decreases the reference count
*    of the packet buffer point to by mNext(if not NULL), and then returns
*    this packet buffer to the appropriate packet buffer pool.
*****************************************************************************/
void PacketBuffer_DecreaseReference(PPACKET_BUFFER packetBuffer);

/*****************************************************************************
* FUNCTION: PacketBuffer_IncreaseReference
* DESCRIPTION:
*    Increases the reference count of a packet buffer. This prevents a
*    following call to PacketBuffer_DecreaseReference from deallocating the 
*    packet buffer
*****************************************************************************/
void PacketBuffer_IncreaseReference(PPACKET_BUFFER packetBuffer);

/*****************************************************************************
* FUNCTION: PacketBuffer_ReduceTotalLength
* DESCRIPTION:
*    Sets a new length for a packet buffer list. The new length must be less
*    than the current length. Any packet buffers at the end of the list that
*    do not remain with in the new length are freed(reference is decreased)
*****************************************************************************/
void PacketBuffer_ReduceTotalLength(PPACKET_BUFFER packetBuffer,u32_t newLength);

/*****************************************************************************
* FUNCTION: PacketBuffer_JoinChains
* DESCRIPTION:
*    Attaches tailChain to the end of the headChain.
*****************************************************************************/
void PacketBuffer_JoinChains(PPACKET_BUFFER headChain, PPACKET_BUFFER tailChain);

/*****************************************************************************
* FUNCTION: PacketBuffer_Dump
* DESCRIPTION:
*    Dumps the contents of the packet to the debug message console.
*    Useful for debugging purposes only.
*****************************************************************************/
void PacketBuffer_Dump(int enabled, PPACKET_BUFFER packet);

/*****************************************************************************
* FUNCTION: PacketBuffer_CopyFromExternalBuffer
* DESCRIPTION:
*    Copies data from an external buffer to a packet buffer
* PARAMETERS:
*    packet, this is the packet buffer list to copy data to.
*    offset, The offset within the packet buffer list where
*            to start copying data to.
*    data, a pointer to the external buffer where data is copied from.
*    length, the length of the external buffer where data is copied from.
*****************************************************************************/
void PacketBuffer_CopyFromExternalBuffer(PPACKET_BUFFER packet,u32_t offset, u8_t * data, u32_t length);

/*****************************************************************************
* FUNCTION: PacketBuffer_CopyToExternalBuffer
* DESCRIPTION:
*    Copies data from a packet buffer to an external buffer
* PARAMETERS:
*    packet, this is the packet buffer list to copy data from.
*    offset, The offset within the packet buffer list where
*            to start copying data from.
*    data, a pointer to the external buffer where data is copied to.
*    length, the length of the external buffer where data is copied to.
*****************************************************************************/
void PacketBuffer_CopyToExternalBuffer(PPACKET_BUFFER packet,u32_t offset, u8_t * data, u32_t length);

/*****************************************************************************
* FUNCTION: PacketBuffer_Copy
* DESCRIPTION:
*    Returns a copy of a packet buffer
*****************************************************************************/
PPACKET_BUFFER PacketBuffer_Copy(PPACKET_BUFFER packetBuffer);

/*****************************************************************************
* FUNCTION: PacketBuffer_Create
* DESCRIPTION:
*    Allocates a packet buffer from the appropriate packet buffer pool.
*    Initializes the following members
*        mNext=NULL;
*        mStartOffset=0;
*        mThisLength=length;
*        mTotalLength=length;
*        mReferenceCount=1;
*****************************************************************************/
PPACKET_BUFFER PacketBuffer_Create(u16_t length);

/*****************************************************************************
* FUNCTION: PacketManager_Initialize
* DESCRIPTION:
*    Initializes the packet buffer pools.
*    This function must be called before any calls to PacketBuffer_ functions
*****************************************************************************/
void PacketManager_Initialize(void);

#endif /* PACKET_MANAGER_H */
