/*
 * message/thread.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Fully-symetric multi-threaded messaging romp.
 */

#include "harness.h"

/*
 * Test structure.
 *
 * This tests creates a number of worker threads that listen to a specific
 * port. Every time a message is received the worker threads examine the
 * message and either sends it to another port (the next stop) using
 * sendMessage (following a path coded into the message) or frees it and 
 * creates a another message using fabricateMessage.
 *
 * Because each port consumes one message and then transmits one message
 * there is a limit to the number of messages flowing at one time, therefore
 * as the test starts up a number of messages are injected into the system
 * by each CPU.
 */

/* setting this to be larger than 10 will cause problems with
 * the port names
 */
#define NUM_PORTS     4

/* the number of messages to be injected into our test system */
#define NUM_INJECTED  16

/* the number of ports the message must pass through before it is
 * freed
 */
#define NUM_STOPS     8

/* the number of messages one side will send before shutting down the
 * test. assuming relatively well distributed random numbers we should
 * in fact see twice this number of messages created
 */
#ifdef CONF_SLOW_SIMULATOR
#define NUM_MESSAGES 2500
#else
#define NUM_MESSAGES  50000
#endif

static int numMessages = 0;

static EMBX_TRANSPORT transport;
static EMBX_PORT inPort[NUM_PORTS];
static EMBX_PORT outPort[2 * NUM_PORTS];

static EMBX_VOID *fabricateMessage(void)
{
	int  *message;
	int   i;
	int   size;
	int   path[NUM_STOPS];

	harness_interruptLock();

	size = sizeof(int) + sizeof(path) + (rand() % 4096);

	/* setup the path the this message will take */
	for (i=0; i<NUM_STOPS; i++) {
		path[i] = rand() % (2 * NUM_PORTS);
	}

	numMessages++;

	harness_interruptUnlock();

	EMBX(Alloc(transport, size, (void *) &message));
	
	/* fill in the message */
	memset(message, 0, size);
	message[0] = 0;
	memcpy(&(message[1]), path, sizeof(path));

	return (EMBX_VOID *) message;
}

static int sendMessage(EMBX_VOID *message)
{
	int        *typedMessage = (int *) message;
	EMBX_UINT   size;
	int         stop;
	int         portIndex;
	EMBX_ERROR  err;

	/* get the size of the current message */
	EMBX(GetBufferSize(message, &size));

	/* get the number of the current stop */
	stop = typedMessage[0];
	assert(stop <= NUM_STOPS);
	if (stop == NUM_STOPS) {
		return 0;
	}

	/* determine the port that this message is next due
	 * to stop at
	 */
	portIndex = typedMessage[stop + 1];

	/* update the current stop index */
	typedMessage[0] = stop + 1;

	VERBOSE(printf("Sending message to portIndex %d\n", portIndex));

	/* finally send the message to the next stop */
	err = EMBX_I(SendMessage(outPort[portIndex], message, size));
	assert(EMBX_SUCCESS == err || EMBX_INVALID_PORT == err);

	return 1;
}

static void workerThread(void *p)
{
	int portIndex = (int) p;
	int i, status;

	/* make the jabber less bursty */
	status = (portIndex * NUM_MESSAGES * NUM_STOPS) / (160 * NUM_PORTS);

	while (1) {
		EMBX_RECEIVE_EVENT event;

		EMBX(ReceiveBlock(inPort[portIndex], &event));
		assert(event.type == EMBX_REC_MESSAGE);

		if (0 == event.size) {
			EMBX(Free(event.data));
			return;
		}

#if ! defined CONF_SOAKTEST
		if (numMessages > NUM_MESSAGES) {
			EMBX(Free(event.data));
			break;
		}
#endif

		if (!sendMessage(event.data)) {
			EMBX(Free(event.data));
			event.data = fabricateMessage();
			assert(event.data);
			assert(sendMessage(event.data));
		}

		/* this is a long running test so give it some 'jabber' */
		if (status++ >= ((NUM_MESSAGES * NUM_STOPS) / 160)) {
			status = 0;
#if defined CONF_MASTER
                        printf("-");
#else
                        printf("|");
#endif
			fflush(stdout);
		}
	}

	for (i=0; i<(NUM_PORTS*2); i++) {
		EMBX_VOID  *buf;
		EMBX_ERROR  err;

		/* here we send a zero-sized message to force a remote close */
		EMBX(Alloc(transport, 1, &buf));
		err = EMBX_I(SendMessage(outPort[i], buf, 0));
		assert(EMBX_SUCCESS == err || EMBX_INVALID_PORT == err);
	}
}

int main(void)
{
	int i;

	harness_boot();
	transport = harness_openTransport();

	/* since we are multi-threaded and subject to off-chip timing
         * differences this will not be sufficient to make this test 
         * deterministic
         */
        srand(0);

	/* create all the ports */
	for (i=0; i<NUM_PORTS; i++) {
		char localName[]  = "port?" LOCAL;
		char remoteName[] = "port?" REMOTE;

		/* update the port names */
		localName[4] = remoteName[4] = '0' + i;

		/* create the receiving port */
		EMBX(CreatePort(transport, localName, &(inPort[i])));
		
		/* now create both transmission ports */
		EMBX(ConnectBlock(transport, remoteName, &(outPort[i])));
		EMBX(ConnectBlock(transport, localName,  &(outPort[i+NUM_PORTS])));
	}

	/* now create the threads that will actually run the test */
	for (i=0; i<NUM_PORTS; i++) {
		harness_createThread(workerThread, (void *) i);
	}

	/* now inject some messages into the test system (we only inject
	 * half the requested number because our partner will inject the
	 * other half)
	 */
	for (i=0; i<(NUM_INJECTED / 2); i++) {
		EMBX_VOID *message;
		message = fabricateMessage();
		assert(message);
		assert(sendMessage(message));
	}

	/* now wait for the test to run out of steam */
	harness_waitForChildren();

	/* clean up after ourselves */
	for (i=0; i<NUM_PORTS; i++) {
		EMBX(ClosePort(inPort[i]));
		EMBX(ClosePort(outPort[i]));
		EMBX(ClosePort(outPort[i+NUM_PORTS]));
	}

	printf("\n"); /* pretty printing */
	harness_shutdown();
	return 0;
}
