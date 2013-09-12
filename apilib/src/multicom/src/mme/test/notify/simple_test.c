/*
 * simple_test.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * A simple string reversing transformer.
 */

#include "harness.h"

#include "simple_test.h"

static	int	instanceType;
static	char	instanceName[16];

MME_ERROR SimpleTest_AbortCommand(void *context, MME_CommandId_t commandId) {
	/* Nothing to do */
	return MME_SUCCESS;
}

MME_ERROR SimpleTest_InitTransformer(MME_UINT paramsLength, MME_GenericParams_t params, void **handle)
{
	int i;
	instanceType = MME_PARAM(params, SimpleParamsInit_instanceType);
	for (i=0; i<SimpleParamsInit_instanceNameLength; i++) {
		instanceName[i] = MME_INDEXED_PARAM(params, SimpleParamsInit_instanceName, i);
	}
	assert(instanceType==TEST1_HANDLE_VALUE);
	assert(strcmp(instanceName, TEST1_INSTANCE_NAME)==0);

	*handle = (void*)(TEST1_HANDLE_VALUE*2);
	return MME_SUCCESS;
}

MME_ERROR SimpleTest_ProcessCommand(void* context, MME_Command_t * commandInfo)
{
	MME_DataBuffer_t** buffer = commandInfo->DataBuffers_p;
	MME_ScatterPage_t *srcPage, *dstPage;
	unsigned char *src, *dst;
	MME_UINT srcSize, dstSize, size, i;
	SimpleParamsStatus_t simpleParams;

	VERBOSE(printf(MACHINE "SimpleTest_ProcessCommand\n"));
	VERBOSE(printf(MACHINE "SimpleTest_ProcessCommand inputBuffers %d, outputBuffers %d\n", 
                commandInfo->NumberInputBuffers, commandInfo->NumberOutputBuffers ));

	/* We want one input and one output buffer */
	if (commandInfo->NumberInputBuffers != 1 || commandInfo->NumberOutputBuffers != 1) {
		VERBOSE(printf(MACHINE "Incorrect buffers\n"));
		return MME_INVALID_ARGUMENT;
	}

	if ((int)context != (int)TEST1_HANDLE_VALUE*2) {
		VERBOSE(printf(MACHINE "Value of handle incorrect\n"));
		return MME_INVALID_HANDLE;
	} 

	/* The input buffer must be linear */
	srcPage = buffer[0]->ScatterPages_p;
	src = srcPage->Page_p;
	srcSize = srcPage->Size;

	VERBOSE(printf(MACHINE "src num pages %d, offset %d, totalSize %d, size %d\n",
			       buffer[0]->NumberOfScatterPages, buffer[0]->StartOffset, buffer[0]->TotalSize, srcSize));

	if (buffer[0]->NumberOfScatterPages != 1 || buffer[0]->StartOffset != 0 || buffer[0]->TotalSize < srcSize) {
		VERBOSE(printf(MACHINE "src is bad\n"));
		return MME_INVALID_ARGUMENT;
	}

	/* the output buffer must be linear. */
	dstPage = buffer[1]->ScatterPages_p;
	dst = dstPage->Page_p;
	dstSize = dstPage->Size;

	VERBOSE(printf(MACHINE "dst num pages %d, offset %d, totalSize %d, size %d\n",
				       buffer[1]->NumberOfScatterPages, buffer[1]->StartOffset, buffer[1]->TotalSize, dstSize));
	if (buffer[1]->NumberOfScatterPages != 1 || buffer[1]->StartOffset != 0 || buffer[1]->TotalSize < dstSize) {
		VERBOSE(printf(MACHINE "dst is bad\n"));
		return MME_INVALID_ARGUMENT;
	} 
	
	/* Invert the buffer. */
	size = (dstSize < srcSize) ? dstSize : srcSize;
	for (i = 0; i < size; i++) {
		dst[size - 1 - i] = src[i];
	}

	dstPage->BytesUsed = size;

	MME_PARAM(simpleParams, SimpleParamsStatus_status1) = 0x12345678;
	MME_PARAM(simpleParams, SimpleParamsStatus_status2) = 1.0;
	VERBOSE(printf("Add bytes %d, copy bytes %d\n", commandInfo->CmdStatus.AdditionalInfoSize,
                                                MME_LENGTH_BYTES(SimpleParamsStatus)));

	memcpy(commandInfo->CmdStatus.AdditionalInfo_p, simpleParams, MME_LENGTH_BYTES(SimpleParamsStatus));

	MME_NotifyHost(MME_DATA_UNDERFLOW_EVT, commandInfo, MME_DATA_UNDERFLOW);

	return MME_SUCCESS;
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

