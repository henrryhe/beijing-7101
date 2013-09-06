/*=====================================================

File Name:
	CH_TUNER.H

Purpose:


Author:
	ZhouXuCheng

History:
	Date				Author			Actor
------------------------------------------------------
	2007-7-12		ZhouXuCheng		Create

======================================================*/



#ifndef	CH_TUNER_H_
#define	CH_TUNER_H_
/******************************Include files******************************/
#include "stddefs.h"
#include "..\dbase\vdbase.h"

/******************************Macro define******************************/


/*************************globle variable extern*************************/


/***************************Function externs***************************/


/**************************************************************
Fuction Name:
	CHDVBNewTuneReq

Author:
	ZhouXucheng

Purpose:
	Set the freqency and symbol

Input parameter:
	NULL
	
Output parameter:
	NULL
	
Return Status:
	NULL
**************************************************************/
void	CHDVBNewTuneReq ( TRANSPONDER_INFO_STRUCT *pstTransponderInfo );


/**************************************************************
Fuction Name:
	CH_TUNER_SetParameters

Purpose:


Using the globle and struct:

Input parameter:
	ri_FrequencyKHz:	Frequency in K Hz
	ri_SymbolRateKHz:	SymbolRate in K Hz
	ri_QamMode:		0 8 16 32 64 128 256
	ri_IQInversion:	0 or 1

Output parameter:
	NULL

Return Status:
	TRUE	FAILED
	FALSE	SUCCESFULL

Note:
	NULL

**************************************************************/
void CH_TUNER_SetParameters(int ri_FrequencyKHz, int ri_SymbolRateKHz, int ri_QamMode, int ri_IQInversion);



/**************************************************************
Fuction Name:
	CH_TUNER_IsLocked

Purpose:
	判断高频头是否锁定

Using the globle and struct:

Input parameter:
	NULL

Output parameter:
	NULL

Return Status:
	TRUE	高频头锁定状态
	FALSE	高频头未锁定状态

Note:
	NULL

**************************************************************/
BOOL CH_TUNER_IsLocked(void);



/**************************************************************
Fuction Name:
	CH_TUNER_GetStrengthAndQuality

Purpose:
	得到信号质量与强度函数，百分比显示

Using the globle and struct:

Input parameter:
	*rpi_Strength	

Output parameter:
	返回值:  0-100 分别代表0% - 100%

Return Status:
	TRUE	FAILED
	FALSE	SUCCESFULL

Note:
	NULL

**************************************************************/
void CH_TUNER_GetStrengthAndQuality(int *rpi_Strength, int *rpi_Quality);



/**************************************************************
Fuction Name:
	CH_TUNER_GetSNR

Purpose:
	Get the tuner signal noise rate in the db value
	得到信号的信噪比， 单位DB

Using the globle and struct:

Input parameter:
	NULL

Output parameter:
	NULL

Return Status:
	TRUE	FAILED
	FALSE	SUCCESFULL

Note:
	NULL

**************************************************************/
U32 CH_TUNER_GetSNR(void);




/**************************************************************
Fuction Name:
	CH_TUNER_GetRFLevel

Purpose:
	得到单独的信号强度，已经换算成0-100之间的值

Using the globle and struct:

Input parameter:
	NULL

Output parameter:
	NULL

Return Status:
	TRUE	FAILED
	FALSE	SUCCESFULL

Note:
	NULL

**************************************************************/
U32 CH_TUNER_GetRFLevel(void);



/**************************************************************
Fuction Name:
	CH_TUNER_GetBerRateString

Purpose:
	得到误码率的字符串。形式以xx E -4 提供

Using the globle and struct:

Input parameter:
	NULL

Output parameter:
	NULL

Return Status:
	TRUE	FAILED
	FALSE	SUCCESFULL

Note:
	NULL

**************************************************************/
void CH_TUNER_GetBerRateString(char *rpuc_BerString);



/**************************************************************
Fuction Name:
	CH_TUNER_GetBerRate

Purpose:
	得到信号的误码率
	1 代表 1的负4次方
	10000 代表1

Using the globle and struct:

Input parameter:
	NULL

Output parameter:
	NULL

Return Status:
	TRUE	FAILED
	FALSE	SUCCESFULL

Note:
	NULL

**************************************************************/
float CH_TUNER_GetBerRate(void);


/**************************************************************
Fuction Name:
	CH_TUNER_Setup

Purpose:
	初始化高频头，请保证此函数中的参数是否正确
	方法:先询问硬件工程师，得到RF的具体型号，然后
		把型号正确传送给CH_DVBTunerInit函数

Using the globle and struct:
	gST_PTSema :	6964的控制信号，主要用于控制锁定灯的状态
	请确保此变量的初始化在调用此函数之前

Input parameter:
	NULL

Output parameter:
	NULL

Return Status:
	TRUE	FAILED
	FALSE	SUCCESFULL

Note:
	NULL

**************************************************************/
int  CH_TUNER_Setup(void);


#endif


