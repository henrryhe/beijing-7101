/* -------------------------------------------------------------------------
File Name: d0899_util.h

History:
Description: STb0899 driver LLA	V1.0.8 17/01/2005

Date 18/04/05
Comments: LLA2.0.0 integration
author: SD

Date 31/05/05
Comments: LLA3.0.0 integration
author: SD
---------------------------------------------------------------------------- */
#ifndef H_STB0899_UTIL
	#define H_STB0899_UTIL
	
	#include "ioreg.h"

	#define FE_STB0899_MAXLOOKUPSIZE 50    
	
/* Delay calling task for a period of microseconds */

#define STB0899_Delay(micro_sec) STOS_TaskDelayUs(micro_sec)
#define WAIT_N_MS_899(x) STB0899_Delay((x*1000))
	/*	Signal type enum	*/
	typedef enum
	{
		NOAGC1_899=0,
		AGC1OK_899,
		NOTIMING_899,
		ANALOGCARRIER_899,
		TIMINGOK_899,
		NOAGC2_899,
		AGC2OK_899,
		NOCARRIER_899,
		CARRIEROK_899,
		NODATA_899,
		FALSELOCK_899,
		DATAOK_899,
		OUTOFRANGE_899,
		RANGEOK_899
	} FE_STB0899_SIGNALTYPE_t;
	
	/*	Synthesizer enum	*/
	typedef enum 
	{
		SYNTH_DIGITAL_899,			/*	digital synthesizer	*/
		SYNTH_ANALOG_899			/*	analog synthesizer	*/
	} SYNTHESIZER_899;
	
	/* Counter enum */
	typedef enum
	{
		COUNTER1_899 = 0,
		COUNTER2_899 = 1,
		COUNTER3_899 = 2
	} ERRORCOUNTER;
	
		
	/* One point of the lookup table */
	typedef struct
	{
		S32 realval;	/*	real value */
		S32 regval;		/*	binary value */
	} FE_STB0899_LOOKPOINT_t;

	/*	Lookup table definition	*/
	typedef struct
	{
		S32 size;		/*	Size of the lookup table	*/
		FE_STB0899_LOOKPOINT_t table[FE_STB0899_MAXLOOKUPSIZE]; 	/*	Lookup table	*/
	} FE_STB0899_LOOKUP_t;
		
	long FE_899_PowOf2(int number);
	U32 FE_899_XtoPowerY(U32 Number, U32 Power);
	S32 FE_899_RoundToNextHighestInteger(S32 Number,U32 Digits);
	long FE_899_GetPowOf2(int number); 
	long Log2Int(int number);
	long Log10Int(long number);
	int SqrtInt(int Sq);	
	
	S32 FE_STB0899_CalcAGC1Gain(U8 regvalue);
	U8 FE_STB0899_CalcAGC1Integrator(S32 gain); 
	S32 FE_STB0899_CalcAGC0Gain(U8 regvalue);
	
	S32 FE_STB0899_InputFifoSize(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
	S32 FE_STB0899_InputFifoLevel(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
	BOOL FE_STB0899_RegulateInputFifo(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle); 

	U32 FE_STB0899_GetMclkFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 SyntFreq);
	U32 FE_STB0899_SetSyntFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,SYNTHESIZER_899 Synthe, U32 Quartz, U32 Target); 
	U32 FE_STB0899_GetSyntFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,SYNTHESIZER_899 Synthe, U32 Quartz);   
	
	U32 FE_STB0899_AuxClkFreq(U32 DigFreq,U32 Prescaler,U32 Divider);
	U32 FE_STB0899_F22Freq(U32 DigFreq,U32 F22);

	U32 FE_STB0899_SetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 MasterClock,U32 SymbolRate);
	U32 FE_STB0899_GetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 MasterClock); 
	

	S32 FE_STB0899_GetDerotFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 MasterClock);

	U32 FE_STB0899_GetErrorCount(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,ERRORCOUNTER Counter);
	void FE_STB0899_SetStandard(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,STTUNER_FECType_t FECType, STTUNER_FECMode_t FECMode);
	U32 FE_STB0899_GetError(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, STTUNER_FECType_t FECType, STTUNER_FECMode_t FECMode);
	
	
#endif
