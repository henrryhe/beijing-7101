#ifndef H_399DRV
	#define H_399DRV
	
		#include "399_init.h"
		#include "util399E.h"
		#include "sttuner.h"
		#include "diseqc.h"
		#include "mlnb.h"
	

#define WAIT_N_MS_399(X)     STOS_TaskDelayUs(X *1000)  
	
/* MACRO definitions */
#define MAC0399_ABS(X)   ((X)<0 ?   (-X) : (X))

	typedef enum
	{								
		FE_PARALLEL_CLOCK,		
		FE_SERIAL_MASTER_CLOCK,
		FE_SERIAL_VCODIV6_CLOCK
	} FE_399_Clock_t;

	
	/****************************************************************
							SEARCH STRUCTURES
	 ****************************************************************/



	/************************
		SATELLITE DELIVERY
	 ************************/
typedef enum
	{
		NOAGC1_399E=0,
		AGC1OK_399E,
		NOTIMING_399E,
		ANALOGCARRIER_399E,
		TIMINGOK_399E,
		NOAGC2_399E,
		AGC2OK_399E,
		NOCARRIER_399E,
		CARRIEROK_399E,
		NODATA_399E,
		FALSELOCK_399E,
		DATAOK_399E,
		OUTOFRANGE_399E,
		RANGEOK_399E
	} FE_399_SIGNALTYPE_t;




	typedef struct
	{
		U32 Frequency;						/* transponder frequency (in KHz)		*/
		U32 SymbolRate;						/* transponder symbol rate  (in bds)	*/
		U32 SearchRange;					/* range of the search (in Hz)  		*/	
		FE_399_Modulation_t Modulation;		/* modulation							*/
		STTUNER_IQMode_t    DemodIQMode; /* added to pass IQ mode for data search*/
		FE_399_SIGNALTYPE_t State; /* Added for Dish Positioning */
	} FE_399_SearchParams_t;


	typedef struct
	{
		BOOL Locked;						/* Transponder found 					*/
		U32	Frequency;						/* found frequency	 					*/
		U32 SymbolRate;						/* founded symbol rate					*/
		FE_399_Rate_t Rate;					/* puncture rate 						*/
	} FE_399_SearchResult_t;
	
	/*	Internal result structure	*/  
	
	  
typedef struct
{
	FE_399_SIGNALTYPE_t      SignalType;	/*	Type of founded signal	*/
	FE_399_Polarization_t    Polarization;	/*	Polarization	*/
	FE_399_Rate_t            PunctureRate;	/*	Puncture rate found	*/
	U32                      Frequency;		/*	Transponder frequency (KHz)	*/
	U32                      SymbolRate; 	/*	Symbol rate (Bds)	*/
} FE_399_InternalResults_t;


	/************************
		INFO STRUCTURE
	************************/
	#ifndef STTUNER_MINIDRIVER
	
	
	
	/*	Internal param structure	*/ 
	typedef struct
	{
	
	FE_399_SIGNALTYPE_t State;	/*	Current state of the search algorithm */

	S32		Quartz,				/*	Quartz frequency (Hz) */
			Frequency,			/*	Current tuner frequency (KHz) */
			FreqOffset,			/*	Frequency offset (Hz) for spurious workaround */
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
			Tderot,				/*	Derotator time constant (ms) */
			Tdata,				/*	Data recovery time constant (ms) */
			SubDir;			/*	Direction of the next sub range */
			
	FE_399_InternalResults_t	Results;	/* Results of the search	*/
	FE_399_Error_t			Error;
	STTUNER_IQMode_t            DemodIQMode;/*added for passing IQ mode for data search*/
}FE_399_InternalParams_t;
	
	
	
        #endif
        #ifdef STTUNER_MINIDRIVER
	
	typedef struct
	{
		BOOL Locked;						/* Transponder locked					*/
		U32 Frequency;						/* transponder frequency (in KHz)		*/
		U32 SymbolRate;						/* transponder symbol rate  (in Mbds)	*/
		FE_399_Modulation_t Modulation;
		FE_399_Rate_t Rate;					/* puncture rate 						*/       
		S16 Power;							/* Power of the RF signal (dBm)			*/			
		U32	C_N;							/* Carrier to noise ratio				*/
		U32	BER;							/* Bit error rate						*/
	} FE_399_SignalInfo_t;
	
typedef struct
{
    ST_DeviceName_t            *DeviceName;         /* unique name for opening under */
    STTUNER_Handle_t            TopLevelHandle;     /* access tuner, lnb etc. using this */
   
    FE_399_Modulation_t         FE_399_Modulation;
    ST_Partition_t             *MemoryPartition;    /* which partition this data block belongs to  */
    U32                         ExternalClock;      /* External VCO */
    STTUNER_TSOutputMode_t      TSOutputMode;
    STTUNER_SerialDataMode_t    SerialDataMode;
    STTUNER_BlockSyncMode_t     BlockSyncMode;     /* add block sync bit control for bug GNBvd27452*/
    STTUNER_DataFIFOMode_t      DataFIFOMode;      /* add block sync bit control for bug GNBvd27452*/
    STTUNER_OutputFIFOConfig_t  OutputFIFOConfig;  /* add block sync bit control for bug GNBvd27452*/
    STTUNER_SerialClockSource_t SerialClockSource;
    STTUNER_FECMode_t           FECMode;
    STTUNER_DiSEqCConfig_t      DiSEqCConfig; /** Added DiSEqC configuration structure to get the trace of current state**/
    BOOL                        DISECQ_ST_ENABLE;  /* Demod is capable to send DiSEqC-ST command or not Direct-True; LOOPTHROUGH-FALSE*/
} D0399_InstanceData_t;  
	
        #endif

	/****************************************************************
						API FUNCTIONS
	****************************************************************/

	ST_Revision_t Drv0399_GetLLARevision(void);
	#ifdef STTUNER_MINIDRIVER
	void FE_399_Init(void);
	FE_399_Error_t	FE_399_Search(FE_399_SearchParams_t	*pParams, FE_399_SearchResult_t	*pResult);
	FE_399_Error_t	FE_399_GetSignalInfo(FE_399_SignalInfo_t	*pInfo);
	FE_399_Error_t	FE_399_SetPol(FE_399_Handle_t		Handle,FE_399_Polarization_t	Polarization);
	FE_399_Error_t	FE_399_SetBand(FE_399_Handle_t		Handle,FE_399_Bands_t Band);
	void	FE_399_Term(void); 
	#endif
	#ifndef STTUNER_MINIDRIVER
		
        FE_399_Error_t	FE_399_Search(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_399_SearchParams_t *pParams, FE_399_SearchResult_t *pResult);
								
	FE_399_Error_t	FE_399_CarrierTracking(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_399_InternalParams_t *pParams);    		
	
	FE_399_Error_t	FE_399_SetPol(	STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_399_Polarization_t	Polarization);
    
	FE_399_Error_t  FE_399_SetPower(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,LNB_Status_t Power,FE_399_InternalParams_t *pParams);
								
	FE_399_Error_t	FE_399_SetBand(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,	FE_399_Bands_t Band,FE_399_InternalParams_t *pParams);

	void FE_399_LockAgc0and1Bis(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
	
	#endif	
#endif
