/* ----------------------------------------------------------------------------
File Name: open.c

Description:

     This module handles the ter management functions for the STAPI level
    interface for STTUNER for Open.


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

/* local to sttuner */

#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "terapi.h"     /* terrestrial manager API (functions in this file exported via this header) */
#include "tertask.h"    /* terrestrial scan task */
#ifdef STTUNER_MINIDRIVER
  #ifdef STTUNER_DRV_TER_STV0360
	#include "d0360.h"
	#include "tuntdrv.h"
	#endif
	#include "iodirect.h"
#endif
#ifndef STTUNER_MINIDRIVER
#include "stevt.h"
#include "util.h"       /* generic utility functions for sttuner */
/* driver API*/
#include "drivers.h"
#include "ioarch.h"

#endif

/* ----------------------------------------------------------------------------
Name: STTUNER_TERR_Open()

Description:
    opens ter drivers to make a tuner.

Parameters:
    Handle      Handle inti instance array;
   *InitParams  pointer to initialization parameters.

Return Value:
    ST_NO_ERROR,                    the operation was carried out without error.
    ST_ERROR_NO_MEMORY,             memory allocation failed.

See Also:
    STTUNER_Term()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_TERR_Open(STTUNER_Handle_t Handle, const STTUNER_OpenParams_t *OpenParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_OPEN
    const char *identity = "STTUNER open.c STTUNER_TERR_Open()";
#endif
    ST_ErrorCode_t Error;
    DEMOD_OpenParams_t  DEMOD_OpenParams;
    TUNER_OpenParams_t  TUNER_OpenParams;

    DEMOD_Capability_t  DEMOD_Capability;
    TUNER_Capability_t  TUNER_Capability;

    STTUNER_DriverDbase_t   *Drv;
    STTUNER_InstanceDbase_t *Inst;


    Drv  = STTUNER_GetDrvDb();
    Inst = STTUNER_GetDrvInst();

    /* give drivers Handle to instance that uses then so they can access the other drivers of this instance */
    TUNER_OpenParams.TopLevelHandle = Handle;
    DEMOD_OpenParams.TopLevelHandle = Handle;
#ifndef STTUNER_MINIDRIVER

    /* ---------- open demod ---------- */
    Error = (Inst[Handle].Terr.Demod.Driver->demod_Open)(&Inst[Handle].Name, &DEMOD_OpenParams, &DEMOD_Capability, &Inst[Handle].Terr.Demod.DrvHandle);
    Inst[Handle].Terr.Demod.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_OPEN
        STTBX_Print(("%s fail open() demod Error=%d\n", identity, Error));
#endif
        return(Error);
    }
#ifdef STTUNER_DEBUG_MODULE_TERR_OPEN
        STTBX_Print(("%s demod #%d opened ok\n", identity, Inst[Handle].Terr.Demod.Driver->ID));
#endif

    /* ---------- open tuner ---------- */
    Error = (Inst[Handle].Terr.Tuner.Driver->tuner_Open)(&Inst[Handle].Name, &TUNER_OpenParams, &TUNER_Capability, &Inst[Handle].Terr.Tuner.DrvHandle);
    Inst[Handle].Terr.Tuner.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_OPEN
        STTBX_Print(("%s fail open() tuner Error=%d\n", identity, Error));
#endif
        return(Error);
    }
#ifdef STTUNER_DEBUG_MODULE_TERR_OPEN
        STTBX_Print(("%s tuner #%d opened ok\n", identity, Inst[Handle].Terr.Tuner.Driver->ID));
#endif


    /* ---------- fill capability  ---------- */

    /* API visible */
    Inst[Handle].Capability.Device = STTUNER_DEVICE_TERR;

    Inst[Handle].Capability.DemodType   = Inst[Handle].Terr.Demod.Driver->ID;
    Inst[Handle].Capability.TunerType   = Inst[Handle].Terr.Tuner.Driver->ID;

    Inst[Handle].Capability.AGCControl       = DEMOD_Capability.AGCControl;
    Inst[Handle].Capability.SymbolMin        = DEMOD_Capability.SymbolMin;
    Inst[Handle].Capability.SymbolMax        = DEMOD_Capability.SymbolMax;
    Inst[Handle].Capability.BerMax           = DEMOD_Capability.BerMax;
    Inst[Handle].Capability.SignalQualityMax = DEMOD_Capability.SignalQualityMax;
    Inst[Handle].Capability.AgcMax           = DEMOD_Capability.AgcMax;
    Inst[Handle].Capability.FECRates         = DEMOD_Capability.FECAvail;
    Inst[Handle].Capability.Modulation       = DEMOD_Capability.ModulationAvail;

    /* internal */
    Inst[Handle].Terr.Demod_Capability.FECAvail        = DEMOD_Capability.FECAvail;
    Inst[Handle].Terr.Demod_Capability.ModulationAvail = DEMOD_Capability.ModulationAvail;
    Inst[Handle].Terr.Demod_Capability.AGCControl      = DEMOD_Capability.AGCControl;
    Inst[Handle].Terr.Demod_Capability.SymbolMin       = DEMOD_Capability.SymbolMin;
    Inst[Handle].Terr.Demod_Capability.SymbolMax       = DEMOD_Capability.SymbolMax;

    Inst[Handle].Terr.Demod_Capability.BerMax           = DEMOD_Capability.BerMax;
    Inst[Handle].Terr.Demod_Capability.SignalQualityMax = DEMOD_Capability.SignalQualityMax;
    Inst[Handle].Terr.Demod_Capability.AgcMax           = DEMOD_Capability.AgcMax;


    /* ---------- kick off scan task ---------- */

    Error = TERTASK_Open(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_OPEN
        STTBX_Print(("%s fail scan task open() Error=%d\n", identity, Error));
#endif
        return(Error);
    }
#endif
/**************************MINIDRIVER**********************************
************************************************************************
*************************************************************************
**************************************************************************/
#ifdef STTUNER_MINIDRIVER
    /* ---------- open demod ---------- */
    #ifdef STTUNER_DRV_TER_STV0360
    Error = demod_d0360_Open(&Inst[Handle].Name, &DEMOD_OpenParams, &DEMOD_Capability, &Inst[Handle].Terr.Demod.DrvHandle);
    
    #endif
    Inst[Handle].Terr.Demod.Error = Error;
    
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_OPEN
        STTBX_Print(("%s fail open() demod Error=%d\n", identity, Error));
#endif
        return(Error);
    }
#ifdef STTUNER_DEBUG_MODULE_TERR_OPEN
        STTBX_Print(("%s demod #%d opened ok\n", identity, Inst[Handle].Terr.Demod.Driver->ID));
#endif
   #ifdef STTUNER_DRV_TER_STV0360
    /* ---------- open tuner ---------- */
	    Error = tuner_tuntdrv_Open(&Inst[Handle].Name, &TUNER_OpenParams, &TUNER_Capability, &Inst[Handle].Terr.Tuner.DrvHandle);
	    Inst[Handle].Terr.Tuner.Error = Error;
	    if (Error != ST_NO_ERROR)
	    {
		#ifdef STTUNER_DEBUG_MODULE_Terr_OPEN
		        STTBX_Print(("%s fail open() tuner Error=%d\n", identity, Error));
		#endif
	        return(Error);
	    }
   #endif
   Inst[Handle].Capability.Device = STTUNER_DEVICE_TERR;

 /*   Inst[Handle].Capability.DemodType   = Inst[Handle].Terr.Demod.Driver->ID;
    Inst[Handle].Capability.TunerType   = Inst[Handle].Terr.Tuner.Driver->ID;*/

    Inst[Handle].Capability.AGCControl       = DEMOD_Capability.AGCControl;
    Inst[Handle].Capability.SymbolMin        = DEMOD_Capability.SymbolMin;
    Inst[Handle].Capability.SymbolMax        = DEMOD_Capability.SymbolMax;
    Inst[Handle].Capability.BerMax           = DEMOD_Capability.BerMax;
    Inst[Handle].Capability.SignalQualityMax = DEMOD_Capability.SignalQualityMax;
    Inst[Handle].Capability.AgcMax           = DEMOD_Capability.AgcMax;
    Inst[Handle].Capability.FECRates         = DEMOD_Capability.FECAvail;
    Inst[Handle].Capability.Modulation       = DEMOD_Capability.ModulationAvail;

    /* internal */
    Inst[Handle].Terr.Demod_Capability.FECAvail        = DEMOD_Capability.FECAvail;
    Inst[Handle].Terr.Demod_Capability.ModulationAvail = DEMOD_Capability.ModulationAvail;
    Inst[Handle].Terr.Demod_Capability.AGCControl      = DEMOD_Capability.AGCControl;
    Inst[Handle].Terr.Demod_Capability.SymbolMin       = DEMOD_Capability.SymbolMin;
    Inst[Handle].Terr.Demod_Capability.SymbolMax       = DEMOD_Capability.SymbolMax;

    Inst[Handle].Terr.Demod_Capability.BerMax           = DEMOD_Capability.BerMax;
    Inst[Handle].Terr.Demod_Capability.SignalQualityMax = DEMOD_Capability.SignalQualityMax;
    Inst[Handle].Terr.Demod_Capability.AgcMax           = DEMOD_Capability.AgcMax;


    /* ---------- kick off scan task ---------- */

    Error = TERTASK_Open(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_OPEN
        STTBX_Print(("%s fail scan task open() Error=%d\n", identity, Error));

#endif
        return(Error);
    }

#endif
#ifdef STTUNER_DEBUG_MODULE_TERR_OPEN
        STTBX_Print(("%s done\n", identity));
#endif
   return(Error);

} /* STTUNER_TERR_Open() */



/* End of open.c */
