/* ----------------------------------------------------------------------------
File Name: l0399.h

Description: 

    stv0399 LNB driver.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 20-August-2001
version: 3.2.0
 author: GJP
comment: Initial version.
    
Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SAT_L0399_H
#define __STTUNER_SAT_L0399_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/* includes ---------------------------------------------------------------- */

#include "stddefs.h"            /* Standard definitions */

/* internal types */
#include "dbtypes.h"    /* data types for databases */

#ifdef STTUNER_MINIDRIVER
ST_ErrorCode_t lnb_l0399_Init(ST_DeviceName_t *DeviceName,LNB_InitParams_t *InitParams);
ST_ErrorCode_t lnb_l0399_Term(ST_DeviceName_t *DeviceName,LNB_TermParams_t *TermParams);

ST_ErrorCode_t lnb_l0399_Open (ST_DeviceName_t *DeviceName, LNB_OpenParams_t *OpenParams, LNB_Capability_t *Capability, LNB_Handle_t *Handle);
ST_ErrorCode_t lnb_l0399_Close(LNB_Handle_t  Handle, LNB_CloseParams_t *CloseParams);

ST_ErrorCode_t lnb_l0399_GetConfig(LNB_Handle_t Handle, LNB_Config_t *Config);
ST_ErrorCode_t lnb_l0399_SetConfig(LNB_Handle_t Handle, LNB_Config_t *Config);

#endif
/* constants --------------------------------------------------------------- */

/* functions --------------------------------------------------------------- */


/* populate database & init driver (not actual hardware) */
ST_ErrorCode_t STTUNER_DRV_LNB_STV0399_Install(STTUNER_lnb_dbase_t *Lnb);
ST_ErrorCode_t STTUNER_DRV_LNB_STV0399_UnInstall(STTUNER_lnb_dbase_t *Lnb);
#if defined(STTUNER_DRV_SAT_LNB21) || defined(STTUNER_DRV_SAT_LNBH21) /*Added for GNBvd40148-->Error
if sttuner compiled with LNBH21 option but use another one: This will only come when more than 1 lnb is
in use & atleast one of them is LNB21 or LNBH21*/
ST_ErrorCode_t lnb_l0399_overloadcheck(LNB_Handle_t Handle, BOOL  *IsOverTemp, BOOL *IsCurrentOvrLoad);
#if defined(STTUNER_DRV_SAT_LNBH21)
ST_ErrorCode_t lnb_l0399_setttxmode(LNB_Handle_t Handle, STTUNER_LnbTTxMode_t Ttxmode);
#endif
#endif


#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SAT_L0399_H */


/* End of l0399.h */
