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
/****************************************************************************
Modifications of this software Copyright(C) 2008, Standard Microsystems Corporation
  All Rights Reserved.
  The modifications to this program code are proprietary to SMSC and may not be copied,
  distributed, or used without a license to do so.  Such license may have
  Limited or Restricted Rights. Please refer to the license for further
  clarification.
*/
/*
FILE: tcp.c
Description: 
	Implements TCP
*/

#include "smsc_environment.h"

#if !TCP_ENABLED
#error TCP is not enabled
#endif

#include "ip.h"
#include "tcp.h"
#include "memory_pool.h"
#include "task_manager.h"
/*#ifdef M_WUFEI_080825
#undef M_WUFEI_080825
#endif
*/
#define TCP_HEADER_LENGTH	(20)
#define TCP_HEADER_GET_SOURCE_PORT(tcpHeader)	\
	smsc_ntohs(((u16_t *)(tcpHeader))[0])
#define TCP_HEADER_GET_DESTINATION_PORT(tcpHeader)	\
	smsc_ntohs(((u16_t *)(tcpHeader))[1])
#ifdef M_WUFEI_080825
#define TCP_HEADER_GET_SEQUENCE_NUMBER(tcpHeader)	\
	smsc_ntohl(((u32_t *)(tcpHeader))[1])
#define TCP_HEADER_GET_ACKNOWLEDGEMENT_NUMBER(tcpHeader)	\
	smsc_ntohl(((u32_t *)(tcpHeader))[2])
#else
#define TCP_HEADER_GET_SEQUENCE_NUMBER(tcpHeader)		\
	smsc_ntohl(((u32_t)tcpHeader[4])+(((u32_t)tcpHeader[5])<<8)+(((u32_t)tcpHeader[6])<<16)+(((u32_t)tcpHeader[7])<<24))
/*	smsc_ntohl(((u32_t *)(tcpHeader))[1])*/
#define TCP_HEADER_GET_ACKNOWLEDGEMENT_NUMBER(tcpHeader)		\
	smsc_ntohl(((u32_t)tcpHeader[8])+(((u32_t)tcpHeader[9])<<8)+(((u32_t)tcpHeader[10])<<16)+(((u32_t)tcpHeader[11])<<24))
/*	smsc_ntohl(((u32_t *)(tcpHeader))[2])*/
#endif
#define TCP_HEADER_GET_DATA_OFFSET(tcpHeader)	\
	((((u8_t *)(tcpHeader))[12])>>4)
#define TCP_HEADER_GET_FLAGS(tcpHeader)	\
	(((u8_t *)(tcpHeader))[13])
/* TCP_FLAGS */ 
#define TCP_HEADER_FLAG_FIN (0x01U)
#define TCP_HEADER_FLAG_SYN (0x02U)
#define TCP_HEADER_FLAG_RST (0x04U)
#define TCP_HEADER_FLAG_PSH (0x08U)
#define TCP_HEADER_FLAG_ACK (0x10U)
#define TCP_HEADER_FLAG_URG (0x20U)
#define TCP_HEADER_FLAG_ECE (0x40U)
#define TCP_HEADER_FLAG_CWR (0x80U) 
#ifndef M_WUFEI_080825
#define TCP_HEADER_GET_WINDOW(tcpHeader)	\
	smsc_ntohs(((u16_t *)(tcpHeader))[7])
	
/* TCP_HEADER_GET_CHECKSUM does not include
	endian conversion. The user is assumed to take care of that
	and checksum calculations may take advantage of this. */
#define TCP_HEADER_GET_CHECKSUM(tcpHeader)	\
	(((u16_t *)(tcpHeader))[8])

#define TCP_HEADER_GET_URGENT_POINTER(tcpHeader)	\
	smsc_ntohs(((u16_t *)(tcpHeader))[9])

#define TCP_HEADER_NTOH_SOURCE_PORT(tcpHeader)	\
	(((u16_t *)(tcpHeader))[0])=smsc_ntohs(((u16_t *)(tcpHeader))[0])
#define TCP_HEADER_NTOH_DESTINATION_PORT(tcpHeader)	\
	(((u16_t *)(tcpHeader))[1])=smsc_ntohs(((u16_t *)(tcpHeader))[1])
#define TCP_HEADER_NTOH_SEQUENCE_NUMBER(tcpHeader)	\
	(((u32_t *)(tcpHeader))[1])=smsc_ntohl(((u32_t *)(tcpHeader))[1])
#define TCP_HEADER_NTOH_ACKNOWLEDGEMENT_NUMBER(tcpHeader)	\
	(((u32_t *)(tcpHeader))[2])=smsc_ntohl(((u32_t *)(tcpHeader))[2])
#define TCP_HEADER_NTOH_WINDOW(tcpHeader)	\
	(((u16_t *)(tcpHeader))[7])=smsc_ntohs(((u16_t *)(tcpHeader))[7])
#define TCP_HEADER_NTOH_URGENT_POINTER(tcpHeader)	\
	(((u16_t *)(tcpHeader))[9])=smsc_ntohs(((u16_t *)(tcpHeader))[9])

/* The following read and write macros do not use endian correction
	They are indended to work with the NTOH macros above */
#define TCP_HEADER_READ_SOURCE_PORT(tcpHeader)	\
	(((u16_t *)(tcpHeader))[0])
#define TCP_HEADER_READ_DESTINATION_PORT(tcpHeader)	\
	(((u16_t *)(tcpHeader))[1])
#define TCP_HEADER_READ_SEQUENCE_NUMBER(tcpHeader)	\
	(((u32_t *)(tcpHeader))[1])
#define TCP_HEADER_READ_ACKNOWLEDGEMENT_NUMBER(tcpHeader)	\
	(((u32_t *)(tcpHeader))[2])
#define TCP_HEADER_READ_WINDOW(tcpHeader)	\
	(((u16_t *)(tcpHeader))[7])
#define TCP_HEADER_READ_URGENT_POINTER(tcpHeader)	\
	(((u16_t *)(tcpHeader))[9])
#else
#define TCP_HEADER_GET_WINDOW(tcpHeader)	\
	smsc_ntohs(((u16_t)tcpHeader[14])<<8 + (u16_t)tcpHeader[15])
	
/* TCP_HEADER_GET_CHECKSUM does not include
	endian conversion. The user is assumed to take care of that
	and checksum calculations may take advantage of this. */
#define TCP_HEADER_GET_CHECKSUM(tcpHeader)	\
	(((u16_t)tcpHeader[16])<<8 + ((u16_t)tcpHeader[17]))
#define TCP_HEADER_GET_URGENT_POINTER(tcpHeader)	\
	smsc_ntohs(((u16_t)tcpHeader[18])<<8 + ((u16_t)tcpHeader[19]))
#define TCP_HEADER_NTOH_SOURCE_PORT(tcpHeader)	\
	do{	\
		unsigned char temp;	\
		temp = tcpHeader[0];	\
		tcpHeader[0] = tcpHeader[1];	\
		tcpHeader[1] = temp;	\
	}while(0)
#define TCP_HEADER_NTOH_DESTINATION_PORT(tcpHeader)	\
	do{	\
		unsigned char temp;	\
		temp = tcpHeader[2];	\
		tcpHeader[2] = tcpHeader[3];	\
		tcpHeader[3] = temp;	\
	}while(0)
#define TCP_HEADER_NTOH_SEQUENCE_NUMBER(tcpHeader)	\
	do{	\
		unsigned char temp;	\
		temp = tcpHeader[4];	\
		tcpHeader[4] = tcpHeader[7];	\
		tcpHeader[7] = temp;	\
		temp = tcpHeader[5];	\
		tcpHeader[5] = tcpHeader[6];	\
		tcpHeader[6] = temp;	\
	}while(0)
#define TCP_HEADER_NTOH_ACKNOWLEDGEMENT_NUMBER(tcpHeader)	\
	do{	\
		unsigned char temp;	\
		temp = tcpHeader[8];	\
		tcpHeader[8] = tcpHeader[11];	\
		tcpHeader[11] = temp;	\
		temp = tcpHeader[9];	\
		tcpHeader[9] = tcpHeader[10];	\
		tcpHeader[10] = temp;	\
	}while(0)
#define TCP_HEADER_NTOH_WINDOW(tcpHeader)	\
	do{	\
		unsigned char temp;	\
		temp = tcpHeader[14];	\
		tcpHeader[14] = tcpHeader[15];	\
		tcpHeader[15] = temp;	\
	}while(0)
#define TCP_HEADER_NTOH_URGENT_POINTER(tcpHeader)	\
	do{	\
		unsigned char temp;	\
		temp = tcpHeader[18];	\
		tcpHeader[18] = tcpHeader[19];	\
		tcpHeader[19] = temp;	\
	}while(0)

/* The following read and write macros do not use endian correction
	They are indended to work with the NTOH macros above */
#define TCP_HEADER_READ_SOURCE_PORT(tcpHeader)	\
	(((u16_t *)(tcpHeader))[0])
/*	
	(((u16_t)tcpHeader[0])<<8 + ((u16_t)tcpHeader[1]))*/
#define TCP_HEADER_READ_DESTINATION_PORT(tcpHeader)	\
	(((u16_t *)(tcpHeader))[1])
/*
	(((u16_t)tcpHeader[2])<<8 + ((u16_t)tcpHeader[3]))*/
#define TCP_HEADER_READ_SEQUENCE_NUMBER(tcpHeader)	\
	(((u32_t)tcpHeader[4])+(((u32_t)tcpHeader[5])<<8)+(((u32_t)tcpHeader[6])<<16)+(((u32_t)tcpHeader[7])<<24))
#define TCP_HEADER_READ_ACKNOWLEDGEMENT_NUMBER(tcpHeader)	\
	(((u32_t)tcpHeader[8])+(((u32_t)tcpHeader[9])<<8)+(((u32_t)tcpHeader[10])<<16)+(((u32_t)tcpHeader[11])<<24))
#define TCP_HEADER_READ_WINDOW(tcpHeader)	\
	(((u16_t)tcpHeader[14]) + (((u16_t)tcpHeader[15])<<8))
#define TCP_HEADER_READ_URGENT_POINTER(tcpHeader)	\
	(((u16_t)tcpHeader[18]) + (((u16_t)tcpHeader[19])<<8))
#endif
#ifndef M_WUFEI_080825
#define TCP_HEADER_WRITE_SEQUENCE_NUMBER(tcpHeader,sequenceNumber)	\
	(((u32_t *)(tcpHeader))[1])=(u32_t)(sequenceNumber)
#define TCP_HEADER_SET_SOURCE_PORT(tcpHeader,sourcePort)	\
	(((u16_t *)(tcpHeader))[0])=smsc_htons(sourcePort)
#define TCP_HEADER_SET_DESTINATION_PORT(tcpHeader,destinationPort)	\
	(((u16_t *)(tcpHeader))[1])=smsc_htons(destinationPort)	
#define TCP_HEADER_SET_SEQUENCE_NUMBER(tcpHeader,sequenceNumber)	\
	(((u32_t *)(tcpHeader))[1])=smsc_htonl(sequenceNumber)
#define TCP_HEADER_SET_ACKNOWLEDGEMENT_NUMBER(tcpHeader,acknowledgementNumber)	\
	(((u32_t *)(tcpHeader))[2])=smsc_htonl(acknowledgementNumber)
#else
#define TCP_HEADER_WRITE_SEQUENCE_NUMBER(tcpHeader,sequenceNumber)	\
	do{	\
		tcpHeader[4] = (((u32_t)sequenceNumber)>>24)&0xff;	\
		tcpHeader[5] = (((u32_t)sequenceNumber)>>16)&0xff;	\
		tcpHeader[6] = (((u32_t)sequenceNumber)>>8)&0xff;	\
		tcpHeader[7] = ((u32_t)sequenceNumber)&0xff;	\
	}while(0)

#define TCP_HEADER_SET_SOURCE_PORT(tcpHeader,sourcePort)	\
	do{	\
		tcpHeader[0] = (sourcePort>>8)&0xff;	\
		tcpHeader[1] = (sourcePort)&0xff;	\
	}while(0)
#define TCP_HEADER_SET_DESTINATION_PORT(tcpHeader,destinationPort)	\
	do{	\
		tcpHeader[2] = (destinationPort>>8)&0xff;	\
		tcpHeader[3] = (destinationPort)&0xff;	\
	}while(0)
#define TCP_HEADER_SET_SEQUENCE_NUMBER(tcpHeader,sequenceNumber)	\
	do{	\
		tcpHeader[4] = (((u32_t)sequenceNumber)>>24)&0xff;	\
		tcpHeader[5] = (((u32_t)sequenceNumber)>>16)&0xff;	\
		tcpHeader[6] = (((u32_t)sequenceNumber)>>8)&0xff;	\
		tcpHeader[7] = (((u32_t)sequenceNumber))&0xff;	\
	}while(0)/**/
#define TCP_HEADER_SET_ACKNOWLEDGEMENT_NUMBER(tcpHeader,acknowledgementNumber)	\
	do{	\
		tcpHeader[8] = ((acknowledgementNumber)>>24)&0xff;	\
		tcpHeader[9] = ((acknowledgementNumber)>>16)&0xff;	\
		tcpHeader[10] = ((acknowledgementNumber)>>8)&0xff;	\
		tcpHeader[11] = (acknowledgementNumber)&0xff;	\
	}while(0)/**/
#endif
#define TCP_HEADER_SET_DATA_OFFSET(tcpHeader,dataOffset)	\
	(((u8_t *)(tcpHeader))[12])=((u8_t)((dataOffset)<<4))
#define TCP_HEADER_SET_FLAGS(tcpHeader,flags)	\
	(((u8_t *)(tcpHeader))[13])=((u8_t)(flags))
#define TCP_HEADER_SET_FLAG(tcpHeader,flag)	\
	(((u8_t *)(tcpHeader))[13])=(TCP_HEADER_GET_FLAGS(tcpHeader)|(flag))
#define TCP_HEADER_UNSET_FLAG(tcpHeader,flag) \
	(((u8_t *)(tcpHeader))[13])=(TCP_HEADER_GET_FLAGS(tcpHeader)&(~(flag)))
#define TCP_HEADER_SET_WINDOW(tcpHeader,window)	\
	(((u16_t *)(tcpHeader))[7])=smsc_htons(window)
#ifdef M_WUFEI_080825
/* TCP_HEADER_SET_CHECKSUM does not include
	endian conversion. The user is assumed to take care of that
	and checksum calculations may take advantage of this */
#define TCP_HEADER_SET_CHECKSUM(tcpHeader,checksum)	\
	(((u16_t *)(tcpHeader))[8])=((u16_t)(checksum))

#define TCP_HEADER_SET_URGENT_POINTER(tcpHeader,urgentPointer)	\
	(((u16_t *)(tcpHeader))[9])=smsc_htons(urgentPointer)
#else
/* TCP_HEADER_SET_CHECKSUM does not include
	endian conversion. The user is assumed to take care of that
	and checksum calculations may take advantage of this */
#define TCP_HEADER_SET_CHECKSUM(tcpHeader,checksum)	\
	do{	\
		tcpHeader[16] = (checksum>>8)&0xff;	\
		tcpHeader[17] = (checksum)&0xff;	\
	}while(0)	
#define TCP_HEADER_SET_URGENT_POINTER(tcpHeader,urgentPointer)	\
	do{	\
		tcpHeader[18] = (urgentPointer>>8)&0xff;	\
		tcpHeader[19] = ((urgentPointer))&0xff;	\
	}while(0)	
#endif
#define TCP_GET_TCP_LENGTH(segment) (((u32_t)((segment)->mLength)) + ((TCP_HEADER_GET_FLAGS((segment)->mTcpHeader) & TCP_HEADER_FLAG_FIN || \
          TCP_HEADER_GET_FLAGS((segment)->mTcpHeader) & TCP_HEADER_FLAG_SYN)? 1: 0))

#define TCP_SEQUENCE_LESS_THAN(a,b)     ((s32_t)((a)-(b)) < 0)
#define TCP_SEQUENCE_LESS_OR_EQUAL(a,b)    ((s32_t)((a)-(b)) <= 0)
#define TCP_SEQUENCE_GREATER_THAN(a,b)     ((s32_t)((a)-(b)) > 0)
#define TCP_SEQUENCE_GREATER_OR_EQUAL(a,b)    ((s32_t)((a)-(b)) >= 0)
#define TCP_SEQUENCE_BETWEEN(a,b,c) (TCP_SEQUENCE_GREATER_OR_EQUAL(a,b) && TCP_SEQUENCE_LESS_OR_EQUAL(a,c))


static PTCP_SEGMENT Tcp_AllocateSegment(void);
static void Tcp_SendSegment(PTCP_SEGMENT segment, struct TCP_CONTROL_BLOCK * tcpControlBlock);
static err_t Tcp_QueueControlData(
	struct TCP_CONTROL_BLOCK * tcpControlBlock, u8_t flags,
	u8_t *optdata, u8_t optlen);
static void Tcp_KillOldestTimeWaitBlock(void);
static void Tcp_KillOldestBlockWithLowerPriority(u8_t priority);
static void Tcp_Abort(struct TCP_CONTROL_BLOCK * tcpControlBlock);
static void Tcp_RemoveControlBlockFromList(struct TCP_CONTROL_BLOCK ** controlBlockList, struct TCP_CONTROL_BLOCK * tcpControlBlock);
static void Tcp_PurgeControlBlock(struct TCP_CONTROL_BLOCK * tcpControlBlock);
static void Tcp_FreeSegmentList(PTCP_SEGMENT segment);
static void Tcp_FreeSegment(PTCP_SEGMENT segment);
static void Tcp_SendReset(u32_t sequenceNumber, u32_t acknowledgementNumber,
  PIP_ADDRESS mLocalAddress, PIP_ADDRESS remoteAddress,
  u16_t localPort, u16_t remotePort);
static u32_t Tcp_GetNextInitialSequenceNumber(void);
static err_t Tcp_NullReceiverCallBack(void *arg, struct TCP_CONTROL_BLOCK * tcpControlBlock, PPACKET_BUFFER packet, err_t error);
static err_t Tcp_SendControlFlags(struct TCP_CONTROL_BLOCK * tcpControlBlock, u8_t flags);
static u16_t Tcp_GetNewPortNumber(void);
static void Tcp_ActivateTimer(void);
static void Tcp_TimerCallBack(void *arg);
static void Tcp_MainTimer(void);
static void Tcp_FastTimer(void);
static void Tcp_SlowTimer(void);
#define Tcp_QueueAcknowledgement(tcpControlBlock)							\
	if((tcpControlBlock)->mFlags & TCB_FLAG_ACK_DELAY) { 	\
		(tcpControlBlock)->mFlags &= ~TCB_FLAG_ACK_DELAY;	\
		(tcpControlBlock)->mFlags |= TCB_FLAG_ACK_NOW;		\
		Tcp_SendQueuedData(tcpControlBlock); 				\
	} else { 												\
		(tcpControlBlock)->mFlags |= TCB_FLAG_ACK_DELAY; 	\
	}
#define Tcp_SendAcknowledgementNow(tcpControlBlock) 				\
	(tcpControlBlock)->mFlags |= TCB_FLAG_ACK_NOW;	\
	Tcp_SendQueuedData(tcpControlBlock)
static void Tcp_RetransmitAllSegments(struct TCP_CONTROL_BLOCK * tcpControlBlock);
static void Tcp_RetransmitOneSegment(struct TCP_CONTROL_BLOCK * tcpControlBlock);
static void Tcp_SendKeepAlive(struct TCP_CONTROL_BLOCK * tcpControlBlock);
static void Tcp_ParseOptions(struct TCP_CONTROL_BLOCK * tcpControlBlock, u8_t * tcpHeader);
static PPACKET_BUFFER Tcp_AllocateHeaderOnlyPacket(PIP_ADDRESS destinationAddress);
/* lzf 20081011 add for 对TCP_CONTROL_BLOCK链表增加和删除时进行检查，避免出现两个相同节点连续出现在链表中从而造成死循环*/
/* */#define TCP_CONTROL_BLOCK_CHECK


#ifdef 	 TCP_CONTROL_BLOCK_CHECK
void TCP_REGISTER_CONTROL_BLOCK(struct TCP_CONTROL_BLOCK **controlBlockList, struct TCP_CONTROL_BLOCK * newControlBlock) 
{
	struct TCP_CONTROL_BLOCK * tempTcb = *controlBlockList;
	
	while(tempTcb)
	{
		if(newControlBlock == tempTcb){
			SMSC_WARNING(1,("newControlBlock[%p]has in the TCB list!!!"));
			return;
		}
		tempTcb = tempTcb->mNext;
	}
	if(((newControlBlock)!= *(controlBlockList))||(*(controlBlockList)==NULL))
	{
		(newControlBlock)->mNext = *(controlBlockList);
		*(controlBlockList) = (newControlBlock); 
		Tcp_ActivateTimer();
	}
	else
	{
		SMSC_WARNING(1,("\n TCP_REGISTER_CONTROL_BLOCK error!!!!!!!!!!"));
	}
}
void TCP_REMOVE_CONTROL_BLOCK(struct TCP_CONTROL_BLOCK ** controlBlockList, struct TCP_CONTROL_BLOCK * blockToRemove) 
{
	do { 																															
			struct TCP_CONTROL_BLOCK *  tempControlBlock;																				
			if(*(controlBlockList) == (blockToRemove)) { 																				
				(*(controlBlockList)) = (*controlBlockList)->mNext; 																	
			} else for(tempControlBlock = *(controlBlockList); tempControlBlock != NULL; tempControlBlock = tempControlBlock->mNext) 
			{	
				if((tempControlBlock->mNext != NULL) && (tempControlBlock->mNext == (blockToRemove))) 
				{									
					if((tempControlBlock->mNext) !=( (blockToRemove)->mNext))
					{
						tempControlBlock->mNext = (blockToRemove)->mNext;																	
					}
					else
					{
						tempControlBlock->mNext = ((blockToRemove)->mNext)->mNext;	
						SMSC_WARNING(1,("\n TCP_REMOVE_CONTROL_BLOCK error!!!!!!!!!!"));
					}
					break;	
				}																														
			}																															
			(blockToRemove)->mNext = NULL;																								
		} while(0);
}
#else
#define TCP_REGISTER_CONTROL_BLOCK(controlBlockList, newControlBlock) 	\
	do { 						\
		(newControlBlock)->mNext = *(controlBlockList); \
		*(controlBlockList) = (newControlBlock); 		\
		Tcp_ActivateTimer(); 	\
	} while(0)
#define TCP_REMOVE_CONTROL_BLOCK(controlBlockList, blockToRemove) 																	\
	do { 																															\
		struct TCP_CONTROL_BLOCK *  tempControlBlock;																				\
		if(*(controlBlockList) == (blockToRemove)) { 																				\
			(*(controlBlockList)) = (*controlBlockList)->mNext; 																	\
		} else for(tempControlBlock = *(controlBlockList); tempControlBlock != NULL; tempControlBlock = tempControlBlock->mNext) {	\
			if((tempControlBlock->mNext != NULL) && (tempControlBlock->mNext == (blockToRemove))) {									\
				tempControlBlock->mNext = (blockToRemove)->mNext;																	\
				break;																												\
			}																														\
		}																															\
		(blockToRemove)->mNext = NULL;																								\
	} while(0)
#endif

#if TCP_DEBUG
static void Tcp_PrintHeader(u8_t * tcpHeader);
static const char * Tcp_StateToString(TCP_STATE state);
static void Tcp_PrintFlags(u8_t flags);
#endif
 void Tcp_PrintControlBlocks(void);

#if SMSC_ERROR_ENABLED
static s16_t Tcp_CheckControlBlock(void);
#endif

static TASK gTcpTimerTask;


/** List of all TCP PCBs in LISTEN state */
static struct TCP_CONTROL_BLOCK * gTcpListenerList;
/** List of all TCP PCBs that are in a state in which
 * they accept or send data. */
static struct TCP_CONTROL_BLOCK * gTcpActiveList;  
/** List of all TCP PCBs in TIME-WAIT state */
static struct TCP_CONTROL_BLOCK * gTcpTimeWaitList;
/* Incremented every coarse grained timer shot (typically every 500 ms). */
static struct TCP_CONTROL_BLOCK * gInputControlBlock;
static u32_t gTcpTicks;
const u8_t gTcpBackOff[13] =
    { 1, 2, 3, 4, 5, 6, 7, 7, 7, 7, 7, 7, 7};
static u8_t gTcpTimer;
static int gTcpTimerActive = 0;

static u8_t gTcpControlBlockMemory[
	MEMORY_POOL_SIZE(
		sizeof(struct TCP_CONTROL_BLOCK),
		TCP_CONTROL_BLOCK_COUNT)];
/*static*/ MEMORY_POOL_HANDLE gTcpControlBlockPool=NULL;

static u8_t gTcpSegmentMemory[
	MEMORY_POOL_SIZE(
		sizeof(TCP_SEGMENT),
		TCP_SEGMENT_COUNT)];
/*static*/ MEMORY_POOL_HANDLE gTcpSegmentPool=NULL;

 /* Main initialization function must be called before any other Tcp_ function */
void Tcp_Initialize()
{
	gTcpControlBlockPool=MemoryPool_Initialize(
		gTcpControlBlockMemory,
		sizeof(struct TCP_CONTROL_BLOCK),
		TCP_CONTROL_BLOCK_COUNT,NULL);
	gTcpSegmentPool=MemoryPool_Initialize(
		gTcpSegmentMemory,
		sizeof(TCP_SEGMENT),
		TCP_SEGMENT_COUNT,NULL);

	/* Clear globals. */
	gTcpListenerList = NULL;
	gTcpActiveList = NULL;
	gTcpTimeWaitList = NULL;

	/* initialize timer */
	gTcpTicks = 0;
	gTcpTimer = 0;
	Task_Initialize(&gTcpTimerTask,/*TASK_PRIORITY_TIMER*/TASK_PRIORITY_TIMER_TCP,
		Tcp_TimerCallBack,NULL);

	SMSC_TRACE(TCP_DEBUG,("Tcp_Initialize: Successful"));
}

static PTCP_SEGMENT Tcp_AllocateSegment(void)
{
	PTCP_SEGMENT result=MemoryPool_Allocate(gTcpSegmentPool);
	if(result!=NULL) {
		ASSIGN_SIGNATURE(result,TCP_SEGMENT_SIGNATURE);
	}
	return result;
}
/**
 * Deallocates a list of TCP segments (TCP_SEGMENT structures).
 *
 */
static void Tcp_FreeSegmentList(PTCP_SEGMENT segment)
{
	PTCP_SEGMENT nextSegment;
	while (segment != NULL) {
		CHECK_SIGNATURE(segment,TCP_SEGMENT_SIGNATURE);
		nextSegment = segment->mNext;
		Tcp_FreeSegment(segment);
		segment = nextSegment;
	}
}

/**
 * Frees a TCP segment.
 *
 */
static void Tcp_FreeSegment(PTCP_SEGMENT segment)
{
	if (segment != NULL) {
		CHECK_SIGNATURE(segment,TCP_SEGMENT_SIGNATURE);
#ifdef MEMORY_POOL_CHANGE_090217
		do{
			if (segment->mPacket != NULL) {
				PacketBuffer_DecreaseReference(segment->mPacket);
			}
			else{
				break;
			}
		}while(segment->mPacket = segment->mPacket->mNext);
#else
		if (segment->mPacket != NULL) {
			PacketBuffer_DecreaseReference(segment->mPacket);
			segment->mPacket = NULL;
		}
#endif
		MemoryPool_Free(gTcpSegmentPool, segment);
	}
}

/**
 * Purges a TCP Control Block. Removes any buffered data and frees the buffer memory.
 *
 */
static void Tcp_PurgeControlBlock(struct TCP_CONTROL_BLOCK * tcpControlBlock)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(tcpControlBlock!=NULL);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);
	
	if ((tcpControlBlock->mState != TCP_STATE_CLOSED) &&
		(tcpControlBlock->mState != TCP_STATE_TIME_WAIT) &&
		(tcpControlBlock->mState != TCP_STATE_LISTEN)) 
	{
		/*
		if (tcpControlBlock->mUnsentSegments != NULL) {    
			SMSC_TRACE(TCP_DEBUG, ("Tcp_PurgeControlBlock: not all data sent"));
		}
		if (tcpControlBlock->mUnackedSegments != NULL) {    
			SMSC_TRACE(TCP_DEBUG, ("Tcp_PurgeControlBlock: data left on ->mUnackedSegments"));
		}
		if (tcpControlBlock->mOutOfSequenceSegments != NULL) {    
			SMSC_TRACE(TCP_DEBUG, ("Tcp_PurgeControlBlock: data left on ->mOutOfSequenceSegments"));
		}*/
		SMSC_NOTICE(TCP_DEBUG,("1Free tcpControlBlock[%p]->mOutOfSequenceSegments[%p]",tcpControlBlock,tcpControlBlock->mOutOfSequenceSegments));
		Tcp_FreeSegmentList(tcpControlBlock->mOutOfSequenceSegments);
		tcpControlBlock->mOutOfSequenceSegments = NULL;
		Tcp_FreeSegmentList(tcpControlBlock->mUnsentSegments);
		Tcp_FreeSegmentList(tcpControlBlock->mUnackedSegments);
		tcpControlBlock->mUnackedSegments = tcpControlBlock->mUnsentSegments = NULL;
	}
}

/**
 * Purges the Control Block and removes it from a Control Block list. Any delayed ACKs are sent first.
 *
 */
static void Tcp_RemoveControlBlockFromList(struct TCP_CONTROL_BLOCK **controlBlockList, struct TCP_CONTROL_BLOCK * tcpControlBlock)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(controlBlockList!=NULL);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(tcpControlBlock!=NULL);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);

	TCP_REMOVE_CONTROL_BLOCK(controlBlockList,tcpControlBlock);
	
	Tcp_PurgeControlBlock(tcpControlBlock);
  
	/* if there is an outstanding delayed ACKs, send it */
	if ((tcpControlBlock->mState != TCP_STATE_TIME_WAIT) &&
		(tcpControlBlock->mState != TCP_STATE_LISTEN) &&
		(tcpControlBlock->mFlags & TCB_FLAG_ACK_DELAY)) 
	{
		tcpControlBlock->mFlags |= TCB_FLAG_ACK_NOW;
		Tcp_SendQueuedData(tcpControlBlock);
	}  
	tcpControlBlock->mState = TCP_STATE_CLOSED;

	SMSC_ASSERT(Tcp_CheckControlBlock());
}

static void Tcp_SendReset(u32_t sequenceNumber, u32_t acknowledgementNumber,
	PIP_ADDRESS localAddress, PIP_ADDRESS remoteAddress,
	u16_t localPort, u16_t remotePort)
{
	PPACKET_BUFFER packet;
	u8_t * tcpHeader;
	packet = Tcp_AllocateHeaderOnlyPacket(remoteAddress);
	if (packet == NULL) {
		SMSC_NOTICE(TCP_DEBUG, ("Tcp_SendReset: could not allocate memory for packet buffer"));
		return;
	}

	tcpHeader=PacketBuffer_GetStartPoint(packet);
	TCP_HEADER_SET_SOURCE_PORT(tcpHeader,localPort);
	TCP_HEADER_SET_DESTINATION_PORT(tcpHeader,remotePort);
	TCP_HEADER_SET_SEQUENCE_NUMBER(tcpHeader,sequenceNumber);
	TCP_HEADER_SET_ACKNOWLEDGEMENT_NUMBER(tcpHeader,acknowledgementNumber);
	TCP_HEADER_SET_FLAGS(tcpHeader,TCP_HEADER_FLAG_RST|TCP_HEADER_FLAG_ACK);
	TCP_HEADER_SET_WINDOW(tcpHeader,TCP_WINDOW_SIZE);
	TCP_HEADER_SET_URGENT_POINTER(tcpHeader,0);
	TCP_HEADER_SET_DATA_OFFSET(tcpHeader,5);
	SMSC_TRACE(TCP_DEBUG,("TCP() sending RST and ACK"));

	/* make sure following typecast is acceptable */
	SMSC_ASSERT(!(packet->mTotalLength&0xFFFF0000));
	
	TCP_HEADER_SET_CHECKSUM(tcpHeader,0);
	TCP_HEADER_SET_CHECKSUM(tcpHeader,
		Ip_PseudoCheckSum(packet,localAddress,remoteAddress,
			IP_PROTOCOL_TCP,(u16_t)(packet->mTotalLength)));
	Ip_Output(packet,localAddress,remoteAddress,IP_PROTOCOL_TCP);
	SMSC_TRACE(TCP_DEBUG, ("Tcp_SendReset: sequenceNumber %"U32_F" acknowledgementNumber %"U32_F, sequenceNumber, acknowledgementNumber));
}

/**
 * Aborts a connection by sending a RST to the remote host and deletes
 * the local protocol control block. This is done when a connection is
 * killed because of shortage of memory.
 *
 */
static void Tcp_Abort(struct TCP_CONTROL_BLOCK * tcpControlBlock)
{
	u32_t sequenceNumber, acknowledgementNumber;
	u16_t remotePort, localPort;
	IP_ADDRESS remoteAddress, localAddress;
	TCP_ERROR_CALLBACK errorFunction;
	void *errorFunctionArgument;

	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(tcpControlBlock!=NULL);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);

	/* Figure out on which TCP CONTROL BLOCK list we are, and remove us. If we
	are in an active state, call the receive function associated with
	the CONTROL BLOCK with a NULL argument, and send an RST to the remote end. */
	if (tcpControlBlock->mState == TCP_STATE_TIME_WAIT) {
		Tcp_RemoveControlBlockFromList(&gTcpTimeWaitList, tcpControlBlock);
		MemoryPool_Free(gTcpControlBlockPool, tcpControlBlock);
	} else {
		sequenceNumber = tcpControlBlock->mSenderNextSequenceNumber;
		acknowledgementNumber = tcpControlBlock->mReceiverNextSequenceNumber;
		IP_ADDRESS_COPY(&localAddress, &(tcpControlBlock->mLocalAddress));
		IP_ADDRESS_COPY(&remoteAddress, &(tcpControlBlock->mRemoteAddress));
		localPort = tcpControlBlock->mLocalPort;
		remotePort = tcpControlBlock->mRemotePort;
		errorFunction = (TCP_ERROR_CALLBACK)(tcpControlBlock->mErrorCallBack);
		errorFunctionArgument = tcpControlBlock->mCallbackArgument;
		Tcp_RemoveControlBlockFromList(&gTcpActiveList, tcpControlBlock);
		if (tcpControlBlock->mUnackedSegments != NULL) {
			Tcp_FreeSegmentList(tcpControlBlock->mUnackedSegments);
		}
		if (tcpControlBlock->mUnsentSegments != NULL) {
			Tcp_FreeSegmentList(tcpControlBlock->mUnsentSegments);
		}
		if (tcpControlBlock->mOutOfSequenceSegments != NULL) {
			SMSC_NOTICE(TCP_DEBUG,("2Free tcpControlBlock[%p]->mOutOfSequenceSegments[%p]",tcpControlBlock,tcpControlBlock->mOutOfSequenceSegments));
			Tcp_FreeSegmentList(tcpControlBlock->mOutOfSequenceSegments);
		}
		MemoryPool_Free(gTcpControlBlockPool,tcpControlBlock);
		if(errorFunction!=NULL)
			((TCP_ERROR_CALLBACK)errorFunction)(errorFunctionArgument,ERR_ABRT);
		SMSC_TRACE(TCP_DEBUG, ("Tcp_Abort: sending RST"));
		Tcp_SendReset(sequenceNumber, acknowledgementNumber, &localAddress, &remoteAddress, localPort, remotePort);
	}
}

static void Tcp_KillOldestBlockWithLowerPriority(u8_t priority)
{
	struct TCP_CONTROL_BLOCK * tcpControlBlock;
	struct TCP_CONTROL_BLOCK * inActiveControlBlock;
	u32_t inActivityDuration;
	u8_t maximumPriority;

	maximumPriority = TCP_PRIORITY_MAX;

	/* We kill the oldest active connection that has lower priority than
		priority. */
	inActivityDuration = 0;
	inActiveControlBlock = NULL;
	for(tcpControlBlock = gTcpActiveList; tcpControlBlock != NULL; tcpControlBlock = tcpControlBlock->mNext) {
		if (tcpControlBlock->mPriority <= priority &&
			tcpControlBlock->mPriority <= maximumPriority &&
			(u32_t)(gTcpTicks - tcpControlBlock->mTimer) >= inActivityDuration) 
		{
			inActivityDuration = gTcpTicks - tcpControlBlock->mTimer;
			inActiveControlBlock = tcpControlBlock;
			maximumPriority = tcpControlBlock->mPriority;
		}
	}
	if (inActiveControlBlock != NULL) {
		SMSC_NOTICE(TCP_DEBUG, ("TCP(%p) killing oldest control block (%"S32_F")",
			(void *)inActiveControlBlock, inActivityDuration));
		Tcp_Abort(inActiveControlBlock);
	}      
}

static void Tcp_KillOldestTimeWaitBlock(void)
{
	struct TCP_CONTROL_BLOCK * tcpControlBlock;
	struct TCP_CONTROL_BLOCK * inActiveControlBlock;
	u32_t inActivityDuration;

	inActivityDuration = 0;
	inActiveControlBlock = NULL;
	for(tcpControlBlock = gTcpTimeWaitList; tcpControlBlock != NULL; tcpControlBlock = tcpControlBlock->mNext) {
		if ((u32_t)(gTcpTicks - tcpControlBlock->mTimer) >= inActivityDuration) {
			inActivityDuration = gTcpTicks - tcpControlBlock->mTimer;
			inActiveControlBlock = tcpControlBlock;
		}
	}
	if (inActiveControlBlock != NULL) {
		SMSC_NOTICE(TCP_DEBUG, ("TCP(%p) killing oldest TIME-WAIT control block (%"S32_F")",			
			(void *)inActiveControlBlock, inActivityDuration));
		Tcp_Abort(inActiveControlBlock);
	}      
}
  
/**
 * Calculates a new initial sequence number for new connections.
 *
 */
static u32_t Tcp_GetNextInitialSequenceNumber(void)
{
	static u32_t initialSequenceNumber = 6510;

	initialSequenceNumber += gTcpTicks;       /* XXX */
	return initialSequenceNumber;
}

void Tcp_FreeControlBlock(struct TCP_CONTROL_BLOCK * tcpControlBlock)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(tcpControlBlock!=NULL);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);
	
	SMSC_ASSERT(tcpControlBlock->mState==TCP_STATE_CLOSED);
	SMSC_ASSERT(tcpControlBlock->mUnsentSegments==NULL);
	SMSC_ASSERT(tcpControlBlock->mUnackedSegments==NULL);
	SMSC_ASSERT(tcpControlBlock->mOutOfSequenceSegments==NULL);
	
	MemoryPool_Free(gTcpControlBlockPool,tcpControlBlock);
}

struct TCP_CONTROL_BLOCK * Tcp_AllocateControlBlock(u8_t priority)
{
	struct TCP_CONTROL_BLOCK * tcpControlBlock;
	u32_t initialSequenceNumber;

	tcpControlBlock = MemoryPool_Allocate(gTcpControlBlockPool);
	if (tcpControlBlock == NULL) {
		/* Try killing oldest connection in TIME-WAIT. */
		SMSC_NOTICE(TCP_DEBUG, ("TCP() killing off oldest TIME-WAIT connection"));
		Tcp_KillOldestTimeWaitBlock();
		tcpControlBlock = MemoryPool_Allocate(gTcpControlBlockPool);
		if (tcpControlBlock == NULL) {
			Tcp_KillOldestBlockWithLowerPriority(priority);    
			tcpControlBlock = MemoryPool_Allocate(gTcpControlBlockPool);
		}
	}
	if (tcpControlBlock != NULL) {
		memset(tcpControlBlock, 0, sizeof(struct TCP_CONTROL_BLOCK));
		ASSIGN_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);
		tcpControlBlock->mPriority = TCP_PRIORITY_NORMAL;
		tcpControlBlock->mAvailableSendSpace = TCP_SEND_SPACE;
		/*tcpControlBlock->snd_queuelen = 0;*/
		tcpControlBlock->mReceiverWindow = TCP_WINDOW_SIZE;
		/*tcpControlBlock->mTypeOfService = 0;*/
		/*tcpControlBlock->mTimeToLive = TCP_TIME_TO_LIVE;*/
		tcpControlBlock->mMaximumSegmentSize = TCP_MAXIMUM_SEGMENT_SIZE;
		tcpControlBlock->mRetransmissionTimeOut = 3000 / TCP_SLOW_INTERVAL;
		tcpControlBlock->mScaledAverage = 0;
		tcpControlBlock->mScaledDeviation = 3000 / TCP_SLOW_INTERVAL;
		tcpControlBlock->mRetransmissionTimer = 0;
		tcpControlBlock->mCongestionWindow = TCP_MAXIMUM_SEGMENT_SIZE; /* previously set to 1 */
		initialSequenceNumber = Tcp_GetNextInitialSequenceNumber();
		tcpControlBlock->mSenderLastAcknowledgementNumber = initialSequenceNumber;
		tcpControlBlock->mSenderNextSequenceNumber = initialSequenceNumber;
		tcpControlBlock->mSenderHighestSentSequenceNumber = initialSequenceNumber;
		tcpControlBlock->mLastAcknowledgedSequenceNumber = initialSequenceNumber;
		tcpControlBlock->mSenderNumberByteToBeBuffered = initialSequenceNumber;   
		tcpControlBlock->mTimer = gTcpTicks;

		/*tcpControlBlock->polltmr = 0;*/

		tcpControlBlock->mReceiveCallBack = Tcp_NullReceiverCallBack;

		/* Init KEEPALIVE timer */
		tcpControlBlock->mKeepAliveTimer = TCP_KEEPDEFAULT;
		tcpControlBlock->mKeepAliveCounter = 0;
	}
	return tcpControlBlock;
}

static err_t tcp_accept_null(void *arg, struct TCP_CONTROL_BLOCK * tcpControlBlock, err_t error)
{
	(void)arg;
	(void)tcpControlBlock;
	(void)error;
    SMSC_WARNING(TCP_DEBUG,("tcp_accept_null: custom accept function not installed"));
    SMSC_FUNCTION_PARAMS_CHECK_RETURN(tcpControlBlock!=NULL,ERR_ARG);
    CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);
	return ERR_ABRT;
}

/**
 * Set the state of the connection to be LISTEN, which means that it
 * is able to accept incoming connections. The protocol control block
 * is reallocated in order to consume less memory. Setting the
 * connection to LISTEN is an irreversible process.
 *
 */
void Tcp_Listen(struct TCP_CONTROL_BLOCK * tcpControlBlock)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(tcpControlBlock!=NULL);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);
	
	/* already listening? */
	if (tcpControlBlock->mState == TCP_STATE_LISTEN) {
		SMSC_NOTICE(TCP_DEBUG,("TCP(%p) Already Listening",(void *)tcpControlBlock));
		return;
	}
	SMSC_ASSERT(tcpControlBlock->mState==TCP_STATE_CLOSED);
	tcpControlBlock->mState = TCP_STATE_LISTEN;
	tcpControlBlock->mSocketOptions |= SOCKET_OPTION_FLAG_ACCEPTCONN;
	if(tcpControlBlock->mAcceptCallBack==NULL) {
		tcpControlBlock->mAcceptCallBack = tcp_accept_null;
	}
	TCP_REGISTER_CONTROL_BLOCK(&gTcpListenerList, tcpControlBlock);
	SMSC_TRACE(TCP_DEBUG,("TCP(%p) is now Listening",(void *)tcpControlBlock));
}

/**
 * This function should be called by the application when it has
 * processed the data. The purpose is to advertise a larger window
 * when the data has been processed.
 *
 */
void Tcp_AcknowledgeReceivedData(struct TCP_CONTROL_BLOCK * tcpControlBlock, u32_t length)
{
	u16_t length16;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(tcpControlBlock!=NULL);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);
	SMSC_ASSERT(length<0x10000);/* currently not designed to take 32 bit length */
	length16=(u16_t)length;
	if (((u32_t)tcpControlBlock->mReceiverWindow + (u32_t)length16) > TCP_WINDOW_SIZE) {
		tcpControlBlock->mReceiverWindow = TCP_WINDOW_SIZE;
	} else {
		tcpControlBlock->mReceiverWindow += length16;
	}
	if (!(tcpControlBlock->mFlags & TCB_FLAG_ACK_DELAY) &&
		!(tcpControlBlock->mFlags & TCB_FLAG_ACK_NOW)) 
	{
		/*
		 * We send an ACK here (if one is not already pending, hence
		 * the above tests) as Tcp_AcknowledgeReceivedData() implies that the application
		 * has processed some data, and so we can open the receiver's
		 * window to allow more to be transmitted.  This could result in
		 * two ACKs being sent for each received packet in some limited cases
		 * (where the application is only receiving data, and is slow to
		 * process it) but it is necessary to guarantee that the sender can
		 * continue to transmit.
		 */
		Tcp_QueueAcknowledgement(tcpControlBlock);
	} 
	else if (tcpControlBlock->mFlags & TCB_FLAG_ACK_DELAY && tcpControlBlock->mReceiverWindow >= TCP_WINDOW_SIZE/2) {
		/* If we can send a window update such that there is a full
		 * segment available in the window, do so now.  This is sort of
		 * nagle-like in its goals, and tries to hit a compromise between
		 * sending acks each time the window is updated, and only sending
		 * window updates when a timer expires.  The "threshold" used
		 * above (currently TCP_WINDOW_SIZE/2) can be tuned to be more or less
		 * aggressive  */
		Tcp_SendAcknowledgementNow(tcpControlBlock);
	}
}

#if TCP_DEBUG
static void Tcp_PrintHeader(u8_t * tcpHeader)
{
	SMSC_PRINT(TCP_DEBUG, ("TCP header:\n"));
	SMSC_PRINT(TCP_DEBUG, ("+-------------------------------+\n"));
	SMSC_PRINT(TCP_DEBUG, ("|    %5"U16_F"      |    %5"U16_F"      | (src port, dest port)\n",
		TCP_HEADER_GET_SOURCE_PORT(tcpHeader),
		TCP_HEADER_GET_DESTINATION_PORT(tcpHeader)));
	SMSC_PRINT(TCP_DEBUG, ("+-------------------------------+\n"));
	SMSC_PRINT(TCP_DEBUG, ("|           %010"U32_F"          | (seq no)\n",
		TCP_HEADER_GET_SEQUENCE_NUMBER(tcpHeader)));
	SMSC_PRINT(TCP_DEBUG, ("+-------------------------------+\n"));
	SMSC_PRINT(TCP_DEBUG, ("|           %010"U32_F"          | (ack no)\n",
		TCP_HEADER_GET_ACKNOWLEDGEMENT_NUMBER(tcpHeader)));
	SMSC_PRINT(TCP_DEBUG, ("+-------------------------------+\n"));
	SMSC_PRINT(TCP_DEBUG, ("| %2"U16_F" |   |%"U16_F"%"U16_F"%"U16_F"%"U16_F"%"U16_F"%"U16_F"|     %5"U16_F"     | (headerLength, flags (",
		TCP_HEADER_GET_DATA_OFFSET(tcpHeader),
		TCP_HEADER_GET_FLAGS(tcpHeader) >> 5 & 1,
		TCP_HEADER_GET_FLAGS(tcpHeader) >> 4 & 1,
		TCP_HEADER_GET_FLAGS(tcpHeader) >> 3 & 1,
		TCP_HEADER_GET_FLAGS(tcpHeader) >> 2 & 1,
		TCP_HEADER_GET_FLAGS(tcpHeader) >> 1 & 1,
		TCP_HEADER_GET_FLAGS(tcpHeader) & 1,
		TCP_HEADER_GET_WINDOW(tcpHeader)));
	Tcp_PrintFlags(TCP_HEADER_GET_FLAGS(tcpHeader));
	SMSC_PRINT(TCP_DEBUG, ("), win)\n"));
	SMSC_PRINT(TCP_DEBUG, ("+-------------------------------+\n"));
	SMSC_PRINT(TCP_DEBUG, ("|    0x%04"X16_F"     |     %5"U16_F"     | (chksum, urgp)\n",
		TCP_HEADER_GET_CHECKSUM(tcpHeader),
		TCP_HEADER_GET_URGENT_POINTER(tcpHeader)));
	SMSC_PRINT(TCP_DEBUG, ("+-------------------------------+\n"));
}


static void Tcp_PrintFlags(u8_t flags)
{
	if (flags & TCP_HEADER_FLAG_FIN) {
		SMSC_PRINT(TCP_DEBUG, ("FIN "));
	}
	if (flags & TCP_HEADER_FLAG_SYN) {
		SMSC_PRINT(TCP_DEBUG, ("SYN "));
	}
	if (flags & TCP_HEADER_FLAG_RST) {
		SMSC_PRINT(TCP_DEBUG, ("RST "));
	}
	if (flags & TCP_HEADER_FLAG_PSH) {
		SMSC_PRINT(TCP_DEBUG, ("PSH "));
	}
	if (flags & TCP_HEADER_FLAG_ACK) {
		SMSC_PRINT(TCP_DEBUG, ("ACK "));
	}
	if (flags & TCP_HEADER_FLAG_URG) {
		SMSC_PRINT(TCP_DEBUG, ("URG "));
	}
	if (flags & TCP_HEADER_FLAG_ECE) {
		SMSC_PRINT(TCP_DEBUG, ("ECE "));
	}
	if (flags & TCP_HEADER_FLAG_CWR) {
		SMSC_PRINT(TCP_DEBUG, ("CWR "));
	}
}


#endif /* TCP_DEBUG */
static const char * Tcp_StateToString(TCP_STATE state)
{
	const char * result="TCP_STATE_UNKNOWN";
	switch (state) {
	case TCP_STATE_CLOSED:
		result="TCP_STATE_CLOSED";
		break;
	case TCP_STATE_LISTEN:
		result="TCP_STATE_LISTEN";
		break;
	case TCP_STATE_SYN_SENT:
		result="TCP_STATE_SYN_SENT";
		break;
	case TCP_STATE_SYN_RCVD:
		result="TCP_STATE_SYN_RCVD";
		break;
	case TCP_STATE_ESTABLISHED:
		result="TCP_STATE_ESTABLISHED";
		break;
	case TCP_STATE_FIN_WAIT_1:
		result="TCP_STATE_FIN_WAIT_1";
		break;
	case TCP_STATE_FIN_WAIT_2:
		result="TCP_STATE_FIN_WAIT_2";
		break;
	case TCP_STATE_CLOSE_WAIT:
		result="TCP_STATE_CLOSE_WAIT";
		break;
	case TCP_STATE_CLOSING:
		result="TCP_STATE_CLOSING";
		break;
	case TCP_STATE_LAST_ACK:
		result="TCP_STATE_LAST_ACK";
		break;
	case TCP_STATE_TIME_WAIT:
		result="TCP_STATE_TIME_WAIT";
		break;
	}
	return result;
}

 void Tcp_PrintControlBlocks(void)
{
	struct TCP_CONTROL_BLOCK * tcpControlBlock;
	SMSC_PRINT(1, ("Active Control Block states:\n"));
	for(tcpControlBlock = gTcpActiveList; tcpControlBlock != NULL; tcpControlBlock = tcpControlBlock->mNext) {
		SMSC_PRINT(1, ("Local port %"U16_F", foreign port %"U16_F" mSenderNextSequenceNumber %"U32_F" mReceiverNextSequenceNumber %"U32_F" state=%s",
				tcpControlBlock->mLocalPort, tcpControlBlock->mRemotePort,
				tcpControlBlock->mSenderNextSequenceNumber, tcpControlBlock->mReceiverNextSequenceNumber,
				Tcp_StateToString(tcpControlBlock->mState)));
	}    
#if 0	
	SMSC_PRINT(1, ("Listen Control Block states:\n"));
	for(tcpControlBlock = gTcpListenerList; tcpControlBlock != NULL; tcpControlBlock = tcpControlBlock->mNext) {
		SMSC_PRINT(1, ("Local port %"U16_F", foreign port %"U16_F" mSenderNextSequenceNumber %"U32_F" mReceiverNextSequenceNumber %"U32_F" state=%s",
				tcpControlBlock->mLocalPort, tcpControlBlock->mRemotePort,
				tcpControlBlock->mSenderNextSequenceNumber, tcpControlBlock->mReceiverNextSequenceNumber,
				Tcp_StateToString(tcpControlBlock->mState)));
	}    
	SMSC_PRINT(1, ("TIME-WAIT Control Block states:\n"));
	for(tcpControlBlock = gTcpTimeWaitList; tcpControlBlock != NULL; tcpControlBlock = tcpControlBlock->mNext) {
		SMSC_PRINT(1, ("Local port %"U16_F", foreign port %"U16_F" mSenderNextSequenceNumber %"U32_F" mReceiverNextSequenceNumber %"U32_F" state=%s",
				tcpControlBlock->mLocalPort, tcpControlBlock->mRemotePort,
				tcpControlBlock->mSenderNextSequenceNumber, tcpControlBlock->mReceiverNextSequenceNumber,
				Tcp_StateToString(tcpControlBlock->mState)));
	}  
#endif	
}
#if SMSC_ERROR_ENABLED
/* Tcp_CheckControlBlock: This function is only used in SMSC_ASSERT statements 
  so we don't need it when SMSC_ERROR_ENABLED is 0 */
static s16_t Tcp_CheckControlBlock(void)
{
	struct TCP_CONTROL_BLOCK * tcpControlBlock;
	for(tcpControlBlock = gTcpActiveList; tcpControlBlock != NULL; tcpControlBlock = tcpControlBlock->mNext) {
		SMSC_ASSERT(tcpControlBlock->mState != TCP_STATE_CLOSED);
		SMSC_ASSERT(tcpControlBlock->mState != TCP_STATE_LISTEN);
		SMSC_ASSERT(tcpControlBlock->mState != TCP_STATE_TIME_WAIT);
	}
	for(tcpControlBlock = gTcpTimeWaitList; tcpControlBlock != NULL; tcpControlBlock = tcpControlBlock->mNext) {
		SMSC_ASSERT(tcpControlBlock->mState == TCP_STATE_TIME_WAIT);
	}
	return 1;
}
#endif

static err_t Tcp_SendControlFlags(struct TCP_CONTROL_BLOCK * tcpControlBlock, u8_t flags)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(tcpControlBlock!=NULL,ERR_ARG);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);
	/* flags, no optdata, no optdatalen */
	return Tcp_QueueControlData(tcpControlBlock, flags, NULL, 0);
}

/**
 * Closes the connection held by the Control Block.
 *
 */
err_t Tcp_Close(struct TCP_CONTROL_BLOCK * tcpControlBlock)
{
	err_t result;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(tcpControlBlock!=NULL,ERR_ARG);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);

#if TCP_DEBUG
	SMSC_NOTICE(TCP_DEBUG, ("TCP(%p) closing in %s state",
		(void *)tcpControlBlock,Tcp_StateToString(tcpControlBlock->mState)));
#endif

	tcpControlBlock->mReceiveCallBack = Tcp_NullReceiverCallBack;
	tcpControlBlock->mSentCallBack=NULL;
	tcpControlBlock->mConnectedCallBack=NULL;
	tcpControlBlock->mAcceptCallBack=NULL;
	tcpControlBlock->mErrorCallBack=NULL;

	switch (tcpControlBlock->mState) {
	case TCP_STATE_CLOSED:
		/* Closing a tcpControlBlock in the CLOSED state might seem erroneous,
		* however, it is in this state once allocated and as yet unused
		* and the user needs some way to free it should the need arise.
		* Calling Tcp_Close() with a tcpControlBlock that has already been closed, (i.e. twice)
		* or for a tcpControlBlock that has been used and then entered the CLOSED state 
		* is erroneous, but this should never happen as the tcpControlBlock has in those cases
		* been freed, and so any remaining handles are bogus. */
		result = ERR_OK;
		MemoryPool_Free(gTcpControlBlockPool, tcpControlBlock);
		tcpControlBlock = NULL;
		break;
	case TCP_STATE_LISTEN:
		result = ERR_OK;
		Tcp_RemoveControlBlockFromList(&gTcpListenerList, tcpControlBlock);
		MemoryPool_Free(gTcpControlBlockPool, tcpControlBlock);
		tcpControlBlock = NULL;
		break;
	case TCP_STATE_SYN_SENT:
		result = ERR_OK;
		Tcp_RemoveControlBlockFromList(&gTcpActiveList, tcpControlBlock);
		MemoryPool_Free(gTcpControlBlockPool, tcpControlBlock);
		tcpControlBlock = NULL;
		break;
	case TCP_STATE_SYN_RCVD:
		result = Tcp_SendControlFlags(tcpControlBlock, TCP_HEADER_FLAG_FIN);
		if (result == ERR_OK) {
			tcpControlBlock->mState = TCP_STATE_FIN_WAIT_1;
		}
		break;
	case TCP_STATE_ESTABLISHED:
		result = Tcp_SendControlFlags(tcpControlBlock, TCP_HEADER_FLAG_FIN);
		if (result == ERR_OK) {
			tcpControlBlock->mState = TCP_STATE_FIN_WAIT_1;
		}
		break;
	case TCP_STATE_CLOSE_WAIT:
		result = Tcp_SendControlFlags(tcpControlBlock, TCP_HEADER_FLAG_FIN);
		if (result == ERR_OK) {
			tcpControlBlock->mState = TCP_STATE_LAST_ACK;
		}
		break;
	default:
		/* Has already been closed, do nothing. */
		result = ERR_OK;
		tcpControlBlock = NULL;
		break;
	}

	if ((tcpControlBlock != NULL) && (result == ERR_OK)) {
		result = Tcp_SendQueuedData(tcpControlBlock);
	}
	return result;
}

static err_t Tcp_NullReceiverCallBack(void *argument, struct TCP_CONTROL_BLOCK * tcpControlBlock, PPACKET_BUFFER packet, err_t error)
{
	argument = argument;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(tcpControlBlock!=NULL,ERR_ARG);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);
	if (packet != NULL) {
		PacketBuffer_DecreaseReference(packet);
	} else if (error == ERR_OK) {
		return Tcp_Close(tcpControlBlock);
	}
	return ERR_OK;
}

/* Application interface functions */
struct TCP_CONTROL_BLOCK * Tcp_NewControlBlock(void)
{
	return Tcp_AllocateControlBlock(TCP_PRIORITY_NORMAL);
}

/**
 * Binds the connection to a local portnumber and IP address. If the
 * IP address is not given (i.e., ipaddr == NULL), the IP address of
 * the outgoing network interface is used instead.
 *
 */
err_t Tcp_Bind(struct TCP_CONTROL_BLOCK * tcpControlBlock, PIP_ADDRESS ipAddress, u16_t port)
{
	struct TCP_CONTROL_BLOCK * tempControlBlock;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(tcpControlBlock!=NULL,ERR_ARG);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);

	if (port == 0) {
		port = Tcp_GetNewPortNumber();
	}

	/* Check if the address already is in use. */
	for(tempControlBlock = gTcpListenerList; tempControlBlock != NULL; tempControlBlock = tempControlBlock->mNext) 
	{
		if (tempControlBlock->mLocalPort == port) {
			if (IP_ADDRESS_IS_ANY(&(tempControlBlock->mLocalAddress)) ||
				IP_ADDRESS_IS_ANY(ipAddress) ||
				IP_ADDRESS_COMPARE(&(tempControlBlock->mLocalAddress), ipAddress)) 
			{
				#if SMSC_WARNING_ENABLED
				char addressString1[IP_ADDRESS_STRING_SIZE];
				char addressString2[IP_ADDRESS_STRING_SIZE];
				SMSC_WARNING(TCP_DEBUG,("Tcp_Bind: %s:%"U16_F" in use by listener %s:%"U16_F,
					IP_ADDRESS_TO_STRING(addressString1,ipAddress),port,
					IP_ADDRESS_TO_STRING(addressString2,&(tempControlBlock->mLocalAddress)),tempControlBlock->mLocalPort));
				#endif
				return ERR_USE;
			}
		}
	}

	for(tempControlBlock = gTcpActiveList;
		tempControlBlock != NULL; tempControlBlock = tempControlBlock->mNext) 
	{
		if (tempControlBlock->mLocalPort == port) {
			if (IP_ADDRESS_IS_ANY(&(tempControlBlock->mLocalAddress)) ||
				IP_ADDRESS_IS_ANY(ipAddress) ||
				IP_ADDRESS_COMPARE(&(tempControlBlock->mLocalAddress), ipAddress)) 
			{
				#if SMSC_WARNING_ENABLED
				char addressString1[IP_ADDRESS_STRING_SIZE];
				char addressString2[IP_ADDRESS_STRING_SIZE];
				SMSC_WARNING(TCP_DEBUG,("Tcp_Bind: %s:%"U16_F" in use by %s:%"U16_F,
					IP_ADDRESS_TO_STRING(addressString1,ipAddress),port,
					IP_ADDRESS_TO_STRING(addressString2,&(tempControlBlock->mLocalAddress)),tempControlBlock->mLocalPort));
				#endif
				return ERR_USE;
			}
		}
	}

	if (!IP_ADDRESS_IS_ANY(ipAddress)) {
		IP_ADDRESS_COPY(&(tcpControlBlock->mLocalAddress),ipAddress);
	}
	tcpControlBlock->mLocalPort = port;
	#if SMSC_TRACE_ENABLED
	{
		char addressString[IP_ADDRESS_STRING_SIZE];
		SMSC_TRACE(TCP_DEBUG, ("TCP(%p) binded to %s:%"U16_F,
			(void *)tcpControlBlock, IP_ADDRESS_TO_STRING(addressString,&(tcpControlBlock->mLocalAddress)), port));
	}
	#endif
	return ERR_OK;
}

/**
 * Used for specifying the function that should be called when a
 * LISTENing connection has been connected to another host.
 *
 */ 
void Tcp_SetAcceptCallBack(struct TCP_CONTROL_BLOCK * tcpControlBlock, TCP_ACCEPT_CALLBACK callBack)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(tcpControlBlock!=NULL);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);
	
	tcpControlBlock->mAcceptCallBack = callBack;
}

/**
 * Used to specify the function that should be called when a fatal error
 * has occured on the connection.
 *
 */ 
void Tcp_SetErrorCallBack(struct TCP_CONTROL_BLOCK * tcpControlBlock, TCP_ERROR_CALLBACK callBack)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(tcpControlBlock!=NULL);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);
	
	tcpControlBlock->mErrorCallBack = callBack;
}

/**
 * Used to specify the function that should be called when more data can be sent
 */
void Tcp_SetSentCallBack(struct TCP_CONTROL_BLOCK * tcpControlBlock, TCP_SENT_CALLBACK callBack)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(tcpControlBlock!=NULL);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);
	
	tcpControlBlock->mSentCallBack=callBack;
}

/**
 * Enqueue either data or TCP options (but not both) for tranmission
 * 
 * @arg tcpControlBlock Protocol control block for the TCP connection to enqueue data for.
 * @arg flags
 * @arg optionData
 * @arg optionLength
 */
static err_t Tcp_QueueControlData(
	struct TCP_CONTROL_BLOCK * tcpControlBlock,
	u8_t flags,
	u8_t *optionData, u8_t optionLength)
{
	PTCP_SEGMENT segment, tempSegment, segmentQueue;
	u32_t sequenceNumber;
	u16_t segmentLength;
        
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(tcpControlBlock!=NULL,ERR_ARG);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);
	
	/*SMSC_TRACE(TCP_DEBUG, ("Tcp_QueueControlData(tcpControlBlock=%p, flags=%"X16_F")",
		(void *)tcpControlBlock, (u16_t)flags));*/
	
	/* sequenceNumber will be the sequence number of the first segment enqueued
	* by the call to this function. */
	sequenceNumber = tcpControlBlock->mSenderNumberByteToBeBuffered;

	/* First, break up the data into segments and tuck them together in
	* the local "segmentQueue" variable. */
	segmentQueue = segment = NULL;
	{
		/* only option data is support so segment length is zero */
		segmentLength = 0;

		/* Allocate memory for TCP_SEGMENT, and fill in fields. */
		segment = Tcp_AllocateSegment();
		if (segment == NULL) {
			SMSC_NOTICE(TCP_DEBUG, ("Tcp_QueueControlData: could not allocate memory for TCP_SEGMENT"));
			goto MEMORY_ERROR;
		}
		segment->mNext = NULL;
		segment->mPacket = NULL;

		segmentQueue = segment;

		if ((segment->mPacket = Tcp_AllocateDataBuffers(tcpControlBlock,optionLength)) == NULL) {
			goto MEMORY_ERROR;
		}
		segment->mDataPointer = PacketBuffer_GetStartPoint(segment->mPacket);

		segment->mLength = segmentLength;

		/* build TCP header */
		if (PacketBuffer_MoveStartPoint(segment->mPacket, -TCP_HEADER_LENGTH)!=ERR_OK) {
			SMSC_ERROR(("Tcp_QueueControlData: no room for TCP header in PACKET_BUFFER."));
		}
		segment->mTcpHeader=PacketBuffer_GetStartPoint(segment->mPacket);
		TCP_HEADER_SET_SOURCE_PORT(segment->mTcpHeader,tcpControlBlock->mLocalPort);
		TCP_HEADER_SET_DESTINATION_PORT(segment->mTcpHeader,tcpControlBlock->mRemotePort);
		TCP_HEADER_SET_SEQUENCE_NUMBER(segment->mTcpHeader,sequenceNumber);
		TCP_HEADER_SET_URGENT_POINTER(segment->mTcpHeader,0);
		TCP_HEADER_SET_FLAGS(segment->mTcpHeader,flags);
		/* don't fill in ACKNOWLEDGEMENT_NUMBER and WINDOW until later */
                                 
		/* Copy the options into the header, if they are present. */
		if (optionData == NULL) {
			TCP_HEADER_SET_DATA_OFFSET(segment->mTcpHeader,5);
		} else {
			SMSC_ASSERT((optionLength&0x03)==0);/*optlen must be 4 byte aligned */
			TCP_HEADER_SET_DATA_OFFSET(segment->mTcpHeader, (5 + optionLength / 4));
			/* Copy options into data portion of segment.
			Options can thus only be sent in non data carrying
			segments such as SYN|ACK. */
			memcpy(segment->mDataPointer, optionData, optionLength);
		}
		/*SMSC_TRACE(TCP_DEBUG, ("Tcp_QueueControlData: queueing %"U32_F":%"U32_F" (0x%"X16_F")",
			TCP_HEADER_GET_SEQUENCE_NUMBER(segment->mTcpHeader),
			TCP_HEADER_GET_SEQUENCE_NUMBER(segment->mTcpHeader)+TCP_GET_TCP_LENGTH(segment),
			(u16_t)flags));*/

		sequenceNumber += segmentLength;
	}

	/* Now that the data to be enqueued has been broken up into TCP
	segments in the segmentQueue variable, we add them to the end of the
	tcpControlBlock->mUnsentSegments queue. */
	if (tcpControlBlock->mUnsentSegments == NULL) {
		tempSegment = NULL;
	}
	else {
		for (tempSegment = tcpControlBlock->mUnsentSegments; 
			tempSegment->mNext != NULL; tempSegment = tempSegment->mNext);
	}
	/* { tempSegment is last segment on the mUnsentSegments queue, NULL if list is empty } */

	/* empty list */
	if (tempSegment == NULL) {
		/* initialize list with this segment */
		tcpControlBlock->mUnsentSegments = segmentQueue;
	} else {
		/* enqueue segment */
		tempSegment->mNext = segmentQueue;
	}
	if ((flags & TCP_HEADER_FLAG_SYN) || (flags & TCP_HEADER_FLAG_FIN)) {
		tcpControlBlock->mSenderNumberByteToBeBuffered++;
	}
	
	/* Set the PSH flag in the last segment that we enqueued, but only
	if the segment has data (indicated by segmentLength > 0). */
	if ((segment != NULL) && (segmentLength > 0) && (segment->mTcpHeader != NULL)) {
		TCP_HEADER_SET_FLAG(segment->mTcpHeader,TCP_HEADER_FLAG_PSH);
	}

	return ERR_OK;
MEMORY_ERROR:

	if (segmentQueue != NULL) {
		Tcp_FreeSegmentList(segmentQueue);
	}
	SMSC_NOTICE(TCP_DEBUG, ("Tcp_QueueControlData: with memory error"));
	return ERR_MEM;
}

/*
 * Tcp_SetCallBackArgument():
 *
 * Used to specify the argument that should be passed callback
 * functions.
 *
 */ 

void Tcp_SetCallBackArgument(struct TCP_CONTROL_BLOCK * tcpControlBlock, void *arg)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(tcpControlBlock!=NULL);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);
	
	tcpControlBlock->mCallbackArgument = arg;
}

/**
 * Used to specify the function that should be called when a TCP
 * connection receives data.
 *
 */ 
void Tcp_SetReceiveCallBack(struct TCP_CONTROL_BLOCK * tcpControlBlock,TCP_RECEIVE_CALLBACK callBack)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(tcpControlBlock!=NULL);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);
	
	tcpControlBlock->mReceiveCallBack = callBack;
}

/* find out what we can send and send it */
err_t CH_Tcp_SendQueuedDataNow(struct TCP_CONTROL_BLOCK * tcpControlBlock)
{
	PPACKET_BUFFER packet;
	u8_t * tcpHeader;
	PTCP_SEGMENT segment, tempSegment;
	PTCP_SEGMENT outofsegment;
	u32_t window;
#if TCP_DEBUG
	s16_t index = 0;
#endif /* TCP_DEBUG */

	SMSC_FUNCTION_PARAMS_CHECK_RETURN(tcpControlBlock!=NULL,ERR_ARG);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);



	window = SMSC_MINIMUM(tcpControlBlock->mSenderWindow, tcpControlBlock->mCongestionWindow);

	segment = tcpControlBlock->mUnsentSegments;
       outofsegment = tcpControlBlock->mOutOfSequenceSegments;

	   
	/* tempSegment should point to last segment on mUnackedSegments queue */
	tempSegment = tcpControlBlock->mUnackedSegments;
	if (tempSegment != NULL) {
		for (; tempSegment->mNext != NULL; tempSegment = tempSegment->mNext);
	}                                                                             

	/* If the TCB_FLAG_ACK_NOW flag is set and no data will be sent (either
	* because the ->mUnsentSegments queue is empty or because the window does
	* not allow it), construct an empty ACK segment and send it.
	*
	* If data is to be sent, we will just piggyback the ACK (see below).
	*/
	(tcpControlBlock)->mFlags |= TCB_FLAG_ACK_NOW;
	if (1/*(tcpControlBlock->mFlags & TCB_FLAG_ACK_NOW) &&
		((segment == NULL) ||
		(TCP_HEADER_GET_SEQUENCE_NUMBER(segment->mTcpHeader) - tcpControlBlock->mLastAcknowledgedSequenceNumber + segment->mLength) > window)*/) 
	{
		packet = Tcp_AllocateHeaderOnlyPacket(&(tcpControlBlock->mRemoteAddress));
		if (packet == NULL) {
			SMSC_NOTICE(TCP_DEBUG, ("Tcp_SendQueuedData: (ACK) could not allocate pbuf"));
			return ERR_BUF;
		}
		/*SMSC_TRACE(TCP_DEBUG, ("Tcp_SendQueuedData: sending ACK for %"U32_F, tcpControlBlock->mReceiverNextSequenceNumber));*/
		/* remove ACK flags from the Control Block, as we send an empty ACK now */
		tcpControlBlock->mFlags &= ~(TCB_FLAG_ACK_DELAY | TCB_FLAG_ACK_NOW);

		tcpHeader=PacketBuffer_GetStartPoint(packet);
		TCP_HEADER_SET_SOURCE_PORT(tcpHeader,tcpControlBlock->mLocalPort);
		TCP_HEADER_SET_DESTINATION_PORT(tcpHeader,tcpControlBlock->mRemotePort);
		TCP_HEADER_SET_SEQUENCE_NUMBER(tcpHeader,tcpControlBlock->mSenderNextSequenceNumber);

		TCP_HEADER_SET_ACKNOWLEDGEMENT_NUMBER(tcpHeader,tcpControlBlock->mReceiverNextSequenceNumber);
		//	Tcp_PrintControlBlocks();
		TCP_HEADER_SET_FLAGS(tcpHeader,TCP_HEADER_FLAG_ACK);
		TCP_HEADER_SET_WINDOW(tcpHeader,tcpControlBlock->mReceiverWindow);
		TCP_HEADER_SET_URGENT_POINTER(tcpHeader,0);
		TCP_HEADER_SET_DATA_OFFSET(tcpHeader,5);
		/*SMSC_TRACE(TCP_DEBUG,("TCP(%p): Sending ACK = %"U32_F,(void *)tcpControlBlock,tcpControlBlock->mReceiverNextSequenceNumber));*/
		/* make sure the following typecast of acceptable */
		SMSC_ASSERT(!(packet->mTotalLength&0xFFFF0000));
		TCP_HEADER_SET_CHECKSUM(tcpHeader,0);
		TCP_HEADER_SET_CHECKSUM(tcpHeader,
			Ip_PseudoCheckSum(packet,&(tcpControlBlock->mLocalAddress), &(tcpControlBlock->mRemoteAddress),
				IP_PROTOCOL_TCP, ((u16_t)(packet->mTotalLength))));
		Ip_Output(packet, &(tcpControlBlock->mLocalAddress), &(tcpControlBlock->mRemoteAddress),IP_PROTOCOL_TCP);
		return ERR_OK;
	}
#if 0
	/* data available and window allows it to be sent? */
	while ((segment != NULL) &&
		(TCP_HEADER_GET_SEQUENCE_NUMBER(segment->mTcpHeader) - tcpControlBlock->mLastAcknowledgedSequenceNumber + segment->mLength) <= window) 
	{

		tcpControlBlock->mUnsentSegments = segment->mNext;

		if (tcpControlBlock->mState != TCP_STATE_SYN_SENT) {
			TCP_HEADER_SET_FLAG(segment->mTcpHeader, TCP_HEADER_FLAG_ACK);
			/*SMSC_TRACE(TCP_DEBUG,("TCP(%p) Setting ACK flag of outgoing segment",(void *)tcpControlBlock));*/
			tcpControlBlock->mFlags &= ~(TCB_FLAG_ACK_DELAY | TCB_FLAG_ACK_NOW);
		}

		Tcp_SendSegment(segment, tcpControlBlock);
		tcpControlBlock->mSenderNextSequenceNumber = TCP_HEADER_GET_SEQUENCE_NUMBER(segment->mTcpHeader) + TCP_GET_TCP_LENGTH(segment);
		if (TCP_SEQUENCE_LESS_THAN(tcpControlBlock->mSenderHighestSentSequenceNumber, tcpControlBlock->mSenderNextSequenceNumber)) {
			tcpControlBlock->mSenderHighestSentSequenceNumber = tcpControlBlock->mSenderNextSequenceNumber;
		}
		/* put segment on unacknowledged list if length > 0 */
		if (TCP_GET_TCP_LENGTH(segment) > 0) {
			segment->mNext = NULL;
			/* mUnackedSegments list is empty? */
			if (tcpControlBlock->mUnackedSegments == NULL) {
				tcpControlBlock->mUnackedSegments = segment;
				tempSegment = segment;
				/* mUnackedSegments list is not empty? */
			} else {
				/* In the case of fast retransmit, the packet should not go to the tail
				* of the mUnackedSegments queue, but rather at the head. We need to check for
				* this case. -STJ Jul 27, 2004 */
				if (TCP_SEQUENCE_LESS_THAN(TCP_HEADER_GET_SEQUENCE_NUMBER(segment->mTcpHeader), 
							TCP_HEADER_GET_SEQUENCE_NUMBER(tempSegment->mTcpHeader))) 
				{
					/* add segment to head of mUnackedSegments list */
					segment->mNext = tcpControlBlock->mUnackedSegments;
					tcpControlBlock->mUnackedSegments = segment;
				} else {
					/* add segment to tail of mUnackedSegments list */
					tempSegment->mNext = segment;
					tempSegment = tempSegment->mNext;
				}
			}
		/* do not queue empty segments on the mUnackedSegments list */
		} else {
			Tcp_FreeSegment(segment);
		}
		segment = tcpControlBlock->mUnsentSegments;
	}
#endif	
	return ERR_OK;
	
}


/* find out what we can send and send it */
err_t Tcp_SendQueuedData(struct TCP_CONTROL_BLOCK * tcpControlBlock)
{
	PPACKET_BUFFER packet;
	u8_t * tcpHeader;
	PTCP_SEGMENT segment, tempSegment;
	u32_t window;
#if TCP_DEBUG
	s16_t index = 0;
#endif /* TCP_DEBUG */

	SMSC_FUNCTION_PARAMS_CHECK_RETURN(tcpControlBlock!=NULL,ERR_ARG);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);

	/* First, check if we are invoked by the TCP input processing
	code. If so, we do not output anything. Instead, we rely on the
	input processing code to call us when input processing is done
	with. */
	if (gInputControlBlock == tcpControlBlock  ) 
	{
		return ERR_OK;
	}

	window = SMSC_MINIMUM(tcpControlBlock->mSenderWindow, tcpControlBlock->mCongestionWindow);

	segment = tcpControlBlock->mUnsentSegments;
	   
	/* tempSegment should point to last segment on mUnackedSegments queue */
	tempSegment = tcpControlBlock->mUnackedSegments;
	if (tempSegment != NULL) {
		for (; tempSegment->mNext != NULL; tempSegment = tempSegment->mNext);
	}                                                                             

	/* If the TCB_FLAG_ACK_NOW flag is set and no data will be sent (either
	* because the ->mUnsentSegments queue is empty or because the window does
	* not allow it), construct an empty ACK segment and send it.
	*
	* If data is to be sent, we will just piggyback the ACK (see below).
	*/
	if ((tcpControlBlock->mFlags & TCB_FLAG_ACK_NOW) &&
		((segment == NULL) ||
		(TCP_HEADER_GET_SEQUENCE_NUMBER(segment->mTcpHeader) - tcpControlBlock->mLastAcknowledgedSequenceNumber + segment->mLength) > window)) 
	{
		packet = Tcp_AllocateHeaderOnlyPacket(&(tcpControlBlock->mRemoteAddress));
		if (packet == NULL) {
			SMSC_NOTICE(TCP_DEBUG, ("Tcp_SendQueuedData: (ACK) could not allocate pbuf"));
			return ERR_BUF;
		}
		/*SMSC_TRACE(TCP_DEBUG, ("Tcp_SendQueuedData: sending ACK for %"U32_F, tcpControlBlock->mReceiverNextSequenceNumber));*/
		/* remove ACK flags from the Control Block, as we send an empty ACK now */
		tcpControlBlock->mFlags &= ~(TCB_FLAG_ACK_DELAY | TCB_FLAG_ACK_NOW);

		tcpHeader=PacketBuffer_GetStartPoint(packet);
		TCP_HEADER_SET_SOURCE_PORT(tcpHeader,tcpControlBlock->mLocalPort);
		TCP_HEADER_SET_DESTINATION_PORT(tcpHeader,tcpControlBlock->mRemotePort);
		TCP_HEADER_SET_SEQUENCE_NUMBER(tcpHeader,tcpControlBlock->mSenderNextSequenceNumber);
		TCP_HEADER_SET_ACKNOWLEDGEMENT_NUMBER(tcpHeader,tcpControlBlock->mReceiverNextSequenceNumber);
		//	Tcp_PrintControlBlocks();
		TCP_HEADER_SET_FLAGS(tcpHeader,TCP_HEADER_FLAG_ACK);
		TCP_HEADER_SET_WINDOW(tcpHeader,tcpControlBlock->mReceiverWindow);
		TCP_HEADER_SET_URGENT_POINTER(tcpHeader,0);
		TCP_HEADER_SET_DATA_OFFSET(tcpHeader,5);
		/*SMSC_TRACE(TCP_DEBUG,("TCP(%p): Sending ACK = %"U32_F,(void *)tcpControlBlock,tcpControlBlock->mReceiverNextSequenceNumber));*/
		/* make sure the following typecast of acceptable */
		SMSC_ASSERT(!(packet->mTotalLength&0xFFFF0000));
		TCP_HEADER_SET_CHECKSUM(tcpHeader,0);
		TCP_HEADER_SET_CHECKSUM(tcpHeader,
			Ip_PseudoCheckSum(packet,&(tcpControlBlock->mLocalAddress), &(tcpControlBlock->mRemoteAddress),
				IP_PROTOCOL_TCP, ((u16_t)(packet->mTotalLength))));
		Ip_Output(packet, &(tcpControlBlock->mLocalAddress), &(tcpControlBlock->mRemoteAddress),IP_PROTOCOL_TCP);
		return ERR_OK;
	}

#if TCP_DEBUG
	/*if (segment == NULL) {
		SMSC_NOTICE(TCP_DEBUG, ("Tcp_SendQueuedData: nothing to send (%p)", (void*)tcpControlBlock->mUnsentSegments));
	}*/
	/*if (segment == NULL) {
		SMSC_TRACE(TCP_DEBUG, ("Tcp_SendQueuedData: mSenderWindow %"U32_F", mCongestionWindow %"U16_F", wnd %"U32_F", segment == NULL, ack %"U32_F,
			tcpControlBlock->mSenderWindow, tcpControlBlock->mCongestionWindow, wnd,
			tcpControlBlock->mLastAcknowledgedSequenceNumber));
	} else {
		SMSC_TRACE(TCP_DEBUG, ("Tcp_SendQueuedData: mSenderWindow %"U32_F", mCongestionWindow %"U16_F", wnd %"U32_F", effwnd %"U32_F", seq %"U32_F", ack %"U32_F,
			tcpControlBlock->mSenderWindow, tcpControlBlock->mCongestionWindow, wnd,
			TCP_HEADER_GET_SEQUENCE_NUMBER(segment->mTcpHeader) - tcpControlBlock->mLastAcknowledgedSequenceNumber + segment->len,
			TCP_HEADER_GET_SEQUENCE_NUMBER(segment->mTcpHeader), tcpControlBlock->mLastAcknowledgedSequenceNumber));
	}*/
#endif /* TCP_DEBUG */
	/* data available and window allows it to be sent? */
	while ((segment != NULL) &&
		(TCP_HEADER_GET_SEQUENCE_NUMBER(segment->mTcpHeader) - tcpControlBlock->mLastAcknowledgedSequenceNumber + segment->mLength) <= window) 
	{
#if TCP_DEBUG
		/*SMSC_TRACE(TCP_DEBUG, ("Tcp_SendQueuedData: mSenderWindow %"U32_F", mCongestionWindow %"U16_F", window %"U32_F", effwnd %"U32_F", seq %"U32_F", ack %"U32_F", index %"S16_F,
			tcpControlBlock->mSenderWindow, tcpControlBlock->mCongestionWindow, window,
			TCP_HEADER_GET_SEQUENCE_NUMBER(segment->mTcpHeader) + segment->len -
			tcpControlBlock->mLastAcknowledgedSequenceNumber,
			TCP_HEADER_GET_SEQUENCE_NUMBER(segment->mTcpHeader), tcpControlBlock->mLastAcknowledgedSequenceNumber, index));*/
		++index;
#endif /* TCP_DEBUG */

		tcpControlBlock->mUnsentSegments = segment->mNext;

		if (tcpControlBlock->mState != TCP_STATE_SYN_SENT) {
			TCP_HEADER_SET_FLAG(segment->mTcpHeader, TCP_HEADER_FLAG_ACK);
			/*SMSC_TRACE(TCP_DEBUG,("TCP(%p) Setting ACK flag of outgoing segment",(void *)tcpControlBlock));*/
			tcpControlBlock->mFlags &= ~(TCB_FLAG_ACK_DELAY | TCB_FLAG_ACK_NOW);
		}

		Tcp_SendSegment(segment, tcpControlBlock);
		tcpControlBlock->mSenderNextSequenceNumber = TCP_HEADER_GET_SEQUENCE_NUMBER(segment->mTcpHeader) + TCP_GET_TCP_LENGTH(segment);
		if (TCP_SEQUENCE_LESS_THAN(tcpControlBlock->mSenderHighestSentSequenceNumber, tcpControlBlock->mSenderNextSequenceNumber)) {
			tcpControlBlock->mSenderHighestSentSequenceNumber = tcpControlBlock->mSenderNextSequenceNumber;
		}
		/* put segment on unacknowledged list if length > 0 */
		if (TCP_GET_TCP_LENGTH(segment) > 0) {
			segment->mNext = NULL;
			/* mUnackedSegments list is empty? */
			if (tcpControlBlock->mUnackedSegments == NULL) {
				tcpControlBlock->mUnackedSegments = segment;
				tempSegment = segment;
				/* mUnackedSegments list is not empty? */
			} else {
				/* In the case of fast retransmit, the packet should not go to the tail
				* of the mUnackedSegments queue, but rather at the head. We need to check for
				* this case. -STJ Jul 27, 2004 */
				if (TCP_SEQUENCE_LESS_THAN(TCP_HEADER_GET_SEQUENCE_NUMBER(segment->mTcpHeader), 
							TCP_HEADER_GET_SEQUENCE_NUMBER(tempSegment->mTcpHeader))) 
				{
					/* add segment to head of mUnackedSegments list */
					segment->mNext = tcpControlBlock->mUnackedSegments;
					tcpControlBlock->mUnackedSegments = segment;
				} else {
					/* add segment to tail of mUnackedSegments list */
					tempSegment->mNext = segment;
					tempSegment = tempSegment->mNext;
				}
			}
		/* do not queue empty segments on the mUnackedSegments list */
		} else {
			Tcp_FreeSegment(segment);
		}
		segment = tcpControlBlock->mUnsentSegments;
	}
	return ERR_OK;
}

/**
 * Actually send a TCP segment over IP
 */
static void Tcp_SendSegment(PTCP_SEGMENT segment, struct TCP_CONTROL_BLOCK * tcpControlBlock)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(segment!=NULL);
	CHECK_SIGNATURE(segment,TCP_SEGMENT_SIGNATURE);
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(tcpControlBlock!=NULL);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);
	
	/* The TCP header has already been constructed, but the acknowledgementNumber and
	wnd fields remain. */

	TCP_HEADER_SET_ACKNOWLEDGEMENT_NUMBER(segment->mTcpHeader,tcpControlBlock->mReceiverNextSequenceNumber);
	/* silly window avoidance */
	if (tcpControlBlock->mReceiverWindow < tcpControlBlock->mMaximumSegmentSize) {
		TCP_HEADER_SET_WINDOW(segment->mTcpHeader,0);
	} else {
		/* advertise our receive window size in this TCP segment */
		TCP_HEADER_SET_WINDOW(segment->mTcpHeader,tcpControlBlock->mReceiverWindow);
	}

	if (IP_ADDRESS_IS_ANY(&(tcpControlBlock->mLocalAddress))) {
		/* local ip address is not yet assigned */
		/* get the local ip address */
		Ip_GetSourceAddress(&(tcpControlBlock->mLocalAddress),&(tcpControlBlock->mRemoteAddress));
	}

	tcpControlBlock->mRetransmissionTimer = 0;

	if (tcpControlBlock->mRoundTripTest == 0) {
		tcpControlBlock->mRoundTripTest = gTcpTicks;
		tcpControlBlock->mRoundTripSequenceNumber = TCP_HEADER_GET_SEQUENCE_NUMBER(segment->mTcpHeader);
		/*SMSC_TRACE(TCP_DEBUG, ("Tcp_SendSegment: mRoundTripSequenceNumber %"U32_F, tcpControlBlock->mRoundTripSequenceNumber));*/
	}
	/*SMSC_TRACE(TCP_DEBUG, ("Tcp_SendSegment: %"U32_F":%"U32_F,
		TCP_HEADER_GET_SEQUENCE_NUMBER(segment->mTcpHeader),
		TCP_HEADER_GET_SEQUENCE_NUMBER(segment->mTcpHeader) +
		segment->mLength));*/

	PacketBuffer_RestoreStartPoint(segment->mPacket,segment->mTcpHeader);

    /* make sure the following typecast is acceptable */
	SMSC_ASSERT(!((segment->mPacket->mTotalLength)&0xFFFF0000));
	TCP_HEADER_SET_CHECKSUM(segment->mTcpHeader, 0);
	TCP_HEADER_SET_CHECKSUM(segment->mTcpHeader,
		Ip_PseudoCheckSum(segment->mPacket,
			&(tcpControlBlock->mLocalAddress),
			&(tcpControlBlock->mRemoteAddress),
			IP_PROTOCOL_TCP, (u16_t)(segment->mPacket->mTotalLength)));

	/* Hold a reference to this packet so it is not freed
		when Ip_Output is done with it. */
	PacketBuffer_IncreaseReference(segment->mPacket);

	Ip_Output(segment->mPacket, &(tcpControlBlock->mLocalAddress), &(tcpControlBlock->mRemoteAddress),
		IP_PROTOCOL_TCP);
}

/*
Tcp_QueueData: Queues a packet buffer list
Assumption: packet should be allocated with Tcp_AllocatePacket
	This insures that the packet list has proper size segments.
	Each packet buffer should hold up to a full segment and fit 
	with in a link layer frame. The total length of the list is
	stored in packet->mTotalLength
Returns: In the event that not all buffers can be sent,
	The mUnsentSegments buffers are returned. When all buffers are queued
	for sending then NULL is returned.
*/
PPACKET_BUFFER Tcp_QueueData(struct TCP_CONTROL_BLOCK * tcpControlBlock,PPACKET_BUFFER packet)
{
	PTCP_SEGMENT segment, tempSegment, segmentQueue;
	u32_t sequenceNumber;
	u16_t segmentLength;
	u32_t queuedLength=0;

	SMSC_FUNCTION_PARAMS_CHECK_RETURN(tcpControlBlock!=NULL,NULL);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(packet!=NULL,NULL);
	CHECK_SIGNATURE(packet,PACKET_BUFFER_SIGNATURE);

	SMSC_TRACE(TCP_DEBUG, ("TCP(%p) Queueing Packet = %p",(void *)tcpControlBlock, (void *)packet));

	/* sequenceNumber will be the sequence number of the first segment enqueued
	 * by the call to this function. */
	sequenceNumber = tcpControlBlock->mSenderNumberByteToBeBuffered;

	/* First, break up the data into segments and tuck them together in
	 * the local "segmentQueue" variable. */
	tempSegment = segmentQueue = segment = NULL;
	segmentLength = 0;
	while ((packet!=NULL)&&((packet->mThisLength)<=(tcpControlBlock->mAvailableSendSpace))) {
		CHECK_SIGNATURE(packet,PACKET_BUFFER_SIGNATURE);
		
		SMSC_ASSERT((packet->mThisLength)<=(tcpControlBlock->mMaximumSegmentSize));
		segmentLength = packet->mThisLength;
		
		/* Allocate memory for tcp_seg, and fill in fields. */
		segment = Tcp_AllocateSegment();
		if (segment == NULL) {
			SMSC_NOTICE(TCP_DEBUG, ("(TCP(%p) while queueing could not allocate memory for tcp_seg",
				(void *)tcpControlBlock));
			goto MEMORY_ERROR;
		}
		segment->mNext = NULL;
		segment->mPacket = NULL;

		/* first segment of to-be-queued data? */
		if (segmentQueue == NULL) {
			segmentQueue = segment;
		}
		/* subsequent segments of to-be-queued data */
		else {
			/* Attach the segment to the end of the queued segments */
			SMSC_ASSERT(tempSegment != NULL);
			tempSegment->mNext = segment;
		}
		/* remember last segment of to-be-queued data for next iteration */
		tempSegment = segment;
		
		packet->mTotalLength=packet->mThisLength;
		segment->mPacket=packet;
		packet=packet->mNext;
		segment->mPacket->mNext=NULL;
		segment->mDataPointer=PacketBuffer_GetStartPoint(segment->mPacket);

		segment->mLength = segmentLength;

		/* build TCP header */
		if(PacketBuffer_MoveStartPoint(segment->mPacket, -TCP_HEADER_LENGTH)!=ERR_OK) {
			SMSC_ERROR(("TCP(%p) while queueing no room for TCP header in packet buffer",
				(void *)tcpControlBlock));
		}
		
		segment->mTcpHeader = PacketBuffer_GetStartPoint(segment->mPacket);
		TCP_HEADER_SET_SOURCE_PORT(segment->mTcpHeader,tcpControlBlock->mLocalPort);
		TCP_HEADER_SET_DESTINATION_PORT(segment->mTcpHeader,tcpControlBlock->mRemotePort);
		TCP_HEADER_SET_SEQUENCE_NUMBER(segment->mTcpHeader,sequenceNumber);
		TCP_HEADER_SET_URGENT_POINTER(segment->mTcpHeader,0);
		TCP_HEADER_SET_FLAGS(segment->mTcpHeader,0);
		
		/* don't fill in ACKNOWLEDGEMENT_NUMBER and WINDOW until later */

		TCP_HEADER_SET_DATA_OFFSET(segment->mTcpHeader,5);
		
		SMSC_TRACE(TCP_DEBUG, ("TCP(%p) queueing %"U32_F":%"U32_F,
			(void *)tcpControlBlock,
			TCP_HEADER_GET_SEQUENCE_NUMBER(segment->mTcpHeader),
			TCP_HEADER_GET_SEQUENCE_NUMBER(segment->mTcpHeader) + TCP_GET_TCP_LENGTH(segment)));

		sequenceNumber += segmentLength;
		
		tcpControlBlock->mAvailableSendSpace-=segmentLength;
		queuedLength+=segmentLength;
	}

	/* Now that the data to be enqueued has been broken up into TCP
	segments in the segmentQueue variable, we add them to the end of the
	tcpControlBlock->mUnsentSegments queue. */
	if (tcpControlBlock->mUnsentSegments == NULL) {
		tempSegment = NULL;
	}
	else {
		for (tempSegment = tcpControlBlock->mUnsentSegments; 
			tempSegment->mNext != NULL; tempSegment = tempSegment->mNext);
	}
	/* { tempSegment is last segment on the mUnsentSegments queue, NULL if list is empty } */

	/* empty list */
	if (tempSegment == NULL) {
		/* initialize list with this segment */
		tcpControlBlock->mUnsentSegments = segmentQueue;
	}
	/* enqueue segment */
	else {
		tempSegment->mNext = segmentQueue;
	}
	
	tcpControlBlock->mSenderNumberByteToBeBuffered += queuedLength;

	/* Set the PSH flag in the last segment that we enqueued, but only
	if the segment has data (indicated by segmentLength > 0). */
	if ((segment != NULL) && (segmentLength > 0) && (segment->mTcpHeader != NULL)) {
		TCP_HEADER_SET_FLAG(segment->mTcpHeader, TCP_HEADER_FLAG_PSH);
	}
	
	return packet;
MEMORY_ERROR:
	if (segmentQueue != NULL) {
		Tcp_FreeSegmentList(segmentQueue);
	}
	SMSC_WARNING(TCP_DEBUG, ("TCP(%p) queueing with memory error",
		(void *)tcpControlBlock));
	return packet;
}

/**
 * A nastly hack featuring 'goto' statements that allocates a
 * new TCP local port.
 */
static u16_t Tcp_GetNewPortNumber(void)
{
	struct TCP_CONTROL_BLOCK * tempControlBlock;
	static u16_t port = TCP_LOCAL_PORT_RANGE_START;

TRY_AGAIN:
	if (++port > TCP_LOCAL_PORT_RANGE_END) {
		port = TCP_LOCAL_PORT_RANGE_START;
	}

	for(tempControlBlock = gTcpActiveList; tempControlBlock != NULL; tempControlBlock = tempControlBlock->mNext) {
		if (tempControlBlock->mLocalPort == port) {
			goto TRY_AGAIN;
		}
	}
	for(tempControlBlock = gTcpTimeWaitList; tempControlBlock != NULL; tempControlBlock = tempControlBlock->mNext) {
		if (tempControlBlock->mLocalPort == port) {
			goto TRY_AGAIN;
		}
	}
	for(tempControlBlock = gTcpListenerList; tempControlBlock != NULL; tempControlBlock = tempControlBlock->mNext) {
		if (tempControlBlock->mLocalPort == port) {
			goto TRY_AGAIN;
		}
	}
	return port;
}


/**
 * Is called every TCP_FAST_INTERVAL (250 ms) and sends delayed ACKs.
 */
static void Tcp_FastTimer(void)
{
	struct TCP_CONTROL_BLOCK * tcpControlBlock;
#if 1
	/* send delayed ACKs */  
	for(tcpControlBlock = gTcpActiveList; tcpControlBlock != NULL; tcpControlBlock = tcpControlBlock->mNext) {
		if (tcpControlBlock->mFlags & TCB_FLAG_ACK_DELAY) 
	        {
	              if(tcpControlBlock->mOutOfSequenceSegments != NULL)
	          	{
			SMSC_TRACE(TCP_DEBUG, ("Tcp_FastTimer: delayed ACK"));
			Tcp_SendAcknowledgementNow(tcpControlBlock);
			tcpControlBlock->mFlags &= ~(TCB_FLAG_ACK_DELAY | TCB_FLAG_ACK_NOW);
	          	}
		}
	}
#endif	
}
 
/* requeue all mUnackedSegments for retransmission */
static void Tcp_RetransmitAllSegments(struct TCP_CONTROL_BLOCK * tcpControlBlock)
{
	PTCP_SEGMENT segment;

	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(tcpControlBlock!=NULL);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);

	if (tcpControlBlock->mUnackedSegments == NULL) {
		return;
	}

	/* Move all mUnackedSegments to the head of the mUnsentSegments queue */
	for (segment = tcpControlBlock->mUnackedSegments; segment->mNext != NULL; segment = segment->mNext);
	/* concatenate mUnsentSegments queue after mUnackedSegments queue */
	segment->mNext = tcpControlBlock->mUnsentSegments;
	/* mUnsentSegments queue is the concatenated queue (of mUnackedSegments, mUnsentSegments) */
	tcpControlBlock->mUnsentSegments = tcpControlBlock->mUnackedSegments;
	/* mUnackedSegments queue is now empty */
	tcpControlBlock->mUnackedSegments = NULL;

	tcpControlBlock->mSenderNextSequenceNumber = TCP_HEADER_GET_SEQUENCE_NUMBER(tcpControlBlock->mUnsentSegments->mTcpHeader);
	/* increment number of retransmissions */
	++tcpControlBlock->mNumberOfRetransmissions;

	/* Don't take any RTT measurements after retransmitting. */
	tcpControlBlock->mRoundTripTest = 0;

	/* Do the actual retransmission */
	Tcp_SendQueuedData(tcpControlBlock);
}

static void Tcp_RetransmitOneSegment(struct TCP_CONTROL_BLOCK * tcpControlBlock)
{
	PTCP_SEGMENT segment;

	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(tcpControlBlock!=NULL);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);

	if (tcpControlBlock->mUnackedSegments == NULL) {
		return;
	}

	/* Move the first mUnackedSegments segment to the mUnsentSegments queue */
	segment = tcpControlBlock->mUnackedSegments->mNext;
	tcpControlBlock->mUnackedSegments->mNext = tcpControlBlock->mUnsentSegments;
	tcpControlBlock->mUnsentSegments = tcpControlBlock->mUnackedSegments;
	tcpControlBlock->mUnackedSegments = segment;

	tcpControlBlock->mSenderNextSequenceNumber = TCP_HEADER_GET_SEQUENCE_NUMBER(tcpControlBlock->mUnsentSegments->mTcpHeader);

	++tcpControlBlock->mNumberOfRetransmissions;

	/* Don't take any rtt measurements after retransmitting. */
	tcpControlBlock->mRoundTripTest = 0;

	/* Do the actual retransmission. */
	Tcp_SendQueuedData(tcpControlBlock);
}

static void Tcp_SendKeepAlive(struct TCP_CONTROL_BLOCK * tcpControlBlock)
{
	PPACKET_BUFFER packet;
	u8_t * tcpHeader;

	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(tcpControlBlock!=NULL);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);

	#if SMSC_TRACE_ENABLED
	{
		char addressString[IP_ADDRESS_STRING_SIZE];
		SMSC_TRACE(TCP_DEBUG, ("Tcp_SendKeepAlive: sending KEEPALIVE probe to %s",
			IP_ADDRESS_TO_STRING(addressString,&tcpControlBlock->mRemoteAddress)));
	}
	#endif

	SMSC_TRACE(TCP_DEBUG, ("Tcp_SendKeepAlive: gTcpTicks %"U32_F"   tcpControlBlock->mTimer %"U32_F"  tcpControlBlock->mKeepAliveCounter %"U16_F"\n", gTcpTicks, tcpControlBlock->mTimer, tcpControlBlock->mKeepAliveCounter));

	packet=Tcp_AllocateHeaderOnlyPacket(&(tcpControlBlock->mRemoteAddress));

	if(packet == NULL) {
		SMSC_NOTICE(TCP_DEBUG, ("Tcp_SendKeepAlive: could not allocate memory for packet"));
		return;
	}

	tcpHeader=PacketBuffer_GetStartPoint(packet);
	TCP_HEADER_SET_SOURCE_PORT(tcpHeader,tcpControlBlock->mLocalPort);
	TCP_HEADER_SET_DESTINATION_PORT(tcpHeader,tcpControlBlock->mRemotePort);
	TCP_HEADER_SET_SEQUENCE_NUMBER(tcpHeader,tcpControlBlock->mSenderNextSequenceNumber-1);

	TCP_HEADER_SET_ACKNOWLEDGEMENT_NUMBER(tcpHeader,tcpControlBlock->mReceiverNextSequenceNumber);
	TCP_HEADER_SET_WINDOW(tcpHeader,tcpControlBlock->mReceiverWindow);
	TCP_HEADER_SET_URGENT_POINTER(tcpHeader,0);
	TCP_HEADER_SET_DATA_OFFSET(tcpHeader,5);
	
	/* Make sure following typecast is acceptable */
	SMSC_ASSERT(!((packet->mTotalLength)&0xFFFF0000));
	
	TCP_HEADER_SET_CHECKSUM(tcpHeader,0);
	TCP_HEADER_SET_CHECKSUM(tcpHeader,
		Ip_PseudoCheckSum(packet,
			&tcpControlBlock->mLocalAddress, &tcpControlBlock->mRemoteAddress, 
			IP_PROTOCOL_TCP, (u16_t)(packet->mTotalLength)));

	/* Send output to IP */
	Ip_Output(packet, &tcpControlBlock->mLocalAddress, &tcpControlBlock->mRemoteAddress,IP_PROTOCOL_TCP);

	SMSC_TRACE(TCP_DEBUG, ("Tcp_SendKeepAlive: sequenceNumber %"U32_F" acknowledgementNumber %"U32_F".", tcpControlBlock->mSenderNextSequenceNumber - 1, tcpControlBlock->mReceiverNextSequenceNumber));
}

/**
 * Called every 500 ms and implements the retransmission timer and the timer that
 * removes Control Blocks that have been in TIME-WAIT for enough time. It also increments
 * various timers such as the inactivity timer in each Control Block.
 */
static void Tcp_SlowTimer(void)
{
	struct TCP_CONTROL_BLOCK * tcpControlBlock;
	struct TCP_CONTROL_BLOCK * tempControlBlock;
	struct TCP_CONTROL_BLOCK * previousControlBlock;
	u32_t effectiveWindow;
	u8_t removeControlBlock;      /* flag if a Control Block should be removed */
	err_t error;

	error = ERR_OK;

	++gTcpTicks;

	/* Steps through all of the active Control Blocks. */
	previousControlBlock = NULL;
	tcpControlBlock = gTcpActiveList;
	if (tcpControlBlock == NULL) {
		SMSC_NOTICE(TCP_DEBUG, ("Tcp_SlowTimer: no active control blocks"));
	}
	while (tcpControlBlock != NULL) {
		/*SMSC_TRACE(TCP_DEBUG, ("Tcp_SlowTimer: processing active tcpControlBlock"));*/
		SMSC_ASSERT(tcpControlBlock->mState != TCP_STATE_CLOSED);
		SMSC_ASSERT(tcpControlBlock->mState != TCP_STATE_LISTEN);
		SMSC_ASSERT(tcpControlBlock->mState != TCP_STATE_TIME_WAIT);

		removeControlBlock = 0;

		if ((tcpControlBlock->mState == TCP_STATE_SYN_SENT) && (tcpControlBlock->mNumberOfRetransmissions == TCP_SYNMAXRTX)) {
			++removeControlBlock;
			SMSC_NOTICE(TCP_DEBUG, ("Tcp_SlowTimer: max SYN retries reached"));
		} else if (tcpControlBlock->mNumberOfRetransmissions == TCP_MAXRTX) {
			++removeControlBlock;
			SMSC_NOTICE(TCP_DEBUG, ("Tcp_SlowTimer: max DATA retries reached"));
		} else {
			++tcpControlBlock->mRetransmissionTimer;
			if ((tcpControlBlock->mUnackedSegments != NULL) && (tcpControlBlock->mRetransmissionTimer >= tcpControlBlock->mRetransmissionTimeOut)) {

				/* Time for a retransmission. */
				/*SMSC_TRACE(TCP_DEBUG, ("Tcp_SlowTimer: mRetransmissionTimer %"U16_F" tcpControlBlock->mRetransmissionTimeOut %"U16_F,
					tcpControlBlock->mRetransmissionTimer, tcpControlBlock->mRetransmissionTimeOut));*/

				/* Double retransmission time-out unless we are trying to
				* connect to somebody (i.e., we are in SYN_SENT). */
				if (tcpControlBlock->mState != TCP_STATE_SYN_SENT) {
					tcpControlBlock->mRetransmissionTimeOut = 
						((tcpControlBlock->mScaledAverage >> 3) + tcpControlBlock->mScaledDeviation) 
							<< gTcpBackOff[tcpControlBlock->mNumberOfRetransmissions];
				}
				/* Reduce congestion window and mSlowStartThreshold. */
				effectiveWindow = SMSC_MINIMUM(tcpControlBlock->mCongestionWindow, tcpControlBlock->mSenderWindow);
				tcpControlBlock->mSlowStartThreshold = (u16_t)(effectiveWindow >> 1);
				if (tcpControlBlock->mSlowStartThreshold < tcpControlBlock->mMaximumSegmentSize) {
					tcpControlBlock->mSlowStartThreshold = tcpControlBlock->mMaximumSegmentSize * 2;
				}
				tcpControlBlock->mCongestionWindow = tcpControlBlock->mMaximumSegmentSize;
				/*SMSC_TRACE(TCP_DEBUG, ("Tcp_SlowTimer: mCongestionWindow %"U16_F" mSlowStartThreshold %"U16_F,
					tcpControlBlock->mCongestionWindow, tcpControlBlock->mSlowStartThreshold));*/
 
				/* The following needs to be called AFTER mCongestionWindow is set to one mMaximumSegmentSize - STJ */
				Tcp_RetransmitAllSegments(tcpControlBlock);
			}
		}
		/* Check if this PCB has stayed too long in FIN-WAIT-2 */
		if (tcpControlBlock->mState == TCP_STATE_FIN_WAIT_2) {
			if ((u32_t)(gTcpTicks - tcpControlBlock->mTimer) >
				TCP_FIN_WAIT_TIMEOUT / TCP_SLOW_INTERVAL) 
			{
				++removeControlBlock;
				SMSC_NOTICE(TCP_DEBUG, ("Tcp_SlowTimer: removing tcpControlBlock stuck in FIN-WAIT-2"));
			}
		}

		/* Check if KEEPALIVE should be sent */
		if((tcpControlBlock->mSocketOptions & SOCKET_OPTION_FLAG_KEEPALIVE) && ((tcpControlBlock->mState == TCP_STATE_ESTABLISHED) || (tcpControlBlock->mState == TCP_STATE_CLOSE_WAIT))) {
			if((u32_t)(gTcpTicks - tcpControlBlock->mTimer) > (tcpControlBlock->mKeepAliveTimer + TCP_MAXIDLE) / TCP_SLOW_INTERVAL)  {
      			#if SMSC_NOTICE_ENABLED
      			char addressString[IP_ADDRESS_STRING_SIZE];
				SMSC_NOTICE(TCP_DEBUG, ("Tcp_SlowTimer: KEEPALIVE timeout. Aborting connection to %s.",
					IP_ADDRESS_TO_STRING(addressString,&(tcpControlBlock->mRemoteAddress))));
				#endif
				Tcp_Abort(tcpControlBlock);
			}
			else if((u32_t)(gTcpTicks - tcpControlBlock->mTimer) > (tcpControlBlock->mKeepAliveTimer + ((u32_t)(tcpControlBlock->mKeepAliveCounter)) * TCP_KEEPINTVL) / TCP_SLOW_INTERVAL) {
				Tcp_SendKeepAlive(tcpControlBlock);
				tcpControlBlock->mKeepAliveCounter++;
			}
		}

		/* If this PCB has queued out of sequence data, but has been
			inactive for too long, will drop the data (it will eventually
			be retransmitted). */
		if (tcpControlBlock->mOutOfSequenceSegments != NULL &&
			(u32_t)gTcpTicks - tcpControlBlock->mTimer >=
			((u32_t)(tcpControlBlock->mRetransmissionTimeOut)) * TCP_OUT_OF_SEQUENCE_TIMEOUT) 
		{
			SMSC_NOTICE(TCP_DEBUG,("3Free tcpControlBlock[%p]->mOutOfSequenceSegments[%p]",tcpControlBlock,tcpControlBlock->mOutOfSequenceSegments));
			Tcp_FreeSegmentList(tcpControlBlock->mOutOfSequenceSegments);
			tcpControlBlock->mOutOfSequenceSegments = NULL;
			SMSC_NOTICE(TCP_DEBUG, ("Tcp_SlowTimer: dropping Out Of Sequence queued data"));
		}

		/* Check if this PCB has stayed too long in SYN-RCVD */
		if (tcpControlBlock->mState == TCP_STATE_SYN_RCVD) {
			if ((u32_t)(gTcpTicks - tcpControlBlock->mTimer) >
				TCP_SYN_RECEIVED_TIMEOUT / TCP_SLOW_INTERVAL) 
			{
				++removeControlBlock;
				SMSC_NOTICE(TCP_DEBUG, ("Tcp_SlowTimer: removing tcpControlBlock stuck in SYN-RCVD"));
			}
		}

		/* Check if this PCB has stayed too long in LAST-ACK */
		if (tcpControlBlock->mState == TCP_STATE_LAST_ACK) {
			if ((u32_t)(gTcpTicks - tcpControlBlock->mTimer) > (2 * TCP_MAXIMUM_SEGMENT_LIFETIME) / TCP_SLOW_INTERVAL) {
				++removeControlBlock;
				SMSC_NOTICE(TCP_DEBUG, ("Tcp_SlowTimer: removing tcpControlBlock stuck in LAST-ACK"));
			}
		}

		/* If the PCB should be removed, do it. */
		if (removeControlBlock) {
			Tcp_PurgeControlBlock(tcpControlBlock);      
			/* Remove PCB from gTcpActiveList list. */
			if (previousControlBlock != NULL) {
				SMSC_ASSERT(tcpControlBlock != gTcpActiveList);
				previousControlBlock->mNext = tcpControlBlock->mNext;
			} else {
				/* This PCB was the first. */
				SMSC_ASSERT(gTcpActiveList == tcpControlBlock);
				gTcpActiveList = tcpControlBlock->mNext;
			}

			if(tcpControlBlock->mErrorCallBack != NULL) {
				((TCP_ERROR_CALLBACK)(tcpControlBlock->mErrorCallBack))(tcpControlBlock->mCallbackArgument,ERR_ABRT);
			} else {
				SMSC_NOTICE(TCP_DEBUG,("Tcp_SlowTimer: connection aborted but no error handler available"));
			}

			tempControlBlock = tcpControlBlock->mNext;
			MemoryPool_Free(gTcpControlBlockPool,tcpControlBlock);
			tcpControlBlock = tempControlBlock;
		} else {
			previousControlBlock = tcpControlBlock;
			tcpControlBlock = tcpControlBlock->mNext;
		}
	}


	/* Steps through all of the TIME_WAIT PCBs. */
	previousControlBlock = NULL;    
	tcpControlBlock = gTcpTimeWaitList;
	while (tcpControlBlock != NULL) {
		SMSC_ASSERT(tcpControlBlock->mState == TCP_STATE_TIME_WAIT);
		removeControlBlock = 0;

		/* Check if this PCB has stayed long enough in TIME_WAIT */
		if ((u32_t)(gTcpTicks - tcpControlBlock->mTimer) > ((2 * TCP_MAXIMUM_SEGMENT_LIFETIME) / TCP_SLOW_INTERVAL)) {
			++removeControlBlock;
		}

		/* If the PCB should be removed, do it. */
		if (removeControlBlock) {
			Tcp_PurgeControlBlock(tcpControlBlock);      
			/* Remove Control Block from gTcpTimeWaitList list. */
			if (previousControlBlock != NULL) {
				SMSC_ASSERT(tcpControlBlock != gTcpTimeWaitList);
				previousControlBlock->mNext = tcpControlBlock->mNext;
			} else {
				/* This Control Block was the first. */
				SMSC_ASSERT(gTcpTimeWaitList == tcpControlBlock);
				gTcpTimeWaitList = tcpControlBlock->mNext;
			}
			tempControlBlock = tcpControlBlock->mNext;
			MemoryPool_Free(gTcpControlBlockPool,tcpControlBlock);
			tcpControlBlock = tempControlBlock;
		} else {
			previousControlBlock = tcpControlBlock;
			tcpControlBlock = tcpControlBlock->mNext;
		}
	}
}

/**
 * Called periodically to dispatch TCP timers.
 *
 */
static void Tcp_MainTimer(void)
{
	/* Call Tcp_FastTimer() every 250 ms */
	
	Tcp_FastTimer();
	if (++gTcpTimer & 1) {
		/* Call Tcp_MainTimer() every 500 ms, i.e., every other timer
			Tcp_MainTimer() is called. */
	
		Tcp_SlowTimer();
	}
}

static void Tcp_TimerCallBack(void *arg)
{
	(void)arg;

	/* call TCP timer handler */
	Tcp_MainTimer();
	/* timer still needed? */
	if (gTcpActiveList || gTcpTimeWaitList) {
		/* restart timer */
		TaskManager_ScheduleByTimer(&gTcpTimerTask,TCP_TMR_INTERVAL);
	} else {
		/* disable timer */
		gTcpTimerActive = 0;
	}
}

static void Tcp_ActivateTimer(void)
{
	/* timer is off but needed again? */
	if (!gTcpTimerActive && (gTcpActiveList || gTcpTimeWaitList)) {
		/* enable and start timer */
		gTcpTimerActive = 1;
		TaskManager_ScheduleByTimer(&gTcpTimerTask,TCP_TMR_INTERVAL);
	}
}

/**
 * Connects to another host. The function given as the "connected"
 * argument will be called when the connection has been established.
 *
 */
err_t Tcp_Connect(struct TCP_CONTROL_BLOCK * tcpControlBlock, PIP_ADDRESS ipAddress, u16_t port,
	TCP_CONNECTED_CALLBACK callBack)
{
	u8_t optionData[4];
	err_t result;
	u32_t initialSequenceNumber;

	SMSC_FUNCTION_PARAMS_CHECK_RETURN(tcpControlBlock!=NULL,ERR_ARG);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);
    
    #if SMSC_TRACE_ENABLED
	{
		char addressString[IP_ADDRESS_STRING_SIZE];
		SMSC_TRACE(TCP_DEBUG, ("TCP(%p) connecting to %s:%"U16_F,(void *)tcpControlBlock,
			IP_ADDRESS_TO_STRING(addressString,ipAddress),port));
	}
	#endif
	
	if (ipAddress != NULL) {
		IP_ADDRESS_COPY(&(tcpControlBlock->mRemoteAddress),ipAddress);
	} else {
		SMSC_WARNING(TCP_DEBUG,("Tcp_Connect: invalid ipAddress"));
		return ERR_VAL;
	}
	tcpControlBlock->mRemotePort = port;
	if (tcpControlBlock->mLocalPort == 0) {
		tcpControlBlock->mLocalPort = Tcp_GetNewPortNumber();
	}
	initialSequenceNumber = Tcp_GetNextInitialSequenceNumber();
	tcpControlBlock->mReceiverNextSequenceNumber = 0;
	tcpControlBlock->mSenderNextSequenceNumber = initialSequenceNumber;
	tcpControlBlock->mLastAcknowledgedSequenceNumber = initialSequenceNumber - 1;
	tcpControlBlock->mSenderNumberByteToBeBuffered = initialSequenceNumber - 1;
	tcpControlBlock->mReceiverWindow = TCP_WINDOW_SIZE;
	tcpControlBlock->mSenderWindow = TCP_WINDOW_SIZE;
	tcpControlBlock->mMaximumSegmentSize = TCP_MAXIMUM_SEGMENT_SIZE;
	tcpControlBlock->mCongestionWindow = TCP_MAXIMUM_SEGMENT_SIZE;/* previously set to 1 */
	tcpControlBlock->mSlowStartThreshold = tcpControlBlock->mMaximumSegmentSize * 10;
	tcpControlBlock->mState = TCP_STATE_SYN_SENT;
	tcpControlBlock->mConnectedCallBack = callBack;
	
	/* Add Control Block to list */
	TCP_REGISTER_CONTROL_BLOCK(&gTcpActiveList,tcpControlBlock);

	/* Build an MSS option */
	optionData[0]=2;
	optionData[1]=4;
	optionData[2]=(u8_t)((tcpControlBlock->mMaximumSegmentSize)>>8);
	optionData[3]=(u8_t)((tcpControlBlock->mMaximumSegmentSize)&0x00FF);

	result = Tcp_QueueControlData(tcpControlBlock, TCP_HEADER_FLAG_SYN, optionData, 4);
	if (result == ERR_OK) { 
		Tcp_SendQueuedData(tcpControlBlock);
	} else {
		SMSC_WARNING(TCP_DEBUG,("Tcp_Connect: Tcp_QueueControlData returned %"S16_F,(s16_t)result));
		TCP_REMOVE_CONTROL_BLOCK(&gTcpActiveList,tcpControlBlock);
		tcpControlBlock->mState=TCP_STATE_CLOSED;
	}
	return result;
}

static PPACKET_BUFFER Tcp_AllocateHeaderOnlyPacket(PIP_ADDRESS destinationAddress)
{
	PPACKET_BUFFER newBuffer=
		Ip_AllocatePacket(
				destinationAddress,
				TCP_HEADER_LENGTH);
	if(newBuffer!=NULL) {
		SMSC_ASSERT(newBuffer->mNext==NULL);
	} else {
		SMSC_WARNING(TCP_DEBUG,("Tcp_AllocatePacket: unable to allocate buffer"));
	}
	return newBuffer;
}

u16_t Tcp_GetMaximumSegmentSize(struct TCP_CONTROL_BLOCK * tcpControlBlock)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(tcpControlBlock!=NULL,0);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);
	return tcpControlBlock->mMaximumSegmentSize;
}

PPACKET_BUFFER Tcp_AllocateDataBuffers(struct TCP_CONTROL_BLOCK *  tcpControlBlock, u32_t size)
{
	PPACKET_BUFFER result=NULL;
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(tcpControlBlock!=NULL,NULL);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);

	SMSC_ASSERT(tcpControlBlock->mMaximumSegmentSize!=0);
	SMSC_ASSERT(!IP_ADDRESS_IS_ANY(&(tcpControlBlock->mRemoteAddress)));
	
	if(size==0) {
		result=Ip_AllocatePacket(&(tcpControlBlock->mRemoteAddress),TCP_HEADER_LENGTH);
		if(result!=NULL) {
			if(PacketBuffer_MoveStartPoint(result,TCP_HEADER_LENGTH)!=ERR_OK) {
				SMSC_ERROR(("Tcp_AllocateDataBuffers: unable to move start point"));
				goto MEMORY_ERROR;
			}
		} else {
			SMSC_WARNING(TCP_DEBUG,("Tcp_AllocateDataBuffers: Unable to allocate zero length buffer"));
			goto MEMORY_ERROR;
		}
	} else {
		PPACKET_BUFFER lastBuffer;
		u32_t maximumSegmentSize;

		maximumSegmentSize=(u32_t)(tcpControlBlock->mMaximumSegmentSize);

		lastBuffer=NULL;
		while(size>0) {
			u32_t bufferSize=((maximumSegmentSize<size)?maximumSegmentSize:size);
			PPACKET_BUFFER newBuffer=
				Ip_AllocatePacket(
							&(tcpControlBlock->mRemoteAddress),
							bufferSize+TCP_HEADER_LENGTH);
			if(newBuffer==NULL) {
				SMSC_WARNING(TCP_DEBUG,("Tcp_AllocateDataBuffers: unable to allocate buffer"));
				goto MEMORY_ERROR;
			} else {
				/* a single segment should not be split into multiple ip fragments */
				/* If these assertions fail then the maximum segment size was not set 
					according to the IP MTU */
				SMSC_ASSERT(newBuffer->mNext==NULL);
				SMSC_ASSERT(newBuffer->mTotalLength==newBuffer->mThisLength);
				
				if(PacketBuffer_MoveStartPoint(newBuffer,TCP_HEADER_LENGTH)!=ERR_OK) {
					SMSC_ERROR(("Tcp_AllocateDataBuffers: unable to move start point"));
				}
			}
			newBuffer->mTotalLength=size;
			if(lastBuffer==NULL) {
				lastBuffer=result=newBuffer;
			} else {
				lastBuffer->mNext=newBuffer;
				lastBuffer=newBuffer;
			}
			size-=bufferSize;
		}
	}
	return result;
MEMORY_ERROR:
	/* unable to allocate at least one buffer */
	if(result!=NULL) {
		/* release all buffers that were allocated */
		PacketBuffer_DecreaseReference(result);
	}
	return NULL;
}

/*
 * Tcp_ParseOptions:
 *
 * Parses the options contained in the incoming segment. (Code taken
 * from uIP with only small changes.)
 *
 */

static void Tcp_ParseOptions(struct TCP_CONTROL_BLOCK * tcpControlBlock, u8_t * tcpHeader)
{
	u8_t index;
	u8_t * options, option;
	u16_t maximumSegmentSize;

	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(tcpControlBlock!=NULL);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);

	options = (u8_t *)tcpHeader + TCP_HEADER_LENGTH;

	/* Parse the TCP MSS option, if present. */
	if(TCP_HEADER_GET_DATA_OFFSET(tcpHeader) > 0x5) {
		for(index = 0; index < (TCP_HEADER_GET_DATA_OFFSET(tcpHeader) - 5) << 2 ;) {
			option = options[index];
			if (option == 0x00) {
				/* End of options. */
				break;
			} else if (option == 0x01) {
				++index;
				/* NOP option. */
			} else if ((option == 0x02) && (options[index + 1] == 0x04)) {
				/* An MSS option with the right option length. */
				maximumSegmentSize = (options[index + 2] << 8) | options[index + 3];
				tcpControlBlock->mMaximumSegmentSize = maximumSegmentSize > TCP_MAXIMUM_SEGMENT_SIZE? TCP_MAXIMUM_SEGMENT_SIZE: maximumSegmentSize;

				/* And we are done processing options. */
				break;
			} else {
				if (options[index + 1] == 0) {
					/* If the length field is zero, the options are malformed
					and we don't process them further. */
					break;
				}
				/* All other options have a length field, so that we easily
				can skip past them. */
				index += options[index + 1];
			}
		}
	}
}

/**
 * Returns a copy of the given TCP segment.
 *
 */ 
static PTCP_SEGMENT Tcp_CopySegment(PTCP_SEGMENT segment)
{
	PTCP_SEGMENT copyOfSegment;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(segment!=NULL,NULL);
	CHECK_SIGNATURE(segment,TCP_SEGMENT_SIGNATURE);
	copyOfSegment = Tcp_AllocateSegment();
	if (copyOfSegment == NULL) {
		return NULL;
	}
	memcpy((u8_t *)copyOfSegment, (const u8_t *)segment, sizeof(TCP_SEGMENT));
	PacketBuffer_IncreaseReference(copyOfSegment->mPacket);
	return copyOfSegment;
}

/* TODO need to change the names of these variables so 
  its obvious they are globals. */
static u8_t * gTcpHeader;
static u8_t flags;
static u32_t gSequenceNumber;
static u32_t gAcknowledgementNumber;
static u16_t gTcpLength;
static TCP_SEGMENT gInputSegment;
static u8_t gReceiveFlags;
static PPACKET_BUFFER gReceiveData;
static PIP_ADDRESS gSourceAddress;
static PIP_ADDRESS gDestinationAddress;

/* Tcp_Receive:
 *
 * Called by Tcp_Process. Checks if the given segment is an ACK for outstanding
 * data, and if so frees the memory of the buffered data. Next, is places the
 * segment on any of the receive queues (tcpControlBlock->recved or tcpControlBlock->mOutOfSequenceSegments). If the segment
 * is buffered, the pbuf is referenced by pbuf_ref so that it will not be freed until
 * i it has been removed from the buffer.
 *
 * If the incoming segment constitutes an ACK for a segment that was used for RTT
 * estimation, the RTT is estimated here as well.
 *
 * @return 1 if 
 */
static u8_t Tcp_Receive(struct TCP_CONTROL_BLOCK * tcpControlBlock)
{
	PTCP_SEGMENT nextSegment;
	PTCP_SEGMENT prevSegment, tempSegment;
	PPACKET_BUFFER packet;
	s32_t offset;
	s16_t measurement;
	u32_t rightWindowEdge;
	u16_t newTotalLength;
	u8_t acceptedInputSequence = 0;
       
	SMSC_ASSERT(tcpControlBlock!=NULL);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);

	if (flags & TCP_HEADER_FLAG_ACK) {
		/*SMSC_TRACE(TCP_DEBUG,("TCP(%p) received ACK %"U32_F,(void *)tcpControlBlock,gAcknowledgementNumber));*/
		rightWindowEdge = tcpControlBlock->mSenderWindow + tcpControlBlock->mSenderLastSequenceNumber;

		/* Update window. */
		if (TCP_SEQUENCE_LESS_THAN(tcpControlBlock->mSenderLastSequenceNumber, gSequenceNumber) ||
			((tcpControlBlock->mSenderLastSequenceNumber == gSequenceNumber) && TCP_SEQUENCE_LESS_THAN(tcpControlBlock->mSenderLastAcknowledgementNumber, gAcknowledgementNumber)) ||
			((tcpControlBlock->mSenderLastAcknowledgementNumber == gAcknowledgementNumber) && (TCP_HEADER_READ_WINDOW(gTcpHeader) > tcpControlBlock->mSenderWindow))) 
		{
			tcpControlBlock->mSenderWindow = TCP_HEADER_READ_WINDOW(gTcpHeader);
			tcpControlBlock->mSenderLastSequenceNumber = gSequenceNumber;
			tcpControlBlock->mSenderLastAcknowledgementNumber = gAcknowledgementNumber;
			/*SMSC_TRACE(TCP_DEBUG, ("Tcp_Receive: window update %"U32_F, tcpControlBlock->mSenderWindow));*/
#if TCP_DEBUG
		} else {
			if (tcpControlBlock->mSenderWindow != TCP_HEADER_READ_WINDOW(gTcpHeader)) {
				SMSC_TRACE(TCP_DEBUG, ("TCP(%p) rx no window update mLastAcknowledgedSequenceNumber %"U32_F" mSenderHighestSentSequenceNumber %"U32_F" gAcknowledgementNumber %"U32_F" wl1 %"U32_F" gSequenceNumber %"U32_F" wl2 %"U32_F,
					(void *)tcpControlBlock,tcpControlBlock->mLastAcknowledgedSequenceNumber, tcpControlBlock->mSenderHighestSentSequenceNumber, gAcknowledgementNumber, tcpControlBlock->mSenderLastSequenceNumber, gSequenceNumber, tcpControlBlock->mSenderLastAcknowledgementNumber));
			}
#endif /* TCP_DEBUG */
		}

		if (tcpControlBlock->mLastAcknowledgedSequenceNumber == gAcknowledgementNumber) /*接收不带数据ACK或者重复的ACK*/
			{
			tcpControlBlock->mAcknowledged = 0;

			if ((tcpControlBlock->mSenderLastSequenceNumber + tcpControlBlock->mSenderWindow) == rightWindowEdge){
				++tcpControlBlock->mDuplicateAcks;
				if ((tcpControlBlock->mDuplicateAcks >= 3) && (tcpControlBlock->mUnackedSegments != NULL)) {
					if (!(tcpControlBlock->mFlags & TCB_FLAG_IN_FAST_RECOVERY)) {
						/* This is fast retransmit. Retransmit the first mUnackedSegments segment. */
						SMSC_TRACE(TCP_DEBUG, ("TCP(%p) rx mDuplicateAcks %"U16_F" (%"U32_F"), fast retransmit %"U32_F,
							(void *)tcpControlBlock,(u16_t)tcpControlBlock->mDuplicateAcks, tcpControlBlock->mLastAcknowledgedSequenceNumber,
							TCP_HEADER_GET_SEQUENCE_NUMBER(tcpControlBlock->mUnackedSegments->mTcpHeader)));
						Tcp_RetransmitOneSegment(tcpControlBlock);
						/* Set mSlowStartThreshold to max (FlightSize / 2, 2*SMSS) */
						/*tcpControlBlock->mSlowStartThreshold = LWIP_MAX((tcpControlBlock->mSenderHighestSentSequenceNumber -
							tcpControlBlock->mLastAcknowledgedSequenceNumber) / 2,
							2 * tcpControlBlock->mMaximumSegmentSize);*/
						/* Set mSlowStartThreshold to half of the minimum of the currenct mCongestionWindow and the advertised window */
						if (tcpControlBlock->mCongestionWindow > tcpControlBlock->mSenderWindow)
							tcpControlBlock->mSlowStartThreshold = (u16_t)(tcpControlBlock->mSenderWindow / 2);
						else
							tcpControlBlock->mSlowStartThreshold = tcpControlBlock->mCongestionWindow / 2;

						tcpControlBlock->mCongestionWindow = tcpControlBlock->mSlowStartThreshold + 3 * tcpControlBlock->mMaximumSegmentSize;
						tcpControlBlock->mFlags |= TCB_FLAG_IN_FAST_RECOVERY;
					} else {
						/* Inflate the congestion window, but not if it means that
						the value overflows. */
						if ((u16_t)(tcpControlBlock->mCongestionWindow + tcpControlBlock->mMaximumSegmentSize) > tcpControlBlock->mCongestionWindow) {
							tcpControlBlock->mCongestionWindow += tcpControlBlock->mMaximumSegmentSize;
						}
					}
				}
			} else {
				SMSC_TRACE(TCP_DEBUG, ("TCP(%p) rx dupack averted %"U32_F" %"U32_F,
					(void *)tcpControlBlock, tcpControlBlock->mSenderLastSequenceNumber + tcpControlBlock->mSenderWindow, rightWindowEdge));
			}
		} 
		/*接收到新的携带数据的ACK*/
		 else if (TCP_SEQUENCE_BETWEEN(gAcknowledgementNumber, tcpControlBlock->mLastAcknowledgedSequenceNumber+1, tcpControlBlock->mSenderHighestSentSequenceNumber))
		{
			/* We come here when the ACK acknowledges new data. */
      
			/* Reset the "IN Fast Retransmit" flag, since we are no longer
			in fast retransmit. Also reset the congestion window to the
			slow start threshold. */
			if (tcpControlBlock->mFlags & TCB_FLAG_IN_FAST_RECOVERY) {
				tcpControlBlock->mFlags &= ~TCB_FLAG_IN_FAST_RECOVERY;
				tcpControlBlock->mCongestionWindow = tcpControlBlock->mSlowStartThreshold;
			}

			/* Reset the number of retransmissions. */
			tcpControlBlock->mNumberOfRetransmissions = 0;

			/* Reset the retransmission time-out. */
			tcpControlBlock->mRetransmissionTimeOut = (tcpControlBlock->mScaledAverage >> 3) + tcpControlBlock->mScaledDeviation;

			/* Update the send buffer space. */
			tcpControlBlock->mAcknowledged = (u16_t)(gAcknowledgementNumber - tcpControlBlock->mLastAcknowledgedSequenceNumber);
			
			tcpControlBlock->mAvailableSendSpace +=tcpControlBlock->mAcknowledged;
			
			/* Reset the fast retransmit variables. */
			tcpControlBlock->mDuplicateAcks = 0;
			tcpControlBlock->mLastAcknowledgedSequenceNumber = gAcknowledgementNumber;

			/* Update the congestion control variables (mCongestionWindow and
			mSlowStartThreshold). */
			if (tcpControlBlock->mState >= TCP_STATE_ESTABLISHED) {
				if (tcpControlBlock->mCongestionWindow < tcpControlBlock->mSlowStartThreshold) {
					if ((u16_t)(tcpControlBlock->mCongestionWindow + tcpControlBlock->mMaximumSegmentSize) > tcpControlBlock->mCongestionWindow) {
						tcpControlBlock->mCongestionWindow += tcpControlBlock->mMaximumSegmentSize;
					}
					/*SMSC_TRACE(TCP_DEBUG, ("TCP(%p) rx slow start mCongestionWindow %"U16_F, (void *)tcpControlBlock, tcpControlBlock->mCongestionWindow));*/
				} else {
					u16_t new_cwnd = (tcpControlBlock->mCongestionWindow + tcpControlBlock->mMaximumSegmentSize * tcpControlBlock->mMaximumSegmentSize / tcpControlBlock->mCongestionWindow);
					if (new_cwnd > tcpControlBlock->mCongestionWindow) {
						tcpControlBlock->mCongestionWindow = new_cwnd;
					}
					SMSC_TRACE(TCP_DEBUG, ("TCP(%p) rx congestion avoidance mCongestionWindow %"U16_F, (void *)tcpControlBlock, tcpControlBlock->mCongestionWindow));
				}
			}
			/*SMSC_TRACE(TCP_DEBUG, ("TCP(%p) rx ACK for %"U32_F", mUnackedSegments->gSequenceNumber %"U32_F":%"U32_F,
				(void *)tcpControlBlock,
				gAcknowledgementNumber,
				tcpControlBlock->mUnackedSegments != NULL?
				TCP_HEADER_GET_SEQUENCE_NUMBER(tcpControlBlock->mUnackedSegments->mTcpHeader): 0,
				tcpControlBlock->mUnackedSegments != NULL?
				TCP_HEADER_GET_SEQUENCE_NUMBER(tcpControlBlock->mUnackedSegments->mTcpHeader) + TCP_GET_TCP_LENGTH(tcpControlBlock->mUnackedSegments): 0));*/

			/* Remove segment from the unacknowledged list if the incoming
				ACK acknowlegdes them. */
			while (tcpControlBlock->mUnackedSegments != NULL &&
				TCP_SEQUENCE_LESS_OR_EQUAL(TCP_HEADER_GET_SEQUENCE_NUMBER(tcpControlBlock->mUnackedSegments->mTcpHeader) +
				TCP_GET_TCP_LENGTH(tcpControlBlock->mUnackedSegments), gAcknowledgementNumber)) 
			{
				/*SMSC_TRACE(TCP_DEBUG, ("TCP(%p) rx removing %"U32_F":%"U32_F" from tcpControlBlock->mUnackedSegments",
					(void *)tcpControlBlock,
					TCP_HEADER_GET_SEQUENCE_NUMBER(tcpControlBlock->mUnackedSegments->mTcpHeader),
					TCP_HEADER_GET_SEQUENCE_NUMBER(tcpControlBlock->mUnackedSegments->mTcpHeader) +
					TCP_GET_TCP_LENGTH(tcpControlBlock->mUnackedSegments)));*/

				nextSegment = tcpControlBlock->mUnackedSegments;
				tcpControlBlock->mUnackedSegments = tcpControlBlock->mUnackedSegments->mNext;

				Tcp_FreeSegment(nextSegment);
			}
			/*tcpControlBlock->polltmr = 0;*/
		}

		/* We go through the ->mUnsentSegments list to see if any of the segments
			on the list are acknowledged by the ACK. This may seem
			strange since an "mUnsentSegments" segment shouldn't be acked. The
			rationale is that lwIP puts all outstanding segments on the
			->mUnsentSegments list after a retransmission, so these segments may
			in fact have been sent once. */
		while (tcpControlBlock->mUnsentSegments != NULL &&
			TCP_SEQUENCE_BETWEEN(gAcknowledgementNumber, TCP_HEADER_GET_SEQUENCE_NUMBER(tcpControlBlock->mUnsentSegments->mTcpHeader) + TCP_GET_TCP_LENGTH(tcpControlBlock->mUnsentSegments), tcpControlBlock->mSenderHighestSentSequenceNumber)) 
		{
			/*SMSC_TRACE(TCP_DEBUG, ("TCP(%p) rx removing %"U32_F":%"U32_F" from tcpControlBlock->mUnsentSegments",
				(void *)tcpControlBlock,
				TCP_HEADER_GET_SEQUENCE_NUMBER(tcpControlBlock->mUnsentSegments->mTcpHeader), 
				TCP_HEADER_GET_SEQUENCE_NUMBER(tcpControlBlock->mUnsentSegments->mTcpHeader) +
				TCP_GET_TCP_LENGTH(tcpControlBlock->mUnsentSegments)));*/

			nextSegment = tcpControlBlock->mUnsentSegments;
			tcpControlBlock->mUnsentSegments = tcpControlBlock->mUnsentSegments->mNext;
			
			Tcp_FreeSegment(nextSegment);

			if (tcpControlBlock->mUnsentSegments != NULL) {
				tcpControlBlock->mSenderNextSequenceNumber = TCP_HEADER_GET_SEQUENCE_NUMBER(tcpControlBlock->mUnsentSegments->mTcpHeader);
			}
		}
		/* End of ACK for new data processing. */

		/*SMSC_TRACE(TCP_DEBUG, ("TCP(%p) rx tcpControlBlock->mRoundTripTest %"U32_F" mRoundTripSequenceNumber %"U32_F" gAcknowledgementNumber %"U32_F,
			(void *)tcpControlBlock,
			tcpControlBlock->mRoundTripTest, tcpControlBlock->mRoundTripSequenceNumber, gAcknowledgementNumber));*/

		/* RTT estimation calculations. This is done by checking if the
			incoming segment acknowledges the segment we use to take a
			round-trip time measurement. */
		if (tcpControlBlock->mRoundTripTest && TCP_SEQUENCE_LESS_THAN(tcpControlBlock->mRoundTripSequenceNumber, gAcknowledgementNumber)) {
			measurement = ((s16_t)(gTcpTicks - tcpControlBlock->mRoundTripTest));

			/*SMSC_TRACE(TCP_DEBUG, ("TCP(%p) rx experienced rtt %"U16_F" ticks (%"U16_F" msec).",
				(void *)tcpControlBlock,
				measurement, measurement * TCP_SLOW_INTERVAL));*/

			/* This is taken directly from VJs original code in his paper */
			measurement = measurement - (tcpControlBlock->mScaledAverage >> 3);
			tcpControlBlock->mScaledAverage += measurement;
			if (measurement < 0) {
				measurement = -measurement;
			}
			measurement = measurement - (tcpControlBlock->mScaledDeviation >> 2);
			tcpControlBlock->mScaledDeviation += measurement;
			tcpControlBlock->mRetransmissionTimeOut = (tcpControlBlock->mScaledAverage >> 3) + tcpControlBlock->mScaledDeviation;

			/*SMSC_TRACE(TCP_DEBUG, ("TCP(%p) rx RTO %"U16_F" (%"U16_F" miliseconds)",
				(void *)tcpControlBlock,
				tcpControlBlock->mRetransmissionTimeOut, tcpControlBlock->mRetransmissionTimeOut * TCP_SLOW_INTERVAL));*/

			tcpControlBlock->mRoundTripTest = 0;
		}
	/*} else {
		SMSC_TRACE(TCP_DEBUG,("TCP(%p) did not receive ACK",(void *)tcpControlBlock));*/
	}

	/* If the incoming segment contains data, we must process it
		further. */
	if (gTcpLength > 0) {
		/* This code basically does three things:

		+) If the incoming segment contains data that is the next
		in-sequence data, this data is passed to the application. This
		might involve trimming the first edge of the data. The mReceiverNextSequenceNumber
		variable and the advertised window are adjusted.

		+) If the incoming segment has data that is above the next
		sequence number expected (->mReceiverNextSequenceNumber), the segment is placed on
		the ->mOutOfSequenceSegments queue. This is done by finding the appropriate
		place in the ->mOutOfSequenceSegments queue (which is ordered by sequence
		number) and trim the segment in both ends if needed. An
		immediate ACK is sent to indicate that we received an
		out-of-sequence segment.

		+) Finally, we check if the first segment on the ->mOutOfSequenceSegments queue
		now is in sequence (i.e., if mReceiverNextSequenceNumber >= mOutOfSequenceSegments->gSequenceNumber). If
		mReceiverNextSequenceNumber > mOutOfSequenceSegments->gSequenceNumber, we must trim the first edge of the
		segment on ->mOutOfSequenceSegments before we adjust mReceiverNextSequenceNumber. The data in the
		segments that are now on sequence are chained onto the
		incoming segment so that we only need to call the application
		once.
		*/

		/* First, we check if we must trim the first edge. We have to do
			this if the sequence number of the incoming segment is less
			than mReceiverNextSequenceNumber, and the sequence number plus the length of the
			segment is larger than mReceiverNextSequenceNumber. */
		if (TCP_SEQUENCE_BETWEEN(tcpControlBlock->mReceiverNextSequenceNumber, gSequenceNumber + 1, gSequenceNumber + gTcpLength - 1)){
			/* Trimming the first edge is done by pushing the payload
				pointer in the pbuf downwards. This is somewhat tricky since
				we do not want to discard the full contents of the pbuf up to
				the new starting point of the data since we have to keep the
				TCP header which is present in the first pbuf in the chain.

				What is done is really quite a nasty hack: the first pbuf in
				the pbuf chain is pointed to by gInputSegment.packet. Since we need to be
				able to deallocate the whole pbuf, we cannot change this
				gInputSegment.packet pointer to point to any of the later pbufs in the
				chain. Instead, we point the ->payload pointer in the first
				pbuf to data in one of the later pbufs. We also set the
				gInputSegment.data pointer to point to the right place. This way, the
				->p pointer will still point to the first pbuf, but the
				->p->payload pointer will point to data in another pbuf.

				After we are done with adjusting the pbuf pointers we must
				adjust the ->data pointer in the segment and the segment
				length.*/

			offset = tcpControlBlock->mReceiverNextSequenceNumber - gSequenceNumber;
			packet = gInputSegment.mPacket;
			SMSC_ASSERT(gInputSegment.mPacket);
			SMSC_ASSERT(offset<0x7FFF);/* make sure the s16_t conversions are valid */
			if (gInputSegment.mPacket->mThisLength < offset) {
				newTotalLength = (u16_t)(gInputSegment.mPacket->mTotalLength - offset);
				while (packet->mThisLength < offset) {
					offset -= packet->mThisLength;
					/* KJM following line changed (with addition of newTotalLength var)
						to fix bug #9076
						gInputSegment.packet->mTotalLength -= packet->mThisLength; */
					packet->mTotalLength = newTotalLength;
					packet->mThisLength = 0;
					packet = packet->mNext;
				}
				PacketBuffer_MoveStartPoint(packet,((s16_t)offset));
			} else {
				PacketBuffer_MoveStartPoint(gInputSegment.mPacket, ((s16_t)offset));
			}

			gInputSegment.mDataPointer = PacketBuffer_GetStartPoint(packet);
			gInputSegment.mLength -= ((u16_t)(tcpControlBlock->mReceiverNextSequenceNumber - gSequenceNumber));
			gSequenceNumber = tcpControlBlock->mReceiverNextSequenceNumber;
			TCP_HEADER_WRITE_SEQUENCE_NUMBER(gInputSegment.mTcpHeader,gSequenceNumber);
		} else {
			if (TCP_SEQUENCE_LESS_THAN(gSequenceNumber, tcpControlBlock->mReceiverNextSequenceNumber)){
				/* the whole segment is < mReceiverNextSequenceNumber */
				/* must be a duplicate of a packet that has already been correctly handled */

				SMSC_TRACE(TCP_DEBUG, ("TCP(%p) rx duplicate gSequenceNumber %"U32_F, gSequenceNumber));/**/
				Tcp_SendAcknowledgementNow(tcpControlBlock);
			}
		}

		/* The sequence number must be within the window (above mReceiverNextSequenceNumber
			and below mReceiverNextSequenceNumber + mReceiverWindow) in order to be further
			processed. */
		if (TCP_SEQUENCE_BETWEEN(gSequenceNumber, tcpControlBlock->mReceiverNextSequenceNumber, tcpControlBlock->mReceiverNextSequenceNumber + tcpControlBlock->mReceiverWindow - 1)) {
			if (tcpControlBlock->mReceiverNextSequenceNumber == gSequenceNumber) {
				acceptedInputSequence = 1; 
				/* The incoming segment is the next in sequence. We check if
					we have to trim the end of the segment and update mReceiverNextSequenceNumber
					and pass the data to the application. */
				if ((tcpControlBlock->mOutOfSequenceSegments != NULL) &&
					TCP_SEQUENCE_LESS_OR_EQUAL(TCP_HEADER_READ_SEQUENCE_NUMBER(tcpControlBlock->mOutOfSequenceSegments->mTcpHeader), gSequenceNumber + gInputSegment.mLength)) 
				{
					/* We have to trim the second edge of the incoming
						segment. */
					SMSC_TRACE(TCP_DEBUG, ("Localport[%d].mOutOfSequenceSegments'SequenceNumber[0x%08x]-gSequenceNumber[0x%08x]=[%d]",tcpControlBlock->mLocalPort,
					TCP_HEADER_READ_SEQUENCE_NUMBER(tcpControlBlock->mOutOfSequenceSegments->mTcpHeader),gSequenceNumber,
					(TCP_HEADER_READ_SEQUENCE_NUMBER(tcpControlBlock->mOutOfSequenceSegments->mTcpHeader) - gSequenceNumber)));
					SMSC_TRACE(TCP_DEBUG,("gInputSegment.mLength[%d]",gInputSegment.mLength));
					gInputSegment.mLength = ((u16_t)(TCP_HEADER_READ_SEQUENCE_NUMBER(tcpControlBlock->mOutOfSequenceSegments->mTcpHeader) - gSequenceNumber));
					PacketBuffer_ReduceTotalLength(gInputSegment.mPacket, gInputSegment.mLength);
				}

				gTcpLength = ((u16_t)(TCP_GET_TCP_LENGTH(&gInputSegment)));

				/* First received FIN will be ACKed +1, on any successive (duplicate)
				* FINs we are already in CLOSE_WAIT and have already done +1.
				*/
				if (tcpControlBlock->mState != TCP_STATE_CLOSE_WAIT)
				{
					tcpControlBlock->mReceiverNextSequenceNumber += gTcpLength;
				}

				/* Update the receiver's (our) window. */
				if (tcpControlBlock->mReceiverWindow < gTcpLength) {
					tcpControlBlock->mReceiverWindow = 0;
				} else {
					tcpControlBlock->mReceiverWindow -= gTcpLength;
				}

			  //  SMSC_NOTICE(1,("correct tcpControlBlock[%p]seq[%x]mReceiverNextSequenceNumber [%x]\n",tcpControlBlock,gSequenceNumber,tcpControlBlock->mReceiverNextSequenceNumber ));
                            
				/* If there is data in the segment, we make preparations to
					pass this up to the application. The ->gReceiveData variable
					is used for holding the pbuf that goes to the
					application. The code for reassembling out-of-sequence data
					chains its data on this pbuf as well.

					If the segment was a FIN, we set the TCB_FLAG_GOT_FIN flag that will
					be used to indicate to the application that the remote side has
					closed its end of the connection. */
				if (gInputSegment.mPacket->mTotalLength > 0) {
					gReceiveData = gInputSegment.mPacket;
					/* Since this pbuf now is the responsibility of the
						application, we delete our reference to it so that we won't
						(mistakingly) deallocate it. */
					gInputSegment.mPacket = NULL;
					
				        
				}
				if (TCP_HEADER_GET_FLAGS(gInputSegment.mTcpHeader) & TCP_HEADER_FLAG_FIN) {
					/*SMSC_TRACE(TCP_DEBUG, ("TCP(%p) rx received FIN."));*/
					gReceiveFlags = TCB_FLAG_GOT_FIN;
				}
                         
				/* We now check if we have segments on the ->mOutOfSequenceSegments queue that
					is now in sequence. */
				while ((tcpControlBlock->mOutOfSequenceSegments != NULL) &&
					(TCP_HEADER_READ_SEQUENCE_NUMBER(tcpControlBlock->mOutOfSequenceSegments->mTcpHeader) == tcpControlBlock->mReceiverNextSequenceNumber)) 
				{
					tempSegment = tcpControlBlock->mOutOfSequenceSegments;
					gSequenceNumber = TCP_HEADER_READ_SEQUENCE_NUMBER(tcpControlBlock->mOutOfSequenceSegments->mTcpHeader);

			              SMSC_NOTICE(1,("Resemble tcpControlBlock[%p]->mOutOfSequenceSegments[%p] seq[%x]\n",tcpControlBlock,tcpControlBlock->mOutOfSequenceSegments,gSequenceNumber));
					
					tcpControlBlock->mReceiverNextSequenceNumber += TCP_GET_TCP_LENGTH(tempSegment);
					if (tcpControlBlock->mReceiverWindow < TCP_GET_TCP_LENGTH(tempSegment)) {
						tcpControlBlock->mReceiverWindow = 0;
					} else {
						tcpControlBlock->mReceiverWindow -= (u16_t)(TCP_GET_TCP_LENGTH(tempSegment));
					}
				
					if (tempSegment->mPacket->mTotalLength > 0) {
						/* Chain this pbuf onto the pbuf that we will pass to
						the application. */
						if (gReceiveData) {
							PacketBuffer_JoinChains(gReceiveData, tempSegment->mPacket);
						} else {
							gReceiveData = tempSegment->mPacket;
						}
						tempSegment->mPacket = NULL;
					}
					if (TCP_HEADER_GET_FLAGS(tempSegment->mTcpHeader) & TCP_HEADER_FLAG_FIN) {
						/*SMSC_TRACE(TCP_DEBUG, ("TCP(%p) rx dequeued FIN."));*/
						gReceiveFlags = TCB_FLAG_GOT_FIN;
						if (tcpControlBlock->mState == TCP_STATE_ESTABLISHED) { /* force passive close or we can move to active close */
							tcpControlBlock->mState = TCP_STATE_CLOSE_WAIT;
						} 
					}

					tcpControlBlock->mOutOfSequenceSegments = tempSegment->mNext;
			              SMSC_NOTICE(TCP_DEBUG,("4Free tcpControlBlock[%p]->mOutOfSequenceSegments[%p]",tcpControlBlock,tcpControlBlock->mOutOfSequenceSegments));
					Tcp_FreeSegment(tempSegment);
				}
				

				/* Acknowledge the segment(s). */
				#if 0
				Tcp_SendAcknowledgementNow(tcpControlBlock);
				#else
				Tcp_QueueAcknowledgement(tcpControlBlock);
				#endif

			} else {
				/* We get here if the incoming segment is out-of-sequence. */
				
				#if 1
				Tcp_SendAcknowledgementNow(tcpControlBlock);
				#else
				Tcp_QueueAcknowledgement(tcpControlBlock);
				#endif
				/* We queue the segment on the ->mOutOfSequenceSegments queue. */
				if (tcpControlBlock->mOutOfSequenceSegments == NULL) {
					tcpControlBlock->mOutOfSequenceSegments = Tcp_CopySegment(&gInputSegment);
					/*do_report(0,"1 tcpControlBlock[%p]->mOutOfSequenceSegments[%p],Now pkt's seqno[%x]\n",tcpControlBlock,tcpControlBlock->mOutOfSequenceSegments,gSequenceNumber) sqzow20100602*/;
				} else {
					/* If the queue is not empty, we walk through the queue and
						try to find a place where the sequence number of the
						incoming segment is between the sequence numbers of the
						previous and the next segment on the ->mOutOfSequenceSegments queue. That is
						the place where we put the incoming segment. If needed, we
						trim the second edges of the previous and the incoming
						segment so that it will fit into the sequence.

						If the incoming segment has the same sequence number as a
						segment on the ->mOutOfSequenceSegments queue, we discard the segment that
						contains less data. */
					/*do_report(0,"2 tcpControlBlock[%p]->mOutOfSequenceSegments[%p],Now pkt's seqno[%x]\n",tcpControlBlock,tcpControlBlock->mOutOfSequenceSegments,gSequenceNumber) sqzow20100602*/;

					prevSegment = NULL;
					for(nextSegment = tcpControlBlock->mOutOfSequenceSegments; 
						nextSegment != NULL; nextSegment = nextSegment->mNext) 
					{
						if (gSequenceNumber == TCP_HEADER_READ_SEQUENCE_NUMBER(nextSegment->mTcpHeader)) {
							/* The sequence number of the incoming segment is the
								same as the sequence number of the segment on
								->mOutOfSequenceSegments. We check the lengths to see which one to
								discard. */
							if (gInputSegment.mLength > nextSegment->mLength) {
								/* The incoming segment is larger than the old
								segment. We replace the old segment with the new
								one. */
								tempSegment = Tcp_CopySegment(&gInputSegment);
								if (tempSegment != NULL) {
									tempSegment->mNext = nextSegment->mNext;
									if (prevSegment != NULL) {
										prevSegment->mNext = tempSegment;
									} else {
										tcpControlBlock->mOutOfSequenceSegments = tempSegment;
									}
								}
								break;
							} else {
								/* Either the lenghts are the same or the incoming
									segment was smaller than the old one; in either
									case, we ditch the incoming segment. */
								break;
							}
						} else {
							if (prevSegment == NULL) {
								if (TCP_SEQUENCE_LESS_THAN(gSequenceNumber, TCP_HEADER_READ_SEQUENCE_NUMBER(nextSegment->mTcpHeader))) {
									/* The sequence number of the incoming segment is lower
									than the sequence number of the first segment on the
									queue. We put the incoming segment first on the
									queue. */

									if (TCP_SEQUENCE_GREATER_THAN(gSequenceNumber + gInputSegment.mLength, TCP_HEADER_READ_SEQUENCE_NUMBER(nextSegment->mTcpHeader))) {
										/* We need to trim the incoming segment. */
										gInputSegment.mLength = (u16_t)(TCP_HEADER_READ_SEQUENCE_NUMBER(nextSegment->mTcpHeader) - gSequenceNumber);
										PacketBuffer_ReduceTotalLength(gInputSegment.mPacket, gInputSegment.mLength);
									}
									tempSegment = Tcp_CopySegment(&gInputSegment);
									if (tempSegment != NULL) {
										tempSegment->mNext = nextSegment;
										tcpControlBlock->mOutOfSequenceSegments = tempSegment;
									}
									break;
								}
							} else if(TCP_SEQUENCE_BETWEEN(gSequenceNumber, TCP_HEADER_READ_SEQUENCE_NUMBER(prevSegment->mTcpHeader)+1,
									 TCP_HEADER_READ_SEQUENCE_NUMBER(nextSegment->mTcpHeader)-1)) 
							{
								/* The sequence number of the incoming segment is in
									between the sequence numbers of the previous and
									the next segment on ->mOutOfSequenceSegments. We trim and insert the
									incoming segment and trim the previous segment, if
									needed. */
								if (TCP_SEQUENCE_GREATER_THAN(gSequenceNumber + gInputSegment.mLength, TCP_HEADER_READ_SEQUENCE_NUMBER(nextSegment->mTcpHeader))) {
									/* We need to trim the incoming segment. */
									gInputSegment.mLength = (u16_t)(TCP_HEADER_READ_SEQUENCE_NUMBER(nextSegment->mTcpHeader) - gSequenceNumber);
									PacketBuffer_ReduceTotalLength(gInputSegment.mPacket, gInputSegment.mLength);
								}

								tempSegment = Tcp_CopySegment(&gInputSegment);
								if (tempSegment != NULL) {
									tempSegment->mNext = nextSegment;
									prevSegment->mNext = tempSegment;
									if (TCP_SEQUENCE_GREATER_THAN(TCP_HEADER_READ_SEQUENCE_NUMBER(prevSegment->mTcpHeader) + prevSegment->mLength, gSequenceNumber)) {
										/* We need to trim the prev segment. */
										prevSegment->mLength = (u16_t)(gSequenceNumber - TCP_HEADER_READ_SEQUENCE_NUMBER(prevSegment->mTcpHeader));
										PacketBuffer_ReduceTotalLength(prevSegment->mPacket, prevSegment->mLength);
									}
								}
								break;
							}
							/* If the "next" segment is the last segment on the
								mOutOfSequenceSegments queue, we add the incoming segment to the end
								of the list. */
							if ((nextSegment->mNext == NULL) &&
								TCP_SEQUENCE_GREATER_THAN(gSequenceNumber, TCP_HEADER_READ_SEQUENCE_NUMBER(nextSegment->mTcpHeader))) 
							{
								nextSegment->mNext = Tcp_CopySegment(&gInputSegment);
								if (nextSegment->mNext != NULL) {
									if (TCP_SEQUENCE_GREATER_THAN(TCP_HEADER_READ_SEQUENCE_NUMBER(nextSegment->mTcpHeader) + nextSegment->mLength, gSequenceNumber)) {
										/* We need to trim the last segment. */
										nextSegment->mLength = (u16_t)(gSequenceNumber - TCP_HEADER_READ_SEQUENCE_NUMBER(nextSegment->mTcpHeader));
										PacketBuffer_ReduceTotalLength(nextSegment->mPacket, nextSegment->mLength);
									}
								}
								break;
							}
						}
						prevSegment = nextSegment;
					}
				}
			}
		} else {
			if(!TCP_SEQUENCE_BETWEEN(gSequenceNumber, tcpControlBlock->mReceiverNextSequenceNumber, tcpControlBlock->mReceiverNextSequenceNumber + tcpControlBlock->mReceiverWindow-1)){
				Tcp_SendAcknowledgementNow(tcpControlBlock);
			}
		}
	} else {
		/* Segments with length 0 is taken care of here. Segments that
			fall out of the window are ACKed. */
		if(!TCP_SEQUENCE_BETWEEN(gSequenceNumber, tcpControlBlock->mReceiverNextSequenceNumber, tcpControlBlock->mReceiverNextSequenceNumber + tcpControlBlock->mReceiverWindow-1)){
			Tcp_SendAcknowledgementNow(tcpControlBlock);
		}
	}
	return acceptedInputSequence;
}

/* Tcp_Process
 *
 * Implements the TCP state machine. Called by tcp_input. In some
 * states Tcp_Receive() is called to receive data. The tcp_seg
 * argument will be freed by the caller (tcp_input()) unless the
 * gReceiveData pointer in the tcpControlBlock is set.
 */
static err_t Tcp_Process(struct TCP_CONTROL_BLOCK * tcpControlBlock)
{
	PTCP_SEGMENT receivedSegment;
	u8_t acceptable = 0;
	err_t error;
	u8_t acceptedInputSequence;

	SMSC_ASSERT(tcpControlBlock!=NULL);
	CHECK_SIGNATURE(tcpControlBlock,TCP_CONTROL_BLOCK_SIGNATURE);

	error = ERR_OK;

	/* Process incoming RST segments. */
	if (flags & TCP_HEADER_FLAG_RST) {
		/* First, determine if the reset is acceptable. */
		if (tcpControlBlock->mState == TCP_STATE_SYN_SENT) {
			if (gAcknowledgementNumber == tcpControlBlock->mSenderNextSequenceNumber) {
				acceptable = 1;
			}
		} else {
			/*if (TCP_SEQUENCE_GREATER_OR_EQUAL(gSequenceNumber, tcpControlBlock->mReceiverNextSequenceNumber) &&
				TCP_SEQUENCE_LESS_OR_EQUAL(gSequenceNumber, tcpControlBlock->mReceiverNextSequenceNumber + tcpControlBlock->mReceiverWindow)) {
			*/
			if (TCP_SEQUENCE_BETWEEN(gSequenceNumber, tcpControlBlock->mReceiverNextSequenceNumber, tcpControlBlock->mReceiverNextSequenceNumber+tcpControlBlock->mReceiverWindow)) {
				acceptable = 1;
			}
		}

		if (acceptable) {
			SMSC_TRACE(TCP_DEBUG, ("TCP(%p) process Connection RESET",(void *)tcpControlBlock));
			SMSC_ASSERT(tcpControlBlock->mState != TCP_STATE_CLOSED);
			gReceiveFlags = TCB_FLAG_RESET;
			tcpControlBlock->mFlags &= ~TCB_FLAG_ACK_DELAY;
			return ERR_RST;
		} else {
			SMSC_NOTICE(TCP_DEBUG, ("TCP(%p) process unacceptable reset gSequenceNumber %"U32_F" mReceiverNextSequenceNumber %"U32_F,
				(void *)tcpControlBlock, gSequenceNumber, tcpControlBlock->mReceiverNextSequenceNumber));
			return ERR_OK;
		}
	}

	/* Update the Control Block (in)activity timer. */
	tcpControlBlock->mTimer = gTcpTicks;
	tcpControlBlock->mKeepAliveCounter = 0;

	/* Do different things depending on the TCP state. */
	switch (tcpControlBlock->mState) {
	case TCP_STATE_SYN_SENT:
		/*SMSC_TRACE(TCP_DEBUG, ("SYN-SENT: gAcknowledgementNumber %"U32_F" tcpControlBlock->mSenderNextSequenceNumber %"U32_F" mUnackedSegments %"U32_F, gAcknowledgementNumber,
			tcpControlBlock->mSenderNextSequenceNumber, TCP_HEADER_GET_SEQUENCE_NUMBER(tcpControlBlock->mUnackedSegments->mTcpHeader)));*/
		/* received SYN ACK with expected sequence number? */
		if ((flags & TCP_HEADER_FLAG_ACK) && (flags & TCP_HEADER_FLAG_SYN)
			&& (gAcknowledgementNumber == (TCP_HEADER_GET_SEQUENCE_NUMBER(tcpControlBlock->mUnackedSegments->mTcpHeader) + 1)) ) 
		{
			SMSC_NOTICE(TCP_DEBUG,("TCP(%p) process Received ACK and SYN",(void *)tcpControlBlock));/**/
			tcpControlBlock->mAvailableSendSpace++;
			tcpControlBlock->mReceiverNextSequenceNumber = gSequenceNumber + 1;
			tcpControlBlock->mLastAcknowledgedSequenceNumber = gAcknowledgementNumber;
			tcpControlBlock->mSenderWindow = TCP_HEADER_READ_WINDOW(gTcpHeader);
			tcpControlBlock->mSenderLastSequenceNumber = gSequenceNumber - 1; /* initialise to gSequenceNumber - 1 to force window update */
			tcpControlBlock->mState = TCP_STATE_ESTABLISHED;
			tcpControlBlock->mCongestionWindow = tcpControlBlock->mMaximumSegmentSize;
			receivedSegment = tcpControlBlock->mUnackedSegments;
			tcpControlBlock->mUnackedSegments = receivedSegment->mNext;
			Tcp_FreeSegment(receivedSegment);

			/* Parse any options in the SYNACK. */
			Tcp_ParseOptions(tcpControlBlock,gTcpHeader);
			#if SMSC_TRACE_ENABLED
			{
				char addressString1[IP_ADDRESS_STRING_SIZE];
				char addressString2[IP_ADDRESS_STRING_SIZE];
				SMSC_TRACE(TCP_DEBUG,("TCP(%p) connection established",(void *)tcpControlBlock));
				SMSC_TRACE(TCP_DEBUG,("    local=%s:%"U16_F" <-> remote=%s:%"U16_F,
					IP_ADDRESS_TO_STRING(addressString1,&(tcpControlBlock->mLocalAddress)),tcpControlBlock->mLocalPort,
					IP_ADDRESS_TO_STRING(addressString2,&(tcpControlBlock->mRemoteAddress)),tcpControlBlock->mRemotePort));
			}
			#endif
			/* Call the user specified function to call when sucessfully connected. */
			if(tcpControlBlock->mConnectedCallBack != NULL) {
				error = ((TCP_CONNECTED_CALLBACK)(tcpControlBlock->mConnectedCallBack))(tcpControlBlock->mCallbackArgument,tcpControlBlock,ERR_OK);
				if(error!=ERR_OK) {
					SMSC_WARNING(TCP_DEBUG,("TCP(%p) error returned from connected call back is not handled",(void *)tcpControlBlock));
				}
			} else {
				SMSC_WARNING(TCP_DEBUG,("TCP(%p) Connection established but no connected call back available",(void *)tcpControlBlock));
			}
			Tcp_QueueAcknowledgement(tcpControlBlock);
		}
		/* received ACK? possibly a half-open connection */
		else if (flags & TCP_HEADER_FLAG_ACK) {
			/*SMSC_NOTICE(TCP_DEBUG,("TCP(%p) received ack but no notice",(void *)tcpControlBlock));*/
			/* send a RST to bring the other side in a non-synchronized state. */
			Tcp_SendReset(gAcknowledgementNumber, gSequenceNumber + gTcpLength, gDestinationAddress, gSourceAddress,
				TCP_HEADER_READ_DESTINATION_PORT(gTcpHeader),
				TCP_HEADER_READ_SOURCE_PORT(gTcpHeader));
		}
		break;
	case TCP_STATE_SYN_RCVD:
		if ((flags & TCP_HEADER_FLAG_ACK) && !(flags & TCP_HEADER_FLAG_RST)) {
			/*SMSC_TRACE(TCP_DEBUG,("TCP(%p) Received ACK after SYN",(void *)tcpControlBlock));*/
			/* expected ACK number? */
			if (TCP_SEQUENCE_BETWEEN(gAcknowledgementNumber, tcpControlBlock->mLastAcknowledgedSequenceNumber+1, tcpControlBlock->mSenderNextSequenceNumber)) {
				tcpControlBlock->mState = TCP_STATE_ESTABLISHED;
				SMSC_NOTICE(TCP_DEBUG, ("TCP(%p) connection established %"U16_F" -> %"U16_F,
					(void *)tcpControlBlock,
					TCP_HEADER_READ_SOURCE_PORT(gTcpHeader),
					TCP_HEADER_READ_DESTINATION_PORT(gTcpHeader)));/**/
				SMSC_ASSERT(tcpControlBlock->mAcceptCallBack != NULL);
				/* Call the accept function. */
				if(tcpControlBlock->mAcceptCallBack != NULL) {
					error = ((TCP_ACCEPT_CALLBACK)(tcpControlBlock->mAcceptCallBack))(tcpControlBlock->mCallbackArgument,tcpControlBlock,ERR_OK);
				} else {
					SMSC_WARNING(TCP_DEBUG,("TCP(%p) Connection established but no accept call back available",(void *)tcpControlBlock));
					error=ERR_ABRT;
				}
				if (error != ERR_OK) {
					/* If the accept function returns with an error, we abort
					* the connection. */
					Tcp_Abort(tcpControlBlock);
					return ERR_ABRT;
				} else {
					#if SMSC_TRACE_ENABLED
					{
						char addressString1[IP_ADDRESS_STRING_SIZE];
						char addressString2[IP_ADDRESS_STRING_SIZE];
						SMSC_TRACE(TCP_DEBUG,("TCP(%p) connection accepted",(void *)tcpControlBlock));
						SMSC_TRACE(TCP_DEBUG,("    local=%s:%"U16_F" <-> remote=%s:%"U16_F,
							IP_ADDRESS_TO_STRING(addressString1,&(tcpControlBlock->mLocalAddress)),tcpControlBlock->mLocalPort,
							IP_ADDRESS_TO_STRING(addressString2,&(tcpControlBlock->mRemoteAddress)),tcpControlBlock->mRemotePort));
					}
					#endif
				}
				
				/* If there was any data contained within this ACK,
				* we'd better pass it on to the application as well. */
				Tcp_Receive(tcpControlBlock);
				tcpControlBlock->mCongestionWindow = tcpControlBlock->mMaximumSegmentSize;
			}
			/* incorrect ACK number */
			else {
				/* send RST */
				Tcp_SendReset(gAcknowledgementNumber, gSequenceNumber + gTcpLength, gDestinationAddress, gSourceAddress,
					TCP_HEADER_READ_DESTINATION_PORT(gTcpHeader),
					TCP_HEADER_READ_SOURCE_PORT(gTcpHeader));
			}
		}
		break;
	case TCP_STATE_CLOSE_WAIT:
		/* FALLTHROUGH */
	case TCP_STATE_ESTABLISHED:
		acceptedInputSequence = Tcp_Receive(tcpControlBlock);
		if ((flags & TCP_HEADER_FLAG_FIN) && acceptedInputSequence) { /* passive close */
			Tcp_SendAcknowledgementNow(tcpControlBlock);
			tcpControlBlock->mState = TCP_STATE_CLOSE_WAIT;
		}
		break;
	case TCP_STATE_FIN_WAIT_1:
		Tcp_Receive(tcpControlBlock);
		if (flags & TCP_HEADER_FLAG_FIN) {
			if ((flags & TCP_HEADER_FLAG_ACK) && (gAcknowledgementNumber == tcpControlBlock->mSenderNextSequenceNumber)) {
				SMSC_TRACE(TCP_DEBUG,
					("TCP(%p) connection closed %"U16_F" -> %"U16_F".",
					(void *)tcpControlBlock,
					TCP_HEADER_READ_SOURCE_PORT(gInputSegment.mTcpHeader), 
					TCP_HEADER_READ_DESTINATION_PORT(gInputSegment.mTcpHeader)));
				Tcp_SendAcknowledgementNow(tcpControlBlock);
				Tcp_PurgeControlBlock(tcpControlBlock);
				
				/* Remove the tcpControlBlock from the active list */
				TCP_REMOVE_CONTROL_BLOCK(&gTcpActiveList,tcpControlBlock);
				
				tcpControlBlock->mState = TCP_STATE_TIME_WAIT;
                
                /* Add tcpControlBlock to the time wait list */
                TCP_REGISTER_CONTROL_BLOCK(&gTcpTimeWaitList,tcpControlBlock);
				
			} else {
				Tcp_SendAcknowledgementNow(tcpControlBlock);
				tcpControlBlock->mState = TCP_STATE_CLOSING;
			}
		} else if ((flags & TCP_HEADER_FLAG_ACK) && (gAcknowledgementNumber == tcpControlBlock->mSenderNextSequenceNumber)) {
			/*SMSC_TRACE(TCP_DEBUG,("TCP(%p) received ACK in FIN_WAIT_1 state",(void *)tcpControlBlock));*/
			tcpControlBlock->mState = TCP_STATE_FIN_WAIT_2;
		}
		break;
	case TCP_STATE_FIN_WAIT_2:
		Tcp_Receive(tcpControlBlock);
		if (flags & TCP_HEADER_FLAG_FIN) {
			SMSC_TRACE(TCP_DEBUG, ("TCP(%p) connection closed %"U16_F" -> %"U16_F".", 
				(void *)tcpControlBlock,
				TCP_HEADER_READ_SOURCE_PORT(gInputSegment.mTcpHeader), 
				TCP_HEADER_READ_DESTINATION_PORT(gInputSegment.mTcpHeader)));
			Tcp_SendAcknowledgementNow(tcpControlBlock);
			Tcp_PurgeControlBlock(tcpControlBlock);
			TCP_REMOVE_CONTROL_BLOCK(&gTcpActiveList, tcpControlBlock);
			tcpControlBlock->mState = TCP_STATE_TIME_WAIT;
			TCP_REGISTER_CONTROL_BLOCK(&gTcpTimeWaitList, tcpControlBlock);
		}
		break;
	case TCP_STATE_CLOSING:
		Tcp_Receive(tcpControlBlock);
		if ((flags & TCP_HEADER_FLAG_ACK) && (gAcknowledgementNumber == tcpControlBlock->mSenderNextSequenceNumber)) {
			SMSC_TRACE(TCP_DEBUG, ("TCP(%p).CLOSING connection closed %"U16_F" -> %"U16_F".",
				(void *)tcpControlBlock,
				TCP_HEADER_READ_SOURCE_PORT(gInputSegment.mTcpHeader),
				TCP_HEADER_READ_DESTINATION_PORT(gInputSegment.mTcpHeader)));
			Tcp_SendAcknowledgementNow(tcpControlBlock);
			Tcp_PurgeControlBlock(tcpControlBlock);
			TCP_REMOVE_CONTROL_BLOCK(&gTcpActiveList, tcpControlBlock);
			tcpControlBlock->mState = TCP_STATE_TIME_WAIT;
			TCP_REGISTER_CONTROL_BLOCK(&gTcpTimeWaitList, tcpControlBlock);
		}
		break;
	case TCP_STATE_LAST_ACK:
		Tcp_Receive(tcpControlBlock);
		if ((flags & TCP_HEADER_FLAG_ACK) && (gAcknowledgementNumber == tcpControlBlock->mSenderNextSequenceNumber)) {
			SMSC_NOTICE(TCP_DEBUG, ("TCP(%p).LAST_ACK connection closed %"U16_F" -> %"U16_F".",
				(void *)tcpControlBlock,
				TCP_HEADER_READ_SOURCE_PORT(gInputSegment.mTcpHeader),
				TCP_HEADER_READ_DESTINATION_PORT(gInputSegment.mTcpHeader)));
#ifndef MEMORY_POOL_CHANGE_090217
			tcpControlBlock->mState = TCP_STATE_CLOSED;
#endif
			gReceiveFlags = TCB_FLAG_CLOSED;
		}
		break;
	default:
		break;
	}
	return ERR_OK;
}

/* Tcp_Input:
 *
 * The initial input processing of TCP. It verifies the TCP header, demultiplexes
 * the segment between the Control Blocks and passes it on to Tcp_Process(), which implements
 * the TCP finite state machine. This function is called by the IP layer (in
 * ip_input()).
 */
void Tcp_Input(
	struct NETWORK_INTERFACE * networkInterface, PPACKET_BUFFER packet, 
	PIP_ADDRESS destinationAddress,PIP_ADDRESS sourceAddress,
	u8_t protocol)
{	
	struct TCP_CONTROL_BLOCK * tcpControlBlock;
	struct TCP_CONTROL_BLOCK * previousControlBlock;
	struct TCP_CONTROL_BLOCK * listenerControlBlock;
	u8_t headerLength;
	err_t error;

	gTcpHeader=PacketBuffer_GetStartPoint(packet);

#if TCP_DEBUG
	/*Tcp_PrintHeader(gTcpHeader);*/
#endif

	if (packet->mThisLength<TCP_HEADER_LENGTH) {
		/* drop short packets */
		SMSC_NOTICE(TCP_DEBUG, ("Tcp_Input: short packet (%"U32_F" bytes) discarded", packet->mTotalLength));
		goto DROP_PACKET;
	}

	/* Don't even process incoming broadcasts/multicasts. */
	if (IP_ADDRESS_IS_BROADCAST(destinationAddress, networkInterface) ||
		IP_ADDRESS_IS_MULTICAST(destinationAddress)) 
	{
		SMSC_NOTICE(TCP_DEBUG, ("Tcp_Input: broadcast/multicast packet discarded"));
		goto DROP_PACKET;
	}

    /* make sure the following typecast is acceptable */
	SMSC_ASSERT(!(packet->mTotalLength&0xFFFF0000));
	
	/* Verify TCP checksum. */
	if(Ip_PseudoCheckSum(packet,
		sourceAddress,destinationAddress,
		IP_PROTOCOL_TCP,(u16_t)(packet->mTotalLength))!= 0) 
	{
		SMSC_NOTICE(TCP_DEBUG, ("Tcp_Input: packet discarded due to failing checksum 0x%04"X16_F,
			Ip_PseudoCheckSum(packet,sourceAddress,destinationAddress,IP_PROTOCOL_TCP,(u16_t)(packet->mTotalLength))));
#if TCP_DEBUG
		Tcp_PrintHeader(gTcpHeader);
#endif /* TCP_DEBUG */
		goto DROP_PACKET;
	}

	/* Move the payload pointer in the pbuf so that it points to the
		TCP data instead of the TCP header. */
	headerLength = TCP_HEADER_GET_DATA_OFFSET(gTcpHeader);
	
	gDestinationAddress=destinationAddress;
	gSourceAddress=sourceAddress;
	
	PacketBuffer_MoveStartPoint(packet,headerLength*4);


	/* Convert fields in TCP header to host byte order. */
	TCP_HEADER_NTOH_SOURCE_PORT(gTcpHeader);
	TCP_HEADER_NTOH_DESTINATION_PORT(gTcpHeader);
	TCP_HEADER_NTOH_SEQUENCE_NUMBER(gTcpHeader);
	TCP_HEADER_NTOH_ACKNOWLEDGEMENT_NUMBER(gTcpHeader);
	TCP_HEADER_NTOH_WINDOW(gTcpHeader);

	gSequenceNumber=TCP_HEADER_READ_SEQUENCE_NUMBER(gTcpHeader);
	gAcknowledgementNumber=TCP_HEADER_READ_ACKNOWLEDGEMENT_NUMBER(gTcpHeader);
	/*SMSC_NOTICE(TCP_DEBUG, ("gSequenceNumber=0x%x, gAcknowledgementNumber=0x%x",gSequenceNumber,gAcknowledgementNumber));*/
	flags = TCP_HEADER_GET_FLAGS(gTcpHeader);

	/* Make sure the following typecast is acceptable */
	SMSC_ASSERT(!(packet->mTotalLength&0xFFFF0000));
	
	gTcpLength = ((u16_t)(packet->mTotalLength)) + (((flags & TCP_HEADER_FLAG_FIN) || (flags & TCP_HEADER_FLAG_SYN))? 1: 0);

	/* Demultiplex an incoming segment. First, we check if it is destined
	for an active connection. */
	previousControlBlock = NULL;


	for(tcpControlBlock = gTcpActiveList; tcpControlBlock != NULL; tcpControlBlock = tcpControlBlock->mNext) {
		SMSC_ASSERT(tcpControlBlock->mState != TCP_STATE_CLOSED);
		SMSC_ASSERT(tcpControlBlock->mState != TCP_STATE_TIME_WAIT);
		SMSC_ASSERT(tcpControlBlock->mState != TCP_STATE_LISTEN);
		if ((tcpControlBlock->mRemotePort == TCP_HEADER_READ_SOURCE_PORT(gTcpHeader)) &&
			(tcpControlBlock->mLocalPort == TCP_HEADER_READ_DESTINATION_PORT(gTcpHeader)) &&
			IP_ADDRESS_COMPARE(&(tcpControlBlock->mRemoteAddress), sourceAddress) &&
			IP_ADDRESS_COMPARE(&(tcpControlBlock->mLocalAddress), destinationAddress)) 
		{
			/* Move this Control Block to the front of the list so that subsequent
			lookups will be faster (we exploit locality in TCP segment
			arrivals). */
			SMSC_ASSERT(tcpControlBlock->mNext != tcpControlBlock);
			if (previousControlBlock != NULL) {
				previousControlBlock->mNext = tcpControlBlock->mNext;
				#ifdef 	 TCP_CONTROL_BLOCK_CHECK
				if((tcpControlBlock != gTcpActiveList)||(gTcpActiveList==NULL))
				#endif
				{
				tcpControlBlock->mNext = gTcpActiveList;
				gTcpActiveList = tcpControlBlock;
				}
				#ifdef 	 TCP_CONTROL_BLOCK_CHECK
				else
				{
					SMSC_ERROR(("\n gTcpActiveList insert  error!!!!!!!!!!"));
				}
				#endif
			}
			SMSC_ASSERT(tcpControlBlock->mNext != tcpControlBlock);
			break;
		}
		previousControlBlock = tcpControlBlock;
	}

	if (tcpControlBlock == NULL) {
		/* If it did not go to an active connection, we check the connections
		in the TIME-WAIT state. */
		for(tcpControlBlock = gTcpTimeWaitList; tcpControlBlock != NULL; tcpControlBlock = tcpControlBlock->mNext) {
			SMSC_ASSERT(tcpControlBlock->mState == TCP_STATE_TIME_WAIT);
			if ((tcpControlBlock->mRemotePort == TCP_HEADER_READ_SOURCE_PORT(gTcpHeader)) &&
				(tcpControlBlock->mLocalPort == TCP_HEADER_READ_DESTINATION_PORT(gTcpHeader)) &&
				IP_ADDRESS_COMPARE(&(tcpControlBlock->mRemoteAddress), sourceAddress) &&
				IP_ADDRESS_COMPARE(&(tcpControlBlock->mLocalAddress), destinationAddress)) 
			{
				/* We don't really care enough to move this Control Block to the front
				of the list since we are not very likely to receive that
				many segments for connections in TIME-WAIT. */
				SMSC_NOTICE(TCP_DEBUG, ("Tcp_Input: packet for TIME_WAITing connection."));
				if (TCP_SEQUENCE_GREATER_THAN(gSequenceNumber + gTcpLength, tcpControlBlock->mReceiverNextSequenceNumber)) 
				{
					tcpControlBlock->mReceiverNextSequenceNumber = gSequenceNumber + gTcpLength;
				}
				if (gTcpLength > 0) {
					Tcp_SendAcknowledgementNow(tcpControlBlock);
				}
				Tcp_SendQueuedData(tcpControlBlock);
				goto DROP_PACKET;
			}
		}

		/* Finally, if we still did not get a match, we check all Control Blocks that
		are LISTENing for incoming connections. */
		previousControlBlock = NULL;
		for(listenerControlBlock = gTcpListenerList; listenerControlBlock != NULL; listenerControlBlock = listenerControlBlock->mNext) {
			SMSC_ASSERT(listenerControlBlock->mState==TCP_STATE_LISTEN);
			if ((IP_ADDRESS_IS_ANY(&(listenerControlBlock->mLocalAddress)) ||
				IP_ADDRESS_COMPARE(&(listenerControlBlock->mLocalAddress), destinationAddress)) &&
				(listenerControlBlock->mLocalPort == TCP_HEADER_READ_DESTINATION_PORT(gTcpHeader))) 
			{
				/* Move this Control Block to the front of the list so that subsequent
				lookups will be faster (we exploit locality in TCP segment
				arrivals). */
				if (previousControlBlock != NULL) {
					previousControlBlock->mNext = listenerControlBlock->mNext;
					/* our successor is the remainder of the listening list */
				#ifdef 	 TCP_CONTROL_BLOCK_CHECK
				if((listenerControlBlock != gTcpListenerList)||(gTcpListenerList==NULL))
				#endif
				{
					listenerControlBlock->mNext = gTcpListenerList;
					/* put this listening tcpControlBlock at the head of the listening list */
					gTcpListenerList= listenerControlBlock;
				}
				#ifdef 	 TCP_CONTROL_BLOCK_CHECK
				else
					{
					SMSC_ERROR(("\n  gTcpListenerList  error!!!!!!!!!!"));
					}
				#endif
				}

				/*SMSC_TRACE(TCP_DEBUG, ("Tcp_Input: packet for LISTENing connection (%p)",(void *)listenerControlBlock));*/
				
				{/* copied from tcp_listen_input(tcpControlBlock) */
					struct TCP_CONTROL_BLOCK * newControlBlock;
					u8_t optdata[4];

					/* In the LISTEN state, we check for incoming SYN segments,
					creates a new Control Block, and responds with a SYN|ACK. */
					if (flags & TCP_HEADER_FLAG_ACK) {
						/* For incoming segments with the ACK flag set, respond with a
						 RST. */
						SMSC_NOTICE(TCP_DEBUG, ("Tcp_Input: ACK in LISTEN, sending reset (%p)",(void *)listenerControlBlock));
						Tcp_SendReset(gAcknowledgementNumber + 1, gSequenceNumber + gTcpLength,
							destinationAddress, sourceAddress,
							TCP_HEADER_READ_DESTINATION_PORT(gTcpHeader), TCP_HEADER_READ_SOURCE_PORT(gTcpHeader));
					} else if (flags & TCP_HEADER_FLAG_SYN) {
						#if SMSC_TRACE_ENABLED
						{
							char addressString1[IP_ADDRESS_STRING_SIZE];
							char addressString2[IP_ADDRESS_STRING_SIZE];
							SMSC_TRACE(TCP_DEBUG, ("TCP(%p) received connection request",(void *)listenerControlBlock));
							SMSC_TRACE(TCP_DEBUG, ("    local=%s:%"U16_F" <- remote=%s:%"U16_F,
								IP_ADDRESS_TO_STRING(addressString2,&(listenerControlBlock->mLocalAddress)),listenerControlBlock->mLocalPort,
								IP_ADDRESS_TO_STRING(addressString1,sourceAddress),TCP_HEADER_READ_SOURCE_PORT(gTcpHeader)));
						}
						#endif
						newControlBlock = Tcp_AllocateControlBlock(listenerControlBlock->mPriority);
						/* If a new Control Block could not be created (probably due to lack of memory),
						we don't do anything, but rely on the sender will retransmit the
						SYN at a time when we have more memory available. */
						if (newControlBlock == NULL) {
							SMSC_NOTICE(TCP_DEBUG, ("Tcp_Input: could not allocate control block for connection request"));
							goto DROP_PACKET;
						}
						/* Set up the new Control Block. */
						IP_ADDRESS_COPY(&(newControlBlock->mLocalAddress), destinationAddress);
						newControlBlock->mLocalPort = listenerControlBlock->mLocalPort;
						IP_ADDRESS_COPY(&(newControlBlock->mRemoteAddress), sourceAddress);
						newControlBlock->mRemotePort = TCP_HEADER_READ_SOURCE_PORT(gTcpHeader);
						#if SMSC_TRACE_ENABLED
						{
							char addressString1[IP_ADDRESS_STRING_SIZE];
							char addressString2[IP_ADDRESS_STRING_SIZE];
							SMSC_TRACE(TCP_DEBUG, ("TCP(%p) listener created (%p)",(void *)listenerControlBlock,(void *)newControlBlock));
							SMSC_TRACE(TCP_DEBUG, ("    local=%s:%"U16_F" <-> remote=%s:%"U16_F,
								IP_ADDRESS_TO_STRING(addressString1,&(newControlBlock->mLocalAddress)),newControlBlock->mLocalPort,
								IP_ADDRESS_TO_STRING(addressString2,&(newControlBlock->mRemoteAddress)),newControlBlock->mRemotePort));
						}
						#endif
						newControlBlock->mState = TCP_STATE_SYN_RCVD;
						newControlBlock->mReceiverNextSequenceNumber = gSequenceNumber + 1;
						newControlBlock->mSenderWindow = TCP_HEADER_READ_WINDOW(gTcpHeader);
						newControlBlock->mSlowStartThreshold = (u16_t)(newControlBlock->mSenderWindow);
						newControlBlock->mSenderLastSequenceNumber = gSequenceNumber - 1;/* initialise to gSequenceNumber-1 to force window update */
						newControlBlock->mCallbackArgument = listenerControlBlock->mCallbackArgument;
						newControlBlock->mAcceptCallBack = listenerControlBlock->mAcceptCallBack;
						/* inherit socket options */
						newControlBlock->mSocketOptions = listenerControlBlock->mSocketOptions & 
							(SOCKET_OPTION_FLAG_DEBUG|
							SOCKET_OPTION_FLAG_DONTROUTE|
							SOCKET_OPTION_FLAG_KEEPALIVE|
							SOCKET_OPTION_FLAG_OOBINLINE|
							SOCKET_OPTION_FLAG_LINGER);
						/* Register the new Control Block so that we can begin receiving segments
						for it. */
						TCP_REGISTER_CONTROL_BLOCK(&gTcpActiveList,newControlBlock);

						/* Parse any options in the SYN. */
						Tcp_ParseOptions(newControlBlock,gTcpHeader);

						/* Build an MSS option. */
						optdata[0]=2;
						optdata[1]=4;
						optdata[2]=(u8_t)((newControlBlock->mMaximumSegmentSize)>>8);
						optdata[3]=(u8_t)((newControlBlock->mMaximumSegmentSize)&0x00FF);
						
						/*SMSC_TRACE(TCP_DEBUG,("TCP(%p) Sending SYN and ACK from listener",(void *)newControlBlock));*/
						/* Send a SYN|ACK together with the MSS option. */
						Tcp_QueueControlData(newControlBlock, TCP_HEADER_FLAG_SYN | TCP_HEADER_FLAG_ACK, optdata, 4);
						Tcp_SendQueuedData(newControlBlock);
					}
				}
				goto DROP_PACKET;
			}
			previousControlBlock = listenerControlBlock;
		}
	}

#if TCP_DEBUG
	/*SMSC_PRINT(TCP_DEBUG, ("\n+-+-+-+-+-+-+-+- Tcp_Input: flags "));
	Tcp_PrintFlags(TCP_HEADER_GET_FLAGS(gTcpHeader));
	SMSC_PRINT(TCP_DEBUG, ("-+-+-+-+-+-+-+-+\n"));*/
#endif /* TCP_INPUT_DEBUG */


	if (tcpControlBlock != NULL) {
		/* The incoming segment belongs to a connection. */

		/* make sure the following typecast is acceptable */
		SMSC_ASSERT(!(packet->mTotalLength&0xFFFF0000));

		/* Set up a TCP_SEGMENT structure. */
		ASSIGN_SIGNATURE(&(gInputSegment),TCP_SEGMENT_SIGNATURE);
		gInputSegment.mNext = NULL;
		gInputSegment.mLength = (u16_t)(packet->mTotalLength);
		gInputSegment.mDataPointer = PacketBuffer_GetStartPoint(packet);
		gInputSegment.mPacket = packet;
		gInputSegment.mTcpHeader = gTcpHeader;

		gReceiveData = NULL;
		gReceiveFlags = 0;

		gInputControlBlock = tcpControlBlock;
		error=Tcp_Process(tcpControlBlock);
		gInputControlBlock = NULL;
		/* A return value of ERR_ABRT means that Tcp_Abort() was called
		and that the tcpControlBlock has been freed. If so, we don't do anything. */
		if (error != ERR_ABRT) {
			if (gReceiveFlags & TCB_FLAG_RESET) {
				/* TCB_FLAG_RESET means that the connection was reset by the other
				end. We then call the error callback to inform the
				application that the connection is dead before we
				deallocate the Control Block. */
				
				if(tcpControlBlock->mErrorCallBack != NULL) {
					((TCP_ERROR_CALLBACK)(tcpControlBlock->mErrorCallBack))(tcpControlBlock->mCallbackArgument,ERR_RST);
				} else {
					SMSC_NOTICE(TCP_DEBUG,("Tcp_Input: Connection Reset, but no error handler available"));
				}
				Tcp_RemoveControlBlockFromList(&gTcpActiveList, tcpControlBlock);
				MemoryPool_Free(gTcpControlBlockPool,tcpControlBlock);
			} else if (gReceiveFlags & TCB_FLAG_CLOSED) {
				/* The connection has been closed and we will deallocate the
				Control Block. */
				Tcp_RemoveControlBlockFromList(&gTcpActiveList, tcpControlBlock);
				MemoryPool_Free(gTcpControlBlockPool,tcpControlBlock);
			} else {
				error = ERR_OK;
				/* If the application has registered a "sent" function to be
				called when new send buffer space is available, we call it
				now. */
				if (tcpControlBlock->mAcknowledged > 0) {
					if(tcpControlBlock->mSentCallBack != NULL)
						error = ((TCP_SENT_CALLBACK)(tcpControlBlock->mSentCallBack))
							(tcpControlBlock->mCallbackArgument,tcpControlBlock,tcpControlBlock->mAcknowledged);
				}

				if (gReceiveData != NULL) {
					/* Notify application that data has been received. */
					if(tcpControlBlock->mReceiveCallBack != NULL) {
						error = ((TCP_RECEIVE_CALLBACK)(tcpControlBlock->mReceiveCallBack))
							(tcpControlBlock->mCallbackArgument,tcpControlBlock,gReceiveData,ERR_OK);
					} else if (gReceiveData) {
						PacketBuffer_DecreaseReference(gReceiveData);
					}
				}

				/* If a FIN segment was received, we call the callback
				function with a NULL buffer to indicate EOF. */
				if (gReceiveFlags & TCB_FLAG_GOT_FIN) {
					if(tcpControlBlock->mReceiveCallBack != NULL) {
						error = ((TCP_RECEIVE_CALLBACK)(tcpControlBlock->mReceiveCallBack))
							(tcpControlBlock->mCallbackArgument,tcpControlBlock,NULL,ERR_OK);
					}
				}
				/* If there were no errors, we try to send something out. */
				if (error == ERR_OK) {
					Tcp_SendQueuedData(tcpControlBlock);
				}
			}
		}

		/* give up our reference to gInputSegment.p */
		if (gInputSegment.mPacket != NULL)
		{
			PacketBuffer_DecreaseReference(gInputSegment.mPacket);
			gInputSegment.mPacket = NULL;
		}
		
	} else {

		/* If no matching Control Block was found, send a TCP RST (reset) to the
		sender. */
		SMSC_TRACE(TCP_DEBUG, ("Tcp_Input: no control block match found, resetting."));
		if (!(TCP_HEADER_GET_FLAGS(gTcpHeader) & TCP_HEADER_FLAG_RST)) {
			Tcp_SendReset(gAcknowledgementNumber, gSequenceNumber + gTcpLength,
				gDestinationAddress, gSourceAddress,
				TCP_HEADER_READ_DESTINATION_PORT(gTcpHeader),
				TCP_HEADER_READ_SOURCE_PORT(gTcpHeader));
		}
		goto DROP_PACKET;
	}

	SMSC_ASSERT(Tcp_CheckControlBlock());
	return;
DROP_PACKET:
	PacketBuffer_DecreaseReference(packet);
}

/*eof*/
