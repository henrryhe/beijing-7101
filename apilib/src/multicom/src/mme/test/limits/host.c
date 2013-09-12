/* host.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * This test ensures that resource allocations made as a result of MME_SendCommand
 * are released
 *
 * The test also ensures that allocations not made with MME_AllocDataBuffer
 * can be handled on Linux so long as they are physically contiguous
 */

#ifdef __OS20__
/* The internal partition on OS20 leaks with transient tasks */
#include <partitio.h>
#endif

#include <mme.h>
#include <os_abstractions.h>

#include "harness.h"
#include "params.h"
#include "simple_test.h"
#include "allocate.h"


#ifdef __KERNEL__
/* INSbl23529 */
#define MAX_TRANSFORMERS_INIT_DEINIT 100
#define HIGH_PRECISION long /* this is good enough for LKM since HZ is small */
#else
/* INSbl25388: transformer deinit is too slow; DO NOT REDUCE THIS NUMBER! */
#define MAX_TRANSFORMERS_INIT_DEINIT 10000
#define HIGH_PRECISION double
#endif

#ifdef __WINCE__
/* Due to lack of memroy under WinCE we halve the number of commands but double the reps */
#define MAX_TRANSFORMERS_COMMANDS    16
#define MAX_SETPARAMS_REPS           20000
#define MAX_TRANSFORM_REPS           2000
#else
/* Must match the lookup table size in MME_Init (manager.c) */
#define MAX_TRANSFORMERS_COMMANDS    32
#define MAX_SETPARAMS_REPS           10000
#define MAX_TRANSFORM_REPS           1000
#endif

#include "common.h"

/* get the current clock time and wait for a clock edge */
#define TIME_START(startTime)   startTime = harness_getTime(); while(startTime == harness_getTime())

/* calculate the elapsed time (and compensate for waiting for the clock edge) */
#define TIME_STOP(startTime)    (harness_getTime() - (startTime + 1))

static OS_SIGNAL callbackReceived;

/* template containing default parameters for a command */
static const MME_Command_t commandTemplate = {
	sizeof(MME_Command_t),          /* struct size */
	MME_TRANSFORM,                  /* CmdCode */
	MME_COMMAND_END_RETURN_NOTIFY,  /* CmdEnd */
	(MME_Time_t) 0,                 /* DueTime */
	0,                              /* Num in bufs */
	0,                              /* Num out bufs */
	NULL,                           /* Buffer ptr */
	{
		0,                      /* CmdId */
		MME_COMMAND_IDLE,       /* State */
		0,                      /* ProcessedTime */
		MME_SUCCESS,            /* Error */
		0,                      /* Num params */
		NULL                    /* Params */
	},
	0,                              /* Num params */
	NULL                            /* Params */
};

static void TransformerCallback(MME_Event_t Event, MME_Command_t *CallbackData, void *UserData)
{
	VERBOSE(printf(MACHINE "received callback\n"));

	assert(MME_COMMAND_COMPLETED_EVT == Event);
	assert((void *) 0x78563412 == UserData);

	OS_SIGNAL_POST(&callbackReceived);
}


static  const char* test1InstanceName = TEST1_INSTANCE_NAME;

static void DoTransformerInstantiates(const char* name, MME_TransformerInitParams_t* initParams, unsigned* instantiateCounter) {
	unsigned instantiations;
	MME_TransformerHandle_t transformerHandle;

	/* (busy) wait until the transformer exists */
	while (MME_SUCCESS != MME_InitTransformer(name, initParams, &transformerHandle)) {
		harness_sleep(1);
	}

        /* INSbl23335 - terminate function not called for local transformers */
	assert(0 == instantiateCounter || 1 == *instantiateCounter);
	MME(TermTransformer(transformerHandle));
	assert(0 == instantiateCounter || 0 == *instantiateCounter);
	
	/* Instantiate and release lots of transformers */
	printf(MACHINE "Doing %d transformer instantiation/terminates\n", 
	       MAX_TRANSFORMERS_INIT_DEINIT);
        for (instantiations=0; instantiations<MAX_TRANSFORMERS_INIT_DEINIT; instantiations++) {
	        MME(InitTransformer(name, initParams, &transformerHandle));
	        MME(TermTransformer(transformerHandle));

		if (1 == instantiations % ((MAX_TRANSFORMERS_INIT_DEINIT / 80)+1)) {
			printf("#");
			fflush(stdout);
		}
        }
	printf("\n");
}

static void DoGetTransformerCapabilities(const char *name)
{
	unsigned instantiations;
	MME_TransformerCapability_t capability = { sizeof(MME_TransformerCapability_t) };

	/* (busy) wait until the transformer exists */
	while (MME_SUCCESS != MME_GetTransformerCapability(name, &capability)) {
		harness_sleep(1);
	}

	/* Instantiate and release lots of transformers */
	printf(MACHINE "Doing %d get transformer capabilities\n", 
	       MAX_TRANSFORMERS_INIT_DEINIT);
        for (instantiations=0; instantiations<MAX_TRANSFORMERS_INIT_DEINIT; instantiations++) {
		MME(GetTransformerCapability(name, &capability));

		if (1 == instantiations % ((MAX_TRANSFORMERS_INIT_DEINIT / 80)+1)) {
			printf("#");
			fflush(stdout);
		}
        }
	printf("\n");
}

static void DoInvalidSends(MME_TransformerHandle_t handle) {
	MME_ERROR err;
	MME_Command_t command = commandTemplate;
	MME_DataBuffer_t* buffer;
	void *old;

	MME(AllocDataBuffer(handle, 1024 * 128, 0, &buffer));
	old = buffer->ScatterPages_p->Page_p;

        command.NumberInputBuffers = 1;
        command.NumberOutputBuffers = 0;
        command.DataBuffers_p = &buffer;
        command.CmdCode = MME_TRANSFORM;

 	buffer->ScatterPages_p->Page_p = (void*)0x12345678;
        MME_E(MME_INTERNAL_ERROR, SendCommand(handle, &command));

	/* INSbl23505 - ensure non-contig pages cause failure 
         * Important - we make the big assumption (based on observation
         * on current platforms) that a large malloc (128k) 
         * will be physically non-contiguous
         */
	buffer->ScatterPages_p->Page_p = malloc(1024 * 128);
        MME_E(MME_INTERNAL_ERROR, SendCommand(handle, &command));
 	free(buffer->ScatterPages_p->Page_p);
	buffer->ScatterPages_p->Page_p = old;
	
	MME_FreeDataBuffer(buffer);
}

static void DoInstantiateAndSend(const char* name, MME_TransformerInitParams_t* initParams, int remoteOrKernelTransformer) {
	static unsigned transformCount;
	unsigned instantiations, reps, i;

	MME_TransformerHandle_t transformerHandles[MAX_TRANSFORMERS_COMMANDS];

	unsigned char*          inputBytes;
	unsigned char*          outputBytes;
	unsigned char*          expectBytes;

	MME_Command_t		command               = commandTemplate;
	MME_Command_t		tranformParamsCommand = commandTemplate;

	/* This type is so large that under WinCE there is not enough stack for
	 * it to be a local variable. To work around this we just declare the
	 * offending variable as static.
	 */
	static SimpleParamsTransform_t	simpleParamsTransform;
	SimpleParamsStatus_t	simpleParamsStatus;

        volatile int start, elapsed;

	printf(MACHINE "Doing %d instantiations with set params and transforms \n", 
	       MAX_TRANSFORMERS_COMMANDS);
	/* InstantiateN transformers, send them commands, and terminate them all */
        for (instantiations=0; instantiations<MAX_TRANSFORMERS_COMMANDS; instantiations++) {

	        MME(InitTransformer(name, initParams, &transformerHandles[instantiations]));
        }

	printf(MACHINE "Sending %d set params\n", MAX_SETPARAMS_REPS);

        for (reps=0; reps<MAX_SETPARAMS_REPS; reps++) {

	        /* Send global transform params */
	        MME_PARAM(simpleParamsTransform, SimpleParamsTransform_repetitionsA) = 1;
	        MME_PARAM(simpleParamsTransform, SimpleParamsTransform_repetitionsB) = 1;

	        /* Send tranaform params */
		/* INSbl23110 - pass significant sized params iteratively in an
                 *  attempt to detect memory leaks in the kernel
                 */
	        tranformParamsCommand.CmdCode = MME_SET_GLOBAL_TRANSFORM_PARAMS;
	        tranformParamsCommand.Param_p = (void*) &simpleParamsTransform;
	        tranformParamsCommand.ParamSize = sizeof(simpleParamsTransform);

	        MME(SendCommand(transformerHandles[reps % MAX_TRANSFORMERS_COMMANDS], &tranformParamsCommand));
	        VERBOSE(printf(MACHINE "waiting for send params callback ...\n"));
	        OS_SIGNAL_WAIT(&callbackReceived);

		if (1 == reps % ((MAX_SETPARAMS_REPS / 80)+1)) {
			printf("#");
			fflush(stdout);
		}
        }
	printf("\n");

	/* Send commands
          Use multiple scatter pages
          Use MME_AllocDataBuffer and 'implicit allocation'
        */

	inputBytes = malloc(MAX_ALLOCATION); assert(inputBytes);
	outputBytes = malloc(MAX_ALLOCATION); assert(outputBytes);
	expectBytes = malloc(MAX_ALLOCATION); assert(expectBytes);

        for (i=0; i<MAX_ALLOCATION; i++) {
               	inputBytes[i] = outputBytes[i] = i % 256;
		expectBytes[i] = TRANSFORM(inputBytes[i]);
        }

#ifdef USERMODE
	if (remoteOrKernelTransformer) {
	        DoInvalidSends(transformerHandles[0]);
	}
#endif
	printf(MACHINE "Sending %d transforms\n", MAX_TRANSFORM_REPS);

        TIME_START(start);

        for (reps=0; reps<MAX_TRANSFORM_REPS; reps++) {
	       	MME_DataBuffer_t*       bufferPtrs[2];
        	AllocInfo_t*            allocateInfo[2];
        	unsigned                cached = 0;

		MME_TransformerHandle_t handle = transformerHandles[reps % MAX_TRANSFORMERS_COMMANDS];

  	        /* send a command and wait for it to complete */
	        command.CmdStatus.AdditionalInfo_p = (void*) &simpleParamsStatus;
	        command.CmdStatus.AdditionalInfoSize = MME_LENGTH_BYTES(SimpleParamsStatus);
	        command.Param_p = NULL;

                /* Alloc data buffers */
		VERBOSE(printf(MACHINE "Allocating 0 with MME_AllocDataBuffer cached %s\n", 
		               cached?"YES":"NO"));
                allocateInfo[0] = AllocBuffer(0, handle, MAX_ALLOCATION, cached);
		VERBOSE(printf(MACHINE "Allocating 1 with KernelAlloc cached %s\n", 
		               cached?"YES":"NO"));
                allocateInfo[1] = AllocBuffer(1, handle, MAX_ALLOCATION, cached);

                bufferPtrs[0] = allocateInfo[0]->dataBuffer;
                bufferPtrs[1] = allocateInfo[1]->dataBuffer;

		VERBOSE(printf(MACHINE "Filling 0\n"));
                FillBuffer(bufferPtrs[0], inputBytes, MAX_ALLOCATION);
		VERBOSE(printf(MACHINE "Filling 1\n"));
                FillBuffer(bufferPtrs[1], outputBytes, MAX_ALLOCATION);

	        command.NumberInputBuffers = 1;
	        command.NumberOutputBuffers = 1;
	        command.DataBuffers_p = bufferPtrs;
	        command.CmdCode = MME_TRANSFORM;
	        MME(SendCommand(handle, &command));

                VERBOSE(printf(MACHINE "waiting for send buffers callback ...\n"));
                OS_SIGNAL_WAIT(&callbackReceived);

                /* check that the output is correct */
                {
                        ExtractBuffer(bufferPtrs[1], outputBytes, MAX_ALLOCATION);
                        assert(0 == memcmp(expectBytes, outputBytes, MAX_ALLOCATION));
	                assert(MME_COMMAND_COMPLETED == command.CmdStatus.State);
	                assert(MME_SUCCESS == command.CmdStatus.Error);
 	                assert(MME_PARAM(simpleParamsStatus, SimpleParamsStatus_status1)==TEST1_STATUS_INT);
 	                assert(MME_PARAM(simpleParamsStatus, SimpleParamsStatus_status2)==TEST1_STATUS_CHAR);
                }

                FreeBuffer(allocateInfo[0]);
                FreeBuffer(allocateInfo[1]);

		if (1 == reps % ((MAX_TRANSFORM_REPS / 80)+1)) {
			printf("#");
			fflush(stdout);
		}
        }
	printf("\n");

        elapsed = (int) ((HIGH_PRECISION) TIME_STOP(start) * (HIGH_PRECISION) 1000000 / (HIGH_PRECISION) harness_getTicksPerSecond());

	printf("  %-56s%6d.%03d%10d\n", name,
               elapsed/1000000, 
               elapsed / 1000 % 1000,
	       (unsigned) ((HIGH_PRECISION) elapsed / (HIGH_PRECISION) reps));

	/* Terminate all those transformers */
        for (instantiations=0; instantiations<MAX_TRANSFORMERS_COMMANDS; instantiations++) {
	        MME(TermTransformer(transformerHandles[instantiations]));
	}
	free(inputBytes); 
	free(outputBytes);
	free(expectBytes);
}

unsigned instantiatedCounter;

int main(void)
{
	MME_TransformerInitParams_t initParams = {
		sizeof(MME_TransformerInitParams_t),
		MME_PRIORITY_NORMAL,
		TransformerCallback,
		(void *) 0x78563412,
		MME_LENGTH_BYTES(SimpleParamsInit),
		NULL,
	};
	SimpleParamsInit_t	simpleParamsInit;

	int i;

#ifdef __OS20__
        /* The internal partition on OS20 leaks with transient tasks */
        internal_partition = system_partition;
#endif

	/* bring up the API */
	harness_boot();
	MME(Init());

        BufferInit();
	
#ifndef USERMODE
	MME(RegisterTransport(TRANSPORT_NAME));
#endif
	/* Some parameters */
	initParams.TransformerInitParams_p = (void*) &simpleParamsInit;

	MME_PARAM(simpleParamsInit, SimpleParamsInit_instanceType) = TEST1_HANDLE_VALUE;
	assert(strlen(test1InstanceName)+1 <= SimpleParamsInit_instanceNameLength);
	for (i=0; i<strlen(test1InstanceName)+1; i++) {
		MME_INDEXED_PARAM(simpleParamsInit, SimpleParamsInit_instanceName, i) = test1InstanceName[i];
	}

	/* Init the sub list params 
	 * The transformer will assert: valueA == 2*valueB + valueC
         */
	{
		SimpleParamsSubInit_t* subParams = MME_PARAM_SUBLIST(simpleParamsInit, SimpleParamsInit_values);

		MME_PARAM( *subParams, SimpleParamsSubInit_valueC) = 4;
		MME_PARAM( *subParams, SimpleParamsSubInit_valueB) = -2300;
		MME_PARAM( *subParams, SimpleParamsSubInit_valueA) = (long) -4596;
	}

	/* side-effects are acceptable in the test suite (it is pointless
	 * to compile the test suite with NDEBUG since it will eliminate all
	 * the tests).
	 */
	OS_SIGNAL_INIT(&callbackReceived);

	/* Remote transformer tests */
	printf(MACHINE "Doing user remote transfomer tests\n");

	DoGetTransformerCapabilities("com.st.mcdt.mme.test");
	DoTransformerInstantiates("com.st.mcdt.mme.test", &initParams, NULL);
	DoInstantiateAndSend("com.st.mcdt.mme.test", &initParams, 1);

	/* Local transformer tests */
	printf(MACHINE "Doing (user space) local transfomer tests\n");

	MME(RegisterTransformer("local",
				SimpleTest_AbortCommand,
				SimpleTest_GetTransformerCapability,
				SimpleTest_InitTransformer,
				SimpleTest_ProcessCommand,
				SimpleTest_TermTransformer));

	DoGetTransformerCapabilities("local");
	DoTransformerInstantiates("local", &initParams, &instantiatedCounter);
	DoInstantiateAndSend("local", &initParams, 0);

	MME_DeregisterTransformer("local");

#ifdef USERMODE
	printf(MACHINE "Doing user space kernel local transfomer tests\n");

	DoGetTransformerCapabilities("com.st.mcdt.mme.local");
	DoTransformerInstantiates("com.st.mcdt.mme.local", &initParams, NULL);
	DoInstantiateAndSend("com.st.mcdt.mme.local", &initParams, 1);

        /* Remove the local transformer prior to terminating
           - which tries to remove the mme_host module
         */
        system("/sbin/rmmod local.ko");

#endif

#ifndef USERMODE
        /* Do not allow the transport to deregister untill all tests have run 
           else on mb376_reversed the companion terminates and this terminates
           multirun with success
        */
	MME(DeregisterTransport(TRANSPORT_NAME));
#endif

	MME(Term());
	OS_SIGNAL_DESTROY(&callbackReceived);

	BufferDeinit();

	harness_shutdown();
	return 0;
}
