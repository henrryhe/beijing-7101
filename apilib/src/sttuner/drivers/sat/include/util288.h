/* -------------------------------------------------------------------------
File Name: d0899_util.h

History:
Description: STX0288 driver LLA	V1.0.8 17/01/2005

Date 18/04/05
Comments: LLA2.0.0 integration
author: SD

Date 31/05/05
Comments: LLA3.0.0 integration
author: SD
---------------------------------------------------------------------------- */
#ifndef H_STX0288_UTIL
	#define H_STX0288_UTIL
	
	#include "ioreg.h"

	#define FE_288_MAXLOOKUPSIZE 50    
	
/* Delay calling task for a period of microseconds */

#define STX0288_Delay(micro_sec) STOS_TaskDelayUs(micro_sec)
#define WAIT_N_MS_288(x) STX0288_Delay((x*1000))
	/*	Signal type enum	*/
	typedef enum
	{
		NOSIGNAL_288=0,
		AGC1OK_288,
		NOTIMING_288,
		ANALOGCARRIER_288,
		TIMINGOK_288,
		NOAGC2_288,
		AGC2OK_288,
		NOCARRIER_288,
		CARRIEROK_288,
		NODATA_288,
		FALSELOCK_288,
		DATAOK_288,
		OUTOFRANGE_288,
		RANGEOK_288
	} FE_288_SIGNALTYPE_t;
	
	/*	Synthesizer enum	*/
		
	/* Counter enum */
	typedef enum
	{
		COUNTER1_288 = 0,
		COUNTER2_288 = 1,
		COUNTER3_288 = 2
	} FE_288_ERRORCOUNTER_t;
	
	typedef enum
	{
		DIR_RIGHT = 1,
		DIR_LEFT = -1
	} FE_288_DIRECTION_t;

	
	#define ABS_288(X) ((X)<0 ? (-X) : (X))
	#define MAX_288(X,Y) ((X)>=(Y) ? (X) : (Y))
	#define MIN_288(X,Y) ((X)<=(Y) ? (X) : (Y)) 
	#define INRANGE_288(X,Y,Z) (((X<=Y) && (Y<=Z))||((Z<=Y) && (Y<=X)) ? 1 : 0)  
	#define LSB_288(X) ((X & 0xFF))
	#define MSB_288(Y) ((Y>>8)& 0xFF)
	#define MAKEWORD_288(X,Y) ((X<<8)+(Y))
		
	
	typedef enum
	{
	    FE_288_NO_ERROR,
	    FE_288_INVALID_HANDLE,
	    FE_288_ALLOCATION,
	    FE_288_BAD_PARAMETER,
	    FE_288_ALREADY_INITIALIZED,
	    FE_288_I2C_ERROR,
	    FE_288_SEARCH_FAILED,
	    FE_288_TRACKING_FAILED,
	    FE_288_TERM_FAILED
	} FE_288_Error_t;
	
	typedef enum
	{
		FE_288_BAND_LOW,
		FE_288_BAND_HIGH
	} FE_288_Bands_t;


	typedef enum
	{
	  	FE_288_MOD_BPSK,
	  	FE_288_MOD_QPSK
	} FE_288_Modulation_t;
	
	typedef enum
	{
		FE_288_1_2	= 1,
		FE_288_2_3	= 1<<1,   
		FE_288_3_4	= 1<<2,   
		FE_288_5_6	= 1<<3,
		FE_288_6_7	= 1<<4,
		FE_288_7_8	= 1<<5
	}FE_288_Rate_t;

	
		
	/* One point of the lookup table */
	typedef struct
	{
		S32 realval;	/*	real value */
		S32 regval;		/*	binary value */
	} FE_288_LOOKPOINT_t;

	/*	Lookup table definition	*/
	typedef struct
	{
		S32 size;		/*	Size of the lookup table	*/
		FE_288_LOOKPOINT_t table[FE_288_MAXLOOKUPSIZE]; 	/*	Lookup table	*/
	} FE_288_LOOKUP_t;

	long FE_288_PowOf2(int number);
	long FE_288_GetPowOf2(int number);
	U32  FE_288_XtoPowerY(U32 Number, U32 Power);
	long FE_288_BinaryFloatDiv(long n1, long n2, int precision);
	U32 FE_288_GetMclkFreq( IOARCH_Handle_t IOHandle, U32 ExtClk_Hz);
	U32 FE_288_SetMclkFreq( IOARCH_Handle_t IOHandle, U32 ExtClk_Hz, U32 MasterClock);
	
	U32 FE_288_F22Freq(U32 DigFreq,U32 F22);
	U32 FE_288_AuxClkFreq(U32 DigFreq,U32 Prescaler,U32 Divider);
	U32 FE_288_GetErrorCount( IOARCH_Handle_t IOHandle, FE_288_ERRORCOUNTER_t Counter);
	int FE_288_GetPacketErrors( IOARCH_Handle_t IOHandle,FE_288_ERRORCOUNTER_t Counter);
	U32 FE_288_GetDirectErrors( IOARCH_Handle_t IOHandle,FE_288_ERRORCOUNTER_t Counter);
	int FE_288_GetRollOff( IOARCH_Handle_t IOHandle);
	S32 FE_288_CalcDerotFreq(U8 derotmsb,U8 derotlsb,U32 fm);
	S32 FE_288_GetDerotFreq( IOARCH_Handle_t IOHandle,U32 MasterClock);
	U32 FE_288_CalcSymbolRate(U32 MasterClock,U8 Hbyte,U8 Mbyte,U8 Lbyte);
	U32 FE_288_SetSymbolRate( IOARCH_Handle_t IOHandle,U32 MasterClock,U32 SymbolRate);
	void FE_288_IncSymbolRate( IOARCH_Handle_t IOHandle,S32 Increment);
	void FE_288_OffsetSymbolRate( IOARCH_Handle_t IOHandle,U32 MasterClock,S32 Offset);
	U32 FE_288_GetSymbolRate( IOARCH_Handle_t IOHandle,U32 MasterClock);
	long CarrierWidth(long SymbolRate, long RollOff);
	U32 FE_288_GetError( IOARCH_Handle_t IOHandle);
	BOOL FE_288_Status( IOARCH_Handle_t IOHandle);
	
	
#ifdef __cplusplus
extern "C"
{
#endif 
#endif
