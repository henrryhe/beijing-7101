#ifndef H_370_VSB_DRV
	#define H_370_VSB_DRV
	
	
	#include "370_VSB_init.h"
	#include "370_VSB_util.h"

	#define SYMBOLRATE 10762237
	/****************************************************************
						COMMON STRUCTURES AND TYPEDEF
	 ****************************************************************/		
	typedef int FE_370_VSB_Handle_t;

	typedef enum
	{
		FE_VSB_NO_ERROR,
	    FE_VSB_INVALID_HANDLE,
	    FE_VSB_ALLOCATION,
	    FE_VSB_BAD_PARAMETER,
	    FE_VSB_ALREADY_INITIALIZED,
	    FE_VSB_I2C_ERROR,
	    FE_VSB_SEARCH_FAILED,
	    FE_VSB_TRACKING_FAILED,
	    FE_VSB_TERM_FAILED
	} FE_370_VSB_Error_t;
	
	typedef enum
	{								
		FE_PARALLEL_CLOCK_VSB370,		
		FE_SERIAL_CLOCK_VSB370
	} FE_370_VSB_Clock_t;

	typedef enum
	{
		FE_POLARITY_NORMAL,
		FE_POLARITY_INVERTED,
		FE_POLARITY_DEFAULT
	} FE_370_VSB_ClockPolarity_t;


	typedef struct
	{
		U32 Frequency;						/* transponder frequency (in KHz)		*/
		U32 SymbolRate;						/* transponder symbol rate  (in bds)	*/
		U32 SearchRange;					/* range of the search (in Hz)  		*/	
	        U8  IQMode;                                          /*IQ mode*/
		U32	FistWatchdog;
		U32	DefaultWatchdog;
		U16	LOWPowerTh;
		U16	UPPowerTh; 	
	} FE_370_VSB_SearchParams_t;


	typedef struct
	{
		BOOL Locked;						/* Transponder found 					*/
		U32	Frequency;						/* found frequency	 					*/
		U32 SymbolRate;						/* founded symbol rate					*/
	} FE_370_VSB_SearchResult_t;

	/************************
		INFO STRUCTURE
	************************/
	typedef struct
	{
		BOOL Locked;						/* Transponder locked					*/
		U32 Frequency;						/* transponder frequency (in KHz)		*/
		U32 SymbolRate;						/* transponder symbol rate  (in Mbds)	*/
		U32 Power;							/* Power of the RF signal (dBm)			*/			
		U32	C_N;							/* Carrier to noise ratio				*/
		U32	BER;							/* Bit error rate						*/
	} FE_370_VSB_SignalInfo_t;
	 /****************************************************************/
/* enumerations------------------------------------------------------------- */ 
/*	Internal error definitions	*/
typedef enum
{
	FE_VSB_IERR_NO,				/*	no error		*/
	FE_VSB_IERR_I2C,			/*	I2C error		*/
	FE_VSB_IERR_ZERODIV,		/*	division by zero	*/
	FE_VSB_IERR_PARAM,			/*	wrong parameters	*/
	FE_VSB_IERR_UNKNOWN			/*	unknown error	*/
} FE_370_VSB_ErrorType_t;

typedef enum
{
	FE_VSB_LOC_NOWHERE,			/*	no location		*/
	FE_VSB_LOC_SRHINIT,			/*	in SearchInit	*/
	FE_VSB_LOC_SRHRUN,			/*	in SearchRun	*/
	FE_VSB_LOC_SRHTERM,			/*	in SearchTerm	*/
	FE_VSB_LOC_SETSR,			/*	in SetSymbolRate	*/
	FE_VSB_LOC_TIMTCST,			/*	in TimingTimeConstant	*/
	FE_VSB_LOC_DERTCST,			/*	in DerotTimeConstant	*/
	FE_VSB_LOC_DATTCST,			/*	in DataTimeConstant	*/
	FE_VSB_LOC_CHKTIM,			/*	in CheckTiming	*/
	FE_VSB_LOC_SRHCAR,			/*	in SearchCarrier	*/
	FE_VSB_LOC_SRHDAT,			/*	in SearchData	*/
	FE_VSB_LOC_CHKRNG,			/*	in CheckRange	*/
	FE_VSB_LOC_SELLPF			/*	in SelectLPF	*/
} FE_370_VSB_Location_t;

typedef struct
{
	FE_370_VSB_ErrorType_t Type;	/* Error type	*/
	FE_370_VSB_Location_t Location;	/* Error location	*/
} FE_370_VSB_InternalError_t;



/* structures -------------------------------------------------------------- */ 
/*	Internal result structure	*/    
typedef struct
{
	FE_370_VSB_SIGNALTYPE_t      SignalType;	/*	Type of founded signal	*/
	U32                      Frequency;		/*	Transponder frequenc (KHz)	*/
	U32                      SymbolRate; 	/*	Symbol rate (Bds)	*/
} FE_370_VSB_InternalResults_t;
	 
	 /*	Internal param structure	*/ 
typedef struct
{
	FE_370_VSB_SIGNALTYPE_t State;	/*	Current state of the search algorithm */
        U32            IF;       /* Intermediate frequency (KHz) */
	S32		Quartz,				/*	Quartz frequency (Hz) */
			Frequency,			/*	Current tuner frequency (KHz) */
			FreqOffset,			/*	Frequency offset (Hz) */
			BaseFreq,			/*	Start tuner frequency (KHz) */
			SymbolRate,			/*	Symbol rate (Bds) */
			MasterClock,		/*	Master clock frequency (Hz) */
			Mclk,				/*	Divider factor for masterclock (binary value) */
			SearchRange,		/*	Search range (Hz) */
			SubRange,			/*	Current sub range (Hz) */
			TunerStep,			/*	Tuner step (Hz) */
			TunerOffset,		/*	Tuner offset relative to the carrier (Hz) */
			TunerBW,			/*	Current bandwidth of the tuner (Hz) */ 
			RollOff;			/*	Current RollOff of the filter (x100) */
		
	S16		DerotFreq,			/*	Current frequency of the derotator (Hz) */
			DerotPercent,		/*	Derotator step (in thousands of symbol rate) */
			DerotStep,			/*	Derotator step (binary value) */
			Direction,			/*	Current search direction */ 
			Tagc1,				/*	Agc1 time constant (ms) */
			Tagc2,				/*	Agc2 time constant (ms) */
			Ttiming,			/*	Timing loop time constant (ms) */
			Tcarrier,			/*	Carrier loop time constant (ms) */
			Tdata,				/*	Data recovery time constant (ms) */
			TFSM1,				/*  Demod state machine time constant (ms) */  
			TFSM2,				/*  Equalizer state machine time constant (ms) */
			SubDir;				/*	Direction of the next sub range */
			
	FE_370_VSB_InternalResults_t	Results;	/* Results of the search	*/
	FE_370_VSB_InternalError_t		Error;		/* Last error encountered	*/
	U32 IQMode;   /*IQ mode */
	U32    Watchdog;			/*  First watchdog value in ms */
	U32    TimeOut;	/*  Default watchdog value in ms */  
	U16	LOWPowerTh;
	U16	UPPowerTh; 
}FE_370_VSB_InternalParams_t;


	
	
	FE_370_VSB_Error_t	FE_370_VSB_Search(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,
						FE_370_VSB_SearchParams_t	*pSearch,
						FE_370_VSB_SearchResult_t	*pResult,
						 STTUNER_Handle_t            TopLevelHandle);
						 S32 FE_370_VSB_GetCarrierToNoiseRatio(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
						 S32 FE_370_VSB_GetBitErrorRate(STTUNER_IOREG_DeviceMap_t *DeviceMap,IOARCH_Handle_t IOHandle);
	

#endif
