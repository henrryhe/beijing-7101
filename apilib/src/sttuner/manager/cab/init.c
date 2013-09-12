/* ----------------------------------------------------------------------------
File Name: init.c

Description:

     This module handles the cable management functions for the STAPI level
     interface for STTUNER for Init.

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

#include "cabapi.h"     /* cable manager API (functions in this file exported via this header) */

/* driver API*/
#include "drivers.h"
#include "chip.h"

/* invalid index into an array */
#define BAD_INDEX (0xffffffff)

U32 CAB_FindIndex_SharedTuner(STTUNER_TunerType_t TunerType);
U32 CAB_FindIndex_SharedDemod(STTUNER_DemodType_t DemodType);

/* ----------------------------------------------------------------------------
Name: STTUNER_CABLE_Init()

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
ST_ErrorCode_t STTUNER_CABLE_Init(STTUNER_Handle_t Handle, STTUNER_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
    const char *identity = "STTUNER init.c STTUNER_CABLE_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    U32 Demod_DriverIndex, Tuner_DriverIndex;

    DEMOD_InitParams_t  Demod_InitParams;
    TUNER_InitParams_t  Tuner_InitParams;

    IOARCH_Handle_t              Demod_IOHandle, Tuner_IOHandle;
    STTUNER_IOARCH_OpenParams_t  Demod_IOParams, Tuner_IOParams;


    STTUNER_DriverDbase_t   *Drv;
    STTUNER_InstanceDbase_t *Inst;
    

    /* ---------- check cable specific bits of InitParams data ---------- */
    Error = STTUNER_CABLE_Utils_InitParamCheck(InitParams);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
        STTBX_Print(("%s fail InitParams Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    /* ---------- access driver database and instance database ---------- */
    Drv  = STTUNER_GetDrvDb();
    Inst = STTUNER_GetDrvInst();


    /* clear out any old instance info */
    memset(&Inst[Handle].Cable, 0x00, sizeof(STTUNER_CABLE_InstanceDbase_t));

    /* ---------- look for a shared demod driver of the required type ---------- */
    Demod_DriverIndex = CAB_FindIndex_SharedDemod(InitParams->DemodType);
    if (Demod_DriverIndex == BAD_INDEX)
    {
        /* ---------- look for a cable demod driver of the required type ---------- */
        Demod_DriverIndex = 0;
        while(Drv->Cable.DemodDbase[Demod_DriverIndex].ID != InitParams->DemodType)
        {
            Demod_DriverIndex++;
            if (Demod_DriverIndex >= Drv->Cable.DemodDbaseSize)
            {

#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
                STTBX_Print(("%s fail demod not found\n", identity));
#endif
                
                return(ST_ERROR_UNKNOWN_DEVICE);
            }
        }
        Inst[Handle].Cable.Demod.Driver = &Drv->Cable.DemodDbase[Demod_DriverIndex];
#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
        STTBX_Print(("%s CABLE demod %d dbase index %d\n", identity, InitParams->DemodType, Demod_DriverIndex));
#endif
    }
    else
    {
#ifdef STTUNER_USE_SHARED
        Inst[Handle].Cable.Demod.Driver = &Drv->Share.DemodDbase[Demod_DriverIndex];
#endif
#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
        STTBX_Print(("%s SHARED demod %d dbase index %d\n", identity, InitParams->DemodType, Demod_DriverIndex));
#endif
    }


    /* ---------- look for a shared tuner driver of the required type ---------- */
    Tuner_DriverIndex = CAB_FindIndex_SharedTuner(InitParams->TunerType);
    if (Tuner_DriverIndex == BAD_INDEX)
    {
        /* ---------- look for a tuner driver of the required type ---------- */
        Tuner_DriverIndex = 0;
        while(Drv->Cable.TunerDbase[Tuner_DriverIndex].ID != InitParams->TunerType)
        {
            Tuner_DriverIndex++;
            if (Tuner_DriverIndex >= Drv->Cable.TunerDbaseSize)
            {

#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
                STTBX_Print(("%s fail Tuner not found\n", identity));
#endif
                
                return(ST_ERROR_UNKNOWN_DEVICE);
            }
        }
        Inst[Handle].Cable.Tuner.Driver = &Drv->Cable.TunerDbase[Tuner_DriverIndex];
#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
        STTBX_Print(("%s CABLE Tuner %d dbase index %d\n", identity, InitParams->TunerType, Tuner_DriverIndex));
#endif
    }
    else
    {
#ifdef STTUNER_USE_SHARED
        Inst[Handle].Cable.Tuner.Driver = &Drv->Share.TunerDbase[Tuner_DriverIndex];
#endif
#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
        STTBX_Print(("%s SHARED Tuner %d dbase index %d\n", identity, InitParams->TunerType, Tuner_DriverIndex));
#endif
    }

    /* all I/O paths lead to a demod driver, don't allow anything else */
    if (InitParams->DemodIO.Route != STTUNER_IO_DIRECT)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
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

    Error = ChipOpen(&Demod_IOHandle, &Demod_IOParams);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
        STTBX_Print(("%s fail open demod I/O Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    /* save I/O handle (as driver is not responsible for open/close of this handle) */
    /*GNBvd59840*/
    Inst[Handle].Cable.Demod.IOHandle = Demod_IOHandle;


    /* ---------- configure tuner I/O ---------- */
    Tuner_IOParams.IORoute  = InitParams->TunerIO.Route;    /* direct, repeater or passthru */
    Tuner_IOParams.IODriver = InitParams->TunerIO.Driver;   /* null, I2C or debug driver    */
    Tuner_IOParams.Address  = InitParams->TunerIO.Address;  /* IO address to use */
    /* location of handle to instance of redirection fn(), DrvHandle is returned when driver is opened */
    Tuner_IOParams.hInstance      = &Inst[Handle].Cable.Demod.DrvHandle;
    /* use demod for repeater/passthrough */
    Tuner_IOParams.TargetFunction = Inst[Handle].Cable.Demod.Driver->demod_ioaccess;
    Tuner_IOParams.RepeaterFn = Inst[Handle].Cable.Demod.Driver->demod_repeateraccess;

    strcpy(Tuner_IOParams.DriverName, InitParams->TunerIO.DriverName); /* name, of device address (I2C etc.) */

    Error = ChipOpen(&Tuner_IOHandle, &Tuner_IOParams);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
        STTBX_Print(("%s fail open tuner I/O Error=%d\n", identity, Error));
#endif
        ChipClose(Demod_IOHandle);
        return(Error);
    }
    /* save I/O handle (as driver is not responsible for open/close of this handle) */
    Inst[Handle].Cable.Tuner.IOHandle = Tuner_IOHandle;

    /* ---------- get misc values ---------- */
    Inst[Handle].Cable.ExternalClock = InitParams->ExternalClock;

    /* ---------- initalize demod driver ---------- */
    Demod_InitParams.IOHandle          = Demod_IOHandle;
    Demod_InitParams.MemoryPartition   = InitParams->DriverPartition;
    Demod_InitParams.ExternalClock     = InitParams->ExternalClock;
    Demod_InitParams.TSOutputMode      = InitParams->TSOutputMode;
    Demod_InitParams.SerialDataMode    = InitParams->SerialDataMode;
    Demod_InitParams.SerialClockSource = InitParams->SerialClockSource;
/*    Demod_InitParams.FECMode           = InitParams->FECMode;*/
    Demod_InitParams.Sti5518           = InitParams->Sti5518;
    Demod_InitParams.ClockPolarity     = InitParams->ClockPolarity;
    Demod_InitParams.SyncStripping     = InitParams->SyncStripping;
    Demod_InitParams.BlockSyncMode     = InitParams->BlockSyncMode;

    Error = (Inst[Handle].Cable.Demod.Driver->demod_Init)(&Inst[Handle].Name, &Demod_InitParams);
    Inst[Handle].Cable.Demod.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
        STTBX_Print(("%s fail init() demod Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {

#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
        STTBX_Print(("%s demod type %d at index #%d opened ok\n", identity, InitParams->DemodType, Demod_DriverIndex));
#endif
    }
    /* ---------- initalize tuner driver ---------- */
    Tuner_InitParams.IOHandle        = Tuner_IOHandle;
    Tuner_InitParams.MemoryPartition = InitParams->DriverPartition;
    Tuner_InitParams.TunerType       = InitParams->TunerType;

    Error = (Inst[Handle].Cable.Tuner.Driver->tuner_Init)(&Inst[Handle].Name, &Tuner_InitParams);
    Inst[Handle].Cable.Tuner.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
        STTBX_Print(("%s fail init() tuner Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {

#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
        STTBX_Print(("%s tuner type %d at index #%d opened ok\n", identity, InitParams->TunerType, Tuner_DriverIndex));
#endif
    }
    
#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
        STTBX_Print(("%s initalization completed\n", identity));
#endif
    

    return(Error);

}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */


U32 CAB_FindIndex_SharedDemod(STTUNER_DemodType_t DemodType)
{
#ifdef STTUNER_USE_SHARED

#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
    const char *identity = "STTUNER init.c CAB_FindIndex_SharedDemod()";
#endif
    U32 Demod_DriverIndex;
    STTUNER_DriverDbase_t *Drv;

    Drv  = STTUNER_GetDrvDb();
    Demod_DriverIndex = 0;

    if (Drv->Share.DemodDbaseSize < 1)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
        STTBX_Print(("%s shared driver database empty\n", identity));
#endif
        return(BAD_INDEX);
    }

    while(Drv->Share.DemodDbase[Demod_DriverIndex].ID != DemodType)
    {
        Demod_DriverIndex++;
        if (Demod_DriverIndex >= Drv->Share.DemodDbaseSize)
        {

#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
        STTBX_Print(("%s info: shared demod not found\n", identity));
#endif
        return(BAD_INDEX);
        }
    }
#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
       STTBX_Print(("%s shared demod %d at dbase index %d found\n", identity, DemodType, Demod_DriverIndex));
#endif
    return(Demod_DriverIndex);

#else
    return(BAD_INDEX);
#endif

}




U32 CAB_FindIndex_SharedTuner(STTUNER_TunerType_t TunerType)
{
#ifdef STTUNER_USE_SHARED

#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
    const char *identity = "STTUNER init.c CAB_FindIndex_SharedTuner()";
#endif
    U32 Tuner_DriverIndex;
    STTUNER_DriverDbase_t *Drv;

    Drv  = STTUNER_GetDrvDb();
    Tuner_DriverIndex = 0;

    if (Drv->Share.TunerDbaseSize < 1)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
        STTBX_Print(("%s shared tuner driver database empty\n", identity));
#endif
        return(BAD_INDEX);
    }

    while(Drv->Share.TunerDbase[Tuner_DriverIndex].ID != TunerType)
    {
        Tuner_DriverIndex++;
        if (Tuner_DriverIndex >= Drv->Share.TunerDbaseSize)
        {

#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
        STTBX_Print(("%s info: shared tuner not found\n", identity));
#endif
        return(BAD_INDEX);
        }
    }
#ifdef STTUNER_DEBUG_MODULE_CAB_INIT
       STTBX_Print(("%s shared tuner %d at dbase index %d found\n", identity, TunerType, Tuner_DriverIndex));
#endif
    return(Tuner_DriverIndex);

#else
    return(BAD_INDEX);
#endif
}


/* End of init.c */
