/* ----------------------------------------------------------------------------
File Name: d0299.h

Description: 

    stv0299 demod driver.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 19-June-2001
version: 3.1.0
 author: GJP from work by LW
comment: 
    
Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SAT_D0299_H
#define __STTUNER_SAT_D0299_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

/* Standard definitions */
#include "stddefs.h"

/* internal types */
#include "dbtypes.h"    /* types of satellite driver database */


/* defines ----------------------------------------------------------------- */

/* STV0299 common definitions */
#define DEF0299_SCAN 	0
#define DEF0299_CHANNEL	1

/**/
#define DEF0299_STV0199_4MHZ 1

/**/
#define DEF0299_NBPARAM      50
#define	DEF0299_NBREG		 65
#define DEF0299_NBFIELD		113
/**/
#define	DEF0299_SET			0
#define	DEF0299_GET			1
#define	DEF0299_NOCHANGE    0
#define	DEF0299_END			0
#define	DEF0299_ON		    1
#define	DEF0299_OFF			0

#define DEF0299_UNSIGNED 	0
#define DEF0299_SIGNED		1

/* MACRO definitions */
#define MAC0299_ABS(X)   ((X)<0 ?   (-X) : (X))
#define MAC0299_MAX(X,Y) ((X)>=(Y) ? (X) : (Y))
#define MAC0299_MIN(X,Y) ((X)<=(Y) ? (X) : (Y)) 
#define MAC0299_MAKEWORD(X,Y) ((X<<8)+(Y))
#define MAC0299_LSB(X) ((X & 0xFF))
#define MAC0299_MSB(Y) ((Y>>8)& 0xFF)
#define MAC0299_INRANGE(X,Y,Z) (((X<=Y) && (Y<=Z))||((Z<=Y) && (Y<=X)) ? 1 : 0)


/* Maximum signal quality */
#define STV0299_MAX_SIGNAL_QUALITY     100

/* REGISTER BIT ASSIGNMENTS */

/* STV0299 Register VSTATUS */
#define STV0299_VSTATUS_PR_1_2  (4)     /* Basic 1/2 */
#define STV0299_VSTATUS_PR_2_3  (0)     /* Punctured 2/3 */
#define STV0299_VSTATUS_PR_3_4  (1)     /* Punctured 3/4 */
#define STV0299_VSTATUS_PR_5_6  (2)     /* Punctured 5/6 */
#define STV0299_VSTATUS_PR_7_8  (3)     /* Punctured 7/8 (Mode A) or 6/7 (Mode B) */

/* STV0299 Register VENRATE (0x09) Puncture rate enable */
#define STV0299_VENRATE_E0_MSK  (0x01)  /* Enable Basic Puncture Rate 1/2 */
#define STV0299_VENRATE_E1_MSK  (0x02)  /* Enable Puncture Rate 2/3 */
#define STV0299_VENRATE_E2_MSK  (0x04)  /* Enable Puncture Rate 3/4 */
#define STV0299_VENRATE_E3_MSK  (0x08)  /* Enable Puncture Rate 5/6 */
#define STV0299_VENRATE_E4_MSK  (0x10)  /* Enable Puncture Rate 6/7 Mode B
                                           or 7/8 Mode A */


/* enumerated types -------------------------------------------------------- */

typedef enum
{
    E299_BWTOONARROW = 0,
    E299_BWOK,
    E299_NOAGC1,
    E299_AGC1SATURATION,
    E299_AGC1OK,
    E299_NOTIMING,
    E299_ANALOGCARRIER,
    E299_TIMINGOK,
    E299_NOAGC2,
    E299_AGC2OK,
    E299_NOCARRIER,
    E299_CARRIEROK,
    E299_NODATA,
    E299_FALSELOCK,
    E299_DATAOK,
    E299_OUTOFRANGE,
    E299_RANGEOK
} 
D0299_SignalType_t;


/* Puncture Rate enum */
typedef enum
{	
    E299_PR2_3 = 0,
    E299_PR3_4,
    E299_PR5_6,
    E299_PR7_8,
    E299_PR1_2,
    E299_PRUNKNOWN
}
D0299_Puncturerate_t;


/*	Scan direction enum	*/
typedef enum 
{
    E299_SCANUP   =  1,
    E299_SCANDOWN = -1
}
D0299_Scandir_t; 


/* driver internal types --------------------------------------------------- */

/*	Search result structure	*/    
typedef struct
{
    D0299_SignalType_t    SignalType;
    LNB_Polarization_t    Polarization; /* LNB, Polarization element not referenced in demod */
    D0299_Puncturerate_t  PunctureRate;  
    long Frequency;
    long SymbolRate; 
} D0299_Searchresult_t;


/* AGC2 gain and frequency structure */
typedef struct
{
    int  NbPoints;
    int  Gain[3];
    long Frequency[3];
} D0299_Agc2TimingOK_t;	


/*	Search parameters structure	*/         
typedef struct
{
    D0299_SignalType_t State;

    long Frequency;
    long BaseFreq;
    long SymbolRate;
    long MasterClock;
    long Mclk;
    long RollOff;
    long SearchRange;
    long SubRange;

    long TunerStep;
    long TunerOffset;
    long TunerBW;
    long TunerIF;
    long TunerIQSense;

    STTUNER_IQMode_t DemodIQMode;

    short int DerotFreq;
    short int DerotPercent;
    short int DerotStep;
    short int Direction;
    short int Tagc1;
    short int Tagc2;
    short int Ttiming;
    short int Tderot;
    short int Tdata;
    short int SubDir;
       
    BOOL TestOn;
    long FreqOffset;
    
} D0299_SearchParams_t;


typedef struct
{
    int RegCarrierOffset;
    int RegTimingOffset;
    int lastAGC2Coef;
    int ScanMode;

    D0299_SearchParams_t Params;
    D0299_Searchresult_t Result;

} D0299_StateBlock_t;


/* macros ------------------------------------------------------------------ */

/* Delay calling task for a period of microseconds */

#define STV0299_Delay(micro_sec) STOS_TaskDelayUs(micro_sec)
                                 
                               

/* functions --------------------------------------------------------------- */

/* install into demod database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0299_Install(STTUNER_demod_dbase_t *Demod);

/* remove from database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0299_UnInstall(STTUNER_demod_dbase_t *Demod);
#ifdef STTUNER_MINIDRIVER
ST_ErrorCode_t demod_d0299_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams);
ST_ErrorCode_t demod_d0299_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams);

ST_ErrorCode_t demod_d0299_Open (ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle);
ST_ErrorCode_t demod_d0299_Close(DEMOD_Handle_t  Handle, DEMOD_CloseParams_t *CloseParams);

ST_ErrorCode_t demod_d0299_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber);
ST_ErrorCode_t demod_d0299_GetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t    *FECRates);
ST_ErrorCode_t demod_d0299_GetIQMode       (DEMOD_Handle_t Handle, STTUNER_IQMode_t     *IQMode); /*added for GNBvd26107->I2C failure due to direct access to demod device at API level*/
ST_ErrorCode_t demod_d0299_IsLocked        (DEMOD_Handle_t Handle, BOOL                 *IsLocked);
ST_ErrorCode_t demod_d0299_SetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t     FECRates);
ST_ErrorCode_t demod_d0299_Tracking        (DEMOD_Handle_t Handle, BOOL ForceTracking,   U32 *NewFrequency, BOOL *SignalFound);

ST_ErrorCode_t demod_d0299_ScanFrequency   (DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset, 
                                                                   U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                   U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                   U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                   U32  FreqOff,          U32   ChannelBW,     S32   EchoPos);

/* added for DiSEqC API support*/
ST_ErrorCode_t demod_d0299_DiSEqC       (	DEMOD_Handle_t Handle, 
									    STTUNER_DiSEqCSendPacket_t *pDiSEqCSendPacket,
									    STTUNER_DiSEqCResponsePacket_t *pDiSEqCResponsePacket
									   );

ST_ErrorCode_t demod_d0299_DiSEqCGetConfig ( DEMOD_Handle_t Handle ,STTUNER_DiSEqCConfig_t * DiSEqCConfig);
ST_ErrorCode_t demod_d0299_DiSEqCBurstOFF ( DEMOD_Handle_t Handle );

#endif


#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SAT_D0299_H */


/* End of d0299.h */
