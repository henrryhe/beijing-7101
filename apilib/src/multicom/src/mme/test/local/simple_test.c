/*
 * simple_test.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * A simple string reversing transformer.
 */

#include <mme.h>
#include "params.h"
#include "harness.h"

void harness_exit(int code, char* ch, ...) {
}

#if 0
#ifdef __KERNEL__
/* #define printf printk */
#define assert(x) harness_printf("assert line %d, file %s :- " #x, __LINE__, __FILE__)
#endif

#define VERBOSE(x) x
#define MACHINE
#endif

static	int	instanceType;
static	char	instanceName[16];

MME_ERROR SimpleTest_AbortCommand(void *context, MME_CommandId_t commandId) {
	/* Nothing to do */
	return MME_SUCCESS;
}

MME_ERROR SimpleTest_InitTransformer(MME_UINT paramsLength, MME_GenericParams_t params, void** handle)
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
				VERBOSE(printf("Src 0x%08x, Dst 0x%08x\n", src, dst ));
				/* Invert the buffer. */
				size = (dstSize < srcSize) ? dstSize : srcSize;
				printf("\nChars\n");
				for (i = 0; i < size; i++) {
					dst[size - 1 - i] = src[i];
					printf("%c (%d) ", src[i], (int)src[i]);
				}
				printf("\nSwap done\n");

				status->ProcessedTime = 0;
				dstPage->BytesUsed = size;
				{
					SimpleParamsStatus_t simpleParams;
					MME_PARAM(simpleParams, SimpleParamsStatus_status1) = 0x12345678;
#if 0
					MME_PARAM(simpleParams, SimpleParamsStatus_status2) = 1.0;
#endif
					VERBOSE(printf("Add bytes %d, copy bytes %d\n",
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
	/* we could check here that the transformerType is as expected, but it is not necessary. */
	capability->Version = 1;
	return MME_SUCCESS;
}

/* ================================================================
 * THe following code is used to initialize the module
 * ================================================================
 */
#ifdef __KERNEL__

#include <linux/fs.h>
#include <linux/module.h>

MODULE_DESCRIPTION("MME Module");
MODULE_AUTHOR("STMicroelectronics Ltd");
MODULE_LICENSE("Copyright 2004 STMicroelectronics, All rights reserved");

int init_module(void) {
  return MME_RegisterTransformer("com.st.mcdt.mme.test", 
                                SimpleTest_AbortCommand,
                                SimpleTest_GetTransformerCapability,
                                SimpleTest_InitTransformer,
                                SimpleTest_ProcessCommand, 
                                SimpleTest_TermTransformer);
  
}

void cleanup_module(void)
{
#if 0
  MME_DeregisterTransformer("com.st.mcdt.mme.test");
#endif
}

#endif
