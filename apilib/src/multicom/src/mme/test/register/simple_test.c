/*
 * simple_test.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * A simple string reversing transformer.
 */

#include <mme.h>

#include "harness.h"

#include "simple_test.h"

MME_ERROR SimpleTest_AbortCommand(void *context, MME_CommandId_t commandId) {
	/* Nothing to do */
	return MME_SUCCESS;
}

MME_ERROR SimpleTest_InitTransformer(MME_UINT paramsLength, MME_GenericParams_t params, void** handle)
{
	*handle = (void*)(TEST1_HANDLE_VALUE*2);
	return MME_SUCCESS;
}

MME_ERROR SimpleTest_ProcessCommand(void* context, MME_Command_t * commandInfo)
{
	return MME_INVALID_HANDLE;
}

MME_ERROR SimpleTest_TermTransformer(void* context)
{
	if ((int)context != (int)TEST1_HANDLE_VALUE*2) {
		VERBOSE(printf("SimpleTest_TermTransformer: Incorrect context"));
		return MME_INVALID_HANDLE;
	}

	/* nothing to do. */
	return MME_SUCCESS;
}

MME_ERROR SimpleTest_GetTransformerCapability(MME_TransformerCapability_t * capability)
{
	/* we could check here that the transformerType is as expected, but it is not necessary. */
	capability->Version = 1;
	return MME_SUCCESS;
}
