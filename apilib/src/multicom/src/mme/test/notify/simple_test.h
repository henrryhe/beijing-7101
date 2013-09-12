#ifndef SIMPLE_TEST_H
#define SIMPLE_TEST_H

#include <mme.h>
#include "params.h"

MME_ERROR SimpleTest_AbortCommand(void *context, MME_CommandId_t commandId);
MME_ERROR SimpleTest_GetTransformerCapability(MME_TransformerCapability_t * capability);
MME_ERROR SimpleTest_InitTransformer(MME_UINT paramsLength, MME_GenericParams_t params, void** handle);
MME_ERROR SimpleTest_ProcessCommand(void *context, MME_Command_t * commandInfo);
MME_ERROR SimpleTest_TermTransformer(void *context);


#endif
