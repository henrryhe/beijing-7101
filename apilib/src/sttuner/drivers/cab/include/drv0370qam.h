/* ----------------------------------------------------------------------------
File Name: drv0370qam.h 

Description: 

    

Copyright (C) 2005-2006 STMicroelectronics

   date: 
version: 
 author: 
comment: 
    
Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_CAB_DRV0370QAM_H
#define __STTUNER_CAB_DRV0370QAM_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

#include "stddefs.h"    /* Standard definitions */

/* internal types */
#include "dbtypes.h"    /* data types for databases */

#include "sttuner.h"
#include "d0370qam.h"    /* top level include for this driver */

/* types ------------------------------------------------------------------- */

typedef enum
{
    E370QAM_NOCARRIER=0,
    E370QAM_CARRIEROK,

    E370QAM_NOAGC,
    E370QAM_AGCOK,
    E370QAM_AGCMAX,

    E370QAM_NOEQUALIZER,
    E370QAM_EQUALIZEROK,

    E370QAM_NODATA,
    E370QAM_DATAOK,

    E370QAM_LOCKOK
} 
D0370QAM_SignalType_t;



typedef enum
	{
		NOAGC1_370QAM=0,
		AGC1OK_370QAM,
		NOTIMING_370QAM,
		ANALOGCARRIER_370QAM,
		TIMINGOK_370QAM,
		NOAGC2_370QAM,
		AGC2OK_370QAM,
		NOCARRIER_370QAM,
		CARRIEROK_370QAM,
		NODATA_370QAM,
		FALSELOCK_370QAM,
		DATAOK_370QAM,
		OUTOFRANGE_370QAM,
		RANGEOK_370QAM
	} FE_370QAM_SIGNALTYPE_t;
	
/*	Scan direction enum	*/
typedef enum 
{
    E370QAM_SCANUP   =  1,
    E370QAM_SCANDOWN = -1
}
D0370QAM_Scandir_t; 


/* driver internal types --------------------------------------------------- */
typedef struct
	{
		enum       	    
		{ 
		  _370QAM_NO_LOCKED = 0 ,
		  _370QAM_LOCKED    = 1 , 
		  _370QAM_LOCK_LOSS = 2 
		
		} Status;              
		
		int  NbLockLoss;				
	
	}D0370QAM_SignalStatus_t;

/*	Search result structure	*/    
typedef struct
{
    D0370QAM_SignalStatus_t   Lock;
    D0370QAM_SignalType_t     SignalType;
    long                    Frequency;
    long                    SymbolRate; 
    STTUNER_Modulation_t    QAMSize;
}
D0370QAM_Searchresult_t;

/* AGC2 gain and frequency structure */
typedef struct
{
    int  NbPoints;
    int  Gain[3];
    long Frequency[3];
}
D0370QAM_Agc2TimingOK_t;	


/*	Search parameters structure	*/         
typedef struct
{
    D0370QAM_SignalType_t     State;

    long Frequency;
    long SymbolRate;
    long RollOff;
    long TunerStep;
    long TunerOffset;
    long TunerBW;
    long TunerIF;

    short int Direction;
}
D0370QAM_SearchParams_t;


typedef enum
	{
	  	FE_MOD_16QAM=16,
	  	FE_MOD_32QAM=32,
	  	FE_MOD_64QAM=64,
	  	FE_MOD_128QAM=128,
	  	FE_MOD_256QAM=256
	} FE_370QAM_Modulation_t;
	
typedef struct
{
    int                     CarrierOffset;
    int                     TimingOffset;
    int                     lastAGC2Coef;
    int                     ScanMode;
    /**/
    STTUNER_Modulation_t    QAMSize;
    STTUNER_Spectrum_t      SpectrumInversion;
    STTUNER_J83_t           J83;
    int                     SweepRate;
    long                    InitDemodOffset;
    /**/

    D0370QAM_SearchParams_t   Params;
    D0370QAM_Searchresult_t   Result;

    /**/
    int                     Ber[3];  /* 0 : Ber Cnt
                                        1 : Ber Saturation en %
                                        2 : Ber
                                     */
    int                     CNdB; /* In dB */
    int                     SignalQuality; /* SignalQuality from 0 to 100 % */

    U16                     BlkCounter;
    U16                     CorrBlk;
    U16                     UncorrBlk;
    S32			    Quartz;/*	Quartz frequency (Hz) */

} 
D0370QAM_StateBlock_t;

typedef enum
{
	FE_IERR_NO,				/*	no error		*/
	FE_IERR_I2C,			/*	I2C error		*/
	FE_IERR_ZERODIV,		/*	division by zero	*/
	FE_IERR_PARAM,			/*	wrong parameters	*/
	FE_IERR_UNKNOWN			/*	unknown error	*/
} FE_370QAM_ErrorType_t;


typedef enum
{
	FE_IERRLOC_NOWHERE,		/*	no location		*/
	FE_IERRLOC_SRHINIT,		/*	in SearchInit	*/
	FE_IERRLOC_SRHRUN,		/*	in SearchRun	*/
	FE_IERRLOC_SRHTERM		/*	in SearchTerm	*/
} FE_370QAM_Location_t;

typedef struct
{
	FE_370QAM_ErrorType_t Type;	/* Error type	*/
	FE_370QAM_Location_t Location;	/* Error location	*/
} FE_370QAM_InternalError_t;
typedef struct
{
	FE_370QAM_SIGNALTYPE_t      SignalType;	/*	Type of founded signal	*/
	U32                      Frequency;		/*	Transponder frequency (KHz)	*/
	U32                      SymbolRate; 	/*	Symbol rate (Bds)	*/
	FE_370QAM_Modulation_t  Modulation;
	
} FE_370QAM_InternalResults_t;

typedef struct
{
	/*STCHIP_Handle_t hDemod;*/		/*	Handle to the demod	*/
	/*TUNER_Handle_t hTuner;*/		/*  Handle to the tuner */

	FE_370QAM_SIGNALTYPE_t State;	/*	Current state of the search algorithm */
        U32            IF;       /* Intermediate frequency (KHz) */
	S32		Quartz,				/*	Quartz frequency (Hz) */
			Frequency,			/*	Current tuner frequency (KHz) */
			CarrierOffset,			/*	Old Frequency offset (Hz) for spurious workaround */
			BaseFreq,			/*	Start tuner frequency (KHz) */
			SymbolRate,			/*	Symbol rate (Bds) */
			MasterClock,		/*	Master clock frequency (Hz) */
			Mclk,				/*	Divider factor for masterclock (binary value) */
			SearchRange,		/*	Search range (Hz) */
			SubRange,			/*	Current sub range (Hz) */
			TunerStep,			/*	Tuner step (Hz) */
			TunerOffset,		/*	Tuner offset relative to the carrier (Hz) */
			TunerBW,			/*	Current bandwidth of the tuner (Hz) */ 
			RollOff,			/*	Current RollOff of the filter (x100) */
			SweepRate;
		
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
	FE_370QAM_Modulation_t Modulation;
	long ScanRange; 
	FLAG_370QAM SpecInv;		
	long InitDemodOffset;
	long _TunerJump;
	int _NbSteps;
	int _NbTrials;
	FLAG_370QAM _StopFailure;
	
	
	FE_370QAM_InternalResults_t	Results;	/* Results of the search	*/
	FE_370QAM_InternalError_t		Error;		/* Last error encountered	*/
}FE_370QAM_InternalParams_t;


/* ---------- per instance of driver ---------- */
typedef struct
{
    ST_DeviceName_t           *DeviceName;           /* unique name for opening under */
    STTUNER_Handle_t          TopLevelHandle;       /* access tuner, lnb etc. using this */
    IOARCH_Handle_t           IOHandle;             /* instance access to I/O driver     */
    STTUNER_IOREG_DeviceMap_t DeviceMap;            /* stv0297J register map & data table */
    D0370QAM_StateBlock_t        StateBlock;           /* driver search/state information   */
    ST_Partition_t            *MemoryPartition;     /* which partition this data block belongs to */
    void                      *InstanceChainPrev;   /* previous data block in chain or NULL if not */
    void                      *InstanceChainNext;   /* next data block in chain or NULL if last */

    U32                         ExternalClock;  /* External VCO */
    STTUNER_TSOutputMode_t      TSOutputMode;
    STTUNER_SerialDataMode_t    SerialDataMode;
    STTUNER_SerialClockSource_t SerialClockSource;
    STTUNER_FECMode_t           FECMode;
    STTUNER_Sti5518_t           Sti5518;        /* 297J FEC-B: TS output compatibility with Sti5518 */
    int         Driv0370QAMCNEstimation; /*Used in signal to noise ration calculation*/
    int Driv0370QAMCNEstimatorOffset; /*Used in signal to noise ration calculation*/
    U32                         StandBy_Flag; /*** To take care same Standby Mode doesnot
                                                   execute more than once ***/
 }
D0370QAM_InstanceData_t;



/* functions --------------------------------------------------------------- */

ST_Revision_t Drv0370QAM_GetLLARevision(void);

long Drv0370QAM_CarrierWidth   (long SymbolRate, long RollOff);

D0370QAM_SignalType_t Drv0370QAM_CheckAgc(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0370QAM_SearchParams_t *Params);
D0370QAM_SignalType_t Drv0370QAM_CheckData(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0370QAM_SearchParams_t *Params);


void Drv0370QAM_InitSearch(STTUNER_tuner_instance_t *TunerInstance, D0370QAM_StateBlock_t *StateBlock,
                        STTUNER_Modulation_t Modulation, int Frequency, int SymbolRate, STTUNER_Spectrum_t Spectrum,
                        BOOL ScanExact, STTUNER_J83_t J83);


void Driv0370QAMDemodSetting(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,
                          long Offset,int ExtClk,U32	TunerIF);
void Driv0370QAMCNEstimator(DEMOD_Handle_t Handle,STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,
                           int *Mean_p, int *CN_dB100_p, int *CN_Ratio_p);
                           

void Driv0370QAMBertCount(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,
                       D0370QAM_StateBlock_t *StateBlock_p, STTUNER_tuner_instance_t *TunerInstance_p,
                       int *_pBER_Cnt, int *_pBER);

FLAG_370QAM Driv0370QAMFECLockCheck (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_370QAM_InternalParams_t *pParams, FLAG_370QAM EndOfSearch, int TimeOut, int DataSearchTime);
ST_ErrorCode_t Driv0370QAMFecInit (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
ST_ErrorCode_t LoadRegisters(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,int QAMSize,int TunerType);
BOOL Driv0370QAMDataSearch(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,
                        FE_370QAM_InternalParams_t *pParams, STTUNER_tuner_instance_t *TunerInstance_p);
BOOL      FE_370QAM_Search(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,D0370QAM_StateBlock_t *pSearch, STTUNER_Handle_t  TopLevelHandle);

D0370QAM_InstanceData_t *D0370QAM_GetInstFromHandle(DEMOD_Handle_t Handle);/*This prototype is made here to access this function from drv0370qam.c*/ 

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_CAB_DRV0370QAM_H */


/* End of drv0370qam.h */
