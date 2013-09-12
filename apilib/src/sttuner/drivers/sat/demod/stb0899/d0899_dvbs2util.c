/* -------------------------------------------------------------------------
File Name: drv0899_dvbs2util.c

Description: STB0899 driver LLA	V2.0.0 18/04/2005
author: SD

Description: STB0899 driver LLA	V3.0.3 05/07/2005
author: SD

Date 29/03/06
Description: STB0899 driver LLA V3.6.0 integration
author: HS
---------------------------------------------------------------------------- */
/* includes ---------------------------------------------------------------- */
#include "ioreg.h"
#include "d0899_init.h"
#include "sttuner.h"
#include "d0899_util.h"
#include "d0899_dvbs2util.h"
#include "stcommon.h"

#define ESNO_AVE            3
#define ESNO_QUANT          32
#define AVEFRAMES_COARSE    10
#define AVEFRAMES_FINE      20
#define MISS_THRESHOLD      6
#define UWP_THRESHOLD_ACQ   1125  
#define UWP_THRESHOLD_TRACK 758  
#define UWP_THRESHOLD_SOF   1350  
#define SOF_SEARCH_TIMEOUT 1664100


/*****************************************************
**FUNCTION	::	FE_DVBS2_SetCarrierFreq
**ACTION	::	Set the nominal frequency for carrier search 
**PARAMS IN	::  hChip		==>	handle to the chip
			::	CarrierFreq	==> carrier frequency in MHz.
			::	MasterClock	==> demod clock in MHz.
**PARAMS OUT::	NONE
**RETURN	::	None
*****************************************************/
void FE_DVBS2_SetCarrierFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,S32 CarrierFreq,U32 MasterClock)
{
	S32 crlNomFreq;				
	
	crlNomFreq=(S32)((FE_899_PowOf2(CRL_NCO_BITS))/MasterClock);
	crlNomFreq*=CarrierFreq;
	/*ChipSetFieldImage(DeviceMap,IOHandle,FSTB0899_CRLNOM_FREQ,crlNomFreq);*/
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_CRLNOMFREQ,crlNomFreq);
}


/*****************************************************
**FUNCTION	::	FE_DVBS2_GetSymbolRate
**ACTION	::	Return the DVBS2 Symbol Rate 
**PARAMS IN	::  hChip		==>	handle to the chip
			::	MasterClock	==> demod clock in Hz.
**PARAMS OUT::	NONE
**RETURN	::	Symbol Rate in Symb/s
*****************************************************/
U32 FE_DVBS2_GetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 MasterClock)
{
	U32	bTrNomFreq,
		symbolRate,
		decimRate,
		intval1,intval2;
	int div1,div2,rem1,rem2;

	
	
	div1=BTR_NCO_BITS/2;
	div2=BTR_NCO_BITS-div1-1;
	
	bTrNomFreq=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,RSTB0899_BTRNOMFREQ);
	decimRate=STTUNER_IOREG_GetField_SizeU32(DeviceMap,IOHandle,FSTB0899_DECIM_RATE,FSTB0899_DECIM_RATE_INFO);
	decimRate=(U32)FE_899_PowOf2(decimRate);

	intval1=(U32)(MasterClock/FE_899_PowOf2(div1));
	intval2=(U32)(bTrNomFreq/FE_899_PowOf2(div2));

	rem1=(int)(MasterClock%FE_899_PowOf2(div1));
	rem2=(int)(bTrNomFreq%FE_899_PowOf2(div2));
	symbolRate=(intval1*intval2)+((intval1*rem2)/(U32)FE_899_PowOf2(div2))+((intval2*rem1)/(U32)FE_899_PowOf2(div1));  /*only for integer calculation */
	symbolRate/=decimRate;  	/*symbrate = (btrnomfreq_register_val*MasterClock)/2^(27+decim_rate_field) */
	return	symbolRate; 
	
}


/*****************************************************
**FUNCTION	::	DVBS2CalclSymbRate
**ACTION	::	Compute the value of the register BTRNOMFREQ  
				according to the given Symbol Rate 
**PARAMS IN	::	SymbolRate	==> Symbol rate in Symb/s.
			::	MasterClock	==> demod clock in Hz.
**PARAMS OUT::	NONE
**RETURN	::	BTRNOMFREQ reg value
*****************************************************/
U32 DVBS2CalclSymbRate(U32 SymbolRate,U32 MasterClock)
{
	U32	decimRatio,
	decimRate,
	decimation,
	remain,
	intval,
	btrNomFreq;
	
	decimRatio = (MasterClock*2) / (5 * SymbolRate);
	decimRatio = (decimRatio == 0) ? 1 : decimRatio;
	decimRate = (U32)Log2Int(decimRatio);
	decimation=1<<decimRate;
	MasterClock/=1000; /* for integer Caculation*/
	SymbolRate/=1000;  /* for integer Caculation*/
	
	if(decimation<=4)
    {
    	intval=(U32)((decimation*FE_899_PowOf2(BTR_NCO_BITS-1))/MasterClock);
     	remain= (U32)((decimation*FE_899_PowOf2(BTR_NCO_BITS-1))%MasterClock);
    }
	else
    {
    	intval =(U32)FE_899_PowOf2(BTR_NCO_BITS-1)/(MasterClock/100) * decimation/100; 
     	remain = (U32)((decimation*FE_899_PowOf2(BTR_NCO_BITS-1))%MasterClock);
	}
	btrNomFreq =(intval*SymbolRate)+((remain*SymbolRate)/MasterClock);
	
	return btrNomFreq;
}


/*****************************************************
**FUNCTION	::	CalcCorrection
**ACTION	::	Compute the coorection to be applied to the symbol rate,
				Field BTRFREQ_CORR.  
**PARAMS IN	::	SymbolRate	==> Symbol rate in Symb/s.
			::	MasterClock	==> demod clock in Hz.
**PARAMS OUT::	NONE
**RETURN	::	BTRFREQ_CORR Field value
*****************************************************/
U32 CalcCorrection(U32	SymbolRate,U32	MasterClock)
{
	U32	decimRatio,
		correction;
	

	decimRatio = (MasterClock*2) / (5 * SymbolRate);
	decimRatio = (decimRatio == 0) ? 1 : decimRatio;

	MasterClock/=1000;	/* for integer Caculation*/
	SymbolRate/=1000;	/* for integer Caculation*/
	correction=(512*MasterClock)/(2*decimRatio*SymbolRate);
	
	return	correction;
}
	
	


/*****************************************************
**FUNCTION	::	FE_DVBS2_SetSymbolRate
**ACTION	::	Set the DVBS2 Symbol Rate.  
**PARAMS IN	::  hChip		==>	handle to the chip
			::	SymbolRate	==> Symbol rate in Symb/s.
			::	MasterClock	==> demod clock in Hz.
**PARAMS OUT::	NONE
**RETURN	::	None
*****************************************************/
void FE_DVBS2_SetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 SymbolRate,U32 MasterClock)
{
 	U32	decimRatio,
 		decimRate,
 		winSel,
 		decimation,
 		fSymovSr,
 		btrNomFreq,
 		correction,
 		freqAdjScl,
 		bandLimit,
 		decimcntrlreg;
 
 
 	/*set decimation to 1*/
	decimRatio = (MasterClock*2) / (5 * SymbolRate);
	decimRatio = (decimRatio == 0) ? 1 : decimRatio;
	decimRate = (U32)(Log2Int(decimRatio));
	


	winSel=0;
	if(decimRate >= 5)
		winSel = decimRate - 4;
	
	decimation =(1<<decimRate);

	fSymovSr=MasterClock/((decimation*SymbolRate)/1000); /* (FSamp/Fsymbol *100) for integer Caculation */
	
	if(fSymovSr<=2250)			/* don't band limit signal going into btr block*/
		bandLimit=1;			
 	else						
    	bandLimit=0;			/* band limit signal going into btr block*/
      
    decimcntrlreg=((winSel<<3)&0x18)+((bandLimit<<5)&0x20)+(decimRate&0x7);
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_DECIMCNTRL,decimcntrlreg);


	if(fSymovSr<=3450)
		STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_ANTIALIASSEL,0);
	else if(fSymovSr<=4250)
    	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_ANTIALIASSEL,1);
    else
    	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_ANTIALIASSEL,2);
   
	
	btrNomFreq=DVBS2CalclSymbRate(SymbolRate,MasterClock);
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_BTRNOMFREQ,btrNomFreq);

	correction=CalcCorrection(SymbolRate,MasterClock);
	STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_BTRFREQ_CORR,FSTB0899_BTRFREQ_CORR_INFO,correction);


	/* scale UWP+CSM frequency to sample rate*/
	freqAdjScl =  SymbolRate /( MasterClock/4096);
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_FREQADJSCALE,freqAdjScl);
}





/*****************************************************
**FUNCTION	::	FE_DVBS2_SetBtrLoopBW
**ACTION	::	Set the Bit Timing loop bandwidth 
				as a percentage of the symbol rate .  
**PARAMS IN	::  hChip		==>	handle to the chip
			::	LoopBW	==> Timing loop bandwidth parameters.
**PARAMS OUT::	NONE
**RETURN	::	None
*****************************************************/
void FE_DVBS2_SetBtrLoopBW(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_DVBS2_LoopBW_Params_t LoopBW)
{


	S32 decimRatio,
    	decimRate,
    	kbtr1Rshft,
    	kbtr1,
    	kbtr0Rshft,
    	kbtr0,
    	kbtr2Rshft,
    	kDirectShift,
    	kIndirectShift;
	
	U32	decimation,
		K,
		wn,
		kDirect,
		kIndirect,
		val;



	decimRatio = (LoopBW.MasterClock*2) / (5 * LoopBW.SymbolRate);
	decimRatio = (decimRatio == 0) ? 1 : decimRatio;
	decimRate =(U32)(Log2Int(decimRatio));
	decimation = (1<<decimRate);


	LoopBW.SymPeakVal=LoopBW.SymPeakVal*576000;
   
	K=(U32)FE_899_PowOf2(BTR_NCO_BITS)/(LoopBW.MasterClock/1000);
	K*=(U32)((LoopBW.SymbolRate/1000000)*decimation); /*k=k 10^-8*/
   
	K=LoopBW.SymPeakVal/K;

	if(K!=0)
	{
		wn = (4*LoopBW.Zeta*LoopBW.Zeta)+1000000; 
		wn=(2 * (LoopBW.LoopBwPercent*1000)*40*LoopBW.Zeta)/wn;  /*wn =wn 10^-8*/
	
		kIndirect = (wn*wn)/K;
		kIndirect=kIndirect;	  /*kindirect = kindirect 10^-6*/
	
		kDirect=(2*wn*LoopBW.Zeta)/K;	/*kDirect = kDirect 10^-2*/
		kDirect*=100;
	
		kDirectShift=(S32)Log2Int(kDirect)-(S32)Log2Int(10000)-2;
		kbtr1Rshft =   (S32)((-1*kDirectShift)+BTR_GAIN_SHIFT_OFFSET);
		kbtr1=(S32)(kDirect/FE_899_PowOf2(kDirectShift));
		kbtr1/=10000;

		kIndirectShift = (S32)Log2Int(kIndirect+15)-20 /*- 2*/;
		kbtr0Rshft = (S32)((-1* kIndirectShift)+BTR_GAIN_SHIFT_OFFSET );
		kbtr0=(S32)(kIndirect*FE_899_PowOf2(-kIndirectShift));
		kbtr0/=1000000;

		kbtr2Rshft = 0;
		if( kbtr0Rshft > 15)
		{
			kbtr2Rshft= kbtr0Rshft - 15;
			kbtr0Rshft = 15;
		}
	
		
		val=(kbtr0Rshft & 0xf)+((kbtr0 & 0xf)<<4)+((kbtr1Rshft & 0xf)<<8)+((kbtr1 & 0xf)<<12)+((kbtr2Rshft & 0xf)<<16);
	
		STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_BTRLOOPGAIN,val);
	}
	else
		STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_BTRLOOPGAIN,/*0xd6d5f*/0xc4c4f);
}


/*****************************************************
**FUNCTION	::	FE_DVBS2_BtrInit
**ACTION	::	Init the timming loop 
**PARAMS IN	::  hChip		==>	handle to the chip
**PARAMS OUT::	NONE
**RETURN	::	None
*****************************************************/
void FE_DVBS2_BtrInit(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	
	/* set enable BTR loopback*/
	STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_INTRP_PHS_SENS,FSTB0899_INTRP_PHS_SENS_INFO,1);
	STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_BTRERR_ENA,FSTB0899_BTRERR_ENA_INFO,1);

	/* fix btr freq accum at 0*/
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_BTRFREQINIT, 0x10000000);
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_BTRFREQINIT, 0x00000000);
	
	/* fix btr freq accum at 0*/
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_BTRPHSINIT, 0x10000000);
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_BTRPHSINIT, 0x00000000);
}

/*****************************************************
**FUNCTION	::	FE_DVBS2_ConfigUWP
**ACTION	::	Configure the UWP state machine
**PARAMS IN	::  hChip		==>	handle to the chip
			::	UWPparams	==> UWP parameters.
**PARAMS OUT::	NONE
**RETURN	::	None
*****************************************************/
void FE_DVBS2_ConfigUWP(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_DVBS2_UWPConfig_Params_t UWPparams)
{
	U32 val;
	
	/*Set Fields image value*/
	/*
	STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_UWP_ESN0_AVE,FSTB0899_UWP_ESN0_AVE_INFO,UWPparams.EsNoAve);
	STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_UWP_ESN0_QUANT,FSTB0899_UWP_ESN0_QUANT_INFO,UWPparams.EsNoQuant);
	STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_UWP_THRESHOLD_SOF,FSTB0899_UWP_THRESHOLD_SOF_INFO,UWPparams.ThresholdSof);
	*/
	val=((UWPparams.EsNoAve & 0x3)<<1)+((UWPparams.EsNoQuant & 0xFF)<<3)+((UWPparams.ThresholdSof & 0x7FFF)<<11);
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_UWPCNTRL1,val);
	
	/*ChipSetFieldImage(hChip,FSTB0899_FE_COARSE_TRK,UWPparams.AveFramesCoarse);
	ChipSetFieldImage(hChip,FSTB0899_FE_FINE_TRK,UWPparams.AveframesFine);
	ChipSetFieldImage(hChip,FSTB0899_UWP_MISS_THRESHOLD,UWPparams.MissThreshold);*/
	val=(UWPparams.AveFramesCoarse & 0xFF)+((UWPparams.AveframesFine & 0xFF)<<8)+((UWPparams.MissThreshold & 0xFF)<<16);
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_UWPCNTRL2,val); 
	
	/*ChipSetFieldImage(hChip,FSTB0899_UWP_THRESHOLD_ACQ,UWPparams.ThresholdAcq);
	ChipSetFieldImage(hChip,FSTB0899_UWP_THRESHOLD_TRACK,UWPparams.ThresholdTrack);*/
	val=(UWPparams.ThresholdAcq & 0x7FFF)+ ((UWPparams.ThresholdTrack & 0x7FFF)<<15);
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_UWPCNTRL3,val);
	
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_SOFSRCHTO,UWPparams.SofSearchTimeout);
}

/*****************************************************
**FUNCTION	::	FE_DVBS2_GetUWPstate
**ACTION	::	Read the UWP status locked or not
**PARAMS IN	::  hChip		==>	handle to the chip
			::	TimeOut		==> timeout to wait for UWP lock
**PARAMS OUT::	NONE
**RETURN	::	UWP state
*****************************************************/
int FE_DVBS2_GetUWPstate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,int TimeOut)
{
	int Time=0,
		locked=0;
		
	
	while((Time<TimeOut)&& (locked==0))
	{
		
		locked=STTUNER_IOREG_GetField_SizeU32(DeviceMap,IOHandle,FSTB0899_UWP_LOCK,FSTB0899_UWP_LOCK_INFO);
		WAIT_N_MS_899(1);
		Time++;
	}

	return locked;
}


/*****************************************************
**FUNCTION	::	FE_DVBS2_GetModCod
**ACTION	::	Read the found MODCODE
**PARAMS IN	::  hChip		==>	handle to the chip
**PARAMS OUT::	NONE
**RETURN	::	Found MODCODE
*****************************************************/
U32	FE_DVBS2_GetModCod(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	return(STTUNER_IOREG_GetField_SizeU32(DeviceMap,IOHandle,FSTB0899_UWP_DECODED_MODCODE,FSTB0899_UWP_DECODED_MODCODE_INFO)>>2);
}


/*****************************************************
**FUNCTION	::	FE_DVBS2_GetUWPEsNo
**ACTION	::	Read the Es/N0 indicator
**PARAMS IN	::  hChip		==>	handle to the chip
**PARAMS OUT::	NONE
**RETURN	::	Es/N0
*****************************************************/
S32 FE_DVBS2_GetUWPEsNo(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,S32 Quant)
{

	U32	tempus;
	
	tempus=STTUNER_IOREG_GetField_SizeU32(DeviceMap,IOHandle,FSTB0899_ESN0_ESR,FSTB0899_ESN0_ESR_INFO);
	return tempus; /* to convert value to db tempus=(10*log10(tempus)/(quant^2)*/
				   /* with quant = UWP_ESN0_QUANT field value 				  */

}

					

/*****************************************************
**FUNCTION	::	FE_DVBS2_AutoConfigCSM
**ACTION	::	Set the CSM to Automatic mode
**PARAMS IN	::  hChip		==>	handle to the chip
**PARAMS OUT::	NONE
**RETURN	::	None
*****************************************************/
void FE_DVBS2_AutoConfigCSM(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	/* to auto=config write a 1 to auto_param register */
	/*Set filed image value*/
	STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_AUTO_PARAM,FSTB0899_AUTO_PARAM_INFO,1);
}



/*****************************************************
**FUNCTION	::	FE_DVBS2_AutoConfigCSM
**ACTION	::	Set the CSM to Manual mode. 
**PARAMS IN	::  hChip		==>	handle to the chip
			::  CSMParams	==>	CSM parameters 
**PARAMS OUT::	NONE
**RETURN	::	None
*****************************************************/
void FE_DVBS2_ManualConfigCSM(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_DVBS2_CSMConfig_Params_t CSMParams)
{
	U32 val;
	/* to manually config write a 0 to auto_param register*/

	val=(CSMParams.DvtTable&0x01)+ ((CSMParams.TwoPass & 0x01)<<1)+((CSMParams.AgcGain & 0x1ff)<<2)+
		((CSMParams.AgcShift & 0x7)<<11)+ ((CSMParams.FeLoopShift & 0x7)<<14);
	
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_CSMCNTRL1,val);
		
	/*configure other registers*/
	val=(CSMParams.GammaAcq & 0xf)+ ((CSMParams.GammaRhoAcq & 0x1ff)<<8 );
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_CSMCNTRL2,val);

	val=(CSMParams.GammaTrack & 0xf)+ ((CSMParams.GammaRhoTrack & 0x1ff)<<8 );
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_CSMCNTRL3,val); 
	
	val=(CSMParams.LockCountThreshold & 0xf)+ ((CSMParams.PhaseDiffThreshold & 0x7f)<<8 );
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_CSMCNTRL4,val); 
}


/*****************************************************
**FUNCTION	::	FE_DVBS2_CSMInitialize
**ACTION	::	Set the parameters for Manual mode. 
				Should be used when QPSK pilots on and Mclk/SymbolRate<=4
**PARAMS IN	::  hChip		==>	handle to the chip
			::  Pilots		==>	Pilots On/Off
			::  ModCode		==>	found MODCODE
			::  SymbolRate	==>	Symbol Rate in Symb/s 
			::  MasterClock ==>	Demod Clock in Hz
**PARAMS OUT::	NONE
**RETURN	::	None
*****************************************************/
void FE_DVBS2_CSMInitialize(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,int Pilots,FE_DVBS2_ModCod_t ModCode,U32 SymbolRate,U32 MasterClock)
{
	FE_DVBS2_CSMConfig_Params_t csmParams;
	
	csmParams.DvtTable=1;
	csmParams.TwoPass=0;
	csmParams.AgcGain=6;
	csmParams.AgcShift=0;
	csmParams.FeLoopShift=0;
	csmParams.PhaseDiffThreshold=0x80;
	
	if( ((MasterClock/SymbolRate)<=4)&&(ModCode<=11)&&(Pilots==1))
	{
		switch (ModCode)
		{
			case FE_QPSK_12:
				csmParams.GammaAcq=25;  
				csmParams.GammaRhoAcq=2700;
				csmParams.GammaTrack=12;  
				csmParams.GammaRhoTrack=180; 
				csmParams.LockCountThreshold=8;
			break;
			
			case FE_QPSK_35:
				csmParams.GammaAcq=38;  
				csmParams.GammaRhoAcq=7182;
				csmParams.GammaTrack=14; 
				csmParams.GammaRhoTrack=308; 
				csmParams.LockCountThreshold=8;
			break;
			
			case FE_QPSK_23:
				csmParams.GammaAcq=42;  
				csmParams.GammaRhoAcq=9408;
				csmParams.GammaTrack=17;  
				csmParams.GammaRhoTrack=476; 
				csmParams.LockCountThreshold=8;
			break;
			
			case FE_QPSK_34:
				csmParams.GammaAcq=53;
				csmParams.GammaRhoAcq=16642;
				csmParams.GammaTrack=19;
				csmParams.GammaRhoTrack=646;
				csmParams.LockCountThreshold=8;
			break;
			
			case FE_QPSK_45:
				csmParams.GammaAcq=53;
				csmParams.GammaRhoAcq=17119;
				csmParams.GammaTrack=22;
				csmParams.GammaRhoTrack=880;
				csmParams.LockCountThreshold=8;
			break;
			
			case FE_QPSK_56:
				csmParams.GammaAcq=55;
				csmParams.GammaRhoAcq=19250;
				csmParams.GammaTrack=23;
				csmParams.GammaRhoTrack=989;
				csmParams.LockCountThreshold=8;
			break;
			
			case FE_QPSK_89:
				csmParams.GammaAcq=60;
				csmParams.GammaRhoAcq=24240;
				csmParams.GammaTrack=24;
				csmParams.GammaRhoTrack=1176;
				csmParams.LockCountThreshold=8;
			break;
			
			case FE_QPSK_910:
				csmParams.GammaAcq=66;
				csmParams.GammaRhoAcq=29634;
				csmParams.GammaTrack=24;
				csmParams.GammaRhoTrack=1176;
				csmParams.LockCountThreshold=8;
			break;
			
			default:
				csmParams.GammaAcq=66;
				csmParams.GammaRhoAcq=29634;
				csmParams.GammaTrack=24;
				csmParams.GammaRhoTrack=1176;
				csmParams.LockCountThreshold=8;
			break;
			
		}
		
	FE_DVBS2_ManualConfigCSM(DeviceMap,IOHandle,csmParams);
	}
	
}


/*****************************************************
**FUNCTION	::	FE_DVBS2_GetCSMLock
**ACTION	::	Read the CSM status locked or not
**PARAMS IN	::  hChip		==>	handle to the chip
			::	TimeOut		==> timeout to wait for CSM lock
**PARAMS OUT::	NONE
**RETURN	::	CSM state
*****************************************************/
int FE_DVBS2_GetCSMLock(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,int TimeOut)
{
	int Time=0,
		CSMLocked=0;

	while((!CSMLocked)&&(Time<TimeOut))
	{
		CSMLocked=STTUNER_IOREG_GetField_SizeU32(DeviceMap,IOHandle,FSTB0899_CSM_LOCK,FSTB0899_CSM_LOCK_INFO);
		/*WAIT_N_MS_899(1);*/
		Time++; 
	}
	
	return CSMLocked; 

}

/*****************************************************
--FUNCTION	::	CarrierWidth
--ACTION	::	Compute the width of the carrier
--PARAMS IN	::	SymbolRate	->	Symbol rate of the carrier (Kbauds or Mbauds)
--				RollOff		->	Rolloff * 100
--PARAMS OUT::	NONE
--RETURN	::	Width of the carrier (KHz or MHz) 
--***************************************************/
long FE_DVBS2_CarrierWidth(long SymbolRate,FE_DVBS2_RRCAlpha_t Alpha)
{
	long RollOff=0;
	
	switch(Alpha)
	{
		case RRC_20:
			RollOff=20;
		break;
		case RRC_25:
			RollOff=25;
		break;
		case RRC_35:
			RollOff=35;
		break;
		
	}
	return (SymbolRate  + (SymbolRate*RollOff)/100);
}


/*****************************************************
**FUNCTION	::	FE_DVBS2_GetDataLock
**ACTION	::	Read the FEC status for data lock
**PARAMS IN	::  hChip		==>	handle to the chip
			::	TimeOut		==> timeout to wait for data lock
**PARAMS OUT::	NONE
**RETURN	::	Data Lock status
*****************************************************/
int FE_DVBS2_GetDataLock(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,int TimeOut)
{
	int Time=0,
	DataLocked=0;

	while((!DataLocked)&&(Time<TimeOut))
	{
		DataLocked=STTUNER_IOREG_GetField_SizeU32(DeviceMap,IOHandle,FSTB0899_LOCK,FSTB0899_LOCK_INFO);
		/*WAIT_N_MS_899(1);*/
		Time++; 
	}
	
	return DataLocked; 

	
}


/*****************************************************
**FUNCTION	::	FE_DVBS2_GetDemodStatus
**ACTION	::	Return the DVBS2 DEMOD status Locked or not 
**PARAMS IN	::  hChip		==>	handle to the chip
			::	timeout		==> timeout to wait for DEMOD lock.
**PARAMS OUT::	NONE
**RETURN	::	DEMOD STATE
*****************************************************/
FE_DVBS2_State FE_DVBS2_GetDemodStatus(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,int timeout)
{
	int time=0;
	BOOL Locked=FALSE;
		
	do
	{   
		if(STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,RSTB0899_DMDSTAT2)==0x03) /*CSM_LOCK == 1 and UWP_LOCK ==1*/
		Locked=TRUE;
		time+=3;
		WAIT_N_MS_899(1);
		
	}while((Locked==FALSE) &&(time<=timeout));

	if(Locked)
		return FE_DVBS2_DEMOD_LOCKED;
	else	
		return FE_DVBS2_DEMOD_NOT_LOCKED;
}


/*****************************************************
**FUNCTION	::	FE_DVBS2_GetFecStatus
**ACTION	::	Return the DVBS2 FEC status Locked or not 
**PARAMS IN	::  hChip		==>	handle to the chip
			::	timeout		==> timeout to wait for FEC lock.
**PARAMS OUT::	NONE
**RETURN	::	FEC STATE
*****************************************************/
FE_DVBS2_State FE_DVBS2_GetFecStatus(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,int timeout)
{
	int time=0,
		Locked;
	
	do
	{
		Locked=FE_DVBS2_GetDataLock(DeviceMap,IOHandle,1);
		time++;
		WAIT_N_MS_899(1);
		
	}while((Locked==FALSE) &&(time<timeout));

	if(Locked)
		return FE_DVBS2_DATAOK;
	else
		return FE_DVBS2_FEC_NOT_LOCKED;
}

/*****************************************************
**FUNCTION	::	FE_DVBS2_InitialCalculations
**ACTION	::	Initialize DVBS2 UWP, CSM, carrier 
				and timing loop for  Acquisition 
**PARAMS IN	::  hChip		==>	handle to the chip
			::	InitParams	==> Initializing parameters.
**PARAMS OUT::	NONE
**RETURN	::	None
*****************************************************/
void FE_DVBS2_InitialCalculations(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_STB0899_DVBS2_InitParams_t *InitParams)
{


	FE_DVBS2_UWPConfig_Params_t	uwpParams;
	FE_DVBS2_LoopBW_Params_t loopBW;
	
	S32 numSteps,
	freqStepSize,
	acqcntrl2;
	

	

	uwpParams.EsNoAve=ESNO_AVE;
	uwpParams.EsNoQuant=ESNO_QUANT;
	uwpParams.AveFramesCoarse=InitParams->AveFrameCoarse;
	uwpParams.AveframesFine =InitParams->AveFramefine;
	uwpParams.MissThreshold=MISS_THRESHOLD;
	uwpParams.ThresholdAcq=UWP_THRESHOLD_ACQ;
	uwpParams.ThresholdTrack=UWP_THRESHOLD_TRACK;
	uwpParams.ThresholdSof=UWP_THRESHOLD_SOF;
	uwpParams.SofSearchTimeout=SOF_SEARCH_TIMEOUT;
	
	loopBW.LoopBwPercent=60;
	loopBW.SymbolRate=InitParams->SymbolRate;
	loopBW.MasterClock=InitParams->MasterClock;
	loopBW.Mode=InitParams->ModeMode;
	loopBW.Zeta=707;
	loopBW.SymPeakVal=23;/**5.76*/
	
	/* config uwp and csm */ 
	FE_DVBS2_ConfigUWP(DeviceMap,IOHandle,uwpParams);
	FE_DVBS2_AutoConfigCSM(DeviceMap,IOHandle);

	/* initialize BTR	*/
	FE_DVBS2_SetSymbolRate(DeviceMap,IOHandle,InitParams->SymbolRate,InitParams->MasterClock);
	FE_DVBS2_SetBtrLoopBW(DeviceMap,IOHandle,loopBW);
	
	if(InitParams->SymbolRate/1000000 >=15)
		freqStepSize =(1<<17)/5;
	else if(InitParams->SymbolRate/1000000 >=10)
		freqStepSize =(1<<17)/7;
		
	else if(InitParams->SymbolRate/1000000 >=5)
		freqStepSize =(1<<17)/10;
	else
		freqStepSize =(1<<17)/3;

	numSteps = (10* InitParams->FreqRange*(1<<17))/(freqStepSize * (InitParams->SymbolRate/1000000));
	numSteps =(numSteps+6)/10;
	numSteps = (numSteps ==0)?1:numSteps; 
	   
	if(numSteps%2 == 0)
		FE_DVBS2_SetCarrierFreq(DeviceMap,IOHandle,InitParams->CarrierFrequency-(InitParams->StepSize*(InitParams->SymbolRate/20000000)),(InitParams->MasterClock)/1000000);
	else
		FE_DVBS2_SetCarrierFreq(DeviceMap,IOHandle,InitParams->CarrierFrequency,(InitParams->MasterClock)/1000000);
			
	/*Set Carrier Search params (zigzag, num steps and freq step size*/
	acqcntrl2=(1<<25) |(numSteps<<17) |(freqStepSize);
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_ACQCNTRL2,acqcntrl2);

}



/*****************************************************
**FUNCTION	::	FE_DVBS2_Reacquire
**ACTION	::	Trig a DVBS2 Acquisition 
**PARAMS IN	::  hChip		==>	handle to the chip				
**PARAMS OUT::	NONE
**RETURN	::	None
*****************************************************/
void FE_DVBS2_Reacquire(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	
			
	/* demod soft reset */
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_RESETCNTRL,1);

		
	/*Reset Timing Loop*/
	FE_DVBS2_BtrInit(DeviceMap,IOHandle);
	/* reset Carrier loop	*/
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_CRLFREQINIT,1<<30);
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_CRLFREQINIT,0); 
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_CRLLOOPGAIN,0);
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_CRLPHSINIT,1<<30);
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_CRLPHSINIT,0);
	
	/*release demod soft reset */
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_RESETCNTRL,0);

	/* start acquistion process  */
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_ACQUIRETRIG,1);
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_LOCKLOST,0);
	
	/* equalizer Init*/
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_EQUILIZERINIT,1);

	/*Start equilizer*/
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_EQUILIZERINIT,0); 
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_EQCNTL,0x054800);

	
	/*Reset Packet Delin*/
	STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_ALGOSWRST,FSTB0899_ALGOSWRST_INFO,1);
	STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_ALGOSWRST,FSTB0899_ALGOSWRST_INFO,0);
	STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_HYSTSWRST,FSTB0899_HYSTSWRST_INFO,1);
	STTUNER_IOREG_SetField_SizeU32(DeviceMap, IOHandle,FSTB0899_HYSTSWRST,FSTB0899_HYSTSWRST_INFO,0);
	STTUNER_IOREG_SetRegister(DeviceMap, IOHandle,RSTB0899_PDELCTRL,0x4a);

}

