/* ----------------------------------------------------------------------------
File Name: lstem.h

Description: 

    STEM LNB driver.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 19-March-2001
version: 
 author: SJD from work by GJP from work by LW
comment: 
    
Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SAT_LSTEM_H
#define __STTUNER_SAT_LSTEM_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/* includes ---------------------------------------------------------------- */

#include "stddefs.h"            /* Standard definitions */

/* internal types */
#include "dbtypes.h"    /* data types for databases */


/* constants --------------------------------------------------------------- */

/* map lnb register controls of stem */
#define DEF_LNB_STEM_NBREG   1
#define DEF_LNB_STEM_NBFIELD 8

/* IOCFG register */
#define  RSTEM_REGS         0

/* IOCFG fields */
#define  FSTEM_LED          7
#define  FSTEM_FE2_TONE     6
#define  FSTEM_FE2_LNB      5
#define  FSTEM_FE2_VOLTAGE 	4
#define  FSTEM_FE1_TONE     3
#define  FSTEM_FE1_LNB      2
#define  FSTEM_FE1_VOLTAGE 	1
#define  FSTEM_FE_SELECT    0


/* functions --------------------------------------------------------------- */


/* populate database & init driver (not actual hardware) */
ST_ErrorCode_t STTUNER_DRV_LNB_STEM_Install(STTUNER_lnb_dbase_t *Lnb);
ST_ErrorCode_t STTUNER_DRV_LNB_STEM_UnInstall(STTUNER_lnb_dbase_t *Lnb);
#if defined(STTUNER_DRV_SAT_LNB21) || defined(STTUNER_DRV_SAT_LNBH21) /*Added for GNBvd40148-->Error
if sttuner compiled with LNBH21 option but use another one: This will only come when more than 1 lnb is
in use & atleast one of them is LNB21 or LNBH21*/
ST_ErrorCode_t lnb_lstem_overloadcheck(LNB_Handle_t Handle, BOOL  *IsOverTemp, BOOL *IsCurrentOvrLoad);
#if defined(STTUNER_DRV_SAT_LNBH21)
ST_ErrorCode_t lnb_lstem_setttxmode(LNB_Handle_t Handle, STTUNER_LnbTTxMode_t Ttxmode);
#endif
#endif


#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SAT_LSTEM_H */


/* End of lstem.h */
