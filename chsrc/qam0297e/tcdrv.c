/*=====================================================
Copy Right By:
	R&D  Institute,Sichuan  Changhong  Network  Technologies Co.,Ltd.

File Name:
	TCDRV.C

Version Number:
	1.0
Author:
	


Purpose:


History:
	Date				Author			Actor
------------------------------------------------------
	2007-9-9				Create

======================================================*/



/******************************Include files******************************/
#include "tcdrv.h"
#include "ch_tuner_mid.h"
#include "drv0297e.h"

/******************************Macro define******************************/
#ifndef		NULL
#define		NULL			0
#endif

#define WAITFORLOCK    1

/******************************struct define******************************/


/*************************globle variable define*************************/

static unsigned char    gsuc_TunerRegister[6];
static unsigned char    *gusp_Register = NULL;
static CH_TunerType_t gsCH_TunerType;


/**************************************************************
Fuction Name:
	tuner_tdrv_SetTunerRegisters

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
ST_ErrorCode_t tuner_tdrv_SetTunerRegisters(U8 *rpuc_Data, int ri_Number)
{	
	semaphore_wait(pg_semSignalAccess);
	FE_297e_Repeater(TRUE);
	ch_I2C_TunerReadWrite(CH_I2C_WRITE, rpuc_Data, ri_Number);
	FE_297e_Repeater(FALSE);
	semaphore_signal(pg_semSignalAccess);
}

/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_DCT7040()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Init(void)
{
	ST_ErrorCode_t Error = ST_NO_ERROR;

	CH_TunerType_t ch_TunerType;
	int TunerRegisterNum;

	ch_TunerType = ch_Tuner_GetTunerType();

	gsCH_TunerType = ch_TunerType;

	gusp_Register = &gsuc_TunerRegister[1];

	switch(gsCH_TunerType)
	{

		case TUNER_PLL_THOMSON_DCT7042A:
			gsuc_TunerRegister[0] = 0xc0;/* tuner I2c address*/
			gsuc_TunerRegister[1] = 0x00;/*Divider Byte1*/
			gsuc_TunerRegister[2] = 0x00;/*Divider Byte2*/
			gsuc_TunerRegister[3] = 0x86;
			gsuc_TunerRegister[4] = 0x00;
			TunerRegisterNum = 4;
			break;

		case TUNER_PLL_DCT70700:
			gsuc_TunerRegister[0] = 0xc0;/* tuner I2c address*/
			gsuc_TunerRegister[1] = 0x0b;/*Divider Byte1*/
			gsuc_TunerRegister[2] = 0x00;/*Divider Byte2*/
			gsuc_TunerRegister[3] = 0x8b;
			gsuc_TunerRegister[4] = 0x01;
			gsuc_TunerRegister[5] = 0xc3;
			TunerRegisterNum = 4;
			break; 
/*20080114 add*/			
	      case TUNER_PLL_CD11XX:
	      case TUNER_PLL_CD1616LF:
/**************/		  	
		  	gsuc_TunerRegister[0] = 0xc0;/* tuner I2c address*/
			gsuc_TunerRegister[1] = 0x0b;/*Divider Byte1*/
			gsuc_TunerRegister[2] = 0x00;/*Divider Byte2*/
			gsuc_TunerRegister[3] = /*0xCE*//*0xA6*/0xC6;
			gsuc_TunerRegister[4] = 0x11;
			gsuc_TunerRegister[5] = /*0xc3*/0x50;
			TunerRegisterNum = 4;
		  	 break;
/*************/			 
	}

	tuner_tdrv_SetTunerRegisters(gusp_Register, TunerRegisterNum);	
	return(Error);
}


/* ----------------------------------------------------------------------------
Name: tuner_tdrv_SetFrequency()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_SetFrequency (U32 rui_FrequencyKHz)
{
	ST_ErrorCode_t Error = ST_NO_ERROR;
	U32	uFreqPll,uFrequency;
	U32	uPllStep = 62500;
	U8	TunerRegisterNum = 4;


	switch (gsCH_TunerType)
	{
#if 0/*20080221 del*/
		case TUNER_PLL_CD1616LF:
			{
				uFrequency = uFreqPll = ( U32)(rui_FrequencyKHz*1000+36125000);
				uFreqPll /= uPllStep;

				gusp_Register[0] = ( U8 )(uFreqPll>>8)&0x7f;/*Divider Byte1*/
				gusp_Register[1] = ( U8 )uFreqPll;/*Divider Byte2*/
				gusp_Register[2] = 0xc6;
				if(uFrequency < 167000000)
				{
					gusp_Register[3] = 1;
				}
				else if(uFrequency < 454000000)
				{
					gusp_Register[3] = 2;
				}
				else 
				{
					gusp_Register[3] = 4;
				}

				TunerRegisterNum = 4;
			}
			break;
#endif
              case TUNER_PLL_CD1616LF:/*20080221 add*/
		case TUNER_PLL_CD11XX:
			{
				uFrequency = uFreqPll = ( U32)(rui_FrequencyKHz*1000+36130000);
				uFreqPll /= uPllStep;

				gusp_Register[0] = ( U8 )(uFreqPll>>8)&0x7f;/*Divider Byte1*/
				gusp_Register[1] = ( U8 )uFreqPll;/*Divider Byte2*/
				/*gusp_Register[2] = 0xCE*//*0xc6*/;
#if 0/*20080618 change for CD1616 and CD1116 band switch bug*/				
				if(uFrequency < 163000000)
				{
					/*gusp_Register[3] = 1;*/
				     gusp_Register[3] &= 0xF9; /*xxxxx00?*/
                                  gusp_Register[3] |= 0x1;
				}
				else if(uFrequency < 450000000)
				{
					/*gusp_Register[3] = 2;*/
						gusp_Register[3] &= 0xFA; /*xxxxx00?*/
                                  gusp_Register[3] |= 0x2;
				}
				else 
				{
					/*gusp_Register[3] = 4;*/
						gusp_Register[3] &= 0xFC; /*xxxxx00?*/
                                  gusp_Register[3] |= 0x4;
				}
#else
                            if(rui_FrequencyKHz*1000 < 163000000)
				{
					/*gusp_Register[3] = 1;*/
				     gusp_Register[3] &= 0xF9; /*xxxxx00?*/
                                  gusp_Register[3] |= 0x1;
				}
				else if(rui_FrequencyKHz*1000 < 450000000)
				{
					/*gusp_Register[3] = 2;*/
						gusp_Register[3] &= 0xFA; /*xxxxx00?*/
                                  gusp_Register[3] |= 0x2;
				}
				else 
				{
					/*gusp_Register[3] = 4;*/
						gusp_Register[3] &= 0xFC; /*xxxxx00?*/
                                  gusp_Register[3] |= 0x4;
				}
#endif
        
				TunerRegisterNum = 4;
			}
			break;	
			
		case TUNER_PLL_THOMSON_DCT7042A:
			{
				uFrequency = uFreqPll = ( U32)(rui_FrequencyKHz*1000+36000000);
				uFreqPll /= uPllStep;
				gusp_Register[0] = ( U8 )(uFreqPll>>8)&0x7f;/*Divider Byte1*/
				gusp_Register[1] = ( U8 )uFreqPll;/*Divider Byte2*/

				gusp_Register[2] = 0x86;
				if(uFrequency < 185000000)
				{
					gusp_Register[3] = 0x02;
				}
				else if(uFrequency < 465000000)
				{
					gusp_Register[3] = 0x01;
				}
				else 
				{
					gusp_Register[3] = 0x00;
				}

				TunerRegisterNum = 4;
			}
			break;

		case TUNER_PLL_DCT70700:
			{			
				uFrequency = uFreqPll = ( U32)(rui_FrequencyKHz*1000+36000000);
				uFreqPll /= uPllStep;
				gusp_Register[0] = ( U8 )(uFreqPll>>8)&0x7f;/*Divider Byte1*/
				gusp_Register[1] = ( U8 )uFreqPll;/*Divider Byte2*/
				
				gusp_Register[2] = 0x8B;		/*109, 62.5KHz*/
				if (uFrequency <= 185000000)
				{
					gusp_Register[3] = 0x01;
				}
				else if (uFrequency <= 465000000)
				{
					gusp_Register[3] = 0x06;
				}
				else
				{
					gusp_Register[3] = 0x0C;	
				}
				gusp_Register[4] = 0xC3;

				TunerRegisterNum = 4;
			}
			break;

		default:
			break;
	}
	tuner_tdrv_SetTunerRegisters(gusp_Register, TunerRegisterNum);	
#if 0
	/*   --- Tuner Is Locked ?  */
        iLoop = 0 ;
        while( (!bLocked) && (iLoop < 20) )
        {
            tuner_tdrv_IsTunerLocked(&bLocked);
            CH_TUNER_DelayMSEL(10);
        }

#endif
	return Error;
}


