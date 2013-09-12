#ifndef H_362DRV
	#define H_362DRV

	#include "stddefs.h"
	#include "dbtypes.h"
	#include "sttuner.h"



	typedef int FE_362_Handle_t;

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

	#define NINV 0

	#define INV 1


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
		NOAGC_362=0,
		AGCOK_362=5,
		NOTPS_362=6,
		TPSOK_362=7,
		NOSYMBOL_362=8,
		BAD_CPQ_362=9,
		PRFOUNDOK_362=10,
		NOPRFOUND_362=11,
		LOCKOK_362=12,
		NOLOCK_362=13,
		SYMBOLOK_362=15,
		CPAMPOK_362=16,
		NOCPAMP_362=17,
		SWNOK_362=18
	} FE_362_SignalStatus_t;






	typedef enum
	{
	    FE_362_NO_ERROR,
	    FE_362_INVALID_HANDLE,
	    FE_362_BAD_PARAMETER,
	    FE_362_MISSING_PARAMETER,
	    FE_362_ALREADY_INITIALIZED,
	    FE_362_I2C_ERROR,
	    FE_362_SEARCH_FAILED,
	    FE_362_TRACKING_FAILED,
	    FE_362_TERM_FAILED
	} FE_362_Error_t;

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
STTUNER_MODE_8K,
STTUNER_MODE_4K
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
		FE_362_TPS_1_2	=	0,
		FE_362_TPS_2_3	=	1,
		FE_362_TPS_3_4	=	2,
		FE_362_TPS_5_6	=	3,
		FE_362_TPS_7_8	=	4
	} FE_362_Rate_TPS_t;

	typedef enum
	{
		FE_362_1_2	=	0,
		FE_362_2_3	=	1,
		FE_362_3_4	=	2,
		FE_362_5_6	=	3,
		FE_362_6_7	=	4,
		FE_362_7_8	=	5
	} FE_362_Rate_t;





	typedef enum
	{
		FE_362_NO_FORCE =0,
		FE_362_FORCE_PR_1_2 = 1 ,
		FE_362_FORCE_PR_2_3 = 1<<1,
		FE_362_FORCE_PR_3_4 = 1<<2,
		FE_362_FORCE_PR_5_6 = 1<<3 ,
		FE_362_FORCE_PR_7_8 = 1<<4
	}FE_362_Force_PR_t ;



	typedef enum
	{
		NOT_FORCED_362  = 0,
		WAIT_TRL_362	= 1,
		WAIT_AGC_362	= 2,
		WAIT_SYR_362	= 3,
		WAIT_PPM_362	= 4,
		WAIT_TPS_362	= 5,
		MONITOR_TPS_362	= 6,
		RESERVED_362	= 7
	}FE_362_State_Machine_t ;






	/****************************************************************
						INIT STRUCTURES
	 ****************************************************************/

	typedef enum
	{
		FE_362_PARALLEL_CLOCK,
		FE_362_SERIAL_MASTER_CLOCK,
		FE_362_SERIAL_VCODIV6_CLOCK
	} FE_362_Clock_t;

	typedef enum
	{
		FE_362_PARITY_ON,
		FE_362_PARITY_OFF
	} FE_362_DataParity_t;

	typedef enum
	{
		FE_362_DVB_STANDARD,				/* The FEC mode corresponds to the DVB standard	*/
		FE_362_DSS_STANDARD					/* The FEC mode corresponds to the DSS standard	*/
	} FE_362_CodingStandard_t;

	/*
		structure passed to the FE_362_Init() function
	*/
	typedef struct
	{
		FE_362_SignalStatus_t	SignalType;		/*	Type of founded signal	*/
		STTUNER_FECRate_t		PunctureRate;	/*	Puncture rate found	*/
		double Frequency;						/*	Why double ?Transponder frequency (KHz)	*/
		STTUNER_Mode_t	Mode;				/*	Mode 2K or 8K	*/
		STTUNER_Guard_t	Guard;				/*	Guard interval 	*/
		STTUNER_Modulation_t 	Modulation;		/*modulation*/
		STTUNER_Hierarchy_t		hier;
		FE_362_Rate_TPS_t		Hprate;
		FE_362_Rate_TPS_t		Lprate;
		FE_362_Rate_TPS_t		pr;
		U32 SymbolRate; 						/*	Symbol rate (Bds)	*/
	} FE_362_InternalResults_t;

	typedef struct
{
	#ifndef HOST_PC
	STTUNER_IOREG_DeviceMap_t   *DeviceMap;     /* Handle to the Device   STAPI*/
	#endif
	FE_362_SignalStatus_t State;					/*	Current state of the search algorithm */
	STTUNER_IF_IQ_Mode		IF_IQ_Mode;
	STTUNER_Mode_t	Mode;				/*	Mode 2K or 8K	*/
	STTUNER_Guard_t	Guard;				/*	Guard interval 	*/
	STTUNER_Hierarchy_t Hierarchy; /** Hierarchical Mode***/
	U32		Frequency;					/*	Current tuner frequency (KHz) */
	U8  I2Cspeed;						/*  						*/
	FE_362_OFDMEchoParams_t 	Echo;
	STTUNER_Spectrum_t		Inv	;		/*  0 no spectrum inverted search to be perfomed*/
	STTUNER_FreqOff_t		Offset;		/*	0 no freq offset channel search to be perfomed*/
	U8	Delta;							/*	offset of frequency*/
	U8  Sense;							/*  current search,spectrum not inveerted*/
	U8  Force;							/*  force mode/guard 					 */
	U8  ChannelBW;						/*  channel width   */
	S8  EchoPos;						/*  echo position */
	U8  first_lock;						/*				*/
	U8  prev_lock_status;				/*  verbose status of the previous lock (for scan ) */
	FE_362_InternalResults_t	Results;/*  Results of the search	*/
}FE_362_InternalParams_t;



	/****************************************************************
	        					SEARCH STRUCTURES
	 ****************************************************************/





	typedef struct
	{
	U32                 Frequency;
        STTUNER_IF_IQ_Mode   IF_IQ_Mode;
	STTUNER_Mode_t      Mode;
	STTUNER_Guard_t     Guard;
	STTUNER_FreqOff_t   Offset;
	STTUNER_Force_t     Force;
	STTUNER_Spectrum_t  Inv;
	STTUNER_ChannelBW_t ChannelBW;
	S8                  EchoPos;
	STTUNER_Hierarchy_t Hierarchy;
	}
	FE_362_SearchParams_t;


	/************************
		INFO STRUCTURE
	************************/
	typedef struct
	{
		U32 Frequency;
		U32 Agc_val;/* Agc1 on MSB */
		S32 offset;
		S32 offset_type;

		STTUNER_Mode_t 			Mode;
		STTUNER_Guard_t 		Guard;
		STTUNER_Modulation_t 	Modulation;		/*modulation*/
		STTUNER_Hierarchy_t		hier;
		STTUNER_Spectrum_t		Sense;	/*0 spectrum not inverted*/
		FE_362_Rate_TPS_t		Hprate;
		FE_362_Rate_TPS_t		Lprate;
		FE_362_Rate_TPS_t		pr;

		FE_362_State_Machine_t		State;
		S8 						Echo_pos;
		FE_362_SignalStatus_t	SignalStatus;
		STTUNER_Hierarchy_Alpha_t Hierarchy_Alpha;
		BOOL 					Locked;
	} FE_362_SearchResult_t;

	typedef struct
	{
	U32 Frequency ;
	FE_362_SearchResult_t Result;
	} FE_362_Scan_Result_t;



	/****************************************************************
						API FUNCTIONS
	****************************************************************/

	ST_Revision_t Drv0362_GetLLARevision(void);
	int FE_362_Pow(int number1,int number2);

	FE_362_Error_t	FE_362_Search(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle, FE_362_SearchParams_t	*pSearch,FE_362_InternalParams_t *Params, FE_362_SearchResult_t *pResult, STTUNER_Handle_t  TopLevelHandle);
	void FE_362_FilterCoeffInit( IOARCH_Handle_t IOHandle,U16 CellsCoeffs[][5]);
	/***Definition to be given for FE_362_SearchInit returning FE_362_Error_t instead of void for **
	 *** fix of the bug GNBvd20315 **/

	void FE_362_GetNoiseEstimator( IOARCH_Handle_t IOHandle, U32 *pNoise, U32 *pBer);
	FE_362_Error_t	FE_362_LookFor(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle, FE_362_SearchParams_t	*pSearch, FE_362_SearchResult_t *pResult,STTUNER_Handle_t  TopLevelHandle);
	void FE_362_Tracking( IOARCH_Handle_t IOHandle,U32 *UnlockCounter);
	FE_362_SignalStatus_t CheckCPAMP_362( IOARCH_Handle_t IOHandle,S32 FFTmode );
	FE_362_SignalStatus_t CheckSYR_362( IOARCH_Handle_t IOHandle);
    S32  FE_362_PowOf2(int number);
    S16 duration( S32 mode, int tempo1,int tempo2,int tempo3);



#endif
