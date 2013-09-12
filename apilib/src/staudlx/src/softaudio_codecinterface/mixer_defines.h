/*$Id: mixer_defines.h,v 1.7 2004/03/10 18:43:51 lassureg Exp $*/

#ifndef _MIXER_DEFINES_H_
#define _MIXER_DEFINES_H_

#define __PREFETCH(a,b) __builtin_prefetch((char *)(a)+(b))

#define MIXER_NB_MAX_INPUT            4
#define MIXER_NB_MAX_OUTPUT           1
#define MIXER_NB_MAX_POST_PROCESSINGS 4

#define MIXER_MAX_BLOCK_SIZE        128

#define MIXER_MIN_Q31 ( 0x80000000 )
#define MIXER_MAX_Q31 ( 0x7FFFFFFF )

#define MIXER_ALPHA_1_0 ( 0x7FFF0000 )

#define _VOL_
#define _USE_16BIT_
#define _ST20_C_
#define _ST20_ASM_
#define _PCM_BYPASS_

#endif /* _MIXER_DEFINES_H_ */
