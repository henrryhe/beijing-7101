/* ----------------------------------------------------------------------------
File Name: close.c

Description: 

     This module handles the cab management functions for the STAPI level
     interface for STTUNER for Close.

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
/* Standard includes */
#include "stlite.h"

/* STAPI */
#include "sttbx.h"

#include "stevt.h"
#include "sttuner.h"                    

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "cabtask.h"    /* cable scan task */
#include "cabapi.h"     /* cable manager API (functions in this file exported via this header) */

/* driver API*/
#include "drivers.h"
#include "ioarch.h"


/* ----------------------------------------------------------------------------
Name: STTUNER_CABLE_Close()

Description:
    close cable drivers.
    
Parameters:
    Handle      index into instance array;
   *CloseParams  pointer to closing parameters.
    
Return Value:
    ST_NO_ERROR the operation was carried out without error.
    
See Also:
    STTUNER_CABLE_Open()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_CABLE_Close(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_CLOSE
    const char *identity = "STTUNER close.c STTUNER_CABLE_Close()";
#endif
    ST_ErrorCode_t Error;

    DEMOD_CloseParams_t DEMOD_CloseParams;
    TUNER_CloseParams_t TUNER_CloseParams;

    STTUNER_InstanceDbase_t *Inst;


    Inst = STTUNER_GetDrvInst();

    /* ---------- kill scan task ---------- */
    Error = CABTASK_Close(Handle);

    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_CLOSE
        STTBX_Print(("%s scan task closed ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_CLOSE
        STTBX_Print(("%s fail scan task close\n", identity));
#endif
    }   /* keep going */


    /* ---------- erase capability (API visible) ---------- */
    Inst[Handle].Capability.Device = STTUNER_DEVICE_NONE;

    Inst[Handle].Capability.DemodType = STTUNER_NO_DRIVER;
    Inst[Handle].Capability.TunerType = STTUNER_NO_DRIVER;

    Inst[Handle].Capability.FECRates     = 0; 
    Inst[Handle].Capability.AGCControl   = 0;
    Inst[Handle].Capability.SymbolMin    = 0;
    Inst[Handle].Capability.SymbolMax    = 0;
    Inst[Handle].Capability.BerMax       = 0;
    Inst[Handle].Capability.SignalQualityMax = 0;
    Inst[Handle].Capability.AgcMax       = 0;
    Inst[Handle].Capability.Polarization = 0;

    /* internal */

    Inst[Handle].Cable.Demod_Capability.FECAvail        = 0;
    Inst[Handle].Cable.Demod_Capability.ModulationAvail = 0;
    Inst[Handle].Cable.Demod_Capability.J83Avail        = 0;
    Inst[Handle].Cable.Demod_Capability.AGCControl      = 0;
    Inst[Handle].Cable.Demod_Capability.SymbolMin       = 0;
    Inst[Handle].Cable.Demod_Capability.SymbolMax       = 0;

    Inst[Handle].Cable.Demod_Capability.BerMax           = 0;
    Inst[Handle].Cable.Demod_Capability.SignalQualityMax = 0;
    Inst[Handle].Cable.Demod_Capability.AgcMax           = 0;

    /* ---------- close tuner driver ---------- */
    Error = (Inst[Handle].Cable.Tuner.Driver->tuner_Close)(Inst[Handle].Cable.Tuner.DrvHandle, &TUNER_CloseParams);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_CLOSE
        STTBX_Print(("%s fail close() tuner driver Error=%d\n", identity, Error));
#endif
    }


    /* ---------- close demod driver ---------- */
    Error = (Inst[Handle].Cable.Demod.Driver->demod_Close)(Inst[Handle].Cable.Demod.DrvHandle, &DEMOD_CloseParams);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_CLOSE
        STTBX_Print(("%s fail close() demod driver Error=%d\n", identity, Error));
#endif
    }


#ifdef STTUNER_DEBUG_MODULE_CAB_CLOSE
    STTBX_Print(("%s done\n", identity));
#endif

   return(Error);

} /* STTUNER_CABLE_Close() */



/* End of close.c */
