/***************************************************************************
 *
 * Copyright (C) 2008  SMSC
 *
 * All rights reserved.
 *
 ***************************************************************************
 * File: loop_back.h
 */

#ifndef LOOP_BACK_H
#define LOOP_BACK_H

#include "smsc_environment.h"
#include "task_manager.h"
#include "packet_manager.h"

struct NETWORK_INTERFACE;

#if !LOOP_BACK_ENABLED
#error Loop Back is not enabled
#endif

#if (SMSC_ERROR_ENABLED)
#define LOOP_BACK_PRIVATE_DATA_SIGNATURE (0xE3639473)
#endif

typedef struct LOOP_BACK_PRIVATE_DATA_ {
	DECLARE_SIGNATURE
	TASK mRxTask;
	PACKET_QUEUE mPacketQueue;
} LOOP_BACK_PRIVATE_DATA, * PLOOP_BACK_PRIVATE_DATA;

err_t LoopBack_InitializeInterface(struct NETWORK_INTERFACE * interface);

#endif /* LOOP_BACK_H */
