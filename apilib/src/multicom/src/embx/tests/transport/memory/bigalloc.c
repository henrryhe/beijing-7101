/*
 * bigalloc.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Serialized symetric test to try and break the memory allocators but 
 * issuing very large requests.
 */

#include "harness.h"

int main(void)
{
	EMBX_TRANSPORT  transport;
	EMBX_PORT       ctrlPort;
	EMBX_VOID      *buffer;
	EMBX_TPINFO     info;
	EMBX_UINT       maxAlloc;

	harness_boot();
	transport = harness_openTransport();

	/* create the control port */
	MASTER(EMBX(CreatePort(transport, "ctrlPort", &ctrlPort)));
	SLAVE(EMBX(ConnectBlock(transport, "ctrlPort", &ctrlPort)));

	/* perform a test signal (if our memory allocator does not
	 * completely join blocks when they are deallocated it is
	 * possible that this would prevent us from signaling when
	 * we have completed the test)
	 */
	MASTER(harness_waitForSignal(ctrlPort));
	SLAVE(harness_sendSignal(transport, ctrlPort));

	/* now wait for the slave to finish running its memory test */
	MASTER(harness_waitForSignal(ctrlPort));

	/* maximum allocation ammount */
	EMBX_E(EMBX_NOMEM, Alloc(transport, (EMBX_UINT) -1, &buffer));

	/* large allocations with the sign bit set (assuming 32-bit machines) */
	EMBX_E(EMBX_NOMEM, Alloc(transport, 0xc0000000, &buffer));
	EMBX_E(EMBX_NOMEM, Alloc(transport, 0x80000000, &buffer));

	/* large allocations without the sign bit set */
	EMBX_E(EMBX_NOMEM, Alloc(transport, 0x7fffffff, &buffer));
	EMBX_E(EMBX_NOMEM, Alloc(transport, 0x40000000, &buffer));
	EMBX_E(EMBX_NOMEM, Alloc(transport, 0x20000000, &buffer));

	/* a single byte more then than the maximum */
	EMBX(GetTransportInfo(transport, &info));
	maxAlloc = (EMBX_UINT) info.memEnd - (EMBX_UINT) info.memStart;
	printf("Theoretic maxAlloc = %d [0x%08x]\n", maxAlloc, maxAlloc);
	EMBX_E(EMBX_NOMEM, Alloc(transport, maxAlloc + 1, &buffer));


/* this fails on EMBXSHM because its heap manager cannot perform break
 * reclaimation resulting in failure to allocate the memory required
 * to shutdown
 */
#if 0
	/* now reduce maxAlloc until we can actually allocate something */
	for (maxAlloc &= ~3;
	     EMBX_NOMEM == EMBX_I(Alloc(transport, maxAlloc, &buffer));
	     maxAlloc -= 4);
	assert(EMBX_SUCCESS == lastError);
	printf("Actual maxAlloc = %d [0x%08x]\n", maxAlloc, maxAlloc);

	/* populate the entire block with NULL values before we free it
	 * in the optimistic hope that this will pick up a mis-allocation
	 */
	memset(buffer, 0, maxAlloc);
	EMBX(Free(buffer));
#endif

	/* signal the master to start testing */
	SLAVE(harness_sendSignal(transport, ctrlPort));

	/* clean up and shutdown */
	EMBX(ClosePort(ctrlPort));
	harness_shutdown();

	return 0;
}

