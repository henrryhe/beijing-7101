#ifndef H_361DRV
	#define H_361DRV	
	
	#ifdef HOST_PC
	#include "gen_types.h"
	#include "tun0360.h" 	
	#include "360_util.h"
	#else
	#include "stddefs.h"
	#include "dbtypes.h"
	#include "sttuner.h"

	#endif
	
	typedef int FE_361_Handle_t;
	
	/****************************************************************
						COMMON STRUCTURES AND TYPEDEFS
	 ****************************************************************/		
	#define AGC_LOCK_FLAG   1<<5
	#define SYR_LOCK_FLAG	1<<6
	#define TPS_LOCK_FLAG	1<<7
	#define CHANNEL_SIZE 8 /* for an 8MHz channel wide OFDM channel*/

	/* for previous lock (during scan)*/
	#define NO_LOCK		   0
	#define LOCK_NO_OFF	   1
	#define LOCK_RI_OFF	   2
	#define LOCK_LE_OFF	   3

	/*options for current lock trial*/
	#define NINV_NOFF 0
	#define NINV_ROFF 2
	#define NINV_LOFF 4
	#define INV_NOFF 1
	#define INV_ROFF 3
	#define INV_LOFF 5
	
		
	#define PAL			1
	#define GLOBAL		1<<1
	
	#define NUM_CHANNELS 40 
	
	#define STEP  166
	
	/* channel offset constant */
	#define LEFT_OFFSET 1
	#define CENTER_OFFSET 1<<1
	#define RIGHT_OFFSET 1<<2
	#define CARRIER 	240	
	/* end channel offset constant */

	typedef enum
	{
		NOAGC_361=0,
		AGC1SATURATION_361=1,
		AGC1OK_361=2,
		
		TIMINGOK_361=3,
		NOAGC2_361=4,
		AGC2OK_361=5,
		NOTPS_361=6,
		TPSOK_361=7,
		NOSYMBOL_361=8,
		BAD_CPQ_361=9,
		NODATA_361=10,
		NOPRFOUND_361=11,
		LOCK_OK_361=12,
		LOCK_KO_361=13,
		RANGEOK_361=14
	} FE_361_SignalStatus_t;
	
	
	
	typedef enum
	{
	UP_361   =1,
	DOWN_361 = -1
	}Zap_direction_361_t;
	
	
	#ifdef HOST_PC
	typedef int STI2C_Handle_t;
	#endif
	
	


	typedef enum
	{
	    FE_361_NO_ERROR,
	    FE_361_INVALID_HANDLE,
	    FE_361_BAD_PARAMETER,
	    FE_361_MISSING_PARAMETER,
	    FE_361_ALREADY_INITIALIZED,
	    FE_361_I2C_ERROR,
	    FE_361_SEARCH_FAILED,
	    FE_361_TRACKING_FAILED,
	    FE_361_TERM_FAILED
	} FE_361_Error_t;
	
	/* type of modulation (common) */
#ifdef HOST_PC
typedef enum STTUNER_Modulation_e
{
STTUNER_MOD_NONE   = 0x00,  /* Modulation unknown */ 
STTUNER_MOD_ALL    = 0x1FF, /* Logical OR of all MODs */ 
STTUNER_MOD_QPSK   = 1,
STTUNER_MOD_8PSK   = (1 << 1),
STTUNER_MOD_QAM    = (1 << 2),
STTUNER_MOD_16QAM  = (1 << 3),
STTUNER_MOD_32QAM  = (1 << 4),
STTUNER_MOD_64QAM  = (1 << 5),
STTUNER_MOD_128QAM = (1 << 6),
STTUNER_MOD_256QAM = (1 << 7),
STTUNER_MOD_BPSK   = (1 << 8)
}
STTUNER_Modulation_t;


/* mode of OFDM signal (ter) */
typedef enum STTUNER_Mode_e
{
STTUNER_MODE_2K,
STTUNER_MODE_8K
}
STTUNER_Mode_t;


/* guard of OFDM signal (ter) */
typedef enum STTUNER_Guard_e
{
STTUNER_GUARD_1_32,               /* Guard interval = 1/32 */ 
STTUNER_GUARD_1_16,               /* Guard interval = 1/16 */ 
STTUNER_GUARD_1_8,                /* Guard interval = 1/8  */ 
STTUNER_GUARD_1_4                 /* Guard interval = 1/4  */
}
STTUNER_Guard_t;

/* hierarchy (ter) */

typedef enum STTUNER_Hierarchy_e
{
STTUNER_HIER_NONE,              /* Regular modulation */ 
STTUNER_HIER_1,                 /* Hierarchical modulation a = 1*/
STTUNER_HIER_2,                 /* Hierarchical modulation a = 2*/
STTUNER_HIER_4                  /* Hierarchical modulation a = 4*/
}
STTUNER_Hierarchy_t;


/* (ter & cable) */
typedef enum STTUNER_Spectrum_e
{
STTUNER_INVERSION_NONE = 0,
STTUNER_INVERSION      = 1,
STTUNER_INVERSION_AUTO = 2,
STTUNER_INVERSION_UNK  = 4
}
STTUNER_Spectrum_t;


/* (ter) */
typedef enum STTUNER_FreqOff_e
{
STTUNER_OFFSET_NONE = 0,
STTUNER_OFFSET      = 1
}
STTUNER_FreqOff_t;


/* (ter) */
typedef enum STTUNER_Force_e
{
STTUNER_FORCENONE  = 0,
STTUNER_FORCE_M_G		= 1
}
STTUNER_Force_t;


/* (ter) */
	typedef enum STTUNER_ChannelBW_e
	{
	STTUNER_CHAN_BW_6M  = 6,
	STTUNER_CHAN_BW_7M  = 7,
	STTUNER_CHAN_BW_8M  = 8
	} STTUNER_ChannelBW_t;

	typedef enum STTUNER_FECRate_e
    {
        STTUNER_FEC_NONE = 0x00,    /* no FEC rate specified */
        STTUNER_FEC_ALL = 0xFF,     /* Logical OR of all FECs */
        STTUNER_FEC_1_2 = 1,
        STTUNER_FEC_2_3 = (1 << 1),
        STTUNER_FEC_3_4 = (1 << 2),
        STTUNER_FEC_4_5 = (1 << 3),
        STTUNER_FEC_5_6 = (1 << 4),
        STTUNER_FEC_6_7 = (1 << 5),
        STTUNER_FEC_7_8 = (1 << 6),
        STTUNER_FEC_8_9 = (1 << 7)
    }
    STTUNER_FECRate_t;


#endif
	typedef enum
	{				   
	FE_361_QUARTZ_20_48= 1,		/*according to ena_27 field*/
	FE_361_QUARTZ_27=0
	} FE_361_Quarz_t;
	
	
	
	typedef enum
	{
		FE_361_TPS_1_2	=	0,
		FE_361_TPS_2_3	=	1,   
		FE_361_TPS_3_4	=	2,   
		FE_361_TPS_5_6	=	3,   
		FE_361_TPS_7_8	=	4   
	} FE_361_Rate_TPS_t;
	
	typedef enum
	{
		FE_361_1_2	=	0,
		FE_361_2_3	=	1,   
		FE_361_3_4	=	2,   
		FE_361_5_6	=	3,   
		FE_361_6_7	=	4,
		FE_361_7_8	=	5   
	} FE_361_Rate_t;

	

	
	
	typedef enum
	{
		FE_361_NO_FORCE =0,
		FE_361_FORCE_PR_1_2 = 1 ,
		FE_361_FORCE_PR_2_3 = 1<<1,
		FE_361_FORCE_PR_3_4 = 1<<2,
		FE_361_FORCE_PR_5_6 = 1<<3 ,
		FE_361_FORCE_PR_7_8 = 1<<4
	}FE_361_Force_PR_t ;
	


	typedef enum
	{
		NOT_FORCED_361  = 0,
		WAIT_TRL_361	= 1,
		WAIT_AGC_361	= 2,
		WAIT_SYR_361	= 3,
		WAIT_PPM_361	= 4,
		WAIT_TPS_361	= 5,
		MONITOR_TPS_361	= 6,
		RESERVED_361	= 7
	}FE_361_State_Machine_t ;
	


	
	
	/****************************************************************
						INIT STRUCTURES
	 ****************************************************************/

	typedef enum
	{								
		FE_361_PARALLEL_CLOCK,		
		FE_361_SERIAL_MASTER_CLOCK,
		FE_361_SERIAL_VCODIV6_CLOCK
	} FE_361_Clock_t;

	typedef enum
	{
		FE_361_PARITY_ON,
		FE_361_PARITY_OFF
	} FE_361_DataParity_t;

	typedef enum
	{	
		FE_361_DVB_STANDARD,				/* The FEC mode corresponds to the DVB standard	*/
		FE_361_DSS_STANDARD					/* The FEC mode corresponds to the DSS standard	*/
	} FE_361_CodingStandard_t; 

	/*
		structure passed to the FE_361_Init() function
	*/		
	typedef struct
	{

		FE_361_Clock_t			Clock;
		STTUNER_TunerType_t     Tuner;
		U32 					Frequency;    /* Windows NT specific*/
		FE_361_Quarz_t			Quartz	;
	} FE_361_InitParams_t;
	
	typedef struct
	{
		FE_361_SignalStatus_t	SignalType;		/*	Type of founded signal	*/
		STTUNER_FECRate_t		PunctureRate;	/*	Puncture rate found	*/
		double Frequency;						/*	Transponder frequency (KHz)	*/
		STTUNER_Mode_t	Mode;				/*	Mode 2K or 8K	*/
		STTUNER_Guard_t	Guard;				/*	Guard interval 	*/
		STTUNER_Modulation_t 	Modulation;		/*modulation*/
		STTUNER_Hierarchy_t		hier;
		FE_361_Rate_TPS_t		Hprate;
		FE_361_Rate_TPS_t		Lprate;
		FE_361_Rate_TPS_t		pr;
		U32 SymbolRate; 						/*	Symbol rate (Bds)	*/
	} FE_361_InternalResults_t;

	typedef struct
{
	/* hChip must be the first item in the structure !!! */
					/*	Handle to the chip	*/
	#ifndef HOST_PC
	STTUNER_IOREG_DeviceMap_t   *DeviceMap;     /* Handle to the Device   STAPI*/
	#endif
	FE_361_SignalStatus_t State;					/*	Current state of the search algorithm */
	FE_361_Quarz_t Quartz;				/*	Quartz type 20.48MHz or 27MHz */                   
	STTUNER_Mode_t	Mode;				/*	Mode 2K or 8K	*/
	STTUNER_Guard_t	Guard;				/*	Guard interval 	*/
	STTUNER_Hierarchy_t Hierarchy; /** Hierarchical Mode***/
	U32		Frequency;					/*	Current tuner frequency (KHz) */
	U8  I2Cspeed;						/*  						*/
	FE_361_OFDMEchoParams_t 	Echo;
	STTUNER_TunerType_t		Tuner;		/*  Current tuner (ALPS or TMM) */ 
	STTUNER_Spectrum_t		Inv	;		/*  0 no spectrum inverted search to be perfomed*/
	STTUNER_FreqOff_t		Offset;		/*	0 no freq offset channel search to be perfomed*/	
	U8	Delta;							/*	offset of frequency*/      	
	U8  Sense;							/*  current search,spectrum not inveerted*/     
	U8  Force;							/*  force mode/guard 					 */
	U8  ChannelBW;						/*  channel width   */
	S8  EchoPos;						/*  echo position */
	U8  first_lock;						/*				*/
	U8  prev_lock_status;				/*  verbose status of the previous lock (for scan ) */
	FE_361_InternalResults_t	Results;/*  Results of the search	*/
	BOOL  TrlNormRateFineTunning; /* To allow whether TrlNormRateFineTunning should be done or not*/
	U8  ChannelBWStatus; /* Gives which bandwidth in which the demod is working on*/
	BOOL Channel_6M_Trl_Done;
	BOOL Channel_7M_Trl_Done;
	BOOL Channel_8M_Trl_Done;
	U8   Channel_6M_Trl[3];
	U8   Channel_7M_Trl[3];
	U8   Channel_8M_Trl[3];
}FE_361_InternalParams_t;



	/****************************************************************
	        					SEARCH STRUCTURES
	 ****************************************************************/



	/************************
		SATELLITE DELIVERY
	 ************************/


/*	typedef struct
	{
		U32 Frequency;
		STTUNER_TunerType_t Tuner;
		STTUNER_Mode_t Mode;			 
		STTUNER_Guard_t Guard;
		STTUNER_FreqOff_t Offset;
		U8 Force;		
		FE_360_Spectrum	Inv;

	
	} FE_360_SearchParams_t;
*/
	typedef struct
	{
	U32                 Frequency;
	STTUNER_TunerType_t Tuner;
	STTUNER_Mode_t      Mode;
	STTUNER_Guard_t     Guard;
	STTUNER_FreqOff_t   Offset;
	STTUNER_Force_t     Force;
	STTUNER_Spectrum_t  Inv;
	STTUNER_ChannelBW_t ChannelBW;
	S8                  EchoPos;
	STTUNER_Hierarchy_t Hierarchy;
	}
	FE_361_SearchParams_t;


	/************************					 
		INFO STRUCTURE
	************************/
	typedef struct
	{
		U32 Frequency;
		U32 Agc_val;/* Agc1 on MSB */
		
		STTUNER_Mode_t 			Mode;
		STTUNER_Guard_t 		Guard;
		STTUNER_Modulation_t 	Modulation;		/*modulation*/
		STTUNER_Hierarchy_t		hier;
		STTUNER_Spectrum_t		Sense;	/*0 spectrum not inverted*/
		FE_361_Rate_TPS_t		Hprate;
		FE_361_Rate_TPS_t		Lprate;
		FE_361_Rate_TPS_t		pr;
		STTUNER_TunerType_t 	Tuner;		                         
		FE_361_State_Machine_t		State;
		S8 						Echo_pos;
		FE_361_SignalStatus_t	SignalStatus;
		STTUNER_Hierarchy_Alpha_t Hierarchy_Alpha; 
		BOOL 					Locked;
		STTUNER_Hierarchy_t Hierarchy;
		FE_361_OFDMEchoParams_t 	Echo;
	} FE_361_SearchResult_t;
	
	typedef struct
	{
	U32 Frequency ;
	FE_361_SearchResult_t Result;
	} FE_361_Scan_Result_t;
	


	/****************************************************************
						API FUNCTIONS
	****************************************************************/

	ST_Revision_t Drv0361_GetLLARevision(void);
	
	

	FE_361_Error_t	FE_361_Search(	STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_361_InternalParams_t  *pParams,
									
									FE_361_SearchResult_t    *pResult,
	
									STTUNER_tuner_instance_t *TunerInstance);
	/***Definition to be given for FE_361_SearchInit returning FE_361_Error_t instead of void for **
	 *** fix of the bug GNBvd20315 **/ 							
	
	
	FE_361_Error_t	FE_361_LookFor(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,	
									FE_361_SearchParams_t		*pParams,
									FE_361_SearchResult_t 		*pResult,
									STTUNER_tuner_instance_t 	*TunerInstance);

	
	void FE_361_Tracking(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_361_SearchResult_t  *Result);
	int SpeedInit_361(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
	void FE_361_GetNoiseEstimator(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 *pNoise, U32 *pBer);

	
	#ifdef HOST_PC 
	 /* dcdc debug only   */
	void FE_360_Core_Switch(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
	FE_360_Error_t	FE_360_TunerSet(TUNER_Handle_t Handle,FE_360_InternalParams_t pParams);

	FE_360_SignalStatus_t FE_360_Zap(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_360_SearchResult_t *pScanResult, Zap_direction_t direction, short int *ind_chan,FE_360_InternalParams_t* pParams);
	
	#endif

	
#endif
