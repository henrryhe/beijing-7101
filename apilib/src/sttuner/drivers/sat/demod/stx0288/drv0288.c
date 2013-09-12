/*--------------------------------------------------------------------------
File Name: 288_drv.c

Description: 288 driver LLA

Copyright (C) 2005-2006 STMicroelectronics

author: SD
---------------------------------------------------------------------------- */


/* includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
   #include  <stdlib.h>
#endif
#include "sttbx.h"
/* common includes */ 
#include "drv0288.h"
#include "util288.h"
#include "sysdbase.h"
#include "mtuner.h"
#include "tunsdrv.h"
#include "chip.h"


/*#define STTUNER_DEBUG_MODULE_SATDRV_D0288 1*/


extern TUNSDRV_InstanceData_t *TUNSDRV_GetInstFromHandle(TUNER_Handle_t Handle);
extern IOARCH_HandleData_t IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];

/* Current LLA revision	*/
static const ST_Revision_t Revision288  = "STX0288-LLA_REL_1.9.8";

static FE_288_LOOKUP_t FE_288_RF_LookUp = {
						22,
						{
							{-101,	127},
							{-104,	118},
							{-110,	111},
							{-130,	100},
							{-160,	91},
							{-180,	87},
							{-200,	84},
							{-250,	76},
							{-300,	68},
							{-350,	63},
							{-400,	56},
							{-450,	48},
							{-500,	40},
							{-550,	29},
							{-600,	11},
							{-650,	-22},
							{-700,	-56},
							{-750,	-71},
							{-800,	-82},
							{-850,	-91},
							{-883,	-96},
							{-888,	-128},
						}
						 
					     };
									
static FE_288_LOOKUP_t FE_288_CN_LookUp = {
						31,
						{
							{20, 8900},
							{25,	8680},
							{30,	8420},
							{35,	8217},
							{40,	7897},
							{50,	7333},
							{60,	6747},
							{70,	6162},
							{80,	5580},
							{90,	5029},
							{100,4529},
							{110,4080},
							{120,3685},
							{130,3316},
							{140,2982},
							{150,2688},
							{160,2418},
							{170,2188},
							{180,1982},
							{190,1802},
							{200,1663},
							{210,1520},
							{220,1400},
							{230,1295},
							{240,1201},
							{250,1123},
							{260,1058},
							{270,1004},
							{280,957},
							{290,920},
							{300,890}
						}
					     };
					     
U8 FE_288_VTH_Search[6] =	{0x32,0x1E,0x16,0x0F,0x0D,0x0A};
 
U8 FE_288_VTH_Tracking[6] = {0x3C,0x28,0x1E,0x13,0x12,0x0C};

ST_Revision_t FE_288_GetLLARevision(void)
{
	return (Revision288);
}

/*****************************************************
**FUNCTION	::	BinaryFloatDiv
**ACTION	::	float division (with integer) 
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE
**RETURN	::	Derotator frequency (KHz)
*****************************************************/
long FE_288_BinaryFloatDiv(long n1, long n2, int precision)
{
	int i=0;
	long result=0;
	
	/*	division de N1 par N2 avec N1<N2	*/
	while(i<=precision) /*	n1>0	*/
	{
		if(n1<n2)
		{
			result<<=1;      
			n1<<=1;
		}
		else
		{
			result=(result<<1)+1;
			n1=(n1-n2)<<1;
		}
		i++;
	}
	
	return result;
}

/*****************************************************
**FUNCTION	::	FE_288_XtoPowerY
**ACTION	::	Compute  x^y (where x and y are integers) 
**PARAMS IN	::	Number -> x
**				Power -> y
**PARAMS OUT::	NONE
**RETURN	::	2^n
*****************************************************/
U32 FE_288_XtoPowerY(U32 Number, U32 Power)
{
	int i;
	long result = 1;
	
	for(i=0;i<Power;i++)
		result *= Number;
		
	return (U32)result;
}


/*****************************************************
**FUNCTION	::	FE_288_PowOf2
**ACTION	::	Compute  2^n (where n is an integer) 
**PARAMS IN	::	number -> n
**PARAMS OUT::	NONE
**RETURN	::	2^n
*****************************************************/
long FE_288_PowOf2(int number)
{
	int i;
	long result=1;
	
	for(i=0;i<number;i++)
		result*=2;
		
	return result;
}

/*****************************************************
**FUNCTION	::	GetPowOf2
**ACTION	::	Compute  x according to y with y=2^x 
**PARAMS IN	::	number -> y
**PARAMS OUT::	NONE
**RETURN	::	x
*****************************************************/
long FE_288_GetPowOf2(int number)
{
	int i=0;	
	
	while(FE_288_PowOf2(i)<number) i++;		
	
	return i;
}

/*****************************************************
**FUNCTION	::	FE_288_GetMclkFreq
**ACTION	::	Set the STX0288 master clock frequency
**PARAMS IN	::  DeviceMap		==>	handle to the chip
**				ExtClk		==>	External clock frequency (Hz)
**PARAMS OUT::	NONE
**RETURN	::	MasterClock frequency (Hz)
*****************************************************/
U32 FE_288_GetMclkFreq( IOARCH_Handle_t IOHandle, U32 ExtClk_Hz)
{
	U32 mclk_Hz,			/* master clock frequency (Hz) */ 
		pll_divider,		                /* pll divider */
		pll_selratio,		/* 4 or 6 ratio */
		pll_bypass;		/* pll bypass */ 
	
	pll_divider = ChipGetField( IOHandle, F288_PLL_MDIV)+1;
	pll_selratio=(ChipGetField(IOHandle,F288_PLL_SELRATIO)?4:6);
	pll_bypass=ChipGetField(IOHandle,F288_BYPASS_PLL);
	if(pll_bypass)
		mclk_Hz=ExtClk_Hz;
	else
		mclk_Hz=(ExtClk_Hz*pll_divider)/pll_selratio;
	
	return mclk_Hz;
}

U32 FE_288_SetMclkFreq( IOARCH_Handle_t IOHandle, U32 ExtClk_Hz, U32 MasterClock)
{
	U32 			/* master clock frequency (Hz) */ 
		pll_divider,		/* pll divider */
		pll_selratio;		/* 4 or 6 ratio */
				/* pll bypass */ 
	
	if(ExtClk_Hz == 4000000)
	pll_selratio = 1;
	else
	pll_selratio = 0;
	pll_divider = ((MasterClock/1000000)*(2*(1-pll_selratio) + 4))/(ExtClk_Hz/1000000);
	
	ChipSetField( IOHandle, F288_PLL_MDIV, (pll_divider-1));
	ChipSetField( IOHandle, F288_PLL_SELRATIO, pll_selratio);
	
	MasterClock = FE_288_GetMclkFreq( IOHandle, ExtClk_Hz);
	return MasterClock;
}


/*****************************************************
**FUNCTION	::	FE_288_AuxClkFreq
**ACTION	::	Compute Auxiliary clock frequency 
**PARAMS IN	::	DigFreq		==> Frequency of the digital synthetizer (Hz)
**				Prescaler	==> value of the prescaler
**				Divider		==> value of the divider
**PARAMS OUT::	NONE
**RETURN	::	Auxiliary Clock frequency
*****************************************************/
U32 FE_288_AuxClkFreq(U32 DigFreq,U32 Prescaler,U32 Divider)
{
	U32 aclk,
		factor;
	
	switch(Prescaler)
	{
		case 0:
			aclk = 0;
		break;
	
		case 1:
			if(Divider)
				aclk = DigFreq/2/Divider;
			else
				aclk = DigFreq/2;
		break;
		
		default:
			factor = (U32)(2048L*FE_288_PowOf2(Prescaler-2));
			if(factor)
				aclk = DigFreq/factor/(32+Divider);
			else
				aclk = 0;
		break;
	}
	
	return aclk; 
}

/*****************************************************
**FUNCTION	::	FE_288_F22Freq
**ACTION	::	Compute F22 frequency 
**PARAMS IN	::	DigFreq	==> Frequency of the digital synthetizer (Hz)
**				F22		==> Content of the F22 register
**PARAMS OUT::	NONE
**RETURN	::	F22 frequency
*****************************************************/
U32 FE_288_F22Freq(U32 DigFreq,U32 F22)
{
	return (F22!=0) ? (DigFreq/(32*F22)) : 0; 
}

/*****************************************************
**FUNCTION	::	FE_288_GetErrorCount
**ACTION	::	return the number of error
**PARAMS IN	::	Params		=>	Pointer to STTUNER_IOREG_DeviceMap_t
**				Counter		=>	Counter to use
**PARAMS OUT::	NONE
**RETURN	::	number of errors
*****************************************************/
U32 FE_288_GetErrorCount( IOARCH_Handle_t IOHandle, FE_288_ERRORCOUNTER_t Counter)
{
	U32 lsb,msb;
	
		
	/*Do not modified the read order (lsb first)*/
	if(Counter == COUNTER1_288)
	{
		
		lsb = ChipGetField( IOHandle,F288_ERROR_COUNT_LSB);
		msb = ChipGetField( IOHandle,F288_ERROR_COUNT_MSB);
	}
	else
	{
		
		lsb = ChipGetField( IOHandle,F288_ERROR_COUNT2_LSB);
		msb = ChipGetField( IOHandle,F288_ERROR_COUNT2_MSB);
	}
	
	return (MAKEWORD_288(msb,lsb));
}

/****************************************************
--FUNCTION	::	FE_288_GetPacketErrors
--ACTION	::	Set error counter in packet error mode and check for packet errors
--PARAMS IN	::	NONE
--PARAMS OUT::	NONE
--RETURN	::	Content of error count
--**************************************************/
U32 FE_288_GetDirectErrors( IOARCH_Handle_t IOHandle,FE_288_ERRORCOUNTER_t Counter)
{
	U32 BitErrors=0;
	unsigned char CounterMode, i;
	
	switch(Counter)
	{
		case COUNTER1_288:
			CounterMode = ChipGetOneRegister(IOHandle,R288_ERRCTRL);		/* Save error counter mode */
			ChipSetOneRegister( IOHandle, R288_ERRCTRL, 0x80);  			/* Packet Errors --Error counter in Rate mode, packet error, count period 2^12	*/
			/* Do not remove the for loop : bug work around */
			
			for(i=0;i<2;i++)
			BitErrors = FE_288_GetErrorCount(IOHandle,COUNTER1_288);	/* Error counter must be read twice before returning valid value */ 
			ChipSetOneRegister(IOHandle,R288_ERRCTRL,CounterMode);		/* restore error counter mode	*/
		break;
		
		case COUNTER2_288:
			CounterMode = ChipGetOneRegister(IOHandle,R288_ERRCTRL2);		/* Save error counter mode */
			ChipSetOneRegister(IOHandle,R288_ERRCTRL2,0xa0);  				/* Error counter in Rate mode, packet error, count period 2^12	*/
			for(i=0;i<2;i++)
			BitErrors = FE_288_GetErrorCount(IOHandle,COUNTER2_288);	/* Error counter must be read twice before returning valid value */ 
		
			ChipSetOneRegister(IOHandle,R288_ERRCTRL2,CounterMode);		/* restore error counter mode	*/
		break;
		case COUNTER3_288:
		
		break;
	}
	
	return BitErrors;
}

/*****************************************************
--FUNCTION	::	FE_288_TimingTimeConstant
--ACTION	::	Compute the amount of time needed by the timing loop to lock
--PARAMS IN	::	SymbolRate	->	symbol rate value
--PARAMS OUT::	NONE
--RETURN	::	Timing loop time constant (ms)
--***************************************************/
long FE_288_TimingTimeConstant(long SymbolRate)
{
	if(SymbolRate > 0)
		return (40000/(SymbolRate/1000));
	else
		return 0;
}

/*****************************************************
--FUNCTION	::	FE_288_DerotTimeConstant
--ACTION	::	Compute the amount of time needed by the Derotator to lock
--PARAMS IN	::	SymbolRate	->	symbol rate value
--PARAMS OUT::	NONE
--RETURN	::	Derotator time constant (ms)
--***************************************************/
long FE_288_DerotTimeConstant(long SymbolRate)
{
	if(SymbolRate > 0)  
		return (40000/(SymbolRate/1000));
	else
		return 0;
}

/*****************************************************
--FUNCTION	::	FE_288_DataTimeConstant
--ACTION	::	Compute the amount of time needed to capture data 
--PARAMS IN	::	Er		->	Viterbi rror rate	
--				Sn		->  viterbi averaging period
--				To		->  viterbi time out
--				Hy		->	viterbi hysteresis
--				SymbolRate	->	symbol rate value
--PARAMS OUT::	NONE
--RETURN	::	Data time constant
--***************************************************/
long FE_288_DataTimeConstant( IOARCH_Handle_t IOHandle,long SymbolRate)
{
	U32	Tviterbi = 0,
		TimeOut	= 0,
		THysteresis	= 0,
		Tdata = 0,
		PhaseNumber[6] = {2,6,4,6,14,8},
		averaging[4] = {1024,4096,16384,65536},
		InnerCode = 1000,
		HigherRate = 1000;
		 	
	U32 	i;
	U8 Pr,Sn,To,Hy;
		 	
	/*=======================================================================
	-- Data capture time (in ms)
    -- -------------------------
	-- This time is due to the Viterbi synchronisation.
	--
	--	For each authorized inner code, the Viterbi search time is calculated,
	--	and the results are cumulated in ViterbiSearch.	
	--  InnerCode is multiplied by 1000 in order to obtain timings in ms 
	=======================================================================*/
	Pr=ChipGetOneRegister(IOHandle,R288_PR);
	ChipGetOneRegister(IOHandle,R288_VAVSRCH);
	
	Sn=ChipGetField(IOHandle,F288_SN);
	To=ChipGetField(IOHandle,F288_TO); 
	Hy=ChipGetField(IOHandle,F288_H);
	
	
	
	
	for(i=0;i<6;i++)
	{
		if (((Pr >> i)& 0x01) == 0x01)
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
			
			Tviterbi += (2*PhaseNumber[i]*averaging[Sn]*InnerCode);
			
			if(HigherRate < InnerCode) 
				HigherRate = InnerCode;
		}
	}
	
	/*	  Time out calculation (TimeOut)
	--    ------------------------------
	--    This value indicates the maximum duration of the synchro word research.	*/
	TimeOut   = (U32)(HigherRate * 16384L * (1L<<To));  /* 16384= 16x1024 bits */
	
	/*    Hysteresis duration (Hysteresis)
	--    ------------------------------	*/
	THysteresis = (U32)(HigherRate * 26112L * (1L<<Hy));	/*	26112= 16x204x8 bits  */ 
	
	Tdata =((Tviterbi + TimeOut + THysteresis) / (2*(U32)SymbolRate));
	
	/* a guard time of 1 mS is added */
	return (1L + (long)Tdata);
}

/****************************************************
**FUNCTION	::	FE_288_GetRollOff
**ACTION	::	Read the rolloff value
**PARAMS IN	::	DeviceMap	==>	Handle for the chip
**PARAMS OUT::	NONE
**RETURN	::	rolloff
*****************************************************/
int  FE_288_GetRollOff( IOARCH_Handle_t IOHandle)
{
	if (ChipGetField( IOHandle, F288_MODE_COEF) == 1)
		return 20;
	else
		return 35;
}

/*****************************************************
**FUNCTION	::	FE_288_CalcDerotFreq
**ACTION	::	Compute Derotator frequency 
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE
**RETURN	::	Derotator frequency (KHz)
*****************************************************/
S32 FE_288_CalcDerotFreq(U8 derotmsb,U8 derotlsb,U32 fm)
{
	S32	dfreq;
	S32 Itmp;
		/*printf("\nthe fm is %d\n",fm);*/
	Itmp = (S16)(derotmsb<<8)+derotlsb;
	/*printf("\n the cfrm  value is 0x%x",Itmp);*/
	dfreq = (S32)(Itmp*(fm/10000L));
	dfreq = (S32)(dfreq / 65536L);
	dfreq *= 10;
	
	return dfreq; 
}

/*****************************************************
**FUNCTION	::	FE_288_GetDerotFreq
**ACTION	::	Read current Derotator frequency 
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE
**RETURN	::	Derotator frequency (KHz)
*****************************************************/
S32 FE_288_GetDerotFreq( IOARCH_Handle_t IOHandle,U32 MasterClock)
{
	U8 cfrm[2];
	/*	Read registers	*/
	ChipGetRegisters(IOHandle,R288_CFRM,2,cfrm);   
	/*for( i=0;i<2;i++)*/
	/*printf("the CFRM value is  %x ",cfrm[i]);*/
	return (FE_288_CalcDerotFreq(cfrm[0],cfrm[1],MasterClock));  	
}

/*****************************************************
**FUNCTION	::	FE_288_SetDerotFreq
**ACTION	::	Set current Derotator frequency 
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE
**RETURN	::	NONE
*****************************************************/
void FE_288_SetDerotFreq( IOARCH_Handle_t IOHandle,U32 MasterClock_Hz,S32 DerotFreq_Hz)
{
	S16 s16;
	U8 TMP[2];
	
	s16=(S16)(DerotFreq_Hz/(S32)(MasterClock_Hz/65536L));
		
	TMP[0]=MSB_288(s16);
	TMP[1]=LSB_288(s16);

	
	/*	write registers	*/
	ChipSetRegisters(IOHandle,R288_CFRM,TMP,2); 
	
}

/*****************************************************
**FUNCTION	::	FE_288_CalcSymbolRate
**ACTION	::	Compute symbol frequency
**PARAMS IN	::	Hbyte	->	High order byte
**				Mbyte	->	Mid byte
**				Lbyte	->	Low order byte
**PARAMS OUT::	NONE
**RETURN	::	Symbol frequency
*****************************************************/
U32 FE_288_CalcSymbolRate(U32 MasterClock,U8 Hbyte,U8 Mbyte,U8 Lbyte)
{
	U32	Ltmp,
		Ltmp2,
		Mclk;

	Mclk = (U32)(MasterClock / 4096L);	/* MasterClock*10/2^20 */
	Ltmp = (((U32)Hbyte<<12)+((U32)Mbyte<<4))/16;
	Ltmp *= Mclk;
	Ltmp /=16;
	Ltmp2=((U32)Lbyte*Mclk)/256;     
	Ltmp+=Ltmp2;
	
	return Ltmp;
}

/*****************************************************
**FUNCTION	::	FE_288_SetSymbolRate
**ACTION	::	Set symbol frequency
**PARAMS IN	::	DeviceMap		->	handle to the chip
**				MasterClock	->	Masterclock frequency (Hz)
**				SymbolRate	->	symbol rate (bauds)
**PARAMS OUT::	NONE
**RETURN	::	Symbol frequency
*****************************************************/
U32 FE_288_SetSymbolRate( IOARCH_Handle_t IOHandle,U32 MasterClock,U32 SymbolRate)
{
	U32	U32Tmp;
	U8 TMP[3];
	U8 TMPUP[3];
	U8 TEMP[2];
	/*
	** in order to have the maximum precision, the symbol rate entered into
	** the chip is computed as the closest value of the "true value".
	** In this purpose, the symbol rate value is rounded (1 is added on the bit
	** below the LSB_288 )
	*/
	U32Tmp = (U32)FE_288_BinaryFloatDiv(SymbolRate,MasterClock,20);       
	
	TMP[0]=0x80 ;
	TMP[1]=0x00;
	TMP[2]=0x00;
	
	ChipSetRegisters(IOHandle,R288_SFRH,TMP,3);     	/* warning : symbol rate is set to Masterclock/2 before setting the real value !!!!! */     
	
	TMPUP[0]=(U32Tmp>>12)&0xFF;
	TMPUP[1]=(U32Tmp>>4)&0xFF;
	TMPUP[2]=(((U8)(U32Tmp&0x0F))<<4)&0xF0;
	ChipSetRegisters(IOHandle,R288_SFRH,TMPUP,3);   
	
	TEMP[0]=0;
	TEMP[1]=0;
	ChipSetRegisters(IOHandle,R288_RTFM,TEMP,2);	/* reset timing offset */  
	
	return(SymbolRate) ;
}


void FE_288_ManualAutocenter( IOARCH_Handle_t IOHandle)
{
	U32	U32Tmp;
	S16	S16Tmp;
	S32	tst;
	U8 sfrh[3], rtfm[2];
	
	/* Read timing and timing offset registers */
	ChipGetRegisters(IOHandle,R288_SFRH,3,sfrh);
	ChipGetRegisters(IOHandle,R288_RTFM,2,rtfm);
	
	
	U32Tmp  =  sfrh[0]<<12;
	U32Tmp +=  sfrh[1]<<4;  
	U32Tmp +=  (sfrh[2] & 0xf0)>>4;  

	/* inject offset into symbolrate */
	S16Tmp = MAKEWORD_288(rtfm[0],rtfm[1]); 
	tst = ((S32)(S16Tmp/8)*(S32)U32Tmp)/524288; 

	
	U32Tmp += tst;   
	
	sfrh[0]=(U8)((U32Tmp>>12))&0xFF;
	sfrh[1]=(U8)((U32Tmp>>4))&0xFF;
	sfrh[2]=(((U8)(U32Tmp&0x0F))<<4)&0xF0;

	ChipSetRegisters(IOHandle,R288_SFRH,sfrh,3);   
	
	rtfm[0]=0;
	rtfm[1]=0;
	ChipSetRegisters(IOHandle,R288_RTFM,rtfm,2);	/* reset timing offset */  
	
}


/*****************************************************
**FUNCTION	::	FE_288_GetSymbolRate
**ACTION	::	Get the current symbol rate
**PARAMS IN	::	DeviceMap		->	handle to the chip
**				MasterClock	->	Masterclock frequency (Hz)
**PARAMS OUT::	NONE
**RETURN	::	Symbol rate
*****************************************************/
U32 FE_288_GetSymbolRate( IOARCH_Handle_t IOHandle,U32 MasterClock)
{
	U8 sfrh[3];
	ChipGetRegisters(IOHandle,R288_SFRH,3,sfrh);
	return FE_288_CalcSymbolRate(	MasterClock,
					sfrh[0],
					sfrh[1],
					((sfrh[2]& 0xf0)>>4) );
									
    
}

/*****************************************************
--FUNCTION	::	CarrierWidth
--ACTION	::	Compute the width of the carrier
--PARAMS IN	::	SymbolRate	->	Symbol rate of the carrier (Kbauds or Mbauds)
--				RollOff		->	Rolloff * 100
--PARAMS OUT::	NONE
--RETURN	::	Width of the carrier (KHz or MHz) 
--***************************************************/
long FE_288_CarrierWidth(long SymbolRate, long RollOff)
{
	return (SymbolRate  + (SymbolRate*RollOff)/100);
}

/****************************************************
--FUNCTION	::	FE_288_CheckRange
--ACTION	::	Check if the founded frequency is in the correct range
--PARAMS IN	::	Params->BaseFreq =>	
--PARAMS OUT::	Params->State	=>	Result of the check
--RETURN	::	RANGEOK_288 if check success, OUTOFRANGE_288 otherwise 
--***************************************************/
FE_288_SIGNALTYPE_t FE_288_CheckRange( IOARCH_Handle_t IOHandle,FE_288_InternalParams_t *Params, U32 SymbolRate) 
{
	int	RangeOffset_Khz,
		TransponderFrequency_Khz;
		
	RangeOffset_Khz = Params->SearchRange/2000;  
	TransponderFrequency_Khz = Params->Frequency + ((S32)Params->TunerIQSense*FE_288_GetDerotFreq(IOHandle,Params->MasterClock));
	
		if((TransponderFrequency_Khz >= Params->BaseFreq - RangeOffset_Khz - 500)
		&& (TransponderFrequency_Khz <= Params->BaseFreq + RangeOffset_Khz + 500)) 
		
			{Params->State = RANGEOK_288;
			   #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
			printf("\nThe range is OK..\n");
			#endif
			}
		else
			{Params->State = OUTOFRANGE_288;}
		
	
	return Params->State;
}


void FE_288_NextSubRange(FE_288_InternalParams_t *Params)
{
    S32 OldSubRange;

    if(Params->SubDir > 0)
    {
        OldSubRange = Params->SubRange;
        Params->SubRange = MIN_288( (S32)(Params->SearchRange/2) - (Params->TunerOffset + Params->SubRange/2)  ,  Params->SubRange);
        if(Params->SubRange<=0)
        Params->SubRange = 0;
        Params->TunerOffset = Params->TunerOffset + ((OldSubRange + Params->SubRange)/2 );
     }

    Params->Frequency =  Params->BaseFreq + (Params->SubDir * Params->TunerOffset)/1000;
    Params->SubDir    = -Params->SubDir;
}

S16 FE_288_GetRFLevel( IOARCH_Handle_t IOHandle)
{
	S32 Imin, agcGain = 0, Imax, i, rfLevel = 0;
	FE_288_LOOKUP_t *lookup;
	lookup = &FE_288_RF_LookUp;
	
	if((lookup != NULL) && lookup->size)
	{
		agcGain = (S8)ChipGetField( IOHandle, F288_AGC1_VALUE);
		Imin = 0;
		Imax = lookup->size-1;		
		if(INRANGE_288(lookup->table[Imin].regval,agcGain,lookup->table[Imax].regval))
		{
			while((Imax-Imin)>1)
			{
				i=(Imax+Imin)/2; 
				if(INRANGE_288(lookup->table[Imin].regval,agcGain,lookup->table[i].regval))
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
			rfLevel = 0;
	}
	
	return rfLevel - 100;
}

/*****************************************************
--FUNCTION	::	FE_288_GetCarrierToNoiseRatio
--ACTION	::	Return the carrier to noise of the current carrier
--PARAMS IN	::	NONE	
--PARAMS OUT::	NONE
--RETURN	::	C/N of the carrier, 0 if no carrier 
--***************************************************/
S32 FE_288_GetCarrierToNoiseRatio( IOARCH_Handle_t IOHandle)
{
	S32 c_n = 0,
		regval,
		Imin,
		Imax,
		i;
		U8 nirm[2];
	FE_288_LOOKUP_t *lookup;
	lookup = &FE_288_CN_LookUp;
	if(ChipGetField( IOHandle, F288_CF))
	{
		if((lookup != NULL) && lookup->size)
		{
			ChipGetRegisters(IOHandle,R288_NIRM,2,nirm);
			regval = MAKEWORD_288(nirm[0],nirm[1]);
		
			Imin = 0;
			Imax = lookup->size-1;
			
			if(INRANGE_288(lookup->table[Imin].regval,regval,lookup->table[Imax].regval))
			{
				while((Imax-Imin)>1)
				{
					i=(Imax+Imin)/2; 
					if(INRANGE_288(lookup->table[Imin].regval,regval,lookup->table[i].regval))
						Imax = i;
					else
						Imin = i;
				}
				
				c_n =	((regval - lookup->table[Imin].regval)
						* (lookup->table[Imax].realval - lookup->table[Imin].realval)
						/ (lookup->table[Imax].regval - lookup->table[Imin].regval))
						+ lookup->table[Imin].realval;
			}
			else
				c_n = 100;
				
			if(c_n>=100)
			c_n = 100;
		}
	}
	
	return c_n;
}

U32 FE_288_GetError( IOARCH_Handle_t IOHandle)
{
	U32 ber = 0,i;
	
	ChipGetOneRegister(IOHandle,R288_ACLC);
	ChipGetOneRegister(IOHandle,R288_ERRCTRL); 
	ChipGetOneRegister(IOHandle,R288_VSTATUS);
	
	/* read Veterbi Bit Errors from 2^16 symbols*/
	ChipSetOneRegister(IOHandle,R288_ERRCTRL, 0x13);
	
	FE_288_GetErrorCount(IOHandle,COUNTER1_288); /* remove first counter value */
	/* Average 5 ber values */ 
	for(i=0;i<10;i++)
	{
		WAIT_N_MS_288(100);
		ber += FE_288_GetErrorCount(IOHandle,COUNTER1_288);
	}
	
	ber/=10;
		/*	Check for carrier	*/
	if(ChipGetField( IOHandle, F288_CF))
	{
		
		if(!ChipGetField(IOHandle,F288_ERRMODE)) 
		{
			/*	Error Rate	*/
			ber *= 9766;
				/*  theses two lines => ber = ber * 10^-7	*/
			ber /= (U32)FE_288_PowOf2(2 + 2*ChipGetField(IOHandle,F288_NOE));
			
			
			switch(ChipGetField(IOHandle,F288_ERR_SOURCE))
			{
				case 0 :				/*	QPSK bit errors	*/
					ber /= 8;
					
					switch(ChipGetField(IOHandle,F288_PR))
					{
						case	0:		/*	PR 1/2	*/
							ber *= 1;
							ber /= 2;
						break;
				
						case	1:		/*	PR 2/3	*/
							ber *= 2;
							ber /= 3;
						break;
				
						case	2:		/*	PR 3/4	*/
							ber *= 3;
							ber /= 4;
						break;
				
						case	3:		/*	PR 5/6	*/
							ber *= 5;
							ber /= 6;
						break	;
				
						case	4:		/*	PR 6/7	*/
							ber *= 6;
							ber /= 7;
						break;
				
						case	5:		/*	PR 7/8	*/
							ber *= 7;
							ber /= 8;
						break;
				
						default	:
							ber = 0;
						break;
				  	}
					break;
			
				case 1:		/*	Viterbi bit errors	*/
					ber /= 8;
				break;
		
				case 2:		/*	Viterbi	byte errors	*/
				break;
		  
				case 3:		/*	Packet errors	*/
					if(ChipGetField( IOHandle, F288_FECMODE) != 0x04)
						ber *= 204;	/* DVB */
					else
						ber *= 147; /* DirecTV */
				break;	
			}
		}
	}
	
	return ber;
}

S32 FE_288_Coarse( IOARCH_Handle_t IOHandle,U32 MasterClock_Hz,S32 *Offset_Khz, BOOL fine_scan)
{
	S32 Symbolrate = 0;
	
	ChipSetField(IOHandle,F288_FINE,0);
	ChipSetField(IOHandle,F288_AUTOCENTRE,0);
	ChipSetField(IOHandle,F288_COARSE,1);

	
	WAIT_N_MS_288(60);/* run coarse during 50ms */
	
	
	Symbolrate = (S32)FE_288_GetSymbolRate(IOHandle,MasterClock_Hz);	/* Get result of coarse algorithm */
	*Offset_Khz = FE_288_GetDerotFreq(IOHandle,MasterClock_Hz);	/* Read derotator offset */ 
	ChipSetField(IOHandle,F288_COARSE,0);		/* stop coarse algorithm */

	return Symbolrate;
}

void FE_288_Fine( IOARCH_Handle_t IOHandle,U32 MasterClock_Hz,U32 Symbolrate, BOOL fine_scan )
{
	U32	i=0,
		fmin = 0,
		fmax = 0;
		 U8 fminm[5];
		
	/* fine initialisation */
	if(fine_scan == TRUE)
	{
		if(Symbolrate>=10000000)
		{
			/* +/- 15% search range */
			fmin = ((((U32)(Symbolrate/1000))*((85*32768)/100))/(U32)(MasterClock_Hz/1000));   /* 2^15=32768*/
			fmax = ((((U32)(Symbolrate/1000))*((115*32768)/100))/(U32)(MasterClock_Hz/1000));
			FE_288_SetSymbolRate(IOHandle,MasterClock_Hz,(U32)((Symbolrate*11)/10));/*1.1*/
		}
		else
		{
			/* +/- 15% search range */
			fmin = ((((U32)(Symbolrate/(U32)1000))*((75*32768)/100))/(U32)(MasterClock_Hz/(U32)1000));   /* 2^15=32768*/
			fmax = ((((U32)(Symbolrate/1000))*((125*32768)/100))/(U32)(MasterClock_Hz/1000));
			FE_288_SetSymbolRate(IOHandle,MasterClock_Hz,(U32)((Symbolrate*12)/10));/*1.2*/
		}
		
    }
    else
    {   				
	        fmin = ((((U32)(Symbolrate/(U32)1000))*((95*32768)/100))/(U32)(MasterClock_Hz/(U32)1000));   /* 2^15=32768*/
			fmax = ((((U32)(Symbolrate/1000))*((105*32768)/100))/(U32)(MasterClock_Hz/1000));
	}

    fminm[0]=MSB_288((U32)fmin);
    fminm[0] |= 0x80;  /* set F288_STOP_ON_FMIN to 1 */
	fminm[1]=LSB_288((U32)fmin);
	fminm[2]=MSB_288((U32)fmax);
	fminm[3]=LSB_288((U32)fmax);
	fminm[4]=MAX_288(Symbolrate/1000000,1); /*	Refresh fine increment */
	ChipSetRegisters(IOHandle,R288_FMINM,fminm,5);	/*Update all fine registers*/
	ChipSetField(IOHandle,F288_FINE,1);	/* start fine algorithm */
	i=0;
	do
	{
		WAIT_N_MS_288(10); 
		i++;
	}while(ChipGetField(IOHandle,F288_FINE)/* && (i<1000)*/);	/* wait for end of fine algorithm */  

	ChipSetField(IOHandle,F288_FINE,0);
	   #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
	printf("\nIn FE_288_Fine (). It is stopped on Fmin = 1");
	#endif
}

S32 FE_288_Autocentre( IOARCH_Handle_t IOHandle)
{
	S32 timing,oldtiming=-1;
		
	U8	end=0,
		start=1, rtfm[2];
	
	do
	{
		ChipGetRegisters(IOHandle,R288_RTFM,2,rtfm);	/* read timing offset */
		timing = (short int)MAKEWORD_288(rtfm[0],rtfm[1]);
		
		if(ABS_288(timing)>300)
			FE_288_ManualAutocenter(IOHandle);
		
		if(!start)
		{
			if(ABS_288(oldtiming) < ABS_288(timing))
				end = 1;
		}
		else
			start = 0;
		
			
		oldtiming = timing;
		
				
	}while((ABS_288(timing)>300) && (!end)/*(timeout<100)*/);	/* timing loop is centered or timeout limit is reached */

	return timing;
}

BOOL FE_288_WaitLock( IOARCH_Handle_t IOHandle, S32 TData)
{
	S32 timeout=0;
	BOOL lock;
	
	/*********/
	S32 dtv;
	FE_288_Rate_t pr=0;
	U32	nbErr,maxPckError;	
	dtv=(ChipGetField(IOHandle,F288_FECMODE)==4);
	
	if(dtv)
	    ChipSetField(IOHandle,F288_SYM,0);	
	
	/*********/
	do
	{
		WAIT_N_MS_288(1);	/* wait 1 ms */
		timeout++;
		lock=ChipGetField(IOHandle,F288_LK);
	}while(!lock && (timeout<TData));
	
	
	/************/
	if(dtv)
	{
		/* workaround for IQ invertion in DIRECTV mode */
		WAIT_N_MS_288(4); 
		nbErr = FE_288_GetErrorCount( IOHandle,COUNTER2_288);
		maxPckError = 0x01<<(11+2*ChipGetField(IOHandle,F288_NOE2));	/* packet error rate = 50% */ 
		
		if(lock)
			pr = (1<<ChipGetField(IOHandle,F288_PR));
	
		if((!lock) || (lock && (pr == FE_288_6_7) && (nbErr>maxPckError)))
		{
			/* more than 50% packet errors rate ==> swap I and Q */
		
			timeout=0;
			ChipSetField(IOHandle,F288_SYM,1);
			do
			{
				WAIT_N_MS_288(1);	/* wait 1 ms */
				timeout++;
				lock=ChipGetField(IOHandle,F288_LK);
			}while(!lock && (timeout<TData));     
		}
	}
	
	/***********/
	return lock;
}

void FE_288_AcquisitionMode( IOARCH_Handle_t IOHandle) 
{

	
	ChipSetRegisters( IOHandle,R288_VTH0,FE_288_VTH_Search,6);/* Setup viterbi threshold to improve lock speed */
		
	ChipSetField(IOHandle,F288_SN,1);		/* Setup viterbi averaging to improve lock speed */					
	
	/* Setup timing loop for fast acquisition */
	ChipSetField(IOHandle,F288_ALPHA_TMG,7);	     
	ChipSetField(IOHandle,F288_BETA_TMG,10);
	/*ChipSetField(IOHandle,F288_ACLC,0);*/
	     
	
	/* Setup of carrier loop for fast acquisition */
	ChipSetField(IOHandle,F288_ALPHA,7);	     
	ChipSetField(IOHandle,F288_BETA,28);	     
	
}

void FE_288_TrackingMode( IOARCH_Handle_t IOHandle,S32 SymbolRate_Bds, FE_288_Rate_t PunctureRate)
{
	/* Optimize timing loop settings */
	/* optimize timing loop parameters */   
	ChipSetField(IOHandle,F288_ALPHA_TMG,3);
	ChipSetField(IOHandle,F288_BETA_TMG,3);
	

	/* optimize carrier loop parameters */
	if(SymbolRate_Bds<15000000)
	{
		switch(PunctureRate)
		{
			case FE_288_6_7:
				ChipSetField(IOHandle,F288_ALPHA,8);
				ChipSetField(IOHandle,F288_ACLC,1);
				ChipSetField(IOHandle,F288_BETA,29);
			break;
		
			case FE_288_1_2:     
				if(ChipGetField(IOHandle,F288_FECMODE)==4)
				{
					/* DirecTV mode*/
					ChipSetField(IOHandle,F288_ALPHA,9);
					ChipSetField(IOHandle,F288_ACLC,0);
					ChipSetField(IOHandle,F288_BETA,18);
				}
				else
				{
					/* DVB mode */
					ChipSetField(IOHandle,F288_ALPHA,8);
					ChipSetField(IOHandle,F288_ACLC,1);
					ChipSetField(IOHandle,F288_BETA,28);
				}
			break;
		
			case FE_288_2_3:
			case FE_288_3_4:
			case FE_288_5_6:
			case FE_288_7_8:
			default:
				ChipSetField(IOHandle,F288_ALPHA,8);
				ChipSetField(IOHandle,F288_ACLC,1);
				ChipSetField(IOHandle,F288_BETA,28);
			break;
		}
	}
	else if(SymbolRate_Bds<25000000)
	{
		ChipSetField(IOHandle,F288_ALPHA,8);
		ChipSetField(IOHandle,F288_ACLC,0);
		ChipSetField(IOHandle,F288_BETA,26);
	}
	else
	{
		ChipSetField(IOHandle,F288_ALPHA,7);
		ChipSetField(IOHandle,F288_ACLC,1);
		ChipSetField(IOHandle,F288_BETA,26);
	}
	
	/* Setup viterbi threshold to improve tracking */
	ChipSetRegisters( IOHandle,R288_VTH0,FE_288_VTH_Tracking,6);

	/* Setup viterbi averaging to improve tracking */
	ChipSetField(IOHandle,F288_SN,3);
}


FE_288_SIGNALTYPE_t FE_288_KnownAlgo( IOARCH_Handle_t IOHandle,	FE_288_InternalParams_t *Params, STTUNER_Handle_t TopLevelHandle)
{
	S32		frequency_Khz	= 0,
			zzDerotFreq_Hz	= 0,
			zzDerotStep_Hz	= 0,
			zzIndex		= 0;
			
			
	S16		timingTimCst_ms,
			derotTimCst_ms,
			dataTimCst_ms;
			
	U8		zzNextLoop;
			
	enum {ZZ_CARRIER_CHECK,ZZ_DATA_CHECK,ZZ_RANGE_CHECK,ZZ_NEXT,ZZ_END} state;
				
	FE_288_DIRECTION_t zzDirection = DIR_RIGHT;	/* start zig-zag on the right size */
			
	FE_288_SIGNALTYPE_t signalType = NOSIGNAL_288;
	
	frequency_Khz = Params->Frequency;
	   #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
	printf("\nIn Known_Algo Frequency = %d  \n",Params->Frequency);
	#endif
	/* Set timing loop parameters for fast acquisition */ 
	ChipSetField(IOHandle,F288_ALPHA_TMG,7);
	ChipSetField(IOHandle,F288_BETA_TMG,10);
	
	/* Optimization carrier loop parameters versus symbol rate */
	if(Params->Symbolrate<15000000)
	{
		ChipSetField(IOHandle,F288_ALPHA,8);
		ChipSetField(IOHandle,F288_ACLC,1);
		ChipSetField(IOHandle,F288_BETA,28);

	}
	else if(Params->Symbolrate<25000000)
	{
		ChipSetField(IOHandle,F288_ALPHA,8);
		ChipSetField(IOHandle,F288_ACLC,0);
		ChipSetField(IOHandle,F288_BETA,26);
	}
	else
	{
		ChipSetField(IOHandle,F288_ALPHA,7);
		ChipSetField(IOHandle,F288_ACLC,1);
		ChipSetField(IOHandle,F288_BETA,26);
	}

							/* switch on frequency offset detector */ 
	FE_288_SetSymbolRate(IOHandle,Params->MasterClock,Params->Symbolrate);	/*	Set the symbol rate	*/

	/* Setup viterbi threshold to improve searching */
	ChipSetRegisters( IOHandle,R288_VTH0,FE_288_VTH_Search,6);

	/* Setup viterbi averaging to improve searching */
	ChipSetField(IOHandle,F288_SN,1);

	/*	Initial calculations	*/
	timingTimCst_ms = (S16)FE_288_TimingTimeConstant(Params->Symbolrate);
	derotTimCst_ms = 2 + (S16)FE_288_DerotTimeConstant(Params->Symbolrate);
	dataTimCst_ms = 2 + 2*(S16)FE_288_DataTimeConstant( IOHandle,Params->Symbolrate);	/* x2 because automatic IQ invertion */ 

		/* reset derotator offset */   
					
	signalType = NOSIGNAL_288;
	zzDirection = DIR_RIGHT;									/* start the zig-zag on the right */
	zzNextLoop = 3;	
	zzDerotStep_Hz = Params->Symbolrate/50;							/* derot step = Fs/50 */
	state = ZZ_CARRIER_CHECK;
 
	do
	{
			
		switch(state)
		{
			case ZZ_CARRIER_CHECK:
			   	WAIT_N_MS_288(timingTimCst_ms + derotTimCst_ms);			/*	wait for derotator ok	*/  
				ChipSetField(IOHandle,F288_CFD_ON,0);							/* switch off frequency offset detector */
				if(ChipGetField(IOHandle,F288_CF))
				{
					   #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
					printf("\nIn Known_Algo... Carrier found ");
					#endif
					/* carrier locked */
					zzDerotFreq_Hz = ((S32)Params->TunerIQSense*FE_288_GetDerotFreq(IOHandle,Params->MasterClock))*1000;	/* Read derotator offset */
					if(zzDerotFreq_Hz>0)
						zzDirection = DIR_LEFT;								/* if carrier offset>0 start the data zigzag on the left */ 
					zzDerotStep_Hz = Params->Symbolrate/4;						/* derot step = Fs / 4 */   
					zzIndex=0;
					zzNextLoop = 3;
					signalType = CARRIEROK_288;									
					state = ZZ_DATA_CHECK;	
													/* Check for data */
				}
				else
				{
					/* carrier not locked */
					ChipSetField(IOHandle,F288_CFD_ON,1); /* switch on frequency offset detector */ 
					state = ZZ_NEXT;										/* Jump to next derotator position */
				}
			break;

			case ZZ_DATA_CHECK:
				WAIT_N_MS_288(timingTimCst_ms+derotTimCst_ms);					/*	wait for timing loop and derotator ok	*/ 
				ChipSetField(IOHandle,F288_CFD_ON,0);							/* switch off frequency offset detector */
				if(FE_288_WaitLock(IOHandle,3*dataTimCst_ms))
				{
					/* data locked */
					state = ZZ_RANGE_CHECK;	
					#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
					printf("\nIn Known_Algo... Data found ")	;
					#endif							/* Check if transponder is in search range */
					
				}
				else
				{
					/* data not locked */
					ChipSetField(IOHandle,F288_CFD_ON,1);							/* switch on frequency offset detector */ 
					state = ZZ_NEXT;										/* Jump to next derotator position */
				}
			break;

			case ZZ_RANGE_CHECK:
				Params->Results.Symbolrate=FE_288_GetSymbolRate(IOHandle,Params->MasterClock);
				Params->Results.Frequency = Params->Frequency + ((S32)Params->TunerIQSense*FE_288_GetDerotFreq(IOHandle,Params->MasterClock));
				/* Store puncture rate */ 
				#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
				printf("\nIn Known_Algo... the frequency at which the range is checked is %d ",Params->Results.Frequency);
				#endif
				Params->Results.PunctureRate= 1<<ChipGetField(IOHandle,F288_PR);
				signalType = FE_288_CheckRange(IOHandle,Params,Params->Results.Symbolrate);
				
				state = ZZ_END;
			break;
		
			case ZZ_NEXT:
				zzIndex++;
				zzDerotFreq_Hz += zzIndex*zzDirection*zzDerotStep_Hz;		/* Compute derotator position */
			
				if(ABS_288(zzDerotFreq_Hz) > ((Params->SearchRange/2000)*1000))				/* Check zig-zag limits */
					zzNextLoop--;
				
				if(zzNextLoop)
				{
					FE_288_SetDerotFreq(IOHandle,(U32)(Params->MasterClock),(S32)(Params->TunerIQSense*zzDerotFreq_Hz));	/* Apply new derotator offset */
						
					if(signalType == CARRIEROK_288)
						state = ZZ_DATA_CHECK;
					else	
						state = ZZ_CARRIER_CHECK;
				}
				else
				{
					if(signalType == CARRIEROK_288)
						signalType = NODATA_288;
					else	
						signalType = NOCARRIER_288;
				
					state = ZZ_END;
				}
				
				zzDirection = -zzDirection;
			break;

			default:
				state = ZZ_END;
			break;

		}
	}while(state != ZZ_END);
	
	return	signalType;
}


FE_288_SIGNALTYPE_t FE_288_Algo( IOARCH_Handle_t IOHandle,	FE_288_InternalParams_t *Params, STTUNER_Handle_t TopLevelHandle)
{
	
	FE_288_SIGNALTYPE_t signalType=NOSIGNAL_288;
	S32 TransponderFreq;
	S32 coarseOffset_Khz=0,coarseSymbolRate_Bds=0,oldcoarseSymbolRate_Bds=0;
	ST_ErrorCode_t Error=ST_NO_ERROR;
	STTUNER_InstanceDbase_t *Inst;
    STTUNER_tuner_instance_t *TunerInstance;  
	S16 timing, Kt;
	U32 SymbolRate;
	U8	shannon_ok,
		direcTV = 0;
		U8 cfrm[2],  known;
		BOOL fine_scan = TRUE; 
		
	Inst = STTUNER_GetDrvInst();
    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[TopLevelHandle].Sat.Tuner;
	Params->Frequency = Params->BaseFreq;
	SymbolRate = Params->Symbolrate;
	Params->SubDir      = 1; 
    Params->TunerOffset = 0;
    Params->SubRange = 10000000;
    
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
         printf("\nFrequency = %d and SR =  %d  \n",Params->Frequency,SymbolRate);
    #endif
/* printf("\nFrequency = %d and SR =  %d  \n",Params->Frequency,SymbolRate);*/
    
    if(ChipGetField(IOHandle,F288_FECMODE)==4)
    direcTV = 1;		/* Store current FEC mode */
    
    	if(Inst[TopLevelHandle].Sat.ScanExact)
	{
		fine_scan  =   FALSE;
	}
	known = (SymbolRate != 0);
	do
       	 {
        	TransponderFreq = Params->Frequency ;
	        
	Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, (U32)Params->Frequency, (U32 *)&Params->Frequency);

	if(Error != ST_NO_ERROR)
	{
		return(NOSIGNAL_288);
	}
		 	
	Params->FreqOffset = (S32)((TransponderFreq - Params->Frequency)*1000);
	Params->DerotFreq  = (short int)(((short int)Params->TunerIQSense)*(Params->FreqOffset/(S32)Params->Mclk));
	        /*************/
	cfrm[0]=MSB_288(Params->DerotFreq);
	cfrm[1]=LSB_288(Params->DerotFreq);
	
	ChipSetRegisters(IOHandle,R288_CFRM,cfrm,2); 
	
	ChipSetField(IOHandle,F288_CFD_ON,1);
	
		
	        /************/
	       
			if(known)
			{
				FE_288_AcquisitionMode(IOHandle);
			Error = (TunerInstance->Driver->tuner_SetBandWidth)( TunerInstance->DrvHandle,((((((Params->Symbolrate/1000)*(100+Params->RollOff))/100)+10000)*13)/10), &(Params->TunerBW));
				signalType = FE_288_KnownAlgo( IOHandle, Params, TopLevelHandle);		        
			}
			else
			{
	        Kt = 56; 		
			do
			{
				Error = (TunerInstance->Driver->tuner_SetBandWidth)( TunerInstance->DrvHandle,50000000/1000, &(Params->TunerBW));
					
			/* ALPHA and BETA are in the same register ==> this I2C access update both */
					FE_288_AcquisitionMode(IOHandle);
			
			if(direcTV)
				ChipSetField(IOHandle,F288_SYM,0);
			
			ChipSetField(IOHandle,F288_KT,Kt);
			
			ChipSetField(IOHandle,F288_FROZE_LOCK,1);
			
			ChipSetField(IOHandle,F288_IND1_ACC,0);
			ChipSetField(IOHandle,F288_IND2_ACC,0xff);
			
			FE_288_SetSymbolRate(IOHandle,Params->MasterClock,1000000);/* Set symbolrate to 1.000MBps (minimum 	symbol rate) */	
					
			coarseSymbolRate_Bds = FE_288_Coarse(IOHandle,Params->MasterClock,&coarseOffset_Khz, fine_scan);	/* Symbolrate coarse search */							
								
			Params->FreqOffset = coarseOffset_Khz*1000;
			Params->DerotFreq  = (short int)(((short int)Params->TunerIQSense)*(Params->FreqOffset/(S32)Params->Mclk));


			Params->Symbolrate=coarseSymbolRate_Bds;
				
			/*printf("\nIN ALGO() after FE_288_Coarse( ) the coarseOffset_Khz =  %d, SR = %d  ",coarseOffset_Khz,Params->Symbolrate);*/
			#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
			printf("\nIN ALGO() the coarseOffset_Khz  after FE_288_Coarse is %d   ",coarseOffset_Khz);
			printf("\nIN ALGO() the SR after FE_288_Coarse is %d   ",Params->Symbolrate);
			#endif
			if(Params->Symbolrate>1000)								/* to avoid a divide by zero error */ 
			shannon_ok = Params->MasterClock/(Params->Symbolrate/1000)>2100;			
			else
			shannon_ok = TRUE;
			
					if((Params->Symbolrate>oldcoarseSymbolRate_Bds)						/* Check symbolrate value */  
				&&          (coarseSymbolRate_Bds>2000000)
				&&	(coarseOffset_Khz>=Params->MinOffset)				/* Check minimum derotator offset criteria */ 
				&&	(coarseOffset_Khz<Params->MaxOffset)				/* Check maximum derotator offset criteria */   
				&&	(shannon_ok))	/* Check shannon criteria */
				{/*printf("\n I m Here");*/
			Error = (TunerInstance->Driver->tuner_SetBandWidth)( TunerInstance->DrvHandle,(Params->Symbolrate*3)/1000, &(Params->TunerBW));
			if(Params->Symbolrate < 5000000)
			{
				ChipSetField(IOHandle,F288_ALPHA,8);	
				ChipSetField(IOHandle,F288_BETA,17);
				
				
			}
			else if(Params->Symbolrate > 35000000)
			{
				ChipSetField(IOHandle,F288_ALPHA,8);	
				ChipSetField(IOHandle,F288_BETA,36);					
			}
			
			ChipSetField(IOHandle,F288_FROZE_LOCK,0);			/* if timing loop not locked launch fine algorithm */
			FE_288_Fine(IOHandle,Params->MasterClock,Params->Symbolrate, fine_scan);
			/*********/Params->Symbolrate=(S32)FE_288_GetSymbolRate(IOHandle,Params->MasterClock);	/* Get result of coarse algorithm */
			/*printf("\nIN ALGO() the SR after FE_288_Fine is %d   ",Params->Symbolrate);*/
			#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
			printf("\nIN ALGO() the SR after FE_288_Fine is %d   ",Params->Symbolrate);
			#endif
					
					ChipSetField(IOHandle,F288_CFD_ON,0);						
					timing = FE_288_Autocentre(IOHandle/*,Params->Symbolrate*/);
					#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
					printf("\n The timing after FE_288_Autocentre is %d ",timing);
					#endif
							
					if(ChipGetField(IOHandle,F288_CF))					
					{  
					                #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
						printf("\n Carrier found now check timing");
						#endif
						if(ABS_288(timing)<=300)
						{
						                #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
							printf("\n Timing is <300 then u know the SR, with this SR go to KnownAlgo");
							#endif
							Params->Results.Symbolrate=FE_288_GetSymbolRate(IOHandle,Params->MasterClock);
							Params->Symbolrate = Params->Results.Symbolrate/* = (Params->Results.Symbolrate/1000)*1000*/;
							Error = (TunerInstance->Driver->tuner_SetBandWidth)( TunerInstance->DrvHandle,((((((Params->Results.Symbolrate/1000)*(100+Params->RollOff))/100)+10000)*13)/10), &(Params->TunerBW));/* Adjust carrier loop setting */
							signalType = FE_288_KnownAlgo( IOHandle, Params, TopLevelHandle);
					
				}
		  }/* Carrier Found */
		}	
			else
			ChipSetField(IOHandle,F288_FROZE_LOCK,0);
			Kt-=10;
				}
				while((signalType != RANGEOK_288) &&  (shannon_ok) && ((Kt>=46) /*&& (Params->Symbolrate<=10000000)*/));
		}
		if(signalType != RANGEOK_288) FE_288_NextSubRange(Params);                
    	
    	}
        while(Params->SubRange && ((signalType != RANGEOK_288) && (signalType != OUTOFRANGE_288)));
        
        if(signalType == RANGEOK_288)
        FE_288_TrackingMode(IOHandle,Params->Results.Symbolrate,Params->Results.PunctureRate);
		
	return signalType;
}

/*****************************************************
--FUNCTION	::	FE_288_Search
--ACTION	::	Search for a valid transponder
--PARAMS IN	::	Handle	==>	Front End Handle
				pSearch ==> Search parameters
				pResult ==> Result of the search
--PARAMS OUT::	NONE
--RETURN	::	Error (if any)
--***************************************************/
FE_288_Error_t	FE_288_Search( IOARCH_Handle_t IOHandle,
										    FE_288_SearchParams_t	*pSearch,
										    FE_288_SearchResult_t	*pResult,
										    STTUNER_Handle_t            TopLevelHandle)
{
    FE_288_Error_t error = FE_288_NO_ERROR;
    FE_288_InternalParams_t *Params;
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
    Instance->SymbolRate = (U32)pSearch->Symbolrate;
    
	Params = memory_allocate_clear(IOARCH_Handle[IOHandle].DeviceMap.MemoryPartition, 1, sizeof( FE_288_InternalParams_t));
	Params->BaseFreq = pSearch->Frequency;	
	
	Params->Symbolrate = pSearch->Symbolrate;
	Params->SearchRange = pSearch->SearchRange;
	Params->DerotPercent = 1000;
	Params->Quartz = pSearch->ExternalClock; /* 30 MHz quartz in 5188*/
	Error = (TunerInstance->Driver->tuner_GetStatus)(TunerInstance->DrvHandle, &TunerStatus);
	Params->TunerIQSense = TunerStatus.IQSense;
	Params->DemodIQMode = pSearch->DemodIQMode;
	
	Params->MasterClock = FE_288_GetMclkFreq( IOHandle, Params->Quartz);
	
	Params->Tagc1 = 0;
	Params->Tagc2 = 0;
	Params->Mclk = (U32)(Params->MasterClock/65536L);
	Params->RollOff = FE_288_GetRollOff( IOHandle);
	Params->MinOffset = -(S32)(pSearch->SearchRange/2000);
	Params->MaxOffset = (S32)(pSearch->SearchRange/2000);
	if(Params != NULL)
	{
			if(	(pSearch != NULL) && (pResult != NULL) &&
				(INRANGE_288(0,(S32)pSearch->Symbolrate,60000000)) &&
				(INRANGE_288(500000,pSearch->SearchRange,50000000)) &&
				(INRANGE_288(FE_288_MOD_BPSK,pSearch->Modulation,FE_288_MOD_QPSK))
				)
			{
				
				Params->State = FE_288_Algo( IOHandle, Params, TopLevelHandle	);
				/* check results */
				if( (Params->State == RANGEOK_288) && (IOARCH_Handle[IOHandle].DeviceMap.Error == ST_NO_ERROR))
				{
					pResult->Locked = TRUE;
					/* update results */
					pResult->Frequency = Params->Results.Frequency;			
					pResult->Symbolrate = Params->Results.Symbolrate;										
					pResult->Rate = Params->Results.PunctureRate;
				}
				else
				{
					pResult->Locked = FALSE;
					
					error = FE_288_SEARCH_FAILED;
				}
			}
			else
			{
				error = FE_288_BAD_PARAMETER;
			}
		}
		else
			error = FE_288_INVALID_HANDLE;
    
	    TunerInstance->realfrequency = pResult->Frequency;	
	    memory_deallocate(IOARCH_Handle[IOHandle].DeviceMap.MemoryPartition, Params);
	return error;
}

BOOL FE_288_Status( IOARCH_Handle_t IOHandle)  
{
	BOOL lock = FALSE;
	
	lock=ChipGetField(IOHandle,F288_LK);
			
	return lock;
}

/*****************************************************
--FUNCTION	::	FE_288_GetSignalInfo
--ACTION	::	Return informations on the locked transponder
--PARAMS IN	::	Handle	==>	Front End Handle
--PARAMS OUT::	pInfo	==> Informations (BER,C/N,power ...)
--RETURN	::	Error (if any)
--***************************************************/
FE_288_Error_t	FE_288_GetSignalInfo( IOARCH_Handle_t IOHandle, FE_288_SignalInfo_t	*pInfo)
{
	U8 cfrm[2];
	FE_288_Error_t error = FE_288_NO_ERROR;
			
	pInfo->Locked = ChipGetField(IOHandle,F288_LK);
		
		if(pInfo->Locked)
		{
			ChipGetRegisters(IOHandle,R288_CFRM,2,cfrm);							/*	read derotator value */ 
			pInfo->derotFreq = (S16) MAKEWORD_288(cfrm[0],cfrm[1]);
			
			pInfo->Rate = 1 << ChipGetField(IOHandle,F288_PR);
			pInfo->Power_dBm = FE_288_GetRFLevel(IOHandle);/*+(pInfo->Frequency-945000)/12000;*/ /* correction applied according to RF frequency */
			pInfo->CN_dBx10 = FE_288_GetCarrierToNoiseRatio(IOHandle);
			pInfo->BER = FE_288_GetError(IOHandle);
			pInfo->SpectralInv = ChipGetField(IOHandle,F288_SYM);
		}
			
	return error;
}

FE_288_Error_t  FE_288_SetTSClock_OutputRate( IOARCH_Handle_t IOHandle,U32 SymbolRate, U32 MasterClock)
{
	U32 bitrate;
	U8 ENA8_Level;
	FE_288_Error_t error = FE_288_NO_ERROR;
	bitrate = SymbolRate*2;	
	
	switch(ChipGetField(IOHandle,F288_PR))
	{
		case	0:		/*	PR 1/2	*/
			bitrate *= 1;
			bitrate /= 2;
		break;

		case	1:		/*	PR 2/3	*/
			bitrate *= 2;
			bitrate /= 3;
		break;

		case	2:		/*	PR 3/4	*/
			bitrate *= 3;
			bitrate /= 4;
		break;

		case	3:		/*	PR 5/6	*/
			bitrate *= 5;
			bitrate /= 6;
		break	;

		case	4:		/*	PR 6/7	*/
			bitrate *= 6;
			bitrate /= 7;
		break;

		case	5:		/*	PR 7/8	*/
			bitrate *= 7;
			bitrate /= 8;
		break;

		default	:
			
		break;
	}

	if(!(ChipGetField(IOHandle,F288_OUTRS_PS)))
	{
		
		
		ENA8_Level = (MasterClock*8) / (bitrate*4);
		if(ENA8_Level>=15)
		ENA8_Level = 15;
		ChipSetField( IOHandle, F288_ENA8_LEVEL, ENA8_Level);
	}
	else
	{
		ENA8_Level = MasterClock / bitrate;
		ENA8_Level -= 1;
		if(ENA8_Level>=3)
		ENA8_Level = 3;
		ENA8_Level <<= 2;
		ENA8_Level |= 0x01;
		ChipSetField( IOHandle, F288_ENA8_LEVEL, ENA8_Level);
	}
	
	return error;
}
#ifndef STTUNER_MINIDRIVER
#ifdef STTUNER_DRV_SAT_SCR

U32 Drv0288_SpectrumAnalysis(STTUNER_tuner_instance_t *TunerInstance,  IOARCH_Handle_t IOHandle, U32 StartFreq,U32 StopFreq,U32 StepSize,U32 *ToneList,U8 mode,int* power_detection_level)
{ 
    U32 freq, realTunerFreq, freqstep = 2000000,
	tempfreq,lastfreq,freqlowerthreshold, freqhigherthreshold,epsilon;
    int  i=0, j = 1,pt_max;
    int  agcVal[500],index=0,points;
    int direction = 1, agc_threshold, *spectrum_agc ,agc_threshold_adjust,agc_high,agc_low,agc_threshold_detect;
    U32 *spectrum;
    U8 gain;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    TUNSDRV_InstanceData_t *Instance;

	points = (U16)((StopFreq - StartFreq)/StepSize + 1);
	spectrum = memory_allocate_clear(IOARCH_Handle[IOHandle].DeviceMap.MemoryPartition, points, sizeof(U16));
	spectrum_agc = memory_allocate_clear(IOARCH_Handle[IOHandle].DeviceMap.MemoryPartition, points, sizeof(int));
	
	Instance = TUNSDRV_GetInstFromHandle(TunerInstance->DrvHandle);
	if(Instance->TunerType == STTUNER_TUNER_STB6000)
	{
	gain=0;
		
	
	ChipSetField(FSTB6000_G, 0,Instance->TunerRegVal);   /* Gain = 0 */
	}
	else if(Instance->TunerType == STTUNER_TUNER_MAX2116) 
	{
	ChipSetField(FMAX2116_G,0x1F,Instance->TunerRegVal);
	gain=0x1F;
	} 

	Error = ChipSetRegisters(Instance->IOHandle, IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart, Instance->TunerRegVal,IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers);
	   		
	Error |= ChipSetField( IOHandle, F288_AGC1R_REF, 48); /* set AGC_Ref as 48*/
	lastfreq = StartFreq - 100000000;
	
	/*added by fsu*/
	/*auto detection of max AGC*/
        if(mode==1) 
        {
	 agc_high=-500;
	 agc_low=500;
	 for(freq=StartFreq;freq<StopFreq;freq+=3000000)
	 {
	  Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, freq/1000, (U32 *)&realTunerFreq);
	  WAIT_N_MS_288(2);
	  agc_threshold = (S8)ChipGetField( IOHandle, F288_AGC1_VALUE);
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
	  agc_threshold_detect=agc_threshold_adjust;
	  *power_detection_level=agc_threshold_adjust;
	} else {
	  agc_threshold_detect=*power_detection_level; /*quick setup*/
	}
	
	i=0;
	points=0;
	
	for(freq=StartFreq;freq<StopFreq;freq+=StepSize)
	{
		direction = 1;j = 1;
		Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, freq/1000, (U32 *)&realTunerFreq);
		WAIT_N_MS_288(2);
		agc_threshold = (S8)ChipGetField( IOHandle, F288_AGC1_VALUE);
		if(agc_threshold > agc_threshold_detect)/* - 70 */
		{
				if(((agc_threshold >= agc_threshold_detect) && (ABS_288((S32)(freq - lastfreq)))>40000000))
				{	
					while(agc_threshold > agc_threshold_detect)/* -128*/
					{
						tempfreq = freq+(freqstep)*direction * j/* *2 */;
						Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, tempfreq/1000, (U32 *)&realTunerFreq);
						WAIT_N_MS_288(2);
						agc_threshold = (S8)ChipGetField( IOHandle, F288_AGC1_VALUE);
						++j;
				        }
				        freqhigherthreshold = tempfreq;
				        direction *= -1;
				        agc_threshold = 0; j = 1;
				        do
					{
						tempfreq = freq+(freqstep)*direction * j/* *2 */;
						Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, tempfreq/1000, (U32 *)&realTunerFreq);
						WAIT_N_MS_288(2);
						agc_threshold = (S8)ChipGetField( IOHandle, F288_AGC1_VALUE);
						++j;
				        }while( agc_threshold> agc_threshold_detect);
				        freqlowerthreshold = tempfreq;

				        spectrum[i] = freqlowerthreshold + (freqhigherthreshold - freqlowerthreshold)/2;
				        if(spectrum[i] >= StartFreq && spectrum[i] <= StopFreq)
				        {
				        	lastfreq = spectrum[i];
				        	Error = (TunerInstance->Driver->tuner_SetFrequency)(TunerInstance->DrvHandle, spectrum[i]/1000, (U32 *)&realTunerFreq);
						WAIT_N_MS_288(2);
						spectrum_agc[i] = (S8)ChipGetField( IOHandle, F288_AGC1_VALUE);
				        
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
        
	
	memory_deallocate(IOARCH_Handle[IOHandle].DeviceMap.MemoryPartition, spectrum);
	memory_deallocate(IOARCH_Handle[IOHandle].DeviceMap.MemoryPartition, spectrum_agc);
	/*memory_deallocate(IOARCH_Handle[IOHandle].DeviceMap.MemoryPartition, agcVal);*/

	return points;		
}
#endif
#endif
#ifndef STTUNER_MINIDRIVER
#ifdef STTUNER_DRV_SAT_SCR
U32 Drv0288_ToneDetection(STTUNER_Handle_t TopLevelHandle,U32 StartFreq,U32 StopFreq,U32 *ToneList,U8 mode,int* power_detection_level)
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
	if(TunerInstance->Driver->ID == STTUNER_TUNER_STB6000)
	{
		step = 9000000;
		Error = (TunerInstance->Driver->tuner_SetBandWidth)(TunerInstance->DrvHandle, 12000, &BandWidth); /* LPF cut off freq = 4+5+5MHz*/
	}
	else if(TunerInstance->Driver->ID == STTUNER_TUNER_MAX2116)
	Error = (TunerInstance->Driver->tuner_SetBandWidth)(TunerInstance->DrvHandle, 36000, &BandWidth); /* LPF cut off freq = 18MHz*/
	else if(TunerInstance->Driver->ID == STTUNER_TUNER_HZ1184)
	Error = (TunerInstance->Driver->tuner_SetBandWidth)(TunerInstance->DrvHandle, 36000, &BandWidth); /* LPF cut off freq = 18MHz*/
	else 		
	Error = (TunerInstance->Driver->tuner_SetBandWidth)(TunerInstance->DrvHandle, 36000, &BandWidth); /* LPF cut off freq = 18MHz*/
	
	nbTones = Drv0288_SpectrumAnalysis(TunerInstance,  Inst[TopLevelHandle].Sat.Demod.IOHandle, StartFreq, StopFreq, step, ToneList,mode,power_detection_level);	/* spectrum acquisition */
		
	return nbTones;
}
#endif
#endif


