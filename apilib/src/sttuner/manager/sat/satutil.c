/* ----------------------------------------------------------------------------
File Name: satutil.c

Description: 

     This module handles utility functions for the magagment layer for satellite.

Copyright (C) 1999-2001 STMicroelectronics

History:
 
   date: 18-June-2001
version: 3.1.0
 author: GJP
comment: write for multi-instance.

   date: 30-March-2004
   author: SD
   comment: add code to clean up scr driver.
    
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
  
/* C libs */
#include <string.h>                     
#endif
/* Standard includes */
#include "stlite.h"

/* STAPI */
#include "sttbx.h"

#include "sttuner.h"                    

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "satapi.h"     /* satellite manager API (functions in this file exported via this header) */

/* ----------------------------------------------------------------------------
Name: STTUNER_SAT_Utils_InitParamCheck()

Description:
    check validity of (most of) the data in a STTUNER_InitParams_t structure.
    
Parameters:
    STTUNER_InitParams_t
    
Return Value:
    ST_NO_ERROR             the operation was carried out without error.
    ST_ERROR_BAD_PARAMETER  one or more parameters was invalid.
    Error  bad pointer
    
See Also:
    Revision (variable).
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SAT_Utils_InitParamCheck(STTUNER_InitParams_t *InitParams)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
#ifdef STTUNER_DEBUG_MODULE_SAT_UTILS
    const char *identity = "STTUNER satutil.c STTUNER_SAT_Util_InitParamCheck()";
#endif
    

    /* nothing TODO yet */


    /* ---------- passed checks ---------- */
#ifdef STTUNER_DEBUG_MODULE_SAT_UTILS
        STTBX_Print(("%s PASS\n", identity));
#endif
        return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_SAT_Utils_CleanInstHandle()

Description:
    
Parameters:
    
Return Value:

See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SAT_Utils_CleanInstHandle(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_UTILS
    const char *identity = "STTUNER satutil.c STTUNER_SAT_Utils_CleanInstHandle()";
#endif
    STTUNER_InstanceDbase_t *Inst;

    Inst = STTUNER_GetDrvInst();
    
    /*---------- check handle validity ---------- */
    if (Handle >= STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_UTILS
        STTBX_Print(("%s fail inavlid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* reset satellite portion of a STTUNER handle */
    Inst[Handle].Sat.Demod.Error  = ST_NO_ERROR;
    Inst[Handle].Sat.Demod.Driver = NULL;

    Inst[Handle].Sat.Tuner.Error  = ST_NO_ERROR;
    Inst[Handle].Sat.Tuner.Driver = NULL;

    Inst[Handle].Sat.Lnb.Error  = ST_NO_ERROR;
    Inst[Handle].Sat.Lnb.Driver = NULL;
    
    #ifdef STTUNER_DRV_SAT_SCR
    Inst[Handle].Sat.Scr.Error  = ST_NO_ERROR;
    Inst[Handle].Sat.Scr.Driver = NULL;
    #endif

    return(ST_NO_ERROR);
}



/* End of satutil.c */

