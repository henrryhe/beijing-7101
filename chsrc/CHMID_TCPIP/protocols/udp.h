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
/***************************************************************************** 
* Modifications of this software Copyright(C) 2008, Standard Microsystems Corporation
*   All Rights Reserved.
*   The modifications to this program code are proprietary to SMSC and may not be copied,
*   distributed, or used without a license to do so.  Such license may have
*   Limited or Restricted Rights. Please refer to the license for further
*   clarification.
******************************************************************************
* FILE: udp.h
* PURPOSE: declares the UDP protocol interface for use by SMSC stack internal
*    applications.
* NOTE: Users of this UDP protocol interface must be running in the same 
*    thread as the network stack. To schedule tasks that run in the network
*    stack thread see task_manager.h
*****************************************************************************/

#ifndef UDP_H
#define UDP_H

#include "smsc_environment.h"

#if !UDP_ENABLED
#error UDP is not enabled
#endif

#include "ip.h"
#include "packet_manager.h"

struct UDP_CONTROL_BLOCK;

/*****************************************************************************
* FUNCTION TYPE: UDP_RECEIVE_FUNCTION
* DESCRIPTION:
*    This is the function template for passing received UDP packets to
*    the application.
* PARAMETERS:
*    param, this is an application specific parameter provided in the call
*           to Udp_SetReceiverCallBack
*    udpControlBlock, This is the control block that received the packet.
*    packetBuffer, this is the received packet buffer list.
*    sourceAddress, this is the address the packet came from.
*    sourcePort, this is the port the packet came from.
*****************************************************************************/
typedef void (*UDP_RECEIVE_FUNCTION)(
	void * param, struct UDP_CONTROL_BLOCK * udpControlBlock,
	PPACKET_BUFFER packetBuffer,
	PIP_ADDRESS sourceAddress,
	u16_t sourcePort);

#if SMSC_ERROR_ENABLED
#define UDP_CONTROL_BLOCK_SIGNATURE	(0x98556058)
#endif

/*****************************************************************************
* STRUCTURE: UDP_CONTROL_BLOCK
* DESCRIPTION:
*    This structure maintains context information about what data to receive
*    and where to send data.
*****************************************************************************/
struct UDP_CONTROL_BLOCK {
	DECLARE_SIGNATURE
	struct UDP_CONTROL_BLOCK * mNext;
	IP_ADDRESS mLocalAddress;
	IP_ADDRESS mRemoteAddress;
	u16_t mLocalPort;
	u16_t mRemotePort;

#define UDP_FLAGS_UDPLITE	(0x02)
#define UDP_FLAGS_CONNECTED	(0x04)
	u16_t mFlags;
	
	UDP_RECEIVE_FUNCTION mReceiverCallBack;
	void * mReceiverCallBackParam;
};

/*****************************************************************************
* FUNCTION: Udp_Initialize
* DESCRIPTION:
*    Main initialization function must be called before any other Udp_ function
*****************************************************************************/
void Udp_Initialize(void);

/*****************************************************************************
* FUNCTION: Udp_CreateControlBlock
* DESCRIPTION: Allocates and initializes a UDP_CONTROL_BLOCK
*****************************************************************************/
struct UDP_CONTROL_BLOCK * Udp_CreateControlBlock(void);

/*****************************************************************************
* FUNCTION: Udp_FreeControlBlock
* DESCRIPTION: Deallocates a UDP_CONTROL_BLOCK, 
*     the UDP_CONTROL_BLOCK should not be binded, or connected when this is
*     called.
*****************************************************************************/
void Udp_FreeControlBlock(struct UDP_CONTROL_BLOCK * udpControlBlock);

/*****************************************************************************
* FUNCTION: Udp_Bind
* DESCRIPTION:
*     Binds the UDP_CONTROL_BLOCK to a local IP address and port
*****************************************************************************/
err_t Udp_Bind(struct UDP_CONTROL_BLOCK * udpControlBlock,
	PIP_ADDRESS localAddress,u16_t localPort);

/*****************************************************************************
* FUNCTION: Udp_UnBind
* DESCRIPTION:
*     Unbinds the UDP_CONTROL_BLOCK.
*****************************************************************************/
void Udp_UnBind(struct UDP_CONTROL_BLOCK * udpControlBlock);

/*****************************************************************************
* FUNCTION: Udp_SetReceiverCallBack
* DESCRIPTION: Sets the receiver callback function and parameter
*****************************************************************************/
void Udp_SetReceiverCallBack(
	struct UDP_CONTROL_BLOCK * udpControlBlock,
	UDP_RECEIVE_FUNCTION receiverFunction,void * receiverParam);

/*****************************************************************************
* FUNCTION: Udp_Connect
* DESCRIPTION: Associates a remote address and port with a UDP_CONTROL_BLOCK,
*    While in connected state, Udp_Send will send all packets to the 
*    associated remote address and port. Also only packets arriving from
*    the remote address and port will be passed to the application.
*****************************************************************************/
err_t Udp_Connect(struct UDP_CONTROL_BLOCK * udpControlBlock,
	PIP_ADDRESS remoteAddress,u16_t remotePort);

/*****************************************************************************
* FUNCTION: Udp_Disconnect
* DESCRIPTION: De-associates a UDP_CONTROL_BLOCK from a remote address and port
*****************************************************************************/
void Udp_Disconnect(struct UDP_CONTROL_BLOCK * udpControlBlock);

/*****************************************************************************
* FUNCTION: Udp_AllocatePacket
* DESCRIPTION: 
*    Allocates a packet buffer list
* PARAMETERS:
*    destinationAddress, This is the destination address where the packet
*                        will be send to. This information is necessary
*                        to make sure the packet buffer list has enough
*                        header space in each packet buffer, and to make sure
*                        no packet buffer will exceed the MTU of the interface
*                        that the packet will be sent on.
*    size, This is the size of payload space requested.
*****************************************************************************/
PPACKET_BUFFER Udp_AllocatePacket(PIP_ADDRESS destinationAddress,u32_t size);

/*****************************************************************************
* FUNCTION: Udp_SendTo
* DESCRIPTION:
*    Sends a packet to a specified destination address and port
*****************************************************************************/
void Udp_SendTo(
	struct UDP_CONTROL_BLOCK * udpControlBlock,PPACKET_BUFFER packet,
	PIP_ADDRESS destinationAddress, u16_t destinationPort);

/*****************************************************************************
* FUNCTION: Udp_Send
* DESCRIPTION:
*    Sends a packet to a the destination specified in a previous call to 
*    Udp_Connect
*****************************************************************************/
void Udp_Send(
	struct UDP_CONTROL_BLOCK * udpControlBlock,PPACKET_BUFFER packet);


struct NETWORK_INTERFACE;

/*****************************************************************************
* FUNCTION: Udp_Input
* DESCRIPTION:
*    This is the lower layer interface function for receiving packets from 
*    IP to UDP. This function should not be called by an application.
*****************************************************************************/
void Udp_Input(
	struct NETWORK_INTERFACE * networkInterface, PPACKET_BUFFER packetBuffer, 
	PIP_ADDRESS destinationAddress,PIP_ADDRESS sourceAddress,
	u8_t protocol);

#endif /* UDP_H */
