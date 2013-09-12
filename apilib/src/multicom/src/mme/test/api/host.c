/*
 * host.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * API conformance test (host side)
 */

/*{{{ GUIDELINES FOR WORKING WITH THESE TESTS */
#if 0

1. Do not mess with the fold markers!

2. To obtain a complete list of tests performed by this file:
	grep '/\* *[{]{{.*:' host.c | cut -d{ -f4 | sed -e 's: \*\/.*$::' | sed -e 's:^ ::' | sort

3. Test in this last marked TODO are not implemented yet.

4. When adding a test use to following form:
	/*{{{ <function-under-test>: <one-line-description-of-the-test> */
	MME(DoTheTest());
	/*}}}*/
	/*{{{ <next-function>: <next-description> */
	MME(DoTheNextTest());
	/*}}}*/
#endif
/*}}}*/

/*{{{ includes */
/* this test has a very long runtime so we'll make sure it keeps jabbering */
#ifndef DEBUG
#define DEBUG
#endif

#include "harness.h"
#include <os_abstractions.h>

#include "mme.h"
#include "streaming.h"
#include "transformer.h"
/*}}}*/
/*{{{ statics */
#define STARTOFFSET 0
static OS_SIGNAL callbackReceived;
static int callbackCount;
static const char* test1InstanceName = TEST1_INSTANCE_NAME;

/* staleHandle is always in invalid handle, either because it is 0 (defined to be a bad
 * handle or because it is a handle to a terminated transformer)
 */
static MME_TransformerHandle_t staleHandle = 0;
/*}}}*/

/*{{{ template containing default parameters for a command */
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
/*}}}*/
/*{{{ template containing default parameters for data buffers */
static const MME_DataBuffer_t dataBufferTemplate = {
	sizeof(MME_DataBuffer_t), NULL, 0, 0, 1, NULL, 0, STARTOFFSET
};
/*}}}*/
/*{{{ template for a scatter page (a bit simple this one) */
static const MME_ScatterPage_t scatterPageTemplate = { 0, 0, 0, 0 };
/*}}}*/

/*{{{ the default transformer callback */
static void transformer_callback(MME_Event_t Event, MME_Command_t *CallbackData, void *UserData)
{
	VERBOSE(printf(MACHINE "received callback\n"));

	assert(MME_COMMAND_COMPLETED_EVT == Event);
	assert((void *) 0x78563412 == UserData);

	callbackCount++;
	OS_SIGNAL_POST(&callbackReceived);
}
/*}}}*/
/*{{{ routine to wait for a transformer callback */
static void callback_wait(void)
{
	VERBOSE(printf(MACHINE "waiting for callback ...\n"));
	OS_SIGNAL_WAIT(&callbackReceived);
}
/*}}}*/

/*{{{ check all functions to ensure rejection prior to initialization */
static void test_rejection_prior_to_initialization(void)
{
	/*{{{ MME_AbortCommand: rejection prior to initialization */
	MME_E(MME_DRIVER_NOT_INITIALIZED,AbortCommand(0, 0));
	/*}}}*/
	/*{{{ MME_AllocDataBuffer: rejection prior to initialization */
	MME_E(MME_DRIVER_NOT_INITIALIZED,AllocDataBuffer(0, 0, 0, 0));
	/*}}}*/
	/*{{{ MME_DeregisterTransformer: rejection prior to initialization */
	MME_E(MME_DRIVER_NOT_INITIALIZED,DeregisterTransformer(0));
	/*}}}*/
	/*{{{ MME_FreeDataBuffer: rejection prior to initialization */
	MME_E(MME_DRIVER_NOT_INITIALIZED,FreeDataBuffer(0));
	/*}}}*/
	/*{{{ MME_GetTransformerCapability: rejection prior to initialization */
	MME_E(MME_DRIVER_NOT_INITIALIZED,GetTransformerCapability(0, 0));
	/*}}}*/
	/*{{{ MME_InitTransformer: rejection prior to initialization */
	MME_E(MME_DRIVER_NOT_INITIALIZED,InitTransformer(0, 0, 0));
	/*}}}*/
	/*{{{ MME_InitTransformer: rejection prior to initialization */
	MME_E(MME_DRIVER_NOT_INITIALIZED,InitTransformer(0, 0, 0));
	/*}}}*/
	/*{{{ MME_RegisterTransformer: rejection prior to initialization */
	MME_E(MME_DRIVER_NOT_INITIALIZED,RegisterTransformer(0, 0, 0, 0, 0, 0));
	/*}}}*/
	/*{{{ MME_SendCommand: rejection prior to initialization */
	MME_E(MME_DRIVER_NOT_INITIALIZED,SendCommand(0, 0));
	/*}}}*/
	/*{{{ MME_Term: rejection prior to initialization */
	MME_E(MME_DRIVER_NOT_INITIALIZED,Term());
	/*}}}*/
	/*{{{ MME_TermTransformer: rejection prior to initialization */
	MME_E(MME_DRIVER_NOT_INITIALIZED,TermTransformer(0));
	/*}}}*/
	/*{{{ MME_WaitCommand: rejection prior to initialization */
#ifdef MME_WaitCommand
	MME_E(MME_DRIVER_NOT_INITIALIZED,WaitCommand());
#endif
	/*}}}*/
#ifndef USERMODE
	/*{{{ MME_RegisterTransport: rejection prior to initialization */
	MME_E(MME_DRIVER_NOT_INITIALIZED,RegisterTransport(0));
	/*}}}*/
	/*{{{ MME_DeregisterTransport: rejection prior to initialization */
	MME_E(MME_DRIVER_NOT_INITIALIZED,DeregisterTransport(0));
	/*}}}*/
#else /* USERMODE */
	/*{{{ MME_RegisterTransport: not implemented */
	MME_E(MME_NOT_IMPLEMENTED,RegisterTransport(0));
	/*}}}*/
	/*{{{ MME_DeregisterTranport: not implemented */
	MME_E(MME_NOT_IMPLEMENTED,DeregisterTransport(0));
	/*}}}*/
#endif /* USERMODE */
}
/*}}}*/
/*{{{ check all initialization and registration functions */
static void test_init_and_register(void)
{
	/*{{{ MME_Init: successful initialization */
	MME(Init());
	/*}}}*/
	/*{{{ MME_Init: rejection of second initialization attempt */
	MME_E(MME_DRIVER_ALREADY_INITIALIZED,Init());
	/*}}}*/

	/*{{{ MME_RegisterTransformer: rejection of NULL pointer */
	MME_E(MME_INVALID_ARGUMENT, RegisterTransformer(0, SimpleTest_AbortCommand,
				SimpleTest_GetTransformerCapability, SimpleTest_InitTransformer,
				SimpleTest_ProcessCommand, SimpleTest_TermTransformer));
	/*}}}*/
	/*{{{ MME_RegisterTransformer: successful registration */
	MME(RegisterTransformer("mme.test", SimpleTest_AbortCommand,
				SimpleTest_GetTransformerCapability, SimpleTest_InitTransformer,
				SimpleTest_ProcessCommand, SimpleTest_TermTransformer));
	/*}}}*/
	/*{{{ MME_RegisterTransformer: rejection of already registered transformer */
	MME_E(MME_INVALID_ARGUMENT,RegisterTransformer("mme.test", SimpleTest_AbortCommand,
				SimpleTest_GetTransformerCapability, SimpleTest_InitTransformer,
				SimpleTest_ProcessCommand, SimpleTest_TermTransformer));
	/*}}}*/
	/*{{{ MME_DeregisterTransformer: rejection of NULL pointer */
	MME_E(MME_INVALID_ARGUMENT, DeregisterTransformer(0));
	/*}}}*/
	/*{{{ MME_DeregisterTransformer: successful deregistration */
	MME(DeregisterTransformer("mme.test"));
	/*}}}*/
	/*{{{ MME_DeregisterTransformer: rejection of invalid and deregistered transformers */
	MME_E(MME_INVALID_ARGUMENT, DeregisterTransformer("mme.unknown"));
	MME_E(MME_INVALID_ARGUMENT, DeregisterTransformer("mme.test"));
	/*}}}*/
#ifndef USERMODE
        /*{{{ MME_RegisterTransport: rejection of NULL pointer */
	MME_E(MME_INVALID_ARGUMENT,RegisterTransport(0));
	/*}}}*/
	/*{{{ MME_RegisterTransport: successful registration */
	MME(RegisterTransport(TRANSPORT_NAME));
	/*}}}*/
	/*{{{ MME_RegisterTransport: rejection of already registered transport */
	MME_E(MME_INVALID_ARGUMENT,RegisterTransport(TRANSPORT_NAME));
	/*}}}*/
	/*{{{ MME_DeregisterTransport: rejection of NULL pointer */
	MME_E(MME_INVALID_ARGUMENT,DeregisterTransport(0));
	/*}}}*/
	/*{{{ MME_DeregisterTransport: successful deregistration of previously registered transport */
#ifdef INSbl21633
	MME(DeregisterTransport(TRANSPORT_NAME));
#endif
	/*}}}*/
	/*{{{ MME_DeregisterTransport: rejection of both invalid and already deregistered transport */
#ifdef INSbl21633
	MME_E(MME_INVALID_ARGUMENT,DeregisterTransport(TRANSPORT_NAME));
#endif
	MME_E(MME_INVALID_ARGUMENT,DeregisterTransport("incorrect_value"));
	/*}}}*/
#endif /* USERMODE */
}
/*}}}*/
/*{{{ check MME_InitTransformer and return a transformer handle for other tests */
static MME_TransformerHandle_t test_init_transformer(void)
{
	/*{{{ locals */
	MME_TransformerInitParams_t badParams = { 0 };
	SimpleParamsInit_t simpleParamsInit;
	MME_TransformerInitParams_t goodParams = {
		sizeof(MME_TransformerInitParams_t),
		MME_PRIORITY_NORMAL,
		transformer_callback,
		(void *) 0x78563412,
		MME_LENGTH_BYTES(SimpleParamsInit),
		NULL
	};
	MME_TransformerHandle_t hdl, tmp;
	int i;
	/*}}}*/

	/*{{{ MME_InitTransformer: rejection of invalid transformers */
	MME_E(MME_UNKNOWN_TRANSFORMER,InitTransformer("mme.unknown", &goodParams, &hdl));
	/*}}}*/
	/*{{{ MME_InitTransformer: rejection of NULL pointer (all three arguments) */
	MME_E(MME_INVALID_ARGUMENT,InitTransformer(0, &goodParams, &hdl));
	MME_E(MME_INVALID_ARGUMENT,InitTransformer("mme.test", 0, &hdl));
	MME_E(MME_INVALID_ARGUMENT,InitTransformer("mme.test", &goodParams, 0));
	/*}}}*/
	/*{{{ MME_InitTransformer: rejection of incorrectly sized parameter structure */
	MME_E(MME_INVALID_ARGUMENT,InitTransformer("mme.test", &badParams, &hdl));
	/*}}}*/
	/*{{{ MME_InitTransformer: propagation of rejection from remote transformer */
	badParams = goodParams;
	badParams.TransformerInitParamsSize = 0;

	/* this is the first test to require the presense of the companion so we must loop
	 * until we get the result we require to allow it time to boot.
	 */
	while (MME_INVALID_ARGUMENT != MME_I(InitTransformer("mme.test", &badParams, &hdl))) {
		harness_sleep(1);
	}

	/*}}}*/
	/*{{{ MME_InitTransformer: successful initialization of remote transformer */
	goodParams.TransformerInitParams_p = (void*) &simpleParamsInit;
	MME_PARAM(simpleParamsInit, SimpleParamsInit_instanceType) = TEST1_HANDLE_VALUE;
	assert(strlen(test1InstanceName)+1 <= SimpleParamsInit_instanceNameLength);
	for (i=0; i<strlen(test1InstanceName)+1; i++) {
		MME_INDEXED_PARAM(simpleParamsInit, SimpleParamsInit_instanceName, i) = 
				test1InstanceName[i];
	}
	MME(InitTransformer("mme.test", &goodParams, &hdl));
	/*}}}*/
	/*{{{ MME_InitTransformer: successful initialization of second remote transformer */
	MME(InitTransformer("mme.test", &goodParams, &tmp));
	/* TODO: we should check we can actually issue a command on the temporary handle */
	MME(TermTransformer(tmp));

	/* we cannot assign this any earlier since staleHandle is never allowed to be valid! */
	staleHandle = tmp;
	/*}}}*/
	/*{{{ MME_InitTransformer: TODO - propagation of rejection from local transformer */
#if 0
	badParams = goodParams;
	badParams.TransformerInitParamsSize = 0;
#ifdef INSbl21640
	MME_E(MME_INVALID_ARGUMENT, InitTransformer("mme.local", &badParams, &hdl));
#else
	MME_E(MME_TRANSFORMER_NOT_RESPONDING, 
	      InitTransformer("mme.test", &badParams, &hdl));
#endif
#endif
	/*}}}*/
	/*{{{ MME_InitTransformer: TODO - successful initialization of local transformer */
#if 0
	goodParams.TransformerInitParams_p = (void*) &simpleParamsInit;
	MME_PARAM(simpleParamsInit, SimpleParamsInit_instanceType) = TEST1_HANDLE_VALUE;
	assert(strlen(test1InstanceName)+1 <= SimpleParamsInit_instanceNameLength);
	for (i=0; i<strlen(test1InstanceName)+1; i++) {
		MME_INDEXED_PARAM(simpleParamsInit, SimpleParamsInit_instanceName, i) = 
				test1InstanceName[i];
	}
	MME(InitTransformer("mme.test", &goodParams, &hdl))) {
#endif
	/*}}}*/
	/*{{{ MME_InitTransformer: TODO - successful initialization of second local transformer */
#if 0
	MME(InitTransformer("mme.local", &goodParams, &tmp));
	/* TODO: we should check we can actually issue a command on the temporary handle */
	MME(TermTransformer(tmp));
#endif
	/*}}}*/
	/*{{{ MME_Term: rejection when transformer handles remain open */

	MME_E(MME_HANDLES_STILL_OPEN, Term());
	/*}}}*/

	return hdl;
}
/*}}}*/
/*{{{ check MME_GetTransformerCapability (requires "mme.test" to be registered) */
static void test_get_capability(void)
{
	/*{{{ locals */
	MME_TransformerCapability_t goodCapability = { sizeof(MME_TransformerCapability_t) };
	MME_TransformerCapability_t badCapability = { 12 };
	MME_TransformerCapability_t paramCapability;
	int params[4];
	/*}}}*/

	/*{{{ MME_GetTransformerCapability: rejection of invalid transformers */
	MME_E(MME_UNKNOWN_TRANSFORMER, GetTransformerCapability("mme.unknown", &goodCapability));
	/*}}}*/
	/*{{{ MME_GetTransformerCapability: rejection of NULL pointers (both arguments) */
	MME_E(MME_INVALID_ARGUMENT, GetTransformerCapability(0, &goodCapability));
	MME_E(MME_INVALID_ARGUMENT, GetTransformerCapability("mme.test", 0));
	/*}}}*/
	/*{{{ MME_GetTransformerCapability: rejection of incorrectly sized structure */
	MME_E(MME_INVALID_ARGUMENT, GetTransformerCapability("mme.test", &badCapability));
	/*}}}*/
	/*{{{ MME_GetTransformerCapability: propogation of rejection */
	badCapability = goodCapability;
	badCapability.TransformerInfo_p = (void *) &paramCapability;
	badCapability.TransformerInfoSize = sizeof(paramCapability);
	MME_E(MME_INVALID_ARGUMENT, GetTransformerCapability("mme.test", &badCapability));
	assert(badCapability.Version == 0);
	/*}}}*/
	/*{{{ MME_GetTransformerCapability: successful call */
	MME(GetTransformerCapability("mme.test", &goodCapability));
	assert(goodCapability.Version == 1);
	/*}}}*/
	/*{{{ MME_GetTransformerCapability: successful call with arguments */
	paramCapability = goodCapability;
	paramCapability.TransformerInfo_p = (void *) params;
	paramCapability.TransformerInfoSize = sizeof(params);
	MME_E(MME_SUCCESS, GetTransformerCapability("mme.test", &paramCapability));
	assert(paramCapability.Version == 2);
	assert(params[0] == 0x12345000);
	assert(params[1] == 0x02345600);
	assert(params[2] == 0x00345670);
	assert(params[3] == 0x00045678);
	/*}}}*/
}
/*}}}*/
/*{{{ check memory allocation and deallocation */
void test_memory_allocation(MME_TransformerHandle_t hdl)
{
	/*{{{ locals */
	char in[] =  "abcdefghijklmnopqrstuvwxyz";
	char out[] = "zyxwvutsrqponmlkjihgfedcba";
	MME_Command_t command = commandTemplate;
	MME_DataBuffer_t buffers[2], *input, *output;
        MME_DataBuffer_t* dataBufferPtrs[2];
	SimpleParamsInit_t simpleParamsStatus;
	TransformerCommandParams_t commandParams;
	/*}}}*/

	/*{{{ configure command */
	command.CmdStatus.AdditionalInfo_p = (void *) &simpleParamsStatus;
	command.CmdStatus.AdditionalInfoSize = MME_LENGTH_BYTES(SimpleParamsStatus);
	command.Param_p = (void *) commandParams;
	command.ParamSize = sizeof(commandParams);
	MME_PARAM(commandParams, TransformerCommandParams_SlowMode) = 0;
	command.NumberInputBuffers = 1;
	command.NumberOutputBuffers = 1;

        dataBufferPtrs[0] = &buffers[0];
        dataBufferPtrs[1] = &buffers[1];
	command.DataBuffers_p = dataBufferPtrs;
	command.CmdCode = MME_TRANSFORM;
	/*}}}*/

	/*{{{ MME_AllocDataBuffer: rejection of both invalid and obsolete transformer handles */
	MME_E(MME_INVALID_HANDLE,AllocDataBuffer(0, 1024, 0, &input));
	MME_E(MME_INVALID_HANDLE,AllocDataBuffer(staleHandle, 1024, 0, &input));
	/*}}}*/
	/*{{{ MME_AllocDataBuffer: rejection of invalid flags */
	MME_E(MME_INVALID_ARGUMENT,AllocDataBuffer(hdl, 1024, 0xdeadbeef, &input));
	/*}}}*/
	/*{{{ MME_AllocDataBuffer: rejection of NULL pointer */
	MME_E(MME_INVALID_ARGUMENT,AllocDataBuffer(hdl, 1024, 0, 0));
	/*}}}*/
	/*{{{ MME_FreeDataBuffer: rejection of NULL pointer */
	MME_E(MME_INVALID_ARGUMENT,FreeDataBuffer(0));
	/*}}}*/
	/*{{{ MME_AllocDataBuffer: fails to allocate zero sized block */
	MME_E(MME_INVALID_ARGUMENT,AllocDataBuffer(hdl, 0, 0, &input));
	/*}}}*/
	/*{{{ MME_AllocDataBuffer: fails to allocate very large blocks */
	MME_E(MME_NOMEM,AllocDataBuffer(hdl, 0xffffffff, 0, &input));
	MME_E(MME_NOMEM,AllocDataBuffer(hdl, 0x80000000, 0, &input));

	MME_E(MME_NOMEM,AllocDataBuffer(hdl, 0x7fffffff, 0, &input));
	MME_E(MME_NOMEM,AllocDataBuffer(hdl, 0x40000000, 0, &input));
	MME_E(MME_NOMEM,AllocDataBuffer(hdl, 0x3fffffff, 0, &input));

	MME_E(MME_NOMEM,AllocDataBuffer(hdl, 0x20000000, 0, &input));
	MME_E(MME_NOMEM,AllocDataBuffer(hdl, 0x1fffffff, 0, &input));
	MME_E(MME_NOMEM,AllocDataBuffer(hdl, 0x10000000, 0, &input));
	MME_E(MME_NOMEM,AllocDataBuffer(hdl, 0x0fffffff, 0, &input));
	/*}}}*/
	/*{{{ MME_AllocDataBuffer: simple allocation of memory (transformer afinity) */
	MME(AllocDataBuffer(hdl, 26, 0, &input));
	assert(1 == input->NumberOfScatterPages); /* 26 bytes really should not be scattered */
	memcpy(input->ScatterPages_p->Page_p, in, 26);
	dataBufferPtrs[0] = input;
	MME(AllocDataBuffer(hdl, 26, 0, &output));
	assert(1 == output->NumberOfScatterPages);
	dataBufferPtrs[1] = output;
	MME(SendCommand(hdl, &command));
	callback_wait();

	assert(0 == memcmp((char *) out, (char *) output->ScatterPages_p->Page_p, 26));

	/*}}}*/
	/*{{{ MME_FreeDataBuffer: successful deallocation of previously allocated memory */
	MME(FreeDataBuffer(input));
	MME(FreeDataBuffer(output));
	/*}}}*/

	/*{{{ MME_AllocDataBuffer: uncached allocation */
	/* TODO: check the results really are uncached */
	MME(AllocDataBuffer(hdl, 26, MME_ALLOCATION_UNCACHED, &input));
	assert(1 == input->NumberOfScatterPages); /* 26 bytes really should not be scattered */
	memcpy(input->ScatterPages_p->Page_p, in, 26);
	dataBufferPtrs[0] = input;
	MME(AllocDataBuffer(hdl, 26, MME_ALLOCATION_UNCACHED, &output));
	assert(1 == output->NumberOfScatterPages);
	dataBufferPtrs[1] = output;
	MME(SendCommand(hdl, &command));
	callback_wait();
	assert(0 == memcmp((char *) out, (char *) output->ScatterPages_p->Page_p, 26));
	MME(FreeDataBuffer(input));
	MME(FreeDataBuffer(output));
	/*}}}*/
	/*{{{ MME_AllocDataBuffer: cached allocation */
	/* TODO: check the results really are cached */
	MME(AllocDataBuffer(hdl, 26, MME_ALLOCATION_CACHED, &input));
	assert(1 == input->NumberOfScatterPages); /* 26 bytes really should not be scattered */
	memcpy(input->ScatterPages_p->Page_p, in, 26);
	dataBufferPtrs[0] = input;
	MME(AllocDataBuffer(hdl, 26, MME_ALLOCATION_CACHED, &output));
	assert(1 == output->NumberOfScatterPages);
	dataBufferPtrs[1] = output;
	MME(SendCommand(hdl, &command));
	callback_wait();
	assert(0 == memcmp((char *) out, (char *) output->ScatterPages_p->Page_p, 26));
	MME(FreeDataBuffer(input));
	MME(FreeDataBuffer(output));
	/*}}}*/
	/*{{{ MME_AllocDataBuffer: contiguous memory allocation */
	/* TODO: check that the results really are *physically* contigious */

	MME(AllocDataBuffer(hdl, 256*1024, MME_ALLOCATION_PHYSICAL, &input));
	assert(1 == input->NumberOfScatterPages); /* this only test virtual contigiousness */
	MME(FreeDataBuffer(input));

	/*}}}*/
}
/*}}}*/
/*{{{ multiple outstanding transform commands */
static void test_multiple_send(MME_TransformerHandle_t hdl, MME_Command_t cmdTemplate)
{
	/*{{{ locals */
	MME_Command_t command[4];
	TransformerCommandParams_t commandParams;

	MME_TransformerInitParams_t params = {
		sizeof(MME_TransformerInitParams_t),
		MME_PRIORITY_LOWEST,
		transformer_callback,
		(void *) 0x78563412,
		MME_LENGTH_BYTES(SimpleParamsInit),
		NULL
	};
	SimpleParamsInit_t simpleParamsInit;
	MME_TransformerHandle_t new;

	int i;
	/*}}}*/

	/*{{{ configure a batch of commands */
	MME_PARAM(commandParams, TransformerCommandParams_SlowMode) = 1;
	cmdTemplate.Param_p = (void *) commandParams;
	command[0] = cmdTemplate;
	command[1] = cmdTemplate;
	command[2] = cmdTemplate;
	command[3] = cmdTemplate;
	/*}}}*/
	/*{{{ MME_SendCommand: successful (FIFO) scheduling for identical due dates */
	MME(SendCommand(hdl, command + 0));
	MME(SendCommand(hdl, command + 1));
	MME(SendCommand(hdl, command + 2));

	/* error checking for above test */
	assert(command[0].CmdStatus.CmdId != command[1].CmdStatus.CmdId);
	assert(command[0].CmdStatus.CmdId != command[2].CmdStatus.CmdId);
	assert(command[1].CmdStatus.CmdId != command[2].CmdStatus.CmdId);
	       
	callback_wait();
	assert(MME_COMMAND_COMPLETED == command[0].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED != command[1].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED != command[2].CmdStatus.State);

	callback_wait();
	assert(MME_COMMAND_COMPLETED == command[0].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED == command[1].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED != command[2].CmdStatus.State);
	       
	callback_wait();
	assert(MME_COMMAND_COMPLETED == command[0].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED == command[1].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED == command[2].CmdStatus.State);
	/*}}}*/
	/*{{{ MME_SendCommand: successful out-of-order execution */
	command[0].DueTime = 1024;
	command[1].DueTime = 1026;
	command[2].DueTime = 1025;
	command[3].DueTime = 1025;
	MME(SendCommand(hdl, command + 0));
	MME(SendCommand(hdl, command + 1));
	MME(SendCommand(hdl, command + 2));
	MME(SendCommand(hdl, command + 3));

	/* error checking for above test */
	assert(command[0].CmdStatus.CmdId != command[1].CmdStatus.CmdId);
	assert(command[0].CmdStatus.CmdId != command[2].CmdStatus.CmdId);
	assert(command[0].CmdStatus.CmdId != command[3].CmdStatus.CmdId);
	assert(command[1].CmdStatus.CmdId != command[2].CmdStatus.CmdId);
	assert(command[1].CmdStatus.CmdId != command[3].CmdStatus.CmdId);
	assert(command[2].CmdStatus.CmdId != command[3].CmdStatus.CmdId);
	       
	callback_wait();
	assert(MME_COMMAND_COMPLETED == command[0].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED != command[1].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED != command[2].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED != command[3].CmdStatus.State);

	callback_wait();
	assert(MME_COMMAND_COMPLETED == command[0].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED != command[1].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED == command[2].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED != command[3].CmdStatus.State);
	       
	callback_wait();
	assert(MME_COMMAND_COMPLETED == command[0].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED != command[1].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED == command[2].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED == command[3].CmdStatus.State);

	callback_wait();
	assert(MME_COMMAND_COMPLETED == command[0].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED == command[1].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED == command[2].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED == command[3].CmdStatus.State);
	/*}}}*/
	/*{{{ MME_SendCommand: correct interpretation of wrapped time */
	command[0].DueTime = 0xe0000000;
	command[1].DueTime = 0x10000000;
	command[2].DueTime = 0x00000000;
	command[3].DueTime = 0xf0000000;

	MME(SendCommand(hdl, command + 0));
	MME(SendCommand(hdl, command + 1));
	MME(SendCommand(hdl, command + 2));
	MME(SendCommand(hdl, command + 3));

	/* error checking for above test */
	assert(command[0].CmdStatus.CmdId != command[1].CmdStatus.CmdId);
	assert(command[0].CmdStatus.CmdId != command[2].CmdStatus.CmdId);
	assert(command[0].CmdStatus.CmdId != command[3].CmdStatus.CmdId);
	assert(command[1].CmdStatus.CmdId != command[2].CmdStatus.CmdId);
	assert(command[1].CmdStatus.CmdId != command[3].CmdStatus.CmdId);
	assert(command[2].CmdStatus.CmdId != command[3].CmdStatus.CmdId);
	       
	callback_wait();
	assert(MME_COMMAND_COMPLETED == command[0].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED != command[1].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED != command[2].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED != command[3].CmdStatus.State);

	callback_wait();
	assert(MME_COMMAND_COMPLETED == command[0].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED != command[1].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED != command[2].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED == command[3].CmdStatus.State);
	       
	callback_wait();
	assert(MME_COMMAND_COMPLETED == command[0].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED != command[1].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED == command[2].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED == command[3].CmdStatus.State);

	callback_wait();
	assert(MME_COMMAND_COMPLETED == command[0].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED == command[1].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED == command[2].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED == command[3].CmdStatus.State);
	/*}}}*/
	/*{{{ MME_SendCommand: successful pre-emption of low priority transforms */

	/* get a high priority transformer */
	params.TransformerInitParams_p = (void*) &simpleParamsInit;
	MME_PARAM(simpleParamsInit, SimpleParamsInit_instanceType) = TEST1_HANDLE_VALUE;
	assert(strlen(test1InstanceName)+1 <= SimpleParamsInit_instanceNameLength);
	for (i=0; i<strlen(test1InstanceName)+1; i++) {
		MME_INDEXED_PARAM(simpleParamsInit, SimpleParamsInit_instanceName, i) = 
				test1InstanceName[i];
	}
	MME(InitTransformer("mme.test", &params, &new));

#if !defined __LINUX__
        /* INSbl22514 Local transformer priorities not honoured in user or kernel mode */
        /* Ignore the test below! */

       	/* do the actual test */
       	MME(SendCommand(new, command + 0));
       	MME(SendCommand(hdl, command + 1));
       	MME(SendCommand(new, command + 2));

      	/* error checking for above test */
       	assert(command[0].CmdStatus.CmdId != command[1].CmdStatus.CmdId);
       	assert(command[0].CmdStatus.CmdId != command[2].CmdStatus.CmdId);
       	assert(command[1].CmdStatus.CmdId != command[2].CmdStatus.CmdId);
       	assert(MME_COMMAND_COMPLETED != command[0].CmdStatus.State);
       	assert(MME_COMMAND_COMPLETED != command[1].CmdStatus.State);
       	assert(MME_COMMAND_COMPLETED != command[2].CmdStatus.State);

       	callback_wait();
       	assert(MME_COMMAND_COMPLETED != command[0].CmdStatus.State);
       	assert(MME_COMMAND_COMPLETED == command[1].CmdStatus.State);
       	assert(MME_COMMAND_COMPLETED != command[2].CmdStatus.State);

       	callback_wait();
       	assert(MME_COMMAND_COMPLETED == command[0].CmdStatus.State);
       	assert(MME_COMMAND_COMPLETED == command[1].CmdStatus.State);
       	assert(MME_COMMAND_COMPLETED != command[2].CmdStatus.State);

       	callback_wait();
       	assert(MME_COMMAND_COMPLETED == command[0].CmdStatus.State);
       	assert(MME_COMMAND_COMPLETED == command[1].CmdStatus.State);
       	assert(MME_COMMAND_COMPLETED == command[2].CmdStatus.State);
#endif
	/*}}}*/
	/*{{{ MME_TermTransformer: rejection of transformer handle with outstanding commands */
	MME(SendCommand(new, command + 0));
	MME_E(MME_COMMAND_STILL_EXECUTING,TermTransformer(new));

	callback_wait();
	MME(TermTransformer(new));
	/*}}}*/
	/*{{{ MME_AbortCommand: rejection of both invalid and obsolete transformer handles */
	MME(SendCommand(hdl, command + 0));
	MME_E(MME_INVALID_HANDLE,AbortCommand(0, command[0].CmdStatus.CmdId));
	MME_E(MME_INVALID_HANDLE,AbortCommand(staleHandle, command[0].CmdStatus.CmdId));
	callback_wait();
	/*}}}*/
	/*{{{ MME_AbortCommand: rejection invalid command */
	MME_E(MME_INVALID_ARGUMENT,AbortCommand(hdl, 0xdeadbeef));
	/*}}}*/
	/*{{{ MME_AbortCommand: successful abortion of a command that is not executing */
	callbackCount = 0;
	MME(SendCommand(hdl, command + 0));
	MME(SendCommand(hdl, command + 1));
	MME(AbortCommand(hdl, command[1].CmdStatus.CmdId));
	callback_wait();
	callback_wait();
	assert(MME_COMMAND_COMPLETED == command[0].CmdStatus.State);
	assert(MME_COMMAND_FAILED == command[1].CmdStatus.State);
	assert(MME_COMMAND_ABORTED == command[1].CmdStatus.Error);
	assert(2 == callbackCount);
	/*}}}*/
	/*{{{ MME_AbortCommand: failed abortion of a command that has already completed */
	MME_E(MME_INVALID_ARGUMENT,AbortCommand(hdl, command[0].CmdStatus.CmdId));
	/*}}}*/
	/*{{{ MME_AbortCommand: failed abortion of an executing command when not supported by transformer */
	MME(SendCommand(hdl, command + 0));
	harness_sleep(1);
	(void) MME_I(AbortCommand(hdl, command[0].CmdStatus.CmdId));
	callback_wait();
	assert(MME_COMMAND_COMPLETED == command[0].CmdStatus.State);
	/*}}}*/
	/*{{{ MME_AbortCommand: TODO - successful abortion of command that is executing with a transformer that supports abortion */
	/*}}}*/
}
/*}}}*/
/*{{{ check MME_SendCommand, MME_WaitCommand and transformer callbacks */
static void test_send_and_wait(MME_TransformerHandle_t hdl)
{
	/*{{{ locals */
	char inputBytes[]  = "Hello world";
	char outputBytes[] = "aaaaaaaaaaa";
	char expectBytes[] = "dlrow olleH";

	MME_CommandCode_t codeList[] = { 
		MME_SET_GLOBAL_TRANSFORM_PARAMS, MME_TRANSFORM, MME_SEND_BUFFERS 
	};
	
	MME_Command_t goodCommand = commandTemplate, badCommand;
	MME_DataBuffer_t bufferPassing[3];
	MME_ScatterPage_t inputPage = scatterPageTemplate;
	MME_ScatterPage_t outputPage = scatterPageTemplate;

        MME_DataBuffer_t* dataBufferPtrs[16];

	SimpleParamsInit_t simpleParamsStatus;
	TransformerCommandParams_t commandParams;
	int i;
	/*}}}*/

	/*{{{ configure goodCommand - there's quite a lot of it ... */
        goodCommand.CmdStatus.AdditionalInfo_p = (void*) &simpleParamsStatus;
        goodCommand.CmdStatus.AdditionalInfoSize = MME_LENGTH_BYTES(SimpleParamsStatus);
	goodCommand.Param_p = (void *) commandParams;
	goodCommand.ParamSize = sizeof(commandParams);
	MME_PARAM(commandParams, TransformerCommandParams_SlowMode) = 0;

#ifdef USERMODE
        MME(AllocDataBuffer(hdl, sizeof(inputBytes), 0, &dataBufferPtrs[0] ));
        MME(AllocDataBuffer(hdl, sizeof(outputBytes), 0, &dataBufferPtrs[1] ));
        MME(AllocDataBuffer(hdl, sizeof(outputBytes), 0, &dataBufferPtrs[2] ));
        memcpy(dataBufferPtrs[0]->ScatterPages_p->Page_p, inputBytes, sizeof(inputBytes));
        memcpy(dataBufferPtrs[1]->ScatterPages_p->Page_p, outputBytes, sizeof(outputBytes));
        memcpy(dataBufferPtrs[2]->ScatterPages_p->Page_p, outputBytes, sizeof(outputBytes));
        dataBufferPtrs[0]->TotalSize = dataBufferPtrs[0]->ScatterPages_p->Size = sizeof(inputBytes) - 1;
        dataBufferPtrs[1]->TotalSize = dataBufferPtrs[1]->ScatterPages_p->Size = sizeof(outputBytes) - 1;
        dataBufferPtrs[2]->TotalSize = dataBufferPtrs[2]->ScatterPages_p->Size = sizeof(outputBytes) - 1;
#else
        dataBufferPtrs[0] = &(bufferPassing[0]);
        dataBufferPtrs[1] = &(bufferPassing[1]);
        dataBufferPtrs[2] = &(bufferPassing[2]);

	/* setup the input buffer */
	inputPage.Page_p = inputBytes;
	inputPage.Size = sizeof(inputBytes) - 1;
	bufferPassing[0] = dataBufferTemplate;
	bufferPassing[0].ScatterPages_p = &inputPage;
	bufferPassing[0].TotalSize = inputPage.Size;
	bufferPassing[0].NumberOfScatterPages = 1;
	/* setup the output buffer */
	outputPage.Page_p = outputBytes;
	outputPage.Size = sizeof(outputBytes) - 1;
	bufferPassing[1] = dataBufferTemplate;
	bufferPassing[1].ScatterPages_p = &outputPage;
	bufferPassing[1].TotalSize = outputPage.Size;
	bufferPassing[1].NumberOfScatterPages = 1;

        /* a second output buffer for rejection */
	bufferPassing[2] = dataBufferTemplate;
	bufferPassing[2].ScatterPages_p = &outputPage;
	bufferPassing[2].TotalSize = outputPage.Size;
	bufferPassing[2].NumberOfScatterPages = 1;
#endif
	goodCommand.NumberInputBuffers = 1;
	goodCommand.NumberOutputBuffers = 1;
	goodCommand.DataBuffers_p = dataBufferPtrs;
	goodCommand.CmdCode = MME_TRANSFORM;
	/*}}}*/

	/*{{{ MME_SendCommand: rejection of null pointer */
	MME_E(MME_INVALID_ARGUMENT,SendCommand(hdl, 0));
	/*}}}*/
	/*{{{ MME_SendCommand: rejection of invalid handle */
	MME_E(MME_INVALID_HANDLE, SendCommand(0, &goodCommand));
	MME_E(MME_INVALID_HANDLE, SendCommand(staleHandle, &goodCommand
	));
	/*}}}*/
	/*{{{ MME_SendCommand: successful completion of MME_TRANSFORM command */
	MME(SendCommand(hdl, &goodCommand));

	/*{{{ MME_SendCommand: successful deployment of callback */
	callback_wait();
	/*}}}*/

	/* confirm the data had been correctly processed */
        {
                char* resultBytes = (char*) dataBufferPtrs[1]->ScatterPages_p->Page_p;
	        VERBOSE(printf(MACHINE "transform complete, '%s' -> '%s'\n", inputBytes, resultBytes));
	        assert(0 == strcmp((char *) expectBytes, (char *) resultBytes));
        }
	assert(MME_COMMAND_COMPLETED == goodCommand.CmdStatus.State);
	assert(MME_SUCCESS == goodCommand.CmdStatus.Error);

	assert(MME_PARAM(simpleParamsStatus, SimpleParamsStatus_status1)==TEST1_STATUS_INT);
#if !defined __LINUX__ || !defined __KERNEL__ || !defined __SH4__
	/* SimpleParamsStatus_status2 is not filled in by Linux kernel mode transformers 
	 * (to do so requires FPU in kernel mode). For the same reason we must omit this
	 * test on kernel mode.
	 */
	assert(MME_PARAM(simpleParamsStatus, SimpleParamsStatus_status2)==TEST1_STATUS_DOUBLE);
#endif
	/*}}}*/

	test_multiple_send(hdl, goodCommand);

	/*{{{ MME_WaitCommand: INCOMPLETE and DISABLED */
#if 0
	/* now try again with the callback enabled by using MME_WaitCommand instead */
	memset(outputBytes, 'b', sizeof(outputBytes) - 1);
	MME(SendCommand(transformerHandle, MME_TRANSFORM, &goodCommand));
	MME(WaitCommand(&(goodCommand.CmdStatus.CmdId), 1, 0, &event, &evtCmd, &user_data));
	assert(MME_COMMAND_COMPLETED_EVT == Event);
	assert(&goodCommand = evtCmd);
	assert((void *) 0x78563412 == UserData);

	/* check that the output is correct */
	VERBOSE(printf(MACHINE "transform complete, '%s' -> '%s'\n", inputBytes, outputBytes));
	assert(0 == strcmp((char *) expectBytes, (char *) outputBytes));
	assert(MME_COMMAND_COMPLETED == goodCommand.CmdStatus.State);
	assert(MME_SUCCESS == goodStatus.Error);
#endif
	/*}}}*/
	/*{{{ MME_SendCommand: rejection of incorrectly sized structure */
	badCommand = goodCommand;
	badCommand.StructSize = 12;
	MME_E(MME_INVALID_ARGUMENT, SendCommand(hdl, &badCommand));
	/*}}}*/
	/*{{{ MME_SendCommand: rejection of bad values in command structure */
	badCommand = goodCommand;
	badCommand.CmdCode = 0xdeadbeef;
	MME_E(MME_INVALID_ARGUMENT, SendCommand(hdl, &badCommand));

	badCommand = goodCommand;
	badCommand.CmdEnd = 0xdeadbeef;
	MME_E(MME_INVALID_ARGUMENT, SendCommand(hdl, &badCommand));

	/*}}}*/
	/*{{{ MME_SendCommand: propogation of rejection from each transformer command */
	badCommand = goodCommand;
	badCommand.NumberOutputBuffers = 2;
	for (i=0; i<(sizeof(codeList)/sizeof(codeList[0])); i++) {
		badCommand.CmdCode = codeList[i];
		VERBOSE(printf(MACHINE "CmdCode = %d\n", badCommand.CmdCode));
		badCommand.CmdStatus.State = 0xdeadbeef;
		badCommand.CmdStatus.Error = 0xdeadbeef;
		MME(SendCommand(hdl, &badCommand));
		callback_wait();
		assert(MME_COMMAND_FAILED == badCommand.CmdStatus.State);
		assert(MME_INVALID_ARGUMENT == badCommand.CmdStatus.Error);
	}
	/*}}}*/
	/*{{{ MME_SendCommand: corrent prevention of callback in all cases */
	callbackCount = 0;
	goodCommand.CmdEnd = MME_COMMAND_END_RETURN_NO_INFO;
	badCommand.CmdEnd = MME_COMMAND_END_RETURN_NO_INFO;
	MME(SendCommand(hdl, &goodCommand));
	MME(SendCommand(hdl, &badCommand));
	harness_sleep(2);
	assert(MME_COMMAND_COMPLETED == goodCommand.CmdStatus.State);
	assert(MME_COMMAND_FAILED == badCommand.CmdStatus.State);

	assert(0 == callbackCount);
	/*}}}*/

	/*{{{ cleanup goodCommand  ... */
#ifdef USERMODE
        MME(FreeDataBuffer(dataBufferPtrs[0]));
        MME(FreeDataBuffer(dataBufferPtrs[1]));
        MME(FreeDataBuffer(dataBufferPtrs[2]));
#endif
	/*}}}*/
}
/*}}}*/
/*{{{ check send and its friends with streaming and deferred transformers */
static void test_streaming_and_deferred(void)
{
	/*{{{ locals */
	MME_TransformerInitParams_t tip = {
		sizeof(MME_TransformerInitParams_t),
		MME_PRIORITY_NORMAL,
		transformer_callback,
		(void *) 0x78563412,
		MME_LENGTH_BYTES(StreamingGlobalParams),
		NULL
	};
	StreamingGlobalParams_t sgp;
	MME_TransformerHandle_t hdl;

        MME_DataBuffer_t* inputDataBufferPtrs[2];
        MME_DataBuffer_t* outputDataBufferPtrs[3];

	MME_Command_t input[2];
	MME_Command_t output[3];
	MME_Command_t transform[3];
	MME_Command_t global;
	/*}}}*/

	/*{{{ obtain a transformer handle and configure a batch of commands */
	tip.TransformerInitParams_p = (MME_GenericParams_t *) &sgp;
	MME_PARAM(sgp, StreamingGlobalParams_mode) = 0;
	MME(InitTransformer("mme.streaming", &tip, &hdl));

	input[0] = commandTemplate;
	input[0].CmdCode = MME_SEND_BUFFERS;
	input[0].NumberInputBuffers = 1;
        input[0].DataBuffers_p = &inputDataBufferPtrs[0];
        MME(AllocDataBuffer(hdl, 14, 0, &inputDataBufferPtrs[0]));
        strcpy(inputDataBufferPtrs[0]->ScatterPages_p->Page_p, "aBcDeFgHiJkLm");

	input[1] = commandTemplate;
	input[1].CmdCode = MME_SEND_BUFFERS;
	input[1].NumberInputBuffers = 1;
        input[1].DataBuffers_p = &inputDataBufferPtrs[1];
        MME(AllocDataBuffer(hdl, 14, 0, &inputDataBufferPtrs[1]));
        strcpy(inputDataBufferPtrs[0]->ScatterPages_p->Page_p, "aBcDeFgHiJkLm");

	output[0] = commandTemplate;
	output[0].CmdCode = MME_SEND_BUFFERS;
	output[0].NumberOutputBuffers = 1;
        output[0].DataBuffers_p = &outputDataBufferPtrs[0];
        MME(AllocDataBuffer(hdl, 28, 0, &outputDataBufferPtrs[0]));

	output[1] = commandTemplate;
	output[1].CmdCode = MME_SEND_BUFFERS;
	output[1].NumberOutputBuffers = 1;
        output[1].DataBuffers_p = &outputDataBufferPtrs[1];
        MME(AllocDataBuffer(hdl, 7, 0, &outputDataBufferPtrs[1])); 

	output[2] = commandTemplate;
	output[2].CmdCode = MME_SEND_BUFFERS;
	output[2].NumberOutputBuffers = 1;
        output[2].DataBuffers_p = &outputDataBufferPtrs[2];
	MME(AllocDataBuffer(hdl, 7, 0, &outputDataBufferPtrs[2]));

	transform[0] = commandTemplate;
	transform[1] = commandTemplate;
	transform[2] = commandTemplate;

	global = commandTemplate;
	global.CmdCode = MME_SET_GLOBAL_TRANSFORM_PARAMS;
	global.ParamSize = MME_LENGTH_BYTES(StreamingGlobalParams);
	global.Param_p = (MME_GenericParams_t *) &sgp;
	/*}}}*/

	/*{{{ MME_SendCommand: streamed input buffers into single streamed output buffer */
	callbackCount = 0;
	MME(SendCommand(hdl, input+0));
	MME(SendCommand(hdl, input+1));
	MME(SendCommand(hdl, output+0));
	assert(0 == callbackCount);
	MME(SendCommand(hdl, transform+0));
	callback_wait();
	callback_wait();
	assert(2 == callbackCount);
	MME(SendCommand(hdl, transform+0));
	callback_wait();
	callback_wait();
	callback_wait();
	assert(5 == callbackCount);
#if 0
	memcmp(output[0].DataBuffers_p->ScatterPages_p->Page_p,
	       "ABCDEFGHIJKLM\0NOPQRSTUVWXYZ\0", 28);
#else
	memcmp((*(output[0].DataBuffers_p))->ScatterPages_p->Page_p,
	       "ABCDEFGHIJKLM\0NOPQRSTUVWXYZ\0", 28);
#endif
	/*}}}*/

	/*{{{ MME_SendCommand: single streamed input buffer into streamed output buffers */
	callbackCount = 0;
	MME(SendCommand(hdl, input+0));
	MME(SendCommand(hdl, output+1));
	MME(SendCommand(hdl, output+2));
	assert(0 == callbackCount);
	MME(SendCommand(hdl, transform+0));
	callback_wait();
	callback_wait();
	callback_wait();
	callback_wait();
	assert(4 == callbackCount);
	memcmp((*(output[1].DataBuffers_p))->ScatterPages_p->Page_p, "ABCDEFG",  7);
	memcmp((*(output[2].DataBuffers_p))->ScatterPages_p->Page_p, "HIJKLM\0", 7);
	/*}}}*/
	/*{{{ MME_SendCommand: MME_SET_GLOBAL_TRANSFORM_PARAMS */
	MME_PARAM(sgp, StreamingGlobalParams_mode) = 1;
	MME(SendCommand(hdl, &global));
	callback_wait();
	assert(MME_SUCCESS == global.CmdStatus.Error);
	/*}}}*/
	/*{{{ MME_SendCommand: deferred transforms */
	callbackCount = 0;
	MME(SendCommand(hdl, input+0));
	MME(SendCommand(hdl, input+1));
	MME(SendCommand(hdl, output+0));
	MME(SendCommand(hdl, transform+0));
	MME(SendCommand(hdl, transform+1));
	harness_sleep(1);
	assert(0 == callbackCount);
	MME(SendCommand(hdl, transform+2));
	callback_wait();
	callback_wait();
	harness_sleep(1);
	assert(2 == callbackCount);
	assert(MME_COMMAND_COMPLETED == input[0].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED != input[1].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED != output[0].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED == transform[0].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED != transform[1].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED != transform[2].CmdStatus.State);
	MME(SendCommand(hdl, transform+0));
	callback_wait();
	callback_wait();
	callback_wait();
	harness_sleep(1);
	assert(5 == callbackCount);
	assert(MME_COMMAND_COMPLETED == input[0].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED == input[1].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED == output[0].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED != transform[0].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED == transform[1].CmdStatus.State);
	assert(MME_COMMAND_COMPLETED != transform[2].CmdStatus.State);
	/*}}}*/

	/*{{{ tidy up */	
	/* switch out of deferred mode in order to clear the buffers */
	MME_PARAM(sgp, StreamingGlobalParams_mode) = 0;
	MME(SendCommand(hdl, input+0));
	MME(SendCommand(hdl, input+1));
	MME(SendCommand(hdl, output+0));
	MME(SendCommand(hdl, &global));
	callback_wait(); /* input[0] */
	callback_wait(); /* transform[2] */
	callback_wait(); /* input[1] */
	callback_wait(); /* output[0] */
	callback_wait(); /* transform[0] */
	callback_wait(); /* global */
	harness_sleep(1);
	assert(11 == callbackCount);

	MME(FreeDataBuffer(inputDataBufferPtrs[0]));
	MME(FreeDataBuffer(inputDataBufferPtrs[1]));
	MME(FreeDataBuffer(outputDataBufferPtrs[0]));
	MME(FreeDataBuffer(outputDataBufferPtrs[1]));
	MME(FreeDataBuffer(outputDataBufferPtrs[2]));

	MME(TermTransformer(hdl));
	/*}}}*/
}
/*}}}*/
/*{{{ terminate and tidy up */
void test_clean_up(void)
{
	/*{{{ MME_Term: successful termination */
	MME(Term());
	/*}}}*/
}
/*}}}*/
/*{{{ run all the tests against a single transformer location (either local or remote) */
void test_transformer_location()
{
	MME_TransformerHandle_t transformerHandle;

	/* get a transformer handle and test the transformer initialization functions 
	 * (this function also initialized the staleHandle global which is used by
	 * subsequent tests)
	 */
	transformerHandle = test_init_transformer();

	/* when test_init_transformer() has completed we know that "mme.test" exists
	 * so it is safe to run these tests now.
	 */
	test_get_capability();

	/* now see if we can allocate memory */
	test_memory_allocation(transformerHandle);

	/* use the transformer handle to test basic command handling */
	test_send_and_wait(transformerHandle);

	/* and some more complex command handling */
	test_streaming_and_deferred();

	MME(TermTransformer(transformerHandle));
}

/*}}}*/

/*{{{ boot and run */
int main(void)
{
	/* boot the operating system (it is a pre-requisite for all MME calls) */
	harness_boot();
	OS_SIGNAL_INIT(&callbackReceived);

	/* ensure conformance prior to initialization */
	test_rejection_prior_to_initialization();

	/* initialize everything and test the registration functions */
	test_init_and_register();

	/* run all reusable tests against remotely registered transformers */
#ifdef INSbl21633
	MME(RegisterTransport(TRANSPORT_NAME));
#endif

	test_transformer_location();

#ifdef INSbl21633
	MME(DeregisterTransformer(TRANSPORT_NAME));
#endif

	/* run all reusable tests against locally registered transformers */
	MME(RegisterTransformer("mme.test", SimpleTest_AbortCommand,
				SimpleTest_GetTransformerCapability, SimpleTest_InitTransformer,
				SimpleTest_ProcessCommand, SimpleTest_TermTransformer));
	MME(RegisterTransformer("mme.streaming", Streaming_AbortCommand,
                                Streaming_GetTransformerCapability, Streaming_InitTransformer,
                                Streaming_ProcessCommand, Streaming_TermTransformer));

	test_transformer_location();

	/* terminate MME */
	test_clean_up();

	/* check that all the calls start to fail again */
#ifdef INSbl21664
	test_rejection_prior_to_initialization();
#endif

	/* MME_Init: successful initialization after previous termination */
#ifdef INSbl21664
	test_init_and_register();
#ifdef INSbl21633
	MME(RegisterTransport(TRANSPORT_NAME));
#endif
	transformerHandle = test_init_transformer();
	test_clean_up(transformerHandle);
#endif
	/* tidy up */
	OS_SIGNAL_DESTROY(&callbackReceived);

	harness_shutdown();
	return 0;
}
/*}}}*/
