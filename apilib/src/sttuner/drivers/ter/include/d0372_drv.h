#ifndef H_D0372_DRV
	#define H_D0372_DRV
	
	
	#include "d0372_init.h"
	#include "d0372_util.h"

	#define SYMBOLRATE_372  10762237
	/****************************************************************
						COMMON STRUCTURES AND TYPEDEF
	 ****************************************************************/		
	typedef int FE_372_Handle_t;

	typedef enum
	{
	    FE_372_NO_ERROR,
	    FE_372_INVALID_HANDLE,
	    FE_372_ALLOCATION,
	    FE_372_BAD_PARAMETER,
	    FE_372_ALREADY_INITIALIZED,
	    FE_372_I2C_ERROR,
	    FE_372_SEARCH_FAILED,
	    FE_372_TRACKING_FAILED,
	    FE_372_TERM_FAILED
	} FE_372_Error_t;

	typedef struct
	{
		U32 Frequency;						/* transponder frequency (in KHz)		*/
		U32 SymbolRate;						/* transponder symbol rate  (in bds)	*/
		U32 SearchRange;					/* range of the search (in Hz)  		*/	
	        U8  IQMode;                                          /*IQ mode*/
                S32	EQResetNBMAX;
		U32	FistWatchdog;
		U32	DefaultWatchdog;
		U16	LOWPowerTh;
		U16	UPPowerTh; 
	} FE_372_SearchParams_t;


	typedef struct
	{
		BOOL Locked;						/* Transponder found 					*/
		U32	Frequency;						/* found frequency	 					*/
		U32 SymbolRate;						/* founded symbol rate					*/
	} FE_372_SearchResult_t;

	 /****************************************************************/
/* enumerations------------------------------------------------------------- */ 
/*	Internal error definitions	*/
typedef enum
{
	FE_372_IERR_NO,				/*	no error		*/
	FE_372_IERR_I2C,			/*	I2C error		*/
	FE_372_IERR_ZERODIV,		/*	division by zero	*/
	FE_372_IERR_PARAM,			/*	wrong parameters	*/
	FE_372_IERR_UNKNOWN			/*	unknown error	*/
} FE_372_ErrorType_t;

typedef enum
{
	FE_372_LOC_NOWHERE,			/*	no location		*/
	FE_372_LOC_SRHINIT,			/*	in SearchInit	*/
	FE_372_LOC_SRHRUN,			/*	in SearchRun	*/
	FE_372_LOC_SRHTERM,			/*	in SearchTerm	*/
	FE_372_LOC_SETSR,			/*	in SetSymbolRate	*/
	FE_372_LOC_TIMTCST,			/*	in TimingTimeConstant	*/
	FE_372_LOC_DERTCST,			/*	in DerotTimeConstant	*/
	FE_372_LOC_DATTCST,			/*	in DataTimeConstant	*/
	FE_372_LOC_CHKTIM,			/*	in CheckTiming	*/
	FE_372_LOC_SRHCAR,			/*	in SearchCarrier	*/
	FE_372_LOC_SRHDAT,			/*	in SearchData	*/
	FE_372_LOC_CHKRNG,			/*	in CheckRange	*/
	FE_372_LOC_SELLPF			/*	in SelectLPF	*/
} FE_372_Location_t;

typedef struct
{
	FE_372_ErrorType_t Type;	/* Error type	*/
	FE_372_Location_t Location;	/* Error location	*/
} FE_372_InternalError_t;



/* structures -------------------------------------------------------------- */ 
/*	Internal result structure	*/    
typedef struct
{
	FE_372_SIGNALTYPE_t      SignalType;	/*	Type of founded signal	*/
	U32                      Frequency;		/*	Transponder frequenc (KHz)	*/
	U32                      SymbolRate; 	/*	Symbol rate (Bds)	*/
	S32			 EQResetNB;    
} FE_372_InternalResults_t;
	 
	 /*	Internal param structure	*/ 
typedef struct
{
	FE_372_SIGNALTYPE_t State;	/*	Current state of the search algorithm */
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
			
	FE_372_InternalResults_t	Results;	/* Results of the search	*/
	FE_372_InternalError_t		Error;		/* Last error encountered	*/
	U32 IQMode;   /*IQ mode */
	U32    Watchdog;			/*  First watchdog value in ms */
	U32    TimeOut;	/*  Default watchdog value in ms */  
	S16 EQResetNBMax;
	U16	LOWPowerTh;
	U16	UPPowerTh; 
}FE_372_InternalParams_t;


	
	
	FE_372_Error_t	FE_372_Search(IOARCH_Handle_t IOHandle,
						FE_372_SearchParams_t	*pSearch,
						FE_372_SearchResult_t	*pResult,
						 STTUNER_Handle_t            TopLevelHandle);
						 S32 FE_372_GetCarrierToNoiseRatio(IOARCH_Handle_t IOHandle);
						 S32 FE_372_GetBitErrorRate(IOARCH_Handle_t IOHandle);
	

#endif
