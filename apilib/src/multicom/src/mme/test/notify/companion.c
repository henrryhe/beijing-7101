/*
 * companion.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Simple MME message test.
 */

#include "harness.h"

#include "simple_test.h"
#ifdef ENABLE_MME_WAITCOMMAND
#include "wait_test.h"
#endif

#define TRANSPORT_NAME "shm"

int main(void)
{
	harness_boot();

	MME(Init());
	MME(RegisterTransport(TRANSPORT_NAME));
	MME(RegisterTransformer("com.st.mcdt.mme.test",
				SimpleTest_AbortCommand,
				SimpleTest_GetTransformerCapability,
				SimpleTest_InitTransformer,
				SimpleTest_ProcessCommand,
				SimpleTest_TermTransformer));

#ifdef ENABLE_MME_WAITCOMMAND
	MME(RegisterTransformer("com.st.mcdt.mme.wait",
				WaitTest_AbortCommand,
				WaitTest_GetTransformerCapability,
				WaitTest_InitTransformer,
				WaitTest_ProcessCommand,
				WaitTest_TermTransformer));
#endif

	MME(Run());

	harness_shutdown();
	return 0;
}
