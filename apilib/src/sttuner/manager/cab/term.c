/* ----------------------------------------------------------------------------
File Name: term.c

Description: 

     This module handles the term() interface for cable.

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
  
#include <string.h>
#endif


#include "stlite.h"     /* Standard includes */


/* STAPI */
#include "sttbx.h"

#include "sttuner.h"                    

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "cabapi.h"     /* cable manager API (functions in this file exported via this header) */
#include "stapitnr.h"

/* driver API*/
#include "drivers.h"
#include "chip.h"

/* ----------------------------------------------------------------------------
Name: STTUNER_CABLE_Term()

Description:
    terms all cable devices.
    
Parameters:
    DeviceName  STTUNER global instance name device name as set during initialization.
   *InitParams  pointer to initialization parameters.
    
Return Value:
    ST_NO_ERROR,                    the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_CABLE_Term(STTUNER_Handle_t Handle, const STTUNER_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_TERM
    const char *identity = "STTUNER term.c STTUNER_CABLE_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    DEMOD_TermParams_t  DEMOD_TermParams;
    TUNER_TermParams_t  TUNER_TermParams;

    STTUNER_InstanceDbase_t *Inst;


    Inst = STTUNER_GetDrvInst();

    /* ---------- term tuner driver ---------- */
    TUNER_TermParams.ForceTerminate = TermParams->ForceTerminate;

    Error = (Inst[Handle].Cable.Tuner.Driver->tuner_Term)(&Inst[Handle].Name, &TUNER_TermParams);
    Inst[Handle].Cable.Tuner.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_TERM
        STTBX_Print(("%s fail tuner term() Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_TERM
        STTBX_Print(("%s tuner term() ok\n", identity));
#endif
    }

    /* ---------- term demod driver ---------- */
    DEMOD_TermParams.ForceTerminate = TermParams->ForceTerminate;

    Error = (Inst[Handle].Cable.Demod.Driver->demod_Term)(&Inst[Handle].Name, &DEMOD_TermParams);
    Inst[Handle].Cable.Demod.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_TERM
        STTBX_Print(("%s fail demod term() Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_TERM
        STTBX_Print(("%s demod term() ok\n", identity));
#endif
    }

    /* ---------- close tuner I/O ---------- */
    Error = ChipClose(Inst[Handle].Cable.Tuner.IOHandle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_TERM
        STTBX_Print(("%s fail close tuner I/O Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_TERM
        STTBX_Print(("%s close tuner I/O ok\n", identity));
#endif
    }

    /* ---------- close demod I/O ---------- */
    Error = ChipClose(Inst[Handle].Cable.Demod.IOHandle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_TERM
        STTBX_Print(("%s fail close demod I/O Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_TERM
        STTBX_Print(("%s close demod I/O ok\n", identity));
#endif
    }

    /* ---------- point entries away from driver database ---------- */
    Inst[Handle].Cable.Tuner.Driver  = NULL;
    Inst[Handle].Cable.Demod.Driver  = NULL;


    return(Error);

} /* STTUNER_CABLE_Term() */



/* End of term.c */
