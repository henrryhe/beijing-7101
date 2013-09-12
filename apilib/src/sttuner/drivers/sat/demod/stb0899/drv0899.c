/* -------------------------------------------------------------------------
File Name: drv0899.c

History:
Description: STb0899 driver LLA	V1.0.8 17/01/2005

Date 18/04/05
Comments: LLA2.0.0 integration
author: SD

Date 22/06/05
Comments: LLA V3.0.3 integration
author: SD

Date 29/03/06
Comments: LLA V3.6.0 integration
author: SD

---------------------------------------------------------------------------- */


/* includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
   #include  <stdlib.h>
   #include <string.h>
#endif

	
	/*	STB0899 includes	*/
#include "drv0899.h"
#include "d0899_util.h"
#include "d0899_dvbs2util.h"
#include "dbtypes.h"
#include "sysdbase.h"
#include "d0899.h"
#include "mtuner.h"
#include "tunsdrv.h"
extern TUNSDRV_InstanceData_t *TUNSDRV_GetInstFromHandle(TUNER_Handle_t Handle);

#define MULT32X32(a,b) (((((long)((a)>>16))*((long)((b)>>16)))<<1) +((((long)((a)>>16))*((long)(b&0x0000ffff)))>>15) + ((((long)((b)>>16))*((long)((a)&0x0000ffff)))>>15))
#define angle_threshold 1042
#define LPDPC_MAX_ITER	70
static FE_STB0899_LOOKUP_t FE_STB0899_CN_LookUp =	{
								22,
								{   {0,	10780},
									{1,	10420},
									{15,	10030},
									{20,	9815},
									{30,	9320},
									{40,	8760},
									{50,	8150},
									{60,	7500},
									{70,	6850},
									{80,	6215},
									{90,	5625},
									{100,5080},
									{110,4580},
									{120,4140},
									{130,3740},
									{140,3390},
									{150,3080},
									{160,2800},
									{170,2550},
									{180,2330},
									{190,2160},
									{200,1995}
								}
							};
											
static FE_STB0899_LOOKUP_t	FE_STB0899_RF_LookUp = 	{
							20,
								{
								{-5, 79},
								{-10, 73},
								{-15, 67},
								{-20, 62},
								{-25, 56},
								{-30, 49},
								{-33, 44},
								{-35, 40},
								{-37, 36},
								{-38, 33},
								{-40, 27},
								{-45, 4},
								{-47, -11},
								{-48, -19},
								{-50, -29},
								{-55, -43},
								{-60, -52},
								{-65, -61},
								{-67, -65},
								{-70, -128},
								}
							};
							
static FE_STB0899_LOOKUP_t	FE_STB0899_DVBS2RF_LookUp = 	{
							
								15,
									{
									{-5, 2899},
									{-10, 3330},
									{-15, 3123},
									{-20, 3577},
									{-25, 4004},
									{-30, 4417},
									{-35, 4841},
									{-40, 5300},
									{-45, 5822},
									{-50, 6491},
									{-55, 7516},
									{-60, 9235},
									{-65, 11374},
									{-70, 12364},
									{-75, 13063},
									}
									};



static const ST_Revision_t RevisionSTB0899  = " STB0899-LLA_REL_3.6.0 ";
ST_Revision_t DrvSTB0899_GetLLARevision(void)
{
	return (RevisionSTB0899);
}
/*****************************************************
--FUNCTION	::	FE_STB0899_WaitTuner
--ACTION	::	Wait for tuner locked
--PARAMS IN	::	TimeOut	->	Maximum waiting time (in ms) 
--PARAMS OUT::	NONE
--RETURN	::	NONE
--***************************************************/
ST_ErrorCode_t FE_STB0899_WaitTuner(STTUNER_tuner_instance_t *TunerInstance, int TimeOut)
{
    int Time = 0;
    BOOL TunerLocked = FALSE;
    ST_ErrorCode_t Error=0;

    while(!TunerLocked && (Time < TimeOut))
    {
        WAIT_N_MS_899(1);
        Error = (TunerInstance->Driver->tuner_IsTunerLocked)(TunerInstance->DrvHandle, &TunerLocked);
        if(Error != ST_NO_ERROR)
        return Error;
        Time++;
    }
    Time--;
    
    return Error;
}

/*****************************************************
**FUNCTION	::	FE_STB0899_F22Freq
**ACTION	::	Compute F22 frequency 
**PARAMS IN	::	DigFreq	==> Frequency of the digital synthetizer (Hz)
**				F22		==> Content of the F22 register
**PARAMS OUT::	NONE
**RETURN	::	F22 frequency
*****************************************************/
U32 FE_STB0899_F22Freq(U32 DigFreq,U32 F22)
{
	return (F22!=0) ? (DigFreq/(32*F22)) : 0; 
}

/*****************************************************
--FUNCTION	::	FE_STB0899_CheckTiming
--ACTION	::	Check for timing locked
--PARAMS IN	::	Params->Ttiming	=>	Time to wait for timing loop locked
--PARAMS OUT::	Params->State		=>	result of the check
--RETURN	::	NOTIMING_899 if timing not locked, TIMINGOK otherwise
--***************************************************/
FE_STB0899_SIGNALTYPE_t FE_STB0899_CheckTiming(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_STB0899_InternalParams_t *Params)
{
	int locked,
		timing;
			
	WAIT_N_MS_899(Params->Ttiming);     
	STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_TIMING_LOOP_FREQ,FSTB0899_TIMING_LOOP_FREQ_INFO, 0xf2);
	locked=STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle,FSTB0899_TMG_LOCK_IND, FSTB0899_TMG_LOCK_IND_INFO);
	timing=abs(STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle,FSTB0899_TIMING_LOOP_FREQ, FSTB0899_TIMING_LOOP_FREQ_INFO));
	
	if(locked >= 42)
	{
		if((locked > 48) && (timing >= 110))
			Params->State = ANALOGCARRIER_899; 
		else 
			Params->State = TIMINGOK_899;   
	}
	else									  
		Params->State = NOTIMING_899; 	

	return Params->State;
}


/*****************************************************
--FUNCTION	::	FE_STB0899_CheckCarrier
--ACTION	::	Check for carrier founded
--PARAMS IN	::	Params		=>	Pointer to FE_STB0899_InternalParams_t structure
--PARAMS OUT::	Params->State	=> Result of the check
--RETURN	::	NOCARRIER carrier not founded, CARRIEROK_899 otherwise
--***************************************************/
FE_STB0899_SIGNALTYPE_t FE_STB0899_CheckCarrier(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_STB0899_InternalParams_t *Params)
{
	WAIT_N_MS_899(Params->Tderot);						/*	wait for derotator ok	*/    
	STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_CFD_ON, FSTB0899_CFD_ON_INFO,0);
	
	if (STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle, FSTB0899_CARRIER_FOUND,FSTB0899_CARRIER_FOUND_INFO))
		Params->State = CARRIEROK_899;
	else
		Params->State = NOCARRIER_899;
		
	return Params->State;
}

U32 FE_STB0899_GetErrorCount(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle ,ERRORCOUNTER Counter)
{
	U32 lsb=0,msb=0;
	
	/*Do not modified the read order (lsb first)*/
	switch(Counter)
	{
		case COUNTER1_899:
			lsb = STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle, FSTB0899_ERROR_COUNT_LSB,FSTB0899_ERROR_COUNT_LSB_INFO);
			msb = STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle, FSTB0899_ERROR_COUNT_MSB,FSTB0899_ERROR_COUNT_MSB_INFO);
		break;
	
		case COUNTER2_899:
			lsb = STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle ,FSTB0899_ERROR_COUNT2_LSB,FSTB0899_ERROR_COUNT2_LSB_INFO);
			msb = STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle ,FSTB0899_ERROR_COUNT2_MSB,FSTB0899_ERROR_COUNT2_MSB_INFO);	
		break;
		
		case COUNTER3_899:
			lsb = STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle ,FSTB0899_ERROR_COUNT3_LSB,FSTB0899_ERROR_COUNT3_LSB_INFO);
			msb = STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle ,FSTB0899_ERROR_COUNT3_MSB,FSTB0899_ERROR_COUNT3_MSB_INFO);
		break;
	}
	
	return (MAKEWORD(msb,lsb));
}


/*****************************************************
--FUNCTION	::	FE_STB0899_CheckData
--ACTION	::	Check for data founded
--PARAMS IN	::	Params		=>	Pointer to FE_STB0899_InternalParams_t structure    
--PARAMS OUT::	Params->State	=> Result of the check
--RETURN	::	NODATA data not founded, DATAOK_899 otherwise
--***************************************************/
FE_STB0899_SIGNALTYPE_t FE_STB0899_CheckData(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_STB0899_InternalParams_t *Params)
{
	int lock = 0,loopcount = 0,
	    index=0,
	    dataTime=500;
		
	Params->State = NODATA_899;   
	
	/* reset du FEC */
	STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_FRESACS,FSTB0899_FRESACS_INFO, 1);
	WAIT_N_MS_899(1);
	STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_FRESACS,FSTB0899_FRESACS_INFO, 0);  
	if(Params->SymbolRate <= 2000000)
		dataTime=2000;
	else if(Params->SymbolRate <= 5000000)
		dataTime=1500;
	else if(Params->SymbolRate <= 15000000)
		dataTime=1000;
	else
		dataTime=500;
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, RSTB0899_DSTATUS2,0x00);	/* force search loop */
	
	 index = dataTime/10; 
	 do
	 {
	 	WAIT_N_MS_899(1);
     	++loopcount;
		
	 }
	 while(	!(STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle, FSTB0899_LOCKEDVIT,FSTB0899_LOCKEDVIT_INFO)) &&	/* warning : vit locked has to be tested before end_loop */ 
			!STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle,FSTB0899_END_LOOPVIT,FSTB0899_END_LOOPVIT_INFO) && 
			loopcount<index);
			
		/* wait for viterbi end loop */
	
	lock = STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle, FSTB0899_LOCKEDVIT,FSTB0899_LOCKEDVIT_INFO);
	
	if (lock)									/*	Test DATA LOCK indicator	*/
	Params->State = DATAOK_899;
		
	return Params->State;
}

/*****************************************************
--FUNCTION	::	FE_STB0899_TimingTimeConstant
--ACTION	::	Compute the amount of time needed by the timing loop to lock
--PARAMS IN	::	SymbolRate	->	symbol rate value
--PARAMS OUT::	NONE
--RETURN	::	Timing loop time constant (ms)
--***************************************************/
long FE_STB0899_TimingTimeConstant(long SymbolRate)
{
	if(SymbolRate > 0)
		return (100000/(SymbolRate/1000));
	else
		return 0;
}

/*****************************************************
--FUNCTION	::	FE_STB0899_DerotTimeConstant
--ACTION	::	Compute the amount of time needed by the Derotator to lock
--PARAMS IN	::	SymbolRate	->	symbol rate value
--PARAMS OUT::	NONE
--RETURN	::	Derotator time constant (ms)
--***************************************************/
long FE_STB0899_DerotTimeConstant(long SymbolRate)
{
	if(SymbolRate > 0)  
		return (100000/(SymbolRate/1000)); 
	else
		return 0;
}

/*****************************************************
--FUNCTION	::	FE_STB0899_DataTimeConstant
--ACTION	::	Compute the amount of time needed to capture data 
--PARAMS IN	::	Er		->	Viterbi rror rate	
--				Sn		->  viterbi averaging period
--				To		->  viterbi time out
--				Hy		->	viterbi hysteresis
--				SymbolRate	->	symbol rate value
--PARAMS OUT::	NONE
--RETURN	::	Data time constant
--***************************************************/
long FE_STB0899_DataTimeConstant(int Er, int Sn,int To,int Hy,long SymbolRate)
{
	long	Tviterbi = 0,
			TimeOut	=0,
			THysteresis	= 0,
			PhaseNumber[6] = {2,6,4,6,14,8},
		 	averaging[4] = {1024L,4096L,16384L,65536L},
		 	InnerCode = 1000,
		 	HigherRate = 1000;
		 	
	int 	i;
		 	
	/*	=======================================================================
	-- Data capture time (in ms)
    -- -------------------------
	-- This time is due to the Viterbi synchronisation.
	--
	--	For each authorized inner code, the Viterbi search time is calculated,
	--	and the results are cumulated in ViterbiSearch.	*/
	for(i=0;i<6;i++)
	{
		if (((Er >> i)& 0x01) == 0x01)
		{
			switch(i)
			{
				case 0:					/*	inner code 1/2	*/	
					InnerCode = 2000;	/* 2.0 */
				break;
				
				case 1:					/*	inner code 2/3	*/ 
					InnerCode = 1500; 	/* 1.5 */
				break;
				
				case 2:					/*	inner code 3/4  */ 
					InnerCode = 1333;	/* 1.333 */
				break;
				
				case 3:					/*	inner code 5/6	*/  
					InnerCode = 1200;	/* 1.2 */
				break;
				
				case 4:					/*	inner code 6/7	*/  
					InnerCode = 1167;	/* 1.667 */
				break;
				
				case 5:					/*	inner code 7/8	*/
					InnerCode = 1143;	/* 1.143 */
				break;
			}
			
			Tviterbi +=(int)((PhaseNumber[i]*averaging[Sn]*InnerCode)/SymbolRate);
			
			if(HigherRate < InnerCode) 
				HigherRate = InnerCode;
		}
	}
	
	/*	  time out calculation (TimeOut)
	--    ------------------------------
	--    This value indicates the maximum duration of the synchro word research.	*/
	TimeOut   = (long)((HigherRate * 16384L * (To + 1L))/(2*SymbolRate));
	
	/*    Hysteresis duration (Hysteresis)
	--    ------------------------------	*/
	THysteresis = (long)((HigherRate * 26112L * (Hy +1L))/(2*SymbolRate));	/*	26112= 16*204*8 bits  */ 
	
	/* a guard time of 1 mS is added */
	return (1 + Tviterbi + TimeOut + THysteresis);
}

/****************************************************
**FUNCTION	::	FE_STB0899_GetAlpha
**ACTION	::	Read the rolloff value
**PARAMS IN	::	hChip	==>	Handle for the chip
**PARAMS OUT::	NONE
**RETURN	::	rolloff
*****************************************************/
int  FE_STB0899_GetAlpha(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	if (STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle, FSTB0899_MODE_COEF,FSTB0899_MODE_COEF_INFO) == 1)
		return 20;
	else
		return 35;

}
/*****************************************************
**FUNCTION	::	FE_STB0899_CalcDerotFreq
**ACTION	::	Compute Derotator frequency 
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE
**RETURN	::	Derotator frequency (KHz)
*****************************************************/
S32 FE_STB0899_CalcDerotFreq(U8 derotmsb,U8 derotlsb,U32 fm)
{
	S32	dfreq;
	S32 Itmp;
		
	Itmp = (S16)(derotmsb<<8)+derotlsb;
	dfreq =(S32)(Itmp*( fm /10000L));
	dfreq =(S32)(dfreq/65536L);
	dfreq *= 10;
	
	return dfreq; 
}
/*****************************************************
**FUNCTION	::	FE_STB0899_GetDerotFreq
**ACTION	::	Read current Derotator frequency 
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE
**RETURN	::	Derotator frequency (KHz)
*****************************************************/
S32 FE_STB0899_GetDerotFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 MasterClock)
{
	U32 CarrierFreq[2];
	/*	Read registers	*/
	STTUNER_IOREG_GetContigousRegisters_SizeU32(DeviceMap,IOHandle,RSTB0899_CFRM,2,CarrierFreq); 
	CarrierFreq[0]&=0xff;
	CarrierFreq[1]&=0xff;
	
	return FE_STB0899_CalcDerotFreq(CarrierFreq[0],CarrierFreq[1],MasterClock);

}

/*****************************************************
**FUNCTION	::	FE_899_BinaryFloatDiv
**ACTION	::	float division (with integer) 
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE
**RETURN	::	Derotator frequency (KHz)
*****************************************************/
long FE_899_BinaryFloatDiv(long n1, long n2, int precision)
{
	int i=0;
	long result=0;
	
	/*	division de N1 par N2 avec N1<N2	*/
	while(i<=precision) /*	n1>0	*/
	{
		if(n1<n2)
		{
			result*=2;      
			n1*=2;
		}
		else
		{
			result=result*2+1;
			n1=(n1-n2)*2;
		}
		i++;
	}
	
	return result;
}



/*****************************************************
**FUNCTION	::	FE_STB0899_CalcSymbolRate
**ACTION	::	Compute symbol frequency
**PARAMS IN	::	Hbyte	->	High order byte
**				Mbyte	->	Mid byte
**				Lbyte	->	Low order byte
**PARAMS OUT::	NONE
**RETURN	::	Symbol frequency
*****************************************************/
U32 FE_STB0899_CalcSymbolRate(U32 MasterClock,U8 Hbyte,U8 Mbyte,U8 Lbyte)
{
	U32 Ltmp,
		Ltmp2,
		Mclk;

	Mclk =  (U32) (MasterClock / 4096L);	/* MasterClock*10/2^20 */
	Ltmp = (((U32)Hbyte<<12)+((U32)Mbyte<<4))/16;
	Ltmp *= Mclk;
	Ltmp /=16;
	Ltmp2=((U32)Lbyte*Mclk)/256;     
	Ltmp+=Ltmp2;
	
	return Ltmp;
}

/*****************************************************
**FUNCTION	::	FE_STB0899_SetSymbolRate
**ACTION	::	Set symbol frequency
**PARAMS IN	::	hChip		->	handle to the chip
**				MasterClock	->	Masterclock frequency (Hz)
**				SymbolRate	->	symbol rate (bauds)
**PARAMS OUT::	NONE
**RETURN	::	Symbol frequency
*****************************************************/
U32 FE_STB0899_SetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 MasterClock,U32 SymbolRate)
{
	U32 U32Tmp,
	    U32TmpUp,
	    SymbolRateUp=SymbolRate;
	U32  TMP[3];
	U32   TMPUP[3];
	
	/*
	** in order to have the maximum precision, the symbol rate entered into
	** the chip is computed as the closest value of the "true value".
	** In this purpose, the symbol rate value is rounded (1 is added on the bit
	** below the LSB )
	*/
	
	SymbolRateUp+=((SymbolRateUp*3)/100);
	
	U32Tmp = (U32)(FE_899_BinaryFloatDiv(SymbolRate,MasterClock,20));       
	U32TmpUp=(U32)(FE_899_BinaryFloatDiv(SymbolRateUp,MasterClock,20));  
	
	
	
	/*STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, RSTB0899_TMGCFG,0x40);*//* to activate auto search for timing offset*/
	
	TMP[0] = (U32TmpUp>>12)&0xFF;
	TMP[1] = (U32TmpUp>>4)&0xFF;
	TMP[2] = (U32TmpUp&0x0F);
	
	
	TMPUP[0] = (U32Tmp>>12)&0xFF; 
	TMPUP[1] = (U32Tmp>>4)&0xFF;
	TMPUP[2] = (U32Tmp&0x0F);
	
	STTUNER_IOREG_SetContigousRegisters_SizeU32(DeviceMap, IOHandle,RSTB0899_SFRUPH,TMP,3);
	STTUNER_IOREG_SetContigousRegisters_SizeU32(DeviceMap, IOHandle,RSTB0899_SFRH,TMPUP,3);
		
	
	return(SymbolRate) ;
}

/*****************************************************
**FUNCTION	::	FE_STB0899_GetSymbolRate
**ACTION	::	Get the current symbol rate
**PARAMS IN	::	hChip		->	handle to the chip
**				MasterClock	->	Masterclock frequency (Hz)
**PARAMS OUT::	NONE
**RETURN	::	Symbol rate
*****************************************************/
U32 FE_STB0899_GetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 MasterClock)
{
	U32 sfrh[3];
	
	STTUNER_IOREG_GetContigousRegisters_SizeU32(DeviceMap, IOHandle, RSTB0899_SFRH,3,sfrh);
	sfrh[0]&=0xff;
	sfrh[1]&=0xff;
	sfrh[2]&=0xff;
	return FE_STB0899_CalcSymbolRate(	MasterClock,sfrh[0],
						sfrh[1],
						sfrh[2]);

}

/*****************************************************
--FUNCTION	::	FE_899_CarrierWidth
--ACTION	::	Compute the width of the carrier
--PARAMS IN	::	SymbolRate	->	Symbol rate of the carrier (Kbauds or Mbauds)
--				RollOff		->	Rolloff * 100
--PARAMS OUT::	NONE
--RETURN	::	Width of the carrier (KHz or MHz) 
--***************************************************/
long FE_899_CarrierWidth(long SymbolRate, long RollOff)
{
	return (SymbolRate  + (SymbolRate*RollOff)/100);
}
/*****************************************************
--FUNCTION	::	FE_STB0899_InitialCalculations
--ACTION	::	Set Params fields that are never changed during search algorithm   
--PARAMS IN	::	NONE
--PARAMS OUT::	NONE
--RETURN	::	NONE
--***************************************************/
void FE_STB0899_InitialCalculations(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_STB0899_InternalParams_t *Params)
{
	int  MasterClock;
	
	
	/*	Initial calculations	*/   
	MasterClock = FE_STB0899_GetMclkFreq(DeviceMap, IOHandle,Params->Quartz);
	Params->Tagc1 = 0;
	Params->Tagc2 = 0;
	Params->MasterClock = MasterClock;
	Params->Mclk = (U32)(MasterClock/65536L);
	Params->RollOff = FE_STB0899_GetAlpha(DeviceMap, IOHandle);
	
	/*DVBS2 Initial Calculation from Validation Code*/
	/*Set AGC init value to to the midle*/ 
	Params->AgcGain = 8154;
	
	STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_IF_GAININIT,FSTB0899_IF_GAININIT_INFO,Params->AgcGain);


	Params->RrcAlpha = STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle,FSTB0899_RRC_ALPHA,FSTB0899_RRC_ALPHA_INFO);

	Params->CenterFreq=0;	
	Params->AveFrameCoarse=10;
	Params->AveFramefine=20;	
	Params->StepSize=2;
	
	if((Params->SpectralInv==STTUNER_IQ_MODE_NORMAL)||(Params->SpectralInv==STTUNER_IQ_MODE_AUTO))
		Params->IQLocked=0;
	else
		Params->IQLocked=1;

}


/*****************************************************
--FUNCTION	::	FE_STB0899_SearchTiming
--ACTION	::	Perform an Fs/2 zig zag to found timing
--PARAMS IN	::	NONE
--PARAMS OUT::	NONE
--RETURN	::	NOTIMING if no valid timing had been found, TIMINGOK otherwise
--***************************************************/
FE_STB0899_SIGNALTYPE_t FE_STB0899_SearchTiming(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_STB0899_InternalParams_t *Params)
{
	short int	DerotStep,
				DerotFreq = 0,
				DerotLimit,
				NextLoop = 3;
	int 	index = 0;
	U32   cfrm[2];
			
	Params->State = NOTIMING_899;
		
	/* timing loop computation & symbol rate optimisation	*/
	DerotLimit = (Params->SubRange/2)/Params->Mclk;
	DerotStep = (Params->SymbolRate/2)/Params->Mclk;
	
	while((FE_STB0899_CheckTiming(DeviceMap, IOHandle, Params)!=TIMINGOK_899) && NextLoop)
	{
		
		index++;
		DerotFreq += (short int)Params->TunerIQSense*index*(Params->Direction)*DerotStep;	/*	Compute the next derotator position for the zig zag	*/    
		
		if(ABS(DerotFreq) > DerotLimit)
			NextLoop--;
		
		if(NextLoop)
		{
			cfrm[0] = MSB((short int)DerotFreq);
			cfrm[1] = LSB((short int)DerotFreq);
			
			STTUNER_IOREG_SetContigousRegisters_SizeU32(DeviceMap, IOHandle,RSTB0899_CFRM,cfrm,2); 							/*	Set the derotator frequency	*/			
		
		}
		
		Params->Direction = -Params->Direction;			/*	Change the zigzag direction	*/    
	}
	
	
	if(Params->State == TIMINGOK_899)
	{
		Params->Results.SymbolRate = Params->SymbolRate; 
		
		STTUNER_IOREG_GetContigousRegisters_SizeU32(DeviceMap, IOHandle,RSTB0899_CFRM,2,cfrm);								/*	Get the derotator frequency	*/ 
		cfrm[0]&=0xff;
		cfrm[1]&=0xff;
		Params->DerotFreq =(short int)Params->TunerIQSense*((short int) MAKEWORD(cfrm[0],cfrm[1]));
	
	}
	
	return Params->State;
}

/*****************************************************
--FUNCTION	::	FE_STB0899_SearchCarrier
--ACTION	::	Search a QPSK carrier with the derotator
--PARAMS IN	::	
--PARAMS OUT::	NONE
--RETURN	::	NOCARRIER_899 if no carrier had been found, CARRIEROK_899 otherwise 
--***************************************************/
FE_STB0899_SIGNALTYPE_t FE_STB0899_SearchCarrier(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_STB0899_InternalParams_t *Params)
{
	short int	DerotFreq = 0,
				LastDerotFreq = 0, 
				DerotLimit,
				NextLoop = 3;
	int 	index = 0;
	U32   cfrm[2];
			
	Params->State = NOCARRIER_899;
	
	DerotLimit = (Params->SubRange/2)/Params->Mclk;
	DerotFreq = Params->DerotFreq;
	
	STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_CFD_ON,FSTB0899_CFD_ON_INFO, 1);
	
	do
	{		
		if(FE_STB0899_CheckCarrier(DeviceMap, IOHandle, Params)==NOCARRIER_899)
		{
			index++;
			LastDerotFreq = DerotFreq;
			DerotFreq += (short int)Params->TunerIQSense * index * Params->Direction * Params->DerotStep;	/*	Compute the next derotator position for the zig zag	*/    
		
			if(ABS(DerotFreq) > DerotLimit)
				NextLoop--;
				
			if(NextLoop)
			{
					
				STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_CFD_ON,FSTB0899_CFD_ON_INFO,1);
				cfrm[0] = MSB((short int)DerotFreq);
				cfrm[1] = LSB((short int)DerotFreq);
				
				STTUNER_IOREG_SetContigousRegisters_SizeU32(DeviceMap, IOHandle,RSTB0899_CFRM,cfrm,2); 							/*	Set the derotator frequency	*/
		

			}
		}
		else
		{
			Params->Results.SymbolRate = Params->SymbolRate;
		}
		
		Params->Direction = -Params->Direction;			/*	Change the zigzag direction	*/    
	}
	while(	(Params->State!=CARRIEROK_899) && NextLoop);
	
	
	if(Params->State == CARRIEROK_899)
	{
		
		STTUNER_IOREG_GetContigousRegisters_SizeU32(DeviceMap, IOHandle,RSTB0899_CFRM,2, cfrm); 								/*	Get the derotator frequency	*/ 
		cfrm[0]&=0xff;
		cfrm[1]&=0xff;
		Params->DerotFreq =(short int)Params->TunerIQSense*((short int) MAKEWORD(cfrm[0],cfrm[1]));

	}
	else
	{
		Params->DerotFreq = LastDerotFreq;	
	}
	
	
	return Params->State;
}

/*****************************************************
--FUNCTION	::	FE_STB0899_SearchData
--ACTION	::	Search a QPSK carrier with the derotator, even if there is a false lock 
--PARAMS IN	::	
--PARAMS OUT::	NONE
--RETURN	::	NOCARRIER_899 if no carrier had been found, CARRIEROK_899 otherwise 
--***************************************************/
FE_STB0899_SIGNALTYPE_t FE_STB0899_SearchData(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_STB0899_InternalParams_t *Params)
{
	short int	DerotFreq,
				DerotStep,
				DerotLimit,
				NextLoop = 3;
	int 	index = 1;
	U32   cfrm[2];
	
	DerotStep = (Params->SymbolRate/4)/Params->Mclk; 
	DerotLimit = (Params->SubRange/2)/Params->Mclk;      
	DerotFreq = Params->DerotFreq;
		
	do
	{
		if((Params->State != CARRIEROK_899) || (FE_STB0899_CheckData(DeviceMap, IOHandle, Params)!= DATAOK_899))
		{
			DerotFreq += (short int)Params->TunerIQSense*index*Params->Direction*DerotStep;		/*	Compute the next derotator position for the zig zag	*/ 
		
			if(ABS(DerotFreq) > DerotLimit)
				NextLoop--;
			
			if(NextLoop)
			{
				
				STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_CFD_ON,FSTB0899_CFD_ON_INFO,1);
			
				cfrm[0] = MSB((short int)DerotFreq);
				cfrm[1] = LSB((short int)DerotFreq);
				STTUNER_IOREG_SetContigousRegisters_SizeU32(DeviceMap, IOHandle, RSTB0899_CFRM,cfrm,2); 							/*	Reset the derotator frequency */	
				FE_STB0899_CheckCarrier(DeviceMap, IOHandle, Params);
			
				
			
				index++;
			}
		}
		
		Params->Direction = -Params->Direction;			/*	Change the zigzag direction	*/  
	}
	while((Params->State != DATAOK_899) && NextLoop);
	
	if(Params->State == DATAOK_899)
	{
		
		STTUNER_IOREG_GetContigousRegisters_SizeU32(DeviceMap, IOHandle,RSTB0899_CFRM,2, cfrm); 								/*	Get the derotator frequency	 */
		cfrm[0]&=0xff;
		cfrm[1]&=0xff;
		Params->DerotFreq = (short int)Params->TunerIQSense*((short int) MAKEWORD(cfrm[0],cfrm[1]));

	}	
	
	return Params->State;
}



/****************************************************
--FUNCTION	::	FE_STB0899_CheckRange
--ACTION	::	Check if the founded frequency is in the correct range
--PARAMS IN	::	Params->BaseFreq =>	
--PARAMS OUT::	Params->State	=>	Result of the check
--RETURN	::	RANGEOK_899 if check success, OUTOFRANGE_899 otherwise 
--***************************************************/
FE_STB0899_SIGNALTYPE_t FE_STB0899_CheckRange(FE_STB0899_InternalParams_t *Params) 
{
	int	RangeOffset,
		TransponderFrequency;
		
	RangeOffset = Params->SearchRange/2000;  
	TransponderFrequency = Params->Frequency + (S32)((Params->DerotFreq * ((S16)(Params->Mclk)))/1000);
	
	if((TransponderFrequency >= Params->BaseFreq - RangeOffset)
	&& (TransponderFrequency <= Params->BaseFreq + RangeOffset))
		Params->State = RANGEOK_899;
	else
		Params->State = OUTOFRANGE_899;
		
	return Params->State;
}

/****************************************************
--FUNCTION	::	FE_899_CarrierNotCentered
--ACTION	::	Check if the carrier is correctly centered
--PARAMS IN	::		
--PARAMS OUT::	
--RETURN	::	1 if not centered, 0 otherwise
--***************************************************/
int FE_899_CarrierNotCentered(FE_STB0899_InternalParams_t *Params,int AllowedOffset)
{
	int NotCentered = 0,
		DerotFreq;
	long Fs;
	
	Fs = FE_899_CarrierWidth(Params->SymbolRate,Params->RollOff);
	DerotFreq = abs(Params->DerotFreq * Params->Mclk); 
	
	if(Fs < 4000000)
		NotCentered = (int)(Params->TunerBW - Fs)/4 ;
	else
		NotCentered = ((Params->TunerBW/2 - (U32)DerotFreq - (U32)(Fs/2)) < AllowedOffset) && (DerotFreq > Params->TunerStep);
	
	return NotCentered;
}


/****************************************************
--FUNCTION	::	FE_899_FirstSubRange
--ACTION	::	Compute the first SubRange of the search 
--PARAMS IN	::	Params->SearchRange
--PARAMS OUT::	Params->SubRange
--RETURN	::	NONE
--***************************************************/
void FE_899_FirstSubRange(STTUNER_tuner_instance_t *TunerInstance, FE_STB0899_InternalParams_t *Params)
{
	int maxsubrange;
	
	maxsubrange = (int)((Params->TunerBW)*1000 - (U32)FE_899_CarrierWidth(Params->SymbolRate,Params->RollOff)/2);
	
	if(maxsubrange > 0)
		Params->SubRange = MIN(Params->SearchRange,maxsubrange);
	else
		Params->SubRange = 0;
	Params->Frequency = Params->BaseFreq;
	Params->TunerOffset = 0;
	
	Params->SubDir = 1;

}

  
/****************************************************
--FUNCTION	::	FE_899_NextSubRange
--ACTION	::	Compute the next SubRange of the search 
--PARAMS IN	::	Frequency	->	Start frequency
--				Params->SearchRange
--PARAMS OUT::	Params->SubRange
--RETURN	::	NONE
--***************************************************/
void FE_899_NextSubRange(FE_STB0899_InternalParams_t *Params)
{
	S32 OldSubRange;
	
	if(Params->SubDir > 0)
	{
		OldSubRange = Params->SubRange;
		Params->SubRange = MIN((S32)(Params->SearchRange/2) - (Params->TunerOffset + Params->SubRange/2),Params->SubRange);
		if(Params->SubRange < 0)
			Params->SubRange = 0;	
		Params->TunerOffset += (OldSubRange + Params->SubRange)/2;
	}
	
	Params->Frequency = Params->BaseFreq + (Params->SubDir * Params->TunerOffset)/1000;    
	Params->SubDir = -Params->SubDir;

}

/*****************************************************
--FUNCTION	::	FE_STB0899_Algo
--ACTION	::	Search for Signal, Timing, Carrier and then data at a given Frequency, 
--				in a given range
--PARAMS IN	::	NONE
--PARAMS OUT::	NONE
--RETURN	::	Type of the founded signal (if any)
--***************************************************/
FE_STB0899_SIGNALTYPE_t FE_STB0899_Algo(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_STB0899_InternalParams_t *Params, STTUNER_Handle_t TopLevelHandle)
{
	/*S32	pr,sn,to,hy;*/
		/*beta value for 99MHz*/
    S32	betaTab[5][4]=	{   /*5MBs*/	/*10MBs*/   /*20MBs*/   /*30Mbs*/
			/*QPSK 1/2 */		{37,			34,			32,			31},
			/*QPSK 2/3 */		{37,			35,			33,			31},
			/*QPSK 3/4 */		{37,			35,			33,			31},
			/*QPSK 5/6 */		{37,			36,			33,			32},
			/*QPSK 7/8 */		{37,			36,			33,			32}
						};
    S32 clnI=3;	
    U32   cfrm[2]={0,0},EqCoeffs[10];	
    STTUNER_InstanceDbase_t *Inst;
    STTUNER_tuner_instance_t *TunerInstance;   
    ST_ErrorCode_t Error;
    U32 TransponderFreq;
    /* top level public instance data */ 
    Inst = STTUNER_GetDrvInst();

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[TopLevelHandle].Sat.Tuner;


	Params->Frequency = Params->BaseFreq;
	Params->Direction = 1;
	
	FE_STB0899_SetSymbolRate(DeviceMap, IOHandle, Params->MasterClock,Params->SymbolRate);	/*	Set the symbol rate	*/
	/* Carrier loop optimization versus symbol rate */
	
	if(Params->SymbolRate <= 5000000)
	{
		STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, RSTB0899_ACLC,0x89);
		STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_BETA,FSTB0899_BETA_INFO,0x1c);
		clnI=0;
	}
	else if(Params->SymbolRate <= 15000000)
	{
		STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, RSTB0899_ACLC,0xc9);
		STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_BETA,FSTB0899_BETA_INFO,0x22);
		clnI=1;
	}
	else if(Params->SymbolRate <= 125000000)
	{
		STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, RSTB0899_ACLC,0x89);
		STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_BETA,FSTB0899_BETA_INFO,0x27);
		clnI=2;
	}
	else
	{
		STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, RSTB0899_ACLC,0xc8);
		STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_BETA,FSTB0899_BETA_INFO,0x29);
		clnI=3;
	}
	/*Set the timing loop to acquisition  */
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_RTC,0x46);
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_CFD,0xee);

			
	/*	Initial calculations	*/
	Params->DerotStep = Params->DerotPercent*(Params->SymbolRate/1000)/Params->Mclk;	/*	step of DerotStep/1000 * Fsymbol	*/     
	Params->Ttiming = (S16)FE_STB0899_TimingTimeConstant(Params->SymbolRate);
	Params->Tderot = (S16)FE_STB0899_DerotTimeConstant(Params->SymbolRate);
	Params->Tdata = 500; /*2 + FE_STB0899_DataTimeConstant(pr,sn,to,hy,Params->SymbolRate);*/
	STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_FRESRS, FSTB0899_FRESRS_INFO,1);  /* Reset Stream Merger*/
	/*FE_899_FirstSubRange(TunerInstance, Params);*/
	Params->SubRange = 10000000;
	Params->Frequency = Params->BaseFreq;
	Params->TunerOffset = 0;
	Params->SubDir = 1;
	
	do
	{ 
		/*	Initialisations	*/
		/*	Initialisations	*/
		STTUNER_IOREG_SetContigousRegisters_SizeU32(DeviceMap, IOHandle,RSTB0899_CFRM,cfrm,2);		/*	Reset of the derotator frequency */	
		STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_TIMING_LOOP_FREQ,FSTB0899_TIMING_LOOP_FREQ_INFO,0xf2);
		STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_CFD_ON,FSTB0899_CFD_ON_INFO,1);
		
		Params->DerotFreq = 0;
		Params->State = NOAGC1_899;
		TransponderFreq = Params->Frequency;
		Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, (U32)Params->Frequency, (U32 *)&Params->Frequency);
		
		if(Error != ST_NO_ERROR)
		{
			return(NOAGC1_899);
		}
		/*	Temporisations	*/
		WAIT_N_MS_899(10);	/*	Wait for agc1,agc2 and timing loop	*/
		tuner_tunsdrv_Utilfunction(TunerInstance->DrvHandle, VCO_SEARCH_OFF);
		
		Params->State = AGC1OK_899;	/* No AGC test actually */  
		/*	There is signal in the band	*/
		if(Params->SymbolRate <= (Params->TunerBW)/2)	   
			FE_STB0899_SearchTiming(DeviceMap, IOHandle,Params);			/*	For low rates (SCPC)	*/
			
		else
			FE_STB0899_CheckTiming(DeviceMap, IOHandle,Params);			/*	For high rates (MCPC)	*/ 
		
		if(Params->State == TIMINGOK_899)
		{	
			if(FE_STB0899_SearchCarrier(DeviceMap, IOHandle, Params) == CARRIEROK_899)	/*	Search for carrier	*/  			
			{
				
				if(FE_STB0899_SearchData(DeviceMap, IOHandle, Params) == DATAOK_899)	/*	Check for data	*/ 
				{
					
					if(FE_STB0899_CheckRange(Params) == RANGEOK_899)
					{
						
						Params->Results.Frequency = Params->Frequency + (Params->DerotFreq*((S16)(Params->Mclk)))/1000; 
						Params->Results.PunctureRate = STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle,FSTB0899_VIT_CURPUN, FSTB0899_VIT_CURPUN_INFO); 
					}
				}
			}
		}
		
		if(Params->State != RANGEOK_899) 
			FE_899_NextSubRange(Params);
		
	}	  
	while(Params->SubRange && Params->State!=RANGEOK_899);
	
	Params->Results.SignalType = Params->State;
	/*if locked and range is ok set Kdiv value*/
	if(Params->State == RANGEOK_899)
	{
		/*Set the timing loop to tracking  */
		STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_RTC,0x23);
		STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_CFD,0xF7);
		switch(Params->Results.PunctureRate)
		{
			case 13:		/*1/2*/
				STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_KDIVIDER,FSTB0899_KDIVIDER_INFO,0x1a);
				STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_BETA,FSTB0899_BETA_INFO,betaTab[0][clnI]);
			break;
			
			case 18:		/*2/3*/
				STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_KDIVIDER,FSTB0899_KDIVIDER_INFO,44/*0x27*/);
				STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_BETA,FSTB0899_BETA_INFO,betaTab[1][clnI]);
			break;
			
			case 21:		/*3/4*/
				STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_KDIVIDER,FSTB0899_KDIVIDER_INFO,/*0x34*/60);
				STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_BETA,FSTB0899_BETA_INFO,betaTab[2][clnI]);
			break;
			
			case 24:		/*5/6*/
				STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_KDIVIDER,FSTB0899_KDIVIDER_INFO,75/*0x4f*/);
				STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_BETA,FSTB0899_BETA_INFO,betaTab[3][clnI]);
			break;
			
			case 25:		/*6/7*/	
				STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_KDIVIDER,FSTB0899_KDIVIDER_INFO,/*0x5c*/80);
				STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_BETA,FSTB0899_BETA_INFO,betaTab[3][clnI]);
			break;
			
			case 26:		/*7/8*/	
				STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_KDIVIDER,FSTB0899_KDIVIDER_INFO,94/*0x6a*/);
				STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_BETA,FSTB0899_BETA_INFO,betaTab[4][clnI]);
			break;
			
		}
		
		STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_FRESRS,FSTB0899_FRESRS_INFO,0); /*release Stream merger reset*/ 
		STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_CFD_ON,FSTB0899_CFD_ON_INFO,0); /*Disable Carrier detector*/ 
	
		STTUNER_IOREG_GetContigousRegisters_SizeU32(DeviceMap, IOHandle, RSTB0899_EQUAI1,10,EqCoeffs); 

		
	}
	
	return	Params->State;
}

/*****************************************************
--FUNCTION	::	FE_STB0899_SetMclk
--ACTION	::	Set demod Master Clock  
--PARAMS IN	::	Handle	==>	Front End Handle
			::	Mclk : demod master clock
			::	ExtClk external Quartz
--PARAMS OUT::	NONE.
--RETURN	::	Error (if any)
--***************************************************/
FE_STB0899_Error_t FE_STB0899_SetMclk(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 Mclk, FE_STB0899_InternalParams_t	*Params)
{
	FE_STB0899_Error_t error = FE_899_NO_ERROR;
	
	U32 mDiv;
	if(Params == NULL)
		error=FE_899_INVALID_HANDLE;
	else
	{
		mDiv = ((6*Mclk)/Params->Quartz)-1;
		STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_MDIV, FSTB0899_MDIV_INFO, mDiv);
		Params->MasterClock=FE_STB0899_GetMclkFreq(DeviceMap, IOHandle, Params->Quartz);
	}

	return(FE_899_NO_ERROR);
}

/*****************************************************
--FUNCTION	::	FE_STB0899_DVBS2Algo
--ACTION	::	Locking Algo for DVBS2 Signal
--PARAMS IN	::	Params		=>	Pointer to FE_STB0899_InternalParams_t structure
--PARAMS OUT::	Params->State	=> Result of the check
--RETURN	::	
--***************************************************/
FE_DVBS2_State  FE_STB0899_DVBS2Algo(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_STB0899_InternalParams_t *Params, STTUNER_Handle_t TopLevelHandle)
{
	
    S32 offsetfreq, pilots, searchTime, fecLockTime;
    S16 iqSpectrum;		
    FE_DVBS2_ModCod_t modCode;
    FE_STB0899_DVBS2_InitParams_t initParams;
    STTUNER_InstanceDbase_t *Inst;
    STTUNER_tuner_instance_t *TunerInstance;
    ST_ErrorCode_t Error = ST_NO_ERROR;
      
    /* top level public instance data */ 
    Inst = STTUNER_GetDrvInst();
    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[TopLevelHandle].Sat.Tuner; 
	
	/*Init Params Initialization*/
	initParams.RRCAlpha=Params->RrcAlpha;
	initParams.SymbolRate=Params->DVBS2SymbolRate;
	initParams.MasterClock=Params->MasterClock;
	initParams.CarrierFrequency=Params->CenterFreq;/*carrier freq = 0?*/
	initParams.AveFrameCoarse=Params->AveFrameCoarse;
	initParams.AveFramefine=Params->AveFramefine;
	initParams.StepSize=Params->StepSize;
	/*Check for (1)Demod lock then (2)FEC lock  for different symbol rates */			
	if(Params->DVBS2SymbolRate <= 2000000)
	{
		searchTime=5000;					/*5000 ms time to lock UWP, CSM and FEC when SYMB <= 2Mbs */
		fecLockTime=350;					/*350 ms to lock the FEC only once the demod locked*/
	}
	else if(Params->DVBS2SymbolRate <= 5000000)
	{
		searchTime=2500;						/*2500 ms time to lock UWP, CSM and FEC when 2Mbs< SYMB <= 5Mbs */ 
		fecLockTime=170;					/*170 ms to lock the FEC only once the demod locked*/
	}
	else if(Params->DVBS2SymbolRate <= 10000000)
	{
		searchTime=1500;					 	/*1500 ms time to lock UWP, CSM and FEC when 5Mbs <SYMB <= 10Mbs */
		fecLockTime=100;					/*100 ms to lock the FEC only once the demod locked*/
	}
	else if  (Params->DVBS2SymbolRate <= 15000000)
	{
		searchTime=500;					 	/*700 ms time to lock UWP, CSM and FEC when 10Mbs <SYMB <= 15Mbs */
		fecLockTime=70;						/*70 ms to lock the FEC only once the demod locked*/
	}
	else if  (Params->DVBS2SymbolRate <= 20000000)
	{
		searchTime=300;					 	/*500 ms time to lock UWP, CSM and FEC when 15Mbs < SYMB <= 20Mbs */
		fecLockTime=50;						/*50 ms to lock the FEC only once the demod locked*/
	}
	else if  (Params->DVBS2SymbolRate <= 25000000)
	{
		searchTime=250;					 	/*250 ms time to lock UWP, CSM and FEC when 20 Mbs < SYMB <= 25Mbs */
		fecLockTime=25;						/*30 ms to lock the FEC only once the demod locked*/
	}
	else
	{
		searchTime=150;						/*150 ms time to lock UWP, CSM and FEC when  SYMB > 25Mbs */ 	
		fecLockTime=20;						/*20 ms to lock the FEC only once the demod locked*/
	}					
		
	Params->SubRange = 12000000;
	initParams.FreqRange = Params->SubRange/1000000;
	Params->Frequency = Params->BaseFreq;
	Params->TunerOffset = 0;
	Params->SubDir = 1;
	
	/*Maintain Stream Merger in reset during acquisition*/
	STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_FRESRS,FSTB0899_FRESRS_INFO,1);
	
	FE_DVBS2_InitialCalculations(DeviceMap,IOHandle,&initParams);
	
	/*IQ swap setting*/
	if(Params->SpectralInv == STTUNER_IQ_MODE_NORMAL)
	{
		/* I,Q Spectrum Set to Normal*/ 
		STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_SPECTRUM_INVERT,FSTB0899_SPECTRUM_INVERT_INFO,0);
		
	}
	else if(Params->SpectralInv == STTUNER_IQ_MODE_INVERTED)
	{
		/* I,Q Spectrum Inverted*/ 
		STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_SPECTRUM_INVERT,FSTB0899_SPECTRUM_INVERT_INFO,1);


	}
	else
	{
		/* I,Q Auto use last "successful search" value first  */ 
		STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_SPECTRUM_INVERT,FSTB0899_SPECTRUM_INVERT_INFO,Params->IQLocked);

	}
	
	do{
		Error |= (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, (U32)Params->Frequency , (U32 *)&Params->Frequency);	/*	Move the tuner	*/ 
		
		if(Error != ST_NO_ERROR)
		{
			return(FE_DVBS2_NOAGC);
		}
		/*	Temporisations	*/
		WAIT_N_MS_899(10);	/*	Wait for agc1,agc2 and timing loop	*/
		tuner_tunsdrv_Utilfunction(TunerInstance->DrvHandle, VCO_SEARCH_OFF);
		FE_STB0899_WaitTuner(TunerInstance,100);		/*	Is tuner Locked	? (wait 100 ms maxi)	*/
		/*Set IF AGC to Acquire value*/
		
		STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_IF_LOOPGAIN,FSTB0899_IF_LOOPGAIN_INFO,4);
		STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_IF_AGCREF,FSTB0899_IF_AGCREF_INFO,32);
		
		STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_IF_AGC_DUMPPER,FSTB0899_IF_AGC_DUMPPER_INFO,0);
		FE_DVBS2_Reacquire(DeviceMap,IOHandle);
		/*Wait for UWP,CSM and DATA LOCK 70ms max*/
		Params->Results.DVBS2SignalType=Params->DVBS2State=FE_DVBS2_GetDemodStatus(DeviceMap,IOHandle,searchTime);
		if(Params->DVBS2State==FE_DVBS2_DEMOD_LOCKED) 
		{
			/*Demod Locked, check the FEC */
			Params->Results.DVBS2SignalType=Params->DVBS2State=FE_DVBS2_GetFecStatus(DeviceMap,IOHandle,fecLockTime);
				
		}
		if(Params->DVBS2State!=FE_DVBS2_DATAOK)
		{
			if(Params->SpectralInv == STTUNER_IQ_MODE_AUTO)
			{  
				iqSpectrum=STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle, FSTB0899_SPECTRUM_INVERT,FSTB0899_SPECTRUM_INVERT_INFO);
				/* I,Q Spectrum Inverted*/ 
				STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_SPECTRUM_INVERT,FSTB0899_SPECTRUM_INVERT_INFO,!iqSpectrum);

				/* start acquistion process  */
				FE_DVBS2_Reacquire(DeviceMap,IOHandle);
				/*Whait for UWP,CSM and data LOCK 200ms max*/
				Params->Results.DVBS2SignalType=Params->DVBS2State=FE_DVBS2_GetDemodStatus(DeviceMap,IOHandle,searchTime);
				if(Params->DVBS2State == FE_DVBS2_DEMOD_LOCKED)
				{
					/*Demod Locked, check the FEC */
					Params->Results.DVBS2SignalType=Params->DVBS2State=FE_DVBS2_GetFecStatus(DeviceMap,IOHandle,fecLockTime);
				}
				if(Params->DVBS2State==FE_DVBS2_DATAOK)
				Params->IQLocked=!iqSpectrum;
			}
		}
		
		if(Params->DVBS2State!=FE_DVBS2_DATAOK)
		{
			FE_899_NextSubRange(Params);		
		}
	}	  
	while(Params->SubRange && Params->DVBS2State!=FE_DVBS2_DATAOK);
	
	
	if(Params->DVBS2State==FE_DVBS2_DATAOK)
	{
		modCode = Params->Results.ModCode=(FE_DVBS2_ModCod_t)FE_DVBS2_GetModCod(DeviceMap, IOHandle);
		pilots = Params->Results.Pilots=STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle,FSTB0899_UWP_DECODED_MODCODE, FSTB0899_UWP_DECODED_MODCODE_INFO)&0x01;
		Params->Results.FrameLength= (FE_DVBS2_FRAME)((STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle,FSTB0899_UWP_DECODED_MODCODE, FSTB0899_UWP_DECODED_MODCODE_INFO)>>1)&0x01);
	        if((((10*Params->MasterClock)/(Params->DVBS2SymbolRate/10))<=400)&&(INRANGE(FE_QPSK_23,modCode,FE_QPSK_910))&&(pilots==1))
		{
			FE_DVBS2_CSMInitialize(DeviceMap,IOHandle,pilots,modCode,Params->DVBS2SymbolRate,Params->MasterClock);
			/*Wait for UWP,CSM and data LOCK 20ms max*/
			Params->Results.DVBS2SignalType=Params->DVBS2State=FE_DVBS2_GetFecStatus(DeviceMap,IOHandle,fecLockTime);
		}
		if((((10*Params->MasterClock)/(Params->DVBS2SymbolRate/10))<=400)&&(INRANGE(FE_QPSK_12,modCode,FE_QPSK_35))&&(pilots==1))
		{
			STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_EQ_DISABLE_UPDATE,FSTB0899_EQ_DISABLE_UPDATE_INFO,1);	
		}
		
		STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_EQ_SHIFT,FSTB0899_EQ_SHIFT_INFO,0x2);

		offsetfreq = STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle, FSTB0899_CRL_FREQUENCY, FSTB0899_CRL_FREQUENCY_INFO);
		offsetfreq = offsetfreq/(S32)(FE_899_PowOf2(30)/1000);
		
		offsetfreq *= (S32)(Params->MasterClock/1000000);
		if(STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle,FSTB0899_SPECTRUM_INVERT,FSTB0899_SPECTRUM_INVERT_INFO))
		offsetfreq*=-1;	
		
		Params->Results.Frequency = (U32)(Params->Frequency - offsetfreq);
		if((Params->Results.Frequency > Params->BaseFreq + Params->SearchRange/2000) || (Params->Results.Frequency < Params->BaseFreq - Params->SearchRange/2000))
		{
			Params->DVBS2State = FE_DVBS2_OUTOFRANGE;
			
			return Params->DVBS2State;
		}
		if(!Inst[TopLevelHandle].Sat.ScanExact)
		{
			Params->Results.DVBS2SymbolRate=FE_DVBS2_GetSymbolRate(DeviceMap, IOHandle, Params->MasterClock);
		}
		else 
		{
			Params->Results.DVBS2SymbolRate = Params->DVBS2SymbolRate;
		}		
		
		/*if offset Frequency >5MHz Tuner centring */
		if(offsetfreq>6000)
		{
			Error |= (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, (U32)Params->Results.Frequency , (U32 *)&Params->Frequency);	/*	Move the tuner	*/ 
		}
		
		 /*Set IF AGC to tracquing value*/
		STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_IF_LOOPGAIN,FSTB0899_IF_LOOPGAIN_INFO,3);
		/*if QPSK 1/2,QPSK 3/5 or QPSK 2/3 set IF AGC reference to 16 otherwise 32*/
		if(INRANGE(FE_QPSK_12,Params->Results.ModCode,FE_QPSK_23))
			STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_IF_AGCREF,FSTB0899_IF_AGCREF_INFO,16);	/*Write field Image*/
		
		STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_IF_AGC_DUMPPER,FSTB0899_IF_AGC_DUMPPER_INFO,7);
		
	}
	/*Release Stream Merger Reset*/
	STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_FRESRS,FSTB0899_FRESRS_INFO,0);	
	return	Params->DVBS2State;
}
/*****************************************************
--FUNCTION	::	FE_STB0899_GetRFLevel
--ACTION	::	Return power of the signal
--PARAMS IN	::	NONE	
--PARAMS OUT::	NONE
--RETURN	::	Power of the signal (dBm), -100 if no signal 
--***************************************************/
S32 FE_STB0899_GetRFLevel(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,STTUNER_FECType_t FECType, STTUNER_FECMode_t FECMode)
{
	S32 agcGain = 0,
		Imin,
		Imax,
		i,
		rfLevel = 0;
	FE_STB0899_LOOKUP_t *lookup;
	lookup = &FE_STB0899_RF_LookUp;
	
	if(FECType == STTUNER_FEC_MODE_DVBS2)
	lookup = &FE_STB0899_DVBS2RF_LookUp;
	
	if((lookup != NULL) && lookup->size)
	{
		if((FECType == STTUNER_FEC_MODE_DVBS1) || (FECMode == STTUNER_FEC_MODE_DIRECTV))
		{
			agcGain = STTUNER_IOREG_GetField_SizeU32(DeviceMap,IOHandle,FSTB0899_AGCIQ_VALUE,FSTB0899_AGCIQ_VALUE_INFO);
		}
		else if(FECType == STTUNER_FEC_MODE_DVBS2)
		{	
			agcGain = STTUNER_IOREG_GetField_SizeU32(DeviceMap,IOHandle,FSTB0899_IF_AGCGAIN,FSTB0899_IF_AGCGAIN_INFO);
			
		}
		Imin = 0;
		Imax = lookup->size-1;
		
		if(INRANGE(lookup->table[Imin].regval,agcGain,lookup->table[Imax].regval))
		{
			while((Imax-Imin)>1)
			{
				i=(Imax+Imin)/2; 
				if(INRANGE(lookup->table[Imin].regval,agcGain,lookup->table[i].regval))
					Imax = i;
				else
					Imin = i;
			}
			
			rfLevel =	(((S32)agcGain - lookup->table[Imin].regval)
					* (lookup->table[Imax].realval - lookup->table[Imin].realval)
					/ (lookup->table[Imax].regval - lookup->table[Imin].regval))
					+ lookup->table[Imin].realval;
		}
		else
			rfLevel = -100;
	}
	
	return rfLevel*10;
}


/*****************************************************
--FUNCTION	::	FE_STB0899_GetCarrierQuality
--ACTION	::	Return the carrier to noise of the current carrier
--PARAMS IN	::	NONE	
--PARAMS OUT::	NONE
--RETURN	::	C/N of the carrier, 0 if no carrier 
--***************************************************/
int FE_STB0899_GetCarrierQuality(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,STTUNER_FECType_t FECType, STTUNER_FECMode_t FECMode)  
{
		int c_n = 0,
		quant,
		regval,
		Imin,
		Imax,
		i;
		U32  nirm[2];
	long val2 = 0;
	long coeff2log10 = 646456993;
	FE_STB0899_LOOKUP_t *lookup;
	        lookup = &FE_STB0899_CN_LookUp;
	
	
	if((FECType == STTUNER_FEC_MODE_DVBS1) || (FECMode == STTUNER_FEC_MODE_DIRECTV))
	{
		if(STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle,FSTB0899_CARRIER_FOUND,FSTB0899_CARRIER_FOUND_INFO))
			{
				if((lookup != NULL) && lookup->size)
				{
					STTUNER_IOREG_GetContigousRegisters_SizeU32(DeviceMap, IOHandle,RSTB0899_NIRM,2,nirm);
					regval = MAKEWORD(nirm[0],nirm[1]);
		
					Imin = 0;
					Imax = lookup->size-1;
			
					if(INRANGE(lookup->table[Imin].regval,regval,lookup->table[Imax].regval))
					{
						while((Imax-Imin)>1)
						{
							i=(Imax+Imin)/2; 
							if(INRANGE(lookup->table[Imin].regval,regval,lookup->table[i].regval))
								Imax = i;
							else
								Imin = i;
						}
				
						c_n =	((regval - lookup->table[Imin].regval)
								* (lookup->table[Imax].realval - lookup->table[Imin].realval)
								/ (lookup->table[Imax].regval - lookup->table[Imin].regval))
								+ lookup->table[Imin].realval;
					}
					else if(regval<lookup->table[Imin].regval)
						c_n = 100;
				}
			}
		}
		else if(FECType == STTUNER_FEC_MODE_DVBS2)
		{
			
			quant=STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle, FSTB0899_UWP_ESN0_QUANT, FSTB0899_UWP_ESN0_QUANT_INFO);
			c_n=FE_DVBS2_GetUWPEsNo(DeviceMap,IOHandle,quant);
			if(c_n ==1)
				c_n = 301;
			else if(c_n == 2)
				c_n = 270;
			else
			{
				val2 = (long)(-1*(Log10Int((long)(c_n))-2*Log10Int((long)(quant))));
				val2 = MULT32X32(val2,coeff2log10);
				c_n = (int)(((unsigned long)(val2*100))/FE_899_PowOf2(24));
			}
		}
	
	if(c_n>=100)
	c_n = 100;
	return c_n;

}
/*****************************************************
--FUNCTION	::	FE_STB0899_GetError
--ACTION	::	Return the BER
--PARAMS IN	::		
--PARAMS OUT::	
--RETURN	::	In DVBS1 returns BER & In DVBS2 returns LDPC errors
--***************************************************/
U32 FE_STB0899_GetError(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,STTUNER_FECType_t FECType, STTUNER_FECMode_t FECMode)
{
	U32 ber = 0,i;
	if((FECType == STTUNER_FEC_MODE_DVBS1) || (FECMode == STTUNER_FEC_MODE_DIRECTV))
	{
		/*Read registers*/
		STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,RSTB0899_ERRCTRL1,0x3D);	/* force to viterbi bit error */ 
		STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,RSTB0899_VSTATUS);
		
		/* Average 5 ber values */ 
		for(i=0;i<5;i++)
		{
			WAIT_N_MS_899(100); 
			ber += FE_STB0899_GetErrorCount(DeviceMap,IOHandle,COUNTER1_899);
		}
		
		ber/=5;
		
		if(STTUNER_IOREG_GetField_SizeU32(DeviceMap,IOHandle,FSTB0899_PRFVIT,FSTB0899_PRFVIT_INFO))	/*	Check for carrier	*/
		{
				/*	Error Rate	*/
			ber *= 9766;
			ber /= (U32)(-1+FE_899_PowOf2(0 + 2*STTUNER_IOREG_GetField_SizeU32(DeviceMap,IOHandle,FSTB0899_NOE,FSTB0899_NOE_INFO)));	/*  theses two lines => ber = ber * 10^7	*/
			ber/=8;
				
		}
	}
	else if(FECType == STTUNER_FEC_MODE_DVBS2)
	{
		STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,RSTB0899_ERRCTRL1,0xB6);	/* force to DVBS2 PER  */ 
		STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,RSTB0899_VSTATUS);

		/* Average 5 ber values */ 
		for(i=0;i<5;i++)
		{
			WAIT_N_MS_899(100);
			ber += FE_STB0899_GetErrorCount(DeviceMap,IOHandle,COUNTER1_899);
		}
		ber/=5;
		ber*=10000000;
		ber/=(U32)(-1+FE_899_PowOf2(4 + 2*STTUNER_IOREG_GetField_SizeU32(DeviceMap,IOHandle,FSTB0899_NOE,FSTB0899_NOE_INFO)));	/*  theses two lines => per = ber * 10^7	*/
		
	
	}
	
	return ber;
}


/*Symbol Rate in Hz,Mclk in Hz */
void FE_STB0899_SetIterScal(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 MasterClock,U32 SymbolRate)
{
	S32 iTerScal,
	    maxIter;
	
	iTerScal = 17* (MasterClock/1000);
	iTerScal +=410000;
	iTerScal /= (SymbolRate/1000000);
	iTerScal /=1000;

	maxIter=LPDPC_MAX_ITER;
	
	if(iTerScal>maxIter)
		iTerScal=maxIter;
		
	/*use set field mandatory*/
	STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_ITERATION_SCALE,FSTB0899_ITERATION_SCALE_INFO,iTerScal);

	
}

/*****************************************************
--FUNCTION	::	FE_STB0899_Search
--ACTION	::	Run the search algo for DVBS1 & DVBS2 signal
--PARAMS IN	::	FE_STB0899_SearchParams_t	
--PARAMS OUT::	
--RETURN	::	Error
--***************************************************/
FE_STB0899_Error_t	FE_STB0899_Search(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,
										    FE_STB0899_SearchParams_t	*pSearch,
										    FE_STB0899_SearchResult_t	*pResult,
										    STTUNER_Handle_t            TopLevelHandle)
{
    FE_STB0899_Error_t error = FE_899_NO_ERROR;
    FE_STB0899_InternalParams_t *Params;
    STTUNER_InstanceDbase_t *Inst;
    STTUNER_tuner_instance_t *TunerInstance;
    TUNSDRV_InstanceData_t *Instance;
    TUNER_Status_t  TunerStatus;  
    ST_ErrorCode_t Error;
   
    /* top level public instance data */ 
    Inst = STTUNER_GetDrvInst();

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[TopLevelHandle].Sat.Tuner;
    Instance = TUNSDRV_GetInstFromHandle(TunerInstance->DrvHandle);
    Instance->SymbolRate = (U32)pSearch->SymbolRate;
    
	Params = STOS_MemoryAllocateClear(DeviceMap->MemoryPartition, 1, sizeof( FE_STB0899_InternalParams_t));
	Params->Quartz = 27000000; /* 27 MHz quartz */
	Error = (TunerInstance->Driver->tuner_GetStatus)(TunerInstance->DrvHandle, &TunerStatus);
	Params->TunerIQSense = 1;
	Params->DemodIQMode = pSearch->DemodIQMode;
	Params->Standard = pSearch->Standard;
	Params->FECMode = pSearch->FECMode;
	/*Params->Pilots = pSearch->Pilots;*/
	FE_STB0899_InitialCalculations(DeviceMap, IOHandle, Params);
	
	if(Params != NULL)
	{	
		if(	(INRANGE(1000000,pSearch->SymbolRate,45000000)) &&
			(INRANGE(1000000,pSearch->SearchRange,66000000))
		  )
		  {
			FE_STB0899_SetStandard(DeviceMap, IOHandle, pSearch->Standard, pSearch->FECMode);
			/*For Low Symbol Rate (<=5Mbs) set Mclk to 45MHz, else use 108MHz*/
			if(pSearch->SymbolRate <= 5000000)
			{
				FE_STB0899_SetMclk(DeviceMap, IOHandle, 45000000,Params);	
			}
			else
			{
				FE_STB0899_SetMclk(DeviceMap, IOHandle, 99000000,Params);
			}
			if((Params->FECMode == STTUNER_FEC_MODE_DIRECTV) || (Params->Standard == STTUNER_FEC_MODE_DVBS1))
			{
					/* Fill Params structure with search parameters */
					Params->BaseFreq = pSearch->Frequency;	
					Params->SymbolRate = pSearch->SymbolRate;
					Params->SearchRange = pSearch->SearchRange;
					Params->DerotPercent = 1000;
					Error = (TunerInstance->Driver->tuner_SetBandWidth)( TunerInstance->DrvHandle, 
                                                        (13*((U32)( FE_899_CarrierWidth(Params->SymbolRate,Params->RollOff) + 10000000)/1000)/10),/* set bandwidth some more to optimize scanning->GNBvd26185*/
                                                        &(Params->TunerBW));
                                        
                                       	/* Run the search algorithm */
					/*Set DVB-S1 AGC*/
					STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, RSTB0899_AGCRFCFG, 0x11);
					if((FE_STB0899_Algo(DeviceMap, IOHandle, Params, TopLevelHandle) == RANGEOK_899) && (DeviceMap->Error==ST_NO_ERROR))
						{
							pResult->Locked = TRUE;
							/* update results */
							pResult->Frequency = Params->Results.Frequency;			
							pResult->SymbolRate = Params->Results.SymbolRate;										
							pResult->Rate = Params->Results.PunctureRate;
						}
						else
						{
							pResult->Locked = FALSE;
							error = FE_899_SEARCH_FAILED;
															
						}
				}
				
				else if(Params->Standard == STTUNER_FEC_MODE_DVBS2)				
				{
					
					/* Fill Params structure with search parameters */
					Params->Frequency = pSearch->Frequency;
					Params->BaseFreq = pSearch->Frequency;
					Params->DVBS2SymbolRate = pSearch->SymbolRate;
					Params->SpectralInv=pSearch->DemodIQMode;
					Params->SearchRange = pSearch->SearchRange;
					
					Error = (TunerInstance->Driver->tuner_SetBandWidth)( TunerInstance->DrvHandle,(U32)(((FE_DVBS2_CarrierWidth(Params->DVBS2SymbolRate,Params->RrcAlpha)) + 10000000)/1000), &(Params->TunerBW));
					
					/*Set DVB-S2 AGC*/
					STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, RSTB0899_AGCRFCFG, 0x1c);
					/*Set IterScale =f(MCLK,SYMB,MODULATION*/
					FE_STB0899_SetIterScal(DeviceMap, IOHandle, Params->MasterClock,Params->DVBS2SymbolRate);
					
					/* Run the DVBS2  search algorithm */
					if((FE_STB0899_DVBS2Algo(DeviceMap, IOHandle, Params, TopLevelHandle) == FE_DVBS2_DATAOK)&& (DeviceMap->Error==ST_NO_ERROR))
					{
						
						pResult->Locked = TRUE;
					
						/* update results */
						pResult->Frequency = Params->Results.Frequency;			
						pResult->SymbolRate = Params->Results.DVBS2SymbolRate;										
						pResult->ModCode = Params->Results.ModCode;
						pResult->Pilots  = Params->Results.Pilots;
						pResult->FrameLength = Params->Results.FrameLength;
						
					}
					else
					{
						pResult->Locked = FALSE;
						error = FE_899_SEARCH_FAILED;
					}

				
			}
		
		}
			else
				error = FE_899_BAD_PARAMETER;
	
	}
	else
	error=FE_899_BAD_PARAMETER;
	
    TunerInstance->realfrequency = pResult->Frequency;	
    STOS_MemoryDeallocate(DeviceMap->MemoryPartition, Params);
	return error;

	
}

/*****************************************************
--FUNCTION	::	FE_STB0899_GetSignalInfo
--ACTION	::	Return informations on the locked transponder
--PARAMS IN	::	Handle	==>	Front End Handle
--PARAMS OUT::	pInfo	==> Informations (BER,C/N,power ...)
--RETURN	::	Error (if any)
--***************************************************/
FE_STB0899_Error_t   FE_STB0899_GetSignalInfo(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,STTUNER_Handle_t TopLevelHandle, FE_STB0899_SignalInfo_t	*pInfo, STTUNER_FECType_t FECType, STTUNER_FECMode_t FECMode)
{
	FE_STB0899_Error_t error = FE_899_NO_ERROR;
	STTUNER_tuner_instance_t *TunerInstance;
	STTUNER_InstanceDbase_t *Inst;  
	U32 cfrm[2];
	S32 derotFreq, offsetfreq;
	U32  MasterClock, Mclk;
	Inst = STTUNER_GetDrvInst(); 
	TunerInstance = &Inst[TopLevelHandle].Sat.Tuner;
	MasterClock = FE_STB0899_GetMclkFreq(DeviceMap, IOHandle,27000000);
	Mclk = (U32)(MasterClock/65536L); 
	
	if((FECMode == STTUNER_FEC_MODE_DIRECTV) || (FECType== STTUNER_FEC_MODE_DVBS1))
	{
				pInfo->Locked = STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle,FSTB0899_LOCKEDVIT,FSTB0899_LOCKEDVIT_INFO);
		
				if(pInfo->Locked)
				{
					STTUNER_IOREG_GetContigousRegisters_SizeU32(DeviceMap, IOHandle,RSTB0899_CFRM,2,cfrm);							/*	read derotator value */ 
					cfrm[0]&=0xff;
					cfrm[1]&=0xff;
					derotFreq = (S16) MAKEWORD(cfrm[0],cfrm[1]); 
					/* transponder_frequency = tuner + derotator_frequency */
					pInfo->Frequency = TUNSDRV_GetInstFromHandle(TunerInstance->DrvHandle)->Status.Frequency + ((derotFreq*((S16)Mclk))/1000);
					pInfo->SymbolRate = FE_STB0899_GetSymbolRate(DeviceMap, IOHandle, MasterClock);	/* Get symbol rate */
					pInfo->SymbolRate += (pInfo->SymbolRate*STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle, FSTB0899_TIMING_LOOP_FREQ,FSTB0899_TIMING_LOOP_FREQ_INFO))>>19;	/* Get timing loop offset */
					pInfo->Rate = (FE_STB0899_Rate_t)STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle, FSTB0899_VIT_CURPUN,FSTB0899_VIT_CURPUN_INFO);
					pInfo->Power = FE_STB0899_GetRFLevel(DeviceMap, IOHandle, FECType, FECMode);
					pInfo->C_N = FE_STB0899_GetCarrierQuality(DeviceMap, IOHandle,FECType, FECMode);
					pInfo->BER = FE_STB0899_GetError(DeviceMap, IOHandle, FECType, FECMode);
					if(STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle,FSTB0899_SYMI,FSTB0899_SYMI_INFO)==0)
						pInfo->SpectralInv = STTUNER_IQ_MODE_NORMAL;
					else
						pInfo->SpectralInv = STTUNER_IQ_MODE_INVERTED;
				}
		}
		else
		{
			
				pInfo->Locked=(((STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle, FSTB0899_LOCK, FSTB0899_LOCK_INFO))&&
				   	(STTUNER_IOREG_GetRegister(DeviceMap, IOHandle,RSTB0899_DMDSTAT2)==0x03))?1:0);
				if(pInfo->Locked)
				{
					
					offsetfreq=STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle, FSTB0899_CRL_FREQUENCY, FSTB0899_CRL_FREQUENCY_INFO);
					offsetfreq = (S32)(offsetfreq/(FE_899_PowOf2(30)/1000));
					
					offsetfreq*=(MasterClock/1000000);
					if(STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle, FSTB0899_SPECTRUM_INVERT, FSTB0899_SPECTRUM_INVERT_INFO))
					offsetfreq*=-1;	
					pInfo->Frequency=TUNSDRV_GetInstFromHandle(TunerInstance->DrvHandle)->Status.Frequency - offsetfreq;
					
	  				pInfo->SymbolRate=FE_DVBS2_GetSymbolRate(DeviceMap, IOHandle, MasterClock);
					pInfo->ModCode=(FE_DVBS2_ModCod_t)FE_DVBS2_GetModCod(DeviceMap, IOHandle);
					pInfo->Pilots=STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle,FSTB0899_UWP_DECODED_MODCODE,FSTB0899_UWP_DECODED_MODCODE_INFO)&0x01;
					pInfo->FrameLength= (STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle,FSTB0899_UWP_DECODED_MODCODE,FSTB0899_UWP_DECODED_MODCODE_INFO)>>1)&0x01;
				
					pInfo->C_N = FE_STB0899_GetCarrierQuality(DeviceMap, IOHandle,FECType, FECMode);
					pInfo->Power = FE_STB0899_GetRFLevel(DeviceMap, IOHandle, FECType, FECMode);
					pInfo->BER = FE_STB0899_GetError(DeviceMap, IOHandle,FECType, FECMode);
					if(STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle,FSTB0899_SPECTRUM_INVERT,FSTB0899_SPECTRUM_INVERT_INFO)==0)
						pInfo->SpectralInv = STTUNER_IQ_MODE_NORMAL;
					else
						pInfo->SpectralInv = STTUNER_IQ_MODE_INVERTED;

				}
				
		}
	
	return error;
}

/*****************************************************
--FUNCTION	::	FE_STB0899_ToneDetection
--Description	::	Returns the no. of tones detected and fill their freq in ToneLIst
--Return: No. of tones deteced.

--***************************************************/
U32 FE_STB0899_ToneDetection(STTUNER_IOREG_DeviceMap_t *DeviceMap,STTUNER_Handle_t TopLevelHandle,U32 StartFreq,U32 StopFreq,U32 *ToneList, U8 mode)
{
	U32     step;		/* Frequency step */ 
		
	U32     BandWidth;
	U8	nbTones=0;
	STTUNER_InstanceDbase_t *Inst;
	ST_ErrorCode_t Error = ST_NO_ERROR;

	STTUNER_tuner_instance_t *TunerInstance;
        Inst = STTUNER_GetDrvInst();
       /* get the tuner instance for this driver from the top level handle */
       TunerInstance = &Inst[TopLevelHandle].Sat.Tuner;
	
	/* wide band acquisition */
	step = 36000000;						/* use 36MHz step */
	if(TunerInstance->Driver->ID == STTUNER_TUNER_STB6100)
	Error = (TunerInstance->Driver->tuner_SetBandWidth)(TunerInstance->DrvHandle, 36000, &BandWidth); /* LPF cut off freq = 18MHz*/
	else 		
	Error = (TunerInstance->Driver->tuner_SetBandWidth)(TunerInstance->DrvHandle, 36000, &BandWidth); /* LPF cut off freq = 18MHz*/
	if(Error == ST_NO_ERROR)
	{
	nbTones = FE_STB0899_SpectrumAnalysis(TunerInstance, DeviceMap, Inst[TopLevelHandle].Sat.Demod.IOHandle, StartFreq, StopFreq, step, ToneList, mode);	/* spectrum acquisition */
	}
		
	return nbTones;
}
/*****************************************************
--FUNCTION	::	FE_STB0899_SpectrumAnalysis
--Description	::	Read the tone spectrum in given band.
--Return: No. of tones deteced.

--***************************************************/
U32 FE_STB0899_SpectrumAnalysis(STTUNER_tuner_instance_t *TunerInstance, STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 StartFreq,U32 StopFreq,U32 StepSize,U32 *ToneList, U8 mode)
{
    U32 freq, realTunerFreq, freqstep = 2000000,
	tempfreq,lastfreq,freqlowerthreshold, freqhigherthreshold,epsilon;
   int  agcVal[500],index=0, points = 0, pt_max,i=0, j = 1;
    int direction = 1, agc_threshold, *spectrum_agc ,agc_threshold_adjust,agc_high,agc_low,agc_threshold_detect;
    U32 *spectrum;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    TUNSDRV_InstanceData_t *Instance;

	points = (U16)((StopFreq - StartFreq)/StepSize + 1);
	spectrum = STOS_MemoryAllocateClear(DeviceMap->MemoryPartition, points, sizeof(U16));
	spectrum_agc = STOS_MemoryAllocateClear(DeviceMap->MemoryPartition, points, sizeof(int));
	
	Instance = TUNSDRV_GetInstFromHandle(TunerInstance->DrvHandle);
	if(Instance->TunerType == STTUNER_TUNER_STB6100)
	STTUNER_IOREG_SetFieldVal(&(Instance->DeviceMap),FSTB6100_G,7, Instance->TunerRegVal);		/* Gain = 0 */
	
	Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap), Instance->IOHandle, 5,Instance->TunerRegVal, 1);
	    		
	Error |= STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle, FSTB0899_AGCIQ_REF, FSTB0899_AGCIQ_REF_INFO,16); /* set AGC_Ref as 16*/
	lastfreq = StartFreq - 100000000;
	 if(mode==1) 
        {
	 agc_high=-500;
	 agc_low=500;
	 for(freq=StartFreq;freq<StopFreq;freq+=3000000)
	 {
	  Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, freq/1000, (U32 *)&realTunerFreq);
	  WAIT_N_MS_899(2);
	  agc_threshold = STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle, FSTB0899_AGCIQ_VALUE, FSTB0899_AGCIQ_VALUE_INFO);
	  agcVal[index]=agc_threshold;
	  index++;
	  if(agc_threshold>agc_high) {
	     agc_high=agc_threshold;
	  }
	  if(agc_threshold<agc_low) {
	     agc_low=agc_threshold;
	  }
         };
        
	 /*adjust agc_threshold*/
	 points=0;
	 agc_threshold_adjust=(agc_high+agc_low)/2;
	 pt_max=3*index/4;
	 epsilon=agc_high-agc_low;
	 
	 while(epsilon>5) {
	  points=0;
	  /*get numbers of points for which treshold is lower than agc_threshold_adjust*/		
	  for(i=0;i<index;i++) {
	   if(agcVal[i]<agc_threshold_adjust) {
	   points++;
	   }
	  }
	  if(points>pt_max) { 
	  	/*most of points are bellow agc_threshold_adjust level*/
	  	agc_high=agc_threshold_adjust;
	  } else {
	  	agc_low=agc_threshold_adjust;
	  }
	  agc_threshold_adjust=(agc_high+agc_low)/2;
	  epsilon=agc_high-agc_low;
	 }
	  agc_threshold_detect=agc_threshold_adjust; /*quick setup*/
	} else {
	  agc_threshold_detect=-70; /*quick setup*/
	}
	
	i=0;
	points=0;
	for(freq=StartFreq;freq<StopFreq;freq+=StepSize)
	{
		direction = 1;j = 1;
		Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, freq/1000, (U32 *)&realTunerFreq);
		WAIT_N_MS_899(2);
		agc_threshold = STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle, FSTB0899_AGCIQ_VALUE, FSTB0899_AGCIQ_VALUE_INFO);
		if(agc_threshold > (agc_threshold_detect))/* - 70 */
		{
				if(((agc_threshold >= agc_threshold_detect) && (ABS((S32)(freq - lastfreq)))>50000000))
				{
					
					while(agc_threshold > agc_threshold_detect)
					{
						tempfreq = freq+(freqstep)*direction * j;
						Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, tempfreq/1000, (U32 *)&realTunerFreq);
						WAIT_N_MS_899(2);
						agc_threshold = STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle, FSTB0899_AGCIQ_VALUE, FSTB0899_AGCIQ_VALUE_INFO);
						++j;
				        }
				        freqhigherthreshold = tempfreq;
				        direction *= -1;
				        agc_threshold = 0; j = 1;
				        do
					{
						tempfreq = freq+(freqstep)*direction * j;
						Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, tempfreq/1000, (U32 *)&realTunerFreq);
						WAIT_N_MS_899(2);
						agc_threshold = STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle, FSTB0899_AGCIQ_VALUE, FSTB0899_AGCIQ_VALUE_INFO);
						++j;
				        }while( agc_threshold> agc_threshold_detect);
				        freqlowerthreshold = tempfreq;
				        spectrum[i] = freqlowerthreshold + (freqhigherthreshold - freqlowerthreshold)/2;
				        
				        if(spectrum[i] >= StartFreq && spectrum[i] <= StopFreq)
				        {
				        	lastfreq = spectrum[i];
				        	Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, spectrum[i]/1000, (U32 *)&realTunerFreq);
						WAIT_N_MS_899(2);
						spectrum_agc[i] = STTUNER_IOREG_GetField_SizeU32(DeviceMap, IOHandle, FSTB0899_AGCIQ_VALUE, FSTB0899_AGCIQ_VALUE_INFO);
				                ++i; 
				        	
				        }
				        
					
				}	
		}
				
	}
	if(mode==1) {
	 agc_threshold=agc_threshold_detect;
	 for(j = 0; j<i; ++j) {
			ToneList[points] = spectrum[j];
			points++;
			if(points>=8)
			break;
	 }
	} else {

	 for(j = 0; j<i; ++j) {
			ToneList[points] = spectrum[j];
			points++;
			if(points>=8)
			break;
	 }	
	}	
	
	STOS_MemoryDeallocate(DeviceMap->MemoryPartition, spectrum);
	STOS_MemoryDeallocate(DeviceMap->MemoryPartition, spectrum_agc);
	return points;		
	
}










