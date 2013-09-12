/* ----------------------------------------------------------------------------
File Name: init.c

Description:

     This module handles the ter management functions for the STAPI level
    interface for STTUNER for Init.


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
/* Standard includes */
#include "stlite.h"

/* STAPI */
#include "sttbx.h"

#include "sttuner.h"

/* local to sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "terapi.h"     /* terrestrial manager API (functions in this file exported via this header) */
#include "chip.h"
#ifdef STTUNER_MINIDRIVER
  #ifdef STTUNER_DRV_TER_STV0360
	
	#include "360_echo.h" 
	#include "360_drv.h" /**/ 
	#include "d0360.h"
  #endif
	#include "tuntdrv.h"
	
	#include "iodirect.h"
#endif

/* driver API*/
#ifndef STTUNER_MINIDRIVER
  #include "util.h" /* generic utility functions for sttuner */
  #include "stevt.h"
  #include "drivers.h"
#endif

/* invalid index into an array */
#define BAD_INDEX (0xffffffff)

#ifndef STTUNER_MINIDRIVER
U32 TERR_FindIndex_SharedTuner(STTUNER_TunerType_t TunerType);
U32 TERR_FindIndex_SharedDemod(STTUNER_DemodType_t DemodType);
#endif
/* ----------------------------------------------------------------------------
Name: STTUNER_TERR_Init()

Description:
    Initializes the tuner device.

Parameters:
    DeviceName  STTUNER global instance name device name as set during initialization.
   *InitParams  pointer to initialization parameters.

Return Value:
    ST_NO_ERROR,                    the operation was carried out without error.
    ST_ERROR_NO_MEMORY,             memory allocation failed.

See Also:
    STTUNER_Term()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_TERR_Init(STTUNER_Handle_t Handle, STTUNER_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
    const char *identity = "STTUNER init.c STTUNER_TERR_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
#ifndef STTUNER_MINIDRIVER
    U32 Demod_DriverIndex, Tuner_DriverIndex;
#endif

    DEMOD_InitParams_t  Demod_InitParams;
    TUNER_InitParams_t  Tuner_InitParams;
    STTUNER_IOARCH_OpenParams_t  Demod_IOParams, Tuner_IOParams;
    IOARCH_Handle_t              Demod_IOHandle, Tuner_IOHandle;


    STTUNER_DriverDbase_t   *Drv;
    STTUNER_InstanceDbase_t *Inst;

#ifndef STTUNER_MINIDRIVER
    /* ---------- check terrestrial specific bits of InitParams data ---------- */
    Error = STTUNER_TERR_Utils_InitParamCheck(InitParams);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
        STTBX_Print(("%s fail InitParams Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    /* ---------- access driver database and instance database ---------- */
    Drv  = STTUNER_GetDrvDb();
    Inst = STTUNER_GetDrvInst();


    /* clear out any old instance info */
    memset(&Inst[Handle].Terr, 0x00, sizeof(STTUNER_TERR_InstanceDbase_t));

    /* ---------- look for a shared demod driver of the required type ---------- */
    Demod_DriverIndex = TERR_FindIndex_SharedDemod(InitParams->DemodType);
    if (Demod_DriverIndex == BAD_INDEX)
    {
        /* ---------- look for a ter demod driver of the required type ---------- */
        Demod_DriverIndex = 0;
        while(Drv->Terr.DemodDbase[Demod_DriverIndex].ID != InitParams->DemodType)
        {
            Demod_DriverIndex++;
            if (Demod_DriverIndex >= Drv->Terr.DemodDbaseSize)
            {

#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
                STTBX_Print(("%s fail demod not found\n", identity));
#endif
                return(ST_ERROR_UNKNOWN_DEVICE);
            }
        }
        Inst[Handle].Terr.Demod.Driver = &Drv->Terr.DemodDbase[Demod_DriverIndex];
#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
        STTBX_Print(("%s TERR demod %d dbase index %d\n", identity, InitParams->DemodType, Demod_DriverIndex));
#endif
    }
    else
    {
#ifdef STTUNER_USE_SHARED
        Inst[Handle].Terr.Demod.Driver = &Drv->Share.DemodDbase[Demod_DriverIndex];
#endif
#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
        STTBX_Print(("%s SHARED demod %d dbase index %d\n", identity, InitParams->DemodType, Demod_DriverIndex));
#endif
    }


    /* ---------- look for a shared tuner driver of the required type ---------- */
    Tuner_DriverIndex = TERR_FindIndex_SharedTuner(InitParams->TunerType);
    if (Tuner_DriverIndex == BAD_INDEX)
    {
        /* ---------- look for a tuner driver of the required type ---------- */
        Tuner_DriverIndex = 0;
        while(Drv->Terr.TunerDbase[Tuner_DriverIndex].ID != InitParams->TunerType)
        {
            Tuner_DriverIndex++;
            if (Tuner_DriverIndex >= Drv->Terr.TunerDbaseSize)
            {

#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
                STTBX_Print(("%s fail Tuner not found\n", identity));
#endif
                return(ST_ERROR_UNKNOWN_DEVICE);
            }
        }
        Inst[Handle].Terr.Tuner.Driver = &Drv->Terr.TunerDbase[Tuner_DriverIndex];
#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
        STTBX_Print(("%s TERR Tuner %d dbase index %d\n", identity, InitParams->TunerType, Tuner_DriverIndex));
#endif
    }
    else
    {
#ifdef STTUNER_USE_SHARED
        Inst[Handle].Terr.Tuner.Driver = &Drv->Share.TunerDbase[Tuner_DriverIndex];
#endif
#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
        STTBX_Print(("%s SHARED Tuner %d dbase index %d\n", identity, InitParams->TunerType, Tuner_DriverIndex));
#endif
    }


    /* all I/O paths lead to a demod driver, don't allow anything else */
    if (InitParams->DemodIO.Route != STTUNER_IO_DIRECT)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
        STTBX_Print(("%s only direct mode allowed for demod\n", identity, Error));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* ---------- configure demod I/O ---------- */
    Demod_IOParams.IORoute        = InitParams->DemodIO.Route;    /* direct, repeater or passthru */
    Demod_IOParams.IODriver       = InitParams->DemodIO.Driver;   /* null, I2C or debug driver    */
    Demod_IOParams.Address        = InitParams->DemodIO.Address;  /* IO address to use */
    Demod_IOParams.hInstance      = NULL;   /* no handle to instance of redirection function */
    Demod_IOParams.TargetFunction = NULL;   /* no target function for repeater or passthru mode */

    strcpy(Demod_IOParams.DriverName, InitParams->DemodIO.DriverName); /* name, of device address (I2C etc.) */

    Error = ChipOpen(&Demod_IOHandle,&Demod_IOParams);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
        STTBX_Print(("%s fail open demod I/O Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    /* save I/O handle (as driver is not responsible for open/close of this handle) */
    /*Inst[Handle].Terr.Tuner.IOHandle = Tuner_IOHandle;*/
      Inst[Handle].Terr.Demod.IOHandle = Demod_IOHandle;

    /* ---------- configure tuner I/O ---------- */
    Tuner_IOParams.IORoute  = InitParams->TunerIO.Route;    /* direct, repeater or passthru */
    Tuner_IOParams.IODriver = InitParams->TunerIO.Driver;   /* null, I2C or debug driver    */
    Tuner_IOParams.Address  = InitParams->TunerIO.Address;  /* IO address to use */
    /* location of handle to instance of redirection fn(), DrvHandle is returned when driver is opened */
    
    Tuner_IOParams.hInstance      = &Inst[Handle].Terr.Demod.DrvHandle;
    /* use demod for repeater/passthrough */
    Tuner_IOParams.TargetFunction = Inst[Handle].Terr.Demod.Driver->demod_ioaccess;
    Tuner_IOParams.RepeaterFn = Inst[Handle].Terr.Demod.Driver->demod_repeateraccess;
    strcpy(Tuner_IOParams.DriverName, InitParams->TunerIO.DriverName); /* name, of device address (I2C etc.) */

    Error = ChipOpen(&Tuner_IOHandle,&Tuner_IOParams);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
        STTBX_Print(("%s fail open tuner I/O Error=%d\n", identity, Error));
#endif
        ChipClose(Demod_IOHandle);
        return(Error);
    }
    /* save I/O handle (as driver is not responsible for open/close of this handle) */
    Inst[Handle].Terr.Tuner.IOHandle = Tuner_IOHandle;


    /* ---------- get misc values ---------- */
    Inst[Handle].Terr.ExternalClock = InitParams->ExternalClock;

    /* ---------- initalize demod driver ---------- */
    Demod_InitParams.IOHandle          = Demod_IOHandle;
    Demod_InitParams.MemoryPartition   = InitParams->DriverPartition;
    Demod_InitParams.ExternalClock     = InitParams->ExternalClock;
    Demod_InitParams.TSOutputMode      = InitParams->TSOutputMode;
    Demod_InitParams.SerialDataMode    = InitParams->SerialDataMode;
    Demod_InitParams.SerialClockSource = InitParams->SerialClockSource;
    Demod_InitParams.FECMode           = InitParams->FECMode;
    Demod_InitParams.FECMode           = InitParams->FECMode;
    Demod_InitParams.ClockPolarity           = InitParams->ClockPolarity;
    

    Error = (Inst[Handle].Terr.Demod.Driver->demod_Init)(&Inst[Handle].Name, &Demod_InitParams);
    Inst[Handle].Terr.Demod.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
        STTBX_Print(("%s fail init() demod Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {

#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
        STTBX_Print(("%s demod type %d at index #%d opened ok\n", identity, InitParams->DemodType, Demod_DriverIndex));
#endif
    }

    /* ---------- initalize tuner driver ---------- */
    Tuner_InitParams.IOHandle        = Tuner_IOHandle;
    Tuner_InitParams.MemoryPartition = InitParams->DriverPartition;
    Tuner_InitParams.TunerType       = InitParams->TunerType;

    Error = (Inst[Handle].Terr.Tuner.Driver->tuner_Init)(&Inst[Handle].Name, &Tuner_InitParams);
    Inst[Handle].Terr.Tuner.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
        STTBX_Print(("%s fail init() tuner Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {

#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
        STTBX_Print(("%s tuner type %d at index #%d opened ok\n", identity, InitParams->TunerType, Tuner_DriverIndex));
#endif
    }
#endif  
/*Normal (!Minidriver) ends */
#ifdef STTUNER_MINIDRIVER
      
    /* ---------- access driver database and instance database ---------- */
    Drv  = STTUNER_GetDrvDb();
    Inst = STTUNER_GetDrvInst();

    /* clear out any old instance info */
    
    memset(&Inst[Handle].Terr, 0x00, sizeof(STTUNER_TERR_InstanceDbase_t));
  
		    
    /* ---------- configure demod I/O ---------- */
    
    Demod_IOParams.Address        = InitParams->DemodIO.Address;  /* IO address to use */
    strcpy(Demod_IOParams.DriverName, InitParams->DemodIO.DriverName); /* name, of device address (I2C etc.) */

    Error = STTUNER_IODEMOD_Open(&Demod_IOHandle, &Demod_IOParams);
   
    if (Error != ST_NO_ERROR)
    {
	#ifdef STTUNER_DEBUG_MODULE_TER_INIT
	        STTBX_Print(("%s fail open demod I/O Error=%d\n", identity, Error));
	#endif
        return(Error);
    }
   
     /* ---------- configure tuner I/O ---------- */
   #ifdef STTUNER_DRV_TER_STV0360
    Tuner_IOParams.Address        = InitParams->TunerIO.Address;  /* IO address to use */
    strcpy(Tuner_IOParams.DriverName, InitParams->TunerIO.DriverName); /* name, of device address (I2C etc.) */

    Error = STTUNER_IOTUNER_Open(&Tuner_IOHandle, &Tuner_IOParams);
   #endif 
    if (Error != ST_NO_ERROR)
    {
	#ifdef STTUNER_DEBUG_MODULE_TER_INIT
	        STTBX_Print(("%s fail open demod I/O Error=%d\n", identity, Error));
	#endif
        return(Error);
    }
   

    /* ---------- get misc values ---------- */
/*    Inst[Handle].Sat.ExternalClock = InitParams->ExternalClock; checknab*/
/* checknab */
    Demod_InitParams.MemoryPartition   = InitParams->DriverPartition;
    Demod_InitParams.ExternalClock     = InitParams->ExternalClock;
    Demod_InitParams.TSOutputMode      = InitParams->TSOutputMode;
    Demod_InitParams.SerialDataMode    = InitParams->SerialDataMode;
    Demod_InitParams.SerialClockSource = InitParams->SerialClockSource;
   /* Demod_InitParams.BlockSyncMode     = InitParams->BlockSyncMode;   */   /* add block sync bit control for bug GNBvd27452*/
   /* Demod_InitParams.DataFIFOMode      = InitParams->DataFIFOMode;  */     /* add block sync bit control for bug GNBvd27452*/
   /* Demod_InitParams.OutputFIFOConfig  = InitParams->OutputFIFOConfig; */  /* add block sync bit control for bug GNBvd27452*/
    Demod_InitParams.FECMode           = InitParams->FECMode;
   
    #ifdef STTUNER_DRV_TER_STV0360
    Error = demod_d0360_Init(&Inst[Handle].Name, &Demod_InitParams);/*Checknab temp*/
   /* #elif defined(STTUNER_DRV_TER_STV0361)  || defined(STTUNER_DRV_TER_STV0370)*/
    #endif
    Inst[Handle].Terr.Demod.Error = Error;
    if (Error != ST_NO_ERROR)
    {
	#ifdef STTUNER_DEBUG_MODULE_TER_INIT
	        STTBX_Print(("%s fail init() demod Error=%d\n", identity, Error));
	#endif
        return(Error);
    }
    else
    {

	#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
	        STTBX_Print(("%s demod type %d at index #%d opened ok\n", identity, InitParams->DemodType, Demod_DriverIndex));
	#endif
    }
     #ifdef STTUNER_DRV_TER_STV0360
	    /* ---------- initalize tuner driver ---------- */
	    Tuner_InitParams.MemoryPartition = InitParams->DriverPartition;
	    Tuner_InitParams.TunerType       = InitParams->TunerType;
	
	    Error = tuner_tuntdrv_Init(&Inst[Handle].Name, &Tuner_InitParams);
	    Inst[Handle].Terr.Tuner.Error = Error;
	    if (Error != ST_NO_ERROR)
	    {
		#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
		        STTBX_Print(("%s fail init() tuner Error=%d\n", identity, Error));
		#endif
	        return(Error);
	    }
	    else
	    {
	
		#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
		        STTBX_Print(("%s tuner type %d at index #%d opened ok\n", identity, InitParams->TunerType, Tuner_DriverIndex));
		#endif
	    }
    #endif
    

#endif


#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
        STTBX_Print(("%s initalization completed\n", identity));
#endif

    return(Error);

}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */

#ifndef STTUNER_MINIDRIVER

U32 TERR_FindIndex_SharedDemod(STTUNER_DemodType_t DemodType)
{
#ifdef STTUNER_USE_SHARED

#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
    const char *identity = "STTUNER init.c TERR_FindIndex_SharedDemod()";
#endif
    U32 Demod_DriverIndex;
    STTUNER_DriverDbase_t *Drv;

    Drv  = STTUNER_GetDrvDb();
    Demod_DriverIndex = 0;

    if (Drv->Share.DemodDbaseSize < 1)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
        STTBX_Print(("%s shared driver database empty\n", identity));
#endif
        return(BAD_INDEX);
    }

    while(Drv->Share.DemodDbase[Demod_DriverIndex].ID != DemodType)
    {
        Demod_DriverIndex++;
        if (Demod_DriverIndex >= Drv->Share.DemodDbaseSize)
        {

#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
        STTBX_Print(("%s info: shared demod not found\n", identity));
#endif
        return(BAD_INDEX);
        }
    }
#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
       STTBX_Print(("%s shared demod %d at dbase index %d found\n", identity, DemodType, Demod_DriverIndex));
#endif
    return(Demod_DriverIndex);

#else
    return(BAD_INDEX);
#endif

}




U32 TERR_FindIndex_SharedTuner(STTUNER_TunerType_t TunerType)
{
#ifdef STTUNER_USE_SHARED

#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
    const char *identity = "STTUNER init.c TERR_FindIndex_SharedTuner()";
#endif
    U32 Tuner_DriverIndex;
    STTUNER_DriverDbase_t *Drv;

    Drv  = STTUNER_GetDrvDb();
    Tuner_DriverIndex = 0;

    if (Drv->Share.TunerDbaseSize < 1)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
        STTBX_Print(("%s shared tuner driver database empty\n", identity));
#endif
        return(BAD_INDEX);
    }

    while(Drv->Share.TunerDbase[Tuner_DriverIndex].ID != TunerType)
    {
        Tuner_DriverIndex++;
        if (Tuner_DriverIndex >= Drv->Share.TunerDbaseSize)
        {

#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
        STTBX_Print(("%s info: shared tuner not found\n", identity));
#endif
        return(BAD_INDEX);
        }
    }
#ifdef STTUNER_DEBUG_MODULE_TERR_INIT
       STTBX_Print(("%s shared tuner %d at dbase index %d found\n", identity, TunerType, Tuner_DriverIndex));
#endif
    return(Tuner_DriverIndex);

#else
    return(BAD_INDEX);
#endif
}

#endif /*#ifndef STTUNER_MINIDRIVER */
/* End of init.c */
