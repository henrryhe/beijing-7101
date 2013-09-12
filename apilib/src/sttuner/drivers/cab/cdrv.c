/* ----------------------------------------------------------------------------
File Name: cdrv.c

Description:


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
#include "stlite.h"     /* Standard includes */

/* STAPI */
#include "sttbx.h"

#include "stevt.h"
#include "sttuner.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "cdrv.h"       /* header for this file */

/* STV0297 chip */
#ifdef STTUNER_DRV_CAB_STV0297
 #include "d0297.h"
#endif

#ifdef STTUNER_DRV_CAB_STV0297J
 #include "d0297j.h"
#endif

#ifdef STTUNER_DRV_CAB_STV0297E
 #include "d0297e.h"
#endif

#ifdef STTUNER_DRV_CAB_STV0498
 #include "d0498.h"
#endif

#ifdef STTUNER_DRV_CAB_STB0370QAM
 #include "d0370qam.h"
#endif

/* external tuners */
#ifdef STTUNER_DRV_CAB_EXTTUNERS
 #include "tcdrv.h"     /* Tuner for cable front-end */
#endif


/* private variables ------------------------------------------------------- */

/* memory area to store driver database */
static ST_Partition_t *CDRV_MemoryPartition;


/* local functions --------------------------------------------------------- */

ST_ErrorCode_t CDRV_AllocMem(ST_Partition_t *MemoryPartition);
ST_ErrorCode_t CDRV_FreeMem (ST_Partition_t *MemoryPartition);

ST_ErrorCode_t CDRV_InstallDev  (U32 DeviceTypeID, U32 DatabaseIndex);
ST_ErrorCode_t CDRV_UnInstallDev(U32 DeviceTypeID);


/* ----------------------------------------------------------------------------
Name: STTUNER_CABLE_DRV_Install()

Description:
    install a cable device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_CABLE_DRV_Install(CDRV_InstallParams_t *InstallParams)
{
#ifdef STTUNER_DEBUG_MODULE_CDRV
    const char *identity = "STTUNER cdrv.c STTUNER_CABLE_DRV_Install()";
#endif
    U32 dbindex;
    ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STTUNER_DEBUG_MODULE_CDRV
    STTBX_Print(("%s install BEGIN\n", identity));
#endif

    CDRV_MemoryPartition = InstallParams->MemoryPartition;

    /* allocate memory for drivers in this build */
    Error = CDRV_AllocMem(CDRV_MemoryPartition);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CDRV
        STTBX_Print(("%s fail CDRV_AllocMem()\n", identity));
#endif
        return(Error);
    }

    /* install drivers in their databases */

    /* demods */
    dbindex = 0; /* reset index for demod database */
    if ( CDRV_InstallDev(STTUNER_DEMOD_STV0297, dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_DEMOD_STV0297J, dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_DEMOD_STV0297E, dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_DEMOD_STV0498, dbindex) == ST_NO_ERROR) dbindex++;
#ifdef  STTUNER_DRV_CAB_STB0370QAM
    if ( CDRV_InstallDev(STTUNER_DEMOD_STB0370QAM, dbindex) == ST_NO_ERROR) dbindex++;
#endif    
    /* the debug numbers (below) should match the tally in CDRV_AllocMem()*/
#ifdef STTUNER_DEBUG_MODULE_CDRV
    STTBX_Print(("%s installed %d types of demod\n", identity, dbindex));
#endif


    /* tuners */
    dbindex = 0; /* reset index for tuner database */
    if ( CDRV_InstallDev(STTUNER_TUNER_TDBE1,       dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_TDBE2,       dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_TDDE1,       dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_SP5730,      dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_MT2030,      dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_MT2040,      dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_MT2050,      dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_MT2060,      dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_DCT7040,     dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_DCT7050,     dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_DCT7710,     dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_DCF8710,     dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_DCF8720,     dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_MACOETA50DR, dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_CD1516LI,    dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_DF1CS1223,   dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_SHARPXX,     dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_TDBE1X016A,  dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_TDBE1X601,   dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_TDEE4X012A,  dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_DCT7045,     dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_TDQE3,       dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_DCF8783,     dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_DCT7045EVAL, dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_DCT70700,    dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_TDCHG,       dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_TDCJG,       dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_TDCGG,       dbindex) == ST_NO_ERROR) dbindex++;
    if ( CDRV_InstallDev(STTUNER_TUNER_MT2011,      dbindex) == ST_NO_ERROR) dbindex++;
    
#ifdef STTUNER_DEBUG_MODULE_CDRV
    STTBX_Print(("%s installed %d types of tuner\n", identity, dbindex));
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: CDRV_AllocMem()

Description:
     At first call to STTUNER_Init() count (per type) number of cable device
    drivers and allocate memory for them.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t CDRV_AllocMem(ST_Partition_t  *MemoryPartition)
{
#ifdef STTUNER_DEBUG_MODULE_CDRV
   const char *identity = "STTUNER cdrv.c CDRV_AllocMem()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_DriverDbase_t *Drv;
    U32 demods  = 0;
    U32 tuners  = 0;

    Drv  = STTUNER_GetDrvDb();  /* get driver database pointer */

    /* find number of TYPES of device */

#ifdef STTUNER_DRV_CAB_STV0297
    demods += 1;
#endif

#ifdef STTUNER_DRV_CAB_STV0297J
    demods += 1;
#endif

#ifdef STTUNER_DRV_CAB_STV0297E
    demods += 1;
#endif

#ifdef STTUNER_DRV_CAB_STV0498
    demods += 1;
#endif

#ifdef STTUNER_DRV_CAB_STB0370QAM
    demods += 1;
#endif

#ifdef STTUNER_DRV_CAB_EXTTUNERS
/*      TDBE1           1
        TDBE2           2
        TDDE1           3
        SP5730          4
        MT2030          5
        MT2040          6
        MT2050          7
        MT2060          8
        DCT7040         9
        DCT7050        10
        DCT7710        11
        DCF8710        12
        DCF8720        13
        MACOETA50DR    14
        CD1516LI       15
        DF1CS1223      16
        SHARPXX        17
        TDBE1X016A     18
        TDBE1X601      19
        TDEE4X012A     20
        DCT7045        21
        TDQE3          22
        DCF8783        23
        DCT7045EVAL    24
        DCT70700       25
        TDCHG          26
        TDCJG          27
        TDCGG          28
        MT2011         29
*/
    tuners += 29;
#endif

    /* show tally of driver types */
#ifdef STTUNER_DEBUG_MODULE_CDRV
    STTBX_Print(("%s Demodulators = %d\n", identity, demods));
    STTBX_Print(("%s Tuners       = %d\n", identity, tuners));
#endif

    /* ---------- allocate memory for drivers ----------*/

    Drv->Cable.DemodDbaseSize = demods;
    Drv->Cable.DemodDbase     = STOS_MemoryAllocateClear(MemoryPartition, demods, sizeof(STTUNER_demod_dbase_t));
    if (demods > 0)
    {
        if (Drv->Cable.DemodDbase == NULL)
        {
#ifdef STTUNER_DEBUG_MODULE_CDRV
            STTBX_Print(("%s fail memory allocation DemodDbase\n", identity));
#endif
            return(ST_ERROR_NO_MEMORY);

        }
    }

    Drv->Cable.TunerDbaseSize = tuners;
    if (tuners > 0)
    {
        Drv->Cable.TunerDbase = STOS_MemoryAllocateClear(MemoryPartition, tuners, sizeof(STTUNER_tuner_dbase_t));
        if (Drv->Cable.TunerDbase == NULL)
        {
#ifdef STTUNER_DEBUG_MODULE_CDRV
            STTBX_Print(("%s fail memory allocation TunerDbaseSize\n", identity));
#endif
            return(ST_ERROR_NO_MEMORY);
        }
    }

    return(Error);
}




/* ----------------------------------------------------------------------------
Name: CDRV_InstallDev()

Description:
    install a cable device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t CDRV_InstallDev(U32 DeviceTypeID, U32 DatabaseIndex)
{
#ifdef STTUNER_DEBUG_MODULE_CDRV
   const char *identity = "STTUNER cdrv.c CDRV_InstallDev()";
#endif
    STTUNER_DriverDbase_t  *Drv;

    STTUNER_demod_dbase_t  *Demod;
    STTUNER_tuner_dbase_t  *Tuner;

    Drv = STTUNER_GetDrvDb();

    Demod = &Drv->Cable.DemodDbase[DatabaseIndex]; /* get pointer to storage element in database */
    switch(DeviceTypeID)    /* is it a demod? */
    {
#ifdef STTUNER_DRV_CAB_STV0297
       case STTUNER_DEMOD_STV0297:
            return( STTUNER_DRV_DEMOD_STV0297_Install(Demod) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif

#ifdef STTUNER_DRV_CAB_STV0297J
        case STTUNER_DEMOD_STV0297J:
            return( STTUNER_DRV_DEMOD_STV0297J_Install(Demod) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif

#ifdef STTUNER_DRV_CAB_STV0297E
        case STTUNER_DEMOD_STV0297E:
            return( STTUNER_DRV_DEMOD_STV0297E_Install(Demod) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif

#ifdef STTUNER_DRV_CAB_STV0498
        case STTUNER_DEMOD_STV0498:
            return( STTUNER_DRV_DEMOD_STV0498_Install(Demod) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif

#ifdef STTUNER_DRV_CAB_STB0370QAM
 	case STTUNER_DEMOD_STB0370QAM:
            return( STTUNER_DRV_DEMOD_STB0370QAM_Install(Demod) );
#else
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif


        default:    /* no match, maybe not a demod device ID so try tuners... */
            break;
    }

    Tuner = &Drv->Cable.TunerDbase[DatabaseIndex];
    switch(DeviceTypeID)    /* is it a tuner? */
    {
#ifdef STTUNER_DRV_CAB_EXTTUNERS
	#ifdef STTUNER_DRV_CAB_TUN_TDBE1
        case STTUNER_TUNER_TDBE1:
            return( STTUNER_DRV_TUNER_TDBE1_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_TDBE2    
        case STTUNER_TUNER_TDBE2:
            return( STTUNER_DRV_TUNER_TDBE2_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_TDDE1
       case STTUNER_TUNER_TDDE1:
           return( STTUNER_DRV_TUNER_TDDE1_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_SP5730
        case STTUNER_TUNER_SP5730:
            return( STTUNER_DRV_TUNER_SP5730_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_MT2030
        case STTUNER_TUNER_MT2030:
            return( STTUNER_DRV_TUNER_MT2030_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_MT2040
        case STTUNER_TUNER_MT2040:
            return( STTUNER_DRV_TUNER_MT2040_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_MT2050
        case STTUNER_TUNER_MT2050:
            return( STTUNER_DRV_TUNER_MT2050_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_MT2060
        case STTUNER_TUNER_MT2060:
            return( STTUNER_DRV_TUNER_MT2060_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_DCT7040
        case STTUNER_TUNER_DCT7040:
            return( STTUNER_DRV_TUNER_DCT7040_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_DCT7050
        case STTUNER_TUNER_DCT7050:
            return( STTUNER_DRV_TUNER_DCT7050_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_DCT7710
        case STTUNER_TUNER_DCT7710:
            return( STTUNER_DRV_TUNER_DCT7710_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_DCF8710
        case STTUNER_TUNER_DCF8710:
            return( STTUNER_DRV_TUNER_DCF8710_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_DCF8720
        case STTUNER_TUNER_DCF8720:
            return( STTUNER_DRV_TUNER_DCF8720_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_MACOETA50DR
        case STTUNER_TUNER_MACOETA50DR:
            return( STTUNER_DRV_TUNER_MACOETA50DR_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_CD1516LI
        case STTUNER_TUNER_CD1516LI:
            return( STTUNER_DRV_TUNER_CD1516LI_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_DF1CS1223
        case STTUNER_TUNER_DF1CS1223:
            return( STTUNER_DRV_TUNER_DF1CS1223_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_SHARPXX
        case STTUNER_TUNER_SHARPXX:
            return( STTUNER_DRV_TUNER_SHARPXX_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_TDBE1X016A
        case STTUNER_TUNER_TDBE1X016A:
            return( STTUNER_DRV_TUNER_TDBE1X016A_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_TDBE1X601
        case STTUNER_TUNER_TDBE1X601:
            return( STTUNER_DRV_TUNER_TDBE1X601_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_TDEE4X012A
        case STTUNER_TUNER_TDEE4X012A:
            return( STTUNER_DRV_TUNER_TDEE4X012A_Install(Tuner) );
	    #endif
        #ifdef STTUNER_DRV_CAB_TUN_DCT7045
        case STTUNER_TUNER_DCT7045:
            return( STTUNER_DRV_TUNER_DCT7045_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_TDQE3
        case STTUNER_TUNER_TDQE3:
            return( STTUNER_DRV_TUNER_TDQE3_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_DCF8783
        case STTUNER_TUNER_DCF8783:
            return( STTUNER_DRV_TUNER_DCF8783_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_DCT7045EVAL
        case STTUNER_TUNER_DCT7045EVAL:
            return( STTUNER_DRV_TUNER_DCT7045EVAL_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_DCT70700
        case STTUNER_TUNER_DCT70700:
            return( STTUNER_DRV_TUNER_DCT70700_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_TDCHG
        case STTUNER_TUNER_TDCHG:
            return( STTUNER_DRV_TUNER_TDCHG_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_TDCJG
        case STTUNER_TUNER_TDCJG:
            return( STTUNER_DRV_TUNER_TDCJG_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_TDCGG
        case STTUNER_TUNER_TDCGG:
            return( STTUNER_DRV_TUNER_TDCGG_Install(Tuner) );
        #endif
        #ifdef STTUNER_DRV_CAB_TUN_MT2011
        case STTUNER_TUNER_MT2011:
            return( STTUNER_DRV_TUNER_MT2011_Install(Tuner) );
        #endif
#else
        case STTUNER_TUNER_TDBE1:
        case STTUNER_TUNER_TDBE2:
        case STTUNER_TUNER_TDDE1:
        case STTUNER_TUNER_SP5730:
        case STTUNER_TUNER_MT2030:
        case STTUNER_TUNER_MT2040:
        case STTUNER_TUNER_MT2050:
        case STTUNER_TUNER_MT2060:
        case STTUNER_TUNER_DCT7040:
        case STTUNER_TUNER_DCT7050:
        case STTUNER_TUNER_DCT7710:
        case STTUNER_TUNER_DCF8710:
        case STTUNER_TUNER_DCF8720:
        case STTUNER_TUNER_MACOETA50DR:
        case STTUNER_TUNER_CD1516LI:
        case STTUNER_TUNER_DF1CS1223:
        case STTUNER_TUNER_SHARPXX:
        case STTUNER_TUNER_TDBE1X016A:
        case STTUNER_TUNER_TDBE1X601:
        case STTUNER_TUNER_TDEE4X012A:
        case STTUNER_TUNER_DCT7045:
        case STTUNER_TUNER_TDQE3:
        case STTUNER_TUNER_DCF8783:
        case STTUNER_TUNER_DCT7045EVAL:
        case STTUNER_TUNER_DCT70700:
        case STTUNER_TUNER_TDCHG:
        case STTUNER_TUNER_TDCJG:
        case STTUNER_TUNER_TDCGG:
        case STTUNER_TUNER_MT2011:
            return(ST_ERROR_UNKNOWN_DEVICE);
#endif

        default:    /* no match */
            break;
    }

#ifdef STTUNER_DEBUG_MODULE_CDRV
        STTBX_Print(("%s fail unknown cable device type ID = %d\n", identity, DeviceTypeID));
#endif

    return(STTUNER_ERROR_ID);   /* no matching devices found */
}



/* ----------------------------------------------------------------------------
Name: STTUNER_CABLE_DRV_UnInstall()

Description:
    install a cable device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_CABLE_DRV_UnInstall(void)
{
#ifdef STTUNER_DEBUG_MODULE_CDRV
    const char *identity = "STTUNER cdrv.c STTUNER_CABLE_DRV_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STTUNER_DEBUG_MODULE_CDRV
    STTBX_Print(("%s uninstall BEGIN\n", identity));
#endif

    /* uninstall drivers from their databases */

    /* demods */
    CDRV_UnInstallDev(STTUNER_DEMOD_STV0297);
    CDRV_UnInstallDev(STTUNER_DEMOD_STV0297J);
    CDRV_UnInstallDev(STTUNER_DEMOD_STV0297E);
    CDRV_UnInstallDev(STTUNER_DEMOD_STV0498);
    CDRV_UnInstallDev(STTUNER_DEMOD_STB0370QAM);

    /* tuners */
    #ifdef STTUNER_DRV_CAB_TUN_TDBE1
    CDRV_UnInstallDev(STTUNER_TUNER_TDBE1);    
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_TDBE2
    CDRV_UnInstallDev(STTUNER_TUNER_TDBE2);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_TDDE1
    CDRV_UnInstallDev(STTUNER_TUNER_TDDE1);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_SP5730
    CDRV_UnInstallDev(STTUNER_TUNER_SP5730);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_MT2030
    CDRV_UnInstallDev(STTUNER_TUNER_MT2030);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_MT2040
    CDRV_UnInstallDev(STTUNER_TUNER_MT2040);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_MT2050
    CDRV_UnInstallDev(STTUNER_TUNER_MT2050);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_MT2060
    CDRV_UnInstallDev(STTUNER_TUNER_MT2060);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_DCT7040
    CDRV_UnInstallDev(STTUNER_TUNER_DCT7040);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_DCT7050
    CDRV_UnInstallDev(STTUNER_TUNER_DCT7050);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_DCT7710
    CDRV_UnInstallDev(STTUNER_TUNER_DCT7710);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_DCF8710
    CDRV_UnInstallDev(STTUNER_TUNER_DCF8710);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_DCF8720
    CDRV_UnInstallDev(STTUNER_TUNER_DCF8720);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_MACOETA50DR
    CDRV_UnInstallDev(STTUNER_TUNER_MACOETA50DR);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_CD1516LI
    CDRV_UnInstallDev(STTUNER_TUNER_CD1516LI);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_DF1CS1223
    CDRV_UnInstallDev(STTUNER_TUNER_DF1CS1223);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_SHARPXX
    CDRV_UnInstallDev(STTUNER_TUNER_SHARPXX);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_TDBE1X016A
    CDRV_UnInstallDev(STTUNER_TUNER_TDBE1X016A);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_TDBE1X601
    CDRV_UnInstallDev(STTUNER_TUNER_TDBE1X601);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_TDEE4X012A
    CDRV_UnInstallDev(STTUNER_TUNER_TDEE4X012A);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_DCT7045
    CDRV_UnInstallDev(STTUNER_TUNER_DCT7045);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_TDQE3
    CDRV_UnInstallDev(STTUNER_TUNER_TDQE3);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_DCF8783
    CDRV_UnInstallDev(STTUNER_TUNER_DCF8783);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_DCT7045EVAL
    CDRV_UnInstallDev(STTUNER_TUNER_DCT7045EVAL);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_DCT70700
    CDRV_UnInstallDev(STTUNER_TUNER_DCT70700);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_TDCHG
    CDRV_UnInstallDev(STTUNER_TUNER_TDCHG);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_TDCJG
    CDRV_UnInstallDev(STTUNER_TUNER_TDCJG);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_TDCGG
    CDRV_UnInstallDev(STTUNER_TUNER_TDCGG);
    #endif
    #ifdef STTUNER_DRV_CAB_TUN_MT2011
    CDRV_UnInstallDev(STTUNER_TUNER_MT2011);
    #endif
    Error = CDRV_FreeMem(CDRV_MemoryPartition);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CDRV
        STTBX_Print(("%s fail CDRV_FreeMem()\n", identity));
#endif
        return(Error);
    }

    CDRV_MemoryPartition = NULL;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: CDRV_FreeMem()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t CDRV_FreeMem(ST_Partition_t  *MemoryPartition)
{
#ifdef STTUNER_DEBUG_MODULE_CDRV
   const char *identity = "STTUNER cdrv.c CDRV_FreeMem()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_DriverDbase_t *Drv;

    Drv = STTUNER_GetDrvDb();  /* get driver database pointer */

    /* ---------- free memory ----------*/

    Drv->Cable.DemodDbaseSize  = 0;
    Drv->Cable.TunerDbaseSize  = 0;

    STOS_MemoryDeallocate(MemoryPartition, Drv->Cable.DemodDbase);
    STOS_MemoryDeallocate(MemoryPartition, Drv->Cable.TunerDbase);

    Drv->Cable.DemodDbase  = NULL;
    Drv->Cable.TunerDbase  = NULL;

#ifdef STTUNER_DEBUG_MODULE_CDRV
    STTBX_Print(("%s ok\n", identity));
#endif

    return(Error);
}







/* ----------------------------------------------------------------------------
Name: CDRV_UnInstallDev()

Description:
    uninstall a cable device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t CDRV_UnInstallDev(U32 DeviceTypeID)
{
#ifdef STTUNER_DEBUG_MODULE_CDRV
   const char *identity = "STTUNER cdrv.c CDRV_UnInstallDev()";
#endif
    STTUNER_DriverDbase_t  *Drv;

    STTUNER_demod_dbase_t  *Demod;
    STTUNER_tuner_dbase_t  *Tuner;

    U32 index;

    Drv = STTUNER_GetDrvDb();


    if (Drv->Cable.DemodDbaseSize > 0)
    {
        for(index = 0; index <= Drv->Cable.DemodDbaseSize; index++)
        {
            Demod = &Drv->Cable.DemodDbase[index];
            if (DeviceTypeID == Demod->ID)  /* is it a demod? */
            {
                switch(DeviceTypeID)
                {
                    case STTUNER_DEMOD_STV0297:
#ifdef STTUNER_DRV_CAB_STV0297
                        return( STTUNER_DRV_DEMOD_STV0297_UnInstall(Demod) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif

                    case STTUNER_DEMOD_STV0297J:
#ifdef STTUNER_DRV_CAB_STV0297J
                        return( STTUNER_DRV_DEMOD_STV0297J_UnInstall(Demod) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif

                    case STTUNER_DEMOD_STV0297E:
#ifdef STTUNER_DRV_CAB_STV0297E
                        return( STTUNER_DRV_DEMOD_STV0297E_UnInstall(Demod) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif
                  
                   case STTUNER_DEMOD_STV0498:
#ifdef STTUNER_DRV_CAB_STV0498
                        return( STTUNER_DRV_DEMOD_STV0498_UnInstall(Demod) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif

		   case STTUNER_DEMOD_STB0370QAM:
#ifdef STTUNER_DRV_CAB_STB0370QAM
                        return( STTUNER_DRV_DEMOD_STB0370QAM_UnInstall(Demod) );
#else
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif

                    default:    /* no match, maybe not a demod device ID so try tuners... */
                        break;
                }
            }
        }
    }


    if (Drv->Cable.TunerDbaseSize > 0)
    {
        for(index = 0; index <= Drv->Cable.TunerDbaseSize; index++)
        {
            Tuner = &Drv->Cable.TunerDbase[index];
            if (DeviceTypeID == Tuner->ID)  /* is it a tuner? */
            {
                switch(DeviceTypeID)

                {
#ifdef STTUNER_DRV_CAB_EXTTUNERS
			#ifdef STTUNER_DRV_CAB_TUN_TDBE1
                    case STTUNER_TUNER_TDBE1:
                        return( STTUNER_DRV_TUNER_TDBE1_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_TDBE2
                    case STTUNER_TUNER_TDBE2:
                        return( STTUNER_DRV_TUNER_TDBE2_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_TDDE1
                    case STTUNER_TUNER_TDDE1:
                        return( STTUNER_DRV_TUNER_TDDE1_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_SP5730
                    case STTUNER_TUNER_SP5730:
                        return( STTUNER_DRV_TUNER_SP5730_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_MT2030
                    case STTUNER_TUNER_MT2030:
                        return( STTUNER_DRV_TUNER_MT2030_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_MT2040
                    case STTUNER_TUNER_MT2040:
                        return( STTUNER_DRV_TUNER_MT2040_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_MT2050
                    case STTUNER_TUNER_MT2050:
                        return( STTUNER_DRV_TUNER_MT2050_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_MT2060
                    case STTUNER_TUNER_MT2060:
                        return( STTUNER_DRV_TUNER_MT2060_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_DCT7040
                    case STTUNER_TUNER_DCT7040:
                        return( STTUNER_DRV_TUNER_DCT7040_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_DCT7050
                    case STTUNER_TUNER_DCT7050:
                        return( STTUNER_DRV_TUNER_DCT7050_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_DCT7710
                    case STTUNER_TUNER_DCT7710:
                        return( STTUNER_DRV_TUNER_DCT7710_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_DCF8710
                    case STTUNER_TUNER_DCF8710:
                        return( STTUNER_DRV_TUNER_DCF8710_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_DCF8720
                    case STTUNER_TUNER_DCF8720:
                        return( STTUNER_DRV_TUNER_DCF8720_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_MACOETA50DR
                    case STTUNER_TUNER_MACOETA50DR:
                        return( STTUNER_DRV_TUNER_MACOETA50DR_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_CD1516LI
                    case STTUNER_TUNER_CD1516LI:
                        return( STTUNER_DRV_TUNER_CD1516LI_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_DF1CS1223
                    case STTUNER_TUNER_DF1CS1223:
                        return( STTUNER_DRV_TUNER_DF1CS1223_UnInstall(Tuner) );
                         #endif
                        #ifdef STTUNER_DRV_CAB_TUN_SHARPXX
                    case STTUNER_TUNER_SHARPXX:
                        return( STTUNER_DRV_TUNER_SHARPXX_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_TDBE1X016A
                    case STTUNER_TUNER_TDBE1X016A:
                        return( STTUNER_DRV_TUNER_TDBE1X016A_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_TDBE1X601
                    case STTUNER_TUNER_TDBE1X601:
                        return( STTUNER_DRV_TUNER_TDBE1X601_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_TDEE4X012A
                    case STTUNER_TUNER_TDEE4X012A:
                        return( STTUNER_DRV_TUNER_TDEE4X012A_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_DCT7045
                    case STTUNER_TUNER_DCT7045:
                        return( STTUNER_DRV_TUNER_DCT7045_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_TDQE3
                    case STTUNER_TUNER_TDQE3:
                        return( STTUNER_DRV_TUNER_TDQE3_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_DCF8783
                    case STTUNER_TUNER_DCF8783:
                        return( STTUNER_DRV_TUNER_DCF8783_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_DCT7045EVAL
                    case STTUNER_TUNER_DCT7045EVAL:
                        return( STTUNER_DRV_TUNER_DCT7045EVAL_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_DCT70700
                    case STTUNER_TUNER_DCT70700:
                        return( STTUNER_DRV_TUNER_DCT70700_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_TDCHG
                    case STTUNER_TUNER_TDCHG:
                        return( STTUNER_DRV_TUNER_TDCHG_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_TDCJG
                    case STTUNER_TUNER_TDCJG:
                        return( STTUNER_DRV_TUNER_TDCJG_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_TDCGG
                    case STTUNER_TUNER_TDCGG:
                        return( STTUNER_DRV_TUNER_TDCGG_UnInstall(Tuner) );
                        #endif
                        #ifdef STTUNER_DRV_CAB_TUN_MT2011
                    case STTUNER_TUNER_MT2011:
                        return( STTUNER_DRV_TUNER_MT2011_UnInstall(Tuner) );
                        #endif    
                        
#else
                    case STTUNER_TUNER_TDBE1:
                    case STTUNER_TUNER_TDBE2:
                    case STTUNER_TUNER_TDDE1:
                    case STTUNER_TUNER_SP5730:
                    case STTUNER_TUNER_MT2030:
                    case STTUNER_TUNER_MT2040:
                    case STTUNER_TUNER_MT2050:
                    case STTUNER_TUNER_MT2060:
                    case STTUNER_TUNER_DCT7040:
                    case STTUNER_TUNER_DCT7050:
                    case STTUNER_TUNER_DCT7710:
                    case STTUNER_TUNER_DCF8710:
                    case STTUNER_TUNER_DCF8720:
                    case STTUNER_TUNER_MACOETA50DR:
                    case STTUNER_TUNER_CD1516LI:
                    case STTUNER_TUNER_DF1CS1223:
                    case STTUNER_TUNER_SHARPXX:
                    case STTUNER_TUNER_TDBE1X016A:
                    case STTUNER_TUNER_TDBE1X601:
                    case STTUNER_TUNER_TDEE4X012A:
                    case STTUNER_TUNER_DCT7045:
                    case STTUNER_TUNER_TDQE3:
                    case STTUNER_TUNER_DCF8783:
                    case STTUNER_TUNER_DCT7045EVAL:
                    case STTUNER_TUNER_DCT70700:
                    case STTUNER_TUNER_TDCHG:
                    case STTUNER_TUNER_TDCJG:
                    case STTUNER_TUNER_TDCGG:
                    case STTUNER_TUNER_MT2011:
                        return(ST_ERROR_UNKNOWN_DEVICE);
#endif

                    default:    /* no match */
                        break;
                }
            }
        }
    }


#ifdef STTUNER_DEBUG_MODULE_CDRV
        STTBX_Print(("%s fail unknown cable device type ID = %d\n", identity, DeviceTypeID));
#endif

    return(STTUNER_ERROR_ID);   /* no matching devices found */
}



/* End of cdrv.c */
