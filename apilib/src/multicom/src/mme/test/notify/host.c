/*
 * host.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * A very simple communications test (host side)
 */

/* To test:
Simple wait - companion transformers                          Y
Wait new    - companion transformers                          Y
Simple wait - local transformers
Wait new    - local transformers
Interrupt simple wait

Multithreads waiting specific events only  - companion transformers
Multithreads waiting specific events only  - local transformers
Multithreads waiting specific events only - each wait > 1 event
Multithreads some wait new only, some wait specific events only
Multithreads all waiting new and specific events

Multixformer mutlithread wait
*/

int tracebuffer[4096];

#include <os_abstractions.h>
#include "harness.h"

#include "mme.h"
#include "params.h"
#ifdef ENABLE_MME_WAITCOMMNAD
#include "wait_test.h"
#endif

#define STARTOFFSET 0

#ifndef QUICK
#define QUICK 0
#endif

#ifndef NOTIFY_VERBOSE
#define NOTIFY_VERBOSE 0
#endif

static OS_SIGNAL callbackReceived;
static OS_MUTEX callbackDataMutex;

static MME_Event_t callbackEvent;
static void*       callbackData;

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

static int ignoreCallback = 0;

static void TransformerCallback(MME_Event_t event, MME_Command_t *CallbackData, void *userData)
{
	VERBOSE(printf(MACHINE "received callback - event %d\n", event));

	if (ignoreCallback) {
		return;
	}

	OS_MUTEX_TAKE(&callbackDataMutex);

	callbackEvent = event;
	callbackData = userData;

	OS_SIGNAL_POST(&callbackReceived);
}

typedef struct ThreadData_s {
	OS_SIGNAL* signal;
	char*      transformerName;
	int	   running;
	int        i2;
	int        i3;
} ThreadData_t;

/*
   This function will create a transformer instance, a new transform command
   and then abort the command and remove the transformer 
   It is used to cause a thread waiting in MME_WaitCommand to unblock
*/
#ifdef ENABLE_MME_WAITCOMMAND
static void CreateNewCommandAndAbort(const char* transformerName) {
	MME_TransformerInitParams_t initParams = {
		sizeof(MME_TransformerInitParams_t),
		MME_PRIORITY_NORMAL,
                NULL,
		NULL,
		0,
		NULL,
	};
	MME_TransformerHandle_t transformerHandle;
	MME_Command_t           transformCommand = commandTemplate;

	MME_Event_t             event;
	MME_Command_t*          evtCmd;
	void*                   userData;
	MME_ERROR               error;

	initParams.CallbackUserData = (void*) 0x1234abcd;
	transformCommand.CmdCode = MME_TRANSFORM;

	while (MME_SUCCESS != MME_InitTransformer(transformerName, &initParams, &transformerHandle)) {
		harness_sleep(1);
	}

	MME(SendCommand(transformerHandle, &transformCommand));

	MME(AbortCommand(transformerHandle, transformCommand.CmdStatus.CmdId));

        /* Now wait for command completion */
	/* First wait is MME_DATA_UNDERFLOW_EVT */
	MME(WaitCommand(&(transformCommand.CmdStatus.CmdId), 1, 0, &event, &evtCmd, &userData));

	if (MME_COMMAND_COMPLETED_EVT == event && NULL == evtCmd && 0x1234abcd == (int)userData) {
		/* Local transformers - the command no longer exists in the system by the
                   time MME_WaitCommand is called */
		goto TERMINATE;
	}
	MME(WaitCommand(&(transformCommand.CmdStatus.CmdId), 1, 0, &event, &evtCmd, &userData));

	assert(MME_COMMAND_COMPLETED_EVT == event);
        assert(&transformCommand == evtCmd);
	assert(0x1234abcd == (int)userData);

TERMINATE:

	MME(TermTransformer(transformerHandle));
}

static void WaitThread(void* data)
{
	MME_Event_t    event;
	void*          userData;
	MME_Command_t* evtCmd;
	MME_ERROR      result;
	OS_SIGNAL*     signal = ((ThreadData_t*)data)->signal;
	OS_MUTEX*      lock   = (OS_MUTEX*) (((ThreadData_t*)data)->i3);

	while (((ThreadData_t*)data)->running) {
		result = MME_WaitCommand(NULL, 0, MME_WAIT_NEW_COMMAND, &event, &evtCmd, &userData);
		assert(MME_NEW_COMMAND_EVT == event);

		OS_MUTEX_TAKE(lock);
		((ThreadData_t*)data)->i2++;
		OS_MUTEX_RELEASE(lock);

		if (userData == signal && ((ThreadData_t*)data)->running) {
			/* Signal the thread waiting for this new command */
		        OS_SIGNAL_POST(signal);
		}
	}
}

static void TestWaitSingleThread(void* data)
{
	MME_ERROR      error;
	int            i;
	MME_Event_t    event;
	MME_Command_t* evtCmd;
	void*          userData;
	int            iterations = ((ThreadData_t*)data)->running;
	OS_SIGNAL*     running    = ((ThreadData_t*)data)->signal;
	int            id         = ((ThreadData_t*)data)->i3;

#if !MULTITHREAD_NEWWAIT
	OS_SIGNAL*     done       = (OS_SIGNAL*) ((ThreadData_t*)data)->i2;
	int            waitNew    = 0;
#else
	int            waitNew    = ((ThreadData_t*)data)->i2;
#endif
	const char*    transformerName = ((ThreadData_t*)data)->transformerName;

	MME_Command_t transformCommand = commandTemplate;
	MME_Command_t buffersCommand   = commandTemplate;

	MME_TransformerInitParams_t initParams = {
		sizeof(MME_TransformerInitParams_t),
		MME_PRIORITY_NORMAL,
                NULL,
		NULL,
		0,
		NULL,
	};
	MME_TransformerHandle_t transformerHandle;

	OS_SIGNAL newCommandReceived;

	ThreadData_t waitData;
	waitData.signal = &newCommandReceived;
	waitData.running = 1;

	if (waitNew) {
		OS_SIGNAL_INIT(&newCommandReceived);
	}

	transformCommand.CmdCode = MME_TRANSFORM;
	buffersCommand.CmdCode   = MME_SEND_BUFFERS;

	/* This transformer-specific data used by the waitor to identify who */
	initParams.CallbackUserData = &newCommandReceived;

	while (MME_SUCCESS != MME_InitTransformer(transformerName, &initParams, &transformerHandle)) {
		harness_sleep(1);
	}

	if (waitNew) {
		/* Spawn new command thread */
		harness_createThread(WaitThread, &waitData);
	}
	/* Wait for the thread to set itself up and call MME_WaitCommand */
        harness_sleep(1);

        OS_SIGNAL_POST(running);

	for (i=0; i<iterations; i++) {
#if NOTIFY_VERBOSE
		if (i%300 == 0) {printf("%d [%d].", id, i); fflush(stdout);}
#endif
		/* Send the command */
		MME(SendCommand(transformerHandle, &transformCommand));

		if (waitNew) {
		        OS_SIGNAL_WAIT(&newCommandReceived);
		}

		/* Wait for data underflow */
	        MME(WaitCommand(&(transformCommand.CmdStatus.CmdId), 1, 0, &event, &evtCmd, &userData));

		assert(MME_DATA_UNDERFLOW_EVT == event);
	        assert(&transformCommand == evtCmd);
		assert(&newCommandReceived == userData);

		/* Send buffers */
		MME(SendCommand(transformerHandle, &buffersCommand));

		/* Wait for success */
		MME(WaitCommand(&(buffersCommand.CmdStatus.CmdId), 1, 0, &event, &evtCmd, &userData));

		/* Wait for data overflow */
		MME(WaitCommand(&(transformCommand.CmdStatus.CmdId), 1, 0, &event, &evtCmd, &userData));

	        assert(MME_NOT_ENOUGH_MEMORY_EVT == event);
		assert(&transformCommand == evtCmd);
	        assert(&newCommandReceived == userData);

		/* Send buffers */
		MME(SendCommand(transformerHandle, &buffersCommand));

#if 0
		/* Wait for success */
		/* There is a race here - on a local transformer the command will have been completed
                   and wiped from the command table before reaching this wait
                 */
		MME(WaitCommand(&(buffersCommand.CmdStatus.CmdId), 1, 0, &event, &evtCmd, &userData));
#endif
	        /* Now wait for command completion */
		MME(WaitCommand(&(transformCommand.CmdStatus.CmdId), 1, 0, &event, &evtCmd, &userData));
		assert(MME_COMMAND_COMPLETED_EVT == event);
	        assert(&transformCommand == evtCmd);
		assert(&newCommandReceived == userData);
	}

	MME(TermTransformer(transformerHandle));

	if (waitNew) {
		/* The next two steps will terminate the new command waitor thread */
		waitData.running = 0;
		CreateNewCommandAndAbort(transformerName);
	        OS_SIGNAL_DESTROY(&newCommandReceived);
	}

#if !MULTITHREAD_NEWWAIT
	/* Inform the spawner we are terminating */
        OS_SIGNAL_POST(done);
#endif
}

static void TestWaitMultiThread(int threads, int reps, int newWait, const char* transformerName) 
{
	int i;

	/* Spawn new command thread */

#if !MULTITHREAD_NEWWAIT
	ThreadData_t newWaitData;

	OS_SIGNAL newCommandReceived;
	OS_MUTEX  newCommandCountLock;
	OS_SIGNAL threadDone;

	newWaitData.signal = NULL;
	newWaitData.running = 1;
	newWaitData.i2 = 0;
	newWaitData.i3 = (int) &newCommandCountLock;

	OS_MUTEX_INIT(&newCommandCountLock);
	OS_SIGNAL_INIT(&newCommandReceived);
	OS_SIGNAL_INIT(&threadDone);
	harness_createThread(WaitThread,  &newWaitData);
#endif

#if NOTIFY_VERBOSE
	printf("TestWaitMultiThread: threads %d, repetitions %d, MME_WAIT_NEW_COMMAND %s, transformer %s\n", threads, reps, newWait?"YES":"NO", transformerName );
#endif
	for (i=0; i<threads; i++) {
		ThreadData_t data;
		OS_SIGNAL    threadRunning;

		OS_SIGNAL_INIT(&threadRunning);

		data.signal  = &threadRunning;
		data.running = reps;
#if MULTITHREAD_NEWWAIT
		data.i2      = newWait;
#else
		data.i2      = (int) &threadDone;
#endif
		data.i3      = i;
		data.transformerName = (char*)transformerName;

		harness_createThread(TestWaitSingleThread, (void*)&data);
		OS_SIGNAL_WAIT(&threadRunning);
		OS_SIGNAL_DESTROY(&threadRunning);
	}

#if !MULTITHREAD_NEWWAIT
	/* Wait for threads to terminate */
	for (i=0; i<threads; i++) {
		OS_SIGNAL_WAIT(&threadDone);
	}
	OS_SIGNAL_DESTROY(&threadDone);

	/* The next two steps will terminate the new command waitor thread */
	newWaitData.running = 0;
	CreateNewCommandAndAbort(transformerName);
#endif

	/* Wait for threads */
	harness_waitForChildren();

#if NOTIFY_VERBOSE
	printf("Children done - new commands %d\n", newWaitData.i2);
#endif

#if MULTITHREAD_NEWWAIT
	assert(((reps*3 + 1) * threads + 1) == newWaitData.i2);
#else
	if (3*reps*threads + 1 != newWaitData.i2) {
		printf("FAILURE*** 3*reps*threads + 1 != newWaitData.i2\n");
	}
	assert(3*reps*threads + 1 == newWaitData.i2);
#endif
}


void ComprehensiveTests(void) {
	MME_Event_t    event;
	MME_Command_t* evtCmd;
	void*          userData;

#if !QUICK
	/* Flush out the last new command - MME has a one-deep buffer */
	MME(WaitCommand(NULL, 0, MME_WAIT_NEW_COMMAND, &event, &evtCmd, &userData));
#endif
	/* Local transformer */
        MME(RegisterTransformer("local_wait",
                                WaitTest_AbortCommand,
                                WaitTest_GetTransformerCapability,
                                WaitTest_InitTransformer,
                                WaitTest_ProcessCommand,
                                WaitTest_TermTransformer));
#if QUICK
	TestWaitMultiThread(3,  100,  0, "local_wait");
	TestWaitMultiThread(3,  100,  0, "com.st.mcdt.mme.wait");
#else
	/* Local transformers */
	TestWaitMultiThread(1,  1,      0, "local_wait");
	TestWaitMultiThread(1,  1,      1, "local_wait");

	TestWaitMultiThread(1,  10000,  0, "local_wait");
	TestWaitMultiThread(1,  10000,  1, "local_wait");

	TestWaitMultiThread(3,  10000,  0, "local_wait");
	TestWaitMultiThread(3,  10000,  1, "local_wait");
	TestWaitMultiThread(15, 1000,   1, "local_wait");

	/* Wait - no new wait */
	TestWaitMultiThread(1,  1,      0, "com.st.mcdt.mme.wait");
	TestWaitMultiThread(1,  20,     0, "com.st.mcdt.mme.wait");
	TestWaitMultiThread(15, 80,     0, "com.st.mcdt.mme.wait");
	TestWaitMultiThread(3,  8000,   0, "com.st.mcdt.mme.wait");
	TestWaitMultiThread(15, 700,    0, "com.st.mcdt.mme.wait");

	/* Wait - and new wait */
	TestWaitMultiThread(1,  1,      1, "com.st.mcdt.mme.wait");
	TestWaitMultiThread(1,  20,     1, "com.st.mcdt.mme.wait");
	TestWaitMultiThread(3,  50000,  1, "com.st.mcdt.mme.wait");
	TestWaitMultiThread(15, 5000,   1, "com.st.mcdt.mme.wait");
#endif
}
#endif /* ENABLE_MME_WAITCOMMAND */

static const char* test1InstanceName = TEST1_INSTANCE_NAME;
#define USERDATA ((void*) 0x78563412)

int main(void)
{
	MME_TransformerInitParams_t initParams = {
		sizeof(MME_TransformerInitParams_t),
		MME_PRIORITY_NORMAL,
#if 1
		TransformerCallback,
#else
                NULL,
#endif
		USERDATA,
		MME_LENGTH_BYTES(SimpleParamsInit),
		NULL,
	};
	MME_TransformerHandle_t transformerHandle;

	char inputBytes[] = "Hello world";
	char outputBytes[] = "aaaaaaaaaaa";
	char expectBytes[] = "dlrow olleH";
	MME_Command_t command = commandTemplate;
	MME_CommandStatus_t commandStatus = { sizeof(MME_CommandStatus_t) };
	struct BufferPassing bufferPassing = bufferPassingTemplate;
	MME_ScatterPage_t inputPage = scatterPageTemplate;
	MME_ScatterPage_t outputPage = scatterPageTemplate;

	SimpleParamsInit_t simpleParamsInit;
	SimpleParamsInit_t simpleParamsStatus;
	int i;
	MME_CommandStatus_t* statusPtr;
	MME_DataBuffer_t*    dataBufferList[2];

	/* bring up the API */
	harness_boot();
	MME(Init());

#ifdef USERMODE
        MME_E(MME_NOT_IMPLEMENTED, DeregisterTransport(TRANSPORT_NAME));
#else
	MME_E(MME_INVALID_ARGUMENT, DeregisterTransport(TRANSPORT_NAME));
	MME(RegisterTransport(TRANSPORT_NAME));
#endif

#if !QUICK
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
	OS_MUTEX_INIT(&callbackDataMutex);

	VERBOSE(printf(MACHINE "MME_InitTransformer(com.st.mcdt.mme.test)"));
	while (MME_SUCCESS != MME_InitTransformer("com.st.mcdt.mme.test", &initParams, &transformerHandle)) {
		harness_sleep(1);
	}
	VERBOSE(printf(MACHINE "transformer handle %d\n", transformerHandle));

	/* set up command */
	statusPtr = (MME_CommandStatus_t*) &command.CmdStatus;

        statusPtr->AdditionalInfo_p = (void*) &simpleParamsStatus;
        statusPtr->AdditionalInfoSize = MME_LENGTH_BYTES(SimpleParamsStatus);
	command.Param_p = NULL;
#ifndef USERMODE
	/* setup the input buffer */
	inputPage.Page_p = inputBytes;
	inputPage.Size = sizeof(inputBytes) - 1;
	bufferPassing.buffers[0].ScatterPages_p = &inputPage;
	bufferPassing.buffers[0].TotalSize = inputPage.Size;
	bufferPassing.buffers[0].NumberOfScatterPages = 1;
	/* setup the output buffer */
	outputPage.Page_p = outputBytes;
	outputPage.Size = sizeof(outputBytes) - 1;
	bufferPassing.buffers[1].ScatterPages_p = &outputPage;
	bufferPassing.buffers[1].TotalSize = outputPage.Size;
	bufferPassing.buffers[1].NumberOfScatterPages = 1;

	dataBufferList[0] = &bufferPassing.buffers[0];
	dataBufferList[1] = &bufferPassing.buffers[1];
#else
        MME(AllocDataBuffer(transformerHandle, sizeof(inputBytes), 0, &dataBufferList[0]));
        MME(AllocDataBuffer(transformerHandle, sizeof(outputBytes), 0, &dataBufferList[1]));
        memcpy(dataBufferList[0]->ScatterPages_p->Page_p, inputBytes,  sizeof(inputBytes));
        memcpy(dataBufferList[1]->ScatterPages_p->Page_p, outputBytes, sizeof(outputBytes));
        dataBufferList[0]->TotalSize = dataBufferList[0]->ScatterPages_p->Size = sizeof(inputBytes) - 1;
        dataBufferList[1]->TotalSize = dataBufferList[1]->ScatterPages_p->Size = sizeof(outputBytes) - 1;
#endif
	command.NumberInputBuffers = 1;
	command.NumberOutputBuffers = 1;
	command.DataBuffers_p = dataBufferList;
	command.CmdCode = MME_TRANSFORM;

	/* Send the command */
	MME(SendCommand(transformerHandle, &command));

	/* Wait for the callback to signal us for a data underflow */
	VERBOSE(printf(MACHINE "waiting for callback ... data underflow \n"));
	OS_SIGNAL_WAIT(&callbackReceived);
	assert(callbackEvent == MME_DATA_UNDERFLOW_EVT);
	assert(callbackData == USERDATA);
	OS_MUTEX_RELEASE(&callbackDataMutex);

	/* Wait for the callback to signal us for command completion */
	VERBOSE(printf(MACHINE "waiting for callback ... completion \n"));
	OS_SIGNAL_WAIT(&callbackReceived);
	assert(callbackEvent == MME_COMMAND_COMPLETED_EVT);
	assert(callbackData == USERDATA);
	OS_MUTEX_RELEASE(&callbackDataMutex);

	/* check that the output is correct */
        {
                char* resultBytes = (char*) dataBufferList[1]->ScatterPages_p->Page_p;
                VERBOSE(printf(MACHINE "transform complete, '%s' -> '%s'\n", inputBytes, resultBytes));
                assert(0 == strcmp((char *) expectBytes, (char *) resultBytes));
        }
	assert(MME_COMMAND_COMPLETED == command.CmdStatus.State);
	assert(MME_SUCCESS == command.CmdStatus.Error);

	assert(MME_PARAM(simpleParamsStatus, SimpleParamsStatus_status1)==TEST1_STATUS_INT);
#if !defined __LINUX__ || !defined __KERNEL__
	assert(MME_PARAM(simpleParamsStatus, SimpleParamsStatus_status2)==TEST1_STATUS_DOUBLE);
#endif

#ifdef ENABLE_MME_WAITCOMMAND
	/* Check MME_WaitCommand */
	ignoreCallback = 1;
	/* Send the command */
	MME(SendCommand(transformerHandle, &command));
        {
        	/* now try again with the callback enabled by using MME_WaitCommand instead */
                MME_Event_t    event;
                MME_Command_t* evtCmd;
                void*          userData;
                MME_ERROR      result;

	        memset(outputBytes, 'c', sizeof(outputBytes) - 1);
        	MME(SendCommand(transformerHandle, &command));
             
                /* Wait for data underflow */
                MME(WaitCommand(&(command.CmdStatus.CmdId), 1, 0, &event, &evtCmd, &userData));
        	assert(MME_DATA_UNDERFLOW_EVT == event);
        	assert(&command == evtCmd);
        	assert((void *) 0x78563412 == userData);

                /* Now wait for command completion */
        	MME(WaitCommand(&(command.CmdStatus.CmdId), 1, 0, &event, &evtCmd, &userData));
        	assert(MME_COMMAND_COMPLETED_EVT == event);
        	assert(&command == evtCmd);
        	assert((void *) 0x78563412 == userData);
        }
#endif

	/* tidy up */
#ifdef USERMODE
        MME(FreeDataBuffer(dataBufferList[0]));
        MME(FreeDataBuffer(dataBufferList[1]));
#endif
	MME(TermTransformer(transformerHandle));
#endif

#ifdef ENABLE_MME_WAITCOMMAND
	/* More comprehensive tests */
	ComprehensiveTests();
#endif

	MME(Term());
#if !QUICK
	OS_SIGNAL_DESTROY(&callbackReceived);
	OS_MUTEX_DESTROY(&callbackDataMutex);
#endif
	harness_shutdown();
	return 0;
}
