/*=====================================================
File Name:
	CH_TUNER.C

Purpose:


Author:
	ZhouXuCheng

History:
	Date				Author			Actor
------------------------------------------------------
	2007-7-12		ZhouXuCheng		Create

======================================================*/



/******************************Include files******************************/
#include "ch_tuner.h"
#include "ch_tuner_mid.h"

/******************************Macro define******************************/

/******************************struct define******************************/


/*************************globle variable define*************************/



/***************************Function defines***************************/






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
void	CHDVBNewTuneReq ( TRANSPONDER_INFO_STRUCT *pstTransponderInfo )
{
	int iFrequencyKHz;
	int iSymbolRateKHz;
	int iIQInversion;
	int iQamMode;

	iFrequencyKHz = pstTransponderInfo->iTransponderFreq;
	iSymbolRateKHz = pstTransponderInfo->iSymbolRate;
	iQamMode = pstTransponderInfo->ucQAMMode;
	iIQInversion = pstTransponderInfo->abiIQInversion;

	ch_Tuner_MID_SetParameter(iFrequencyKHz, iSymbolRateKHz, iQamMode, iIQInversion);
		
}

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
void CH_TUNER_SetParameters(int ri_FrequencyKHz, int ri_SymbolRateKHz, int ri_QamMode, int ri_IQInversion)
{
	ch_Tuner_MID_SetParameter(ri_FrequencyKHz, ri_SymbolRateKHz, ri_QamMode, ri_IQInversion);
}


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
BOOL CH_TUNER_IsLocked(void)
{
	return ch_Tuner_MID_StillLocked();
}


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
void CH_TUNER_GetStrengthAndQuality(int *rpi_Strength, int *rpi_Quality)
{
	ch_Tuner_MID_GetStrengthAndQuality(rpi_Strength, rpi_Quality);
}


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
U32 CH_TUNER_GetSNR(void)
{
	return ch_Tuner_MID_GetSNR();
}



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
U32 CH_TUNER_GetRFLevel(void)
{
	return ch_Tuner_MID_GetRFLevel();

}


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
void CH_TUNER_GetBerRateString(char *rpuc_BerString)
{	
	U32  ui_BerRate;
	
	ui_BerRate = ch_Tuner_MID_GetBerRate();
	sprintf(rpuc_BerString, "%d E -4", ui_BerRate);
}


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
float CH_TUNER_GetBerRate(void)
{
	U32  ui_BerRate;
	
	ui_BerRate = ch_Tuner_MID_GetBerRate();

	return (float)(ui_BerRate/10000);
}

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
int  CH_TUNER_Setup(void)
{
	return ch_DVBTunerInit(/*TUNER_PLL_DCT70700*/TUNER_PLL_CD11XX);
}

/*增加为了兼容原来的API调用*/

U8 CH_GetNoiseEstimator(void)
{
  /* FE_297e_SignalInfo_t TestSignalInfo;
      FE_297e_GetSignalInfo(myHandle,	&TestSignalInfo);*/
      int SignalQuality;
      int SignalStrength;
  
      CH_TUNER_GetStrengthAndQuality(&SignalQuality,&SignalStrength);
      return SignalQuality;

}
 
U8 CH_GetPowerEstimator(void)
{
   /*FE_297e_GetSignalInfo(myHandle,FE_297e_SignalInfo_t	*pInfo)*/
      int SignalQuality;
      int SignalStrength;
  
      CH_TUNER_GetStrengthAndQuality(&SignalQuality,&SignalStrength);
      return SignalStrength;
}




boolean CH_IsLocked(void)
{
  return CH_TUNER_IsLocked();
}

void CH_GetSignalinfo(int *SignalQuality, int *SignalStrength)
{
   CH_TUNER_GetStrengthAndQuality(SignalQuality,SignalStrength);
}

boolean SRTNR_IsLocked()
{
	return CH_IsLocked();
}


