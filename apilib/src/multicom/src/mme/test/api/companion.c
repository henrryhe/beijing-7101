/*
 * companion.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * API conformance test (companion side)
 */

/*{{{ includes */
#include <mme.h>

#include "harness.h"

#include "streaming.h"
#include "transformer.h"
/*}}}*/
/*{{{ macros */
#define TRANSPORT_NAME "shm"
/*}}}*/

/*{{{ check all functions to ensure rejection prior to initialization */
static void test_rejection_prior_to_initialization(void)
{
	/*{{{ MME_RegisterTransport: rejection prior to initialization */
	MME_E(MME_DRIVER_NOT_INITIALIZED, RegisterTransport(TRANSPORT_NAME));
	/*}}}*/
	/*{{{ MME_RegisterTransformer: rejection prior to initialization */
	MME_E(MME_DRIVER_NOT_INITIALIZED, RegisterTransformer("mme.test",
				SimpleTest_AbortCommand,
				SimpleTest_GetTransformerCapability,
				SimpleTest_InitTransformer,
				SimpleTest_ProcessCommand,
				SimpleTest_TermTransformer));
	/*}}}*/
	/*{{{ MME_RegisterTransport: rejection prior to initialization */
	MME_E(MME_DRIVER_NOT_INITIALIZED, Run());
	/*}}}*/
	/*{{{ MME_DeregisterTransport: rejection prior to initialization */
	MME_E(MME_DRIVER_NOT_INITIALIZED, DeregisterTransformer(0));
	/*}}}*/
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
	/*{{{ MME_RegisterTransport: rejection of NULL pointer */
	MME_E(MME_INVALID_ARGUMENT,RegisterTransport(0));
	/*}}}*/
	/*{{{ MME_RegisterTransport: successful registration */
	MME(RegisterTransport(TRANSPORT_NAME));
	/*}}}*/
	/*{{{ MME_RegisterTransport: rejection of already registered transport */
#ifdef INSbl21634
	MME_E(MME_INVALID_ARGUMENT,RegisterTransport(TRANSPORT_NAME));
#endif
	/*}}}*/
	/*{{{ MME_DeregisterTransport: rejection of NULL pointer */
	MME_E(MME_INVALID_ARGUMENT,DeregisterTransport(0));
	/*}}}*/
	/*{{{ MME_DeregisterTransport: successful deregistration of previously registered transport */
#ifdef INSbl21676
	MME(DeregisterTransport(TRANSPORT_NAME));
#endif
	/*}}}*/
	/*{{{ MME_DeregisterTransport: rejection of both invalid and already deregistered transport */
#ifdef INSbl21676
	MME_E(MME_INVALID_ARGUMENT,DeregisterTransport(TRANSPORT_NAME));
	MME_E(MME_INVALID_ARGUMENT,DeregisterTransport("incorrect_value"));
#endif
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

	/* before we exit we must make sure there really *is* a registered transport (and
	 * transformer) for the rest of the test to use
	 */
#ifdef INSbl21676
	MME(RegisterTransport(TRANSPORT_NAME));
#endif
	MME(RegisterTransformer("mme.test", SimpleTest_AbortCommand,
				SimpleTest_GetTransformerCapability, SimpleTest_InitTransformer,
				SimpleTest_ProcessCommand, SimpleTest_TermTransformer));
	MME(RegisterTransformer("mme.streaming", Streaming_AbortCommand,
				Streaming_GetTransformerCapability, Streaming_InitTransformer,
				Streaming_ProcessCommand, Streaming_TermTransformer));
}
/*}}}*/

/*{{{ boot and run */
int main(void)
{
	harness_boot();

	test_rejection_prior_to_initialization();
	test_init_and_register();

	/*{{{ MME_Run: successful operation of command dispatch loop */
	MME(Run());
	/*}}}*/

	harness_shutdown();
	return 0;
}
/*}}}*/
