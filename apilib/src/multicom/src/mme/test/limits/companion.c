/*
 * companion.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Simple MME message test.
 */

#ifdef __OS20__
/* The internal partition on OS20 leaks with transient tasks */
#include <partitio.h>
#endif

#include <mme.h>

#include "harness.h"
#include "simple_test.h"

#define TRANSPORT_NAME "shm"
unsigned instantiatedCounter;

int main(void)
{
#ifdef __OS20__
        /* The internal partition on OS20 leaks with transient tasks */
        internal_partition = system_partition;
#endif

	harness_boot();

	MME_Init();
	MME_RegisterTransport(TRANSPORT_NAME);

	MME(RegisterTransformer("com.st.mcdt.mme.test",
				SimpleTest_AbortCommand,
				SimpleTest_GetTransformerCapability,
				SimpleTest_InitTransformer,
				SimpleTest_ProcessCommand,
				SimpleTest_TermTransformer));

	MME(Run());

	/* INSbl23335: prove that the remote termination function really was called */
	assert(0 == instantiatedCounter);

	harness_shutdown();
	return 0;
}
