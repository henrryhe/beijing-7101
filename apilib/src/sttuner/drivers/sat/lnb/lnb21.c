/* ----------------------------------------------------------------------------
File Name: lnb21.c

Description: 

    LNB21 driver (should setup I/O passthrough to a stv0299 demod driver)


Copyright (C) 1999-2001 STMicroelectronics

History:
 
   date: 04-November-2003
version: 
 author: SD,AS
comment: 

 date: 02-June-2006
version: 
 author: HS,SD
comment: 
    
Reference:

    ST API Definition "LNB Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */

/* C libs */
#include <string.h>

/* Standard includes */
#include "stlite.h"

/* STAPI */
#include "stcommon.h"   /* for ST_GetClocksPerSecond() */
#include "sttbx.h"
#include "stevt.h"
#include "sttuner.h"                    

/* local to stlnb */
#include "util.h"       /* generic utility functions for stlnb */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#ifndef STTUNER_MINIDRIVER
#include "ioarch.h"     /* I/O for this driver */
#include "ioreg.h"      /* I/O register mapping */
#include "sdrv.h"       /* utilities */	
#endif

#ifdef STTUNER_MINIDRIVER
#include "iodirect.h"
#endif

#include "lnb21.h"      /* header for this file */


#include "sioctl.h"     /* data structure typedefs for all the the sat ioctl functions */



/* macros ------------------------------------------------------------------ */

/* Delay calling task for a period of microseconds */

#define LNB21_Delay_uS(micro_sec) STOS_TaskDelayUs(micro_sec)                             


/* private variables ------------------------------------------------------- */

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */

/* ---------- per instance of driver ---------- */
#ifndef STTUNER_MINIDRIVER
static BOOL        Installed = FALSE;
typedef struct
{
    ST_DeviceName_t           *DeviceName;           /* unique name for opening under */
    STTUNER_Handle_t          TopLevelHandle;   /* access tuner, demod etc. using this */
    IOARCH_Handle_t           IOHandle;         /* instance access to I/O driver       */
    STTUNER_IOREG_DeviceMap_t DeviceMap;        /* register map & data table   */
    LNB_Config_t              Config;           /* LNB config for each instance        */
    ST_Partition_t            *MemoryPartition;     /* which partition this data block belongs to */
    void                      *InstanceChainPrev;   /* previous data block in chain or NULL if not */
    void                      *InstanceChainNext;   /* next data block in chain or NULL if last */
       U8                         RegVal;
} LNB21_InstanceData_t;
#endif
#ifdef STTUNER_MINIDRIVER
typedef struct
{
    LNB_Config_t              Config;           /* LNB config for each instance        */
    STTUNER_Handle_t          TopLevelHandle;
    ST_Partition_t            *MemoryPartition;     /* which partition this data block belongs to */
} LNB21_InstanceData_t;
#endif
#ifndef STTUNER_MINIDRIVER
/* instance chain, the default boot value is invalid, to catch errors */
static LNB21_InstanceData_t *InstanceChainTop = (LNB21_InstanceData_t *)0x7fffffff;
#endif

#ifdef STTUNER_MINIDRIVER
LNB21_InstanceData_t *LNB21Instance;
#endif

U8 DefLNB21Val = 0xc8;
/* functions --------------------------------------------------------------- */
#ifndef STTUNER_MINIDRIVER
/* API */
ST_ErrorCode_t lnb_lnb21_Init(ST_DeviceName_t *DeviceName,LNB_InitParams_t *InitParams);
ST_ErrorCode_t lnb_lnb21_Term(ST_DeviceName_t *DeviceName,LNB_TermParams_t *TermParams);
ST_ErrorCode_t lnb_lnb21_Open (ST_DeviceName_t *DeviceName, LNB_OpenParams_t *OpenParams, LNB_Capability_t *Capability, LNB_Handle_t *Handle);
ST_ErrorCode_t lnb_lnb21_Close(LNB_Handle_t  Handle, LNB_CloseParams_t *CloseParams);

ST_ErrorCode_t lnb_lnb21_GetConfig(LNB_Handle_t Handle, LNB_Config_t *Config);
ST_ErrorCode_t LNB21_GetPower(LNB_Handle_t Handle, STTUNER_IOREG_DeviceMap_t *DeviceMap, LNB_Status_t *Status);
ST_ErrorCode_t LNB21_GetStatus(LNB_Handle_t Handle, STTUNER_IOREG_DeviceMap_t *DeviceMap, LNB_Status_t *Status);
ST_ErrorCode_t lnb_lnb21_GetProtection(LNB_Handle_t *Handle, LNB_Config_t *Config);
ST_ErrorCode_t lnb_lnb21_GetCurrent(LNB_Handle_t *Handle, LNB_Config_t *Config);
ST_ErrorCode_t lnb_lnb21_GetLossCompensation(LNB_Handle_t *Handle, LNB_Config_t *Config);


/* I/O API */
ST_ErrorCode_t lnb_lnb21_ioaccess(LNB_Handle_t Handle, IOARCH_Handle_t IOHandle,
    STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);

/* access device specific low-level functions */
ST_ErrorCode_t lnb_lnb21_ioctl(LNB_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status);
ST_ErrorCode_t lnb_lnb21_overloadcheck(LNB_Handle_t Handle, BOOL  *IsOverTemp, BOOL *IsCurrentOvrLoad);
#if defined(STTUNER_DRV_SAT_LNB21) /*Added for for GNBvd40148-->Error
if sttuner compiled with LNB21 option but use another one: This will only come when more than 1 lnb is
in use & atleast one of them is LNB21*/
ST_ErrorCode_t lnb_lnb21_setttxmode(LNB_Handle_t Handle, STTUNER_LnbTTxMode_t Ttxmode);
#endif


/* local functions */
void           LNB21_SetLnb         (LNB21_InstanceData_t *Instance , int Lnb);
void           LNB21_SetPolarization(LNB21_InstanceData_t *Instance ,LNB_Polarization_t Polarization);
void           LNB21_SetCurrentThreshold(LNB21_InstanceData_t *Instance , LNB_CurrentThresholdSelection_t CurrentThresholdSelection);
void           LNB21_SetProtectionMode(LNB21_InstanceData_t *Instance , LNB_ShortCircuitProtectionMode_t ShortCircuitProtectionMode);
void           LNB21_SetLossCompensation(LNB21_InstanceData_t *Instance ,BOOL CoaxCableLossCompensation);
ST_ErrorCode_t LNB21_SetPower       (LNB21_InstanceData_t *Instance , LNB_Status_t  Status);

ST_ErrorCode_t lnb_lnb21_SetConfig(LNB_Handle_t Handle, LNB_Config_t *Config);

ST_ErrorCode_t           LNB21_UpdateLNB      (LNB21_InstanceData_t *Instance );

ST_ErrorCode_t LNB21_Reg_Install    (STTUNER_IOREG_DeviceMap_t *DeviceMap);
#endif

/* functions --------------------------------------------------------------- */
#ifdef STTUNER_MINIDRIVER
U8 LNB21_Default_Reg[] = {0x3C};
#endif

#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_LNB_LNB21_Install()

Description:
    install a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_LNB_LNB21_Install(STTUNER_lnb_dbase_t *Lnb)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
   const char *identity = "STTUNER lnb21.c STTUNER_DRV_LNB_LNB21_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s fail driver already installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s installing sat:lnb:LNB21...", identity));
#endif

    /* mark ID in database */
    Lnb->ID = STTUNER_LNB_LNB21;

    /* map API */
    Lnb->lnb_Init  = lnb_lnb21_Init;
    Lnb->lnb_Term  = lnb_lnb21_Term;
    Lnb->lnb_Open  = lnb_lnb21_Open;
    Lnb->lnb_Close = lnb_lnb21_Close;

    Lnb->lnb_GetConfig = lnb_lnb21_GetConfig; 
    Lnb->lnb_SetConfig = lnb_lnb21_SetConfig;

    Lnb->lnb_ioaccess = lnb_lnb21_ioaccess;
    Lnb->lnb_ioctl    = lnb_lnb21_ioctl;
    Lnb->lnb_overloadcheck = lnb_lnb21_overloadcheck;
    #if defined(STTUNER_DRV_SAT_LNB21) /*Added for for GNBvd40148-->Error
    if sttuner compiled with LNB21 option but use another one: This will only come when more than 1 lnb is
    in use & atleast one of them is LNB21*/
    Lnb->lnb_setttxmode       = lnb_lnb21_setttxmode;
    #endif

    InstanceChainTop = NULL;
 
    Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);
  

    Installed = TRUE;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("ok\n"));
#endif    
    return(Error);
}


   
/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_LNB_LNB21_UnInstall()

Description:
    install a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_LNB_LNB21_UnInstall(STTUNER_lnb_dbase_t *Lnb)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
   const char *identity = "STTUNER lnb21.c STTUNER_DRV_LNB_LNB21_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }
   
    if(Lnb->ID != STTUNER_LNB_LNB21)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s fail incorrect driver type\n", identity));
#endif
        return(STTUNER_ERROR_ID);
    }

    /* has all memory been freed, by Term() */
    if(InstanceChainTop != NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s fail at least one instance not terminated\n", identity));
#endif
        return(ST_ERROR_OPEN_HANDLE);
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s uninstalling sat:lnb:LNB21...", identity));
#endif

    /* mark ID in database */
    Lnb->ID = STTUNER_NO_DRIVER;

    /* unmap API */
    Lnb->lnb_Init  = NULL;
    Lnb->lnb_Term  = NULL;
    Lnb->lnb_Open  = NULL;
    Lnb->lnb_Close = NULL;

    Lnb->lnb_GetConfig = NULL; 
    Lnb->lnb_SetConfig = NULL;

    Lnb->lnb_ioaccess = NULL;
    Lnb->lnb_ioctl    = NULL;
    Lnb->lnb_overloadcheck = NULL;
    #if defined(STTUNER_DRV_SAT_LNB21)
    Lnb->lnb_setttxmode       = NULL;
    #endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("<"));
#endif

        STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);
    
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print((">"));
#endif

    InstanceChainTop = (LNB21_InstanceData_t *)0x7ffffffe;
    Installed        = FALSE;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("ok\n"));
#endif

    return(Error);
}

#endif

/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */


   
/* ----------------------------------------------------------------------------
Name: lnb_lnb21_Init()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnb21_Init(ST_DeviceName_t *DeviceName,LNB_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
   const char *identity = "STTUNER lnb21.c lnb_lnb21_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
#ifndef STTUNER_MINIDRIVER
    LNB21_InstanceData_t *InstanceNew, *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }
    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check partition ---------- */
    Error = STTUNER_Util_CheckPtrNull(InitParams->MemoryPartition);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( LNB21_InstanceData_t ));
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s fail memory allocation InstanceNew\n", identity));
#endif    
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_NO_MEMORY);           
    }

    /* slot into chain */
    if (InstanceChainTop == NULL)
    {
        InstanceNew->InstanceChainPrev = NULL; /* no previous instance */
        InstanceChainTop = InstanceNew;
    }
    else    /* tag onto last data block in chain */
    {
        Instance = InstanceChainTop;

        while(Instance->InstanceChainNext != NULL)
        {
            Instance = Instance->InstanceChainNext;   /* next block */
        }
        Instance->InstanceChainNext     = (void *)InstanceNew;
        InstanceNew->InstanceChainPrev  = (void *)Instance;
    }


    InstanceNew->DeviceName          = DeviceName;
    InstanceNew->TopLevelHandle      = STTUNER_MAX_HANDLES; /* mark as not used */
    InstanceNew->IOHandle            = InitParams->IOHandle;
    InstanceNew->MemoryPartition     = InitParams->MemoryPartition;
    InstanceNew->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
    InstanceNew->DeviceMap.Registers = DEF_LNB_LNB21_NBREG;
    InstanceNew->DeviceMap.Fields    = DEF_LNB_LNB21_NBFIELD;
    InstanceNew->DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    InstanceNew->DeviceMap.MemoryPartition = InitParams->MemoryPartition;
    InstanceNew->InstanceChainNext   = NULL; /* always last in the chain */
        InstanceNew->RegVal  = DefLNB21Val;

    /* allocate memory for regmap */
    Error = STTUNER_IOREG_Open(&InstanceNew->DeviceMap);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s fail allocate register database\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);           
    }

    /* install a register map */
    Error = LNB21_Reg_Install(&InstanceNew->DeviceMap);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s fail install a register database\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);           
    }
    
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes)\n", identity, InstanceNew->DeviceName, (U32)InstanceNew, sizeof( LNB21_InstanceData_t ) ));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    #endif
    
#ifdef STTUNER_MINIDRIVER
    
    LNB21Instance = memory_allocate_clear(InitParams->MemoryPartition, 1, sizeof( LNB21_InstanceData_t ));
    if (LNB21Instance == NULL)
    {
        return(ST_ERROR_NO_MEMORY);           
    }
    LNB21Instance->MemoryPartition     = InitParams->MemoryPartition;
#endif
    
    
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: lnb_lnb21_Term()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnb21_Term(ST_DeviceName_t *DeviceName,LNB_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
   const char *identity = "STTUNER lnb21.c lnb_lnb21_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
#ifndef STTUNER_MINIDRIVER
    
    LNB21_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check params ---------- */
    Error = STTUNER_Util_CheckPtrNull(TermParams);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }


    /* reap next matching DeviceName */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
    STTBX_Print(("%s looking for first free handle[\n", identity));
#endif

    Instance = InstanceChainTop;
    while(1)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        if ( strcmp((char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
            STTBX_Print(("]\n"));
#endif
            Error = STTUNER_IOREG_Close(&Instance->DeviceMap);
            if (Error != ST_NO_ERROR)
            {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
                STTBX_Print(("%s fail close register database\n", identity));
#endif
            }
            /* found so now xlink prev and next(if applicable) and deallocate memory */
            InstancePrev = Instance->InstanceChainPrev;
            InstanceNext = Instance->InstanceChainNext;

            /* if instance to delete is first in chain */
            if (Instance->InstanceChainPrev == NULL)
            {
                InstanceChainTop = InstanceNext;        /* which would be NULL if last block to be term'd */
                if (InstanceNext != NULL)
                {
                InstanceNext->InstanceChainPrev = NULL; /* now top of chain, no previous instance */
                }
            }
            else
            {   /* safe to set value for prev instaance (because there IS one) */
                InstancePrev->InstanceChainNext = InstanceNext;
            }

            /* if there is a next block in the chain */            
            if (InstanceNext != NULL)
            {
                InstanceNext->InstanceChainPrev = InstancePrev;
            }   

            STOS_MemoryDeallocate(Instance->MemoryPartition, Instance);
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
            STTBX_Print(("%s terminated ok\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL) 
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
                STTBX_Print(("\n%s fail no free handle before end of list\n", identity));
#endif
                SEM_UNLOCK(Lock_InitTermOpenClose);
                return(STTUNER_ERROR_INITSTATE);
        }
        else
        {
            Instance = Instance->InstanceChainNext;   /* next block */
        }
        
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    
#endif

#ifdef STTUNER_MINIDRIVER
   
            memory_deallocate(LNB21Instance->MemoryPartition, LNB21Instance);   
#endif
    return(Error);
}   

/* ----------------------------------------------------------------------------
Name: lnb_lnb21_Open()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnb21_Open (ST_DeviceName_t *DeviceName, LNB_OpenParams_t *OpenParams, LNB_Capability_t *Capability, LNB_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
   const char *identity = "STTUNER lnb21.c lnb_lnb21_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
#ifndef STTUNER_MINIDRIVER
    LNB21_InstanceData_t     *Instance;
    

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
#endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL) 
        {       
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
    STTBX_Print((" found ok\n"));
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
    STTBX_Print(("%s using block at 0x%08x\n", identity, (U32)Instance));
#endif

    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s fail driver instance already open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    /* now got pointer to free (and valid) data block */
    Instance->TopLevelHandle = OpenParams->TopLevelHandle;  /* mark as valid */
    *Handle = (U32)Instance;
    
    /* Set LNB capabilties */
    Capability->ShortCircuitDetect = TRUE;  /* lie (rewrite to get pwr status using stv0299 i/o pin etc.) */
    Capability->PowerAvailable     = TRUE;
    Capability->PolarizationSelect = STTUNER_PLR_ALL;
 
    /* Set latest configuration */
    Instance->Config.Status    = LNB_STATUS_OFF;
    Instance->Config.Polarization = STTUNER_LNB_OFF;
    
    Instance->Config.ToneState = STTUNER_LNB_TONE_OFF;
    Instance->Config.CurrentThresholdSelection = STTUNER_LNB_CURRENT_THRESHOLD_LOW;
    Instance->Config.ShortCircuitProtectionMode = STTUNER_LNB_PROTECTION_STATIC;
    Instance->Config.CableLossCompensation = FALSE;
    
    /* LNB tone off */
    LNB21_SetLnb(Instance,Instance->Config.ToneState);

    /* LNB power on */
    LNB21_SetPower(Instance, Instance->Config.Status);
    
    /* LNB polarization horizontal*/
    LNB21_SetPolarization(Instance,Instance->Config.Polarization);
    
    /* LNB low current threshold*/
    LNB21_SetCurrentThreshold(Instance,Instance->Config.CurrentThresholdSelection);
    
    /*LNB circuit protection mode*/
    LNB21_SetProtectionMode(Instance,Instance->Config.ShortCircuitProtectionMode);
    
    /* LNB cable loss compenation off */
    LNB21_SetLossCompensation(Instance,Instance->Config.CableLossCompensation);

    Error = LNB21_UpdateLNB(Instance);
    
    LNB21_Delay_uS(500);
    
    /*Checking whether shortcircuit has ocurred i.e. whether OLF bit is high */
    Error |= LNB21_GetStatus(*Handle, &Instance->DeviceMap, &Instance->Config.Status);

    /* Check to ensure there is no problem with the LNB circuit */

    if (Error == ST_NO_ERROR)   /* last i/o (above) operation was good */
    {
        if (Instance->Config.Status == LNB_STATUS_SHORT_CIRCUIT || Instance->Config.Status == LNB_STATUS_OVER_TEMPERATURE  || Instance->Config.Status == LNB_STATUS_SHORTCIRCUIT_OVERTEMPERATURE)
        {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
            STTBX_Print(("%s fail LNB_STATUS_SHORT_CIRCUIT or LNB_STATUS_OVER_TEMPERATURE\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_HWFAIL);
        }
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s fail Error=%u\n", identity, Error));   /* LNB21_GetPower failed */
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
    STTBX_Print(("%s opened ok\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    
#endif

#ifdef STTUNER_MINIDRIVER
    
    *Handle = (U32)LNB21Instance;
    
    /* Set LNB capabilties */
    Capability->ShortCircuitDetect = TRUE;  /* lie (rewrite to get pwr status using stv0299 i/o pin etc.) */
    Capability->PowerAvailable     = TRUE;
    Capability->PolarizationSelect = STTUNER_PLR_ALL;
 
    /* Set latest configuration */
    LNB21Instance->Config.Status    = LNB_STATUS_ON;
    LNB21Instance->Config.ToneState = STTUNER_LNB_TONE_OFF;
    LNB21Instance->Config.CurrentThresholdSelection = STTUNER_LNB_CURRENT_THRESHOLD_LOW;
    LNB21Instance->Config.ShortCircuitProtectionMode = STTUNER_LNB_PROTECTION_STATIC;
    LNB21Instance->Config.CableLossCompensation = FALSE;
    LNB21Instance->Config.Polarization = STTUNER_PLR_HORIZONTAL;
    
    /* LNB tone off */
    LNB21_SetLnb(LNB21Instance->Config.ToneState);

    /* LNB power on */
    LNB21_SetPower(LNB21Instance->Config.Status);
    
    /* LNB polarization horizontal*/
    LNB21_SetPolarization(LNB21Instance->Config.Polarization);
    
    /* LNB low current threshold*/
    LNB21_SetCurrentThreshold(LNB21Instance->Config.CurrentThresholdSelection);
    
    /*LNB circuit protection mode*/
    LNB21_SetProtectionMode(LNB21Instance->Config.ShortCircuitProtectionMode);
    
    /* LNB cable loss compenation off */
    LNB21_SetLossCompensation(LNB21Instance->Config.CableLossCompensation);

    Error = LNB21_UpdateLNB();
    
    LNB21_Delay_uS(500);
    
    /*Checking whether shortcircuit has ocurred i.e. whether OLF bit is high */
    Error |= LNB21_GetStatus(&LNB21Instance->Config.Status);

    /* Check to ensure there is no problem with the LNB circuit */

    if (Error == ST_NO_ERROR)   /* last i/o (above) operation was good */
    {
        if (LNB21Instance->Config.Status == LNB_STATUS_SHORT_CIRCUIT || LNB21Instance->Config.Status == LNB_STATUS_OVER_TEMPERATURE  || LNB21Instance->Config.Status == LNB_STATUS_SHORTCIRCUIT_OVERTEMPERATURE)
        {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
            STTBX_Print(("%s fail LNB_STATUS_SHORT_CIRCUIT or LNB_STATUS_OVER_TEMPERATURE\n", identity));
#endif
            return(STTUNER_ERROR_HWFAIL);
        }
    }
    else
    {

    return(Error);
    }
  
#endif
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: lnb_lnb21_Close()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnb21_Close(LNB_Handle_t Handle, LNB_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
   const char *identity = "STTUNER lnb21.c lnb_lnb21_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    #ifndef STTUNER_MINIDRIVER
    LNB21_InstanceData_t     *Instance;

    Instance = (LNB21_InstanceData_t *)Handle;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
    STTBX_Print(("%s closed\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    
    #endif
    
#ifdef STTUNER_MINIDRIVER
LNB21Instance->TopLevelHandle = STTUNER_MAX_HANDLES;
#endif
    
    return(Error);
}   

#ifndef STTUNER_MINIDRIVER


#if defined(STTUNER_DRV_SAT_LNB21)
/* ----------------------------------------------------------------------------
Name: lnb_lnb21_setttxmode()

Description:
    Dummy Function
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t lnb_lnb21_setttxmode(LNB_Handle_t Handle, STTUNER_LnbTTxMode_t Ttxmode)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    return(Error);
}
#endif


/* ----------------------------------------------------------------------------
Name: lnb_lnb21_GetConfig()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnb21_GetConfig(LNB_Handle_t Handle, LNB_Config_t *Config)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
   const char *identity = "STTUNER lnb21.c lnb_lnb21_GetConfig()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    LNB21_InstanceData_t     *Instance;

    Instance = (LNB21_InstanceData_t *)Handle;

    Config->Status       = Instance->Config.Status;
    Config->Polarization = Instance->Config.Polarization;
    Config->ToneState    = Instance->Config.ToneState;
    Config->CurrentThresholdSelection    = Instance->Config.CurrentThresholdSelection;
    Config->ShortCircuitProtectionMode    = Instance->Config.ShortCircuitProtectionMode;
    Config->CableLossCompensation    = Instance->Config.CableLossCompensation;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
    STTBX_Print(("%s called\n", identity));
#endif
    return(Error);
}   
#endif



#ifdef STTUNER_MINIDRIVER

/* ----------------------------------------------------------------------------
Name: lnb_lnb21_GetConfig()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnb21_GetConfig(LNB_Config_t *Config)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    Config->Status       = LNB21Instance->Config.Status;
    Config->Polarization = LNB21Instance->Config.Polarization;
    Config->ToneState    = LNB21Instance->Config.ToneState;
    Config->CurrentThresholdSelection    = LNB21Instance->Config.CurrentThresholdSelection;
    Config->ShortCircuitProtectionMode    = LNB21Instance->Config.ShortCircuitProtectionMode;
    Config->CableLossCompensation    = LNB21Instance->Config.CableLossCompensation;


    return(Error);
}   
#endif
#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: lnb_lnb21_SetConfig()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnb21_SetConfig(LNB_Handle_t Handle, LNB_Config_t *Config)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
   const char *identity = "STTUNER lnb21.c lnb_lnb21_SetConfig()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    LNB21_InstanceData_t     *Instance;

    Instance = (LNB21_InstanceData_t *)Handle;
    Instance->TopLevelHandle = Config->TopLevelHandle;
    Instance->DeviceMap.Mode = IOREG_MODE_NOSUBADR;

    /* Select LNB band */
    if (Instance->Config.ToneState != Config->ToneState)
    {
        switch (Config->ToneState)
        {
            case STTUNER_LNB_TONE_OFF:      /* No tone */
                LNB21_SetLnb(Instance, Config->ToneState);
                break;

            case STTUNER_LNB_TONE_22KHZ:    /* 22KHz tone */
                LNB21_SetLnb(Instance, Config->ToneState);
                break;

            case STTUNER_LNB_TONE_DEFAULT:  /* no change */
                break;

            default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
                STTBX_Print(("%s fail tone state not default, off or 22KHz\n", identity));
#endif
                return(ST_ERROR_BAD_PARAMETER);
        }
        Instance->Config.ToneState = Config->ToneState;
    }


    /* Select polarization */
    if (Instance->Config.Polarization != Config->Polarization)
    {
        switch (Config->Polarization)
        {
            case STTUNER_PLR_VERTICAL:      /* OP_2 controls -- 18V (high) */
                LNB21_SetPolarization(Instance, VERTICAL);
                break;

            case STTUNER_PLR_HORIZONTAL:    /* OP_2 controls -- 13V (low) */
                LNB21_SetPolarization(Instance, HORIZONTAL);
                break;

            case STTUNER_LNB_OFF:    /* Turn LNB off */
                LNB21_SetPolarization(Instance, NOPOLARIZATION);
                break;

            default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
                STTBX_Print(("%s fail polarity not V or H\n", identity));
#endif
                return(ST_ERROR_BAD_PARAMETER);
        }
       Instance->Config.Polarization = Config->Polarization;
    }
    
    /* Select CurrentSelection*/
    if (Instance->Config.CurrentThresholdSelection != Config->CurrentThresholdSelection)
    {
        switch (Config->CurrentThresholdSelection)
        {
            /* The below two cases are for future use*/
            /*case STTUNER_LNB_CURRENT_THRESHOLD_HIGH:*/ /*ISEL=0*/
                /*LNB21_SetCurrentThreshold(&Instance->DeviceMap, 0);*/
                /*break;*/

            /*case STTUNER_LNB_CURRENT_THRESHOLD_LOW:*/ /*ISEL=1*/
                /*LNB21_SetCurrentThreshold(&Instance->DeviceMap, 1);*/
                /*break;*/

            default:/* Currently above both cases will not be entered, for future use*/
                LNB21_SetCurrentThreshold(Instance, 1);/* always set ISEL=1 for default setting of LNB21*/
            	break;
         }
         /* Below statement is for future use */
       /*Instance->Config.CurrentThresholdSelection = Config->CurrentThresholdSelection;*/
    }
    
    /* Select CableLossCompensation*/
    if (Instance->Config.CableLossCompensation != Config->CableLossCompensation)
    {
        /* Below statement is for future use */
        /*LNB21_SetLossCompensation(&Instance->DeviceMap, Config->CableLossCompensation);*/
        /*Instance->Config.CableLossCompensation = Config->CableLossCompensation;*/
        
        /* Set cable loss compensation off as default*/
        LNB21_SetLossCompensation(Instance, FALSE);
    }
    
    
    /* Select ShortCircuitProtection*/
    if (Instance->Config.ShortCircuitProtectionMode != Config->ShortCircuitProtectionMode)
    {
        switch (Config->ShortCircuitProtectionMode)
        {
        	/* The below two cases are for future use*/
            /*case STTUNER_LNB_PROTECTION_DYNAMIC:*/ /*PCL=0*/
                /*LNB21_SetProtectionMode(&Instance->DeviceMap, Config->ShortCircuitProtectionMode);*/
                /*break;*/

            /*case STTUNER_LNB_PROTECTION_STATIC:*/ /*PCL=1*/
                /*LNB21_SetProtectionMode(&Instance->DeviceMap, Config->ShortCircuitProtectionMode);*/
                /*break;*/

            default:
                LNB21_SetProtectionMode(Instance, 1); /* set lnb protection mode static as default*/
            	break;
         }
        
        /* Below statement is for future use */
        /*Instance->Config.ShortCircuitProtectionMode = Config->ShortCircuitProtectionMode;*/
    }


    /* Select power status */
        switch (Config->Status)
        {
            case LNB_STATUS_ON:
            case LNB_STATUS_OFF:
                LNB21_SetPower(Instance, Config->Status);
                break;

            default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
                STTBX_Print(("%s fail status not ON or OFF\n", identity));
#endif
                return(ST_ERROR_BAD_PARAMETER);
        }
       Instance->Config.Status = Config->Status;
    
    /* now write in LNBP21 to update the status*/
    Error = LNB21_UpdateLNB(Instance);

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
    STTBX_Print(("%s ok\n", identity));
#endif
    return(Error);
}   
#endif

#ifdef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: lnb_lnb21_SetConfig()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnb21_SetConfig(LNB_Config_t *Config)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
   const char *identity = "STTUNER lnb21.c lnb_lnb21_SetConfig()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    /* Select LNB band */
    if (LNB21Instance->Config.ToneState != Config->ToneState)
    {
        switch (Config->ToneState)
        {
            case STTUNER_LNB_TONE_OFF:      /* No tone */
                LNB21_SetLnb(Config->ToneState);
                break;

            case STTUNER_LNB_TONE_22KHZ:    /* 22KHz tone */
                LNB21_SetLnb(Config->ToneState);
                break;

            case STTUNER_LNB_TONE_DEFAULT:  /* no change */
                break;

            default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
                STTBX_Print(("%s fail tone state not default, off or 22KHz\n", identity));
#endif
                return(ST_ERROR_BAD_PARAMETER);
        }
        LNB21Instance->Config.ToneState = Config->ToneState;
    }


    /* Select polarization */
    if (LNB21Instance->Config.Polarization != Config->Polarization)
    {
        switch (Config->Polarization)
        {
            case STTUNER_PLR_VERTICAL:      /* OP_2 controls -- 18V (high) */
                LNB21_SetPolarization(VERTICAL);
                break;

            case STTUNER_PLR_HORIZONTAL:    /* OP_2 controls -- 13V (low) */
                LNB21_SetPolarization(HORIZONTAL);
                break;

            default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
                STTBX_Print(("%s fail polarity not V or H\n", identity));
#endif
                return(ST_ERROR_BAD_PARAMETER);
        }
       LNB21Instance->Config.Polarization = Config->Polarization;
    }
    
    /* Select CurrentSelection*/
    if (LNB21Instance->Config.CurrentThresholdSelection != Config->CurrentThresholdSelection)
    {
        switch (Config->CurrentThresholdSelection)
        {
            /* The below two cases are for future use*/
            /*case STTUNER_LNB_CURRENT_THRESHOLD_HIGH:*/ /*ISEL=0*/
                /*LNB21_SetCurrentThreshold(0);*/
                /*break;*/

            /*case STTUNER_LNB_CURRENT_THRESHOLD_LOW:*/ /*ISEL=1*/
                /*LNB21_SetCurrentThreshold(1);*/
                /*break;*/

            default:/* Currently above both cases will not be entered, for future use*/
                LNB21_SetCurrentThreshold(1);/* always set ISEL=1 for default setting of LNB21*/
            	break;
         }
         /* Below statement is for future use */
       /*Instance->Config.CurrentThresholdSelection = Config->CurrentThresholdSelection;*/
    }
    
    /* Select CableLossCompensation*/
    if (LNB21Instance->Config.CableLossCompensation != Config->CableLossCompensation)
    {
        /* Below statement is for future use */
        /*LNB21_SetLossCompensation(Config->CableLossCompensation);*/
        /*LNB21Instance->Config.CableLossCompensation = Config->CableLossCompensation;*/
        
        /* Set cable loss compensation off as default*/
        LNB21_SetLossCompensation(FALSE);
    }
    
    
    /* Select ShortCircuitProtection*/
    if (LNB21Instance->Config.ShortCircuitProtectionMode != Config->ShortCircuitProtectionMode)
    {
        switch (Config->ShortCircuitProtectionMode)
        {
        	/* The below two cases are for future use*/
            /*case STTUNER_LNB_PROTECTION_DYNAMIC:*/ /*PCL=0*/
                /*LNB21_SetProtectionMode(Config->ShortCircuitProtectionMode);*/
                /*break;*/

            /*case STTUNER_LNB_PROTECTION_STATIC:*/ /*PCL=1*/
                /*LNB21_SetProtectionMode(Config->ShortCircuitProtectionMode);*/
                /*break;*/

            default:
                LNB21_SetProtectionMode(1); /* set lnb protection mode static as default*/
            	break;
         }
        
        /* Below statement is for future use */
        /*LNB21Instance->Config.ShortCircuitProtectionMode = Config->ShortCircuitProtectionMode;*/
    }


    /* Select power status */
        switch (Config->Status)
        {
            case LNB_STATUS_ON:
            case LNB_STATUS_OFF:
                LNB21_SetPower(Config->Status);
                break;

            default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
                STTBX_Print(("%s fail status not ON or OFF\n", identity));
#endif
                return(ST_ERROR_BAD_PARAMETER);
        }
       LNB21Instance->Config.Status = Config->Status;
    
    /* now write in LNBP21 to update the status*/
    Error = LNB21_UpdateLNB();

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
    STTBX_Print(("%s ok\n", identity));
#endif
    return(Error);
}   
#endif

#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: lnb_lnb21_ioctl()

Description:
    access device specific low-level functions
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnb21_ioctl(LNB_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
   
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
   const char *identity = "STTUNER lnb21.c lnb_lnb21_ioctl()";
#endif
    ST_ErrorCode_t Error;
    LNB21_InstanceData_t     *Instance;

    Instance = (LNB21_InstanceData_t *)Handle;

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    switch(Function){

        case STTUNER_IOCTL_RAWIO: /* read/write device register (actual write) */
            Error =  STTUNER_IOARCH_ReadWrite( Instance->IOHandle, 
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->Operation,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->SubAddr,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->Data,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->TransferSize,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->Timeout );
            break;

        case STTUNER_IOCTL_REG_IN: /* read device register */
            *(int *)OutParams = STTUNER_IOREG_GetRegister(&Instance->DeviceMap, Instance->IOHandle, *(int *)InParams);
            break;

        case STTUNER_IOCTL_REG_OUT: /* write device register */
            Error =  STTUNER_IOREG_SetRegister(  &Instance->DeviceMap,Instance->IOHandle,
                  (U32)((SATIOCTL_SETREG_InParams_t *)InParams)->RegIndex,
                  (U32)((SATIOCTL_SETREG_InParams_t *)InParams)->Value );
            break;
		case STTUNER_IOCTL_SETLNB: /* set lnb parameters through ioctl */
			Error = ST_ERROR_FEATURE_NOT_SUPPORTED  ;
			break;

		case STTUNER_IOCTL_GETLNB: /* get lnb parameters through ioctl */
			Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
			break;

        default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
    STTBX_Print(("%s function %d called\n", identity, Function));
#endif
	
	Error |= Instance->DeviceMap.Error;
    
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);

}



/* ----------------------------------------------------------------------------
Name: lnb_lnb21_ioaccess()

Description:
    we get called with the instance of
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnb21_ioaccess(LNB_Handle_t Handle, IOARCH_Handle_t IOHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)
   
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
   const char *identity = "STTUNER lnb21.c lnb_lnb21_ioaccess()";
#endif
    ST_ErrorCode_t        Error = ST_NO_ERROR;
    IOARCH_Handle_t       ThisIOHandle;
    LNB21_InstanceData_t *Instance;

    Instance = (LNB21_InstanceData_t *)Handle;

    /* check handle */
    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    ThisIOHandle = Instance->IOHandle;

    /* if STTUNER_IOARCH_MAX_HANDLES then passthrough function required using our device handle (address) */ 
    if (IOHandle == STTUNER_IOARCH_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s write passthru\n", identity));
#endif
        Error = STTUNER_IOARCH_ReadWrite(ThisIOHandle, Operation, SubAddr, Data, TransferSize, Timeout);
    }
    else    /* repeater */
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s write repeater\n", identity));
#endif
        
        Error = STTUNER_IOARCH_ReadWriteNoRep(IOHandle, Operation, 0, Data, TransferSize, Timeout);
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
    STTBX_Print(("%s called\n", identity));
#endif
    return(Error);
}

#endif

/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */

#ifndef STTUNER_MINIDRIVER
ST_ErrorCode_t LNB21_UpdateLNB(LNB21_InstanceData_t     *Instance)
{
	
	ST_ErrorCode_t        Error = ST_NO_ERROR;
    /* update LNB hardware */
    STTUNER_IOREG_SetContigousRegisters(&Instance->DeviceMap, Instance->IOHandle,RLNB21_REGS, (&(Instance->RegVal))  ,1);
	Error |= Instance->DeviceMap.Error;
    
    Instance->DeviceMap.Error = ST_NO_ERROR;
	return(Error);

}
#endif
#ifdef STTUNER_MINIDRIVER
ST_ErrorCode_t LNB21_UpdateLNB( )
{
	ST_ErrorCode_t        Error = ST_NO_ERROR;
    
    	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_LNB_WRITE, RLNB21_REGS, 0x00, 0x00, LNB21_Default_Reg, 1, FALSE);

	return(Error);

}

#endif


/*----------------------------------------------------
 FUNCTION      LNB21_SetLnb
 ACTION        set the Lnb
 PARAMS IN     Lnb -> true for LnbHigh, false for LnbLow
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
#ifndef STTUNER_MINIDRIVER
void LNB21_SetLnb(LNB21_InstanceData_t     *Instance, int Lnb)
{
	
	/* Low band -> no 22KHz tone */
	if (Lnb == STTUNER_LNB_TONE_OFF) 
	{
		STTUNER_IOREG_SetFieldVal(&Instance->DeviceMap, FLNB21_OP2_CONF, 0, &(Instance->RegVal)  );
	
	}
	/* High band -> 22KHz tone */
	if (Lnb == STTUNER_LNB_TONE_22KHZ) 
	{
	STTUNER_IOREG_SetFieldVal(&Instance->DeviceMap, FLNB21_OP2_CONF, 1, &(Instance->RegVal)  );
	}
}
#endif
#ifdef STTUNER_MINIDRIVER
void LNB21_SetLnb(int Lnb)
{
	/* Low band -> no 22KHz tone */
	if (Lnb == STTUNER_LNB_TONE_OFF) 
	{
		LNB21_Default_Reg[0x00] &= 0xDF;
	}
	else if(Lnb == STTUNER_LNB_TONE_22KHZ) 
	{
		LNB21_Default_Reg[0x00] |= 0x20;
	}
	
}
#endif


/*----------------------------------------------------
 FUNCTION      LNB21_SetPolarization
 ACTION        set the polarization
 PARAMS IN     Polarization -> Polarization
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
#ifndef STTUNER_MINIDRIVER
void LNB21_SetPolarization(LNB21_InstanceData_t     *Instance,LNB_Polarization_t Polarization)
{
    if(Polarization == VERTICAL)/* Set 13V for Vertical Pol*/
    {
         STTUNER_IOREG_SetFieldVal(&Instance->DeviceMap, FLNB21_EN, 1,&(Instance->RegVal) );
        STTUNER_IOREG_SetFieldVal(&Instance->DeviceMap, FLNB21_VSEL, 0,&(Instance->RegVal)  );
       
		    
    }
    if(Polarization == HORIZONTAL)/* Set 18v for Horizontal Pol*/
    {
        
        STTUNER_IOREG_SetFieldVal(&Instance->DeviceMap, FLNB21_EN, 1,&(Instance->RegVal) );
        STTUNER_IOREG_SetFieldVal(&Instance->DeviceMap, FLNB21_VSEL, 1,&(Instance->RegVal)  );
    
    }
    if(Polarization == NOPOLARIZATION)/* Turn off LNB*/
    {
       
        STTUNER_IOREG_SetFieldVal(&Instance->DeviceMap, FLNB21_EN, 0, &(Instance->RegVal)  );
      
    }
}
#endif
#ifdef STTUNER_MINIDRIVER
void LNB21_SetPolarization(LNB_Polarization_t Polarization)
{
   
        /* Low band -> no 22KHz tone */
	if (Polarization == VERTICAL) 
	{
		LNB21_Default_Reg[0x00] &= 0xF7;
	}
	else if(Polarization == HORIZONTAL) 
	{
		LNB21_Default_Reg[0x00] |= 0x08;
	}
}
#endif
/*----------------------------------------------------
 FUNCTION      LNB21_SetCurrentThreshold
 ACTION        set the Current for LNB
 PARAMS IN     CurrentSelection 
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
#ifndef STTUNER_MINIDRIVER
void LNB21_SetCurrentThreshold(LNB21_InstanceData_t     *Instance,LNB_CurrentThresholdSelection_t CurrentThresholdSelection)
{
	if(CurrentThresholdSelection == CURRENT_THRESHOLD_HIGH)/* Set ISEL=0 to set high(500-650mA)*/
        {
        	STTUNER_IOREG_SetFieldVal(&Instance->DeviceMap, FLNB21_ISEL, 0, &(Instance->RegVal)  );
        
        }
        if(CurrentThresholdSelection == CURRENT_THRESHOLD_LOW)/* Set ISEL=1 to set low(400-550mA)*/
        {
        	STTUNER_IOREG_SetFieldVal(&Instance->DeviceMap, FLNB21_ISEL, 1, &(Instance->RegVal)  );
       
        }
}
#endif
#ifdef STTUNER_MINIDRIVER
void LNB21_SetCurrentThreshold(LNB_CurrentThresholdSelection_t CurrentThresholdSelection)
{
	if (CurrentThresholdSelection == CURRENT_THRESHOLD_HIGH) 
	{
		LNB21_Default_Reg[0x00] &= 0xBF;
	}
	else if(CurrentThresholdSelection == CURRENT_THRESHOLD_LOW) 
	{
		LNB21_Default_Reg[0x00] |= 0x40;
	}
}

void LNB21_SetProtectionMode(LNB_ShortCircuitProtectionMode_t ShortCircuitProtectionMode)
{
	if (ShortCircuitProtectionMode == LNB_PROTECTION_DYNAMIC) 
	{
		LNB21_Default_Reg[0x00] &= 0x7F;
	}
	else if(ShortCircuitProtectionMode == LNB_PROTECTION_STATIC) 
	{
		LNB21_Default_Reg[0x00] |= 0x80;
	}
}
#endif

/*----------------------------------------------------
 FUNCTION      LNB21_SetProtectionMode
 ACTION        set the short circuit for LNB
 PARAMS IN     ShortCircuitProtection 
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/	
#ifndef STTUNER_MINIDRIVER
void LNB21_SetProtectionMode(LNB21_InstanceData_t     *Instance, LNB_ShortCircuitProtectionMode_t ShortCircuitProtectionMode)
{
	if(ShortCircuitProtectionMode == LNB_PROTECTION_DYNAMIC)/* dynamic short circuit protection*/
	{
	 STTUNER_IOREG_SetFieldVal(&Instance->DeviceMap, FLNB21_PCL, 0,&(Instance->RegVal) );	
	 
	}
	if(ShortCircuitProtectionMode == LNB_PROTECTION_STATIC)/* static short circuit protection*/
	{
	STTUNER_IOREG_SetFieldVal(&Instance->DeviceMap, FLNB21_PCL, 1,&(Instance->RegVal)  );
	
	}
}


/*----------------------------------------------------
 FUNCTION      LNB21_SetLossCompensation
 ACTION        compensation for coaxial cable loss 
 PARAMS IN     CoaxCableLossCompensation 
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/	
void LNB21_SetLossCompensation(LNB21_InstanceData_t     *Instance,BOOL CoaxCableLossCompensation)
{
	if(CoaxCableLossCompensation == TRUE)/* add +1 in 13/18V*/
	{
		
	STTUNER_IOREG_SetFieldVal(&Instance->DeviceMap, FLNB21_LLC, 1,&(Instance->RegVal)  );
	
	}
	if(CoaxCableLossCompensation == FALSE)/* normal 13/18V */
	{		
	STTUNER_IOREG_SetFieldVal(&Instance->DeviceMap, FLNB21_LLC, 0,&(Instance->RegVal)  );
	
	}
}
#endif
#ifdef STTUNER_MINIDRIVER
void LNB21_SetLossCompensation(BOOL CoaxCableLossCompensation)
{
	
	if (CoaxCableLossCompensation == TRUE) 
	{
		LNB21_Default_Reg[0x00] &= 0xEF;
	}
	else if(CoaxCableLossCompensation == FALSE) 
	{
		LNB21_Default_Reg[0x00] |= 0x10;
	}
}
#endif
#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION      LNB21_GetPower
 ACTION        get lnb power status
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
ST_ErrorCode_t LNB21_GetPower(LNB_Handle_t Handle, STTUNER_IOREG_DeviceMap_t *DeviceMap, LNB_Status_t *Status)
{
    U8 powerstatus;

    LNB21_InstanceData_t     *Instance;
	ST_ErrorCode_t Error = ST_NO_ERROR;

    	Instance = (LNB21_InstanceData_t *)Handle;

	if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
	 {
	   #ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
	        STTBX_Print(("%s invalid handle\n", identity));
	   #endif
	        return(ST_ERROR_INVALID_HANDLE);
	 }
    Instance->DeviceMap.Mode = IOREG_MODE_NOSUBADR;  /* LNB21 read require no subaddress*/
    
    powerstatus = STTUNER_IOREG_GetRegister(&Instance->DeviceMap, Instance->IOHandle, (U16)RLNB21_REGS);
    if((powerstatus & 0x04) == 0x04)
    *Status = LNB_STATUS_ON;
    else
    *Status = LNB_STATUS_OFF;

	Error |= Instance->DeviceMap.Error;
    
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}
#endif

#ifdef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION      LNB21_GetPower
 ACTION        get lnb power status
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
ST_ErrorCode_t LNB21_GetPower(LNB_Status_t *Status)
{
    U8 powerstatus;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    Error = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_LNB_READ, RLNB21_REGS, 0x00, 0x00, &powerstatus, 1, FALSE);
    if((powerstatus & 0x04) == 0x04)
    *Status = LNB_STATUS_ON;
    else
    *Status = LNB_STATUS_OFF;
    return(Error);
}
#endif

#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION     LNB21_SetPower
 ACTION        set lnb power
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
ST_ErrorCode_t LNB21_SetPower(LNB21_InstanceData_t     *Instance,  LNB_Status_t Status)
{
	ST_ErrorCode_t Error = ST_NO_ERROR;
	#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
	    const char *identity = "STTUNER lnb21.c LNB21_SetPower()";
	#endif

    switch (Status)
    {
        case LNB_STATUS_ON:
         STTUNER_IOREG_SetFieldVal(&Instance->DeviceMap, FLNB21_EN, 1, &(Instance->RegVal)  );
                      
        break;
        
        case LNB_STATUS_OFF:
         STTUNER_IOREG_SetFieldVal(&Instance->DeviceMap, FLNB21_EN, 0,&(Instance->RegVal)  );
     
                    break;

        default:
            break;
    }

	Error |= Instance->DeviceMap.Error;
    
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}
#endif

#ifdef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION     LNB21_SetPower
 ACTION        set lnb power
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
ST_ErrorCode_t LNB21_SetPower(LNB_Status_t Status)
{
	U8 powerstatus;
	ST_ErrorCode_t Error = ST_NO_ERROR;

    switch (Status)
    {
        case LNB_STATUS_ON:
        case LNB_STATUS_OFF:/* always remain LNB ON, to get uninterrupted LNB power supply*/
           
            Error = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_LNB_READ, RLNB21_REGS, 0x00, 0x00, &powerstatus, 1, FALSE);
            powerstatus |= 0x04;
            Error |= STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_LNB_WRITE, RLNB21_REGS, 0x00, 0x00, &powerstatus, 1, FALSE);
            break;

        default:
            break;
    }

    return(Error);
}
#endif

#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION     LNB21_GetStatus
 ACTION        Get LNb status -> ShortCircuit(OLF), Over Temperature(OTF)
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
ST_ErrorCode_t LNB21_GetStatus(LNB_Handle_t Handle, STTUNER_IOREG_DeviceMap_t *DeviceMap, LNB_Status_t *Status)
{
	U8 olf_otf; /* short circuit and over temp. reporting*/
	LNB21_InstanceData_t     *Instance;
	ST_ErrorCode_t Error = ST_NO_ERROR;

    	Instance = (LNB21_InstanceData_t *)Handle;

	if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
	 {
	   #ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
	        STTBX_Print(("%s invalid handle\n", identity));
	   #endif
	        return(ST_ERROR_INVALID_HANDLE);
	 }
	 
	 Instance->DeviceMap.Mode = IOREG_MODE_NOSUBADR; /* LNB21 read require no subaddress*/
	
	olf_otf = STTUNER_IOREG_GetRegister(&Instance->DeviceMap, Instance->IOHandle, (U16)RLNB21_REGS);
	
	if((olf_otf & 0x01)== 0x01)
	(*Status) = LNB_STATUS_SHORT_CIRCUIT;
	
	if((olf_otf & 0x02) == 0x02)
        (*Status) = LNB_STATUS_OVER_TEMPERATURE;
        
        if((olf_otf & 0x03) == 0x03)
        (*Status) = LNB_STATUS_SHORTCIRCUIT_OVERTEMPERATURE;

	Error |= Instance->DeviceMap.Error;
    
    Instance->DeviceMap.Error = ST_NO_ERROR;
  
  return(Error);      
}

#endif


#ifdef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION     LNB21_GetStatus
 ACTION        Get LNb status -> ShortCircuit(OLF), Over Temperature(OTF)
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
ST_ErrorCode_t LNB21_GetStatus(LNB_Status_t *Status)
{
	U8 olf_otf; /* short circuit and over temp. reporting*/
	
	ST_ErrorCode_t Error = ST_NO_ERROR;
	
	Error = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_LNB_READ, RLNB21_REGS, 0x00, 0x00, &olf_otf, 1, FALSE);
	
	if((olf_otf & 0x01)== 0x01)
	(*Status) = LNB_STATUS_SHORT_CIRCUIT;
	
	if((olf_otf & 0x02) == 0x02)
        (*Status) = LNB_STATUS_OVER_TEMPERATURE;
        
        if((olf_otf & 0x03) == 0x03)
        (*Status) = LNB_STATUS_SHORTCIRCUIT_OVERTEMPERATURE;

	  
  return(Error);      
}

#endif

#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION      LNB21_Reg_Install
 ACTION        
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
ST_ErrorCode_t LNB21_Reg_Install(STTUNER_IOREG_DeviceMap_t *DeviceMap)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
    const char *identity = "STTUNER lnb21.c LNB21_Reg_Install()";
#endif
   ST_ErrorCode_t Error = ST_NO_ERROR;


    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s fail error=%d\n", identity, Error));
#endif
    }
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s installed ok\n", identity));
#endif
    }

    return (Error);
}
#endif


#ifndef STTUNER_MINIDRIVER
ST_ErrorCode_t lnb_lnb21_overloadcheck(LNB_Handle_t Handle, BOOL  *IsOverTemp, BOOL *IsCurrentOvrLoad)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
   const char *identity = "STTUNER lnb21.c lnb_lnb21_overloadcheck()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    LNB21_InstanceData_t     *Instance;
    
    Instance = (LNB21_InstanceData_t *)Handle;
    Instance->DeviceMap.Mode = IOREG_MODE_NOSUBADR;/* LNB21 read require no subaddress*/
    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB21
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    } 
    
    /* Read OLF value */
    *IsCurrentOvrLoad = STTUNER_IOREG_GetField (&Instance->DeviceMap, Instance->IOHandle,  FLNB21_OLF);
   
     /* Read OTF value */ 
    *IsOverTemp = STTUNER_IOREG_GetField (&Instance->DeviceMap, Instance->IOHandle,  FLNB21_OTF);
    
    Error |= Instance->DeviceMap.Error;  
    Instance->DeviceMap.Error = ST_NO_ERROR;
    return(Error);
	
}
#endif

#ifdef STTUNER_MINIDRIVER
ST_ErrorCode_t lnb_lnb21_overloadcheck(BOOL  *IsOverTemp, BOOL *IsCurrentOvrLoad)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    /* Read OLF value */
    Error = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_LNB_READ, RLNB21_REGS, 0x00, 0x00, (U8 *)IsCurrentOvrLoad, 1, FALSE);
    *IsCurrentOvrLoad &= 0x01;
       
     /* Read OTF value */ 
     Error = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_LNB_READ, RLNB21_REGS, 0x00, 0x00, (U8 *)IsOverTemp, 1, FALSE);
    *IsOverTemp = (*IsOverTemp & 0x02)>>1;
    
    return(Error);
	
}
#endif

/* End of lnb21.c */
