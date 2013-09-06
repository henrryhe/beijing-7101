/* Copyright 2008 SMSC, All rights reserved
FILE: udp_tester.h
*/

#ifndef UDP_TESTER_H
#define UDP_TESTER_H

#include "smsc_environment.h"

#if !UDP_ENABLED
#error UDP is not enabled
#endif

#include "utilities/packet_manager.h"
#include "utilities/task_manager.h"
#include "protocols/udp.h"

#if SMSC_ERROR_ENABLED
#define UDP_RX_TESTER_SIGNATURE 	(0xFB348732)
#endif
typedef struct UDP_RX_TESTER_ {
	DECLARE_SIGNATURE
	struct UDP_CONTROL_BLOCK * mUdpControlBlock;
} UDP_RX_TESTER, * PUDP_RX_TESTER;

void UdpTester_InitializeReceiver(
	PUDP_RX_TESTER udpTester,
	PIP_ADDRESS localAddress,u16_t localPort);

#if SMSC_ERROR_ENABLED
#define UDP_TX_TESTER_SIGNATURE 	(0xF83783CD)
#endif
typedef struct UDP_TX_TESTER_ {
	DECLARE_SIGNATURE
	struct UDP_CONTROL_BLOCK * mUdpControlBlock;
	TASK mTransmitTask;
	u32_t mPeriod;
	IP_ADDRESS mRemoteAddress;
	u16_t mRemotePort;
	u32_t mPacketSize;
} UDP_TX_TESTER, * PUDP_TX_TESTER;

void UdpTester_InitializeTransmitter(
	PUDP_TX_TESTER udpTester,
	PIP_ADDRESS remoteAddress,u16_t remotePort,
	u32_t size);

#endif /* UDP_TESTER_H */
