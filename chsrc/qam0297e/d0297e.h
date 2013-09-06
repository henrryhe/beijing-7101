/*=====================================================
Copy Right By:
	R&D  Institute,Sichuan  Changhong  Network  Technologies Co.,Ltd.

File Name:
	D0297E.H

Version Number:
	1.0
Author:
	


Purpose:


History:
	Date				Author			Actor
------------------------------------------------------
	2007-9-9				Create

======================================================*/



#ifndef	D0297E_H_
#define	D0297E_H_
/******************************Include files******************************/
#include "stddefs.h"
#include "drv0297e.h"

/******************************Macro define******************************/


/******************************struct define******************************/


/*************************globle variable extern*************************/


/***************************Function externs***************************/
extern ST_ErrorCode_t demod_d0297E_Init(void);

extern ST_ErrorCode_t demod_d0297E_GetSignalQuality(U32  *SignalQuality_p, U32 *rpui_Strength, U32 *Ber_p, U32 *rpui_SNRdb);

extern ST_ErrorCode_t demod_d0297E_SetModulation(FE_297e_Modulation_t Modulation);

extern ST_ErrorCode_t demod_d0297E_GetAGC(S16  *Agc);

extern ST_ErrorCode_t demod_d0297E_IsLocked(BOOL *IsLocked);

extern ST_ErrorCode_t demod_d0297E_ScanFrequency(U32 rui_FrequencyKHz, U32 rui_SymbolRateBds, U32 rui_QamMode, U32 rui_Spectrum);
#endif


