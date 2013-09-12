/*
 * burst.c
 *
 * Copyright (C) STMicroelectronics Limited 2002. All rights reserved.
 *
 * Send messages of varying sizes in a 'bursty' fashion.
 * 
 */

#include "harness.h"

static EMBX_TRANSPORT transport;
static EMBX_PORT inPort, outPort;

#define NUM_BUFFERS     32
#define NUM_ITERATIONS  8

static void sendBursts(int bufferSize)
{
	int i, j;
	void *bufs[NUM_BUFFERS];

	/* allocate the blocks we are going to burst with */
	for (i=0; i<NUM_BUFFERS; i++) {
		EMBX(Alloc(transport, bufferSize, &(bufs[i])));
		harness_fillPattern(bufs[i], bufferSize, i);
	}

	for (j=0; j<NUM_ITERATIONS; j++) {
		/* burst send the messages */
		for (i=0; i<NUM_BUFFERS; i++) {
			EMBX(SendMessage(outPort, bufs[i], bufferSize));
		}

		/* now receive the replies */
		for (i=0; i<NUM_BUFFERS; i++) {
			EMBX_RECEIVE_EVENT event;

			EMBX(ReceiveBlock(inPort, &event));
			assert(event.size == bufferSize);
			bufs[i] = event.data;
		}

		/* finally perform an integrety check */
		for (i=0; i<NUM_BUFFERS; i++) {
			assert(harness_testPattern(bufs[i], bufferSize, i));
		}
	}

	/* clean up */
	for (i=0; i<NUM_BUFFERS; i++) {
		EMBX(Free(bufs[i]));
	}
}

static void echoBursts(void)
{
	int i;
	void *bufs[NUM_BUFFERS];

	while (1) {
		EMBX_RECEIVE_EVENT event;

		for (i=0; i<NUM_BUFFERS; i++) {
			EMBX(ReceiveBlock(inPort, &event));

			bufs[i] = event.data;
			if (0 == event.size) {
				EMBX(Free(event.data));
				goto break_break;
			}
		}

		for (i=0; i<NUM_BUFFERS; i++) {
			/* we know that all the buffers on this block are the same size */
			EMBX(SendMessage(outPort, bufs[i], event.size));
		}
	}
break_break: ;
}

int main()
{
	EMBX_VOID *buffer;

	/* boot and connect */
	harness_boot();
	transport = harness_openTransport();
	harness_connect(transport, "burst", &inPort, &outPort);

	/* warning suppression */
	(void) &echoBursts;
	(void) &sendBursts;

	/* do the actual test */
	SLAVE (echoBursts());
	MASTER(sendBursts(16));
	MASTER(sendBursts(64));
	MASTER(sendBursts(256));
	MASTER(sendBursts(1024));

	/* cause the slave to exit */
	MASTER(EMBX(Alloc(transport, 1, &buffer)));
	MASTER(EMBX(SendMessage(outPort, buffer, 0)));

	/* tidy up and close down */
	EMBX(ClosePort(inPort));
	EMBX(ClosePort(outPort));
	MASTER(harness_passed());
	SLAVE (harness_waitForShutdown());

	return 0;
}
