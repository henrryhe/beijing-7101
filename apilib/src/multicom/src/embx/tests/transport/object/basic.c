/*
 * object/basic.c
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Fully symetric test of the fundamental parts of object handling.
 */

#include "harness.h"

/* total number of each type of handle this test creates (the 
 * actual number registered will be four times this number)
 */
#define NUM_HANDLES 4

/* convert an iteration count into the expected size of a buffer */
#define GET_SIZE(i) (4 << (i*3))

int main(void)
{
	/* wow! what a lot of local variables! */
	EMBX_TRANSPORT       transport;
	EMBX_PORT            inPort;
	EMBX_PORT            outPort;
	EMBX_VOID          **clibPointers;	/* pointers allocated using malloc() */
	EMBX_VOID          **embxPointers;	/* pointers allocated using EMBX_Alloc() */
	EMBX_HANDLE         *handles;		/* set of handles derived from above pointers */
	EMBX_UINT            sizeOfHandles;	/* size in bytes of the handles list */
	EMBX_RECEIVE_EVENT   event;
	EMBX_VOID           *object;
	EMBX_UINT            sizeOfObject;
	int                  i;

	/* boot and setup fundamentals of comms. */
	harness_boot();
	transport = harness_openTransport();
	harness_connect(transport, "register", &inPort, &outPort);

	/* allocate somewhere to store all the pointers and handles we are
	 * going to make
	 */
	clibPointers = (EMBX_VOID **) malloc(NUM_HANDLES * sizeof(EMBX_VOID *));
	assert(clibPointers);
	embxPointers = (EMBX_VOID **) malloc(NUM_HANDLES * sizeof(EMBX_VOID *));
	assert(embxPointers);
	sizeOfHandles = 2 * NUM_HANDLES * sizeof(EMBX_HANDLE);
	EMBX(Alloc(transport, sizeOfHandles, (EMBX_VOID *) &handles));
	printf(MACHINE"allocated administrative memory\n");

	for (i=0; i<NUM_HANDLES; i++) {
		/* create and register an object created from the C library heap */
		clibPointers[i] = malloc(GET_SIZE(i));
		harness_fillPattern(clibPointers[i], GET_SIZE(i), 0x0f);
		EMBX(RegisterObject(transport, clibPointers[i], GET_SIZE(i), &(handles[2 * i])));

		/* create and register an object created from the EMBX heap */
		EMBX(Alloc(transport, GET_SIZE(i), &(embxPointers[i])));
		harness_fillPattern(embxPointers[i], GET_SIZE(i), 0xf0);
		EMBX(RegisterObject(transport, embxPointers[i], GET_SIZE(i), &(handles[(2 * i) + 1])));
	}
	printf(MACHINE"allocated test memory\n");

	/* swap the list of object handles with our partner */
	EMBX(SendMessage(outPort, (void *) handles, sizeOfHandles));
	EMBX(ReceiveBlock(inPort, &event));
	assert(EMBX_REC_MESSAGE == event.type);
	assert(sizeOfHandles == event.size);
	handles = (EMBX_HANDLE *) event.data;
	printf(MACHINE"swapped handle lists\n");
	/* handles is now the set of handles registered remotely */

	/* check that the object handles are valid on the other side 
	 * *before* they have ever been updated
	 */
	for (i=0; i<NUM_HANDLES; i++) {
		EMBX(GetObject(transport, handles[(2 * i)    ], &object, &sizeOfObject));
		assert(GET_SIZE(i) == sizeOfObject);
		EMBX(UpdateObject(outPort, handles[(2 * i)    ], 0, sizeOfObject));

		EMBX(GetObject(transport, handles[(2 * i) + 1], &object, &sizeOfObject));
		assert(GET_SIZE(i) == sizeOfObject);
		EMBX(UpdateObject(outPort, handles[(2 * i) + 1], 0, sizeOfObject));
	}
	printf(MACHINE"handles are correctly registered\n");

	/* attempt illegal deregistration of objects (they were created on the 
	 * other CPU) 
	 */
	for (i=0; i<NUM_HANDLES; i++) {
		EMBX_E(EMBX_INVALID_ARGUMENT, DeregisterObject(transport, handles[(2 * i)    ]));
		EMBX_E(EMBX_INVALID_ARGUMENT, DeregisterObject(transport, handles[(2 * i) + 1]));
	}
	printf(MACHINE"handles cannot be deregistered (this is good)\n");

	/* now fill and update half the objects */
	for (i=0; i<NUM_HANDLES; i+=2) {
		EMBX(GetObject(transport, handles[(2 * i)    ], &object, &sizeOfObject));
		assert(GET_SIZE(i) == sizeOfObject);
		harness_fillPattern(object, sizeOfObject, i);
		EMBX(UpdateObject(outPort, handles[(2 * i)    ], 0, sizeOfObject));

		EMBX(GetObject(transport, handles[(2 * i) + 1], &object, &sizeOfObject));
		assert(GET_SIZE(i) == sizeOfObject);
		harness_fillPattern(object, sizeOfObject, i);
		EMBX(UpdateObject(outPort, handles[(2 * i) + 1], 0, sizeOfObject));
	}
	printf(MACHINE"updated half the objects\n");

	/* swap the list of object handles with our partner */
	EMBX(SendMessage(outPort, (void *) handles, sizeOfHandles));
	EMBX(ReceiveBlock(inPort, &event));
	assert(EMBX_REC_MESSAGE == event.type);
	assert(sizeOfHandles == event.size);
	handles = (EMBX_HANDLE *) event.data;
	printf(MACHINE"swapped handle lists\n");
	/* handles is now the set of handles registered locally */

	/* check that the objects now point to matching data */
	for (i=0; i<NUM_HANDLES; i+=2) {
		EMBX(GetObject(transport, handles[(2 * i)    ], &object, &sizeOfObject));
		assert(GET_SIZE(i) == sizeOfObject);
		assert(object == clibPointers[i]);
		assert(harness_testPattern(object, sizeOfObject, i));

		EMBX(GetObject(transport, handles[(2 * i) + 1], &object, &sizeOfObject));
		assert(GET_SIZE(i) == sizeOfObject);
		assert(object == embxPointers[i]);
		assert(harness_testPattern(object, sizeOfObject, i));
	}
	printf(MACHINE"half the objects correctly updated\n");

	/* now fill and update the other half of the objects */
	for (i=1; i<NUM_HANDLES; i+=2) {
		EMBX(GetObject(transport, handles[(2 * i)    ], &object, &sizeOfObject));
		assert(GET_SIZE(i) == sizeOfObject);
		harness_fillPattern(object, sizeOfObject, i);
		EMBX(UpdateObject(outPort, handles[(2 * i)    ], 0, sizeOfObject));

		EMBX(GetObject(transport, handles[(2 * i) + 1], &object, &sizeOfObject));
		assert(GET_SIZE(i) == sizeOfObject);
		harness_fillPattern(object, sizeOfObject, i);
		EMBX(UpdateObject(outPort, handles[(2 * i) + 1], 0, sizeOfObject));
	}
	printf(MACHINE"updated the remaining objects\n");

	/* swap the list of object handles with our partner */
	EMBX(SendMessage(outPort, (void *) handles, sizeOfHandles));
	EMBX(ReceiveBlock(inPort, &event));
	assert(EMBX_REC_MESSAGE == event.type);
	assert(sizeOfHandles == event.size);
	handles = (EMBX_HANDLE *) event.data;
	printf(MACHINE"swapped handle lists\n");
	/* handles is now the set of handles registered remotely */

	/* check that the objects now point to matching data */
	for (i=0; i<NUM_HANDLES; i++) {
		EMBX(GetObject(transport, handles[(2 * i)    ], &object, &sizeOfObject));
		assert(GET_SIZE(i) == sizeOfObject);
		assert(harness_testPattern(object, sizeOfObject, i));

		EMBX(GetObject(transport, handles[(2 * i) + 1], &object, &sizeOfObject));
		assert(GET_SIZE(i) == sizeOfObject);
		assert(harness_testPattern(object, sizeOfObject, i));
	}
	printf(MACHINE"all objects correctly updated\n");

	/* swap the list of object handles with our partner */
	EMBX(SendMessage(outPort, (void *) handles, sizeOfHandles));
	EMBX(ReceiveBlock(inPort, &event));
	assert(EMBX_REC_MESSAGE == event.type);
	assert(sizeOfHandles == event.size);
	handles = (EMBX_HANDLE *) event.data;
	printf(MACHINE"swapped handle lists\n");
	/* handles is now the set of handles registered locally */

	/* the following tests all exercise the SendObject() call and
	 * have different loop semantics meaning that the GET_SIZE()
	 * macro will yield incorrect results.
	 */

	/* alter the value stored in the object and use SendObject to
	 * post it to the other side 
	 */
	for (i=0; i<(NUM_HANDLES * 2); i++) {
		EMBX(GetObject(transport, handles[i], &object, &sizeOfObject));
		harness_fillPattern(object, sizeOfObject, i + NUM_HANDLES);
		EMBX(SendObject(outPort, handles[i], 0, sizeOfObject));
	}
	printf(MACHINE"sent all objects to partner\n");

	/* now receive the object, check its contents, alter its 
	 * contents and send it back
	 */
	for (i=0; i<(NUM_HANDLES * 2); i++) {
		EMBX(ReceiveBlock(inPort, &event));
		assert(EMBX_REC_OBJECT == event.type);

		/* check that that data pointer and size pointer are correct */
		EMBX(GetObject(transport, event.handle, &object, &sizeOfObject));
		assert(event.data == object);
		assert(event.size == sizeOfObject);

		/* validate the contents */
		assert(harness_testPattern(object, sizeOfObject, i + NUM_HANDLES));

		/* alter the contents and send it back */
		harness_fillPattern(object, sizeOfObject, i);
		EMBX(SendObject(outPort, event.handle, 0, sizeOfObject));
	}
	printf(MACHINE"received, validated and re-sent all objects to partner\n");

	/* receive the object and check its contents */
	for (i=0; i<(NUM_HANDLES * 2); i++) {
		EMBX(ReceiveBlock(inPort, &event));
		assert(EMBX_REC_OBJECT == event.type);

		/* check we still have the correct pointer! */
		if (i & 1) {
			assert(event.data == embxPointers[i >> 1]);
		} else {
			assert(event.data == clibPointers[i >> 1]);
		}

		/* validate the contents */
		assert(harness_testPattern(event.data, event.size, i));
	}
	printf(MACHINE"received and validated all objects to partner\n");

	/* clean up */
	for (i=0; i<NUM_HANDLES; i++) {
		/* deregister and free an object created from the C library heap */
		EMBX(DeregisterObject(transport, handles[(2 * i)    ]));
		free(clibPointers[i]);

		/* deregister and free an object created from the EMBX heap */
		EMBX(DeregisterObject(transport, handles[(2 * i) + 1]));
		EMBX(Free(embxPointers[i]));
	}
	printf(MACHINE"cleaned up OK\n");

	/* use the handles after deregistration and check for failure */
	for (i=0; i<(NUM_HANDLES * 2); i++) {
		EMBX_E(EMBX_INVALID_ARGUMENT, GetObject(transport, handles[i], &object, &sizeOfObject));
	}

	/* close the ports and shutdown */
	EMBX(ClosePort(inPort));
	EMBX(ClosePort(outPort));
	harness_shutdown();
	return 0;
}
