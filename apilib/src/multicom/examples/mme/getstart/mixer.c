/*
 * mixer.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Very simple two channel 8-bit mixer.
 */

#include <assert.h>
#include <stdlib.h>

#include <mme.h>
#include "mixer.h"

struct mixerCtx {
	float volume[2]; 
};

static MME_ERROR abortCommand(void *ctx, MME_CommandId_t cmdId)
{
	/* This transformer supports neither aborting the currently executing 
	 * transform nor does it utilize deferred transforms. For this reason
	 * there is nothing useful it can do here.
	 */

	return MME_INVALID_ARGUMENT;
}

static MME_ERROR getTransformerCapability(MME_TransformerCapability_t *capability)
{
	capability->Version = 0x010040;
	return MME_SUCCESS;
}

static MME_ERROR initTransformer(MME_UINT paramsSize, MME_GenericParams_t params, void **ctx)
{
	struct mixerCtx *mixerCtx;

	/* if we have a parameter structure ensure it is the correct size */
	if (paramsSize != 0 && paramsSize != MME_LENGTH_BYTES(MixerParamsInit)) {
		return MME_INVALID_ARGUMENT;
	}

	/* allocate the context structure */
	mixerCtx = malloc(sizeof(struct mixerCtx));
	if (!mixerCtx) {
		return MME_NOMEM;
	}

	/* extract the initialization parameters (if they are present) */
	mixerCtx->volume[0] = params ? MME_INDEXED_PARAM(params, MixerParamsInit_volume, 0) : 1.0f;
	mixerCtx->volume[1] = params ? MME_INDEXED_PARAM(params, MixerParamsInit_volume, 1) : 1.0f;

	/* supply the context pointer and return */
	*ctx = mixerCtx;
	return MME_SUCCESS;
}

static char *scatterPageIterator(char *ptr, char **endOfPage, MME_DataBuffer_t *buffer)
{
	unsigned int n, size;

	/* perform simple iteration */
	if (ptr != *endOfPage) {
		return ptr+1;
	}

	/* iterate to the next scatter page */
	if (ptr) {
		n = (buffer->ScatterPages_p[0].FlagsOut >> 24) + 1;
		size = buffer->ScatterPages_p[0].FlagsOut & ((1 >> 24) - 1);

		if (n >= buffer->NumberOfScatterPages) {
			buffer->ScatterPages_p[0].FlagsOut = 0;
			return 0;
		}

		size += buffer->ScatterPages_p[n].Size;
		buffer->ScatterPages_p[0].FlagsOut = (n << 24) + size;

		*endOfPage = (char *) buffer->ScatterPages_p[n].Page_p +
		             (buffer->ScatterPages_p[n].Size - 1) +
			     (size < buffer->TotalSize ? 0 : buffer->TotalSize - size);
		return (char *) buffer->ScatterPages_p[n].Page_p;
	}

	/* initialize a new iteration */
	n = 0;
	size = buffer->ScatterPages_p[0].Size - buffer->StartOffset;
	buffer->ScatterPages_p[0].FlagsOut = size;
	*endOfPage = (char *) buffer->ScatterPages_p[n].Page_p +
		     (buffer->ScatterPages_p[n].Size - 1) +
		     (size < buffer->TotalSize ? 0 : buffer->TotalSize - size);
	return (char *) buffer->ScatterPages_p[n].Page_p + buffer->StartOffset;
}

/* forcibly optimize single byte iteration (at the expense of code size) */
#define scatterPageIterator(ptr, endOfPage, buffer) \
	((char *) (ptr) != *(endOfPage) ? ((char *) (ptr))+1 : scatterPageIterator(ptr, endOfPage, buffer))

static MME_ERROR transform(void *ctx, MME_Command_t *cmd)
{
	struct mixerCtx *mixerCtx = ctx;

	MME_DataBuffer_t *in[2];
	MME_DataBuffer_t *out;
	char *in_p[2], *inEndOfPage[2] = { 0, 0 };
	char *out_p, *outEndOfPage = 0;

	assert(ctx);
	assert(cmd);

	if (2 != cmd->NumberInputBuffers || 1 != cmd->NumberOutputBuffers) {
		return MME_INVALID_ARGUMENT;
	}

	in[0] = cmd->DataBuffers_p[0];
	in[1] = cmd->DataBuffers_p[1];
	out = cmd->DataBuffers_p[2];

	/* check both input channels are of the same size */
	if (in[0]->TotalSize - in[0]->StartOffset != 
	    in[1]->TotalSize - in[1]->StartOffset) {
		return MME_DATA_UNDERFLOW;
	}

	/* check there is enough space for the mixed output */
	if (out->TotalSize - out->StartOffset < 
	    in[0]->TotalSize - in[1]->StartOffset) {
		return MME_DATA_OVERFLOW;
	}

	/* iterate over every sample from each of the buffers */
	for (in_p[0] = scatterPageIterator(0, &inEndOfPage[0], in[0]),
	     in_p[1] = scatterPageIterator(0, &inEndOfPage[1], in[1]),
	     out_p = scatterPageIterator(0, &outEndOfPage, out);

	     in_p[0];

	     in_p[0] = scatterPageIterator(in_p[0], &inEndOfPage[0], in[0]),
	     in_p[1] = scatterPageIterator(in_p[1], &inEndOfPage[1], in[1]),
	     out_p = scatterPageIterator(out_p, &outEndOfPage, out)) {
		
		/* a very simple floating point mix */
		float smpl = ((float) *in_p[0] * mixerCtx->volume[0] +
		              (float) *in_p[1] * mixerCtx->volume[1]);
		*out_p = (smpl > 255 ? 255 : (char) smpl);
	}

	/* interation should complete on the other input at the same time */
	assert(!in_p[1]);

	return MME_SUCCESS;
}

static MME_ERROR processCommand(void *ctx, MME_Command_t *cmd)
{
	struct mixerCtx *mixerCtx = ctx;

	switch (cmd->CmdCode) {
	case MME_TRANSFORM:
		return transform(ctx, cmd);

	case MME_SET_GLOBAL_TRANSFORM_PARAMS:
		if (MME_LENGTH_BYTES(MixerParamsInit) == cmd->ParamSize) {
			mixerCtx->volume[0] = MME_INDEXED_PARAM(cmd->Param_p, 
								MixerParamsInit_volume, 0);
			mixerCtx->volume[1] = MME_INDEXED_PARAM(cmd->Param_p, 
								MixerParamsInit_volume, 1);
			return MME_SUCCESS;
		}
		break;

	case MME_SEND_BUFFERS: /* not supported by this transformer */
	default:
		break;
	}

	return MME_INVALID_ARGUMENT;
}

static MME_ERROR termTransformer(void *ctx)
{
	free(ctx);
	return MME_SUCCESS;
}


MME_ERROR Mixer_RegisterTransformer(const char *name)
{
	return MME_RegisterTransformer(name, abortCommand, getTransformerCapability,
	                               initTransformer, processCommand, termTransformer);
}
