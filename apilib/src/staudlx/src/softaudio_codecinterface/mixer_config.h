/*$Id: mixer_config.h,v 1.4 2004/03/10 18:43:51 lassureg Exp $*/

#ifndef _MIXER_CONFIG_H_
#define _MIXER_CONFIG_H_
#ifdef _STANDALONE_
#include "acc_defines.h"
#include "acc_api.h"
#include "mixer_defines.h"
#else
#include "acc_mmedefines.h"
#include "mixer_processortypes.h"
#include "mixer_defines.h"
#define CHANS_SUPPORTED   2
#endif
typedef struct
{
#ifdef _STANDALONE_
	enum eBoolean        Enable;
#else
	enum eAccBoolean        Enable;
#endif
	unsigned int         NbInput;
#ifdef _STANDALONE_
	int		             Alpha[MIXER_NB_MAX_INPUT];
	enum eMainChannelIdx FirstOutChan[MIXER_NB_MAX_INPUT];
	enum eBoolean        Fade;
	#ifdef _PCM_BYPASS_
	enum eBoolean Swap[MIXER_NB_MAX_INPUT];
#endif
#else
	int		             Alpha[ACC_MIXER_MAX_NB_INPUT];
	enum eAccMainChannelIdx FirstOutChan[ACC_MIXER_MAX_NB_INPUT];
	enum eAccBoolean        Fade;
#ifdef _PCM_BYPASS_
	enum eAccBoolean Swap[MIXER_NB_MAX_INPUT];
#endif
#endif
#ifdef _VOL_
	int Volume[CHANS_SUPPORTED];
#endif

}
tMixerConfig;

#endif /* _MIXER_CONFIG_H_ */
