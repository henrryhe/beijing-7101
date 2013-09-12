/*
 * transformer.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * A simple string reversing transformer.
 */

#ifndef DEBUG
#define DEBUG
#endif

#include "harness.h"

#include "transformer.h"

static	int	instanceType;
static	char	instanceName[16];

MME_ERROR SimpleTest_AbortCommand(void *context, MME_CommandId_t commandId) {
	/* Nothing to do */
	return MME_INVALID_ARGUMENT;
}

MME_ERROR SimpleTest_InitTransformer(MME_UINT paramsLength, MME_GenericParams_t params, void** handle)
{
	int i;

	if (0 == params && 0 == paramsLength)
	{
		return MME_INVALID_ARGUMENT;
	}

	instanceType = MME_PARAM(params, SimpleParamsInit_instanceType);
	for (i=0; i<SimpleParamsInit_instanceNameLength; i++) {
		instanceName[i] = MME_INDEXED_PARAM(params, SimpleParamsInit_instanceName, i);
	}
	/* using asserts to guarantee the values of parametric data is *wrong*! */
	assert(instanceType==TEST1_HANDLE_VALUE);
	assert(strcmp(instanceName, TEST1_INSTANCE_NAME)==0);

	*handle = (void*)(TEST1_HANDLE_VALUE*2);
	return MME_SUCCESS;
}

MME_ERROR SimpleTest_ProcessCommand(void* context, MME_Command_t * commandInfo)
{
	MME_CommandStatus_t *status = &commandInfo->CmdStatus;
	MME_DataBuffer_t **buffer = commandInfo->DataBuffers_p;
	MME_ERROR result;
	MME_ScatterPage_t *srcPage, *dstPage;
	unsigned char *src, *dst;
	MME_UINT srcSize, dstSize, size, i;

	VERBOSE(printf(MACHINE "SimpleTest_ProcessCommand\n"));

	VERBOSE(printf(MACHINE "SimpleTest_ProcessCommand inputBuffers %d, outputBuffers %d\n", 
                commandInfo->NumberInputBuffers, commandInfo->NumberOutputBuffers ));
	/* we want one input and one output buffer. */
	if (commandInfo->NumberInputBuffers != 1 || commandInfo->NumberOutputBuffers != 1) {
		VERBOSE(printf(MACHINE "Incorrect buffers\n"));
		return MME_INVALID_ARGUMENT;
	}

	if ((int)context != (int)TEST1_HANDLE_VALUE*2) {
		VERBOSE(printf(MACHINE "Value of handle incorrect\n"));
		result = MME_INVALID_HANDLE;
	} else {
		/* this is a special test mode that guarantees it takes at least two seconds to
		 * complete a transform command. it works by waiting for 20 tenths of a second
		 * so that if we are preempted we do not complete immediately (this would break
		 * some of the tests)
		 */
		if (0 != MME_PARAM(commandInfo->Param_p, TransformerCommandParams_SlowMode)) {
			unsigned int i, when, tenth = harness_getTicksPerSecond() / 10;

			/* maths workaround for simulators */
			if (0 == tenth) { tenth = 1; }

			for (i=0; i<20; i++) {
				when = harness_getTime() + tenth;
				while (when > harness_getTime()) {
                                        /*
                                         * For Linux kernel host transformer  must cooperate with other 
                                         * threads - namely the main execution thread of the test harness 
                                        */
                                        harness_schedule();
                                }
			}
		}

		/* the input buffer must be linear. */
		srcPage = buffer[0]->ScatterPages_p;
		src = srcPage->Page_p;
		srcSize = srcPage->Size;

		VERBOSE(printf(MACHINE "src num pages %d, offset %d, totalSize %d, size %d\n",
			       buffer[0]->NumberOfScatterPages, buffer[0]->StartOffset, buffer[0]->TotalSize, srcSize));
		if (buffer[0]->NumberOfScatterPages != 1 || buffer[0]->StartOffset != 0 || buffer[0]->TotalSize < srcSize) {
			VERBOSE(printf(MACHINE "src is bad\n"));
			result = MME_INVALID_ARGUMENT;
		} else {
			/* the output buffer must be linear. */
			dstPage = buffer[1]->ScatterPages_p;
			dst = dstPage->Page_p;
			dstSize = dstPage->Size;

			VERBOSE(printf(MACHINE "dst num pages %d, offset %d, totalSize %d, size %d\n",
				       buffer[1]->NumberOfScatterPages, buffer[1]->StartOffset, buffer[1]->TotalSize, dstSize));
			if (buffer[1]->NumberOfScatterPages != 1 || 
                            buffer[1]->StartOffset != 0 || 
                            buffer[1]->TotalSize < dstSize) {
				VERBOSE(printf(MACHINE "dst is bad\n"));
				result = MME_INVALID_ARGUMENT;
			} else {
				/* Invert the buffer. */
				size = (dstSize < srcSize) ? dstSize : srcSize;
				for (i = 0; i < size; i++) {
					dst[size - 1 - i] = src[i];
				}

				status->ProcessedTime = 0;
				dstPage->BytesUsed = size;
				{
					SimpleParamsStatus_t simpleParams;
					MME_PARAM(simpleParams, SimpleParamsStatus_status1) = 0x12345678;
#if !defined __LINUX__ || !defined __KERNEL__ || !defined __SH4__
					MME_PARAM(simpleParams, SimpleParamsStatus_status2) = 1.0;
#endif
					VERBOSE(printf(MACHINE "Add bytes %d, copy bytes %d\n",
                                                       commandInfo->CmdStatus.AdditionalInfoSize,
                                                       MME_LENGTH_BYTES(SimpleParamsStatus)));
					memcpy(commandInfo->CmdStatus.AdditionalInfo_p, simpleParams, MME_LENGTH_BYTES(SimpleParamsStatus));
				}
				return MME_SUCCESS;	/* success */
			}
		}
	}

	return result;
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
	/* for simple case test */
	if (0 == capability->TransformerInfo_p) {
		capability->Version = 1;
		return MME_SUCCESS;
	}

	/* for params test */
	if (16 == capability->TransformerInfoSize) {
		int *p = (void *) capability->TransformerInfo_p;
		p[0] = 0x12345000;
		p[1] = 0x02345600;
		p[2] = 0x00345670;
		p[3] = 0x00045678;
		capability->Version = 2;
		return MME_SUCCESS;
	}
	
	/* for failure test */
	return MME_INVALID_ARGUMENT; 
}
