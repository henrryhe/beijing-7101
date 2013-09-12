/* ----------------------------------------------------------------------------
File Name: lnb21.h

Description: 

    LNB21 driver.

Copyright (C) 2004-2005 STMicroelectronics

History:

   date: 04-November-2003
version: 
 author: SD,AS
comment: 
date: 05-01-2005
version: 
 author: 
comment: Function decleration for MINIDRIVER.
    
Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SAT_LNB21_H
#define __STTUNER_SAT_LNB21_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/* includes ---------------------------------------------------------------- */

#include "stddefs.h"            /* Standard definitions */

/* internal types */
#include "dbtypes.h"    /* data types for databases */


/* constants --------------------------------------------------------------- */



/* map lnb register controls of lnb21 */
#define DEF_LNB_LNB21_NBREG   2
#define DEF_LNB_LNB21_NBFIELD 9


/* IOCFG register */
#define  RLNB21_REGS         0x00
#define  RLNB21_OP2          0x0a

/* IOCFG fields */
#define  FLNB21_OLF         0x000001		  /* Over current protection Indicator */
#define  FLNB21_OTF         0x000002 /* Over temperature protection Indicator */
#define  FLNB21_EN           0x000004 /* Port Enable */
#define  FLNB21_VSEL       0x000008 /* Voltage Select 13/18V */
#define  FLNB21_LLC         0x000010 /* Coax Cable Loss Compensation */
#define  FLNB21_TEN         0x000020 /* Tone Enable/Disable */
#define  FLNB21_ISEL        0x000040 /* LNB Current Selection */
#define  FLNB21_PCL        0x000080/* Short Circuit Protection, Dynamic/Static Mode */
#define  FLNB21_OP2_CONF    0x0a00e0


/* functions --------------------------------------------------------------- */


/* populate database & init driver (not actual hardware) */
ST_ErrorCode_t STTUNER_DRV_LNB_LNB21_Install(STTUNER_lnb_dbase_t *Lnb);
ST_ErrorCode_t STTUNER_DRV_LNB_LNB21_UnInstall(STTUNER_lnb_dbase_t *Lnb);

#ifdef STTUNER_MINIDRIVER
/* API */
ST_ErrorCode_t lnb_lnb21_Init(ST_DeviceName_t *DeviceName,LNB_InitParams_t *InitParams);
ST_ErrorCode_t lnb_lnb21_Term(ST_DeviceName_t *DeviceName,LNB_TermParams_t *TermParams);
ST_ErrorCode_t lnb_lnb21_Open (ST_DeviceName_t *DeviceName, LNB_OpenParams_t *OpenParams, LNB_Capability_t *Capability, LNB_Handle_t *Handle);
ST_ErrorCode_t lnb_lnb21_Close(LNB_Handle_t  Handle, LNB_CloseParams_t *CloseParams);

ST_ErrorCode_t lnb_lnb21_GetConfig(LNB_Config_t *Config);
ST_ErrorCode_t LNB21_GetPower(LNB_Status_t *Status);
ST_ErrorCode_t LNB21_GetStatus(LNB_Status_t *Status);
ST_ErrorCode_t lnb_lnb21_GetProtection(LNB_Config_t *Config);
ST_ErrorCode_t lnb_lnb21_GetCurrent(LNB_Config_t *Config);
ST_ErrorCode_t lnb_lnb21_GetLossCompensation(LNB_Config_t *Config);
ST_ErrorCode_t lnb_lnb21_overloadcheck(BOOL  *IsOverTemp, BOOL *IsCurrentOvrLoad);
#if defined(STTUNER_DRV_SAT_LNB21) /*Added for GNBvd40148-->Error
if sttuner compiled with LNBH21 option but use another one: This will only come when more than 1 lnb is
in use & atleast one of them is LNB21 or LNBH21*/
ST_ErrorCode_t lnb_lnb21_setttxmode(LNB_Handle_t Handle, STTUNER_LnbTTxMode_t Ttxmode);
#endif

/* local functions */
void           LNB21_SetLnb         (int Lnb);
void           LNB21_SetPolarization(LNB_Polarization_t Polarization);
void           LNB21_SetCurrentThreshold(LNB_CurrentThresholdSelection_t CurrentThresholdSelection);
void           LNB21_SetProtectionMode(LNB_ShortCircuitProtectionMode_t ShortCircuitProtectionMode);
void           LNB21_SetLossCompensation(BOOL CoaxCableLossCompensation);
ST_ErrorCode_t LNB21_SetPower       (LNB_Status_t  Status);
ST_ErrorCode_t lnb_lnb21_SetConfig(LNB_Config_t *Config);

ST_ErrorCode_t           LNB21_UpdateLNB(void);
#endif

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SAT_LNB21_H */


/* End of lnb21.h */
