/* -------------------------------------------------------------------------
File Name: drv0899.h

History:
Description: STb0899 driver LLA	V1.0.8 17/01/2005

Date 18/04/05
Comments: LLA2.0.0 integration
author: SD

Date 29/03/06
Comments: LLA V3.6.0 integration
author: SD
---------------------------------------------------------------------------- */
#ifndef H_STB0899_DRV
	#define H_STB0899_DRV
	
	#include "d0899_init.h"
	#include "d0899_util.h"
	#include "d0899_dvbs2util.h"
	#include "ioreg.h"
	#include "sttuner.h"
	#include "tunsdrv.h"

	/****************************************************************
						COMMON STRUCTURES AND TYPEDEF
	 ****************************************************************/		
	#define ABS(X) ((X)<0 ? (-X) : (X))
	#define MAX(X,Y) ((X)>=(Y) ? (X) : (Y))
	#define MIN(X,Y) ((X)<=(Y) ? (X) : (Y)) 
	#define INRANGE(X,Y,Z) (((X<=Y) && (Y<=Z))||((Z<=Y) && (Y<=X)) ? 1 : 0)  
	#define LSB(X) ((X & 0xFF))
	#define MSB(Y) ((Y>>8)& 0xFF)
	#define MAKEWORD(X,Y) ((X<<8)+(Y))
	
	typedef int FE_STB0899_Handle_t;  
	
	typedef enum
	{
	    FE_899_NO_ERROR,
	    FE_899_INVALID_HANDLE,
	    FE_899_ALLOCATION,
	    FE_899_BAD_PARAMETER,
	    FE_899_ALREADY_INITIALIZED,
	    FE_899_I2C_ERROR,
	    FE_899_SEARCH_FAILED,
	    FE_899_TRACKING_FAILED,
	    FE_899_TERM_FAILED
	} FE_STB0899_Error_t;

	typedef enum
	{
		FE_899_BAND_LOW,
		FE_899_BAND_HIGH
	} FE_STB0899_Bands_t;

	
	typedef enum
	{
	  	FE_899_MOD_QPSK,
	  	FE_899_MOD_BPSK,
	  	FE_899_MOD_OQPSK,
	  	FE_899_MOD_8PSK
	} FE_STB0899_Modulation_t;

	typedef enum
	{
		FE_899_POL_HORIZONTAL	= 1,
		FE_899_POL_VERTICAL		= 1<<1
	}FE_STB0899_Polarization_t;
	
	typedef enum
	{
		FE_899_LNB_CTRL_VOLT_AND_TONE = 0,
		FE_899_LNB_CTRL_TONE_BURST,
		FE_899_LNB_CTRL_DISEQC
	}FE_STB0899_LnbCtrlType_t;
	


	typedef enum
	{
		FE_899_1_2 =13	,
		FE_899_2_3 =18	,	
		FE_899_3_4 =21	 ,
		FE_899_5_6 =24 ,
		FE_899_6_7 =25	 ,
		FE_899_7_8 =26
	}FE_STB0899_Rate_t;
	
	typedef enum
	{
		FE_899_DISPIN_OFF = 0,
		FE_899_DISPIN_ON,
		FE_899_DISPIN_22KHZ,
		FE_899_DISPIN_TONE_BURST,
		FE_899_DISPIN_DISEQC
	}FE_STB0899_DiseqcPinMode_t;
	
	
		typedef enum
	{
		FE_899_LNBTYPE_NONE,		/* NO LNB connected */
		FE_899_LNBTYPE_UNIVERSAL,	/* LNB 22KHz and 13/18v controlled */
		FE_899_LNBTYPE_TONEBURST,	/* LNB 22KHz and 13/18v + tone burst controlled */
		FE_899_LNBTYPE_DISEQC,		/* LNB DiSEqC controlled */
		FE_899_LNBTYPE_SCR_APP1	/* SCR LNB DiSEqC controlled (18V) : One cable single SCR */
		/* add future SCR LNB applications here */
	}FE_LNBTYPE_t;
	
	typedef enum
	{								
		FE_899_PARALLEL_CLOCK,		
		FE_899_SERIAL_MASTER_CLOCK,
		FE_899_SERIAL_VCODIV6_CLOCK
	} FE_STB0899_Clock_t;

	typedef enum
	{
		FE_899_PARITY_ON,
		FE_899_PARITY_OFF
	} FE_STB0899_DataParity_t;

	typedef enum
	{	
		FE_899_DVBS1_STANDARD,					/* The FEC mode corresponds to the DVB standard	*/
		FE_899_DTV_STANDARD,						/* The FEC mode corresponds to the DIRECTV standard	*/
		FE_899_DVBS2_STANDARD
	} FE_STB0899_CodingStandard_t;
	

	/************************
		SATELLITE DELIVERY
	 ************************/

	/****************************************************************
						SEARCH STRUCTURES
	 ****************************************************************/



	typedef struct
	{
		U32 Frequency;						/* transponder frequency (in KHz)		*/
		U32 SymbolRate;						/* transponder symbol rate  (in bds)	*/
		U32 SearchRange;					/* range of the search (in Hz)  		*/	
		FE_STB0899_Modulation_t Modulation;	/* modulation							*/
		STTUNER_FECType_t Standard;	/*Dvb dvbs2 */
		STTUNER_FECMode_t           FECMode; 
		STTUNER_IQMode_t DemodIQMode;
		BOOL Pilots;
	} FE_STB0899_SearchParams_t;
	

	typedef struct
	{
		BOOL Locked;						/* Transponder found 					*/
		U32	Frequency;						/* found frequency	 					*/
		U32 SymbolRate;						/* founded symbol rate					*/
		FE_STB0899_Rate_t Rate;				/* puncture rate  for DVBS1 DTV			*/
		FE_DVBS2_ModCod_t ModCode;			/* only for DVBS2*/
		BOOL Pilots;						/* pilots found							*/
		FE_DVBS2_FRAME FrameLength;			/* found frame length					*/

	} FE_STB0899_SearchResult_t;
	

	/************************
		INFO STRUCTURE
	************************/
	typedef struct
	{
		BOOL Locked;						/* Transponder locked					*/
		U32 Frequency;						/* transponder frequency (in KHz)		*/
		U32 SymbolRate;						/* transponder symbol rate  (in Mbds)	*/
		FE_STB0899_Modulation_t Modulation;	/* modulation							*/
		FE_STB0899_Polarization_t Polarization;	/* Polarization							*/
		FE_STB0899_Bands_t Band;				/* Band									*/
		FE_STB0899_Rate_t Rate;				/* puncture rate 						*/       
		FE_DVBS2_ModCod_t ModCode;
		BOOL Pilots;						/* Pilots on/off only for DVB-S2		*/
		FE_DVBS2_FRAME FrameLength;			/* found frame length					*/
		S32 Power;							/* Power of the RF signal (dBm)			*/			
		U32	C_N;							/* Carrier to noise ratio				*/
		U32	BER;	
		S16	SpectralInv;		/* I,Q Inversion  */ 								/* Bit error rate						*/
	} FE_STB0899_SignalInfo_t;



/*	Internal param structure	*/ 

typedef struct
{
	
	FE_STB0899_Polarization_t Polarization;	/*	Polarization	*/
	
	/*DVB Internal Params*/
	U32 Frequency;				/*	Transponder frequency (KHz)	*/   
	FE_STB0899_SIGNALTYPE_t	SignalType;		/*	Type of founded signal	*/
	FE_STB0899_Rate_t PunctureRate;		/*	Puncture rate found	*/
	U32 SymbolRate; 			/*	Symbol rate (Bds)	*/
	
	/*DVBS2 Internal Params*/
	FE_DVBS2_State	DVBS2SignalType;
	U32 DVBS2SymbolRate; 			/*	Symbol rate (Bds)	*/
	FE_DVBS2_ModCod_t ModCode;
	BOOL Pilots;					/*	Pilots founded	*/
	FE_DVBS2_FRAME FrameLength;			/* found frame length  */

} FE_STB0899_InternalResults_t;
typedef struct
{
	
	STTUNER_FECType_t Standard;
	STTUNER_FECMode_t           FECMode; 
	S32		Quartz;				/*	Quartz frequency (Hz) */
	
		
	S32		Frequency,			/*	Current tuner frequency (KHz) */
			BaseFreq,			/*	Start tuner frequency (KHz) */
			SubRange,			/*	Current sub range (Hz) */
			TunerStep,			/*	Tuner step (Hz) */
			TunerOffset;		/*	Tuner offset relative to the carrier (Hz) */
	U32	    TunerBW;			/*	Current bandwidth of the tuner (Hz) */ 
    long TunerIQSense;
    STTUNER_IQMode_t DemodIQMode;
	/*DVBS1,DTV Params*/
	FE_STB0899_SIGNALTYPE_t State;/*	Current state of the search algorithm */
	FE_DVBS2_State	DVBS2State;

	U32		SymbolRate,			/*	Symbol rate (Bds) */
			MasterClock,		/*	Master clock frequency (Hz) */
			Mclk,				/*	Divider factor for masterclock (binary value) */
			SearchRange,		/*	Search range (Hz) */
			RollOff;			/*	Current RollOff of the filter (x100) */
		
	S16		DerotFreq,			/*	Current frequency of the derotator (Hz) */
			DerotPercent,		/*	Derotator step (in thousands of symbol rate) */
			DerotStep,			/*	Derotator step (binary value) */
			Direction,			/*	Current search direction */ 
			Tagc1,				/*	Agc1 time constant (ms) */
			Tagc2,				/*	Agc2 time constant (ms) */
			Ttiming,			/*	Timing loop time constant (ms) */
			Tderot,				/*	Derotator time constant (ms) */
			Tdata,				/*	Data recovery time constant (ms) */
			SubDir;				/*	Direction of the next sub range */
	/*DVBS2 Params*/
	
	S32 	DVBS2SymbolRate,
			AgcGain,			/* RF AGC Gain */
			AveFrameCoarse,
			AveFramefine,
			CenterFreq;
		
	S16		IQLocked,
			StepSize;
	STTUNER_IQMode_t SpectralInv;
	
	FE_DVBS2_RRCAlpha_t RrcAlpha;
	FE_STB0899_InternalResults_t Results;
	BOOL Pilots;
}FE_STB0899_InternalParams_t;
	/****************************************************************
						API FUNCTIONS
	****************************************************************/

	FE_STB0899_Error_t	FE_STB0899_Search(	STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,
										    FE_STB0899_SearchParams_t	*pParams,
										    FE_STB0899_SearchResult_t	*pResult,
										    STTUNER_Handle_t            TopLevelHandle);
										    
										    
 
	FE_STB0899_Error_t   FE_STB0899_GetSignalInfo(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,STTUNER_Handle_t TopLevelHandle, FE_STB0899_SignalInfo_t	*pInfo, STTUNER_FECType_t FECType, STTUNER_FECMode_t FECMode);
	FE_STB0899_Error_t	FE_STB0899_DiseqcSend(	STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle ,
											    U8 *Data,
											    U32 NbData);
	
	
	/*U32 FE_STB0899_GetError(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_STB0899_CodingStandard_t Standard);*/
    int FE_STB0899_GetCarrierQuality(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, STTUNER_FECType_t FECType, STTUNER_FECMode_t FECMode);
	U32 FE_STB0899_ToneDetection(STTUNER_IOREG_DeviceMap_t *DeviceMap,STTUNER_Handle_t TopLevelHandle,U32 StartFreq,U32 StopFreq,U32 *ToneList, U8 mode);
	U32 FE_STB0899_SpectrumAnalysis(STTUNER_tuner_instance_t *TunerInstance, STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 StartFreq,U32 StopFreq,U32 StepSize,U32 *ToneList, U8 mode);
	FE_STB0899_Error_t FE_STB0899_SetMclk(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 Mclk, FE_STB0899_InternalParams_t	*Params);
#endif

