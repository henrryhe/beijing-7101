/* ----------------------------------------------------------------------------
File Name: sdrv.c

Description: 


Copyright (C) 1999-2001 STMicroelectronics

History:
 
   date: 18-June-2001
version: 3.1.0
 author: GJP
comment: write for multi-instance.
    
Reference:

    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */



/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
  
/* C libs */
#include <string.h>
#endif
#include "stlite.h"     /* Standard includes */

/* STAPI */
#include "sttbx.h"

#include "stevt.h"
#include "sttuner.h"                    

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "sdrv.h"       /* header for this file */

/* null */
#ifdef STTUNER_DRV_SAT_NULL
 #include "lnone.h"
#endif

#ifdef STTUNER_LNB_POWER_THRU_DEMODIO
  #include"lnbIO.h"
#endif

#ifdef STTUNER_LNB_POWER_THRU_BACKENDGPIO
#include "lnbBEIO.h"
#endif

	/* STV0299 chip */
	#ifdef STTUNER_DRV_SAT_STV0299
	 #include "d0299.h"
	 #include "lstem.h"
	#endif
	
	/* STV0399 chip */
	#ifdef STTUNER_DRV_SAT_STV0399
	 #include "d0399.h"
	#endif

	#ifdef STTUNER_DRV_SAT_STB0899
	 #include "d0899.h"
	#endif
	
	#ifdef STTUNER_DRV_SAT_STX0288
	 #include "d0288.h"
	#endif


/* LNB21 */
#ifdef STTUNER_DRV_SAT_LNB21
 #include "lnb21.h"
#endif
/* LNBH21 */
#ifdef STTUNER_DRV_SAT_LNBH21
 #include "lnbh21.h"
#endif

/* LNBH24 */
#if defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23) 
 #include "lnbh24.h"
#endif




/* external tuners */
#if defined(STTUNER_DRV_SAT_STV0299) || defined (STTUNER_DRV_SAT_STB0899) || defined (STTUNER_DRV_SAT_STX0288)
 #include "tunsdrv.h"
#endif

#ifdef STTUNER_DRV_SAT_SCR
#include "scr.h"
#endif

#ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
#include "diseqcdrv.h"
#endif

/* private variables ------------------------------------------------------- */

/* memory area to store driver database */
static ST_Partition_t *SDRV_MemoryPartition;
static STTUNER_TunerType_t SDRV_TunerType;


/* local functions --------------------------------------------------------- */

ST_ErrorCode_t SDRV_AllocMem(ST_Partition_t *MemoryPartition);
ST_ErrorCode_t SDRV_FreeMem (ST_Partition_t *MemoryPartition);

ST_ErrorCode_t SDRV_InstallDev  (U32 DeviceTypeID, U32 DatabaseIndex);
ST_ErrorCode_t SDRV_UnInstallDev(U32 DeviceTypeID);


/* ----------------------------------------------------------------------------
Name: STTUNER_SAT_DRV_Install()

Description:
    install a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SAT_DRV_Install(SDRV_InstallParams_t *InstallParams)
{
#ifdef STTUNER_DEBUG_MODULE_SDRV
    const char *identity = "STTUNER sdrv.c STTUNER_SAT_DRV_Install()";
#endif
    U32 dbindex;
    ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STTUNER_DEBUG_MODULE_SDRV
    STTBX_Print(("%s install BEGIN\n", identity));
#endif

    SDRV_MemoryPartition = InstallParams->MemoryPartition;
    SDRV_TunerType = InstallParams->TunerType;

    /* allocate memory for drivers in this build */
    Error = SDRV_AllocMem(SDRV_MemoryPartition);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SDRV
        STTBX_Print(("%s fail SDRV_AllocMem()\n", identity));
#endif
        return(Error);
    }

    /* install drivers in their databases */

    /* demods */
    dbindex = 0; /* reset index for demod database */
    if ( SDRV_InstallDev(STTUNER_DEMOD_STV0299, dbindex) == ST_NO_ERROR) dbindex++;
    if ( SDRV_InstallDev(STTUNER_DEMOD_STV0399, dbindex) == ST_NO_ERROR) dbindex++;
    if ( SDRV_InstallDev(STTUNER_DEMOD_STB0899, dbindex) == ST_NO_ERROR) dbindex++;
    if ( SDRV_InstallDev(STTUNER_DEMOD_STX0288, dbindex) == ST_NO_ERROR) dbindex++;
    /* the debug numbers (below) should match the tally in SDRV_AllocMem()*/
#ifdef STTUNER_DEBUG_MODULE_SDRV
    STTBX_Print(("%s installed %d types of demod\n", identity, dbindex));
#endif

 
    /* tuners */
    dbindex = 0; /* reset index for tuner database */
	    #ifdef STTUNER_DRV_SAT_TUN_VG1011
	        if ( SDRV_InstallDev(STTUNER_TUNER_VG1011,  dbindex) == ST_NO_ERROR) dbindex++;
	    #endif
	    #ifdef STTUNER_DRV_SAT_TUN_EVALMAX
	        if ( SDRV_InstallDev(STTUNER_TUNER_EVALMAX, dbindex) == ST_NO_ERROR) dbindex++;
	    #endif
	    #ifdef STTUNER_DRV_SAT_TUN_S68G21
	        if ( SDRV_InstallDev(STTUNER_TUNER_S68G21,  dbindex) == ST_NO_ERROR) dbindex++;
	    #endif
	    #ifdef STTUNER_DRV_SAT_TUN_VG0011
	        if ( SDRV_InstallDev(STTUNER_TUNER_VG0011,  dbindex) == ST_NO_ERROR) dbindex++;
	    #endif
	    #ifdef STTUNER_DRV_SAT_TUN_HZ1184
	        if ( SDRV_InstallDev(STTUNER_TUNER_HZ1184,  dbindex) == ST_NO_ERROR) dbindex++;
	    #endif
	    #ifdef STTUNER_DRV_SAT_TUN_MAX2116
	        if ( SDRV_InstallDev(STTUNER_TUNER_MAX2116, dbindex) == ST_NO_ERROR) dbindex++;
	    #endif
	    
	    #ifdef STTUNER_DRV_SAT_TUN_TUA6100
	        if ( SDRV_InstallDev(STTUNER_TUNER_TUA6100, dbindex) == ST_NO_ERROR) dbindex++;
	    #endif
	    
	    #ifdef STTUNER_DRV_SAT_TUN_DSF8910
	        if ( SDRV_InstallDev(STTUNER_TUNER_DSF8910, dbindex) == ST_NO_ERROR) dbindex++;
	    #endif
	    
	    #ifdef STTUNER_DRV_SAT_TUN_IX2476
	        if ( SDRV_InstallDev(STTUNER_TUNER_IX2476, dbindex) == ST_NO_ERROR) dbindex++;
	    #endif
    
   	    #ifdef STTUNER_DRV_SAT_TUN_STB6000
	        if ( SDRV_InstallDev(STTUNER_TUNER_STB6000, dbindex) == ST_NO_ERROR) dbindex++;
	    #endif
	    #ifdef STTUNER_DRV_SAT_TUN_STB6110
	        if ( SDRV_InstallDev(STTUNER_TUNER_STB6110, dbindex) == ST_NO_ERROR) dbindex++;
	    #endif
	    
	    
    	#ifdef STTUNER_DRV_SAT_TUN_STB6100
	        if ( SDRV_InstallDev(STTUNER_TUNER_STB6100, dbindex) == ST_NO_ERROR) dbindex++;
	    #endif
#ifdef STTUNER_DEBUG_MODULE_SDRV
    STTBX_Print(("%s installed %d types of tuner\n", identity, dbindex));
#endif

    /* lnbs */
    dbindex = 0;
    if ( SDRV_InstallDev(STTUNER_LNB_NONE,    dbindex) == ST_NO_ERROR) dbindex++;
    if ( SDRV_InstallDev(STTUNER_LNB_DEMODIO, dbindex) == ST_NO_ERROR) dbindex++;
    if ( SDRV_InstallDev(STTUNER_LNB_BACKENDGPIO, dbindex) == ST_NO_ERROR) dbindex++;
    if ( SDRV_InstallDev(STTUNER_LNB_STEM, dbindex) == ST_NO_ERROR) dbindex++;
#ifdef STTUNER_DRV_SAT_LNB21
    if ( SDRV_InstallDev(STTUNER_LNB_LNB21, dbindex) == ST_NO_ERROR) dbindex++;
#endif
#ifdef STTUNER_DRV_SAT_LNBH21
    if ( SDRV_InstallDev(STTUNER_LNB_LNBH21, dbindex) == ST_NO_ERROR) dbindex++;
#endif
#ifdef STTUNER_DRV_SAT_LNBH24
    if ( SDRV_InstallDev(STTUNER_LNB_LNBH24, dbindex) == ST_NO_ERROR) dbindex++;
#endif  
  
#ifdef STTUNER_DRV_SAT_LNBH23
    if ( SDRV_InstallDev(STTUNER_LNB_LNBH23, dbindex) == ST_NO_ERROR) dbindex++;
#endif    
#ifdef STTUNER_DEBUG_MODULE_SDRV
    STTBX_Print(("%s installed %d types of lnb\n", identity, dbindex));
#endif

#ifdef STTUNER_DRV_SAT_SCR
	    /* scr */
	    dbindex = 0; /* reset index for demod database */
	    if ( SDRV_InstallDev(STTUNER_SCR_APPLICATION, dbindex) == ST_NO_ERROR) dbindex++;
	    /* the debug numbers (below) should match the tally in SDRV_AllocMem()*/
		#ifdef STTUNER_DEBUG_MODULE_SDRV
		    STTBX_Print(("%s installed %d types of scr\n", identity, dbindex));
		#endif
#endif

#ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
	 /* SEC/diseqc */
	    dbindex = 0; /* reset index for demod database */
	    if ( SDRV_InstallDev(STTUNER_DISEQC_5100, dbindex) == ST_NO_ERROR) dbindex++;
	    /* the debug numbers (below) should match the tally in SDRV_AllocMem()*/
		#ifdef STTUNER_DEBUG_MODULE_SDRV
		    STTBX_Print(("%s installed %d types of scr\n", identity, dbindex));
		#endif
#endif 

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: SDRV_AllocMem()

Description:
     At first call to STTUNER_Init() count (per type) number of satellite device
    drivers and allocate memory for them.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t SDRV_AllocMem(ST_Partition_t  *MemoryPartition)
{
#ifdef STTUNER_DEBUG_MODULE_SDRV
   const char *identity = "STTUNER sdrv.c SDRV_AllocMem()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_DriverDbase_t *Drv;
    U32 demods  = 0;
    U32 tuners  = 0;
    U32 lnbs    = 0;
    #ifdef STTUNER_DRV_SAT_SCR
    U32 scrs    = 0;
    #endif
    
    #ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
    U32 diseqcs  = 0;
    #endif 

    Drv  = STTUNER_GetDrvDb();  /* get driver database pointer */

    /* find number of TYPES of device */

#ifdef STTUNER_DRV_SAT_NULL
    lnbs++;     /* lnone.h */
#endif

#ifdef STTUNER_DRV_SAT_STV0299
    demods++;   /* d0299.h */
    lnbs++;     /* l0299.h */
#endif

#ifdef STTUNER_DRV_SAT_STV0399
    demods++;   /* d0399.h */
    lnbs++;     /* l0399.h */
#endif
#ifdef STTUNER_DRV_SAT_STB0899
    demods++;   /* d0899.h */
#endif
#ifdef STTUNER_DRV_SAT_STX0288
    demods++;   /* d0288.h */
#endif





#ifdef STTUNER_LNB_POWER_THRU_DEMODIO
    lnbs++;     /* lnbIO.h */
#endif

#ifdef STTUNER_LNB_POWER_THRU_BACKENDGPIO
    lnbs++;     /* lnbBEIO.h */
#endif

#ifdef STTUNER_DRV_SAT_LNB21
    lnbs++;     /* lnb21.h */
#endif
#ifdef STTUNER_DRV_SAT_LNBH21
    lnbs++;     /* lnbh21.h */
#endif
#if defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23) 
    lnbs++;     /* lnbh24.h */
#endif


#ifdef STTUNER_DRV_SAT_STEM
    lnbs++;
#endif

	#ifdef STTUNER_DRV_SAT_TUN_VG1011
	    tuners++;
	#endif	
	#ifdef STTUNER_DRV_SAT_TUN_EVALMAX
	    tuners++;
	#endif
	#ifdef STTUNER_DRV_SAT_TUN_S68G21
	    tuners++;
	#endif
	#ifdef STTUNER_DRV_SAT_TUN_VG0011
	    tuners++;
	#endif
	#ifdef STTUNER_DRV_SAT_TUN_HZ1184
	    tuners++;
	#endif
	#ifdef STTUNER_DRV_SAT_TUN_MAX2116
	    tuners++;
	#endif
	#ifdef STTUNER_DRV_SAT_TUN_DSF8910
	    tuners++;
	#endif
	#ifdef STTUNER_DRV_SAT_TUN_TUA6100
	    tuners++;
	#endif
	
	#ifdef STTUNER_DRV_SAT_TUN_IX2476
	    tuners++;
	#endif
	#ifdef STTUNER_DRV_SAT_TUN_STB6000
	    tuners++;
	#endif
	#ifdef STTUNER_DRV_SAT_TUN_STB6110
	    tuners++;
	#endif
	#ifdef STTUNER_DRV_SAT_TUN_STB6100
	    tuners++;
	#endif

#ifdef STTUNER_DRV_SAT_SCR
    scrs   +=1;
#endif

#ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
    diseqcs  += 1;
#endif 
    /* show tally of driver types */
#ifdef STTUNER_DEBUG_MODULE_SDRV
    STTBX_Print(("%s Demodulators = %d\n", identity, demods));
    STTBX_Print(("%s Tuners       = %d\n", identity, tuners));
    STTBX_Print(("%s Lnbs         = %d\n", identity, lnbs));
    
    #ifdef STTUNER_DRV_SAT_SCR
    STTBX_Print(("%s Scrs         = %d\n", identity, scrs));
    #endif
    
#endif
  
    /* ---------- allocate memory for drivers ----------*/

    Drv->Sat.DemodDbaseSize = demods;
    Drv->Sat.DemodDbase     = STOS_MemoryAllocateClear(MemoryPartition, demods, sizeof(STTUNER_demod_dbase_t));
    if (demods > 0)
    {
        if (Drv->Sat.DemodDbase == NULL)  
        {
#ifdef STTUNER_DEBUG_MODULE_SDRV
            STTBX_Print(("%s fail memory allocation DemodDbase\n", identity));
#endif          
            return(ST_ERROR_NO_MEMORY);           

        }
    }

    Drv->Sat.TunerDbaseSize = tuners;
    if (tuners > 0)
    {
        Drv->Sat.TunerDbase = STOS_MemoryAllocateClear(MemoryPartition, tuners, sizeof(STTUNER_tuner_dbase_t));
        if (Drv->Sat.TunerDbase == NULL)  
        {
#ifdef STTUNER_DEBUG_MODULE_SDRV
            STTBX_Print(("%s fail memory allocation TunerDbaseSize\n", identity));
#endif    
            return(ST_ERROR_NO_MEMORY);           
        }
    }

    Drv->Sat.LnbDbaseSize = lnbs;
    if (lnbs > 0)
    {
        Drv->Sat.LnbDbase = STOS_MemoryAllocateClear(MemoryPartition, lnbs, sizeof(STTUNER_lnb_dbase_t));
        if (Drv->Sat.LnbDbase == NULL)  
        {
#ifdef STTUNER_DEBUG_MODULE_SDRV
            STTBX_Print(("%s fail memory allocation LnbDbase\n", identity));
#endif    
            return(ST_ERROR_NO_MEMORY);           
        }
    }
      
    #ifdef STTUNER_DRV_SAT_SCR
	    Drv->Sat.ScrDbaseSize = scrs;
	    Drv->Sat.ScrDbase     = STOS_MemoryAllocateClear(MemoryPartition, scrs, sizeof(STTUNER_scr_dbase_t));
	    if (scrs > 0)
	    {
	        if (Drv->Sat.ScrDbase == NULL)  
	        {
				#ifdef STTUNER_DEBUG_MODULE_SDRV
				            STTBX_Print(("%s fail memory allocation ScrDbase\n", identity));
				#endif          
	            return(ST_ERROR_NO_MEMORY);           
	
	        }
	    }
    #endif
            
    #ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
     	    Drv->Sat.DiseqcDbaseSize = diseqcs;
     	    Drv->Sat.DiseqcDbase    = STOS_MemoryAllocateClear(MemoryPartition,diseqcs,sizeof(STTUNER_diseqc_dbase_t));
    #endif
            
    return(Error);
}




/* ----------------------------------------------------------------------------
Name: SDRV_InstallDev()

Description:
    install a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t SDRV_InstallDev(U32 DeviceTypeID, U32 DatabaseIndex)
{
#ifdef STTUNER_DEBUG_MODULE_SDRV
   const char *identity = "STTUNER sdrv.c SDRV_InstallDev()";
#endif
    STTUNER_DriverDbase_t  *Drv;

    STTUNER_demod_dbase_t  *Demod;
    STTUNER_tuner_dbase_t  *Tuner;
    STTUNER_lnb_dbase_t    *Lnb;
    
    #ifdef STTUNER_DRV_SAT_SCR
    STTUNER_scr_dbase_t    *Scr;
    #endif
    
    #ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
    STTUNER_diseqc_dbase_t *Diseqc;
    #endif

    Drv = STTUNER_GetDrvDb();

    Demod = &Drv->Sat.DemodDbase[DatabaseIndex]; /* get pointer to storage element in database */
    
    
    switch(DeviceTypeID)    /* is it a demod? */
    {
        case STTUNER_DEMOD_STV0299:
		#ifdef STTUNER_DRV_SAT_STV0299 
            return( STTUNER_DRV_DEMOD_STV0299_Install(Demod) );
		#else
            return(ST_ERROR_UNKNOWN_DEVICE);
		#endif
                
        case STTUNER_DEMOD_STV0399:
		#ifdef STTUNER_DRV_SAT_STV0399 
            return( STTUNER_DRV_DEMOD_STV0399_Install(Demod) );
		#else
            return(ST_ERROR_UNKNOWN_DEVICE);
		#endif
                
		case STTUNER_DEMOD_STB0899:
		#ifdef STTUNER_DRV_SAT_STB0899
            return( STTUNER_DRV_DEMOD_STB0899_Install(Demod) );
		#else
            return(ST_ERROR_UNKNOWN_DEVICE);
		#endif

		case STTUNER_DEMOD_STX0288:
		#ifdef STTUNER_DRV_SAT_STX0288
            return( STTUNER_DRV_DEMOD_STX0288_Install(Demod) );
		#else
            return(ST_ERROR_UNKNOWN_DEVICE);
		#endif


        default:    /* no match, maybe not a demod device ID so try tuners... */
            break;
    }
        
    Tuner = &Drv->Sat.TunerDbase[DatabaseIndex];
    
    
    
    switch(DeviceTypeID)    /* is it a tuner? */
    {
	#ifdef STTUNER_DRV_SAT_TUN_S68G21    	
	        case STTUNER_TUNER_S68G21:
	#ifdef STTUNER_DRV_SAT_STV0299
	            return( STTUNER_DRV_TUNER_S68G21_Install(Tuner) );
	#else
	            return(ST_ERROR_UNKNOWN_DEVICE);
	#endif
	#endif

	#ifdef  STTUNER_DRV_SAT_TUN_VG1011
	        case STTUNER_TUNER_VG1011:
	#ifdef STTUNER_DRV_SAT_STV0299
	            return( STTUNER_DRV_TUNER_VG1011_Install(Tuner) );
	#else
	            return(ST_ERROR_UNKNOWN_DEVICE);
	#endif
	#endif

	#ifdef STTUNER_DRV_SAT_TUN_TUA6100
	        case STTUNER_TUNER_TUA6100:
	#ifdef STTUNER_DRV_SAT_STV0299
	            return( STTUNER_DRV_TUNER_TUA6100_Install(Tuner) );
	#else
	            return(ST_ERROR_UNKNOWN_DEVICE);
	#endif
	#endif

	#ifdef STTUNER_DRV_SAT_TUN_EVALMAX
	        case STTUNER_TUNER_EVALMAX:
	#ifdef STTUNER_DRV_SAT_STV0299
	            return( STTUNER_DRV_TUNER_EVALMAX_Install(Tuner) );
	#else
	            return(ST_ERROR_UNKNOWN_DEVICE);
	#endif
	#endif

	#ifdef  STTUNER_DRV_SAT_TUN_VG0011
	        case STTUNER_TUNER_VG0011:
	#ifdef STTUNER_DRV_SAT_STV0299
	            return( STTUNER_DRV_TUNER_VG0011_Install(Tuner) );
	#else
	            return(ST_ERROR_UNKNOWN_DEVICE);
	#endif
	#endif

	#ifdef  STTUNER_DRV_SAT_TUN_HZ1184
	        case STTUNER_TUNER_HZ1184:
	#ifdef STTUNER_DRV_SAT_STV0299
	            return( STTUNER_DRV_TUNER_HZ1184_Install(Tuner) );
	#else
	            return(ST_ERROR_UNKNOWN_DEVICE);
	#endif
	#endif

	#ifdef  STTUNER_DRV_SAT_TUN_MAX2116
	        case STTUNER_TUNER_MAX2116:
	#ifdef STTUNER_DRV_SAT_STV0299
	            return( STTUNER_DRV_TUNER_MAX2116_Install(Tuner) );
	#else
	            return(ST_ERROR_UNKNOWN_DEVICE);
	#endif
	#endif

	#ifdef STTUNER_DRV_SAT_TUN_DSF8910            
	        case STTUNER_TUNER_DSF8910:
	#ifdef STTUNER_DRV_SAT_STV0299
	            return( STTUNER_DRV_TUNER_DSF8910_Install(Tuner) );
	#else
	            return(ST_ERROR_UNKNOWN_DEVICE);    
	#endif
	#endif


	#ifdef STTUNER_DRV_SAT_TUN_IX2476
	        case STTUNER_TUNER_IX2476:
	#if defined(STTUNER_DRV_SAT_STV0299) || defined(STTUNER_DRV_SAT_STX0288)
	            return( STTUNER_DRV_TUNER_IX2476_Install(Tuner) );
	#else
	            return(ST_ERROR_UNKNOWN_DEVICE);    
	#endif
	#endif

	

	
	#ifdef STTUNER_DRV_SAT_TUN_STB6000
	        case STTUNER_TUNER_STB6000:
	#if defined(STTUNER_DRV_SAT_STV0299) || defined(STTUNER_DRV_SAT_STX0288)||defined(STTUNER_DRV_SAT_STB0289)
	            return( STTUNER_DRV_TUNER_STB6000_Install(Tuner) );
	#else
	            return(ST_ERROR_UNKNOWN_DEVICE);    
	#endif
	#endif
	
	#ifdef STTUNER_DRV_SAT_TUN_STB6110
	        case STTUNER_TUNER_STB6110:
	#if defined(STTUNER_DRV_SAT_STB0900) 
	            return( STTUNER_DRV_TUNER_STB6110_Install(Tuner) );
	#else
	            return(ST_ERROR_UNKNOWN_DEVICE);    
	#endif
	#endif
	


	#ifdef STTUNER_DRV_SAT_TUN_STB6100
	case STTUNER_TUNER_STB6100:
         return( STTUNER_DRV_TUNER_STB6100_Install(Tuner) );
	#endif

    default:    /* no match */
            break;
    }
       
    Lnb = &Drv->Sat.LnbDbase[DatabaseIndex];
    switch(DeviceTypeID)    /* is it a lnb? */
    {
        case STTUNER_LNB_NONE:
#ifdef STTUNER_DRV_SAT_NULL 
            return( STTUNER_DRV_LNB_NONE_Install(Lnb) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif

        case STTUNER_LNB_DEMODIO:
        
        #ifdef STTUNER_LNB_POWER_THRU_DEMODIO
        return( STTUNER_DRV_LNB_DemodIO_Install(Lnb) );
        #else
        return(ST_ERROR_UNKNOWN_DEVICE);
	#endif
        
        case STTUNER_LNB_BACKENDGPIO:
                #ifdef STTUNER_LNB_POWER_THRU_BACKENDGPIO
                return( STTUNER_DRV_LNB_BackEndIO_Install(Lnb) );
                #else
                return( ST_ERROR_UNKNOWN_DEVICE );
                #endif
                
        case STTUNER_LNB_STEM:
#ifdef STTUNER_DRV_SAT_STEM
            return( STTUNER_DRV_LNB_STEM_Install(Lnb) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif

	case STTUNER_LNB_LNB21:
#ifdef STTUNER_DRV_SAT_LNB21
            return( STTUNER_DRV_LNB_LNB21_Install(Lnb) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif

   case STTUNER_LNB_LNBH21:
#ifdef STTUNER_DRV_SAT_LNBH21
            return( STTUNER_DRV_LNB_LNBH21_Install(Lnb) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#ifdef STTUNER_DRV_SAT_LNBH24 
 case STTUNER_LNB_LNBH24:
            return( STTUNER_DRV_LNB_LNBH24_Install(Lnb) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#ifdef STTUNER_DRV_SAT_LNBH23
 case STTUNER_LNB_LNBH23:
            return( STTUNER_DRV_LNB_LNBH24_Install(Lnb) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif
        default:    /* no match */
            break;
    }
 
#ifdef STTUNER_DRV_SAT_SCR
    Scr = &Drv->Sat.ScrDbase[DatabaseIndex]; /* get pointer to storage element in database */
    switch(DeviceTypeID)    /* is it a scr application? */
    {
        case STTUNER_SCR_APPLICATION:

            return( STTUNER_DRV_SCR_APPLICATION_Install(Scr) );
                               
        default:    /* no match, maybe not a scr device ID so try nothing... */
        return(ST_ERROR_UNKNOWN_DEVICE);
            break;
    }
#endif

#ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
    Diseqc = &Drv->Sat.DiseqcDbase[DatabaseIndex]; /* get pointer to storage element in database */
    switch(DeviceTypeID)    /* is it a back end diseqc  */
    {
    	case STTUNER_DISEQC_5100:
    	return (STTUNER_DRV_DISEQC_5100_530X_Install(Diseqc));
    	default:
    	break;
    }
#endif
#ifdef STTUNER_DEBUG_MODULE_SDRV
        STTBX_Print(("%s fail unknown satellite device type ID = %d\n", identity, DeviceTypeID));
#endif
     
    return(STTUNER_ERROR_ID);   /* no matching devices found */
}   



/* ----------------------------------------------------------------------------
Name: STTUNER_SAT_DRV_UnInstall()

Description:
    install a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SAT_DRV_UnInstall(void)
{
#ifdef STTUNER_DEBUG_MODULE_SDRV
    const char *identity = "STTUNER sdrv.c STTUNER_SAT_DRV_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STTUNER_DEBUG_MODULE_SDRV
    STTBX_Print(("%s uninstall BEGIN\n", identity));
#endif

    /* uninstall drivers from their databases */
    /* demods */
    SDRV_UnInstallDev(STTUNER_DEMOD_STV0299);
    SDRV_UnInstallDev(STTUNER_DEMOD_STV0399);
    SDRV_UnInstallDev(STTUNER_DEMOD_STB0899);
    SDRV_UnInstallDev(STTUNER_DEMOD_STX0288);



    /* tuners */
    #ifdef  STTUNER_DRV_SAT_TUN_VG1011
    SDRV_UnInstallDev(STTUNER_TUNER_VG1011);
    #endif
    #ifdef STTUNER_DRV_SAT_TUN_TUA6100
    SDRV_UnInstallDev(STTUNER_TUNER_TUA6100);
    #endif
    #ifdef STTUNER_DRV_SAT_TUN_EVALMAX
    SDRV_UnInstallDev(STTUNER_TUNER_EVALMAX);
    #endif
    #ifdef STTUNER_DRV_SAT_TUN_S68G21
    SDRV_UnInstallDev(STTUNER_TUNER_S68G21);
    #endif
    #ifdef  STTUNER_DRV_SAT_TUN_VG0011
    SDRV_UnInstallDev(STTUNER_TUNER_VG0011);
    #endif
    #ifdef  STTUNER_DRV_SAT_TUN_HZ1184
    SDRV_UnInstallDev(STTUNER_TUNER_HZ1184);
    #endif
    #ifdef  STTUNER_DRV_SAT_TUN_MAX2116
    SDRV_UnInstallDev(STTUNER_TUNER_MAX2116);
    #endif
    #ifdef STTUNER_DRV_SAT_TUN_DSF8910
    SDRV_UnInstallDev(STTUNER_TUNER_DSF8910);
    #endif
    
    #ifdef STTUNER_DRV_SAT_TUN_IX2476
    SDRV_UnInstallDev(STTUNER_TUNER_IX2476);
    #endif

    #ifdef STTUNER_DRV_SAT_TUN_STB6000
    SDRV_UnInstallDev(STTUNER_TUNER_STB6000);
    #endif
     #ifdef STTUNER_DRV_SAT_TUN_STB6110
    SDRV_UnInstallDev(STTUNER_TUNER_STB6110);
    #endif

#ifdef STTUNER_DRV_SAT_TUN_STB6100
    SDRV_UnInstallDev(STTUNER_TUNER_STB6100);
#endif

    /* lnbs */
    SDRV_UnInstallDev(STTUNER_LNB_NONE);
    SDRV_UnInstallDev(STTUNER_LNB_DEMODIO);
    SDRV_UnInstallDev(STTUNER_LNB_BACKENDGPIO);
    
    SDRV_UnInstallDev(STTUNER_LNB_STEM);

#ifdef STTUNER_DRV_SAT_LNB21
    SDRV_UnInstallDev(STTUNER_LNB_LNB21);
#endif
#ifdef STTUNER_DRV_SAT_LNBH21
    SDRV_UnInstallDev(STTUNER_LNB_LNBH21);
#endif

#ifdef STTUNER_DRV_SAT_LNBH24
    SDRV_UnInstallDev(STTUNER_LNB_LNBH24);
#endif
#ifdef STTUNER_DRV_SAT_LNBH23
    SDRV_UnInstallDev(STTUNER_LNB_LNBH23);
#endif
   
      /* scrs*/
    #ifdef STTUNER_DRV_SAT_SCR
    SDRV_UnInstallDev(STTUNER_SCR_APPLICATION);
    #endif

#ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
    SDRV_UnInstallDev(STTUNER_DISEQC_5100);
#endif     
    Error = SDRV_FreeMem(SDRV_MemoryPartition);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SDRV
        STTBX_Print(("%s fail SDRV_FreeMem()\n", identity));
#endif
        return(Error);
    }

    SDRV_MemoryPartition = NULL;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: SDRV_FreeMem()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t SDRV_FreeMem(ST_Partition_t  *MemoryPartition)
{
#ifdef STTUNER_DEBUG_MODULE_SDRV
   const char *identity = "STTUNER sdrv.c SDRV_FreeMem()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_DriverDbase_t *Drv;

    Drv = STTUNER_GetDrvDb();  /* get driver database pointer */
  
    /* ---------- free memory ----------*/

    Drv->Sat.DemodDbaseSize  = 0;
    Drv->Sat.TunerDbaseSize  = 0;
    Drv->Sat.LnbDbaseSize    = 0;
    
    #ifdef STTUNER_DRV_SAT_SCR
    Drv->Sat.ScrDbaseSize    = 0;
    #endif

    STOS_MemoryDeallocate(MemoryPartition, Drv->Sat.DemodDbase);
    STOS_MemoryDeallocate(MemoryPartition, Drv->Sat.TunerDbase);
    STOS_MemoryDeallocate(MemoryPartition, Drv->Sat.LnbDbase);
    
    #ifdef STTUNER_DRV_SAT_SCR
    STOS_MemoryDeallocate(MemoryPartition, Drv->Sat.ScrDbase);
    #endif
    
    #ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
    STOS_MemoryDeallocate(MemoryPartition, Drv->Sat.DiseqcDbase);
    #endif

    Drv->Sat.DemodDbase  = NULL;
    Drv->Sat.TunerDbase  = NULL;
    Drv->Sat.LnbDbase    = NULL;
    
    #ifdef STTUNER_DRV_SAT_SCR
    Drv->Sat.ScrDbase    = NULL;
    #endif
    
    #ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
    Drv->Sat.DiseqcDbase = NULL;
    #endif

#ifdef STTUNER_DEBUG_MODULE_SDRV
    STTBX_Print(("%s ok\n", identity));
#endif    

    return(Error);
}







/* ----------------------------------------------------------------------------
Name: SDRV_UnInstallDev()

Description:
    install a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t SDRV_UnInstallDev(U32 DeviceTypeID)
{
#ifdef STTUNER_DEBUG_MODULE_SDRV
   const char *identity = "STTUNER sdrv.c SDRV_UnInstallDev()";
#endif
    STTUNER_DriverDbase_t  *Drv;

    STTUNER_demod_dbase_t  *Demod;
    STTUNER_tuner_dbase_t  *Tuner;
    STTUNER_lnb_dbase_t    *Lnb;
    #ifdef STTUNER_DRV_SAT_SCR
    STTUNER_scr_dbase_t    *Scr;
    #endif
    #ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
    STTUNER_diseqc_dbase_t    *Diseqc;
    #endif
    U32 index;

    Drv = STTUNER_GetDrvDb();


    if (Drv->Sat.DemodDbaseSize > 0) 
    {
        for(index = 0; index <= Drv->Sat.DemodDbaseSize; index++)
        {
            Demod = &Drv->Sat.DemodDbase[index];
            if (DeviceTypeID == Demod->ID)  /* is it a demod? */
            {
                switch(DeviceTypeID)    
                {
                    case STTUNER_DEMOD_STV0299:
#ifdef STTUNER_DRV_SAT_STV0299 
                        return( STTUNER_DRV_DEMOD_STV0299_UnInstall(Demod) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif

                    case STTUNER_DEMOD_STV0399:
#ifdef STTUNER_DRV_SAT_STV0399 
                        return( STTUNER_DRV_DEMOD_STV0399_UnInstall(Demod) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif

					case STTUNER_DEMOD_STB0899:
#ifdef STTUNER_DRV_SAT_STB0899 
                        return( STTUNER_DRV_DEMOD_STB0899_UnInstall(Demod) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif

case STTUNER_DEMOD_STX0288:
#ifdef STTUNER_DRV_SAT_STX0288
                        return( STTUNER_DRV_DEMOD_STX0288_UnInstall(Demod) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif



                    default:    /* no match, maybe not a demod device ID so try tuners... */
                        break;
                }
            }
        }
    }


    if (Drv->Sat.TunerDbaseSize > 0) 
    {
        for(index = 0; index <= Drv->Sat.TunerDbaseSize; index++)
        {
            Tuner = &Drv->Sat.TunerDbase[index];
            if (DeviceTypeID == Tuner->ID)  /* is it a tuner? */
            {
                switch(DeviceTypeID)    

                {
                	#ifdef STTUNER_DRV_SAT_TUN_S68G21
                    case STTUNER_TUNER_S68G21:
					#ifdef STTUNER_DRV_SAT_STV0299 
                        return( STTUNER_DRV_TUNER_S68G21_UnInstall(Tuner) );
					#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
					#endif
					#endif

					#ifdef  STTUNER_DRV_SAT_TUN_VG1011
                    case STTUNER_TUNER_VG1011:
					#ifdef STTUNER_DRV_SAT_STV0299 
                        return( STTUNER_DRV_TUNER_VG1011_UnInstall(Tuner) );
					#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
					#endif
					#endif

					#ifdef STTUNER_DRV_SAT_TUN_TUA6100
                    case STTUNER_TUNER_TUA6100:
					#ifdef STTUNER_DRV_SAT_STV0299 
                        return( STTUNER_DRV_TUNER_TUA6100_UnInstall(Tuner) );
					#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
					#endif
					#endif
	
					#ifdef STTUNER_DRV_SAT_TUN_EVALMAX
                    case STTUNER_TUNER_EVALMAX:
					#ifdef STTUNER_DRV_SAT_STV0299 
                        return( STTUNER_DRV_TUNER_EVALMAX_UnInstall(Tuner) );
					#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
					#endif
					#endif
					
					#ifdef  STTUNER_DRV_SAT_TUN_VG0011
                    case STTUNER_TUNER_VG0011:
					#ifdef STTUNER_DRV_SAT_STV0299 
                        return( STTUNER_DRV_TUNER_VG0011_UnInstall(Tuner) );
					#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
					#endif
					#endif
					
					#ifdef  STTUNER_DRV_SAT_TUN_HZ1184
                    case STTUNER_TUNER_HZ1184:
					#ifdef STTUNER_DRV_SAT_STV0299 
                        return( STTUNER_DRV_TUNER_HZ1184_UnInstall(Tuner) );
					#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
					#endif
					#endif
					
					#ifdef  STTUNER_DRV_SAT_TUN_MAX2116
                    case STTUNER_TUNER_MAX2116:
					#ifdef STTUNER_DRV_SAT_STV0299 
                        return( STTUNER_DRV_TUNER_MAX2116_UnInstall(Tuner) );
					#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
					#endif
					#endif

					#ifdef STTUNER_DRV_SAT_TUN_DSF8910
					case STTUNER_TUNER_DSF8910:
					#ifdef STTUNER_DRV_SAT_STV0299 
                        return( STTUNER_DRV_TUNER_DSF8910_UnInstall(Tuner) );
					#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
					#endif
					#endif
					
					#ifdef STTUNER_DRV_SAT_TUN_IX2476
					case STTUNER_TUNER_IX2476:
					#if defined(STTUNER_DRV_SAT_STV0299 ) || defined(STTUNER_DRV_SAT_STX0288) 
                        return( STTUNER_DRV_TUNER_IX2476_UnInstall(Tuner) );
					#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
					#endif
					#endif
					
					#ifdef STTUNER_DRV_SAT_TUN_STB6000
					case STTUNER_TUNER_STB6000:
					#if defined(STTUNER_DRV_SAT_STV0299 ) || defined(STTUNER_DRV_SAT_STX0288) ||defined(STTUNER_DRV_SAT_STB0289) 
                        return( STTUNER_DRV_TUNER_STB6000_UnInstall(Tuner) );
					#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
					#endif
					#endif
					#ifdef STTUNER_DRV_SAT_TUN_STB6110
					case STTUNER_TUNER_STB6110:
					#if defined(STTUNER_DRV_SAT_STB0900 ) 
                        return( STTUNER_DRV_TUNER_STB6110_UnInstall(Tuner) );
					#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
					#endif
					#endif								
					
					
					#ifdef STTUNER_DRV_SAT_TUN_STB6100	
					case STTUNER_TUNER_STB6100:
						return( STTUNER_DRV_TUNER_STB6100_UnInstall(Tuner) );
						
					#endif
					
						
                    default:    /* no match */
                    break;
                }
            }
        }
    }


    if (Drv->Sat.LnbDbaseSize > 0) 
    {
        for(index = 0; index <= Drv->Sat.LnbDbaseSize; index++)
        {
            Lnb = &Drv->Sat.LnbDbase[index];
            if (DeviceTypeID == Lnb->ID)  /* is it a lnb? */
            {
                switch(DeviceTypeID)    

                {
                   case STTUNER_LNB_NONE:
#ifdef STTUNER_DRV_SAT_NULL 
                        return( STTUNER_DRV_LNB_NONE_UnInstall(Lnb) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif

                case STTUNER_LNB_DEMODIO:
                #ifdef STTUNER_LNB_POWER_THRU_DEMODIO
                return( STTUNER_DRV_LNB_DemodIO_UnInstall(Lnb) );
                #else
                        return(ST_ERROR_UNKNOWN_DEVICE);
		#endif
                case STTUNER_LNB_BACKENDGPIO:
                #ifdef STTUNER_LNB_POWER_THRU_BACKENDGPIO
                return( STTUNER_DRV_LNB_BackEndIO_UnInstall(Lnb) );
                #else
                return( ST_ERROR_UNKNOWN_DEVICE);
                #endif
                
                    case STTUNER_LNB_STEM:
#ifdef STTUNER_DRV_SAT_STEM 
                        return( STTUNER_DRV_LNB_STEM_UnInstall(Lnb) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif

		    case STTUNER_LNB_LNB21:
#ifdef STTUNER_DRV_SAT_LNB21 
                        return( STTUNER_DRV_LNB_LNB21_UnInstall(Lnb) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif
			case STTUNER_LNB_LNBH21:
#ifdef STTUNER_DRV_SAT_LNBH21 
                        return( STTUNER_DRV_LNB_LNBH21_UnInstall(Lnb) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#ifdef STTUNER_DRV_SAT_LNBH24
	case STTUNER_LNB_LNBH24:
                        return( STTUNER_DRV_LNB_LNBH24_UnInstall(Lnb) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#ifdef STTUNER_DRV_SAT_LNBH23
	case STTUNER_LNB_LNBH23:
                        return( STTUNER_DRV_LNB_LNBH24_UnInstall(Lnb) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif

                    default:    /* no match */
                    return(ST_ERROR_UNKNOWN_DEVICE);
                        break;
                }
            }
        }
    }

#ifdef STTUNER_DRV_SAT_SCR
if (Drv->Sat.ScrDbaseSize > 0) 
    {
        for(index = 0; index <= Drv->Sat.ScrDbaseSize; index++)
        {
            Scr = &Drv->Sat.ScrDbase[index];
            if (DeviceTypeID == Scr->ID)  /* is it a scr? */
            {
                switch(DeviceTypeID)    
                {
                    case STTUNER_SCR_APPLICATION:

                        return( STTUNER_DRV_SCR_APPLICATION_UnInstall(Scr) );

                    default:    /* no match, maybe not a scr device ID so try ... */
                        break;
                }
            }
        }
    }
#endif

#ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
if (Drv->Sat.DiseqcDbaseSize > 0) 
    {
        for(index = 0; index <= Drv->Sat.DiseqcDbaseSize; index++)
        {
            Diseqc = &Drv->Sat.DiseqcDbase[index];
            if (DeviceTypeID == Diseqc->ID)  /* is it a scr? */
            {
                switch(DeviceTypeID)    
                {
                    case STTUNER_DISEQC_5100:

                        return(STTUNER_DRV_DISEQC_5100_530X_unInstall(Diseqc));

                    default:    /* no match, maybe not a scr device ID so try ... */
                        break;
                }
            }
        }
    }
#endif

#ifdef STTUNER_DEBUG_MODULE_SDRV
        STTBX_Print(("%s fail unknown satellite device type ID = %d\n", identity, DeviceTypeID));
#endif
     
    return(STTUNER_ERROR_ID);   /* no matching devices found */
}   



/* End of sdrv.c */
