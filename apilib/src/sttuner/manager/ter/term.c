/* ----------------------------------------------------------------------------
File Name: term.c

Description:

     This module handles the term() interface for terrestrial.


Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 12-Sept-2001
version: 3.1.0
 author: GB from work by GJP
comment: rewrite for terrestrial.

Reference:

    ST API Definition "TUNER Driver API" DVD-API-06
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
#include "chip.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "terapi.h"     /* terrestrial manager API (functions in this file exported via this header) */
#include "stapitnr.h"

/* driver API*/
#include "drivers.h"
#ifdef STTUNER_MINIDRIVER
  #ifdef STTUNER_DRV_TER_STV0360
	#include "360_echo.h" 
	#include "360_drv.h"
	#include "d0360.h"
	#include "iodirect.h"
	#include "tuntdrv.h"
#endif
#endif

/* ----------------------------------------------------------------------------
Name: STTUNER_TERR_Term()

Description:
    terms all ter devices.

Parameters:
    DeviceName  STTUNER global instance name device name as set during initialization.
   *InitParams  pointer to initialization parameters.

Return Value:
    ST_NO_ERROR,                    the operation was carried out without error.

See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_TERR_Term(STTUNER_Handle_t Handle, const STTUNER_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_TERM
    const char *identity = "STTUNER term.c STTUNER_TERR_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    DEMOD_TermParams_t  DEMOD_TermParams;
    TUNER_TermParams_t  TUNER_TermParams;
    STTUNER_InstanceDbase_t *Inst;

#ifndef STTUNER_MINIDRIVER
    Inst = STTUNER_GetDrvInst();

    /* ---------- term tuner driver ---------- */
    TUNER_TermParams.ForceTerminate = TermParams->ForceTerminate;

    Error = (Inst[Handle].Terr.Tuner.Driver->tuner_Term)(&Inst[Handle].Name, &TUNER_TermParams);
    Inst[Handle].Terr.Tuner.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_TERM
        STTBX_Print(("%s fail tuner term() Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_TERM
        STTBX_Print(("%s tuner term() ok\n", identity));
#endif
    }

    /* ---------- term demod driver ---------- */
    DEMOD_TermParams.ForceTerminate = TermParams->ForceTerminate;

    Error = (Inst[Handle].Terr.Demod.Driver->demod_Term)(&Inst[Handle].Name, &DEMOD_TermParams);
    Inst[Handle].Terr.Demod.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_TERM
        STTBX_Print(("%s fail demod term() Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_TERM
        STTBX_Print(("%s demod term() ok\n", identity));
#endif
    }


    /* ---------- close tuner I/O ---------- */
    Error = ChipClose(Inst[Handle].Terr.Tuner.IOHandle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_TERM
        STTBX_Print(("%s fail close tuner I/O Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_TERM
        STTBX_Print(("%s close tuner I/O ok\n", identity));
#endif
    }

    /* ---------- close demod I/O ---------- */
    Error = ChipClose(Inst[Handle].Terr.Demod.IOHandle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_TERM
        STTBX_Print(("%s fail close demod I/O Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_TERM
        STTBX_Print(("%s close demod I/O ok\n", identity));
#endif
    }

    /* ---------- point entries away from driver database ---------- */

    Inst[Handle].Terr.Tuner.Driver  = NULL;
    Inst[Handle].Terr.Demod.Driver  = NULL;

#endif
#ifdef STTUNER_MINIDRIVER
    
    Inst = STTUNER_GetDrvInst();
   
#ifdef STTUNER_DRV_TER_STV0360
	    TUNER_TermParams.ForceTerminate = TermParams->ForceTerminate;
	    Error = tuner_tuntdrv_Term(&Inst[Handle].Name, &TUNER_TermParams);
	    Inst[Handle].Terr.Tuner.Error = Error;
	    if (Error != ST_NO_ERROR)
	    {
		#ifdef STTUNER_DEBUG_MODULE_TER_TERM
		        STTBX_Print(("%s fail tuner term() Error=%d\n", identity, Error));
		#endif
	        return(Error);
	    }
	    else
	    {
		#ifdef STTUNER_DEBUG_MODULE_TER_TERM
		        STTBX_Print(("%s tuner term() ok\n", identity));
		#endif
	    }
#endif
    DEMOD_TermParams.ForceTerminate = TermParams->ForceTerminate;
    #ifdef STTUNER_DRV_TER_STV0360
    Error = demod_d0360_Term(&Inst[Handle].Name, &DEMOD_TermParams);
    #endif
    Inst[Handle].Terr.Demod.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TER_TERM
        STTBX_Print(("%s fail demod term() Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_TER_TERM
        STTBX_Print(("%s demod term() ok\n", identity));
#endif
    }
    Error = STTUNER_IODEMOD_Close(&Inst[Handle].Terr.Demod.IOHandle, &IOARCH_CloseParams);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TER_TERM
        STTBX_Print(("%s fail close demod I/O Error=%d\n", identity, Error));
#endif
        return(Error);
    }
#endif    


    return(Error);

} /* STTUNER_TER_Term() */



/* End of term.c */
