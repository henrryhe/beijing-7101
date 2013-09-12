/* ----------------------------------------------------------------------------
File Name: lnone.h

Description: 

    null LNB driver.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 20-June-2001
version: 3.1.0
 author: GJP from work by LW
comment: 
    
Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SAT_LNONE_H
#define __STTUNER_SAT_LNONE_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

#include "stddefs.h"            /* Standard definitions */

/* internal types */
#include "dbtypes.h"    /* data types for databases */

ST_ErrorCode_t STTUNER_DRV_LNB_NONE_Install(STTUNER_lnb_dbase_t *Lnb);
ST_ErrorCode_t STTUNER_DRV_LNB_NONE_UnInstall(STTUNER_lnb_dbase_t *Lnb);
#if defined(STTUNER_DRV_SAT_LNB21) || defined(STTUNER_DRV_SAT_LNBH21) || defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)/*Added for GNBvd40148-->Error
if sttuner compiled with LNBH21 option but use another one: This will only come when more than 1 lnb is
in use & atleast one of them is LNB21 or LNBH21*/
ST_ErrorCode_t lnb_lnone_overloadcheck(LNB_Handle_t Handle, BOOL  *IsOverTemp, BOOL *IsCurrentOvrLoad);
#if defined(STTUNER_DRV_SAT_LNBH21)
ST_ErrorCode_t lnb_lnone_setttxmode(LNB_Handle_t Handle, STTUNER_LnbTTxMode_t Ttxmode);
#endif
#endif



#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SAT_LNONE_H */


/* End of lnone.h */
