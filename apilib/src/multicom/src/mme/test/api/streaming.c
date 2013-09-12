/*
 * streaming.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * An example 'streaming' transformer.
 */


#include <mme.h>

#include "harness.h"
#include "streaming.h"

/* MEM_FREE is found in winnt.h as a macro so we must undefine it */
#undef MEM_ALLOC
#undef MEM_FREE

#if defined __LINUX__ && defined __KERNEL__
#   include <linux/slab.h>
#   define MEM_ALLOC(X) kmalloc(X, GFP_KERNEL)
#   define MEM_FREE(X)  kfree(X)
#else
#   include <ctype.h>
#   define MEM_ALLOC(X) malloc(X)
#   define MEM_FREE(X)  free(X)
#endif

typedef struct StreamingContext {
	/* this mode selects whether the transformer uses the defered transformer
	 * infrastructure to implement the command itself
	 */
	int mode;

	/* a list of deferred transform requests */
	MME_Command_t *deferred[2];

	/* lists of send buffer commands */
	MME_Command_t *in[2], *out[2];

	/* the position of the current output buffer */
	MME_Command_t *dst;
	char *dstp;
	MME_UINT dsti;
} StreamingContext_t;


/* extract a buffer (input or output) from the context structure */
static MME_Command_t *get_buffer(StreamingContext_t *ctx, MME_Command_t *bufs[2])
{
	MME_Command_t *buf;

	if (!bufs[0]) {
		/* TODO: wait for output to exist */
		assert(0);
	}

	/* TODO: this is not thread safe */
	buf = bufs[0];
	bufs[0] = bufs[1];
	bufs[1] = NULL;

	return buf;
}


/* this is a very fast scatter page aware iterator for an MME_DataBuffer_t */
static MME_UINT next_address(MME_UINT state, void **ptr, MME_UINT stride, MME_DataBuffer_t *buf)
{
	MME_UINT i;

	/* perform the iteration */
	state -= stride;
	*ptr = ((char *) (*ptr)) + stride;

	/* return directly if the guard bit is unset set (simple iteration) */
	if (0 == (state & (1 << 23))) {
		return state;
	}

	/* calculate the index into the list of scatter pages of the next page */
	i = ((state >> 24) > 254 ? 0 : (state >> 24) + 1);

	/* if the index is valid construct a new state variable and return it */
	if (i < buf->NumberOfScatterPages) {
		*ptr = buf->ScatterPages_p[i].Page_p;
		assert(buf->ScatterPages_p[i].Size < (1 << 23));
		return (i << 24) | buf->ScatterPages_p[i].Size;
	}

	/* if we have reached the end of the data buffer return 0 */
	return 0;
}


/* this is the actual transformer. it is a huge glob of loop iteration code
 * wrapped around a single toupper() call.
 */
static MME_ERROR do_transform(StreamingContext_t *ctx, MME_Command_t *command)
{
	MME_Command_t *src;
	char *srcp;
	MME_UINT srci;

	/* this is a 'pure' streaming transformer so we are going to fail if any
	 * buffers are supplied with the command 
	 *
	 * NOTE: normally a transformer like this one that is paced by the input
	 * buffer would not take streamed input.
	 */
	if (command->NumberInputBuffers || command->NumberOutputBuffers) {
		return MME_INVALID_ARGUMENT;
	}

	/* take a single input frame and try to pack it into an output 
	 * frame. iterate a byte at a time over the input buffer and 
	 * iterate the output buffers to keep up
	 *
	 * NOTE: in this code we access dst, dsti and dstp via the ctx 
	 * pointer for clarity. technically it would be faster to copy 
	 * these into local variables and restore them at the end.
	 */
	src = get_buffer(ctx, ctx->in);
	srci = 0;
	while (0 != (srci = next_address(srci, (void **) &srcp, 1, *src->DataBuffers_p))) {
		if (0 == ctx->dsti) {
			ctx->dst = get_buffer(ctx, ctx->out);
			ctx->dsti = next_address(0, (void **) &ctx->dstp, 1, *ctx->dst->DataBuffers_p);
		}

		/* convert the input buffer to upper case - don't use toupper coz of Liunx kern mode */
                {
                        char src = *srcp;
                        if (src >= 'a' && src <= 'z') {
                                src -= ( 'A' - 'a' );
                        }
                        *ctx->dstp = src;
                }

		/* we must advance the destination buffer before we test the source
		 * buffer to ensure that the destination buffer completes on the host
		 * as soon as it is full.
		 */
		ctx->dsti = next_address(ctx->dsti, (void **) &ctx->dstp, 1, *ctx->dst->DataBuffers_p);
		if (0 == ctx->dsti) {
			MME(NotifyHost(MME_COMMAND_COMPLETED_EVT, ctx->dst, MME_SUCCESS));
		}
	}


	MME(NotifyHost(MME_COMMAND_COMPLETED_EVT, src, MME_SUCCESS));
	return MME_SUCCESS;	
}


/* a delayed version of do_command() where transforms will not complete until
 * two transforms after they were issued.
 *
 * this is purely a demonstration. the usual purpose for deferred transforms
 * is to delegate a transformer to a piece of hardware.
 */
static MME_ERROR do_deferred_transform(StreamingContext_t *ctx, MME_Command_t *command)
{
	if (NULL != ctx->deferred[0]) {
		MME_ERROR err;

		err = do_transform(ctx, ctx->deferred[0]);
		MME(NotifyHost(MME_COMMAND_COMPLETED_EVT, ctx->deferred[0], err));
	}

	/* this is thread safe since no other entity is allowed to
	 * alter the deferred list
	 */
	ctx->deferred[0] = ctx->deferred[1];
	ctx->deferred[1] = command;

	return MME_TRANSFORM_DEFERRED;
}


/* extract the global (and initialization) parameters and put them into 
 * the context structure 
 */
static void update_context(StreamingContext_t *ctx, MME_GenericParams_t *params)
{
	ctx->mode = MME_PARAM(params, StreamingGlobalParams_mode);

	/* clear out any pending defered transformers */
	if (0 == ctx->mode) {
		(void) do_deferred_transform(ctx, NULL);
		(void) do_deferred_transform(ctx, NULL);
	}
}


static MME_ERROR enqueue_buffers(StreamingContext_t *ctx, MME_Command_t *command)
{
	MME_Command_t **list = (command->NumberInputBuffers ? ctx->in : ctx->out);

	if ( (command->NumberInputBuffers && command->NumberOutputBuffers) ||
	     (1 != command->NumberInputBuffers + command->NumberOutputBuffers) ) {
		return MME_INVALID_ARGUMENT;
	}
	
	if (NULL != list[1]) {
		return MME_NOMEM;
	}

	/* this hardcodes a list length of two */
	list[(list[0] ? 1 : 0)] = command;

	return MME_TRANSFORM_DEFERRED;
}

MME_ERROR Streaming_AbortCommand(void *ctx, MME_CommandId_t id)
{
	/* TODO: since this transformer supports deferred completion it is
	 * quite possible to implement support for abortion.
	 */
	return MME_INVALID_ARGUMENT;
}

MME_ERROR Streaming_InitTransformer(MME_UINT length, MME_GenericParams_t params, 
                                    void** ctx_p)
{
	StreamingContext_t *ctx;

	if (0 != length && sizeof(StreamingGlobalParams_t) != length) {
		return MME_INVALID_ARGUMENT;
	}

        ctx = MEM_ALLOC(sizeof(StreamingContext_t));

	if (0 == ctx) {
		return MME_NOMEM;
	}
        memset(ctx, 0, sizeof(StreamingContext_t));

	if (0 != length) {
		update_context(ctx, params);
	}

	*ctx_p = ctx;
	return MME_SUCCESS;
}

MME_ERROR Streaming_ProcessCommand(void* voidctx, MME_Command_t *command)
{
	StreamingContext_t *ctx = voidctx;
	MME_UINT len;

	len = MME_SET_GLOBAL_TRANSFORM_PARAMS == command->CmdCode ?
	      MME_LENGTH_BYTES(StreamingGlobalParams) : 0;
	if (len != command->ParamSize) {
		return MME_INVALID_ARGUMENT;
	}

	switch (command->CmdCode) {
	case MME_SET_GLOBAL_TRANSFORM_PARAMS:
		update_context(ctx, command->Param_p);
		return MME_SUCCESS;

	case MME_TRANSFORM:
		switch (ctx->mode) {
		case 0:
			return do_transform(ctx, command);
		case 1:
			return do_deferred_transform(ctx, command);
		default:
			return MME_INTERNAL_ERROR;
		}

	case MME_SEND_BUFFERS:
		return enqueue_buffers(ctx, command);
	
	default:
		break;
	}

	return MME_INVALID_ARGUMENT;
}

MME_ERROR Streaming_TermTransformer(void* voidctx)
{
	StreamingContext_t *ctx = voidctx;

	/* release our context pointer */
        MEM_FREE(ctx);
	return MME_SUCCESS;
}

MME_ERROR Streaming_GetTransformerCapability(MME_TransformerCapability_t * capability)
{
	MME_DataFormat_t c = { { 'a', 's', 'c', 'i' } };

	capability->Version = 0x12003400;
	capability->InputType = c;
	capability->OutputType = c;

	return MME_SUCCESS;
}


