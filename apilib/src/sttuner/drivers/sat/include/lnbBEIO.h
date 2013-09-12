/* ----------------------------------------------------------------------------
File Name: lnbBEIO.h

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
#ifndef __STTUNER_SAT_LNBBEIO_H
#define __STTUNER_SAT_LNBBEIO_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/* includes ---------------------------------------------------------------- */

#include "stddefs.h"            /* Standard definitions */

/* internal types */
#include "dbtypes.h"    /* data types for databases */
/* API */
ST_ErrorCode_t lnb_backendIO_Init(ST_DeviceName_t *DeviceName,LNB_InitParams_t *InitParams);
ST_ErrorCode_t lnb_backendIO_Term(ST_DeviceName_t *DeviceName,LNB_TermParams_t *TermParams);

ST_ErrorCode_t lnb_backendIO_Open (ST_DeviceName_t *DeviceName, LNB_OpenParams_t *OpenParams, LNB_Capability_t *Capability, LNB_Handle_t *Handle);
ST_ErrorCode_t lnb_backendIO_Close(LNB_Handle_t  Handle, LNB_CloseParams_t *CloseParams);

ST_ErrorCode_t lnb_backendIO_GetConfig(LNB_Handle_t Handle, LNB_Config_t *Config);
ST_ErrorCode_t lnb_backendIO_SetConfig(LNB_Handle_t Handle, LNB_Config_t *Config);
/* populate database & init driver (not actual hardware) */
ST_ErrorCode_t STTUNER_DRV_LNB_BackEndIO_Install(STTUNER_lnb_dbase_t *Lnb);
ST_ErrorCode_t STTUNER_DRV_LNB_BackEndIO_UnInstall(STTUNER_lnb_dbase_t *Lnb);



#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SAT_LNBBEIO_H */


/* End of lnbBEIO.h */
