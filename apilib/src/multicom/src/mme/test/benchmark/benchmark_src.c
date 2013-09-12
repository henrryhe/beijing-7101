/*
 * benchmark.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Very simple performance monitoring benchmark for MME.
 */

#include "mme.h"

#include "os_abstractions.h"
#include "harness.h"

#define TRANSPORT_NAME "shm"
#define ITERATIONS     5000
#define TRANSFORMER    "mme.benchmark"

#if defined __LINUX__ && defined __KERNEL__
#define HIGH_PRECISION long /* this is good enough for LKM since HZ is small */
#else
#define HIGH_PRECISION double
#endif

/* get the current clock time and wait for a clock edge */
#define TIME_START(startTime)   startTime = harness_getTime(); while(startTime == harness_getTime())

/* calculate the elapsed time (and compensate for waiting for the clock edge) */
#define TIME_STOP(startTime)    (harness_getTime() - (startTime + 1))

#if defined CONF_MASTER

static OS_SIGNAL callbackReceived;

static void TransformerCallback(MME_Event_t Event, MME_Command_t *CallbackData, void *UserData)
{
	VERBOSE(printf(MACHINE "received callback\n"));
	OS_SIGNAL_POST(&callbackReceived);
}

void run_benchmark(MME_TransformerHandle_t hdl, MME_Command_t *cmd, char *test)
{
	volatile int start, elapsed;
	int i;

	/* send it a 'warm up' message */
	MME(SendCommand(hdl, cmd));
	OS_SIGNAL_WAIT(&callbackReceived);

	TIME_START(start);
	for (i=0; i<ITERATIONS; i++) {
		MME_SendCommand(hdl, cmd);
		OS_SIGNAL_WAIT(&callbackReceived);
	}

	elapsed = (int) ((HIGH_PRECISION) TIME_STOP(start) * (HIGH_PRECISION) 1000000 / (HIGH_PRECISION) harness_getTicksPerSecond());

	printf("  %-56s%6d.%03d%10d\n", test, elapsed/1000000, elapsed / 1000 % 1000,
			(unsigned) ((HIGH_PRECISION) elapsed / (HIGH_PRECISION) ITERATIONS));
}

int main(void)
{
#define K 1024
	struct { 
		char *name; 
		int input;
		int output;
		MME_AllocationFlags_t flags;
		MME_CacheFlags_t flagsIn;
	} schedule[] = {
		{ "Affinity allocated 64k/64k",     64*K,  64*K, 0, 0 },
		{ "Affinity allocated 64k/256k",    64*K, 256*K, 0, 0 },
		{ "Affinity allocated 256k/64k",   256*K,  64*K, 0, 0 },
		{ "Affinity allocated 256k/256k",  256*K, 256*K, 0, 0 },
		{ "Coherent, affinity, 256k/256k", 256*K, 256*K, 0, MME_DATA_CACHE_COHERENT },
		{ "Transient, affinity, 256k/256k",256*K, 256*K, 0, MME_DATA_TRANSIENT },
		{ "Coherent, transient, affinity, 256k/256k", 
			256*K,256*K, 0, MME_DATA_TRANSIENT | MME_DATA_CACHE_COHERENT },
#if 0
		{ "Cached 64k/64k",    64*K,  64*K, MME_ALLOCATION_CACHED },
		{ "Cached 64k/256k",   64*K, 256*K, MME_ALLOCATION_CACHED },
		{ "Cached 256k/64k",  256*K,  64*K, MME_ALLOCATION_CACHED },
		{ "Cached 256k/256k", 256*K, 256*K, MME_ALLOCATION_CACHED },
		{ "Uncached 64k/64k",   64*K,  64*K,  MME_ALLOCATION_UNCACHED },
		{ "Uncached 64k/256k",   64*K, 256*K, MME_ALLOCATION_UNCACHED },
		{ "Uncached 256k/64k",  256*K,  64*K, MME_ALLOCATION_UNCACHED },
		{ "Uncached 256k/256k", 256*K, 256*K, MME_ALLOCATION_UNCACHED },
#endif
		{ 0 }
	};

	MME_TransformerInitParams_t init = {
		sizeof(MME_TransformerInitParams_t),
		MME_PRIORITY_NORMAL,
		TransformerCallback,
		(void *) 0x0,
		0,
		NULL,
	};
	MME_TransformerHandle_t hdl;
	MME_DataBuffer_t *bufs[2];
	MME_Command_t command = {
		sizeof(MME_Command_t),          /* struct size */
		MME_TRANSFORM,                  /* CmdCode */
		MME_COMMAND_END_RETURN_NOTIFY   /* CmdEnd */
	};
	int i;


	/* bring up the API */
	harness_boot();
	OS_SIGNAL_INIT(&callbackReceived);
	MME(Init());
#if !defined(USERMODE)
	MME(RegisterTransport(TRANSPORT_NAME));
#endif
	/* initialize the benchmark transformer */
	while (MME_SUCCESS != MME_I(InitTransformer(TRANSFORMER, &init, &hdl))) {
		harness_sleep(1);
	}

	/* construct our test command */
	command.NumberInputBuffers = 1;
	command.NumberOutputBuffers = 1;
	command.DataBuffers_p = bufs;

	printf("******** benchmarking with %d iterations ********\n", ITERATIONS);
	printf("\n  %-56s%10s%10s\n\n", "TEST", "TIME/S", "ITER/uS");

	for (i=0; schedule[i].name; i++) {
		MME(AllocDataBuffer(hdl, schedule[i].input, schedule[i].flags, bufs+0));
		MME(AllocDataBuffer(hdl, schedule[i].output, schedule[i].flags, bufs+1));
		bufs[0]->ScatterPages_p->FlagsIn = schedule[i].flagsIn;
		bufs[1]->ScatterPages_p->FlagsIn = schedule[i].flagsIn;

		run_benchmark(hdl, &command, schedule[i].name);

		MME(FreeDataBuffer(bufs[0]));
		MME(FreeDataBuffer(bufs[1]));
	}

	printf("\n");

	/* tidy up */
	MME(TermTransformer(hdl));
	MME(Term());
	OS_SIGNAL_DESTROY(&callbackReceived);

	harness_shutdown();
	return 0;
}

#else /* CONF_MASTER */

static MME_ERROR successful(void) { return MME_SUCCESS; }
int main(void)
{
	MME_ERROR (*fn)() = successful;

	harness_boot();

	MME(Init());
	MME(RegisterTransport(TRANSPORT_NAME));
	MME(RegisterTransformer(TRANSFORMER, fn, fn, fn, fn, fn));
	MME(Run());

	harness_shutdown();
	return 0;
}

#endif /* CONF_MASTER */
