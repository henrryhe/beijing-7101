/* ----------------------------------------------------------------------------
File Name: lnbh21.c

Description: 

    LNBH21 driver 


Copyright (C) 1999-2001 STMicroelectronics

History:
 
   date: 
version: 
 author: 
comment: 
    
Reference:

    ST API Definition "LNB Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
   #include <string.h>
#endif
#include "stlite.h"     /* Standard includes */
#include "sttbx.h"

/* STAPI */
#include "stcommon.h"   /* for ST_GetClocksPerSecond() */

#include "stevt.h"
#include "sttuner.h"                    

/* local to stlnb */
#include "util.h"       /* generic utility functions for stlnb */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#ifndef STTUNER_MINIDRIVER
#include "ioarch.h"     /* I/O for this driver */
#include "ioreg.h"      /* I/O register mapping */
#include "chip.h"
#include "sdrv.h"       /* utilities */
#endif

#ifdef STTUNER_MINIDRIVER
#include "iodirect.h"
#endif
#include "lnbh21.h"      /* header for this file */


#include "sioctl.h"     /* data structure typedefs for all the the sat ioctl functions */

extern IOARCH_HandleData_t IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];
/* macros ------------------------------------------------------------------ */

/* Delay calling task for a period of microseconds */
#define LNBH21_Delay_uS(micro_sec) STOS_TaskDelayUs(micro_sec) 
                              

#ifndef STTUNER_MINIDRIVER
/* private variables ------------------------------------------------------- */

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */

/* ---------- per instance of driver ---------- */
static BOOL        Installed = FALSE;
typedef struct
{
    ST_DeviceName_t           *DeviceName;           /* unique name for opening under */
    STTUNER_Handle_t          TopLevelHandle;   /* access tuner, demod etc. using this */
    IOARCH_Handle_t           IOHandle;         /* instance access to I/O driver       */
    LNB_Config_t              Config;           /* LNB config for each instance        */
    ST_Partition_t            *MemoryPartition;     /* which partition this data block belongs to */
    void                      *InstanceChainPrev;   /* previous data block in chain or NULL if not */
    void                      *InstanceChainNext;   /* next data block in chain or NULL if last */
    U8                         RegVal;
} LNBH21_InstanceData_t;
#endif




#ifdef STTUNER_MINIDRIVER
typedef struct
{
    LNB_Config_t              Config;           /* LNB config for each instance        */
    STTUNER_Handle_t          TopLevelHandle;
    ST_Partition_t            *MemoryPartition;     /* which partition this data block belongs to */
} LNB21_InstanceData_t;
#endif

U8 DefLNBH21Val = 0xc8;

#ifndef STTUNER_MINIDRIVER
/* instance chain, the default boot value is invalid, to catch errors */
static LNBH21_InstanceData_t *InstanceChainTop = (LNBH21_InstanceData_t *)0x7fffffff;
#endif

#ifdef STTUNER_MINIDRIVER
LNB21_InstanceData_t *LNBH21Instance;
#endif

/* functions --------------------------------------------------------------- */
#ifndef STTUNER_MINIDRIVER
/* API */
ST_ErrorCode_t lnb_lnbh21_Init(ST_DeviceName_t *DeviceName,LNB_InitParams_t *InitParams);
ST_ErrorCode_t lnb_lnbh21_Term(ST_DeviceName_t *DeviceName,LNB_TermParams_t *TermParams);
ST_ErrorCode_t lnb_lnbh21_Open (ST_DeviceName_t *DeviceName, LNB_OpenParams_t *OpenParams, LNB_Capability_t *Capability, LNB_Handle_t *Handle);
ST_ErrorCode_t lnb_lnbh21_Close(LNB_Handle_t  Handle, LNB_CloseParams_t *CloseParams);

ST_ErrorCode_t lnb_lnbh21_GetConfig(LNB_Handle_t Handle, LNB_Config_t *Config);
ST_ErrorCode_t LNBH21_GetPower(LNB_Handle_t Handle, LNB_Status_t *Status);
ST_ErrorCode_t LNBH21_GetStatus(LNB_Handle_t Handle,LNB_Status_t *Status);
ST_ErrorCode_t lnb_lnbh21_GetProtection(LNB_Handle_t *Handle, LNB_Config_t *Config);
ST_ErrorCode_t lnb_lnbh21_GetCurrent(LNB_Handle_t *Handle, LNB_Config_t *Config);
ST_ErrorCode_t lnb_lnbh21_GetLossCompensation(LNB_Handle_t *Handle, LNB_Config_t *Config);




/* access device specific low-level functions */
ST_ErrorCode_t lnb_lnbh21_ioctl(LNB_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status);

/* local functions */

void           LNBH21_SetLnb         (LNBH21_InstanceData_t     *Instance, int Lnb);
void           LNBH21_SetPolarization(LNBH21_InstanceData_t     *Instance, LNB_Polarization_t Polarization);
void           LNBH21_SetProtectionMode(LNBH21_InstanceData_t     *Instance, LNB_ShortCircuitProtectionMode_t ShortCircuitProtectionMode);
void           LNBH21_SetLossCompensation(LNBH21_InstanceData_t     *Instance, BOOL CoaxCableLossCompensation);
ST_ErrorCode_t LNBH21_SetPower       (LNBH21_InstanceData_t     *Instance, LNB_Status_t  Status);
ST_ErrorCode_t lnb_lnbh21_SetConfig(LNB_Handle_t Handle, LNB_Config_t *Config);

ST_ErrorCode_t           LNBH21_UpdateLNB      (LNBH21_InstanceData_t     *Instance);


ST_ErrorCode_t lnb_lnbh21_ioctl_set(LNB_Handle_t Handle,void *InParams);
ST_ErrorCode_t lnb_lnbh21_ioctl_get(LNB_Handle_t Handle,void *OutParams);
ST_ErrorCode_t lnb_lnbh21_overloadcheck(LNB_Handle_t Handle, BOOL  *IsOverTemp, BOOL *IsCurrentOvrLoad);
ST_ErrorCode_t lnb_lnbh21_setttxmode(LNB_Handle_t Handle, STTUNER_LnbTTxMode_t Ttxmode);
#endif

#ifdef STTUNER_MINIDRIVER
U8 LNBH21_Default_Reg[] = {0xCC};
#endif

#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_LNB_LNBH21_Install()

Description:
    install a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_LNB_LNBH21_Install(STTUNER_lnb_dbase_t *Lnb)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
   const char *identity = "STTUNER lnbh21.c STTUNER_DRV_LNB_LNBH21_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("%s fail driver already installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("%s installing sat:lnb:LNB21...", identity));
#endif

    /* mark ID in database */
    Lnb->ID = STTUNER_LNB_LNBH21;

    /* map API */
    Lnb->lnb_Init  = lnb_lnbh21_Init;
    Lnb->lnb_Term  = lnb_lnbh21_Term;
    Lnb->lnb_Open  = lnb_lnbh21_Open;
    Lnb->lnb_Close = lnb_lnbh21_Close;

    Lnb->lnb_GetConfig = lnb_lnbh21_GetConfig; 
    Lnb->lnb_SetConfig = lnb_lnbh21_SetConfig;

    Lnb->lnb_ioaccess = NULL;
    Lnb->lnb_ioctl    = lnb_lnbh21_ioctl;
    Lnb->lnb_overloadcheck = lnb_lnbh21_overloadcheck;
    Lnb->lnb_setttxmode = lnb_lnbh21_setttxmode;
    InstanceChainTop = NULL;

    Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);


    Installed = TRUE;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("ok\n"));
#endif    
    return(Error);
}


   
/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_LNB_LNBH21_UnInstall()

Description:
    install a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_LNB_LNBH21_UnInstall(STTUNER_lnb_dbase_t *Lnb)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
   const char *identity = "STTUNER lnbh21.c STTUNER_DRV_LNB_LNBH21_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }
   
    if(Lnb->ID != STTUNER_LNB_LNBH21)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("%s fail incorrect driver type\n", identity));
#endif
        return(STTUNER_ERROR_ID);
    }

    /* has all memory been freed, by Term() */
    if(InstanceChainTop != NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("%s fail at least one instance not terminated\n", identity));
#endif
        return(ST_ERROR_OPEN_HANDLE);
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("%s uninstalling sat:lnb:LNBH21...", identity));
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
    Lnb->lnb_setttxmode = NULL;
    
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("<"));
#endif

        STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);
    
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print((">"));
#endif

    InstanceChainTop = (LNBH21_InstanceData_t *)0x7ffffffe;
    Installed        = FALSE;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("ok\n"));
#endif

    return(Error);
}

#endif

/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */


   
/* ----------------------------------------------------------------------------
Name: lnb_lnbh21_Init()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnbh21_Init(ST_DeviceName_t *DeviceName,LNB_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
   const char *identity = "STTUNER lnbh21.c lnb_lnbh21_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
#ifndef STTUNER_MINIDRIVER
    LNBH21_InstanceData_t *InstanceNew, *Instance=NULL;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
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
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( LNBH21_InstanceData_t ));
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
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
    
    InstanceNew->InstanceChainNext   = NULL; /* always last in the chain */
    InstanceNew->RegVal  = DefLNBH21Val;
        
    /*********************************/
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.Registers = DEF_LNB_LNBH21_NBREG;
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.Fields    = DEF_LNB_LNBH21_NBFIELD;
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.MemoryPartition     = InitParams->MemoryPartition;
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.MemoryPartition,DEF_LNB_LNBH21_NBREG ,sizeof(U8));
    
    /**********************************/
    

    
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes)\n", identity, InstanceNew->DeviceName, (U32)InstanceNew, sizeof( LNBH21_InstanceData_t ) ));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    #endif
    
#ifdef STTUNER_MINIDRIVER
    
    LNBH21Instance = memory_allocate_clear(InitParams->MemoryPartition, 1, sizeof( LNB21_InstanceData_t ));
    if (LNBH21Instance == NULL)
    {
        return(ST_ERROR_NO_MEMORY);           
    }
    LNBH21Instance->MemoryPartition     = InitParams->MemoryPartition;
#endif
    
    
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: lnb_lnbh21_Term()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnbh21_Term(ST_DeviceName_t *DeviceName,LNB_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
   const char *identity = "STTUNER lnbh21.c lnb_lnbh21_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
#ifndef STTUNER_MINIDRIVER
    LNBH21_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
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
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }


    /* reap next matching DeviceName */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
    STTBX_Print(("%s looking for first free handle[\n", identity));
#endif

    Instance = InstanceChainTop;
    while(1)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        if ( strcmp((char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
            STTBX_Print(("]\n"));
#endif

            if (Error != ST_NO_ERROR)
            {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
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
            STOS_MemoryDeallocate(Instance->MemoryPartition, IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream);
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
            STTBX_Print(("%s terminated ok\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL) 
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
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


#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
#endif

#ifdef STTUNER_MINIDRIVER
   
            memory_deallocate(LNBH21Instance->MemoryPartition, LNBH21Instance);   
#endif
    return(Error);
}   

/* ----------------------------------------------------------------------------
Name: lnb_lnbh21_Open()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnbh21_Open (ST_DeviceName_t *DeviceName, LNB_OpenParams_t *OpenParams, LNB_Capability_t *Capability, LNB_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
   const char *identity = "STTUNER lnbh21.c lnb_lnbh21_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
#ifndef STTUNER_MINIDRIVER
    LNBH21_InstanceData_t     *Instance;
    

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
#endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL) 
        {       
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
    STTBX_Print((" found ok\n"));
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
    STTBX_Print(("%s using block at 0x%08x\n", identity, (U32)Instance));
#endif

    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
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
    Instance->Config.ShortCircuitProtectionMode = STTUNER_LNB_PROTECTION_STATIC;
    Instance->Config.CableLossCompensation = FALSE;
    

    /* set LNB tone  */
   LNBH21_SetLnb(Instance, Instance->Config.ToneState);

    /* LNB power on */
    LNBH21_SetPower(Instance, Instance->Config.Status);
    
    /* LNB polarization horizontal*/
    LNBH21_SetPolarization(Instance, Instance->Config.Polarization);

    /* LNB low current threshold*/


    /*LNB circuit protection mode*/
    LNBH21_SetProtectionMode(Instance, Instance->Config.ShortCircuitProtectionMode);
    
    /* LNB cable loss compenation off */
    LNBH21_SetLossCompensation(Instance, Instance->Config.CableLossCompensation);

    Error = LNBH21_UpdateLNB (Instance);

    LNBH21_Delay_uS(25000);

      /*Checking whether shortcircuit has ocurred i.e. whether OLF bit is high */
    Error |= LNBH21_GetStatus(*Handle,&Instance->Config.Status);

    /* Check to ensure there is no problem with the LNB circuit */

    if (Error == ST_NO_ERROR)   /* last i/o (above) operation was good */
    {
        if (Instance->Config.Status == LNB_STATUS_SHORT_CIRCUIT || Instance->Config.Status == LNB_STATUS_OVER_TEMPERATURE  || Instance->Config.Status == LNB_STATUS_SHORTCIRCUIT_OVERTEMPERATURE)
        {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
            STTBX_Print(("%s fail LNB_STATUS_SHORT_CIRCUIT or LNB_STATUS_OVER_TEMPERATURE\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_HWFAIL);
        }
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("%s fail Error=%u\n", identity, Error));   /* LNBH21_GetPower failed */
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
    STTBX_Print(("%s opened ok\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
#endif

#ifdef STTUNER_MINIDRIVER
    
    *Handle = (U32)LNBH21Instance;
    
    /* Set LNB capabilties */
    Capability->ShortCircuitDetect = TRUE;  /* lie (rewrite to get pwr status using stv0299 i/o pin etc.) */
    Capability->PowerAvailable     = TRUE;
    Capability->PolarizationSelect = STTUNER_PLR_ALL;
 
    /* Set latest configuration */
    LNBH21Instance->Config.Status    = LNB_STATUS_ON;
    LNBH21Instance->Config.ToneState = STTUNER_LNB_TONE_OFF;
    LNBH21Instance->Config.ShortCircuitProtectionMode = STTUNER_LNB_PROTECTION_STATIC;
    LNBH21Instance->Config.CableLossCompensation = FALSE;
    LNBH21Instance->Config.Polarization = STTUNER_PLR_HORIZONTAL;
    
    /* LNB tone off */
    LNBH21_SetLnb(LNBH21Instance->Config.ToneState);

    /* LNB power on */
    LNBH21_SetPower(LNBH21Instance->Config.Status);
    
    /* LNB polarization horizontal*/
    LNBH21_SetPolarization(LNBH21Instance->Config.Polarization);
    
    
    /*LNB circuit protection mode*/
    LNBH21_SetProtectionMode(LNBH21Instance->Config.ShortCircuitProtectionMode);
    
    /* LNB cable loss compenation off */
    LNBH21_SetLossCompensation(LNBH21Instance->Config.CableLossCompensation);

    Error = LNBH21_UpdateLNB();
    
    LNBH21_Delay_uS(500);
    
    /*Checking whether shortcircuit has ocurred i.e. whether OLF bit is high */
    Error |= LNBH21_GetStatus(&LNBH21Instance->Config.Status);

    /* Check to ensure there is no problem with the LNB circuit */

    if (Error == ST_NO_ERROR)   /* last i/o (above) operation was good */
    {
        if (LNBH21Instance->Config.Status == LNB_STATUS_SHORT_CIRCUIT || LNBH21Instance->Config.Status == LNB_STATUS_OVER_TEMPERATURE  || LNBH21Instance->Config.Status == LNB_STATUS_SHORTCIRCUIT_OVERTEMPERATURE)
        {

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
Name: lnb_lnbh21_Close()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnbh21_Close(LNB_Handle_t Handle, LNB_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
   const char *identity = "STTUNER lnbh21.c lnb_lnbh21_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    #ifndef STTUNER_MINIDRIVER
    LNBH21_InstanceData_t     *Instance;

    Instance = (LNBH21_InstanceData_t *)Handle;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
    STTBX_Print(("%s closed\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    #endif
    
#ifdef STTUNER_MINIDRIVER
LNBH21Instance->TopLevelHandle = STTUNER_MAX_HANDLES;
#endif
    
    return(Error);
}   

#ifndef STTUNER_MINIDRIVER

/* ----------------------------------------------------------------------------
Name: lnb_lnbh21_GetConfig()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnbh21_GetConfig(LNB_Handle_t Handle, LNB_Config_t *Config)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
   const char *identity = "STTUNER lnbh21.c lnb_lnbh21_GetConfig()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    LNBH21_InstanceData_t     *Instance;

    Instance = (LNBH21_InstanceData_t *)Handle;

    Config->Status       = Instance->Config.Status;
    Config->Polarization = Instance->Config.Polarization;
    Config->ToneState    = Instance->Config.ToneState;
    Config->ShortCircuitProtectionMode    = Instance->Config.ShortCircuitProtectionMode;
    Config->CableLossCompensation    = Instance->Config.CableLossCompensation;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
    STTBX_Print(("%s called\n", identity));
#endif
    return(Error);
}   
#endif



#ifdef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: lnb_lnbh21_GetConfig()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnbh21_GetConfig(LNB_Config_t *Config)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    Config->Status       = LNBH21Instance->Config.Status;
    Config->Polarization = LNBH21Instance->Config.Polarization;
    Config->ToneState    = LNBH21Instance->Config.ToneState;
    
    Config->ShortCircuitProtectionMode    = LNBH21Instance->Config.ShortCircuitProtectionMode;
    Config->CableLossCompensation    = LNBH21Instance->Config.CableLossCompensation;


    return(Error);
}   
#endif

 #ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: lnb_lnbh21_SetConfig()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnbh21_SetConfig(LNB_Handle_t Handle, LNB_Config_t *Config)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
   const char *identity = "STTUNER lnbh21.c lnb_lnbh21_SetConfig()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    LNBH21_InstanceData_t     *Instance;

    Instance = (LNBH21_InstanceData_t *)Handle;
    Instance->TopLevelHandle = Config->TopLevelHandle;


    /* Select LNB band */
    if (Instance->Config.ToneState != Config->ToneState)	
    {
        switch (Config->ToneState)
        {
            case STTUNER_LNB_TONE_OFF:      /* No tone */          
            case STTUNER_LNB_TONE_22KHZ:    /* 22KHz tone */
                 LNBH21_SetLnb(Instance, Config->ToneState);
                break;

            case STTUNER_LNB_TONE_DEFAULT:  /* no change */
                break;

            default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
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
                LNBH21_SetPolarization(Instance, VERTICAL);
                break;

            case STTUNER_PLR_HORIZONTAL:    /* OP_2 controls -- 13V (low) */
                LNBH21_SetPolarization(Instance, HORIZONTAL);
                break;
            case STTUNER_LNB_OFF:    /* OP_2 controls -- 13V (low) */
                LNBH21_SetPolarization(Instance, NOPOLARIZATION);
                break;

            default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
                STTBX_Print(("%s fail polarity not V or H\n", identity));
#endif
                return(ST_ERROR_BAD_PARAMETER);
        }
       Instance->Config.Polarization = Config->Polarization;
    } 
 
    /* Select CableLossCompensation*/
    if (Instance->Config.CableLossCompensation != Config->CableLossCompensation)
    {
        /* Below statement is for future use */
        /*LNBH21_SetLossCompensation(Config->CableLossCompensation);*/
        /*Instance->Config.CableLossCompensation = Config->CableLossCompensation;*/
        
        /* Set cable loss compensation off as default*/
        LNBH21_SetLossCompensation(Instance, FALSE);
    }
    
    
    /* Select ShortCircuitProtection*/
    if (Instance->Config.ShortCircuitProtectionMode != Config->ShortCircuitProtectionMode)
    {
        switch (Config->ShortCircuitProtectionMode)
        {
        	/* The below two cases are for future use*/
            case STTUNER_LNB_PROTECTION_DYNAMIC: /*PCL=0*/
                LNBH21_SetProtectionMode(Instance, Config->ShortCircuitProtectionMode);
                break;

            case STTUNER_LNB_PROTECTION_STATIC: /*PCL=1*/
                LNBH21_SetProtectionMode(Instance, Config->ShortCircuitProtectionMode);
                break;

            default: /* None */
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
                LNBH21_SetPower(Instance, Config->Status);
                break;

            default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
                STTBX_Print(("%s fail status not ON or OFF\n", identity));
#endif
                return(ST_ERROR_BAD_PARAMETER);
        }
       Instance->Config.Status = Config->Status;
    
    /* now write in LNBH21 to update the status*/
    Error = LNBH21_UpdateLNB( Instance);

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
    STTBX_Print(("%s ok\n", identity));
#endif

    return(Error);
}   
#endif

#ifdef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: lnb_lnbh21_SetConfig()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnbh21_SetConfig(LNB_Config_t *Config)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
   const char *identity = "STTUNER lnbh21.c lnb_lnbh21_SetConfig()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    /* Select LNB band */
    if (LNBH21Instance->Config.ToneState != Config->ToneState)
    {
        switch (Config->ToneState)
        {
            case STTUNER_LNB_TONE_OFF:      /* No tone */
                LNBH21_SetLnb(Config->ToneState);
                break;

            case STTUNER_LNB_TONE_22KHZ:    /* 22KHz tone */
                LNBH21_SetLnb(Config->ToneState);
                break;

            case STTUNER_LNB_TONE_DEFAULT:  /* no change */
                break;

            default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
                STTBX_Print(("%s fail tone state not default, off or 22KHz\n", identity));
#endif
                return(ST_ERROR_BAD_PARAMETER);
        }
        LNBH21Instance->Config.ToneState = Config->ToneState;
    }


    /* Select polarization */
    if (LNBH21Instance->Config.Polarization != Config->Polarization)
    {
        switch (Config->Polarization)
        {
            case STTUNER_PLR_VERTICAL:      /* OP_2 controls -- 18V (high) */
                LNBH21_SetPolarization(VERTICAL);
                break;

            case STTUNER_PLR_HORIZONTAL:    /* OP_2 controls -- 13V (low) */
                LNBH21_SetPolarization(HORIZONTAL);
                break;

            default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
                STTBX_Print(("%s fail polarity not V or H\n", identity));
#endif
                return(ST_ERROR_BAD_PARAMETER);
        }
       LNBH21Instance->Config.Polarization = Config->Polarization;
    }
    
    
    
    /* Select CableLossCompensation*/
    if (LNBH21Instance->Config.CableLossCompensation != Config->CableLossCompensation)
    {
        /* Below statement is for future use */
        /*LNBH21_SetLossCompensation(Config->CableLossCompensation);*/
        /*LNBH21Instance->Config.CableLossCompensation = Config->CableLossCompensation;*/
        
        /* Set cable loss compensation off as default*/
        LNBH21_SetLossCompensation(FALSE);
    }
    
    
    /* Select ShortCircuitProtection*/
    if (LNBH21Instance->Config.ShortCircuitProtectionMode != Config->ShortCircuitProtectionMode)
    {
        switch (Config->ShortCircuitProtectionMode)
        {
        	/* The below two cases are for future use*/
            /*case STTUNER_LNB_PROTECTION_DYNAMIC:*/ /*PCL=0*/
                /*LNBH21_SetProtectionMode(Config->ShortCircuitProtectionMode);*/
                /*break;*/

            /*case STTUNER_LNB_PROTECTION_STATIC:*/ /*PCL=1*/
                /*LNBH21_SetProtectionMode(Config->ShortCircuitProtectionMode);*/
                /*break;*/

            default:
                LNBH21_SetProtectionMode(1); /* set lnb protection mode static as default*/
            	break;
         }
        
        /* Below statement is for future use */
        /*LNBH21Instance->Config.ShortCircuitProtectionMode = Config->ShortCircuitProtectionMode;*/
    }


    /* Select power status */
        switch (Config->Status)
        {
            case LNB_STATUS_ON:
            case LNB_STATUS_OFF:
                LNBH21_SetPower(Config->Status);
                break;

            default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
                STTBX_Print(("%s fail status not ON or OFF\n", identity));
#endif
                return(ST_ERROR_BAD_PARAMETER);
        }
       LNBH21Instance->Config.Status = Config->Status;
    
    /* now write in LNBH21 to update the status*/
    Error = LNBH21_UpdateLNB();

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
    STTBX_Print(("%s ok\n", identity));
#endif
    return(Error);
}   
#endif

#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: lnb_lnbh21_ioctl()

Description:
    access device specific low-level functions
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnbh21_ioctl(LNB_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
   
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
   const char *identity = "STTUNER lnbh21.c lnb_lnbh21_ioctl()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    LNBH21_InstanceData_t     *Instance;

    Instance = (LNBH21_InstanceData_t *)Handle;

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    switch(Function){

        case STTUNER_IOCTL_RAWIO: /* read/write device register (actual write) */
            switch(((SATIOCTL_IOARCH_Params_t *)InParams)->Operation)
              {
  	    	      case STTUNER_IO_READ:
                  Error = ChipGetRegisters(Instance->IOHandle,((SATIOCTL_IOARCH_Params_t *)InParams)->SubAddr, ((SATIOCTL_IOARCH_Params_t *)InParams)->TransferSize,((SATIOCTL_IOARCH_Params_t *)InParams)->Data);
                break;
                
                case STTUNER_IO_WRITE:
                	Error = ChipSetRegisters(Instance->IOHandle, ((SATIOCTL_IOARCH_Params_t *)InParams)->SubAddr, ((SATIOCTL_IOARCH_Params_t *)InParams)->Data, ((SATIOCTL_IOARCH_Params_t *)InParams)->TransferSize);
                break;
                
                default:
                	break;
               }
            break;

        case STTUNER_IOCTL_REG_IN: /* read device register */
            *(int *)OutParams = ChipGetOneRegister( Instance->IOHandle, *(int *)InParams);
            break;

        case STTUNER_IOCTL_REG_OUT: /* write device register */
            Error =  ChipSetOneRegister(  
                                                  Instance->IOHandle,
                 (U32) ((SATIOCTL_SETREG_InParams_t *)InParams)->RegIndex,
                  (U32)((SATIOCTL_SETREG_InParams_t *)InParams)->Value );
            break;
		case STTUNER_IOCTL_SETLNB: /* set lnb parameters through ioctl */
			Error = lnb_lnbh21_ioctl_set(Handle,InParams);
			break;

		case STTUNER_IOCTL_GETLNB: /* get lnb parameters through ioctl */
			Error = lnb_lnbh21_ioctl_get(Handle,OutParams);
			break;

        default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
    STTBX_Print(("%s function %d called\n", identity, Function));
#endif


	Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;

    return(Error);

}

#endif


/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */

#ifndef STTUNER_MINIDRIVER
ST_ErrorCode_t LNBH21_UpdateLNB(LNBH21_InstanceData_t     *Instance)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    /* update LNB hardware */
    ChipSetRegisters(Instance->IOHandle, RLNBH21_REGS,  (&(Instance->RegVal))  ,1);
    Error |=  IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;
	return(Error);

}
#endif
#ifdef STTUNER_MINIDRIVER
ST_ErrorCode_t LNBH21_UpdateLNB( )
{
	ST_ErrorCode_t        Error = ST_NO_ERROR;
    
    	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_LNB_WRITE, RLNBH21_REGS, 0x00, 0x00, LNBH21_Default_Reg, 1, FALSE);

	return(Error);

}

#endif


/*----------------------------------------------------
 FUNCTION      LNBH21_SetLnb
 ACTION        set the Lnb
 PARAMS IN     Lnb -> true for LnbHigh, false for LnbLow
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
#ifndef STTUNER_MINIDRIVER
void LNBH21_SetLnb(LNBH21_InstanceData_t     *Instance,  int Lnb)
{
		
	/* Low band -> no 22KHz tone */
	if (Lnb == STTUNER_LNB_TONE_OFF) 
	ChipSetFieldImage(FLNBH21_TEN, 0, &(Instance->RegVal)  );
	
	/* High band -> 22KHz tone */
	if (Lnb == STTUNER_LNB_TONE_22KHZ) 
	ChipSetFieldImage(FLNBH21_TEN, 1, &(Instance->RegVal) );
	
}
#endif
#ifdef STTUNER_MINIDRIVER
void LNBH21_SetLnb(int Lnb)
{
	/* Low band -> no 22KHz tone */
	if (Lnb == STTUNER_LNB_TONE_OFF) 
	{
		LNBH21_Default_Reg[0x00] &= 0xDF;
	}
	else if(Lnb == STTUNER_LNB_TONE_22KHZ) 
	{
		LNBH21_Default_Reg[0x00] |= 0x20;
	}
	
}
#endif


/*----------------------------------------------------
 FUNCTION      LNBH21_SetPolarization
 ACTION        set the polarization
 PARAMS IN     Polarization -> Polarization
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
#ifndef STTUNER_MINIDRIVER
void LNBH21_SetPolarization(LNBH21_InstanceData_t     *Instance, LNB_Polarization_t Polarization)
{
    if(Polarization == VERTICAL)/* Set 13V for Vertical Pol*/
    {
    	ChipSetFieldImage( FLNBH21_EN, 1,&(Instance->RegVal) );
        ChipSetFieldImage( FLNBH21_VSEL, 0,&(Instance->RegVal)  );
       
    }
    else if(Polarization == HORIZONTAL)/* Set 18v for Horizontal Pol*/
    {
    	ChipSetFieldImage(FLNBH21_EN, 1,&(Instance->RegVal) );
        ChipSetFieldImage( FLNBH21_VSEL, 1,&(Instance->RegVal)  );
    
    }
    else if(Polarization == NOPOLARIZATION)/* Set 18v for Horizontal Pol*/
        {
        ChipSetFieldImage( FLNBH21_EN, 0, &(Instance->RegVal)  );
      
	}
}
#endif
#ifdef STTUNER_MINIDRIVER
void LNBH21_SetPolarization(LNB_Polarization_t Polarization)
{

        /* Low band -> no 22KHz tone */
	if (Polarization == VERTICAL) 
	{
		LNBH21_Default_Reg[0x00] &= 0xF7;
		LNBH21_Default_Reg[0x00] |= 0x04;
	}
	else if(Polarization == HORIZONTAL) 
	{
		LNBH21_Default_Reg[0x00] |= 0x08;
		LNBH21_Default_Reg[0x00] |= 0x04;
	}
	else if(Polarization == NOPOLARIZATION) 
	{
		LNBH21_Default_Reg[0x00] &= 0xFB;
	}
}
#endif
/*----------------------------------------------------
 FUNCTION      LNBH21_SetCurrentThreshold
 ACTION        set the Current for LNB
 PARAMS IN     CurrentSelection 
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
/*
void LNBH21_SetCurrentThreshold(STTUNER_IOREG_DeviceMap_t *DeviceMap, LNB_CurrentThresholdSelection_t CurrentThresholdSelection)
{
	if(CurrentThresholdSelection == CURRENT_THRESHOLD_HIGH)*//* Set ISEL=0 to set high(500-650mA)*/
        /*STTUNER_IOREG_FieldSetVal(DeviceMap, FLNB21_ISEL, 0);
        if(CurrentThresholdSelection == CURRENT_THRESHOLD_LOW)*//* Set ISEL=1 to set low(400-550mA)*/
       /* STTUNER_IOREG_FieldSetVal(DeviceMap, FLNB21_ISEL, 1);
}*/
#ifdef STTUNER_MINIDRIVER
void LNBH21_SetProtectionMode(LNB_ShortCircuitProtectionMode_t ShortCircuitProtectionMode)
{
	if (ShortCircuitProtectionMode == LNB_PROTECTION_DYNAMIC) 
	{
		LNBH21_Default_Reg[0x00] &= 0x7F;
	}
	else if(ShortCircuitProtectionMode == LNB_PROTECTION_STATIC) 
	{
		LNBH21_Default_Reg[0x00] |= 0x80;
	}
}
#endif
 
/*----------------------------------------------------
 FUNCTION      LNBH21_SetProtectionMode
 ACTION        set the short circuit for LNB
 PARAMS IN     ShortCircuitProtection 
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/	
#ifndef STTUNER_MINIDRIVER
void LNBH21_SetProtectionMode(LNBH21_InstanceData_t     *Instance, LNB_ShortCircuitProtectionMode_t ShortCircuitProtectionMode)
{
	if(ShortCircuitProtectionMode == LNB_PROTECTION_DYNAMIC)/* dynamic short circuit protection*/
	 ChipSetFieldImage( FLNBH21_PCL, 0,&(Instance->RegVal) );	
	   	
	if(ShortCircuitProtectionMode == LNB_PROTECTION_STATIC)/* static short circuit protection*/
	ChipSetFieldImage(FLNBH21_PCL, 1,&(Instance->RegVal)  );
	    
}


/*----------------------------------------------------
 FUNCTION      LNBH21_SetLossCompensation
 ACTION        compensation for coaxial cable loss 
 PARAMS IN     CoaxCableLossCompensation 
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/	
void LNBH21_SetLossCompensation(LNBH21_InstanceData_t     *Instance, BOOL CoaxCableLossCompensation)
{
	if(CoaxCableLossCompensation == TRUE)/* add +1 in 13/18V*/
	ChipSetFieldImage( FLNBH21_LLC, 1,&(Instance->RegVal)  );
	
	
	if(CoaxCableLossCompensation == FALSE)/* normal 13/18V */
	ChipSetFieldImage( FLNBH21_LLC, 0,&(Instance->RegVal)  );
	   
}
#endif
#ifdef STTUNER_MINIDRIVER
void LNBH21_SetLossCompensation(BOOL CoaxCableLossCompensation)
{

	if (CoaxCableLossCompensation == TRUE) 
	{
		LNBH21_Default_Reg[0x00] &= 0xEF;
	}
	else if(CoaxCableLossCompensation == FALSE) 
	{
		LNBH21_Default_Reg[0x00] |= 0x10;
	}
}
#endif
#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION      LNBH21_GetPower
 ACTION        get lnb power status
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
ST_ErrorCode_t LNBH21_GetPower(LNB_Handle_t Handle, LNB_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
   const char *identity = "STTUNER lnbh21.c lnb_lnbh21_GetPower()";
#endif

    U8 powerstatus;
    LNBH21_InstanceData_t     *Instance;
	ST_ErrorCode_t Error = ST_NO_ERROR;

    	Instance = (LNBH21_InstanceData_t *)Handle;

	if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
	 {
	   #ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
	        STTBX_Print(("%s invalid handle\n", identity));
	   #endif
	        return(ST_ERROR_INVALID_HANDLE);
	 }

    
    powerstatus = ChipGetOneRegister(Instance->IOHandle,(U16) RLNBH21_REGS);
    if((powerstatus & 0x04) == 0x04)
    *Status = LNB_STATUS_ON;
    else
    *Status = LNB_STATUS_OFF;

   	Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}
#endif

#ifdef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION      LNBH21_GetPower
 ACTION        get lnb power status
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
ST_ErrorCode_t LNBH21_GetPower(LNB_Status_t *Status)
{
    U8 powerstatus;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    Error = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_LNB_READ, RLNBH21_REGS, 0x00, 0x00, &powerstatus, 1, FALSE);
    if((powerstatus & 0x04) == 0x04)
    *Status = LNB_STATUS_ON;
    else
    *Status = LNB_STATUS_OFF;
    return(Error);
}
#endif

#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION     LNBH21_SetPower

 ACTION        set lnb power
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
ST_ErrorCode_t LNBH21_SetPower(LNBH21_InstanceData_t     *Instance, LNB_Status_t Status)
{
	ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
    const char *identity = "STTUNER lnbh21.c LNBH21_SetPower()";
#endif

    switch (Status)
    {
        case LNB_STATUS_ON:
        ChipSetFieldImage( FLNBH21_EN, 1, &(Instance->RegVal)  );
           
        break;
        case LNB_STATUS_OFF:
        ChipSetFieldImage( FLNBH21_EN, 0,&(Instance->RegVal)  );
     
        break;
        default:
        break;
    }

	Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}
#endif

#ifdef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION     LNBH21_SetPower
 ACTION        set lnb power
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
ST_ErrorCode_t LNBH21_SetPower(LNB_Status_t Status)
{
	U8 powerstatus;
	ST_ErrorCode_t Error = ST_NO_ERROR;

    switch (Status)
    {
        case LNB_STATUS_ON:
                 
            Error = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_LNB_READ, RLNBH21_REGS, 0x00, 0x00, &powerstatus, 1, FALSE);
            powerstatus |= 0x04;
            Error |= STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_LNB_WRITE, RLNBH21_REGS, 0x00, 0x00, &powerstatus, 1, FALSE);
            break;
        case LNB_STATUS_OFF:/* always remain LNB ON, to get uninterrupted LNB power supply*/
            Error = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_LNB_READ, RLNBH21_REGS, 0x00, 0x00, &powerstatus, 1, FALSE);
            powerstatus &= 0xFB;
            Error |= STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_LNB_WRITE, RLNBH21_REGS, 0x00, 0x00, &powerstatus, 1, FALSE);
            break;
        default:
            break;
    }

    return(Error);
}
#endif

#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION     LNBH21_GetStatus

 ACTION        Get LNb status -> ShortCircuit(OLF), Over Temperature(OTF)
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
ST_ErrorCode_t LNBH21_GetStatus(LNB_Handle_t Handle, LNB_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
   const char *identity = "STTUNER lnbh21.c lnb_lnbh21_getstatus()";
#endif
	U8 olf_otf; /* short circuit and over temp. reporting*/
	LNBH21_InstanceData_t     *Instance;
	ST_ErrorCode_t Error = ST_NO_ERROR;

    	Instance = (LNBH21_InstanceData_t *)Handle;

	if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
	 {
	   #ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
	        STTBX_Print(("%s invalid handle\n", identity));
	   #endif
	        return(ST_ERROR_INVALID_HANDLE);
	 }
	 

	
	olf_otf = ChipGetOneRegister(Instance->IOHandle, (U16)RLNBH21_REGS);
	
	if((olf_otf & 0x01)== 0x01)
	(*Status) = LNB_STATUS_SHORT_CIRCUIT;
	
	if((olf_otf & 0x02) == 0x02)
        (*Status) = LNB_STATUS_OVER_TEMPERATURE;
        
        if((olf_otf & 0x03) == 0x03)
        (*Status) = LNB_STATUS_SHORTCIRCUIT_OVERTEMPERATURE;
  
	Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;
		
		return(Error);      
}
#endif

#ifdef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION     LNBH21_GetStatus
 ACTION        Get LNb status -> ShortCircuit(OLF), Over Temperature(OTF)
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
ST_ErrorCode_t LNBH21_GetStatus(LNB_Status_t *Status)
{
	U8 olf_otf; /* short circuit and over temp. reporting*/
	
	ST_ErrorCode_t Error = ST_NO_ERROR;
	
	Error = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_LNB_READ, RLNBH21_REGS, 0x00, 0x00, &olf_otf, 1, FALSE);
	
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
/* ----------------------------------------------------------------------------
Name: lnb_lnbh21_ioctl_set()

Description:
    access device specific low-level functions
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnbh21_ioctl_set(LNB_Handle_t Handle,void *InParams)
   
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
   const char *identity = "STTUNER lnbh21.c lnb_lnbh21_ioctl_get()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    LNBH21_InstanceData_t     *Instance;
    LNB_IOCTL_Config_t    *lnb_ioctl_input;
	
    
    lnb_ioctl_input = (LNB_IOCTL_Config_t *)InParams;
    Instance = (LNBH21_InstanceData_t *)Handle;


    
    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }
  
/* set the Tx mode */
  switch(lnb_ioctl_input->TTX_Mode)
  {
    case  LNB_IOCTL_TTXMODE_NOCHANGE : /* Do nothing */
           break;
    case  LNB_IOCTL_TTXMODE_RX  : /* set LNBH21 to receiver mode */
        	ChipSetFieldImage(FLNBH21_ISEL, 0, &(Instance->RegVal) );
     break;
    case  LNB_IOCTL_TTXMODE_TX : /* set LNBH21 to transmitter mode */
            ChipSetFieldImage(FLNBH21_ISEL, 1, &(Instance->RegVal)  );
     break;
     default:
       break; /* do nothing */ 
      
  }
  
  /* set current protection mode */
  switch(lnb_ioctl_input->ShortCircuitProtectionMode)
  {
    case  LNB_IOCTL_PROTECTION_NOCHANGE : /* Do nothing */
           break;
    case  LNB_IOCTL_PROTECTION_STATIC  : /* set current portection to static mode */
          ChipSetFieldImage(FLNBH21_PCL, 1, &(Instance->RegVal) );
     break;
    case  LNB_IOCTL_PROTECTION_DYNAMIC : /* set current portection to dynamic mode */
          ChipSetFieldImage( FLNBH21_PCL, 0, &(Instance->RegVal));
     break;
     default:
       break; /* do nothing */ 
      
  }
 
  /* setpower block  mode */
  switch(lnb_ioctl_input->PowerControl)
  {
    case  LNB_IOCTL_POWERBLCOKS_NOCHANGE : /* Do nothing */
           break;
    case  LNB_IOCTL_POWERBLCOKS_ENABLED  : /* set LNBH21 to power enable mode */
      	ChipSetFieldImage(FLNBH21_EN, 1,&(Instance->RegVal)  );
     break;
    case  LNB_IOCTL_POWERBLCOKS_DISABLED : /* set LNBH21 to power disable mode */
       ChipSetFieldImage(FLNBH21_EN, 0,&(Instance->RegVal) );
     break;
     default:
       break; /* do nothing */ 
      
  }
   /* setpower LLC Mode */
  switch(lnb_ioctl_input->LLC_Mode)
  {
    case  LNB_IOCTL_LLCMODE_NOCHANGE : /* Do nothing */
           break;
    case  LNB_IOCTL_LLCMODE_ENABLED  : /* set LNBH21 to LLC mode ON 13/18V +1 */
      	ChipSetFieldImage( FLNBH21_LLC, 1, &(Instance->RegVal)  );
     break;
    case  LNB_IOCTL_LLCMODE_DISABLED : /* set LNBH21 to LLC mode OFF 13/18V */
      ChipSetFieldImage( FLNBH21_LLC, 0, &(Instance->RegVal) );
     break;
     default:
       break; /* do nothing */ 
      
  }
  
	Error = LNBH21_UpdateLNB(Instance);

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
    STTBX_Print(("%s function called\n", identity));
#endif


	Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;

    return(Error);

}
/* ----------------------------------------------------------------------------
Name: lnb_lnbh21_ioctl_get()

Description:
    access device specific low-level functions
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnbh21_ioctl_get(LNB_Handle_t Handle,void *OutParams)
   
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
   const char *identity = "STTUNER lnbh21.c lnb_lnbh21_ioctl_get()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    LNBH21_InstanceData_t     *Instance;
    LNB_IOCTL_Config_t   *lnb_ioctl_output;
    
     
    lnb_ioctl_output = (LNB_IOCTL_Config_t *)OutParams;  
    Instance = (LNBH21_InstanceData_t *)Handle;

    
       
    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    } 
  /* Get the TTX mode */
  lnb_ioctl_output->TTX_Mode = ChipGetField (Instance->IOHandle,  FLNBH21_ISEL);
  switch( lnb_ioctl_output->TTX_Mode)
  {
    case 0:
	    lnb_ioctl_output->TTX_Mode = LNB_IOCTL_TTXMODE_RX;
		break;
    case 1:
	    lnb_ioctl_output->TTX_Mode = LNB_IOCTL_TTXMODE_TX;
		break;
    default:
	    break;
  } 
  /* Get current protection mode */
  lnb_ioctl_output->ShortCircuitProtectionMode = ChipGetField( Instance->IOHandle,  FLNBH21_PCL);
   switch( lnb_ioctl_output->ShortCircuitProtectionMode)
  {
    case 0:
	    lnb_ioctl_output->ShortCircuitProtectionMode = LNB_IOCTL_PROTECTION_DYNAMIC;
		break;
    case 1:
	    lnb_ioctl_output->ShortCircuitProtectionMode = LNB_IOCTL_PROTECTION_STATIC;
		break;
    default:
	    break;
  } 
  /* Get lnb power mode */
  lnb_ioctl_output->PowerControl = ChipGetField( Instance->IOHandle ,  FLNBH21_EN);
   switch( lnb_ioctl_output->PowerControl)
  {
    case 0:
	    lnb_ioctl_output->PowerControl = LNB_IOCTL_POWERBLCOKS_DISABLED;
		break;
    case 1:
	    lnb_ioctl_output->PowerControl = LNB_IOCTL_POWERBLCOKS_ENABLED;
		break;
    default:
	    break;
  } 
  
  lnb_ioctl_output->LLC_Mode = ChipGetField( Instance->IOHandle ,  FLNBH21_LLC);
   switch( lnb_ioctl_output->LLC_Mode)
  {
    case 0:
	    lnb_ioctl_output->LLC_Mode = LNB_IOCTL_LLCMODE_DISABLED;
		break;
    case 1:
	    lnb_ioctl_output->LLC_Mode = LNB_IOCTL_LLCMODE_ENABLED;
		break;
    default:
	    break;
  } 

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
    STTBX_Print(("%s function  called\n", identity));
#endif

    Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
	    
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;


    return(Error);

}
#endif
#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: lnb_lnbh21_overloadcheck()

Description:
    access device specific low-level functions
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t lnb_lnbh21_overloadcheck(LNB_Handle_t Handle, BOOL  *IsOverTemp, BOOL *IsCurrentOvrLoad)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
   const char *identity = "STTUNER lnbh21.c lnb_lnbh21_overloadcheck()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    LNBH21_InstanceData_t     *Instance;
    
    Instance = (LNBH21_InstanceData_t *)Handle;

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    } 
    
    /* Read OLF value */
    *IsCurrentOvrLoad = ChipGetField( Instance->IOHandle,  FLNBH21_OLF);
   
     /* Read OTF value */ 
    *IsOverTemp = ChipGetField( Instance->IOHandle,  FLNBH21_OTF);
    
    Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;  
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;
    return(Error);
	
}
#endif
#ifdef STTUNER_MINIDRIVER
ST_ErrorCode_t lnb_lnbh21_overloadcheck(BOOL  *IsOverTemp, BOOL *IsCurrentOvrLoad)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    /* Read OLF value */
    Error = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_LNB_READ, RLNBH21_REGS, 0x00, 0x00, (U8 *)IsCurrentOvrLoad, 1, FALSE);
    *IsCurrentOvrLoad &= 0x01;
       
     /* Read OTF value */ 
     Error = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_LNB_READ, RLNBH21_REGS, 0x00, 0x00, (U8 *)IsOverTemp, 1, FALSE);
    *IsOverTemp = (*IsOverTemp & 0x02)>>1;
    
    return(Error);
	
}
#endif

#ifndef STTUNER_MINIDRIVER
ST_ErrorCode_t lnb_lnbh21_setttxmode(LNB_Handle_t Handle, STTUNER_LnbTTxMode_t Ttxmode)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
   const char *identity = "STTUNER lnbh21.c lnb_lnbh21_setttxmode()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    LNBH21_InstanceData_t     *Instance;
    Instance = (LNBH21_InstanceData_t *)Handle;

      
    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    switch(Ttxmode)
    {
    	case STTUNER_LNB_RX:
    	   ChipSetFieldImage(FLNBH21_ISEL,0, &(Instance->RegVal) );
    	   break;
    	case STTUNER_LNB_TX:
    	   ChipSetFieldImage( FLNBH21_ISEL,1, &(Instance->RegVal)  );
    	   break;
    	default:
    	break; /* do nothing */
    }
    
    Error = LNBH21_UpdateLNB(Instance);
       
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
    STTBX_Print(("%s function called\n", identity));
#endif
    Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;   
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;
    return(Error);  
}
#endif

#ifdef STTUNER_MINIDRIVER
ST_ErrorCode_t lnb_lnbh21_setttxmode(STTUNER_LnbTTxMode_t Ttxmode)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
   const char *identity = "STTUNER lnbh21.c lnb_lnbh21_setttxmode()";
#endif
     U8 txrxstatus;    
     ST_ErrorCode_t Error = ST_NO_ERROR;
    

    
    switch(Ttxmode)
    {
    	case STTUNER_LNB_RX:
    	    Error = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_LNB_READ, RLNBH21_REGS, 0x00, 0x00, &txrxstatus, 1, FALSE);
            txrxstatus &= 0xBF;
            Error |= STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_LNB_WRITE, RLNBH21_REGS, 0x00, 0x00, &txrxstatus, 1, FALSE);
            break;
    	case STTUNER_LNB_TX:
    	    Error = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_LNB_READ, RLNBH21_REGS, 0x00, 0x00, &txrxstatus, 1, FALSE);
            txrxstatus |= 0x40;
            Error |= STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_LNB_WRITE, RLNBH21_REGS, 0x00, 0x00, &txrxstatus, 1, FALSE);
            break;
    	default:
    	break; /* do nothing */
    }
       
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNBH21
    STTBX_Print(("%s function called\n", identity));
#endif
    
    return(Error);  
}
#endif

/* End of lnbh21.c */
