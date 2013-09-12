/*
 * host.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * A very simple communications test (host side)
 */

#include <mme.h>
#include <os_abstractions.h>

#include "harness.h"

#include "params.h"

#define STARTOFFSET 0

static OS_SIGNAL callbackReceived;

/* We use this structure to feed in a command Param_p that follows the buffer passing convention. */
struct BufferPassing {
	MME_UINT NumInputBuffers;	/* input MME_DataBuffer_t structures, even if it's zero */
	MME_UINT NumOutputBuffers;	/* output MME_DataBuffer_t structures, even if it's zero */
	MME_DataBuffer_t buffers[2];
};

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

/* template containing default value for use in the buffer passing convention */
static const struct BufferPassing bufferPassingTemplate = {
	1,
	1,
	{
	 {			/* input buffer */
	  sizeof(MME_DataBuffer_t),
	  NULL, 0, 0, 1, NULL, 0, STARTOFFSET}
	 ,
	 {			/* output buffer */
	  sizeof(MME_DataBuffer_t),
	  NULL, 0, 0, 1, NULL, 0, STARTOFFSET}
	 }
};

/* template for a scatter page (a bit simple this one) */
static const MME_ScatterPage_t scatterPageTemplate = { 0, 0, 0, 0 };


static void TransformerCallback(MME_Event_t Event, MME_Command_t *CallbackData, void *UserData)
{
	VERBOSE(printf(MACHINE "received callback\n"));

	assert(MME_COMMAND_COMPLETED_EVT == Event);
	assert((void *) 0x78563412 == UserData);

	OS_SIGNAL_POST(&callbackReceived);
}

static  const char* test1InstanceName = TEST1_INSTANCE_NAME;

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
	MME_TransformerHandle_t transformerHandle;

	char inputBytes[]  = "Hello world";
	char outputBytes[] = "aaaaaaaaaaa";
	char expectBytes[] = "dlrow olleH";
        char* resultBytes;
	MME_Command_t command = commandTemplate;
	MME_CommandStatus_t commandStatus = { sizeof(MME_CommandStatus_t) };
	struct BufferPassing bufferPassing = bufferPassingTemplate;
	MME_ScatterPage_t inputPage = scatterPageTemplate;
	MME_ScatterPage_t outputPage = scatterPageTemplate;
        MME_DataBuffer_t* bufferPtrs[2];

	SimpleParamsInit_t simpleParamsInit;
	SimpleParamsInit_t simpleParamsStatus;
	int i;

	/* bring up the API */
	harness_boot();
	MME(Init());
#ifdef USERMODE
	MME_E(MME_NOT_IMPLEMENTED, DeregisterTransport(TRANSPORT_NAME));
	MME_E(MME_NOT_IMPLEMENTED, RegisterTransport(TRANSPORT_NAME));
#else
	MME_E(MME_INVALID_ARGUMENT,DeregisterTransport(TRANSPORT_NAME));
	MME(RegisterTransport(TRANSPORT_NAME));
#endif
	/* Some parameters */
	initParams.TransformerInitParams_p = (void*) &simpleParamsInit;

	MME_PARAM(simpleParamsInit, SimpleParamsInit_instanceType) = TEST1_HANDLE_VALUE;
	assert(strlen(test1InstanceName)+1 <= SimpleParamsInit_instanceNameLength);
	for (i=0; i<strlen(test1InstanceName)+1; i++) {
		MME_INDEXED_PARAM(simpleParamsInit, SimpleParamsInit_instanceName, i) = test1InstanceName[i];
	}

	/* side-effects are acceptable in the test suite (it is pointless
	 * to compile the test suite with NDEBUG since it will eliminate all
	 * the tests).
	 */
	OS_SIGNAL_INIT(&callbackReceived);

	VERBOSE(printf(MACHINE "MME_InitTransformer(com.st.mcdt.mme.test)"));
	while (MME_SUCCESS != MME_InitTransformer("com.st.mcdt.mme.test", &initParams, &transformerHandle)) {
		harness_sleep(1);
	}
	VERBOSE(printf(MACHINE "transformer handle %d\n", transformerHandle));

	/* send a command and wait for it to complete */
	command.CmdStatus = commandStatus;
        command.CmdStatus.AdditionalInfo_p = (void*) &simpleParamsStatus;
        command.CmdStatus.AdditionalInfoSize = MME_LENGTH_BYTES(SimpleParamsStatus);
	command.Param_p = NULL;

        MME_AllocDataBuffer(transformerHandle, sizeof(inputBytes), 0, &bufferPtrs[0]);
        MME_AllocDataBuffer(transformerHandle, sizeof(outputBytes), 0, &bufferPtrs[1]);
        memcpy(bufferPtrs[0]->ScatterPages_p->Page_p, inputBytes,  sizeof(inputBytes));
        memcpy(bufferPtrs[1]->ScatterPages_p->Page_p, outputBytes, sizeof(outputBytes));
        bufferPtrs[0]->TotalSize = bufferPtrs[0]->ScatterPages_p->Size = sizeof(inputBytes) - 1;
        bufferPtrs[1]->TotalSize = bufferPtrs[1]->ScatterPages_p->Size = sizeof(outputBytes) - 1;

	command.NumberInputBuffers = 1;
	command.NumberOutputBuffers = 1;
	command.DataBuffers_p = bufferPtrs;
	command.CmdCode = MME_TRANSFORM;

	MME(SendCommand(transformerHandle, &command));

	VERBOSE(printf(MACHINE "waiting for callback ...\n"));
	OS_SIGNAL_WAIT(&callbackReceived);

	/* check that the output is correct */
        resultBytes = (char*) bufferPtrs[1]->ScatterPages_p->Page_p;
        VERBOSE(printf(MACHINE "transform complete, '%s' -> '%s'\n", inputBytes, resultBytes));
        assert(0 == strcmp((char *) expectBytes, (char *) resultBytes));
	VERBOSE(printf(MACHINE "BytesUsed = %d (expected %d)\n", 
	               bufferPtrs[1]->ScatterPages_p->BytesUsed, sizeof(outputBytes) - 1));
	assert(bufferPtrs[1]->ScatterPages_p->BytesUsed == (sizeof(outputBytes) - 1));
        
	assert(MME_COMMAND_COMPLETED == command.CmdStatus.State);
	assert(MME_SUCCESS == command.CmdStatus.Error);
	assert(MME_PARAM(simpleParamsStatus, SimpleParamsStatus_status1)==TEST1_STATUS_INT);
#if !defined __LINUX__ || !defined __KERNEL__
	assert(MME_PARAM(simpleParamsStatus, SimpleParamsStatus_status2)==TEST1_STATUS_DOUBLE);
#endif

	/* tidy up */
	MME(FreeDataBuffer(bufferPtrs[0]));
	MME(FreeDataBuffer(bufferPtrs[1]));
	MME(TermTransformer(transformerHandle));
	MME(Term());
	OS_SIGNAL_DESTROY(&callbackReceived);

	harness_shutdown();
	return 0;
}
