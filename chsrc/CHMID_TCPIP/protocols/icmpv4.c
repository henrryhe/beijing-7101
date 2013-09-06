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
FILE: ipv4.c
*/

#include "smsc_environment.h"
#include "task_manager.h"
#include "network_stack.h"
#include "memory_pool.h"
#include "ipv4.h"
#include "icmpv4.h"

#if !IPV4_ENABLED 
/* IPv4 is required */
#error IPv4 is not enabled
#endif

#define ICMP_TYPE_ECHO_REPLY				(0)
#define ICMP_TYPE_DESTINATION_UNREACHABLE	(3)
#define ICMP_TYPE_SOURCE_QUENCH				(4)
#define ICMP_TYPE_REDIRECT					(5)
#define ICMP_TYPE_ECHO_REQUEST				(8)
#define ICMP_TYPE_TIME_EXCEEDED				(11)
#define ICMP_TYPE_PARAMETER_PROBLEM			(12)
#define ICMP_TYPE_TIME_STAMP_REQUEST		(13)
#define ICMP_TYPE_TIME_STAMP_REPLY			(14)
#define ICMP_TYPE_INFORMATION_REQUEST		(15)
#define ICMP_TYPE_INFORMATION_REPLY			(16)

#define ICMPV4_HEADER_SIZE	(8)
#define ICMPV4_HEADER_GET_TYPE(icmpHeader)	(((u8_t *)(icmpHeader))[0])
#define ICMPV4_HEADER_GET_CODE(icmpHeader)	(((u8_t *)(icmpHeader))[1])
#ifndef M_WUFEI_080825
/* NOTE: I'm neglecting to do an endian conversion here to make it consistant
with the set version. If endian conversion is necessary then the user of this
macro should apply the endian convertion.  */
#define ICMPV4_HEADER_GET_CHECK_SUM(icmpHeader)	\
	(((u16_t *)(icmpHeader))[1])
	
#define ICMPV4_HEADER_GET_IDENTIFIER(icmpHeader)	\
	smsc_ntohs(((u16_t *)(icmpHeader))[2])
#define ICMPV4_HEADER_GET_SEQUENCE_NUMBER(icmpHeader)	\
	smsc_ntohs(((u16_t *)(icmpHeader))[3])

#define ICMPV4_HEADER_SET_TYPE(icmpHeader,type)	(((u8_t *)(icmpHeader))[0])=((u8_t)(type))
#define ICMPV4_HEADER_SET_CODE(icmpHeader,code)	(((u8_t *)(icmpHeader))[1])=((u8_t)(code))

/* NOTE: the check sum field should be returned from the check sum calculation in the
  proper endian form therefor there is no need for endian convertion here */
#define ICMPV4_HEADER_SET_CHECK_SUM(icmpHeader,checkSum)	\
	(((u16_t *)(icmpHeader))[1])=(checkSum)

#define ICMPV4_HEADER_SET_IDENTIFIER(icmpHeader,identifier)	\
	(((u16_t *)(icmpHeader))[2])=smsc_htons(identifier)
#define ICMPV4_HEADER_SET_SEQUENCE_NUMBER(icmpHeader,sequenceNumber)	\
	(((u16_t *)(icmpHeader))[3])=smsc_htons(sequenceNumber)
#else
/* NOTE: I'm neglecting to do an endian conversion here to make it consistant
with the set version. If endian conversion is necessary then the user of this
macro should apply the endian convertion.  */
#define ICMPV4_HEADER_GET_CHECK_SUM(icmpHeader)	\
	(((u16_t)icmpHeader[2])+(((u16_t)icmpHeader[3])<<8))
	
#define ICMPV4_HEADER_GET_IDENTIFIER(icmpHeader)	\
	smsc_ntohs(((u16_t)icmpHeader[4])+(((u16_t)icmpHeader[5])<<8))
#define ICMPV4_HEADER_GET_SEQUENCE_NUMBER(icmpHeader)	\
	smsc_ntohs(((u16_t)icmpHeader[6])+(((u16_t)icmpHeader[7])<<8))

#define ICMPV4_HEADER_SET_TYPE(icmpHeader,type)	(((u8_t *)(icmpHeader))[0])=((u8_t)(type))
#define ICMPV4_HEADER_SET_CODE(icmpHeader,code)	(((u8_t *)(icmpHeader))[1])=((u8_t)(code))

/* NOTE: the check sum field should be returned from the check sum calculation in the
  proper endian form therefor there is no need for endian convertion here */
#define ICMPV4_HEADER_SET_CHECK_SUM(icmpHeader,checkSum)	\
	do{	\
		(((u8_t *)(icmpHeader))[2])=(checkSum&0xff);	\
		(((u8_t *)(icmpHeader))[3])=(checkSum&0xff00)>>8;	\
	}while(0)

#define ICMPV4_HEADER_SET_IDENTIFIER(icmpHeader,identifier)	\
	do{	\
		(((u8_t *)(icmpHeader))[4])=(smsc_htons(identifier)&0xff);	\
		(((u8_t *)(icmpHeader))[5])=(smsc_htons(identifier)&0xff00)>>8;	\
	}while(0)

#define ICMPV4_HEADER_SET_SEQUENCE_NUMBER(icmpHeader,sequenceNumber)	\
	do{	\
		(((u8_t *)(icmpHeader))[6])=(smsc_htons(sequenceNumber)&0xff);	\
		(((u8_t *)(icmpHeader))[7])=(smsc_htons(sequenceNumber)&0xff00)>>8;	\
	}while(0)

#endif
#if ICMPV4_ECHO_REQUEST_ENABLED
typedef struct ICMPV4_ECHO_REQUEST_CONTEXT_ {
	struct ICMPV4_ECHO_REQUEST_CONTEXT_ * mNext;
	u16_t mIdentifier;
	u16_t mSequenceNumber;
	ICMPV4_ECHO_REPLY_CALLBACK mCallBackFunction;
	void * mCallBackParam;
	u8_t mTimeOut;
} ICMPV4_ECHO_REQUEST_CONTEXT, * PICMPV4_ECHO_REQUEST_CONTEXT;

u8_t gEchoContextMemory[
	MEMORY_POOL_SIZE(
		sizeof(ICMPV4_ECHO_REQUEST_CONTEXT),
		ICMPV4_ECHO_REQUEST_COUNT)];
MEMORY_POOL_HANDLE gEchoContextPool=NULL;
u16_t gNextSequenceNumber=0;
PICMPV4_ECHO_REQUEST_CONTEXT gEchoContextList=NULL;
TASK gEchoTimerTask;
#define ICMPV4_SCHEDULE_TIME_MS	100/*wufei add for ping test 2008-11-25*/
void Icmpv4_EchoTimerFunction(void * param)
{
	PICMPV4_ECHO_REQUEST_CONTEXT echoContext=gEchoContextList;
	PICMPV4_ECHO_REQUEST_CONTEXT lastContext=NULL;
	while(echoContext!=NULL) {
		echoContext->mTimeOut--;
		if(echoContext->mTimeOut==0) {
			/* this echo Context has expired, perform clean up */
			
			/* save callback information */
			ICMPV4_ECHO_REPLY_CALLBACK callback=echoContext->mCallBackFunction;
			void * callBackParam=echoContext->mCallBackParam;
			
			/* remove echoContext from list */
			PICMPV4_ECHO_REQUEST_CONTEXT next=echoContext->mNext;
			echoContext->mNext=NULL;
			if(lastContext==NULL) {
				gEchoContextList=next;
			} else {
				lastContext->mNext=next;
			}
			MemoryPool_Free(gEchoContextPool,echoContext);
			echoContext=next;
			
			/* call Callback */
			SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(callback!=NULL);
			callback(callBackParam,NULL);/* NULL indicates a time out was reached */
		} else {
			lastContext=echoContext;
			echoContext=echoContext->mNext;
		}
	}
	if((gEchoContextList!=NULL)&&(Task_IsIdle(&gEchoTimerTask))) {
		/* Schedule to be called back in one second */
		TaskManager_ScheduleByTimer(&gEchoTimerTask,ICMPV4_SCHEDULE_TIME_MS);
	}
}
#endif /* ICMPV4_ECHO_REQUEST_ENABLED */

void Icmpv4_Initialize(void)
{
#if ICMPV4_ECHO_REQUEST_ENABLED
	gEchoContextPool=MemoryPool_Initialize(
		gEchoContextMemory,
		sizeof(ICMPV4_ECHO_REQUEST_CONTEXT),
		ICMPV4_ECHO_REQUEST_COUNT,NULL);
	Task_Initialize(
		&gEchoTimerTask,/*TASK_PRIORITY_TIMER*/TASK_PRIORITY_TIMER_ICMP,
		Icmpv4_EchoTimerFunction,NULL);
#endif
}

#if ICMPV4_ECHO_REQUEST_ENABLED
PPACKET_BUFFER Icmpv4_AllocatePacket(
	PIPV4_ADDRESS destinationAddress,
	u32_t size)
{
	PPACKET_BUFFER packet=
		Ipv4_AllocatePacket(
			destinationAddress,
			size+ICMPV4_HEADER_SIZE);
	if(packet!=NULL) {
		PacketBuffer_MoveStartPoint(packet,ICMPV4_HEADER_SIZE);
	}
	return packet;
}

err_t Icmpv4_SendEchoRequest(
	IPV4_ADDRESS destinationAddress,PPACKET_BUFFER packet,
	ICMPV4_ECHO_REPLY_CALLBACK callback,void * callBackParam,
	u8_t timeOut)/*timeOut is in units of seconds */
	/* NOTE: When sending more than one echo request at a time
		the actual time out T may be, timeOut >= T >=(timeOut-1)
		This is because the first echo request starts an echo timer.
		The Echo Timer is called once per second and decreases timeout 
		counters of all echo requests. If a second echo request is issued
		0.9 seconds after the first echo request, then the timeout of the 
		second echo request will be decreased only 0.1 seconds after issuing 
		the request, because that is the time when the echo timer is called, 
		which is 1 second after the first echo request.
		
		At the moment this anomaly is not important enough to correct.*/
{
	PICMPV4_ECHO_REQUEST_CONTEXT context;
	u8_t * icmpHeader;
	
	if(packet==NULL) {
		packet=Icmpv4_AllocatePacket(&destinationAddress,0);
		if(packet==NULL) {
			SMSC_NOTICE(ICMPV4_DEBUG,("Icmpv4_SendEchoRequest: unable to allocate packet"));
			return ERR_MEM;
		}
	}
	
	if(PacketBuffer_MoveStartPoint(packet,-ICMPV4_HEADER_SIZE)!=ERR_OK) {
		/* Packet Must be allocated with Icmpv4_AllocatePacket */
		SMSC_ERROR(("Icmpv4_SendEchoRequest: Unable to move start point"));
	}
	
	icmpHeader=PacketBuffer_GetStartPoint(packet);
	ICMPV4_HEADER_SET_TYPE(icmpHeader,ICMP_TYPE_ECHO_REQUEST);
	ICMPV4_HEADER_SET_CODE(icmpHeader,0);
	ICMPV4_HEADER_SET_IDENTIFIER(icmpHeader,gNextSequenceNumber);
	ICMPV4_HEADER_SET_SEQUENCE_NUMBER(icmpHeader,gNextSequenceNumber);
	
	/* set check sum to zero before calculating check sum below */
	ICMPV4_HEADER_SET_CHECK_SUM(icmpHeader,0);
	
	ICMPV4_HEADER_SET_CHECK_SUM(icmpHeader,
		Ipv4_ChainedCheckSum(packet));
	
	if(callback!=NULL) {
		context=MemoryPool_Allocate(gEchoContextPool);
		if(context==NULL) {
			/* If this error happens increase ICMPV4_ECHO_REQUEST_COUNT */
			SMSC_NOTICE(ICMPV4_DEBUG,("Icmpv4_SendEchoRequest: unable to allocate context object"));
			PacketBuffer_DecreaseReference(packet);
			return ERR_MEM;
		}
		context->mNext=NULL;
		context->mIdentifier=gNextSequenceNumber;
		context->mSequenceNumber=gNextSequenceNumber;
		context->mCallBackFunction=callback;
		context->mCallBackParam=callBackParam;
		context->mTimeOut=timeOut;
		
		if(Task_IsIdle(&gEchoTimerTask)) {
			/* activate the echo timer */
			TaskManager_ScheduleByTimer(&gEchoTimerTask,ICMPV4_SCHEDULE_TIME_MS);
		}
		
		/* Add this echo context to the list */
		context->mNext=gEchoContextList;
		gEchoContextList=context;
	}
    	
	Ipv4_Output(
		packet,
		0,/* source address = 0 = any */
		destinationAddress,
		IP_PROTOCOL_ICMP);

	gNextSequenceNumber++;	
}
#endif /*ICMPV4_ECHO_REQUEST_ENABLED */

void Icmpv4_Input(
	struct NETWORK_INTERFACE * arrivalInterface,
	PPACKET_BUFFER packet)
{
	u8_t * ipv4Header;
	u8_t * icmpHeader;
	u16_t ipHeaderLength;
	PIPV4_ADDRESS destinationAddress;
#ifdef M_WUFEI_080825
	IPV4_ADDRESS addr_buf = 0;
#endif
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(arrivalInterface!=NULL);
	CHECK_SIGNATURE(arrivalInterface,NETWORK_INTERFACE_SIGNATURE);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packet!=NULL);
	CHECK_SIGNATURE(packet,PACKET_BUFFER_SIGNATURE);
	SMSC_ASSERT(packet->mThisLength>=IPV4_HEADER_LENGTH);
	ipv4Header=PacketBuffer_GetStartPoint(packet);
	
	/* Get ip header length in number of 32 bit words */
	ipHeaderLength=IPV4_HEADER_GET_IP_HEADER_LENGTH(ipv4Header);
	
	/* calculated IP header length in number of bytes */
	ipHeaderLength<<=2;
	
	/* move start point */
	if(PacketBuffer_MoveStartPoint(packet,ipHeaderLength)!=ERR_OK) {
		SMSC_NOTICE(ICMPV4_DEBUG,("Icmpv4_Input: packet dropped, unable to move start point"));
		goto DROP_PACKET;
	}
	
	if(packet->mThisLength<4) {
		SMSC_NOTICE(ICMPV4_DEBUG,("Icmpv4_Input: packet dropped, not enough length for ICMP header"));
		goto DROP_PACKET;
	}
	
	icmpHeader=PacketBuffer_GetStartPoint(packet);
	
	switch(ICMPV4_HEADER_GET_TYPE(icmpHeader)) {
#if ICMPV4_ECHO_REPLY_ENABLED
	case ICMP_TYPE_ECHO_REQUEST:
#ifndef M_WUFEI_080825
		if(IPV4_ADDRESS_IS_BROADCAST(
			IPV4_HEADER_GET_DESTINATION_ADDRESS(ipv4Header),
			arrivalInterface)||
		  IPV4_ADDRESS_IS_MULTICAST(
		  	IPV4_HEADER_GET_DESTINATION_ADDRESS(ipv4Header)))
		{
			SMSC_NOTICE(ICMPV4_DEBUG,("Icmpv4_Input:ECHO_REQUEST: packet dropped, Not echoing to multicast or broadcast pings"));
			goto DROP_PACKET;
		}
#else
{
		addr_buf = 0;
		IPV4_HEADER_GET_DESTINATION_ADDRESS(ipv4Header,addr_buf);
		if(IPV4_ADDRESS_IS_BROADCAST(addr_buf,arrivalInterface)||
		  IPV4_ADDRESS_IS_MULTICAST(addr_buf))
		{
			SMSC_NOTICE(ICMPV4_DEBUG,("Icmpv4_Input:ECHO_REQUEST: packet dropped, Not echoing to multicast or broadcast pings"));
			goto DROP_PACKET;
		}
}
#endif
		if(packet->mThisLength<8) 
		{
			/* buffer length is less than ICMP echo header length */
			SMSC_NOTICE(ICMPV4_DEBUG,("Icmpv4_Input:ECHO_REQUEST: packet dropped, not enough length for ICMP echo header"));
			goto DROP_PACKET;
		}
		if(Ipv4_ChainedCheckSum(packet)!=0) {
			SMSC_NOTICE(ICMPV4_DEBUG,("Icmpv4_Input:ECHO_REQUEST: Checksum Failed"));
			goto DROP_PACKET;
		}
		
#ifndef M_WUFEI_080825
		/* All checks passed, proceed with echo reply */
		{/* swap addresses, no need to adjust IP header checksum since
		    this operation will not change the IP header checksum */
			IPV4_ADDRESS tempAddress=IPV4_HEADER_GET_SOURCE_ADDRESS(ipv4Header);
			IPV4_HEADER_SET_SOURCE_ADDRESS(ipv4Header,
				IPV4_HEADER_GET_DESTINATION_ADDRESS(ipv4Header));
			IPV4_HEADER_SET_DESTINATION_ADDRESS(ipv4Header,tempAddress);
		}
		destinationAddress=&(IPV4_HEADER_GET_DESTINATION_ADDRESS(ipv4Header));
#else
{
			IPV4_ADDRESS tempAddress=0;
			addr_buf = 0;
			IPV4_HEADER_GET_SOURCE_ADDRESS(ipv4Header,addr_buf);
			IPV4_HEADER_GET_DESTINATION_ADDRESS(ipv4Header,tempAddress);
			IPV4_HEADER_SET_SOURCE_ADDRESS(ipv4Header,tempAddress);
			IPV4_HEADER_SET_DESTINATION_ADDRESS(ipv4Header,addr_buf);
}
		destinationAddress=&addr_buf;
#endif
		
		/* Set type to echo reply */
		ICMPV4_HEADER_SET_TYPE(icmpHeader,ICMP_TYPE_ECHO_REPLY);
		
		
		{/* adjust ICMP check sum */
			u16_t tempCheckSum=smsc_ntohs(ICMPV4_HEADER_GET_CHECK_SUM(icmpHeader));
			u16_t checkSumOffset=((ICMP_TYPE_ECHO_REQUEST - ICMP_TYPE_ECHO_REPLY)<< 8);
			if(tempCheckSum>(0xFFFF-checkSumOffset))
			{
				/* adding tempCheckSum and checkSumOffset will cause a wrap around */
				/* Add an additional 1 to counter the double zero property of ones complement form*/
				/* that is 0xFFFF and 0x0000 are both zero in ones compliment form */
				tempCheckSum+=checkSumOffset+1;
			} else {
				/* no wrap around, normal addition is sufficient */
				tempCheckSum+=checkSumOffset;
			}
			ICMPV4_HEADER_SET_CHECK_SUM(icmpHeader,smsc_htons(tempCheckSum));
		}
		
		if(PacketBuffer_MoveStartPoint(packet,-((s16_t)ipHeaderLength))!=ERR_OK) {
			/* This should never happen because we are simply moving the start
			  point back to where it was at the beginning of this function */
			SMSC_ERROR(("Icmpv4_Input:ECHO_REQUEST: Failed to move start point"));
		}
		
		{
			struct NETWORK_INTERFACE * destinationInterface=Ipv4_GetRoute(*destinationAddress);
			if(destinationInterface!=NULL) {
				Ipv4_OutputWithHeaderIncluded(destinationInterface,packet,*destinationAddress);
				SMSC_TRACE(ICMPV4_DEBUG,("mServerAddress:%x",destinationInterface->mDhcpData->mServerAddress));
				SMSC_TRACE(ICMPV4_DEBUG,("mOfferedAddress:%x", destinationInterface->mDhcpData->mOfferedAddress));
				SMSC_TRACE(ICMPV4_DEBUG,("mOfferedNetMask:%x",destinationInterface->mDhcpData->mOfferedNetMask));
				SMSC_TRACE(ICMPV4_DEBUG,("mOfferedGateway:%x",destinationInterface->mDhcpData->mOfferedGateway));
				SMSC_TRACE(ICMPV4_DEBUG,("mOfferedBroadcast:",destinationInterface->mDhcpData->mOfferedBroadcast));
				SMSC_TRACE(ICMPV4_DEBUG,("dns1%x",destinationInterface->mDhcpData->mOfferedDns[0]));
				SMSC_TRACE(ICMPV4_DEBUG,("dns2%x",destinationInterface->mDhcpData->mOfferedDns[1]));

			} else {
				SMSC_NOTICE(ICMPV4_DEBUG,("Icmpv4_Input:ECHO_REQUEST: could not find destination interface"));
				goto DROP_PACKET;
			}
		}
		
		break;
#endif /* ICMPV4_ECHO_REPLY_ENABLED */
#if ICMPV4_ECHO_REQUEST_ENABLED
	case ICMP_TYPE_ECHO_REPLY:
		if(packet->mThisLength<8) 
		{
			/* buffer length is less than ICMP echo header length */
			SMSC_NOTICE(ICMPV4_DEBUG,("Icmpv4_Input:ECHO_REPLY: packet dropped, not enough length for ICMP echo header"));
			goto DROP_PACKET;
		}
		if(Ipv4_ChainedCheckSum(packet)!=0) {
			SMSC_NOTICE(ICMPV4_DEBUG,("Icmpv4_Input:ECHO_REPLY: Checksum Failed"));
			goto DROP_PACKET;
		}
		{
			u16_t identifier=ICMPV4_HEADER_GET_IDENTIFIER(icmpHeader);
			u16_t sequenceNumber=ICMPV4_HEADER_GET_SEQUENCE_NUMBER(icmpHeader);
			PICMPV4_ECHO_REQUEST_CONTEXT echoContext=gEchoContextList;
			PICMPV4_ECHO_REQUEST_CONTEXT lastContext=NULL;
			while(echoContext!=NULL) {
				if((echoContext->mIdentifier==identifier)&&
					(echoContext->mSequenceNumber==sequenceNumber))
				{
					/* found matching context */
					
					/* store call back information */
					ICMPV4_ECHO_REPLY_CALLBACK callBackFunction=echoContext->mCallBackFunction;
					void * callBackParam=echoContext->mCallBackParam;

					/* remove echoContext from list */
					PICMPV4_ECHO_REQUEST_CONTEXT next=echoContext->mNext;
					echoContext->mNext=NULL;
					if(lastContext==NULL) {
						gEchoContextList=next;
					} else {
						lastContext->mNext=next;
					}
					MemoryPool_Free(gEchoContextPool,echoContext);
					echoContext=next;
					
					if((gEchoContextList==NULL)&&(!Task_IsIdle(&gEchoTimerTask)))
					{
						/* There is no more need for the echo timer so lets cancel it */
						TaskManager_CancelTask(&gEchoTimerTask);
					}

					/* call Callback */
					SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(callBackFunction!=NULL);
					PacketBuffer_MoveStartPoint(packet,ICMPV4_HEADER_SIZE);
					callBackFunction(callBackParam,packet);
					break;
				} else {
					lastContext=echoContext;
					echoContext=echoContext->mNext;
				}
			}
		}
		break;
#endif /* ICMPV4_ECHO_REQUEST_ENABLED */
	default:
		SMSC_NOTICE(ICMPV4_DEBUG, ("Icmpv4_Input: ICMP type=%"S16_F", code=%"S16_F" not supported.",
			ICMPV4_HEADER_GET_TYPE(icmpHeader),ICMPV4_HEADER_GET_CODE(icmpHeader)));
		goto DROP_PACKET;
	}
	return;
DROP_PACKET:
	PacketBuffer_DecreaseReference(packet);
}

/***********test shift here***************/

static PPACKET_BUFFER gEchoPacket=NULL;
static u8_t gEchoTimeOut=0;
static IPV4_ADDRESS gEchoDestination=0;
static u16_t gOriginalStartOffset=0;
static u16_t gOriginalLength=0;
static s32_t gIcmpv4Test=0;/*wufei add 2008-11-25*/
static void IcmpEchoTest_Reply(void * param, PPACKET_BUFFER packet)
{
	if(packet==NULL) {
		SMSC_NOTICE(1,("Icmpv4EchoTester_Reply: Echo Request Timed Out"));
		PacketBuffer_DecreaseReference(gEchoPacket);
		gEchoPacket=NULL;
		gIcmpv4Test = -1;/*ping time out wufei add 2008-11-25*/
		return;
	} else {
		/* Check packet has incrementing data data */
		u8_t data=0;
		PPACKET_BUFFER buffer=packet;
		while(buffer!=NULL) {
			u16_t index;
			u8_t * dataPointer=PacketBuffer_GetStartPoint(buffer);
			for(index=0;index<buffer->mThisLength;index++,data++) {
				if(dataPointer[index]!=data) {
					SMSC_NOTICE(1,("Icmpv4EchoTester_Reply: Data corruption detected"));
					goto RELEASE_PACKET;
				}
			}
			buffer=buffer->mNext;
		}
		{
		int sourceAddress;		
		sourceAddress=Ipv4_GetSourceAddress(gEchoDestination);
		SMSC_TRACE(1,("From 0x%x to 0x%x Icmpv4EchoTester_Reply: Receive Echo Reply Correctly",sourceAddress,gEchoDestination));
		PacketBuffer_DecreaseReference(packet);
		PacketBuffer_DecreaseReference(gEchoPacket);
		gEchoPacket=NULL;
		gIcmpv4Test = 1;/*ping success wufei add 2008-11-25*/
		return;
		}
	RELEASE_PACKET:
		PacketBuffer_DecreaseReference(packet);
		PacketBuffer_DecreaseReference(gEchoPacket);
		gEchoPacket=NULL;
		gIcmpv4Test = -1;/*ping time out wufei add 2008-11-25*/
		return;		
	}
}

void IcmpEchoTest_Initialize(IPV4_ADDRESS destination, u32_t size, u8_t timeOut)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(gEchoPacket==NULL);
	gEchoPacket=Icmpv4_AllocatePacket(&destination,size);
	if(gEchoPacket==NULL) {
		SMSC_NOTICE(1,("Icmpv4EchoTester_Initialize: Failed to allocate gEchoPacket"));
		return;
	}
	
	/* store original start offset, and length */
	gOriginalStartOffset=gEchoPacket->mStartOffset;
	gOriginalLength=gEchoPacket->mThisLength;
	SMSC_ASSERT(gOriginalLength==gEchoPacket->mTotalLength);
	
	/* We will be using this same packet again and again
		so lets hold a reference for it */
	PacketBuffer_IncreaseReference(gEchoPacket);
	
	{/* Initialize packet with incrementing data */
		u8_t data=0;
		PPACKET_BUFFER buffer=gEchoPacket;
		while(buffer!=NULL) {
			u16_t index;
			u8_t * dataPointer=PacketBuffer_GetStartPoint(buffer);
			for(index=0;index<buffer->mThisLength;index++,data++) {
				dataPointer[index]=data;
			}
			buffer=buffer->mNext;
		}
	}	
	
	/* store period and destination */
	gEchoTimeOut=timeOut;
	gEchoDestination=destination;
	gIcmpv4Test = 0;
	/* Send Echo Request */
	Icmpv4_SendEchoRequest(
		gEchoDestination,gEchoPacket,
		IcmpEchoTest_Reply,NULL,
		gEchoTimeOut);
}

s32_t Icmpv4EchoReply(void)
{
	return gIcmpv4Test;
}
/*eof*/
