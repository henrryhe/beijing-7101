#ifndef H_STB0899_DVBS2UTIL_H
	#define	H_STB0899_DVBS2UTIL_H
	
	#include "ioreg.h"  
	
#define BTR_NCO_BITS	28 
#define CRL_NCO_BITS 30
#define CRL_GAIN_SHIFT_OFFSET 11
#define BTR_GAIN_SHIFT_OFFSET 15


#define ESNO_AVE            3
#define ESNO_QUANT          32
#define AVEFRAMES_COARSE    10
#define AVEFRAMES_FINE      20
#define MISS_THRESHOLD      6
#define UWP_THRESHOLD_ACQ   1125  
#define UWP_THRESHOLD_TRACK 758  
#define UWP_THRESHOLD_SOF   1350  
#define SOF_SEARCH_TIMEOUT 1664100

	
	
	typedef enum
	{
		CORR_PEAK,
		MIN_FREQ_EST,
		UWP_LOCK,
		FEC_LOCK,
		NO_SEARCH
		
	}FE_DVBS2_AcqMode;
	
	typedef enum
	{
		FE_DVBS2_BPSK,
		FE_DVBS2_QPSK,
		FE_DVBS2_8PSK,
		FE_DVBS2_16APSK,
		FE_DVBS2_32APSK
		
	}FE_DVBS2_Mapping_t;

	typedef enum
	{ 
		BPSK,
		QPSK,
		OQPSK,
		PSK8
		
	}FE_DVBS2_Mode_t;
	
	
	typedef enum
	{
		FE_DUMMY_PLF,
		FE_QPSK_14,
		FE_QPSK_13,
		FE_QPSK_25,
		FE_QPSK_12,
		FE_QPSK_35,
		FE_QPSK_23,
		FE_QPSK_34,
		FE_QPSK_45,
		FE_QPSK_56,
		FE_QPSK_89,
		FE_QPSK_910,
		FE_8PSK_35,
		FE_8PSK_23,
		FE_8PSK_34,
		FE_8PSK_56,
		FE_8PSK_89,
		FE_8PSK_910,
		FE_16APSK_23,
		FE_16APSK_34,
		FE_16APSK_45,
		FE_16APSK_56,
		FE_16APSK_89,
		FE_16APSK_910,
		FE_32APSK_34,
		FE_32APSK_45,
		FE_32APSK_56,
		FE_32APSK_89,
		FE_32APSK_910
	}FE_DVBS2_ModCod_t;

	typedef enum
	{
		FE_LONG_FRAME,
		FE_SHORT_FRAME
	}FE_DVBS2_FRAME;
		
	typedef enum
	{
		RRC_20,
		RRC_25,
		RRC_35
		
	}FE_DVBS2_RRCAlpha_t;
	
	typedef enum
	{
		Q_SIGN_I,
		ARCTAN_LUT
		
	}FE_DVBS2_PhaseDetector_t;
	
	typedef enum
	{
		UW_SNRESTIM		   =0,
		UW_INITIAL		   =1,
		COARSE_FREQ_SEARCH =2,
		COARSE_FREQ_EST    =4,
		FINE_FREQ_SEARCH   =8,
		FINE_FREQ_EST      =16,
		SOF_SEARCH         =32,
		SOF_DECODE         =64,
		WAIT			   =128,
		TRACKING           =256,
		RESET              =512
	}FE_DVBS2_UWP_States;
	
	typedef enum
	{
		FE_DVBS2_NOAGC,
		FE_DVBS2_AGCOK,
		FE_DVBS2_TIMINGOK,
		FE_DVBS2_NOTIMING,
		FE_DVBS2_NOCARRIER,
		FE_DVBS2_CARRIEROK,
		FE_DVBS2_NOUWP,
		FE_DVBS2_UWPOK,
		FE_DVBS2_NODATA,
		FE_DVBS2_DATAOK,
		FE_DVBS2_OUTOFRANGE,
		FE_DVBS2_DEMOD_LOCKED,
		FE_DVBS2_DEMOD_NOT_LOCKED,
		FE_DVBS2_FEC_NOT_LOCKED
		
	}FE_DVBS2_State;
	
	
	typedef struct
	{
		U32	LoopBwPercent;
		U32	SymbolRate;
		U32	MasterClock;
		FE_DVBS2_Mode_t	Mode;
		U32	Zeta;
		U32 SymPeakVal;
		
	}FE_DVBS2_LoopBW_Params_t;
	
	
	typedef struct
	{
		U32	Adapt;
		U32	AmplImbEstim;
		U32	PhsImbEstim;
		U32	AmplAdaptLsht;
		U32	PhsAdaptLsht;
	
	}FE_DVBS2_ImbCompInit_Params_t;
	
	
	typedef struct
	{
		S32 EsNoAve,
			EsNoQuant,
			AveFramesCoarse,
			AveframesFine,
			MissThreshold,
			ThresholdAcq,
			ThresholdTrack,
			ThresholdSof,
			SofSearchTimeout;
			
	}FE_DVBS2_UWPConfig_Params_t;
	
	
	typedef struct
	{
		S32	DvtTable,
			TwoPass,
			AgcGain,
			AgcShift,
			FeLoopShift,
			GammaAcq,
			GammaRhoAcq,
			GammaTrack,
			GammaRhoTrack,
			LockCountThreshold,
			PhaseDiffThreshold;
			
	}FE_DVBS2_CSMConfig_Params_t;
	
	typedef struct
	{
		FE_DVBS2_RRCAlpha_t RRCAlpha;
		FE_DVBS2_Mode_t	ModeMode;
		
		S32		SymbolRate,
				MasterClock,
				CarrierFrequency,
				AveFrameCoarse,
				AveFramefine,
				AgcThreshold,
				FreqRange;
		S16 StepSize;		
		U16		SpectralInv;
		
	} FE_STB0899_DVBS2_InitParams_t;
			FE_DVBS2_State FE_DVBS2_GetDemodStatus(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,int timeout);
		
		void FE_DVBS2_SetCarrierFreq(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,
		                             S32 CarrierFreq, U32 MasterClock);
		void FE_DVBS2_SetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,
		                            U32 SymbolRate, U32 MasterClock);
		void FE_DVBS2_SetBtrLoopBW(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,
		                           FE_DVBS2_LoopBW_Params_t LoopBW);
		void FE_DVBS2_BtrInit(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
		void FE_DVBS2_ConfigUWP(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,
		                        FE_DVBS2_UWPConfig_Params_t UWPparams);
		int FE_DVBS2_GetUWPstate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,int TimeOut);
		U32 FE_DVBS2_GetModCod(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
		S32 FE_DVBS2_GetUWPEsNo(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,
		                        S32 Quant);
		void FE_DVBS2_AutoConfigCSM(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
		void FE_DVBS2_ManualConfigCSM(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,
		                              FE_DVBS2_CSMConfig_Params_t CSMParams);
		int FE_DVBS2_GetCSMLock(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,int TimeOut);
		void FE_DVBS2_InitialCalculations(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,
                                         FE_STB0899_DVBS2_InitParams_t *InitParams);
        
        	void FE_DVBS2_Reacquire(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
        
	        long FE_DVBS2_CarrierWidth(long SymbolRate,FE_DVBS2_RRCAlpha_t Aplha);
	        U32 FE_DVBS2_GetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 MasterClock);
	        void FE_DVBS2_CSMInitialize(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,int Pilots,FE_DVBS2_ModCod_t ModCode,U32 SymbolRate,U32 MasterClock);	 
		FE_DVBS2_State FE_DVBS2_GetFecStatus(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,int timeout); 
	
	
#endif


