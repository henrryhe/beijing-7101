/* ----------------------------------------------------------------------------
File Name: open.c

Description: 

     This module handles the sat management functions for the STAPI level
    interface for STTUNER for Open.


Copyright (C) 1999-2001 STMicroelectronics

History:
 
   date: 20-June-2001
version: 3.1.0
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


#include "sttuner.h"                    
/* local to sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "satapi.h"     /* satellite manager API (functions in this file exported via this header) */
#include "sattask.h"    /* satellite scan task */

#define DEMOD_TYPE 1

#ifdef STTUNER_MINIDRIVER
  #include "scr.h"
  #ifdef STTUNER_DRV_SAT_STV0299
	#include "d0299.h"
	#include "tunsdrv.h"
	#ifdef STTUNER_DRV_SAT_LNBH21
	  #include "lnbh21.h"
	#elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
	  #include "lnbh24.h"
	#elif defined STTUNER_DRV_SAT_LNB21
	  #include "lnb21.h"
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
#ifndef STTUNER_MINIDRIVER
#include "stevt.h"
#include "util.h"       /* generic utility functions for sttuner */
/* driver API*/
#include "drivers.h"
#include "ioarch.h"
/******Externing from Init.c********/
extern U32 DemodHandleOne;
extern U32 DemodHandleTwo;
extern U8 DemodHandleOneFlag;
extern U8 DemodHandleTwoFlag;
/**********************************/
#endif

/********for dual test with loopthrough mode SCR application*************/
#ifdef STTUNER_DRV_SAT_SCR
#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
U8 DemodOpenFlag, LnbOpenFlag;
U32 DemodDrvHandleOne, LnbDrvHandleOne;
#endif
#endif

/* ----------------------------------------------------------------------------
Name: STTUNER_SAT_Open()

Description:
    opens sat drivers to make a tuner.
    
Parameters:
    Handle      Handle inti instance array;
   *InitParams  pointer to initialization parameters.
    
Return Value:
    ST_NO_ERROR,                    the operation was carried out without error.
    ST_ERROR_NO_MEMORY,             memory allocation failed.
    
See Also:
    STTUNER_Term()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SAT_Open(STTUNER_Handle_t Handle, const STTUNER_OpenParams_t *OpenParams)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
    const char *identity = "STTUNER open.c STTUNER_SAT_Open()";
#endif
   
    ST_ErrorCode_t Error;
    DEMOD_OpenParams_t  DEMOD_OpenParams;
    TUNER_OpenParams_t  TUNER_OpenParams;
    LNB_OpenParams_t    LNB_OpenParams;
    
    DEMOD_Capability_t  DEMOD_Capability;
    LNB_Capability_t    LNB_Capability;
    #ifndef STTUNER_MINIDRIVER
    TUNER_Capability_t  TUNER_Capability;
    #endif
    
    #ifdef STTUNER_MINIDRIVER
	    #ifdef STTUNER_DRV_SAT_STV0299
	    TUNER_Capability_t  TUNER_Capability;
	    #endif
    #endif
	
    #ifdef STTUNER_DRV_SAT_SCR
    SCR_OpenParams_t    SCR_OpenParams;
    /* ADD SCR capability if required */
    SCR_Capability_t    SCR_Capability;
    
    U8 i;
    #endif
 
    #ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
    DISEQC_OpenParams_t    DISEQC_OpenParams;
     /* ADD DISEQC capability if required */
    DISEQC_Capability_t    DISEQC_Capability;
    #endif 
   
    STTUNER_DriverDbase_t   *Drv;
    STTUNER_InstanceDbase_t *Inst;


    Drv  = STTUNER_GetDrvDb();
    Inst = STTUNER_GetDrvInst();

    /* give drivers Handle to instance that uses then so they can access the other drivers of this instance */
    LNB_OpenParams.TopLevelHandle   = Handle;
    TUNER_OpenParams.TopLevelHandle = Handle;
    DEMOD_OpenParams.TopLevelHandle = Handle;
    
    #ifdef STTUNER_DRV_SAT_SCR
    SCR_OpenParams.TopLevelHandle   = Handle;
    #endif
    
    #ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
    DISEQC_OpenParams.TopLevelHandle   = Handle;
    #endif
#ifndef STTUNER_MINIDRIVER
    /* Sequence now changed to lnb->demod->tuner */
    #ifdef STTUNER_DRV_SAT_DUAL_STEM_299 /* This is for dual stem specific open sequence otherwise #else open sequence will be used*/
    /* ---------- open lnb ---------- */
    Error = (Inst[Handle].Sat.Lnb.Driver->lnb_Open)(&Inst[Handle].Name, &LNB_OpenParams, &LNB_Capability, &Inst[Handle].Sat.Lnb.DrvHandle);
    #ifdef STTUNER_DRV_SAT_SCR
	    #ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
	    
		    if(DemodHandleOneFlag ==1 && LnbOpenFlag == 0)
		    {
			    LnbDrvHandleOne = Inst[Handle].Sat.Lnb.DrvHandle;
			    LnbOpenFlag = 1;
		    }
		    
		#endif
    #endif
		Inst[Handle].Sat.Lnb.Error = Error;
    if (Error != ST_NO_ERROR)
    {
		#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
		        STTBX_Print(("%s fail open() lnb Error=%d\n", identity, Error));
		#endif
        return(Error);
    }
	#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
	        STTBX_Print(("%s lnb #%d opened ok\n", identity, Inst[Handle].Sat.Lnb.Driver->ID));
	#endif

    /* ---------- open demod ---------- */
    Error = (Inst[Handle].Sat.Demod.Driver->demod_Open)(&Inst[Handle].Name, &DEMOD_OpenParams, &DEMOD_Capability, &Inst[Handle].Sat.Demod.DrvHandle);
    #ifdef STTUNER_DRV_SAT_SCR
		#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
		    
			if(DemodHandleOneFlag ==1 && DemodOpenFlag == 0)
			    {
				    DemodDrvHandleOne = Inst[Handle].Sat.Demod.DrvHandle;
				    DemodOpenFlag = 1;
			    }
			    
		#endif
	#endif
    Inst[Handle].Sat.Demod.Error = Error;
    if (Error != ST_NO_ERROR)
    {
		#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
		        STTBX_Print(("%s fail open() demod Error=%d\n", identity, Error));
		#endif
        return(Error);
    }
	#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
	        STTBX_Print(("%s demod #%d opened ok\n", identity, Inst[Handle].Sat.Demod.Driver->ID));
	#endif

    /* ---------- open tuner ---------- */
    Error = (Inst[Handle].Sat.Tuner.Driver->tuner_Open)(&Inst[Handle].Name, &TUNER_OpenParams, &TUNER_Capability, &Inst[Handle].Sat.Tuner.DrvHandle);
    Inst[Handle].Sat.Tuner.Error = Error;
    if (Error != ST_NO_ERROR)
    {
		#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
		       STTBX_Print(("%s fail open() tuner Error=%d\n", identity, Error));
		#endif
        return(Error);
    }
	#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
	        STTBX_Print(("%s tuner #%d opened ok\n", identity, Inst[Handle].Sat.Tuner.Driver->ID));
	#endif

/* ---------- open scr ---------- */
	#ifdef STTUNER_DRV_SAT_SCR
		if (Inst[Handle].Capability.SCREnable)
		{ 	
		    Error = (Inst[Handle].Sat.Scr.Driver->scr_Open)(&Inst[Handle].Name, &SCR_OpenParams, &Inst[Handle].Sat.Scr.DrvHandle, Inst[Handle].Sat.Demod.DrvHandle, &SCR_Capability);
		    Inst[Handle].Sat.Scr.Error = Error;
		    if (Error != ST_NO_ERROR)
		    {
				#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
		        STTBX_Print(("%s fail open() scr Error=%d\n", identity, Error));
				#endif
		        return(Error);
		    }
			#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
		    STTBX_Print(("%s scr #%d opened ok\n", identity, Inst[Handle].Sat.Scr.Driver->ID));
			#endif
	        }
	#endif


/*---------- open diseqc----------*/
#ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
		     Error = (Inst[Handle].Diseqc.Diseqc.Driver->diseqc_Open)(&Inst[Handle].Name, &DISEQC_OpenParams, &Inst[Handle].Sat.Diseqc.DrvHandle,&DISEQC_Capability);
		    Inst[Handle].Sat.Diseqc.Error = Error;
		    if (Error != ST_NO_ERROR)
		    {
			#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
		        STTBX_Print(("%s fail open() diseqc Error=%d\n", identity, Error));
			#endif
		        return(Error);
		    }
			#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
		        STTBX_Print(("%s diseqc #%d opened ok\n", identity, Inst[Handle].Sat.Diseqc.Driver->ID));
			#endif

#endif 
/************/
#else
  
     /* ---------- open lnb ---------- */
   if(Inst[Handle].Sat.Demod.Driver->ID == STTUNER_DEMOD_STB0899)
   {
  #ifdef DEMOD_TYPE
  
	Error = (Inst[Handle].Sat.Lnb.Driver->lnb_Open)(&Inst[Handle].Name, &LNB_OpenParams, &LNB_Capability, &Inst[Handle].Sat.Lnb.DrvHandle);
	#ifdef STTUNER_DRV_SAT_SCR
	#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
	if (Inst[Handle].Capability.SCREnable)
	{
		if(DemodHandleOneFlag ==1 && LnbOpenFlag == 0)
		{
		    LnbDrvHandleOne = Inst[Handle].Sat.Lnb.DrvHandle;
		    LnbOpenFlag = 1;
		}
	}
	#endif
	#endif
	Inst[Handle].Sat.Lnb.Error = Error;
	if (Error != ST_NO_ERROR)
	{
		#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
		        STTBX_Print(("%s fail open() lnb Error=%d\n", identity, Error));
		#endif
	        return(Error);
	    }
	#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
	        STTBX_Print(("%s lnb #%d opened ok\n", identity, Inst[Handle].Sat.Lnb.Driver->ID));
	#endif
        Error = (Inst[Handle].Sat.Demod.Driver->demod_Open)(&Inst[Handle].Name, &DEMOD_OpenParams, &DEMOD_Capability, &Inst[Handle].Sat.Demod.DrvHandle);
        #ifdef STTUNER_DRV_SAT_SCR
		#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
		if (Inst[Handle].Capability.SCREnable)
		{
		    
			if(DemodHandleOneFlag ==1 && DemodOpenFlag == 0)
			    {
				    DemodDrvHandleOne = Inst[Handle].Sat.Demod.DrvHandle;
				    DemodOpenFlag = 1;
			    }
		}
		#endif
		#endif
        Inst[Handle].Sat.Demod.Error = Error;
    
        if (Error != ST_NO_ERROR)
        {
		#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
		        STTBX_Print(("%s fail open() demod Error=%d\n", identity, Error));
		#endif
	        return(Error);
        }
	#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
	        STTBX_Print(("%s demod #%d opened ok\n", identity, Inst[Handle].Sat.Demod.Driver->ID));
	#endif
  #endif
  
    }
  
    else
    {
	Error = (Inst[Handle].Sat.Demod.Driver->demod_Open)(&Inst[Handle].Name, &DEMOD_OpenParams, &DEMOD_Capability, &Inst[Handle].Sat.Demod.DrvHandle);
    #ifdef STTUNER_DRV_SAT_SCR
	#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
	if (Inst[Handle].Capability.SCREnable)
	{
		if(DemodHandleOneFlag ==1 && DemodOpenFlag == 0)
		{
		    DemodDrvHandleOne = Inst[Handle].Sat.Demod.DrvHandle;
		    DemodOpenFlag = 1;
		}
	}
	#endif
	#endif
        Inst[Handle].Sat.Demod.Error = Error;
        if (Error != ST_NO_ERROR)
        {
		#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
		        STTBX_Print(("%s fail open() demod Error=%d\n", identity, Error));
		#endif
		return(Error);
        }
	#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
	        STTBX_Print(("%s demod #%d opened ok\n", identity, Inst[Handle].Sat.Demod.Driver->ID));
	#endif
        Error = (Inst[Handle].Sat.Lnb.Driver->lnb_Open)(&Inst[Handle].Name, &LNB_OpenParams, &LNB_Capability, &Inst[Handle].Sat.Lnb.DrvHandle);
        #ifdef STTUNER_DRV_SAT_SCR
		#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
		if (Inst[Handle].Capability.SCREnable)
		{
			if(DemodHandleOneFlag ==1 && LnbOpenFlag == 0)
			{
			    LnbDrvHandleOne = Inst[Handle].Sat.Lnb.DrvHandle;
			    LnbOpenFlag = 1;
			}
		}
		#endif
		#endif
       Inst[Handle].Sat.Lnb.Error = Error;
       if (Error != ST_NO_ERROR)
       {
	#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
	        STTBX_Print(("%s fail open() lnb Error=%d\n", identity, Error));
	#endif
        return(Error);
       }
	#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
	        STTBX_Print(("%s lnb #%d opened ok\n", identity, Inst[Handle].Sat.Lnb.Driver->ID));
	#endif
   }

   /* ---------- open tuner ---------- */
    Error = (Inst[Handle].Sat.Tuner.Driver->tuner_Open)(&Inst[Handle].Name, &TUNER_OpenParams, &TUNER_Capability, &Inst[Handle].Sat.Tuner.DrvHandle);
    Inst[Handle].Sat.Tuner.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
        STTBX_Print(("%s fail open() tuner Error=%d\n", identity, Error));
#endif
        return(Error);
    }
#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
        STTBX_Print(("%s tuner #%d opened ok\n", identity, Inst[Handle].Sat.Tuner.Driver->ID));
#endif
   
/* ---------- open scr ---------- */
	#ifdef STTUNER_DRV_SAT_SCR
		if (Inst[Handle].Capability.SCREnable)
		{ 
		    Error = (Inst[Handle].Sat.Scr.Driver->scr_Open)(&Inst[Handle].Name, &SCR_OpenParams, &Inst[Handle].Sat.Scr.DrvHandle, Inst[Handle].Sat.Demod.DrvHandle, &SCR_Capability);
		   
		    Inst[Handle].Sat.Scr.Error = Error;
		    if (Error != ST_NO_ERROR)
		    {
				#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
		        STTBX_Print(("%s fail open() scr Error=%d\n", identity, Error));
				#endif
		        return(Error);
		    }
			#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
		    STTBX_Print(("%s scr #%d opened ok\n", identity, Inst[Handle].Sat.Scr.Driver->ID));
			#endif
        }	
	#endif
/*------------open diseqc -----------*/
#ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
		     Error = (Inst[Handle].Sat.Diseqc.Driver->diseqc_Open)(&Inst[Handle].Name, &DISEQC_OpenParams, &Inst[Handle].Sat.Diseqc.DrvHandle,&DISEQC_Capability);
		    Inst[Handle].Sat.Diseqc.Error = Error;
		    if (Error != ST_NO_ERROR)
		    {
			#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
		        STTBX_Print(("%s fail open() diseqc Error=%d\n", identity, Error));
			#endif
		        return(Error);
		    }
			#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
		        STTBX_Print(("%s diseqc #%d opened ok\n", identity, Inst[Handle].Sat.Diseqc.Driver->ID));
			#endif

#endif 
/*********/
#endif

    /* ---------- fill capability  ---------- */

    /* API visible */
    Inst[Handle].Capability.Device = STTUNER_DEVICE_SATELLITE;

    Inst[Handle].Capability.DemodType   = Inst[Handle].Sat.Demod.Driver->ID;
    Inst[Handle].Capability.TunerType   = Inst[Handle].Sat.Tuner.Driver->ID;
    Inst[Handle].Capability.LnbType     = Inst[Handle].Sat.Lnb.Driver->ID;
    #ifdef STTUNER_DRV_SAT_SCR
    
    #endif
    if(Inst[Handle].Capability.TunerType == STTUNER_TUNER_NONE)
    {
    	Inst[Handle].Capability.FreqMin        = DEMOD_Capability.FreqMin;
    	Inst[Handle].Capability.FreqMax        = DEMOD_Capability.FreqMax;
    }
    else
    {
    	Inst[Handle].Capability.FreqMin        = TUNER_Capability.FreqMin;
    	Inst[Handle].Capability.FreqMax        = TUNER_Capability.FreqMax;
    }
        
    Inst[Handle].Capability.AGCControl       = DEMOD_Capability.AGCControl;
    Inst[Handle].Capability.SymbolMin        = DEMOD_Capability.SymbolMin;
    Inst[Handle].Capability.SymbolMax        = DEMOD_Capability.SymbolMax;
    Inst[Handle].Capability.BerMax           = DEMOD_Capability.BerMax;
    Inst[Handle].Capability.SignalQualityMax = DEMOD_Capability.SignalQualityMax;
    Inst[Handle].Capability.AgcMax           = DEMOD_Capability.AgcMax;
    Inst[Handle].Capability.FECRates         = DEMOD_Capability.FECAvail;
    Inst[Handle].Capability.Modulation       = DEMOD_Capability.ModulationAvail;

    Inst[Handle].Capability.Polarization  = LNB_Capability.PolarizationSelect;

    /* internal */
    Inst[Handle].Sat.Lnb_Capability.ShortCircuitDetect = LNB_Capability.ShortCircuitDetect;
    Inst[Handle].Sat.Lnb_Capability.PowerAvailable     = LNB_Capability.PowerAvailable;
    Inst[Handle].Sat.Lnb_Capability.PolarizationSelect = LNB_Capability.PolarizationSelect;

    Inst[Handle].Sat.Demod_Capability.FECAvail        = DEMOD_Capability.FECAvail;
    Inst[Handle].Sat.Demod_Capability.ModulationAvail = DEMOD_Capability.ModulationAvail;
    Inst[Handle].Sat.Demod_Capability.AGCControl      = DEMOD_Capability.AGCControl;
    Inst[Handle].Sat.Demod_Capability.SymbolMin       = DEMOD_Capability.SymbolMin;
    Inst[Handle].Sat.Demod_Capability.SymbolMax       = DEMOD_Capability.SymbolMax;

    Inst[Handle].Sat.Demod_Capability.BerMax           = DEMOD_Capability.BerMax;
    Inst[Handle].Sat.Demod_Capability.SignalQualityMax = DEMOD_Capability.SignalQualityMax;
    Inst[Handle].Sat.Demod_Capability.AgcMax           = DEMOD_Capability.AgcMax;
    
    /* Fill SCR capability in Inst data base */
    #ifdef STTUNER_DRV_SAT_SCR
    if (Inst[Handle].Capability.SCREnable)
    { 
        
	    Inst[Handle].Sat.Scr_Capability.Scr_App_Type       = SCR_Capability.Scr_App_Type;
	    Inst[Handle].Sat.Scr_Capability.SCRBPF             = SCR_Capability.SCRBPF;
	    Inst[Handle].Sat.Scr_Capability.Number_of_SCR      = SCR_Capability.Number_of_SCR;
	    Inst[Handle].Sat.Scr_Capability.Number_of_LNB      = SCR_Capability.Number_of_LNB;
	    Inst[Handle].Sat.Scr_Capability.LNBIndex           = SCR_Capability.LNBIndex;
	    Inst[Handle].Sat.Scr_Capability.SCREnable          = SCR_Capability.SCREnable;
	
	    Inst[Handle].Capability.Scr_App_Type               = SCR_Capability.Scr_App_Type;
	    Inst[Handle].Capability.SCRBPF                     = SCR_Capability.SCRBPF;
	    Inst[Handle].Capability.NbScr                      = SCR_Capability.Number_of_SCR;
	    Inst[Handle].Capability.NbLnb                      = SCR_Capability.Number_of_LNB;
	    Inst[Handle].Capability.LNBIndex                   = SCR_Capability.LNBIndex;
	    
	    
	    for(i=0; i<Inst[Handle].Sat.Scr_Capability.Number_of_SCR; ++i)
	    {
		Inst[Handle].Sat.Scr_Capability.SCRBPFFrequencies[i] = SCR_Capability.SCRBPFFrequencies[i];
		Inst[Handle].Capability.SCRBPFFrequencies[i] = SCR_Capability.SCRBPFFrequencies[i];
	    }
	    
	    for(i=0; i<Inst[Handle].Sat.Scr_Capability.Number_of_LNB; ++i)
	    {
		Inst[Handle].Sat.Scr_Capability.SCRLNB_LO_Frequencies[i] = SCR_Capability.SCRLNB_LO_Frequencies[i];	
		Inst[Handle].Capability.SCRLNB_LO_Frequencies[i] = SCR_Capability.SCRLNB_LO_Frequencies[i];	
	    }
    }
    #endif
    
    #ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
    Inst[Handle].Sat.Diseqc_Capability.DISEQC_Mode      = DISEQC_Capability.DISEQC_Mode;
    Inst[Handle].Sat.Diseqc_Capability.DISEQC_VER       = DISEQC_Capability.DISEQC_VER;
    #endif

    /* ---------- kick off scan task ---------- */

    Error = SATTASK_Open(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
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
    #ifdef STTUNER_DRV_SAT_STV0299
    Error = demod_d0299_Open(&Inst[Handle].Name, &DEMOD_OpenParams, &DEMOD_Capability, &Inst[Handle].Sat.Demod.DrvHandle);
    #elif defined STTUNER_DRV_SAT_STV0399  || defined(STTUNER_DRV_SAT_STV0399E)
    Error = demod_d0399_Open(&Inst[Handle].Name, &DEMOD_OpenParams, &DEMOD_Capability, &Inst[Handle].Sat.Demod.DrvHandle);
    #endif
    Inst[Handle].Sat.Demod.Error = Error;
    
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
        STTBX_Print(("%s fail open() demod Error=%d\n", identity, Error));
#endif
        return(Error);
    }
#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
        STTBX_Print(("%s demod #%d opened ok\n", identity, Inst[Handle].Sat.Demod.Driver->ID));
#endif
   #ifdef STTUNER_DRV_SAT_STV0299
    /* ---------- open tuner ---------- */
	    Error = tuner_tunsdrv_Open(&Inst[Handle].Name, &TUNER_OpenParams, &TUNER_Capability, &Inst[Handle].Sat.Tuner.DrvHandle);
	    Inst[Handle].Sat.Tuner.Error = Error;
	    if (Error != ST_NO_ERROR)
	    {
		#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
		        STTBX_Print(("%s fail open() tuner Error=%d\n", identity, Error));
		#endif
	        return(Error);
	    }
   #endif
    /* ---------- open lnb ---------- */
#ifdef STTUNER_DRV_SAT_STV0299
    #ifdef STTUNER_DRV_SAT_LNB21
    Error = lnb_lnb21_Open(&Inst[Handle].Name, &LNB_OpenParams, &LNB_Capability, &Inst[Handle].Sat.Lnb.DrvHandle);
    #elif defined STTUNER_DRV_SAT_LNBH21
    Error = lnb_lnbh21_Open(&Inst[Handle].Name, &LNB_OpenParams, &LNB_Capability, &Inst[Handle].Sat.Lnb.DrvHandle);
    #elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
    Error = lnb_lnbh24_Open(&Inst[Handle].Name, &LNB_OpenParams, &LNB_Capability, &Inst[Handle].Sat.Lnb.DrvHandle);
    #elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
    Error = lnb_lnb_demodIO_Open(&Inst[Handle].Name, &LNB_OpenParams, &LNB_Capability, &Inst[Handle].Sat.Lnb.DrvHandle);
    #endif
#elif defined STTUNER_DRV_SAT_STV0399  || defined(STTUNER_DRV_SAT_STV0399E)
    #ifdef STTUNER_DRV_SAT_LNB21
    Error = lnb_lnb21_Open(&Inst[Handle].Name, &LNB_OpenParams, &LNB_Capability, &Inst[Handle].Sat.Lnb.DrvHandle);
    #elif defined STTUNER_DRV_SAT_LNBH21
    Error = lnb_lnbh21_Open(&Inst[Handle].Name, &LNB_OpenParams, &LNB_Capability, &Inst[Handle].Sat.Lnb.DrvHandle);
    #elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
    Error = lnb_lnbh24_Open(&Inst[Handle].Name, &LNB_OpenParams, &LNB_Capability, &Inst[Handle].Sat.Lnb.DrvHandle);
    #elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
    Error = lnb_lnb_demodIO_Open(&Inst[Handle].Name, &LNB_OpenParams, &LNB_Capability, &Inst[Handle].Sat.Lnb.DrvHandle);
    #endif
#endif
    Inst[Handle].Sat.Lnb.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
        STTBX_Print(("%s fail open() lnb Error=%d\n", identity, Error));
#endif
        return(Error);
    }
#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
        STTBX_Print(("%s lnb #%d opened ok\n", identity, Inst[Handle].Sat.Lnb.Driver->ID));
#endif

/* ---------- open scr ---------- */
	#ifdef STTUNER_DRV_SAT_SCR
		   
		    Error = scr_scrdrv_Open(&Inst[Handle].Name, &SCR_OpenParams, &Inst[Handle].Sat.Scr.DrvHandle, Inst[Handle].Sat.Demod.DrvHandle, &SCR_Capability);
		   
		    Inst[Handle].Sat.Scr.Error = Error;
		    if (Error != ST_NO_ERROR)
		    {
				#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
		        STTBX_Print(("%s fail open() scr Error=%d\n", identity, Error));
				#endif
		        return(Error);
		    }
			#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
		    STTBX_Print(("%s scr #%d opened ok\n", identity, Inst[Handle].Sat.Scr.Driver->ID));
			#endif
	
	#endif
    /* ---------- fill capability  ---------- */

   #if defined(STTUNER_DRV_SAT_STV0399)  || defined(STTUNER_DRV_SAT_STV0399E)
    
    	Inst[Handle].Capability.FreqMin        = DEMOD_Capability.FreqMin;
    	Inst[Handle].Capability.FreqMax        = DEMOD_Capability.FreqMax;
   #elif  defined(STTUNER_DRV_SAT_STV0299)
    	Inst[Handle].Capability.FreqMin          = TUNER_Capability.FreqMin;
    	Inst[Handle].Capability.FreqMax          = TUNER_Capability.FreqMax;
   #endif
    Inst[Handle].Capability.SymbolMin            = DEMOD_Capability.SymbolMin;
    Inst[Handle].Capability.SymbolMax            = DEMOD_Capability.SymbolMax;
    Inst[Handle].Sat.Demod_Capability.SymbolMin       = DEMOD_Capability.SymbolMin;
    Inst[Handle].Sat.Demod_Capability.SymbolMax       = DEMOD_Capability.SymbolMax;
     
     
    /* Fill SCR capability in Inst data base */
    #ifdef STTUNER_DRV_SAT_SCR
    Inst[Handle].Sat.Scr_Capability.Scr_App_Type       = SCR_Capability.Scr_App_Type;
    Inst[Handle].Sat.Scr_Capability.SCRBPF             = SCR_Capability.SCRBPF;
    Inst[Handle].Sat.Scr_Capability.Number_of_SCR      = SCR_Capability.Number_of_SCR;
    Inst[Handle].Sat.Scr_Capability.Number_of_LNB      = SCR_Capability.Number_of_LNB;
    Inst[Handle].Sat.Scr_Capability.LNBIndex           = SCR_Capability.LNBIndex;

    Inst[Handle].Capability.Scr_App_Type               = SCR_Capability.Scr_App_Type;
    Inst[Handle].Capability.SCRBPF                     = SCR_Capability.SCRBPF;
    Inst[Handle].Capability.NbScr                      = SCR_Capability.Number_of_SCR;
    Inst[Handle].Capability.NbLnb                      = SCR_Capability.Number_of_LNB;
    Inst[Handle].Capability.LNBIndex                   = SCR_Capability.LNBIndex;
    
    for(i=0; i<Inst[Handle].Sat.Scr_Capability.Number_of_SCR; ++i)
    {
	Inst[Handle].Sat.Scr_Capability.SCRBPFFrequencies[i] = SCR_Capability.SCRBPFFrequencies[i];
	Inst[Handle].Capability.SCRBPFFrequencies[i] = SCR_Capability.SCRBPFFrequencies[i];
    }
    for(i=0; i<Inst[Handle].Sat.Scr_Capability.Number_of_LNB; ++i)
    {
	Inst[Handle].Sat.Scr_Capability.SCRLNB_LO_Frequencies[i] = SCR_Capability.SCRLNB_LO_Frequencies[i];	
	Inst[Handle].Capability.SCRLNB_LO_Frequencies[i] = SCR_Capability.SCRLNB_LO_Frequencies[i];	
    }
    #endif
    /* ---------- kick off scan task ---------- */

    Error = SATTASK_Open(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
        STTBX_Print(("%s fail scan task open() Error=%d\n", identity, Error));
#endif
        return(Error);
    }

#endif

#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
        STTBX_Print(("%s done\n", identity));
#endif
   return(Error);

} /* STTUNER_SAT_Open() */


/* End of open.c */
