#ifndef H_360DRV
	#define H_360DRV	
	
	#ifdef HOST_PC
	#include "gen_types.h"
	#include "tun0360.h" 	
	#include "360_util.h"
	#else
	#include "stddefs.h"
	#include "dbtypes.h"
	#include "sttuner.h"
#endif


	typedef int FE_360_Handle_t;
	
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
		NOAGC=0,
		AGC1SATURATION=1,
		AGC1OK=2,
		
		TIMINGOK=3,
		NOAGC2=4,
		AGC2OK=5,
		NOTPS=6,
		TPSOK=7,
		NOSYMBOL=8,
		BAD_CPQ=9,
		NODATA=10,
		NOPRFOUND=11,
		LOCK_OK=12,
		LOCK_KO=13,
		RANGEOK=14
	} FE_360_SignalStatus_t;
	
	
	
	typedef enum
	{
	UP   =1,
	DOWN = -1
	}Zap_direction_t;
	
	typedef enum
	{
	    FE_NO_ERROR,
	    FE_INVALID_HANDLE,
	    FE_BAD_PARAMETER,
	    FE_MISSING_PARAMETER,
	    FE_ALREADY_INITIALIZED,
	    FE_I2C_ERROR,
	    FE_SEARCH_FAILED,
	    FE_TRACKING_FAILED,
	    FE_TERM_FAILED,
	    FE_TIMEOUT_ERROR
	} FE_360_Error_t;
	
	/* type of modulation (common) */
	typedef enum
	{				   
	FE_QUARTZ_20_48= 1,		/*according to ena_27 field*/
	FE_QUARTZ_27=0
	} FE_360_Quarz_t;
	
	
	
	typedef enum
	{
		FE_TPS_1_2	=	0,
		FE_TPS_2_3	=	1,   
		FE_TPS_3_4	=	2,   
		FE_TPS_5_6	=	3,   
		FE_TPS_7_8	=	4   
	} FE_360_Rate_TPS_t;
	
	typedef enum
	{
		FE_1_2	=	0,
		FE_2_3	=	1,   
		FE_3_4	=	2,   
		FE_5_6	=	3,   
		FE_6_7	=	4,
		FE_7_8	=	5   
	} FE_360_Rate_t;

	

	
	
	typedef enum
	{
		FE_NO_FORCE =0,
		FE_FORCE_PR_1_2 = 1 ,
		FE_FORCE_PR_2_3 = 1<<1,
		FE_FORCE_PR_3_4 = 1<<2,
		FE_FORCE_PR_5_6 = 1<<3 ,
		FE_FORCE_PR_7_8 = 1<<4
	}FE_360_Force_PR_t ;
	


	typedef enum
	{
		NOT_FORCED  = 0,
		WAIT_TRL	= 1,
		WAIT_AGC	= 2,
		WAIT_SYR	= 3,
		WAIT_PPM	= 4,
		WAIT_TPS	= 5,
		MONITOR_TPS	= 6,
		RESERVED	= 7
	}FE_State_Machine_t ;
	


	
	
	/****************************************************************
						INIT STRUCTURES
	 ****************************************************************/

	typedef enum
	{								
		FE_PARALLEL_CLOCK,		
		FE_SERIAL_MASTER_CLOCK,
		FE_SERIAL_VCODIV6_CLOCK
	} FE_360_Clock_t;

	typedef enum
	{
		FE_PARITY_ON,
		FE_PARITY_OFF
	} FE_360_DataParity_t;

	typedef enum
	{	
		FE_DVB_STANDARD,				/* The FEC mode corresponds to the DVB standard	*/
		FE_DSS_STANDARD					/* The FEC mode corresponds to the DSS standard	*/
	} FE_360_CodingStandard_t; 

	/*
		structure passed to the FE_360_Init() function
	*/	

	typedef struct
	{
		
		FE_360_Clock_t			Clock;
		STTUNER_TunerType_t     Tuner;
		U32 					Frequency;    /* Windows NT specific*/
		FE_360_Quarz_t			Quartz	;
	} FE_360_InitParams_t;

	
	typedef struct
	{
		FE_360_SignalStatus_t	SignalType;		/*	Type of founded signal	*/
		STTUNER_FECRate_t		PunctureRate;	/*	Puncture rate found	*/
		double Frequency;						/*	Transponder frequency (KHz)	*/
		STTUNER_Mode_t	Mode;				/*	Mode 2K or 8K	*/
		STTUNER_Guard_t	Guard;				/*	Guard interval 	*/
		STTUNER_Modulation_t 	Modulation;		/*modulation*/
		STTUNER_Hierarchy_t		hier;
		FE_360_Rate_TPS_t		Hprate;
		FE_360_Rate_TPS_t		Lprate;
		FE_360_Rate_TPS_t		pr;
		U32 SymbolRate; 						/*	Symbol rate (Bds)	*/
	} FE_360_InternalResults_t;

	typedef struct
{
	/* hChip must be the first item in the structure !!! */
#ifndef STTUNER_MINIDRIVER
					
	#ifndef HOST_PC
	STTUNER_IOREG_DeviceMap_t   *DeviceMap;     /* Handle to the Device   STAPI*/
	#endif
#endif
	FE_360_SignalStatus_t State;					/*	Current state of the search algorithm */
	FE_360_Quarz_t Quartz;				/*	Quartz type 20.48MHz or 27MHz */                   
	STTUNER_Mode_t	Mode;				/*	Mode 2K or 8K	*/
	STTUNER_Guard_t	Guard;				/*	Guard interval 	*/
	STTUNER_Hierarchy_t Hierarchy; /** Hierarchical Mode***/
	U32		Frequency;					/*	Current tuner frequency (KHz) */
	U8  I2Cspeed;						/*  						*/
	FE_OFDMEchoParams_t 	Echo;	
	STTUNER_TunerType_t		Tuner;	/**/	/*  check nab Current tuner (ALPS or TMM) */ 
	STTUNER_Spectrum_t		Inv	;		/*  0 no spectrum inverted search to be perfomed*/
	STTUNER_FreqOff_t		Offset;		/*	0 no freq offset channel search to be perfomed*/	
	U8	Delta;							/*	offset of frequency*/      	
	U8  Sense;							/*  current search,spectrum not inveerted*/     
	U8  Force;							/*  force mode/guard 					 */
	U8  ChannelBW;						/*  channel width   */
	S8  EchoPos;						/*  echo position */
	U8  first_lock;						/*				*/
	U8  prev_lock_status;				/*  verbose status of the previous lock (for scan ) */
	FE_360_InternalResults_t	Results;/*  Results of the search	*/
	BOOL  TrlNormRateTunning; /* To allow whether TrlNormRateTunning should be done or not*/
	BOOL  TrlNormRateFineTunning; /* To allow whether TrlNormRateFineTunning should be done or not*/
	U8  ChannelBWStatus; /* Gives which bandwidth in which the demod is working on*/
	BOOL Channel_6M_Trl_Done;
	BOOL Channel_7M_Trl_Done;
	BOOL Channel_8M_Trl_Done;
	U8   Channel_6M_Trl[3];
	U8   Channel_7M_Trl[3];
	U8   Channel_8M_Trl[3];
	
	
} FE_360_InternalParams_t;



	/****************************************************************
	        					SEARCH STRUCTURES
	 ****************************************************************/



	/************************
		SATELLITE DELIVERY
	 ************************/



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
	} FE_360_SearchParams_t;


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
		FE_360_Rate_TPS_t		Hprate;
		FE_360_Rate_TPS_t		Lprate;
		FE_360_Rate_TPS_t		pr;
		STTUNER_TunerType_t 	Tuner;		                         
		FE_State_Machine_t		State;
		S8 						Echo_pos;
		FE_360_SignalStatus_t	SignalStatus;
		STTUNER_Hierarchy_Alpha_t Hierarchy_Alpha; 
		BOOL 					Locked;
		FE_OFDMEchoParams_t 	Echo;
	} FE_360_SearchResult_t;
	
	typedef struct
	{
	U32 Frequency ;
	FE_360_SearchResult_t Result;
	} FE_360_Scan_Result_t;
	
        typedef struct
        {
	    int lsb;
	    int msb; 
	
        } Cell_Id_Value;

	/****************************************************************
						API FUNCTIONS
	****************************************************************/
#ifndef  STTUNER_MINIDRIVER
	ST_Revision_t Drv0360_GetLLARevision(void);



	void FE_360_GetNoiseEstimator(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 *pNoise, U32 *pBer);

	FE_360_SignalStatus_t FE_360_TRLNOMRATE_Tuning(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
	ST_ErrorCode_t FE_360_GETCELLID(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,Cell_Id_Value * cell_id );
    void FE_360_Core_Switch(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
    int SpeedInit(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
#endif

#ifdef  STTUNER_MINIDRIVER
FE_360_Handle_t	FE_360_Init(void);
void	FE_360_Term(void);
void FE_360_GetNoiseEstimator( U32 *pNoise, U32 *pBer);

	FE_360_SignalStatus_t FE_360_TRLNOMRATE_Tuning(void);
    void FE_360_Core_Switch(void );
    int SpeedInit(void);
#endif
	
	
	FE_360_Error_t	FE_360_Search(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,	FE_360_InternalParams_t  *pParams,
									
									FE_360_SearchResult_t    *pResult,
									STTUNER_tuner_instance_t *TunerInstance);
	/***Definition to be given for FE_360_SearchInit returning FE_360_Error_t instead of void for **
	 *** fix of the bug GNBvd20315 **/ 							
								
	
	FE_360_Error_t	FE_360_LookFor(	STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,
									FE_360_SearchParams_t		*pParams,
									FE_360_SearchResult_t 		*pResult,
									STTUNER_tuner_instance_t 	*TunerInstance);

	void FE_360_Tracking(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,FE_360_SearchResult_t  *Result);
	

	FE_360_Error_t	FE_360_TunerSet(TUNER_Handle_t Handle,FE_360_InternalParams_t* pParams);
	

	
#endif
