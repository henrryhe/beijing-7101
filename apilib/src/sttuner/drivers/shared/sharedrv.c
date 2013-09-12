/* ----------------------------------------------------------------------------
File Name: sharedrv.c

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

/* C libs */
#ifndef ST_OSLINUX
   
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
#include "sharedrv.h"       /* header for this file */

#include "ioreg.h"

/* null */
#ifdef STTUNER_DRV_SHARED_NULL
 #include "dnone.h"
 #include "tnone.h"
#endif

#ifdef STTUNER_DRV_SHARED_EXTTUNERS
#include "tunshdrv.h"
#endif


/* private variables ------------------------------------------------------- */

/* memory area to store driver database */
static ST_Partition_t *SHAREDRV_MemoryPartition;


/* local functions --------------------------------------------------------- */

ST_ErrorCode_t SHAREDRV_AllocMem(ST_Partition_t *MemoryPartition);
ST_ErrorCode_t SHAREDRV_FreeMem (ST_Partition_t *MemoryPartition);

ST_ErrorCode_t SHAREDRV_InstallDev  (U32 DeviceTypeID, U32 DatabaseIndex);
ST_ErrorCode_t SHAREDRV_UnInstallDev(U32 DeviceTypeID);


/* ----------------------------------------------------------------------------
Name: STTUNER_SHARE_DRV_Install()

Description:
    install a satellite device driver.
    
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SHARE_DRV_Install(SHARE_InstallParams_t *InstallParams)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED
    const char *identity = "STTUNER sharedrv.c STTUNER_SHARE_DRV_Install()";
#endif
    U32 dbindex;
    ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STTUNER_DEBUG_MODULE_SHARED
    STTBX_Print(("%s install BEGIN\n", identity));
#endif

    SHAREDRV_MemoryPartition = InstallParams->MemoryPartition;

    /* allocate memory for drivers in this build */
    Error = SHAREDRV_AllocMem(SHAREDRV_MemoryPartition);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED
        STTBX_Print(("%s fail SHAREDRV_AllocMem()\n", identity));
#endif
        return(Error);
    }

    /* install drivers in their databases */

    /* demods */
    dbindex = 0; /* reset index for demod database */
    if ( SHAREDRV_InstallDev(STTUNER_DEMOD_NONE, dbindex) == ST_NO_ERROR) dbindex++;
    /* the debug numbers (below) should match the tally in SHAREDRV_AllocMem()*/
#ifdef STTUNER_DEBUG_MODULE_SHARED
    STTBX_Print(("%s installed %d types of demod\n", identity, dbindex));
#endif

 
    /* tuners */
    dbindex = 0; /* reset index for tuner database */
    if ( SHAREDRV_InstallDev(STTUNER_TUNER_NONE,  dbindex) == ST_NO_ERROR) dbindex++;
    #ifdef STTUNER_DRV_SHARED_TUN_TD1336  
    if ( SHAREDRV_InstallDev(STTUNER_TUNER_TD1336,  dbindex) == ST_NO_ERROR) dbindex++;
    #endif
    #ifdef STTUNER_DRV_SHARED_TUN_FQD1236  
    if ( SHAREDRV_InstallDev(STTUNER_TUNER_FQD1236,  dbindex) == ST_NO_ERROR) dbindex++;
    #endif
    #ifdef STTUNER_DRV_SHARED_TUN_T2000  
    if ( SHAREDRV_InstallDev(STTUNER_TUNER_T2000,  dbindex) == ST_NO_ERROR) dbindex++;
    #endif
    #ifdef STTUNER_DRV_SHARED_TUN_DTT7600  
    if ( SHAREDRV_InstallDev(STTUNER_TUNER_DTT7600,  dbindex) == ST_NO_ERROR) dbindex++;
    #endif
    #ifdef STTUNER_DRV_SHARED_TUN_MT2060  
    if ( SHAREDRV_InstallDev(STTUNER_TUNER_MT2060,  dbindex) == ST_NO_ERROR) dbindex++;
    #endif
     #ifdef STTUNER_DRV_SHARED_TUN_MT2131  
    if ( SHAREDRV_InstallDev(STTUNER_TUNER_MT2131,  dbindex) == ST_NO_ERROR) dbindex++;
    #endif
    #ifdef STTUNER_DRV_SHARED_TUN_TDVEH052F  
    if ( SHAREDRV_InstallDev(STTUNER_TUNER_TDVEH052F,  dbindex) == ST_NO_ERROR) dbindex++;
    #endif
     #ifdef STTUNER_DRV_SHARED_TUN_DTT761X  
    if ( SHAREDRV_InstallDev(STTUNER_TUNER_DTT761X,  dbindex) == ST_NO_ERROR) dbindex++;
    #endif
     #ifdef STTUNER_DRV_SHARED_TUN_DTT768XX  
    if ( SHAREDRV_InstallDev(STTUNER_TUNER_DTT768XX,  dbindex) == ST_NO_ERROR) dbindex++;
    #endif
#ifdef STTUNER_DEBUG_MODULE_SHARED
    STTBX_Print(("%s installed %d types of tuner\n", identity, dbindex));
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: SHAREDRV_AllocMem()

Description:
     At first call to STTUNER_Init() count (per type) number of satellite device
    drivers and allocate memory for them.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t SHAREDRV_AllocMem(ST_Partition_t  *MemoryPartition)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED
   const char *identity = "STTUNER sharedrv.c SHAREDRV_AllocMem()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_DriverDbase_t *Drv;
    U32 demods  = 0;
    U32 tuners  = 0;

    Drv  = STTUNER_GetDrvDb();  /* get driver database pointer */

    /* find number of TYPES of device */
#ifdef STTUNER_USE_SHARED

    /* null */
#ifdef STTUNER_DRV_SHARED_NULL
    demods++;   /* dnone.h */
    tuners++;   /* tnone.h */
#endif

/*#ifdef STTUNER_DRV_SHARED_EXTTUNERS
     tuners +=3;   
#endif*/

/* for removing bug ,now tuners ++ according to defined tuner */


#ifdef STTUNER_DRV_SHARED_TUN_TD1336
    tuners++;
#endif
#ifdef STTUNER_DRV_SHARED_TUN_FQD1236
    tuners++;
#endif
#ifdef STTUNER_DRV_SHARED_TUN_T2000
    tuners++;
#endif
#ifdef STTUNER_DRV_SHARED_TUN_MT2131
    tuners++;
#endif
#ifdef STTUNER_DRV_SHARED_TUN_TDVEH052F
    tuners++;
#endif
#ifdef STTUNER_DRV_SHARED_TUN_DTT761X
    tuners++;
#endif
#ifdef STTUNER_DRV_SHARED_TUN_DTT7600
    tuners++;
#endif
#ifdef STTUNER_DRV_SHARED_TUN_DTT768XX
    tuners++;
#endif

#endif

    /* show tally of driver types */
#ifdef STTUNER_DEBUG_MODULE_SHARED
    STTBX_Print(("%s Demodulators = %d\n", identity, demods));
    STTBX_Print(("%s Tuners       = %d\n", identity, tuners));
#endif
  
    /* ---------- allocate memory for drivers ----------*/

    Drv->Share.DemodDbaseSize = demods;
    Drv->Share.DemodDbase     = STOS_MemoryAllocateClear(MemoryPartition, demods, sizeof(STTUNER_demod_dbase_t));
    if (demods > 0)
    {
        if (Drv->Share.DemodDbase == NULL)  
        {
#ifdef STTUNER_DEBUG_MODULE_SHARED
            STTBX_Print(("%s fail memory allocation DemodDbase\n", identity));
#endif          
            return(ST_ERROR_NO_MEMORY);           

        }
    }

    Drv->Share.TunerDbaseSize = tuners;
    if (tuners > 0)
    {
        Drv->Share.TunerDbase = STOS_MemoryAllocateClear(MemoryPartition, tuners, sizeof(STTUNER_tuner_dbase_t));
        if (Drv->Share.TunerDbase == NULL)  
        {
#ifdef STTUNER_DEBUG_MODULE_SHARED
            STTBX_Print(("%s fail memory allocation TunerDbaseSize\n", identity));
#endif    
            return(ST_ERROR_NO_MEMORY);           
        }
    }

    return(Error);
}




/* ----------------------------------------------------------------------------
Name: InstallDev()

Description:
    install device drivers
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t SHAREDRV_InstallDev(U32 DeviceTypeID, U32 DatabaseIndex)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED
   const char *identity = "STTUNER sharedrv.c InstallDev()";
#endif
    STTUNER_DriverDbase_t  *Drv;

    STTUNER_demod_dbase_t  *Demod;
    STTUNER_tuner_dbase_t  *Tuner;

    Drv = STTUNER_GetDrvDb();

    Demod = &Drv->Share.DemodDbase[DatabaseIndex]; /* get pointer to storage element in database */
    switch(DeviceTypeID)    /* is it a demod? */
    {
        case STTUNER_DEMOD_NONE:
#ifdef STTUNER_DRV_SHARED_NULL 
            return( STTUNER_DRV_DEMOD_NONE_Install(Demod) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif
                
        default:    /* no match, maybe not a demod device ID so try tuners... */
            break;
    }
        
    Tuner = &Drv->Share.TunerDbase[DatabaseIndex];
    switch(DeviceTypeID)    /* is it a tuner? */
    {
    	#ifdef STTUNER_DRV_SHARED_NULL 
        case STTUNER_TUNER_NONE:

            return( STTUNER_DRV_TUNER_NONE_Install(Tuner) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#ifdef STTUNER_DRV_SHARED_TUN_TD1336 
	case STTUNER_TUNER_TD1336:
#ifdef STTUNER_DRV_SHARED_EXTTUNERS 
            return( STTUNER_DRV_TUNER_TD1336_Install(Tuner) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#endif
#ifdef STTUNER_DRV_SHARED_TUN_FQD1236 

	case STTUNER_TUNER_FQD1236:
#ifdef STTUNER_DRV_SHARED_EXTTUNERS 
            return( STTUNER_DRV_TUNER_FQD1236_Install(Tuner) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#endif
#ifdef STTUNER_DRV_SHARED_TUN_T2000 

      case STTUNER_TUNER_T2000:
#ifdef STTUNER_DRV_SHARED_EXTTUNERS 
            return( STTUNER_DRV_TUNER_T2000_Install(Tuner) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#endif
#ifdef STTUNER_DRV_SHARED_TUN_DTT7600 

      case STTUNER_TUNER_DTT7600:
#ifdef STTUNER_DRV_SHARED_EXTTUNERS 
            return( STTUNER_DRV_TUNER_DTT7600_Install(Tuner) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#endif
#ifdef STTUNER_DRV_SHARED_TUN_MT2060 

      case STTUNER_TUNER_MT2060:
#ifdef STTUNER_DRV_SHARED_EXTTUNERS 
            return( STTUNER_DRV_TUNER_MT2060_Install(Tuner) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#endif

#ifdef STTUNER_DRV_SHARED_TUN_MT2131 

      case STTUNER_TUNER_MT2131:
#ifdef STTUNER_DRV_SHARED_EXTTUNERS 
            return( STTUNER_DRV_TUNER_MT2131_Install(Tuner) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#endif


#ifdef STTUNER_DRV_SHARED_TUN_TDVEH052F 

      case STTUNER_TUNER_TDVEH052F:
#ifdef STTUNER_DRV_SHARED_EXTTUNERS 
            return( STTUNER_DRV_TUNER_TDVEH052F_Install(Tuner) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#endif
#ifdef STTUNER_DRV_SHARED_TUN_DTT761X 

      case STTUNER_TUNER_DTT761X:
#ifdef STTUNER_DRV_SHARED_EXTTUNERS 
            return( STTUNER_DRV_TUNER_DTT761X_Install(Tuner) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#endif

#ifdef STTUNER_DRV_SHARED_TUN_DTT768XX 

      case STTUNER_TUNER_DTT768XX:
#ifdef STTUNER_DRV_SHARED_EXTTUNERS 
            return( STTUNER_DRV_TUNER_DTT768XX_Install(Tuner) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#endif
        default:    /* no match */
            break;
    }

#ifdef STTUNER_DEBUG_MODULE_SHARED
        STTBX_Print(("%s fail unknown shared driver ID = %d\n", identity, DeviceTypeID));
#endif
     
    return(STTUNER_ERROR_ID);   /* no matching devices found */
}   



/* ----------------------------------------------------------------------------
Name: STTUNER_SHARE_DRV_UnInstall()

Description:
    install a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SHARE_DRV_UnInstall(void)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED
    const char *identity = "STTUNER sharedrv.c STTUNER_SHARE_DRV_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STTUNER_DEBUG_MODULE_SHARED
    STTBX_Print(("%s uninstall BEGIN\n", identity));
#endif

    /* uninstall drivers from their databases */

    /* demods */
    SHAREDRV_UnInstallDev(STTUNER_DEMOD_NONE);
 
    /* tuners */
    #ifdef STTUNER_DRV_SHARED_NULL
    SHAREDRV_UnInstallDev(STTUNER_TUNER_NONE);
    #endif
    #ifdef STTUNER_DRV_SHARED_TUN_TD1336 
    SHAREDRV_UnInstallDev(STTUNER_TUNER_TD1336);
    #endif
    #ifdef STTUNER_DRV_SHARED_TUN_FQD1236 
    SHAREDRV_UnInstallDev(STTUNER_TUNER_FQD1236);
    #endif
    #ifdef STTUNER_DRV_SHARED_TUN_T2000 
    SHAREDRV_UnInstallDev(STTUNER_TUNER_T2000);
	#endif
	#ifdef STTUNER_DRV_SHARED_TUN_DTT7600 
    SHAREDRV_UnInstallDev(STTUNER_TUNER_DTT7600);
	#endif
	
	#ifdef STTUNER_DRV_SHARED_TUN_MT2060 
    SHAREDRV_UnInstallDev(STTUNER_TUNER_MT2060);
	#endif
	
	#ifdef STTUNER_DRV_SHARED_TUN_MT2131 
    SHAREDRV_UnInstallDev(STTUNER_TUNER_MT2131);
	#endif
	
	#ifdef STTUNER_DRV_SHARED_TUN_TDVEH052F 
    SHAREDRV_UnInstallDev(STTUNER_TUNER_TDVEH052F);
	#endif
		#ifdef STTUNER_DRV_SHARED_TUN_DTT761X 
    SHAREDRV_UnInstallDev(STTUNER_TUNER_DTT761X);
	#endif
	#ifdef STTUNER_DRV_SHARED_TUN_DTT768XX 
    SHAREDRV_UnInstallDev(STTUNER_TUNER_DTT768XX);
	#endif
    Error = SHAREDRV_FreeMem(SHAREDRV_MemoryPartition);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED
        STTBX_Print(("%s fail SHAREDRV_FreeMem()\n", identity));
#endif
        return(Error);
    }

    SHAREDRV_MemoryPartition = NULL;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: SHAREDRV_FreeMem()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t SHAREDRV_FreeMem(ST_Partition_t  *MemoryPartition)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED
   const char *identity = "STTUNER sharedrv.c SHAREDRV_FreeMem()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_DriverDbase_t *Drv;

    Drv = STTUNER_GetDrvDb();  /* get driver database pointer */
  
    /* ---------- free memory ----------*/

    Drv->Share.DemodDbaseSize  = 0;
    Drv->Share.TunerDbaseSize  = 0;

    STOS_MemoryDeallocate(MemoryPartition, Drv->Share.DemodDbase);
    STOS_MemoryDeallocate(MemoryPartition, Drv->Share.TunerDbase);

#ifdef STTUNER_DEBUG_MODULE_SHARED
    STTBX_Print(("%s ok\n", identity));
#endif    

    return(Error);
}







/* ----------------------------------------------------------------------------
Name: SHAREDRV_UnInstallDev()

Description:
    install a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t SHAREDRV_UnInstallDev(U32 DeviceTypeID)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED
   const char *identity = "STTUNER sharedrv.c SHAREDRV_UnInstallDev()";
#endif
    STTUNER_DriverDbase_t  *Drv;

    STTUNER_demod_dbase_t  *Demod;
    STTUNER_tuner_dbase_t  *Tuner;
    U32 index;

    Drv = STTUNER_GetDrvDb();


    if (Drv->Share.DemodDbaseSize > 0) 
    {
        for(index = 0; index <= Drv->Share.DemodDbaseSize; index++)
        {
            Demod = &Drv->Share.DemodDbase[index];
            if (DeviceTypeID == Demod->ID)  /* is it a demod? */
            {
                switch(DeviceTypeID)    
                {
                    case STTUNER_DEMOD_NONE:
#ifdef STTUNER_DRV_SHARED_NULL 
                        return( STTUNER_DRV_DEMOD_NONE_UnInstall(Demod) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif

                    default:    /* no match, maybe not a demod device ID so try tuners... */
                        break;
                }
            }
        }
    }


    if (Drv->Share.TunerDbaseSize > 0) 
    {
        for(index = 0; index <= Drv->Share.TunerDbaseSize; index++)
        {
            Tuner = &Drv->Share.TunerDbase[index];
            if (DeviceTypeID == Tuner->ID)  /* is it a tuner? */
            {
                switch(DeviceTypeID)    

                {
 #ifdef STTUNER_DRV_SHARED_NULL 
                    case STTUNER_TUNER_NONE:

                        return( STTUNER_DRV_TUNER_NONE_UnInstall(Tuner) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif
		#ifdef STTUNER_DRV_SHARED_TUN_TD1336 
		   case STTUNER_TUNER_TD1336:                    
#ifdef STTUNER_DRV_SHARED_EXTTUNERS
                        return( STTUNER_DRV_TUNER_TD1336_UnInstall(Tuner) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#endif
		#ifdef STTUNER_DRV_SHARED_TUN_FQD1236 
		  case STTUNER_TUNER_FQD1236:                    
#ifdef STTUNER_DRV_SHARED_EXTTUNERS
                        return( STTUNER_DRV_TUNER_FQD1236_UnInstall(Tuner) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#endif


		#ifdef STTUNER_DRV_SHARED_TUN_T2000 
	          case STTUNER_TUNER_T2000:                    
#ifdef STTUNER_DRV_SHARED_EXTTUNERS
                        return( STTUNER_DRV_TUNER_T2000_UnInstall(Tuner) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#endif

#ifdef STTUNER_DRV_SHARED_TUN_DTT7600 
	          case STTUNER_TUNER_DTT7600:                    
#ifdef STTUNER_DRV_SHARED_EXTTUNERS
                        return( STTUNER_DRV_TUNER_DTT7600_UnInstall(Tuner) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#endif
#ifdef STTUNER_DRV_SHARED_TUN_MT2060 
	          case STTUNER_TUNER_MT2060:                    
#ifdef STTUNER_DRV_SHARED_EXTTUNERS
                        return( STTUNER_DRV_TUNER_MT2060_UnInstall(Tuner) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#endif

#ifdef STTUNER_DRV_SHARED_TUN_MT2131 
	          case STTUNER_TUNER_MT2131:                    
#ifdef STTUNER_DRV_SHARED_EXTTUNERS
                        return( STTUNER_DRV_TUNER_MT2131_UnInstall(Tuner) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#endif

#ifdef STTUNER_DRV_SHARED_TUN_TDVEH052F 
	          case STTUNER_TUNER_TDVEH052F:                    
#ifdef STTUNER_DRV_SHARED_EXTTUNERS
                        return( STTUNER_DRV_TUNER_TDVEH052F_UnInstall(Tuner) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#endif

#ifdef STTUNER_DRV_SHARED_TUN_DTT761X 
	          case STTUNER_TUNER_DTT761X:                    
#ifdef STTUNER_DRV_SHARED_EXTTUNERS
                        return( STTUNER_DRV_TUNER_DTT761X_UnInstall(Tuner) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#endif

#ifdef STTUNER_DRV_SHARED_TUN_DTT768XX 
	          case STTUNER_TUNER_DTT768XX:                    
#ifdef STTUNER_DRV_SHARED_EXTTUNERS
                        return( STTUNER_DRV_TUNER_DTT768XX_UnInstall(Tuner) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif
#endif
                    default:    /* no match */
                        break;
                }
            }
        }
    }



#ifdef STTUNER_DEBUG_MODULE_SHARED
        STTBX_Print(("%s fail unknown shared driver ID = %d\n", identity, DeviceTypeID));
#endif
     
    return(STTUNER_ERROR_ID);   /* no matching devices found */
}   



/* End of sharedrv.c */
