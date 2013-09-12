/*
 * size.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Fully symetric test of GetBufferSize and it's friends
 */

#include "harness.h"

static EMBX_TRANSPORT transport;
static EMBX_PORT      inPort, outPort;

int main(void)
{
	EMBX_UINT size;

	harness_boot();
	transport = harness_openTransport();
	harness_connect(transport, "size", &inPort, &outPort);

	for (size=1; size<=4096; size++) {
		EMBX_UINT           actualSize;
		EMBX_UINT          *buffer;
		EMBX_RECEIVE_EVENT  event;
		EMBX_UINT           i;

		MASTER(if (0 == (size & 31)) printf(MACHINE "%d of 4096\n", size));

		/* allocate a buffer and ensure it as at least as
		 * big as we asked for
		 */
		EMBX(Alloc(transport, size, (void *) &buffer));
		VERBOSE(printf(MACHINE ": buffer = 0x%08x\n", (unsigned) buffer));
		EMBX(GetBufferSize(buffer, &actualSize));
		assert(actualSize >= size);

		/* fill all the memory we can with a small but non-NULL
		 * value 
		 */
		for (i=0; i<(actualSize/4); i++) {
			buffer[i] = 0x00000bad;
		}

		/* send a byte longer than the actual size and check it fails */
		EMBX_E(EMBX_INVALID_ARGUMENT, SendMessage(outPort, buffer, actualSize+1));

		/* now swap buffers with our partner */
		EMBX(SendMessage(outPort, buffer, actualSize));
		EMBX(ReceiveBlock(inPort, &event));
		buffer = event.data;

		/* check it is still the same actual size now that
		 * we have it (otherwise buffers could grow indefinately
		 * as they are transfered from one side to another)
		 */
		EMBX(GetBufferSize(buffer, &actualSize));
		assert(actualSize >= size);
		assert(actualSize == event.size);

		/* check the fill pattern */
		for (i=0; i<(actualSize/4); i++) {
			assert(0x00000bad == buffer[i]);
		}

		/* free the block and try the next size */
		EMBX(Free(buffer));
	}
	
	/* clean up and shutdown */
	EMBX(ClosePort(inPort));
	EMBX(ClosePort(outPort));
	harness_shutdown();

	return 0;
}
