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
File: ethernet.c
*/

#include "smsc_environment.h"
#include "network_stack.h"
#include "task_manager.h"
#include "smsc_threading.h"
#if IPV4_ENABLED
#include "ipv4.h"
#endif
#include "ethernet.h"

#if SMSC_ERROR_ENABLED
#define ETHERNET_DATA_SIGNATURE	(0x85BCBC63)
#endif

/* TODO need structure packing */
/*typedef struct ETHERNET_HEADER_ {
	ETHERNET_ADDRESS mDestination;
	ETHERNET_ADDRESS mSource;
	u16_t mType;
} ETHERNET_HEADER, * PETHERNET_HEADER;*/

#define ETHERNET_HEADER_GET_DESTINATION_ADDRESS(ethernetHeader)	\
	((u8_t *)(ethernetHeader))
#define ETHERNET_HEADER_GET_SOURCE_ADDRESS(ethernetHeader)	\
	(&(((u8_t *)(ethernetHeader))[6]))
#define ETHERNET_HEADER_GET_TYPE(ethernetHeader)	\
	smsc_ntohs(((u16_t *)(ethernetHeader))[6])

#define ETHERNET_HEADER_SET_DESTINATION_ADDRESS(ethernetHeader,macAddress)	\
	memcpy(ethernetHeader,macAddress,6)
#define ETHERNET_HEADER_SET_SOURCE_ADDRESS(ethernetHeader,macAddress)	\
	memcpy((&(((u8_t *)(ethernetHeader))[6])),macAddress,6)
#define ETHERNET_HEADER_SET_TYPE(ethernetHeader,type)	\
	((u16_t *)(ethernetHeader))[6]=smsc_htons(type)
	


static const u8_t gEthernetBroadcastAddress[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

static s8_t EthernetArp_FindEntry(
	u16_t type,			/* must be in host byte order */
	void * ipAddress,	/* must be in network byte order */
	int forceFlag);

#if SMSC_TRACE_ENABLED
static char stringOfEthernetMacAddress[20];
static char * ToString_ETHERNET_ADDRESS(u8_t address[6])
{
	sprintf(stringOfEthernetMacAddress,
		"%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F,
		address[0],address[1],address[2],address[3],address[4],address[5]);
	return stringOfEthernetMacAddress;
}
static void PRINT_ETHERNET_HEADER(int debugMode,u8_t * ethernetHeader)
{
	SMSC_TRACE(debugMode,("Destination = %s",ToString_ETHERNET_ADDRESS(ETHERNET_HEADER_GET_DESTINATION_ADDRESS(ethernetHeader))));
	SMSC_TRACE(debugMode,("Source = %s",ToString_ETHERNET_ADDRESS(ETHERNET_HEADER_GET_SOURCE_ADDRESS(ethernetHeader))));
	SMSC_TRACE(debugMode,("Type = 0x%04"X16_F,ETHERNET_HEADER_GET_TYPE(ethernetHeader)));
}
#else 
#define PRINT_ETHERNET_HEADER(debugMode,header)	{}
#endif

#define ARP_STATE_EMPTY		(0x00)
#define ARP_STATE_PENDING	(0x01)
#define ARP_STATE_STABLE	(0x02)

#if SMSC_ERROR_ENABLED
#define ETHERNET_ARP_ENTRY_SIGNATURE	(0x84EC6235)
#endif

typedef struct ETHERNET_ARP_ENTRY_ {
	DECLARE_SIGNATURE
	PACKET_QUEUE mPacketQueue;
	union {
#if IPV4_ENABLED
		IPV4_ADDRESS mIpv4Address;/* must be stored in network byte order */
#endif
#if IPV6_ENABLED
		IPV6_ADDRESS mIpv6Address;/* must be stored in network byte order */
#endif
	} mIpAddress;
	u8_t mEthernetAddress[6];
	struct NETWORK_INTERFACE * mNetworkInterface;
	u16_t mType; /* Set by ETHERNET_TYPE_... */
	u8_t mState;/* Set by ARP_STATE_... */
	
	/* mEntryAge: This cleared on every access, and 
	  incremented on every call to EthernetArp_Timer 
	  which is called every 5 seconds. It allows us to judge
	  how long until an arp entry should expire. */
	u8_t mEntryAge;
} ETHERNET_ARP_ENTRY, *PETHERNET_ARP_ENTRY;

static ETHERNET_ARP_ENTRY gArpTable[ARP_TABLE_SIZE];

static TASK gArpTimerTask;

#define ETHERNET_TYPE_UNDEFINED	(0x0000)
#define ETHERNET_TYPE_ARP		(0x0806)
#define ETHERNET_TYPE_IPV4		(0x0800)

/* TODO need structure packing */
#if 0
typedef struct ETHERNET_ARP_HEADER_ {
	ETHERNET_HEADER mEthernetHeader;
	u16_t mHardwareType;/* to designate ethernet */
	u16_t mProtocol;    /* to designate IPv4 or IPv6 */
	u8_t mHardwareAddressLength;
	u8_t mProtocolAddressLength;
	u16_t mOpCode;		/* set to ARP_OPCODE_... */
} ETHERNET_ARP_HEADER, * PETHERNET_ARP_HEADER;
#endif

#define ETHERNET_ARP_HEADER_GET_HARDWARE_TYPE(ethernetHeader)	\
	smsc_ntohs(((u16_t *)(ethernetHeader))[7])
#define ETHERNET_ARP_HEADER_GET_PROTOCOL(ethernetHeader)	\
	smsc_ntohs(((u16_t *)(ethernetHeader))[8])
#define ETHERNET_ARP_HEADER_GET_HARDWARE_ADDRESS_LENGTH(ethernetHeader)	\
	(((u8_t *)(ethernetHeader))[18])
#define ETHERNET_ARP_HEADER_GET_PROTOCOL_ADDRESS_LENGTH(ethernetHeader)	\
	(((u8_t *)(ethernetHeader))[19])
#define ETHERNET_ARP_HEADER_GET_OP_CODE(ethernetHeader)	\
	smsc_ntohs(((u16_t *)(ethernetHeader))[10])
	
#define ETHERNET_ARP_HEADER_SET_HARDWARE_TYPE(ethernetHeader,hardwareType)	\
	(((u16_t *)(ethernetHeader))[7])=smsc_htons(hardwareType)
#define ETHERNET_ARP_HEADER_SET_PROTOCOL(ethernetHeader,protocol)	\
	(((u16_t *)(ethernetHeader))[8])=smsc_htons(protocol)
#define ETHERNET_ARP_HEADER_SET_HARDWARE_ADDRESS_LENGTH(ethernetHeader,hardwareAddressLength)	\
	(((u8_t *)(ethernetHeader))[18])=(u8_t)(hardwareAddressLength)
#define ETHERNET_ARP_HEADER_SET_PROTOCOL_ADDRESS_LENGTH(ethernetHeader,protocolAddressLength)	\
	(((u8_t *)(ethernetHeader))[19])=(u8_t)(protocolAddressLength)
#define ETHERNET_ARP_HEADER_SET_OP_CODE(ethernetHeader,opCode)	\
	(((u16_t *)(ethernetHeader))[10])=smsc_htons(opCode)

#define ARP_OPCODE_REQUEST	(1)
#define ARP_OPCODE_REPLY	(2)
#define HARDWARETYPE_ETHERNET	(1)

#if SMSC_TRACE_ENABLED
static void PRINT_ETHERNET_ARP_HEADER(int debugMode,u8_t * ethernetArpHeader)
{
	PRINT_ETHERNET_HEADER(debugMode,ethernetArpHeader);
	SMSC_TRACE(debugMode,("HardwareType = 0x%04"X16_F,ETHERNET_ARP_HEADER_GET_HARDWARE_TYPE(ethernetArpHeader)));
	SMSC_TRACE(debugMode,("Protocol = 0x%04"X16_F,ETHERNET_ARP_HEADER_GET_PROTOCOL(ethernetArpHeader)));
	SMSC_TRACE(debugMode,("HardwareAddressLength = %"U16_F,ETHERNET_ARP_HEADER_GET_HARDWARE_ADDRESS_LENGTH(ethernetArpHeader)));
	SMSC_TRACE(debugMode,("ProtocolAddressLength = %"U16_F,ETHERNET_ARP_HEADER_GET_PROTOCOL_ADDRESS_LENGTH(ethernetArpHeader)));
	SMSC_TRACE(debugMode,("OpCode = 0x%04"X16_F,ETHERNET_ARP_HEADER_GET_OP_CODE(ethernetArpHeader)));
}
#else
#define PRINT_ETHERNET_ARP_HEADER(debugMode,header)	{}
#endif

#if IPV4_ENABLED

#include "ipv4.h"

#if 0
/* IPV4_ADDRESS2 is used by ARP to support compilers that don't have
   structure packing */
/* TODO need structure packing */
typedef struct IPV4_ADDRESS2_ {
	u16_t mHigh16;
	u16_t mLow16;
} IPV4_ADDRESS2, * PIPV4_ADDRESS2;

#define GET_HIGH16(num32) ((u16_t)((num32)>>16))
#define GET_LOW16(num32)  ((u16_t)(num32))

/* TODO need structure packing */
typedef struct ETHERNET_IPV4_ARP_HEADER_ {
	ETHERNET_ARP_HEADER mArpHeader;
	ETHERNET_ADDRESS mSourceHardwareAddress;/*22*/
	IPV4_ADDRESS2 mSourceProtocolAddress; /*28*/
	ETHERNET_ADDRESS mTargetHardwareAddress;/*32*/
	IPV4_ADDRESS2 mTargetProtocolAddress;	/*38*/
} ETHERNET_IPV4_ARP_HEADER, * PETHERNET_IPV4_ARP_HEADER;
#endif /*0*/

#define ETHERNET_IPV4_ARP_HEADER_GET_SOURCE_HARDWARE_ADDRESS(ethernetHeader)	\
	(&(((u8_t *)(ethernetHeader))[22]))
#define ETHERNET_IPV4_ARP_HEADER_GET_SOURCE_PROTOCOL_ADDRESS(ethernetHeader)	\
	(&(((u8_t *)(ethernetHeader))[28]))
#define ETHERNET_IPV4_ARP_HEADER_GET_TARGET_HARDWARE_ADDRESS(ethernetHeader)	\
	(&(((u8_t *)(ethernetHeader))[32]))
#define ETHERNET_IPV4_ARP_HEADER_GET_TARGET_PROTOCOL_ADDRESS(ethernetHeader)	\
	(&(((u8_t *)(ethernetHeader))[38]))
	
#define ETHERNET_IPV4_ARP_HEADER_SET_SOURCE_HARDWARE_ADDRESS(ethernetHeader,macAddress)	\
	memcpy(&(((u8_t *)(ethernetHeader))[22]),macAddress,6)
#define ETHERNET_IPV4_ARP_HEADER_SET_SOURCE_PROTOCOL_ADDRESS(ethernetHeader,ipv4Address)	\
	memcpy(&(((u8_t *)(ethernetHeader))[28]),ipv4Address,4)
#define ETHERNET_IPV4_ARP_HEADER_SET_TARGET_HARDWARE_ADDRESS(ethernetHeader,macAddress)	\
	memcpy(&(((u8_t *)(ethernetHeader))[32]),macAddress,6)
#define ETHERNET_IPV4_ARP_HEADER_SET_TARGET_PROTOCOL_ADDRESS(ethernetHeader,ipv4Address)	\
	memcpy(&(((u8_t *)(ethernetHeader))[38]),ipv4Address,4)
                          
#if SMSC_TRACE_ENABLED
static char stringOfIpv4Address[20];
static char * ToString_IPV4_ADDRESS(u8_t * address)
{
	sprintf(stringOfIpv4Address,
		"%"U16_F".%"U16_F".%"U16_F".%"U16_F,
		address[0],address[1],address[2],address[4]);
	return stringOfIpv4Address;
}
static void PRINT_ETHERNET_IPV4_ARP_HEADER(int debugMode,u8_t * header)
{
	PRINT_ETHERNET_ARP_HEADER(debugMode,header);
	SMSC_TRACE(debugMode,("SourceHardwareAddress = %s",
		ToString_ETHERNET_ADDRESS(ETHERNET_IPV4_ARP_HEADER_GET_SOURCE_HARDWARE_ADDRESS(header))));
	SMSC_TRACE(debugMode,("SourceProtocolAddress = %s",
		ToString_IPV4_ADDRESS(ETHERNET_IPV4_ARP_HEADER_GET_SOURCE_PROTOCOL_ADDRESS(header))));
	SMSC_TRACE(debugMode,("TargetHardwareAddress = %s",
		ToString_ETHERNET_ADDRESS(ETHERNET_IPV4_ARP_HEADER_GET_TARGET_HARDWARE_ADDRESS(header))));
	SMSC_TRACE(debugMode,("TargetProtocolAddress = %s",
		ToString_IPV4_ADDRESS(ETHERNET_IPV4_ARP_HEADER_GET_TARGET_PROTOCOL_ADDRESS(header))));
}
#else /* SMSC_TRACE_ENABLED */
#define PRINT_ETHERNET_IPV4_ARP_HEADER(header)
#endif /* SMSC_TRACE_ENABLED else */

static err_t EthernetArp_Ipv4Request(
	struct NETWORK_INTERFACE * networkInterface,
	PIPV4_ADDRESS ipv4Address)
{
	/*ipv4Address is assumed to be in network byte order */
	PPACKET_BUFFER packet;
	u8_t * sourceMacAddress;
	err_t result=ERR_OK;
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(networkInterface!=NULL,ERR_ARG);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	sourceMacAddress=networkInterface->mLinkData.mEthernetData.mMacAddress;

	/* Allocate a packet for the outgoing ARP request packet */
	packet=PacketBuffer_Create(42+2);
	if (packet != NULL) {
		/* packet was allocated */
		u8_t * header;
		if(PacketBuffer_MoveStartPoint(packet,2)!=ERR_OK) {
			/* This should not happen because we allocated an extra 2 bytes */
			SMSC_ERROR(("EthernetArp_Ipv4Request: could not move start point"));
		}
		header=PacketBuffer_GetStartPoint(packet);
		
		SMSC_TRACE(ETHERNET_DEBUG, ("EthernetArp_Ipv4Request: sending ARP request.\n"));
		ETHERNET_ARP_HEADER_SET_OP_CODE(header,ARP_OPCODE_REQUEST);
		
		ETHERNET_IPV4_ARP_HEADER_SET_SOURCE_HARDWARE_ADDRESS(header,sourceMacAddress);
		
		/*we don't know the target hardware address so we will fill it with zeros */
		memset(ETHERNET_IPV4_ARP_HEADER_GET_TARGET_HARDWARE_ADDRESS(header),0,6);
		
		ETHERNET_IPV4_ARP_HEADER_SET_TARGET_PROTOCOL_ADDRESS(header,ipv4Address);
		if(networkInterface->mFlags & NETWORK_INTERFACE_FLAG_DHCP)
			ETHERNET_IPV4_ARP_HEADER_SET_SOURCE_PROTOCOL_ADDRESS(header,&(networkInterface->mDhcpData->mOfferedAddress));
		else
			ETHERNET_IPV4_ARP_HEADER_SET_SOURCE_PROTOCOL_ADDRESS(header,&(networkInterface->mIpv4Data.mAddress));/*wufei modif this 2008-12-9*/

		ETHERNET_ARP_HEADER_SET_HARDWARE_TYPE(header,HARDWARETYPE_ETHERNET);
		ETHERNET_ARP_HEADER_SET_HARDWARE_ADDRESS_LENGTH(header,6);
		ETHERNET_ARP_HEADER_SET_PROTOCOL(header,ETHERNET_TYPE_IPV4);
		ETHERNET_ARP_HEADER_SET_PROTOCOL_ADDRESS_LENGTH(header,4);

		memset(ETHERNET_HEADER_GET_DESTINATION_ADDRESS(header),0xFFFF,6);
		
		ETHERNET_HEADER_SET_SOURCE_ADDRESS(header,sourceMacAddress);
		
		ETHERNET_HEADER_SET_TYPE(header,ETHERNET_TYPE_ARP);
		
		/* send ARP query */
		((ETHERNET_OUTPUT_FUNCTION)(networkInterface->mLinkData.mEthernetData.mOutputFunction))(
			networkInterface,packet);
	} else {
		/* packet was not allocated */
		result = ERR_MEM;
		SMSC_NOTICE(ETHERNET_DEBUG, ("EthernetArp_Ipv4Request: could not allocate packet for ARP request."));
	}
	return result;
}

void Ethernet_Ipv4Output(struct NETWORK_INTERFACE * networkInterface,PPACKET_BUFFER packet,
	IPV4_ADDRESS destinationIpAddress)
{
	u8_t * ethernetHeader;
	const u8_t * destinationEthernetAddress;
	u8_t * sourceEthernetAddress;
	u8_t multicastEthernetAddress[6];
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	if(packet==NULL)
		goto DO_QUERY_ONLY;

	CHECK_SIGNATURE(packet,PACKET_BUFFER_SIGNATURE);
	SMSC_ASSERT(packet->mNext==NULL);
	SMSC_ASSERT(packet->mThisLength==packet->mTotalLength);
	SMSC_ASSERT(packet->mThisLength<=networkInterface->mMTU);
	
	if(PacketBuffer_MoveStartPoint(packet,-14)!=ERR_OK) 
	{
		/*This should not happen because all packets should be preallocated with 
		  enough space for the ethernet header */
		SMSC_ERROR(("Ethernet_Ipv4Output: unable to move start point"));
	}
	
	if(IPV4_ADDRESS_IS_BROADCAST(destinationIpAddress,networkInterface)) {
		/* This is a broadcast packet, so we must broadcast on ethernet link */
		destinationEthernetAddress=gEthernetBroadcastAddress;
	} else if(IPV4_ADDRESS_IS_MULTICAST(destinationIpAddress)) {
		/* This is a multicast packet, so we must construct a multicast ethernet address */
		multicastEthernetAddress[0]=0x01;
		multicastEthernetAddress[1]=0x00;
		multicastEthernetAddress[2]=0x5E;
		multicastEthernetAddress[3]=IPV4_ADDRESS_GET_BYTE_2(destinationIpAddress)&0x7F;
		multicastEthernetAddress[4]=IPV4_ADDRESS_GET_BYTE_3(destinationIpAddress);
		multicastEthernetAddress[5]=IPV4_ADDRESS_GET_BYTE_4(destinationIpAddress);
		destinationEthernetAddress=multicastEthernetAddress;
	} else {
		/* This is a unicast packet */
		if(!IPV4_ADDRESS_NETWORK_COMPARE(destinationIpAddress,
			networkInterface->mIpv4Data.mAddress,
			networkInterface->mIpv4Data.mNetMask))
		{
			/* This packet is going to a different network */
			/* Send it to the default gateway */
			destinationIpAddress=networkInterface->mIpv4Data.mGateway;
		}
		if(!IPV4_ADDRESS_IS_ANY(&destinationIpAddress))
		{
			s8_t arpEntryIndex;
DO_QUERY_ONLY:
			arpEntryIndex=EthernetArp_FindEntry(ETHERNET_TYPE_IPV4,&destinationIpAddress,1);
			    
			/* could not find or create entry? */
			if (arpEntryIndex < 0)
			{
				SMSC_NOTICE(ETHERNET_DEBUG, ("Ethernet_Ipv4Output: could not create ARP entry, packet dropped"));
				goto DROP_PACKET;
			}

			if(gArpTable[arpEntryIndex].mState==ARP_STATE_EMPTY) {
				gArpTable[arpEntryIndex].mNetworkInterface=networkInterface;
				gArpTable[arpEntryIndex].mState=ARP_STATE_PENDING;
			}

			if((gArpTable[arpEntryIndex].mState==ARP_STATE_PENDING)||(packet==NULL)) {
				/* Issue a new Arp request */
				EthernetArp_Ipv4Request(networkInterface,&destinationIpAddress);
				if(packet!=NULL) {
					/* Put packet on queue for transmission later */
					PacketQueue_Push(&(gArpTable[arpEntryIndex].mPacketQueue),packet);
				}
			} else {
				/* must be stable entry */
				SMSC_ASSERT(gArpTable[arpEntryIndex].mState==ARP_STATE_STABLE);
				/* we have a valid IP->Ethernet address mapping,
				* fill in the Ethernet header for the outgoing packet */
				{
					u8_t * ethernetHeader=PacketBuffer_GetStartPoint(packet);
					ETHERNET_HEADER_SET_DESTINATION_ADDRESS(ethernetHeader,
						gArpTable[arpEntryIndex].mEthernetAddress);
					ETHERNET_HEADER_SET_SOURCE_ADDRESS(ethernetHeader,
						networkInterface->mLinkData.mEthernetData.mMacAddress);
					ETHERNET_HEADER_SET_TYPE(ethernetHeader,ETHERNET_TYPE_IPV4);
					
					((ETHERNET_OUTPUT_FUNCTION)(networkInterface->mLinkData.mEthernetData.mOutputFunction))(
						networkInterface,packet);
				}
			}
			return;
		} else {
			SMSC_NOTICE(ETHERNET_DEBUG,("Ethernet_Ipv4Output: No Route to Destination"));
			goto DROP_PACKET;
		}
	}
	/* continuation for multicast/broadcast destinations */
	/* obtain source Ethernet address of the given interface */
	sourceEthernetAddress=networkInterface->mLinkData.mEthernetData.mMacAddress;
	ethernetHeader=PacketBuffer_GetStartPoint(packet);
	
	ETHERNET_HEADER_SET_DESTINATION_ADDRESS(ethernetHeader,destinationEthernetAddress);
	ETHERNET_HEADER_SET_SOURCE_ADDRESS(ethernetHeader,sourceEthernetAddress);
	ETHERNET_HEADER_SET_TYPE(ethernetHeader,ETHERNET_TYPE_IPV4);
	
	((ETHERNET_OUTPUT_FUNCTION)(networkInterface->mLinkData.mEthernetData.mOutputFunction))(
		networkInterface,packet);
	return;
DROP_PACKET:
	if(packet!=NULL) {
		PacketBuffer_DecreaseReference(packet);
	}
}
void Ethernet_Ipv4Query(struct NETWORK_INTERFACE * networkInterface,
	IPV4_ADDRESS destinationIpAddress)
{
	Ethernet_Ipv4Output(networkInterface,NULL,destinationIpAddress);
}

#endif /* IPV4_ENABLED */

#if IPV6_ENABLED
#error IPv6 is not supported
#include "new_ipv6.h"

/* TODO need structure packing */
typedef struct ETHERNET_IPV6_ARP_HEADER_ {
	ETHERNET_ARP_HEADER mArpHeader;
	ETHERNET_ADDRESS mSourceHardwareAddress;
	IPV6_ADDRESS mSourceProtocolAddress;
	ETHERNET_ADDRESS mTargetHardwareAddress;
	IPV6_ADDRESS mTargetProtocolAddress;
} ETHERNET_IPV6_ARP_HEADER, * PETHERNET_IPV6_ARP_HEADER;


static err_t EthernetArp_Ipv6Request(
	PNETWORK_INTERFACE networkInterface,
	PIPV6_ADDRESS ipv6Address)
{
	SMSC_ERROR(("EthernetArp_RequestIpv6: NOT IMPLEMENTED"));
}

#endif /* IPV6_ENABLED */

static void EthernetArp_Timer(void * param)
{
	u8_t index;
	PETHERNET_ARP_ENTRY arpEntry;
	
	SMSC_TRACE(ETHERNET_DEBUG,("EthernetArp_Timer"));
	
	for(index=0;index<ARP_TABLE_SIZE;index++) {
		arpEntry=&(gArpTable[index]);
		arpEntry->mEntryAge++;
		if(arpEntry->mState==ARP_STATE_STABLE) {
			if(arpEntry->mEntryAge>=ARP_ENTRY_MAX_AGE) {
				SMSC_TRACE(ETHERNET_DEBUG,("EthernetArp_Timer: expired stable entry %"U16_F,(u16_t)index));
				arpEntry->mState=ARP_STATE_EMPTY;
				
				/* ARP queue must be empty */
				SMSC_ASSERT(arpEntry->mPacketQueue.mHead==NULL);
			}
		} else if (arpEntry->mState==ARP_STATE_PENDING) {
			if(arpEntry->mEntryAge>=ARP_ENTRY_MAX_PENDING_AGE) {
				SMSC_TRACE(ETHERNET_DEBUG,("EthernetArp_Timer: expired pending entry %"U16_F,(u16_t)index));
				{ /* Empty the pending queue */
					PPACKET_BUFFER packet;
					while((packet=PacketQueue_Pop(&(arpEntry->mPacketQueue)))!=NULL) {
						PacketBuffer_DecreaseReference(packet);
					}
				}
				arpEntry->mState=ARP_STATE_EMPTY;
			} else if(!PacketQueue_IsEmpty(&(arpEntry->mPacketQueue))) {
#if IPV4_ENABLED
				if(arpEntry->mType==ETHERNET_TYPE_IPV4) {
					EthernetArp_Ipv4Request(
						arpEntry->mNetworkInterface,
						&arpEntry->mIpAddress.mIpv4Address);
				} else
#endif
#if IPV6_ENABLED
				if(arpEntry->mType==ETHERNET_TYPE_IPV6) {
					SMSC_ERROR(("EthernetArp_Timer: Can't resent Arp Query, IPV6 is not implemented"));
				} else 
#endif
				{
					SMSC_WARNING(ETHERNET_DEBUG,("EthernetArp_Timer: Can't resend Arp Query, unknown type=%"U16_F,
						arpEntry->mType));			
				}
			}
		}
	}
	
	/* Call again in 5 seconds */
	TaskManager_ScheduleByTimer(&gArpTimerTask,5000);
}

static int EthernetArp_IpAddressCompare(u16_t type, void * ipAddress1,void * ipAddress2)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ipAddress1!=NULL,ERR_ARG);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(ipAddress2!=NULL,ERR_ARG);
#if IPV4_ENABLED
	if(type==ETHERNET_TYPE_IPV4) {
		if((*((PIPV4_ADDRESS)ipAddress1))==(*((PIPV4_ADDRESS)ipAddress2))) {
			return 1;
		}
	} else 
#endif
#if IPV6_ENABLED
	if(type==ETHERNET_TYPE_IPV6) {
		SMSC_ERROR(("IPV6 Address Compare is not implemented"));
		if((((PIPV6_ADDRESS)ipAddress1)->mAddress)==(((PIPV6_ADDRESS)ipAddress2)->mAddress)) {
			return 1;
		}
	} else
#endif
	{
		SMSC_ERROR(("EthernetArp_IpAddressCompare: type=%"U16_F", is not supported",(u16_t)type));
	}
	return 0;
}

static s8_t EthernetArp_FindEntry(
	u16_t type,			/* must be in host byte order */
	void * ipAddress,	/* must be in network byte order */
	int forceFlag)
{   
	s8_t oldest_pending = ARP_TABLE_SIZE, oldest_stable = ARP_TABLE_SIZE;
	s8_t empty = ARP_TABLE_SIZE;
	u8_t index = 0, age_pending = 0, age_stable = 0;
	/* oldest entry with packets on queue */
	s8_t oldest_queue = ARP_TABLE_SIZE;
	/* its age */
	u8_t age_queue = 0;

	/**
	* a) do a search through the cache, remember candidates
	* b) select candidate entry
	* c) create new entry
	*/

	/* a) in a single search sweep, do all of this
	* 1) remember the first empty entry (if any)
	* 2) remember the oldest stable entry (if any)
	* 3) remember the oldest pending entry without queued packets (if any)
	* 4) remember the oldest pending entry with queued packets (if any)
	* 5) search for a matching IP entry, either pending or stable
	*    until 5 matches, or all entries are searched for.
	*/

	for (index = 0; index < ARP_TABLE_SIZE; index++) {
		u8_t state=gArpTable[index].mState;
		if(state==ARP_STATE_EMPTY) {
			/* Found an empty entry */
			if(empty==ARP_TABLE_SIZE) {
				/* This is the first empty entry found */
				SMSC_TRACE(ETHERNET_DEBUG, ("EthernetArp_FindEntry: found empty entry %"U16_F, (u16_t)index));
				/* remember first empty entry */
				empty = index;
			}
		} else if (state == ARP_STATE_PENDING) {
			/* found a pending entry */
			if (ipAddress && (type==gArpTable[index].mType) && 
				EthernetArp_IpAddressCompare(type,ipAddress,&gArpTable[index].mIpAddress)) 
			{
				/* IP address matches the IP address in the ARP entry */
				SMSC_TRACE(ETHERNET_DEBUG, ("EthernetArp_FindEntry: found matching pending entry %"U16_F, (u16_t)index));
				/* found exact IP address match, simply bail out */
				return index;
			} else if (!PacketQueue_IsEmpty(&(gArpTable[index].mPacketQueue))) {
				/* pending with queued packets */
				if (gArpTable[index].mEntryAge >= age_queue) {
					oldest_queue = index;
					age_queue = gArpTable[index].mEntryAge;
				}
			} else {
				/* pending without queued packets */
				if (gArpTable[index].mEntryAge >= age_pending) {
					oldest_pending = index;
					age_pending = gArpTable[index].mEntryAge;
				}
			} 
		} else if (state == ARP_STATE_STABLE) {
			/* found a stable entry */
			/* if given, does IP address match IP address in ARP entry? */
			if (ipAddress && (type==gArpTable[index].mType) && 
				EthernetArp_IpAddressCompare(type,ipAddress,&gArpTable[index].mIpAddress)) 
			{
				/* IP address matches the IP address in the ARP entry */
				SMSC_TRACE(ETHERNET_DEBUG, ("EthernetArp_FindEntry: found matching stable entry %"U16_F, (u16_t)index));
				/* found exact IP address match, simply bail out */
				return index;
			} else if (gArpTable[index].mEntryAge >= age_stable) {
				/* remember the oldest stable entry */
				oldest_stable = index;
				age_stable = gArpTable[index].mEntryAge;
			}
		}
	}

	if ((empty == ARP_TABLE_SIZE) && (forceFlag == 0))
	{
		/* no empty entry found and not allowed to recycle */
		return (s8_t)ERR_MEM;
	}
  
	/* b) choose the least destructive entry to recycle:
	* 1) empty entry
	* 2) oldest stable entry
	* 3) oldest pending entry without queued packets
	* 4) oldest pending entry without queued packets
	* 
	*/ 

	if (empty < ARP_TABLE_SIZE) {
		/* 1) empty entry is available */
		index = empty;
		SMSC_TRACE(ETHERNET_DEBUG, ("EthernetArp_FindEntry: selecting empty entry %"U16_F, (u16_t)index));
	} else if (oldest_stable < ARP_TABLE_SIZE) {
		/* 2) found recyclable stable entry */
		index = oldest_stable;
		SMSC_TRACE(ETHERNET_DEBUG, ("EthernetArp_FindEntry: selecting oldest stable entry %"U16_F, (u16_t)index));
		/* no queued packets should exist on stable entries */
		SMSC_ASSERT(PacketQueue_IsEmpty(&(gArpTable[index].mPacketQueue)));
	} else if (oldest_pending < ARP_TABLE_SIZE) {
		/* 3) found recyclable pending entry without queued packets */
		index = oldest_pending;
		SMSC_TRACE(ETHERNET_DEBUG, ("EthernetArp_FindEntry: selecting oldest pending entry %"U16_F" (without queued packets)", (u16_t)index));
	} else if (oldest_queue < ARP_TABLE_SIZE) {
		/* 4) found recyclable pending entry with queued packets */
		/* recycle oldest pending */
		index = oldest_queue;
		SMSC_TRACE(ETHERNET_DEBUG, ("EthernetArp_FindEntry: selecting oldest pending entry %"U16_F", freeing queued packets", (u16_t)index));
		{ /* Empty the pending queue */
			PPACKET_BUFFER packet;
			while((packet=PacketQueue_Pop(&(gArpTable[index].mPacketQueue)))!=NULL) {
				PacketBuffer_DecreaseReference(packet);
			}
		}
	} else {
		/* no empty or recyclable entries found */
		SMSC_ERROR(("EthernetArp_FindEntry: Could not find any entries, yet we are willing to recycle any entry."));
		return (s8_t)ERR_MEM;
	}

	/* { empty or recyclable entry found } */
	SMSC_ASSERT(index < ARP_TABLE_SIZE);

	/* recycle entry (no-op for an already empty entry) */
	gArpTable[index].mState = ARP_STATE_EMPTY;

	if (ipAddress != NULL) {
		/* IP address is provided, lets set it */
		gArpTable[index].mType=type;
#if IPV4_ENABLED
		if(type==ETHERNET_TYPE_IPV4) {
			gArpTable[index].mIpAddress.mIpv4Address=
				(*((PIPV4_ADDRESS)ipAddress));
		} else 
#endif
#if IPV6_ENABLED
		if(type==ETHERNET_TYPE_IPV6) {
			SMSC_ERROR(("EthernetArp_FindEntry: IPV6 is not supported"));
		} else
#endif
		{
			SMSC_ERROR(("EthernetArp_FindEntry: Unknown type"));
		}
	}
	gArpTable[index].mEntryAge = 0;
	return (err_t)index;	
}

static void EthernetArp_UpdateArpEntry(
	struct NETWORK_INTERFACE * networkInterface,
	u16_t type,/* must be in host byte order */
	void * ipAddress,/* must be in network byte order */
	u8_t * ethernetAddress,
	int forceFlag)
{
	s8_t entryIndex;
	PPACKET_BUFFER packet;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
#if IPV4_ENABLED
	if(type==ETHERNET_TYPE_IPV4) {
		/* non-unicast address? */
		if ((ipAddress==NULL)||
			IPV4_ADDRESS_IS_ANY(((PIPV4_ADDRESS)ipAddress)) ||
			IPV4_ADDRESS_IS_BROADCAST(*((PIPV4_ADDRESS)ipAddress), networkInterface) ||
			IPV4_ADDRESS_IS_MULTICAST(*((PIPV4_ADDRESS)ipAddress))) 
		{
			SMSC_NOTICE(ETHERNET_DEBUG, ("EthernetArp_UpdateArpEntry: will not add non-unicast IP address to ARP cache"));
			return;
		}
		#if SMSC_TRACE_ENABLED
		{
			char ipAddressString[IPV4_ADDRESS_STRING_SIZE];
			SMSC_TRACE(ETHERNET_DEBUG, ("EthernetArp_UpdateArpEntry: %s - %02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F,
											IPV4_ADDRESS_TO_STRING(ipAddressString,*((PIPV4_ADDRESS)ipAddress)),
											ethernetAddress[0], ethernetAddress[1], 
											ethernetAddress[2], ethernetAddress[3], 
											ethernetAddress[4], ethernetAddress[5]));
		}
		#endif
	} else 
#endif
#if IPV6_ENABLED
	if(type==ETHERNET_TYPE_IPV6) {
		SMSC_ERROR(("EthernetArp_UpdateArpEntry: IPV6 is not supported"));
		/* TODO check address is unicast */
	} else
#endif
	{
		SMSC_ERROR(("EthernetArp_UpdateArpEntry: unknown type found, type=0x%04"X16_F,type));
	}
	/* find or create ARP entry */
	entryIndex = EthernetArp_FindEntry(type,ipAddress, forceFlag);
	/* bail out if no entry could be found */
	if (entryIndex < 0) {
		/* Entry does not exist, nothing to update */
		return;
	}

	/* mark it stable */
	gArpTable[entryIndex].mState = ARP_STATE_STABLE;
	/* record network interface */
	gArpTable[entryIndex].mNetworkInterface = networkInterface;

	SMSC_TRACE(ETHERNET_DEBUG, ("EthernetArp_UpdateArpEntry: updating stable entry %"S16_F, (s16_t)entryIndex));
	
	/* update address */
	memcpy(gArpTable[entryIndex].mEthernetAddress,ethernetAddress,6);
	/* reset time stamp */
	gArpTable[entryIndex].mEntryAge = 0;
	
	/* this is where we will send out queued packets! */
	while((packet=PacketQueue_Pop(&(gArpTable[entryIndex].mPacketQueue)))!=NULL) {
		/* Ethernet header */
		u8_t * ethernetHeader = PacketBuffer_GetStartPoint(packet);
		
		/* fill-in Ethernet header */
		ETHERNET_HEADER_SET_DESTINATION_ADDRESS(ethernetHeader,ethernetAddress);
		ETHERNET_HEADER_SET_SOURCE_ADDRESS(ethernetHeader,networkInterface->mLinkData.mEthernetData.mMacAddress);
		ETHERNET_HEADER_SET_TYPE(ethernetHeader,gArpTable[entryIndex].mType);
		
		SMSC_TRACE(ETHERNET_DEBUG, ("EthernetArp_UpdateArpEntry: sending queued IP packet %p.", (void *)packet));
		/* send the queued IP packet */
		((ETHERNET_OUTPUT_FUNCTION)(networkInterface->mLinkData.mEthernetData.mOutputFunction))(
			networkInterface,packet);
	}
	return;
}

void Ethernet_Initialize(void)
{
	/* Initialize the ARP table */
	{
		int index=0;
		while(index<ARP_TABLE_SIZE) {
			PETHERNET_ARP_ENTRY arpEntry=(PETHERNET_ARP_ENTRY)(&(gArpTable[index]));
			memset(arpEntry,0,sizeof(*arpEntry));
			ASSIGN_SIGNATURE(arpEntry,ETHERNET_ARP_ENTRY_SIGNATURE);
			PacketQueue_Initialize(&(arpEntry->mPacketQueue));
			arpEntry->mState=ARP_STATE_EMPTY;
			arpEntry->mType=ETHERNET_TYPE_UNDEFINED;
			arpEntry->mEntryAge=0;
			index++;
		}
	}
	Task_Initialize(
		&gArpTimerTask,/*TASK_PRIORITY_TIMER*/TASK_PRIORITY_TIMER_ARP,
		EthernetArp_Timer,NULL);
	TaskManager_ScheduleAsSoonAsPossible(&gArpTimerTask);
}

void Ethernet_InitializeInterface(
	struct NETWORK_INTERFACE * networkInterface,
	ETHERNET_OUTPUT_FUNCTION outputFunction,
	u8_t hardwareAddress[6],
	u8_t enableFlags)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(outputFunction!=NULL);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(hardwareAddress!=NULL);
	SMSC_ASSERT(enableFlags!=0);/* Must enable one ip type */
	networkInterface->mLinkType=NETWORK_INTERFACE_LINK_TYPE_ETHERNET;
	ASSIGN_SIGNATURE(&(networkInterface->mLinkData.mEthernetData),ETHERNET_DATA_SIGNATURE);
	networkInterface->mLinkData.mEthernetData.mOutputFunction=outputFunction;
	memcpy(networkInterface->mLinkData.mEthernetData.mMacAddress,hardwareAddress,6);
#if IPV4_ENABLED
	if(enableFlags&ETHERNET_FLAG_ENABLE_IPV4) {
		/* TODO Setup IPv4 entry point */
		Ipv4_InitializeInterface(networkInterface,Ethernet_Ipv4Output);
	}
#endif
#if IPV6_ENABLED
	if(enableFlags&ETHERNET_FLAG_ENABLE_IPV6) {
		/* TODO Setup IPv6 entry point */
	}
#endif
}

#if IPV4_ENABLED
#define ETHERNET_ARP_HEADER_MINIMUM_LENGTH	(42)
#elif IPV6_ENABLED
#define ETHERNET_ARP_HEADER_MINIMUM_LENGTH	(2000)
#else
/* No protocol type is supported, set minimum length beyond
	ethernet frame size so all arp packets will be treated as bad */
#define ETHERNET_ARP_HEADER_MINIMUM_LENGTH	(2000)
#endif


static void EthernetArp_Input(
	struct NETWORK_INTERFACE * networkInterface,
	PPACKET_BUFFER packet)
{
	u8_t * ethernetArpHeader;
   	u8_t for_us=0;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packet!=NULL);
	CHECK_SIGNATURE(packet,PACKET_BUFFER_SIGNATURE);
	SMSC_ASSERT(packet->mNext==NULL);
	SMSC_ASSERT(packet->mThisLength==packet->mTotalLength);
	if(packet->mThisLength<ETHERNET_ARP_HEADER_MINIMUM_LENGTH) {
		SMSC_NOTICE(ETHERNET_DEBUG,("EthernetArp_Input: packet dropped, too short (%"U16_F"/%"U16_F")",
			packet->mThisLength,(u16_t)ETHERNET_ARP_HEADER_MINIMUM_LENGTH));
		goto DONE;
	}
	ethernetArpHeader=PacketBuffer_GetStartPoint(packet);
	
/*	PRINT_ETHERNET_IPV4_ARP_HEADER(1,ethernetArpHeader);*/
	
    if(ETHERNET_ARP_HEADER_GET_HARDWARE_TYPE(ethernetArpHeader)!=HARDWARETYPE_ETHERNET) {
    	SMSC_NOTICE(ETHERNET_DEBUG,("EthernetArp_Input: packet dropped, invalid hardware type = 0x%04"U16_F,
    		ETHERNET_ARP_HEADER_GET_HARDWARE_TYPE(ethernetArpHeader)));
    	goto DONE;
    }
    if(ETHERNET_ARP_HEADER_GET_HARDWARE_ADDRESS_LENGTH(ethernetArpHeader)!=6) {
    	SMSC_NOTICE(ETHERNET_DEBUG,("EthernetArp_Input: packet dropped, invalid hardware address length = %"U16_F,
    		(u16_t)ETHERNET_ARP_HEADER_GET_HARDWARE_ADDRESS_LENGTH(ethernetArpHeader)));
    	goto DONE;
    }
#if IPV4_ENABLED
    if(ETHERNET_ARP_HEADER_GET_PROTOCOL(ethernetArpHeader)==ETHERNET_TYPE_IPV4) {
    	IPV4_ADDRESS sourceIpAddress,destinationIpAddress;
    	if(ETHERNET_ARP_HEADER_GET_PROTOCOL_ADDRESS_LENGTH(ethernetArpHeader)!=4) {
    		SMSC_NOTICE(ETHERNET_DEBUG,("EthernetArp_Input: packet dropped, invalid ipv4 address length = %"U16_F,
    			(u16_t)ETHERNET_ARP_HEADER_GET_PROTOCOL_ADDRESS_LENGTH(ethernetArpHeader)));
    		goto DONE;
    	}
    	
		/* Copy struct IPV4_ADDRESS2 to aligned IPV4_ADDRESS, to support compilers without
		* structure packing (not using structure copy which breaks strict-aliasing rules). */
		memcpy(&sourceIpAddress, ETHERNET_IPV4_ARP_HEADER_GET_SOURCE_PROTOCOL_ADDRESS(ethernetArpHeader), 4);
		memcpy(&destinationIpAddress, ETHERNET_IPV4_ARP_HEADER_GET_TARGET_PROTOCOL_ADDRESS(ethernetArpHeader), 4);

		if (networkInterface->mIpv4Data.mAddress == 0) {
			/* This interface is not configured */ 
			/* this arp can not be for us */
			for_us = 0;
		} else {
			/* set for_us if we are the destination */
			for_us = IPV4_ADDRESS_COMPARE(destinationIpAddress,networkInterface->mIpv4Data.mAddress);
		}
		
		if(for_us) {
			/* ARP message is directed to us */
			/* add IP address in ARP cache; assume requester wants to talk to us.
			* can result in directly sending the queued packets for this host. */
			EthernetArp_UpdateArpEntry(
				networkInterface,
				ETHERNET_TYPE_IPV4,
				&sourceIpAddress,
				ETHERNET_IPV4_ARP_HEADER_GET_SOURCE_HARDWARE_ADDRESS(ethernetArpHeader),
				1);/* 1==recycle an old entry if necessary */	
		} else {
			/* ARP message is not directed to us */
			/* Update only if present */
			EthernetArp_UpdateArpEntry(
				networkInterface,
				ETHERNET_TYPE_IPV4,
				&sourceIpAddress,
				ETHERNET_IPV4_ARP_HEADER_GET_SOURCE_HARDWARE_ADDRESS(ethernetArpHeader),
				0);/* 0==only update if an entry is already present */	
		}
   		#if (DHCP_ENABLED && DHCP_DOES_ARP_CHECK)
		if(ETHERNET_ARP_HEADER_GET_OP_CODE(ethernetArpHeader)==ARP_OPCODE_REPLY) {
		if(networkInterface->mFlags & NETWORK_INTERFACE_FLAG_DHCP){   			
   			Dhcp_ArpReply(networkInterface,&sourceIpAddress);
}else{
		Manual_ArpReply(networkInterface,&sourceIpAddress,ETHERNET_IPV4_ARP_HEADER_GET_SOURCE_HARDWARE_ADDRESS(ethernetArpHeader));
}
		}
   		#endif
    } else
#endif /* IPV4_ENABLED */
#if IPV6_ENABLED
    if(ethernetArpHeader->mProtocol==smsc_htons(ETHERNET_TYPE_IPV6)) {
    	PETHERNET_IPV6_ARP_HEADER ethernetIpv6ArpHeader=(PETHERNET_IPV6_ARP_HEADER)ethernetArpHeader;
    	if(ethernetArpHeader->mProtocolAddressLength!=sizeof(IPV6_ADDRESS)) {
    		SMSC_NOTICE(ETHERNET_DEBUG,("EthernetArp_Input: packet dropped, invalid ipv6 address length = %"U16_F,
    			(u16_t)ethernetArpHeader->mProtocolAddressLength));
    		goto DONE;
    	}
    	/* TODO update Arp Cache here,
    	also update for_us variable as done in IPV4 code,
    	it is used below in the request response code */
    	SMSC_ERROR(("EthernetArp_Input: IPV6 is not Implemented"));
    } else
#endif /* IPV6_ENABLED */
    {
    	SMSC_NOTICE(ETHERNET_DEBUG,("EthernetArp_Input: packet dropped, unknown protocol = 0x%04"U16_F,
    		ETHERNET_ARP_HEADER_GET_PROTOCOL(ethernetArpHeader)));
    	goto DONE;
    }
    switch(ETHERNET_ARP_HEADER_GET_OP_CODE(ethernetArpHeader)) {
    case ARP_OPCODE_REQUEST:
    	SMSC_TRACE(ETHERNET_DEBUG,("EthernetArp_Input: Incoming ARP Request"));
    	if(for_us) {
    		SMSC_TRACE(ETHERNET_DEBUG,("EthernetArp_Input: Replying to ARP request for our IP address"));
    		/* reusing packet buffer for outgoing reply */
    		ETHERNET_ARP_HEADER_SET_OP_CODE(ethernetArpHeader,ARP_OPCODE_REPLY);
#if IPV4_ENABLED
    		if(ETHERNET_ARP_HEADER_GET_PROTOCOL(ethernetArpHeader)==ETHERNET_TYPE_IPV4)
			{
				ETHERNET_IPV4_ARP_HEADER_SET_TARGET_PROTOCOL_ADDRESS(ethernetArpHeader,
					ETHERNET_IPV4_ARP_HEADER_GET_SOURCE_PROTOCOL_ADDRESS(ethernetArpHeader));
				ETHERNET_IPV4_ARP_HEADER_SET_SOURCE_PROTOCOL_ADDRESS(ethernetArpHeader,
					&(networkInterface->mIpv4Data.mAddress));
				ETHERNET_IPV4_ARP_HEADER_SET_TARGET_HARDWARE_ADDRESS(ethernetArpHeader,
					ETHERNET_IPV4_ARP_HEADER_GET_SOURCE_HARDWARE_ADDRESS(ethernetArpHeader));
				ETHERNET_IPV4_ARP_HEADER_SET_SOURCE_HARDWARE_ADDRESS(ethernetArpHeader,
					networkInterface->mLinkData.mEthernetData.mMacAddress);
				ETHERNET_HEADER_SET_DESTINATION_ADDRESS(ethernetArpHeader,
					ETHERNET_IPV4_ARP_HEADER_GET_TARGET_HARDWARE_ADDRESS(ethernetArpHeader));
				ETHERNET_HEADER_SET_SOURCE_ADDRESS(ethernetArpHeader,
					networkInterface->mLinkData.mEthernetData.mMacAddress);
				
				/* set proper packet size */
				packet->mThisLength=42;
				packet->mTotalLength=42;
			} else 
#endif /* IPV4_ENABLED */
#if IPV6_ENABLED
			if(ethernetArpHeader->mProtocol==smsc_htons(ETHERNET_TYPE_IPV6)) {
				SMSC_ERROR(("EthernetArp_Input: IPV6 is not supported"));
				goto DONE;
			} else
#endif /* IPV6_ENABLED */
			{
				/* mProtocol was checked above and there for this error 
				  should never happen */
				SMSC_ERROR(("EthernetArp_Input: Invalid protocol type"));
			}
			/* The following settings are checked earlier and they should not have changed
					So we are not taking the time to reset them here 
				ethernetArpHeader->mHardwareType=smsc_htons(HARDWARETYPE_ETHERNET);
				ethernetArpHeader->mHardwareAddressLength=6;
				ethernetArpHeader->mEthernetHeader.mType=smsc_htons(ETHERNET_TYPE_ARP);*/

			((ETHERNET_OUTPUT_FUNCTION)(networkInterface->mLinkData.mEthernetData.mOutputFunction))(
				networkInterface,packet);
			/* packet is now owned by the interface,
			  no need to decrease its reference as would be done
			  if we do not return here */
			return;
    	} else {
    		SMSC_NOTICE(ETHERNET_DEBUG,("EthernetArp_Input: ARP request was not for us"));
    	}
		break;
   	case ARP_OPCODE_REPLY:
   		/*ARP reply, We already updated the ARP cache earlier */
   		SMSC_TRACE(ETHERNET_DEBUG,("EthernetArp_Input: Incoming ARP reply"));
   		break;
    default:
    	SMSC_NOTICE(ETHERNET_DEBUG,("EthernetArp_Input: ARP unknown opcode type %"U16_F,
    		ETHERNET_ARP_HEADER_GET_OP_CODE(ethernetArpHeader)));
    	break;
    }
DONE:
	PacketBuffer_DecreaseReference(packet);
}

void Ethernet_Input(
	struct NETWORK_INTERFACE * networkInterface,
	PPACKET_BUFFER packet)
{
	u8_t * ethernetHeader;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packet!=NULL);
	ethernetHeader=PacketBuffer_GetStartPoint(packet);
	switch(ETHERNET_HEADER_GET_TYPE(ethernetHeader)) {
#if IPV4_ENABLED
	case ETHERNET_TYPE_IPV4:
		/* This is an IPv4 Packet */
		/*TODO grab ethernet and ip source address and update the arp cache */
		if(PacketBuffer_MoveStartPoint(packet,14)!=ERR_OK) {
			SMSC_NOTICE(ETHERNET_DEBUG,("Ethernet_Input: IPv4 detected but could not move start point"));
			goto DROP_PACKET;
		}
		Ipv4_Input(networkInterface,packet);
		break;
#endif
#if IPV6_ENABLED
	case ETHERNET_TYPE_IPV6:
		SMSC_ERROR(("Ethernet_Input: IPv6 is not implemented"));
		break;
#endif
	case ETHERNET_TYPE_ARP:
		/* This is an ARP packet */
		SMSC_TRACE(ETHERNET_DEBUG,("ETHERNET_TYPE_ARP detected"));
		EthernetArp_Input(networkInterface,packet);
		break;
	default:
		SMSC_NOTICE(ETHERNET_DEBUG,
			("Ethernet_Input: Discarding unknown packet type 0x%"X16_F,
				ETHERNET_HEADER_GET_TYPE(ethernetHeader)));
		goto DROP_PACKET;
	}
	return;
DROP_PACKET:
	PacketBuffer_DecreaseReference(packet);
}

/************************函数说明***************************/

/*函数名:void Manual_ArpInit(void)*/
/*开发人和开发时间:                                      */
/*函数功能描述:                                    */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                  */
static boolean b_ManuIp_Valid = true;
extern SMSC_SEMAPHORE ptcpip_sema;
extern struct NETWORK_INTERFACE smscNetworkInterface;

void Manual_ArpInit(void)
{
   b_ManuIp_Valid = true;
}


/************************函数说明***************************/

/*函数名:boolean CH_GetManualIPStatus(void)*/
/*开发人和开发时间:                                      */
/*函数功能描述:                                    */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                  */
boolean CH_GetManualIPStatus(void)
{
	boolean b_ret;
	smsc_semaphore_wait(ptcpip_sema);
	b_ret =  b_ManuIp_Valid ;
	smsc_semaphore_signal(ptcpip_sema);
	return b_ret;
   
}
/************************函数说明***************************/

/*函数名: void ManualIP_Check(struct NETWORK_INTERFACE * networkInterface)*/
/*开发人和开发时间:                                      */
/*函数功能描述:发送ARP请求                                    */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                  */
 void ManualIP_Check(struct NETWORK_INTERFACE * networkInterface)
{
	u8_t i = 0;

	struct NETWORK_INTERFACE * netif = &smscNetworkInterface;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(netif!=NULL);
	CHECK_SIGNATURE(netif,NETWORK_INTERFACE_SIGNATURE);
	for(;i < 3;i++)
	{
	smsc_semaphore_wait(ptcpip_sema);
	Ethernet_Ipv4Output(netif,NULL,netif->mIpv4Data.mAddress);
	smsc_semaphore_signal(ptcpip_sema);
}

}

/************************函数说明***************************/
/*函数名:void Manual_ArpReply(struct NETWORK_INTERFACE * networkInterface, PIPV4_ADDRESS SourceipAddress,u8_t * ethernetAddress)
*/
/*开发人和开发时间:                */
/*函数功能描述: 在手动设置模式下，通过ARP功能判断手动设置的IP 是否有存在冲突*/
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                  */
void Manual_ArpReply(struct NETWORK_INTERFACE * networkInterface, PIPV4_ADDRESS SourceipAddress,u8_t * ethernetAddress)
{
        extern u8_t SMSC_MAC_ADDRESS[6];
		
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(SourceipAddress!=NULL);	
       if(memcmp(ethernetAddress,SMSC_MAC_ADDRESS,6) == 0)
       	{
                       return;
       	}
	if (IPV4_ADDRESS_COMPARE(*SourceipAddress, networkInterface->mIpv4Data.mAddress)) 
	{
		b_ManuIp_Valid = false;
	}
}
/*EOF*/
