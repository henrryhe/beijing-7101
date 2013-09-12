/*
 * simple_test.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * A simple string reversing transformer.
 */

#include <mme.h>
#include <os_abstractions.h>

#include "harness.h"

#include "simple_test.h"

static int	 instanceType;
static char	 instanceName[16];

static int       repetitions;
static int       extraSize;
static char*     extraPage;
static MME_Command_t *extraCmd;

static OS_SIGNAL receivedParams;
static OS_SIGNAL receivedBuffers;

MME_ERROR SimpleTest_AbortCommand(void *context, MME_CommandId_t commandId) {
	/* Nothing to do */
	return MME_SUCCESS;
}

MME_ERROR SimpleTest_InitTransformer(MME_UINT paramsLength, MME_GenericParams_t params, void** handle)
{
	int   i;
	long valueA;
	short valueB;
	char  valueC;

	/* extract the list parameters */
	instanceType = MME_PARAM(params, SimpleParamsInit_instanceType);
	for (i=0; i<SimpleParamsInit_instanceNameLength; i++) {
		instanceName[i] = MME_INDEXED_PARAM(params, SimpleParamsInit_instanceName, i);
	}
	assert(instanceType == TEST1_HANDLE_VALUE);
	assert(strcmp(instanceName, TEST1_INSTANCE_NAME) == 0);

	/* extract the sub list parameters */
	{

		SimpleParamsSubInit_t* subParams = MME_PARAM_SUBLIST(params, SimpleParamsInit_values);
		valueA = MME_PARAM( *subParams, SimpleParamsSubInit_valueA);
		valueB = MME_PARAM( *subParams, SimpleParamsSubInit_valueB);
		valueC = MME_PARAM( *subParams, SimpleParamsSubInit_valueC);

		VERBOSE(printf(MACHINE"value A %f, value B %d, value C %d\n", valueA, (int)valueB, (int)valueC));

	}
	assert(valueA == 2*valueB + valueC);

	*handle = (void*)(TEST1_HANDLE_VALUE*2);

	OS_SIGNAL_INIT(&receivedParams);
	OS_SIGNAL_INIT(&receivedBuffers);

	return MME_SUCCESS;
}

static MME_ERROR Transform(void* context, MME_Command_t * commandInfo)
{
	MME_DataBuffer_t **bufferPtrs = commandInfo->DataBuffers_p;
        MME_DataBuffer_t* buffer;
	MME_ScatterPage_t *srcPage, *dstPage;
	unsigned char *src, *dst;
	MME_UINT srcSize, dstSize, size, i, bytesCopied;

	SimpleParamsStatus_t simpleParams;

	VERBOSE(printf(MACHINE "SimpleTest_ProcessCommand inputBuffers %d, outputBuffers %d\n", 
                commandInfo->NumberInputBuffers, commandInfo->NumberOutputBuffers ));

	/* we want one input and one output buffer. */
	if (commandInfo->NumberInputBuffers != 1 || commandInfo->NumberOutputBuffers != 1) {
		VERBOSE(printf(MACHINE "Incorrect buffers\n"));
		return MME_INVALID_ARGUMENT;
	}

	if ((int)context != (int)TEST1_HANDLE_VALUE*2) {
		VERBOSE(printf(MACHINE "Value of handle incorrect\n"));
		return MME_INVALID_HANDLE;
	}

	/* the input buffer must be linear. */
        buffer  = bufferPtrs[0];
	srcPage = buffer->ScatterPages_p;
	src     = srcPage->Page_p;
	srcSize = srcPage->Size;

	VERBOSE(printf(MACHINE "src num pages %d, offset %d, totalSize %d, size %d\n",
		       buffer->NumberOfScatterPages, buffer->StartOffset, buffer->TotalSize, srcSize));
	if (buffer->NumberOfScatterPages != 1 || buffer->StartOffset != 0 || buffer->TotalSize < srcSize) {
		VERBOSE(printf(MACHINE "src is bad\n"));
		return MME_INVALID_ARGUMENT;
        }
	
        buffer  = bufferPtrs[1];
	dstPage = buffer->ScatterPages_p;
	dst     = dstPage->Page_p;
	dstSize = dstPage->Size;

	VERBOSE(printf(MACHINE "dst num pages %d, offset %d, totalSize %d, size %d\n",
		       buffer->NumberOfScatterPages, buffer->StartOffset, buffer->TotalSize, dstSize));

	if (buffer->NumberOfScatterPages != 1 || buffer->StartOffset != 0 || buffer->TotalSize < dstSize) {
		VERBOSE(printf(MACHINE "dst is bad\n"));
		return MME_INVALID_ARGUMENT;
	}

	/* Wait for the transform params */
	OS_SIGNAL_WAIT(&receivedParams);

	/* Now wait for the extra buffers */
	OS_SIGNAL_WAIT(&receivedBuffers);

	/* Invert the buffer. */
	size = (dstSize < srcSize) ? dstSize : srcSize;
	for (i = 0; i < size; i++) {
		dst[size - 1 - i] = src[i];
	}

	bytesCopied=0;
	while (bytesCopied<extraSize) {
		for (i = 0; i < size && bytesCopied<extraSize; i++, bytesCopied++, extraPage++) {
			*extraPage = dst[i];
		}
	}

	dstPage->BytesUsed = size;

	MME_PARAM(simpleParams, SimpleParamsStatus_status1) = 0x12345678;
	MME_PARAM(simpleParams, SimpleParamsStatus_status2) = 1.0;
	VERBOSE(printf("Add bytes %d, copy bytes %d\n", commandInfo->CmdStatus.AdditionalInfoSize, MME_LENGTH_BYTES(SimpleParamsStatus)));
	memcpy(commandInfo->CmdStatus.AdditionalInfo_p, simpleParams, MME_LENGTH_BYTES(SimpleParamsStatus));

	MME(NotifyHost(MME_COMMAND_COMPLETED_EVT, extraCmd, MME_SUCCESS));
	return MME_SUCCESS;
}

static MME_ERROR AcceptBuffers(void* context, MME_Command_t * commandInfo)
{
	int			numBuffers = commandInfo->NumberOutputBuffers;
	MME_DataBuffer_t**      bufferPtrs = commandInfo->DataBuffers_p;
	MME_ScatterPage_t*	dstPage;

	VERBOSE(printf("AcceptBuffers: numBuffers %d\n", numBuffers));

	if (numBuffers<1) {
		return MME_INVALID_ARGUMENT;
	}

	dstPage   = bufferPtrs[0]->ScatterPages_p;
	extraPage = dstPage->Page_p;
	extraSize = dstPage->Size;
	extraCmd = commandInfo;

	VERBOSE(printf("AcceptBuffers: extraSize %d, extraPage 0x%08x\n", extraSize, extraPage));

	OS_SIGNAL_POST(&receivedBuffers);

        return MME_SUCCESS;
}

static MME_ERROR SetParams(void* context, MME_Command_t * commandInfo)
{
	short         repetitionsA;
	unsigned char repetitionsB;

	/* Get the params */
	repetitionsA = MME_PARAM(*((SimpleParamsTransform_t*)commandInfo->Param_p), SimpleParamsTransform_repetitionsA);
	repetitionsB = MME_PARAM(*((SimpleParamsTransform_t*)commandInfo->Param_p), SimpleParamsTransform_repetitionsB);

	VERBOSE(printf("SetParams: Repititions A %d\n, Repititions B %d\n", repetitionsA, repetitionsB));

	repetitions = repetitionsA + repetitionsB;
	if (repetitions<1) {
		return MME_INVALID_ARGUMENT;
	}

	OS_SIGNAL_POST(&receivedParams);

        return MME_SUCCESS;
}

MME_ERROR SimpleTest_ProcessCommand(void* context, MME_Command_t * commandInfo)
{
	MME_ERROR result;

	VERBOSE(printf(MACHINE "SimpleTest_ProcessCommand\n"));

	switch (commandInfo->CmdCode) {
	case MME_SEND_BUFFERS:
		result = AcceptBuffers(context, commandInfo);
		break;
	case MME_TRANSFORM:
		result = Transform(context, commandInfo);
		break;
	case MME_SET_GLOBAL_TRANSFORM_PARAMS:
		result = SetParams(context, commandInfo);
		break;
	default:
		VERBOSE(printf("Unrecognised command\n"));
		result = MME_INVALID_ARGUMENT;
		break;
	}

	return result;
}

MME_ERROR SimpleTest_TermTransformer(void* context)
{
	if ((int)context != (int)TEST1_HANDLE_VALUE*2) {
		VERBOSE(printf("SimpleTest_TermTransformer: Incorrect context"));
		return MME_INVALID_HANDLE;
	}

	OS_SIGNAL_DESTROY(&receivedParams);
	OS_SIGNAL_DESTROY(&receivedBuffers);

	return MME_SUCCESS;
}

MME_ERROR SimpleTest_GetTransformerCapability(MME_TransformerCapability_t * capability)
{
	/* we could check here that the transformerType is as expected, but it is not necessary. */
	capability->Version = 1;
	return MME_SUCCESS;
}
