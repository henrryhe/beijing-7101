/*
 * port.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Create ports on each CPU and see if the shutdown process works
 * correctly.
 */

/* WARNING: this test is not generic. it will only work on transports
 * where the disconnection of one agent causes the transport to be 
 * invalidated.
 */

#include "harness.h"

static EMBX_TRANSPORT transport;
static EMBX_PORT inPort, outPort;

void blocker(void *notUsed)
{
	EMBX_RECEIVE_EVENT event;

	EMBX_E(EMBX_PORT_INVALIDATED, ReceiveBlock(inPort, &event));
	EMBX(ClosePort(inPort));
	EMBX(ClosePort(outPort));
	EMBX(CloseTransport(transport));
	MASTER(EMBX(Deinit()));
}

int main()
{
	harness_boot();
	transport = harness_openTransport();
	harness_connect(transport, "litter", &inPort, &outPort);
	harness_createThread(blocker, NULL);

	SLAVE (harness_sleep(10));
	SLAVE (EMBX(Deinit()));

	harness_waitForChildren();

	/* leave some arbitary period of time for the slave to exit */
	MASTER(printf("******** Pausing to allow slave to exit ********\n"));
	MASTER(harness_sleep(10));
	MASTER(printf("******** Passed ********\n"));
	SLAVE (printf("******** "CPU"/"OS" shutdown OK ********\n"));

	return 0;
}
