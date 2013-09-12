

/* -------------------------------------------------------------------------
File Name: drv399E.c

Description: 399E driver LLA

Copyright (C) 2003-2004 STMicroelectronics

author: SD

History:

15-07-2004
Comments: First time Integrated in STTUNER STAPI
Revision 1.0.0
---------------------------------------------------------------------------- */


/* includes ---------------------------------------------------------------- */

/*	generic includes	*/
#include "gen_macros.h"
#include "sysdbase.h"
#include "tunsdrv.h"

#ifdef HOST_PC
	#include <utility.h>
	#include <stdio.h>
	#include "math.h"
	#include "stdlib.h"
	#ifndef NO_GUI
		#include "Appl.h"
		#include "UserPar.h"
		#include "Pnl_report.h"
		#include "399_Report_Msg.h"
		#define REPORTMSG(a,b,c) Report399(a,b,c);
	#else
		#define REPORTMSG(a,b,c)
	#endif
#else
	#define REPORTMSG(a,b,c) 
#endif

/* common includes */ 
#include"sttuner.h"
#include "399_drv.h"
#include"399_util.h"
#include "util399E.h"


#define MINIMUM_GAP 18000 /* Minimum gap size between master clock harmonic and tuner frequency (KHz) */
#define MINIMUM_GAP_low 10000
#define MINIMUM_GAP_2 33000

#ifndef STTUNER_MINIDRIVER
/* Current LLA revision	*/
static const ST_Revision_t Revision399  = "STV0399-LLA_REL_2.2.0";
#endif

 
static FE_399_LOOKUP_t FE_399_RF_LookUp =	{
												16,
												{
													{-100,	-200},
													{-150,	-190},
													{-200,	-140},
													{-250,	-90},
													{-300,	-40},
													{-350,	10},
													{-400,	60},
													{-450,	110},
													{-500,	140},
													{-550,	190},
													{-600,	235},
													{-650,	285},
													{-700,	340},
													{-750,	390},
													{-800,	435},
													{-840,	475}
												}
											};

											
static FE_399_LOOKUP_t FE_399_CN_LookUp =	{
												20,
												{
													{15,	9600},
													{20,	9450},
													{30,	9000},
													{40,	8250},
													{50,	7970},
													{60,	7360},
													{70,	6770},
													{80,	6200},
													{90,	5670},
													{100,5190},
													{110,4740},
													{120,4360},
													{130,4010},
													{140,3710},
													{150,3440},
													{160,3210},
													{170,3020},
													{180,2860},
													{190,2700},
													{200,2600}
												}
											};







/* structures -------------------------------------------------------------- */ 




#ifdef STTUNER_MINIDRIVER
		extern D0399_InstanceData_t *DEMODInstance;
#endif
	
#ifndef STTUNER_MINIDRIVER
/***********************************************************
**FUNCTION	::	Drv0399_GetLLARevision
**ACTION	::	Returns the 399 LLA driver revision
**RETURN	::	Revision399
***********************************************************/
ST_Revision_t Drv0399_GetLLARevision(void)
{
	return (Revision399);
}

#endif
/*****************************************************
**FUNCTION	::	XtoPowerY
**ACTION	::	Compute  x^y (where x and y are integers) 
**PARAMS IN	::	Number -> x
**				Power -> y
**PARAMS OUT::	NONE
**RETURN	::	2^n
*****************************************************/
#ifndef STTUNER_MINIDRIVER
U32 XtoPowerY(U32 Number, U32 Power)
{
	int i;
	long result = 1;
	
	for(i=0;i<Power;i++)
		result *= Number;
		
	return (U32)result;
}
#endif
S32 InRange(S32 x,S32 y,S32 z) 
{
	return ((x<=y && y<=z) || (z<=y && y<=x))?1:0;
}

#ifndef STTUNER_MINIDRIVER
S32 RoundToNextHighestInteger(S32 Number,U32 Digits)
{
	S32 result = 0,
		highest = 0;
	
	highest = XtoPowerY(10,Digits);
	result = Number / highest;
	
	if((Number - (Number/highest)*highest) > 0)
		result += 1;
	
	return result;
}
#endif
/*****************************************************
**FUNCTION	::	FE_399E_PowOf2
**ACTION	::	Compute  2^n (where n is an integer) 
**PARAMS IN	::	number -> n
**PARAMS OUT::	NONE
**RETURN	::	2^n
*****************************************************/

long FE_399E_PowOf2(int number)
{
	int i;
	long result=1;
	
	for(i=0;i<number;i++)
		result*=2;
		
	return result;
}

#ifndef STTUNER_MINIDRIVER
/*****************************************************
**FUNCTION	::	FE_399E_GetPowOf2
**ACTION	::	Compute  x according to y with y=2^x 
**PARAMS IN	::	number -> y
**PARAMS OUT::	NONE
**RETURN	::	x
*****************************************************/
long FE_399E_GetPowOf2(int number)
{
	int i=0;
	
	while(FE_399E_PowOf2(i)<number) i++;
		
	return i;
}
#endif
#ifndef STTUNER_MINIDRIVER
BOOL FE_399_Harmonic(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 MasterClock,U32 RfFrequency,U32 OFFSET)
{
	S32 gap;
	
	gap = RfFrequency % (MasterClock / 1000);
	
	/*if((gap <= MINIMUM_GAP) || (gap >= (MasterClock / 1000)-MINIMUM_GAP))*/
	if((gap <= OFFSET) || (gap >= (MasterClock / 1000)-OFFSET))
		return TRUE;
	else
		return FALSE;
}
#endif
#ifdef STTUNER_MINIDRIVER
BOOL FE_399_Harmonic(U32 MasterClock,U32 RfFrequency,U32 OFFSET)
{
	S32 gap;
	
	gap = RfFrequency % (MasterClock / 1000);
	
	/*if((gap <= MINIMUM_GAP) || (gap >= (MasterClock / 1000)-MINIMUM_GAP))*/
	if((gap <= OFFSET) || (gap >= (MasterClock / 1000)-OFFSET))
		return TRUE;
	else
		return FALSE;
}
#endif
BOOL FE_399_Triple(S32 MasterClock,S32 SymbolRate) 
{
	S32 ratio;
	
	ratio = MasterClock/(SymbolRate/1000);
	
	if((ratio >= 2990) && (ratio <= 3010))
		return TRUE;
	else
		return FALSE;
}


U32 FE_399_SelectOffset(S32 SymbolRate)
{

U32 OFFSET;
if(SymbolRate <= 10000000)
   {
    OFFSET = MINIMUM_GAP_low;
   }
   
else
  {
   OFFSET =MINIMUM_GAP; 
  }
  
return OFFSET;
}

#ifdef STTUNER_MINIDRIVER
S32 FE_399_SelectMclk(U32 RfFrequency,S32 SymbolRate,S32 ExtClk)
{
	BOOL harmonic,triple;
	U32 pll_div;
	U32 coef_div;
	/*S32 clock[2][2]={{65571428,13},{70615384,14}};*/
	S32 clock[2][2];
	S32 index = 0;
	S32 ratio;
	
	
	pll_div = FE_399_Get_PLL_mult_coeff( );
	
	if((pll_div == 34)&&(SymbolRate <= 10000000))
	{
	 coef_div=12;
	}
	
	else if((pll_div == 34)&&(SymbolRate > 10000000)&&(SymbolRate <= 20000000))
	{
	 coef_div=9;
	}
	
	else if((pll_div == 34)&&(SymbolRate > 20000000))
	{
	 coef_div=6;
	}
	
	else if((pll_div == 32)&&(SymbolRate <= 10000000))
	{
	 coef_div=12;
	}
	
	else if((pll_div == 32)&&(SymbolRate > 10000000)&&(SymbolRate <= 20000000))
	{
	 coef_div=9;
	}
	
	else if((pll_div == 32)&&(SymbolRate > 20000000))
	{
	 coef_div=6;
	}
	clock[0][0]=pll_div*27000000/coef_div;
	clock[0][1]=coef_div; 
	clock[1][0]=pll_div*27000000/(coef_div+1); 
	clock[1][1]=coef_div+1; 
	
	ratio = FE_399_GetDivRatio( );
	if((ratio>=6) && (ratio<=13))
	{
		do
		{
			harmonic = FE_399_Harmonic(clock[index][0],RfFrequency,FE_399_SelectOffset(SymbolRate));
			triple = FE_399_Triple(clock[index][0],SymbolRate);
			index++;
		}
		while((harmonic || triple) && (index<2));
	
		if(harmonic || triple)
			FE_399_SetDivRatio(coef_div);
		else
			FE_399_SetDivRatio(clock[index-1][1]);
	}
	
	return FE_399_GetMclkFreq(ExtClk);
	
}
#endif

#ifndef STTUNER_MINIDRIVER
S32 FE_399_SelectMclk(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 RfFrequency,S32 SymbolRate,S32 ExtClk)
{
	BOOL harmonic,triple;
	U32 pll_div;
	U32 coef_div=0;
	/*S32 clock[2][2]={{65571428,13},{70615384,14}};*/
	S32 clock[2][2];
	S32 index = 0;
	S32 ratio;
	
		
	pll_div = FE_399_Get_PLL_mult_coeff(DeviceMap, IOHandle);
	
	if((pll_div == 34)&&(SymbolRate <= 10000000))
	{
	 coef_div=12;
	}
	
	else if((pll_div == 34)&&(SymbolRate > 10000000)&&(SymbolRate <= 20000000))
	{
	 coef_div=9;
	}
	
	else if((pll_div == 34)&&(SymbolRate > 20000000))
	{
	 coef_div=6;
	}
	
	else if((pll_div == 32)&&(SymbolRate <= 10000000))
	{
	 coef_div=12;
	}
	
	else if((pll_div == 32)&&(SymbolRate > 10000000)&&(SymbolRate <= 20000000))
	{
	 coef_div=9;
	}
	
	else if((pll_div == 32)&&(SymbolRate > 20000000))
	{
	 coef_div=6;
	}
	
	
	
	clock[0][0]=pll_div*27000000/coef_div;
	clock[0][1]=coef_div; 
	clock[1][0]=pll_div*27000000/(coef_div+1); 
	clock[1][1]=coef_div+1; 
	
	ratio = FE_399_GetDivRatio(DeviceMap, IOHandle);
	if((ratio>=6) && (ratio<=13))
	{
		do
		{

		
			
			harmonic = FE_399_Harmonic(DeviceMap, IOHandle,clock[index][0],RfFrequency,FE_399_SelectOffset(SymbolRate));
			triple = FE_399_Triple(clock[index][0],SymbolRate);
			index++;
		}
		while((harmonic || triple) && (index<2));
	
		if(harmonic || triple)
			FE_399_SetDivRatio(DeviceMap, IOHandle,coef_div);
		else
			FE_399_SetDivRatio(DeviceMap, IOHandle,clock[index-1][1]);
	}
	
	return FE_399_GetMclkFreq(DeviceMap, IOHandle,ExtClk);
	
}

#endif
/*****************************************************
**FUNCTION	::	FE_399_CalcAGC1Gain
**ACTION	::	Compute the AGC1 gain
**PARAMS IN	::  regvalue	==>	register value
**PARAMS OUT::	NONE
**RETURN	::	AGC1 gain multiplied by two
*****************************************************/
S32 FE_399_CalcAGC1Gain(U8 regvalue)
{
	S32 gain = 0;
	
	gain = (S32)((regvalue >> 2) & 0x1F);
	
	if(!(regvalue & 0x80))
		gain += 16;
		
	return gain;
}

/*****************************************************
**FUNCTION	::	FE_399_CalcAGC1Integrator
**ACTION	::	Compute the AGC1 gain
**PARAMS IN	::  gain	==>	gain multiplied by two
**PARAMS OUT::	NONE
**RETURN	::	AGC1 integrator value
*****************************************************/
U8 FE_399_CalcAGC1Integrator(S32 gain)
{
	U8 integrator = 0x00;
	
	if(gain > 31)
		gain-= 16;
	else
		integrator |= 0x80;
		
	integrator |= (U8)(gain & 0x1F) << 2;
	
	return integrator;
}

/*****************************************************
**FUNCTION	::	FE_399_CalcAGC0Gain
**ACTION	::	Compute the AGC0 gain
**PARAMS IN	::  regvalue	==>	register value
**PARAMS OUT::	NONE
**RETURN	::	AGC0 gain
*****************************************************/
S32 FE_399_CalcAGC0Gain(U8 regvalue) 
{
	S32 gain = 0;
	
	if((regvalue>0)&&(regvalue<10))
	{
		gain = (S32)((regvalue - 1)%4) * 6; 
	
		if(regvalue < 5) 
			gain -= 20; 
		else if(regvalue == 9)
			gain += 24;
	}
		
	return gain;
}
#ifndef STTUNER_MINIDRIVER

/*****************************************************
**FUNCTION	::	FE_399_GetVcoState
**ACTION	::	return the status of the VCO 
**PARAMS IN	::  hChip	==>	handle to the chip
**PARAMS OUT::	NONE
**RETURN	::	the current state of the VCO
*****************************************************/
FE_399_VCO_STATE_t FE_399_GetVcoState(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	FE_399_VCO_STATE_t vcostate = VCO_LOCKED;
	
	if(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_VLOW) == 0)	/* VCO is too low */ 
		vcostate = VCO_TOO_LOW;	
	else if(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_VHIGH) == 1)	/* VCO is too high */
		vcostate = VCO_TOO_HIGH;
	
	return vcostate;
}
#endif
#ifdef STTUNER_MINIDRIVER
FE_399_VCO_STATE_t FE_399_GetVcoState( )
{
	U8 Data1, Data2;
	FE_399_VCO_STATE_t vcostate = VCO_LOCKED;
	
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_SYSCTRL, F399_VLOW, F399_VLOW_L, &Data1, 1, FALSE);
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_SYSCTRL, F399_VHIGH, F399_VHIGH_L, &Data2, 1, FALSE);	
	if(Data1 == 0)	/* VCO is too low */ 
	vcostate = VCO_TOO_LOW;	
	else if(Data2 == 1)	/* VCO is too high */
	vcostate = VCO_TOO_HIGH;
	
	return vcostate;
}
#endif
/*****************************************************
**FUNCTION	::	FE_399_StartSynt
**ACTION	::	start STV0399 analog synthesizer
**PARAMS IN	::  hChip	==>	handle to the chip
**PARAMS OUT::	NONE
**RETURN	::	NONE
*****************************************************/
#ifndef STTUNER_MINIDRIVER
void FE_399_StartSynt(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_BYP,0);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_DIS,1);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_DIS,0);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_BYP,1);
}
#endif
#ifdef STTUNER_MINIDRIVER
void FE_399_StartSynt( )
{
	U8 Data;
	
	Data = 0;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_SYNTCTRL, F399_BYP, F399_BYP_L, &Data, 1, FALSE);
	
	Data = 1;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_SYNTCTRL, F399_DIS, F399_DIS_L, &Data, 1, FALSE);
	
	Data = 0;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_SYNTCTRL, F399_DIS, F399_DIS_L, &Data, 1, FALSE);
	
	Data = 1;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_SYNTCTRL, F399_BYP, F399_BYP_L, &Data, 1, FALSE);
}
#endif
#ifdef STTUNER_MINIDRIVER
void FE_399_Synt( )
{
	U8 Data;
	Data = 1;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_SYNTCTRL2, F399_RESET_FILT, F399_RESET_FILT_L, &Data, 1, FALSE);
	
	Data = 1;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_SYNTCTRL2, F399_RESET_FILT, F399_RESET_FILT_L, &Data, 1, FALSE);
}
#endif

#ifndef STTUNER_MINIDRIVER
void FE_399_Synt(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle )
{
	
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_RESET_FILT,1);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_RESET_FILT,0);  
}
#endif
/*--------------------------------------------------------------------------
  --------------------------------------------------------------------------
  --------------------------------------------------------------------------
**FUNCTION	::	FE_399_CalcSyntFreqInt
**ACTION	::	compute the STV0399 digital synthetizer frequency (with integers)
**PARAMS IN	::  hChip	==>	handle to the chip
**				mdiv	==> mdiv divider value
**				ipe		==> ipe divider value
**PARAMS OUT::	NONE
**RETURN	::	Synthesizer frequency (Hz)
-----------------------------------------------------------------------------
-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/

U32 FE_399_CalcSyntFreqInt(S32 quartz,S32 mdiv,U32 ipe,U32 pll_mc)
{
	U32 frequency = 0;
	
	/*frequency = 1057*(pll_mc)*(((32*quartz)/4)/((((32767*(mdiv+33)-ipe))/31)));*//*original line*/
	frequency = 1057*(pll_mc)*((8*quartz)/((((32767*(mdiv+33)-ipe))/31)));
	return frequency;
}

/*--------------------------------------------------------------------------
  --------------------------------------------------------------------------
  --------------------------------------------------------------------------
**FUNCTION	::	FE_399_CalcSyntFreqFloat
**ACTION	::	compute the STV0399 digital synthetizer frequency (with floats)
**PARAMS IN	::  hChip	==>	handle to the chip
**				mdiv	==> mdiv divider value
**				ipe		==> ipe divider value
**PARAMS OUT::	NONE
**RETURN	::	Synthesizer frequency (Hz)
-----------------------------------------------------------------------------
-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
/*
U32 FE_399_CalcSyntFreqFloat(S32 quartz,S32 mdiv,U32 ipe,U32 pll_mc)
{
	U32 frequency;
*/	
	/*frequency = (U32)((32.0)*((pll_mc*(double)quartz/4.0)/(mdiv+33.0-(double)(ipe)/32767.0)));*/  /*original line */
/*	frequency = (U32)((32)*((pll_mc*quartz/4)/(mdiv+33-(ipe)/32767)));
	return frequency;
}
*/
/*----------------------------------------------------------------
------------------------------------------------------------------
------------------------------------------------------------------
**FUNCTION	::	FE_399_SetSyntFreq
**ACTION	::	Set the STV0399 synthetizer frequency
**PARAMS IN	::  hChip	==>	handle to the chip
**				Quartz	==>	quartz frequency (Hz)
**				Target	==> target frequency (Hz)
**PARAMS OUT::	NONE
**RETURN	::	Synthesizer frequency (Hz)
------------------------------------------------------------------*/
#ifndef STTUNER_MINIDRIVER
U32 FE_399_SetSyntFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 Quartz, U32 Target)
{
	U32 ipe = 32767;
	S32 mdiv = 0;
	U32 pll_mc = 32;
	
	
	pll_mc = FE_399_Get_PLL_mult_coeff(DeviceMap,IOHandle);
	if((Target >= 202000000) && (Target <= 450000000) && Quartz)
	{
		
		mdiv = (4*(((2*pll_mc)*Quartz)/(Target/10000)))/10000 - 32;
		ipe = (32767 * ((mdiv+33)*10000 - 4*(((2*pll_mc)*Quartz)/(Target/10000))))/10000;
		
		/* round ipe divider */
		if ( ((ipe & 0x03ff) > 0x01ff) || (ipe < 0x03ff) )
			ipe |= 0x03ff;
		else
			ipe = (ipe & 0xfc00) - 1;		
		/* Set analog synthesizer	*/
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_MD,mdiv);
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_PE_MSB,MSB(ipe));
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_PE_LSB,LSB(ipe));
	}
	else
	{
		mdiv = STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_MD);
		ipe = MAKEWORD(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_PE_MSB),STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_PE_LSB));
		
	}
	
	return FE_399_CalcSyntFreqInt(Quartz,mdiv,ipe,pll_mc);
}
#endif

#ifdef STTUNER_MINIDRIVER
U32 FE_399_SetSyntFreq(U32 Quartz, U32 Target)
{
	U8 Data, nsbuffer[2];
	U32 ipe = 32767;
	S32 mdiv = 0, temp = 0;
	U32 pll_mc = 32;
	
	pll_mc = FE_399_Get_PLL_mult_coeff( );
	if((Target >= 202000000) && (Target <= 450000000) && Quartz)
	{
		
		mdiv = (4*(((2*pll_mc)*Quartz)/(Target/10000)))/10000 - 32;
		ipe = (32767 * ((mdiv+33)*10000 - 4*(((2*pll_mc)*Quartz)/(Target/10000))))/10000;
		
		/* round ipe divider */
		if ( ((ipe & 0x03ff) > 0x01ff) || (ipe < 0x03ff) )
			ipe |= 0x03ff;
		else
			ipe = (ipe & 0xfc00) - 1;		
		/* Set analog synthesizer */
		temp = (mdiv > 0 ) ? mdiv : mdiv + (1 << 5);
		Data = (U8)temp;
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_ACOARSE, F399_MD, F399_MD_L, &Data, 1, FALSE);
		
		nsbuffer[0] = ((ipe) & 0xff00)>>8;
		nsbuffer[1] = ((ipe) & 0x00ff);
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_AFINEMSB, 0, 0, nsbuffer, 2, FALSE);
		
	}
	else
	{
		
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_ACOARSE, F399_MD, F399_MD_L, &Data, 1, FALSE);
		mdiv = (S32)(Data - (1 << 5));
		/*mdiv = Data;*/ 
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_AFINEMSB, 0, 0, nsbuffer, 2, FALSE);
		ipe = MAKEWORD(nsbuffer[0],nsbuffer[1]);
	}
	
	return FE_399_CalcSyntFreqInt(Quartz,mdiv,ipe,pll_mc);
}
#endif
/*----------------------------------------------------------------
------------------------------------------------------------------
------------------------------------------------------------------
**FUNCTION	::	FE_399_Get_PLL_mult_coeff

------------------------------------------------------------------*/
#ifndef STTUNER_MINIDRIVER
U32 FE_399_Get_PLL_mult_coeff(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	U32 pll_div;
	
	
	switch(STTUNER_IOREG_GetField(DeviceMap, IOHandle,F399_PLL_DIV))
	{
		default:
			pll_div = 32;	
		break;
		
		case 2: 
			pll_div = 30;	
		break;
		
		case 4:
			pll_div = 32;
		break;
		
		case 6:
			pll_div = 31;
		break;
		
		case 8:
			pll_div = 34;
		break;
		
		case 12:
			pll_div = 33;
		break;
	}
	
	return pll_div;
}
#endif
#ifdef STTUNER_MINIDRIVER
U32 FE_399_Get_PLL_mult_coeff( )
{
	U32 pll_div;
	U8 Data;
	
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_DCLK1, F399_PLL_DIV, F399_PLL_DIV_L, &Data, 1, FALSE);
	switch(Data)
	{
		default:
			pll_div = 32;	
		break;
		
		case 2: 
			pll_div = 30;	
		break;
		
		case 4:
			pll_div = 32;
		break;
		
		case 6:
			pll_div = 31;
		break;
		
		case 8:
			pll_div = 34;
		break;
		
		case 12:
			pll_div = 33;
		break;
	}
	
	return pll_div;
}
#endif
/*----------------------------------------------------------------
------------------------------------------------------------------
------------------------------------------------------------------
**FUNCTION	::	FE_399_GetSyntFreq
**ACTION	::	Set the STV0399 digital synthetizer frequency
**PARAMS IN	::  hChip	==>	handle to the chip
**				Quartz	==>	quartz frequency (Hz)
**PARAMS OUT::	NONE
**RETURN	::	Synthesizer frequency (Hz)

------------------------------------------------------------------*/
#ifndef STTUNER_MINIDRIVER
U32 FE_399_GetSyntFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 Quartz)
{
	S8 mdiv;
	U32 ipe,Fout,pll_mc;
		
	/*	analog synthesizer	*/
	mdiv = STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_MD);
	ipe = MAKEWORD(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_PE_MSB),STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_PE_LSB));
	pll_mc = FE_399_Get_PLL_mult_coeff(DeviceMap,IOHandle);
	
	/*Fout = FE_399_CalcSyntFreqFloat(Quartz,mdiv,ipe,pll_mc);*/
	Fout = FE_399_CalcSyntFreqInt(Quartz,mdiv,ipe,pll_mc);
	
	return Fout;
}
#endif
#ifdef STTUNER_MINIDRIVER
U32 FE_399_GetSyntFreq(U32 Quartz)
{
	U8 Data, nsbuffer[2];
	S32 mdiv;
	U32 ipe,
		Fout,pll_mc;
		
	/*	analog synthesizer	*/
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_ACOARSE, F399_MD, F399_MD_L, &Data, 1, FALSE);
	mdiv = Data; 
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_AFINEMSB, 0, 0, nsbuffer, 2, FALSE);
	
	ipe = MAKEWORD(nsbuffer[0],nsbuffer[1]);
	
	pll_mc = FE_399_Get_PLL_mult_coeff( );
	
	Fout = FE_399_CalcSyntFreqInt(Quartz,mdiv,ipe,pll_mc);
	
	return Fout;
}
#endif
/*****************************************************
**FUNCTION	::	FE_399_SetDivRatio
**ACTION	::	Set the STV0399 synthesizer first divider ratio
**PARAMS IN	::  hChip		==>	handle to the chip
**				Ratio		==> Divider Ratio (1..15)
**PARAMS OUT::	NONE
**RETURN	::	NONE
*****************************************************/
#ifndef STTUNER_MINIDRIVER
void FE_399_SetDivRatio(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U8 Ratio)
{
	U8	byte = 0x01;
	U32 mode;
	
		
	if((Ratio & 0x01) == 0x01)
		byte = 0x03;
	
	byte <<= ((Ratio/2)-1);
	
	mode = STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_CMD_DIV_DIG);
	
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_BYP,0);					/* Synthesizer bypass */
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_CMD_DIV_DIG,0);			/* Divider bypass */
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_CMD_DIV,0); 			/* Reset divider */
	
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_CMD_DIV,byte);   		/* Set divider */
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_CMD_DIV_DIG,mode); 		/* Enable divider */
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_BYP,1);					/* Enable synthesizer*/  
}
#endif
#ifdef STTUNER_MINIDRIVER
void FE_399_SetDivRatio(U8 Ratio)
{
	U8	byte = 0x01, Data;
	U32 mode;
		
	if((Ratio & 0x01) == 0x01)
		byte = 0x03;
	
	byte <<= ((Ratio/2)-1);
	
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_DCLK1, F399_CMD_DIV_DIG, F399_CMD_DIV_DIG_L, &Data, 1, FALSE);
	mode = Data;
	
	Data = 0;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_SYNTCTRL, F399_BYP, F399_BYP_L, &Data, 1, FALSE);
	Data = 0;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DCLK1, F399_CMD_DIV_DIG, F399_CMD_DIV_DIG_L, &Data, 1, FALSE);
	Data = 0;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DCLK2, F399_CMD_DIV, F399_CMD_DIV_L, &Data, 1, FALSE);
	
	Data = byte;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DCLK2, F399_CMD_DIV, F399_CMD_DIV_L, &Data, 1, FALSE);
	
	Data = mode;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DCLK1, F399_CMD_DIV_DIG, F399_CMD_DIV_DIG_L, &Data, 1, FALSE);
	
	Data = 1;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_SYNTCTRL, F399_BYP, F399_BYP_L, &Data, 1, FALSE);
	
}
#endif

/*****************************************************
**FUNCTION	::	FE_399_GetDivRatio
**ACTION	::	Retreive the STV0399 synthesizer first divider ratio
**PARAMS IN	::  hChip		==>	handle to the chip
**PARAMS OUT::	NONE
**RETURN	::	Ratio (1..15)
*****************************************************/
#ifndef STTUNER_MINIDRIVER
U8 FE_399_GetDivRatio(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	U8	ratio = 1,
		byte,
		nbbitused = 0,
		nbbithigh = 0;
	
	byte = STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_CMD_DIV);
	
	while(((byte & 0x01) == 0x00) && (nbbitused <= 8))
	{
		byte >>=1;
		nbbitused++;
	}
	
	while(((byte & 0x01) == 0x01) && (nbbitused <= 8))
	{
		byte >>=1;
		nbbitused++;
		nbbithigh++;
	}
	
	ratio = (nbbitused*2)-(nbbithigh-1);
	
	return ratio;
}
#endif
#ifdef STTUNER_MINIDRIVER
U8 FE_399_GetDivRatio( )
{
	U8	ratio = 1,Data,
		byte,
		nbbitused = 0,
		nbbithigh = 0;
	
	
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_DCLK2, F399_CMD_DIV, F399_CMD_DIV_L, &Data, 1, FALSE);
	byte = Data;
	
	while(((byte & 0x01) == 0x00) && (nbbitused <= 8))
	{
		byte >>=1;
		nbbitused++;
	}
	
	while(((byte & 0x01) == 0x01) && (nbbitused <= 8))
	{
		byte >>=1;
		nbbitused++;
		nbbithigh++;
	}
	
	ratio = (nbbitused*2)-(nbbithigh-1);
	
	return ratio;
}
#endif

/*-----------------------------------------------------------
-------------------------------------------------------------
-------------------------------------------------------------
**FUNCTION	::	FE_399_GetMclkFreq
**ACTION	::	Set the STV0399 master clock frequency
**PARAMS IN	::  hChip		==>	handle to the chip
**				ExtClk		==>	External clock frequency (Hz)
**PARAMS OUT::	NONE
**RETURN	::	MasterClock frequency (Hz)
-------------------------------------------------------------*/
#ifndef STTUNER_MINIDRIVER
U32 FE_399_GetMclkFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 ExtClk)
{
	U32 mclk = 0;
	U32 pll_mc;
	
	pll_mc = FE_399_Get_PLL_mult_coeff(DeviceMap,IOHandle);
	
	switch(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_CMD_DIV_DIG))
	{
		default:
			mclk = ExtClk;	/* synthe bypass	*/
		break;
		
		case 1: 
			mclk = (ExtClk*pll_mc)/2;	/* Divider bypass */
		break;
		
		case 2:
			mclk = (ExtClk*pll_mc)/FE_399_GetDivRatio(DeviceMap,IOHandle);
		break;
		
		case 3:
			mclk = (ExtClk*pll_mc)/(2*FE_399_GetDivRatio(DeviceMap,IOHandle));
		break;
		
		case 4:
			mclk = (ExtClk*pll_mc)/(4*FE_399_GetDivRatio(DeviceMap,IOHandle));
		break;
	}
	
	return mclk;
}
#endif
#ifdef STTUNER_MINIDRIVER
U32 FE_399_GetMclkFreq(U32 ExtClk)
{
	U8 Data;
	U32 mclk = 0;
	U32 pll_mc;
	
	pll_mc = FE_399_Get_PLL_mult_coeff( );
	
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_DCLK1, F399_CMD_DIV_DIG, F399_CMD_DIV_DIG_L, &Data, 1, FALSE);
	switch(Data)
	{
		default:
			mclk = ExtClk;	/* synthe bypass	*/
		break;
		
		case 1: 
			mclk = (ExtClk*pll_mc)/2;	/* Divider bypass */
		break;
		
		case 2:
			mclk = (ExtClk*pll_mc)/FE_399_GetDivRatio( );
		break;
		
		case 3:
			mclk = (ExtClk*pll_mc)/(2*FE_399_GetDivRatio( ));
		break;
		
		case 4:
			mclk = (ExtClk*pll_mc)/(4*FE_399_GetDivRatio( ));
		break;
	}
	
	return mclk;
}
#endif
/*----------------------------------------------------------------
------------------------------------------------------------------
------------------------------------------------------------------
**FUNCTION	::	FE_399_SetTunerFreq
**ACTION	::	Set the STV0399 analog synthetizer frequency
**PARAMS IN	::  Quartz	==>	quartz frequency (Hz)
**				Frequency	==>	tuner frequency (KHz)    
**PARAMS OUT::	NONE
**RETURN	::	tuner frequency (KHz)
------------------------------------------------------------------*/
#ifndef STTUNER_MINIDRIVER
U32 FE_399_SetTunerFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 Quartz,U32 Frequency,U32 SymbolRate)
{
	U32 result;
	U32 pllmult = 0;
	FE_399_PLL_MULTIPLIER_t pll;
	
	if((Frequency >= 1400000000)&&(Frequency <= 1550000000))
    {
    STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_PLL_DIV,0);		 /*pll_div = 32*/
    }
    
    else if((Frequency >= 1810000000)&&(Frequency <= 1860000000))
    {
    STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_PLL_DIV,0);		 /*pll_div = 32*/
    }
    
    else
    {
     STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_PLL_DIV,8);   /* pll_div = 34 */
    }
	
	pll = (Frequency >= 1200000000) ? PLL_X5: PLL_X4;
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_PLL_FACTOR,pll); 			/* select pll	*/  
	

	pllmult = (pll == PLL_X4) ? 4:5;
	
	result = FE_399_SetSyntFreq(DeviceMap,IOHandle,Quartz,Frequency/pllmult) * pllmult;
	
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_RESET_SYNT,1);	/* Reset analog synthesizer */
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_RESET_SYNT,0);
	
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_CMD,VCO_LOW);
	
	STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R399_SYSCTRL);
	
	if ((STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_VLOW)==0)||(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_VHIGH)==1))
	{
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_CMD,VCO_HIGH);
	}
	
	return result;
}
#endif
#ifdef STTUNER_MINIDRIVER
U32 FE_399_SetTunerFreq(U32 Quartz,U32 Frequency,U32 SymbolRate)
{
	U8 Data;
	U32 result;
	U32 pllmult = 0;
	FE_399_PLL_MULTIPLIER_t pll;
		
	if((Frequency >= 1400000000)&&(Frequency <= 1550000000))
    {
    Data = 0;
    STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DCLK1, F399_PLL_DIV, F399_PLL_DIV_L, &Data, 1, FALSE);
     /*pll_div = 32*/
    }
    
    else if((Frequency >= 1810000000)&&(Frequency <= 1860000000))
    {
    Data = 0;
    STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DCLK1, F399_PLL_DIV, F399_PLL_DIV_L, &Data, 1, FALSE);
     /*pll_div = 32*/
    }
    
    else
    {
     Data = 8;
     STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DCLK1, F399_PLL_DIV, F399_PLL_DIV_L, &Data, 1, FALSE);
     /* pll_div = 34 */
    }
	
	pll = (Frequency >= 1200000000) ? PLL_X5: PLL_X4;
	Data = pll;
        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_SYNTCTRL2, F399_PLL_FACTOR, F399_PLL_FACTOR_L, &Data, 1, FALSE);
	/* select pll	*/  
	

	pllmult = (pll == PLL_X4) ? 4:5;
	
	result = FE_399_SetSyntFreq(DeviceMap,IOHandle,Quartz,Frequency/pllmult) * pllmult;
	
	Data = 1;
        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_SYNTCTRL2, F399_RESET_SYNT, F399_RESET_SYNT_L, &Data, 1, FALSE);
	Data = 0;
        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_SYNTCTRL2, F399_RESET_SYNT, F399_RESET_SYNT_L, &Data, 1, FALSE);
	
	Data = VCO_LOW;
        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_SYSCTRL, F399_CMD, F399_CMD_L, &Data, 1, FALSE);
	
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_SYSCTRL, 0, 0, &Data, 1, FALSE);
	
	if ((((Data & 0x02)>>1)==0)||(((Data & 0x20)>>5)==1))
	{
		Data = VCO_HIGH;
        	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_SYSCTRL, F399_CMD, F399_CMD_L, &Data, 1, FALSE);
		
	}
	
	return result;
}
#endif
/*****************************************************
**FUNCTION	::	FE_399_AuxClkFreq
**ACTION	::	Compute Auxiliary clock frequency 
**PARAMS IN	::	DigFreq		==> Frequency of the digital synthetizer (Hz)
**				Prescaler	==> value of the prescaler
**				Divider		==> value of the divider
**PARAMS OUT::	NONE
**RETURN	::	Auxiliary Clock frequency
*****************************************************/
U32 FE_399_AuxClkFreq(U32 DigFreq,U32 Prescaler,U32 Divider)
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
			factor = (U32)(2048L*FE_399E_PowOf2(Prescaler-2));
			if(factor)
				aclk = DigFreq/factor/(32+Divider);
			else
				aclk = 0;
		break;
	}
	
	return aclk; 
}

/*****************************************************
**FUNCTION	::	FE_399_F22Freq
**ACTION	::	Compute F22 frequency 
**PARAMS IN	::	DigFreq	==> Frequency of the digital synthetizer (Hz)
**				F22		==> Content of the F22 register
**PARAMS OUT::	NONE
**RETURN	::	F22 frequency
*****************************************************/
U32 FE_399_F22Freq(U32 DigFreq,U32 F22)
{
	return (F22!=0) ? (DigFreq/(32*F22)) : 0; 
}


FE_399_SIGNALTYPE_t FE_399_LockAgc0and1(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_399_InternalParams_t *pParams)
{
	U32 agc0int;
	U8 temp[2];
	S32 agc1gain = 0;
	#ifndef STTUNER_MINIDRIVER
	agc0int = 5;
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_AGC0_INT,agc0int);
	STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R399_AGC1S,2,temp);
	STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle, R399_AGC1S, temp, 2);			/* Write into AGC1 registers to force update	*/ 
	WAIT_N_MS_399(2); 
	
	agc1gain = FE_399_CalcAGC1Gain(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_AGC1_INT_PRIM))/2; 
	
	do
	{
		if(agc1gain < 11)
		{
			agc0int = MAX(1,agc0int-1);
			STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_AGC0_INT,agc0int);		/* Write AGC0 integrator	*/
			STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R399_AGC1S,2,temp);
			STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R399_AGC1S,temp,2);			/* Write into AGC1 registers to force update	*/ 
		}
		else if(agc1gain > 19)
		{
			agc0int = MIN(9,agc0int+1);  
			STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_AGC0_INT,agc0int);		/* Write AGC0 integrator	*/
			STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R399_AGC1S,2,temp);
			STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R399_AGC1S,temp,2);			/* Write into AGC1 registers to force update	*/ 
		}
		
		WAIT_N_MS_399(2);
		
		agc1gain = FE_399_CalcAGC1Gain(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_AGC1_INT_PRIM))/2;
		
		
	}while(((agc1gain < 11) || (agc1gain > 19)) && (agc0int != 1) && (agc0int != 9));
	
	pParams->State = AGC1OK_399E;
	#endif
	/********************MINIDRIVER*****************
	************************************************/
	#ifdef STTUNER_MINIDRIVER
	
	U8 Data, nsbuffer[2];
	agc0int = 5;
	Data = agc0int;
        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_AGC0I, F399_AGC0_INT, F399_AGC0_INT_L, &Data, 1, FALSE);
	
	nsbuffer[0] = 195;
	nsbuffer[1] = 191;
        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_AGC1S, 0, 0, nsbuffer, 2, FALSE);
	
	WAIT_N_MS_399(2); 
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_AGC1P, F399_AGC1_INT_PRIM, F399_AGC1_INT_PRIM_L, &Data, 1, FALSE);
	agc1gain = FE_399_CalcAGC1Gain(Data)/2; 
	do
	{
		if(agc1gain < 11)
		{
			agc0int = MAX(1,agc0int-1);
			Data = agc0int;
        		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_AGC0I, F399_AGC0_INT, F399_AGC0_INT_L, &Data, 1, FALSE);
			
			nsbuffer[0] = 195;
			nsbuffer[1] = 191;
        		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_AGC1S, 0, 0, nsbuffer, 2, FALSE);
		}
		else if(agc1gain > 19)
		{
			agc0int = MIN(9,agc0int+1);  
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_AGC0I, F399_AGC0_INT, F399_AGC0_INT_L, &Data, 1, FALSE);
			
			nsbuffer[0] = 195;
			nsbuffer[1] = 191;
        		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_AGC1S, 0, 0, nsbuffer, 2, FALSE);
		}		
		WAIT_N_MS_399(2);
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_AGC1P, F399_AGC1_INT_PRIM, F399_AGC1_INT_PRIM_L, &Data, 1, FALSE);
		agc1gain = FE_399_CalcAGC1Gain(Data/2);
	}while(((agc1gain < 11) || (agc1gain > 19)) && (agc0int != 1) && (agc0int != 9));
	
	pParams->State = AGC1OK_399E;
	#endif
	return pParams->State;
}


/*****************************************************
--FUNCTION	::	FE_399_CheckTiming
--ACTION	::	Check for timing locked
--PARAMS IN	::	pParams->Ttiming	=>	Time to wait for timing loop locked
--PARAMS OUT::	pParams->State		=>	result of the check
--RETURN	::	NOTIMING_399E if timing not locked, TIMINGOK_399E otherwise
--***************************************************/
FE_399_SIGNALTYPE_t FE_399_CheckTiming(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_399_InternalParams_t *pParams)
{
	int locked;
	U32 timing;
	
	#ifndef STTUNER_MINIDRIVER     
	WAIT_N_MS_399(pParams->Ttiming);
	locked=STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_TMG_LOCK_IND);
	timing=(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_TIMING_LOOP_FREQ));
	#endif
	#ifdef STTUNER_MINIDRIVER
	U8 Data;
	WAIT_N_MS_399(pParams->Ttiming);
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_TLIR, F399_TMG_LOCK_IND, F399_TMG_LOCK_IND_L, &Data, 1, FALSE);
	locked = (int)Data;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_RTF, F399_TIMING_LOOP_FREQ, F399_TIMING_LOOP_FREQ_L, &Data, 1, FALSE);
	timing = ((U32)Data - (1 << 8));
	
	#endif
	if(locked >= 42)
	{
		if((locked > 48) && (timing >= 110))
			pParams->State = ANALOGCARRIER_399E; 
		else 
			pParams->State = TIMINGOK_399E;   
	}
	else
		pParams->State = NOTIMING_399E; 	

	return pParams->State;
}


/*****************************************************
--FUNCTION	::	FE_399_CheckCarrier
--ACTION	::	Check for carrier founded
--PARAMS IN	::	pParams		=>	Pointer to FE_399_InternalParams_t structure
--PARAMS OUT::	pParams->State	=> Result of the check
--RETURN	::	NOCARRIER_399E carrier not founded, CARRIEROK_399E otherwise
--***************************************************/
FE_399_SIGNALTYPE_t FE_399_CheckCarrier(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_399_InternalParams_t *pParams)
{
	#ifndef STTUNER_MINIDRIVER						/*	wait for derotator ok	*/    
	pParams->State = NOCARRIER_399E;
	WAIT_N_MS_399(pParams->Tderot);
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_CFD_ON,0);
	if (STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_CF))
	pParams->State = CARRIEROK_399E;
	else
	pParams->State = NOCARRIER_399E;
	#endif
	
	#ifdef STTUNER_MINIDRIVER						/*	wait for derotator ok	*/    
	U8 Data;
	pParams->State = NOCARRIER_399E;
	WAIT_N_MS_399(pParams->Tderot);
	
	Data = 0;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_CFD, F399_CFD_ON, F399_CFD_ON_L, &Data, 1, FALSE);	
	
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_VSTATUS, F399_CF, F399_CF_L, &Data, 1, FALSE);
	if (Data)
	pParams->State = CARRIEROK_399E;
	else
	pParams->State = NOCARRIER_399E;
	#endif
	return pParams->State;
}
#ifndef STTUNER_MINIDRIVER
U32 FE_399_GetErrorCount(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_399_ERRORCOUNTER_t Counter)
{
	U32 lsb,msb;
	
	
	if(Counter == COUNTER1)
	{
		lsb = STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_ERROR_COUNT_LSB);
		msb = STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_ERROR_COUNT_MSB);
	}
	else
	{
		lsb = STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_ERROR_COUNT2_LSB);
		msb = STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_ERROR_COUNT2_MSB);	
	}
	
	return (MAKEWORD(msb,lsb));
}
#endif
#ifdef STTUNER_MINIDRIVER
U32 FE_399_GetErrorCount(FE_399_ERRORCOUNTER_t Counter)
{
	U8 Data;
	U32 lsb,msb;
	if(Counter == COUNTER1)
	{
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_ECNTL, F399_ERROR_COUNT_LSB, F399_ERROR_COUNT_LSB_L, &Data, 1, FALSE);
		lsb = Data;
		
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_ECNTM, F399_ERROR_COUNT_MSB, F399_ERROR_COUNT_MSB_L, &Data, 1, FALSE);
		msb = Data;
	}
	else
	{
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_ECNTL2, F399_ERROR_COUNT2_LSB, F399_ERROR_COUNT2_LSB_L, &Data, 1, FALSE);
		lsb = Data;
		
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_ECNTM2, F399_ERROR_COUNT2_MSB, F399_ERROR_COUNT2_LSB_L, &Data, 1, FALSE);
		msb = Data;
	}
	
	return (MAKEWORD(msb,lsb));
}
#endif
/****************************************************
--FUNCTION	::	FE_399_GetPacketErrors
--ACTION	::	Set error counter 2 in packet error mode and check for packet errors 
--PARAMS IN	::	NONE
--PARAMS OUT::	NONE
--RETURN	::	Content of error count 
--**************************************************/
#ifndef STTUNER_MINIDRIVER
int FE_399_GetPacketErrors(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	int PacketErrors,i;
	unsigned char CounterMode;
	
	CounterMode = STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R399_ERRCTRL2);	/* Save error counter mode */
	STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R399_ERRCTRL2,0x30);  		/* Error counter in Rate mode, packet error, count period 2^12	*/
	
	WAIT_N_MS_399(4);
	
	for(i=0;i<2;i++)									/* Do not remove the for loop : bug work around */
		PacketErrors = FE_399_GetErrorCount(DeviceMap,IOHandle,COUNTER2);	/* Error counter must be read twice before returning valid value */ 
		
	STTUNER_IOREG_SetRegister(DeviceMap,IOHandle,R399_ERRCTRL2,CounterMode);	/* restore error counter mode	*/
	
	return PacketErrors;
}
#endif
#ifdef STTUNER_MINIDRIVER
int FE_399_GetPacketErrors( )
{
	int PacketErrors,i;
	U8 Data;
	unsigned char CounterMode;
	
	
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_ERRCTRL2, 0, 0, &Data, 1, FALSE);
	CounterMode = (unsigned char)Data;
	Data = 0x30;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_ERRCTRL2, 0, 0, &Data, 1, FALSE);
	
	WAIT_N_MS_399(4);
	for(i=0;i<2;i++)/* Do not remove the for loop : bug work around */
	PacketErrors = FE_399_GetErrorCount(COUNTER2);/*Errorcounter must be read twice before returning valid value */ 
	
	Data = CounterMode;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_ERRCTRL2, 0, 0, &Data, 1, FALSE);	
	
	
	return PacketErrors;
}
#endif

/*****************************************************
--FUNCTION	::	FE_399_CheckData
--ACTION	::	Check for data founded
--PARAMS IN	::	pParams		=>	Pointer to FE_399_InternalParams_t structure    
--PARAMS OUT::	pParams->State	=> Result of the check
--RETURN	::	NODATA_399E data not founded, DATAOK_399E otherwise
--***************************************************/
FE_399_SIGNALTYPE_t FE_399_CheckData(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_399_InternalParams_t *pParams)
{
	int lock =0,i;	
	#ifndef STTUNER_MINIDRIVER
	pParams->State = NODATA_399E;
	for(i=0;i<20;i++)
	{
		lock = STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_LK);		/*	Read DATA LOCK indicator	*/
		
		if(lock)
			break;
		else
			WAIT_N_MS_399(1);
	}
	
	if(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_FECMODE) != 0x04)	/*	Read FEC mode				*/
	{
		/*	DVB Mode	*/
		if(lock)											/*	Test DATA LOCK indicator	*/
			pParams->State = DATAOK_399E;
	}
	else
	{
		/*	DSS Mode	*/
		if(lock && FE_399_GetPacketErrors(DeviceMap,IOHandle) <= 10) 	/*  Test DATA LOCK and Packet errors	*/		
			pParams->State = DATAOK_399E;
	}
	#endif	
	#ifdef STTUNER_MINIDRIVER
	U8 Data;
	pParams->State = NODATA_399E;
	for(i=0;i<20;i++)
	{
		
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_VSTATUS, F399_LK, F399_LK_L, &Data, 1, FALSE);
		lock = Data;
		if(lock)
			break;
		else
			WAIT_N_MS_399(1);
	}
	
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_FECM, F399_FECMODE, F399_FECMODE_L, &Data, 1, FALSE);
	if(Data != 0x04)	/*	Read FEC mode				*/
	{
		/*	DVB Mode	*/
		if(lock)											/*	Test DATA LOCK indicator	*/
			pParams->State = DATAOK_399E;
	}
	else
	{
		/*	DSS Mode	*/
		if(lock && FE_399_GetPacketErrors( ) <= 10) 	/*  Test DATA LOCK and Packet errors	*/		
			pParams->State = DATAOK_399E;
	}
	#endif	
	return pParams->State;
}

/*****************************************************
--FUNCTION	::	FE_399_IQInvertion
--ACTION	::	Invert I and Q
--PARAMS IN	::	NONE
--PARAMS OUT::	NONE
--RETURN	::	NONE
--***************************************************/
void FE_399_IQInvertion(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_399_InternalParams_t *pParams)
{	
	#ifndef STTUNER_MINIDRIVER
	if(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_SYM)==0)
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_SYM,1);
	else
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_SYM,0);
	#endif
	#ifdef STTUNER_MINIDRIVER
	U8 Data;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_FECM, F399_SYM, F399_SYM_L, &Data, 1, FALSE);
	if(Data==0)
	{Data = 1;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_FECM, F399_SYM, F399_SYM_L, &Data, 1, FALSE);}
	else
	{Data = 0;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_FECM, F399_SYM, F399_SYM_L, &Data, 1, FALSE);}
	#endif
}


/*****************************************************
--FUNCTION	::	FE_399_TimingTimeConstant
--ACTION	::	Compute the amount of time needed by the timing loop to lock
--PARAMS IN	::	SymbolRate	->	symbol rate value
--PARAMS OUT::	NONE
--RETURN	::	Timing loop time constant (ms)
--***************************************************/
long FE_399_TimingTimeConstant(long SymbolRate)
{
	if(SymbolRate > 0)
		return (100000/(SymbolRate/1000));
	else
		return 0;
}

/*****************************************************
--FUNCTION	::	FE_399_DerotTimeConstant
--ACTION	::	Compute the amount of time needed by the Derotator to lock
--PARAMS IN	::	SymbolRate	->	symbol rate value
--PARAMS OUT::	NONE
--RETURN	::	Derotator time constant (ms)
--***************************************************/
long FE_399_DerotTimeConstant(long SymbolRate)
{
	if(SymbolRate > 0)  
		return (10000/(SymbolRate/1000));
	else
		return 0;
}

/*****************************************************
--FUNCTION	::	FE_399_DataTimeConstant
--ACTION	::	Compute the amount of time needed to capture data 
--PARAMS IN	::	Er		->	Viterbi rror rate	
--				Sn		->  viterbi averaging period
--				To		->  viterbi time out
--				Hy		->	viterbi hysteresis
--				SymbolRate	->	symbol rate value
--PARAMS OUT::	NONE
--RETURN	::	Data time constant
--***************************************************/
long FE_399_DataTimeConstant(int Pr, int Sn,int To,int Hy,long SymbolRate)
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
		 	
	/*=======================================================================
	-- Data capture time (in ms)
    -- -------------------------
	-- This time is due to the Viterbi synchronisation.
	--
	--	For each authorized inner code, the Viterbi search time is calculated,
	--	and the results are cumulated in ViterbiSearch.	
	--  InnerCode is multiplied by 1000 in order to obtain timings in ms 
	=======================================================================*/
	
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
#ifndef STTUNER_MINIDRIVER
/****************************************************
**FUNCTION	::	FE_399_GetRollOff
**ACTION	::	Read the rolloff value
**PARAMS IN	::	hChip	==>	Handle for the chip
**PARAMS OUT::	NONE
**RETURN	::	rolloff
*****************************************************/
int  FE_399_GetRollOff(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	if (STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_MODE_COEF) == 1)
		return 20;
	else
		return 35;
}

/****************************************************
**FUNCTION	::	FE_399_Trigger
**ACTION	::	Set trigger state
**PARAMS IN	::	hChip	==>	Handle for the chip
**				State	==> 0 / 1
**PARAMS OUT::	NONE
*****************************************************/
void FE_399_Trigger(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,BOOL State)
{
	
}
#endif
/*****************************************************
**FUNCTION	::	FE_399_CalcDerotFreq
**ACTION	::	Compute Derotator frequency 
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE
**RETURN	::	Derotator frequency (KHz)
*****************************************************/
S32 FE_399_CalcDerotFreq(U8 derotmsb,U8 derotlsb,U32 fm)
{
	S32	dfreq;
	S32 Itmp;
		
	Itmp = (S16)(derotmsb<<8)+derotlsb;
	dfreq = (S32)(Itmp*(fm/10000L));
	dfreq = (S32)(dfreq / 65536L);
	dfreq *= 10;
	
	return dfreq; 
}

/*****************************************************
**FUNCTION	::	FE_399_GetDerotFreq
**ACTION	::	Read current Derotator frequency 
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE
**RETURN	::	Derotator frequency (KHz)
*****************************************************/
#ifndef STTUNER_MINIDRIVER
S32 FE_399_GetDerotFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 MasterClock)
{
	U8 temp[2];
	/*	Read registers	*/
	STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R399_CFRM,2,temp);   
	
	return FE_399_CalcDerotFreq(temp[0],temp[1],MasterClock);  	
}
#endif
#ifdef STTUNER_MINIDRIVER
S32 FE_399_GetDerotFreq(U32 MasterClock)
{
	U8 nsbuffer[2];
	/*	Read registers	*/
	
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_CFRM, 0, 0, nsbuffer, 2, FALSE);
	return FE_399_CalcDerotFreq(nsbuffer[0],nsbuffer[1],MasterClock);  	
}
#endif
/*****************************************************
**FUNCTION	::	FE_399_BinaryFloatDiv
**ACTION	::	float division (with integer) 
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE
**RETURN	::	Derotator frequency (KHz)
*****************************************************/
long FE_399_BinaryFloatDiv(long n1, long n2, int precision)
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
**FUNCTION	::	FE_399_CalcSymbolRate
**ACTION	::	Compute symbol frequency
**PARAMS IN	::	Hbyte	->	High order byte
**				Mbyte	->	Mid byte
**				Lbyte	->	Low order byte
**PARAMS OUT::	NONE
**RETURN	::	Symbol frequency
*****************************************************/
U32 FE_399_CalcSymbolRate(U32 MasterClock,U8 Hbyte,U8 Mbyte,U8 Lbyte)
{
	U32 Ltmp,
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
**FUNCTION	::	FE_399_SetSymbolRate
**ACTION	::	Set symbol frequency
**PARAMS IN	::	DeviceMap, IOHandle
**				MasterClock	->	Masterclock frequency (Hz)
**				SymbolRate	->	symbol rate (bauds)
**PARAMS OUT::	NONE
**RETURN	::	Symbol frequency
*****************************************************/
#ifndef STTUNER_MINIDRIVER
U32 FE_399_SetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 MasterClock,U32 SymbolRate)
{
	U32 U32Tmp;
	U8 temp[3];
	
	/*
	** in order to have the maximum precision, the symbol rate entered into
	** the chip is computed as the closest value of the "true value".
	** In this purpose, the symbol rate value is rounded (1 is added on the bit
	** below the LSB )
	*/
	U32Tmp = (U32)FE_399_BinaryFloatDiv(SymbolRate,MasterClock,20);       
		
	temp[0]=(U32Tmp>>12)&0xFF;
	temp[1]=(U32Tmp>>4)&0xFF;
	
	temp[2]=U32Tmp&0x0F;
	STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle, R399_SFRH, temp, 3);     
	
	return(SymbolRate) ;
}
#endif
#ifdef STTUNER_MINIDRIVER
U32 FE_399_SetSymbolRate(U32 MasterClock,U32 SymbolRate)
{
	U32 U32Tmp;
	U8 nsbuffer[3];
	/*
	** in order to have the maximum precision, the symbol rate entered into
	** the chip is computed as the closest value of the "true value".
	** In this purpose, the symbol rate value is rounded (1 is added on the bit
	** below the LSB )
	*/
	U32Tmp = (U32)FE_399_BinaryFloatDiv(SymbolRate,MasterClock,20);       
	
	nsbuffer[0] = (U8)((U32Tmp>>12)&0xFF);
	nsbuffer[1] = (U8)((U32Tmp>>4)&0xFF);
	nsbuffer[2] = (U8)((U32Tmp&0x0F)<<4);
	
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_SFRH, 0, 0, nsbuffer, 3, FALSE);
	return(SymbolRate) ;
}
#endif
/*****************************************************
**FUNCTION	::	FE_399_IncSymbolRate
**ACTION	::	Increment symbol frequency
**PARAMS IN	::	hChip		->	handle to the chip
**				MasterClock	->	Masterclock frequency (Hz)
**				Increment	->	positive or negative increment
**PARAMS OUT::	NONE
**RETURN	::	Symbol frequency
*****************************************************/
#ifndef STTUNER_MINIDRIVER
void FE_399_IncSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,S32 Increment)
{
	U32  U32Tmp;
	U8 temp[2];
	
	STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R399_SFRH,3,temp);     
	
	U32Tmp = (temp[0] & 0xFF)<<12;
	U32Tmp += (temp[1] & 0xFF)<<4;  
	U32Tmp += (temp[2] & 0x0F);  
	
	U32Tmp += Increment;
	
	temp[0]=(U32Tmp>>12)&0xFF;
	temp[1]=(U32Tmp>>4)&0xFF;
	temp[2]=(U32Tmp&0x0F);
	
	STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle, R399_SFRH, temp, 3);     
}


void FE_399_OffsetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 MasterClock,S32 Offset)
{
	U32  U32Tmp;
	S32 tst;
	U8 temp[2];
	STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R399_SFRH,3,temp);     
	temp[2]=(temp[2]>>4)&0x0F;
	U32Tmp = (temp[0] & 0xFF)<<12;
	U32Tmp += (temp[1] & 0xFF)<<4;  
	U32Tmp += (temp[2] & 0x0F);  
	
	
	tst = (Offset*(S32)U32Tmp)>>19; 
	U32Tmp += tst;   
	
	temp[0]=(U32Tmp>>12)&0xFF;
	temp[1]=(U32Tmp>>4)&0xFF;
	temp[3]=U32Tmp&0x0F;

	
	STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle, R399_SFRH, temp, 3);     
}
#endif
/*****************************************************
**FUNCTION	::	FE_399_GetSymbolRate
**ACTION	::	Get the current symbol rate
**PARAMS IN	::	hChip		->	handle to the chip
**				MasterClock	->	Masterclock frequency (Hz)
**PARAMS OUT::	NONE
**RETURN	::	Symbol rate
*****************************************************/
#ifndef STTUNER_MINIDRIVER
U32 FE_399_GetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 MasterClock)
{
	U8 temp[2];
	STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R399_SFRH,3,temp);
	temp[2]=(temp[2]&0x0F)>>4;
	return FE_399_CalcSymbolRate(MasterClock,temp[0],temp[1],temp[2]);
									
    
}
#endif
#ifdef STTUNER_MINIDRIVER
U32 FE_399_GetSymbolRate(U32 MasterClock)
{
	U8 nsbuffer[3];
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_SFRH, 0, 0, nsbuffer, 3, FALSE);
	
	return FE_399_CalcSymbolRate(	MasterClock,
									nsbuffer[0]/*HSB*/,
									nsbuffer[1]/*MSB*/,
									nsbuffer[2]/*LSB*/);
									
    
}
#endif
/*****************************************************
--FUNCTION	::	FE_399_CarrierWidth
--ACTION	::	Compute the width of the carrier
--PARAMS IN	::	SymbolRate	->	Symbol rate of the carrier (Kbauds or Mbauds)
--				RollOff		->	Rolloff * 100
--PARAMS OUT::	NONE
--RETURN	::	Width of the carrier (KHz or MHz) 
--***************************************************/
long FE_399_CarrierWidth(long SymbolRate, long RollOff)
{
	return (SymbolRate  + (SymbolRate*RollOff)/100);
}


/*****************************************************
--FUNCTION	::	FE_399_InitialCalculations
--ACTION	::	Set Params fields that are never changed during search algorithm   
--PARAMS IN	::	NONE
--PARAMS OUT::	NONE
--RETURN	::	NONE
--***************************************************/
void FE_399_InitialCalculations(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_399_InternalParams_t *pParams)
{
	U8 temp[1];
	
	#ifndef STTUNER_MINIDRIVER
	int		stdby,
			m1,betaagc1,
			agc2coef;	
	/*	Read registers (in burst mode)	*/
	STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R399_AGC1C);
	STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R399_AGC1R,2,temp);		/*	Read AGC1R and AGC2O registers */
	
	/*	Get fields values	*/
	stdby=STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_STANDBY);
	m1=STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_AGC1R_REF);
	betaagc1=STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_AGC1C_BETA1);   
	agc2coef=STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_AGC2COEFF);
	
	/*	Initial calculations	*/ 
	pParams->Tagc1 = 1;
	pParams->Tagc2 = 1;
	pParams->RollOff = FE_399_GetRollOff(DeviceMap,IOHandle);
	#endif
	#ifdef STTUNER_MINIDRIVER	
	/*	Initial calculations	*/ 
	pParams->Tagc1 = 1;
	pParams->Tagc2 = 1;
	
	#endif
}


/*****************************************************
--FUNCTION	::	FE_399_SearchTiming
--ACTION	::	Perform an Fs/2 zig zag to found timing
--PARAMS IN	::	NONE
--PARAMS OUT::	NONE
--RETURN	::	NOTIMING_399E if no valid timing had been found, TIMINGOK_399E otherwise
--***************************************************/
FE_399_SIGNALTYPE_t FE_399_SearchTiming(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_399_InternalParams_t *pParams)
{
	S16	DerotStep,
		DerotFreq = 0,
		DerotMin,DerotMax,
		NextLoop = 2;
		
	S32 index = 0;
	U8 temp[2];
			
	#ifndef STTUNER_MINIDRIVER
	pParams->State = NOTIMING_399E;
	
	REPORTMSG(SEARCHTIMINGMSG,pParams->Frequency,DerotFreq*pParams->Mclk)
	
	/* timing loop computation & symbol rate optimisation	*/
	DerotMin =  (short int)  (((-(long)(pParams->SubRange))/ 2L + pParams->FreqOffset) / pParams->Mclk); /* introduce 14 MHz derotator offset */
    	DerotMax =  (short int)  ((pParams->SubRange/2L + pParams->FreqOffset)/pParams->Mclk);  /* introduce 14 MHz derotator offset */
	DerotStep = (S16)((pParams->SymbolRate/2L)/pParams->Mclk);    
	while((FE_399_CheckTiming(DeviceMap,IOHandle,pParams)!=TIMINGOK_399E) && NextLoop)
	{
		REPORTMSG(SEARCHOFFSET,pParams->Frequency,DerotFreq*pParams->Mclk)
		
		index++;
		DerotFreq += index*pParams->Direction*DerotStep;	/*	Compute the next derotator position for the zig zag	*/    
		
		if((DerotFreq < DerotMin) || (DerotFreq > DerotMax))
			NextLoop--;
			
		if(NextLoop)
		{
			temp[0]=MSB(DerotFreq);
			temp[1]=LSB(DerotFreq);
			
			STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R399_CFRM,temp,2); 							/*	Set the derotator frequency	*/
		}
		
		pParams->Direction = -pParams->Direction;			/*	Change the zigzag direction	*/    
	}
	
	
	if(pParams->State == TIMINGOK_399E)
	{
		pParams->Results.SymbolRate = pParams->SymbolRate; 
		STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R399_CFRM,2,temp); 								/*	Get the derotator frequency	*/ 
		pParams->DerotFreq = (short int) MAKEWORD(temp[0], temp[1]);
	}
	#endif
	/************************MINIDRIVER***********************/
	#ifdef STTUNER_MINIDRIVER
	U8 nsbuffer[2];
	pParams->State = NOTIMING_399E;
	REPORTMSG(SEARCHTIMINGMSG,pParams->Frequency,DerotFreq*pParams->Mclk)
	/* timing loop computation & symbol rate optimisation	*/
	DerotMin =  (short int)  (((-(long)(pParams->SubRange))/ 2L + pParams->FreqOffset) / pParams->Mclk); /* introduce 14 MHz derotator offset */
    	DerotMax =  (short int)  ((pParams->SubRange/2L + pParams->FreqOffset)/pParams->Mclk);  /* introduce 14 MHz derotator offset */
	DerotStep = (S16)((pParams->SymbolRate/2L)/pParams->Mclk);    
	while((FE_399_CheckTiming(pParams)!=TIMINGOK_399E) && NextLoop)
	{
		index++;
		DerotFreq += index*pParams->Direction*DerotStep;
		if((DerotFreq < DerotMin) || (DerotFreq > DerotMax))
			NextLoop--;
			
		if(NextLoop)
		{
			nsbuffer[0] = MSB(DerotFreq);
    	        	nsbuffer[1] = LSB(DerotFreq);
    	        	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_CFRM, 0, 0, nsbuffer,2, FALSE);
		}		
		pParams->Direction = -pParams->Direction;
	}	
	if(pParams->State == TIMINGOK_399E)
	{
		pParams->Results.SymbolRate = pParams->SymbolRate; 
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_CFRM, 0, 0, nsbuffer,2, FALSE);
		pParams->DerotFreq = (short int) MAKEWORD(nsbuffer[0],nsbuffer[1]);
	}
	#endif
	return pParams->State;
}

/****************************************************
--FUNCTION	::	FE_399_SearchLock
--ACTION	::	Search for Lock 
--PARAMS IN	::	pParams->Tdata	=>	Time to wait for data
--PARAMS OUT::	pParams->State	=>	Result of the search
--RETURN	::	NODATA_399E if data not founded, DATAOK_399E otherwise 
--**************************************************/
FE_399_SIGNALTYPE_t FE_399_SearchLock(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_399_InternalParams_t *pParams)
{
	#ifndef STTUNER_MINIDRIVER
	switch(pParams->DemodIQMode)
	{
		case 0:
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_SYM,0);
		FE_399_CheckData(DeviceMap,IOHandle,pParams);
		break;
		case 1:
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_SYM,1);
		FE_399_CheckData(DeviceMap,IOHandle,pParams);
		break;
		case 2:
		if(FE_399_CheckData(DeviceMap,IOHandle,pParams) != DATAOK_399E)		/*	Check for data	*/
		{
			FE_399_IQInvertion(DeviceMap,IOHandle,pParams);			/*	Invert I and Q	*/
	
			if(FE_399_CheckData(DeviceMap,IOHandle,pParams) != DATAOK_399E) /*	Check for data	*/
			{
				FE_399_IQInvertion(DeviceMap,IOHandle,pParams);		/*	Invert I and Q	*/
			}
			else
			{
				REPORTMSG(SEARCHIQSWAP,0,0)
			}
		 }
		 break;
		 
		 default:
		 break;
        }
	#endif
	/***************************************MINIDRIVER***************************
	*******************************************************************************/
	#ifdef STTUNER_MINIDRIVER
	U8 Data;
	switch(pParams->DemodIQMode)
	{
		case 0:
		Data = 0;
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_FECM, F399_SYM, F399_SYM_L, &Data, 1, FALSE);
		FE_399_CheckData(pParams);
		break;
		case 1:
		Data = 1;
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_FECM, F399_SYM, F399_SYM_L, &Data, 1, FALSE);
		FE_399_CheckData(pParams);
		break;
		case 2:
		if(FE_399_CheckData(pParams) != DATAOK_399E)		/*	Check for data	*/
		{
			FE_399_IQInvertion(pParams);			/*	Invert I and Q	*/
	
			if(FE_399_CheckData(pParams) != DATAOK_399E) /*	Check for data	*/
			{
				FE_399_IQInvertion(pParams);		/*	Invert I and Q	*/
			}
			else
			{
				REPORTMSG(SEARCHIQSWAP,0,0)
			}
		 }
		 break;
		 
		 default:
		 break;
        }
	#endif
	return pParams->State;
}


/*****************************************************
--FUNCTION	::	FE_399_SearchCarrier
--ACTION	::	Search a QPSK carrier with the derotator
--PARAMS IN	::	
--PARAMS OUT::	NONE
--RETURN	::	NOCARRIER_399E if no carrier had been found, CARRIEROK_399E otherwise 
--***************************************************/
FE_399_SIGNALTYPE_t FE_399_SearchCarrier(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_399_InternalParams_t *pParams)
{
	short int	DerotFreq = 0,
		LastDerotFreq = 0, 
		DerotMin,DerotMax,
		NextLoop = 2;
	int 	index = 0;
	U8 temp[2];

	#ifndef STTUNER_MINIDRIVER
	pParams->State = NOCARRIER_399E;
	
	REPORTMSG(SEARCHCARRIERMSG,0,0)   
	
	DerotMin =  (short int)  (((-(long)(pParams->SubRange))/ 2L + pParams->FreqOffset) / pParams->Mclk); /* introduce 14 MHz derotator offset */
    	DerotMax =  (short int)  ((pParams->SubRange/2L + pParams->FreqOffset)/pParams->Mclk);  /* introduce 14 MHz derotator offset */
	DerotFreq = pParams->DerotFreq;
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_CFD_ON,1);
	
	do
	{
		REPORTMSG(SEARCHOFFSET,pParams->Frequency,DerotFreq*pParams->Mclk)  
		
		if(FE_399_CheckCarrier(DeviceMap,IOHandle,pParams) != CARRIEROK_399E)
		{
			index++;
			LastDerotFreq = DerotFreq;
			DerotFreq += index*pParams->Direction*pParams->DerotStep;	/*	Compute the next derotator position for the zig zag	*/    
		
			if((DerotFreq < DerotMin) || (DerotFreq > DerotMax))
				NextLoop--;
				
			if(NextLoop)
			{
				temp[0]=MSB(DerotFreq);
				temp[1]=LSB(DerotFreq);   
				STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R399_CFRM,temp,2);
				STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_CFD_ON,1); 							/*	Set the derotator frequency	*/
			}
		}
		else
		{
			pParams->Results.SymbolRate = pParams->SymbolRate;
		}
		
		pParams->Direction = -pParams->Direction;			/*	Change the zigzag direction	*/    
	}
	while(	(pParams->State!=CARRIEROK_399E) && NextLoop);
	
	
	if(pParams->State == CARRIEROK_399E)
	{
		STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R399_CFRM,2,temp); 								/*	Get the derotator frequency	*/ 
		pParams->DerotFreq = (short int) MAKEWORD(temp[0], temp[1]);
	}
	else
	{
		pParams->DerotFreq = LastDerotFreq;	
	}
	
	#endif
	/****************************MINIDRIVER****************************
	***********************************************************************/
	#ifdef STTUNER_MINIDRIVER
	U8 nsbuffer[2], Data;
	pParams->State = NOCARRIER_399E;
	
	REPORTMSG(SEARCHCARRIERMSG,0,0)   
	
	DerotMin =  (short int)  (((-(long)(pParams->SubRange))/ 2L + pParams->FreqOffset) / pParams->Mclk); /* introduce 14 MHz derotator offset */
    	DerotMax =  (short int)  ((pParams->SubRange/2L + pParams->FreqOffset)/pParams->Mclk);  /* introduce 14 MHz derotator offset */
	DerotFreq = pParams->DerotFreq;
	Data = 1;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_CFD, F399_CFD_ON, F399_CFD_ON_L, &Data,1, FALSE);
	do
	{
		if(FE_399_CheckCarrier(pParams) != CARRIEROK_399E)
		{
			index++;
			LastDerotFreq = DerotFreq;
			DerotFreq += index*pParams->Direction*pParams->DerotStep;
			if((DerotFreq < DerotMin) || (DerotFreq > DerotMax))
				NextLoop--;
				
			if(NextLoop)
			{
				nsbuffer[0] = MSB(DerotFreq);
    	        		nsbuffer[1] = LSB(DerotFreq);
    	        		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_CFRM, 0, 0, nsbuffer,2, FALSE);
				Data = 1;
				STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_CFD, F399_CFD_ON, F399_CFD_ON_L, &Data,1, FALSE);

			}
		}
		else
		{
			pParams->Results.SymbolRate = pParams->SymbolRate;
		}
		
		pParams->Direction = -pParams->Direction;
	}
	while(	(pParams->State!=CARRIEROK_399E) && NextLoop);
	
	
	if(pParams->State == CARRIEROK_399E)
	{
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_CFRM, 0, 0, nsbuffer,2, FALSE);
		pParams->DerotFreq = (short int) MAKEWORD(nsbuffer[0],nsbuffer[1]);
	}
	else
	{
		pParams->DerotFreq = LastDerotFreq;	
	}
	#endif
	return pParams->State;
}


/*****************************************************
--FUNCTION	::	FE_399_SearchData
--ACTION	::	Search a QPSK carrier with the derotator, even if there is a false lock 
--PARAMS IN	::	
--PARAMS OUT::	NONE
--RETURN	::	NOCARRIER_399E if no carrier had been found, CARRIEROK_399E otherwise 
--***************************************************/
FE_399_SIGNALTYPE_t FE_399_SearchData(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_399_InternalParams_t *pParams)
{
	S16	DerotFreq,
		DerotStep,
		DerotMin,DerotMax,
		NextLoop = 3;
	S32 index = 1;
	U8 temp[2];
	
	#ifndef STTUNER_MINIDRIVER
	DerotStep = (S16)((pParams->SymbolRate/4L)/pParams->Mclk);
	DerotMin =  (short int)  (((-(long)(pParams->SubRange))/ 2L + pParams->FreqOffset) / pParams->Mclk); /* introduce 14 MHz derotator offset */
        DerotMax =  (short int)  ((pParams->SubRange/2L + pParams->FreqOffset)/pParams->Mclk);  /* introduce 14 MHz derotator offset */
	DerotFreq = pParams->DerotFreq;
	
	REPORTMSG(SEARCHDATAMSG,0,0)   
	do
	{
		REPORTMSG(SEARCHOFFSET,pParams->Frequency,DerotFreq*pParams->Mclk);
	
		if((pParams->State != CARRIEROK_399E) || (FE_399_SearchLock(DeviceMap,IOHandle,pParams)!= DATAOK_399E))
		{
			DerotFreq += index*pParams->Direction*DerotStep;		/*	Compute the next derotator position for the zig zag	*/ 
		
			if((DerotFreq < DerotMin) || (DerotFreq > DerotMax))
				NextLoop--;
			
			if(NextLoop)
			{
				
				
				STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_CFD_ON,1);
				temp[0]=MSB(DerotFreq);
				temp[1]=LSB(DerotFreq);
				STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R399_CFRM,temp,2); 						/*	Reset the derotator frequency	*/
				
				WAIT_N_MS_399(pParams->Ttiming);
				FE_399_CheckCarrier(DeviceMap,IOHandle,pParams);
				
				index++;
			}
		}
		
		pParams->Direction = -pParams->Direction;			/*	Change the zigzag direction	*/  
	}
	while((pParams->State != DATAOK_399E) && NextLoop);
	
	if(pParams->State == DATAOK_399E)
	{
		STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R399_CFRM,2,temp); 								/*	Get the derotator frequency	*/ 
		pParams->DerotFreq = (short int) MAKEWORD(temp[0], temp[1]);
	}	
	#endif
	/****************************MINIDRIVER***********************
	*************************************************************/
	#ifdef STTUNER_MINIDRIVER
	U8 nsbuffer[2], Data;
	DerotStep = (S16)((pParams->SymbolRate/4L)/pParams->Mclk);
	DerotMin =  (short int)  (((-(long)(pParams->SubRange))/ 2L + pParams->FreqOffset) / pParams->Mclk); /* introduce 14 MHz derotator offset */
    DerotMax =  (short int)  ((pParams->SubRange/2L + pParams->FreqOffset)/pParams->Mclk);  /* introduce 14 MHz derotator offset */
	DerotFreq = pParams->DerotFreq;
	
	REPORTMSG(SEARCHDATAMSG,0,0)  
	do
	{
		if((pParams->State != CARRIEROK_399E) || (FE_399_SearchLock(pParams)!= DATAOK_399E))
		{
			DerotFreq += index*pParams->Direction*DerotStep;
			if((DerotFreq < DerotMin) || (DerotFreq > DerotMax))
				NextLoop--;
			
			if(NextLoop)
			{
				Data = 1;
				STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_CFD, F399_CFD_ON, F399_CFD_ON_L, &Data,1, FALSE);
				nsbuffer[0] = MSB(DerotFreq);
    	        		nsbuffer[1] = LSB(DerotFreq);
    	        		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_CFRM, 0, 0, nsbuffer,2, FALSE);
				
				WAIT_N_MS_399(pParams->Ttiming);
				FE_399_CheckCarrier(pParams);				
				index++;
			}
		}
		
		pParams->Direction = -pParams->Direction;
	}
	while((pParams->State != DATAOK_399E) && NextLoop);
	
	if(pParams->State == DATAOK_399E)
	{
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_CFRM, 0, 0, nsbuffer,2, FALSE);
		pParams->DerotFreq = (short int) MAKEWORD(nsbuffer[0],nsbuffer[1]);
	}	
	#endif
	return pParams->State;
}



/****************************************************
--FUNCTION	::	FE_399_CheckRange
--ACTION	::	Check if the founded frequency is in the correct range
--PARAMS IN	::	pParams->BaseFreq =>
--PARAMS OUT::	pParams->State	=>	Result of the check
--RETURN	::	RANGEOK_399E if check success, OUTOFRANGE_399E otherwise
--***************************************************/
FE_399_SIGNALTYPE_t FE_399_CheckRange(FE_399_InternalParams_t *pParams, S32 tuner_freq_correction)
{
	int	RangeOffset,
		TransponderFrequency;

	RangeOffset = pParams->SearchRange/2000;
	TransponderFrequency = pParams->Frequency + (pParams->DerotFreq * pParams->Mclk)/1000;
	if(tuner_freq_correction < 0)
	{
		tuner_freq_correction *= -1;
	}
	
        if((TransponderFrequency >= pParams->BaseFreq - RangeOffset - tuner_freq_correction)
	&& (TransponderFrequency <= pParams->BaseFreq + RangeOffset + tuner_freq_correction))
		pParams->State = RANGEOK_399E;
	else
		{
			pParams->State = OUTOFRANGE_399E;
		}
		

	return pParams->State;
}
#ifndef STTUNER_MINIDRIVER
/****************************************************
--FUNCTION	::	FE_399_CarrierNotCentered
--ACTION	::	Check if the carrier is correctly centered
--PARAMS IN	::		
--PARAMS OUT::	
--RETURN	::	1 if not centered, 0 otherwise
--***************************************************/
int FE_399_CarrierNotCentered(FE_399_InternalParams_t *pParams,int AllowedOffset)
{
	S32 NotCentered = 0,
		DerotFreq,
		Fs;
	
	Fs = (S32)FE_399_CarrierWidth(pParams->SymbolRate,pParams->RollOff);
	DerotFreq = ABS(pParams->DerotFreq * pParams->Mclk); 
	
	if(Fs < 4000000)
		NotCentered = (pParams->TunerBW - Fs)/4 ;
	else
		NotCentered = ((pParams->TunerBW/2 - DerotFreq - Fs/2) < AllowedOffset) && (DerotFreq > pParams->TunerStep);
	
	return NotCentered;
}
#endif


/****************************************************
--FUNCTION	::	FE_399_FirstSubRange
--ACTION	::	Compute the first SubRange of the search 
--PARAMS IN	::	pParams->SearchRange
--PARAMS OUT::	pParams->SubRange
--RETURN	::	NONE
--***************************************************/
void FE_399_FirstSubRange(FE_399_InternalParams_t *pParams)
{
	pParams->SubRange = MIN(pParams->SearchRange,pParams->TunerBW);
	
	pParams->Frequency = pParams->BaseFreq;
	pParams->TunerOffset = (S32)0L;
	
	pParams->SubDir = 1; 
}

  
/****************************************************
--FUNCTION	::	FE_399_NextSubRange
--ACTION	::	Compute the next SubRange of the search 
--PARAMS IN	::	Frequency	->	Start frequency
--				pParams->SearchRange
--PARAMS OUT::	pParams->SubRange
--RETURN	::	NONE
--***************************************************/
void FE_399_NextSubRange(FE_399_InternalParams_t *pParams)
{
	S32 OldSubRange;
	
	if(pParams->SubDir > 0)
	{
		OldSubRange = pParams->SubRange;
		pParams->SubRange = MIN((pParams->SearchRange/2) - (pParams->TunerOffset + OldSubRange/2),pParams->TunerBW);
		if(pParams->SubRange < 0)
			pParams->SubRange = 0;	
		pParams->TunerOffset += (OldSubRange + pParams->SubRange)/2;
	}

	pParams->Frequency = pParams->BaseFreq + (pParams->SubDir * pParams->TunerOffset)/1000;    
	pParams->SubDir = -pParams->SubDir;
}

/*****************************************************
**FUNCTION	::	TunerSelectBandwidth
**ACTION	::	Select the best low pass filter according to the symbol rate searched
**PARAMS IN	::	pParams	==> pointer to internal params structure
**PARAMS OUT::	NONE  
**RETURN	::	Selected bandwidth in KHz, 0 if error
*****************************************************/
U32 FE_399_SelectLPF(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_399_InternalParams_t *pParams,U32 SymbolRate)
{
	U32 filterBand = 0,
		neededBand;
	#ifndef STTUNER_MINIDRIVER
	if(pParams != NULL)
	{
		neededBand = (U32)(FE_399_CarrierWidth(pParams->SymbolRate,pParams->RollOff) + 5000000);
	
		if(neededBand <= 10000000)
		{
			filterBand = 48000000;
			STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_FILTER,0x38);
		}
		else
		{
			filterBand = 88000000;
			STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_FILTER,0x3F); 
		}
		
		if(SymbolRate <= 4000000)
		{
		 STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_FILTER,0x0);
		}
		
		if((SymbolRate > 4000000)&&(SymbolRate <= 8000000))
		{
		 STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_FILTER,0x20);
		}
		
		if((SymbolRate > 8000000)&&(SymbolRate <= 22000000))
		{
		 STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_FILTER,0x30);
		}
		
		if((SymbolRate > 22000000)&&(SymbolRate <= 32000000))
		{
		 STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_FILTER,0x38);
		}
		
		if((SymbolRate > 32000000)&&(SymbolRate <= 40000000))
		{
		 STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_FILTER,0x3C);
		}
		
		if(SymbolRate > 40000000)
		{
		 STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_FILTER,0x3E);
		}		
	}
	#endif
	/************************MINIDRIVER**********************
	**********************************************************/
	#ifdef STTUNER_MINIDRIVER
	U8 Data;
	if((pParams != NULL) && (!pParams->Error.Type))
	{
		neededBand = (U32)(FE_399_CarrierWidth(pParams->SymbolRate,pParams->RollOff) + 5000000);
	
		if(neededBand <= 10000000)
		{
			filterBand = 48000000;
			Data = 0x38;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_LPF, F399_FILTER, F399_FILTER_L, &Data, 1, FALSE);
			
		}
		else
		{
			filterBand = 88000000;
			Data = 0x3f;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_LPF, F399_FILTER, F399_FILTER_L, &Data, 1, FALSE);
			
		}		
		if(SymbolRate <= 4000000)
		{
		 Data = 0x0;
		 STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_LPF, F399_FILTER, F399_FILTER_L, &Data, 1, FALSE);
		
		}
		
		if((SymbolRate > 4000000)&&(SymbolRate <= 8000000))
		{
		 Data = 0x20;
		 STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_LPF, F399_FILTER, F399_FILTER_L, &Data, 1, FALSE);
		
		}
		
		if((SymbolRate > 8000000)&&(SymbolRate <= 22000000))
		{
		 Data = 0x30;
		 STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_LPF, F399_FILTER, F399_FILTER_L, &Data, 1, FALSE);
		
		}
		
		if((SymbolRate > 22000000)&&(SymbolRate <= 32000000))
		{
		 Data = 0x38;
		 STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_LPF, F399_FILTER, F399_FILTER_L, &Data, 1, FALSE);
		 
		}
		
		if((SymbolRate > 32000000)&&(SymbolRate <= 40000000))
		{
		 Data = 0x3C;
	         STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_LPF, F399_FILTER, F399_FILTER_L, &Data, 1, FALSE);
		 
		}
		
		if(SymbolRate > 40000000)
		{
		 Data = 0x3E;
		 STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_LPF, F399_FILTER, F399_FILTER_L, &Data, 1, FALSE);
		 
		}		
	}
	#endif
	
	return filterBand;
}

/*****************************************************
--FUNCTION	::	FE_399_Algo
--ACTION	::	Search for Signal, Timing, Carrier and then data at a given Frequency, 
--				in a given range
--PARAMS IN	::	NONE
--PARAMS OUT::	NONE
--RETURN	::	Type of the founded signal (if any)
--***************************************************/
FE_399_SIGNALTYPE_t FE_399_Algo(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_399_InternalParams_t *pParams )
{
	S32	pr,sn,to,hy;
	S32 tuner_freq, tuner_freq_correction;
	int	TransponderFrequency;
	U8 temp[2];
    
	#ifndef STTUNER_MINIDRIVER
	pParams->Frequency = pParams->BaseFreq;
	pParams->Direction = 1;
	/*	Insert I2C access here	*/
	FE_399_Trigger(DeviceMap,IOHandle,TRUE);	/*______________________________________TRIGGER____________________________________________*/  
	
	/*	Get fields value	*/
	pr = (STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R399_PR) & 0x3F);
	sn=STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_SN); /* SN,TO and H are in the same register */  
	to=STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_TO);  
	hy=STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_H);  
	  
	
	
	pParams->Ttiming = (S16)FE_399_TimingTimeConstant(pParams->SymbolRate);
	pParams->Tderot = (S16)FE_399_DerotTimeConstant(pParams->SymbolRate);
	pParams->Tdata = (S16)FE_399_DataTimeConstant(pr,sn,to,hy,pParams->SymbolRate);
	FE_399_Trigger(DeviceMap,IOHandle,FALSE);	/*______________________________________TRIGGER____________________________________________*/ 
	
	FE_399_FirstSubRange(pParams);
	
	do
	{ 
		/*	Initialisations	*/
		if(pParams->SymbolRate > 4000000)
			tuner_freq = FE_399_SetTunerFreq(DeviceMap,IOHandle,pParams->Quartz,pParams->Frequency*1000,pParams->SymbolRate)/1000;	/*	Move the tuner to exact frequency	*/ 
			
		else
			tuner_freq = FE_399_SetTunerFreq(DeviceMap,IOHandle,pParams->Quartz,(pParams->Frequency*1000)+2000000,pParams->SymbolRate)/1000;	/*	Move the tuner with 2Mhz offset	*/ 
			
			tuner_freq_correction = (pParams->Frequency - tuner_freq);
			
		pParams->MasterClock = FE_399_SelectMclk(DeviceMap,IOHandle,tuner_freq,pParams->SymbolRate,pParams->Quartz);			/*	Select the best Master clock */ 
		pParams->Mclk = (U32)(pParams->MasterClock/65536L);												
		FE_399_SetSymbolRate(DeviceMap,IOHandle,pParams->MasterClock,pParams->SymbolRate);					/*	Set the symbol rate	*/
		
		
		/* Carrier loop optimization versus symbol rate */
		if(pParams->SymbolRate <= 2000000)
			STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_BETA,0x17);
		else if(pParams->SymbolRate <= 5000000)
			STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_BETA,0x1C);    
		else if(pParams->SymbolRate <= 15000000) 
			STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_BETA,0x22);    
		else if(pParams->SymbolRate <= 30000000)
			STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_BETA,0x27);    
		else
			STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_BETA,0x29);  
		
		pParams->DerotStep = (S16)(pParams->DerotPercent*(pParams->SymbolRate/1000L)/pParams->Mclk);	/*	step of DerotStep/1000 * Fsymbol	*/     
	
		/*	Temporisations	*/
		WAIT_N_MS_399(pParams->Ttiming);		/*	Wait for agc1,agc2 and timing loop	*/
		
		REPORTMSG(SEARCHFREQUENCY,pParams->Frequency,0) 
		REPORTMSG(TUNERFREQUENCY,tuner_freq,0) 
		
		pParams->FreqOffset = (pParams->Frequency-tuner_freq) * 1000;
		pParams->DerotFreq = pParams->FreqOffset/pParams->Mclk;
		temp[0]=MSB(pParams->DerotFreq);
		temp[1]=LSB(pParams->DerotFreq);
		STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R399_CFRM,temp,2);		/*	Reset of the derotator frequency	*/
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_TIMING_LOOP_FREQ,0);
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_CFD_ON,1);
	
		pParams->Frequency = tuner_freq;
		pParams->State = NOAGC1_399E;
		
		FE_399_Trigger(DeviceMap,IOHandle,TRUE);	/*______________________________________TRIGGER____________________________________________*/    
		
		if(FE_399_LockAgc0and1(DeviceMap,IOHandle,pParams) == AGC1OK_399E)
		{
			
			REPORTMSG(SEARCHAGC1,0,0)   
			
			if(FE_399_SearchTiming(DeviceMap,IOHandle,pParams) == TIMINGOK_399E)		/*	Search for timing	*/
			{	
				REPORTMSG(SEARCHTIMING,pParams->Frequency,pParams->DerotFreq*pParams->Mclk)   
				
				if(FE_399_SearchCarrier(DeviceMap,IOHandle,pParams) == CARRIEROK_399E)	/*	Search for carrier	*/  			
				{
					REPORTMSG(SEARCHCARRIER,pParams->Frequency,pParams->DerotFreq*pParams->Mclk)  
					if((MAC0399_ABS(pParams->DerotFreq* pParams->Mclk))>1000000 && (pParams->SymbolRate > 4000000)&& (MAC0399_ABS(pParams->DerotFreq* pParams->Mclk)) <= pParams->SearchRange/2)
					{
						TransponderFrequency = (int)(pParams->Frequency + (pParams->DerotFreq * pParams->Mclk)/1000);  /* cast to eliminate compiler warning --SFS */
						tuner_freq = FE_399_SetTunerFreq(DeviceMap,IOHandle,pParams->Quartz,TransponderFrequency*1000,pParams->SymbolRate)/1000;	/*	Move the tuner to exact frequency	*/ 
						pParams->Frequency = tuner_freq;
						pParams->DerotFreq = ((TransponderFrequency - tuner_freq)*1000)/pParams->Mclk;
						temp[0]=MSB(pParams->DerotFreq);
		                                temp[1]=LSB(pParams->DerotFreq);
						STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_CFD_ON,0);
						STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle, R399_CFRM, temp, 2);
						STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_CFD_ON,1);
					}
					
					if(FE_399_SearchData(DeviceMap,IOHandle,pParams) == DATAOK_399E)	/*	Check for data	*/ 
					{
						if(FE_399_CheckRange(pParams, tuner_freq_correction) == RANGEOK_399E)
						{
								pParams->Results.Frequency = pParams->Frequency + (pParams->DerotFreq*pParams->Mclk)/1000; 
								pParams->Results.PunctureRate = 1 << STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_PR); 
								
								
								REPORTMSG(SEARCHDATA,pParams->Results.Frequency,0)  
								REPORTMSG(SEARCH_SR_PR,pParams->Results.SymbolRate,pParams->Results.PunctureRate)
						}
					}
				}
			}
		}
		
		if(pParams->State != RANGEOK_399E) 
			FE_399_NextSubRange(pParams);
			
		
	}	  
	while(pParams->SubRange && pParams->State!=RANGEOK_399E );
	
	REPORTMSG(SEARCHSTATUS,pParams->State,0);
	
	pParams->Results.SignalType = pParams->State;
	#endif
	/***************************MINIDRIVER************************
	*****************************************************************/
	#ifdef STTUNER_MINIDRIVER
	U8 Data, nsbuffer[2];
	pParams->Frequency = pParams->BaseFreq;
	pParams->Direction = 1;
	
	/*	Get fields value	*/
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_PR, 0, 0, &Data, 1, FALSE);
	
	pr = Data & 0x3F;	 
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_VAVSRCH, F399_SN, F399_SN_L, &Data, 1, FALSE);
	sn = Data;	
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_VAVSRCH, F399_TO, F399_TO_L, &Data, 1, FALSE);
	to = Data;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_VAVSRCH, F399_H, F399_H_L, &Data, 1, FALSE);
	hy = Data;

	pParams->Ttiming = (S16)FE_399_TimingTimeConstant(pParams->SymbolRate);
	pParams->Tderot = (S16)FE_399_DerotTimeConstant(pParams->SymbolRate);
	pParams->Tdata = (S16)FE_399_DataTimeConstant(pr,sn,to,hy,pParams->SymbolRate);
	
	FE_399_FirstSubRange(pParams);
	
	do
	{ 
		/*	Initialisations	*/
		if(pParams->SymbolRate > 4000000)
		tuner_freq = FE_399_SetTunerFreq(pParams->Quartz,pParams->Frequency*1000,pParams->SymbolRate)/1000;
		else
		tuner_freq = FE_399_SetTunerFreq(pParams->Quartz,(pParams->Frequency*1000)+2000000,pParams->SymbolRate)/1000;
		tuner_freq_correction = (pParams->Frequency - tuner_freq);			
		pParams->MasterClock = FE_399_SelectMclk(tuner_freq,pParams->SymbolRate,pParams->Quartz); /*Select the best Master clock */ 
		pParams->Mclk = (U32)(pParams->MasterClock/65536L);						
		FE_399_SetSymbolRate(pParams->MasterClock,pParams->SymbolRate);
		/* Carrier loop optimization versus symbol rate */
		if(pParams->SymbolRate <= 2000000)
		{
			Data = 0x17;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_BCLC, F399_BETA, F399_BETA_L, &Data, 1, FALSE);
		}
		else if(pParams->SymbolRate <= 5000000)
		{
			Data = 0x1C;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_BCLC, F399_BETA, F399_BETA_L, &Data, 1, FALSE);
		}
		else if(pParams->SymbolRate <= 15000000) 
		{
			Data = 0x22;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_BCLC, F399_BETA, F399_BETA_L, &Data, 1, FALSE);
		}
		else if(pParams->SymbolRate <= 30000000)
		{
			Data = 0x27;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_BCLC, F399_BETA, F399_BETA_L, &Data, 1, FALSE);
		}
		else
		{
			Data = 0x29;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_BCLC, F399_BETA, F399_BETA_L, &Data, 1, FALSE);
		}	
		pParams->DerotStep = (S16)(pParams->DerotPercent*(pParams->SymbolRate/1000L)/pParams->Mclk);	/*	step of DerotStep/1000 * Fsymbol	*/     
	
		/*	Temporisations	*/
		WAIT_N_MS_399(pParams->Ttiming);		/*	Wait for agc1,agc2 and timing loop	*/
		pParams->FreqOffset = (pParams->Frequency-tuner_freq) * 1000;
		pParams->DerotFreq = pParams->FreqOffset/pParams->Mclk;		
		nsbuffer[0] = MSB(pParams->DerotFreq);
    	        nsbuffer[1] = LSB(pParams->DerotFreq);
    	        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_CFRM, 0, 0, nsbuffer,2, FALSE);
		Data = 0;
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_RTF, F399_TIMING_LOOP_FREQ, F399_TIMING_LOOP_FREQ_L, &Data, 1, FALSE);
		Data = 1;
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_CFD, F399_CFD_ON, F399_CFD_ON_L, &Data, 1, FALSE);
		pParams->Frequency = tuner_freq;
		pParams->State = NOAGC1_399E;
		
		if(FE_399_LockAgc0and1(pParams) == AGC1OK_399E)
		{
			REPORTMSG(SEARCHAGC1,0,0)   
			
			if(FE_399_SearchTiming(pParams) == TIMINGOK_399E)		/*	Search for timing	*/
			{
				if(FE_399_SearchCarrier(pParams) == CARRIEROK_399E)	/*	Search for carrier	*/ 	
				{
					if((MAC0399_ABS(pParams->DerotFreq* pParams->Mclk))>1000000 && (pParams->SymbolRate > 4000000)&& (MAC0399_ABS(pParams->DerotFreq* pParams->Mclk)) <= pParams->SearchRange/2)
					{
						TransponderFrequency = (int)(pParams->Frequency + (pParams->DerotFreq * pParams->Mclk)/1000);  /* cast to eliminate compiler warning --SFS */
						tuner_freq = FE_399_SetTunerFreq(pParams->Quartz,TransponderFrequency*1000,pParams->SymbolRate)/1000;	/*	Move the tuner to exact frequency	*/ 
						pParams->Frequency = tuner_freq;
						pParams->DerotFreq = ((TransponderFrequency - tuner_freq)*1000)/pParams->Mclk;
						nsbuffer[0] = MSB(pParams->DerotFreq);
    	        				nsbuffer[1] = LSB(pParams->DerotFreq);
    	        				Data = 0;
						STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_CFD, F399_CFD_ON, F399_CFD_ON_L, &Data, 1, FALSE);
    	        				STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_CFRM, 0, 0, nsbuffer,2, FALSE);
						Data = 1;
						STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_CFD, F399_CFD_ON, F399_CFD_ON_L, &Data, 1, FALSE);
					}					
					if(FE_399_SearchData(pParams) == DATAOK_399E)	/*	Check for data	*/ 
					{
						if(FE_399_CheckRange(pParams, tuner_freq_correction) == RANGEOK_399E)
						{
							pParams->Results.Frequency = pParams->Frequency + (pParams->DerotFreq*pParams->Mclk)/1000; 
							STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_VSTATUS, F399_PR, F399_PR_L, &Data, 1, FALSE);
							pParams->Results.PunctureRate = 1 << Data; 
					
						}
					}
				}
			}
		}
		
		if(pParams->State != RANGEOK_399E) 
			FE_399_NextSubRange(pParams);

	}	  
	while(pParams->SubRange && pParams->State!=RANGEOK_399E && !pParams->Error.Type);	
	pParams->Results.SignalType = pParams->State;
	#endif
	return	pParams->State;
}



#ifndef STTUNER_MINIDRIVER
/*****************************************************
--FUNCTION	::	SetPower
--ACTION	::	set the lnb power on/off
--PARAMS IN	::	Power	->	Power
--PARAMS OUT::	NONE
--RETURN	::	FE_399_Error_t (Error if any)
--***************************************************/
FE_399_Error_t FE_399_SetPower(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, LNB_Status_t Power,FE_399_InternalParams_t *pParams)
{
	FE_399_Error_t error = FE_399_NO_ERROR;
	

	

	if(pParams != NULL)
	{
		switch(Power)
		{
			case LNB_STATUS_ON:
			    STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_OP1_1,1);
			break;

			case LNB_STATUS_OFF:
				STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_OP1_1,0);
			break;
			
			default:
				error = FE_399_BAD_PARAMETER;
			break;
		}
	}
	else
		error = FE_399_INVALID_HANDLE;

	return error;
}
#endif

/*****************************************************
--FUNCTION	::	GetPolarization
--ACTION	::	Get the polarization
--PARAMS IN	::	Polarization	->	Polarization
--PARAMS OUT::	NONE
--RETURN	::	NONE
--***************************************************/
FE_399_Polarization_t FE_399_GetPol(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_399_InternalParams_t *pParams)
{
	FE_399_Polarization_t polarization=0;
	#ifndef STTUNER_MINIDRIVER
	
	
	if(pParams != NULL)
	{
		switch(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_LOCK_CONF))
		{
			case 0:
				polarization = FE_POL_VERTICAL;
			break;	
	
			case 1:
				polarization = FE_POL_HORIZONTAL;
			break;
		}
	}
	#endif	
	
	#ifdef STTUNER_MINIDRIVER
	U8 Data;
	if(pParams != NULL)
	{    
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_IOCFG1, F399_LOCK_CONF, F399_LOCK_CONF_L, &Data, 1, FALSE);
		switch(Data)
		{
			case 0:
				polarization = FE_POL_VERTICAL;
			break;	
	
			case 1:
				polarization = FE_POL_HORIZONTAL;
			break;
		}
	}
	#endif	
	return polarization;
}


/*****************************************************
--FUNCTION	::	FE_399_SetBand
--ACTION	::	Select the local oscillator corresponding to the current frequency 
--PARAMS IN	::	
--PARAMS OUT::	
--RETURN	::	NONE
--***************************************************/
FE_399_Error_t FE_399_SetBand(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_399_Bands_t Band,FE_399_InternalParams_t *pParams)
{
	FE_399_Error_t error = FE_399_NO_ERROR;
	#ifdef STTUNER_MINIDRIVER
	U8 Data;
	if(pParams != NULL)
	{
		switch(Band)
		{
			case FE_BAND_LOW:
			
			Data = 0;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_IOCFG2, F399_OP0_1, F399_OP0_1_L, &Data, 1, FALSE);
			break;
			
			case FE_BAND_HIGH:
			
			Data = 1;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_IOCFG2, F399_OP0_1, F399_OP0_1_L, &Data, 1, FALSE);
			break;
			
			default:
				error = FE_399_BAD_PARAMETER;  
			break;
		}
	}
	else
		error = FE_399_INVALID_HANDLE;
	#endif
	
	#ifndef STTUNER_MINIDRIVER
	
	
	if(pParams != NULL)
	{
		switch(Band)
		{
			case FE_BAND_LOW:
				STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_OP0_1,0x00);		
			break;
			
			case FE_BAND_HIGH:
				STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_OP0_1,0x01);
			break;
			
			default:
				error = FE_399_BAD_PARAMETER;  
			break;
		}
	}
	else
		error = FE_399_INVALID_HANDLE;
	#endif
	
	return error;
}

/*****************************************************
--FUNCTION	::	GetBand
--ACTION	::	Get the selected local oscillator 
--PARAMS IN	::	Handle	==>	Front End Handle
--PARAMS OUT::	NONE
--RETURN	::	selected band
--***************************************************/
FE_399_Bands_t FE_399_GetBand(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_399_InternalParams_t *pParams)
{
	FE_399_Bands_t band =0;
	#ifndef STTUNER_MINIDRIVER
	
	
	if(pParams != NULL)
	{
		switch(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_OP0_1))
		{
			case 0:
				band = FE_BAND_LOW;
			break;	
	
			case 1:
				band = FE_BAND_HIGH;  
			break;
		}
	}
	#endif	
	
	#ifdef STTUNER_MINIDRIVER
	U8 Data;
	if(pParams != NULL)
	{
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_IOCFG2, F399_OP0_1, F399_OP0_1_L, &Data, 1, FALSE);
		switch(Data)
		{
			case 0:
				band = FE_BAND_LOW;
			break;	
	
			case 1:
				band = FE_BAND_HIGH;  
			break;
		}
	}
	#endif	
	return band;
}

#ifndef STTUNER_MINIDRIVER
S32 FE_399_GetRFLevel(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	S32 agcGain = 0;
	FE_399_LOOKUP_t *lookup	;
		S32 Imin,Imax,i,rfLevel = 0;
	lookup=&FE_399_RF_LookUp;
	if((lookup != NULL) && lookup->size)
	{
		/*FE_399_LockAgc0and1Bis(hChip); */
		agcGain = FE_399_CalcAGC0Gain(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_AGC0_INT))*10;
		agcGain += FE_399_CalcAGC1Gain(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_AGC1_INT_PRIM))*5; 
		
		Imin = 0;
		Imax = lookup->size-1;
		
		if(InRange(lookup->table[Imin].regval,agcGain,lookup->table[Imax].regval))
		{
			while((Imax-Imin)>1)
			{
				i=(Imax+Imin)/2; 
				if(InRange(lookup->table[Imin].regval,agcGain,lookup->table[i].regval))
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
	
	return rfLevel;
}
#endif
#ifdef STTUNER_MINIDRIVER
S32 FE_399_GetRFLevel(FE_399_LOOKUP_t *lookup)
{
	U8 Data;
	U32 agcGain = 0;	
	S32 Imin,
	    Imax,
	    i,
	    rfLevel = 0;	
	if((lookup != NULL) && lookup->size)
	{
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_AGC0I, F399_AGC0_INT, F399_AGC0_INT_L, &Data, 1, FALSE);
		agcGain = FE_399_CalcAGC0Gain(Data)*10;
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_AGC1P, F399_AGC1_INT_PRIM, F399_AGC1_INT_PRIM_L, &Data, 1, FALSE);
		agcGain += FE_399_CalcAGC1Gain(Data)*5; 
		
		Imin = 0;
		Imax = lookup->size-1;		
		if(InRange(lookup->table[Imin].regval,agcGain,lookup->table[Imax].regval))
		{
			while((Imax-Imin)>1)
			{
				i=(Imax+Imin)/2; 
				if(InRange(lookup->table[Imin].regval,agcGain,lookup->table[i].regval))
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
	
	return rfLevel;
}
#endif
/*****************************************************
--FUNCTION	::	FE_399_GetCarrierToNoiseRatio
--ACTION	::	Return the carrier to noise of the current carrier
--PARAMS IN	::	NONE	
--PARAMS OUT::	NONE
--RETURN	::	C/N of the carrier, 0 if no carrier 
--***************************************************/
#ifndef STTUNER_MINIDRIVER
S32 FE_399_GetCarrierToNoiseRatio(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	S32 c_n = 0,
		regval,
		Imin,
		Imax,
		i;
	U8 temp[1];
	FE_399_LOOKUP_t *lookup;
	lookup=&FE_399_CN_LookUp;
	
	if(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_CF))
	{
		if((lookup != NULL) && lookup->size)
		{
			STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R399_NIRM,2,temp);
			regval = MAKEWORD(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_NOISE_IND_MSB),STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_NOISE_IND_LSB));
		
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
			else
				c_n = 100;
		}
	}
	
	return c_n;
}
#endif
#ifdef STTUNER_MINIDRIVER
S32 FE_399_GetCarrierToNoiseRatio(FE_399_LOOKUP_t *lookup)
{
	U8 nsbuffer[2], Data;
	S32 c_n = 0,regval,Imin,Imax,i;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_VSTATUS, F399_CF, F399_CF_L, &Data, 1, FALSE);
	if(Data)
	{
		if((lookup != NULL) && lookup->size)
		{
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_NIRM, 0, 0, nsbuffer, 2, FALSE);
			
			regval = MAKEWORD(nsbuffer[0],nsbuffer[1]);
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
			else
				c_n = 100;
		}
	}
	
	return c_n;
}
#endif
#ifndef STTUNER_MINIDRIVER
U32 FE_399_GetError(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	U32 ber = 0,i;
	
	
	STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R399_ACLC);
	STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R399_ERRCTRL); 
	STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R399_VSTATUS);
	
	/* Average 5 ber values */ 
	for(i=0;i<5;i++)
	{
		ber += FE_399_GetErrorCount(DeviceMap,IOHandle,COUNTER1);
	}
	
	ber/=5;
	
	if(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_CF))	/*	Check for carrier	*/
	{
		if(!STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_ERRMODE))   
		{
			/*	Error Rate	*/
			ber *= 9766;
			ber /= (U32)FE_399E_PowOf2(2 + 2*STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_NOE));	/*  theses two lines => ber = ber * 10^7	*/
			
			switch(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_ERR_SOURCE))
			{
				case 0 :				/*	QPSK bit errors	*/
					ber /= 8;
					switch(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_PR))
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
					if(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_FECMODE) != 0x04)
						ber *= 204;	/* DVB */
					else
						ber *= 147; /* DirecTV */
				break;	
			}
		}
	}
	
	return ber;
}
#endif
#ifdef STTUNER_MINIDRIVER
U32 FE_399_GetError( )
{
	U8 Data;
	U32 ber = 0,i;

	for(i=0;i<5;i++)
	{
		ber += FE_399_GetErrorCount(COUNTER1);
	}	
	ber/=5;	
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_VSTATUS, F399_CF, F399_CF_L, &Data, 1, FALSE);
	if(Data)	/*	Check for carrier	*/
	{
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_ERRCTRL, F399_ERRMODE, F399_ERRMODE_L, &Data, 1, FALSE);
		if(!Data)   
		{
			ber *= 9766;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_ERRCTRL, F399_NOE, F399_NOE_L, &Data, 1, FALSE);
			ber /= (U32)FE_399E_PowOf2(2 + 2*Data);
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_ERRCTRL, F399_ERR_SOURCE, F399_ERR_SOURCE_L, &Data, 1, FALSE);
			switch(Data)
			{
				case 0 :				/*	QPSK bit errors	*/
					ber /= 8;
					STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_VSTATUS, F399_PR, F399_PR_L, &Data, 1, FALSE);
					switch(Data)
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
					STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_FECM, F399_FECMODE, F399_FECMODE_L, &Data, 1, FALSE);
					if(Data != 0x04)
						ber *= 204;	/* DVB */
					else
						ber *= 147; /* DirecTV */
				break;	
			}
		}
	}
	
	return ber;
}
#endif
#ifdef STTUNER_MINIDRIVER
void FE_399_Init(void)
{
	pParams = memory_allocate_clear(DEMODInstance->MemoryPartition, 1, sizeof( FE_399_InternalParams_t ));	
	pParams->Quartz = 27000000;
}
#endif


/*****************************************************
--FUNCTION	::	FE_399_Search
--ACTION	::	Search for a valid transponder
--PARAMS IN	::	Handle	==>	Front End Handle
				pSearch ==> Search parameters
				pResult ==> Result of the search
--PARAMS OUT::	NONE
--RETURN	::	Error (if any)
--***************************************************/
#ifndef STTUNER_MINIDRIVER
FE_399_Error_t	FE_399_Search(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_399_SearchParams_t *pSearch, FE_399_SearchResult_t *pResult)
{
	FE_399_Error_t error = FE_399_NO_ERROR;
	FE_399_InternalParams_t *pParams;
	
	pParams = STOS_MemoryAllocateClear(DeviceMap->MemoryPartition, 1, sizeof( FE_399_InternalParams_t ));
		
	if(pParams != NULL)
	{
		pParams->Quartz = 27000000; /* 27 MHz quartz */
			
				
			FE_399_InitialCalculations(DeviceMap,IOHandle,pParams);
			
			FE_399_Synt(DeviceMap,IOHandle);
		
            	
		
			if(	(pSearch != NULL) && (pResult != NULL) &&
				(INRANGE(1000000,pSearch->SymbolRate,45000000)) &&
				(INRANGE(864000,pSearch->Frequency,2160000)) &&
				(INRANGE(500000,pSearch->SearchRange,50000000)) &&
				(INRANGE(FE_MOD_BPSK,pSearch->Modulation,FE_MOD_8PSK))
				)
			{
				/* Fill pParams structure with search parameters */
				pParams->BaseFreq = pSearch->Frequency;	
				pParams->SymbolRate = pSearch->SymbolRate;
				pParams->SearchRange = pSearch->SearchRange;
				pParams->DerotPercent = 100;
				pParams->TunerBW = FE_399_SelectLPF(DeviceMap,IOHandle,pParams,pParams->SymbolRate);
				pParams->DemodIQMode = pSearch->DemodIQMode;
				
				/* Run the search algorithm */
				if((FE_399_Algo(DeviceMap,IOHandle,pParams) == RANGEOK_399E) )
				{
					pResult->Locked = TRUE;
					
					/* update results */
					pResult->Frequency = pParams->Results.Frequency;			
					pResult->SymbolRate = pParams->Results.SymbolRate;										
					pResult->Rate = pParams->Results.PunctureRate;
				}
				else
				{
					pResult->Locked = FALSE;
					error = FE_399_SEARCH_FAILED;
					
				}
				
			}
			else
				error = FE_399_BAD_PARAMETER;
		}
	
	
	pSearch->State = pParams->State;
	return error;
}
#endif

#ifdef STTUNER_MINIDRIVER
FE_399_Error_t	FE_399_Search(FE_399_SearchParams_t	*pSearch,FE_399_SearchResult_t	*pResult)
{
	FE_399_Error_t error = FE_399_NO_ERROR;
	
	if(	(pSearch != NULL) && (pResult != NULL) &&
				(INRANGE(1000000,pSearch->SymbolRate,45000000)) &&
				(INRANGE(864000,pSearch->Frequency,2160000)) &&
				(INRANGE(500000,pSearch->SearchRange,50000000)) &&
				(INRANGE(FE_MOD_BPSK,pSearch->Modulation,FE_MOD_8PSK))
				)
			{
				/* Fill pParams structure with search parameters */
				pParams->BaseFreq = pSearch->Frequency;	
				pParams->SymbolRate = pSearch->SymbolRate;
				pParams->SearchRange = pSearch->SearchRange;
				pParams->DerotPercent = 100;
				pParams->TunerBW = FE_399_SelectLPF(pParams,pParams->SymbolRate);
				pParams->DemodIQMode = pSearch->DemodIQMode;
				
				/* Run the search algorithm */
				if((FE_399_Algo(DeviceMap,IOHandle,pParams) == RANGEOK_399E))
				{
					pResult->Locked = TRUE;
					
					/* update results */
					pResult->Frequency = pParams->Results.Frequency;			
					pResult->SymbolRate = pParams->Results.SymbolRate;										
					pResult->Rate = pParams->Results.PunctureRate;
				}
				else
				{
					pResult->Locked = FALSE;
					
					error = FE_399_SEARCH_FAILED;
				}
				
			}
			else
				error = FE_399_BAD_PARAMETER;
		
	pSearch->State = pParams->State;
	memory_deallocate(DeviceMap->MemoryPartition, pParams);
	return error;
}
#endif
/*****************************************************
--FUNCTION	::	FE_399_GetSignalInfo
--ACTION	::	Return informations on the locked transponder
--PARAMS IN	::	Handle	==>	Front End Handle
--PARAMS OUT::	pInfo	==> Informations (BER,C/N,power ...)
--RETURN	::	Error (if any)
--***************************************************/
#ifndef STTUNER_MINIDRIVER
FE_399_Error_t	FE_399_GetSignalInfo(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_399_SignalInfo_t	*pInfo)
{
	FE_399_Error_t error = FE_399_NO_ERROR;	
	S16 derotFreq;
	U8 temp[2];
	U32 tempvar,MasterClock,Mclk;
	MasterClock = FE_399_GetMclkFreq(DeviceMap, IOHandle,27000000);
	Mclk = (U32)(MasterClock/65536L);
	
		
		pInfo->Locked = STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_LK);
		
		if(pInfo->Locked)
		{
			tempvar = 0;
			STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R399_CFRM,2,temp);							/*	read derotator value */ 
			derotFreq = MAKEWORD(temp[0],temp[1]); 
		
			/* transponder_frequency = synthe_frequency*PLL_factor + derotator_frequency */
			pInfo->Frequency = ((FE_399_GetSyntFreq(DeviceMap,IOHandle,27000000))*(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_PLL_FACTOR)+4)+(derotFreq*(S16)Mclk))/1000;
			
			pInfo->SymbolRate = FE_399_GetSymbolRate(DeviceMap,IOHandle,MasterClock);	/* Get symbol rate */
			pInfo->SymbolRate += (pInfo->SymbolRate*STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_TIMING_LOOP_FREQ))>>19;	/* Get timing loop offset */
			pInfo->Rate = 1 << STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_PR);
			pInfo->Power = FE_399_GetRFLevel(DeviceMap,IOHandle)+(pInfo->Frequency-945000)/10000; /* correction applied according to RF frequency */
			pInfo->C_N = FE_399_GetCarrierToNoiseRatio(DeviceMap,IOHandle);
			pInfo->BER = FE_399_GetError(DeviceMap,IOHandle);
			
			/*retrieves the modulation from the R8PSK register-> value 0 indicates QPSK, Value 1 indicates 8PSK*/
			tempvar = STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_MODE_8PSK);
			if(tempvar==0) 
			pInfo->Modulation = STTUNER_MOD_QPSK;
			else
			pInfo->Modulation = STTUNER_MOD_8PSK;
		}
	
	else
		error = FE_399_INVALID_HANDLE; 
		
	return error;
}
#endif
#ifdef STTUNER_MINIDRIVER
FE_399_Error_t	FE_399_GetSignalInfo(FE_399_SignalInfo_t	*pInfo)
{
	FE_399_Error_t error = FE_399_NO_ERROR;
	S16 derotFreq;
	U8 Data, nsbuffer[2];
	
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_VSTATUS, F399_LK, F399_LK_L, &Data, 1, FALSE);
	pInfo->Locked = Data;
		
		if(pInfo->Locked)
		{
			U32 tempvar = 0;
			/*ChipGetRegisters(R399_CFRM,2);*/
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_CFRM, 0, 0, nsbuffer, 2, FALSE);
			derotFreq = (S16) MAKEWORD(nsbuffer[0],nsbuffer[1]); 
		
			/* transponder_frequency = synthe_frequency*PLL_factor + derotator_frequency */
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_SYNTCTRL2, F399_PLL_FACTOR, F399_PLL_FACTOR_L, &Data, 1, FALSE);
			pInfo->Frequency = ((FE_399_GetSyntFreq(pParams->Quartz))*(Data+4)+(derotFreq*pParams->Mclk))/1000;
			
			pInfo->SymbolRate = FE_399_GetSymbolRate(pParams->MasterClock);	/* Get symbol rate */
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_RTF, F399_TIMING_LOOP_FREQ, F399_TIMING_LOOP_FREQ_L, &Data, 1, FALSE);
			pInfo->SymbolRate += (pInfo->SymbolRate*Data)>>19;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_VSTATUS, F399_PR, F399_PR_L, &Data, 1, FALSE);
			pInfo->Rate = 1 << Data;
			/*pInfo->Power = FE_399_GetRFLevel(&FE_399_RF_LookUp)+(pInfo->Frequency-945000)/10000;*/ /* correction applied according to RF frequency */
			pInfo->C_N = FE_399_GetCarrierToNoiseRatio(&FE_399_CN_LookUp);
			pInfo->BER = FE_399_GetError( );
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_R8PSK, F399_MODE_8PSK, F399_MODE_8PSK_L, &Data, 1, FALSE);
			tempvar = Data;
			if(tempvar==0) 
			pInfo->Modulation = STTUNER_MOD_QPSK;
			else
			pInfo->Modulation = STTUNER_MOD_8PSK;
		}
	
	else
		error = FE_399_INVALID_HANDLE; 
		
	return error;
}
#endif
#ifndef STTUNER_MINIDRIVER									
/*****************************************************
--FUNCTION	::	FE_399_CarrierTracking
--ACTION	::	Optimize STV0399 parameters according to signal changes 
--PARAMS IN	::	Handle	==>	Front End Handle
--PARAMS OUT::	NONE
--RETURN	::	Error (if any)
--***************************************************/
FE_399_Error_t	FE_399_CarrierTracking(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_399_InternalParams_t	*pParams)
{
	FE_399_Error_t error = FE_399_NO_ERROR;
	
	U32 agc0int;
	S32 agc1gain;
	U8 temp[1];
	
	
	
	if(pParams != NULL)
	{
		do
		{

			agc0int = STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_AGC0_INT);
			agc1gain = FE_399_CalcAGC1Gain(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_AGC1_INT_PRIM))/2; 

			if((agc0int != 1) && (agc1gain < 11))
			{
				agc1gain = FE_399_CalcAGC1Gain(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_AGC1_INT_PRIM))/2; 
		
				agc0int = MAX(1,agc0int-1);
				STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_AGC0_INT,agc0int);		/* Write AGC0 integrator	*/
				STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R399_AGC1S,2,temp);
				STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R399_AGC1S,temp,2);			/* Write into AGC1 registers to force update	*/ 
			}
			else if ( (agc0int != 9)&&(agc1gain > 19))
			{
				agc0int = MIN(9,agc0int+1);    
				STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_AGC0_INT,agc0int); /* Write AGC0 integrator	*/
				STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R399_AGC1S,2,temp);
				STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R399_AGC1S,temp,2);	/* Write into AGC1 registers to force update	*/ 
			}
		
			WAIT_N_MS_399(2);
		
			agc1gain = FE_399_CalcAGC1Gain(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_AGC1_INT_PRIM))/2;
		
		}
		while(((agc1gain < 11) || (agc1gain > 19)) && (agc0int != 1) && (agc0int != 9));
	}
	else
	{
		error = FE_399_INVALID_HANDLE; 
	}
		
	return error;
}
#endif									

#ifdef STTUNER_MINIDRIVER
void	FE_399_Term(void)
{
	memory_deallocate(DEMODInstance->MemoryPartition, pParams);
        
}
#endif
#ifndef STTUNER_MINIDRIVER
void FE_399_LockAgc0and1Bis(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	U32 agc0int;
	S32 agc1gain = 0;
	U8 temp[1];
			
	agc0int = 5;
	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_AGC0_INT,agc0int);
	STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R399_AGC1S,2,temp);
	STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R399_AGC1S,temp,2);			/* Write into AGC1 registers to force update	*/ 
	WAIT_N_MS_399(2); 
	
	agc1gain = FE_399_CalcAGC1Gain(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_AGC1_INT_PRIM))/2; 
	
	do
	{
		if(agc1gain < 11)
		{
			agc0int = MAX(1,agc0int-1);
			STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_AGC0_INT,agc0int);		/* Write AGC0 integrator	*/
			STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R399_AGC1S,2,temp);
			STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R399_AGC1S,temp,2);			/* Write into AGC1 registers to force update	*/ 
		}
		else if(agc1gain > 19)
		{
			agc0int = MIN(9,agc0int+1);  
			STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_AGC0_INT,agc0int);		/* Write AGC0 integrator	*/
			STTUNER_IOREG_GetContigousRegisters(DeviceMap,IOHandle,R399_AGC1S,2,temp);
			STTUNER_IOREG_SetContigousRegisters(DeviceMap,IOHandle,R399_AGC1S,temp,2);			/* Write into AGC1 registers to force update	*/ 
		}
		
		WAIT_N_MS_399(2);
		
		agc1gain = FE_399_CalcAGC1Gain(STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_AGC1_INT_PRIM))/2;
		
	}while(((agc1gain < 11) || (agc1gain > 19)) && (agc0int != 1) && (agc0int != 9));
}
#endif
#ifdef STTUNER_MINIDRIVER
void FE_399_LockAgc0and1Bis( )
{
	U8 Data;
	U32 agc0int;
	S32 agc1gain = 0;			
	agc0int = 5;
	
	Data = agc0int;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_AGC0I, F399_AGC0_INT, F399_AGC0_INT_L, &Data, 1, FALSE);
	Data = 2;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_AGC1S, 0, 0, &Data, 1, FALSE);
	WAIT_N_MS_399(2); 	
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_AGC1P, F399_AGC1_INT_PRIM, F399_AGC1_INT_PRIM_L, &Data, 1, FALSE);
	agc1gain = FE_399_CalcAGC1Gain(Data)/2; 	
	do
	{
		if(agc1gain < 11)
		{
			agc0int = MAX(1,agc0int-1);
			Data = agc0int;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_AGC0I, F399_AGC0_INT, F399_AGC0_INT_L, &Data, 1, FALSE);
			Data = 2;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_AGC1S, 0, 0, &Data, 1, FALSE);
		}
		else if(agc1gain > 19)
		{
			agc0int = MIN(9,agc0int+1);  
			Data = agc0int;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_AGC0I, F399_AGC0_INT, F399_AGC0_INT_L, &Data, 1, FALSE);
			Data = 2;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_AGC1S, 0, 0, &Data, 1, FALSE);
		}
		WAIT_N_MS_399(2);		
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_AGC1P, F399_AGC1_INT_PRIM, F399_AGC1_INT_PRIM_L, &Data, 1, FALSE);
		agc1gain = FE_399_CalcAGC1Gain(Data)/2;
		
	}while(((agc1gain < 11) || (agc1gain > 19)) && (agc0int != 1) && (agc0int != 9));
}
#endif
/*****************************************************
--FUNCTION	::	FE_399_SpectrumAnalysis
--ACTION	::	Spectrum analysis using AGC2
--PARAMS IN	::	pInit	==>	pointer to FE_399_InitParams_t structure
--PARAMS OUT::	NONE
--RETURN	::	Handle to STV0399
--***************************************************/
#ifndef STTUNER_MINIDRIVER
U32 FE_399_SpectrumAnalysis(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 Quartz,U32 StartFreq,U32 StopFreq,U32 StepSize,U32 *ToneList)
	{
		U32 freq,
			realTunerFreq,
			masterClock;
		U32 *spectrum,freqstep = 2000000,tempfreq,lastfreq=0,freqlowerthreshold, freqhigherthreshold;
			int *spectrum_agc, agc_threshold;
		U8  i=0, j = 1, points = 0;

		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_AGC1R_REF,33);
		points = (U16)((StopFreq - StartFreq)/StepSize + 1);
		spectrum = STOS_MemoryAllocateClear(DeviceMap->MemoryPartition, points, sizeof(U16));
		spectrum_agc = STOS_MemoryAllocateClear(DeviceMap->MemoryPartition, points, sizeof(int));
		
		for(freq=StartFreq;freq<StopFreq;freq+=StepSize)
		{      U8 direction = 1,j = 1;
			realTunerFreq = FE_399_SetTunerFreq(DeviceMap,IOHandle,Quartz,freq,StepSize);
			masterClock = FE_399_SelectMclk(DeviceMap,IOHandle,realTunerFreq,StepSize,Quartz);	/* Select the best */ 
			FE_399_LockAgc0and1Bis(DeviceMap,IOHandle); 
			
			STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_IAGC1R,0);
			WAIT_N_MS_399(2);
			STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_IAGC1R,1);
			WAIT_N_MS_399(2);
			agc_threshold = STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_AGC1_INT_PRIM);
			
				if(agc_threshold > (-70))/* - 70 */
		                {
		                if(((agc_threshold >= -70) && (MAC0399_ABS((S32)(freq - lastfreq)))>50000000))
				{
					agc_threshold = 0;
					do
					{
						tempfreq = freq+(freqstep)*direction * j;
						realTunerFreq = FE_399_SetTunerFreq(DeviceMap,IOHandle,Quartz,tempfreq,StepSize);
						STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_IAGC1R,0);
						WAIT_N_MS_399(2);
						STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_IAGC1R,1);
						WAIT_N_MS_399(2);
						agc_threshold = STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_AGC1_INT_PRIM);
						++j;
				        }while( agc_threshold> -70);
				        freqhigherthreshold = tempfreq;
				        direction *= -1;
				        agc_threshold = 0; j = 1;
				        do
					{
						tempfreq = freq+(freqstep)*direction * j;
						realTunerFreq = FE_399_SetTunerFreq(DeviceMap,IOHandle,Quartz,tempfreq,StepSize);
						STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_IAGC1R,0);
						WAIT_N_MS_399(2);
						STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_IAGC1R,1);
						WAIT_N_MS_399(2);
						agc_threshold = STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_AGC1_INT_PRIM);
						++j;
				        }while( agc_threshold> -70);
				        freqlowerthreshold = tempfreq;
					spectrum[i] = (freqlowerthreshold + (freqhigherthreshold - freqlowerthreshold)/2);
				        if(spectrum[i] >= StartFreq && spectrum[i] <= StopFreq)
				        {
				        	lastfreq = spectrum[i];
				        	realTunerFreq = FE_399_SetTunerFreq(DeviceMap,IOHandle,Quartz,spectrum[i],StepSize);
				        	
				        	STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_IAGC1R,0);
						WAIT_N_MS_399(2);
						STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_IAGC1R,1);
						WAIT_N_MS_399(2);
						spectrum_agc[i] = STTUNER_IOREG_GetField(DeviceMap,IOHandle,F399_AGC1_INT_PRIM);
						++i;				        	
				        }
		
				}	
		}
		}
		
		points = 0;
		
		for(j = 0; j<i; ++j)
		{
			
			{
				ToneList[points] = spectrum[j];
				points++;
				if(points>=8)/* Max 8 valid tones allowed*/
				break;
			}
		}
		
		STOS_MemoryDeallocate(DeviceMap->MemoryPartition, spectrum);
		STOS_MemoryDeallocate(DeviceMap->MemoryPartition, spectrum_agc);
		
	return points;		
	}

U32 FE_399_ToneDetection(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 StartFreq,U32 StopFreq,U32 *ToneList,U8 MaxNbTone)
{
	U32     step,Quartz = 27000000;
	U8	nbTones=0;
	
	if((StopFreq-StartFreq)>100000000)
	{
		/* wide band acquisition */
		step = 18000000;						/* use 20MHz step */
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_FILTER,0x20); 	/* select 36Mhz low-pass filter */
	}
	else
	{
		/* narrow band acquisition */
		step = 4000000;							/* use 4MHz step */
		STTUNER_IOREG_SetField(DeviceMap,IOHandle,F399_FILTER,0x0);	/* select 6MHz low-pass filter */
	}
	nbTones=FE_399_SpectrumAnalysis(DeviceMap,IOHandle,Quartz,StartFreq,StopFreq,step,ToneList);	/* spectrum acquisition */
	return nbTones;
}
#endif
#ifdef STTUNER_MINIDRIVER
U32 FE_399_SpectrumAnalysis(U32 Quartz,U32 StartFreq,U32 StopFreq,U32 StepSize,U16 *Spectrum)
	{
		U32 freq,
			realTunerFreq,
			masterClock,
			power,
			i=0,
			points;
		
		for(freq=StartFreq;freq<StopFreq;freq+=StepSize)
		{
			realTunerFreq = FE_399_SetTunerFreq(Quartz,freq,StepSize);
			masterClock = FE_399_SelectMclk(realTunerFreq,StepSize,Quartz);	/*Select the best Master clock */ 
			WAIT_N_MS_399(2);
			FE_399_LockAgc0and1Bis();
			power = 840+FE_399_GetRFLevel(&FE_399_RF_LookUp)+(realTunerFreq/1000-945000)/10000; /* correction applied according to RF frequency */ 		
			if(power)
				Spectrum[i++] = (power/10)*(power/10);
			else
				Spectrum[i++] = 0;
		}

		points = i-1;

		return points;
	}
#endif

#ifdef STTUNER_MINIDRIVER
U32 FE_399_ToneDetection(U32 StartFreq,U32 StopFreq,U32 *ToneList,U8 MaxNbTone)
{
	U8 Data;
	U32 step,		/* Frequency step */ 
		points,		/* Number of points in the spectrum */
		risingEdge, /* Tone rising edge index */
		index;
	U16 *spectrum,	/* Spectrum array */
		max;
	U32 Quartz = 27000000;
	U8	nbTones=0;
	
	if((StopFreq-StartFreq)>100000000)
	{
		/* wide band acquisition */
		step = 20000000;
		Data = 0x3c;
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_LPF, F399_FILTER, F399_FILTER_L, &Data, 1, FALSE);						/* use 20MHz step */

	}
	else
	{
		/* narrow band acquisition */
		step = 4000000;							/* use 4MHz step */
		Data = 0x0;
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_LPF, F399_FILTER, F399_FILTER_L, &Data, 1, FALSE);	

	}
	
	
	points = (StopFreq-StartFreq)/step+1;	/* Compute maximum number of points needed to store the spectrum */
	/*spectrum = calloc(points,sizeof(U16));*/	/* Allocate spectrum array */
	
	spectrum = STOS_MemoryAllocateClear(DEMODInstance->MemoryPartition, points, sizeof(U16));
	
	if(spectrum)
	{
		points=FE_399_SpectrumAnalysis(Quartz,StartFreq,StopFreq,step,spectrum);	/* spectrum acquisition */
	
		/* Max Search */ 
		max=0;
		for(index=0;index<points;index++)
		{
			if(spectrum[index]>max)
				max=spectrum[index];
		}
		
		/* Tone detection */
		risingEdge=0;
		for(index=0;index<points;index++)
		{
			if(spectrum[index]>max/3)
			{
				if(!risingEdge)
					risingEdge=index;	/* rising edge detected : store index */
			}
			else
			{
				if(risingEdge)
				{
					/* falling edge detected */
					if(nbTones<MaxNbTone)
						ToneList[nbTones++]=(StartFreq/1000+(step/1000*(risingEdge*10+((index-1)*10-risingEdge*10)/2))/10)*1000; /* store tone center frequency */ 
					
					risingEdge=0;	/* reset rising edge index to allow next tone detection */
				}
			}
		}
		
		
	}
	
	return nbTones;
}
#endif









