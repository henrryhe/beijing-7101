/*
 * overflow.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Queue thousands of messages on a port and check that the transports
 * never loses a message it has committed to transmitting.
 *
 * NOTE: this test sends a I've finished message as part of its control
 * data. if the transport is losing messages this can cause the test to
 * lock up.
 */

#include "harness.h"

static EMBX_TRANSPORT transport;
static EMBX_PORT inPort, outPort;

#define NUM_BUFFERS    512

static void sendMessages(char *map)
{
	int *bufs[NUM_BUFFERS];
	int *buf;
	int i;
	EMBX_ERROR err;

	for (i=0; i<NUM_BUFFERS; i++) {
		EMBX(Alloc(transport, sizeof(int), (void **) &(bufs[i])));
		*(bufs[i]) = i;
		err = EMBX_I(SendMessage(outPort, bufs[i], sizeof(int)));
		assert(EMBX_SUCCESS == err || EMBX_NOMEM == err);
		map[i] = (err == EMBX_SUCCESS ? '-' : '#');
		if (err != EMBX_SUCCESS) {
			EMBX(Free(bufs[i]));
		}
	}

	map[NUM_BUFFERS] = '\0';

	/* check that we really did overflow port capacity at some point! */
	assert('#' == map[NUM_BUFFERS-1]);

	EMBX(Alloc(transport, sizeof(int), (void **) &buf));
	while (EMBX_NOMEM == (err = EMBX_I(SendMessage(outPort, buf, 0)))) {
	}
	assert(EMBX_SUCCESS == err);

	harness_sleep(1);
	harness_rendezvous(transport, inPort, outPort);
}

static void receiveMessages(char *map)
{
	EMBX_RECEIVE_EVENT event;
	int *buf;

	memset(map, '#', NUM_BUFFERS);

	do {
		EMBX(ReceiveBlock(inPort, &event));
		assert(EMBX_REC_MESSAGE == event.type);
		buf = event.data;
		if (0 != event.size) {
			assert(sizeof(int) == event.size);
			map[*buf] = '-';
		}
		EMBX(Free(buf));
	} while (0 != event.size);

	map[NUM_BUFFERS] = '\0';

	harness_sleep(1);
	harness_rendezvous(transport, inPort, outPort);
}

static void selfTest(void)
{
	int *buf;
	EMBX_RECEIVE_EVENT event;
	EMBX_ERROR err;
	int i;
	EMBX_PORT in, out;
	char map[NUM_BUFFERS+1];

	EMBX(CreatePort(transport, "local"LOCAL, &in));
	EMBX(Connect(transport, "local"LOCAL, &out));

	for (i=0; i<NUM_BUFFERS; i++) {
		EMBX(Alloc(transport, sizeof(int), (void **) &buf));
		*buf = i;
		err = EMBX_I(SendMessage(out, buf, sizeof(int)));
		assert(EMBX_SUCCESS == err || EMBX_NOMEM == err);
		map[i] = (err == EMBX_SUCCESS ? '-' : '#');
		if (EMBX_SUCCESS != err) {
			EMBX(Free(buf));
		}
	}

	map[NUM_BUFFERS] = '\0';
	printf(MACHINE"loopback map\n%s\n", map);

	for (i=0; i<NUM_BUFFERS; i++) {
		if ('-' == map[i]) {
			EMBX(Receive(in, &event));
			assert(EMBX_REC_MESSAGE == event.type);
			assert(sizeof(int) == event.size);
			buf = event.data;
			assert(i == *buf);
			EMBX(Free(buf));
		}
	}
	
	/* prove there are no further messages */
	EMBX_E(EMBX_INVALID_STATUS, Receive(in, &event));

	EMBX(ClosePort(in));
	EMBX(ClosePort(out));
}

static void runTests(void)
{
	EMBX_VOID *buffer;
	EMBX_RECEIVE_EVENT event;
	char sendMap[NUM_BUFFERS+4];
	char recvMap[NUM_BUFFERS+4];

	/* the master sends a vast number of messages while the slave waits */
	SLAVE (harness_sleep(5));
	MASTER(printf(MACHINE "(1) started sending\n"));
	MASTER(sendMessages(sendMap));
	SLAVE (printf(MACHINE "(2) started receiving\n"));
	SLAVE (receiveMessages(recvMap));
	MASTER(printf(MACHINE "(3) finished sending\n"));
	SLAVE (printf(MACHINE "(4) finished receiving\n"));

	MASTER(harness_sleep(5));
	SLAVE (printf(MACHINE "(5) started sending\n"));
	SLAVE (sendMessages(sendMap));
	MASTER(printf(MACHINE "(6) started receiving\n"));
	MASTER(receiveMessages(recvMap));
	SLAVE (printf(MACHINE "(7) finished sending\n"));
	MASTER(printf(MACHINE "(8) finished receiving\n"));

	/* now swap the receive maps with our partner */
	EMBX(Alloc(transport, sizeof(recvMap), &buffer));
	memcpy(buffer, recvMap, sizeof(recvMap));
	EMBX(SendMessage(outPort, buffer, sizeof(recvMap)));
	EMBX(ReceiveBlock(inPort, &event));
	assert(EMBX_REC_MESSAGE == event.type);
	assert(sizeof(recvMap) == event.size);
	buffer = event.data;
	memcpy(recvMap, buffer, sizeof(recvMap));
	EMBX(Free(buffer));

	/* finally check that the messages received by our partner match
	 * those that we transmitted
	 */
	SLAVE(harness_sleep(2));
	printf(MACHINE "send map\n%s\n"
	       MACHINE "receive map\n%s\n", sendMap, recvMap);
	assert(0 == strcmp(sendMap, recvMap));

	selfTest();
}

int main()
{
	/* boot and connect */
	harness_boot();
	transport = harness_openTransport();
	harness_connect(transport, "burst", &inPort, &outPort);

	runTests();

	/* tidy up and close down */
	EMBX(ClosePort(inPort));
	EMBX(ClosePort(outPort));
	MASTER(harness_passed());
	SLAVE (harness_waitForShutdown());

	return 0;
}
