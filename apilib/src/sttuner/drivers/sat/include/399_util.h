/* -------------------------------------------------------------------------
File Name: 399_util.h

Description: 399 utilities

Copyright (C) 1999-2001 STMicroelectronics

History:
   date: 10-October-2001
version: 1.0.0
 author: SA
comment: STAPIfied by GP

---------------------------------------------------------------------------- */

#ifndef H_399UTIL
	#define H_399UTIL
	
	#include "stddefs.h"   
	
	#define FE_399_MAXLOOKUPSIZE 50
	
	/* VCO state enum	*/
	typedef enum
	{
		VCO_TOO_HIGH,			/* VCO selected is too high */
		VCO_TOO_LOW,			/* VCO selected is too low	*/
		VCO_LOCKED				/* VCO locked				*/
	} FE_399_VCO_STATE_t;

	/* VCO enum	*/
	typedef enum
	{
		VCO_HIGH = 0,			/* FASTEST VCO */
		VCO_LOW = 1				/* SLOWEST VCO */
	} FE_399_VCO_t;

	/* PLL enum	*/
	typedef enum
	{
		PLL_X4 = 0,				/* PLL by 4 */
		PLL_X5 = 1				/* PLL by 5 */
	} FE_399_PLL_MULTIPLIER_t;
	
	/* Counter enum */
	typedef enum
	{
		COUNTER1 = 0,
		COUNTER2 = 1
	} FE_399_ERRORCOUNTER_t;
	
	/* One point of the lookup table */
	typedef struct
	{
		S32 realval;	/*	real value (10000 x C/N or power )	*/
		S32 regval;		/*	register value (C/N estimator value or AGC1 integrator value )	*/
	} FE_399_LOOKPOINT_t;

	/*	Lookup table definition	*/
	typedef struct
	{
		S32 size;		/*	Size of the lookup table	*/
		FE_399_LOOKPOINT_t table[FE_399_MAXLOOKUPSIZE]; 	/*	Lookup table	*/
	} FE_399_LOOKUP_t;


	/* Utility routine */
	S32 FE_399_CalcAGC1Gain(U8 regvalue);
	
	S32 FE_399_CalcAGC0Gain(U8 regvalue);
	
	U32 FE_399_SetSyntFreq(STCHIP_Handle_t hChip,U32 Quartz, U32 Target);
	U32 FE_399_GetSyntFreq(STCHIP_Handle_t hChip,U32 Quartz);	  
	void FE_399_SetDivRatio(STCHIP_Handle_t hChip, U8 Ratio);  
	U8 FE_399_GetDivRatio(STCHIP_Handle_t hChip);
	U32 FE_399_GetMclkFreq(STCHIP_Handle_t hChip, U32 ExtFreq);   
	U32 FE_399_SetTunerFreq(STCHIP_Handle_t hChip,U32 Quartz,U32 Frequency);   
	
	U32 FE_399_SetSymbolRate(STCHIP_Handle_t hChip,U32 MasterClock,U32 SymbolRate);
	U32 FE_399_GetSymbolRate(STCHIP_Handle_t hChip,U32 MasterClock); 
	U32 FE_399_SpectrumAnalysis(STCHIP_Handle_t hChip,U32 Quartz,U32 StartFreq,U32 StopFreq,U32 StepSize,U32 *Spectrum);
	U32 FE_399_ToneDetection(STCHIP_Handle_t hChip,U32 StartFreq,U32 StopFreq,U32 *ToneList,U8 MaxNbTone);

	U32 FE_399_GetErrorCount(STCHIP_Handle_t hChip,FE_399_ERRORCOUNTER_t Counter);
	S32 FE_399_GetCarrierToNoiseRatio(STCHIP_Handle_t hChip,FE_399_LOOKUP_t *lookup);
	
#endif
