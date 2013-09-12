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

static void TransformerCallback(MME_Event_t event, MME_Command_t *CallbackData, void *userData)
{
}

#define USERDATA ((void*) 0x78563412)

/* TODO:
 * Multiple companion CPU tests
 */
int main(void)
{
	MME_TransformerInitParams_t initParams = {
		sizeof(MME_TransformerInitParams_t),
		MME_PRIORITY_NORMAL,
		TransformerCallback,
		USERDATA,
		0,
		NULL,
	};
	MME_TransformerHandle_t transformerHandle;
	MME_TransformerHandle_t transformerHandle2;

	MME_ERROR error;

	/* bring up the API */
	harness_boot();
	MME(Init());

#ifndef USERMODE
	assert(MME_DeregisterTransport(TRANSPORT_NAME)==MME_INVALID_ARGUMENT);
	MME(RegisterTransport(TRANSPORT_NAME));
#endif

#if 0
	/* Currently a bug in EMBX prevents a transport from being 
           closed and reopened
         */
	printf("Calling MME_DeregisterTransport\n");
	error = MME_DeregisterTransport(TRANSPORT_NAME);
	printf("MME_DeregisterTransport err %d\n", error);

	VERBOSE(printf(MACHINE "MME_InitTransformer(com.st.mcdt.mme.test)"));
	/* this should fail - no transports yet */
	error = MME_InitTransformer("com.st.mcdt.mme.test", &initParams, &transformerHandle);
	assert(error == MME_UNKNOWN_TRANSFORMER);


	MME(RegisterTransport(TRANSPORT_NAME));

#endif

	/* this should fail - not a valid xformer */
	error = MME_InitTransformer("com.st.mcdt.mme.testX", &initParams, &transformerHandle);
	assert(error == MME_UNKNOWN_TRANSFORMER);

        do {
		VERBOSE(printf("Attempting MME_InitTransformer(com.st.mcdt.mme.test)\n"));
		/* loop till registered on companion */
		error = MME_InitTransformer("com.st.mcdt.mme.test", &initParams, &transformerHandle);
		harness_sleep(1);
	} while (error == MME_UNKNOWN_TRANSFORMER);
    

	/* And again - two instantiations on the same CPU */
	MME(InitTransformer("com.st.mcdt.mme.test", &initParams, &transformerHandle2));

	/* tidy up */
	MME(TermTransformer(transformerHandle));

	error = MME_Term();
	assert(error == MME_HANDLES_STILL_OPEN);

	MME(TermTransformer(transformerHandle2));

	MME(Term());

	harness_shutdown();
	return 0;
}
