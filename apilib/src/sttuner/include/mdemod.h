/* ----------------------------------------------------------------------------
File Name: mdemod.h

Description:

    demdo driver,instance and API structure.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 6-June-2001
version: 3.1.0
 author: GJP from work by LW
comment:

   date: 17-August-2001
version: 3.1.1
 author: GJP
comment: update SubAddr to U16

   date: 22-Oct-2001
version: 3.1.1
 author: GJP
comment: Added new API function: SetModulation

Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_MDEMOD_H
#define __STTUNER_MDEMOD_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

#include "stddefs.h"            /* Standard definitions */
#include "sttuner.h"
#include "ioarch.h"
#include "ioreg.h"
#ifdef STTUNER_USE_SAT
#include "diseqc.h"		/*added for DiSEqC API support*/
#endif
/* ----------------------------------------------------------------------- */

     /* support type: demod capability - not for all devices! */
     typedef struct
     {
        U16                    FECAvail;
        STTUNER_Modulation_t   ModulationAvail;
        STTUNER_J83_t          J83Avail;
        BOOL                   AGCControl;
        U32                    SymbolMin;
        U32                    SymbolMax;

        U32  BerMax;
        U32  SignalQualityMax;
        U32  AgcMax;
        U32  FreqMin;
        U32  FreqMax;
     } DEMOD_Capability_t;


    /* support type: demod initialization paramaters */
    typedef struct
    {
        /* handle to IOFunction (e.g. STTUNER_IOARCH_ReadWrite or rptr/passthru) */
        IOARCH_Handle_t  IOHandle;
        ST_Partition_t  *MemoryPartition;

        U32                         ExternalClock;  /* External VCO */
        STTUNER_TSOutputMode_t      TSOutputMode;
        STTUNER_SerialDataMode_t    SerialDataMode;
        STTUNER_BlockSyncMode_t     BlockSyncMode;    /* add block sync bit control for bug GNBvd27452*/
        STTUNER_DataFIFOMode_t      DataFIFOMode;     /* add block sync bit control for bug GNBvd27452*/
        STTUNER_OutputFIFOConfig_t  OutputFIFOConfig; /* add block sync bit control for bug GNBvd27452*/
        STTUNER_SerialClockSource_t SerialClockSource;
        STTUNER_FECMode_t           FECMode;
        STTUNER_Sti5518_t           Sti5518;         /* add 5518 specific TS output mode-> bug GNBvd27452*/
        STTUNER_DataClockPolarity_t ClockPolarity;
        #ifdef STTUNER_USE_CAB
        STTUNER_SyncStrip_t         SyncStripping;
        #endif
        STTUNER_DataClockAtParityBytes_t DataClockAtParityBytes;
    	STTUNER_TSDataOutputControl_t    TSDataOutputControl;
    	STTUNER_DEMOD_t 		Path;

    } DEMOD_InitParams_t;


    /* support type: demod initialization paramaters */
    typedef struct
    {
        BOOL ForceTerminate;        /* force driver termination (don't wait) */
    } DEMOD_TermParams_t;


    /* support type: demod open paramaters */
    typedef struct
    {
        STTUNER_Handle_t TopLevelHandle;
    } DEMOD_OpenParams_t;


    /* support type: demod initialization paramaters */
    typedef struct
    {
        STTUNER_Da_Status_t Status;
    } DEMOD_CloseParams_t;


    typedef U32 DEMOD_Handle_t; /* handle TO demod driver for open */


    /* we have one of these for every TYPE of driver in the system */
    typedef struct
    {
        STTUNER_DemodType_t ID;  /* enumerated hardware ID */

        /* ---------- API to demod driver ---------- */

        /* global for the driver */
        ST_ErrorCode_t (*demod_Init)(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams); /* done for every driver (299, 399 etc) */
        ST_ErrorCode_t (*demod_Term)(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams);

        /* repeater/passthrough port for other drivers to use, type: STTUNER_IOARCH_RedirFn_t */
        ST_ErrorCode_t (*demod_ioaccess)(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);
        /* repeater function used in chip functions */
        ST_ErrorCode_t (*demod_repeateraccess)(DEMOD_Handle_t Handle, BOOL REPEATER_STATUS);

        /* access device specific low-level functions */
        ST_ErrorCode_t (*demod_ioctl)(DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status);

        /* instance (was init(), now called Open() )*/
        ST_ErrorCode_t (*demod_Open) (ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle);
        ST_ErrorCode_t (*demod_Close)(DEMOD_Handle_t  Handle, DEMOD_CloseParams_t *CloseParams);
        ST_ErrorCode_t (*demod_GetTunerInfo)    (DEMOD_Handle_t Handle, STTUNER_TunerInfo_t *TunerInfo_p);

        ST_ErrorCode_t (*demod_IsAnalogCarrier) (DEMOD_Handle_t Handle, BOOL *IsAnalog);        
        ST_ErrorCode_t (*demod_GetSignalQuality)(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber);
        ST_ErrorCode_t (*demod_StandByMode)	(DEMOD_Handle_t Handle,STTUNER_StandByMode_t PowerMode );
	
        ST_ErrorCode_t (*demod_GetModulation)   (DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation);
        ST_ErrorCode_t (*demod_SetModulation)   (DEMOD_Handle_t Handle, STTUNER_Modulation_t  Modulation);  /* added after 3.2.0 release */
        ST_ErrorCode_t (*demod_GetMode)         (DEMOD_Handle_t Handle, STTUNER_Mode_t *Mode);              /* added after 3.2.0 release */
        ST_ErrorCode_t (*demod_GetModeCode)         (DEMOD_Handle_t Handle, STTUNER_ModeCode_t *ModeCode);
        ST_ErrorCode_t (*demod_GetGuard)        (DEMOD_Handle_t Handle, STTUNER_Guard_t  *Guard);           /* added after 3.2.0 release */
        ST_ErrorCode_t (*demod_GetAGC)          (DEMOD_Handle_t Handle, S16  *Agc);
        ST_ErrorCode_t (*demod_GetFECRates)     (DEMOD_Handle_t Handle, STTUNER_FECRate_t *FECRates);
        ST_ErrorCode_t (*demod_GetIQMode)       (DEMOD_Handle_t Handle, STTUNER_IQMode_t *IQMode); /*added for GNBvd26107->I2C failure due to direct access to demod device at API level*/
        ST_ErrorCode_t (*demod_IsLocked)        (DEMOD_Handle_t Handle, BOOL *IsLocked);
        ST_ErrorCode_t (*demod_SetFECRates)     (DEMOD_Handle_t Handle, STTUNER_FECRate_t FECRates);
        ST_ErrorCode_t (*demod_Tracking)        (DEMOD_Handle_t Handle, BOOL ForceTracking,    U32  *NewFrequency,  BOOL *SignalFound);
 	ST_ErrorCode_t (*demod_GetSymbolrate)(DEMOD_Handle_t Handle,   U32  *SymbolRate);
        /* changed for 3.2.0 */
        ST_ErrorCode_t (*demod_ScanFrequency)   (DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset, 
                                                                        U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                        U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                        U32  Force,            U32   Hierarchy,     U32 Spectrum,
                                                                        U32  FreqOff,          U32   ChannelBW,     S32   EchoPos);
        #ifdef STTUNER_USE_SAT 
        /*added for DiSEqc API support*/ 
        ST_ErrorCode_t (* demod_DiSEqC )	(DEMOD_Handle_t Handle, 
												 STTUNER_DiSEqCSendPacket_t *pDiSEqCSendPacket,
												 STTUNER_DiSEqCResponsePacket_t *pDiSEqCResponsePacket
												   );
	    ST_ErrorCode_t (* demod_tonedetection)(DEMOD_Handle_t Handle,U32 StartFreq, U32 StopFreq, U8  *NbTones,U32 *ToneList, U8 mode, int* power_detection_level);
        ST_ErrorCode_t (* demod_GetConfigDiSEqC) ( DEMOD_Handle_t Handle, STTUNER_DiSEqCConfig_t *DiSEqCConfig);

        ST_ErrorCode_t (* demod_SetDiSEqCBurstOFF) ( DEMOD_Handle_t Handle);
        /*************************************************/
        #endif	
        ST_ErrorCode_t (*demod_GetRFLevel)	(DEMOD_Handle_t Handle, S32  *Rflevel);
        #if defined(STTUNER_USE_TER) || defined(STTUNER_USE_SAT) || defined(STTUNER_USE_ATSC_VSB)
        ST_ErrorCode_t (*demod_GetTPSCellId)	(DEMOD_Handle_t Handle, U16  *TPSCellID);
       
        #endif

        } STTUNER_demod_dbase_t;


    /* we have one of these for every INSTANCE of driver in the system */
    typedef struct
    {
        ST_ErrorCode_t          Error;     /* last reported driver error  */
        STTUNER_demod_dbase_t  *Driver;    /* which driver.               */
        IOARCH_Handle_t         IOHandle;  /* IO handle                   */
        DEMOD_Handle_t          DrvHandle; /* handle to instance of driver*/
        STTUNER_IOREG_DeviceMap_t *DeviceMap;
        U32                         TEN_FieldIndex;
        U32                         VSEL_FieldIndex;
        U32                         VEN_FieldIndex;
    } STTUNER_demod_instance_t;

#ifdef STTUNER_USE_TER 
typedef struct
    {
    	U8	RPLLDIV; /*PLLNDIV register value*/
    	U8      TRLNORMRATELSB; /*TRL Normrate registers value*/
	U8	TRLNORMRATELO; 
	U8	TRLNORMRATEHI; 
	U8 	INCDEROT1;/*INC derotator register value*/
	U8 	INCDEROT2;
    	
    	}STTUNER_demod_IOCTL_30MHZ_REG_t  ;

typedef struct
    {
    	
    	U8      TRLNORMRATELSB; /*TRL Normrate registers value*/
	U8	TRLNORMRATELO; 
	U8	TRLNORMRATEHI; 
	    	
    	}STTUNER_demod_TRL_IOCTL_t  ;
#endif

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_MDEMOD_H */


/* End of mdemod.h */
