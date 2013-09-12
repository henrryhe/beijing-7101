/*
 * leak.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Fully-symetric test to try to expose memory leaks in the ClosePort
 * sequence.
 */

#include "harness.h"

#ifdef CONF_SLOW_SIMULATOR
#define NUM_ITERATIONS 500
#else
#define NUM_ITERATIONS 10000
#endif

#define SIZEOF_BLOCK   16384

int main(void)
{
	EMBX_TRANSPORT   transport;
	EMBX_PORT	 ctrlInPort, ctrlOutPort;
	int              i;

	harness_boot();
	transport = harness_openTransport();
	harness_connect(transport, "control", &ctrlInPort, &ctrlOutPort);

	for (i=0; i<NUM_ITERATIONS; i++) {
		EMBX_PORT leakInPort, leakOutPort;
		EMBX_VOID *block;
		EMBX_RECEIVE_EVENT event;

		/* 'jabber' during the execution of this long test */
		if (1 == (i % (NUM_ITERATIONS / 160))) {
#if defined CONF_MASTER
                        printf("-");
#else
                        printf("|");
#endif
                        fflush(stdout);
		}

		/* we can't use harness_connect() here because it performs
		 * a printf which will give this test an artifically long
		 * runtime 
		 */
		EMBX(CreatePort(transport, "leak" LOCAL, &leakInPort));

		/* stuff a message onto the remote receive queue */
		EMBX(Alloc(transport, SIZEOF_BLOCK, &block));
		EMBX(ConnectBlock(transport, "leak" REMOTE, &leakOutPort));
		EMBX(SendMessage(leakOutPort, block, SIZEOF_BLOCK));
		EMBX(ClosePort(leakOutPort));

		/* now notify the other side via the control port */
		EMBX(Alloc(transport, 1, &block));
		EMBX(SendMessage(ctrlOutPort, block, 0));

		/* stuff a message onto the local receive queue */
		EMBX(Alloc(transport, SIZEOF_BLOCK, &block));
		EMBX(ConnectBlock(transport, "leak" LOCAL, &leakOutPort));
		EMBX(SendMessage(leakOutPort, block, SIZEOF_BLOCK));
		EMBX(ClosePort(leakOutPort));

		/* wait for the notification on the control port */
		EMBX(ReceiveBlock(ctrlInPort, &event));
		assert(EMBX_REC_MESSAGE == event.type);
		assert(0 == event.size);
		EMBX(Free(event.data));

		/* now close the input port with its two outstanding
		 * messages on it
		 */
		if (i & 1) {
			EMBX(InvalidatePort(leakInPort));
		}
		EMBX(ClosePort(leakInPort));
	}

	EMBX(ClosePort(ctrlInPort));
	EMBX(ClosePort(ctrlOutPort));
	printf("\n"); /* pretty up the output */
	harness_shutdown();
	return 0;
}
