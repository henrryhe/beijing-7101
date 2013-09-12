/* ----------------------------------------------------------------------------
File Name: close.c

Description: 

     This module handles the sat management functions for the STAPI level
    interface for STTUNER for Close.


Copyright (C) 1999-2001 STMicroelectronics

History:
 
   date: 20-June-2001
version: 4.0.0
 author: GJP from work by LW
comment: rewrite for multi-instance.

date: 07-April-2004
version: 
 author: SD 
comment: Add support for SatCR.
    
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

#include "sattask.h"    /* satellite scan task */
#include "satapi.h"     /* satellite manager API (functions in this file exported via this header) */

/* driver API*/
#ifndef STTUNER_MINIDRIVER
#include "drivers.h"
#include "ioarch.h"
#endif
#ifdef STTUNER_MINIDRIVER
  #include "scr.h"
  #ifdef STTUNER_DRV_SAT_STV0299
	#include "d0299.h"
	#include "tunsdrv.h"
	#ifdef STTUNER_DRV_SAT_LNBH21
	  #include "lnbh21.h"
	#elif defined STTUNER_DRV_SAT_LNB21
	  #include "lnb21.h"
	#elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
	  #include "lnbh24.h"
	#else
	  #include "lnbIO.h"
	#endif
	#include "iodirect.h"
  #endif
  #if defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STV0399E)
        #include "d0399.h"
	#ifdef STTUNER_DRV_SAT_LNBH21
	  #include "lnbh21.h"
	#elif defined STTUNER_DRV_SAT_LNB21
	  #include "lnb21.h"
	#elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
	  #include "lnbh24.h"
	#else
	  #include "lnbIO.h"
	#endif
	#include "iodirect.h"
  #endif
  
#endif


/* ----------------------------------------------------------------------------
Name: STTUNER_SAT_Close()

Description:
    close sat drivers to make a tuner.
    
Parameters:
    Handle      index into instance array;
   *CloseParams  pointer to closing parameters.
    
Return Value:
    ST_NO_ERROR the operation was carried out without error.
    
See Also:
    STTUNER_SAT_Open()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SAT_Close(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_CLOSE
    const char *identity = "STTUNER close.c STTUNER_SAT_Close()";
#endif
    ST_ErrorCode_t Error;

    DEMOD_CloseParams_t DEMOD_CloseParams;
    
    LNB_CloseParams_t   LNB_CloseParams;
    #ifdef STTUNER_DRV_SAT_SCR
	    SCR_CloseParams_t SCR_CloseParams;
	    #ifndef STTUNER_MINIDRIVER
	    U8 i;
	    #endif
    #endif
    #ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
    DISEQC_CloseParams_t DISEQC_CloseParams;
    #endif

    STTUNER_InstanceDbase_t *Inst;

#ifndef STTUNER_MINIDRIVER
    TUNER_CloseParams_t TUNER_CloseParams;
    Inst = STTUNER_GetDrvInst();

    /* ---------- kill scan task ---------- */
    Error = SATTASK_Close(Handle);

    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_CLOSE
        STTBX_Print(("%s scan task closed ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_CLOSE
        STTBX_Print(("%s fail scan task close\n", identity));
#endif
    }   /* keep going */
    /* ---------- erase capability (API visible) ---------- */
    Inst[Handle].Capability.Device = STTUNER_DEVICE_NONE;

    Inst[Handle].Capability.DemodType = STTUNER_NO_DRIVER;
    Inst[Handle].Capability.TunerType = STTUNER_NO_DRIVER;
    Inst[Handle].Capability.LnbType   = STTUNER_NO_DRIVER;
    #ifdef STTUNER_DRV_SAT_SCR
    Inst[Handle].Capability.Scr_App_Type   = STTUNER_NO_DRIVER;
    #endif
    Inst[Handle].Capability.FECRates     = 0; 
    Inst[Handle].Capability.AGCControl   = 0;
    Inst[Handle].Capability.SymbolMin    = 0;
    Inst[Handle].Capability.SymbolMax    = 0;
    Inst[Handle].Capability.BerMax       = 0;
    Inst[Handle].Capability.SignalQualityMax = 0;
    Inst[Handle].Capability.AgcMax       = 0;
    Inst[Handle].Capability.Polarization = 0;

    /* internal */
    Inst[Handle].Sat.Lnb_Capability.ShortCircuitDetect = 0;
    Inst[Handle].Sat.Lnb_Capability.PowerAvailable     = 0;
    Inst[Handle].Sat.Lnb_Capability.PolarizationSelect = 0;

    Inst[Handle].Sat.Demod_Capability.FECAvail        = 0;
    Inst[Handle].Sat.Demod_Capability.ModulationAvail = 0;
    Inst[Handle].Sat.Demod_Capability.AGCControl      = 0;
    Inst[Handle].Sat.Demod_Capability.SymbolMin       = 0;
    Inst[Handle].Sat.Demod_Capability.SymbolMax       = 0;

    Inst[Handle].Sat.Demod_Capability.BerMax           = 0;
    Inst[Handle].Sat.Demod_Capability.SignalQualityMax = 0;
    Inst[Handle].Sat.Demod_Capability.AgcMax           = 0;
    
    /*  cleaning SCR capability  */
    #ifdef STTUNER_DRV_SAT_SCR
    if(Inst[Handle].Capability.SCREnable)
    {   
	    for(i=0; i<Inst[Handle].Sat.Scr_Capability.Number_of_SCR; ++i)
	    {
		Inst[Handle].Sat.Scr_Capability.SCRBPFFrequencies[i] = 0;
		Inst[Handle].Capability.SCRBPFFrequencies[i] = 0;
	    }
	    for(i=0; i<Inst[Handle].Sat.Scr_Capability.Number_of_LNB; ++i)
	    {
		Inst[Handle].Sat.Scr_Capability.SCRLNB_LO_Frequencies[i] = 0;	
		Inst[Handle].Capability.SCRLNB_LO_Frequencies[i] = 0;	
	    }
	    
	    Inst[Handle].Sat.Scr_Capability.Number_of_SCR      = 0;
	    Inst[Handle].Sat.Scr_Capability.Number_of_LNB      = 0;
	    Inst[Handle].Sat.Scr_Capability.SCR_Mode           = 0;
	    Inst[Handle].Sat.Scr_Capability.LegacySupport      = 0;
	    Inst[Handle].Sat.Scr_Capability.LNBIndex      = 0;
	  	Inst[Handle].Sat.Scr_Capability.SCREnable          = 0;
	    Inst[Handle].Capability.NbScr      = 0;
	    Inst[Handle].Capability.NbLnb      = 0;
	    Inst[Handle].Capability.SCRBPF     = 0;
	    Inst[Handle].Capability.LNBIndex   = 0;
    }
    #endif
    
    #ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
    Inst[Handle].Sat.Diseqc_Capability.DISEQC_Mode      = 0;
    Inst[Handle].Sat.Diseqc_Capability.DISEQC_VER      = 0;
    #endif

    /* ---------- close lnb driver ---------- */
    Error = (Inst[Handle].Sat.Lnb.Driver->lnb_Close)(Inst[Handle].Sat.Lnb.DrvHandle, &LNB_CloseParams);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_CLOSE
        STTBX_Print(("%s fail close() lnb driver Error=%d\n", identity, Error));
#endif
    }


    /* ---------- close tuner driver ---------- */
    Error = (Inst[Handle].Sat.Tuner.Driver->tuner_Close)(Inst[Handle].Sat.Tuner.DrvHandle, &TUNER_CloseParams);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_CLOSE
        STTBX_Print(("%s fail close() tuner driver Error=%d\n", identity, Error));
#endif
    }


    /* ---------- close demod driver ---------- */
    Error = (Inst[Handle].Sat.Demod.Driver->demod_Close)(Inst[Handle].Sat.Demod.DrvHandle, &DEMOD_CloseParams);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_CLOSE
        STTBX_Print(("%s fail close() demod driver Error=%d\n", identity, Error));
#endif
    }
#ifdef STTUNER_DRV_SAT_SCR
 /* ---------- close scr driver ---------- */
  if(Inst[Handle].Capability.SCREnable)
  {
    Error = (Inst[Handle].Sat.Scr.Driver->scr_Close)(Inst[Handle].Sat.Scr.DrvHandle, &SCR_CloseParams);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_CLOSE
        STTBX_Print(("%s fail close() scr driver Error=%d\n", identity, Error));
#endif
    }
    
  }
#endif
#ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
/*--------- close diseqc -------*/
     Error = (Inst[Handle].Sat.Diseqc.Driver->diseqc_Close)(Inst[Handle].Sat.Diseqc.DrvHandle, &DISEQC_CloseParams);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_CLOSE
        STTBX_Print(("%s fail close() scr driver Error=%d\n", identity, Error));
#endif
    }
#endif
/******/

#ifdef STTUNER_DEBUG_MODULE_SAT_CLOSE
    STTBX_Print(("%s done\n", identity));
#endif
#endif

#ifdef STTUNER_MINIDRIVER
  #ifdef STTUNER_DRV_SAT_STV0299
  TUNER_CloseParams_t TUNER_CloseParams;
  #endif
    Inst = STTUNER_GetDrvInst();
    /* ---------- kill scan task ---------- */
    Error = SATTASK_Close(Handle);
    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_CLOSE
        STTBX_Print(("%s scan task closed ok\n", identity));
#endif
    }
        
    Inst[Handle].Sat.Demod_Capability.SymbolMin       = 0;
    Inst[Handle].Sat.Demod_Capability.SymbolMax       = 0;
#ifdef STTUNER_DRV_SAT_STV0299    
    Inst[Handle].Sat.Tuner_Capability.FreqMin       = 0;
    Inst[Handle].Sat.Tuner_Capability.FreqMax       = 0;
#endif
    /*  cleaning SCR capability  */
    #ifdef STTUNER_DRV_SAT_SCR
    Inst[Handle].Sat.Scr_Capability.Number_of_SCR      = 0;
    Inst[Handle].Sat.Scr_Capability.Number_of_LNB      = 0;
    Inst[Handle].Sat.Scr_Capability.SCR_Mode           = 0;
    Inst[Handle].Sat.Scr_Capability.LegacySupport      = 0;
    #endif
    /* ---------- close lnb driver ---------- */
    #ifdef STTUNER_DRV_SAT_STV0299
	    #ifdef STTUNER_DRV_SAT_LNB21
	    Error = lnb_lnb21_Close(Inst[Handle].Sat.Lnb.DrvHandle, &LNB_CloseParams);
	    #elif defined STTUNER_DRV_SAT_LNBH21
	    Error = lnb_lnbh21_Close(Inst[Handle].Sat.Lnb.DrvHandle, &LNB_CloseParams);
	    #elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
	    Error = lnb_lnbh24_Close(Inst[Handle].Sat.Lnb.DrvHandle, &LNB_CloseParams);
	    #elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
	    Error = lnb_lnb_demodIO_Close(Inst[Handle].Sat.Lnb.DrvHandle, &LNB_CloseParams);
	    #endif 
	    /* ---------- close tuner driver ---------- */
    	    Error |= tuner_tunsdrv_Close(Inst[Handle].Sat.Tuner.DrvHandle, &TUNER_CloseParams);
    
    		/* ---------- close demod driver ---------- */
            Error |= demod_d0299_Close(Inst[Handle].Sat.Demod.DrvHandle, &DEMOD_CloseParams);
    #elif defined STTUNER_DRV_SAT_STV0399  || defined(STTUNER_DRV_SAT_STV0399E)
     	    #ifdef STTUNER_DRV_SAT_LNB21
	    Error = lnb_lnb21_Close(Inst[Handle].Sat.Lnb.DrvHandle, &LNB_CloseParams);
	    #elif defined STTUNER_DRV_SAT_LNBH21
	    Error = lnb_lnbh21_Close(Inst[Handle].Sat.Lnb.DrvHandle, &LNB_CloseParams);
	    #elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
	    Error = lnb_lnbh24_Close(Inst[Handle].Sat.Lnb.DrvHandle, &LNB_CloseParams);
	    #elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
	    Error = lnb_lnb_demodIO_Close(Inst[Handle].Sat.Lnb.DrvHandle, &LNB_CloseParams);
	    #endif
	    /* ---------- close demod driver ---------- */
            Error |= demod_d0399_Close(Inst[Handle].Sat.Demod.DrvHandle, &DEMOD_CloseParams);
     #endif 
#ifdef STTUNER_DRV_SAT_SCR
 /* ---------- close scr driver ---------- */
    Error |= scr_scrdrv_Close(Inst[Handle].Sat.Scr.DrvHandle, &SCR_CloseParams);
#endif  

	#ifdef STTUNER_DEBUG_MODULE_SAT_CLOSE
	    STTBX_Print(("%s done\n", identity));
	#endif
#endif
   return(Error);

} /* STTUNER_SAT_Close() */



/* End of close.c */
