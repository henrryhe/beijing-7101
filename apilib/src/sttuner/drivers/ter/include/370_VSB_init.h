#ifndef H_370_VSB_INIT
	#define H_370_VSB_INIT

	#include "ioreg.h"
	/**************************/	
	/*Registers and Fields difinitions*/
	
	#ifdef __cplusplus
	 extern "C"
	 {
	#endif                  /* __cplusplus */

	/* functions --------------------------------------------------------------- */

	/* create instance of stb370 8VSB register mappings */
	ST_ErrorCode_t STB0370_VSB_Init(STTUNER_IOREG_DeviceMap_t *DeviceMap);   

	#ifdef __cplusplus
	 }
	#endif                  /* __cplusplus */
	
#endif

