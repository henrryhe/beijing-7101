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
	
	
	typedef enum
	{
	    FE_399_NO_ERROR,
	    FE_399_INVALID_HANDLE,
	    FE_399_ALLOCATION,
	    FE_399_BAD_PARAMETER,
	    FE_399_ALREADY_INITIALIZED,
	    FE_399_I2C_ERROR,
	    FE_399_SEARCH_FAILED,
	    FE_399_TRACKING_FAILED,
	    FE_399_TERM_FAILED
	} FE_399_Error_t;
	
	typedef enum
	{
		FE_BAND_LOW,
		FE_BAND_HIGH
	} FE_399_Bands_t;


	typedef enum
	{
	  	FE_MOD_BPSK,
	  	FE_MOD_QPSK,
	  	FE_MOD_8PSK
	} FE_399_Modulation_t;

	typedef enum
	{
		FE_POL_HORIZONTAL	  = 1,
		FE_POL_VERTICAL		  = 1<<1,
		FE_POL_NOPOLARIZATION = 1<<2
	}FE_399_Polarization_t;
	
	typedef enum
	{
		FE_1_2	= 1,
		FE_2_3	= 1<<1,   
		FE_3_4	= 1<<2,   
		FE_5_6	= 1<<3,
		FE_6_7	= 1<<4,
		FE_7_8	= 1<<5,
		FE_8_9	= 1<<6
	}FE_399_Rate_t;

	typedef struct
	{
		BOOL Locked;						/* Transponder locked					*/
		U32 Frequency;						/* transponder frequency (in KHz)		*/
		U32 SymbolRate;						/* transponder symbol rate  (in Mbds)	*/
		FE_399_Modulation_t Modulation;		/* modulation							*/
		FE_399_Polarization_t Polarization;	/* Polarization							*/
		FE_399_Bands_t Band;				/* Band									*/
		FE_399_Rate_t Rate;					/* puncture rate 						*/       
		S16 Power;							/* Power of the RF signal (dBm)			*/			
		U32	C_N;							/* Carrier to noise ratio				*/
		U32	BER;							/* Bit error rate						*/
	} FE_399_SignalInfo_t;
	
	
	/* One point of the lookup table */
	typedef struct
	{
		S32 realval;	/*	real value */
		S32 regval;		/*	binary value */
	} FE_399_LOOKPOINT_t;

	/*	Lookup table definition	*/
	typedef struct
	{
		S32 size;		/*	Size of the lookup table	*/
		FE_399_LOOKPOINT_t table[FE_399_MAXLOOKUPSIZE]; 	/*	Lookup table	*/
	} FE_399_LOOKUP_t;


	/* Utility routine */
	S32 FE_399_CalcAGC1Gain(U8 regvalue);
	U8 FE_399_CalcAGC1Integrator(S32 gain); 
	S32 FE_399_CalcAGC0Gain(U8 regvalue);
	#ifndef STTUNER_MINIDRIVER
	U32 FE_399_SetSyntFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 Quartz, U32 Target);
	FE_399_Error_t	FE_399_GetSignalInfo(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_399_SignalInfo_t *pInfo);
	U32 FE_399_GetSyntFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 Quartz);
	U32 FE_399_GetError(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
	void FE_399_SetDivRatio(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U8 Ratio);  
	U8 FE_399_GetDivRatio(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
	U32 FE_399_GetMclkFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 ExtFreq);
	U32 FE_399_Get_PLL_mult_coeff(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
	S32 FE_399_GetRFLevel(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
	U32 FE_399_SetTunerFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 Quartz,U32 Frequency,U32 SymbolRate);   
	
	
	U32 FE_399_AuxClkFreq(U32 DigFreq,U32 Prescaler,U32 Divider);
	U32 FE_399_F22Freq(U32 DigFreq,U32 F22);
	
	U32 FE_399_SetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 MasterClock,U32 SymbolRate);
	U32 FE_399_GetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 MasterClock); 

	S32 FE_399_GetDerotFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 MasterClock);
	U32 FE_399_SpectrumAnalysis(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 Quartz,U32 StartFreq,U32 StopFreq,U32 StepSize,U32 *Spectrum);
	U32 FE_399_ToneDetection(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 StartFreq,U32 StopFreq,U32 *ToneList,U8 MaxNbTone);
	
	#ifdef BLINDSCAN
		void FE_399_ExtractChannels(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U16 *Spectrum,U32 points,U32 StartFreq,U32 StepSize/*,void *Channels*/);
	#endif
	
	U32 FE_399_GetErrorCount(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_399_ERRORCOUNTER_t Counter);
	S32 FE_399_GetCarrierToNoiseRatio(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
	#endif
	
	#ifdef STTUNER_MINIDRIVER
	
	
	U32 FE_399_SetSyntFreq(U32 Quartz, U32 Target);
	
	U32 FE_399_GetSyntFreq(U32 Quartz);
	
	void FE_399_SetDivRatio(U8 Ratio);  
	U8 FE_399_GetDivRatio(void);
	U32 FE_399_GetMclkFreq(U32 ExtFreq);
	U32 FE_399_Get_PLL_mult_coeff(void);
	
	U32 FE_399_SetTunerFreq(U32 Quartz,U32 Frequency,U32 SymbolRate);   
	
	
	U32 FE_399_AuxClkFreq(U32 DigFreq,U32 Prescaler,U32 Divider);
	U32 FE_399_F22Freq(U32 DigFreq,U32 F22);
	
	U32 FE_399_SetSymbolRate(U32 MasterClock,U32 SymbolRate);
	U32 FE_399_GetSymbolRate(U32 MasterClock); 

	S32 FE_399_GetDerotFreq(U32 MasterClock);
	U32 FE_399_SpectrumAnalysis(U32 Quartz,U32 StartFreq,U32 StopFreq,U32 StepSize,U16 *Spectrum);
	U32 FE_399_ToneDetection(U32 StartFreq,U32 StopFreq,U32 *ToneList,U8 MaxNbTone);
	
	#ifdef BLINDSCAN
		void FE_399_ExtractChannels(U16 *Spectrum,U32 points,U32 StartFreq,U32 StepSize/*,void *Channels*/);
	#endif
	
	U32 FE_399_GetErrorCount(FE_399_ERRORCOUNTER_t Counter);
	S32 FE_399_GetCarrierToNoiseRatio(FE_399_LOOKUP_t *lookup);
	#endif
#endif
