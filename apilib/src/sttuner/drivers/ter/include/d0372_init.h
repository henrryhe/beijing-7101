#ifndef H_D0372_INIT
	#define H_D0372_INIT

	#include "ioreg.h"
	/**************************/	
	/*Registers and Fields difinitions*/
	
	#ifdef __cplusplus
	 extern "C"
	 {
	#endif                  /* __cplusplus */

	/* functions --------------------------------------------------------------- */

	/* create instance of STV0372 register mappings */
	ST_ErrorCode_t STV0372_VSB_Init(STTUNER_IOREG_DeviceMap_t *DeviceMap);   

	#ifdef __cplusplus
	 }
	#endif                  /* __cplusplus */
	
#endif

