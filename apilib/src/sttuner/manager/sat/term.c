/* ----------------------------------------------------------------------------
File Name: term.c

Description: 

     This module handles the term() interface for satellite.


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
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "satapi.h"     /* satellite manager API (functions in this file exported via this header) */
#include "stapitnr.h"
#include "chip.h"
/* driver API*/
#include "drivers.h"
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

#ifndef STTUNER_MINIDRIVER
/**********Global Variable externed for getting Tuner Handle**********/
extern U32 TunerHandleOne;
extern U32 TunerHandleTwo;
/***************************************************************/
/*********Global Variable externed for getting LNB Handle*********/
extern U32 LnbHandleOne;
extern U32 LnbHandleTwo;
/********Global Variable externed for getting Demod Handle************/
extern U32 DemodHandleOne;
extern U32 DemodHandleTwo;
extern U8 DemodHandleOneFlag;
extern U8 DemodHandleTwoFlag;
/*********************************************************************/
#endif
/**********Global Variable externed for getting Scr Handle**********/
#ifdef STTUNER_DRV_SAT_SCR
	extern U32 ScrHandleOne;
	extern U32 ScrHandleTwo;
	extern U8 DemodOpenFlag, LnbOpenFlag;
#endif
/***************************************************************/
/* ----------------------------------------------------------------------------
Name: STTUNER_SAT_Term()

Description:
    terms all sat devices.
    
Parameters:
    DeviceName  STTUNER global instance name device name as set during initialization.
   *InitParams  pointer to initialization parameters.
    
Return Value:
    ST_NO_ERROR,                    the operation was carried out without error.
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SAT_Term(STTUNER_Handle_t Handle, const STTUNER_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
    const char *identity = "STTUNER term.c STTUNER_SAT_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    DEMOD_TermParams_t  DEMOD_TermParams;
    LNB_TermParams_t    LNB_TermParams;
    
    #ifdef STTUNER_DRV_SAT_SCR
    SCR_TermParams_t    SCR_TermParams;
    #endif
    
    #ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
    DISEQC_TermParams_t DISEQC_TermParams;
    #endif 

    STTUNER_InstanceDbase_t *Inst;

#ifndef STTUNER_MINIDRIVER
    TUNER_TermParams_t  TUNER_TermParams;
    Inst = STTUNER_GetDrvInst();

    /* ---------- term lnb driver ---------- */
    LNB_TermParams.ForceTerminate = TermParams->ForceTerminate;

    Error = (Inst[Handle].Sat.Lnb.Driver->lnb_Term)(&Inst[Handle].Name, &LNB_TermParams);
    Inst[Handle].Sat.Lnb.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
        STTBX_Print(("%s fail lnb term() Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
        STTBX_Print(("%s lnb term() ok\n", identity));
#endif
    }

    /* ---------- term tuner driver ---------- */
    TUNER_TermParams.ForceTerminate = TermParams->ForceTerminate;

    Error = (Inst[Handle].Sat.Tuner.Driver->tuner_Term)(&Inst[Handle].Name, &TUNER_TermParams);
    Inst[Handle].Sat.Tuner.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
        STTBX_Print(("%s fail tuner term() Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
        STTBX_Print(("%s tuner term() ok\n", identity));
#endif
    }

    /* ---------- term demod driver ---------- */
    DEMOD_TermParams.ForceTerminate = TermParams->ForceTerminate;

    Error = (Inst[Handle].Sat.Demod.Driver->demod_Term)(&Inst[Handle].Name, &DEMOD_TermParams);
    Inst[Handle].Sat.Demod.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
        STTBX_Print(("%s fail demod term() Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
        STTBX_Print(("%s demod term() ok\n", identity));
#endif
    }

#ifdef STTUNER_DRV_SAT_SCR
/* ---------- term scr driver ---------- */
    if(Inst[Handle].Capability.SCREnable) 
    {
	    SCR_TermParams.ForceTerminate = TermParams->ForceTerminate;
	
	    Error = (Inst[Handle].Sat.Scr.Driver->scr_Term)(&Inst[Handle].Name, &SCR_TermParams);
	    Inst[Handle].Sat.Scr.Error = Error;
	    if (Error != ST_NO_ERROR)
	    {
		#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
		        STTBX_Print(("%s fail scr term() Error=%d\n", identity, Error));
		#endif
	        return(Error);
	    }
	    else
	    {
		#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
		        STTBX_Print(("%s scr term() ok\n", identity));
		#endif
	    }
	    
    }
#endif

/*---------- term diseqc driver ---------*/     
#ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
    DISEQC_TermParams.ForceTerminate = TermParams->ForceTerminate;

    Error = (Inst[Handle].Sat.Diseqc.Driver->diseqc_Term)(&Inst[Handle].Name, &DISEQC_TermParams);
    Inst[Handle].Sat.Diseqc.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
        STTBX_Print(("%s fail diseqc term() Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
        STTBX_Print(("%s diseqc term() ok\n", identity));
#endif
    }
#endif
/********/
    /* ---------- close lnb I/O ---------- */
    Error = ChipClose(Inst[Handle].Sat.Lnb.IOHandle);
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
    STTBX_Print(("%s close lnb handle %d\n", identity, Inst[Handle].Sat.Lnb.IOHandle));
#endif
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
        STTBX_Print(("%s fail close lnb I/O Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
        STTBX_Print(("%s close lnb I/O ok\n", identity));
#endif
    }
    /******Reset Global handle variables for lnb***********/
    if(Inst[Handle].Sat.Lnb.IOHandle==LnbHandleOne)
    {
       LnbHandleOne=0;
    }
    else if(Inst[Handle].Sat.Lnb.IOHandle==LnbHandleTwo)
    {
       LnbHandleTwo=0;
    }
    /* ---------- close tuner I/O ---------- */
    Error = ChipClose(Inst[Handle].Sat.Tuner.IOHandle);
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
    STTBX_Print(("%s close tuner handle %d\n", identity, Inst[Handle].Sat.Tuner.IOHandle));
#endif
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
        STTBX_Print(("%s fail close tuner I/O Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
        STTBX_Print(("%s close tuner I/O ok\n", identity));
#endif
    }
    /******Reset Global handle variables for tuner***********/
    if(Inst[Handle].Sat.Tuner.IOHandle==TunerHandleOne)
    {
       TunerHandleOne=0;
    }
    else if(Inst[Handle].Sat.Tuner.IOHandle==TunerHandleTwo)
    {
    	TunerHandleTwo=0;
    }
    /* ---------- close demod I/O ---------- */
    Error = ChipClose(Inst[Handle].Sat.Demod.IOHandle);
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
    STTBX_Print(("%s close demod handle %d\n", identity, Inst[Handle].Sat.Demod.IOHandle));
#endif
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
        STTBX_Print(("%s fail close demod I/O Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
        STTBX_Print(("%s close demod I/O ok\n", identity));
#endif
    }
    /******Reset Global handle variables for demod***********/
    if((Inst[Handle].Sat.Demod.IOHandle==DemodHandleOne) && (DemodHandleOneFlag==1))
    {
       DemodHandleOneFlag=0;
       DemodHandleOne=0;
       
       #ifdef STTUNER_DRV_SAT_SCR
	        #ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
		       DemodOpenFlag = 0;
		       LnbOpenFlag = 0;
	       #endif
       #endif
    }
    else if((Inst[Handle].Sat.Demod.IOHandle==DemodHandleTwo) && (DemodHandleTwoFlag==1))
    {
       DemodHandleTwoFlag=0;
       DemodHandleTwo=0;
    }

/* ---------- close scr I/O ---------- */
#ifdef STTUNER_DRV_SAT_SCR
	if(Inst[Handle].Capability.SCREnable) 
	{
	    Error = ChipClose(Inst[Handle].Sat.Scr.IOHandle);
		#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
	    STTBX_Print(("%s close scr handle %d\n", identity, Inst[Handle].Sat.Scr.IOHandle));
		#endif
	    if (Error != ST_NO_ERROR)
	    {
		#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
	        STTBX_Print(("%s fail close scr I/O Error=%d\n", identity, Error));
		#endif
	        return(Error);
	    }
	    else
	    {
		#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
	        STTBX_Print(("%s close scr I/O ok\n", identity));
		#endif
	    }
	    /******Reset Global handle variables for scr***********/
	    if(Inst[Handle].Sat.Scr.IOHandle==ScrHandleOne)
	    {
	       ScrHandleOne=0;
	    }
	    else if(Inst[Handle].Sat.Scr.IOHandle==ScrHandleTwo)
	    {
	    	ScrHandleTwo=0;
	    }
	    
	    Inst[Handle].Sat.Scr.Driver    = NULL;
	    Inst[Handle].Capability.SCREnable = FALSE;
    }
#endif

/* -------- close diseqc I/O -------*/
#ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
	
	#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
	    STTBX_Print(("%s close diseqc handle %d\n", identity, Inst[Handle].Sat.Diseqc.IOHandle));
        #endif
	    if (Error != ST_NO_ERROR)
	    {
		#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
	        STTBX_Print(("%s fail close diseqc I/O Error=%d\n", identity, Error));
		#endif
	        return(Error);
	    }
	    else
	    {
		#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
	        STTBX_Print(("%s close diseqc I/O ok\n", identity));
		#endif
	    }
	    
	    Inst[Handle].Sat.Diseqc.Driver    = NULL;
#endif 
/********/
    /* ---------- point entries away from driver database ---------- */

    Inst[Handle].Sat.Lnb.Driver    = NULL;
    Inst[Handle].Sat.Tuner.Driver  = NULL;
    Inst[Handle].Sat.Demod.Driver  = NULL;
#endif

#ifdef STTUNER_MINIDRIVER
    #ifdef STTUNER_DRV_SAT_STV0299
    TUNER_TermParams_t  TUNER_TermParams;
    #endif
    Inst = STTUNER_GetDrvInst();

    /* ---------- term lnb driver ---------- */
    LNB_TermParams.ForceTerminate = TermParams->ForceTerminate;
    #ifdef STTUNER_DRV_SAT_STV0299
	    #ifdef STTUNER_DRV_SAT_LNB21
	    Error = lnb_lnb21_Term(&Inst[Handle].Name, &LNB_TermParams);
	    #elif defined STTUNER_DRV_SAT_LNBH21
	    Error = lnb_lnbh21_Term(&Inst[Handle].Name, &LNB_TermParams);
	    #elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
	    Error = lnb_lnbh24_Term(&Inst[Handle].Name, &LNB_TermParams);
	    #elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
	    Error = lnb_lnb_demodIO_Term(&Inst[Handle].Name, &LNB_TermParams);
	    #endif
    #elif defined STTUNER_DRV_SAT_STV0399  || defined(STTUNER_DRV_SAT_STV0399E)
	    #ifdef STTUNER_DRV_SAT_LNB21
	    Error = lnb_lnb21_Term(&Inst[Handle].Name, &LNB_TermParams);
	    #elif defined STTUNER_DRV_SAT_LNBH21
	    Error = lnb_lnbh21_Term(&Inst[Handle].Name, &LNB_TermParams);
	    #elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
	    Error = lnb_lnbh24_Term(&Inst[Handle].Name, &LNB_TermParams);
	    #elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
	    Error = lnb_lnb_demodIO_Term(&Inst[Handle].Name, &LNB_TermParams);
	    #endif
    #endif
    Inst[Handle].Sat.Lnb.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
        STTBX_Print(("%s fail lnb term() Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
        STTBX_Print(("%s lnb term() ok\n", identity));
#endif
    }
#ifdef STTUNER_DRV_SAT_STV0299
	    /* ---------- term tuner driver ---------- */
	    TUNER_TermParams.ForceTerminate = TermParams->ForceTerminate;
	
	    Error = tuner_tunsdrv_Term(&Inst[Handle].Name, &TUNER_TermParams);
	    Inst[Handle].Sat.Tuner.Error = Error;
	    if (Error != ST_NO_ERROR)
	    {
		#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
		        STTBX_Print(("%s fail tuner term() Error=%d\n", identity, Error));
		#endif
	        return(Error);
	    }
	    else
	    {
		#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
		        STTBX_Print(("%s tuner term() ok\n", identity));
		#endif
	    }
#endif
    /* ---------- term demod driver ---------- */
    DEMOD_TermParams.ForceTerminate = TermParams->ForceTerminate;
    #ifdef STTUNER_DRV_SAT_STV0299
    Error = demod_d0299_Term(&Inst[Handle].Name, &DEMOD_TermParams);
    #elif defined(STTUNER_DRV_SAT_STV0399)  ||  defined(STTUNER_DRV_SAT_STV0399E)
    Error = demod_d0399_Term(&Inst[Handle].Name, &DEMOD_TermParams);
    #endif
    
    Inst[Handle].Sat.Demod.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
        STTBX_Print(("%s fail demod term() Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
        STTBX_Print(("%s demod term() ok\n", identity));
#endif
    }

#ifdef STTUNER_DRV_SAT_SCR
/* ---------- term scr driver ---------- */
	    SCR_TermParams.ForceTerminate = TermParams->ForceTerminate;
	
	    Error = scr_scrdrv_Term(&Inst[Handle].Name, &SCR_TermParams);
	    Inst[Handle].Sat.Scr.Error = Error;
	    if (Error != ST_NO_ERROR)
	    {
	#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
	        STTBX_Print(("%s fail scr term() Error=%d\n", identity, Error));
	#endif
	        return(Error);
	    }
	    else
	    {
	#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
	        STTBX_Print(("%s scr term() ok\n", identity));
	#endif
	     }
#endif

    /* ---------- close lnb I/O ---------- */
    #if defined(STTUNER_DRV_SAT_LNB21) || defined(STTUNER_DRV_SAT_LNBH21) || defined(STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
    Error = STTUNER_IOLNB_Close(&Inst[Handle].Sat.Lnb.IOHandle, &IOARCH_CloseParams);

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
        STTBX_Print(("%s fail close lnb I/O Error=%d\n", identity, Error));
#endif
        return(Error);
    }
   #endif
    /* ---------- close demod I/O ---------- */
    Error = STTUNER_IODEMOD_Close(&Inst[Handle].Sat.Demod.IOHandle, &IOARCH_CloseParams);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_TERM
        STTBX_Print(("%s fail close demod I/O Error=%d\n", identity, Error));
#endif
        return(Error);
    }
#endif    


    return(Error);

} /* STTUNER_SAT_Term() */



/* End of term.c */
