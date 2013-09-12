/*
 * producer.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Producer side of the buffer pool example
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harness.h"

#define BUFFER_POOL_SIZE 10
#define BUFFER_SIZE 1024

int main()
{
EMBX_TRANSPORT hTrans;
EMBX_PORT bufferpool, consumer, mypoolconnection;
EMBX_ERROR err;
int i;

	embxRuntimeInit();
	err = EMBX_OpenTransport("shm", &hTrans);
	assert(EMBX_SUCCESS == err);

	/* Create the port which will hold the pool of work buffers */
	err = EMBX_CreatePort(hTrans, "bufferpool", &bufferpool);
	assert(EMBX_SUCCESS == err);

	/* Make a connection to the port we just created so we can inject
	 * empty buffers onto the port's queue as part of the initialization
	 */
	err = EMBX_Connect(hTrans, "bufferpool", &mypoolconnection);
	assert(EMBX_SUCCESS == err);

	/* Now wait for the consumer port to come into existence and 
	 * make a connection to it.
	 */
	err = EMBX_ConnectBlock(hTrans, "consumer", &consumer);
	assert(EMBX_SUCCESS == err);


	/* Inject empty buffers into the buffer pool */
	for(i=0;i<BUFFER_POOL_SIZE;i++)
	{
	EMBX_VOID *buffer;
		
		err = EMBX_Alloc(hTrans, BUFFER_SIZE, &buffer);
		assert(EMBX_SUCCESS == err);

		/* Send empty buffer to the buffer pool port */
		err = EMBX_SendMessage(mypoolconnection, buffer, 0);
		assert(EMBX_SUCCESS == err);
	}

	/* We don't need our connection to the buffer pool anymore
	 * so close it down.
	 */
	EMBX_ClosePort(mypoolconnection);

	for(i=0;i<100;i++)
	{
	EMBX_RECEIVE_EVENT ev;
	EMBX_UINT buffersize;
		
		/* Jabber ... */
		printf("producer: Issuing message %d of 100\n", i+1);


		/* Get an empty buffer from the pool */
		err = EMBX_ReceiveBlock(bufferpool, &ev);
		assert(EMBX_SUCCESS == err);

		/* The event size field does not represent the actual
		 * size of the buffer in this case, hence if you need
		 * to find that out use the following. However in this
		 * case where all the buffers are the same known size 
		 * it wouldn't be necessary expect as self checking debug.
		 */
		err = EMBX_GetBufferSize(ev.data, &buffersize);
		assert(EMBX_SUCCESS == err);

		/* Do something to fill the buffer with stuff to be used
		 * by the consumer...... use your imagination ......
		 */


		/* Now send the buffer to the consumer with the real amount
		 * of data in the buffer as the size argument (this can be
		 * less than the buffer size). For this example we assume
		 * the whole buffer contains valid data.
		 */
		err = EMBX_SendMessage(consumer, ev.data, buffersize);
		assert(EMBX_SUCCESS == err);
	}

	/* Shut the communication system down. This has the side effect
	 * of causing the consumer to also close down, almost certainly
	 * before outstanding messages have been processed.
	 */
	EMBX_ClosePort(bufferpool);
	EMBX_ClosePort(consumer);
	EMBX_CloseTransport(hTrans);
	EMBX_Deinit();

	return 0;
}

