/* ----------------------------------------------------------------------------
File Name: terutil.c

Description:

     This module handles utility functions for the magagment layer for terrestrial.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 12-Sept-2001
version: 3.1.0
 author: GB from the work by GJP
comment: write for multi-instance.

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

#include "terapi.h"     /* terrestrial manager API (functions in this file exported via this header) */

/* ----------------------------------------------------------------------------
Name: STTUNER_TERR_Utils_InitParamCheck()

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
ST_ErrorCode_t STTUNER_TERR_Utils_InitParamCheck(STTUNER_InitParams_t *InitParams)
{
    ST_ErrorCode_t Error;
#ifdef STTUNER_DEBUG_MODULE_TERR_UTILS
    const char *identity = "STTUNER terutil.c STTUNER_TERR_Util_InitParamCheck()";
#endif

    /*---------- check device lists ---------- */
    Error = STTUNER_Util_CheckPtrNull(InitParams);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_UTILS
        STTBX_Print(("%s fail *Demod not valid\n", identity));
#endif
        return(Error);
    }



    /* TODO */



    /* ---------- passed checks ---------- */
#ifdef STTUNER_DEBUG_MODULE_TERR_UTILS
        STTBX_Print(("%s PASS\n", identity));
#endif
        return(ST_NO_ERROR);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_TERR_Utils_CleanInstHandle()

Description:

Parameters:

Return Value:

See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_TERR_Utils_CleanInstHandle(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_UTILS
    const char *identity = "STTUNER terutil.c STTUNER_TERR_Utils_CleanInstHandle()";
#endif
    STTUNER_InstanceDbase_t *Inst;

    Inst = STTUNER_GetDrvInst();

    /*---------- check handle validity ---------- */
    if (Handle >= STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_UTILS
        STTBX_Print(("%s fail inavlid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* reset terrestrial portion of a STTUNER handle */
    Inst[Handle].Terr.Demod.Error  = ST_NO_ERROR;
    Inst[Handle].Terr.Demod.Driver = NULL;

    Inst[Handle].Terr.Tuner.Error  = ST_NO_ERROR;
    Inst[Handle].Terr.Tuner.Driver = NULL;


    return(ST_NO_ERROR);
}



/* End of terutil.c */

