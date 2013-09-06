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
File: icmpv4.h
*/

#ifndef ICMPV4_H
#define ICMPV4_H     

#include "smsc_environment.h"
#include "packet_manager.h"
#include "ipv4.h"

struct NETWORK_INTERFACE;

#if !IPV4_ENABLED
/* IPv4 is required */
#error IPv4 is not enabled
#endif

void Icmpv4_Initialize(void);
void Icmpv4_Input(
	struct NETWORK_INTERFACE * arrivalInterface,
	PPACKET_BUFFER packet);
void IcmpEchoTest_Initialize(IPV4_ADDRESS destination, u32_t size, u8_t timeOut);
s32_t Icmpv4EchoReply(void);

#if ICMPV4_ECHO_REQUEST_ENABLED
typedef void (*ICMPV4_ECHO_REPLY_CALLBACK)(void * param,PPACKET_BUFFER packetBuffer);
PPACKET_BUFFER Icmpv4_AllocatePacket(
	PIPV4_ADDRESS destinationAddress,u32_t size);
err_t Icmpv4_SendEchoRequest(
	IPV4_ADDRESS destination,PPACKET_BUFFER packet,
	ICMPV4_ECHO_REPLY_CALLBACK callback,void * callBackParam,
	u8_t timeOut);/* timeOut is in units of seconds */
#endif /*ICMPV4_ECHO_REQUEST_ENABLED */

#endif /* IPV4_H */
