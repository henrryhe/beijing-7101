/*
 * streaming.h
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * An example 'streaming' transformer.
 */

enum StreamingGlobalParams {
	MME_OFFSET_StreamingGlobalParams_mode,
	MME_LENGTH_StreamingGlobalParams

#define MME_TYPE_StreamingGlobalParams_mode MME_UINT
};
typedef MME_GENERIC64 StreamingGlobalParams_t[MME_LENGTH(StreamingGlobalParams)];


MME_ERROR Streaming_AbortCommand(void *ctx, MME_CommandId_t id);
MME_ERROR Streaming_InitTransformer(MME_UINT length, MME_GenericParams_t params, void** ctx_p);
MME_ERROR Streaming_ProcessCommand(void* voidctx, MME_Command_t *command);
MME_ERROR Streaming_TermTransformer(void* voidctx);
MME_ERROR Streaming_GetTransformerCapability(MME_TransformerCapability_t * capability);
