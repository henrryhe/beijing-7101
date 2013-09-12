/* ----------------------------------------------------------------------------
File Name: tdrv.c

Description:


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
#include "stlite.h"     /* Standard includes */

/* STAPI */
#include "sttbx.h"

#include "stevt.h"
#include "sttuner.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "tdrv.h"       /* header for this file */
#include "ioreg.h"

/* null */
#ifdef STTUNER_DRV_TER_NULL
 #include "lnone.h"
#endif

/* STV0360 chip */
#ifdef STTUNER_DRV_TER_STV0360
 #include "360_echo.h"
 #include "d0360.h"
#endif

/* STV0361 chip */
#ifdef STTUNER_DRV_TER_STV0361
  #include "361_echo.h"
 #include "d0361.h"
#endif

/* STV0362 chip */
#ifdef STTUNER_DRV_TER_STV0362
 #include "d0362_echo.h"
 #include "d0362.h"
#endif

#ifdef STTUNER_DRV_TER_STB0370VSB
 #include "d0370VSB.h"
#endif

#ifdef STTUNER_DRV_TER_STV0372
 #include "d0372.h"
#endif

#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361)|| defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STB0370VSB) || defined(STTUNER_DRV_TER_STV0372)
 #include "tuntdrv.h"
#endif

 #ifdef STTUNER_DRV_TER_TUN_MT2266
#include "mt2266.h"
#endif

/* private variables ------------------------------------------------------- */

/* memory area to store driver database */
static ST_Partition_t *TDRV_MemoryPartition;
static STTUNER_TunerType_t TDRV_TunerType;

/* local functions --------------------------------------------------------- */

ST_ErrorCode_t TDRV_AllocMem(ST_Partition_t *MemoryPartition);
ST_ErrorCode_t TDRV_FreeMem (ST_Partition_t *MemoryPartition);

ST_ErrorCode_t TDRV_InstallDev  (U32 DeviceTypeID, U32 DatabaseIndex);
ST_ErrorCode_t TDRV_UnInstallDev(U32 DeviceTypeID);


/* ----------------------------------------------------------------------------
Name: STTUNER_TERR_DRV_Install()

Description:
    install a terrestrial device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_TERR_DRV_Install(TDRV_InstallParams_t *InstallParams)
{
#ifdef STTUNER_DEBUG_MODULE_TDRV
    const char *identity = "STTUNER tdrv.c STTUNER_TERR_DRV_Install()";
#endif
    U32 dbindex;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
#ifdef STTUNER_DEBUG_MODULE_TDRV
    STTBX_Print(("%s install BEGIN\n", identity));
#endif

    TDRV_MemoryPartition = InstallParams->MemoryPartition;
    TDRV_TunerType = InstallParams->TunerType;

    /* allocate memory for drivers in this build */
    Error = TDRV_AllocMem(TDRV_MemoryPartition);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TDRV
        STTBX_Print(("%s fail TDRV_AllocMem()\n", identity));
#endif
        return(Error);
    }

    /* install drivers in their databases */

    /* demods */
    dbindex = 0; /* reset index for demod database */
		    if ( TDRV_InstallDev(STTUNER_DEMOD_STV0360, dbindex) == ST_NO_ERROR) dbindex++;
		    if ( TDRV_InstallDev(STTUNER_DEMOD_STV0361, dbindex) == ST_NO_ERROR) dbindex++;
		#ifdef STTUNER_DRV_TER_STV0362
		    if ( TDRV_InstallDev(STTUNER_DEMOD_STV0362, dbindex) == ST_NO_ERROR) dbindex++;
		#endif
		#ifdef STTUNER_DRV_TER_STB0370VSB
		    if ( TDRV_InstallDev(STTUNER_DEMOD_STB0370VSB, dbindex) == ST_NO_ERROR) dbindex++;
		#endif
#ifdef STTUNER_DRV_TER_STV0372
    if ( TDRV_InstallDev(STTUNER_DEMOD_STV0372, dbindex) == ST_NO_ERROR) dbindex++;
#endif
    /* the debug numbers (below) should match the tally in TDRV_AllocMem()*/
#ifdef STTUNER_DEBUG_MODULE_TDRV
    STTBX_Print(("%s installed %d types of demod\n", identity, dbindex));
#endif


    /* tuners */
    dbindex = 0; /* reset index for tuner database */
    
#ifdef STTUNER_DRV_TER_TUN_DTT7572
    if ( TDRV_InstallDev(STTUNER_TUNER_DTT7572, dbindex) == ST_NO_ERROR) dbindex++;
#endif
#ifdef STTUNER_DRV_TER_TUN_DTT7578
    if ( TDRV_InstallDev(STTUNER_TUNER_DTT7578, dbindex) == ST_NO_ERROR) dbindex++;
#endif
#ifdef STTUNER_DRV_TER_TUN_DTT7592
    if ( TDRV_InstallDev(STTUNER_TUNER_DTT7592, dbindex) == ST_NO_ERROR) dbindex++;
#endif
#ifdef STTUNER_DRV_TER_TUN_TDLB7
    if ( TDRV_InstallDev(STTUNER_TUNER_TDLB7, dbindex) == ST_NO_ERROR) dbindex++;
#endif
#ifdef STTUNER_DRV_TER_TUN_TDQD3
    if ( TDRV_InstallDev(STTUNER_TUNER_TDQD3, dbindex) == ST_NO_ERROR) dbindex++;
#endif
#ifdef STTUNER_DRV_TER_TUN_DTT7300X
    if ( TDRV_InstallDev(STTUNER_TUNER_DTT7300X, dbindex) == ST_NO_ERROR) dbindex++;
#endif
#ifdef STTUNER_DRV_TER_TUN_ENG47402G1
    if ( TDRV_InstallDev(STTUNER_TUNER_ENG47402G1, dbindex) == ST_NO_ERROR) dbindex++;
#endif
#ifdef STTUNER_DRV_TER_TUN_EAL2780
    if ( TDRV_InstallDev(STTUNER_TUNER_EAL2780, dbindex) == ST_NO_ERROR) dbindex++;
#endif
#ifdef STTUNER_DRV_TER_TUN_TDA6650
    if ( TDRV_InstallDev(STTUNER_TUNER_TDA6650, dbindex) == ST_NO_ERROR) dbindex++;
#endif
#ifdef STTUNER_DRV_TER_TUN_TDEB2
    if ( TDRV_InstallDev(STTUNER_TUNER_TDEB2, dbindex) == ST_NO_ERROR) dbindex++;
#endif
#ifdef STTUNER_DRV_TER_TUN_TDM1316
    if ( TDRV_InstallDev(STTUNER_TUNER_TDM1316, dbindex) == ST_NO_ERROR) dbindex++;
#endif
#ifdef STTUNER_DRV_TER_TUN_ED5058
    if ( TDRV_InstallDev(STTUNER_TUNER_ED5058, dbindex) == ST_NO_ERROR) dbindex++;
#endif
#ifdef STTUNER_DRV_TER_TUN_MIVAR
    if ( TDRV_InstallDev(STTUNER_TUNER_MIVAR, dbindex) == ST_NO_ERROR) dbindex++;
#endif
#ifdef STTUNER_DRV_TER_TUN_TDED4
    if ( TDRV_InstallDev(STTUNER_TUNER_TDED4, dbindex) == ST_NO_ERROR) dbindex++;
#endif
#ifdef STTUNER_DRV_TER_TUN_DTT7102
    if ( TDRV_InstallDev(STTUNER_TUNER_DTT7102, dbindex) == ST_NO_ERROR) dbindex++;
#endif
#ifdef STTUNER_DRV_TER_TUN_TECC2849PG
    if ( TDRV_InstallDev(STTUNER_TUNER_TECC2849PG, dbindex) == ST_NO_ERROR) dbindex++;
#endif
#ifdef STTUNER_DRV_TER_TUN_TDCC2345
    if ( TDRV_InstallDev(STTUNER_TUNER_TDCC2345, dbindex) == ST_NO_ERROR) dbindex++;
#endif

#ifndef ST_OSLINUX
#ifdef STTUNER_DRV_TER_TUN_RF4000
    if ( TDRV_InstallDev(STTUNER_TUNER_RF4000, dbindex) == ST_NO_ERROR) dbindex++;
#endif
#endif
#ifdef STTUNER_DRV_TER_TUN_TDTGD108
    if ( TDRV_InstallDev(STTUNER_TUNER_TDTGD108, dbindex) == ST_NO_ERROR) dbindex++;
#endif
#ifdef STTUNER_DRV_TER_TUN_MT2266
    if ( TDRV_InstallDev(STTUNER_TUNER_MT2266, dbindex) == ST_NO_ERROR) dbindex++;
#endif 

#ifdef STTUNER_DRV_TER_TUN_DTVS223
    if ( TDRV_InstallDev(STTUNER_TUNER_DTVS223, dbindex) == ST_NO_ERROR) dbindex++;
#endif

    
#ifdef STTUNER_DEBUG_MODULE_TDRV
    STTBX_Print(("%s installed %d types of tuner\n", identity, dbindex));
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: TDRV_AllocMem()

Description:
     At first call to STTUNER_Init() count (per type) number of terrestrial device
    drivers and allocate memory for them.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t TDRV_AllocMem(ST_Partition_t  *MemoryPartition)
{
#ifdef STTUNER_DEBUG_MODULE_TDRV
   const char *identity = "STTUNER tdrv.c TDRV_AllocMem()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_DriverDbase_t *Drv;
    U32 demods  = 0;
    U32 tuners  = 0;

    Drv  = STTUNER_GetDrvDb();  /* get driver database pointer */

    /* find number of TYPES of device */
#ifdef STTUNER_DRV_TER_STV0360
    demods++;   /* d0360.h */
#endif

#ifdef STTUNER_DRV_TER_STV0361
    demods++;   /* d0361.h */
#endif

#ifdef STTUNER_DRV_TER_STV0362
    demods++;   /* d0362.h */
#endif

#ifdef STTUNER_DRV_TER_STB0370VSB
   demods++;
#endif
#ifdef STTUNER_DRV_TER_STV0372
   demods++;
#endif

#ifdef STTUNER_DRV_TER_TUN_DTT7572
    tuners++;
#endif
#ifdef STTUNER_DRV_TER_TUN_DTT7578
    tuners++;
#endif
#ifdef STTUNER_DRV_TER_TUN_DTT7592
    tuners++;
#endif
#ifdef STTUNER_DRV_TER_TUN_TDLB7
    tuners++;
#endif
#ifdef STTUNER_DRV_TER_TUN_TDQD3
    tuners++;
#endif
#ifdef STTUNER_DRV_TER_TUN_DTT7300X
    tuners++;
#endif
#ifdef STTUNER_DRV_TER_TUN_ENG47402G1
    tuners++;
#endif
#ifdef STTUNER_DRV_TER_TUN_EAL2780
    tuners++;
#endif
#ifdef STTUNER_DRV_TER_TUN_TDA6650
    tuners++;
#endif
#ifdef STTUNER_DRV_TER_TUN_TDEB2
    tuners++;
#endif
#ifdef STTUNER_DRV_TER_TUN_TDM1316
    tuners++;
#endif
#ifdef STTUNER_DRV_TER_TUN_ED5058
    tuners++;
#endif
#ifdef STTUNER_DRV_TER_TUN_MIVAR
    tuners++;
#endif
#ifdef STTUNER_DRV_TER_TUN_TDED4
    tuners++;
#endif
#ifdef STTUNER_DRV_TER_TUN_DTT7102
    tuners++;
#endif
#ifdef STTUNER_DRV_TER_TUN_TECC2849PG
    tuners++;
#endif
#ifdef STTUNER_DRV_TER_TUN_TDCC2345
    tuners++;
#endif

#ifndef ST_OSLINUX
#ifdef STTUNER_DRV_TER_TUN_RF4000
    tuners++;
#endif
#endif

#ifdef STTUNER_DRV_TER_TUN_MT2266
    tuners++;
#endif

#ifdef STTUNER_DRV_TER_TUN_TDTGD108
    tuners++;
#endif
#ifdef STTUNER_DRV_TER_TUN_DTVS223
    tuners++;
#endif


    /* show tally of driver types */
#ifdef STTUNER_DEBUG_MODULE_TDRV
    STTBX_Print(("%s Demodulators = %d\n", identity, demods));
    STTBX_Print(("%s Tuners       = %d\n", identity, tuners));
#endif

    /* ---------- allocate memory for drivers ----------*/

    Drv->Terr.DemodDbaseSize = demods;
    Drv->Terr.DemodDbase     = STOS_MemoryAllocateClear(MemoryPartition, demods, sizeof(STTUNER_demod_dbase_t));
    if (demods > 0)
    {
        if (Drv->Terr.DemodDbase == NULL)
        {
#ifdef STTUNER_DEBUG_MODULE_TDRV
            STTBX_Print(("%s fail memory allocation DemodDbase\n", identity));
#endif
            return(ST_ERROR_NO_MEMORY);

        }
    }

    Drv->Terr.TunerDbaseSize = tuners;
    if (tuners > 0)
    {
        Drv->Terr.TunerDbase = STOS_MemoryAllocateClear(MemoryPartition, tuners, sizeof(STTUNER_tuner_dbase_t));
        if (Drv->Terr.TunerDbase == NULL)
        {
#ifdef STTUNER_DEBUG_MODULE_TDRV
            STTBX_Print(("%s fail memory allocation TunerDbaseSize\n", identity));
#endif
            return(ST_ERROR_NO_MEMORY);
        }
    }

    return(Error);
}




/* ----------------------------------------------------------------------------
Name: TDRV_InstallDev()

Description:
    install a terrestrial device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t TDRV_InstallDev(U32 DeviceTypeID, U32 DatabaseIndex)
{
#ifdef STTUNER_DEBUG_MODULE_TDRV
   const char *identity = "STTUNER tdrv.c TDRV_InstallDev()";
#endif
    STTUNER_DriverDbase_t  *Drv;

    STTUNER_demod_dbase_t  *Demod;
    STTUNER_tuner_dbase_t  *Tuner;

    Drv = STTUNER_GetDrvDb();

    Demod = &Drv->Terr.DemodDbase[DatabaseIndex]; /* get pointer to storage element in database */
    switch(DeviceTypeID)    /* is it a demod? */
    {
		        case STTUNER_DEMOD_STV0360:
		#ifdef STTUNER_DRV_TER_STV0360
		            return( STTUNER_DRV_DEMOD_STV0360_Install(Demod) );
		#else
		            return(ST_ERROR_UNKNOWN_DEVICE);
		#endif
		        case STTUNER_DEMOD_STV0361:
		#ifdef STTUNER_DRV_TER_STV0361
		            return( STTUNER_DRV_DEMOD_STV0361_Install(Demod) );
		#else
		            return(ST_ERROR_UNKNOWN_DEVICE);
		#endif
		       case STTUNER_DEMOD_STV0362:
		#ifdef STTUNER_DRV_TER_STV0362
		            return( STTUNER_DRV_DEMOD_STV0362_Install(Demod) );
		#else
		            return(ST_ERROR_UNKNOWN_DEVICE);
		#endif
		
		       case STTUNER_DEMOD_STB0370VSB:
		#ifdef STTUNER_DRV_TER_STB0370VSB
		            return( STTUNER_DRV_DEMOD_STV0370VSB_Install(Demod) );
		#else
		            return(ST_ERROR_UNKNOWN_DEVICE);
		#endif

       case STTUNER_DEMOD_STV0372:
#ifdef STTUNER_DRV_TER_STV0372
            return( STTUNER_DRV_DEMOD_STV0372_Install(Demod) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif

        default:    /* no match, maybe not a demod device ID so try tuners... */
            break;
    }

    Tuner = &Drv->Terr.TunerDbase[DatabaseIndex];
 

switch(DeviceTypeID)   
    {
	#ifdef STTUNER_DRV_TER_TUN_DTT7572
	case  STTUNER_TUNER_DTT7572:
	            return( STTUNER_DRV_TUNER_DTT7572_Install(Tuner) );
	#endif
	#ifdef STTUNER_DRV_TER_TUN_DTT7578
	case  STTUNER_TUNER_DTT7578:
	            return( STTUNER_DRV_TUNER_DTT7578_Install(Tuner) );
	#endif
	#ifdef STTUNER_DRV_TER_TUN_DTT7592
	case  STTUNER_TUNER_DTT7592:
	
	            return( STTUNER_DRV_TUNER_DTT7592_Install(Tuner) );
	#endif        
	#ifdef STTUNER_DRV_TER_TUN_TDLB7
	case  STTUNER_TUNER_TDLB7:
	            return( STTUNER_DRV_TUNER_TDLB7_Install(Tuner) );
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDQD3
	case  STTUNER_TUNER_TDQD3:
	            return( STTUNER_DRV_TUNER_TDQD3_Install(Tuner) );
	#endif
	#ifdef STTUNER_DRV_TER_TUN_DTT7300X
	case STTUNER_TUNER_DTT7300X:
	            return( STTUNER_DRV_TUNER_DTT7300X_Install(Tuner) );
	
	#endif
	
	#ifdef STTUNER_DRV_TER_TUN_ENG47402G1
	case  STTUNER_TUNER_ENG47402G1:
	
	            return( STTUNER_DRV_TUNER_ENG47402G1_Install(Tuner) );
	 #endif
	#ifdef STTUNER_DRV_TER_TUN_EAL2780
	case  STTUNER_TUNER_EAL2780:
	
	            return( STTUNER_DRV_TUNER_EAL2780_Install(Tuner) );
	#endif        
	#ifdef STTUNER_DRV_TER_TUN_TDA6650
	case  STTUNER_TUNER_TDA6650:
	            return( STTUNER_DRV_TUNER_TDA6650_Install(Tuner) );
	#endif            
	#ifdef STTUNER_DRV_TER_TUN_TDM1316
	case  STTUNER_TUNER_TDM1316:
	            return( STTUNER_DRV_TUNER_TDM1316_Install(Tuner) );
	#endif            
	#ifdef STTUNER_DRV_TER_TUN_TDEB2
	case  STTUNER_TUNER_TDEB2:
	            return( STTUNER_DRV_TUNER_TDEB2_Install(Tuner) );
	#endif
	#ifdef STTUNER_DRV_TER_TUN_ED5058
	case  STTUNER_TUNER_ED5058:
	           return( STTUNER_DRV_TUNER_ED5058_Install(Tuner) );
	#endif
	#ifdef STTUNER_DRV_TER_TUN_MIVAR
	case  STTUNER_TUNER_MIVAR:
	
	            return( STTUNER_DRV_TUNER_MIVAR_Install(Tuner) );
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDED4
	case  STTUNER_TUNER_TDED4:
	            return( STTUNER_DRV_TUNER_TDED4_Install(Tuner) );
	#endif
	#ifdef STTUNER_DRV_TER_TUN_DTT7102
	case  STTUNER_TUNER_DTT7102:
	
	            return( STTUNER_DRV_TUNER_DTT7102_Install(Tuner) );
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TECC2849PG
	case  STTUNER_TUNER_TECC2849PG:
	            return( STTUNER_DRV_TUNER_TECC2849PG_Install(Tuner) );
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDCC2345
	case  STTUNER_TUNER_TDCC2345:
	
	            return( STTUNER_DRV_TUNER_TDCC2345_Install(Tuner) );
	#endif
	
	#ifdef STTUNER_DRV_TER_TUN_RF4000
	  #ifndef ST_OSLINUX
	case  STTUNER_TUNER_RF4000:
	
		    return( STTUNER_DRV_TUNER_RF4000_Install(Tuner) );   
	#endif 
	#endif  
	#ifdef STTUNER_DRV_TER_TUN_TDTGD108
	case  STTUNER_TUNER_TDTGD108:
	            return( STTUNER_DRV_TUNER_TDTGD108_Install(Tuner) );
	#endif 
	
	#ifdef STTUNER_DRV_TER_TUN_MT2266
	case  STTUNER_TUNER_MT2266:
	            return( STTUNER_DRV_TUNER_MT2266_Install(Tuner) );
#endif  

	#ifdef STTUNER_DRV_TER_TUN_DTVS223
	case  STTUNER_TUNER_DTVS223:
      
	    return( STTUNER_DRV_TUNER_DTVS223_Install(Tuner) );	
	#endif	 
         
       default:    
        return(ST_ERROR_UNKNOWN_DEVICE);
      }











#ifdef STTUNER_DEBUG_MODULE_TDRV
        STTBX_Print(("%s fail unknown terrestrial device type ID = %d\n", identity, DeviceTypeID));
#endif

    return(STTUNER_ERROR_ID);   /* no matching devices found */
}



/* ----------------------------------------------------------------------------
Name: STTUNER_TERR_DRV_UnInstall()

Description:
    install a terrestrial device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_TERR_DRV_UnInstall(void)
{
#ifdef STTUNER_DEBUG_MODULE_TDRV
    const char *identity = "STTUNER tdrv.c STTUNER_TERR_DRV_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STTUNER_DEBUG_MODULE_TDRV
    STTBX_Print(("%s uninstall BEGIN\n", identity));
#endif

    /* uninstall drivers from their databases */

    /* demods */
	    TDRV_UnInstallDev(STTUNER_DEMOD_STV0360);
	    TDRV_UnInstallDev(STTUNER_DEMOD_STV0361);
	#ifdef STTUNER_DRV_TER_STV0362
	    TDRV_UnInstallDev(STTUNER_DEMOD_STV0362);
	#endif 
	#ifdef STTUNER_DRV_TER_STB0370VSB
	    TDRV_UnInstallDev(STTUNER_DEMOD_STB0370VSB);
	#endif 
#ifdef STTUNER_DRV_TER_STV0372
    TDRV_UnInstallDev(STTUNER_DEMOD_STV0372);
#endif    

    /* tuners */
    #ifdef STTUNER_DRV_TER_TUN_DTT7572
    TDRV_UnInstallDev(STTUNER_TUNER_DTT7572);
    #endif
    #ifdef STTUNER_DRV_TER_TUN_DTT7578
    TDRV_UnInstallDev(STTUNER_TUNER_DTT7578);
    #endif
    #ifdef STTUNER_DRV_TER_TUN_DTT7592
    TDRV_UnInstallDev(STTUNER_TUNER_DTT7592);
    #endif
    #ifdef STTUNER_DRV_TER_TUN_TDLB7
    TDRV_UnInstallDev(STTUNER_TUNER_TDLB7);
    #endif
    #ifdef STTUNER_DRV_TER_TUN_TDQD3
    TDRV_UnInstallDev(STTUNER_TUNER_TDQD3);
    #endif
    #ifdef STTUNER_DRV_TER_TUN_DTT7300X
    TDRV_UnInstallDev(STTUNER_TUNER_DTT7300X);
    #endif
    
    #ifdef STTUNER_DRV_TER_TUN_ENG47402G1
    TDRV_UnInstallDev(STTUNER_TUNER_ENG47402G1);
    #endif
    #ifdef STTUNER_DRV_TER_TUN_EAL2780
    TDRV_UnInstallDev(STTUNER_TUNER_EAL2780);
    #endif
    #ifdef STTUNER_DRV_TER_TUN_TDA6650
    TDRV_UnInstallDev(STTUNER_TUNER_TDA6650);
    #endif
    #ifdef STTUNER_DRV_TER_TUN_TDM1316
    TDRV_UnInstallDev(STTUNER_TUNER_TDM1316);
    #endif
    #ifdef STTUNER_DRV_TER_TUN_TDEB2
    TDRV_UnInstallDev(STTUNER_TUNER_TDEB2);
    #endif
    /*******New tuner added************/
    #ifdef STTUNER_DRV_TER_TUN_ED5058
    TDRV_UnInstallDev(STTUNER_TUNER_ED5058);
    #endif
     #ifdef STTUNER_DRV_TER_TUN_MIVAR
     TDRV_UnInstallDev(STTUNER_TUNER_MIVAR);
     #endif
     #ifdef STTUNER_DRV_TER_TUN_TDCC2345
    TDRV_UnInstallDev(STTUNER_TUNER_TDCC2345);
    #endif
    #ifdef STTUNER_DRV_TER_TUN_TDED4
    TDRV_UnInstallDev(STTUNER_TUNER_TDED4);
    #endif
    #ifdef STTUNER_DRV_TER_TUN_DTT7102
    TDRV_UnInstallDev(STTUNER_TUNER_DTT7102);
    #endif
    #ifdef STTUNER_DRV_TER_TUN_TECC2849PG
    TDRV_UnInstallDev(STTUNER_TUNER_TECC2849PG);
    #endif
    #ifdef STTUNER_DRV_TER_TUN_TDCC2345
    TDRV_UnInstallDev(STTUNER_TUNER_TDCC2345);
    #endif
    
   #ifdef STTUNER_DRV_TER_TUN_RF4000
     #ifndef ST_OSLINUX
    TDRV_UnInstallDev(STTUNER_TUNER_RF4000); 
    #endif
    #endif
    #ifdef STTUNER_DRV_TER_TUN_MT2266
    TDRV_UnInstallDev(STTUNER_TUNER_MT2266);
    #endif
    
    #ifdef STTUNER_DRV_TER_TUN_TDTGD108
    TDRV_UnInstallDev(STTUNER_TUNER_TDTGD108);
    #endif
    #ifdef STTUNER_DRV_TER_TUN_DTVS223
    TDRV_UnInstallDev(STTUNER_TUNER_DTVS223); 
    #endif
    Error = TDRV_FreeMem(TDRV_MemoryPartition);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TDRV
        STTBX_Print(("%s fail TDRV_FreeMem()\n", identity));
#endif
        return(Error);
    }

    TDRV_MemoryPartition = NULL;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: TDRV_FreeMem()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t TDRV_FreeMem(ST_Partition_t  *MemoryPartition)
{
#ifdef STTUNER_DEBUG_MODULE_TDRV
   const char *identity = "STTUNER tdrv.c TDRV_FreeMem()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_DriverDbase_t *Drv;

    Drv = STTUNER_GetDrvDb();  /* get driver database pointer */

    /* ---------- free memory ----------*/

    Drv->Terr.DemodDbaseSize  = 0;
    Drv->Terr.TunerDbaseSize  = 0;

    STOS_MemoryDeallocate(MemoryPartition, Drv->Terr.DemodDbase);
    STOS_MemoryDeallocate(MemoryPartition, Drv->Terr.TunerDbase);

    Drv->Terr.DemodDbase  = NULL;
    Drv->Terr.TunerDbase  = NULL;

#ifdef STTUNER_DEBUG_MODULE_TDRV
    STTBX_Print(("%s ok\n", identity));
#endif

    return(Error);
}







/* ----------------------------------------------------------------------------
Name: TDRV_UnInstallDev()

Description:
    install a terrestrial device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t TDRV_UnInstallDev(U32 DeviceTypeID)
{
#ifdef STTUNER_DEBUG_MODULE_TDRV
   const char *identity = "STTUNER tdrv.c TDRV_UnInstallDev()";
#endif
    STTUNER_DriverDbase_t  *Drv;

    STTUNER_demod_dbase_t  *Demod;
    STTUNER_tuner_dbase_t  *Tuner;
    U32 index;

    Drv = STTUNER_GetDrvDb();


    if (Drv->Terr.DemodDbaseSize > 0)
    {
        for(index = 0; index <= Drv->Terr.DemodDbaseSize; index++)
        {
            Demod = &Drv->Terr.DemodDbase[index];
            if (DeviceTypeID == Demod->ID)  /* is it a demod? */
            {
                switch(DeviceTypeID)
                {
	                    case STTUNER_DEMOD_STV0360:
	#ifdef STTUNER_DRV_TER_STV0360
	                        return( STTUNER_DRV_DEMOD_STV0360_UnInstall(Demod) );
	#else
	                        return(ST_ERROR_UNKNOWN_DEVICE);
	#endif
	                    case STTUNER_DEMOD_STV0361:
	#ifdef STTUNER_DRV_TER_STV0361
	                        return( STTUNER_DRV_DEMOD_STV0361_UnInstall(Demod) );
	#else
	                        return(ST_ERROR_UNKNOWN_DEVICE);
	#endif
	                    case STTUNER_DEMOD_STV0362:
	#ifdef STTUNER_DRV_TER_STV0362
	                        return( STTUNER_DRV_DEMOD_STV0362_UnInstall(Demod) );
	#else
	                        return(ST_ERROR_UNKNOWN_DEVICE);
	#endif
	                    case STTUNER_DEMOD_STB0370VSB:
	#ifdef   STTUNER_DRV_TER_STB0370VSB
				return( STTUNER_DRV_DEMOD_STV0370VSB_UnInstall(Demod) );
	#else
				 return(ST_ERROR_UNKNOWN_DEVICE);
	#endif           
                    case STTUNER_DEMOD_STV0372:
#ifdef   STTUNER_DRV_TER_STV0372
			return( STTUNER_DRV_DEMOD_STV0372_UnInstall(Demod) );
#else
			 return(ST_ERROR_UNKNOWN_DEVICE);
#endif                         

                    default:    /* no match, maybe not a demod device ID so try tuners... */
                        break;
                }
            }
        }
    }


    if (Drv->Terr.TunerDbaseSize > 0)
    {
        for(index = 0; index <= Drv->Terr.TunerDbaseSize; index++)
        {
            Tuner = &Drv->Terr.TunerDbase[index];
            if (DeviceTypeID == Tuner->ID)  /* is it a tuner? */
            {
                switch(DeviceTypeID)

                {
                    	#ifdef STTUNER_DRV_TER_TUN_DTT7572
                        case STTUNER_TUNER_DTT7572:
                        return( STTUNER_DRV_TUNER_DTT7572_UnInstall(Tuner) );
			#endif 
                   	 #ifdef STTUNER_DRV_TER_TUN_DTT7578
                    	case STTUNER_TUNER_DTT7578:
                        return( STTUNER_DRV_TUNER_DTT7578_UnInstall(Tuner) );
			#endif 
                    	#ifdef STTUNER_DRV_TER_TUN_DTT7592
                    	case STTUNER_TUNER_DTT7592:
                        return( STTUNER_DRV_TUNER_DTT7592_UnInstall(Tuner) );
			#endif 
                    	#ifdef STTUNER_DRV_TER_TUN_TDLB7
                    	case STTUNER_TUNER_TDLB7:
                        return( STTUNER_DRV_TUNER_TDLB7_UnInstall(Tuner) );
                   	 #endif 
                   	#ifdef STTUNER_DRV_TER_TUN_TDQD3
                   	case STTUNER_TUNER_TDQD3:
                        return( STTUNER_DRV_TUNER_TDQD3_UnInstall(Tuner) );
			#endif 
			#ifdef STTUNER_DRV_TER_TUN_DTT7300X
			 case STTUNER_TUNER_DTT7300X:
                        return( STTUNER_DRV_TUNER_DTT7300X_UnInstall(Tuner) );
			#endif 

			
                    	#ifdef STTUNER_DRV_TER_TUN_ENG47402G1
                    	case STTUNER_TUNER_ENG47402G1:
                        return( STTUNER_DRV_TUNER_ENG47402G1_UnInstall(Tuner) );
			#endif 
                    	#ifdef STTUNER_DRV_TER_TUN_EAL2780
                    	case STTUNER_TUNER_EAL2780:
                        return( STTUNER_DRV_TUNER_EAL2780_UnInstall(Tuner) );
			#endif 
                    	#ifdef STTUNER_DRV_TER_TUN_TDA6650
                    	case STTUNER_TUNER_TDA6650:
                        return( STTUNER_DRV_TUNER_TDA6650_UnInstall(Tuner) );
			#endif 
                    	#ifdef STTUNER_DRV_TER_TUN_TDM1316
                    	case STTUNER_TUNER_TDM1316:
                    	    return( STTUNER_DRV_TUNER_TDM1316_UnInstall(Tuner) );
                        #endif 
                   	 #ifdef STTUNER_DRV_TER_TUN_TDEB2
                   	 case STTUNER_TUNER_TDEB2:
                        return( STTUNER_DRV_TUNER_TDEB2_UnInstall(Tuner) );
			#endif 
                    	#ifdef STTUNER_DRV_TER_TUN_ED5058
                    	case STTUNER_TUNER_ED5058:
                        return( STTUNER_DRV_TUNER_ED5058_UnInstall(Tuner) );
			#endif 
                    	#ifdef STTUNER_DRV_TER_TUN_MIVAR
                    	case STTUNER_TUNER_MIVAR:
                        return( STTUNER_DRV_TUNER_MIVAR_UnInstall(Tuner) );
			#endif 
                   	 	#ifdef STTUNER_DRV_TER_TUN_TDED4
						case STTUNER_TUNER_TDED4:
                        return( STTUNER_DRV_TUNER_TDED4_UnInstall(Tuner) );
                 	 	#endif 
                    	#ifdef STTUNER_DRV_TER_TUN_DTT7102
                    	case STTUNER_TUNER_DTT7102:
                        return( STTUNER_DRV_TUNER_DTT7102_UnInstall(Tuner) );
			#endif
                    	#ifdef STTUNER_DRV_TER_TUN_TECC2849PG
                    	case STTUNER_TUNER_TECC2849PG:
                        return( STTUNER_DRV_TUNER_TECC2849PG_UnInstall(Tuner) );
			 #endif 
                    	#ifdef STTUNER_DRV_TER_TUN_TDCC2345
                    	case STTUNER_TUNER_TDCC2345:
                        return( STTUNER_DRV_TUNER_TDCC2345_UnInstall(Tuner) );
			#endif 
                   	 
  		    	#ifdef STTUNER_DRV_TER_TUN_RF4000
  		    	  #ifndef ST_OSLINUX
  		    	case STTUNER_TUNER_RF4000:
			return( STTUNER_DRV_TUNER_RF4000_UnInstall(Tuner) );
			#endif  
			
			#endif
			#ifdef STTUNER_DRV_TER_TUN_TDTGD108
                   	case STTUNER_TUNER_TDTGD108:
                        return( STTUNER_DRV_TUNER_TDTGD108_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_TER_TUN_MT2266
                   	case STTUNER_TUNER_MT2266:
                        return( STTUNER_DRV_TUNER_MT2266_UnInstall(Tuner) );
			#endif 
			
	            	#ifdef STTUNER_DRV_TER_TUN_DTVS223
	            	case STTUNER_TUNER_DTVS223:
			return( STTUNER_DRV_TUNER_DTVS223_UnInstall(Tuner) );  
			#endif 
		    default:   
                    return(ST_ERROR_UNKNOWN_DEVICE);
             
              
                }
            }
        }
    }



#ifdef STTUNER_DEBUG_MODULE_TDRV
        STTBX_Print(("%s fail unknown terrestrial device type ID = %d\n", identity, DeviceTypeID));
#endif

    return(STTUNER_ERROR_ID);   /* no matching devices found */
}



/* End of tdrv.c */
