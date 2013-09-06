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

#if !IPV4_ENABLED
#error IPv4 is not enabled
#endif

#include "network_stack.h"
#include "ipv4.h"
#include "icmpv4.h"

#if TCP_ENABLED
#include "tcp.h"
#endif

#if UDP_ENABLED
#include "udp.h"
#endif

	
#if SMSC_ERROR_ENABLED
#define IPV4_DATA_SIGNATURE		(0xE59C7321)
#endif

IPV4_ADDRESS IPV4_ADDRESS_ANY=0;
IPV4_ADDRESS IPV4_ADDRESS_BROADCAST=0xFFFFFFFF;

void Ipv4_Initialize(void)
{
	/* Check to make sure the IPV4_ADDRESS is the correct size */
	SMSC_ASSERT(sizeof(IPV4_ADDRESS)==4);
	Icmpv4_Initialize();
}

struct NETWORK_INTERFACE * Ipv4_GetRoute(IPV4_ADDRESS destination)
{
	struct NETWORK_INTERFACE * result=gInterfaceList;
	while(result!=NULL) {
		if(IPV4_ADDRESS_NETWORK_COMPARE(destination,
			result->mIpv4Data.mAddress,
			result->mIpv4Data.mNetMask))
		{
#if LOOP_BACK_ENABLED
			if(IPV4_ADDRESS_COMPARE(destination,
				result->mIpv4Data.mAddress))
			{
				/* perfect match, send to loop back interface */
				return &gLoopBackInterface;
			} else 
#endif /* LOOP_BACK_ENABLED */
			return result;
		}
		result=result->mNext;
	}
	return gDefaultInterface;
}

IPV4_ADDRESS Ipv4_GetSourceAddress(IPV4_ADDRESS destination)
{
	struct NETWORK_INTERFACE * interface=gInterfaceList;
	while(interface!=NULL) {
		if(IPV4_ADDRESS_NETWORK_COMPARE(destination,
			interface->mIpv4Data.mAddress,
			interface->mIpv4Data.mNetMask))
		{
			return interface->mIpv4Data.mAddress;
		}
		interface=interface->mNext;
	}
	if(gDefaultInterface!=NULL) {
		return gDefaultInterface->mIpv4Data.mAddress;
	}
	return 0;
}

u16_t Ipv4_GetMaximumTransferUnit(IPV4_ADDRESS destination)
{
	
	struct NETWORK_INTERFACE * interface=gInterfaceList;
	while(interface!=NULL) {
		if(IPV4_ADDRESS_NETWORK_COMPARE(destination,
			interface->mIpv4Data.mAddress,
			interface->mIpv4Data.mNetMask))
		{
			SMSC_ASSERT(interface->mMTU>0);
			return (interface->mMTU)-IPV4_HEADER_LENGTH;
		}
		interface=interface->mNext;
	}
	if(gDefaultInterface!=NULL) {
		SMSC_ASSERT(gDefaultInterface->mMTU>0);
		return (gDefaultInterface->mMTU)-IPV4_HEADER_LENGTH;
	}
	return 0;
}

void Ipv4_InitializeInterface(
	struct NETWORK_INTERFACE * networkInterface,
	IPV4_OUTPUT_FUNCTION outputFunction)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(outputFunction!=NULL);
	memset(&(networkInterface->mIpv4Data),0,
		sizeof(networkInterface->mIpv4Data));
	ASSIGN_SIGNATURE(&(networkInterface->mIpv4Data),IPV4_DATA_SIGNATURE);
	networkInterface->mIpv4Data.mOutputFunction=outputFunction;	
}

void Ipv4_SetAddresses(
	struct NETWORK_INTERFACE * networkInterface,
	IPV4_ADDRESS ipAddress,
	IPV4_ADDRESS netMask,
	IPV4_ADDRESS gateway)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	networkInterface->mIpv4Data.mAddress=ipAddress;
	networkInterface->mIpv4Data.mNetMask=netMask;
	networkInterface->mIpv4Data.mGateway=gateway;
}

PPACKET_BUFFER Ipv4_AllocatePacket(
	PIPV4_ADDRESS destinationAddress,
	u32_t size)
{
	PPACKET_BUFFER packetBuffer=NULL;
	PPACKET_BUFFER lastBuffer=NULL;
	u32_t mtu;
	struct NETWORK_INTERFACE * networkInterface;
	
	if(destinationAddress!=NULL) {
		networkInterface=Ipv4_GetRoute(*destinationAddress);
		SMSC_FUNCTION_PARAMS_CHECK_RETURN(networkInterface!=NULL,NULL);
		mtu=(u32_t)networkInterface->mMTU;
	} else {
		mtu=MINIMUM_MTU;
	}
	SMSC_ASSERT(size<=0x10000);/* IPV4 does not support data grams larger than 64K */
	SMSC_ASSERT(mtu>=(0x8+20));	/* make sure mtu has been assigned and that it is at least as large as a fragment unit */
	mtu-=20;/* every unit will require an IPV4 header so lets subtract it */
	while(size>0) {
		PPACKET_BUFFER newBuffer;
		u32_t thisSize=size;
		if(thisSize>mtu) {
			thisSize=mtu&(~(0x7));/* make sure thisSize is 8 byte aligned as all fragments must be */
		}
		size-=thisSize;
		newBuffer=PacketBuffer_Create((u16_t)(thisSize+16+20));/* add 16 for link header and 20 for IPv4 header */
		if(newBuffer==NULL) {
			if(packetBuffer!=NULL) {
				PacketBuffer_DecreaseReference(packetBuffer);
			}
			return NULL;
		}
		PacketBuffer_MoveStartPoint(newBuffer,16+20);
		if(packetBuffer==NULL) {
			SMSC_ASSERT(lastBuffer==NULL);
			packetBuffer=newBuffer;
			lastBuffer=newBuffer;
		} else {
			SMSC_FUNCTION_PARAMS_CHECK_RETURN(lastBuffer!=NULL,NULL);
			SMSC_ASSERT(lastBuffer->mNext==NULL);
			lastBuffer->mNext=newBuffer;
			lastBuffer=newBuffer;
		}
	}
	return packetBuffer;
}

char * IPV4_ADDRESS_TO_STRING(char * stringDataSpace,IPV4_ADDRESS address)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(stringDataSpace!=NULL,NULL);
	sprintf(stringDataSpace,
		"%"U16_F".%"U16_F".%"U16_F".%"U16_F,
		(u16_t)IPV4_ADDRESS_GET_BYTE_1(address),
		(u16_t)IPV4_ADDRESS_GET_BYTE_2(address),
		(u16_t)IPV4_ADDRESS_GET_BYTE_3(address),
		(u16_t)IPV4_ADDRESS_GET_BYTE_4(address));
	return stringDataSpace;
}

#if SMSC_TRACE_ENABLED
static void IPV4_HEADER_PRINT(int debugMode, u8_t * headerPointer)
{
	char ipAddressString[IPV4_ADDRESS_STRING_SIZE];
	SMSC_TRACE(debugMode,("IP HEADER"));
	SMSC_TRACE(debugMode,("  Version = %"U16_F,
		(u16_t)IPV4_HEADER_GET_VERSION(headerPointer)));
	SMSC_TRACE(debugMode,("  IP Header Length = %"U16_F" (32 bit words)",
		(u16_t)IPV4_HEADER_GET_IP_HEADER_LENGTH(headerPointer)));
	SMSC_TRACE(debugMode,("  Type of Service = 0x%02"X16_F,
		(u16_t)IPV4_HEADER_GET_TYPE_OF_SERVICE(headerPointer)));
	SMSC_TRACE(debugMode,("  Total Length = %"U16_F" (octets)",
		(u16_t)IPV4_HEADER_GET_TOTAL_LENGTH(headerPointer)));
	SMSC_TRACE(debugMode,("  Identification = 0x%"X16_F"==%"U16_F,
		(u16_t)IPV4_HEADER_GET_IDENTIFICATION(headerPointer),
		(u16_t)IPV4_HEADER_GET_IDENTIFICATION(headerPointer)));
	SMSC_TRACE(debugMode,("  Flags = 0x%"X16_F,
		(u16_t)IPV4_HEADER_GET_FLAGS(headerPointer)));
	SMSC_TRACE(debugMode,("  Fragment Offset = %"U16_F" (8 octets)",
		(u16_t)IPV4_HEADER_GET_FRAGMENT_OFFSET(headerPointer)));
	SMSC_TRACE(debugMode,("  Time to Live = %"U16_F" (seconds/hops)",
		(u16_t)IPV4_HEADER_GET_TIME_TO_LIVE(headerPointer)));
	SMSC_TRACE(debugMode,("  Protocol = %"U16_F,
		(u16_t)IPV4_HEADER_GET_PROTOCOL(headerPointer)));
	SMSC_TRACE(debugMode,("  Header Checksum = 0x%"X16_F,
		(u16_t)smsc_ntohs(IPV4_HEADER_GET_HEADER_CHECKSUM(headerPointer))));
#ifndef M_WUFEI_080825
	SMSC_TRACE(debugMode,("  Source Address = %s",
		IPV4_ADDRESS_TO_STRING(ipAddressString,
			IPV4_HEADER_GET_SOURCE_ADDRESS(headerPointer))));
	SMSC_TRACE(debugMode,("  Destination Address = %s",
		IPV4_ADDRESS_TO_STRING(ipAddressString,
			IPV4_HEADER_GET_DESTINATION_ADDRESS(headerPointer))));
#else
{
	IPV4_ADDRESS addr_buf = 0;
	IPV4_HEADER_GET_SOURCE_ADDRESS(headerPointer,addr_buf);
	SMSC_TRACE(debugMode,("  Source Address = %s",IPV4_ADDRESS_TO_STRING(ipAddressString,addr_buf)));
	IPV4_HEADER_GET_DESTINATION_ADDRESS(headerPointer,addr_buf);
	SMSC_TRACE(debugMode,("  Destination Address = %s",	IPV4_ADDRESS_TO_STRING(ipAddressString,addr_buf)));
}
#endif
}
#else
#define IPV4_HEADER_PRINT(debugMode,headerPointer)
#endif

u8_t IPV4_ADDRESS_IS_BROADCAST(IPV4_ADDRESS ipAddress,struct NETWORK_INTERFACE * networkInterface)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(networkInterface!=NULL,ERR_ARG);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	
	if ((ipAddress == 0xFFFFFFFF) ||
			(ipAddress == 0x00000000))
	{
		/* all ones (broadcast) or all zeroes (old skool broadcast) */
		return 1;
	}
	else if ((networkInterface->mFlags & NETWORK_INTERFACE_FLAG_BROADCAST) == 0)
	{
		/* no broadcast support on this network interface */
		/* the given address cannot be a broadcast address
		 * nor can we check against any broadcast addresses */
		return 0;
	}
	else if (ipAddress == networkInterface->mIpv4Data.mAddress)
	{
		/* address matches network interface address exactly => no broadcast */
		return 0;
	}
	else if (IPV4_ADDRESS_NETWORK_COMPARE(ipAddress, networkInterface->mIpv4Data.mAddress, 
				networkInterface->mIpv4Data.mNetMask)
					&& ((ipAddress & (~(networkInterface->mIpv4Data.mNetMask))) ==
					(~(networkInterface->mIpv4Data.mNetMask)) ))
	{
		/*  on the same (sub) network... */
		/* ...and host identifier bits are all ones? =>... */
		/* => network broadcast address */
		return 1;
	}
	return 0;
}

/* Ipv4_CheckSum:
 *
 * Calculates the Internet checksum over a portion of memory. Used primarily for IP
 * and ICMP.
 */

u16_t Ipv4_CheckSum(void *dataptr, u16_t len)
{
	u32_t accumulator;

	accumulator = SMSC_IPCHECKSUM(dataptr, len);
	while (accumulator >> 16) {
		accumulator = (accumulator & 0xffff) + (accumulator >> 16);
	}
	return (u16_t)~(accumulator & 0xffff);
}

u16_t Ipv4_ChainedCheckSum(PPACKET_BUFFER packetBufferList)
{          
	u32_t accumulator;
	PPACKET_BUFFER packetBuffer;
	u8_t swapped;

	accumulator = 0;
	swapped = 0;
	for(packetBuffer = packetBufferList; packetBuffer != NULL; packetBuffer = packetBuffer->mNext) {
		accumulator += SMSC_IPCHECKSUM(PacketBuffer_GetStartPoint(packetBuffer), packetBuffer->mThisLength);
		while (accumulator >> 16) {
			accumulator = (accumulator & 0xffffUL) + (accumulator >> 16);
		}
		if (packetBuffer->mThisLength % 2 != 0) {
			swapped = 1 - swapped;
			accumulator = (accumulator & 0x00ffUL << 8) | (accumulator & 0xff00UL >> 8);
		}
	}

	if (swapped) {
		accumulator = ((accumulator & 0x00ffUL) << 8) | ((accumulator & 0xff00UL) >> 8);
	}
	return (u16_t)~(accumulator & 0xffffUL);
}

/* Ipv4_PseudoCheckSum:
 *
 * Calculates the pseudo Internet checksum used by TCP and UDP for a PacketBuffer chain.
 */
u16_t Ipv4_PseudoCheckSum(PPACKET_BUFFER packet,
		PIPV4_ADDRESS sourceAddress, 
		PIPV4_ADDRESS destinationAddress,
		u8_t protocol, u16_t length)
{
	u32_t accumulator;
	PPACKET_BUFFER buffer;
	u8_t swapped;

	accumulator = 0;
	swapped = 0;
	/* iterate through all pbuf in chain */
	for(buffer = packet; buffer != NULL; buffer = buffer->mNext) {
		/*SMSC_TRACE(IPV4_DEBUG, ("Ipv4_PseudoCheckSum: checksumming PacketBuffer %p (has next %p)",
			(void *)buffer, (void *)buffer->mNext));*/
		accumulator += SMSC_IPCHECKSUM(PacketBuffer_GetStartPoint(buffer), buffer->mThisLength);
		/*SMSC_TRACE(IPV4_DEBUG, ("Ipv4_PseudoCheckSum: unwrapped SMSC_IPCHECKSUM()=%"X32_F, accumulator));*/
		while (accumulator >> 16) {
			accumulator = (accumulator & 0xffffUL) + (accumulator >> 16);
		}
		if (buffer->mThisLength % 2 != 0) {
			swapped = 1 - swapped;
			accumulator = ((accumulator & 0xff) << 8) | ((accumulator & 0xff00UL) >> 8);
		}
		/*SMSC_TRACE(IPV4_DEBUG, ("Ipv4_PseudoCheckSum: wrapped SMSC_IPCHECKSUM()=%"X32_F, accumulator));*/
	}

	if (swapped) {
		accumulator = ((accumulator & 0xff) << 8) | ((accumulator & 0xff00UL) >> 8);
	}
	accumulator += ((*sourceAddress) & 0xffffUL);
	accumulator += (((*sourceAddress) >> 16) & 0xffffUL);
	accumulator += ((*destinationAddress) & 0xffffUL);
	accumulator += (((*destinationAddress) >> 16) & 0xffffUL);
	accumulator += (u32_t)smsc_htons((u16_t)protocol);
	accumulator += (u32_t)smsc_htons(length);

	while (accumulator >> 16) {
		accumulator = (accumulator & 0xffffUL) + (accumulator >> 16);
	}
	/*SMSC_TRACE(IPV4_DEBUG, ("Ipv4_PseudoCheckSum: PacketBuffer chain SMSC_IPCHECKSUM()=%"X32_F, accumulator));*/
	return (u16_t)~(accumulator & 0xffffUL);
}

void Ipv4_Input(
	struct NETWORK_INTERFACE * arrivalInterface,
	PPACKET_BUFFER packet)
{
	struct NETWORK_INTERFACE * destinationInterface=NULL;
	u8_t * ipv4Header;
	u16_t ipHeaderLength;
	u16_t ipPacketLength;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(arrivalInterface!=NULL);
	CHECK_SIGNATURE(arrivalInterface,NETWORK_INTERFACE_SIGNATURE);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packet!=NULL);
	CHECK_SIGNATURE(packet,PACKET_BUFFER_SIGNATURE);
	SMSC_ASSERT(packet->mNext==NULL);
	SMSC_ASSERT(packet->mThisLength==packet->mTotalLength);
	SMSC_ASSERT(packet->mThisLength>=20);
	ipv4Header=PacketBuffer_GetStartPoint(packet);
	
	if(IPV4_HEADER_GET_VERSION(ipv4Header)!=4)
	{
		SMSC_NOTICE(IPV4_DEBUG,("IPv4 packet dropped due to bad version, version = %"U16_F,
			(u16_t)(IPV4_HEADER_GET_VERSION(ipv4Header))));
		goto DROP_PACKET;
	}
	
	/* Get the IP packet length */
	ipPacketLength = IPV4_HEADER_GET_TOTAL_LENGTH(ipv4Header);
	/* obtain IP header length in number of 32-it words */
	ipHeaderLength=IPV4_HEADER_GET_IP_HEADER_LENGTH(ipv4Header);

	/* calculate IP header length in bytes */
	ipHeaderLength<<=2;
	
	if(ipHeaderLength>packet->mThisLength) {
		SMSC_NOTICE(IPV4_DEBUG, ("IP header (len %"U16_F") does not fit in first buffer (len %"U16_F"), IP packet droppped.",
			ipHeaderLength, packet->mThisLength));
		goto DROP_PACKET;
	}
	
	if(Ipv4_CheckSum(ipv4Header,ipHeaderLength)!=0) {
		SMSC_NOTICE(IPV4_DEBUG, ("Checksum (0x%"X16_F") failed, IP packet dropped.", 
			Ipv4_CheckSum(ipv4Header, ipHeaderLength)));
		goto DROP_PACKET;
	}
	
	for(destinationInterface=gInterfaceList;
		destinationInterface!=NULL;
		destinationInterface=destinationInterface->mNext)
	{
		if(NetworkInterface_IsUp(destinationInterface)&&
			(!IPV4_ADDRESS_IS_ANY(&(destinationInterface->mIpv4Data.mAddress))))
		{
			/*Interface is up and configured */
#ifndef M_WUFEI_080825
			if(IPV4_ADDRESS_COMPARE(
				IPV4_HEADER_GET_DESTINATION_ADDRESS(ipv4Header),
				destinationInterface->mIpv4Data.mAddress) ||
			   IPV4_ADDRESS_IS_BROADCAST(
				IPV4_HEADER_GET_DESTINATION_ADDRESS(ipv4Header),
				destinationInterface))
			{
				/* packet is destined for this interface */
				SMSC_TRACE(IPV4_DEBUG,("Ipv4_Input: packet accepted on interface, \"%s\"",
					destinationInterface->mName));
				break;
			}
#else
{
			IPV4_ADDRESS addr_buf = 0;
			IPV4_HEADER_GET_DESTINATION_ADDRESS(ipv4Header,addr_buf);
			if(IPV4_ADDRESS_COMPARE(addr_buf,
				destinationInterface->mIpv4Data.mAddress) ||
			   IPV4_ADDRESS_IS_BROADCAST(addr_buf,destinationInterface))

			{
				/* packet is destined for this interface */
				SMSC_TRACE(IPV4_DEBUG,("Ipv4_Input: packet accepted on interface, \"%s\"",
					destinationInterface->mName));
				break;
			}
}
#endif		
		}			
	}
#if DHCP_ENABLED
#ifndef M_WUFEI_080825
#define UDP_HEADER_GET_DESTINATION_PORT(udpHeader)	\
	smsc_ntohs(((u16_t *)(udpHeader))[1])
#else
#define UDP_HEADER_GET_DESTINATION_PORT(udpHeader)	\
	smsc_ntohs((u16_t )(udpHeader[2])+(((u16_t )udpHeader[3])<<8))
#endif
	/* Pass DHCP messages regardless of destination address. DHCP traffic is addressed
	 * using link layer addressing (such as Ethernet MAC) so we must not filter on IP.
	 * According to RFC 1542 section 3.1.1, referred by RFC 2131).
	 */
	if (destinationInterface == NULL) {
		/* remote port is DHCP server? */
		if (IPV4_HEADER_GET_PROTOCOL(ipv4Header) == IP_PROTOCOL_UDP) {
			u8_t * udpHeader=ipv4Header+ipHeaderLength;
			SMSC_TRACE(IPV4_DEBUG, ("Ipv4_Input: UDP packet to DHCP client port %"U16_F,
				UDP_HEADER_GET_DESTINATION_PORT(udpHeader)));
			if (UDP_HEADER_GET_DESTINATION_PORT(udpHeader) == DHCP_CLIENT_PORT) {
				SMSC_TRACE(IPV4_DEBUG, ("Ipv4_Input: DHCP packet accepted."));
				destinationInterface = arrivalInterface;
			}
		}
	}
#endif /* DHCP_ENABLED */
  
	if(destinationInterface==NULL) {
		/* packet is not for us, route or discard */
		SMSC_NOTICE(IPV4_DEBUG, ("Ipv4_Input: packet not for us."));
#if IPV4_FORWARDING_ENABLED
		if(!IPV4_ADDRESS_IS_BROADCAST(
			IPV4_HEADER_GET_DESTINATION_ADDRESS(ipv4Header),
			arrivalInterface))
		{
			/* TODO call/implement Ipv4_Forward();*/
			SMSC_ERROR(("Ipv4_Input: Forwarding is not implemented"));
			/* if packet was forwarded then we should return here */
			return;
		} else 
#endif /* IPV4_FORWARDING_ENABLED */
		{
			goto DROP_PACKET;
		}
	}
	
	if((IPV4_HEADER_GET_FRAGMENT_OFFSET(ipv4Header)!=0)||
		(IPV4_HEADER_GET_FLAGS(ipv4Header)&IPV4_FLAG_MORE_FRAGMENTS))
	{
		/* packet consists of multiple fragments */
#if IPV4_FRAGMENTS_ENABLED
		SMSC_ERROR(("Ipv4_Input: Fragments Not Implemented"));
#else	/* IPV4_FRAGMENTS_ENABLED */
		SMSC_NOTICE(IPV4_DEBUG,("Ipv4_Input: Packet Dropped, Fragments not supported"));
		goto DROP_PACKET;
#endif
	}
	
	if(ipHeaderLength>20) {
		SMSC_NOTICE(IPV4_DEBUG,("Ipv4_Input: IP packet dropped since there were IP options"));
		goto DROP_PACKET;
	}
	
	IPV4_HEADER_PRINT(IPV4_DEBUG,ipv4Header);/**/
	
	switch(IPV4_HEADER_GET_PROTOCOL(ipv4Header)) {
#if UDP_ENABLED
	case IP_PROTOCOL_UDP:
	case IP_PROTOCOL_UDPLITE:
		{
			IP_ADDRESS destinationAddress;
			IP_ADDRESS sourceAddress;
			u8_t protocol;
#ifndef M_WUFEI_080825
			IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&destinationAddress,
				&(IPV4_HEADER_GET_DESTINATION_ADDRESS(ipv4Header)));
			IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&sourceAddress,
				&(IPV4_HEADER_GET_SOURCE_ADDRESS(ipv4Header)));
#else
{
		IPV4_ADDRESS addr_buf = 0;
		IPV4_HEADER_GET_DESTINATION_ADDRESS(ipv4Header,addr_buf);	
		IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&destinationAddress,&addr_buf);
		IPV4_HEADER_GET_SOURCE_ADDRESS(ipv4Header,addr_buf);
		IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&sourceAddress,	&addr_buf);
}
#endif
			protocol=IPV4_HEADER_GET_PROTOCOL(ipv4Header);
			if(PacketBuffer_MoveStartPoint(packet,ipHeaderLength)!=ERR_OK) {
				SMSC_ERROR(("Ipv4_Input: unabled to move start point"));
			}
			PacketBuffer_ReduceTotalLength(packet, ((u32_t)(ipPacketLength)) - ((u32_t)(ipHeaderLength)) );
			SMSC_ASSERT(packet->mNext==NULL);/* fragments not supported */
			Udp_Input(destinationInterface,packet,
				&destinationAddress,&sourceAddress,
				protocol);
		}
		break;
#endif
#if TCP_ENABLED
	case IP_PROTOCOL_TCP:
		{
			IP_ADDRESS destinationAddress;
			IP_ADDRESS sourceAddress;
			u8_t protocol;
#ifndef M_WUFEI_080825
			IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&destinationAddress,
				&(IPV4_HEADER_GET_DESTINATION_ADDRESS(ipv4Header)));
			IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&sourceAddress,
				&(IPV4_HEADER_GET_SOURCE_ADDRESS(ipv4Header)));
#else
{
		IPV4_ADDRESS addr_buf = 0;
		IPV4_HEADER_GET_DESTINATION_ADDRESS(ipv4Header,addr_buf);	
		IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&destinationAddress,&addr_buf);
		IPV4_HEADER_GET_SOURCE_ADDRESS(ipv4Header,addr_buf);
		IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&sourceAddress,	&addr_buf);
}
#endif
			protocol=IPV4_HEADER_GET_PROTOCOL(ipv4Header);
			if(PacketBuffer_MoveStartPoint(packet,ipHeaderLength)!=ERR_OK) {
				SMSC_ERROR(("Ipv4_Input: unabled to move start point"));
			}
			PacketBuffer_ReduceTotalLength(packet, ((u32_t)(ipPacketLength)) - ((u32_t)(ipHeaderLength)) );
			SMSC_ASSERT(packet->mNext==NULL);/* fragments not supported */
			Tcp_Input(destinationInterface,packet,
				&destinationAddress,&sourceAddress,
				protocol);
		}
		break;
#endif
	case IP_PROTOCOL_ICMP:
		Icmpv4_Input(destinationInterface,packet);
		break;
	default: 
		/* send ICMP destination protocol unreachable unless is was a broadcast */
		/*if (!ip_addr_isbroadcast(&(iphdr->dest), inp) &&
			!ip_addr_ismulticast(&(iphdr->dest))) 
		{
			p->payload = iphdr;
			icmp_dest_unreach(p, ICMP_DUR_PROTO);
		}
		pbuf_free(p);*/

		SMSC_NOTICE(IPV4_DEBUG, 
			("Unsupported transport protocol %"U16_F"\n", 
			IPV4_HEADER_GET_PROTOCOL(ipv4Header)));
		goto DROP_PACKET;
	}
	return;
DROP_PACKET:
	PacketBuffer_DecreaseReference(packet);
}

void Ipv4_OutputWithHeaderIncluded(
	struct NETWORK_INTERFACE * networkInterface,
	PPACKET_BUFFER packet,
	IPV4_ADDRESS destinationAddress)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packet!=NULL);
	CHECK_SIGNATURE(packet,PACKET_BUFFER_SIGNATURE);

	if((networkInterface->mMTU)&&
		(packet->mTotalLength>networkInterface->mMTU))
	{
		/* Need to fragment */
#if IPV4_FRAGMENTS_ENABLED
		SMSC_ERROR(("Ipv4_OutputWithHeaderIncluded: Fragments not implemented"));
#else
		SMSC_NOTICE(IPV4_DEBUG,("Ipv4_OutputWithHeaderIncluded: Need to fragment, but fragmenting is not enabled, dropping packet"));
		SMSC_NOTICE(IPV4_DEBUG,("                               packet length = %"U16_F", MTU = %"U16_F,
			packet->mTotalLength,networkInterface->mMTU));
		PacketBuffer_DecreaseReference(packet);
		return;
#endif
	}
	
	((IPV4_OUTPUT_FUNCTION)(networkInterface->mIpv4Data.mOutputFunction))(
		networkInterface,packet,destinationAddress);
}

static u16_t gIpv4NextIdentification=0;

void Ipv4_Output(
	PPACKET_BUFFER packet,
	IPV4_ADDRESS sourceAddress,	IPV4_ADDRESS destinationAddress,
	u8_t protocol)
{
	u8_t * ipv4Header;
	struct NETWORK_INTERFACE * networkInterface;
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packet!=NULL);
	SMSC_ASSERT(packet->mNext==NULL);/* fragments not supported yet */
	SMSC_ASSERT(!IPV4_ADDRESS_IS_ANY(&(destinationAddress)));
	
	networkInterface=Ipv4_GetRoute(destinationAddress);
	if(networkInterface==NULL) {
		SMSC_NOTICE(IPV4_DEBUG,("Ipv4_Output: No Route"));
		goto DROP_PACKET;
	}
	
	if(PacketBuffer_MoveStartPoint(packet,-IPV4_HEADER_LENGTH)!=ERR_OK) 
	{
		SMSC_ERROR(("Ipv4_Output: Unable to move start point"));
		goto DROP_PACKET;
	}
	
	ipv4Header=PacketBuffer_GetStartPoint(packet);
	IPV4_HEADER_SET_VERSION_AND_IP_HEADER_LENGTH(ipv4Header,4,IPV4_HEADER_LENGTH/4);
	IPV4_HEADER_SET_TYPE_OF_SERVICE(ipv4Header,0);
	IPV4_HEADER_SET_TOTAL_LENGTH(ipv4Header,packet->mThisLength);
	
	/* TODO: when fragmenting make sure the same identification goes into each packet */
	IPV4_HEADER_SET_IDENTIFICATION(ipv4Header,gIpv4NextIdentification);
	
	/* TODO: when fragmenting update fragment correctly */
	IPV4_HEADER_SET_FLAGS_AND_FRAGMENT_OFFSET(ipv4Header,0,0);
	
	IPV4_HEADER_SET_TIME_TO_LIVE(ipv4Header,255);
	IPV4_HEADER_SET_PROTOCOL(ipv4Header,protocol);
	
	if(IPV4_ADDRESS_IS_ANY(&(sourceAddress))) {
		/* NOTE: can not grab the source address from the destination interface
		  because the destination interface may be a loop back interface.
		  Getting the source address this way will ignore the loop back interface */
		sourceAddress=Ipv4_GetSourceAddress(destinationAddress);
	}
	
	/*In the case of DHCP we do not know our source address yet so 
	  it is ok to use the any address
	if(IPV4_ADDRESS_IS_ANY(&(sourceAddress))) {
		SMSC_NOTICE(IPV4_DEBUG,("Ipv4_Output: Source Address is not assigned"));
		goto DROP_PACKET;
	}*/
	
	IPV4_HEADER_SET_SOURCE_ADDRESS(ipv4Header,sourceAddress);
	IPV4_HEADER_SET_DESTINATION_ADDRESS(ipv4Header,destinationAddress);
	
	IPV4_HEADER_SET_HEADER_CHECKSUM(ipv4Header,0);
	IPV4_HEADER_SET_HEADER_CHECKSUM(ipv4Header,
		Ipv4_CheckSum(ipv4Header,IPV4_HEADER_LENGTH));
		
	((IPV4_OUTPUT_FUNCTION)(networkInterface->mIpv4Data.mOutputFunction))(
		networkInterface,packet,destinationAddress);

	gIpv4NextIdentification++;	
	return;
DROP_PACKET:
	PacketBuffer_DecreaseReference(packet);
}
