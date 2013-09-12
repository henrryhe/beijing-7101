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
	harness_boot();

	MME(Init());
	MME(RegisterTransport(TRANSPORT_NAME));
	MME(RegisterTransformer("com.st.mcdt.mme.test",
				SimpleTest_AbortCommand,
				SimpleTest_GetTransformerCapability,
				SimpleTest_InitTransformer,
				SimpleTest_ProcessCommand,
				SimpleTest_TermTransformer));

	MME(Run());

	harness_shutdown();
	return 0;
}
