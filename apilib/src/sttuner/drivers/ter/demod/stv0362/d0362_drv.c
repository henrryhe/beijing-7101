/* ----------------------------------------------------------------------------
File Name: d0362_drv.c

Description:

    stv0362 Terrestrial COFDM driver


Copyright (C) 2006-2007 STMicroelectronics

History:

   date: 13 Jul 2007( last modified)
version: sttuner_basic 6.2.6
 author:
comment: this version is sompatible with GUI 1.8.4 but one tuner (TD1300ALF) change is not added (this tuner is not integrated in sttuner driver)

Reference:


---------------------------------------------------------------------------- */
/* includes ---------------------------------------------------------------- */
/* C libs */
#ifndef ST_OSLINUX
   #include  <stdlib.h>
#endif
#include "sttbx.h"

/* STAPI */
#include "sttuner.h"
#include "sti2c.h"
#include "stcommon.h"

#include "stevt.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "ioarch.h"     /* I/O for this driver */
#include "ioreg.h"      /* I/O for this driver */
#include "chip.h"

#include "tuntdrv.h"
/* common includes */
#include "d0362_echo.h"
#include "d0362.h"
#include "d0362_drv.h"
#include "d0362_map.h"

/* common includes */

/* Current LLA revision	*/
static ST_Revision_t Revision362  = "STV0362-LLA_REL_1.8.4(GUI)";

#define M6_F_GAIN_SRC_HI        0x0A
#define M6_F_GAIN_SRC_LO        0x28
#define M6_F_TRL_NOMRATE0       0x00
#define M6_F_TRL_NOMRATE8_1     0x01
#define M6_F_TRL_NOMRATE16_9    0x41

#define M7_GAIN_SRC_HI        	0x0C
#define M7_GAIN_SRC_LO        	0xE4    /*For normal IF Mode*/

#define M7_E_GAIN_SRC_HI        0x07
#define M7_E_GAIN_SRC_LO        0x6C    /* For long path IF / IQ modes */

#define M7_F_TRL_NOMRATE0       0x00	   	/* ok */
#define M7_F_TRL_NOMRATE8_1     0xD6   	/* ok */
#define M7_F_TRL_NOMRATE16_9    0x4B   	/* ok */


#define M8_GAIN_SRC_HI        	0x0B
#define M8_GAIN_SRC_LO        	0xB8 	/*For normal IF Mode*/

#define M8_E_GAIN_SRC_HI		0x08
#define M8_E_GAIN_SRC_LO		0x86	/* For long path IF / IQ modes */

#define M8_F_TRL_NOMRATE0       0x01	   	/* ok */
#define M8_F_TRL_NOMRATE8_1     0xAB   	/* ok */
#define M8_F_TRL_NOMRATE16_9    0x56   	/* ok */


#ifdef ST_OS21
#define Delay(x) STOS_TaskDelay((signed int)x);
#else
#define Delay(x) STOS_TaskDelay((unsigned int)x);
#endif
#define UTIL_Delay(micro_sec) STOS_TaskDelayUs(micro_sec)
#define SystemWaitFor(x) UTIL_Delay((x*1000))

#define WAIT_N_MS_362(X)     STOS_TaskDelayUs(X * 1000)


TUNTDRV_InstanceData_t *TUNTDRV_GetInstFromHandle(TUNER_Handle_t Handle);
extern IOARCH_HandleData_t IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];



#define DELTA 666

  #define DELTA_T 10

/* dcdc #define CPQ_LIMIT 23*/
#define CPQ_LIMIT 23

U16 CellsCoeffs_8MHz[6][5]={
						{0x10EF,0xE205,0x10EF,0xCE49,0x6DA7},  /* CELL 1 COEFFS */
						{0x2151,0xc557,0x2151,0xc705,0x6f93},  /* CELL 2 COEFFS */
						{0x2503,0xc000,0x2503,0xc375,0x7194},  /* CELL 3 COEFFS */
						{0x20E9,0xca94,0x20e9,0xc153,0x7194},  /* CELL 4 COEFFS */
						{0x06EF,0xF852,0x06EF,0xC057,0x7207},  /* CELL 5 COEFFS */
						{0x0000,0x0ECC,0x0ECC,0x0000,0x3647}   /* CELL 6 COEFFS */
					  } ;


U16 CellsCoeffs_7MHz[6][5]={
				{0x12CA,0xDDAF,0x12CA,0xCCEB,0x6FB1},  /* CELL 1 COEFFS */
				{0x2329,0xC000,0x2329,0xC6B0,0x725F},  /* CELL 2 COEFFS */
				{0x2394,0xC000,0x2394,0xC2C7,0x7410},  /* CELL 3 COEFFS */
				{0x251C,0xC000,0x251C,0xC103,0x74D9},  /* CELL 4 COEFFS */
				{0x0804,0xF546,0x0804,0xC040,0x7544},  /* CELL 5 COEFFS */
				{0x0000,0x0CD9,0x0CD9,0x0000,0x370A}   /* CELL 6 COEFFS */
			  } ;

U16 CellsCoeffs_6MHz[6][5]={
				{0x12CA,0xDDAF,0x12CA,0xCCEB,0x6FB1},  /* CELL 1 COEFFS */
				{0x2329,0xC000,0x2329,0xC6B0,0x725F},  /* CELL 2 COEFFS */
				{0x2394,0xC000,0x2394,0xC2C7,0x7410},  /* CELL 3 COEFFS */
				{0x251C,0xC000,0x251C,0xC103,0x74D9},  /* CELL 4 COEFFS */
				{0x0804,0xF546,0x0804,0xC040,0x7544},  /* CELL 5 COEFFS */
				{0x0000,0x0CD9,0x0CD9,0x0000,0x370A}   /* CELL 6 COEFFS */
			  } ;


/***********************************************************
**FUNCTION	::	Drv0362_GetLLARevision
**ACTION	::	Returns the 362 LLA driver revision
**RETURN	::	Revision362
***********************************************************/
ST_Revision_t Drv0362_GetLLARevision(void)
{
	return (Revision362);
}




/*********************************************************
--FUNCTION	::	CheckCPAMP_362
--ACTION	::	Get CPAMP status
--PARAMS IN	:: IOARCH_Handle,STTUNER_IOREG_DeviceMap_t
--PARAMS OUT::	CPAMP status
--********************************************************/
FE_362_SignalStatus_t CheckCPAMP_362(IOARCH_Handle_t IOHandle,S32 FFTmode )
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
   const char *identity = "STTUNER d0362_drv.c CheckCPAMP_362()";
#endif

	S32  CPAMPvalue=0, CPAMPStatus, CPAMPMin;
	int wd=0;

	switch(FFTmode)
	{
		case 0: /*2k mode*/
			 CPAMPMin=20;
			 wd=10;
		   break;
		case 1: /*8k mode*/
			 CPAMPMin=80;
			 wd=55;
		   break;
		case 2: /*4k mode*/
			 CPAMPMin=40;
			 wd=30;
		   break;
		default:
			CPAMPMin=0xffff;  /*drives to NOCPAMP*/
		  break;
	}

	CPAMPvalue=ChipGetField(IOHandle,F0362_PPM_CPAMP_DIRECT);
	while  ((CPAMPvalue<CPAMPMin) && (wd>0))
	{
			WAIT_N_MS_362(1);
			wd-=1;
			CPAMPvalue=ChipGetField(IOHandle,F0362_PPM_CPAMP_DIRECT);

	}

	if (CPAMPvalue<CPAMPMin)
		{
	     CPAMPStatus=NOCPAMP_362;
	  }
	else
		{
	     CPAMPStatus=CPAMPOK_362;
	  }

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
  STTBX_Print(("%s CPAMPStatus %d\n", identity,CPAMPStatus));
#endif

	return CPAMPStatus;
}

/*****************************************************
**FUNCTION	::	FE_362_Pow
**ACTION	::	Compute  x^y (where n is an integer)
**PARAMS IN ::  number -> n
**PARAMS OUT::  NONE
**RETURN	::	x^y
*****************************************************/
int FE_362_Pow(int number1,int number2)
{
int i;
int result=1;

	for(i=0;i<number2;i++)
		result *=number1;

return result;
}
/*****************************************************
**FUNCTION	::	FE_362_Pow
**ACTION	::	Compute  2^n (where n is an integer)
**PARAMS IN	::	number -> n
**PARAMS OUT::	NONE
**RETURN	::	2^2
*****************************************************/
int FE_362_PowOf2(int number)
{
	int i;
	int result=1;

	for(i=0;i<number;i++)
	{
		result*=2;
	}
	return result;
}


/*****************************************************
--FUNCTION		::	FE_362_AGC_IIR_LOCK_DETECTOR_SET
--ACTION		::	Sets Good values for AGC IIR lock detector
--PARAMS IN		::
--PARAMS OUT	::	None
--***************************************************/
void  FE_362_AGC_IIR_LOCK_DETECTOR_SET( IOARCH_Handle_t IOHandle)

{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
   const char *identity = "STTUNER d0362_drv.c FE_362_AGC_IIR_LOCK_DETECTOR_SET()";
#endif

	ChipSetField(IOHandle,F0362_LOCK_DETECT_LSB,0x00);

	/* Lock detect 1 */
	ChipSetField(IOHandle,F0362_LOCK_DETECT_CHOICE,0x00);
	ChipSetField(IOHandle,F0362_LOCK_DETECT_MSB,0x06);
	ChipSetField(IOHandle,F0362_AUT_AGC_TARGET_LSB,0x04);

	/* Lock detect 2 */
	ChipSetField(IOHandle,F0362_LOCK_DETECT_CHOICE,0x01);
	ChipSetField(IOHandle,F0362_LOCK_DETECT_MSB,0x06);
	ChipSetField(IOHandle,F0362_AUT_AGC_TARGET_LSB,0x04);

	/* Lock detect 3 */
	ChipSetField(IOHandle,F0362_LOCK_DETECT_CHOICE,0x02);
	ChipSetField(IOHandle,F0362_LOCK_DETECT_MSB,0x01);
	ChipSetField(IOHandle,F0362_AUT_AGC_TARGET_LSB,0x00);

	/* Lock detect 4 */
	ChipSetField(IOHandle,F0362_LOCK_DETECT_CHOICE,0x03);
	ChipSetField(IOHandle,F0362_LOCK_DETECT_MSB,0x01);
	ChipSetField(IOHandle,F0362_AUT_AGC_TARGET_LSB,0x00);


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
  STTBX_Print(("%s AGC threshold setting done\n", identity));
#endif
}

/*****************************************************
--FUNCTION		::	FE_362_IIR_FILTER_INIT
--ACTION		::
--PARAMS IN		::
--PARAMS OUT	::	None
--***************************************************/

U32  FE_362_IIR_FILTER_INIT( IOARCH_Handle_t IOHandle,U8 Bandwidth)

{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
   const char *identity = "STTUNER d0362_drv.c FE_362_IIR_FILTER_INIT()";
#endif

	ChipSetField(IOHandle,F0362_NRST_IIR,0);


        switch (Bandwidth)
        {

        case STTUNER_CHAN_BW_6M:
        FE_362_FilterCoeffInit(IOHandle,CellsCoeffs_6MHz);
            break;

        case STTUNER_CHAN_BW_7M:
        FE_362_FilterCoeffInit(IOHandle,CellsCoeffs_7MHz);
            break;

        case STTUNER_CHAN_BW_8M:
        FE_362_FilterCoeffInit(IOHandle,CellsCoeffs_8MHz);
            break;
        default:
        	return 0;


        }


	ChipSetField(IOHandle,F0362_NRST_IIR,1);
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
  STTBX_Print(("%s IIR filter initialization completed\n", identity));
#endif
	return 1;
}


/*****************************************************
--FUNCTION		::	FE_362_AGC_IIR_RESET
--ACTION		::	AGC reset procedure
--PARAMS IN		::
--PARAMS OUT	::	None
--***************************************************/
void FE_362_AGC_IIR_RESET( IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
   const char *identity = "STTUNER d0362_drv.c FE_362_AGC_IIR_RESET()";
#endif
	U8 com_n;

	com_n=ChipGetField(IOHandle,F0362_COM_N);

	ChipSetField(IOHandle,F0362_COM_N,0x07);

	ChipSetField(IOHandle,F0362_COM_SOFT_RSTN,0x00);
	ChipSetField(IOHandle,F0362_COM_AGC_ON,0x00);

	ChipSetField(IOHandle,F0362_COM_SOFT_RSTN,0x01);
	ChipSetField(IOHandle,F0362_COM_AGC_ON,0x01);

	ChipSetField(IOHandle,F0362_COM_N,com_n);
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
  STTBX_Print(("%s AGC reset procedure done\n", identity));
#endif
}

/*********************************************************
--FUNCTION	::	FE_362_FilterCoeffInit
--ACTION	::	Apply filter coeffs values

--PARAMS IN	::	Handle to the Chip
--PARAMS OUT::	NONE
--********************************************************/
void FE_362_FilterCoeffInit( IOARCH_Handle_t IOHandle,U16 CellsCoeffs[][5])
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
   const char *identity = "STTUNER d0362_drv.c FE_362_FilterCoeffInit()";
#endif

	S32 i,j;
	U32 x,y,k;
k=F0362_IIR_CX_COEFF2_MSB - F0362_IIR_CX_COEFF1_MSB;
for(i=1;i<=6;i++)
		{
			ChipSetField(IOHandle,F0362_IIR_CELL_NB,i-1);
			x=F0362_IIR_CX_COEFF1_MSB;
			y=F0362_IIR_CX_COEFF1_LSB;

			for(j=1;j<=5;j++)
			{

				ChipSetField(IOHandle,x, MSB(CellsCoeffs[i-1][j-1]));

				ChipSetField(IOHandle,y, LSB(CellsCoeffs[i-1][j-1]));
				x= x+k;
				y= y+k;




			}

		}


#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
  STTBX_Print(("%s setting IIR filter co-efficient done\n", identity));
#endif
}


/*****************************************************
--FUNCTION	::	FE_362_WaitTuner
--ACTION	::	Wait for tuner locked
--PARAMS IN	::	TimeOut	->	Maximum waiting time (in ms)
--PARAMS OUT::	NONE
--RETURN	::	NONE
--***************************************************/
void FE_362_WaitTuner(STTUNER_tuner_instance_t *TunerInstance,U32 TimeOut, BOOL *TunerLocked )
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
   const char *identity = "STTUNER d0362_drv.c FE_362_WaitTuner()";
#endif
	int Time=0;
	ST_ErrorCode_t Error;
	*TunerLocked = FALSE;
	while(!*TunerLocked && (Time<TimeOut))
	{
		WAIT_N_MS_362(1);
		Error = (TunerInstance->Driver->tuner_IsTunerLocked)(TunerInstance->DrvHandle, TunerLocked);
		Time++;
	}
	Time--;
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
  STTBX_Print(("%s Value of TunerLocked flag %d \n", identity,*TunerLocked));
#endif
}





/*****************************************************
--FUNCTION	::	FE_362_Algo
--ACTION	::	Search for Signal, Timing, Carrier and then data at a given Frequency,
--				in a given range:

--PARAMS IN	::
--PARAMS OUT::
--RETURN	::	Type of the founded signal (if any)

--REMARKS   ::  This function is supposed to replace FE_362_SearchRun according
--				to last findings on SYR block
--***************************************************/
FE_362_SignalStatus_t FE_362_Algo( IOARCH_Handle_t IOHandle,FE_362_SearchParams_t *pParams,STTUNER_Handle_t TopLevelHandle)
{
#if defined (STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV) || defined (STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV_LOCKSTAGES)
   const char *identity = "STTUNER d0362_drv.c FE_362_Algo()";
#endif

	FE_362_SignalStatus_t ret_flag;
	S16  wd,tempo;
	U16  try,u_var1=0, u_var2=0, u_var3=0,mode,guard;



	try=0;
	do
	{
			ret_flag=LOCKOK_362;

			ChipSetField(IOHandle,F0362_CORE_ACTIVE,0);

			if(pParams->IF_IQ_Mode!=STTUNER_NORMAL_IF_TUNER)
				{
			       ChipSetField(IOHandle,F0362_COM_N,0x07);
			  }

			ChipSetField(IOHandle,F0362_GUARD,3); /* suggested mode is 2k 1/4*/
			ChipSetField(IOHandle,F0362_MODE,0);

			WAIT_N_MS_362(5);


			ChipSetField(IOHandle,F0362_CORE_ACTIVE,1);

			if (CheckSYR_362(IOHandle)==NOSYMBOL_362)
			{
#if defined (STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV) || defined (STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV_LOCKSTAGES)
             STTBX_Print(("%s failed NOSYMBOL_362\n", identity));
#endif
				return NOSYMBOL_362;
			}
			else
			{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV_LOCKSTAGES
				STTBX_Print(("%s SYMBOL_362 = OK\n", identity));
#endif
				/* if chip locked on wrong mode first try, it must lock correctly second try *db*/
				mode= ChipGetField(IOHandle,F0362_SYR_MODE);
				if (CheckCPAMP_362(IOHandle,mode)==NOCPAMP_362)
				{
					if (try==0)
					{
						ret_flag=NOCPAMP_362;
#if defined (STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV) || defined (STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV_LOCKSTAGES)
             STTBX_Print(("%s NOCPAMP_362 in first trial\n", identity));
#endif
					}

				}
#if defined (STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV) || defined (STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV_LOCKSTAGES)
				STTBX_Print(("%s CPAMP_362 = OK\n", identity));
#endif
			}

			try++;
		} while ( (try<2) && (ret_flag!=LOCKOK_362) );


				if ( (mode!=0)&&(mode!=1)&&(mode!=2) )
				{
#if defined (STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV) || defined (STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV_LOCKSTAGES)
     	             STTBX_Print(("%s failed SWNOK_362\n", identity));
#endif
					return SWNOK_362;
				}
				guard=ChipGetField(IOHandle,F0362_SYR_GUARD);

				/*supress EPQ auto for SYR_GARD 1/16 or 1/32 */
				 switch (guard)
				 {

					 case 0:
					 case 1:
							ChipSetField(IOHandle,F0362_AUTO_LE_EN,0);
							ChipSetOneRegister(IOHandle,R0362_CHC_CTL1, 0x1);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
             STTBX_Print(("%s Supress EPQ auto for GI 1/16 and 1/32\n", identity));
#endif
							break;
					case 2:
					case 3:
							ChipSetField(IOHandle,F0362_AUTO_LE_EN,1);
							ChipSetOneRegister(IOHandle,R0362_CHC_CTL1, 0x11);
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
             STTBX_Print(("%s EPQ Auto Enable for GI 1/8 and 1/4\n", identity));
#endif
							break;
					default:

						return SWNOK_362;
					break;
				 }


				/************************************************/
				if(mode == STTUNER_MODE_2K)
				{
				   ChipSetOneRegister(IOHandle,R0362_CHP_TAPS,0x01); /***For 2k mode the chp_taps reg. must
				                                               ***always set to 0x01***/
				}
				else
				{
				  ChipSetOneRegister(IOHandle,R0362_CHP_TAPS,0x03);/***For 8k mode the chp_taps reg. must
				                                               ***always set to 0x03***/
				}


				u_var1=ChipGetField(IOHandle,F0362_LK);
				u_var2=ChipGetField(IOHandle,F0362_PRF);
				u_var3=ChipGetField(IOHandle,F0362_TPS_LOCK);
				wd=duration( mode, 125,500,250);
				tempo= duration( mode, 4,16,8);

				while ( ((!u_var1)||(!u_var2)||(!u_var3))  && (wd>=0))
				{
					WAIT_N_MS_362( tempo);
					wd-=tempo;
					u_var1=ChipGetField(IOHandle,F0362_LK);
					u_var2=ChipGetField(IOHandle,F0362_PRF);
					u_var3=ChipGetField(IOHandle,F0362_TPS_LOCK);
				}

			if(!u_var1)
			{
#if defined (STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV) || defined (STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV_LOCKSTAGES)
             STTBX_Print(("%s failed NOLOCK_362\n", identity));
#endif
				return NOLOCK_362;
			}


			if(!u_var2)
			{
#if defined (STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV) || defined (STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV_LOCKSTAGES)
             STTBX_Print(("%s failed NOPRFOUND_362\n", identity));
#endif
				return NOPRFOUND_362;
			}
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV_LOCKSTAGES
             STTBX_Print(("%s PRFOUND_362 = OK\n", identity));
#endif
			if (!u_var3)
			{
#if defined (STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV) || defined (STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV_LOCKSTAGES)
             STTBX_Print(("%s failed NOTPS_362\n", identity));
#endif
				return NOTPS_362;
			}
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV_LOCKSTAGES
             STTBX_Print(("%s TPS_362 = OK\n", identity));
#endif

	if(pParams->IF_IQ_Mode!=STTUNER_NORMAL_IF_TUNER)

	{
	 tempo=0;
	 while (((ChipGetField(IOHandle,F0362_COM_USEGAINTRK)!=1)&&(ChipGetField(IOHandle,F0362_COM_AGCLOCK)!=1))&&(tempo<100)) /* to be checked 1000 or 100 */
	 {
	  WAIT_N_MS_362(1);
	  tempo+=1;
	 }
	 ChipSetField(IOHandle,F0362_COM_N,0x17);
	}

#if defined (STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV) || defined (STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV_LOCKSTAGES)
             STTBX_Print(("%s Success  LOCKOK_362\n", identity));
#endif
	return	LOCKOK_362;
}

/*****************************************************
--FUNCTION	::	FE_362_LookFor
--ACTION	::	Intermediate layer before launching Search
--PARAMS IN	::	Handle	==>	Front End Handle
				pSearch ==> Search parameters
				pResult ==> Result of the search
--PARAMS OUT::	NONE
--RETURN	::	Error (if any)
--***************************************************/

FE_362_Error_t	FE_362_LookFor(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle, FE_362_SearchParams_t	*pSearch, FE_362_SearchResult_t *pResult,STTUNER_Handle_t  TopLevelHandle)

{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
   const char *identity = "STTUNER d0362_drv.c FE_362_LookFor()";
#endif
	FE_362_SearchParams_t	 pLook;

	U8 trials[2];
	S8 num_trials=0,index;
	FE_362_Error_t error = FE_362_NO_ERROR;
	U8 flag_spec_inv;

	U8 flag;


	FE_362_InternalParams_t pParams;



	pLook.Frequency	= pSearch->Frequency;
	pLook.Mode 		= pSearch->Mode;
	pLook.Guard		= pSearch->Guard;
	pLook.Inv		= pSearch->Inv;
	pLook.Force     = pSearch->Force;
	pLook.ChannelBW	= pSearch->ChannelBW;
	pLook.EchoPos   = pSearch->EchoPos;

	pLook.IF_IQ_Mode= pSearch->IF_IQ_Mode;
	pParams.Inv	= pSearch->Inv;
	pLook.Hierarchy=pParams.Hierarchy = pSearch->Hierarchy;/*added for hierarchical*/

	flag_spec_inv	= 0;
	flag		= ((pSearch->Inv>>1)&1);


	/*This is made as 0 whenevr FE_362_LookFor fucntion is called.
	  If in a band two different frequencies have different spectrum type then
	  re-initialization of first_lock flag is good. As it will search both the cases
	  in the next freqeuency evenif it finds a channel (INVERTED or NORMAL) spectrum */
	pParams.first_lock=0;

  trials[0]=NINV;
	trials[1]= INV;
	switch (flag)
	{
	  	case 0:
	  		/*trial[] is taken locan unlike GUI 1.8.1 where it is global(Taking consideration of
	  		  multi instance case of sttuner driver)  */

					if ( (pParams.Inv == STTUNER_INVERSION_NONE) || (pParams.Inv == STTUNER_INVERSION))
			   		{
								num_trials=1;
			   		}

					else
			   		{
							num_trials=2;
			   		}

			break;

		case 1:
			num_trials=2;

			if ( (pParams.first_lock)	&& (pParams.Inv == STTUNER_INVERSION_AUTO))
					{
							num_trials=1;
					}
			break;
		default:
			break;

	}



		pResult->SignalStatus=NOLOCK_362;
		index=0;

		while ( ( (index) < num_trials) && (pResult->SignalStatus!=LOCKOK_362))
		{

			if ((!pParams.first_lock)||(pParams.Inv == STTUNER_INVERSION_AUTO) || (pParams.Inv == STTUNER_INVERSION_UNK) )
				{
					pParams.Sense	=  trials[index];
				}

			error=FE_362_Search(Handle,IOHandle,&pLook,&pParams,pResult,TopLevelHandle);


   /*There was updation of trial[] in GUI 1.8.1 . But as this is local then
    it is no longer useful change the values to memorize whether it locked earlier in normal
     or inverted spectrum*/



			index++;
		}
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
   STTBX_Print(("%s  Return value of FE_362_Search Error = %d, \n", identity ,error));
#endif
		return  error;
}

/*****************************************************
--FUNCTION	::	FE_362_Search
--ACTION	::	Search for a valid channel
--PARAMS IN	::	Handle	==>	Front End Handle
				pSearch ==> Search parameters
				pResult ==> Result of the search
--PARAMS OUT::	NONE
--RETURN	::	Error (if any)
--***************************************************/
FE_362_Error_t	FE_362_Search(DEMOD_Handle_t Handle,IOARCH_Handle_t IOHandle, FE_362_SearchParams_t	*pSearch,FE_362_InternalParams_t *Params, FE_362_SearchResult_t *pResult, STTUNER_Handle_t  TopLevelHandle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
   const char *identity = "STTUNER d0362_drv.c FE_362_Search()";
#endif
       S32 offset=0;
S32 offset_type=0;
       U8 constell,counter;
       U8 Rgain,gain_src[2]={0},Rtrlctl,trl_ctl[3]={0},inc_derot[2]={0},tps_rcvd[3]={0},syr_stat[1]={0},agc2max[13]={0},trl_ctl1[5]={0},trl_ctl2[2]={0},crl_freq[3]={0};
       S8 step;
       BOOL TunerLocked=FALSE;
       S32 timing_offset=0;
       U32 trl_nomrate=0;
       STTUNER_tuner_instance_t *TunerInstance;
       STTUNER_InstanceDbase_t     *Inst;
       D0362_InstanceData_t   *Instance;
       FE_362_Error_t error = FE_362_NO_ERROR;
       U32 CrlRegVal=0;
       S16 tempo;
       U8 u_var=0;
			/* top level public instance data */
       Inst = STTUNER_GetDrvInst();
        /* private driver instance data */
       Instance = d0362_GetInstFromHandle(Handle);
        /* get the tuner instance for this driver from the top level handle */
       TunerInstance = &Inst[TopLevelHandle].Terr.Tuner;

      /*added to make sttuner and GUI 1.7.4 and later compatible*/
       pSearch->Force = pSearch->Force + ChipGetField(IOHandle,F0362_FORCE)*2;


    switch(pSearch->IF_IQ_Mode)/*Use macro here */
    {
     case STTUNER_NORMAL_IF_TUNER:  /* Normal IF mode */
    				ChipSetField(IOHandle,F0362_TUNER_BB,0);
    				ChipSetField(IOHandle,F0362_LONGPATH_IF,0);
    				ChipSetField(IOHandle,F0362_INV_SPECTR,1);
    				ChipSetField(IOHandle,F0362_PPM_INVSEL,0); /*spectrum inversion hw detection  off  *db*/
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
   STTBX_Print(("%s Normal IF mode Hardware Spectrum inversion OFF\n", identity ));
#endif
    				break;

    case STTUNER_NORMAL_LONGPATH_IF_TUNER:  /* Long IF mode */
    				ChipSetField(IOHandle,F0362_TUNER_BB,0);
    				ChipSetField(IOHandle,F0362_LONGPATH_IF,1);
    				ChipSetField(IOHandle,F0362_INV_SPECTR,0);
    				ChipSetField(IOHandle,F0362_PPM_INVSEL,0); /*spectrum inversion hw detection off */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
   STTBX_Print(("%s Long Path IF mode Hardware Spectrum inversion OFF\n", identity ));
#endif
    				break;

    	case STTUNER_LONGPATH_IQ_TUNER:  /* IQ mode */

    			ChipSetField(IOHandle,F0362_TUNER_BB,1);
    		 /*ChipSetField(IOHandle,F0362_PMC2_SWAP,1);*/
    			ChipSetField(IOHandle,F0362_INV_SPECTR,0);
    			ChipSetField(IOHandle,F0362_PPM_INVSEL,0); /*spectrum inversion hw detection off */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
   STTBX_Print(("%s Long Path IQ mode Hardware Spectrum inversion OFF \n", identity ));
#endif
    			break;
			default:
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
   STTBX_Print(("%s !!!! Error  FE_362_BAD_PARAMETER (Bad IFIQ mode selected)\n", identity ));
#endif
			return FE_362_BAD_PARAMETER;


    }


/****this part of code is not compared yet with lla*/
    if  ((pSearch->Inv == STTUNER_INVERSION_NONE))
    	{
    		if (pSearch->IF_IQ_Mode== STTUNER_LONGPATH_IQ_TUNER)
            {
            ChipSetField(IOHandle,F0362_PMC2_SWAP,0);
            Instance->ResultSpectrum = (pSearch->IF_IQ_Mode == STTUNER_LONGPATH_IQ_TUNER)? STTUNER_INVERSION_NONE : STTUNER_INVERSION;
            }
			else
            {

            ChipSetField(IOHandle,F0362_INV_SPECTR,(pSearch->IF_IQ_Mode==0));
            Instance->ResultSpectrum = (pSearch->IF_IQ_Mode == STTUNER_NORMAL_IF_TUNER)? STTUNER_INVERSION_NONE : STTUNER_INVERSION;
            }

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
STTBX_Print(("%s ....Spectrum inversion none mode \n", identity ));
#endif

    	}


    else if  (pSearch->Inv == STTUNER_INVERSION)
    	{
    		if (pSearch->IF_IQ_Mode== STTUNER_LONGPATH_IQ_TUNER)
            {
            ChipSetField(IOHandle,F0362_PMC2_SWAP,1);
            Instance->ResultSpectrum = (pSearch->IF_IQ_Mode ==  STTUNER_LONGPATH_IQ_TUNER)?STTUNER_INVERSION : STTUNER_INVERSION_NONE;
            }
            else
            {

            ChipSetField(IOHandle,F0362_INV_SPECTR,!(pSearch->IF_IQ_Mode==0));
            Instance->ResultSpectrum = (pSearch->IF_IQ_Mode ==  STTUNER_NORMAL_IF_TUNER)?STTUNER_INVERSION : STTUNER_INVERSION_NONE;
            }

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
STTBX_Print(("%s ....Spectrum inversion  mode \n", identity ));
#endif

    	}

    else if ((pSearch->Inv == STTUNER_INVERSION_AUTO)||((pSearch->Inv == STTUNER_INVERSION_UNK)&& (!Params->first_lock)) )
    	{
	if (pSearch->IF_IQ_Mode== STTUNER_LONGPATH_IQ_TUNER)
		{
                    if (Params->Sense==1)
                    {

                    ChipSetField(IOHandle,F0362_PMC2_SWAP,1);
                     Instance->ResultSpectrum = (pSearch->IF_IQ_Mode ==  STTUNER_LONGPATH_IQ_TUNER)?STTUNER_INVERSION : STTUNER_INVERSION_NONE;
                    }

                    else
                    {

                    ChipSetField(IOHandle,F0362_PMC2_SWAP,0);
                    Instance->ResultSpectrum = (pSearch->IF_IQ_Mode == STTUNER_LONGPATH_IQ_TUNER)?STTUNER_INVERSION_NONE :  STTUNER_INVERSION ;

                    }
		}
		else
		{
                    if (Params->Sense==1)
                    {

                    ChipSetField(IOHandle,F0362_INV_SPECTR,!(pSearch->IF_IQ_Mode==0));
                    Instance->ResultSpectrum = (pSearch->IF_IQ_Mode ==  STTUNER_NORMAL_IF_TUNER)?STTUNER_INVERSION : STTUNER_INVERSION_NONE;

                    }
    			else
                    {

                    ChipSetField(IOHandle,F0362_INV_SPECTR,(pSearch->IF_IQ_Mode==0));
                    Instance->ResultSpectrum = (pSearch->IF_IQ_Mode == STTUNER_NORMAL_IF_TUNER)?STTUNER_INVERSION_NONE :  STTUNER_INVERSION ;

                    }
                }
 #ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
   STTBX_Print(("%s ....Spectrum inversion auto or unknown  mode with sense %d \n", identity,Params->Sense ));
#endif

    	}

    	/**********/

    if(pSearch->IF_IQ_Mode!=  STTUNER_NORMAL_IF_TUNER)
    {
            FE_362_AGC_IIR_LOCK_DETECTOR_SET(IOHandle);
            if(!FE_362_IIR_FILTER_INIT(IOHandle,pSearch->ChannelBW))
            {
            	return FE_362_BAD_PARAMETER;/*This return value should be changed to a meaningful name*/
            }
                FE_362_AGC_IIR_RESET(IOHandle);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
            STTBX_Print(("%s IIR filter setting for Long path IF and IQ mode \n", identity));
#endif

    	}


/*********Code Added For Hierarchical Modulation****************/
	if (ChipGetField(IOHandle,F0362_TPS_HIERMODE)!=0)
	{

		if ((pSearch->Hierarchy== STTUNER_HIER_PRIO_ANY) || (pSearch->Hierarchy== STTUNER_HIER_HIGH_PRIO)|| (pSearch->Hierarchy== STTUNER_HIER_NONE))
		  {
		/* lock/secure the FEC for HP*/
		/*Reading is not required as it is not used*/
		/* 	ChipGetField(IOHandle,F0362_TPS_HPCODE);*/
		    pSearch->Hierarchy=STTUNER_HIER_HIGH_PRIO;

	    }
	  else
	    {
	    	/*Reading is not required as it is not used*/
	       	/*ChipGetField(IOHandle,F0362_TPS_LPCODE); */
	       pSearch->Hierarchy=STTUNER_HIER_LOW_PRIO;

	    }
  	}
  	else
  	{
  			    	/*Reading is not required as it is not used*/
	     /*ChipGetField(IOHandle,F0362_TPS_HPCODE); */
	     pSearch->Hierarchy=STTUNER_HIER_NONE;

  	}
	if (pSearch->Hierarchy==STTUNER_HIER_LOW_PRIO)
	{
	  	ChipSetField(IOHandle,F0362_BDI_LPSEL,0x01);
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
      STTBX_Print(("%s Hierarchical  Low priority mode  \n", identity));
#endif
	}
	else
	{
	     ChipSetField(IOHandle,F0362_BDI_LPSEL,0x00);
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
	     STTBX_Print(("%s Hierarchical  High priority mode  \n", identity));
#endif
	}
	/*************************/
    if (TunerInstance->Driver->ID != STTUNER_TUNER_RF4000)
    {
    if (pSearch->ChannelBW == STTUNER_CHAN_BW_6M)
            {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
             STTBX_Print(("%s GAIN_SRC, TRL, INC DEROT settings for Tuner other than RF4K in 6MHz mode  \n", identity));
#endif
            Rgain=ChipGetOneRegister(IOHandle,R0362_GAIN_SRC1);
            gain_src[0]=(Rgain & 0xf0) |M6_F_GAIN_SRC_HI;
            gain_src[1]=M6_F_GAIN_SRC_LO;

            ChipSetRegisters(IOHandle,R0362_GAIN_SRC1,gain_src,2);

            Rtrlctl=ChipGetOneRegister(IOHandle,R0362_TRL_CTL);

            trl_ctl[0]=(Rtrlctl & 0x7f)| (M6_F_TRL_NOMRATE0<<7);
            trl_ctl[1]=M6_F_TRL_NOMRATE8_1;
            trl_ctl[2]=M6_F_TRL_NOMRATE16_9;

            ChipSetRegisters(IOHandle,R0362_TRL_CTL,trl_ctl,3);
            }
    else if (pSearch->ChannelBW == STTUNER_CHAN_BW_7M)
            {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
             STTBX_Print(("%s GAIN_SRC, TRL, INC DEROT settings for Tuner other than RF4K in 7MHz mode  \n", identity));
#endif
 if (TunerInstance->Driver->ID != STTUNER_TUNER_TDTGD108)/*for LG tuner*/
            {

            Rtrlctl=ChipGetOneRegister(IOHandle,R0362_TRL_CTL);

            trl_ctl[0]=(Rtrlctl & 0x7f)| (M7_F_TRL_NOMRATE0<<7);
            trl_ctl[1]=M7_F_TRL_NOMRATE8_1;
            trl_ctl[2]=M7_F_TRL_NOMRATE16_9;

 }else
            {
            trl_ctl[0]=0x94;
            trl_ctl[1]=0xda;
            trl_ctl[2]=0x4b;
            }


            ChipSetRegisters(IOHandle,R0362_TRL_CTL,trl_ctl,3);

            if (pSearch->IF_IQ_Mode == STTUNER_NORMAL_IF_TUNER)
            {

  if (TunerInstance->Driver->ID != STTUNER_TUNER_TDTGD108)/*for LG tuner*/
            {

            Rgain=ChipGetOneRegister(IOHandle,R0362_GAIN_SRC1);
            gain_src[0]=(Rgain & 0xf0) |M7_GAIN_SRC_HI;
            gain_src[1]=M7_GAIN_SRC_LO;
 }else{
		gain_src[0]=0xf9;
            	gain_src[1]=0x7b;
	}

            ChipSetRegisters(IOHandle,R0362_GAIN_SRC1,gain_src,2);
            }
            else
            {

            Rgain=ChipGetOneRegister(IOHandle,R0362_GAIN_SRC1);
            gain_src[0]=(Rgain & 0xf0) |M7_E_GAIN_SRC_HI;
            gain_src[1]=M7_E_GAIN_SRC_LO;

            ChipSetRegisters(IOHandle,R0362_GAIN_SRC1,gain_src,2);

            }


            }
    else /* Channel Bandwidth = 8M) */
            {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
             STTBX_Print(("%s GAIN_SRC, TRL, INC DEROT settings for Tuner other than RF4K in 8MHz mode  \n", identity));
#endif


            if (Inst->IOCTL_Set_30MZ_REG_Flag != TRUE)

            {
            if (TunerInstance->Driver->ID != STTUNER_TUNER_TDTGD108)/*for LG tuner*/
            {
            if (TunerInstance->Driver->ID == STTUNER_TUNER_MT2266)
            {

            trl_ctl[0]=0x14;
            trl_ctl[1]=0xb2;
            trl_ctl[2]=M8_F_TRL_NOMRATE16_9;
            }
            else
            {

            Rtrlctl=ChipGetOneRegister(IOHandle,R0362_TRL_CTL);

            trl_ctl[0]=(Rtrlctl & 0x7f)| (M8_F_TRL_NOMRATE0<<7);
            trl_ctl[1]=M8_F_TRL_NOMRATE8_1;
            trl_ctl[2]=M8_F_TRL_NOMRATE16_9;


            }


            }

            else
            {
            trl_ctl[0]=0x94;
            trl_ctl[1]=0xb1;
            trl_ctl[2]=M8_F_TRL_NOMRATE16_9;

            }

            ChipSetRegisters(IOHandle,R0362_TRL_CTL,trl_ctl,3);

if (TunerInstance->Driver->ID == STTUNER_TUNER_TDQD3)/*as IF changed to 36.167 MHz*/
            {
            inc_derot[0]=0x54;
            inc_derot[1]=0x8b;

            ChipSetRegisters(IOHandle,R0362_INC_DEROT1,inc_derot,2);

            /*Make COR_CTL(0x80) register value 0x23*/
            
            }


            }
            if (pSearch->IF_IQ_Mode == STTUNER_NORMAL_IF_TUNER)
            {

            if (TunerInstance->Driver->ID == STTUNER_TUNER_TDTGD108)/*for LG tuner*/
            {
            gain_src[0]=0xfc;
            gain_src[1]=0xb2;
            }
            else
            {
            Rgain=ChipGetOneRegister(IOHandle,R0362_GAIN_SRC1);
            gain_src[0]=(Rgain & 0xf0) |M8_GAIN_SRC_HI;
            gain_src[1]=M8_GAIN_SRC_LO;
            }
            ChipSetRegisters(IOHandle,R0362_GAIN_SRC1,gain_src,2);
            }
            else
            {

            Rgain=ChipGetOneRegister(IOHandle,R0362_GAIN_SRC1);
            gain_src[0]=(Rgain & 0xf0) |M8_E_GAIN_SRC_HI;
            gain_src[1]=M8_E_GAIN_SRC_LO;

            ChipSetRegisters(IOHandle,R0362_GAIN_SRC1,gain_src,2);

            }


            if (TunerInstance->Driver->ID == STTUNER_TUNER_TDTGD108)/*for LG tuner*/
            {
            inc_derot[0]=0x54;
            inc_derot[1]=0x97;

            ChipSetRegisters(IOHandle,R0362_INC_DEROT1,inc_derot,2);

            /*Make COR_CTL(0x80) register value 0x23*/
            ChipSetOneRegister(IOHandle,R0362_COR_CTL,0x23);
            }


            }

            }
    else
		/****************/
            {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
             STTBX_Print(("%s GAIN_SRC, TRL, INC DEROT settings for  RF4K tuner in 8MHz mode  \n", identity));
#endif
            ChipSetOneRegister(IOHandle,R0362_GAIN_SRC1,0x5f);   /*RF4K work around*/
            Rgain=ChipGetOneRegister(IOHandle,R0362_GAIN_SRC1);
            gain_src[0]=(Rgain & 0xf0) |0x0f;
            gain_src[1]=0x3c;

            ChipSetRegisters(IOHandle,R0362_GAIN_SRC1,gain_src,2);



            Rtrlctl=ChipGetOneRegister(IOHandle,R0362_TRL_CTL);

            trl_ctl[0]=(Rtrlctl & 0x7f)| (0x01<<7);
            trl_ctl[1]=0x83;
            trl_ctl[2]=0x61;
            ChipSetRegisters(IOHandle,R0362_TRL_CTL,trl_ctl,3);

            inc_derot[0]=0x40;
            inc_derot[1]=0x00;

            ChipSetRegisters(IOHandle,R0362_INC_DEROT1,inc_derot,2);


            }

            pSearch->EchoPos   = pSearch->EchoPos;
            ChipSetField(IOHandle,F0362_LONG_ECHO,pSearch->EchoPos);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
STTBX_Print(("%s  ,Frequency = %d is going to set in tuner  \n", identity,pSearch->Frequency));
#endif
            error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle,(U32)pSearch->Frequency,(U32 *)&pSearch->Frequency);


            if (TunerInstance->Driver->ID != STTUNER_TUNER_DTT7592)
            {
            FE_362_WaitTuner(TunerInstance,100,&TunerLocked);		/*	Is tuner Locked	? (wait 100 ms maxi)	*/
            }
            else
            {
            TunerLocked= TRUE;/*For DTT7592 Tunerlocked flag is made true by default. Actually tuner locked isn't checked */
            }

            if((error != ST_NO_ERROR) || (TunerLocked ==FALSE) )
            {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
             STTBX_Print(("%s failed ....Tuner not locked \n", identity));
#endif
            return(FE_362_SEARCH_FAILED);

            }


   	if((pResult->SignalStatus = FE_362_Algo( IOHandle,pSearch,TopLevelHandle)) == LOCKOK_362)
    {


    	pResult->Locked = TRUE;

			pResult->SignalStatus =LOCKOK_362;
			/* update results */


			tps_rcvd[0]=ChipGetOneRegister(IOHandle,R0362_TPS_RCVD2);
			tps_rcvd[1]=ChipGetOneRegister(IOHandle,R0362_TPS_RCVD3);
			tps_rcvd[2]=ChipGetOneRegister(IOHandle,R0362_TPS_RCVD4);

			ChipGetRegisters( IOHandle,R0362_SYR_STAT,1,syr_stat);

			pResult->Mode=(syr_stat[0]&0x04)>>2; /*SYR_SAT@0x92[2]--Previously equivalent to get field image*/
			pResult->Guard=syr_stat[0]&0x03; /*SYR_SAT@0x92[:0]--Previously equivalent to get field image*/


			constell = tps_rcvd[0]&0x03;/*TPS_RCV2@0xA8[1:0]*/


			if (constell == 0)
				{
					pResult->Modulation = STTUNER_MOD_QPSK;
				}
			else if (constell==1)
				{
					pResult->Modulation= STTUNER_MOD_16QAM;
				}
			else
				{
					pResult->Modulation= STTUNER_MOD_64QAM;
				}

			/***Code replced and changed  for HM**/
			pResult->hier=pSearch->Hierarchy;
			pResult->Hierarchy_Alpha  =(tps_rcvd[0]&0x70)>>4;/*TPS_RCV2@0xA8[6:4]*/
			/****/
			pResult->Hprate=tps_rcvd[1] & 0x07;/*TPS_RCV2@0xA9[2:0]*/


			pResult->Lprate=(tps_rcvd[1] &0x70)>>4;/*TPS_RCV2@0xA9[6:4]*/
			/****/
			constell = ChipGetField(IOHandle,F0362_PR);
			if (constell==5)
				{
						constell = 4;
				}
			pResult->pr = (FE_362_Rate_t) constell;


			pResult->Sense=ChipGetField(IOHandle,F0362_INV_SPECTR);/*????*/


			Params->first_lock=1;


			ChipGetRegisters( IOHandle,R0362_AGC2MAX,13,agc2max);/*It should be 13 instead of 6???*/
			pResult->Agc_val=	(agc2max[9]<<16) 	+
        			((agc2max[10]&0x0f)<<24) +
        			agc2max[11] +
        			((agc2max[12]&0x0f)<<8);

			/* Carrier offset calculation */
			ChipSetField(IOHandle,F0362_FREEZE,1);
			ChipGetRegisters(IOHandle,R0362_CRL_FREQ1,3,crl_freq);
			ChipSetField(IOHandle,F0362_FREEZE,0);

			CrlRegVal = (crl_freq[2]<<16) ;
      CrlRegVal+= (crl_freq[1] <<8);
      CrlRegVal+= crl_freq[0];

			if(CrlRegVal >8388607)
			{
				offset = CrlRegVal - 16777216;/*2's complement negative value*/
				
			}else
			{
			offset = CrlRegVal;	
			
			}
					
			
			offset=offset*2/16384;

      if(pResult->Mode==STTUNER_MODE_2K)
        {
            offset=(offset*4464)/1000;/*** 1 FFT BIN=4.464khz***/
        }
			else if(pResult->Mode==STTUNER_MODE_4K)
         {
            offset=(offset*223)/100;/*** 1 FFT BIN=2.23khz***/
         }
       else  if(pResult->Mode==STTUNER_MODE_8K)
          {
             offset=(offset*111)/100;/*** 1 FFT BIN=1.1khz***/
			 		}
			 		
			 		  
         
         /*pSearch->Frequency +=offset;*/
        pResult->Frequency= pSearch->Frequency;
        
     if (Instance->ResultSpectrum == STTUNER_INVERSION)
     {
     	offset=offset;
     	}else
     	{
     	
     	offset=offset*-1;
     	}
     
     if (offset > 0)
     {
     	offset_type=STTUNER_OFFSET_POSITIVE;
     	}  else if (offset < 0)
     	{
     		offset_type=STTUNER_OFFSET_NEGATIVE;
     		}else
     		{
     		offset_type=STTUNER_OFFSET_NONE;
     		}
       
       pResult->offset=offset;
        pResult->offset_type=offset_type;


			  pResult->Echo_pos=ChipGetField(IOHandle,F0362_LONG_ECHO);

				tempo=10;  /* exit even if timing_offset stays null *db* */
				while (( timing_offset==0)&&( tempo>0))
				{
					WAIT_N_MS_362(10) ;  /*was 20ms*/
					/* fine tuning of timing offset if required */
					 ChipGetRegisters(IOHandle,R0362_TRL_CTL,5,trl_ctl1);
					timing_offset=trl_ctl1[3]+ 256*trl_ctl1[4];/*TRL_TIME2@0x9E = TRL_OFFSET[15:8],TRL_TIME1@9D=TRL_OFFSET[7:0]*/

					if (timing_offset>=32768)
						{
							timing_offset-=65536;
						}
				    /*	timing_offset=(timing_offset+10)/20; */ /* rounding */
					trl_nomrate=  (512*trl_ctl1[2]+trl_ctl1[1]*2 + ((trl_ctl1[0]&0x80)>>7));/*TRL_NOMRATE @0x9C[16:9]+TRL_NOMRATE @0x9B[8:1]+TRL_CTL 0x@9A,TRL_NOMRATE[0]*/
						/*Calculate the timing offset in ppm.. (TRL_OFFSET*10e6)/(32*128*TRL_NOMRATE/2)*/
					timing_offset=((signed)(1000000/trl_nomrate)*timing_offset)/2048;
					tempo--;
				}

				if (timing_offset<=0)
					{
					timing_offset=(timing_offset-11)/22;
					step=-1;
					}
				else
					{
					timing_offset=(timing_offset+11)/22;
					step=1;
					}

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
             STTBX_Print(("%s Timing offset value %d \n", identity,timing_offset));
#endif

				for (counter=0;counter<abs(timing_offset);counter++)
					{
						trl_nomrate+=step;
						Rtrlctl=ChipGetOneRegister(IOHandle,R0362_TRL_CTL);
						trl_ctl2[0]=((trl_nomrate%2)<<7) | (Rtrlctl & 0x7f) ;
            trl_ctl2[1]=trl_nomrate/2;

            ChipSetRegisters(IOHandle,R0362_TRL_CTL,trl_ctl2,2);
						WAIT_N_MS_362(1);
					}

				WAIT_N_MS_362(5);
				/* unlocks could happen in case of trl centring big step, then a core off/on restarts demod */
				u_var=ChipGetField(IOHandle,F0362_LK);

				 if(!u_var)
				{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
             STTBX_Print(("%s Core ON /OFF after TRL fine centering \n", identity));
#endif
					ChipSetField(IOHandle,F0362_CORE_ACTIVE,0);
					WAIT_N_MS_362(20);
					ChipSetField(IOHandle,F0362_CORE_ACTIVE,1);
	      }


        }
        else
        {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
             STTBX_Print(("%s failed .... Demod  not locked \n", identity));
#endif
            pResult->Locked = FALSE;
               error = FE_362_SEARCH_FAILED;
        }


	return error;

}

/*----------------------------------------------------
FUNCTION    :  FE_362_GetNoiseEstimator
ACTION :
PARAMS IN :
PARAMS OUT :
RETURN :
------------------------------------------------------*/
void FE_362_GetNoiseEstimator(IOARCH_Handle_t IOHandle, U32 *pNoise, U32 *pBer)
{
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
   const char *identity = "STTUNER d0362_drv.c FE_362_GetNoiseEstimator()";
#endif
short unsigned int  source,prf;
U32                 quoz,error;
U32 snr=0;

error  = ChipGetField(IOHandle,F0362_ERROR_COUNT1_LO);
error += ChipGetField(IOHandle,F0362_ERROR_COUNT1_HI) * 256;
error  = error*100;

quoz=1;
if (!ChipGetField(IOHandle,F0362_ERRMODE1))
{
source=ChipGetField(IOHandle,F0362_ERR_SOURCE1);
quoz=FE_362_PowOf2(12+2*(ChipGetField(IOHandle,F0362_NUM_EVENT1)));

switch (source)
{
case 0:
quoz=quoz*8;
prf=ChipGetField(IOHandle,F0362_PR);

switch(prf)
{
case 0:
error=(U32)(error*(1/2)); /*gbgbcast*/
break;

case 1:
error=(U32)(error*(2/3)); /*gbgbcast*/
break;

case 2:
error=(U32)(error*(3/4)); /*gbgbcast*/
break;

case 3:
error=(U32)(error*(5/6)); /*gbgbcast*/
break;

case 4:
error=(U32)(error*(6/7)); /*gbgbcast*/
break;

case 5:
error=(U32)(error*(7/8)); /*gbgbcast*/
break;

default:
error=error;
break;

}
break;

case 1:
quoz=quoz*8;
break;

case 2:
break;

case 3:
error=error*188;
break;

default:
error=error;
break;
}
}

snr=ChipGetField(IOHandle,F0362_CHC_SNR);


/**pNoise = (snr*10) >> 3;*/
/**  fix done here for the bug GNBvd20972 where pNoise is calculated in right percentage **/
*pNoise=((snr/8)*100)/32;
*pBer   = error*(100000000/quoz); /**** (error/quoz) gives my actual BER . We multiply it with
                                        10*e+8 now and 10*e+2 before for scaling purpose so that we can send a integer
                                        value to application user . So total scaling factor is 10*e+10*****/
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
  STTBX_Print(("%s C/N %d and BER %d values \n", identity,*pNoise,*pBer));
#endif
return;
}



/*********************************************************
--FUNCTION	::	FE_362_Tracking
--ACTION	::	Once the chip is locked tracks its state
--PARAMS IN	::	period/duration of tracking in seconds
			::	Handle to Chip
--PARAMS OUT::	NONE
--RETURN	::	NONE
--********************************************************/
void FE_362_Tracking(IOARCH_Handle_t IOHandle,U32 *UnlockCounter)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
   const char *identity = "STTUNER d0362_drv.c FE_362_Tracking()";
#endif

 if ((!ChipGetField(IOHandle,F0362_LK)))
  {

    *UnlockCounter +=1;

  }
 else
  {
    *UnlockCounter =0;

    return;
  }
 if (*UnlockCounter >2 )
 {

	if (!ChipGetField(IOHandle,F0362_TPS_LOCK) || (!ChipGetField(IOHandle,F0362_LK))  )
		{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
  STTBX_Print(("%s Doing Core ON/OFF in tracking\n", identity));
#endif

			 ChipSetField(IOHandle,F0362_CORE_ACTIVE,0);
			 WAIT_N_MS_362(2);
			 ChipSetField(IOHandle,F0362_CORE_ACTIVE,1);
			 WAIT_N_MS_362(250);/*Delay changed to give more time to stabilize lock 21/12/2006*/

		}
}


	return;


}

/*********************************************************
--FUNCTION	::	duration
--ACTION	::	wait for a duration regarding mode
--PARAMS IN	::	mode, tempo1,tempo2,tempo3
--PARAMS OUT::	none
--********************************************************/
S16 duration( S32 mode, int tempo1,int tempo2,int tempo3)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
   const char *identity = "STTUNER d0362_drv.c duration()";
#endif
   int local_tempo=0;
	switch(mode)
	{
		case 0:
			local_tempo=tempo1;
		break;
	    case 1:
			local_tempo=tempo2;
		break ;

		case 2:
			local_tempo=tempo3;
		break;

		default:

		break;
	}

	return local_tempo;
}

/*********************************************************
--FUNCTION	::	CheckSYR_362
--ACTION	::	Check for SYMBOL recovery loop lock status
--PARAMS IN	::	IOARCH_Handle,STTUNER_IOREG_DeviceMap_t
--PARAMS OUT::	SYR check status
--********************************************************/
FE_362_SignalStatus_t CheckSYR_362( IOARCH_Handle_t IOHandle)
{

#ifdef STTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
   const char *identity = "STTUNER d0362_drv.c CheckSYR_362()";
#endif

		S32 wd=100;
		U16 SYR_var;
		S32 SYRStatus;

	  SYR_var=ChipGetField(IOHandle,F0362_SYR_LOCK);

			while ( (!SYR_var) && (wd>0))
			{
				WAIT_N_MS_362(2);
				wd-=2;
				SYR_var=ChipGetField(IOHandle,F0362_SYR_LOCK);
			}
			if(!SYR_var)
			{

				SYRStatus= NOSYMBOL_362;
			}
			else
			{
				SYRStatus=  SYMBOLOK_362;
			}
	return SYRStatus;
}












