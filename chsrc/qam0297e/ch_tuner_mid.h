/*=====================================================
Copy Right By:
	R&D  Institute,Sichuan  Changhong  Network  Technologies Co.,Ltd.

File Name:
	CH_TUNER_MID.H

Version Number:
	1.0
Author:
	Zhou Xu Cheng


Purpose:


History:
	Date				Author			Actor
------------------------------------------------------
	2007-9-27		Zhou Xu Cheng		Create

======================================================*/



#ifndef	CH_TUNER_MID_H_
#define	_CH_TUNER_MID_H_
/******************************Include files******************************/
#include "..\ch_led\ch_pt6964.h"
#include "sti2c.h"


/******************************Macro define******************************/
#define TUNER_PROCESS_STACK_SIZE                              8192
#define TUNER_PROCESS_PRIORITY                              	6


#define Tuner_UnLock			TunerLightOff()/*STPIO_Set(LockHandle,Lock_MASK)*/
#define Tuner_Lock			TunerLightOn()/*STPIO_Clear(LockHandle,Lock_MASK)*/


/******************************struct define******************************/
typedef enum
{
	MESSAGE_LEVEL_NORMAL = 0,
	MESSAGE_LEVEL_SERIOUS,
	MESSAGE_LEVEL_DEADLY
} ch_tuner_debug_leve_t;


typedef enum tuner_type
{
	CABLE_TUNER_ALPS_TDBE2 = 0,
	CABLE_TUNER_PHILIPS_CD1516_I,
	CABLE_TUNER_NOKIA_DF1CS1223,
	CABLE_TUNER_THOMSON_DCT7710,
       CABLE_TUNER_THOMSON_DCF8726,
	TUNER_DCT7040,
	TUNER_PLL_CD1616LF,
	TUNER_PLL_DCT70700,
	TUNER_PLL_THOMSON_DCT7042A,
	TUNER_PLL_CD11XX            /*20080114 add PHILIPS CD11XX 系列tuner*/
}CH_TunerType_t;


typedef enum
{
	TUNER_IDLE,
	SIGNAL_DETECTION,
	ACQUISITION,
	MONITOR_TUNING
} tuner_state_t;




typedef enum
{
	CH_TUNER_NO_ERROR = 0,
	CH_TUNER_INVALID_PARA
};

enum
{
	CH_I2C_WRITE,
	CH_I2C_READ
};



/*************************globle variable extern*************************/
extern semaphore_t  *psemTunerStateWriteAccess;
extern semaphore_t  *psemTunerIdleStateEntered;
extern semaphore_t  *psemTunerIdleStateReleased;
extern semaphore_t  *psemLinkIcAccess;
extern semaphore_t  *psemErrorCountAccess;

extern semaphore_t	*p_semSTv0297RegAccess;

extern semaphore_t	*pg_semSignalAccess;/*在操作Repeater时候，禁止调用得到信号信息函数*/

extern STI2C_Handle_t          I2CDemodHandle;
extern STI2C_Handle_t          I2CTunerHandle;


extern tuner_state_t  		iCurTunerTaskState;
extern tuner_state_t		iSavedTunerTaskState;
extern CH_TunerType_t		gsenm_CHTunerType;


/***************************Function externs***************************/

/**************************************************************
Fuction Name:
	ch_Tuner_SetTunerType

Purpose:
	Set the RF tuner type

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
void ch_Tuner_SetTunerType(CH_TunerType_t enmTunerType);


/**************************************************************
Fuction Name:
	ch_Tuner_GetTunerType

Purpose:
	Get the RF Tuner type

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
CH_TunerType_t ch_Tuner_GetTunerType(void);


/***********************************/

/*
* update iCurTunerTaskState if the current state has not changed,
* this will be invoked only by the tuner_process.
*/
void UPDATE_TUNER_STATE(tuner_state_t old_state, tuner_state_t new_state);



/**************************************************************
Fuction Name:
	ch_DVBTunerInit

Author:
	ZhouXucheng

Purpose:


Input parameter:
	NULL
	
Output parameter:
	NULL
	
Return Status:
	TRUE	FAILED
	FALSE	SUCCESFULL

Note:	
	调用的时候请一定要传送正确的高频头型号，具体型号请参考
	该目录下的移植文档(高频移植文挡.doc)
**************************************************************/
int ch_DVBTunerInit ( CH_TunerType_t  rCHstr_TunerType );


/**************************************************************
Fuction Name:
	ch_TUNER_MemMollac

Purpose:


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
void *ch_TUNER_MemMollac(int ri_Size);



/**************************************************************
Fuction Name:
	ch_TUNER_MemFree

Purpose:


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
void ch_TUNER_MemFree(void *rp_Buffer);



/**************************************************************
Fuction Name:
	ch_TUNER_DelayMSEL

Purpose:


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
int ch_TUNER_DelayMSEL(int ri_MilliSecond);


/**************************************************************
Fuction Name:
	CH_TUNER_DebugMessage

Purpose:


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
int ch_TUNER_DebugMessage(int ri_Level, char *buffer,...);



/**************************************************************
Fuction Name:
	ch_Tuner_MID_StillLocked

Purpose:


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
BOOL ch_Tuner_MID_StillLocked( void );


/**************************************************************
Fuction Name:
	ch_Tuner_MID_GetStrengthAndQuality

Author:
	ZhouXucheng

Purpose:
	Get the Quality and Strenth in the format 0-100 stand for 0%-100%

Input parameter:
	NULL

Output parameter:
	NULL

Return Status:
	TRUE	FAILED
	FALSE	SUCCESFULL
**************************************************************/
void ch_Tuner_MID_GetStrengthAndQuality(int *rpi_SignalStrength, int *rpi_SignalQuality);


/**************************************************************
Fuction Name:
	ch_Tuner_MID_GetBerRateString

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
U32 ch_Tuner_MID_GetBerRate(void);


/**************************************************************
Fuction Name:
	ch_Tuner_MID_GetSNR

Purpose:
	Get the signal noise rate
	in the db

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
U32 ch_Tuner_MID_GetSNR(void);

/**************************************************************
Fuction Name:
	ch_Tuner_MID_GetRFLevel

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
U32	ch_Tuner_MID_GetRFLevel(void);


/**************************************************************
Fuction Name:
	ch_Tuner_MID_SetParameter

Purpose:


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
U32  ch_Tuner_MID_SetParameter (int ri_FrequencyKHz, int ri_SymbolRateKHz, int ri_QamMode, int ri_IQInversion);



/**************************************************************
Fuction Name:
	CH_I2C_DemodReadAndWrite

Purpose:


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
ST_ErrorCode_t ch_I2C_DemodReadAndWrite(int mode, unsigned char *Data,int NbData);

/**************************************************************
Fuction Name:
	ch_I2C_TunerReadWrite

Purpose:


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
ST_ErrorCode_t ch_I2C_TunerReadWrite(int mode, unsigned char *Data,int NbData);



#endif


