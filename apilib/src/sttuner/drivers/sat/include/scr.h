/* ----------------------------------------------------------------------------
File Name: lnb21.h

Description: 

    LNB21 driver.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 04-November-2003
version: 
 author: SD,AS
comment: 
    
Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SAT_SCR_H
#define __STTUNER_SAT_SCR_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/* includes ---------------------------------------------------------------- */

#include "stddefs.h"            /* Standard definitions */

/* internal types */
#include "dbtypes.h"    /* data types for databases */

/* functions --------------------------------------------------------------- */

#ifndef STTUNER_MINIDRIVER
/* populate database & init driver (not actual hardware) */
ST_ErrorCode_t STTUNER_DRV_SCR_APPLICATION_Install(STTUNER_scr_dbase_t *Scr);
ST_ErrorCode_t STTUNER_DRV_SCR_APPLICATION_UnInstall(STTUNER_scr_dbase_t *Scr);
#endif

#ifdef STTUNER_MINIDRIVER
/* API */
ST_ErrorCode_t scr_scrdrv_Init(ST_DeviceName_t *DeviceName, SCR_InitParams_t *InitParams);
ST_ErrorCode_t scr_scrdrv_Term(ST_DeviceName_t *DeviceName, SCR_TermParams_t *TermParams);
ST_ErrorCode_t scr_scrdrv_Open (ST_DeviceName_t *DeviceName, SCR_OpenParams_t  *OpenParams, SCR_Handle_t  *Handle, DEMOD_Handle_t DemodHandle, SCR_Capability_t *Capability);
ST_ErrorCode_t scr_scrdrv_Close(SCR_Handle_t  Handle, SCR_CloseParams_t *CloseParams);
ST_ErrorCode_t scr_scrdrv_IsLocked(SCR_Handle_t  Handle, DEMOD_Handle_t DemodHandle, BOOL  *IsLocked);
ST_ErrorCode_t scr_scrdrv_SetFrequency   (STTUNER_Handle_t Handle, DEMOD_Handle_t DemodHandle, ST_DeviceName_t *DeviceName, U32  InitialFrequency,  U8 LNBIndex, U8 SCRBPF );
ST_ErrorCode_t scr_scrdrv_Off (SCR_OpenParams_t  *OpenParams, DEMOD_Handle_t DemodHandle, U8 SCRBPF);
ST_ErrorCode_t scr_auto_detect(SCR_OpenParams_t  *OpenParams, DEMOD_Handle_t DemodHandle, U32 StartFreq, U32 StopFreq, U8  *NbTones, U32 *ToneList);
U8 scr_get_application_number(SCR_OpenParams_t *OpenParams, DEMOD_Handle_t DemodHandle, U8 SCRBPF, U8  *NbTones, U32 *ToneList);

#endif

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SAT_LNB21_H */


/* End of lnb21.h */
