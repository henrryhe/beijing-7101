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
File: ipv4.h
*/

#ifndef IPV4_H
#define IPV4_H     

#include "smsc_environment.h"
#include "packet_manager.h"

struct NETWORK_INTERFACE;

#if !IPV4_ENABLED
#error IPv4 is not enabled
#endif

typedef u32_t IPV4_ADDRESS, * PIPV4_ADDRESS;

/* This is the type of function the IPv4 layer will
  call when it wants to transmit on a network interface */
typedef void (*IPV4_OUTPUT_FUNCTION)(
	struct NETWORK_INTERFACE * networkInterface, PPACKET_BUFFER packet,
	IPV4_ADDRESS destinationAddress);

typedef struct IPV4_DATA_ {
	DECLARE_SIGNATURE
	IPV4_OUTPUT_FUNCTION mOutputFunction;
	IPV4_ADDRESS mAddress;/* must be stored in network byte order */
	IPV4_ADDRESS mNetMask;/* must be stored in network byte order */
	IPV4_ADDRESS mGateway;/* must be stored in network byte order */
#define DHCP_MAX_DNS 2/*wufei add for Manul set dns 2008-12-25*/
	u32_t mDnsNm; /* actual number of DNS servers obtained */
	IPV4_ADDRESS mManuDns[DHCP_MAX_DNS];/* DNS server addresses */
} IPV4_DATA, * PIPV4_DATA;

void Ipv4_Initialize(void);
void Ipv4_InitializeInterface(
	struct NETWORK_INTERFACE * networkInterface,
	IPV4_OUTPUT_FUNCTION outputFunction);
void Ipv4_SetAddresses(
	struct NETWORK_INTERFACE * networkInterface,
	IPV4_ADDRESS ipAddress,
	IPV4_ADDRESS netMask,
	IPV4_ADDRESS gateway);
PPACKET_BUFFER Ipv4_AllocatePacket(
	PIPV4_ADDRESS destinationAddress,
	u32_t size);
u16_t Ipv4_GetMaximumTransferUnit(IPV4_ADDRESS destinationAddress);
void Ipv4_Input(
	struct NETWORK_INTERFACE * networkInterface,
	PPACKET_BUFFER packet);
void Ipv4_OutputWithHeaderIncluded(
	struct NETWORK_INTERFACE * networkInterface,
	PPACKET_BUFFER packet,
	IPV4_ADDRESS destinationAddress);

void Ipv4_Output(
	PPACKET_BUFFER packet,
	IPV4_ADDRESS sourceAddress,
	IPV4_ADDRESS destinationAddress,
	u8_t protocol);

#define IP_PROTOCOL_ICMP 	(1)
#define IP_PROTOCOL_UDP		(17)
#define IP_PROTOCOL_UDPLITE	(136)
#define IP_PROTOCOL_TCP		(6)

struct NETWORK_INTERFACE * Ipv4_GetRoute(IPV4_ADDRESS destination);
IPV4_ADDRESS Ipv4_GetSourceAddress(IPV4_ADDRESS destination);
u16_t Ipv4_CheckSum(void * dataptr, u16_t len);
u16_t Ipv4_ChainedCheckSum(PPACKET_BUFFER packetBufferList);
u16_t Ipv4_PseudoCheckSum(PPACKET_BUFFER packet,
		PIPV4_ADDRESS sourceAddress, 
		PIPV4_ADDRESS destinationAddress,
		u8_t protocol, u16_t length);

#define IPV4_ADDRESS_STRING_SIZE	(16)
char * IPV4_ADDRESS_TO_STRING(char * stringDataSpace,IPV4_ADDRESS address);

extern IPV4_ADDRESS IPV4_ADDRESS_ANY;
extern IPV4_ADDRESS IPV4_ADDRESS_BROADCAST;
#define IPV4_ADDRESS_IS_ANY(pIpAddress) ((pIpAddress==NULL)||((*(pIpAddress))==((IPV4_ADDRESS)0)))
u8_t IPV4_ADDRESS_IS_BROADCAST(IPV4_ADDRESS ipAddress,struct NETWORK_INTERFACE * networkInterface);
#define IPV4_ADDRESS_COMPARE(ipAddress1,ipAddress2) ((ipAddress1)==(ipAddress2))
#define IPV4_ADDRESS_NETWORK_COMPARE(ipAddress1,ipAddress2,netMask)	\
	((ipAddress1&netMask)==(ipAddress2&netMask))

#if (ENDIAN_SETTING==LITTLE_ENDIAN)
#define IPV4_ADDRESS_IS_MULTICAST(ipAddress) ((((u32_t)(ipAddress)) & ((u32_t)0x000000F0)) == ((u32_t)0x000000E0))
#define IPV4_ADDRESS_GET_BYTE_1(ipAddress) ((u8_t)((ipAddress)))
#define IPV4_ADDRESS_GET_BYTE_2(ipAddress) ((u8_t)((ipAddress)>>8))
#define IPV4_ADDRESS_GET_BYTE_3(ipAddress) ((u8_t)((ipAddress)>>16))
#define IPV4_ADDRESS_GET_BYTE_4(ipAddress) ((u8_t)((ipAddress)>>24))
#define IPV4_ADDRESS_MAKE(byte1,byte2,byte3,byte4)	\
	((IPV4_ADDRESS)((((u32_t)((u8_t)(byte4)))<<24)|	\
				(((u32_t)((u8_t)(byte3)))<<16)|		\
				(((u32_t)((u8_t)(byte2)))<<8)|		\
				((u32_t)((u8_t)(byte1)))))
#elif (ENDIAN_SETTING==BIG_ENDIAN)
#define IPV4_ADDRESS_IS_MULTICAST(ipAddress) ((((u32_t)(ipAddress)) & ((u32_t)0xF0000000)) == ((u32_t)0xE0000000))
#define IPV4_ADDRESS_GET_BYTE_1(ipAddress) ((u8_t)((ipAddress)>>24))
#define IPV4_ADDRESS_GET_BYTE_2(ipAddress) ((u8_t)((ipAddress)>>16))
#define IPV4_ADDRESS_GET_BYTE_3(ipAddress) ((u8_t)((ipAddress)>>8))
#define IPV4_ADDRESS_GET_BYTE_4(ipAddress) ((u8_t)((ipAddress)))
#define IPV4_ADDRESS_MAKE(byte1,byte2,byte3,byte4)	\
	((IPV4_ADDRESS)((((u32_t)((u8_t)(byte1)))<<24)|	\
				(((u32_t)((u8_t)(byte2)))<<16)|		\
				(((u32_t)((u8_t)(byte3)))<<8)|		\
				((u32_t)((u8_t)(byte4)))))
#else
#error Invalid ENDIAN_SETTING
#endif

#define IPV4_ADDRESS_SET(ipAddress,byte1,byte2,byte3,byte4)	\
	ipAddress=IPV4_ADDRESS_MAKE(byte1,byte2,byte3,byte4)

#define IPV4_HEADER_LENGTH	(20)

#define IPV4_FLAG_MORE_FRAGMENTS	(0x01)

/* The following macros work with either endian setting */
#define IPV4_HEADER_GET_VERSION(headerPointer)	\
	((((u8_t *)(headerPointer))[0])>>4)
#define IPV4_HEADER_GET_IP_HEADER_LENGTH(headerPointer) \
	((((u8_t *)(headerPointer))[0])&0x0F)
#define IPV4_HEADER_GET_TYPE_OF_SERVICE(headerPointer)	\
	(((u8_t *)(headerPointer))[1])
#ifndef M_WUFEI_080825
#define IPV4_HEADER_GET_TOTAL_LENGTH(headerPointer)	\
	smsc_ntohs(((u16_t *)(headerPointer))[1])
#define IPV4_HEADER_GET_IDENTIFICATION(headerPointer)	\
	smsc_ntohs(((u16_t *)(headerPointer))[2])
#define IPV4_HEADER_GET_FLAGS(headerPointer)	\
	((((u8_t *)(headerPointer))[6])>>5)
#define IPV4_HEADER_GET_FRAGMENT_OFFSET(headerPointer)	\
	(smsc_ntohs(((u16_t *)(headerPointer))[3])&0x1FFF)
#else
#define IPV4_HEADER_GET_TOTAL_LENGTH(headerPointer)	\
	smsc_ntohs(((u16_t)headerPointer[2])+(((u16_t)headerPointer[3])<<8))
#define IPV4_HEADER_GET_IDENTIFICATION(headerPointer)	\
	smsc_ntohs(((u16_t)headerPointer[4])+(((u16_t)headerPointer[5])<<8))	
#define IPV4_HEADER_GET_FLAGS(headerPointer)	\
	((((u8_t *)(headerPointer))[6])>>5)
#define IPV4_HEADER_GET_FRAGMENT_OFFSET(headerPointer)	\
	(smsc_ntohs(((u16_t)headerPointer[6])+(((u16_t)headerPointer[7])<<8))&0x1FFF)
#endif
#define IPV4_HEADER_GET_TIME_TO_LIVE(headerPointer)	\
	(((u8_t *)(headerPointer))[8])
#define IPV4_HEADER_GET_PROTOCOL(headerPointer)	\
	(((u8_t *)(headerPointer))[9])

/* NOTE: Endian conversion is not built into this macro
  it is assumed the user will apply endian conversion when necessary */
#ifndef M_WUFEI_080825
#define IPV4_HEADER_GET_HEADER_CHECKSUM(headerPointer)	\
	(((u16_t *)(headerPointer))[5])/*wufei modified from 9 to 5.080825*/
	
#define IPV4_HEADER_GET_SOURCE_ADDRESS(headerPointer)	\
	(((PIPV4_ADDRESS)(headerPointer))[3])
#define IPV4_HEADER_GET_DESTINATION_ADDRESS(headerPointer)	\
	(((PIPV4_ADDRESS)(headerPointer))[4])
#else
#define IPV4_HEADER_GET_HEADER_CHECKSUM(headerPointer)	\
	((u16_t)headerPointer[10]+(((u16_t)headerPointer[11])<<8))
	
#define IPV4_HEADER_GET_SOURCE_ADDRESS(headerPointer,srcAddr)	\
	(srcAddr =((IPV4_ADDRESS)headerPointer[12])|(((IPV4_ADDRESS)headerPointer[13])<<8)|(((IPV4_ADDRESS)headerPointer[14])<<16)|(((IPV4_ADDRESS)headerPointer[15])<<24))

#define IPV4_HEADER_GET_DESTINATION_ADDRESS(headerPointer,dstAddr)	\
	(dstAddr =((IPV4_ADDRESS)headerPointer[16])|(((IPV4_ADDRESS)headerPointer[17])<<8)|(((IPV4_ADDRESS)headerPointer[18])<<16)|(((IPV4_ADDRESS)headerPointer[19])<<24))
#endif
#define IPV4_HEADER_SET_VERSION(headerPointer,version)	\
	(((u8_t *)(headerPointer))[0])=(((version)<<4)|((((u8_t *)(headerPointer))[0])&0x0F))
#define IPV4_HEADER_SET_IP_HEADER_LENGTH(headerPointer,length)	\
	(((u8_t *)(headerPointer))[0])=(((length)&0x0F)|((((u8_t *)(headerPointer))[0])&0xF0))
#define IPV4_HEADER_SET_VERSION_AND_IP_HEADER_LENGTH(headerPointer,version,length)	\
	(((u8_t *)(headerPointer))[0])=(((version)<<4)|((length)&0x0F))
#define IPV4_HEADER_SET_TYPE_OF_SERVICE(headerPointer,typeOfService)	\
	(((u8_t *)(headerPointer))[1])=((u8_t)(typeOfService))
#define IPV4_HEADER_SET_TIME_TO_LIVE(headerPointer,timeToLive)	\
	(((u8_t *)(headerPointer))[8])=((u8_t)(timeToLive))
#define IPV4_HEADER_SET_PROTOCOL(headerPointer,protocol)	\
	(((u8_t *)(headerPointer))[9])=((u8_t)(protocol))
#ifndef M_WUFEI_080825
#define IPV4_HEADER_SET_TOTAL_LENGTH(headerPointer,totalLength)	\
	(((u16_t *)(headerPointer))[1])=smsc_htons(totalLength)
#define IPV4_HEADER_SET_IDENTIFICATION(headerPointer,identification)	\
	(((u16_t *)(headerPointer))[2])=smsc_htons(identification)
#define IPV4_HEADER_SET_FLAGS_AND_FRAGMENT_OFFSET(headerPointer,flags,fragmentOffset)	\
	(((u16_t *)(headerPointer))[3])=smsc_htons((((u16_t)(flags))<<13)|(((u16_t)(fragmentOffset))&0x1FFF))

/* NOTE: endian convertion is not built into this macro because it is assumed the 
  user will apply the endian conversion when necessary */
#define IPV4_HEADER_SET_HEADER_CHECKSUM(headerPointer,checksum)	\
	(((u16_t *)(headerPointer))[5])=((u16_t)(checksum))

#define IPV4_HEADER_SET_SOURCE_ADDRESS(headerPointer,ipv4Address)	\
	(((PIPV4_ADDRESS)(headerPointer))[3])=ipv4Address
#define IPV4_HEADER_SET_DESTINATION_ADDRESS(headerPointer,ipv4Address)	\
	(((PIPV4_ADDRESS)(headerPointer))[4])=ipv4Address
#else
#define IPV4_HEADER_SET_TOTAL_LENGTH(headerPointer,totalLength)	\
	do{	\
		(((u8_t *)(headerPointer))[2])=(smsc_htons(totalLength)&0xff);	\
		(((u8_t *)(headerPointer))[3])=(smsc_htons(totalLength)&0xff00)>>8;	\
	}while(0)

#define IPV4_HEADER_SET_IDENTIFICATION(headerPointer,identification)	\
	do{	\
		(((u8_t *)(headerPointer))[4])=(smsc_htons(identification)&0xff);	\
		(((u8_t *)(headerPointer))[5])=(smsc_htons(identification)&0xff00)>>8;	\
	}while(0)

#define IPV4_HEADER_SET_FLAGS_AND_FRAGMENT_OFFSET(headerPointer,flags,fragmentOffset)	\
	do{	\
		(((u8_t *)(headerPointer))[6])=(smsc_htons((((u16_t)(flags))<<13)|(((u16_t)(fragmentOffset))&0x1FFF))&0xff);	\
		(((u8_t *)(headerPointer))[7])=(smsc_htons((((u16_t)(flags))<<13)|(((u16_t)(fragmentOffset))&0x1FFF))&0xff00)>>8;	\
	}while(0)

#define IPV4_HEADER_SET_HEADER_CHECKSUM(headerPointer,checksum)	\
	do{	\
		(((u8_t *)(headerPointer))[10])=(((u16_t)checksum)&0xff);	\
		(((u8_t *)(headerPointer))[11])=(((u16_t)checksum)&0xff00)>>8;	\
	}while(0)

#define IPV4_HEADER_SET_SOURCE_ADDRESS(headerPointer,ipv4Address)	\
	do{	\
		(headerPointer[12])=(((IPV4_ADDRESS)ipv4Address)&0xff);	\
		(headerPointer[13])=(((IPV4_ADDRESS)ipv4Address)&0xff00)>>8;	\
		(headerPointer[14])=(((IPV4_ADDRESS)ipv4Address)&0xff0000)>>16;	\
		(headerPointer[15])=(((IPV4_ADDRESS)ipv4Address)&0xff000000)>>24;	\
	}while(0)

#define IPV4_HEADER_SET_DESTINATION_ADDRESS(headerPointer,ipv4Address)	\
	do{	\
		(headerPointer[16])=(((IPV4_ADDRESS)ipv4Address)&0xff);	\
		(headerPointer[17])=(((IPV4_ADDRESS)ipv4Address)&0xff00)>>8;	\
		(headerPointer[18])=(((IPV4_ADDRESS)ipv4Address)&0xff0000)>>16;	\
		(headerPointer[19])=(((IPV4_ADDRESS)ipv4Address)&0xff000000)>>24;	\
	}while(0)

#endif

#endif /* IPV4_H */
