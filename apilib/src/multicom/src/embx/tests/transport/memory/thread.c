/*
 * thread.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Fully symetric multi-threaded multi-cpu memory allocation romp!
 */

#include "harness.h"

static volatile int allocations = 0;
#ifdef CONF_SLOW_SIMULATOR
#define MAX_ALLOCATIONS 5000
#else
#define MAX_ALLOCATIONS 100000
#endif
static EMBX_TRANSPORT transport;

#ifdef CONF_MASTER
static const unsigned int marker = 0x11111118;
#else
static const unsigned int marker = 0x22222229;
#endif

static void randomAllocator(int limit, int min, int max)
{
	int status = 0;
	int i;
	int offset = min;
	int range = max - min;
	void **list = malloc(sizeof(*list) * limit);

	assert(list);

	harness_sleep(1);

	do {
		for (i=0; i<limit; i++) {
			EMBX_ERROR err;
			EMBX_UINT size, actualSize;

			/* maintain our status */
			status++;

			/* rand is not thread-safe on all our devices so we
			 * lock out interrupts while set size and record our
			 * allocations
			 */
			harness_interruptLock();
			size = offset + (rand() % range);
			allocations++;
			harness_interruptUnlock();

			/* allocate and give up if we have run out of space */
			err = EMBX_I(Alloc(transport, size, &(list[i])));
			if (EMBX_NOMEM == err) {
				break;
			}
			assert(EMBX_SUCCESS == err);

			EMBX(GetBufferSize(list[i], &actualSize));
			harness_fillPattern(list[i], actualSize, marker);
		}

		/* this test has a long life time so we'd better show that we
		 * still alive
		 */
		if (status > (MAX_ALLOCATIONS / 160)) {
			status = 0;
#if defined CONF_MASTER
			printf("-");
#else
			printf("|");
#endif
		}

		for (i-=1; i>=0; i--) {
			EMBX_UINT size;

			EMBX(GetBufferSize(list[i], &size));
			assert(harness_testPattern(list[i], size, marker));
			EMBX(Free(list[i]));
		}
	} 
#if defined CONF_SOAKTEST
	while (1);
#else
	while (allocations < MAX_ALLOCATIONS);
#endif
}

static void smallAllocator(void *p)
{
	randomAllocator((int) p, 1, 1024);
}

static void largeAllocator(void *p)
{
	randomAllocator((int) p, 1024, 4096);
}

int main(void)
{
	/* get the machine going */
	harness_boot();
	transport = harness_openTransport();

	/* since we are multi-threaded and subject to off-chip timing
	 * differences this will not be sufficient to make this test 
	 * deterministic
	 */
	srand(0);

	/* create the multiple threads that will do our allocating */
	harness_createThread(smallAllocator, (void *) 256);
	harness_createThread(largeAllocator, (void *) 256);
	harness_createThread(smallAllocator, (void *) 16);
	harness_createThread(largeAllocator, (void *) 16);

	/* wait for our threads to complete */
	harness_waitForChildren();

	/* now syncronize with our partner and shutdown */
	printf("\n"); /* pretty printing! */
	harness_shutdown();

	return 0;
}
