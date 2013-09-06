/*=====================================================
Copy Right By:
	R&D  Institute,Sichuan  Changhong  Network  Technologies Co.,Ltd.

File Name:
	TCDRV.H

Version Number:
	1.0
Author:
	


Purpose:


History:
	Date				Author			Actor
------------------------------------------------------
	2007-9-9				Create

======================================================*/



#ifndef	TCDRV_H_
#define	TCDRV_H_
/******************************Include files******************************/
#include "stddefs.h"
#include "sttbx.h"
#include "ch_tuner.h"

/******************************Macro define******************************/


/******************************struct define******************************/

/*************************globle variable extern*************************/


/***************************Function externs***************************/


extern ST_ErrorCode_t tuner_tdrv_Init(void);


extern ST_ErrorCode_t tuner_tdrv_SetFrequency (U32 rui_FrequencyKHz);

#endif

