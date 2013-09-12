/*
 * simple_test.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * A simple string reversing transformer.
 */

#define TRANSFORM(BYTE) (BYTE+10)

#include <mme.h>
#include <os_abstractions.h>

#include "harness.h"

#include "simple_test.h"

static int	 instanceType;
static char	 instanceName[16];

static int       repetitions;

extern unsigned instantiatedCounter;

int x[16384];	
int y[16384];
#define FLUSH_CACHE \
{ \
  /* Flush cache */ \
  int i; \
  x[0] = 1; \
  x[16383] = -1; \
  for (i=0; i<16384; i++) { \
    y[i] = x[i]+1; \
  } \
  printf( "x[0] %d, x[16383] %d, y[0] %d, y[16383] %d\n", x[0], x[16383], y[0], y[16383] ); \
}

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
	
	instantiatedCounter++;
	return MME_SUCCESS;
}

static MME_ERROR Transform(void* context, MME_Command_t * commandInfo)
{
	MME_DataBuffer_t **  bufferPtrs = commandInfo->DataBuffers_p;
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

	if (bufferPtrs[0]->TotalSize != bufferPtrs[1]->TotalSize) {
		VERBOSE(printf(MACHINE "Mismatched buffer sizes\n"));
		return MME_INVALID_ARGUMENT;
	}

        {
	unsigned count = 0, page = 0;
	unsigned char* srcScatterAddress = NULL;
	unsigned char* dstScatterAddress = NULL;

	while (page<bufferPtrs[0]->NumberOfScatterPages && page<bufferPtrs[1]->NumberOfScatterPages) {
		int i;
		MME_ScatterPage_t* srcScatterPage = &bufferPtrs[0]->ScatterPages_p[page];
		MME_ScatterPage_t* dstScatterPage = &bufferPtrs[1]->ScatterPages_p[page];
		srcScatterAddress = srcScatterPage->Page_p;
		dstScatterAddress = dstScatterPage->Page_p;
#if 0
printf("page %d src page 0x%08x dst page 0x%08x\n", page, srcScatterAddress, dstScatterAddress);
#endif
		for (i=0; i<srcScatterPage->Size && i<dstScatterPage->Size; i++) {
			*dstScatterAddress++ = TRANSFORM(*srcScatterAddress++);
#if 0
if (count == 10208 || count == 10272 || count == 10304 || count == 10336 || 
    count == 10368 || count == 10432 || count == 10432 || count == 11200) {	
	printf("count %d, src 0x%02x, dst 0x%02x\n", 
	       count, (int) *(srcScatterAddress-1), (int) *(dstScatterAddress-1) );
}
#endif
			count++;

		}
		page++;
	}
	}
	
	MME_PARAM(simpleParams, SimpleParamsStatus_status1) = TEST1_STATUS_INT;
	MME_PARAM(simpleParams, SimpleParamsStatus_status2) = TEST1_STATUS_CHAR;
	memcpy(commandInfo->CmdStatus.AdditionalInfo_p, simpleParams, MME_LENGTH_BYTES(SimpleParamsStatus));

/*	MME(NotifyHost(MME_COMMAND_COMPLETED_EVT, extraCmd, MME_SUCCESS));
*/
	return MME_SUCCESS;
}

static MME_ERROR AcceptBuffers(void* context, MME_Command_t * commandInfo)
{
#if 0
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

        return MME_SUCCESS;
#else
	return MME_INVALID_ARGUMENT;
#endif
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

	instantiatedCounter--;

	return MME_SUCCESS;
}

MME_ERROR SimpleTest_GetTransformerCapability(MME_TransformerCapability_t * capability)
{
	/* we could check here that the transformerType is as expected, but it is not necessary. */
	capability->Version = 1;
	return MME_SUCCESS;
}
