#ifndef  _SOFT_AUDIO_CODEC_INTERFACE_
#define  _SOFT_AUDIO_CODEC_INTERFACE_

#include "mme_interface.h"

extern MME_ERROR audiodecoder_GetTransformerCapability(MME_TransformerCapability_t* TransformerCaps_p);
extern MME_ERROR  audioencoder_GetTransformerCapability(MME_TransformerCapability_t* TransformerCaps_p);
extern MME_ERROR mixer_GetTransformerCapability(MME_TransformerCapability_t* TransformerCaps_p);
extern MME_ERROR  pcmprocessing_GetTransformerCapability(MME_TransformerCapability_t* TransformerCaps_p);

extern MME_ERROR audiodecoder_InitTransformer(MME_UINT initParamsLength,MME_GenericParams_t  Params_p, void **Handle);
extern MME_ERROR audioencoder_InitTransformer(MME_UINT initParamsLength,MME_GenericParams_t  Params_p, void **Handle);
extern MME_ERROR mixer_InitTransformer(MME_UINT initParamsLength,MME_GenericParams_t  Params_p, void **Handle);
extern MME_ERROR pcmprocessing_InitTransformer(MME_UINT initParamsLength,MME_GenericParams_t  Params_p, void **Handle);


extern MME_ERROR audiodecoder_ProcessCommand(void * Handle, MME_Command_t * CmdInfo_p);
extern MME_ERROR audionencoder_ProcessCommand(void * Handle, MME_Command_t * CmdInfo_p);
extern MME_ERROR mixer_ProcessCommand(void * Handle, MME_Command_t * CmdInfo_p);
extern MME_ERROR pcmprocessing_ProcessCommand(void * Handle, MME_Command_t * CmdInfo_p);

extern MME_ERROR audiodecoder_AbortCommand(void * handle, MME_CommandId_t cmd_id);
extern MME_ERROR audioencoder_AbortCommand(void * handle, MME_CommandId_t cmd_id);
extern MME_ERROR mixer_AbortCommand(void * handle, MME_CommandId_t cmd_id);
extern MME_ERROR pcmprocessing_AbortCommand(void * handle, MME_CommandId_t cmd_id);



extern MME_ERROR audiodecoder_TermTransformer(void * Handle);
extern MME_ERROR audioencoder_TermTransformer(void * Handle);
extern MME_ERROR mixer_TermTransformer(void * Handle);
extern MME_ERROR  pcmprocessing_TermTransformer(void * Handle);






#endif 



