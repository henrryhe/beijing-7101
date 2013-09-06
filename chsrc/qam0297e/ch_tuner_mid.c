/*=====================================================
Copy Right By:
	R&D  Institute,Sichuan  Changhong  Network  Technologies Co.,Ltd.

File Name:
	CH_TUNER_MID.C

Version Number:
	1.0
	
Author:
	Zhou Xu Cheng


Purpose:
	这个文件用于联系应用程序和底层驱动。如果要更改TUNER，只需要更改此文件
	中的内容，但是函数名称不能更改。

History:
	Date				Author			Actor
------------------------------------------------------
	2007-9-27		Zhou Xu Cheng		Create
    2008-01-11      zxg              移植到sti7101平台
======================================================*/



/******************************Include files******************************/
#include <string.h>
#include <stdarg.h>
#include "stddefs.h"

#include "..\main\initterm.h"
#include "ch_tuner_mid.h"
#include "..\report\report.h"
#include "ch_tuner.h"
#include "qam0297.h"
#include "sttbx.h"
#include "d0297e.h"

/******************************Macro define******************************/
#define  	I2C_SLAVE_ADDR_FOR_DEMO          	0x38
#define 	TUNER_SOFTWARE_UPDATE_ID    		"1.3Beta"
#define  	I2C_BUS_ID    0

#define 	I2C_TIME_OUT                                       1000
#define  	I2C_TUNER_TIME_OUT                           1000


/******************************struct define******************************/


/*************************globle variable define*************************/

semaphore_t  *psemTunerStateWriteAccess  	= NULL;
semaphore_t  *psemTunerIdleStateEntered  	= NULL;
semaphore_t  *psemTunerIdleStateReleased 	= NULL;
semaphore_t  *psemLinkIcAccess           		= NULL;
semaphore_t  *psemErrorCountAccess       		= NULL;


semaphore_t	*p_semSTv0297RegAccess;

semaphore_t	*pg_semSignalAccess;/*在操作Repeater时候，禁止调用得到信号信息函数*/

STI2C_Handle_t          I2CDemodHandle;
STI2C_Handle_t          I2CTunerHandle;

tuner_state_t  	iCurTunerTaskState   = TUNER_IDLE;
tuner_state_t		iSavedTunerTaskState = TUNER_IDLE;
CH_TunerType_t	gsenm_CHTunerType;


static char	       report_buffer[256*5];


static boolean		StillLocked = false;


/***************************Function defines***************************/


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
void ch_Tuner_SetTunerType(CH_TunerType_t enmTunerType)
{
	gsenm_CHTunerType = enmTunerType;
}

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
CH_TunerType_t ch_Tuner_GetTunerType(void)
{
	return gsenm_CHTunerType;
}

/***********************************/

/*
* update iCurTunerTaskState if the current state has not changed,
* this will be invoked only by the tuner_process.
*/
void UPDATE_TUNER_STATE(tuner_state_t old_state, tuner_state_t new_state)
{ 
	semaphore_wait ( psemTunerStateWriteAccess ); 
	if ( iCurTunerTaskState == old_state) 
	{  
		iSavedTunerTaskState = new_state;
		iCurTunerTaskState = new_state;
		if(iCurTunerTaskState == MONITOR_TUNING)
			StillLocked = true;   
		else	
			StillLocked = false;
		
	}  
	semaphore_signal ( psemTunerStateWriteAccess ); 
}


/*函数名:void CH_ResetQAMChip(void)
*/
/*开发人和开发时间:zengxianggen     2006-06-01                    */
/*函数功能描述:  复位解调芯片                             */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*输入:无                                                          */
/*调用的主要函数列表：                                             */
/*返回值说明：无*/

/******************************************************************/
void CH_ResetQAMChip(void)
{

  	STPIO_OpenParams_t    ST_OpenParams;
       STPIO_Handle_t        QAMPIOhandle;
	ST_OpenParams.ReservedBits      = PIO_BIT_6;
	ST_OpenParams.BitConfigure[6]  = STPIO_BIT_OUTPUT;          
	ST_OpenParams.IntHandler       = NULL;         

	STPIO_Open(PIO_DeviceName[2], &ST_OpenParams, &QAMPIOhandle );
	STPIO_Clear(QAMPIOhandle,0x40); 
	MILLI_DELAY(1000);

       STPIO_Set(QAMPIOhandle,0x40); 
	MILLI_DELAY(1000); 
}

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
int ch_DVBTunerInit ( CH_TunerType_t  rCHstr_TunerType )
{

	STI2C_OpenParams_t   	I2cOpenParams;
	ST_ErrorCode_t 		Error = ST_NO_ERROR;
	boolean 				bInitSuccesfull = FALSE;

#if 0/*复位解调芯片*/

	/*configure pio3 bit3 for output*/
	*(volatile U8*)(0x20823040) &= ~0x08; 
	*(volatile U8*)(0x20823030) |=  0x08;    
	*(volatile U8*)(0x20823020) &= ~0x08;  

	/*send a low and then high to FE_RST*/
	*(volatile U8*)(0x20823008) = 0x08; 	

	MILLI_DELAY(1000);

	*(volatile U8*)(0x20823004) = 0x08; 	

	MILLI_DELAY(1000);

#else
  //   CH_ResetQAMChip();
#endif
      
	ch_TUNER_DebugMessage(MESSAGE_LEVEL_NORMAL, "TUNER Module Initialisation ver[%s]\n", TUNER_SOFTWARE_UPDATE_ID );

	ch_Tuner_SetTunerType(rCHstr_TunerType);


	/*I2C控制信号，请与E2P信号量保持一致*/
#if 0
	semSTv0297RegAccess	=	semNvmAccessLock;
#else	
//	semaphore_init_fifo (&semSTv0297RegAccess, 1 );	
//	semaphore_init_fifo (&g_semSignalAccess, 1 );
#endif	
    p_semSTv0297RegAccess = semaphore_create_fifo ( 1 );
    pg_semSignalAccess = semaphore_create_fifo ( 1 );
	psemLinkIcAccess = semaphore_create_fifo ( 1 );
	psemErrorCountAccess = semaphore_create_fifo ( 1 );
	psemTunerStateWriteAccess = semaphore_create_fifo ( 1 );
	psemTunerIdleStateEntered = semaphore_create_fifo ( 0 );
	psemTunerIdleStateReleased = semaphore_create_fifo ( 0 );
	
	if ( psemLinkIcAccess == NULL 
		|| psemErrorCountAccess == NULL
		|| psemTunerStateWriteAccess == NULL
		|| psemTunerIdleStateEntered == NULL
		||psemTunerIdleStateReleased == NULL)
	{
		ch_TUNER_DebugMessage(MESSAGE_LEVEL_NORMAL, "TUNER_INIT=> Unable to Initialise Semaphores\n");
		return TRUE;
	}
		
	
	/* Initialize the Demo I2C Handle */
	I2cOpenParams.BusAccessTimeOut     = 100000;
	
	I2cOpenParams.AddressType 	= STI2C_ADDRESS_7_BITS;
	I2cOpenParams.I2cAddress         	= I2C_SLAVE_ADDR_FOR_DEMO;
		
	if ( ( Error = STI2C_Open ( I2C_DeviceName [ I2C_BUS_ID ],
		&I2cOpenParams,
		&I2CDemodHandle ) ) != ST_NO_ERROR )
	{
		ch_TUNER_DebugMessage(MESSAGE_LEVEL_NORMAL, "%s %d> Failed (%d) to open I2C access for Demodulator on [%s]\n",
			__FILE__,
			__LINE__,
			Error,
			I2C_DeviceName [ I2C_BUS_ID ] );
		
		return  TRUE;
	}

	
	/* Initialize the Tuner I2C Handle */
	I2cOpenParams .I2cAddress = 0xC0;
	
	if ( STI2C_Open( I2C_DeviceName [ I2C_BUS_ID ],
		&I2cOpenParams,
		&I2CTunerHandle ) != ST_NO_ERROR )
	{
		ch_TUNER_DebugMessage(MESSAGE_LEVEL_NORMAL, "%s %d> Failed (%d) to open I2C access for TUNER on [%s]\n",
			__FILE__,
			__LINE__,
			Error,
			I2C_DeviceName [ I2C_BUS_ID ] );
		
		/* Clean-up previously allocated resources */
		STI2C_Close ( I2CDemodHandle );
		return  TRUE;
	}
		
	bInitSuccesfull = ch_TUNER_Qam0297_Init();
			
	ch_TUNER_DebugMessage(MESSAGE_LEVEL_NORMAL, "TUNER Module Initialisation ver[%s]", TUNER_SOFTWARE_UPDATE_ID );

	if (bInitSuccesfull == false)
	{
		ch_TUNER_DebugMessage(MESSAGE_LEVEL_NORMAL, "Succesfull\n" );
	}
	else
	{
		ch_TUNER_DebugMessage(MESSAGE_LEVEL_NORMAL, "Failure\n" );
	}
			
	return bInitSuccesfull;
}

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
void *ch_TUNER_MemMollac(int ri_Size)
{
	return memory_allocate(SystemPartition, ri_Size);
}


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
void ch_TUNER_MemFree(void *rp_Buffer)
{
	if ( rp_Buffer != NULL)
	{
		memory_deallocate(SystemPartition, rp_Buffer);
		rp_Buffer = NULL;
	}
}


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
int ch_TUNER_DelayMSEL(int ri_MilliSecond)
{
	MILLI_DELAY(ri_MilliSecond)
}


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
int ch_TUNER_DebugMessage(int ri_Level, char *buffer,...)
{
	va_list      list;    

	if (ri_Level >= MESSAGE_LEVEL_NORMAL)
	{
		va_start(list, buffer);
		vsprintf(report_buffer, buffer, list);
		va_end(list);

		sttbx_Print(("%s",report_buffer));
	}
}



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
BOOL ch_Tuner_MID_StillLocked( void )
{

	boolean bTunerLocked;
	semaphore_wait ( psemTunerStateWriteAccess ); 
	bTunerLocked = StillLocked;   		
	semaphore_signal ( psemTunerStateWriteAccess ); 

	return bTunerLocked;

}

BOOL ch_Tuner_EIS_StillLocked( void )
{

	boolean bTunerLocked;
	semaphore_wait ( psemTunerStateWriteAccess ); 
	if(StillLocked&&(iCurTunerTaskState == MONITOR_TUNING))
		bTunerLocked = true;   	
	else
		bTunerLocked = false;  
	semaphore_signal ( psemTunerStateWriteAccess ); 

	return bTunerLocked;

}

void  ch_Tuner_MID_StillLocked_Resume( void )
{
	semaphore_wait ( psemTunerStateWriteAccess ); 
	 StillLocked =FALSE;   		
	semaphore_signal ( psemTunerStateWriteAccess ); 

}


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
void ch_Tuner_MID_GetStrengthAndQuality(int *rpi_SignalStrength, int *rpi_SignalQuality)
{
	U32 ui_SNR, ui_Ber;
	
	demod_d0297E_GetSignalQuality((U32*)rpi_SignalQuality, (U32*)rpi_SignalStrength, &ui_Ber, &ui_SNR);
}


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
U32 ch_Tuner_MID_GetBerRate(void)
{
	U32 ui_Strength, ui_Quality;
	U32 ui_SNR,ui_Ber;

	demod_d0297E_GetSignalQuality(&ui_Quality, &ui_Strength, &ui_Ber, &ui_SNR);

	return ui_Ber;
}


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
U32 ch_Tuner_MID_GetSNR(void)
{
	U32 ui_Strength, ui_Quality;
	U32 ui_SNR,ui_Ber;

	demod_d0297E_GetSignalQuality(&ui_Quality, &ui_Strength, &ui_Ber, &ui_SNR);

	return ui_SNR;
}

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
U32	ch_Tuner_MID_GetRFLevel(void)
{
	U32 ui_Strength, ui_Quality;
	U32 ui_SNR,ui_Ber;

	demod_d0297E_GetSignalQuality(&ui_Quality, &ui_Strength, &ui_Ber, &ui_SNR);

	return ui_Strength;
}


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
U32  ch_Tuner_MID_SetParameter (int ri_FrequencyKHz, int ri_SymbolRateKHz, int ri_QamMode, int ri_IQInversion)
{
	ch_QAM0297E_SetParameter(ri_FrequencyKHz, ri_SymbolRateKHz, ri_QamMode, ri_IQInversion);
}



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
ST_ErrorCode_t ch_I2C_DemodReadAndWrite(int mode, unsigned char *Data,int NbData)
{
	ST_ErrorCode_t Error = ST_NO_ERROR;
	int NbRW;
	int retries = 1;

	semaphore_wait ( p_semSTv0297RegAccess );
	do
	{
		switch ( mode )
		{
		case CH_I2C_WRITE:
			Error = STI2C_Write (I2CDemodHandle,
				(U8 *) Data,
				(U32) NbData,
				I2C_TIME_OUT,
				(U32 *) &NbRW);
			break;
			
		case CH_I2C_READ:
			Error = STI2C_Write ( I2CDemodHandle,
				(U8 *) Data,
				1,
				I2C_TIME_OUT,
				(U32 *) &NbRW);

			if (Error == ST_NO_ERROR)
			{
				Error = STI2C_Read ( I2CDemodHandle,
					(U8 *) Data,
					(U32) NbData,
					I2C_TIME_OUT,
					(U32 *) &NbRW);
			}			
			break;
			
		default:
			break;
		}

	} 
	while (Error != ST_NO_ERROR && --retries > 0 );
	
	semaphore_signal ( p_semSTv0297RegAccess);
	
	if (Error != ST_NO_ERROR)
	{
		ch_TUNER_DebugMessage(MESSAGE_LEVEL_NORMAL, "STI2C_Read/STI2C_Write(I2cDemodHandle) Add[%d] Error=%s\n",Data[0],GetErrorText(Error));
	}
	
	return ST_NO_ERROR;
}

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
ST_ErrorCode_t ch_I2C_TunerReadWrite(int mode, unsigned char *Data,int NbData)
{
	ST_ErrorCode_t Error = ST_NO_ERROR;
	int NbRW;
	int retries = 1;

	semaphore_wait ( p_semSTv0297RegAccess );
	do
	{
		switch ( mode )
		{
		case CH_I2C_WRITE:
			Error = STI2C_Write ( I2CTunerHandle,
				(U8 *) Data,
				(U32) NbData,
				I2C_TUNER_TIME_OUT,
				(U32 *) &NbRW);
			break;
			
		case CH_I2C_READ:
#if 0			
			Error = STI2C_Write ( I2cTunerHandle,
				(U8 *) Data,
				1,
				I2C_TIME_OUT,
				(U32 *) &NbRW);

			if (Error == ST_NO_ERROR)
			{
				Error = STI2C_Read ( I2cTunerHandle,
					(U8 *) Data,
					(U32) NbData,
					I2C_TIME_OUT,
					(U32 *) &NbRW);
			}
#else
			Error = STI2C_Read ( I2CTunerHandle,
					(U8 *) Data,
					(U32) NbData,
					I2C_TIME_OUT,
					(U32 *) &NbRW);
#endif
			break;
			
		default:
			break;
		}
	} 
	while (Error != ST_NO_ERROR && --retries > 0 );
	
	semaphore_signal (p_semSTv0297RegAccess );

	if (Error != ST_NO_ERROR)
	{
		if ( mode == CH_I2C_WRITE)
		{
			ch_TUNER_DebugMessage(MESSAGE_LEVEL_NORMAL, "STI2C_Write(I2cTunerHandle) Add[%x]=%s\n",Data[0],GetErrorText(Error));
		}
		else
		{
			ch_TUNER_DebugMessage(MESSAGE_LEVEL_NORMAL, "STI2C_Read(I2cTunerHandle) Add[%x]=%s\n",Data[0],GetErrorText(Error));
		}
	}
	
	return Error;

}


