/* ----------------------------------------------------------------------------
File Name: l0299.h

Description: 

    stv0299 LNB driver.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 5-July-2001
version: 3.1.0
 author: GJP from work by LW
comment: 
    
Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SAT_L0299_H
#define __STTUNER_SAT_L0299_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/* includes ---------------------------------------------------------------- */

#include "stddefs.h"            /* Standard definitions */

/* internal types */
#include "dbtypes.h"    /* data types for databases */


/* constants --------------------------------------------------------------- */

/* map lnb register controls within stv0299 */
#define DEF_LNB_0299_NBREG   4
#define DEF_LNB_0299_NBFIELD 13


/* DISEQC register */
#define  R0299_DISEQC   0

/* DISEQC fields */
#define  F0299_LOCKOUTPUT           0
#define  F0299_LOCKCONFIGURATION    1
#define  F0299_UNMODULATEDBURST     2
#define  F0299_DISEQCMODE           3


/* IOCFG register */
#define  R0299_IOCFG    1

/* IOCFG fields */
#define  F0299_OP1CONTROL       4
#define  F0299_OP1VALUE         5
#define  F0299_OP0CONTROL       6
#define  F0299_OP0VALUE         7
#define  F0299_NYQUISTFILTER    8
#define  F0299_IQ               9

/*DACR1*/
#define R0299_DACR1  2
/*DACR1 fields*/
#define F0299_DACMODE  10
#define F0299_DACMSB   11


/*DACr2*/
#define R0299_DACR2  3
/*DACR2 fields*/
#define F0299_DACLSB   12


/* functions --------------------------------------------------------------- */


/* populate database & init driver (not actual hardware) */
ST_ErrorCode_t STTUNER_DRV_LNB_STV0299_Install(STTUNER_lnb_dbase_t *Lnb);
ST_ErrorCode_t STTUNER_DRV_LNB_STV0299_UnInstall(STTUNER_lnb_dbase_t *Lnb);
#if defined(STTUNER_DRV_SAT_LNB21) || defined(STTUNER_DRV_SAT_LNBH21) /*Added for GNBvd40148-->Error
if sttuner compiled with LNBH21 option but use another one: This will only come when more than 1 lnb is
in use & atleast one of them is LNB21 or LNBH21*/
ST_ErrorCode_t lnb_l0299_overloadcheck(LNB_Handle_t Handle, BOOL  *IsOverTemp, BOOL *IsCurrentOvrLoad);
#if defined(STTUNER_DRV_SAT_LNBH21)
ST_ErrorCode_t lnb_l0299_setttxmode(LNB_Handle_t Handle, STTUNER_LnbTTxMode_t Ttxmode);
#endif
#endif

#ifdef STTUNER_MINIDRIVER
/* API */
ST_ErrorCode_t lnb_l0299_Init(ST_DeviceName_t *DeviceName,LNB_InitParams_t *InitParams);
ST_ErrorCode_t lnb_l0299_Term(ST_DeviceName_t *DeviceName,LNB_TermParams_t *TermParams);

ST_ErrorCode_t lnb_l0299_Open (ST_DeviceName_t *DeviceName, LNB_OpenParams_t *OpenParams, LNB_Capability_t *Capability, LNB_Handle_t *Handle);
ST_ErrorCode_t lnb_l0299_Close(LNB_Handle_t  Handle, LNB_CloseParams_t *CloseParams);

ST_ErrorCode_t lnb_l0299_GetConfig(LNB_Handle_t Handle, LNB_Config_t *Config);
ST_ErrorCode_t lnb_l0299_SetConfig(LNB_Handle_t Handle, LNB_Config_t *Config);
#endif

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SAT_L0299_H */


/* End of l0299.h */
