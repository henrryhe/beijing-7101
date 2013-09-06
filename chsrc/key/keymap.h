/*
 * USIF - Remote Control mapping
 */



#ifndef __keymap_h__
#define __keymap_h__



/*yxl 2005-04-04 add below section*/
#ifdef USE_NEC_REMOTE
#include "NEC_keymap.h"
#endif
/*end yxl 2005-04-04 add below section*/


/*yxl 2005-04-04 add below section*/
#ifdef USE_RC6_REMOTE
#include "RC6_keymap.h"

#endif


#define NEC_SYSTEM_CODE 0x805F
#define RC6_SYSTEM_CODE 0x80
/*end yxl 2005-04-04 add below section*/

/*20051220 add for front key*/
/*yxl 2005-08-23 add below line,前面板按键对应的头文件*/

#if 0 /*yxl 2006-05-03 modify below section*/
#include "..\main\ch_frontkey.h"

#else
	#ifdef LEDANDBUTTON_CONTROL_MODE_BY_ATMEL
		#include "..\ch_led\ch_frontkey_ATMEL.h"
	#endif

	#ifdef LEDANDBUTTON_CONTROL_MODE_BY_PT6964
		#include "..\ch_led\ch_frontkey_PT6964.h"
	#endif
	#ifdef LEDANDBUTTON_CONTROL_MODE_BY_PT6315
		#include "..\ch_led\ch_frontkey_PT6315.h"
	#endif

#endif  /*end yxl 2006-05-03 modify below section*/

/****************************/
#endif


