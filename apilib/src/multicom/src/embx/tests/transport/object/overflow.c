/*
 * overflow.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Queue thousands of distributed objects on a port and check that the 
 * transports never loses a message it has committed to transmitting.
 *
 * NOTE: this test sends a I've finished message as part of its control
 * data. if the transport is losing messages this can cause the test to
 * lock up.
 */

#include "harness.h"

#define TRACE(v) printf(MACHINE __FILE__ " (%d) - " #v " = %#010x\n", __LINE__, (unsigned) v)

static EMBX_TRANSPORT transport;
static EMBX_PORT inPort, outPort;

#define NUM_BUFFERS    512

static void sendObjects(char *map)
{
	int *buf;
	EMBX_HANDLE h;
	int i;
	EMBX_ERROR err;

	EMBX(Alloc(transport, NUM_BUFFERS, (void **) &buf));
	EMBX(RegisterObject(transport, buf, NUM_BUFFERS, &h));

	for (i=0; i<NUM_BUFFERS; i++) {
		err = EMBX_I(SendObject(outPort, h, i, 1));
		assert(EMBX_SUCCESS == err || EMBX_NOMEM == err);
		map[i] = (err == EMBX_SUCCESS ? '-' : '#');
	}

	map[NUM_BUFFERS] = '\0';

	/* check that we really did overflow port capacity at some point */
	assert('#' == map[NUM_BUFFERS-1]);

	while (EMBX_NOMEM == (err = EMBX_I(SendObject(outPort, h, 0, NUM_BUFFERS)))) {
		/* loop very fast (helps show we cannot break the memory interface) */
	}
	assert(EMBX_SUCCESS == err);

	harness_sleep(1);
	harness_rendezvous(transport, inPort, outPort);

	EMBX(DeregisterObject(transport, h));
}

static void receiveObjects(char *map)
{
	EMBX_RECEIVE_EVENT event;

	memset(map, '#', NUM_BUFFERS);

	do {
		EMBX(ReceiveBlock(inPort, &event));
		assert(EMBX_REC_OBJECT == event.type);
		if (1 == event.size) {
			map[event.offset] = '-';
		}
	} while (1 == event.size);
	TRACE(event.size);
	assert(NUM_BUFFERS == event.size);

	map[NUM_BUFFERS] = '\0';

	harness_sleep(1);
	harness_rendezvous(transport, inPort, outPort);
}

static void selfTest(void)
{
	int *buf;
	EMBX_HANDLE h;
	int i;
	EMBX_ERROR err;
	EMBX_PORT in, out;
	char map[NUM_BUFFERS+1];
	EMBX_RECEIVE_EVENT event;

	EMBX(CreatePort(transport, "local"LOCAL, &in));
	EMBX(Connect(transport, "local"LOCAL, &out));

	EMBX(Alloc(transport, NUM_BUFFERS, (void **) &buf));
	EMBX(RegisterObject(transport, buf, NUM_BUFFERS, &h));

	for (i=0; i<NUM_BUFFERS; i++) {
		err = EMBX_I(SendObject(out, h, i, 1));
		assert(EMBX_SUCCESS == err || EMBX_NOMEM == err);
		map[i] = (EMBX_SUCCESS == err ? '-' : '#');
	}

	map[NUM_BUFFERS] = '\0';
	printf(MACHINE"loopback map\n%s\n", map);

	for (i=0; i<NUM_BUFFERS; i++) {
		if ('-' == map[i]) {
			EMBX(Receive(in, &event));
			assert(EMBX_REC_OBJECT == event.type);
			assert(1 == event.size);
			assert(i == event.offset);
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
	MASTER(sendObjects(sendMap));
	SLAVE (printf(MACHINE "(2) started receiving\n"));
	SLAVE (receiveObjects(recvMap));
	MASTER(printf(MACHINE "(3) finished sending\n"));
	SLAVE (printf(MACHINE "(4) finished receiving\n"));

	MASTER(harness_sleep(5));
	SLAVE (printf(MACHINE "(5) started sending\n"));
	SLAVE (sendObjects(sendMap));
	MASTER(printf(MACHINE "(6) started receiving\n"));
	MASTER(receiveObjects(recvMap));
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
	harness_connect(transport, "overflow", &inPort, &outPort);

	runTests();

	/* tidy up and close down */
	EMBX(ClosePort(inPort));
	EMBX(ClosePort(outPort));
	MASTER(harness_passed());
	SLAVE (harness_waitForShutdown());

	return 0;
}
