/*
 * consumer.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Consumer side of the buffer pool example
 */

#include "harness.h"

int main()
{
EMBX_TRANSPORT hTrans;
EMBX_PORT consumer, bufferpool;
EMBX_ERROR err;

	embxRuntimeInit();

	err = EMBX_OpenTransport("shm", &hTrans);
	assert(err == EMBX_SUCCESS);

	/* Create the port which the consumer will receive work
	 * messages on.
	 */
	err = EMBX_CreatePort(hTrans, "consumer", &consumer);
	assert(err == EMBX_SUCCESS);

	/* Connect to the buffer pool on the producer, we will send
	 * back finished message buffers to this so the producer can
	 * re-use them.
	 */
	err = EMBX_ConnectBlock(hTrans, "bufferpool", &bufferpool);
	assert(err == EMBX_SUCCESS);

	while(1)
	{
	EMBX_RECEIVE_EVENT ev;

		/* Wait for some work to do */
		err = EMBX_ReceiveBlock(consumer, &ev);
		if(err != EMBX_SUCCESS)
		{
			/* Assume, for the purposes of this example
			 * that this means the producer wants to close
			 * down. Hence clean up the transport and close
			 * down EMBX.
			 */
			EMBX_ClosePort(bufferpool);
			EMBX_ClosePort(consumer);
			EMBX_CloseTransport(hTrans);
			EMBX_Deinit();
			exit(1);
		}

		/* Do some work with the received message data here */
		printf("consumer: Processing message\n");
	
		/* Now send the message buffer back, note the zero size
		 * for the message length, this will prevent any data copying
		 * on a non shared memory transport.
		 */	
		err = EMBX_SendMessage(bufferpool, ev.data, 0);
		if(err != EMBX_SUCCESS)
		{
			EMBX_ClosePort(bufferpool);
			EMBX_ClosePort(consumer);
			EMBX_CloseTransport(hTrans);
			EMBX_Deinit();
			exit(1);
		}
	}

	return 0;
}

