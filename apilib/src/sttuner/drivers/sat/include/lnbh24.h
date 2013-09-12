/* ----------------------------------------------------------------------------
File Name: lnbh24.h

Description: 

    LNBH24 driver.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 
version: 
 author: 
comment: 
    
Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SAT_LNBH24_H
#define __STTUNER_SAT_LNBH24_H

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
#define DEF_LNB_LNBH24_NBREG   1
#define DEF_LNB_LNBH24_NBFIELD 8

/* IOCFG register */
#define  RLNBH24_REGS         0x00


/* IOCFG fields */
#define  FLNBH24_OLF        0x000001/* Over current protection Indicator */
#define  FLNBH24_OTF        0x000002 /* Over temperature protection Indicator */
#define  FLNBH24_EN          0x000004 /* Port Enable */
#define  FLNBH24_VSEL      0x000008 /* Voltage Select 13/18V */
#define  FLNBH24_LLC        0x000010 /* Coax Cable Loss Compensation */
#define  FLNBH24_TEN        0x000020 /* Tone Enable/Disable */
#define  FLNBH24_ISEL        0x000040 /*  LNB mode selection (Rx or Tx) */
#define  FLNBH24_PCL        0x000080/* Short Circuit Protection, Dynamic/Static Mode */


/* functions --------------------------------------------------------------- */


 typedef enum
 {
  LNB_IOCTL_PROTECTION_NOCHANGE = 0 ,	
  LNB_IOCTL_PROTECTION_STATIC,
  LNB_IOCTL_PROTECTION_DYNAMIC 	
 }STTUNER_IOCTL_LNBShortCircuitProtectionMode_t;
 
typedef enum
{
 LNB_IOCTL_POWERBLCOKS_NOCHANGE = 0,
 LNB_IOCTL_POWERBLCOKS_ENABLED ,
 LNB_IOCTL_POWERBLCOKS_DISABLED 
}STTUNER_IOCTL_PowerControl_t ;
typedef enum
{
 LNB_IOCTL_TTXMODE_NOCHANGE = 0,
 LNB_IOCTL_TTXMODE_RX,
 LNB_IOCTL_TTXMODE_TX  
}STTUNER_IOCTL_TTX_Mode_t;
typedef enum
{
 LNB_IOCTL_LLCMODE_NOCHANGE = 0,
 LNB_IOCTL_LLCMODE_ENABLED,
 LNB_IOCTL_LLCMODE_DISABLED  
}STTUNER_IOCTL_LLC_Mode_t;
typedef struct
{
 STTUNER_IOCTL_TTX_Mode_t               TTX_Mode;
 STTUNER_IOCTL_PowerControl_t             PowerControl;
 STTUNER_IOCTL_LNBShortCircuitProtectionMode_t ShortCircuitProtectionMode;
 STTUNER_IOCTL_LLC_Mode_t     LLC_Mode;
}LNB_IOCTL_Config_t;

/* populate database & init driver (not actual hardware) */
ST_ErrorCode_t STTUNER_DRV_LNB_LNBH24_Install(STTUNER_lnb_dbase_t *Lnb);
ST_ErrorCode_t STTUNER_DRV_LNB_LNBH24_UnInstall(STTUNER_lnb_dbase_t *Lnb);

#ifdef STTUNER_MINIDRIVER
/* API */
ST_ErrorCode_t lnb_lnbh24_Init(ST_DeviceName_t *DeviceName,LNB_InitParams_t *InitParams);
ST_ErrorCode_t lnb_lnbh24_Term(ST_DeviceName_t *DeviceName,LNB_TermParams_t *TermParams);
ST_ErrorCode_t lnb_lnbh24_Open (ST_DeviceName_t *DeviceName, LNB_OpenParams_t *OpenParams, LNB_Capability_t *Capability, LNB_Handle_t *Handle);
ST_ErrorCode_t lnb_lnbh24_Close(LNB_Handle_t  Handle, LNB_CloseParams_t *CloseParams);

ST_ErrorCode_t lnb_lnbh24_GetConfig(LNB_Config_t *Config);
ST_ErrorCode_t LNBH24_GetPower(LNB_Status_t *Status);
ST_ErrorCode_t LNBH24_GetStatus(LNB_Status_t *Status);
ST_ErrorCode_t lnb_lnbh24_GetProtection(LNB_Config_t *Config);
ST_ErrorCode_t lnb_lnbh24_GetCurrent(LNB_Config_t *Config);
ST_ErrorCode_t lnb_lnbh24_GetLossCompensation(LNB_Config_t *Config);
ST_ErrorCode_t lnb_lnbh24_overloadcheck(BOOL  *IsOverTemp, BOOL *IsCurrentOvrLoad);
ST_ErrorCode_t lnb_lnbh24_setttxmode(STTUNER_LnbTTxMode_t Ttxmode);
/* local functions */
void           LNBH24_SetLnb         (int Lnb);
void           LNBH24_SetPolarization(LNB_Polarization_t Polarization);
void           LNBH24_SetProtectionMode(LNB_ShortCircuitProtectionMode_t ShortCircuitProtectionMode);
void           LNBH24_SetLossCompensation(BOOL CoaxCableLossCompensation);
ST_ErrorCode_t LNBH24_SetPower       (LNB_Status_t  Status);
ST_ErrorCode_t lnb_lnbh24_SetConfig(LNB_Config_t *Config);

ST_ErrorCode_t           LNBH24_UpdateLNB(void);
#endif

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SAT_LNBH24_H */


/* End of lnbh24.h */
