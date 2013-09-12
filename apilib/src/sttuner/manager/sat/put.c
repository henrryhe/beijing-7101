/* ----------------------------------------------------------------------------
File Name: put.c

Description: 

     This module handles the sat management functions for the STAPI level
    interface for STTUNER for setting parameters.


Copyright (C) 1999-2001 STMicroelectronics

History:
 
   date: 20-June-2001
version: 3.1.0
 author: GJP from work by LW
comment: rewrite for multi-instance.
    
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
#ifdef STTUNER_MINIDRIVER
  #ifdef STTUNER_DRV_SAT_STV0299
	#include "d0299.h"
	#ifdef STTUNER_DRV_SAT_LNBH21
	  #include "lnbh21.h"
	#elif defined STTUNER_DRV_SAT_LNB21
	  #include "lnb21.h"
	#elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
	  #include "lnbh24.h"
	#else
	  #include "lnbIO.h"
	#endif
  #endif
  #if defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STV0399E)
        #include "d0399.h"
	
	#ifdef STTUNER_DRV_SAT_LNBH21
	  #include "lnbh21.h"
	#elif defined STTUNER_DRV_SAT_LNB21
	  #include "lnb21.h"
        #elif defined STTUNER_DRV_SAT_LNB24
	  #include "lnbh24.h"
	#else
	  #include "lnbIO.h"
	#endif
	
  #endif  
#endif  
#include "util.h"
#ifndef STTUNER_MINIDRIVER
   #include "stevt.h"
        /* generic utility functions for sttuner */
#endif
#include "sattask.h"    /* scan task */
#include "satapi.h"     /* satellite manager API (functions in this file exported via this header) */
#ifdef STTUNER_DRV_SAT_SCR
extern ST_ErrorCode_t scr_tone_enable(SCR_OpenParams_t  *OpenParams, DEMOD_Handle_t DemodHandle);
#endif
/* ----------------------------------------------------------------------------
Name: STTUNER_SAT_SetBandList()
Description:
    Sets the current frequency bands and associated local oscillator
    frequencies.
Parameters:
    Handle, the handle of the tuner device.
Return Value:
    ST_NO_ERROR,                the operation was carried out without error.
    ST_ERROR_INVALID_HANDLE,    the handle was invalid.
    ST_ERROR_DEVICE_BUSY,       the band/lo frequencies can not be set
                                during a scan.
    ST_ERROR_BAD_PARAMETER      Handle or BandList_p NULL 
See Also:
    STTUNER_GetBandList()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SAT_SetBandList(STTUNER_Handle_t Handle, const STTUNER_BandList_t *BandList)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
    const char *identity = "STTUNER put.c STTUNER_SAT_SetBandList()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    U32 nbytes;

    Inst = STTUNER_GetDrvInst();

    /* Check the parameters */
    if(BandList == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
        STTBX_Print(("%s fail  BandList is null\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    

    /* Check device is idle (he tuner is not currently scanning) before setting new list */
    if (Inst[Handle].TunerInfo.Status != STTUNER_STATUS_SCANNING)
    {
        /* Ensure there is enough spece for the list */
        if (Inst[Handle].Max_BandList >= BandList->NumElements)
        {

            /* Copy the supplied bank/LO frequencies into the tuner control block 
            we must ensure we lock out the scan task whilst doing this */
         
            STOS_SemaphoreWait(Inst[Handle].Sat.ScanTask.GuardSemaphore);
 

            Inst[Handle].BandList.NumElements = BandList->NumElements;

            nbytes = BandList->NumElements * sizeof(STTUNER_Band_t);

            memcpy(Inst[Handle].BandList.BandList, BandList->BandList, nbytes);
            
            STOS_SemaphoreSignal(Inst[Handle].Sat.ScanTask.GuardSemaphore);
          
        }
        else
        {
            #ifdef STTUNER_DEBUG_MODULE_SAT_PUT
            STTBX_Print(("%s fail Scan task is busy\n", identity));
	    #endif
            Error = ST_ERROR_BAD_PARAMETER;
        }
    }
    else
    {
        /* Scan task is busy */
        Error = ST_ERROR_DEVICE_BUSY;
    }

   
    return(Error);

} /* STTUNER_SAT_SetBandList() */
#ifdef STTUNER_DRV_SAT_SCR
#ifndef STTUNER_MINIDRIVER
ST_ErrorCode_t STTUNER_SAT_SetScrTone(STTUNER_Handle_t Handle)
{
	STTUNER_InstanceDbase_t *Inst;
    	SCR_OpenParams_t    SCR_OpenParams;
    	ST_ErrorCode_t Error = ST_NO_ERROR;
        Inst = STTUNER_GetDrvInst();
	SCR_OpenParams.TopLevelHandle   = Handle;
	Error = scr_tone_enable(&SCR_OpenParams, Inst[Handle].Sat.Demod.DrvHandle);
	return Error;
}
#endif
#endif
/* ----------------------------------------------------------------------------
Name: STTUNER_SAT_SetFrequency()
Description:
    Scan for an exact frequency.
Parameters:
    Handle,      handle to sttuner device
    Frequency,   required frequency to scan to
    ScanParams,  scan settings to use during scan
    Timeout,     timeout (in ms) to allow for lock
Return Value:
    ST_NO_ERROR,                the operation was carried out without error.
    ST_ERROR_BAD_PARAMETER      one or more params was invalid.
See Also:
    STTUNER_Scan()
    STTUNER_ScanContinue()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SAT_SetFrequency(STTUNER_Handle_t Handle, U32 Frequency, STTUNER_Scan_t *ScanParams, U32 Timeout)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
    const char *identity = "STTUNER put.c STTUNER_SAT_SetFrequency()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    U32  ScanFrequency;
    U32  index;
    STTUNER_Band_t   *Band;
    
    Inst = STTUNER_GetDrvInst();

    /* Check the parameters */
    if (ScanParams == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
        STTBX_Print(("%s fail ScanParams is null\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    /* Check the scan parameters are valid. Note that it does not
     make sense to allow xxx_ALL because we are scanning for an
     exact frequency.  We therefore reject the scan unless all
     parameters are exact.
    */
     /* Set requested band */
	index = ScanParams->Band;
	Band  = &Inst[Handle].BandList.BandList[index];
	 /* Ensure current frequency falls inside selected band */
#ifndef STTUNER_MINIDRIVER
        if( (Frequency >= Band->BandStart) && (Frequency <= Band->BandEnd) )
        {
            if (Band->DownLink == STTUNER_DOWNLINK_Ku)
            {
                ScanFrequency = Frequency - Band->LO;
            }
            else if (Band->DownLink == STTUNER_DOWNLINK_C)
            {
                ScanFrequency = Band->LO - Frequency;
            }
            else
            {
                return(ST_ERROR_BAD_PARAMETER);
            }                
	    if((ScanFrequency >= Inst[Handle].Capability.FreqMin) &&  (ScanFrequency <= Inst[Handle].Capability.FreqMax))
            {
            	if((ScanParams->SymbolRate >= Inst[Handle].Capability.SymbolMin) &&  (ScanParams->SymbolRate <= Inst[Handle].Capability.SymbolMax))
            	{
	            	if ( (ScanParams->Polarization == STTUNER_PLR_ALL) || (ScanParams->Modulation == STTUNER_MOD_ALL))
		    	{
		
			#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
			        STTBX_Print(("%s fail Polarization, Modulation or FECRates\n", identity));
			#endif
			        return(ST_ERROR_BAD_PARAMETER);
			    }
			
			    /* Set internal control block parameters */
			    /* ISSUE: scan +/- 3MHz from this frequency for tuner tolerence? (ioctl on/off/setvalue it) */
			    Inst[Handle].Sat.ScanInfo.FrequencyStart = Frequency;
			    Inst[Handle].Sat.ScanInfo.FrequencyEnd   = Frequency;
			    Inst[Handle].Sat.Timeout    = Timeout;
			    Inst[Handle].Sat.SingleScan = *ScanParams;
			    Inst[Handle].Sat.ScanExact  = TRUE;
			    Inst[Handle].Sat.BlindScan  = FALSE;
			
			    /* Ensure the tuner is not already scanning */
			    if (Inst[Handle].TunerInfo.Status == STTUNER_STATUS_SCANNING)
			    {
			        SATTASK_ScanAbort(Handle);      /* We must first abort the current scanning operation */
			    }
			
			    Error = SATTASK_ScanStart(Handle);  /* Commence a scan with the required parameters */
		}
		else return(ST_ERROR_BAD_PARAMETER);
             }
             else return(ST_ERROR_BAD_PARAMETER);
        }
        else return(ST_ERROR_BAD_PARAMETER);
        
#endif
#ifdef STTUNER_MINIDRIVER

if( (ScanParams->Frequency >= Band->BandStart) && (ScanParams->Frequency <= Band->BandEnd) )
        {
            if (Band->DownLink == STTUNER_DOWNLINK_Ku)
            {
                ScanFrequency = ScanParams->Frequency - Band->LO;
            }
            else if (Band->DownLink == STTUNER_DOWNLINK_C)
            {
                ScanFrequency = Band->LO - ScanParams->Frequency;
            }
            else
            {
                return(ST_ERROR_BAD_PARAMETER);
            }                
	    if((ScanFrequency >= Inst[Handle].Capability.FreqMin) &&  (ScanFrequency <= Inst[Handle].Capability.FreqMax))
            {
            	if((ScanParams->SymbolRate >= Inst[Handle].Sat.Demod_Capability.SymbolMin) &&  (ScanParams->SymbolRate <= Inst[Handle].Sat.Demod_Capability.SymbolMax))
            	{
	            	if ( (ScanParams->Polarization == STTUNER_PLR_ALL) || (ScanParams->Modulation == STTUNER_MOD_ALL))
		    	{
		
			#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
			        STTBX_Print(("%s fail Polarization, Modulation or FECRates\n", identity));
			#endif
			        return(ST_ERROR_BAD_PARAMETER);
			    }
			
			    /* Set internal control block parameters */
			    /* ISSUE: scan +/- 3MHz from this frequency for tuner tolerence? (ioctl on/off/setvalue it) */
			    Inst[Handle].Sat.ScanInfo.FrequencyStart = ScanParams->Frequency;
			    Inst[Handle].Sat.ScanInfo.FrequencyEnd   = ScanParams->Frequency;
			    Inst[Handle].Sat.Timeout    = Timeout;
			    Inst[Handle].Sat.SingleScan = *ScanParams;
			    Inst[Handle].Sat.ScanExact  = TRUE;
			
			    /* Ensure the tuner is not already scanning */
			    if (Inst[Handle].TunerInfo.Status == STTUNER_STATUS_SCANNING)
			    {
			        /*SATTASK_ScanAbort(Handle);*/      /* We must first abort the current scanning operation */
			    }
			
			    Error = SATTASK_ScanStart(Handle);  /* Commence a scan with the required parameters */
		}
		else return(ST_ERROR_BAD_PARAMETER);
             }
             else return(ST_ERROR_BAD_PARAMETER);
        }
        else return(ST_ERROR_BAD_PARAMETER);  

#endif
#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
    STTBX_Print(("%s done ok\n", identity));
#endif
    return(Error);

} /* STTUNER_SAT_SetFrequency() */



/* ----------------------------------------------------------------------------
Name: STTUNER_SAT_SetLNBConfig()
Description:
    Sets the LNB tone generator and  Sets LNBP output voltage.
Parameters:
    Handle,        handle to sttuner device
    LNBToneState,  new state for LNB tone.
Return Value:
    ST_NO_ERROR,                the operation was carried out without error.
    ST_ERROR_BAD_PARAMETER      Handle NULL or incorrect tone setting.
See Also:
    Nothing.
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SAT_SetLNBConfig (STTUNER_Handle_t Handle, STTUNER_LNBToneState_t LNBToneState, STTUNER_Polarization_t LNBVoltage)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
    const char *identity = "STTUNER put.c STTUNER_SAT_SetLNBConfig()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    LNB_Config_t LnbConfig;
    
    Inst = STTUNER_GetDrvInst();

    /* Ensure tone state is valid */
    if ( (LNBToneState != STTUNER_LNB_TONE_OFF) &&
         (LNBToneState != STTUNER_LNB_TONE_22KHZ) )
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
        STTBX_Print(("%s fail  LNBToneState not valid\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

#ifndef STTUNER_MINIDRIVER
    /* Get current settings */
    Error = (Inst[Handle].Sat.Lnb.Driver->lnb_GetConfig)(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
    if (Error == ST_NO_ERROR)
    {
        LnbConfig.ToneState = LNBToneState;
	LnbConfig.Polarization = LNBVoltage;
        Error = (Inst[Handle].Sat.Lnb.Driver->lnb_SetConfig)(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
        if (Error == ST_NO_ERROR)
        {
        }
        else
        {
#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
        STTBX_Print(("%s fail  lnb_SetConfig\n", identity));
#endif
        }

    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
        STTBX_Print(("%s fail  lnb_GetConfig\n", identity));
#endif
    }
#endif

/**********************************MINIDRIVER*********************
*******************************************************************
**********************************************************************/
#ifdef STTUNER_MINIDRIVER 

	/* Get current settings */
	#ifdef STTUNER_DRV_SAT_STV0299
	    #ifdef STTUNER_DRV_SAT_LNB21
	    Error = lnb_lnb21_GetConfig(&LnbConfig);
	    #elif defined STTUNER_DRV_SAT_LNBH21
	    Error = lnb_lnbh21_GetConfig(&LnbConfig);
	    #elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
	    Error = lnb_lnbh24_GetConfig(&LnbConfig);	    
	    #elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
	    Error = lnb_lnb_demodIO_GetConfig(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
	    #endif
	#elif defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STV0399E)
	    #ifdef STTUNER_DRV_SAT_LNB21
	    Error = lnb_lnb21_GetConfig( &LnbConfig);
	    #elif defined(STTUNER_DRV_SAT_LNBH21)
	    Error = lnb_lnbh21_GetConfig(&LnbConfig);
	    #elif defined(STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
	    Error = lnb_lnbh24_GetConfig(&LnbConfig);
	    #elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
	    Error = lnb_lnb_demodIO_GetConfig(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
	    #endif
	#endif
    if (Error == ST_NO_ERROR)
	    {
	        LnbConfig.ToneState = LNBToneState;
			LnbConfig.Polarization = LNBVoltage;
	     #ifdef STTUNER_DRV_SAT_STV0299
		    #ifdef STTUNER_DRV_SAT_LNB21
			Error = lnb_lnb21_SetConfig( &LnbConfig);
			#elif defined(STTUNER_DRV_SAT_LNBH21)
			Error = lnb_lnbh21_SetConfig(&LnbConfig);
			#elif defined(STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
			Error = lnb_lnbh24_SetConfig(&LnbConfig);
			#elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
			Error = lnb_lnb_demodIO_SetConfig(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
			#endif
		#elif defined STTUNER_DRV_SAT_STV0399
		    #ifdef STTUNER_DRV_SAT_LNB21
			Error = lnb_lnb21_SetConfig( &LnbConfig);
			#elif defined STTUNER_DRV_SAT_LNBH21
			Error = lnb_lnbh21_SetConfig( &LnbConfig);
			#elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
			Error = lnb_lnbh24_SetConfig( &LnbConfig);
			#elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
			Error = lnb_lnb_demodIO_SetConfig(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
			#endif
		#endif      
	
	    }
    	    else
	    {
	#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
	        STTBX_Print(("%s fail  lnb_GetConfig\n", identity));
	#endif
           }

#endif


    return(Error);

} /* STTUNER_SAT_SetLNBConfig() */
/* ----------------------------------------------------------------------------
Name: STTUNER_SAT_DiSEqCSendReceive
Description:
    Sends the DiSEqc commands appropriate driver for further processing
Parameters:
    Handle,        handle to sttuner device
    *pDiSEqCSendPacket,  The command & data bytes to be sent.
Return Value:
    ST_NO_ERROR,                the operation was carried out without error.
    ST_ERROR_BAD_PARAMETER      Handle NULL or incorrect tone setting.
See Also:
    Nothing.
---------------------------------------------------------------------------- */

ST_ErrorCode_t STTUNER_SAT_DiSEqCSendReceive (STTUNER_Handle_t Handle, 
									   STTUNER_DiSEqCSendPacket_t *pDiSEqCSendPacket,
									   STTUNER_DiSEqCResponsePacket_t *pDiSEqCResponsePacket
									   )


{
#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
    const char *identity = "STTUNER put.c STTUNER_SAT_DiSEqCSendReceive()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    LNB_Config_t LnbConfig;
	STTUNER_DiSEqCConfig_t DiSEqCConfig;
	STTUNER_LNBToneState_t pre_tonestatus=STTUNER_LNB_TONE_OFF;
	
    Inst = STTUNER_GetDrvInst();
/*** get current LNB tone state*/
#ifndef STTUNER_MINIDRIVER
	
    Error = (Inst[Handle].Sat.Lnb.Driver->lnb_GetConfig)(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
	
   if(Error == ST_NO_ERROR)
   {
   	    /* Before sending any DiSEqC command make sure that LNB power is "ON" */
	    if(LnbConfig.Status == LNB_STATUS_OFF)
   		{
   			LnbConfig.Status = LNB_STATUS_ON;
   		}
        if( LnbConfig.ToneState == STTUNER_LNB_TONE_DEFAULT)
	    {

	         (Inst[Handle].Sat.Demod.Driver->demod_GetConfigDiSEqC)( Inst[Handle].Sat.Demod.DrvHandle,&DiSEqCConfig);
	 		 if(DiSEqCConfig.ToneState==STTUNER_DiSEqC_TONE_CONTINUOUS)
			 {
			    /******Stop the continuous state but which mode to be taken*************/
				(Inst[Handle].Sat.Demod.Driver->demod_SetDiSEqCBurstOFF )
				( Inst[Handle].Sat.Demod.DrvHandle);
				
				
			 } 
					 
	      }
		  else if(LnbConfig.ToneState == STTUNER_LNB_TONE_22KHZ)/* incase,continuous tone from LNBP is ON ; stop it and wait for 15ms  */
		  {
					 pre_tonestatus = STTUNER_LNB_TONE_22KHZ;
			         LnbConfig.ToneState = STTUNER_LNB_TONE_OFF;										
		  }/** end of if ((LnbConfig.ToneState == STTUNER_LNB_TONE_22KHZ)||(LnbConfig.ToneState == STTUNER_LNB_TONE_OFF)) */		  
				/*enable tone from Demod ic*/
		
	}
	else
	{ 

      #ifdef STTUNER_DEBUG_MODULE_SAT_PUT
			STTBX_Print(("%s fail  lnb_GetConfig\n", identity));
	  #endif
	}

	STOS_SemaphoreWait(Inst[Handle].Sat.ScanTask.GuardSemaphore);
	Error = (Inst[Handle].Sat.Lnb.Driver->lnb_SetConfig)(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
	
     if (Error != ST_NO_ERROR)	
	 {			
	 	STOS_SemaphoreSignal(Inst[Handle].Sat.ScanTask.GuardSemaphore);       
	    return (Error);
	 }
 	DiSEqC_Delay_ms(15);
	#ifndef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
		Error = (Inst[Handle].Sat.Demod.Driver->demod_DiSEqC)( Inst[Handle].Sat.Demod.DrvHandle, pDiSEqCSendPacket, 
			                                                 pDiSEqCResponsePacket);
	#else
	       Error = (Inst[Handle].Sat.Diseqc.Driver->diseqc_SendReceive)( Inst[Handle].Sat.Diseqc.DrvHandle, pDiSEqCSendPacket, 
			                                                 pDiSEqCResponsePacket);
	#endif 
    DiSEqC_Delay_ms(15);
	STOS_SemaphoreSignal(Inst[Handle].Sat.ScanTask.GuardSemaphore);
    if(pre_tonestatus == STTUNER_LNB_TONE_22KHZ)
	{
		LnbConfig.ToneState = STTUNER_LNB_TONE_22KHZ;
		Error = (Inst[Handle].Sat.Lnb.Driver->lnb_SetConfig)(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
		
	}
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
        STTBX_Print(("%s fail  lnb_Diseqc\n", identity));
#endif  
	}
	
#endif


/*******************STTUNER  MINIDRIVER****************************************************
******************************************************************
*****************************************************************/

#ifdef STTUNER_MINIDRIVER
	
    /* Get current settings */
	#ifdef STTUNER_DRV_SAT_STV0299
	    #ifdef STTUNER_DRV_SAT_LNB21
	    Error = lnb_lnb21_GetConfig(&LnbConfig);
	    #elif defined STTUNER_DRV_SAT_LNBH21
	    Error = lnb_lnbh21_GetConfig(&LnbConfig);
	    #elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
	    Error = lnb_lnbh24_GetConfig(&LnbConfig);
	    #elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
	    Error = lnb_lnb_demodIO_GetConfig(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
	    #endif
	#elif defined STTUNER_DRV_SAT_STV0399  || defined(STTUNER_DRV_SAT_STV0399E)
	    #ifdef STTUNER_DRV_SAT_LNB21
	    Error = lnb_lnb21_GetConfig(&LnbConfig);
	    #elif defined STTUNER_DRV_SAT_LNBH21
	    Error = lnb_lnbh21_GetConfig(&LnbConfig);
	    #elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
	    Error = lnb_lnbh24_GetConfig(&LnbConfig);
	    #elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
	    Error = lnb_lnb_demodIO_GetConfig(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
	    #endif
	#endif
	
   if(Error == ST_NO_ERROR)
   {
   	    /* Before sending any DiSEqC command make sure that LNB power is "ON" */
	 if(LnbConfig.Status == LNB_STATUS_OFF)
	 {
		LnbConfig.Status = LNB_STATUS_ON;
		/* Get current settings */
		#ifdef STTUNER_DRV_SAT_STV0299
		    #ifdef STTUNER_DRV_SAT_LNB21
		    Error = lnb_lnb21_SetConfig(&LnbConfig);
		    #elif defined STTUNER_DRV_SAT_LNBH21
		    Error = lnb_lnbh21_SetConfig(&LnbConfig);
		    #elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
		    Error = lnb_lnbh24_SetConfig(&LnbConfig);
		    #elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
		    Error = lnb_lnb_demodIO_SetConfig(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
		    #endif
		#elif defined STTUNER_DRV_SAT_STV0399  || defined(STTUNER_DRV_SAT_STV0399E)
		    #ifdef STTUNER_DRV_SAT_LNB21
		    Error = lnb_lnb21_SetConfig(&LnbConfig);
		    #elif defined STTUNER_DRV_SAT_LNBH21
		    Error = lnb_lnbh21_SetConfig(&LnbConfig);
		    #elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
		    Error = lnb_lnbh24_SetConfig(&LnbConfig);
		    #elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
		    Error = lnb_lnb_demodIO_SetConfig(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
		    #endif
		#endif
	 }
         if( LnbConfig.ToneState == STTUNER_LNB_TONE_DEFAULT)
	  	 {
	         #ifdef STTUNER_DRV_SAT_STV0299
	         demod_d0299_DiSEqCGetConfig( Inst[Handle].Sat.Demod.DrvHandle,&DiSEqCConfig);
	         #elif defined STTUNER_DRV_SAT_STV0399
	         demod_d0399_DiSEqCGetConfig( Inst[Handle].Sat.Demod.DrvHandle,&DiSEqCConfig);
	         #endif
	 		 if(DiSEqCConfig.ToneState==STTUNER_DiSEqC_TONE_CONTINUOUS)
			 {
			    /******Stop the continuous state but which mode to be taken*************/
				#ifdef STTUNER_DRV_SAT_STV0299
				demod_d0299_DiSEqCBurstOFF( Inst[Handle].Sat.Demod.DrvHandle);
				#elif defined STTUNER_DRV_SAT_STV0399  || defined(STTUNER_DRV_SAT_STV0399E)
				demod_d0399_DiSEqCBurstOFF( Inst[Handle].Sat.Demod.DrvHandle);
				#endif
				DiSEqC_Delay_ms(15);
			 } 
					 
	  }
	  else 
	  {
					
	      if ((LnbConfig.ToneState == STTUNER_LNB_TONE_22KHZ)||(LnbConfig.ToneState == STTUNER_LNB_TONE_OFF))/* incase,continuous tone from LNBP is ON ; stop it and wait for 15ms  */
		  {
		
		      if(LnbConfig.ToneState == STTUNER_LNB_TONE_22KHZ)
			  {
		
		         LnbConfig.ToneState = STTUNER_LNB_TONE_OFF;
	                #ifdef STTUNER_DRV_SAT_STV0299
			    #ifdef STTUNER_DRV_SAT_LNB21
			    Error = lnb_lnb21_SetConfig(&LnbConfig);
			    #elif defined STTUNER_DRV_SAT_LNBH21
			    Error = lnb_lnbh21_SetConfig(&LnbConfig);
			    #elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
			    Error = lnb_lnbh24_SetConfig(&LnbConfig);
			    #elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
			    Error = lnb_lnb_demodIO_SetConfig(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
			    #endif
			#elif defined STTUNER_DRV_SAT_STV0399  || defined(STTUNER_DRV_SAT_STV0399E)
			    #ifdef STTUNER_DRV_SAT_LNB21
			    Error = lnb_lnb21_SetConfig(&LnbConfig);
			    #elif defined STTUNER_DRV_SAT_LNBH21
			    Error = lnb_lnbh21_SetConfig(&LnbConfig);
			    #elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
			    Error = lnb_lnbh24_SetConfig(&LnbConfig);
			    #elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
			    Error = lnb_lnb_demodIO_SetConfig(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
			    #endif
			#endif
				
		         if (Error == ST_NO_ERROR)	
				 {
			        DiSEqC_Delay_ms(15);
					
				 }
			     else
				 {
				    #ifdef STTUNER_DEBUG_MODULE_SAT_PUT
						      STTBX_Print(("%s fail  lnb_SetConfig\n", identity));
				    #endif
			        return (Error);
				 }
			}
					
					/*****Take the lnb to the default state*********/
            LnbConfig.ToneState = STTUNER_LNB_TONE_DEFAULT;
	            #ifdef STTUNER_DRV_SAT_STV0299
			    #ifdef STTUNER_DRV_SAT_LNB21
			    Error = lnb_lnb21_SetConfig(&LnbConfig);
			    #elif defined STTUNER_DRV_SAT_LNBH21
			    Error = lnb_lnbh21_SetConfig(&LnbConfig);
			    #elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
			    Error = lnb_lnbh24_SetConfig(&LnbConfig);
			    #elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
			    Error = lnb_lnb_demodIO_SetConfig(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
			    #endif
			#elif defined STTUNER_DRV_SAT_STV0399  || defined(STTUNER_DRV_SAT_STV0399E)
			    #ifdef STTUNER_DRV_SAT_LNB21
			    Error = lnb_lnb21_SetConfig(&LnbConfig);
			    #elif defined STTUNER_DRV_SAT_LNBH21
			    Error = lnb_lnbh21_SetConfig(&LnbConfig);
			    #elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
			    Error = lnb_lnbh24_SetConfig(&LnbConfig);
			    #elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
			    Error = lnb_lnb_demodIO_SetConfig(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
			    #endif
			#endif
				
		    if (Error != ST_NO_ERROR)	
			{
			   	#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
						STTBX_Print(("%s fail  lnb_SetConfig\n", identity));
			    #endif
			    return (Error);
			}					
		  }/** end of if ((LnbConfig.ToneState == STTUNER_LNB_TONE_22KHZ)||(LnbConfig.ToneState == STTUNER_LNB_TONE_OFF)) */		  
				/*enable tone from Demod ic*/
		}/**end of else of if( LnbConfig.ToneState == STTUNER_LNB_TONE_DEFAULT) **/
	}
	else
	{ 

      #ifdef STTUNER_DEBUG_MODULE_SAT_PUT
			STTBX_Print(("%s fail  lnb_GetConfig\n", identity));
	  #endif
	}
	
	/*set LNB tone state to default enable tone from 299 through PCF8574*/
	/*if using LSTEM proceed further else report error*/
	/*start sending diseqc command*/
		
	#if defined(ST_OS21) || defined(ST_OSLINUX)
		semaphore_wait(Inst[Handle].Sat.ScanTask.GuardSemaphore);
	#else
		semaphore_wait(&Inst[Handle].Sat.ScanTask.GuardSemaphore);
	#endif	
		#ifdef STTUNER_DRV_SAT_STV0299
		Error = demod_d0299_DiSEqC( Inst[Handle].Sat.Demod.DrvHandle, pDiSEqCSendPacket, 
			                                                 pDiSEqCResponsePacket);
                #elif defined STTUNER_DRV_SAT_STV0399  || defined(STTUNER_DRV_SAT_STV0399E)
                Error = demod_d0399_DiSEqC( Inst[Handle].Sat.Demod.DrvHandle, pDiSEqCSendPacket, 
			                                                 pDiSEqCResponsePacket);
                #endif
	#if defined(ST_OS21) || defined(ST_OSLINUX)	
		semaphore_signal(Inst[Handle].Sat.ScanTask.GuardSemaphore);
	#else
		semaphore_signal(&Inst[Handle].Sat.ScanTask.GuardSemaphore);
	#endif	
	 		
#endif
    
    return(Error);

} /* STTUNER_SAT_DiSEqCSendReceive() */

/***************************************************************************************************/


#ifndef STTUNER_MINIDRIVER
/* --------

------------------------------------------------------------------
Name: STTUNER_SAT_StandByMode()
Description:
    To put satellite device in Standby Mode or out of it .
Parameters:
    Handle,        handle to sttuner device
    StandByMode,     Indicate in which mode the user want the 
                     device .
    
Return Value:
    ST_NO_ERROR,                the operation was carried out without error.
    STTUNER_ERROR_UNSPECIFIED   when task_suspend and task_resume does not
                                occur succesfully
See Also:
    
---------------------------------------------------------------------------- */

ST_ErrorCode_t STTUNER_SAT_StandByMode(STTUNER_Handle_t Handle, STTUNER_StandByMode_t PowerMode)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
    const char *identity = "STTUNER put.c STTUNER_SAT_StandByMode()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    int task_state = 0;
   
    Inst = STTUNER_GetDrvInst();
   
    STOS_SemaphoreWait(Inst[Handle].Sat.ScanTask.GuardSemaphore);
    
    if (PowerMode == STTUNER_STANDBY_POWER_MODE)
    {
       if (Inst[Handle].Task_Run == 0 )
       {
       #ifdef DEBUG_STANDBYMODE
       STTBX_Print(("\n Scan task has been suspended \n"));
       #endif
        Error  = (Inst[Handle].Sat.Demod.Driver->demod_StandByMode)(Inst[Handle].Sat.Demod.DrvHandle,STTUNER_STANDBY_POWER_MODE);
        if(Error != ST_NO_ERROR)
        {
        	STOS_SemaphoreSignal(Inst[Handle].Sat.ScanTask.GuardSemaphore);
        	return(Error);
        }  
                  
       Inst[Handle].PreStatus = Inst[Handle].TunerInfo.Status;
       Inst[Handle].PowerMode=STTUNER_STANDBY_POWER_MODE;
         
       Inst[Handle].TunerInfo.Status = STTUNER_STATUS_STANDBY ; 
       
       STOS_SemaphoreSignal(Inst[Handle].Sat.ScanTask.GuardSemaphore);
      
       if(task_state == 0)
       {
          Inst[Handle].Task_Run = 1;
       }
       else
       {
          Error = STTUNER_ERROR_UNSPECIFIED;
          return(Error);
       }
       #ifdef DEBUG_STANDBYMODE
       STTBX_Print(("\n The value of task_state for task_suspend is <%d> \n",task_state));
       #endif
       
       
       }
       else
       {
       	 #ifdef DEBUG_STANDBYMODE
          STTBX_Print(("\n Task already suspended \n"));
         #endif
       }
       
    }
    else if (PowerMode == STTUNER_NORMAL_POWER_MODE)
    {
      
       if (Inst[Handle].Task_Run == 1)
       {
       #ifdef DEBUG_STANDBYMODE
       STTBX_Print(("\n Scan task has been resumed \n"));
       #endif
       Error  = (Inst[Handle].Sat.Demod.Driver->demod_StandByMode)(Inst[Handle].Sat.Demod.DrvHandle,STTUNER_NORMAL_POWER_MODE);
         if(Error != ST_NO_ERROR)
        {
        	STOS_SemaphoreSignal(Inst[Handle].Sat.ScanTask.GuardSemaphore);
        	return(Error);
        }   
        Inst[Handle].PowerMode=STTUNER_NORMAL_POWER_MODE;
        
         
         if(Inst[Handle].PreStatus == STTUNER_STATUS_IDLE)
         {
           Inst[Handle].TunerInfo.Status = STTUNER_STATUS_IDLE;
         }
         else
         {
            Inst[Handle].TunerInfo.Status = STTUNER_STATUS_SCANNING ; 
         }  
          STOS_SemaphoreSignal(Inst[Handle].Sat.ScanTask.GuardSemaphore);
             
       #ifdef DEBUG_STANDBYMODE
        STTBX_Print(("\n The value of task_state for task_resume is <%d> \n",task_state));
       #endif
       if(task_state == 0)
       {
          Inst[Handle].Task_Run = 0;
       }
       else
       {
          Error = STTUNER_ERROR_UNSPECIFIED;
          return(Error);
       }
       
       }
       else
       {
       	#ifdef DEBUG_STANDBYMODE
          STTBX_Print(("\n Task already resumed \n"));
        #endif
       }
    }
    
   
   
   
     if (Error != ST_NO_ERROR)
     {
        return(Error);
     }
   STOS_TaskDelayUs(500000);/*Descheduling point*/
    
     STOS_SemaphoreWait(Inst[Handle].Sat.ScanTask.GuardSemaphore);
     
    
     #ifdef DEBUG_STANDBYMODE
          STTBX_Print(("\n Try to Sync scan task with main task after Scan task resume \n"));
     #endif
    
     STOS_SemaphoreSignal(Inst[Handle].Sat.ScanTask.GuardSemaphore);
     
#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
    STTBX_Print(("%s done ok\n", identity));
#endif
    
    return(Error);

} /* STTUNER_SAT_StandByMode() */
#endif




#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
ame: STTUNER_SAT_SetScanList()
Description:
    Sets the scanning criteria for a tuner scan.
Parameters:
    Handle,    the handle of the tuner device.
    ScanList,  pointer to a scan list reflecting the scanning criteria.
Return Value:
    ST_NO_ERROR,                the operation was carried out without error.
    ST_ERROR_INVALID_HANDLE,    the handle was invalid.
    ST_ERROR_DEVICE_BUSY,       the tuner is scanning, we do not allow
                                the parameters to be set during a scan.
    ST_ERROR_BAD_PARAMETER      Handle or ScanList_p NULL 
See Also:
    STTUNER_GetScanList()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SAT_SetScanList(STTUNER_Handle_t Handle, const STTUNER_ScanList_t *ScanList)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
    const char *identity = "STTUNER put.c STTUNER_SAT_SetScanList()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    U32 nbytes;


    Inst = STTUNER_GetDrvInst();
 
    /* Check the parameters */
    if (ScanList == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
        STTBX_Print(("%s fail ScanList is null\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check device is idle before setting new list */
    if (Inst[Handle].TunerInfo.Status != STTUNER_STATUS_SCANNING)
    {
        /* Ensure the tuner is not currently scanning */
        if (Inst[Handle].Max_ScanList >= ScanList->NumElements)
        {
            /* Copy the supplied scan list into the tuner control block */
        
            STOS_SemaphoreWait(Inst[Handle].Sat.ScanTask.GuardSemaphore);
      

            Inst[Handle].ScanList.NumElements = ScanList->NumElements;

            nbytes = ScanList->NumElements * sizeof(STTUNER_Scan_t);

            memcpy(Inst[Handle].ScanList.ScanList, ScanList->ScanList, nbytes);
            
            STOS_SemaphoreSignal(Inst[Handle].Sat.ScanTask.GuardSemaphore);

        }
        else
        {
            Error = ST_ERROR_BAD_PARAMETER; /* Number of elements exceeds maximum allowed */
        }
    }
    else
    {
        Error = ST_ERROR_DEVICE_BUSY;   /* Scan task is busy */
    }


#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
    STTBX_Print(("%s done ok\n", identity));
#endif

 
    return(Error);

} /* STTUNER_SAT_SetScanList() */


#endif
#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: STTUNER_SAT_SetThresholdList()
Description:
    Sets the current threshold list for checking the signal quality value.
    The caller-specificed callback may be invoked if the signal threshold
    changes position in the threshold list.
Parameters:
    Handle,         the handle of the tuner device.
    ThresholdList,  pointer to a threshold list for signal checking.
    QualityFormat,  units of measurement used in threshold list values.
Return Value:
    ST_NO_ERROR,                the operation was carried out without error.
    ST_ERROR_INVALID_HANDLE,    the handle was invalid.
    ST_ERROR_BAD_PARAMETER      Handle or ThresholdList_p NULL 
See Also:
    STTUNER_GetThresholdList()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SAT_SetThresholdList(STTUNER_Handle_t Handle, const STTUNER_ThresholdList_t *ThresholdList, STTUNER_QualityFormat_t QualityFormat)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
    const char *identity = "STTUNER put.c STTUNER_SAT_SetThresholdList()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    U32 nbytes;
    

    Inst = STTUNER_GetDrvInst();

    /* Check the parameters */
    if (ThresholdList == NULL)
    {
	#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
	        STTBX_Print(("%s fail ThresholdList is null\n", identity));
	#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    if ( (QualityFormat != STTUNER_QUALITY_BER) && 
         (QualityFormat != STTUNER_QUALITY_CN)  )
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_PUT
        STTBX_Print(("%s fail QualityFormat not valid\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    /* Check device is idle before setting new list */
    if (Inst[Handle].TunerInfo.Status != STTUNER_STATUS_SCANNING)
    {
        if (Inst[Handle].Max_ThresholdList >= ThresholdList->NumElements)
        {
            /* Copy the supplied threshold list into the tuner control block, also flag
             the tuner to begin signal checking at this point. */
	
	            STOS_SemaphoreWait(Inst[Handle].Sat.ScanTask.GuardSemaphore);
	

            Inst[Handle].ThresholdList.NumElements = ThresholdList->NumElements;

            nbytes = ThresholdList->NumElements * sizeof(STTUNER_SignalThreshold_t);

            memcpy(Inst[Handle].ThresholdList.ThresholdList, ThresholdList->ThresholdList, nbytes);

            Inst[Handle].QualityFormat = QualityFormat;
	
	            STOS_SemaphoreSignal(Inst[Handle].Sat.ScanTask.GuardSemaphore);
	
        }
        else
        {
            Error = ST_ERROR_BAD_PARAMETER; /* Number of elements exceeds maximum allowed */
        }
    }
    else
    {
        Error = ST_ERROR_DEVICE_BUSY;   /* Scan task is busy */
    }


    return(Error);

} /* STTUNER_SAT_SetThresholdList() */
#endif
#ifndef STTUNER_MINIDRIVER
#ifdef STTUNER_DRV_SAT_SCR
/* ----------------------------------------------------------------------------
Name: STTUNER_SAT_UBCHANGE()

Description:
   
Parameters:
   
Return Value:

See Also:
 
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SAT_UBCHANGE(STTUNER_Handle_t Handle,STTUNER_SCRBPF_t SCR)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
    const char *identity = "STTUNER open.c STTUNER_SAT_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;

    Inst = STTUNER_GetDrvInst();
    /* API visible */
    Inst[Handle].Sat.Scr_Capability.SCRBPF      = SCR;
    Inst[Handle].Capability.SCRBPF              = SCR;
    return(Error);

}

ST_ErrorCode_t STTUNER_SAT_SCRPARAM_UPDATE(STTUNER_Handle_t Handle,STTUNER_SCRBPF_t SCR,U16* SCRBPFFrequencies)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_OPEN
    const char *identity = "STTUNER open.c STTUNER_SAT_Open()";
#endif

SCR_OpenParams_t    SCR_OpenParams;

    U8 i;
    ST_ErrorCode_t Error;

 
    STTUNER_DriverDbase_t   *Drv;
    STTUNER_InstanceDbase_t *Inst;


    Drv  = STTUNER_GetDrvDb();
    Inst = STTUNER_GetDrvInst();
    
   SCR_OpenParams.TopLevelHandle   = Handle;

    /*IFA new feature*/
    /*switch off currently used SatCR before switching to the other one*/
    Error = (Inst[Handle].Sat.Scr.Driver->scr_Off)(&SCR_OpenParams, Inst[Handle].Sat.Demod.DrvHandle,Inst[Handle].Capability.SCRBPF);
    if (Error != ST_NO_ERROR)
    {
        return(Error);
    }
    /*end IFA*/
  


    /* ---------- fill capability  ---------- */

    /* API visible */
    
    Inst[Handle].Sat.Scr_Capability.SCRBPF      = SCR;
    Inst[Handle].Capability.SCRBPF              = SCR;
    for(i=0;i<Inst[Handle].Capability.NbScr;i++) {
    	Inst[Handle].Capability.SCRBPFFrequencies[i]=SCRBPFFrequencies[i];
    	Inst[Handle].Sat.Scr_Capability.SCRBPFFrequencies[i]=SCRBPFFrequencies[i];
    }
    return(Error);

} /* STTUNER_SAT_Open() */

#endif
#endif
/* End of put.c */


