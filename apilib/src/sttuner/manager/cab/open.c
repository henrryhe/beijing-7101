/* ----------------------------------------------------------------------------
File Name: open.c

Description: 

     This module handles the cable management functions for the STAPI level
     interface for STTUNER for Open.

Copyright (C) 1999-2001 STMicroelectronics

   date: 01-October-2001
version: 3.2.0
 author: from STV0299 and MB validation drivers.
comment: Write for multi-instance/multi-FrontEnd.

Revision History:
    
Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
--------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */

#ifndef ST_OSLINUX
   
#include <string.h>
#endif


#include "stlite.h"     /* Standard includes */


/* STAPI */
#include "sttbx.h"

/*#include "stevt.h"*/
#include "sttuner.h"                    

/* local to sttuner */
/*#include "util.h"*/       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "cabapi.h"     /* cable manager API (functions in this file exported via this header) */
#include "cabtask.h"    /* cable scan task */

/* driver API*/
#include "drivers.h"
#include "ioarch.h"



/* ----------------------------------------------------------------------------
Name: STTUNER_CABLE_Open()

Description:
    opens cable drivers to make a tuner.
    
Parameters:
    Handle      Handle inti instance array;
   *InitParams  pointer to initialization parameters.
    
Return Value:
    ST_NO_ERROR,                    the operation was carried out without error.
    ST_ERROR_NO_MEMORY,             memory allocation failed.
    
See Also:
    STTUNER_Term()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_CABLE_Open(STTUNER_Handle_t Handle, const STTUNER_OpenParams_t *OpenParams)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_OPEN
    const char *identity = "STTUNER open.c STTUNER_CABLE_Open()";
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

    /* ---------- open demod ---------- */
    Error = (Inst[Handle].Cable.Demod.Driver->demod_Open)(&Inst[Handle].Name, &DEMOD_OpenParams, &DEMOD_Capability, &Inst[Handle].Cable.Demod.DrvHandle);
   
    Inst[Handle].Cable.Demod.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_OPEN
        STTBX_Print(("%s fail open() demod Error=%d\n", identity, Error));
#endif
        return(Error);
    }
#ifdef STTUNER_DEBUG_MODULE_CAB_OPEN
        STTBX_Print(("%s demod #%d opened ok\n", identity, Inst[Handle].Cable.Demod.Driver->ID));
#endif

    /* ---------- open tuner ---------- */
    Error = (Inst[Handle].Cable.Tuner.Driver->tuner_Open)(&Inst[Handle].Name, &TUNER_OpenParams, &TUNER_Capability, &Inst[Handle].Cable.Tuner.DrvHandle);
    
    Inst[Handle].Cable.Tuner.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_OPEN
        STTBX_Print(("%s fail open() tuner Error=%d\n", identity, Error));
#endif
        return(Error);
    }
#ifdef STTUNER_DEBUG_MODULE_CAB_OPEN
        STTBX_Print(("%s tuner #%d opened ok\n", identity, Inst[Handle].Cable.Tuner.Driver->ID));
#endif

    /* ---------- fill capability  ---------- */

    /* API visible */
    Inst[Handle].Capability.Device = STTUNER_DEVICE_CABLE;

    Inst[Handle].Capability.DemodType   = Inst[Handle].Cable.Demod.Driver->ID;
    Inst[Handle].Capability.TunerType   = Inst[Handle].Cable.Tuner.Driver->ID;
    Inst[Handle].Capability.AGCControl       = DEMOD_Capability.AGCControl;
    Inst[Handle].Capability.SymbolMin        = DEMOD_Capability.SymbolMin;
    Inst[Handle].Capability.SymbolMax        = DEMOD_Capability.SymbolMax;
    Inst[Handle].Capability.BerMax           = DEMOD_Capability.BerMax;
    Inst[Handle].Capability.SignalQualityMax = DEMOD_Capability.SignalQualityMax;
    Inst[Handle].Capability.AgcMax           = DEMOD_Capability.AgcMax;
    Inst[Handle].Capability.FECRates         = DEMOD_Capability.FECAvail;
    Inst[Handle].Capability.Modulation       = DEMOD_Capability.ModulationAvail;
    Inst[Handle].Capability.J83              = DEMOD_Capability.J83Avail;

    /* internal */
    Inst[Handle].Cable.Demod_Capability.FECAvail        = DEMOD_Capability.FECAvail;
    Inst[Handle].Cable.Demod_Capability.ModulationAvail = DEMOD_Capability.ModulationAvail;
    Inst[Handle].Cable.Demod_Capability.AGCControl      = DEMOD_Capability.AGCControl;
    Inst[Handle].Cable.Demod_Capability.SymbolMin       = DEMOD_Capability.SymbolMin;
    Inst[Handle].Cable.Demod_Capability.SymbolMax       = DEMOD_Capability.SymbolMax;

    Inst[Handle].Cable.Demod_Capability.BerMax           = DEMOD_Capability.BerMax;
    Inst[Handle].Cable.Demod_Capability.SignalQualityMax = DEMOD_Capability.SignalQualityMax;
    Inst[Handle].Cable.Demod_Capability.AgcMax           = DEMOD_Capability.AgcMax;

    Inst[Handle].Cable.SingleScan.Spectrum               = STTUNER_INVERSION_AUTO;

    /* ---------- kick off scan task ---------- */

    Error = CABTASK_Open(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_OPEN
        STTBX_Print(("%s fail scan task open() Error=%d\n", identity, Error));
#endif
        return(Error);
    }

#ifdef STTUNER_DEBUG_MODULE_CAB_OPEN
        STTBX_Print(("%s done\n", identity));
#endif

   return(Error);

} /* STTUNER_CABLE_Open() */



/* End of open.c */
