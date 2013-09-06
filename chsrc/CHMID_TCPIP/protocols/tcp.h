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
* FILE: tcp.h
* PURPOSE: declares the TCP protocol interface for use by SMSC stack internal
*    applications.
* NOTE: Users of this TCP protocol interface must be running in the same
*    thread as the network stack. To schedule tasks that run in the network
*    stack thread see task_manager.h
*****************************************************************************/

#ifndef TCP_H
#define TCP_H

#include "smsc_environment.h"

#if !TCP_ENABLED
#error TCP is not enabled
#endif

#include "ip.h"
#include "network_stack.h"
#include "packet_manager.h"

#define TCP_TMR_INTERVAL       200  /* The TCP timer interval in
                                       milliseconds. */
#define TCP_FAST_INTERVAL      TCP_TMR_INTERVAL /* the fine grained timeout in
                                       milliseconds */
#define TCP_SLOW_INTERVAL      (2*TCP_TMR_INTERVAL)  /* the coarse grained timeout in
                                       milliseconds */
#define TCP_FIN_WAIT_TIMEOUT			20000 /* milliseconds */
#define TCP_SYN_RECEIVED_TIMEOUT		20000 /* milliseconds */
#define TCP_OUT_OF_SEQUENCE_TIMEOUT		6U /* x RTO */
#define TCP_MAXIMUM_SEGMENT_LIFETIME	60000  /* The maximum segment lifetime in milliseconds */

/*
 * User-settable options (used with setsockopt).
 */
#define TCP_NODELAY    0x01    /* don't delay send to coalesce packets */
#define TCP_KEEPALIVE  0x02    /* send KEEPALIVE probes when idle for tcpControlBlock->keepalive miliseconds */

/* Keepalive values */
#define TCP_KEEPDEFAULT   7200000                       /* KEEPALIVE timer in miliseconds */
#define TCP_KEEPINTVL     75000                         /* Time between KEEPALIVE probes in miliseconds */
#define TCP_KEEPCNT       9                             /* Counter for KEEPALIVE probes */
#define TCP_MAXIDLE       TCP_KEEPCNT * TCP_KEEPINTVL   /* Maximum KEEPALIVE probe time */

/*
 * Option flags per-socket. These are the same like SO_XXX.
 */
#define SOCKET_OPTION_FLAG_DEBUG		((u16_t)0x0001U)	/* turn on debugging info recording */
#define SOCKET_OPTION_FLAG_ACCEPTCONN	((u16_t)0x0002U)	/* socket has had listen() */
#define SOCKET_OPTION_FLAG_REUSEADDR	((u16_t)0x0004U)	/* allow local address reuse */
#define SOCKET_OPTION_FLAG_KEEPALIVE	((u16_t)0x0008U)	/* keep connections alive */
#define SOCKET_OPTION_FLAG_DONTROUTE	((u16_t)0x0010U)	/* just use interface addresses */
#define SOCKET_OPTION_FLAG_BROADCAST	((u16_t)0x0020U)	/* permit sending of broadcast msgs */
#define SOCKET_OPTION_FLAG_USELOOPBACK	((u16_t)0x0040U)	/* bypass hardware when possible */
#define SOCKET_OPTION_FLAG_LINGER		((u16_t)0x0080U)	/* linger on close if data present */
#define SOCKET_OPTION_FLAG_OOBINLINE	((u16_t)0x0100U)	/* leave received OOB data in line */
#define SOCKET_OPTION_FLAG_REUSEPORT	((u16_t)0x0200U)	/* allow local address & port reuse */

typedef enum TCP_STATE_ {
	TCP_STATE_CLOSED      = 0,
	TCP_STATE_LISTEN      = 1,
	TCP_STATE_SYN_SENT    = 2,
	TCP_STATE_SYN_RCVD    = 3,
	TCP_STATE_ESTABLISHED = 4,
	TCP_STATE_FIN_WAIT_1  = 5,
	TCP_STATE_FIN_WAIT_2  = 6,
	TCP_STATE_CLOSE_WAIT  = 7,
	TCP_STATE_CLOSING     = 8,
	TCP_STATE_LAST_ACK    = 9,
	TCP_STATE_TIME_WAIT   = 10
} TCP_STATE;

#if SMSC_ERROR_ENABLED
#define TCP_SEGMENT_SIGNATURE	(0xB4828EC8)
#endif

/* This structure represents a TCP segment on the unsent and unacked queues */
typedef struct TCP_SEGMENT_ {
	DECLARE_SIGNATURE
	struct TCP_SEGMENT_ *mNext;	/* used when putting segements on a queue */
	PPACKET_BUFFER mPacket;		/* buffer containing data + TCP header */
	u8_t * mDataPointer;		/* pointer to the TCP data in the packet Buffer */
	u16_t mLength;				/* the TCP length of this segment */
	u8_t * mTcpHeader;			/* the TCP header */
} TCP_SEGMENT, * PTCP_SEGMENT;

struct TCP_CONTROL_BLOCK;

/* Function to be called when more send buffer space is available. */
typedef err_t (*TCP_SENT_CALLBACK)(void *arg, struct TCP_CONTROL_BLOCK * tcpControlBlock, u16_t space);

/* Function to be called when (in-sequence) data has arrived. */
typedef err_t (*TCP_RECEIVE_CALLBACK)(void *arg, struct TCP_CONTROL_BLOCK * tcpControlBlock, PPACKET_BUFFER packet, err_t err);

/* Function to be called when a connection has been set up. */
typedef err_t (*TCP_CONNECTED_CALLBACK)(void *arg, struct TCP_CONTROL_BLOCK * tcpControlBlock, err_t err);

/* Function to call when a listener has been connected. */
typedef err_t (*TCP_ACCEPT_CALLBACK)(void *arg, struct TCP_CONTROL_BLOCK * newControlBlock, err_t err);

/* Function to be called whenever a fatal error occurs. */
typedef void (*TCP_ERROR_CALLBACK)(void *arg, err_t err);


#if SMSC_ERROR_ENABLED
#define TCP_CONTROL_BLOCK_SIGNATURE		(0x984BD89F)
#endif

/* the TCP protocol control block */
struct TCP_CONTROL_BLOCK {
	DECLARE_SIGNATURE

	/** common control block members */
	IP_ADDRESS mLocalAddress;
	IP_ADDRESS mRemoteAddress;
	/* socket options */
	u16_t mSocketOptions;
	/* Type of Service */
	/*u8_t mTypeOfService;*/
	/* Time to live */
	/*u8_t mTimeToLive;*/
	/** protocol specific members */
	struct TCP_CONTROL_BLOCK * mNext; /* for the linked list */
	TCP_STATE mState; /* TCP state */
	u8_t mPriority;
	void * mCallbackArgument;

	u16_t mLocalPort;
	u16_t mRemotePort;

	u8_t mFlags;
#define TCB_FLAG_ACK_DELAY			((u8_t)0x01U)	/* Delayed ACK. */
#define TCB_FLAG_ACK_NOW			((u8_t)0x02U)	/* Immediate ACK. */
#define TCB_FLAG_IN_FAST_RECOVERY	((u8_t)0x04U)	/* In fast recovery. */
#define TCB_FLAG_RESET				((u8_t)0x08U)	/* Connection was reset. */
#define TCB_FLAG_CLOSED				((u8_t)0x10U)	/* Connection was sucessfully closed. */
#define TCB_FLAG_GOT_FIN			((u8_t)0x20U)	/* Connection was closed by the remote end. */
#define TCB_FLAG_NODELAY			((u8_t)0x40U)	/* Disable Nagle algorithm */

	/* receiver variables */
	u32_t mReceiverNextSequenceNumber;	                    /* next seqno expected */
	u16_t mReceiverWindow;				                            /* receiver window */
	/* Timers */
	u32_t mTimer;
	/*u8_t polltmr, pollinterval;*/

	/* Retransmission timer. */
	u16_t mRetransmissionTimer;

	u16_t mMaximumSegmentSize;   /* maximum segment size */

	/* RTT (round trip time) estimation variables */
	u32_t mRoundTripTest; /* RTT estimate in 500ms ticks */
	u32_t mRoundTripSequenceNumber;  /* sequence number being timed */
	s16_t mScaledAverage, mScaledDeviation; /* @todo document this */

	u16_t mRetransmissionTimeOut;    /* retransmission time-out */
	u8_t mNumberOfRetransmissions;    /* number of retransmissions */

	/* fast retransmit/recovery */
	u32_t mLastAcknowledgedSequenceNumber; /* Highest acknowledged seqno. */
	u8_t mDuplicateAcks;

	/* congestion avoidance/control variables */
	u16_t mCongestionWindow;  
	u16_t mSlowStartThreshold;

	/* sender variables */
	u32_t mSenderNextSequenceNumber;		/* next sequence number to be sent */
	u32_t mSenderHighestSentSequenceNumber;	/* Highest sequence number sent. */
	u32_t mSenderWindow;					/* sender window */
	u32_t mSenderLastSequenceNumber;		/* Sequence number of last window update */
	u32_t mSenderLastAcknowledgementNumber;	/* Acknowledgement number of last window update. */
	u32_t mSenderNumberByteToBeBuffered;	/* Sequence number of next byte to be buffered. */

	u16_t mAcknowledged;

	u16_t mAvailableSendSpace;   /* Available buffer space for sending (in bytes). */
	/*u8_t snd_queuelen;*/ /* Available buffer space for sending (in tcp_segs). */

	/* These are ordered by sequence number: */
	PTCP_SEGMENT mUnsentSegments;			       /* Unsent (queued) segments. */
	PTCP_SEGMENT mUnackedSegments;			/* Sent but unacknowledged segments. */
	PTCP_SEGMENT mOutOfSequenceSegments; 	/* Received out of sequence segments. */

	/* Function to be called when more send buffer space is available. */
	TCP_SENT_CALLBACK mSentCallBack;
	
	/* Function to be called when (in-sequence) data has arrived. */
	TCP_RECEIVE_CALLBACK mReceiveCallBack;

	/* Function to be called when a connection has been set up. */
	TCP_CONNECTED_CALLBACK mConnectedCallBack;

	/* Function to call when a listener has been connected. */
	TCP_ACCEPT_CALLBACK mAcceptCallBack;

	/* Function to be called whenever a fatal error occurs. */
	TCP_ERROR_CALLBACK mErrorCallBack;

	/* idle time before KEEPALIVE is sent */
	u32_t mKeepAliveTimer;

	/* KEEPALIVE counter */
	u8_t mKeepAliveCounter;
};

#define TCP_PRIORITY_MIN    1
#define TCP_PRIORITY_NORMAL 64
#define TCP_PRIORITY_MAX    127

/*****************************************************************************
* FUNCTION: Tcp_Initialize
* DESCRIPTION:
*    This is the TCP initialization function. It must be called before any
*    other Tcp_ functions.
*****************************************************************************/
void Tcp_Initialize(void);

/*****************************************************************************
* FUNCTION: Tcp_NewControlBlock
* DESCRIPTION:
*    Allocates a new TCP control block
*****************************************************************************/
struct TCP_CONTROL_BLOCK * Tcp_NewControlBlock(void);

/*****************************************************************************
* FUNCTION: Tcp_AllocateControlBlock
* DESCRIPTION:
*    Allocates a new TCP control block and assigns a priority to it.
*****************************************************************************/
struct TCP_CONTROL_BLOCK * Tcp_AllocateControlBlock(u8_t priority);

/*****************************************************************************
* FUNCTION: Tcp_FreeControlBlock
* DESCRIPTION:
*    Frees a TCP control block
*****************************************************************************/
void Tcp_FreeControlBlock(struct TCP_CONTROL_BLOCK * tcpControlBlock);

/*****************************************************************************
* FUNCTION: Tcp_SetCallBackArgument
* DESCRIPTION:
*    Sets the call back argument for a tcpControlBlock
*****************************************************************************/
void Tcp_SetCallBackArgument(struct TCP_CONTROL_BLOCK * tcpControlBlock, void * argument);

/*****************************************************************************
* FUNCTION: Tcp_SetErrorCallBack
* DESCRIPTION:
*    Sets the error call back function for a tcpControlBlock
*****************************************************************************/
void Tcp_SetErrorCallBack(struct TCP_CONTROL_BLOCK * tcpControlBlock, TCP_ERROR_CALLBACK callback);

/*****************************************************************************
* FUNCTION: Tcp_SetReceiveCallBack
* DESCRIPTION:
*    Sets the Receive call back function for a tcpControlBlock
*****************************************************************************/
void Tcp_SetReceiveCallBack(struct TCP_CONTROL_BLOCK * tcpControlBlock, TCP_RECEIVE_CALLBACK callback);

/*****************************************************************************
* FUNCTION: Tcp_SetSentCallBack
* DESCRIPTION:
*    Sets the Sent call back function for a tcpControlBlock
*****************************************************************************/
void Tcp_SetSentCallBack(struct TCP_CONTROL_BLOCK * tcpControlBlock, TCP_SENT_CALLBACK callback);

/*****************************************************************************
* FUNCTION: Tcp_SetAcceptCallBack
* DESCRIPTION:
*    Sets the Accept call back function for a tcpControlBlock
*****************************************************************************/
void Tcp_SetAcceptCallBack(struct TCP_CONTROL_BLOCK * tcpControlBlock, TCP_ACCEPT_CALLBACK callback);

/*****************************************************************************
* FUNCTION: Tcp_Bind
* DESCRIPTION:
*    Binds a tcpControlBlock to an ipAddress and port
*****************************************************************************/
err_t Tcp_Bind(struct TCP_CONTROL_BLOCK * tcpControlBlock, PIP_ADDRESS ipAddress, u16_t port);

/*****************************************************************************
* FUNCTION: Tcp_Connect
* DESCRIPTION:
*    Initiates a connection request to an ipAddress and port. The callback
*    will be called on completion.
*****************************************************************************/
err_t Tcp_Connect(struct TCP_CONTROL_BLOCK * tcpControlBlock, PIP_ADDRESS ipAddress, u16_t port,
			TCP_CONNECTED_CALLBACK callback);

/*****************************************************************************
* FUNCTION: Tcp_Listen
* DESCRIPTION:
*    Puts a tcpControlBlock into the listening state. Should have previously
*    called Tcp_Bind
*****************************************************************************/
void Tcp_Listen(struct TCP_CONTROL_BLOCK * tcpControlBlock);

/*****************************************************************************
* FUNCTION: Tcp_AcknowledgeReceivedData
* DESCRIPTION:
*    This function is called by the application to acknowledge that the data
*    was received. This increases the receiver window size.
*****************************************************************************/
void Tcp_AcknowledgeReceivedData(struct TCP_CONTROL_BLOCK * tcpControlBlock, u32_t len);

/*****************************************************************************
* FUNCTION: Tcp_Close
* DESCRIPTION:
*    Closes a connected tcpControlBlock
*****************************************************************************/
err_t Tcp_Close(struct TCP_CONTROL_BLOCK * tcpControlBlock);

/*****************************************************************************
* FUNCTION: Tcp_GetMaximumSegmentSize
* DESCRIPTION:
*    Gets the maximum segment size. This is useful information because the
*    application can use this to allocate data buffers that are a multiple
*    of this maximum segment size, to allow for more efficient transfers.
*****************************************************************************/
u16_t Tcp_GetMaximumSegmentSize(struct TCP_CONTROL_BLOCK * tcpControlBlock);

/*****************************************************************************
* FUNCTION: Tcp_AllocateDataBuffers
* DESCRIPTION:
*    Allocates Packet buffers that the application can use to fill in and then
*    queue them with Tcp_QueueData.
*****************************************************************************/
PPACKET_BUFFER Tcp_AllocateDataBuffers(struct TCP_CONTROL_BLOCK * tcpControlBlock, u32_t size);

/*****************************************************************************
* FUNCTION: Tcp_QueueData
* DESCRIPTION:
*    Queues data to be sent on this tcp connection. Any packet buffers that
*    can not be queue will be returned.
*****************************************************************************/
PPACKET_BUFFER Tcp_QueueData(struct TCP_CONTROL_BLOCK * tcpControlBlock,PPACKET_BUFFER packet);

/*****************************************************************************
* FUNCTION: Tcp_SendQueuedData
* DESCRIPTION:
*    Sends the data that was previously queued with Tcp_QueueData
*****************************************************************************/
err_t Tcp_SendQueuedData(struct TCP_CONTROL_BLOCK * tcpControlBlock);

struct NETWORK_INTERFACE;

/*****************************************************************************
* FUNCTION: Tcp_Input
* DESCRIPTION:
*    This is the lower layer interface function for receiving packets from 
*    IP to TCP. This function should not be called by an application.
*****************************************************************************/
void Tcp_Input(
	struct NETWORK_INTERFACE * networkInterface, PPACKET_BUFFER packetBuffer, 
	PIP_ADDRESS destinationAddress,PIP_ADDRESS sourceAddress,
	u8_t protocol);

#endif /* TCP_H */
