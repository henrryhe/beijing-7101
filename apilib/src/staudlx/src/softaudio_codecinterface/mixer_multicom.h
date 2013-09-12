/********************************************************************************
 *   Filename   	: mixer_multicom.h											*
 *   Start       	: 27.06.2007                                                *
 *   By          	: Satej Pankey											    *
 *   Contact     	: satej.pankey@st.com										*
 *   Description  	: The file contains the Mixer Multicom Layer				*
 *				 										                        *
 ********************************************************************************
 */

#ifndef _MIXER_MULTICOM_H_
#define _MIXER_MULTICOM_H_

#include "stcodec_interface.h"
#ifdef ST_51XX
#include "mme_interface.h"
#else
#include "mme.h"
#endif
#include "staudlx.h"
#include "audio_encodertypes.h"
#include "mixer_config.h"
#include "mixer_processortypes.h"
#include "mixer_multicom_heap.h"

extern MME_ERROR mixer_GetTransformerCapability(MME_TransformerCapability_t* TransformerCaps_p);

extern MME_ERROR mixer_InitTransformer(MME_UINT initParamsLength,MME_GenericParams_t Params_p, void **Handle);

extern MME_ERROR mixer_ProcessCommand(void * Handle, MME_Command_t * CmdInfo_p);

extern MME_ERROR mixer_AbortCommand(void * Handle, MME_CommandId_t commandId);

extern MME_ERROR mixer_TermTransformer(void * Handle);

#endif
