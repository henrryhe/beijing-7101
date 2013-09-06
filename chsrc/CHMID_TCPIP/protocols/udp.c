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
FILE: udp.c
Description: 
	Implements UDP
*/

#include "smsc_environment.h"

#if !UDP_ENABLED
#error UDP is not enabled
#endif

#include "ip.h"
#include "udp.h"
#include "memory_pool.h"
#include "network_stack.h"

#define UDP_HEADER_LENGTH		(8)
#ifndef M_WUFEI_080825
#define UDP_HEADER_GET_SOURCE_PORT(udpHeader)	\
	smsc_ntohs(((u16_t *)(udpHeader))[0])
#define UDP_HEADER_GET_DESTINATION_PORT(udpHeader)	\
	smsc_ntohs(((u16_t *)(udpHeader))[1])
#define UDP_HEADER_GET_LENGTH(udpHeader)	\
	smsc_ntohs(((u16_t *)(udpHeader))[2])

/* NOTE: UDP_HEADER_GET_CHECKSUM does not include 
	endian conversion. The user is assumed to take care of that,
	and checksum calculations may take advantage of this */
#define UDP_HEADER_GET_CHECKSUM(udpHeader)		\
	(((u16_t *)(udpHeader))[3])
	
#define UDP_HEADER_SET_SOURCE_PORT(udpHeader,sourcePort)	\
	(((u16_t *)(udpHeader))[0])=smsc_htons(sourcePort)
#define UDP_HEADER_SET_DESTINATION_PORT(udpHeader,destinationPort)	\
	(((u16_t *)(udpHeader))[1])=smsc_htons(destinationPort)	
#define UDP_HEADER_SET_LENGTH(udpHeader,length)	\
	(((u16_t *)(udpHeader))[2])=smsc_htons(length)

/* NOTE: UDP_HEADER_SET_CHECKSUM does not include
	endian conversion. The user is assumed to take care of that,
	and checksum calculations may take advantage of this */
#define UDP_HEADER_SET_CHECKSUM(udpHeader,checksum)	\
	(((u16_t *)(udpHeader))[3])=((u16_t)(checksum))
#else
#define UDP_HEADER_GET_SOURCE_PORT(udpHeader)	\
	smsc_ntohs((u16_t)udpHeader[0] + (((u16_t)udpHeader[1])<<8))
#define UDP_HEADER_GET_DESTINATION_PORT(udpHeader)	\
	smsc_ntohs((u16_t)udpHeader[2] + (((u16_t)udpHeader[3])<<8))
#define UDP_HEADER_GET_LENGTH(udpHeader)	\
	smsc_ntohs((u16_t)udpHeader[4] + (((u16_t)udpHeader[5])<<8))

/* NOTE: UDP_HEADER_GET_CHECKSUM does not include 
	endian conversion. The user is assumed to take care of that,
	and checksum calculations may take advantage of this */
#define UDP_HEADER_GET_CHECKSUM(udpHeader)		\
	(((u16_t)udpHeader[6])<<8 + (u16_t)udpHeader[7])	
#define UDP_HEADER_SET_SOURCE_PORT(udpHeader,sourcePort)	\
	do{	\
		udpHeader[0] = smsc_htons(sourcePort)&0xff;	\
		udpHeader[1] = (smsc_htons(sourcePort)>>8)&0xff;	\
	}while(0)
#define UDP_HEADER_SET_DESTINATION_PORT(udpHeader,destinationPort)	\
	do{	\
		udpHeader[2] = smsc_htons(destinationPort)&0xff;	\
		udpHeader[3] = (smsc_htons(destinationPort)>>8)&0xff;	\
	}while(0)	
#define UDP_HEADER_SET_LENGTH(udpHeader,length)	\
	do{	\
		udpHeader[4] = smsc_htons(length)&0xff;	\
		udpHeader[5] = (smsc_htons(length)>>8)&0xff;	\
	}while(0)	
/* NOTE: UDP_HEADER_SET_CHECKSUM does not include
	endian conversion. The user is assumed to take care of that,
	and checksum calculations may take advantage of this */
#if 0
#define UDP_HEADER_SET_CHECKSUM(udpHeader,checksum)	\
	do{	\
		udpHeader[6] = smsc_htons(checksum)&0xff;	\
		udpHeader[7] = (smsc_htons(checksum)>>8)&0xff;	\
	}while(0)	
#else
#define UDP_HEADER_SET_CHECKSUM(udpHeader,checksum)	\
	(((u16_t *)(udpHeader))[3])=((u16_t)(checksum))

#endif

#endif
static u8_t gUdpControlBlockMemory[
	MEMORY_POOL_SIZE(
		sizeof(struct UDP_CONTROL_BLOCK),
		UDP_CONTROL_BLOCK_COUNT)];
/*static*/ MEMORY_POOL_HANDLE gUdpControlBlockPool=NULL;

struct UDP_CONTROL_BLOCK * gUdpControlBlockList;

 /* Main initialization function must be called before any other Udp_ function */
void Udp_Initialize()
{
	gUdpControlBlockPool=MemoryPool_Initialize(
		gUdpControlBlockMemory,
		sizeof(struct UDP_CONTROL_BLOCK),
		UDP_CONTROL_BLOCK_COUNT,NULL);
	gUdpControlBlockList=NULL;
}

/* Application interface functions */
struct UDP_CONTROL_BLOCK * Udp_CreateControlBlock()
{
	struct UDP_CONTROL_BLOCK * result=(struct UDP_CONTROL_BLOCK *)MemoryPool_Allocate(gUdpControlBlockPool);
	if(result!=NULL) {
		memset(result,0,sizeof(struct UDP_CONTROL_BLOCK));
		ASSIGN_SIGNATURE(result,UDP_CONTROL_BLOCK_SIGNATURE);
	}
	return result;
}
void Udp_FreeControlBlock(struct UDP_CONTROL_BLOCK * udpControlBlock)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(udpControlBlock!=NULL);
	CHECK_SIGNATURE(udpControlBlock,UDP_CONTROL_BLOCK_SIGNATURE);
	
	/*make sure we are not still binded */
	SMSC_ASSERT(udpControlBlock->mLocalPort==0);
	SMSC_ASSERT(udpControlBlock->mNext==NULL);
	ASSIGN_SIGNATURE(udpControlBlock,0);
	
	MemoryPool_Free(gUdpControlBlockPool,udpControlBlock);
}

err_t Udp_Bind(struct UDP_CONTROL_BLOCK * udpControlBlock,PIP_ADDRESS localAddress,u16_t localPort)
{
	struct UDP_CONTROL_BLOCK * tempControlBlock;
	u8_t rebind;
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(udpControlBlock!=NULL,ERR_ARG);
	CHECK_SIGNATURE(udpControlBlock,UDP_CONTROL_BLOCK_SIGNATURE);

	#if SMSC_TRACE_ENABLED
	{
		char ipAddressString[IP_ADDRESS_STRING_SIZE];
		SMSC_TRACE(UDP_DEBUG, ("Udp_Bind(localAddress = %s, localPort = %"U16_F")",
			IP_ADDRESS_TO_STRING(ipAddressString,localAddress),localPort));
	}
	#endif

	rebind = 0;
	/* Check for double bind and rebind of the same pcb */
	for (tempControlBlock = gUdpControlBlockList; tempControlBlock != NULL; tempControlBlock = tempControlBlock->mNext) {
		/* is this UDP PCB already on active list? */
		if (udpControlBlock == tempControlBlock) {
			/* This udpControlBlock is already on the active list */
			/* A udpControlBlock may occur at most once in active list */
			SMSC_ASSERT(rebind == 0);
			/* udpControlBlock is already in list, just rebind */
			rebind = 1;
		}

		/* this code does not allow upper layer to share a UDP port for
			listening to broadcast or multicast traffic (See SO_REUSE_ADDR and
			SO_REUSE_PORT under *BSD). TODO: See where it fits instead, OR
			combine with implementation of UDP PCB flags. Leon Woestenberg. */
		else
		if ((tempControlBlock->mLocalPort == localPort) &&
			(IP_ADDRESS_IS_ANY(&(tempControlBlock->mLocalAddress)) ||
			IP_ADDRESS_IS_ANY(localAddress) ||
			IP_ADDRESS_COMPARE(&(tempControlBlock->mLocalAddress), localAddress))) 
		{
			/* other context already binds to this local IP and port */
			SMSC_NOTICE(UDP_DEBUG,
				("Udp_Bind: local port %"U16_F" already bound by another udpControlBlock", localPort));
			return ERR_USE;
		}
	}
	if(localAddress!=NULL) {
		IP_ADDRESS_COPY(&(udpControlBlock->mLocalAddress),localAddress);
	} else {
		IP_ADDRESS_COPY(&(udpControlBlock->mLocalAddress),&IP_ADDRESS_ANY);
	}

	if (localPort == 0) {
		/* no port specified */
		localPort = UDP_LOCAL_PORT_RANGE_START;
		tempControlBlock = gUdpControlBlockList;
		while ((tempControlBlock != NULL) && (localPort != UDP_LOCAL_PORT_RANGE_END)) {
			if (tempControlBlock->mLocalPort == localPort) {
				localPort++;
				tempControlBlock = gUdpControlBlockList;
			} else
				tempControlBlock = tempControlBlock->mNext;
		}
		if (tempControlBlock != NULL) {
			/* no more ports available in local range */
			SMSC_NOTICE(UDP_DEBUG, ("Udp_Bind: out of free UDP ports\n"));
			return ERR_USE;
		}
	}
	udpControlBlock->mLocalPort = localPort;
	if (rebind == 0) {
		/* udpControlBlock is not active yet */
		/* place the UDP_CONTROL_BLOCK on the active list */
		udpControlBlock->mNext = gUdpControlBlockList;
		gUdpControlBlockList = udpControlBlock;
	}
	#if SMSC_TRACE_ENABLED
	{
		char ipAddressString[IP_ADDRESS_STRING_SIZE];
		SMSC_TRACE(UDP_DEBUG,
			("Udp_Bind: bound to %s, port %"U16_F,
				IP_ADDRESS_TO_STRING(ipAddressString,&(udpControlBlock->mLocalAddress)),
				udpControlBlock->mLocalPort));
	}
	#endif
	return ERR_OK;
}

void Udp_UnBind(struct UDP_CONTROL_BLOCK * udpControlBlock)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(udpControlBlock!=NULL);
	CHECK_SIGNATURE(udpControlBlock,UDP_CONTROL_BLOCK_SIGNATURE);

	/*Remote udpControlBlock from list */
	if(gUdpControlBlockList==udpControlBlock) {
		gUdpControlBlockList=gUdpControlBlockList->mNext;
	} else {
		struct UDP_CONTROL_BLOCK * tempControlBlock=gUdpControlBlockList;
		while(tempControlBlock!=NULL) {
			if(tempControlBlock->mNext==udpControlBlock) {
				/* remove from list */
				tempControlBlock->mNext=udpControlBlock->mNext;
			}
			tempControlBlock=tempControlBlock->mNext;
		}
	}
	udpControlBlock->mLocalPort=0;
	udpControlBlock->mNext=NULL;
}

void Udp_SetReceiverCallBack(
	struct UDP_CONTROL_BLOCK * udpControlBlock,
	UDP_RECEIVE_FUNCTION receiverFunction,void * receiverParam)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(udpControlBlock!=NULL);
	CHECK_SIGNATURE(udpControlBlock,UDP_CONTROL_BLOCK_SIGNATURE);
	udpControlBlock->mReceiverCallBack=receiverFunction;
	udpControlBlock->mReceiverCallBackParam=receiverParam;
}

err_t Udp_Connect(
	struct UDP_CONTROL_BLOCK * udpControlBlock,
	PIP_ADDRESS remoteAddress,u16_t remotePort)
{
	if (udpControlBlock->mLocalPort == 0) {
		err_t err = Udp_Bind(udpControlBlock, 
							&udpControlBlock->mLocalAddress, 
							udpControlBlock->mLocalPort);
		if (err != ERR_OK)
			return err;
	}

	IP_ADDRESS_COPY(&udpControlBlock->mRemoteAddress,remoteAddress);
	udpControlBlock->mRemotePort = remotePort;
	udpControlBlock->mFlags |= UDP_FLAGS_CONNECTED;

#if 0
	/** TODO: this functionality belongs in upper layers */
	/* Nail down local IP for netconn_addr()/getsockname() */
	if (IP_ADDRESS_IS_ANY(&udpControlBlock->mLocalAddress) && !IP_ADDRESS_IS_ANY(&udpControlBlock->mRemoteAddress)) 
	{
		/** TODO: this will bind the udp control block locally, to the interface which
			is used to route output packets to the remote address. However, we
			might want to accept incoming packets on any interface! */
		Ip_GetSourceAddress(&(udpControlBlock->mLocalAddress),&(udpControlBlock->mRemoteAddress));
	} else if (IP_ADDRESS_IS_ANY(&udpControlBlock->mRemoteAddress)) {
		IP_ADDRESS_COPY(&(udpControlBlock->mLocalAddress),&IP_ADDRESS_ANY);
	}
#endif
	
	#if SMSC_TRACE_ENABLED
	{
		char addressString[IP_ADDRESS_STRING_SIZE];
		SMSC_TRACE(UDP_DEBUG,
			("udp_connect: connected to %s:%"U16_F,
				IP_ADDRESS_TO_STRING(addressString,&(udpControlBlock->mRemoteAddress)),
				udpControlBlock->mRemotePort));
	}
	#endif

	{
		struct UDP_CONTROL_BLOCK * tempControlBlock;
		/* Insert UDP PCB into the list of active UDP PCBs. */
		for (tempControlBlock = gUdpControlBlockList; tempControlBlock != NULL; tempControlBlock = tempControlBlock->mNext) {
			if (udpControlBlock == tempControlBlock) {
				/* already on the list, just return */
				return ERR_OK;
			}
		}
	}
	/* Control Block not yet on the list, add control block now */
	udpControlBlock->mNext = gUdpControlBlockList;
	gUdpControlBlockList = udpControlBlock;
	return ERR_OK;
}

void Udp_Disconnect(struct UDP_CONTROL_BLOCK * udpControlBlock)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(udpControlBlock!=NULL);
	CHECK_SIGNATURE(udpControlBlock,UDP_CONTROL_BLOCK_SIGNATURE);
	/* reset remote address association */
	IP_ADDRESS_COPY(&(udpControlBlock->mRemoteAddress), &IP_ADDRESS_ANY);
	udpControlBlock->mRemotePort=0;
	/* mark context as unconnected */
	udpControlBlock->mFlags &= ~UDP_FLAGS_CONNECTED;
}

PPACKET_BUFFER Udp_AllocatePacket(PIP_ADDRESS destinationAddress,u32_t size)
{
	PPACKET_BUFFER packet=
		Ip_AllocatePacket(
			destinationAddress,
			size+UDP_HEADER_LENGTH);
	if(packet!=NULL) {
		PacketBuffer_MoveStartPoint(packet,UDP_HEADER_LENGTH);
	}
	return packet;
}

void Udp_SendTo(
	struct UDP_CONTROL_BLOCK * udpControlBlock,PPACKET_BUFFER packet,
	PIP_ADDRESS destinationAddress, u16_t destinationPort)
{
	u8_t * udpHeader;
	IP_ADDRESS sourceAddress;
	u32_t totalLength;
	u16_t checkSum;
	u8_t protocol;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(udpControlBlock!=NULL);
	CHECK_SIGNATURE(udpControlBlock,UDP_CONTROL_BLOCK_SIGNATURE);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(destinationAddress!=NULL);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packet!=NULL);
	CHECK_SIGNATURE(packet,PACKET_BUFFER_SIGNATURE);
	SMSC_ASSERT(packet->mNext==NULL);/* fragments not supported */

	SMSC_TRACE(UDP_DEBUG, ("Udp_SendTo"));

	if (udpControlBlock->mLocalPort == 0) {
		SMSC_TRACE(UDP_DEBUG, ("Udp_SendTo: not yet bound to a port, binding now"));
		if(Udp_Bind(udpControlBlock, 
				&(udpControlBlock->mLocalAddress),
				udpControlBlock->mLocalPort) != ERR_OK)
		{
			SMSC_NOTICE(UDP_DEBUG, ("Udp_SendTo: forced port bind failed"));
			PacketBuffer_DecreaseReference(packet);
		}
	}
	
	if(PacketBuffer_MoveStartPoint(packet,-UDP_HEADER_LENGTH)!=ERR_OK) {
		SMSC_ERROR(("Udp_SendTo: Unable to more start point"));
	}
	
	totalLength=packet->mTotalLength;
	
	udpHeader=PacketBuffer_GetStartPoint(packet);
	
	UDP_HEADER_SET_SOURCE_PORT(udpHeader,udpControlBlock->mLocalPort);
	UDP_HEADER_SET_DESTINATION_PORT(udpHeader,destinationPort);
	UDP_HEADER_SET_LENGTH(udpHeader,totalLength);
	
	/* Set check sum to zero before calculating checksum below */
	UDP_HEADER_SET_CHECKSUM(udpHeader,0);

	if (IP_ADDRESS_IS_ANY(&(udpControlBlock->mLocalAddress))) {
		/* Local address is not assigned
		 get the address of the interface for which this packet will
		 go out on */
		Ip_GetSourceAddress(&sourceAddress,destinationAddress);
	} else {
		/* use UDP CONTROL BLOCK local IP address as source address */
		IP_ADDRESS_COPY(&sourceAddress,&(udpControlBlock->mLocalAddress));
	}

	SMSC_TRACE(UDP_DEBUG, ("Udp_SendTo: sending datagram of length %"U32_F, packet->mTotalLength));

	if (udpControlBlock->mFlags & UDP_FLAGS_UDPLITE) {
		protocol=IP_PROTOCOL_UDPLITE;    
	} else {      /* UDP */
		protocol=IP_PROTOCOL_UDP;
	}
	
	/* make sure the following typecast is acceptable */
	SMSC_ASSERT(!(totalLength&0xFFFF0000));	
	
	checkSum=Ip_PseudoCheckSum(
		packet,&sourceAddress,destinationAddress,
		protocol,(u16_t)totalLength);
	/* checkSum zero must become 0xffff, as zero means 'no checksum' */
	if (checkSum == 0x0000)
		checkSum = 0xffff;
	UDP_HEADER_SET_CHECKSUM(udpHeader,checkSum);
	
	/* output to IP */
	Ip_Output(packet, &sourceAddress, destinationAddress, protocol);
}

void Udp_Send(
	struct UDP_CONTROL_BLOCK * udpControlBlock,PPACKET_BUFFER packet)
{
	Udp_SendTo(udpControlBlock,packet,
		&(udpControlBlock->mRemoteAddress),
		udpControlBlock->mRemotePort);
}

/* lower layer interface function */
void Udp_Input(
	struct NETWORK_INTERFACE * networkInterface, PPACKET_BUFFER packet, 
	PIP_ADDRESS destinationAddress,PIP_ADDRESS sourceAddress,
	u8_t protocol)
{
	u8_t * udpHeader;
	u16_t sourcePort;
	u16_t destinationPort;
	struct UDP_CONTROL_BLOCK * udpControlBlock;
	struct UDP_CONTROL_BLOCK * unconnectedControlBlock;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packet!=NULL);
	CHECK_SIGNATURE(packet,PACKET_BUFFER_SIGNATURE);
	
	if (packet->mThisLength < UDP_HEADER_LENGTH) {
		/* drop short packets */
		SMSC_NOTICE(UDP_DEBUG,
			("Udp_Input: short UDP datagram (%"U32_F" bytes) discarded", packet->mTotalLength));
		goto DROP_PACKET;
	}
	
	udpHeader=PacketBuffer_GetStartPoint(packet);

	SMSC_TRACE(UDP_DEBUG, ("Udp_Input: received datagram of length %"U32_F, packet->mTotalLength));

	sourcePort=UDP_HEADER_GET_SOURCE_PORT(udpHeader);
	destinationPort=UDP_HEADER_GET_DESTINATION_PORT(udpHeader);

	#if SMSC_TRACE_ENABLED
	{/* print the UDP source and destination */
		char sourceString[IP_ADDRESS_STRING_SIZE];
		char destinationString[IP_ADDRESS_STRING_SIZE];		
		SMSC_TRACE(UDP_DEBUG,
				("udp (%s, %"U16_F") <-- "
				"(%s, %"U16_F")",
				IP_ADDRESS_TO_STRING(destinationString,destinationAddress), destinationPort,
				IP_ADDRESS_TO_STRING(sourceString,sourceAddress),sourcePort));
	}
	#endif

	unconnectedControlBlock=NULL;
	/* Iterate through the UDP context list for a matching context object */
	for (udpControlBlock = gUdpControlBlockList; udpControlBlock != NULL; udpControlBlock = udpControlBlock->mNext) {
		/* compare udpControlBlock local addr+port to UDP destination addr+port */
		if ((udpControlBlock->mLocalPort == destinationPort) &&
			(IP_ADDRESS_IS_ANY(&(udpControlBlock->mLocalAddress)) ||
			 IP_ADDRESS_COMPARE(&(udpControlBlock->mLocalAddress), destinationAddress) || 
			 IP_ADDRESS_IS_BROADCAST(destinationAddress, networkInterface)))
		{
			if ((unconnectedControlBlock == NULL) && 
				((udpControlBlock->mFlags & UDP_FLAGS_CONNECTED) == 0)) 
			{
				/* the first unconnected matching udpControlBlock */     
				unconnectedControlBlock = udpControlBlock;
			}
			/* compare udpControlBlock remote addr+port to UDP source addr+port */
			if ((udpControlBlock->mRemotePort == sourcePort) &&
				(IP_ADDRESS_IS_ANY(&(udpControlBlock->mRemoteAddress)) ||
				IP_ADDRESS_COMPARE(&(udpControlBlock->mRemoteAddress), sourceAddress))) 
			{
				/* the first fully matching udpControlBlock */
				break;
			}
		}
	}
	if (udpControlBlock == NULL) {
		/* no fully matching udpControlBlock found */
		/* grab the first unconnected context */
		udpControlBlock = unconnectedControlBlock;
	}

	/* Check checksum if this is a match or if it was directed at us. */
	if (udpControlBlock != NULL) {
		SMSC_TRACE(UDP_DEBUG, ("Udp_Input: calculating checksum"));
		
		/* make sure the following typecast is acceptable */
		SMSC_ASSERT(!((packet->mTotalLength)&0xFFFF0000));  
		
		if (protocol == IP_PROTOCOL_UDPLITE) {		
		#if 0/*20090116 del for CHECK SUM CHECK*/	
			/* Do the UDP Lite checksum */
			if (Ip_PseudoCheckSum(packet, 
				sourceAddress,destinationAddress,
				IP_PROTOCOL_UDPLITE,((u16_t)packet->mTotalLength)) != 0) 
			{
				SMSC_NOTICE(UDP_DEBUG,
					("Udp_Input: UDP Lite datagram discarded due to failing checksum"));
				goto DROP_PACKET;
			}
		#endif
		} else {
			SMSC_ASSERT(protocol==IP_PROTOCOL_UDP);
			#if 0/*20090116 del for CHECK SUM CHECK*/
			if (UDP_HEADER_GET_CHECKSUM(udpHeader) != 0) {
				if (Ip_PseudoCheckSum(packet, 
					sourceAddress,destinationAddress,
					IP_PROTOCOL_UDP,((u16_t)packet->mTotalLength)) != 0) 
				{
					SMSC_NOTICE(UDP_DEBUG,
						("Udp_Input: UDP datagram discarded due to failing checksum"));
					goto DROP_PACKET;
				}
			}
			#endif
		}
		if(PacketBuffer_MoveStartPoint(packet,UDP_HEADER_LENGTH)!=ERR_OK) {
			/* This should never happen since we verified the packet length above */
			SMSC_ERROR(("Udp_Input: unable to move start point"));
		}

		if (udpControlBlock->mReceiverCallBack != NULL) {
			/* Call the receive function */
			((UDP_RECEIVE_FUNCTION)(udpControlBlock->mReceiverCallBack))(
				udpControlBlock->mReceiverCallBackParam,
				udpControlBlock,packet,sourceAddress,sourcePort);
		} else {
			SMSC_NOTICE(UDP_DEBUG,("Udp_Input: Received packet but no receive function available"));
			goto DROP_PACKET;
		}
	} else {
		SMSC_TRACE(UDP_DEBUG, ("Udp_Input: not for us."));
		/* No match was found, send ICMP destination port unreachable unless
			destination address was broadcast/multicast. */
		if (!IP_ADDRESS_IS_BROADCAST(destinationAddress, networkInterface) &&
			!IP_ADDRESS_IS_MULTICAST(destinationAddress)) 
		{
			SMSC_NOTICE(UDP_DEBUG,("Udp_Input: Should send ICMP destination unreachable, but thats not implemented"));
			/* restore pbuf pointer */
			/*p->payload = iphdr;
			icmp_dest_unreach(p, ICMP_DUR_PORT);*/
		}
		goto DROP_PACKET;
	}
	return;
DROP_PACKET:
	PacketBuffer_DecreaseReference(packet);
}
/*eof*/
