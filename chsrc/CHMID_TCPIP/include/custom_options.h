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

#ifndef CUSTOM_OPTIONS_H
#define CUSTOM_OPTIONS_H

/*
SMSC_THREADING_ENABLED:
	set to 1 to enable multithreaded support functions.
	set to 0 to disable multithreaded support functions.
	
	The core network stack is designed to run in a single threaded
	environment. This provides about a 20% performance boost on 
	some embedded platforms. It would be advantagous if all
	applications were written to run in this single threaded 
	environment (see task_manager.h for how to create tasks that
	run in the single threaded environment). This eliminates the 
	need for context switches and synchronization primatives. 
	However some may not have the time to port their applications to
	this single theaded environment. For this reason the socket API is
	provided but the developer should be aware that the socket API requires
	a multithreaded environment, which is expected to have lower
	throughput speeds.
*/
#define SMSC_THREADING_ENABLED		(1 )

/*
INLINE_SIMPLE_FUNCTIONS: 
	if set to 1, many simple functions will be converted to
		macros. This will also cause many sanity checks to be ignored
	if set to 0, all simple functions will be implemented through
		actual function calls. This also enables many sanity checks, 
		provided that SMSC_ERROR_ENABLED==1.
*/
#define INLINE_SIMPLE_FUNCTIONS		(0)

/* ---------- Memory options ---------- */
/* MEMORY_ALIGNMENT: should be set to the alignment of the CPU for which
   lwIP is compiled. 4 byte alignment -> define MEM_ALIGNMENT to 4, 2
   byte alignment -> define MEM_ALIGNMENT to 2. */
#define MEMORY_ALIGNMENT          (4)

/* Timer callback feature 
Timer callbacks are controlled using the following two functions
smsc_callback_register, and smsc_callback_cancel
Set TIMER_ENTRY_COUNT to the maximum number of simultaneous users of the
Timer Callback feature. 
Setting TIMER_ENTRY_COUNT to 0 will cause all timer users to generate a syntax error.
*/
/* Users of the timer callback by function name
   1) tcpip_tcp_timer
   2) ip_timer
   3) DisplayStatisticsTimer
   TOTAL==TIMER_ENTRY_COUNT==3
*/
/* DUE to single threaded environment TIMER_ENTRY_COUNT should be 0 */
#define TIMER_ENTRY_COUNT		(3)

/*The following defines configure the memory pools for packet buffers */
#define SMALL_BUFFER_SIZE	(128*2)
#define SMALL_BUFFER_COUNT	(128*16)
#define LARGE_BUFFER_SIZE	(1520)
#define LARGE_BUFFER_COUNT	(128*16)

/* TASK_MANAGER_HIGHEST_PRIORITY indicates the highest priority allowed
	by the Task Manager. Priorities will range from 
		0 to TASK_MANAGER_HIGHEST_PRIORITY
*/
#define TASK_PRIORITY_APPLICATION_LOW		(0) /* Must be lower than RX */
#define TASK_PRIORITY_RX					       (14)         /*IP/TCP接收处理*/
#define TASK_PRIORITY_APPLICATION_NORMAL	       (14)         /*SOCKET数据传送给应用*/
#define TASK_PRIORITY_TIMER_ARP			(13)
#define TASK_PRIORITY_TIMER_TCP			(13)
#define TASK_PRIORITY_TIMER_ICMP			(10)
#define TASK_PRIORITY_TIMER					(12)
#define TASK_PRIORITY_TX					       (15)       /*IP/TCP发送*/
#define TASK_MANAGER_HIGHEST_PRIORITY		(15)

/* ARP_TABLE_SIZE defines the ARP Table size of ARP protocols */
#define ARP_TABLE_SIZE				(32)
#define ARP_ENTRY_MAX_AGE			(240)
#define ARP_ENTRY_MAX_PENDING_AGE	(4)

/*This is the maximum number of packets that can be received
 on a single task call */
#define SMSC911X_RX_BURST_COUNT		(4)
#define LOOP_BACK_RX_BURST_COUNT	(4)

/*This is the maximum number of packets that can be transmitted
 on a single task call */
#define SMSC911X_TX_BURST_COUNT	(4)

/* interface enabling switches */
#define LOOP_BACK_ENABLED	(1)
#define ETHERNET_ENABLED	(1) /*zhangjian*/
#define SMSC911X_ENABLED	(1)/*zhangjian*/
	/* Phy Workaround */
#define USE_PHY_WORK_AROUND (1)

/* MINIMUM_MTU is used when allocating packets whose destination
interface is unknown. It should be set to the smallest MTU of
all interfaces. */
#define MINIMUM_MTU	(1500)

/* protocol enabling switches */
#define IPV4_ENABLED		(1)
#define IPV4_FORWARDING_ENABLED		(0)
#define IPV4_FRAGMENTS_ENABLED		(0)
#define ICMPV4_ECHO_REPLY_ENABLED	(1)
#define ICMPV4_ECHO_REQUEST_ENABLED	(1)
#define ICMPV4_ECHO_REQUEST_COUNT	(5)

/* IPV6 is not currently implemented but you may search
for IPV6_ENABLED to find places where it is believed that
IPV6 code should be added.*/
#define IPV6_ENABLED		(0)

#define RAW_ENABLED				(0)

#define UDP_ENABLED				(1)
#define UDP_CONTROL_BLOCK_COUNT		(5)
#define UDP_LOCAL_PORT_RANGE_START 	(0x1000)
#define UDP_LOCAL_PORT_RANGE_END   	(0x7FFF)


#define TCP_ENABLED				(1)
#define TCP_TIME_TO_LIVE					(255)
#define TCP_MAXIMUM_SEGMENT_SIZE			/*(1460)*/(1024)		/* Maximum Segment Size */
#define TCP_WINDOW_SIZE						(32*TCP_MAXIMUM_SEGMENT_SIZE)	/* TCP receive window */
#define TCP_SEND_SPACE						(TCP_WINDOW_SIZE)	/* TCP sender buffer space (bytes) */
#define TCP_SYNMAXRTX						(4)			/* Maximum number of retransmissions of SYN segments */
#define TCP_MAXRTX							(12)		/* Maximum number of retransmissions of data segments */
#define TCP_CONTROL_BLOCK_COUNT				(64*4)
#define TCP_SEGMENT_COUNT					(100*20)
#define TCP_LOCAL_PORT_RANGE_START 			(0x1000)
#define TCP_LOCAL_PORT_RANGE_END			(0x7FFF)

/* TCP sender buffer space (pbufs). This must be at least = 2 *
   TCP_SND_BUF/TCP_MSS for things to work. */
	/*#define TCP_SND_QUEUELEN					(4 * TCP_SND_BUF/TCP_MSS)*/

/* TCP writable space (bytes). This must be less than or equal
   to TCP_SND_BUF. It is the amount of space which must be
   available in the tcp snd_buf for select to return writable */
#define TCP_SENDABLE_THRESHOLD            TCP_SEND_SPACE/2

#define DHCP_ENABLED			(1) /*zhangjian*/
#define DHCP_DOES_ARP_CHECK		(1)

#define SOCKETS_ENABLED			(1) /*zhangjian*/
#define MAXIMUM_NUMBER_OF_SOCKETS	/*(2)*/(64)
/*#define NET_BUFFER_COUNT			(2)
#define NET_CONNECTION_COUNT		(4)*/
#define SOCKET_REQUEST_BURST_COUNT	(32/*16*/)/*flex modified from 16 to 32 2009-4-16*/
#define SOCKET_USE_STANDARD_NAMES	(1)
#define SOCKET_MAXIMUM_BACKLOG		(5)

#if SOCKETS_ENABLED && !SMSC_THREADING_ENABLED
#error Sockets can not be enabled unless threading is also enabled.
#endif


/* NETIF_RX_PRIORITY sets the priority of receiver threads. */
/*#define NETIF_RX_PRIORITY		SMSC_THREAD_PRIORITY_LOWEST*/

/* SMSC_TRACE_ENABLED is used to enable messages intended for informational
   purposes only */
#define SMSC_TRACE_ENABLED		0 /*1 wz del*/

/* SMSC_NOTICE_ENABLED is used to enable messages intended to alert the user
   about anticipated error conditions that are properly handled.
   (an example is packet drops) */
#define SMSC_NOTICE_ENABLED		0 /*1 wz del*/

/* SMSC_WARNING_ENABLED is used to enable messages intended to alert the user
   about unknown or unexpected conditions, which may cause errors that may 
   not be handled properly. 
   (an examples is unknown or unexpected values, common in the 
       'default' section of a switch statement)*/
#define SMSC_WARNING_ENABLED	0 /*1 wz del*/

/* SMSC_ERROR_ENABLED is used to enable messages that alert the user that
   an unrecoverable error has been detected.
   (this includes violated assumptions encoded with SMSC_ASSERT) */
#define SMSC_ERROR_ENABLED		0

/* Enable individual types of TRACE, and NOTICE messages*/
#define PACKET_MANAGER_DEBUG	0
#define ETHERNET_DEBUG    		0
#define IPV4_DEBUG				0
#define ICMPV4_DEBUG			0
#define LOOP_BACK_DEBUG		0
#define SMSC911X_DEBUG          	0
#define UDP_DEBUG				0
#define TCP_DEBUG				0
#define DHCP_DEBUG				0
/*#define NET_PERF_DEBUG			1*/
#define SOCKETS_DEBUG			0
#define TCPIP_DEBUG				1
/*--------------------------MODIFICATION  NOTE----------------------------------------*/
/*
* 1,修改平台相关的内容邋（内存中由于4字节强制对齐所引发的问题 ）
* 2,修改DHCP_RELEASE时目标地址为广播地址。
*/
#define	M_WUFEI_080825/**/

/*
* 1,修改信号量等待时间，当等待时间为0时不为永久等待，否则会死锁
* 2,修改错误代码值(协议栈中错误代码宏与系统中的宏重复定义,目前将这些宏与系统中的宏区分开,修改成为负数值)
*/
#define M_WUFEI_080922

/*
* 为连云港市场添加DHCP报文中的OPTION 60字段
*/
#define M_WUFEI_081120

/*
为SocketsInternal_TcpReceiveCallBack()和SocketsInternal_TcpSentCallBack()两个函数增加返回值
*/
#define M_WUFEI_090206

/*
1,修改内存分配部分
2,修改客户端主动断开连接时，过早的tcpControlBlock->mState = TCP_STATE_CLOSED;导致内存泄露
3,添加packet队列处理信号量
*/
#define MEMORY_POOL_CHANGE_090217

/*
  控制在CHMID_TCPIP_IO.c中结收数据模式，多任务接收或但任务接收
*/
/*#define USE_MULTI_RX_THREAD*/

#endif /* CUSTOM_OPTIONS_H */
