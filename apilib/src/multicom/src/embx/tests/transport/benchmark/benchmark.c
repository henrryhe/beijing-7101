/*
 * benchmark.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Benchmark EMBX2 roundtrip latancy and uni and bi directional bandwidth
 */

#include "harness.h"

#ifdef CONF_SLOW_SIMULATOR
#define ITERATIONS 500
#else
#define ITERATIONS 10000
#endif

EMBX_TRANSPORT transport;
EMBX_PORT inPort;
EMBX_PORT outPort;

/* get the current clock time and wait for a clock edge */
#define TIME_START(start)   start = harness_getTime(); while(start == harness_getTime())

/* calculate the elapsed time (and compensate for waiting for the clock edge) */
#define TIME_STOP(start)    (harness_getTime() - (start + 1))

void benchmarkPreallocated(char *name, int size) /*{{{*/
{
	int i;
	volatile int start, elapsed;
	EMBX_VOID *buffer;
	EMBX_RECEIVE_EVENT event;

	/* allocate our test buffer */
	EMBX(Alloc(transport, (size ? size : 1), &buffer));

	/* untimed round trip to populate the cache and ensure our partner
	 * has booted and everying is loaded
	 */
	EMBX(SendMessage(outPort, buffer, size));
	EMBX(ReceiveBlock(inPort, &event));
	assert(EMBX_REC_MESSAGE == event.type);
	assert(          buffer == event.data);
	assert(            size == event.size);

	/* benchmark simple round tripping */
	TIME_START(start);
	for (i=0; i<ITERATIONS; i++) {
		(void) EMBX_SendMessage(outPort, buffer, size);
		(void) EMBX_ReceiveBlock(inPort, &event);
	}
	elapsed = (int) ((double) TIME_STOP(start) * 1000000.0 / 
	                 (double) harness_getTicksPerSecond());

	printf("  %-56s%6d.%03d%10d\n", name, 
			elapsed/1000000, elapsed / 1000 % 1000,
			(unsigned) ((double) elapsed / (double) ITERATIONS));

	/* run it again with all the asserts */
	EMBX(SendMessage(outPort, buffer, size));
	EMBX(ReceiveBlock(inPort, &event));
	assert(EMBX_REC_MESSAGE == event.type);
	assert(          buffer == event.data);
	assert(            size == event.size);

	EMBX(Free(buffer));
}
/*}}}*/
void benchmarkAllocation(char *name, int size) /*{{{*/
{
	int i;
	volatile int start, elapsed;
	EMBX_VOID *buffer;

	/* check we can actually satisfy this allocation (this will also 
	 * eliminate the one-off breaking delay)
	 */
	EMBX(Alloc(transport, size, &buffer));
	EMBX(Free(buffer));

	/* perform the actual benchmark */
	TIME_START(start);
	for (i=0; i<ITERATIONS; i++) {
		(void) EMBX_Alloc(transport, size, &buffer);
		(void) EMBX_Free(buffer);
	}
	elapsed = (int) ((double) TIME_STOP(start) * 1000000.0 / 
	                 (double) harness_getTicksPerSecond());

	printf("  %-56s%6d.%03d%10d\n", name, 
			elapsed/1000000, elapsed / 1000 % 1000,
			(unsigned) ((double) elapsed / (double) ITERATIONS));

	/* double check we can still satisfy the allocation */
	EMBX(Alloc(transport, 512*1024, &buffer));
	EMBX(Free(buffer));
}
/*}}}*/
void benchmarkComplete(char *name, int size) /*{{{*/
{
	int i;
	volatile int start, elapsed;
	EMBX_VOID *buffer;
	EMBX_RECEIVE_EVENT event;

	/* untimed round trip to populate the cache and ensure our partner
	 * has booted and everying is loaded
	 */
	EMBX(Alloc(transport, size, &buffer));
	EMBX(SendMessage(outPort, buffer, size));
	EMBX(ReceiveBlock(inPort, &event));
	assert(EMBX_REC_MESSAGE == event.type);
	assert(          buffer == event.data);
	assert(            size == event.size);
	EMBX(Free(buffer));

	/* benchmark simple round tripping */
	TIME_START(start);
	for (i=0; i<ITERATIONS; i++) {
		(void) EMBX_Alloc(transport, size, &buffer);
		(void) EMBX_SendMessage(outPort, buffer, size);
		(void) EMBX_ReceiveBlock(inPort, &event);
		(void) EMBX_Free(buffer);
	}
	elapsed = (int) ((double) TIME_STOP(start) * 1000000.0 / 
	                 (double) harness_getTicksPerSecond());

	printf("  %-56s%6d.%03d%10d\n", name, 
			elapsed/1000000, elapsed / 1000 % 1000,
			(unsigned) ((double) elapsed / (double) ITERATIONS));

	/* run it again with all the asserts */
	EMBX(Alloc(transport, size, &buffer));
	EMBX(SendMessage(outPort, buffer, size));
	EMBX(ReceiveBlock(inPort, &event));
	assert(EMBX_REC_MESSAGE == event.type);
	assert(          buffer == event.data);
	assert(            size == event.size);
	EMBX(Free(buffer));
}
/*}}}*/
void benchmarkRegistration(char *name, void *p, int size) /*{{{*/
{
	int i;
	volatile int start, elapsed;
	EMBX_HANDLE handle;

	EMBX(RegisterObject(transport, p, size, &handle));
	EMBX(DeregisterObject(transport, handle));

	/* perform the actual benchmark */
	TIME_START(start);
	for (i=0; i<ITERATIONS; i++) {
		(void) EMBX(RegisterObject(transport, p, size, &handle));
		(void) EMBX(DeregisterObject(transport, handle));
	}
	elapsed = (int) ((double) TIME_STOP(start) * 1000000.0 / 
	                 (double) harness_getTicksPerSecond());

	printf("  %-56s%6d.%03d%10d\n", name, 
			elapsed/1000000, elapsed / 1000 % 1000,
			(unsigned) ((double) elapsed / (double) ITERATIONS));

	EMBX(RegisterObject(transport, p, size, &handle));
	EMBX(DeregisterObject(transport, handle));
}
/*}}}*/
void benchmarkMaster() /*{{{*/
{
	EMBX_VOID *buffer;
	EMBX_RECEIVE_EVENT event;

	printf("******** benchmarking with %d iterations ********\n", ITERATIONS);
	printf("\n  %-56s%10s%10s\n\n", "TEST", "TIME/S", "ITER/uS");

	/* benchmark simple round tripping */
	benchmarkPreallocated("Round trip with pre-allocated buffer (empty)", 0);
	benchmarkPreallocated("Round trip with pre-allocated buffer (8 bytes)", 8);
	benchmarkPreallocated("Round trip with pre-allocated buffer (256 bytes)", 256);
	benchmarkPreallocated("Round trip with pre-allocated buffer (8K)", 8 * 1024);
	benchmarkPreallocated("Round trip with pre-allocated buffer (128K)", 128 * 1024);

	/* benchmark allocation */
	benchmarkAllocation("Best case allocation (8 bytes)", 8);
	benchmarkAllocation("Best case allocation (256 bytes)", 256);
	benchmarkAllocation("Best case allocation (8K)", 128 * 1024);
	benchmarkAllocation("Best case allocation (128K)", 128 * 1024);

	/* benchmark round tripping with allocation in the inner loop */
	benchmarkComplete("Round trip with allocation in inner loop (8 bytes)", 8);
	benchmarkComplete("Round trip with allocation in inner loop (256 bytes)", 256);
	benchmarkComplete("Round trip with allocation in inner loop (8K)", 8 * 1024);
	benchmarkComplete("Round trip with allocation in inner loop (128K)", 128 * 1024);

	/* benchmark object registration/deregistration */
	EMBX(Alloc(transport, 256, &buffer));
	benchmarkRegistration("EMBX object registration/deregistration (256 bytes)", buffer, 256);
	EMBX(Free(buffer));
	buffer = malloc(256);
	benchmarkRegistration("Heap object registration/deregistration (256 bytes)", buffer, 256);
	free(buffer);

	/* this coded signal gets our partner to exit */
	EMBX(Alloc(transport, 4, &buffer));
	EMBX(SendMessage(outPort, buffer, 4));
	EMBX(ReceiveBlock(inPort, &event));
	assert(EMBX_REC_MESSAGE == event.type);
	assert(          buffer == event.data);
	assert(               4 == event.size);
	EMBX(Free(buffer));

	printf("\n");
}
/*}}}*/
void benchmarkSlave() /*{{{ */
{
	EMBX_RECEIVE_EVENT event;

	/* untimed round trip to populate the cache (for which we have
	 * assertions etc enabled)
	 */
	EMBX(ReceiveBlock(inPort, &event));
	assert(EMBX_REC_MESSAGE == event.type);
	EMBX(SendMessage(outPort, event.data, event.size));

	/* benchmark loop for benchmarking uninitialized data */
	while (event.size != 4) {
		/* but now we must run with no external error checking */
		EMBX_ReceiveBlock(inPort, &event);
		EMBX_SendMessage(outPort, event.data, event.size);
	}
}
/*}}}*/

int main(void) /*{{{*/
{
	/* ensure that OS and EMBX are booted properly */
	harness_boot();

	/* open the transport and connect our comms. ports */
	transport = harness_openTransport();
	harness_connect(transport, "benchmark", &inPort, &outPort);
	harness_rendezvous(transport, inPort, outPort);

	MASTER(benchmarkMaster());
	SLAVE (benchmarkSlave());

	EMBX(ClosePort(inPort));
	EMBX(ClosePort(outPort));
	MASTER(harness_passed());
	SLAVE (harness_waitForShutdown());

	return 0;
}
/*}}}*/
