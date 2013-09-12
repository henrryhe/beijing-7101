/*
 * companion.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Simple MME message test.
 */

#include <mme.h>

#include "harness.h"

#include "simple_test.h"

#define TRANSPORT_NAME "shm"

int main(void)
{
	char transformerName[] = "com.st.mcdt.mme.test";

	harness_boot();

	MME_Init();
	MME_RegisterTransport(TRANSPORT_NAME);
#if 0
    /* Once the EMBX transport reuse bug is fixed this can be
       enabled
     */

	printf("Calling MME_DeregisterTransport\n" );
	MME_DeregisterTransport(TRANSPORT_NAME);
	printf("MME_DeregisterTransport err %d\n", error);

	MME_RegisterTransport(TRANSPORT_NAME);
#endif

	/* INSbl21680: Register transformer must record a copy of the name argument. */
	MME(RegisterTransformer(transformerName,
				SimpleTest_AbortCommand,
				SimpleTest_GetTransformerCapability,
				SimpleTest_InitTransformer,
				SimpleTest_ProcessCommand,
				SimpleTest_TermTransformer));
	memset(transformerName, 0, sizeof(transformerName));

	MME(Run());

#if 0
    /* Once the EMBX transport reuse bug is fixed this can be
       enabled
     */

	/* Run again - the host deregistered the transport */
	MME_RegisterTransport(TRANSPORT_NAME);
	MME(RegisterTransformer("com.st.mcdt.mme.test",
				SimpleTest_AbortCommand,
				SimpleTest_GetTransformerCapability,
				SimpleTest_InitTransformer,
				SimpleTest_ProcessCommand,
				SimpleTest_TermTransformer));

	MME(Run());
#endif
	harness_shutdown();
	return 0;
}
