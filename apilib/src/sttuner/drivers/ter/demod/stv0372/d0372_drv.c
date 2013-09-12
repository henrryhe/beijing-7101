/* -------------------------------------------------------------------------
File Name: d0372_drv.c

Description: 372 driver LLA

Copyright (C) 2005-206 STMicroelectronics

History:
   date: 16-December-2005
version: 1.0.0
 author: 
comment: 

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
#include"ioreg.h"
#include "chip.h"
#include "tuntdrv.h"
/* common includes */
#include "d0372.h" 
#include "d0372_drv.h"
#include "d0372_util.h"
#include "d0372_map.h"

/* Current LLA revision	*/
static ST_Revision_t Revision372  = "STV0372-LLA_REL_1.0(GUI) ";


#define WAIT_N_MS_372(X)     STOS_TaskDelayUs(X * 1000)   
extern IOARCH_HandleData_t IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];

const FE_372_LOOKUP_t FE_372_CN_LookUp ={44,{ {19,  16383},
					 {20,  16144},													
					 {30,  12823},
					 {40,  10186},
					 {50,  8091},
					 {60,  64267},
					 {70,  5105},
					 {80,  4055},
					 {90,  3221},
					 {100, 2559},
					 {110, 2032},
					 {120, 1614},
					 {130, 1282},
					 {140, 1019},
					 {150, 809},
					 {160, 643},
					 {170, 511},
					 {180, 406},
					 {190, 322},
					 {200, 256},
					 {210, 203},
					 {220, 161},
					 {230, 128},
					 {240, 102},
					 {250, 81},
					 {260, 64},
					 {270, 51},
					 {280, 41},
					 {290, 32},
					 {300, 26},
					 {310, 20},
					 {320, 16},
					 {330, 13},
					 {340, 10},
					 {350, 8},
					 {360, 6},
					 {370, 5},
					 {380, 4},
					 {390, 3},
					 {400, 3},
					 {410, 2},
					 {420, 2},
					 {430, 1},
					 {440, 1},
					}
				};
											

const FE_372_LOOKUP_t FE_372_BER_LookUp =	{115,{{0,	0},
						 {126,	75},
						 {260,	150},
						 {387,	220},
						 {533,	300},
						 {681,	380},
						 {831,	460},
						 {981,	540},
						 {1113,	610},
						 {1265,	690},
						 {1417,	770},
						 {1570,	850},
						 {1704,	920},
						 {1858,	1000},
						 {2051,	1100},
						 {2244,	1200},
						 {2438,	1300},
						 {2633,	1400},
						 {2828,	1500},
						 {3023,	1600},
						 {3219,	1700},
						 {3416,	1800},
						 {3613,	1900},
						 {3810,	2000},
						 {4007,	2100},
						 {4205,	2200},
						 {4404,	2300},
						 {4602,	2400},
						 {4801,	2500},
						 {5000,	2600},
						 {5199,	2700},
						 {5399,	2800},
						 {5599,	2900},
						 {5799,	3000},
						 {6000,	3100},
						 {6200,	3200},
						 {6401,	3300},
						 {6602,	3400},
						 {6803,	3500},
						 {7005,	3600},
						 {7207,	3700},
						 {7409,	3800},
						 {7611,	3900},
						 {7813,	4000},
						 {8015,	4100},
						 {8218,	4200},
						 {8421,	4300},
						 {8624,	4400},
						 {8827,	4500},
						 {9030,	4600},
						 {9234,	4700},
						 {9437,	4800},
						 {9641,	4900},
						 {9845,	5000},
						 {10049,	5100},
						 {10253,	5200},
						 {10458,	5300},
						 {10662,	5400},
						 {10867,	5500},
						 {11072,	5600},
						 {11276,	5700},
						 {11481,	5800},
						 {11687,	5900},
						 {11892,	6000},
						 {12097,	6100},
						 {12303,	6200},
						 {12508,	6300},
						 {12714,	6400},
						 {12920,	6500},
						 {13126,	6600},
						 {13332,	6700},
						 {13538,	6800},
						 {13745,	6900},
						 {13951,	7000},
						 {14158,	7100},
						 {14364,	7200},
						 {14571,	7300},
						 {14778,	7400},
						 {14985,	7500},
						 {15192,	7600},
						 {15399,	7700},
						 {15606,	7800},
						 {15814,	7900},
						 {16021,	8000},
						 {16229,	8100},
						 {16436,	8200},
						 {16644,	8300},
						 {16852,	8400},
						 {17060,	8500},
						 {17268,	8600},
						 {17476,	8700},
						 {17684,	8800},
						 {17892,	8900},
						 {18100,	9000},
						 {18309,	9100},
						 {18517,	9200},
						 {18726,	9300},
						 {18935,	9400},
						 {19143,	9500},
						 {19352,	9600},
						 {19561,	9700},
						 {19770,	9800},
						 {19979,	9900},
						 {20188,	10000},
						 {41397,	20000},
						 {63009,	30000},
						{84886,	40000},
						{106964,	50000},
						{129202,	60000},
						{151574,	70000},
						{174063,	80000},
						{196653,	90000},
						{219333,	100000},
						{449751,	200000},
						{684546,	300000},
					}
				};
/***********************************************************
**FUNCTION	::	Drv0361_GetLLARevision
**ACTION	::	Returns the 361 LLA driver revision
**RETURN	::	Revision361
***********************************************************/
ST_Revision_t Drv0372_GetLLARevision(void)
{
	return (Revision372);
}											

/*****************************************************
--FUNCTION	::	FE_372_WaitTuner
--ACTION	::	Wait for tuner locked
--PARAMS IN	::	TimeOut	->	Maximum waiting time (in ms) 
--PARAMS OUT::	NONE
--RETURN	::	NONE
--***************************************************/
void FE_372_WaitTuner(STTUNER_tuner_instance_t *TunerInstance,int TimeOut)
{
	int Time=0;
	BOOL TunerLocked = FALSE;
	ST_ErrorCode_t Error;
	
	while(!TunerLocked && (Time<TimeOut))
	{
		WAIT_N_MS_372(1);
		Error = (TunerInstance->Driver->tuner_IsTunerLocked)(TunerInstance->DrvHandle, &TunerLocked); 
		Time++;
	}
	Time--;
}

/*****************************************************
--FUNCTION	::	CheckTiming_372
--ACTION	::	Check for timing locked
--PARAMS IN	::	pParams->Ttiming	=>	Time to wait for timing loop locked
--PARAMS OUT::	pParams->State		=>	result of the check
--RETURN	::	NOTIMING if timing not locked, TIMINGOK otherwise
--***************************************************/
FE_372_SIGNALTYPE_t CheckTiming_372(IOARCH_Handle_t IOHandle,FE_372_InternalParams_t *pParams)
{
	U32 TimingLock,Time=0; 
	do
	{
	
	 WAIT_N_MS_372(1);
	 TimingLock=ChipGetField(IOHandle,F0372_FREQ_LOCK);
	 Time++; 
	}while((TimingLock==0)&&(Time < pParams->Ttiming)); 
	
	
	if (ChipGetField(IOHandle,F0372_FREQ_LOCK))
		pParams->State = TIMINGOK_372;
	else
		pParams->State = NOTIMING_372;
	
	return pParams->State;
}


/*****************************************************
--FUNCTION	::	CheckCarrier_372
--ACTION	::	Check for carrier founded
--PARAMS IN	::	pParams		=>	Pointer to SEARCHPARAMS structure
--PARAMS OUT::	pParams->State	=> Result of the check
--RETURN	::	NOCARRIER carrier not founded, CARRIEROK otherwise
--***************************************************/
FE_372_SIGNALTYPE_t CheckCarrier_372(IOARCH_Handle_t IOHandle,FE_372_InternalParams_t *pParams)
{
	U32 CarrierLock,Time=0;
	/*WAIT_N_MS_372(pParams->Tcarrier);*/
	do
	{
	
	 WAIT_N_MS_372(1);
	 CarrierLock=ChipGetField(IOHandle,F0372_CY_LOCK);
	 Time++; 
	}while((CarrierLock==0)&&(Time < pParams->Tcarrier)); 
	
	
	if (ChipGetField(IOHandle,F0372_CY_LOCK))
		pParams->State = CARRIEROK_372;
	else
		pParams->State = NOCARRIER_372;
		
	return pParams->State;
}


/*****************************************************
--FUNCTION	::	CheckData_372
--ACTION	::	Check for data founded
--PARAMS IN	::	pParams		=>	Pointer to SEARCHPARAMS structure    
--PARAMS OUT::	pParams->State	=> Result of the check
--RETURN	::	NODATA data not founded, DATAOK otherwise
--***************************************************/
FE_372_SIGNALTYPE_t CheckData_372(IOARCH_Handle_t IOHandle,FE_372_InternalParams_t *pParams)
{
	U32 DataLock,Time=0;
	pParams->State = NODATA_372;   
	
	/*WAIT_N_MS_372(pParams->Tdata);*/			/*	Wait for data				*/    
	do
	{
	
	 WAIT_N_MS_372(1);
	 DataLock=ChipGetField(IOHandle,F0372_FRMLOCK);
	 Time++; 
	}while((DataLock==0)&&(Time < pParams->Tdata)); 

	if (ChipGetField(IOHandle,F0372_FRMLOCK))   /*	Read DATA LOCK indicator	*/
	{							
		pParams->State = DATAOK_372; 
	}
	else
	{
		pParams->State = NODATA_372;
	}

		
	return pParams->State;
}

FE_372_SIGNALTYPE_t CheckFSM1_372(IOARCH_Handle_t IOHandle,FE_372_InternalParams_t *pParams)
{
	U32 FSM1Lock,Time=0;
	/*	Wait for FSM1 Locked				*/   
	pParams->State = NOFSM1_372;   
	do
	{
	
	 WAIT_N_MS_372(1);
	 FSM1Lock=(ChipGetField(IOHandle,F0372_MAINSTATE)==0x0A);
	 Time++; 
	}while((FSM1Lock==0)&&(Time < pParams->TFSM1)); 
	

	if (ChipGetField(IOHandle,F0372_MAINSTATE)==0x0A)							/*	Read FSM1 LOCK indicator	*/
	{
		pParams->State = FSM1OK_372; 
	}
	else
	{
		pParams->State = NOFSM1_372; 
	}

		
	return pParams->State;
}

FE_372_SIGNALTYPE_t CheckFSM2_372(IOARCH_Handle_t IOHandle,FE_372_InternalParams_t *pParams)
{
	U32 FSM2Lock,Time=0;
	/*	Wait for FSM2 Locked				*/   
	pParams->State = NOFSM2_372;   
		do
	{
	
	 WAIT_N_MS_372(1);
	 FSM2Lock=(ChipGetField(IOHandle,F0372_EQSTATE)==0x05);
	 Time++; 
	}while((FSM2Lock==0)&&(Time<pParams->TFSM2)); 

	if (ChipGetField(IOHandle,F0372_EQSTATE)==0x05)							/*	Read FSM2 LOCK indicator	*/
	{
		pParams->State = FSM2OK_372; 
	}
	else
	{
		pParams->State = NOFSM2_372; 
	}

		
	return pParams->State;
}

/*****************************************************
**FUNCTION	::	FE_372_SetInternalError
**ACTION	::	Set the internal error value and location 
**PARAMS IN	::	Type		==> Type of the error
**				Location	==> Location of the error
**PARAMS OUT::	pError
**RETURN	::	NONE
*****************************************************/
void FE_372_SetInternalError(FE_372_ErrorType_t Type,FE_372_Location_t Location,FE_372_InternalError_t *pError)
{
	if(pError != NULL)
	{
		pError->Type = Type;
		pError->Location = Location;
	}
}


FE_372_Error_t STV0372_SetCarrier(IOARCH_Handle_t IOHandle,U32 IF,int clk,U32 IQMode)
{
	FE_372_Error_t error = FE_372_NO_ERROR;
	S32 IFfrequency;
	if(IQMode == STTUNER_INVERSION)
	{
	IFfrequency=(S32)(IF*1000 - clk);
        }
        else
        {
        	IFfrequency=(S32)(2*clk-IF*1000);
        }
	
	UTIL_372_Set_NCOcnst_Regs(IOHandle,(int)UTIL_372_Calc_NCOcnst(IFfrequency,clk));
	
	
	return error;
}

FE_372_Error_t STV0372_SetTiming(IOARCH_Handle_t IOHandle,int clk)
{
	FE_372_Error_t error = FE_372_NO_ERROR;
		
	 UTIL_372_Set_vcxoOffset_Regs(IOHandle,(int)UTIL_372_Calc_vcxoOffset(SYMBOLRATE_372,clk));
		
	return error;
}



/*****************************************************
--FUNCTION	::	FE_372_Algo
--ACTION	::	Search for Signal, Timing, Carrier and then data at a given Frequency, 
--				in a given range
--PARAMS IN	::	NONE
--PARAMS OUT::	NONE
--RETURN	::	Type of the founded signal (if any)
--***************************************************/
FE_372_SIGNALTYPE_t FE_372_Algo(IOARCH_Handle_t IOHandle,FE_372_InternalParams_t *pParams,STTUNER_Handle_t TopLevelHandle)
{
        S32 EQResetNb;
	S32 EQState,MAINState;
	STTUNER_InstanceDbase_t *Inst;
        STTUNER_tuner_instance_t *TunerInstance; 
        ST_ErrorCode_t              Error = ST_NO_ERROR;
       	U8 EnableEQreset=1;
	U32 FirstWatchdog,FirstTimeOutEQ = 0,Watchdog = 0,TimeOutEQ;
	S32 powerestimation;
	U8 AgcVal1,AgcVal2,AgcVal3; 
	/* top level public instance data */ 
        Inst = STTUNER_GetDrvInst();
       
        /* get the tuner instance for this driver from the top level handle */
        TunerInstance = &Inst[TopLevelHandle].Terr.Tuner;
	/*pParams->Frequency = pParams->BaseFreq;*/ /*commented for repeated call*/
	
	pParams->Ttiming  = 100;
	pParams->Tcarrier = 150;
	
	pParams->Tdata = 200;
	pParams->TFSM1 = 500;
	pParams->TFSM2 = 300;
	pParams->Frequency = pParams->BaseFreq;
	pParams->TunerOffset = 0;
	EQResetNb=0;
		
	/* May be must called from Init routine */
	STV0372_SetCarrier(IOHandle,pParams->IF,pParams->Quartz,pParams->IQMode); 
	STV0372_SetTiming(IOHandle,pParams->Quartz);
	   
	   
	     	
	/**********************************************/
	Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle,(U32)pParams->Frequency,(U32 *)&pParams->Frequency);	/*	Move the tuner	*/   
	
	/*	Temporisations	*/ 
	FE_372_WaitTuner(TunerInstance,100);		/*	Is tuner Locked	? (wait 100 ms maxi)	*/  
	
	
	
	/*Power estimation. Check whether power level is within 
	 threshold level or not. If yes then proceed for lock
	  else return without doing anything*/
	  
	  AgcVal1 = ChipGetField(IOHandle,F0372_AGC_IND_LSB);
	  AgcVal2 = ChipGetField(IOHandle,F0372_AGC_IND_MSB);
	  AgcVal3 = ChipGetField(IOHandle,F0372_AGC_IND_MMSB);
	  
	  powerestimation=AgcVal1 + AgcVal2*256+ AgcVal3*65536; 
	          					
	
	
	if (powerestimation>=131072)
	{
		powerestimation= powerestimation-262144;
		
		}
	
	if(powerestimation<=0)
	{
		powerestimation=-powerestimation;
	}
		
	powerestimation=(S32)(powerestimation/(UTIL_372_PowOf2(11-ChipGetField(IOHandle,F0372_AGCBWSEL))));
	if ((powerestimation< pParams->LOWPowerTh)||(powerestimation > pParams->UPPowerTh))
	{		
		pParams->State=NOPOWER_372;		
		return pParams->State;
	}
	else
	{		
		pParams->State=POWEROK_372;
		
	}	
				
	ChipSetField(IOHandle,F0372_MAXNBFRAMEDD_LSB,0x00)	;
	ChipSetField(IOHandle,F0372_MAXNBFRAMERCA,0x00)	;
	ChipSetField(IOHandle,F0372_NCO_TIMEOUT_MMSB,0x00)	;
	ChipSetField(IOHandle,F0372_NCO_TIMEOUT_MSB,0x00)	;
	ChipSetField(IOHandle,F0372_NCO_TIMEOUT_LSB,0x00);
			
	
		
						
	/*	There is signal in the band	*/
	ChipSetField(IOHandle,F0372_RST_TIMING,1);
	ChipSetField(IOHandle,F0372_RST_CARRIER,1);
	ChipSetField(IOHandle,F0372_RSTAGC,1);
	ChipSetField(IOHandle,F0372_RSTFSM1,1);
	ChipSetField(IOHandle,F0372_EQSWRSTN,0);
	ChipSetField(IOHandle,F0372_RSTFSM2,1);
			
	WAIT_N_MS_372(5);
			
	ChipSetField(IOHandle,F0372_RSTAGC,0);	
	ChipSetField(IOHandle,F0372_RST_CARRIER,0);			
	ChipSetField(IOHandle,F0372_RST_TIMING,0);
	ChipSetField(IOHandle,F0372_RSTFSM1,0);
	ChipSetField(IOHandle,F0372_EQSWRSTN,1); 
	ChipSetField(IOHandle,F0372_RSTFSM2,0);		
			
			
	FirstWatchdog=STOS_time_now();
		
	if(CheckFSM1_372(IOHandle,pParams) == FSM1OK_372)	/*	Check for FSM1 Lock	*/ 
	   {
		
		do
		 {
		   EQState=ChipGetField(IOHandle,F0372_EQSTATE); /* Read EQSTATE */
									
			if (EnableEQreset)
			   {
				FirstTimeOutEQ=STOS_time_now();
			   }
										
			if(EQState==0)
			  {
										
				/* FULL RESET */
				ChipSetField(IOHandle,F0372_RSTAGC,1);
				ChipSetField(IOHandle,F0372_RST_TIMING,1);
				ChipSetField(IOHandle,F0372_RST_CARRIER,1);
				ChipSetField(IOHandle,F0372_RSTFSM1,1);
				ChipSetField(IOHandle,F0372_EQSWRSTN,0);
				ChipSetField(IOHandle,F0372_RSTFSM2,1);
		
				WAIT_N_MS_372(20);

				ChipSetField(IOHandle,F0372_RSTAGC,0);
				ChipSetField(IOHandle,F0372_RST_CARRIER,0); 
				ChipSetField(IOHandle,F0372_RST_TIMING,0);
				ChipSetField(IOHandle,F0372_RSTFSM1,0);
				ChipSetField(IOHandle,F0372_EQSWRSTN,1); 
				ChipSetField(IOHandle,F0372_RSTFSM2,0);		
				
				/* END OF FULL RESET */
	
				Watchdog= STOS_time_now()-FirstWatchdog;

				if ( Watchdog<=pParams->Watchdog )
				   {
					FirstTimeOutEQ=STOS_time_now();
					EnableEQreset=1;
				   }
				else
				   {
					pParams->State = NOFSM2_372;											
				   }
							
			   }
									  
									  
			   if(EQState>5)  
			     {
				ChipSetField(IOHandle,F0372_EQSWRSTN,0);  /* Reset Equalizer */
				WAIT_N_MS_372(5); 
				ChipSetField(IOHandle,F0372_EQSWRSTN,1); 
							
				ChipSetField(IOHandle,F0372_RSTFSM2,1);   
				WAIT_N_MS_372(5); 
				ChipSetField(IOHandle,F0372_RSTFSM2,0); 
									  	
				Watchdog= STOS_time_now()-FirstWatchdog;

				if ( Watchdog<=pParams->Watchdog )
				   {
					FirstTimeOutEQ=STOS_time_now();
					EnableEQreset=1;
				   }

				else
				   {
					pParams->State = NOFSM2_372;
				   }											
									  
			     }
									  
									  
			   if(EQState==5)  
			     {
				   EQState   = ChipGetField(IOHandle,F0372_EQSTATE);
				   MAINState = ChipGetField(IOHandle,F0372_MAINSTATE);
								  
				   Watchdog= STOS_time_now()-FirstWatchdog;
									  
				   if((MAINState==0x0A)&&(EQState==0x05))    
				     {
				     				
							pParams->FreqOffset=UTIL_372_Get_FrequencyOffset(IOHandle,pParams->Quartz);   
							pParams->Results.Frequency = pParams->Frequency+(pParams->FreqOffset/1000);
							pParams->Results.EQResetNB = EQResetNb;
																		/*	Read FSM2 LOCK indicator	*/
							pParams->State = FSM2OK_372; 

						    }
						  else
						    {
							pParams->State = NOFSM2_372;
						     }
									  
				           }
									  
				if((EQState>0)&&(EQState<5))  
				  {
						TimeOutEQ=STOS_time_now()- FirstTimeOutEQ;
						if(TimeOutEQ>pParams->TimeOut) 
						   {
							ChipSetField(IOHandle,F0372_EQSWRSTN,0);  /* Reset Equalizer */
							WAIT_N_MS_372(5); 
							ChipSetField(IOHandle,F0372_EQSWRSTN,1); 
							
							ChipSetField(IOHandle,F0372_RSTFSM2,1);   
							WAIT_N_MS_372(5); 
							ChipSetField(IOHandle,F0372_RSTFSM2,0); 
									  	
							Watchdog= STOS_time_now()-FirstWatchdog;

						if ( Watchdog<=pParams->Watchdog )
						    {
							 FirstTimeOutEQ=STOS_time_now();
							 EnableEQreset=1;
						    }
												
						else
						    {
							pParams->State = NOFSM2_372;											
						    }
					 }
					else
					 {
						EnableEQreset=0; 
						Watchdog= STOS_time_now()-FirstWatchdog;
					 }
									  	
					}
									  
									  
										
				}
				while( (Watchdog <= pParams->Watchdog)&&(pParams->State != FSM2OK_372)&&(pParams->State != NOFSM2_372) );
			    }	
		
	
	
	pParams->Results.SignalType = pParams->State;    
	
	return	pParams->State;
}

/*****************************************************
--FUNCTION	::	FE_372_Search
--ACTION	::	Search for a valid transponder
--PARAMS IN	::	Handle	==>	Front End Handle
pSearch ==> Search parameters
pResult ==> Result of the search
--PARAMS OUT::	NONE
--RETURN	::	Error (if any)
--***************************************************/
FE_372_Error_t	FE_372_Search(IOARCH_Handle_t IOHandle,
							  FE_372_SearchParams_t	*pSearch,
							  FE_372_SearchResult_t	*pResult,
							  STTUNER_Handle_t            TopLevelHandle)
{
	FE_372_Error_t error = FE_372_NO_ERROR;
	FE_372_InternalParams_t pParams;
	STTUNER_tuner_instance_t *TunerInstance; 
	STTUNER_InstanceDbase_t     *Inst;
	TUNER_Status_t  TunerStatus; 
	/* top level public instance data */ 
	Inst = STTUNER_GetDrvInst();
	
	/* get the tuner instance for this driver from the top level handle */
	TunerInstance = &Inst[TopLevelHandle].Terr.Tuner;
	
	
	/*This is to be put in initparams*/pParams.Quartz = 27000000; /*For 27Mhz Quartz Frequency*/
	
	FE_372_SetInternalError(FE_372_IERR_NO,FE_372_LOC_NOWHERE,&pParams.Error);
	
	
	/* Fill pParams structure with search parameters */
	pParams.BaseFreq = pSearch->Frequency;	
	pParams.SymbolRate = pSearch->SymbolRate;
	
	pParams.SearchRange = pSearch->SearchRange;
	pParams.DerotPercent = 30;
	pParams.IQMode = pSearch->IQMode;
	pParams.Watchdog = pSearch->FistWatchdog;	
	pParams.TimeOut = pSearch->DefaultWatchdog;
	pParams.LOWPowerTh = pSearch->LOWPowerTh;	
	pParams.UPPowerTh = pSearch->UPPowerTh;
	
	if (TunerInstance->Driver->ID == STTUNER_TUNER_DTT768XX)
		{
			
		ChipSetField(IOHandle,F0372_INMODE,0x00)	;
		ChipSetOneRegister(IOHandle,R0372_AGCPWR_LSB, 0x5f);
		
		}
	
				
	/* Run the search algorithm */
	error = (TunerInstance->Driver->tuner_GetStatus)(TunerInstance->DrvHandle, &TunerStatus);
	pParams.IF = TunerStatus.IntermediateFrequency ;
				
	if(FE_372_Algo(IOHandle,&pParams,TopLevelHandle) == FSM2OK_372)
	  {
		pResult->Locked = TRUE;
		/* update results */
		pResult->Frequency = pParams.Results.Frequency;			
		pResult->SymbolRate = pParams.Results.SymbolRate;										
	  }
	else
	  {
		pResult->Locked = FALSE;
					
		switch(pParams.Error.Type)
			{
			   case FE_372_IERR_I2C:				/*	I2C error	*/
				error = FE_372_I2C_ERROR;
				break;
						
			   case FE_372_IERR_NO:
				default:
			   error = FE_372_SEARCH_FAILED;
				break;
			}
	  }
				
				
				
				
  return error;
}

void FE_372_Status(FE_372_Handle_t	Handle,FE_372_SIGNALTYPE_t *SignalType)  
{
	if(Handle)
		*SignalType = ((FE_372_InternalParams_t *)Handle)->Results.SignalType;
}

/*****************************************************
--FUNCTION	::	FE_372_GetCarrierToNoiseRatio
--ACTION	::	Return the carrier to noise of the current carrier
--PARAMS IN	::	NONE	
--PARAMS OUT::	NONE
--RETURN	::	C/N of the carrier, 0 if no carrier 
--***************************************************/
S32 FE_372_GetCarrierToNoiseRatio(IOARCH_Handle_t IOHandle)
{
	S32 c_n = 0,
		regval,
		Imin,
		Imax,
		i;
	
		if(FE_372_CN_LookUp.size)
		{

			regval = MAKEWORD(ChipGetField(IOHandle,F0372_ERROR_POWER_MSB),ChipGetField(IOHandle,F0372_ERROR_POWER_LSB));
			Imin = 0;
			Imax = FE_372_CN_LookUp.size-1;
			
			if(INRANGE(FE_372_CN_LookUp.table[Imin].regval,regval,FE_372_CN_LookUp.table[Imax].regval))
			{
				while((Imax-Imin)>1)
				{
					i=(Imax+Imin)/2; 
					if(INRANGE(FE_372_CN_LookUp.table[Imin].regval,regval,FE_372_CN_LookUp.table[i].regval))
						Imax = i;
					else
						Imin = i;
				}
				
				c_n =	((regval - FE_372_CN_LookUp.table[Imin].regval)
						* (FE_372_CN_LookUp.table[Imax].realval - FE_372_CN_LookUp.table[Imin].realval)
						/ (FE_372_CN_LookUp.table[Imax].regval - FE_372_CN_LookUp.table[Imin].regval))
						+ FE_372_CN_LookUp.table[Imin].realval;
			}
			else
				c_n = 10;
		}

	
	return ( c_n);
}

/*****************************************************
--FUNCTION	::	FE_372_GetBitErrorRate
--ACTION	::	Return the bit error rate 
--PARAMS IN	::	NONE	
--PARAMS OUT::	NONE
--RETURN	::	100000000 * ber 
--***************************************************/
S32 FE_372_GetBitErrorRate(IOARCH_Handle_t IOHandle)
{
	S32 ber = 0,
		regval,
		divider,
		Imin,
		Imax,
		i;
	unsigned char low_precision=1;	
	
		if(FE_372_BER_LookUp.size)
		{

			regval = MAKEWORD(ChipGetField(IOHandle,F0372_SER_MSB),ChipGetField(IOHandle,F0372_SER_LSB));
			regval =regval*10000; 
			
			
			divider= MAKEWORD(ChipGetField(IOHandle,F0372_SER_PERIOD_MSB),ChipGetField(IOHandle,F0372_SER_PERIOD_LSB));
			if (regval < divider )/*If regval*10e4 < divider then div of regval by
			                      divider will give 0. To avoid this multiply regval by 100 */
			{
				regval = regval *100;
				low_precision=0;
			}
			else
			{
				low_precision =1;
			}
			
			if (divider!=0)
			{
			regval/=divider;
			
			}
			
			if (low_precision ==1 )/*Now multiply after division by 100 if it is not multiplied by 10e6 before*/
			{
		        regval =regval *100;
		        }
		        
			Imin = 0;
			Imax = FE_372_BER_LookUp.size-1;
			
			if(INRANGE(FE_372_BER_LookUp.table[Imin].regval,regval,FE_372_BER_LookUp.table[Imax].regval))
			{
				while((Imax-Imin)>1)
				{
					i=(Imax+Imin)/2; 
					if(INRANGE(FE_372_BER_LookUp.table[Imin].regval,regval,FE_372_BER_LookUp.table[i].regval))
						Imax = i;
					else
						Imin = i;
				}
				
				ber =	((regval - FE_372_BER_LookUp.table[Imin].regval)
						* (FE_372_BER_LookUp.table[Imax].realval - FE_372_BER_LookUp.table[Imin].realval)
						/ (FE_372_BER_LookUp.table[Imax].regval - FE_372_BER_LookUp.table[Imin].regval))
						+ FE_372_BER_LookUp.table[Imin].realval;
			}
			else
				ber = 684546;
		}

	
	return ber; /* returned value is 100000000 time the ber */
}

/*
--
--==============================================================================
--
-- END of the source code .
--
--==============================================================================
--
--   TTT  H H  EEE
--    T   HHH  EE
--    T   H H  EEE
--
--
--             EEEEEEEEEEEE        NNNNNNNN      NNNN       DDDDDDDDDDDDDD
--             EEEEEEEEEEEE        NNNNNNNN      NNNN       DDDDDDDDDDDDDDDD
--             EEEE                NNNN NNNN     NNNN       DDDD         DDDD
--             EEEE                NNNN  NNNN    NNNN       DDDD          DDDD
--             EEEEEEEE            NNNN   NNNN   NNNN       DDDD          DDDD
--             EEEEEEEE            NNNN    NNNN  NNNN       DDDD          DDDD
--             EEEE                NNNN     NNNN NNNN       DDDD         DDDD
--             EEEE                NNNN      NNNNNNNN       DDDD        DDDD
--             EEEEEEEEEEEE        NNNN       NNNNNNN       DDDDDDDDDDDDDDD
--             EEEEEEEEEEEE        NNNN        NNNNNN       DDDDDDDDDDDDDD
--
--
*/
