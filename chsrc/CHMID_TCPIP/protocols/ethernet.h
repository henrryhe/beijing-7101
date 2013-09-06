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
File: ethernet.h
*/

#ifndef ETHERNET_H
#define ETHERNET_H

#include "smsc_environment.h"
#include "packet_manager.h"

struct NETWORK_INTERFACE;

typedef void (*ETHERNET_OUTPUT_FUNCTION)(
	struct NETWORK_INTERFACE * networkInterface,
	PPACKET_BUFFER packet);

typedef struct ETHERNET_DATA_ {
	DECLARE_SIGNATURE
	ETHERNET_OUTPUT_FUNCTION mOutputFunction;
	u8_t mMacAddress[6];
} ETHERNET_DATA, * PETHERNET_DATA;

#define ETHERNET_FLAG_ENABLE_IPV4	(0x01)
#define ETHERNET_FLAG_ENABLE_IPV6	(0x02)

void Ethernet_Initialize(void);
void Ethernet_InitializeInterface(
	struct NETWORK_INTERFACE * networkInterface,
	ETHERNET_OUTPUT_FUNCTION outputFunction,
	u8_t hardwareAddress[6],
	u8_t enableFlags);
	
void Ethernet_Input(
	struct NETWORK_INTERFACE * networkInterface,
	PPACKET_BUFFER packet);
void Manual_ArpReply(struct NETWORK_INTERFACE * networkInterface, PIPV4_ADDRESS SourceipAddress,u8_t * ethernetAddress);
void Manual_ArpInit(void);

#endif
