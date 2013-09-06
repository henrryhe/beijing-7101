/* Copyright 2008 SMSC, All rights reserved
FILE: tcp_tester.h
*/

#ifndef TCP_TESTER_H
#define TCP_TESTER_H

#include "smsc_environment.h"

#if !TCP_ENABLED
#error TCP is not enabled
#endif

#include "utilities/packet_manager.h"
#include "utilities/task_manager.h"
#include "protocols/tcp.h"

#if SMSC_ERROR_ENABLED
#define TCP_RX_TESTER_SIGNATURE 	(0xFB348732)
#endif
typedef struct TCP_RX_TESTER_ {
	DECLARE_SIGNATURE
	struct TCP_CONTROL_BLOCK * mTcpControlBlock;
	u8_t mNextIncrementValue;
} TCP_RX_TESTER, * PTCP_RX_TESTER;

void TcpTester_InitializeReceiver(
	PTCP_RX_TESTER udpTester,
	PIP_ADDRESS localAddress,u16_t localPort);

#if SMSC_ERROR_ENABLED
#define TCP_TX_TESTER_SIGNATURE 	(0xF83783CD)
#endif
typedef struct TCP_TX_TESTER_ {
	DECLARE_SIGNATURE
	struct TCP_CONTROL_BLOCK * mTcpControlBlock;
	/*TASK mTransmitTask;
	u8_t mNextIncrementValue;*/
	IP_ADDRESS mRemoteAddress;
	u16_t mRemotePort;
	u8_t mContinuousMode;
} TCP_TX_TESTER, * PTCP_TX_TESTER;

void TcpTester_InitializeTransmitter(
	PTCP_TX_TESTER udpTester,
	PIP_ADDRESS remoteAddress,u16_t remotePort,
	u8_t mode);

#endif /* TCP_TESTER_H */
