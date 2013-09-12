
/* ----------------------------------------------------------------------------
File Name: mlnb.h

Description: 

    LNB driver, instance and API structure.

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

Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SAT_MLNB_H
#define __STTUNER_SAT_MLNB_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

#include "stddefs.h"            /* Standard definitions */
#include "sttuner.h"
#include "ioarch.h"
#include "dastatus.h"

    
/* -------------------------------------------------------------------------
    STTUNER_lnb_dbase_t
   ------------------------------------------------------------------------- */

    /* Polarization (internal) */
    typedef enum 
    {
        NOPOLARIZATION = 0,
        HORIZONTAL,
        VERTICAL,
        H_V,
        V_H
    } LNB_Polarization_t;

/* Lnb Current Threshold Selection in LNB21*/
typedef enum 
  {
  	CURRENT_THRESHOLD_HIGH = 0,
  	CURRENT_THRESHOLD_LOW
  } LNB_CurrentThresholdSelection_t;
  
  /* Short Circuit Protection Mode */
  typedef enum 
  {
  	LNB_PROTECTION_DYNAMIC = 0,
  	LNB_PROTECTION_STATIC
  } LNB_ShortCircuitProtectionMode_t;
  
    /* support type: LNB status */
    

    typedef enum
    {
        LNB_STATUS_ON,
        LNB_STATUS_OFF,
        LNB_STATUS_SHORT_CIRCUIT,
        LNB_STATUS_OVER_TEMPERATURE,  /*Supported by LNB21 -> OTF=1 when Temp >140*/
        LNB_STATUS_SHORTCIRCUIT_OVERTEMPERATURE   
} LNB_Status_t;

    /* support type: LNB open/ returned capability */
    typedef struct
    {
        STTUNER_Handle_t TopLevelHandle;
    } LNB_OpenParams_t;


    typedef struct
    {
        BOOL                    ShortCircuitDetect;
        BOOL                    PowerAvailable;
        STTUNER_Polarization_t  PolarizationSelect;
    } LNB_Capability_t;


    /* support type: LNB configuration */
    typedef struct
    {
        LNB_Status_t            Status;
        STTUNER_Handle_t        TopLevelHandle;
        STTUNER_Polarization_t  Polarization;
        STTUNER_LNBToneState_t  ToneState;      /* use lnb for this */
        STTUNER_22KHzToneControl_t ToneSourceControl;
        STTUNER_PowerControl_t     LNBPowerControl;
        #ifdef STTUNER_DRV_SAT_LNB21
        STTUNER_LNBCurrentThresholdSelection_t CurrentThresholdSelection;              /* for lnb21 */
        #endif
        #if defined(STTUNER_DRV_SAT_LNB21) ||defined(STTUNER_DRV_SAT_LNBH21) || defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)	   
	STTUNER_LNBShortCircuitProtectionMode_t ShortCircuitProtectionMode;  /* for lnb21 */
        STTUNER_CoaxCableLossCompensation_t CableLossCompensation;   /* for lnb21 */
        #endif
    } LNB_Config_t;
    

    /* support type: LNB initialization parameters */
    typedef struct
    {
        IOARCH_Handle_t  IOHandle;  /* already open handle for STTUNER_IOARCH_ReadWrite I/O */
        ST_Partition_t  *MemoryPartition;
        #ifdef STTUNER_LNB_POWER_THRU_BACKENDGPIO
        STTUNER_LNB_Via_PIO_t *LnbIOPort;
        #endif
    } LNB_InitParams_t;


    /* support type: LNB driver termination parameters */
    typedef struct
    {
        BOOL ForceTerminate;        /* force driver termination (don't wait) */
    } LNB_TermParams_t;


    /* support type: LNB driver close parameters */
    typedef struct
    {
        STTUNER_Da_Status_t Status;
    } LNB_CloseParams_t;


    typedef U32 LNB_Handle_t; /* handle returned by LNB driver open */


    /* we have one of these for every TYPE of driver in the system */
    typedef struct
    {

        STTUNER_LnbType_t ID;     /* enumerated hardware ID */

        /* ---------- API to lnb ---------- */
        
        ST_ErrorCode_t (*lnb_Init)(ST_DeviceName_t *DeviceName, LNB_InitParams_t *InitParams);
        ST_ErrorCode_t (*lnb_Term)(ST_DeviceName_t *DeviceName, LNB_TermParams_t *TermParams);
        

        ST_ErrorCode_t (*lnb_Open) (ST_DeviceName_t *DeviceName, LNB_OpenParams_t *OpenParams, LNB_Capability_t *Capability, LNB_Handle_t *Handle);
        ST_ErrorCode_t (*lnb_Close)(LNB_Handle_t  Handle, LNB_CloseParams_t *CloseParams);
        
        ST_ErrorCode_t (*lnb_GetConfig)(LNB_Handle_t Handle, LNB_Config_t *Config);
        ST_ErrorCode_t (*lnb_SetConfig)(LNB_Handle_t Handle, LNB_Config_t *Config);

        /* repeater/passthrough port for other drivers to use, type: STTUNER_IOARCH_RedirFn_t */
        ST_ErrorCode_t (*lnb_ioaccess)(LNB_Handle_t Handle, IOARCH_Handle_t IOHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);

        /* access device specific low-level functions */
        ST_ErrorCode_t (*lnb_ioctl)(LNB_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status);
	ST_ErrorCode_t (*lnb_overloadcheck)(LNB_Handle_t Handle, BOOL  *IsOverTemp, BOOL *IsCurrentOvrLoad);

	ST_ErrorCode_t (*lnb_setttxmode)(LNB_Handle_t Handle, STTUNER_LnbTTxMode_t Ttxmode);

    } STTUNER_lnb_dbase_t;


    /* we have one of these for every INSTANCE of driver in the system (equal to how
    many were declared to be in the system at init time */
    typedef struct
    {
        ST_ErrorCode_t       Error;     /* last reported driver error  */
        STTUNER_lnb_dbase_t *Driver;    /* which driver                */ 
        IOARCH_Handle_t      IOHandle;  /* IO handle                   */
        LNB_Handle_t         DrvHandle; /* handle to instance of driver*/
    } STTUNER_lnb_instance_t;

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SAT_MLNB_H */


/* End of mlnb.h */
