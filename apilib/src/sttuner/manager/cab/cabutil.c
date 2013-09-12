/* ----------------------------------------------------------------------------
File Name: cabutil.c

Description: 

     This module handles utility functions for the magagment layer for cable.

Copyright (C) 1999-2001 STMicroelectronics

   date: 01-October-2001
version: 3.2.0
 author: from STV0299 and MB validation drivers.
comment: Write for multi-instance/multi-FrontEnd.

Revision History:
    
Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */

#ifndef ST_OSLINUX
  
/* C libs */
#include <string.h>
#endif
#include "stlite.h"
#include "sttbx.h"

#include "sttuner.h"                    

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "cabapi.h"     /* cable manager API (functions in this file exported via this header) */

/* ----------------------------------------------------------------------------
Name: STTUNER_CABLE_Utils_InitParamCheck()

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
ST_ErrorCode_t STTUNER_CABLE_Utils_InitParamCheck(STTUNER_InitParams_t *InitParams)
{
    ST_ErrorCode_t Error;
#ifdef STTUNER_DEBUG_MODULE_CAB_UTILS
    const char *identity = "STTUNER cabutil.c STTUNER_CABLE_Utils_InitParamCheck()";
#endif
    
    /*---------- check device lists ---------- */
    Error = STTUNER_Util_CheckPtrNull(InitParams);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_UTILS
        STTBX_Print(("%s fail *Demod not valid\n", identity));
#endif
        return(Error);
    }



    /* TODO */



    /* ---------- passed checks ---------- */
#ifdef STTUNER_DEBUG_MODULE_CAB_UTILS
        STTBX_Print(("%s PASS\n", identity));
#endif
        return(ST_NO_ERROR);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_CABLE_Utils_CleanInstHandle()

Description:
    
Parameters:
    
Return Value:

See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_CABLE_Utils_CleanInstHandle(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_UTILS
    const char *identity = "STTUNER cabutil.c STTUNER_CABLE_Utils_CleanInstHandle()";
#endif
    STTUNER_InstanceDbase_t *Inst;

    Inst = STTUNER_GetDrvInst();
    
    /*---------- check handle validity ---------- */
    if (Handle >= STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_UTILS
        STTBX_Print(("%s fail invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* reset cable portion of a STTUNER handle */
    Inst[Handle].Cable.Demod.Error  = ST_NO_ERROR;
    Inst[Handle].Cable.Demod.Driver = NULL;

    Inst[Handle].Cable.Tuner.Error  = ST_NO_ERROR;
    Inst[Handle].Cable.Tuner.Driver = NULL;

    return(ST_NO_ERROR);
}



/* End of cabutil.c */

