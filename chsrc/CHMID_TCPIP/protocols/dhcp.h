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
 * Integration with your code:
 *
 * The application must first call Dhcp_Initialize, this sets up
 * the timers.
 *
 * Then call Dhcp_Start(struct NETWORK_INTERFACE * networkInterface, PDHCP_DATA dhcpData);
 * to attach a DHCP client on a particular interface. This immediately tries to
 * obtain an IP address lease and maintain it.
 *
 * Use Dhcp_Release(networkInterface) to end the lease and use Dhcp_Stop(networkInterface)
 * to remove the DHCP client.
 *
 */
/*****************************************************************************
* Modifications of this software Copyright(C) 2008, Standard Microsystems Corporation
*   All Rights Reserved.
*   The modifications to this program code are proprietary to SMSC and may not be copied,
*   distributed, or used without a license to do so.  Such license may have
*   Limited or Restricted Rights. Please refer to the license for further
*   clarification.
******************************************************************************
* FILE: dhcp.h
* PURPOSE: This file declares DHCP function
* USAGE:
*   The application must first call Dhcp_Initialize, this sets up
*   the timers.
*
*   Then call Dhcp_Start(struct NETWORK_INTERFACE * networkInterface, PDHCP_DATA dhcpData);
*   to attach a DHCP_DATA structure on a particular interface. This initializes the dhcpData
*   structure and immediately tries to obtain an IP address lease and maintain it.
*
*   Use Dhcp_Release(networkInterface) to end the lease and use Dhcp_Stop(networkInterface)
*   to remove the DHCP client.
*****************************************************************************/

#ifndef DHCP_H
#define DHCP_H

#include "smsc_environment.h"

#if !DHCP_ENABLED /* don't build if not configured for use in custom_options.h */
#error DHCP is not enabled
#endif
#if !IPV4_ENABLED
#error IPv4 is not enabled
#endif
#if !UDP_ENABLED
#error UDP is not enabled
#endif

#include "ipv4.h"
#include "udp.h"
#include "packet_manager.h"

#define DHCP_CLIENT_PORT 68  
#define DHCP_SERVER_PORT 67

#if SMSC_ERROR_ENABLED
#define DHCP_DATA_SIGNATURE	(0x894B8763)
#endif
/*****************************************************************************
* STRUCTURE: DHCP_DATA
* DESCRIPTION: Maintains context information for the DHCP protocol
*****************************************************************************/
typedef struct DHCP_DATA_
{
	DECLARE_SIGNATURE
	/** current DHCP state machine state */
	u8_t mState;
	/** retries of current request */
	u8_t mTryCount;
	/** transaction identifier of last sent request */ 
	u32_t mTransactionId;
	/** our connection to the DHCP server */ 
	struct UDP_CONTROL_BLOCK * mUdpControlBlock;
	/** (first) packet buffer of incoming msg */
	PPACKET_BUFFER mIncomingPacket;
	/** incoming msg */
	u8_t * mIncomingMessage;
	/** incoming msg options */
	u8_t * mIncomingOptions;
	/** incoming msg options length */
	u16_t mIncomingOptionsLength;

	PPACKET_BUFFER mOutgoingPacket;
	u8_t * mOutgoingMessage;
	u16_t mOutgoingOptionsLength;
	u16_t mRequestTimeOut; /* #ticks with period DHCP_FINE_TIMER_SECS for request timeout */
	u16_t mRenewTimeOut;  /* #ticks with period DHCP_COARSE_TIMER_SECS for renewal time */
	u16_t mRebindTimeOut;  /* #ticks with period DHCP_COARSE_TIMER_SECS for rebind time */
	IPV4_ADDRESS mServerAddress;/* dhcp server address that offered this lease */
	IPV4_ADDRESS mOfferedAddress;
	IPV4_ADDRESS mOfferedNetMask;
	IPV4_ADDRESS mOfferedGateway;
	IPV4_ADDRESS mOfferedBroadcast;
#define DHCP_MAX_DNS 2
	u32_t mDnsCount; /* actual number of DNS servers obtained */
	IPV4_ADDRESS mOfferedDns[DHCP_MAX_DNS];/* DNS server addresses */

	u32_t mOfferedLeaseTime; /* lease period (in seconds) */
	u32_t mRecommendedRenewTime; /* recommended renew time (usually 50% of lease period) */
	u32_t mRecommendedRebindTime; /* recommended rebind time (usually 66% of lease period)  */
/** Patch #1308
 *  TODO: See dhcp.c "TODO"s
 */
#if 0
	IPV4_ADDRESS mOfferedServerAddress;
	u8_t mBootFileName[128];
#endif
} DHCP_DATA, * PDHCP_DATA;

struct NETWORK_INTERFACE;

/*****************************************************************************
* FUNCTION: Dhcp_Initialize
* DESCRIPTION:
*    This function must be called before any other Dhcp_ function.
*    It is responsible for setting up the DHCP timer task.
*****************************************************************************/
void Dhcp_Initialize(void);

/*****************************************************************************
* FUNCTION: Dhcp_Start
* DESCRIPTION:
*    Initializes and attaches a DHCP_DATA structure to a network interface,
*    Then immediately tries to obtain an ip address.
*****************************************************************************/
err_t Dhcp_Start(struct NETWORK_INTERFACE * networkInterface,PDHCP_DATA dhcpData);

/*****************************************************************************
* FUNCTION: Dhcp_Renew
* DESCRIPTION:
*    Begin an early lease renewal. This is not needed normally
*****************************************************************************/
err_t Dhcp_Renew(struct NETWORK_INTERFACE * networkInterface);

/*****************************************************************************
* FUNCTION: Dhcp_Release
* DESCRIPTION:
*    Releases a DHCP lease, usually called before Dhcp_Stop
*****************************************************************************/
err_t Dhcp_Release(struct NETWORK_INTERFACE * networkInterface);

/*****************************************************************************
* FUNCTION: Dhcp_Stop
* DESCRIPTION:
*    Stops a DHCP client and unattaches the DHCP_DATA structure from the
*    Network Interface.
*****************************************************************************/
void Dhcp_Stop(struct NETWORK_INTERFACE * networkInterface);

/*****************************************************************************
* FUNCTION: Dhcp_Inform
* DESCRIPTION:
*    Informs the DHCP server of our manual assigned IP address
*****************************************************************************/
void Dhcp_Inform(struct NETWORK_INTERFACE * networkInterface);


#if DHCP_DOES_ARP_CHECK
/*****************************************************************************
* FUNCTION: Dhcp_ArpReply
* DESCRIPTION:
*    This function is called by the ethernet layer to inform a DHCP client
*    about ARP replies. ARPs are used by the DHCP client to make sure that
*    an address assigned by the DHCP server is not actually in use.
*    This function should not be called by the upper layers.
*****************************************************************************/
void Dhcp_ArpReply(struct NETWORK_INTERFACE * networkInterface, PIPV4_ADDRESS ipAddress);
#endif

#endif /*DHCP_H*/
