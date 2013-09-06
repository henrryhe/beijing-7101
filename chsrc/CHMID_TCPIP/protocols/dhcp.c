/* Copyright (c) 2001-2004 Leon Woestenberg <leon.woestenberg@gmx.net>
 * Copyright (c) 2001-2004 Axon Digital Design B.V., The Netherlands.
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
 * This file is a contribution to the lwIP TCP/IP stack.
 * The Swedish Institute of Computer Science and Adam Dunkels
 * are specifically granted permission to redistribute this
 * source code.
 *
 * Author: Leon Woestenberg <leon.woestenberg@gmx.net>
 *
 * This is a DHCP client for the lwIP TCP/IP stack. It aims to conform
 * with RFC 2131 and RFC 2132.
 *
 * TODO:
 * - Proper parsing of DHCP messages exploiting file/sname field overloading.
 * - Add JavaDoc style documentation (API, internals).
 * - Support for interfaces other than Ethernet (SLIP, PPP, ...)
 *
 * Please coordinate changes and requests with Leon Woestenberg
 * <leon.woestenberg@gmx.net>
 *
 * Integration with your code:
 *
 * In lwip/dhcp.h
 * #define DHCP_COARSE_TIMER_SECS (recommended 60 which is a minute)
 * #define DHCP_FINE_TIMER_MSECS (recommended 500 which equals TCP coarse timer)
 *
 * Then have your application call Dhcp_CoarseTimer() and
 * Dhcp_FineTimer() on the defined intervals.
 *
 * Dhcp_Start(struct NETWORK_INTERFACE * networkInterface);
 * starts a DHCP client instance which configures the interface by
 * obtaining an IP address lease and maintaining it.
 *
 * Use Dhcp_Release(networkInterface) to end the lease and use Dhcp_Stop(networkInterface)
 * to remove the DHCP client.
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

#include "smsc_environment.h"

#if !DHCP_ENABLED /* don't build if not configured for use in custom_options.h */
/*#error DHCP is not enabled*/ /*zhangjian*/
#endif
#if !IPV4_ENABLED
#error IPv4 is not enabled
#endif
#if !UDP_ENABLED
#error UDP is not enabled
#endif
#if !ETHERNET_ENABLED
#error Ethernet is not enabled
#endif

#include "dhcp.h"
#include "ip.h"
#include "udp.h"
#include "network_stack.h"
#include "task_manager.h"


/** period (in seconds) of the application calling dhcp_coarse_tmr() */
#define DHCP_COARSE_TIMER_SECS 60 
/** period (in milliseconds) of the application calling dhcp_fine_tmr() */
#define DHCP_FINE_TIMER_MSECS 500 

 
/** DHCP message item offsets and length */
#define DHCP_MSG_OFS (UDP_DATA_OFS)  
  #define DHCP_OP_OFS (DHCP_MSG_OFS + 0)
  #define DHCP_HTYPE_OFS (DHCP_MSG_OFS + 1)
  #define DHCP_HLEN_OFS (DHCP_MSG_OFS + 2)
  #define DHCP_HOPS_OFS (DHCP_MSG_OFS + 3)
  #define DHCP_XID_OFS (DHCP_MSG_OFS + 4)
  #define DHCP_SECS_OFS (DHCP_MSG_OFS + 8)
  #define DHCP_FLAGS_OFS (DHCP_MSG_OFS + 10)
  #define DHCP_CIADDR_OFS (DHCP_MSG_OFS + 12)
  #define DHCP_YIADDR_OFS (DHCP_MSG_OFS + 16)
  #define DHCP_SIADDR_OFS (DHCP_MSG_OFS + 20)
  #define DHCP_GIADDR_OFS (DHCP_MSG_OFS + 24)
  #define DHCP_CHADDR_OFS (DHCP_MSG_OFS + 28)
  #define DHCP_SNAME_OFS (DHCP_MSG_OFS + 44)
  #define DHCP_FILE_OFS (DHCP_MSG_OFS + 108)
#define DHCP_MSG_LEN 236

#define DHCP_COOKIE_OFS (DHCP_MSG_OFS + DHCP_MSG_LEN)
#define DHCP_OPTIONS_OFS (DHCP_MSG_OFS + DHCP_MSG_LEN + 4)

/** DHCP client states */
#define DHCP_REQUESTING 1
#define DHCP_INIT 2
#define DHCP_REBOOTING 3
#define DHCP_REBINDING 4
#define DHCP_RENEWING 5
#define DHCP_SELECTING 6
#define DHCP_INFORMING 7
#define DHCP_CHECKING 8
#define DHCP_PERMANENT 9
#define DHCP_BOUND 10
/** not yet implemented #define DHCP_RELEASING 11 */
#define DHCP_BACKING_OFF 12
#define DHCP_OFF 13
 
#define DHCP_BOOTREQUEST 1
#define DHCP_BOOTREPLY 2

#define DHCP_DISCOVER 1
#define DHCP_OFFER 2
#define DHCP_REQUEST 3
#define DHCP_DECLINE 4
#define DHCP_ACK 5
#define DHCP_NAK 6
#define DHCP_RELEASE 7
#define DHCP_INFORM 8

#define DHCP_HTYPE_ETH 1

#define DHCP_HLEN_ETH 6

#define DHCP_BROADCAST_FLAG 15
#define DHCP_BROADCAST_MASK (1 << DHCP_FLAG_BROADCAST)

/** BootP options */
#define DHCP_OPTION_PAD 0
#define DHCP_OPTION_SUBNET_MASK 1 /* RFC 2132 3.3 */
#define DHCP_OPTION_ROUTER 3
#define DHCP_OPTION_DNS_SERVER 6 
#define DHCP_OPTION_HOSTNAME 12
#define DHCP_OPTION_IP_TTL 23
#define DHCP_OPTION_MTU 26
#define DHCP_OPTION_BROADCAST 28
#define DHCP_OPTION_TCP_TTL 37
#define DHCP_OPTION_END 255

/** DHCP options */
#define DHCP_OPTION_REQUESTED_IP 50 /* RFC 2132 9.1, requested IP address */
#define DHCP_OPTION_LEASE_TIME 51 /* RFC 2132 9.2, time in seconds, in 4 bytes */
#define DHCP_OPTION_OVERLOAD 52 /* RFC2132 9.3, use file and/or sname field for options */

#define DHCP_OPTION_MESSAGE_TYPE 53 /* RFC 2132 9.6, important for DHCP */
#define DHCP_OPTION_MESSAGE_TYPE_LEN 1

#define DHCP_OPTION_SERVER_ID 54 /* RFC 2132 9.7, server IP address */
#define DHCP_OPTION_PARAMETER_REQUEST_LIST 55 /* RFC 2132 9.8, requested option types */

#define DHCP_OPTION_MAX_MSG_SIZE 57 /* RFC 2132 9.10, message size accepted >= 576 */
#define DHCP_OPTION_MAX_MSG_SIZE_LEN 2

#define DHCP_OPTION_T1 58 /* T1 renewal time */
#define DHCP_OPTION_T2 59 /* T2 rebinding time */
#define DHCP_OPTION_CLIENT_ID 61
#define DHCP_OPTION_TFTP_SERVERNAME 66
#define DHCP_OPTION_BOOTFILE 67
#ifdef M_WUFEI_081120
#define DHCP_OPTION_60	60
#endif
/** possible combinations of overloading the file and sname fields with options */
#define DHCP_OVERLOAD_NONE 0
#define DHCP_OVERLOAD_FILE 1
#define DHCP_OVERLOAD_SNAME  2
#define DHCP_OVERLOAD_SNAME_FILE 3

#define DHCP_HEADER_LENGTH	(240)

#define DHCP_HEADER_GET_OP_CODE(header)	\
	(((u8_t *)(header))[0])
#define DHCP_HEADER_GET_HARDWARE_TYPE(header)	\
	(((u8_t *)(header))[1])
#define DHCP_HEADER_GET_HARDWARE_ADDRESS_LENGTH(header)	\
	(((u8_t *)(header))[2])
#define DHCP_HEADER_GET_HOPS(header)	\
	(((u8_t *)(header))[3])
#ifndef M_WUFEI_080825
#define DHCP_HEADER_GET_TRANSACTION_ID(header)	\
	smsc_ntohl(((u32_t *)(header))[1])
#define DHCP_HEADER_GET_YOUR_IP_ADDRESS(header)	\
	(((PIPV4_ADDRESS)(header))[4])
#else
#define DHCP_HEADER_GET_TRANSACTION_ID(header)	\
	smsc_ntohl(((u32_t)header[4])+(((u32_t)header[5])<<8)+(((u32_t)header[6])<<16)+(((u32_t)header[7])<<24))
#define DHCP_HEADER_GET_YOUR_IP_ADDRESS(header)	\
	(((u32_t)header[16])+(((u32_t)header[17])<<8)+(((u32_t)header[18])<<16)+(((u32_t)header[19])<<24))
#endif
#define DHCP_HEADER_GET_SECONDS(header)	\
	smsc_ntohs(((u16_t *)(header))[4])
#define DHCP_HEADER_GET_FLAGS(header)	\
	smsc_ntohs(((u16_t *)(header))[5])
#define DHCP_HEADER_GET_CLIENT_IP_ADDRESS(header)	\
	(((PIPV4_ADDRESS)(header))[3])
#define DHCP_HEADER_GET_NEXT_SERVER_IP_ADDRESS(header)	\
	(((PIPV4_ADDRESS)(header))[5])
#define DHCP_HEADER_GET_RELAY_IP_ADDRESS(header)	\
	(((PIPV4_ADDRESS)(header))[6])
#define DHCP_HEADER_GET_CLIENT_HARDWARE_ADDRESS(header)	\
	(&(((u8_t *)(header))[28]))
#define DHCP_HEADER_GET_SERVER_NAME(header)	\
	(&(((u8_t *)(header))[44]))
#define DHCP_HEADER_GET_BOOT_FILE_NAME(header)	\
	(&(((u8_t *)(header))[108]))
#define DHCP_HEADER_GET_COOKIE(header)	\
	smsc_ntohl(((u32_t *)(header))[59])
#define DHCP_HEADER_GET_OPTIONS(header)	\
	(&(((u8_t *)(header))[240]))

#define DHCP_CLIENT_HARDWARE_ADDRESS_LENGTH	(16)
#define DHCP_SERVER_NAME_LENGTH				(64)
#define DHCP_BOOT_FILE_NAME_LENGTH			(128)
#define DHCP_OPTIONS_LENGTH					(68)

#define DHCP_HEADER_SET_OP_CODE(header,opCode)	\
	(((u8_t *)(header))[0])=(opCode)
#define DHCP_HEADER_SET_HARDWARE_TYPE(header,type)	\
	(((u8_t *)(header))[1])=(type)
#define DHCP_HEADER_SET_HARDWARE_ADDRESS_LENGTH(header,length)	\
	(((u8_t *)(header))[2])=(length)
#define DHCP_HEADER_SET_HOPS(header,hops)	\
	(((u8_t *)(header))[3])=(hops)
#define DHCP_HEADER_SET_TRANSACTION_ID(header,transactionId)	\
	(((u32_t *)(header))[1])=smsc_htonl(transactionId)
#define DHCP_HEADER_SET_SECONDS(header,seconds)	\
	(((u16_t *)(header))[4])=smsc_htons(seconds)
#define DHCP_HEADER_SET_FLAGS(header,flags)	\
	(((u16_t *)(header))[5])=smsc_htons(flags)
#define DHCP_HEADER_SET_CLIENT_IP_ADDRESS(header,clientAddress)	\
	(((PIPV4_ADDRESS)(header))[3])=(clientAddress)
#define DHCP_HEADER_SET_YOUR_IP_ADDRESS(header,yourAddress)	\
	(((PIPV4_ADDRESS)(header))[4])=(yourAddress)
#define DHCP_HEADER_SET_NEXT_SERVER_IP_ADDRESS(header,serverAddress)	\
	(((PIPV4_ADDRESS)(header))[5])=(serverAddress)
#define DHCP_HEADER_SET_RELAY_IP_ADDRESS(header,relayAddress)	\
	(((PIPV4_ADDRESS)(header))[6])=(relayAddress)
#define DHCP_HEADER_SET_COOKIE(header,cookie)	\
	(((u32_t *)(header))[59])=smsc_htonl(cookie)


/** global transaction identifier, must be
 *  unique for each DHCP request. We simply increment, starting
 *  with this value (easy to match with a packet analyzer) */
static u32_t gTransactionID = 0xABCD0000;

/** DHCP client state machine functions */
static void Dhcp_HandleAck(struct NETWORK_INTERFACE * networkInterface);
static void Dhcp_HandleNak(struct NETWORK_INTERFACE * networkInterface);
static void Dhcp_HandleOffer(struct NETWORK_INTERFACE * networkInterface);

static err_t Dhcp_Discover(struct NETWORK_INTERFACE * networkInterface);
static err_t Dhcp_Select(struct NETWORK_INTERFACE * networkInterface);
static void Dhcp_Bind(struct NETWORK_INTERFACE * networkInterface);
#if DHCP_DOES_ARP_CHECK
static void Dhcp_Check(struct NETWORK_INTERFACE * networkInterface);
static err_t Dhcp_Decline(struct NETWORK_INTERFACE * networkInterface);
#endif
static err_t Dhcp_Rebind(struct NETWORK_INTERFACE * networkInterface);
static void Dhcp_SetState(PDHCP_DATA dhcpData, u8_t new_state);

/** receive, unfold, parse and free incoming messages */
static void Dhcp_Receive(
	void * param, struct UDP_CONTROL_BLOCK * udpControlBlock, 
	PPACKET_BUFFER packetBuffer,
	PIP_ADDRESS sourceAddress,
	u16_t sourcePort);
	
static err_t Dhcp_UnfoldReply(PDHCP_DATA dhcpData);
static u8_t *Dhcp_GetOptionPointer(PDHCP_DATA dhcpData, u8_t option_type);
static u8_t Dhcp_GetOptionByte(u8_t *pointer);
#if 0
static u16_t Dhcp_GetOptionShort(u8_t *pointer);
#endif
static u32_t Dhcp_GetOptionLong(u8_t *pointer);
static void Dhcp_FreeReply(PDHCP_DATA dhcpData);

/** set the DHCP timers */
static void Dhcp_RequestTimeOut(struct NETWORK_INTERFACE * networkInterface);
static void Dhcp_RenewalTimeOut(struct NETWORK_INTERFACE * networkInterface);
static void Dhcp_RebindTimeOut(struct NETWORK_INTERFACE * networkInterface);

/** build outgoing messages */
/** create a DHCP request, fill in common headers */
static err_t Dhcp_CreateRequest(struct NETWORK_INTERFACE * networkInterface);
/** free a DHCP request */
static void Dhcp_DeleteRequest(struct NETWORK_INTERFACE * networkInterface);
/** add a DHCP option (type, then length in bytes) */
static void Dhcp_SetOption(PDHCP_DATA dhcpData, u8_t option_type, u8_t option_len);
/** add option values */
static void Dhcp_SetOptionByte(PDHCP_DATA dhcpData, u8_t value);
static void Dhcp_SetOptionShort(PDHCP_DATA dhcpData, u16_t value);
static void Dhcp_SetOptionLong(PDHCP_DATA dhcpData, u32_t value);
/** always add the DHCP options trailer to end and pad */
static void Dhcp_AddOptionTrailer(PDHCP_DATA dhcpData);

/** to be called every minute */
static void Dhcp_CoarseTimer(void);
/** to be called every half second */
static void Dhcp_FineTimer(void);

static TASK gDhcpTimer;
static void Dhcp_Timer(void * param)
{
	static int count=0;
	
	/* reschedule for later */
	TaskManager_ScheduleByTimer(&gDhcpTimer,DHCP_FINE_TIMER_MSECS);
	
	(void)param;/* avoid compiler warning */
	Dhcp_FineTimer();
	if(count>=((DHCP_COARSE_TIMER_SECS*1000)/DHCP_FINE_TIMER_MSECS)) {
		/* call this once per minute */
		Dhcp_CoarseTimer();
		count=0;
	} else {
		count++;
	}
	
}
#ifdef M_WUFEI_081120
extern char g_Opt60ServerName[DHCP_OPTION_60_LEN];	
extern char g_Opt60ClientName[DHCP_OPTION_60_LEN];
#endif
void Dhcp_Initialize(void)
{
	Task_Initialize(&gDhcpTimer,TASK_PRIORITY_TIMER,
		Dhcp_Timer,NULL);
	TaskManager_ScheduleByTimer(&gDhcpTimer,DHCP_FINE_TIMER_MSECS);
#ifdef M_WUFEI_081120
{
	memset(g_Opt60ServerName,0,DHCP_OPTION_60_LEN);
	memset(g_Opt60ClientName,0,DHCP_OPTION_60_LEN);
}
#endif
	
}

/**
 * Back-off the DHCP client (because of a received NAK response).
 *
 * Back-off the DHCP client because of a received NAK. Receiving a
 * NAK means the client asked for something non-sensible, for
 * example when it tries to renew a lease obtained on another network.
 *
 * We back-off and will end up restarting a fresh DHCP negotiation later.
 *
 * @param state pointer to DHCP state structure
 */
static void Dhcp_HandleNak(struct NETWORK_INTERFACE * networkInterface) 
{
	PDHCP_DATA dhcpData;
	u16_t milliSeconds;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	dhcpData = networkInterface->mDhcpData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData!=NULL);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	milliSeconds = 10 * 1000;
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_HandleNak(networkInterface=%p) %s", 
		(void*)networkInterface, networkInterface->mName));
	dhcpData->mRequestTimeOut = (milliSeconds + DHCP_FINE_TIMER_MSECS - 1) / DHCP_FINE_TIMER_MSECS;
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_HandleNak(): set request timeout %"U16_F" milliSeconds", milliSeconds));
	Dhcp_SetState(dhcpData, DHCP_BACKING_OFF);
}

#if DHCP_DOES_ARP_CHECK

void Ethernet_Ipv4Output(struct NETWORK_INTERFACE * networkInterface,PPACKET_BUFFER packet,
	IPV4_ADDRESS destinationIpAddress);
/**
 * Checks if the offered IP address is already in use.
 *
 * It does so by sending an ARP request for the offered address and
 * entering CHECKING state. If no ARP reply is received within a small
 * interval, the address is assumed to be free for use by us.
 */
static void Dhcp_Check(struct NETWORK_INTERFACE * networkInterface)
{
	PDHCP_DATA dhcpData;
	u16_t milliSeconds;
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	
	dhcpData = networkInterface->mDhcpData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData!=NULL);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Check(networkInterface=%p) %s", (void *)networkInterface, networkInterface->mName));
	/* create an ARP query for the offered IP address, expecting that no host
		responds, as the IP address should not be in use. */
	
	Ethernet_Ipv4Output(networkInterface,NULL,dhcpData->mOfferedAddress);
	
	dhcpData->mTryCount++;
	milliSeconds = 500;
	dhcpData->mRequestTimeOut = (milliSeconds + DHCP_FINE_TIMER_MSECS - 1) / DHCP_FINE_TIMER_MSECS;
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Check(): set request timeout %"U16_F" milliSeconds", milliSeconds));
	Dhcp_SetState(dhcpData, DHCP_CHECKING);
}
#endif /* DHCP_DOES_ARP_CHECK */

/**
 * Remember the configuration offered by a DHCP server.
 *
 * @param state pointer to DHCP state structure
 */
static void Dhcp_HandleOffer(struct NETWORK_INTERFACE * networkInterface)
{
	PDHCP_DATA dhcpData;
	u8_t * optionPointer;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	dhcpData = networkInterface->mDhcpData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData!=NULL);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	/* obtain the server address */
	optionPointer = Dhcp_GetOptionPointer(dhcpData, DHCP_OPTION_SERVER_ID);
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_HandleOffer(networkInterface=%p) %s",
		(void*)networkInterface, networkInterface->mName));
	if (optionPointer != NULL)
	{
		dhcpData->mServerAddress = smsc_htonl(Dhcp_GetOptionLong(&optionPointer[2]));
		#if SMSC_TRACE_ENABLED
		{
			char addressString[IPV4_ADDRESS_STRING_SIZE];
			SMSC_TRACE(DHCP_DEBUG, ("Dhcp_HandleOffer(): server %s", 
				IPV4_ADDRESS_TO_STRING(addressString,dhcpData->mServerAddress)));
		}
		#endif
		SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData->mIncomingMessage!=NULL);
		/* remember offered address */
		dhcpData->mOfferedAddress=DHCP_HEADER_GET_YOUR_IP_ADDRESS(dhcpData->mIncomingMessage);
		#if SMSC_TRACE_ENABLED
		{
			char addressString[IPV4_ADDRESS_STRING_SIZE];
			SMSC_TRACE(DHCP_DEBUG, ("Dhcp_HandleOffer(): offer for %s", 
				IPV4_ADDRESS_TO_STRING(addressString,dhcpData->mOfferedAddress)));
		}
		#endif
		Dhcp_Select(networkInterface);
	}
}

/**
 * Select a DHCP server offer out of all offers.
 *
 * Simply select the first offer received.
 *
 * @param networkInterface the networkInterface under DHCP control
 * @return lwIP specific error (see smsc_environment.h)
 */
static err_t Dhcp_Select(struct NETWORK_INTERFACE * networkInterface)
{
	PDHCP_DATA dhcpData;
	err_t result;
	u32_t milliSeconds;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(networkInterface!=NULL,ERR_ARG);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	dhcpData = networkInterface->mDhcpData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(dhcpData!=NULL,ERR_ARG);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Select(networkInterface=%p) %s",
		(void*)networkInterface, networkInterface->mName));

	/* create and initialize the DHCP message header */
	result = Dhcp_CreateRequest(networkInterface);
	if (result == ERR_OK)
	{
		Dhcp_SetOption(dhcpData, DHCP_OPTION_MESSAGE_TYPE, DHCP_OPTION_MESSAGE_TYPE_LEN);
		Dhcp_SetOptionByte(dhcpData, DHCP_REQUEST);

		Dhcp_SetOption(dhcpData, DHCP_OPTION_MAX_MSG_SIZE, DHCP_OPTION_MAX_MSG_SIZE_LEN);
		Dhcp_SetOptionShort(dhcpData, 576);

		/* MUST request the offered IP address */
		Dhcp_SetOption(dhcpData, DHCP_OPTION_REQUESTED_IP, 4);
		Dhcp_SetOptionLong(dhcpData, smsc_ntohl(dhcpData->mOfferedAddress));

		Dhcp_SetOption(dhcpData, DHCP_OPTION_SERVER_ID, 4);
		Dhcp_SetOptionLong(dhcpData, smsc_ntohl(dhcpData->mServerAddress));
#ifdef M_WUFEI_081120
		{
				int i_opLen = 0;
				int i_Loop = 0;
		
				i_opLen = strlen(g_Opt60ClientName);
				Dhcp_SetOption(dhcpData, DHCP_OPTION_60, i_opLen);
				for(i_Loop = 0;i_Loop<i_opLen;i_Loop++)
				{
					if(g_Opt60ClientName[i_Loop] != '\0')
						Dhcp_SetOptionByte(dhcpData, g_Opt60ClientName[i_Loop]);
					else
						break;
				}
				SMSC_NOTICE(DHCP_DEBUG,("DHCP_Disvover: add option60 Client name[%s]\n",g_Opt60ClientName));
		}		
#endif

		Dhcp_SetOption(dhcpData, DHCP_OPTION_PARAMETER_REQUEST_LIST, 4/*num options*/);
		Dhcp_SetOptionByte(dhcpData, DHCP_OPTION_SUBNET_MASK);
		Dhcp_SetOptionByte(dhcpData, DHCP_OPTION_ROUTER);
		Dhcp_SetOptionByte(dhcpData, DHCP_OPTION_BROADCAST);
		Dhcp_SetOptionByte(dhcpData, DHCP_OPTION_DNS_SERVER);

		Dhcp_AddOptionTrailer(dhcpData);
		/* shrink the packet buffer to the actual content length */
		PacketBuffer_ReduceTotalLength(dhcpData->mOutgoingPacket,
			(u32_t)(DHCP_HEADER_LENGTH + ((u32_t)dhcpData->mOutgoingOptionsLength)));

		/* TODO: we really should bind to a specific local interface here
			but we cannot specify an unconfigured networkInterface as it is addressless */
		{
			IP_ADDRESS ipAddress;
			IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&ipAddress,&IPV4_ADDRESS_ANY);
			Udp_Bind(dhcpData->mUdpControlBlock, &ipAddress, DHCP_CLIENT_PORT);
			/* send broadcast to any DHCP server */
			IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&ipAddress,&IPV4_ADDRESS_BROADCAST);
			Udp_Connect(dhcpData->mUdpControlBlock, &ipAddress, DHCP_SERVER_PORT);
			Udp_Send(dhcpData->mUdpControlBlock, dhcpData->mOutgoingPacket);
			/* reconnect to any (or to server here?!) */
			IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&ipAddress,&IPV4_ADDRESS_ANY);
			Udp_Connect(dhcpData->mUdpControlBlock, &ipAddress, DHCP_SERVER_PORT);
		}
		Dhcp_DeleteRequest(networkInterface);
		SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Select: REQUESTING"));
		Dhcp_SetState(dhcpData, DHCP_REQUESTING);
	} else {
		SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Select: could not allocate DHCP request"));
	}
	dhcpData->mTryCount++;
	milliSeconds = ((u32_t)(dhcpData->mTryCount)) < 4 ? ((u32_t)(dhcpData->mTryCount)) * 1000 : 4 * 1000;
	dhcpData->mRequestTimeOut = ((u16_t)((milliSeconds + DHCP_FINE_TIMER_MSECS - 1) / DHCP_FINE_TIMER_MSECS));
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Select(): set request timeout %"U32_F" milliSeconds", milliSeconds));
	return result;
}

/**
 * The DHCP timer that checks for lease renewal/rebind timeouts.
 *
 */
static void Dhcp_CoarseTimer()
{
	struct NETWORK_INTERFACE * networkInterface=gInterfaceList;
	SMSC_TRACE(DHCP_DEBUG, ("DHCP() Coarse Timer Called"));
	/* iterate through all network interfaces */
	while (networkInterface != NULL) {
		/* only act on DHCP configured interfaces */
		if (networkInterface->mDhcpData != NULL) {
			CHECK_SIGNATURE(networkInterface->mDhcpData,DHCP_DATA_SIGNATURE);
			/* timer is active (non zero), and triggers (zeroes) now? */
			if (networkInterface->mDhcpData->mRebindTimeOut-- == 1) {
				SMSC_TRACE(DHCP_DEBUG, ("Dhcp_CoarseTimer(): t2 timeout"));
				/* this clients' rebind timeout triggered */
				Dhcp_RebindTimeOut(networkInterface);
			/* timer is active (non zero), and triggers (zeroes) now */
			} else if (networkInterface->mDhcpData->mRenewTimeOut-- == 1) {
				SMSC_TRACE(DHCP_DEBUG, ("Dhcp_CoarseTimer(): t1 timeout"));
				/* this clients' renewal timeout triggered */
				Dhcp_RenewalTimeOut(networkInterface);
			}
		}
		/* proceed to next networkInterface */
		networkInterface = networkInterface->mNext;
	}
}

/**
 * DHCP transaction timeout handling
 *
 * A DHCP server is expected to respond within a short period of time.
 * This timer checks whether an outstanding DHCP request is timed out.
 * 
 */
static void Dhcp_FineTimer()
{
	struct NETWORK_INTERFACE * networkInterface = gInterfaceList;
	/* loop through networkInterface's */
	while (networkInterface != NULL) {
		/* only act on DHCP configured interfaces */
		if (networkInterface->mDhcpData != NULL) {
			CHECK_SIGNATURE(networkInterface->mDhcpData,DHCP_DATA_SIGNATURE);
			/* timer is active (non zero), and is about to trigger now */      
			if (networkInterface->mDhcpData->mRequestTimeOut > 1) {
				networkInterface->mDhcpData->mRequestTimeOut--;
			}
			else if (networkInterface->mDhcpData->mRequestTimeOut == 1) {
				networkInterface->mDhcpData->mRequestTimeOut--;
				/* { networkInterface->mDhcpData->mRequestTimeOut == 0 } */
				SMSC_TRACE(DHCP_DEBUG, ("Dhcp_FineTimer(): request timeout"));
				/* this clients' request timeout triggered */
				Dhcp_RequestTimeOut(networkInterface);
			}
		}
		/* proceed to next network interface */
		networkInterface = networkInterface->mNext;
	}
}

/**
 * A DHCP negotiation transaction, or ARP request, has timed out.
 *
 * The timer that was started with the DHCP or ARP request has
 * timed out, indicating no response was received in time.
 *
 * @param networkInterface the networkInterface under DHCP control
 *
 */
static void Dhcp_RequestTimeOut(struct NETWORK_INTERFACE * networkInterface)
{
	PDHCP_DATA dhcpData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	dhcpData = networkInterface->mDhcpData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData!=NULL);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	SMSC_TRACE(DHCP_DEBUG , ("Dhcp_RequestTimeOut()"));
	/* back-off period has passed, or server selection timed out */
	if ((dhcpData->mState == DHCP_BACKING_OFF) || (dhcpData->mState == DHCP_SELECTING)) {
		SMSC_TRACE(DHCP_DEBUG, ("Dhcp_RequestTimeOut(): restarting discovery"));
		Dhcp_Discover(networkInterface);
	/* receiving the requested lease timed out */
	} else if (dhcpData->mState == DHCP_REQUESTING) {
		SMSC_TRACE(DHCP_DEBUG, ("Dhcp_RequestTimeOut(): REQUESTING, DHCP request timed out"));
		if (dhcpData->mTryCount <= 5) {
			Dhcp_Select(networkInterface);
		} else {
			SMSC_TRACE(DHCP_DEBUG, ("Dhcp_RequestTimeOut(): REQUESTING, releasing, restarting"));
			Dhcp_Release(networkInterface);
			Dhcp_Discover(networkInterface);
		}
	}
#if DHCP_DOES_ARP_CHECK
	/* received no ARP reply for the offered address (which is good) */
	else if (dhcpData->mState == DHCP_CHECKING) {
		SMSC_TRACE(DHCP_DEBUG, ("Dhcp_RequestTimeOut(): CHECKING, ARP request timed out"));
		if (dhcpData->mTryCount <= 1) {
			Dhcp_Check(networkInterface);
		/* no ARP replies on the offered address,
			looks like the IP address is indeed free */
		} else 
		{
			/* bind the interface to the offered address */
			Dhcp_Bind(networkInterface);
		}
	}
#endif
	/* did not get response to renew request? */
	else if (dhcpData->mState == DHCP_RENEWING) {
		SMSC_TRACE(DHCP_DEBUG, ("Dhcp_RequestTimeOut(): RENEWING, DHCP request timed out"));
		/* just retry renewal */
		/* note that the rebind timer will eventually time-out if renew does not work */
		Dhcp_Renew(networkInterface);
	/* did not get response to rebind request? */
	} else if (dhcpData->mState == DHCP_REBINDING) {
		SMSC_TRACE(DHCP_DEBUG, ("Dhcp_RequestTimeOut(): REBINDING, DHCP request timed out"));
		if (dhcpData->mTryCount <= 8) {
			Dhcp_Rebind(networkInterface);
		} else {
			SMSC_TRACE(DHCP_DEBUG, ("Dhcp_RequestTimeOut(): RELEASING, DISCOVERING"));
			Dhcp_Release(networkInterface);
			Dhcp_Discover(networkInterface);
		}
	} else {
		SMSC_ERROR(("Dhcp_RequestTimeOut(): unknown state"));
	}
}

/**
 * The renewal period has timed out.
 *
 * @param networkInterface the networkInterface under DHCP control
 */
static void Dhcp_RenewalTimeOut(struct NETWORK_INTERFACE * networkInterface)
{
	PDHCP_DATA dhcpData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	dhcpData = networkInterface->mDhcpData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData!=NULL);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_RenewalTimeOut()"));
	if ((dhcpData->mState == DHCP_REQUESTING) || (dhcpData->mState == DHCP_BOUND) || (dhcpData->mState == DHCP_RENEWING)) {
		/* just retry to renew - note that the rebind timer (t2) will
		 * eventually time-out if renew tries fail. */
		SMSC_TRACE(DHCP_DEBUG, ("Dhcp_RenewalTimeOut(): must renew"));
		Dhcp_Renew(networkInterface);
	}
}

/**
 * The rebind period has timed out.
 *
 */
static void Dhcp_RebindTimeOut(struct NETWORK_INTERFACE * networkInterface)
{   
	PDHCP_DATA dhcpData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	dhcpData = networkInterface->mDhcpData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData!=NULL);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_RebindTimeOut()"));
	if ((dhcpData->mState == DHCP_REQUESTING) || (dhcpData->mState == DHCP_BOUND) || (dhcpData->mState == DHCP_RENEWING)) {
		/* just retry to rebind */
		SMSC_TRACE(DHCP_DEBUG, ("Dhcp_RebindTimeOut(): must rebind"));
		Dhcp_Rebind(networkInterface);
	}
}

/**
 *
 * @param networkInterface the networkInterface under DHCP control
 */
static void Dhcp_HandleAck(struct NETWORK_INTERFACE * networkInterface)
{
	PDHCP_DATA dhcpData;
	u8_t *optionPointer;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	dhcpData = networkInterface->mDhcpData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData!=NULL);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	/* clear options we might not get from the ACK */
	dhcpData->mOfferedNetMask = 0;
	dhcpData->mOfferedGateway = 0;
	dhcpData->mOfferedBroadcast = 0;

	/* lease time given? */
	optionPointer = Dhcp_GetOptionPointer(dhcpData, DHCP_OPTION_LEASE_TIME);
	if (optionPointer != NULL) {
		/* remember offered lease time */
		dhcpData->mOfferedLeaseTime = Dhcp_GetOptionLong(optionPointer + 2);
		SMSC_TRACE(DHCP_DEBUG,("Dhcp_HandleAck():lease time[%d]s",dhcpData->mOfferedLeaseTime));
	}
	/* renewal period given? */
	optionPointer = Dhcp_GetOptionPointer(dhcpData, DHCP_OPTION_T1);
	if (optionPointer != NULL) {
		/* remember given renewal period */
		dhcpData->mRecommendedRenewTime = Dhcp_GetOptionLong(optionPointer + 2);
	} else {
		/* calculate safe periods for renewal */
		dhcpData->mRecommendedRenewTime = dhcpData->mOfferedLeaseTime / 2;
	}

	/* renewal period given? */
	optionPointer = Dhcp_GetOptionPointer(dhcpData, DHCP_OPTION_T2);
	if (optionPointer != NULL) {
		/* remember given rebind period */
		dhcpData->mRecommendedRebindTime = Dhcp_GetOptionLong(optionPointer + 2);
	} else {
		/* calculate safe periods for rebinding */
		dhcpData->mRecommendedRebindTime = dhcpData->mOfferedLeaseTime;
	}

	/* (y)our internet address */
	dhcpData->mOfferedAddress=DHCP_HEADER_GET_YOUR_IP_ADDRESS(dhcpData->mIncomingMessage);

/**
 * Patch #1308
 * TODO: we must check if the file field is not overloaded by DHCP options!
 */
#if 0
	/* boot server address */
	dhcpData->mOfferedServerAddress=DHCP_HEADER_GET_NEXT_SERVER_IP_ADDRESS(dhcpData->mIncomingMessage);
	/* boot file name */
	strcpy(dhcpData->mBootFileName, DHCP_HEADER_GET_BOOT_FILE_NAME(dhcpData->mIncomingMessage));
#endif

	/* subnet mask */
	optionPointer = Dhcp_GetOptionPointer(dhcpData, DHCP_OPTION_SUBNET_MASK);
	/* subnet mask given? */
	if (optionPointer != NULL) {
		dhcpData->mOfferedNetMask = smsc_htonl(Dhcp_GetOptionLong(&optionPointer[2]));
	}

	/* gateway router */
	optionPointer = Dhcp_GetOptionPointer(dhcpData, DHCP_OPTION_ROUTER);
	if (optionPointer != NULL) {
		dhcpData->mOfferedGateway = smsc_htonl(Dhcp_GetOptionLong(&optionPointer[2]));
	}

	/* broadcast address */
	optionPointer = Dhcp_GetOptionPointer(dhcpData, DHCP_OPTION_BROADCAST);
	if (optionPointer != NULL) {
		dhcpData->mOfferedBroadcast = smsc_htonl(Dhcp_GetOptionLong(&optionPointer[2]));
	}
  
	/* DNS servers */
	optionPointer = Dhcp_GetOptionPointer(dhcpData, DHCP_OPTION_DNS_SERVER);
	if (optionPointer != NULL) {
		u8_t n;
		dhcpData->mDnsCount = Dhcp_GetOptionByte(&optionPointer[1]) / (u32_t)sizeof(IPV4_ADDRESS);
		/* limit to at most DHCP_MAX_DNS DNS servers */
		if (dhcpData->mDnsCount > DHCP_MAX_DNS)
			dhcpData->mDnsCount = DHCP_MAX_DNS;
		for (n = 0; n < dhcpData->mDnsCount; n++) {
			dhcpData->mOfferedDns[n] = smsc_htonl(Dhcp_GetOptionLong(&optionPointer[2 + n * 4]));
		}
	}
}

/**
 * Start DHCP negotiation for a network interface.
 *
 * If no DHCP client instance was attached to this interface,
 * a new client is created first. If a DHCP client instance
 * was already present, it restarts negotiation.
 *
 * @param networkInterface The lwIP network interface
 * @return lwIP error code
 * - ERR_OK - No error
 * - ERR_MEM - Out of memory
 *
 */
err_t Dhcp_Start(struct NETWORK_INTERFACE * networkInterface,PDHCP_DATA dhcpData)
{
	err_t result = ERR_OK;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(networkInterface!=NULL,ERR_ARG);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(dhcpData!=NULL,ERR_ARG);

	SMSC_FUNCTION_PARAMS_CHECK_RETURN(networkInterface != NULL,ERR_ARG);
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Start(networkInterface=%p) %s", 
		(void*)networkInterface, networkInterface->mName));
	networkInterface->mFlags &= ~NETWORK_INTERFACE_FLAG_DHCP;

	networkInterface->mDhcpData=dhcpData;

	/* clear data structure */
	memset(dhcpData, 0, sizeof(DHCP_DATA));
	ASSIGN_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	/* allocate UDP PCB */
	dhcpData->mUdpControlBlock = Udp_CreateControlBlock();
	if (dhcpData->mUdpControlBlock == NULL) {
		SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Start(): could not obtain mUdpControlBlock"));
		networkInterface->mDhcpData = NULL;
		return ERR_MEM;
	}
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Start(): starting DHCP configuration"));
	/* (re)start the DHCP negotiation */
	result = Dhcp_Discover(networkInterface);
	if (result != ERR_OK) {
		/* free resources allocated above */
		Dhcp_Stop(networkInterface);
		return ERR_MEM;
	}
	networkInterface->mFlags |= NETWORK_INTERFACE_FLAG_DHCP;
	return result;
}

/**
 * Inform a DHCP server of our manual configuration.
 *
 * This informs DHCP servers of our fixed IP address configuration
 * by sending an INFORM message. It does not involve DHCP address
 * configuration, it is just here to be nice to the network.
 *
 * @param networkInterface The lwIP network interface
 *
 */
static DHCP_DATA gDhcpInformData;
void Dhcp_Inform(struct NETWORK_INTERFACE * networkInterface)
{
	err_t result = ERR_OK;
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	
	networkInterface->mDhcpData = &gDhcpInformData;
	memset(&gDhcpInformData, 0, sizeof(DHCP_DATA));
	ASSIGN_SIGNATURE(&gDhcpInformData,DHCP_DATA_SIGNATURE);

	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Inform(): allocated dhcpData"));
	gDhcpInformData.mUdpControlBlock = Udp_CreateControlBlock();
	if (gDhcpInformData.mUdpControlBlock == NULL) {
		SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Inform(): could not obtain mUdpControlBlock"));
		networkInterface->mDhcpData=NULL;
		return;
	}
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Inform(): created new mUdpControlBlock"));
	/* create and initialize the DHCP message header */
	result = Dhcp_CreateRequest(networkInterface);
	if (result == ERR_OK) {
		Dhcp_SetOption(&gDhcpInformData, DHCP_OPTION_MESSAGE_TYPE, DHCP_OPTION_MESSAGE_TYPE_LEN);
		Dhcp_SetOptionByte(&gDhcpInformData, DHCP_INFORM);

		Dhcp_SetOption(&gDhcpInformData, DHCP_OPTION_MAX_MSG_SIZE, DHCP_OPTION_MAX_MSG_SIZE_LEN);
		/* TODO: use networkInterface->mtu ?! */
		Dhcp_SetOptionShort(&gDhcpInformData, 576);

		Dhcp_AddOptionTrailer(&gDhcpInformData);

		PacketBuffer_ReduceTotalLength(gDhcpInformData.mOutgoingPacket, 
			DHCP_HEADER_LENGTH + ((u32_t)gDhcpInformData.mOutgoingOptionsLength));

		{
			IP_ADDRESS ipAddress;
			
			IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&ipAddress,&IPV4_ADDRESS_ANY);
			Udp_Bind(gDhcpInformData.mUdpControlBlock, &ipAddress, DHCP_CLIENT_PORT);
			
			IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&ipAddress,&IPV4_ADDRESS_BROADCAST);
			Udp_Connect(gDhcpInformData.mUdpControlBlock, &ipAddress, DHCP_SERVER_PORT);
			
			SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Inform: INFORMING"));
			Udp_Send(gDhcpInformData.mUdpControlBlock, gDhcpInformData.mOutgoingPacket);
			
			IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&ipAddress,&IPV4_ADDRESS_ANY);
			Udp_Connect(gDhcpInformData.mUdpControlBlock, &ipAddress, DHCP_SERVER_PORT);
		}
		Dhcp_DeleteRequest(networkInterface);
	} else {
		SMSC_NOTICE(DHCP_DEBUG, ("Dhcp_Inform: could not allocate DHCP request"));
	}

	if (gDhcpInformData.mUdpControlBlock != NULL) {
		Udp_UnBind(gDhcpInformData.mUdpControlBlock);
		Udp_FreeControlBlock(gDhcpInformData.mUdpControlBlock);
	}
	gDhcpInformData.mUdpControlBlock = NULL;
	ASSIGN_SIGNATURE(&gDhcpInformData,0);
	networkInterface->mDhcpData = NULL;
}

#if DHCP_DOES_ARP_CHECK
/**
 * Match an ARP reply with the offered IP address.
 *
 * @param addr The IP address we received a reply from
 *
 */
void Dhcp_ArpReply(struct NETWORK_INTERFACE * networkInterface, PIPV4_ADDRESS ipAddress)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(ipAddress!=NULL);
	
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_ArpReply()"));
	/* is a DHCP client doing an ARP check? */
	if ((networkInterface->mDhcpData != NULL) && (networkInterface->mDhcpData->mState == DHCP_CHECKING)) {
		CHECK_SIGNATURE(networkInterface->mDhcpData,DHCP_DATA_SIGNATURE);
		#if SMSC_TRACE_ENABLED
		{
			char addressString[IPV4_ADDRESS_STRING_SIZE];
			SMSC_TRACE(DHCP_DEBUG, ("Dhcp_ArpReply(): CHECKING, arp reply for %s",
				IPV4_ADDRESS_TO_STRING(addressString,*ipAddress)));
		}
		#endif
		/* did a host respond with the address we
			were offered by the DHCP server? */
		if (IPV4_ADDRESS_COMPARE(*ipAddress, networkInterface->mDhcpData->mOfferedAddress)) 
		{
			/* we will not accept the offered address */
			SMSC_TRACE(DHCP_DEBUG, ("Dhcp_ArpReply(): arp reply matched with offered address, declining"));
			Dhcp_Decline(networkInterface);
		}
	}
}

/**
 * Decline an offered lease.
 *
 * Tell the DHCP server we do not accept the offered address.
 * One reason to decline the lease is when we find out the address
 * is already in use by another host (through ARP).
 */
static err_t Dhcp_Decline(struct NETWORK_INTERFACE * networkInterface)
{
	PDHCP_DATA dhcpData;
	err_t result = ERR_OK;
	u16_t milliSeconds;
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(networkInterface!=NULL,ERR_ARG);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	
	dhcpData = networkInterface->mDhcpData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(dhcpData!=NULL,ERR_ARG);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Decline()"));
	Dhcp_SetState(dhcpData, DHCP_BACKING_OFF);
	/* create and initialize the DHCP message header */
	result = Dhcp_CreateRequest(networkInterface);
	if (result == ERR_OK)
	{
		Dhcp_SetOption(dhcpData, DHCP_OPTION_MESSAGE_TYPE, DHCP_OPTION_MESSAGE_TYPE_LEN);
		Dhcp_SetOptionByte(dhcpData, DHCP_DECLINE);

		Dhcp_SetOption(dhcpData, DHCP_OPTION_MAX_MSG_SIZE, DHCP_OPTION_MAX_MSG_SIZE_LEN);
		Dhcp_SetOptionShort(dhcpData, 576);

		Dhcp_SetOption(dhcpData, DHCP_OPTION_REQUESTED_IP, 4);
		Dhcp_SetOptionLong(dhcpData, smsc_ntohl(dhcpData->mOfferedAddress));
              /*20081231 add IP ³åÍ»ÐÞ¸Ä*/
	       Dhcp_SetOption(dhcpData, DHCP_OPTION_SERVER_ID, 4);
		Dhcp_SetOptionLong(dhcpData, smsc_ntohl(dhcpData->mServerAddress));
		/*************/
		Dhcp_AddOptionTrailer(dhcpData);
		/* resize pbuf to reflect true size of options */
		PacketBuffer_ReduceTotalLength(dhcpData->mOutgoingPacket, 
			DHCP_HEADER_LENGTH + (u32_t)(dhcpData->mOutgoingOptionsLength));
        
        {
        	IP_ADDRESS ipAddress;
        	IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&ipAddress,&IPV4_ADDRESS_ANY);
			Udp_Bind(dhcpData->mUdpControlBlock, &ipAddress, DHCP_CLIENT_PORT);
			/* @todo: should we really connect here? we are performing sendto() */
			Udp_Connect(dhcpData->mUdpControlBlock, &ipAddress, DHCP_SERVER_PORT);
			
			IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&ipAddress, &IPV4_ADDRESS_BROADCAST);
			/* per section 4.4.4, broadcast DECLINE messages */
			Udp_SendTo(dhcpData->mUdpControlBlock, dhcpData->mOutgoingPacket, &ipAddress, DHCP_SERVER_PORT);
		}		
		Dhcp_DeleteRequest(networkInterface);
		SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Decline: BACKING OFF"));
	} else {
		SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Decline: could not allocate DHCP request"));
	}
	dhcpData->mTryCount++;

    /*20081231 add and change IP ³åÍ»ÐÞ¸Ä*/
    Ipv4_SetAddresses(networkInterface,
		IPV4_ADDRESS_MAKE(0,0,0,0),
		IPV4_ADDRESS_MAKE(0,0,0,0),
		IPV4_ADDRESS_MAKE(0,0,0,0));
	
	milliSeconds = /*10**/1000;
   /*************************/
	dhcpData->mRequestTimeOut = (milliSeconds + DHCP_FINE_TIMER_MSECS - 1) / DHCP_FINE_TIMER_MSECS;
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Decline(): set request timeout %"U16_F" milliSeconds", milliSeconds));
	return result;
}
#endif /*DHCP_DOES_ARP_CHECK*/


/**
 * Start the DHCP process, discover a DHCP server.
 *
 */
static err_t Dhcp_Discover(struct NETWORK_INTERFACE * networkInterface)
{
	PDHCP_DATA dhcpData;
	err_t result = ERR_OK;
	u16_t milliSeconds;
		
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(networkInterface!=NULL,ERR_ARG);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	
	dhcpData = networkInterface->mDhcpData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(dhcpData!=NULL,ERR_ARG);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Discover()"));
	
	dhcpData->mOfferedAddress=IPV4_ADDRESS_ANY;
	
	/* create and initialize the DHCP message header */
	result = Dhcp_CreateRequest(networkInterface);
	if (result == ERR_OK)
	{
		SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Discover: making request"));
		Dhcp_SetOption(dhcpData, DHCP_OPTION_MESSAGE_TYPE, DHCP_OPTION_MESSAGE_TYPE_LEN);
		Dhcp_SetOptionByte(dhcpData, DHCP_DISCOVER);

		Dhcp_SetOption(dhcpData, DHCP_OPTION_MAX_MSG_SIZE, DHCP_OPTION_MAX_MSG_SIZE_LEN);
		Dhcp_SetOptionShort(dhcpData, 576);

		Dhcp_SetOption(dhcpData, DHCP_OPTION_PARAMETER_REQUEST_LIST, 4/*num options*/);
		Dhcp_SetOptionByte(dhcpData, DHCP_OPTION_SUBNET_MASK);
		Dhcp_SetOptionByte(dhcpData, DHCP_OPTION_ROUTER);
		Dhcp_SetOptionByte(dhcpData, DHCP_OPTION_BROADCAST);
		Dhcp_SetOptionByte(dhcpData, DHCP_OPTION_DNS_SERVER);
#ifdef M_WUFEI_081120
		{
				int i_opLen = 0;
				int i_Loop = 0;
		
				i_opLen = strlen(g_Opt60ClientName);
				Dhcp_SetOption(dhcpData, DHCP_OPTION_60, i_opLen);
				for(i_Loop = 0;i_Loop<i_opLen;i_Loop++)
				{
					if(g_Opt60ClientName[i_Loop] != '\0')
						Dhcp_SetOptionByte(dhcpData, g_Opt60ClientName[i_Loop]);
					else
						break;
				}
				SMSC_NOTICE(DHCP_DEBUG,("DHCP_Disvover: add option60 Client name[%s]\n",g_Opt60ClientName));
		}		
#endif
		Dhcp_AddOptionTrailer(dhcpData);

		PacketBuffer_ReduceTotalLength(dhcpData->mOutgoingPacket, 
			DHCP_HEADER_LENGTH + ((u32_t)(dhcpData->mOutgoingOptionsLength)));

		/* set receive callback function with networkInterface as user data */
		Udp_SetReceiverCallBack(dhcpData->mUdpControlBlock, Dhcp_Receive, networkInterface);
		{
			IPV4_ADDRESS ipAddress;
			IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&ipAddress,&IPV4_ADDRESS_ANY);
			Udp_Bind(dhcpData->mUdpControlBlock, &ipAddress, DHCP_CLIENT_PORT);
			Udp_Connect(dhcpData->mUdpControlBlock, &ipAddress, DHCP_SERVER_PORT);
			SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Discover: sendto(DISCOVER, IP_ADDR_BROADCAST, DHCP_SERVER_PORT)"));
			
			IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&ipAddress,&IPV4_ADDRESS_BROADCAST);
			Udp_SendTo(dhcpData->mUdpControlBlock, dhcpData->mOutgoingPacket,&ipAddress,DHCP_SERVER_PORT);
		}
		Dhcp_DeleteRequest(networkInterface);
		SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Discover: SELECTING"));
		Dhcp_SetState(dhcpData, DHCP_SELECTING);
	} else {
		SMSC_NOTICE(DHCP_DEBUG, ("Dhcp_Discover: could not allocate DHCP request"));
	}
	dhcpData->mTryCount++;
	milliSeconds = dhcpData->mTryCount < 4 ? (dhcpData->mTryCount + 1) * 1000 : 10 * 1000;
	dhcpData->mRequestTimeOut = (milliSeconds + DHCP_FINE_TIMER_MSECS - 1) / DHCP_FINE_TIMER_MSECS;
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Discover(): set request timeout %"U16_F" milliSeconds", milliSeconds));
	return result;
}


/**
 * Bind the interface to the offered IP address.
 *
 * @param networkInterface network interface to bind to the offered address
 */
static void Dhcp_Bind(struct NETWORK_INTERFACE * networkInterface)
{
	PDHCP_DATA dhcpData;
	IPV4_ADDRESS subNetMask, gatewayAddress;
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	
	dhcpData = networkInterface->mDhcpData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData!=NULL);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	/*SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Bind(networkInterface=%p) %s",
		(void*)networkInterface, networkInterface->mName));*/

	/* temporary DHCP lease? */
	if (dhcpData->mRecommendedRenewTime != 0xffffffffUL) {
		/* set renewal period timer */
		/*SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Bind(): t1 renewal timer %"U32_F" secs", dhcpData->mRecommendedRenewTime));*/
		dhcpData->mRenewTimeOut = ((u16_t)((dhcpData->mRecommendedRenewTime + DHCP_COARSE_TIMER_SECS / 2) / DHCP_COARSE_TIMER_SECS));
		if (dhcpData->mRenewTimeOut == 0)
			dhcpData->mRenewTimeOut = 1;
		/*SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Bind(): set request timeout %"U32_F" milliSeconds", dhcpData->mRecommendedRenewTime*1000));*/
	}
	/* set renewal period timer */
	if (dhcpData->mRecommendedRebindTime != 0xffffffffUL) {
		/*SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Bind(): t2 rebind timer %"U32_F" secs", dhcpData->mRecommendedRebindTime));*/
		dhcpData->mRebindTimeOut = ((u16_t)((dhcpData->mRecommendedRebindTime + DHCP_COARSE_TIMER_SECS / 2) / DHCP_COARSE_TIMER_SECS));
		if (dhcpData->mRebindTimeOut == 0)
			dhcpData->mRebindTimeOut = 1;
		/*SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Bind(): set request timeout %"U32_F" milliSeconds", dhcpData->mRecommendedRebindTime*1000));*/
	}
	/* copy offered network mask */
	subNetMask=dhcpData->mOfferedNetMask;

	/* subnet mask not given? */
	/* TODO: this is not a valid check. what if the network mask is 0? */
	if (subNetMask == 0) {
		/* choose a safe subnet mask given the network class */
		u8_t first_octet = IPV4_ADDRESS_GET_BYTE_1(dhcpData->mOfferedAddress);
		if (first_octet <= 127)
			subNetMask = smsc_htonl(0xff000000);
		else if (first_octet >= 192)
			subNetMask = smsc_htonl(0xffffff00);
		else
			subNetMask = smsc_htonl(0xffff0000);
	}

	gatewayAddress=dhcpData->mOfferedGateway;
	/* gateway address not given? */
	if (gatewayAddress == 0) {
		/* copy network address */
		gatewayAddress = (dhcpData->mOfferedAddress & subNetMask);
		/* use first host address on network as gateway */
		gatewayAddress |= smsc_htonl(0x00000001);
	}

	#if SMSC_TRACE_ENABLED
	{
		char addressString[IPV4_ADDRESS_STRING_SIZE];
		SMSC_TRACE(DHCP_DEBUG, ("DHCP(%s) Binding IP address to %s",
			networkInterface->mName,
			IPV4_ADDRESS_TO_STRING(addressString,dhcpData->mOfferedAddress)));
		SMSC_TRACE(DHCP_DEBUG, ("DHCP(%s) Binding Sub Net to %s",
			networkInterface->mName,
			IPV4_ADDRESS_TO_STRING(addressString, subNetMask)));
		SMSC_TRACE(DHCP_DEBUG, ("DHCP(%s) Binding Gateway to %s",
			networkInterface->mName,
			IPV4_ADDRESS_TO_STRING(addressString, gatewayAddress)));
	}
	#endif
	Ipv4_SetAddresses(networkInterface,
		dhcpData->mOfferedAddress,
		subNetMask,gatewayAddress);
	/* bring the interface up */
	NetworkInterface_SetUp(networkInterface);
	/* networkInterface is now bound to DHCP leased address */
	Dhcp_SetState(dhcpData, DHCP_BOUND);
}

/**
 * Renew an existing DHCP lease at the involved DHCP server.
 *
 * @param networkInterface network interface which must renew its lease
 */
err_t Dhcp_Renew(struct NETWORK_INTERFACE * networkInterface)
{
	PDHCP_DATA dhcpData;
	err_t result;
	u16_t milliSeconds;
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(networkInterface!=NULL,ERR_ARG);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	
	dhcpData = networkInterface->mDhcpData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(dhcpData!=NULL,ERR_ARG);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Renew()"));
	Dhcp_SetState(dhcpData, DHCP_RENEWING);

	/* create and initialize the DHCP message header */
	result = Dhcp_CreateRequest(networkInterface);
	if (result == ERR_OK) {
		Dhcp_SetOption(dhcpData, DHCP_OPTION_MESSAGE_TYPE, DHCP_OPTION_MESSAGE_TYPE_LEN);
		Dhcp_SetOptionByte(dhcpData, DHCP_REQUEST);

		Dhcp_SetOption(dhcpData, DHCP_OPTION_MAX_MSG_SIZE, DHCP_OPTION_MAX_MSG_SIZE_LEN);
		/* TODO: use networkInterface->mtu in some way */
		Dhcp_SetOptionShort(dhcpData, 576);
#ifdef M_WUFEI_081120
		{
				int i_opLen = 0;
				int i_Loop = 0;
		
				i_opLen = strlen(g_Opt60ClientName);
				Dhcp_SetOption(dhcpData, DHCP_OPTION_60, i_opLen);
				for(i_Loop = 0;i_Loop<i_opLen;i_Loop++)
				{
					if(g_Opt60ClientName[i_Loop] != '\0')
						Dhcp_SetOptionByte(dhcpData, g_Opt60ClientName[i_Loop]);
					else
						break;
				}
				SMSC_NOTICE(DHCP_DEBUG,("DHCP_Disvover: add option60 Client name[%s]\n",g_Opt60ClientName));
		}		
#endif

#if 0
		Dhcp_SetOption(dhcpData, DHCP_OPTION_REQUESTED_IP, 4);
		Dhcp_SetOptionLong(dhcpData, smsc_ntohl(dhcpData->mOfferedAddress));
#endif

#if 0
		Dhcp_SetOption(dhcpData, DHCP_OPTION_SERVER_ID, 4);
		Dhcp_SetOptionLong(dhcpData, smsc_ntohl(dhcpData->mServerAddress));
#endif
		/* append DHCP message trailer */
		Dhcp_AddOptionTrailer(dhcpData);

		PacketBuffer_ReduceTotalLength(dhcpData->mOutgoingPacket,
			DHCP_HEADER_LENGTH + ((u32_t)(dhcpData->mOutgoingOptionsLength)));

		{
			IP_ADDRESS ipAddress;
			IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&ipAddress,&IPV4_ADDRESS_ANY);
			Udp_Bind(dhcpData->mUdpControlBlock, &ipAddress, DHCP_CLIENT_PORT);
			Udp_Connect(dhcpData->mUdpControlBlock, &dhcpData->mServerAddress, DHCP_SERVER_PORT);
			Udp_Send(dhcpData->mUdpControlBlock, dhcpData->mOutgoingPacket);
		}
		Dhcp_DeleteRequest(networkInterface);

		SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Renew: RENEWING"));
	} else {
		SMSC_NOTICE(DHCP_DEBUG, ("Dhcp_Renew: could not allocate DHCP request"));
	}
	dhcpData->mTryCount++;
	/* back-off on retries, but to a maximum of 20 seconds */
	milliSeconds = dhcpData->mTryCount < 10 ? dhcpData->mTryCount * 2000 : 20 * 1000;
	dhcpData->mRequestTimeOut = (milliSeconds + DHCP_FINE_TIMER_MSECS - 1) / DHCP_FINE_TIMER_MSECS;
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Renew(): set request timeout %"U16_F" milliSeconds", milliSeconds));
	return result;
}

/**
 * Rebind with a DHCP server for an existing DHCP lease.
 *
 * @param networkInterface network interface which must rebind with a DHCP server
 */
static err_t Dhcp_Rebind(struct NETWORK_INTERFACE * networkInterface)
{
	PDHCP_DATA dhcpData;
	err_t result;
	u16_t milliSeconds;
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(networkInterface!=NULL,ERR_ARG);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	
	dhcpData = networkInterface->mDhcpData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(dhcpData!=NULL,ERR_ARG);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Rebind()"));
	Dhcp_SetState(dhcpData, DHCP_REBINDING);

	/* create and initialize the DHCP message header */
	result = Dhcp_CreateRequest(networkInterface);
	if (result == ERR_OK)
	{
		Dhcp_SetOption(dhcpData, DHCP_OPTION_MESSAGE_TYPE, DHCP_OPTION_MESSAGE_TYPE_LEN);
		Dhcp_SetOptionByte(dhcpData, DHCP_REQUEST);

		Dhcp_SetOption(dhcpData, DHCP_OPTION_MAX_MSG_SIZE, DHCP_OPTION_MAX_MSG_SIZE_LEN);
		Dhcp_SetOptionShort(dhcpData, 576);
#ifdef M_WUFEI_081120
		{
				int i_opLen = 0;
				int i_Loop = 0;
		
				i_opLen = strlen(g_Opt60ClientName);
				Dhcp_SetOption(dhcpData, DHCP_OPTION_60, i_opLen);
				for(i_Loop = 0;i_Loop<i_opLen;i_Loop++)
				{
					if(g_Opt60ClientName[i_Loop] != '\0')
						Dhcp_SetOptionByte(dhcpData, g_Opt60ClientName[i_Loop]);
					else
						break;
				}
				SMSC_NOTICE(DHCP_DEBUG,("DHCP_Disvover: add option60 Client name[%s]\n",g_Opt60ClientName));
		}		
#endif

#if 0
		Dhcp_SetOption(dhcpData, DHCP_OPTION_REQUESTED_IP, 4);
		Dhcp_SetOptionLong(dhcpData, smsc_ntohl(dhcpData->mOfferedAddress));

		Dhcp_SetOption(dhcpData, DHCP_OPTION_SERVER_ID, 4);
		Dhcp_SetOptionLong(dhcpData, smsc_ntohl(dhcpData->mServerAddress));
#endif

		Dhcp_AddOptionTrailer(dhcpData);
		
		PacketBuffer_ReduceTotalLength(dhcpData->mOutgoingPacket,
			DHCP_HEADER_LENGTH + ((u32_t)(dhcpData->mOutgoingOptionsLength)));

		{
			IP_ADDRESS ipAddress;
			
			/* set remote IP association to any DHCP server */
			IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&ipAddress,&IPV4_ADDRESS_ANY);
			Udp_Bind(dhcpData->mUdpControlBlock, &ipAddress, DHCP_CLIENT_PORT);
			Udp_Connect(dhcpData->mUdpControlBlock, &ipAddress, DHCP_SERVER_PORT);
			
			/* broadcast to server */			
			IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&ipAddress,&IPV4_ADDRESS_BROADCAST);
			Udp_SendTo(dhcpData->mUdpControlBlock, dhcpData->mOutgoingPacket, &ipAddress, DHCP_SERVER_PORT);
		}
		Dhcp_DeleteRequest(networkInterface);
		SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Rebind: REBINDING"));
	} else {
		SMSC_NOTICE(DHCP_DEBUG, ("Dhcp_Rebind: could not allocate DHCP request"));
	}
	dhcpData->mTryCount++;
	milliSeconds = dhcpData->mTryCount < 10 ? dhcpData->mTryCount * 1000 : 10 * 1000;
	dhcpData->mRequestTimeOut = (milliSeconds + DHCP_FINE_TIMER_MSECS - 1) / DHCP_FINE_TIMER_MSECS;
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Rebind(): set request timeout %"U16_F" milliSeconds", milliSeconds));
	return result;
}

/**
 * Release a DHCP lease.
 *
 * @param networkInterface network interface which must release its lease
 */
err_t Dhcp_Release(struct NETWORK_INTERFACE * networkInterface)
{
	PDHCP_DATA dhcpData;
	err_t result;
	u16_t milliSeconds;
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(networkInterface!=NULL,ERR_ARG);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	
	dhcpData = networkInterface->mDhcpData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(dhcpData!=NULL,ERR_ARG);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Release()"));

	/* idle DHCP client */
	Dhcp_SetState(dhcpData, DHCP_OFF);
	/* clean old DHCP offer */
#ifndef M_WUFEI_080825
	dhcpData->mServerAddress = 0;
#else
	dhcpData->mServerAddress = IPV4_ADDRESS_BROADCAST/*0*/;
#endif

	dhcpData->mOfferedAddress = dhcpData->mOfferedNetMask = 0;
	dhcpData->mOfferedGateway = dhcpData->mOfferedBroadcast = 0;
	dhcpData->mOfferedLeaseTime = dhcpData->mRecommendedRenewTime = dhcpData->mRecommendedRebindTime = 0;
	dhcpData->mDnsCount = 0;

	/* create and initialize the DHCP message header */
	result = Dhcp_CreateRequest(networkInterface);
	if (result == ERR_OK) {
		Dhcp_SetOption(dhcpData, DHCP_OPTION_MESSAGE_TYPE, DHCP_OPTION_MESSAGE_TYPE_LEN);
		Dhcp_SetOptionByte(dhcpData, DHCP_RELEASE);

		Dhcp_AddOptionTrailer(dhcpData);

		PacketBuffer_ReduceTotalLength(dhcpData->mOutgoingPacket,
			DHCP_HEADER_LENGTH + ((u32_t)(dhcpData->mOutgoingOptionsLength)));

		{
			IP_ADDRESS ipAddress;
			IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&ipAddress,&IPV4_ADDRESS_ANY);
			Udp_Bind(dhcpData->mUdpControlBlock, &ipAddress, DHCP_CLIENT_PORT);
			IP_ADDRESS_COPY_FROM_IPV4_ADDRESS(&ipAddress,&(dhcpData->mServerAddress));
			Udp_Connect(dhcpData->mUdpControlBlock, &ipAddress, DHCP_SERVER_PORT);
			Udp_Send(dhcpData->mUdpControlBlock, dhcpData->mOutgoingPacket);
		}
		Dhcp_DeleteRequest(networkInterface);
		SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Release: RELEASED, DHCP_OFF"));
	} else {
		SMSC_NOTICE(DHCP_DEBUG, ("Dhcp_Release: could not allocate DHCP request"));
	}
	dhcpData->mTryCount++;
	milliSeconds = dhcpData->mTryCount < 10 ? dhcpData->mTryCount * 1000 : 10 * 1000;
	dhcpData->mRequestTimeOut = (milliSeconds + DHCP_FINE_TIMER_MSECS - 1) / DHCP_FINE_TIMER_MSECS;
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Release(): set request timeout %"U16_F" milliSeconds", milliSeconds));
	
	/* bring the interface down */
	NetworkInterface_SetDown(networkInterface);
	
	/* remove IP address from interface */
	Ipv4_SetAddresses(networkInterface,
		IPV4_ADDRESS_ANY,
		IPV4_ADDRESS_ANY,
		IPV4_ADDRESS_ANY);

	return result;
}
/**
 * Remove the DHCP client from the interface.
 *
 * @param networkInterface The network interface to stop DHCP on
 */
void Dhcp_Stop(struct NETWORK_INTERFACE * networkInterface)
{
	PDHCP_DATA dhcpData;
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	
	dhcpData = networkInterface->mDhcpData;

	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Stop()"));
	/* networkInterface is DHCP configured? */
	if (dhcpData != NULL)
	{
		CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
		if (dhcpData->mUdpControlBlock != NULL)
		{
			Udp_UnBind(dhcpData->mUdpControlBlock);
			Udp_FreeControlBlock(dhcpData->mUdpControlBlock);
			dhcpData->mUdpControlBlock = NULL;
		}
		if (dhcpData->mIncomingPacket != NULL)
		{
			PacketBuffer_DecreaseReference(dhcpData->mIncomingPacket);
			dhcpData->mIncomingPacket = NULL;
		}
		/* free unfolded reply */
		Dhcp_FreeReply(dhcpData);
		ASSIGN_SIGNATURE(dhcpData,0);
		networkInterface->mDhcpData = NULL;
		networkInterface->mFlags &= ~NETWORK_INTERFACE_FLAG_DHCP;/*wufei add 2009-1-10*/
	}
}

/*
 * Set the DHCP state of a DHCP client.
 *
 * If the state changed, reset the number of tries.
 *
 * TODO: we might also want to reset the timeout here?
 */
static void Dhcp_SetState(PDHCP_DATA dhcpData, u8_t new_state)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData!=NULL);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	if (new_state != dhcpData->mState)
	{
		dhcpData->mState = new_state;
		dhcpData->mTryCount = 0;
	}
}

/*
 * Concatenate an option type and length field to the outgoing
 * DHCP message.
 *
 */
static void Dhcp_SetOption(PDHCP_DATA dhcpData, u8_t option_type, u8_t option_len)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData!=NULL);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData->mOutgoingOptionsLength + 2 + option_len <= DHCP_OPTIONS_LENGTH);
	DHCP_HEADER_GET_OPTIONS(dhcpData->mOutgoingMessage)[dhcpData->mOutgoingOptionsLength++] = option_type;
	DHCP_HEADER_GET_OPTIONS(dhcpData->mOutgoingMessage)[dhcpData->mOutgoingOptionsLength++] = option_len;
}
/*
 * Concatenate a single byte to the outgoing DHCP message.
 *
 */
static void Dhcp_SetOptionByte(PDHCP_DATA dhcpData, u8_t value)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData!=NULL);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData->mOutgoingOptionsLength < DHCP_OPTIONS_LENGTH);
	DHCP_HEADER_GET_OPTIONS(dhcpData->mOutgoingMessage)[dhcpData->mOutgoingOptionsLength++] = value;
}
static void Dhcp_SetOptionShort(PDHCP_DATA dhcpData, u16_t value)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData!=NULL);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData->mOutgoingOptionsLength + 2 <= DHCP_OPTIONS_LENGTH);
	DHCP_HEADER_GET_OPTIONS(dhcpData->mOutgoingMessage)[dhcpData->mOutgoingOptionsLength++] = (value & 0xff00U) >> 8;
	DHCP_HEADER_GET_OPTIONS(dhcpData->mOutgoingMessage)[dhcpData->mOutgoingOptionsLength++] =  value & 0x00ffU;
}
static void Dhcp_SetOptionLong(PDHCP_DATA dhcpData, u32_t value)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData!=NULL);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData->mOutgoingOptionsLength + 4 <= DHCP_OPTIONS_LENGTH);
	DHCP_HEADER_GET_OPTIONS(dhcpData->mOutgoingMessage)[dhcpData->mOutgoingOptionsLength++] = ((u8_t)((value & 0xff000000UL) >> 24));
	DHCP_HEADER_GET_OPTIONS(dhcpData->mOutgoingMessage)[dhcpData->mOutgoingOptionsLength++] = ((u8_t)((value & 0x00ff0000UL) >> 16));
	DHCP_HEADER_GET_OPTIONS(dhcpData->mOutgoingMessage)[dhcpData->mOutgoingOptionsLength++] = ((u8_t)((value & 0x0000ff00UL) >> 8));
	DHCP_HEADER_GET_OPTIONS(dhcpData->mOutgoingMessage)[dhcpData->mOutgoingOptionsLength++] = ((u8_t)((value & 0x000000ffUL)));
}

/**
 * Extract the DHCP message and the DHCP options.
 *
 * Extract the DHCP message and the DHCP options, each into a contiguous
 * piece of memory. As a DHCP message is variable sized by its options,
 * and also allows overriding some fields for options, the easy approach
 * is to first unfold the options into a conitguous piece of memory, and
 * use that further on.
 *
 */
static err_t Dhcp_UnfoldReply(PDHCP_DATA dhcpData)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(dhcpData!=NULL,ERR_ARG);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	/* NOTE: The lwip stack previously copied the mIncomingMessage, and mIncomingOptions
	  into a malloc'd memory space to insure that the data is in a contiguous memory buffer.
	  The new packet buffers will always be big enough to hold a DHCP packet so there
	  is no need to do a malloc and copy. Instead I just set the pointers to the proper place
	  in the original packet. */
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(dhcpData->mIncomingPacket != NULL,ERR_ARG);
	/* free any left-overs from previous unfolds */
	Dhcp_FreeReply(dhcpData);
	/* options present? */
	if (dhcpData->mIncomingPacket->mTotalLength > DHCP_HEADER_LENGTH)
	{
		dhcpData->mIncomingOptionsLength = ((u16_t)(dhcpData->mIncomingPacket->mTotalLength - DHCP_HEADER_LENGTH));
		dhcpData->mIncomingOptions = &(PacketBuffer_GetStartPoint(dhcpData->mIncomingPacket)[240]);
	} else {
		dhcpData->mIncomingOptionsLength=0;
		dhcpData->mIncomingOptions=NULL;
	}
	dhcpData->mIncomingMessage = PacketBuffer_GetStartPoint(dhcpData->mIncomingPacket);

	return ERR_OK;
}

/**
 * Free the incoming DHCP message including contiguous copy of
 * its DHCP options.
 *
 */
static void Dhcp_FreeReply(PDHCP_DATA dhcpData)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData!=NULL);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	dhcpData->mIncomingMessage = NULL;
	dhcpData->mIncomingOptions = NULL;
	dhcpData->mIncomingOptionsLength = 0;
	SMSC_TRACE(DHCP_DEBUG, ("Dhcp_FreeReply(): free'd"));
}


/**
 * If an incoming DHCP message is in response to us, then trigger the state machine
 */
static void Dhcp_Receive(
	void * param, struct UDP_CONTROL_BLOCK * udpControlBlock,
	PPACKET_BUFFER packetBuffer,
	PIP_ADDRESS sourceAddress,
	u16_t sourcePort)
{
	struct NETWORK_INTERFACE * networkInterface = (struct NETWORK_INTERFACE * )param;
	PDHCP_DATA dhcpData;
	u8_t * replyMessage = PacketBuffer_GetStartPoint(packetBuffer);
	u8_t * optionsPointer;
	u8_t messageType;
	u8_t index;
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	
	dhcpData=networkInterface->mDhcpData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData!=NULL);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	#if SMSC_TRACE_ENABLED
	{
		char addressString[IP_ADDRESS_STRING_SIZE];
		SMSC_TRACE(DHCP_DEBUG, ("Dhcp_Receive(pbuf = %p) from DHCP server %s:%"U16_F, (void*)packetBuffer,
			IP_ADDRESS_TO_STRING(addressString,sourceAddress), sourcePort));
	}
	#endif
	/*SMSC_TRACE(DHCP_DEBUG, ("packetBuffer->mThisLength = %"U16_F, packetBuffer->mThisLength));
	SMSC_TRACE(DHCP_DEBUG, ("packetBuffer->mTotalLength = %"U32_F, packetBuffer->mTotalLength));*/
	/* prevent warnings about unused arguments */
	(void)udpControlBlock; (void)sourceAddress; (void)sourcePort;
	dhcpData->mIncomingPacket = packetBuffer;
	/* TODO: check packet length before reading them */
	if (DHCP_HEADER_GET_OP_CODE(replyMessage) != DHCP_BOOTREPLY) {
		SMSC_NOTICE(DHCP_DEBUG, ("not a DHCP reply message, but type %"U16_F, 
			(u16_t)DHCP_HEADER_GET_OP_CODE(replyMessage)));
		PacketBuffer_DecreaseReference(packetBuffer);
		dhcpData->mIncomingPacket = NULL;
		return;
	}
	/* iterate through hardware address and match against DHCP message */
	if(networkInterface->mLinkType==NETWORK_INTERFACE_LINK_TYPE_ETHERNET) {
		for (index = 0; index < 6; index++) {
			if (networkInterface->mLinkData.mEthernetData.mMacAddress[index] != 
				DHCP_HEADER_GET_CLIENT_HARDWARE_ADDRESS(replyMessage)[index]) 
			{
				SMSC_NOTICE(DHCP_DEBUG, ("networkInterface->hwaddr[%"U16_F"]==%02"X16_F" != replyMessage->chaddr[%"U16_F"]==%02"X16_F,
					(u16_t)index, (u16_t)networkInterface->mLinkData.mEthernetData.mMacAddress[index],
					(u16_t)index, (u16_t)DHCP_HEADER_GET_CLIENT_HARDWARE_ADDRESS(replyMessage)[index]));
				PacketBuffer_DecreaseReference(packetBuffer);
				dhcpData->mIncomingPacket = NULL;
				return;
			}
		}
	} else {
		SMSC_ERROR(("DHCP only supports ethernet link type at this time"));
		PacketBuffer_DecreaseReference(packetBuffer);
		dhcpData->mIncomingPacket = NULL;
	}
	/* match transaction ID against what we expected */
	if (DHCP_HEADER_GET_TRANSACTION_ID(replyMessage) != dhcpData->mTransactionId) {
		SMSC_NOTICE(DHCP_DEBUG, ("transaction id mismatch replyMessage->mTransactionId(%"X32_F")!=dhcpData->mTransactionId(%"X32_F")",
			DHCP_HEADER_GET_TRANSACTION_ID(replyMessage),dhcpData->mTransactionId));
		PacketBuffer_DecreaseReference(packetBuffer);
		dhcpData->mIncomingPacket = NULL;
		return;
	}
	/* option fields could be unfold? */
	if (Dhcp_UnfoldReply(dhcpData) != ERR_OK) {
		SMSC_NOTICE(DHCP_DEBUG, ("problem unfolding DHCP message - too short on memory?"));
		PacketBuffer_DecreaseReference(packetBuffer);
		dhcpData->mIncomingPacket = NULL;
		return;
	}

	SMSC_TRACE(DHCP_DEBUG, ("searching DHCP_OPTION_MESSAGE_TYPE"));
	/* obtain pointer to DHCP message type */
	optionsPointer = Dhcp_GetOptionPointer(dhcpData, DHCP_OPTION_MESSAGE_TYPE);
	if (optionsPointer == NULL) {
		SMSC_NOTICE(DHCP_DEBUG, ("DHCP_OPTION_MESSAGE_TYPE option not found"));
		PacketBuffer_DecreaseReference(packetBuffer);
		dhcpData->mIncomingPacket = NULL;
		return;
	}

	/* read DHCP message type */
	messageType = Dhcp_GetOptionByte(optionsPointer + 2);
	/* message type is DHCP ACK? */
	if (messageType == DHCP_ACK) {
		SMSC_TRACE(DHCP_DEBUG, ("DHCP_ACK received"));
		/* in requesting state? */
		if (dhcpData->mState == DHCP_REQUESTING) {
			Dhcp_HandleAck(networkInterface);
			dhcpData->mRequestTimeOut = 0;
#if DHCP_DOES_ARP_CHECK
			/* check if the acknowledged lease address is already in use */
			Dhcp_Check(networkInterface);
#else
			/* bind interface to the acknowledged lease address */
			Dhcp_Bind(networkInterface);
#endif
		}
		/* already bound to the given lease address? */
		else if ((dhcpData->mState == DHCP_REBOOTING) || (dhcpData->mState == DHCP_REBINDING) || (dhcpData->mState == DHCP_RENEWING)) {
			dhcpData->mRequestTimeOut = 0;
			Dhcp_Bind(networkInterface);
		}
	}
	/* received a DHCP_NAK in appropriate state? */
	else if ((messageType == DHCP_NAK) &&
		((dhcpData->mState == DHCP_REBOOTING) || (dhcpData->mState == DHCP_REQUESTING) ||
		 (dhcpData->mState == DHCP_REBINDING) || (dhcpData->mState == DHCP_RENEWING  ))) 
	{
		SMSC_TRACE(DHCP_DEBUG, ("DHCP_NAK received"));
		dhcpData->mRequestTimeOut = 0;
		Dhcp_HandleNak(networkInterface);
	}
	/* received a DHCP_OFFER in DHCP_SELECTING state? */
	else if ((messageType == DHCP_OFFER) && (dhcpData->mState == DHCP_SELECTING)) {
		SMSC_TRACE(DHCP_DEBUG, ("DHCP_OFFER received in DHCP_SELECTING state"));
		dhcpData->mRequestTimeOut = 0;
		/* remember offered lease */
		Dhcp_HandleOffer(networkInterface);
	}
	PacketBuffer_DecreaseReference(packetBuffer);
	dhcpData->mIncomingPacket = NULL;
}


static err_t Dhcp_CreateRequest(struct NETWORK_INTERFACE * networkInterface)
{
	PDHCP_DATA dhcpData;
	u16_t index;
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(networkInterface!=NULL,ERR_ARG);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	
	dhcpData=networkInterface->mDhcpData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN(dhcpData!=NULL,ERR_ARG);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	SMSC_ASSERT(dhcpData->mOutgoingPacket == NULL);
	SMSC_ASSERT(dhcpData->mOutgoingMessage == NULL);
	dhcpData->mOutgoingPacket = Udp_AllocatePacket(NULL,DHCP_HEADER_LENGTH+DHCP_OPTIONS_LENGTH);
	if (dhcpData->mOutgoingPacket == NULL) {
		SMSC_NOTICE(DHCP_DEBUG, ("Dhcp_CreateRequest(): could not allocate packet buffer"));
		return ERR_MEM;
	}
	/* give unique transaction identifier to this request */
	dhcpData->mTransactionId = gTransactionID++;
	SMSC_TRACE(DHCP_DEBUG, ("gTransactionID++(%"X32_F") dhcpData->mTransactionId(%"U32_F")",gTransactionID,dhcpData->mTransactionId));

	dhcpData->mOutgoingMessage = PacketBuffer_GetStartPoint(dhcpData->mOutgoingPacket);
	memset(dhcpData->mOutgoingMessage,0,DHCP_HEADER_LENGTH+DHCP_OPTIONS_LENGTH);
	
	DHCP_HEADER_SET_OP_CODE(dhcpData->mOutgoingMessage,DHCP_BOOTREQUEST);
	if(networkInterface->mLinkType==NETWORK_INTERFACE_LINK_TYPE_ETHERNET) {
		u8_t * clientHardwareAddress=DHCP_HEADER_GET_CLIENT_HARDWARE_ADDRESS(dhcpData->mOutgoingMessage);
		DHCP_HEADER_SET_HARDWARE_TYPE(dhcpData->mOutgoingMessage,DHCP_HTYPE_ETH);
		DHCP_HEADER_SET_HARDWARE_ADDRESS_LENGTH(dhcpData->mOutgoingMessage,DHCP_HLEN_ETH);
		for (index = 0; index < 6; index++) {
			/* copy networkInterface hardware address */
			clientHardwareAddress[index] = networkInterface->mLinkData.mEthernetData.mMacAddress[index];
		}
	}
	DHCP_HEADER_SET_TRANSACTION_ID(dhcpData->mOutgoingMessage,dhcpData->mTransactionId);
	DHCP_HEADER_SET_CLIENT_IP_ADDRESS(dhcpData->mOutgoingMessage,networkInterface->mIpv4Data.mAddress);
	DHCP_HEADER_SET_COOKIE(dhcpData->mOutgoingMessage,0x63825363UL);
	dhcpData->mOutgoingOptionsLength = 0;
	/* fill options field with an incrementing array (for debugging purposes) */
	for (index = 0; index < DHCP_OPTIONS_LENGTH; index++)
		DHCP_HEADER_GET_OPTIONS(dhcpData->mOutgoingMessage)[index] = index;
	return ERR_OK;
}

static void Dhcp_DeleteRequest(struct NETWORK_INTERFACE * networkInterface)
{
	PDHCP_DATA dhcpData;
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface!=NULL);
	CHECK_SIGNATURE(networkInterface,NETWORK_INTERFACE_SIGNATURE);
	
	dhcpData = networkInterface->mDhcpData;
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData!=NULL);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData->mOutgoingPacket != NULL);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData->mOutgoingMessage != NULL);
	dhcpData->mOutgoingPacket = NULL;
	dhcpData->mOutgoingMessage = NULL;
}

/**
 * Add a DHCP message trailer
 *
 * Adds the END option to the DHCP message, and if
 * necessary, up to three padding bytes.
 */

static void Dhcp_AddOptionTrailer(PDHCP_DATA dhcpData)
{
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData!=NULL);
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData->mOutgoingMessage != NULL);
	SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData->mOutgoingOptionsLength < DHCP_OPTIONS_LENGTH);
	DHCP_HEADER_GET_OPTIONS(dhcpData->mOutgoingMessage)[dhcpData->mOutgoingOptionsLength++] = DHCP_OPTION_END;
	/* packet is too small, or not 4 byte aligned? */
	while ((dhcpData->mOutgoingOptionsLength < DHCP_OPTIONS_LENGTH) || (dhcpData->mOutgoingOptionsLength & 3)) {
		/* SMSC_TRACE(DHCP_DEBUG,("Dhcp_AddOptionTrailer:dhcpData->mOutgoingOptionsLength=%"U16_F", DHCP_OPTIONS_LEN=%"U16_F, dhcpData->mOutgoingOptionsLength, DHCP_OPTIONS_LEN)); */
		SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(dhcpData->mOutgoingOptionsLength < DHCP_OPTIONS_LENGTH);
		/* add a fill/padding byte */
		DHCP_HEADER_GET_OPTIONS(dhcpData->mOutgoingMessage)[dhcpData->mOutgoingOptionsLength++] = 0;
	}
}

/**
 * Find the offset of a DHCP option inside the DHCP message.
 *
 * @param client DHCP client
 * @param option_type
 *
 * @return a byte offset into the UDP message where the option was found, or
 * zero if the given option was not found.
 */
static u8_t *Dhcp_GetOptionPointer(PDHCP_DATA dhcpData, u8_t option_type)
{
	u8_t overload = DHCP_OVERLOAD_NONE;
    
	if(dhcpData==NULL)	return NULL;
	CHECK_SIGNATURE(dhcpData,DHCP_DATA_SIGNATURE);
	
	/* options available? */
	if ((dhcpData->mIncomingOptions != NULL) && (dhcpData->mIncomingOptionsLength > 0)) {
		/* start with options field */
		u8_t *options = (u8_t *)dhcpData->mIncomingOptions;
		u16_t offset = 0;
		/* at least 1 byte to read and no end marker, then at least 3 bytes to read? */
		while ((offset < dhcpData->mIncomingOptionsLength) && (options[offset] != DHCP_OPTION_END)) {
			/* SMSC_TRACE(DHCP_DEBUG, ("msg_offset=%"U16_F", q->len=%"U16_F, msg_offset, q->len)); */
			/* are the sname and/or file field overloaded with options? */
			if (options[offset] == DHCP_OPTION_OVERLOAD) {
				/*SMSC_TRACE(DHCP_DEBUG, ("overloaded message detected"));*/
				/* skip option type and length */
				offset += 2;
				overload = options[offset++];
			}
			/* requested option found */
			else if (options[offset] == option_type) {
				/*SMSC_TRACE(DHCP_DEBUG, ("option found at offset %"U16_F" in options", offset));*/
				return &options[offset];
			/* skip option */
			} else {
				/*SMSC_TRACE(DHCP_DEBUG, ("skipping option %"U16_F" in options", options[offset]));*/
				/* skip option type */
				offset++;
				/* skip option length, and then length bytes */
				offset += 1 + options[offset];
			}
		}
		/* is this an overloaded message? */
		if (overload != DHCP_OVERLOAD_NONE) {
			u16_t field_len;
			if (overload == DHCP_OVERLOAD_FILE) {
				/*SMSC_TRACE(DHCP_DEBUG, ("overloaded file name field"));*/
				options = DHCP_HEADER_GET_BOOT_FILE_NAME(dhcpData->mIncomingMessage);
				field_len = DHCP_BOOT_FILE_NAME_LENGTH;
			} else if (overload == DHCP_OVERLOAD_SNAME) {
				SMSC_TRACE(DHCP_DEBUG, ("overloaded server name field"));
				options = DHCP_HEADER_GET_SERVER_NAME(dhcpData->mIncomingMessage);
				field_len = DHCP_SERVER_NAME_LENGTH;
			/* TODO: check if else if () is necessary */
			} else {
				/*SMSC_TRACE(DHCP_DEBUG, ("overloaded server name and file name field"));*/
				options = DHCP_HEADER_GET_SERVER_NAME(dhcpData->mIncomingMessage);
				field_len = DHCP_BOOT_FILE_NAME_LENGTH + DHCP_SERVER_NAME_LENGTH;
			}
			offset = 0;

			/* at least 1 byte to read and no end marker */
			while ((offset < field_len) && (options[offset] != DHCP_OPTION_END)) {
				if (options[offset] == option_type) {
					/*SMSC_TRACE(DHCP_DEBUG, ("option found at offset=%"U16_F, offset));*/
					return &options[offset];
				/* skip option */
				} else {
					/*SMSC_TRACE(DHCP_DEBUG, ("skipping option %"U16_F, options[offset]));*/
					/* skip option type */
					offset++;
					offset += 1 + options[offset];
				}
			}
		}
	}
	return NULL;
}

/**
 * Return the byte of DHCP option data.
 *
 * @param client DHCP client.
 * @param pointer pointer obtained by Dhcp_GetOptionPointer().
 *
 * @return byte value at the given address.
 */
static u8_t Dhcp_GetOptionByte(u8_t *pointer)
{
	/*SMSC_TRACE(DHCP_DEBUG, ("option byte value=%"U16_F, (u16_t)(*pointer)));*/
	return *pointer;
}

#if 0
/**
 * Return the 16-bit value of DHCP option data.
 *
 * @param client DHCP client.
 * @param pointer pointer obtained by Dhcp_GetOptionPointer().
 *
 * @return byte value at the given address.
 */
static u16_t Dhcp_GetOptionShort(u8_t *pointer)
{
	u16_t value;
	value = *pointer++ << 8;
	value |= *pointer;
	/*SMSC_TRACE(DHCP_DEBUG, ("option short value=%"U16_F, value));*/
	return value;
}
#endif

/**
 * Return the 32-bit value of DHCP option data.
 *
 * @param client DHCP client.
 * @param pointer pointer obtained by Dhcp_GetOptionPointer().
 *
 * @return byte value at the given address.
 */
static u32_t Dhcp_GetOptionLong(u8_t *pointer)
{
	u32_t value;
	value = (u32_t)(*pointer++) << 24;
	value |= (u32_t)(*pointer++) << 16;
	value |= (u32_t)(*pointer++) << 8;
	value |= (u32_t)(*pointer++);
	/*SMSC_TRACE(DHCP_DEBUG, ("option long value=%"U32_F, value));*/
	return value;
}

