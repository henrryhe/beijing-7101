/* ----------------------------------------------------------------------------
File Name: lstem.c

Description: 

    STEM LNB driver (should setup I/O passthrough to a stv0299 demod driver)


Copyright (C) 1999-2001 STMicroelectronics

History:
 
   date: 19-March-2002
version: 
 author: SJD
comment: 
    
   date: 10-April-2002
version: 
 author: GJP
comment: polarity selection corrected.
    
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
#include "ioarch.h"     /* I/O for this driver */
#include "ioreg.h"      /* I/O register mapping */
#include "sdrv.h"       /* utilities */

#include "lstem.h"      /* header for this file */


#include "sioctl.h"     /* data structure typedefs for all the the sat ioctl functions */

/* Global variable to keep track of current running task */
extern task_t *TunerTask1, *TunerTask2; 

/* Global variable which sets to 2 when both task are running and helps
 to differentiate when single tuner is running and when dual tuner is running */
extern U8 ucDualTunerFlag;

/* Added by for Debug 22 July 2k2 */
#ifdef USE_TRACE_BUFFER_FOR_DEBUG
#define DMP
extern void DUMP_DATA(char *info) ;
extern char Trace_PrintBuffer[40]; /* global variable to be seen by modules */
#endif

/* macros ------------------------------------------------------------------ */

/* Delay calling task for a period of microseconds */

#define LSTEM_Delay_uS(micro_sec) STOS_TaskDelayUs(micro_sec) 
  

#if defined (ST_OS21) || defined (ST_OSLINUX)
#define STTUNER_TaskDelay(x) STOS_TaskDelay((signed int)x)
#else
#define STTUNER_TaskDelay(x) STOS_TaskDelay((unsigned int)x)
#endif
/* private variables ------------------------------------------------------- */

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */

static BOOL        Installed = FALSE;
/* ---------- per instance of driver ---------- */

/*  Test Data and Function declaration--------------------------------*/
extern U8 PCF8574RegisterValue;

extern U32 DemodHandleOne;
extern U32 DemodHandleTwo;

extern U32 LnbHandleOne ;
extern U32 LnbHandleTwo ;
U8 AccessFlag=0;

/* Test Data and Function declaration ------------------------------------------------*/

typedef struct
{
    ST_DeviceName_t           *DeviceName;           /* unique name for opening under */
    STTUNER_Handle_t          TopLevelHandle;   /* access tuner, demod etc. using this */
    IOARCH_Handle_t           IOHandle;         /* instance access to I/O driver       */
    STTUNER_IOREG_DeviceMap_t DeviceMap;        /* stem register map & data table   */
    LNB_Config_t              Config;           /* LNB config for each instance        */
    ST_Partition_t            *MemoryPartition;     /* which partition this data block belongs to */
    void                      *InstanceChainPrev;   /* previous data block in chain or NULL if not */
    void                      *InstanceChainNext;   /* next data block in chain or NULL if last */
} LSTEM_InstanceData_t;

/* instance chain, the default boot value is invalid, to catch errors */
static LSTEM_InstanceData_t *InstanceChainTop = (LSTEM_InstanceData_t *)0x7fffffff;

/* functions --------------------------------------------------------------- */

/* API */
ST_ErrorCode_t lnb_lSTEM_Init(ST_DeviceName_t *DeviceName,LNB_InitParams_t *InitParams);
ST_ErrorCode_t lnb_lSTEM_Term(ST_DeviceName_t *DeviceName,LNB_TermParams_t *TermParams);

ST_ErrorCode_t lnb_lSTEM_Open (ST_DeviceName_t *DeviceName, LNB_OpenParams_t *OpenParams, LNB_Capability_t *Capability, LNB_Handle_t *Handle);
ST_ErrorCode_t lnb_lSTEM_Close(LNB_Handle_t  Handle, LNB_CloseParams_t *CloseParams);

ST_ErrorCode_t lnb_lSTEM_GetConfig(LNB_Handle_t Handle, LNB_Config_t *Config);
ST_ErrorCode_t lnb_lSTEM_SetConfig(LNB_Handle_t Handle, LNB_Config_t *Config);

/* I/O API */
ST_ErrorCode_t lnb_lSTEM_ioaccess(LNB_Handle_t Handle, IOARCH_Handle_t IOHandle,
    STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);

/* access device specific low-level functions */
ST_ErrorCode_t lnb_lSTEM_ioctl(LNB_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status);


/* local functions */
void           LSTEM_SetLnb         (IOARCH_Handle_t LNBHandle, int Lnb);
void           LSTEM_SetPolarization(IOARCH_Handle_t LNBHandle, LNB_Polarization_t Polarization);
ST_ErrorCode_t LSTEM_SetPower       (IOARCH_Handle_t LNBHandle, LNB_Status_t  Status);
ST_ErrorCode_t LSTEM_GetPower       (IOARCH_Handle_t LNBHandle, LNB_Status_t *Status);
void           LSTEM_UpdateLNB      (IOARCH_Handle_t LNBHandle);


/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_LNB_STEM_Install()

Description:
    install a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_LNB_STEM_Install(STTUNER_lnb_dbase_t *Lnb)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
   const char *identity = "STTUNER lstem.c STTUNER_DRV_LNB_STEM_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("%s fail driver already installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("%s installing sat:lnb:STEM...", identity));
#endif

    /* mark ID in database */
    Lnb->ID = STTUNER_LNB_STEM;

    /* map API */
    Lnb->lnb_Init  = lnb_lSTEM_Init;
    Lnb->lnb_Term  = lnb_lSTEM_Term;
    Lnb->lnb_Open  = lnb_lSTEM_Open;
    Lnb->lnb_Close = lnb_lSTEM_Close;

    Lnb->lnb_GetConfig = lnb_lSTEM_GetConfig; 
    Lnb->lnb_SetConfig = lnb_lSTEM_SetConfig;

    Lnb->lnb_ioaccess = lnb_lSTEM_ioaccess;
    Lnb->lnb_ioctl    = lnb_lSTEM_ioctl;
    #if defined(STTUNER_DRV_SAT_LNB21) || defined(STTUNER_DRV_SAT_LNBH21) /*Added for for GNBvd40148-->Error
    if sttuner compiled with LNBH21 option but use another one: This will only come when more than 1 lnb is
    in use & atleast one of them is LNB21 or LNBH21*/
    Lnb->lnb_overloadcheck    = lnb_lstem_overloadcheck;
    #if defined(STTUNER_DRV_SAT_LNBH21)
    Lnb->lnb_setttxmode       = lnb_lstem_setttxmode;
    #endif
    #endif

    InstanceChainTop = NULL;
 
    Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);
    

    Installed = TRUE;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("ok\n"));
#endif    
    return(Error);
}


   
/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_LNB_STEM_UnInstall()

Description:
    install a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_LNB_STEM_UnInstall(STTUNER_lnb_dbase_t *Lnb)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
   const char *identity = "STTUNER lstem.c STTUNER_DRV_LNB_STEM_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }
   
    if(Lnb->ID != STTUNER_LNB_STEM)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("%s fail incorrect driver type\n", identity));
#endif
        return(STTUNER_ERROR_ID);
    }

    /* has all memory been freed, by Term() */
    if(InstanceChainTop != NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("%s fail at least one instance not terminated\n", identity));
#endif
        return(ST_ERROR_OPEN_HANDLE);
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("%s uninstalling sat:lnb:STEM...", identity));
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
    #if defined(STTUNER_DRV_SAT_LNB21) || defined(STTUNER_DRV_SAT_LNBH21)
    Lnb->lnb_overloadcheck    = NULL;
    #if defined(STTUNER_DRV_SAT_LNBH21)
    Lnb->lnb_setttxmode       = NULL;
    #endif
    #endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
    STTBX_Print(("<"));
#endif

   
    STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);
    
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
    STTBX_Print((">"));
#endif

    InstanceChainTop = (LSTEM_InstanceData_t *)0x7ffffffe;
    Installed        = FALSE;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
    STTBX_Print(("ok\n"));
#endif

    return(Error);
}


/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */


   
/* ----------------------------------------------------------------------------
Name: lnb_lSTEM_Init()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lSTEM_Init(ST_DeviceName_t *DeviceName,LNB_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
   const char *identity = "STTUNER lstem.c lnb_lSTEM_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    LSTEM_InstanceData_t *InstanceNew, *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
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
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( LSTEM_InstanceData_t ));
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
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
    InstanceNew->DeviceMap.Registers = DEF_LNB_STEM_NBREG;
    InstanceNew->DeviceMap.Fields    = DEF_LNB_STEM_NBFIELD;
    InstanceNew->DeviceMap.Mode      = IOREG_MODE_NOSUBADR; /* Required for STEM LNB type */
    InstanceNew->DeviceMap.MemoryPartition = InitParams->MemoryPartition;
    InstanceNew->InstanceChainNext   = NULL; /* always last in the chain */

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes)\n", identity, InstanceNew->DeviceName, (U32)InstanceNew, sizeof( LSTEM_InstanceData_t ) ));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: lnb_lSTEM_Term()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lSTEM_Term(ST_DeviceName_t *DeviceName,LNB_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
   const char *identity = "STTUNER lstem.c lnb_lstem_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    LSTEM_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
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
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }


    /* reap next matching DeviceName */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
    STTBX_Print(("%s looking for first free handle[\n", identity));
#endif

    Instance = InstanceChainTop;
    while(1)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        if ( strcmp((char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
            STTBX_Print(("]\n"));
#endif
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
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
            STTBX_Print(("%s terminated ok\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL) 
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
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


#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}   

/* ----------------------------------------------------------------------------
Name: lnb_lSTEM_Open()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lSTEM_Open (ST_DeviceName_t *DeviceName, LNB_OpenParams_t *OpenParams, LNB_Capability_t *Capability, LNB_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
   const char *identity = "STTUNER lstem.c lnb_lstem_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    LSTEM_InstanceData_t     *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
#endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL) 
        {       
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
    STTBX_Print((" found ok\n"));
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
    STTBX_Print(("%s using block at 0x%08x\n", identity, (U32)Instance));
#endif

    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("%s fail driver instance already open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    /* now got pointer to free (and valid) data block */

/* Moved here for Dual Stem */
 
    Instance->TopLevelHandle = OpenParams->TopLevelHandle;  /* mark as valid */
    *Handle = (U32)Instance;


    LSTEM_UpdateLNB(Instance->IOHandle);

/* Moved till here for Dual Stem */   
    
    /* Set LNB capabilties */
    Capability->ShortCircuitDetect = FALSE;
    Capability->PowerAvailable     = TRUE;
    Capability->PolarizationSelect = STTUNER_PLR_ALL;
 
    /* Set latest configuration */
    Instance->Config.Status       = LNB_STATUS_OFF;
    Instance->Config.ToneState    = STTUNER_LNB_TONE_OFF;
    Instance->Config.Polarization = STTUNER_LNB_OFF;

    /* LNB tone off */
    LSTEM_SetLnb(Instance->IOHandle,0);
    /* LNB polarization mode set to Horizontal*/
    LSTEM_SetPower(Instance->IOHandle, LNB_STATUS_OFF);

    LSTEM_UpdateLNB(Instance->IOHandle);

    LSTEM_Delay_uS(500);

    /* Check to ensure there is no problem with the LNB circuit */
    LSTEM_UpdateLNB(Instance->IOHandle);
    Error = LSTEM_GetPower(Instance->IOHandle, &Instance->Config.Status);
    if (Error == ST_NO_ERROR)   /* last i/o (above) operation was good */
    {
        if (Instance->Config.Status == LNB_STATUS_SHORT_CIRCUIT)
        {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
            STTBX_Print(("%s fail SAT_LNB_SHORT_CIRCUIT\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_HWFAIL);
        }
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("%s fail Error=%u\n", identity, Error));   /* LSTEM_GetPower failed */
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

/*
    Instance->TopLevelHandle = OpenParams->TopLevelHandle;  *//* mark as valid */
/*    *Handle = (U32)Instance;
*/

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
    STTBX_Print(("%s opened ok\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: lnb_lSTEM_Close()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lSTEM_Close(LNB_Handle_t Handle, LNB_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
   const char *identity = "STTUNER lstem.c lnb_lstem_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    LSTEM_InstanceData_t     *Instance;

    Instance = (LSTEM_InstanceData_t *)Handle;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
    STTBX_Print(("%s closed\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}   

#if defined(STTUNER_DRV_SAT_LNB21) || defined(STTUNER_DRV_SAT_LNBH21)
/* ----------------------------------------------------------------------------
Name: lnb_lstem_overloadcheck()

Description:
    Dummy Function
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t lnb_lstem_overloadcheck(LNB_Handle_t Handle, BOOL  *IsOverTemp, BOOL *IsCurrentOvrLoad)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    return(Error);
}

#if defined(STTUNER_DRV_SAT_LNBH21)
/* ----------------------------------------------------------------------------
Name: lnb_lstem_setttxmode()

Description:
    Dummy Function
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t lnb_lstem_setttxmode(LNB_Handle_t Handle, STTUNER_LnbTTxMode_t Ttxmode)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    return(Error);
}
#endif
#endif


/* ----------------------------------------------------------------------------
Name: lnb_lSTEM_GetConfig()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lSTEM_GetConfig(LNB_Handle_t Handle, LNB_Config_t *Config)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
   const char *identity = "STTUNER lstem.c lnb_lSTEM_GetConfig()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    LSTEM_InstanceData_t     *Instance;

    Instance = (LSTEM_InstanceData_t *)Handle;

    Config->Status       = Instance->Config.Status;
    Config->Polarization = Instance->Config.Polarization;
    Config->ToneState    = Instance->Config.ToneState;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
    STTBX_Print(("%s called\n", identity));
#endif
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: lnb_lSTEM_SetConfig()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lSTEM_SetConfig(LNB_Handle_t Handle, LNB_Config_t *Config)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
   const char *identity = "STTUNER lstem.c lnb_lstem_SetConfig()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    LSTEM_InstanceData_t     *Instance;

    Instance = (LSTEM_InstanceData_t *)Handle;


    /* Select LNB band */
    if (Instance->Config.ToneState != Config->ToneState)
    {
        switch (Config->ToneState)
        {
            case STTUNER_LNB_TONE_OFF:      /* No tone */
                LSTEM_SetLnb(Instance->IOHandle, 0);
                break;

            case STTUNER_LNB_TONE_22KHZ:    /* 22KHz tone */
                LSTEM_SetLnb(Instance->IOHandle, 1);
                break;

            case STTUNER_LNB_TONE_DEFAULT:  /* no change */
                break;

            default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
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
                LSTEM_SetPolarization(Instance->IOHandle, VERTICAL);
                break;

            case STTUNER_PLR_HORIZONTAL:    /* OP_2 controls -- 13V (low) */
                LSTEM_SetPolarization(Instance->IOHandle, HORIZONTAL);
                break;

            default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
                STTBX_Print(("%s fail polarity not V or H\n", identity));
#endif
                return(ST_ERROR_BAD_PARAMETER);
        }
       Instance->Config.Polarization = Config->Polarization;
    }


    /* Select power status */
    if (Instance->Config.Status != Config->Status)
    {
        switch (Config->Status)
        {
            case LNB_STATUS_ON:
            case LNB_STATUS_OFF:
                LSTEM_SetPower(Instance->IOHandle, Config->Status);
                break;

            default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
                STTBX_Print(("%s fail status not ON or OFF\n", identity));
#endif
                return(ST_ERROR_BAD_PARAMETER);
        }
       Instance->Config.Status = Config->Status;
    }

    LSTEM_UpdateLNB(Instance->IOHandle);

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
    STTBX_Print(("%s ok\n", identity));
#endif
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: lnb_lSTEM_ioctl()

Description:
    access device specific low-level functions
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lSTEM_ioctl(LNB_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
   
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
   const char *identity = "STTUNER lstem.c lnb_lstem_ioctl()";
#endif
    ST_ErrorCode_t Error;
    LSTEM_InstanceData_t     *Instance;

    Instance = (LSTEM_InstanceData_t *)Handle;

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    switch(Function){

        case STTUNER_IOCTL_RAWIO: /* read/write device register (actual write to STEM) */
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
            Error =  STTUNER_IOREG_SetRegister(  &Instance->DeviceMap, 
                                                  Instance->IOHandle,
                  ((SATIOCTL_SETREG_InParams_t *)InParams)->RegIndex,
                  ((SATIOCTL_SETREG_InParams_t *)InParams)->Value );
            break;

        default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
    STTBX_Print(("%s function %d called\n", identity, Function));
#endif
    return(Error);

}



/* ----------------------------------------------------------------------------
Name: lnb_lSTEM_ioaccess()

Description:
    we get called with the instance of
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lSTEM_ioaccess(LNB_Handle_t Handle, IOARCH_Handle_t IOHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
   const char *identity = "STTUNER lstem.c lnb_lSTEM_ioaccess()";
#endif
    ST_ErrorCode_t        Error = ST_NO_ERROR;
    IOARCH_Handle_t       LNB_IOHandle;
    LSTEM_InstanceData_t *Instance;
        

    Instance = (LSTEM_InstanceData_t *)Handle;

    /* check handle */
    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    LNB_IOHandle = Instance->IOHandle;

    /* if STTUNER_IOARCH_MAX_HANDLES then passthrough function required using our device handle (address) */ 
    if (IOHandle == STTUNER_IOARCH_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("%s write passthru\n", identity));
#endif
        Error = STTUNER_IOARCH_ReadWrite(LNB_IOHandle, Operation, SubAddr, Data, TransferSize, Timeout);
    }
    
    else    /* repeater */
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
        STTBX_Print(("%s write repeater\n", identity));
#endif
        

#ifdef STTUNER_DRV_SAT_DUAL_STEM_299
/*********I2C READ WRITE for demod is done here******************/
/*****If more number of demod added YOU have to add up here *****/
        /** The variable PCF8574RegisterValue is a global variable which captures the present data of LNB IoarchReadWrite()**/
                             
        /* New function STTUNER_IOARCH_DirectToI2C()for dual stem 
           added to enable the scanning algorithm work properly when demod
           is in repeater mode , instead of STTUNER_IOARCH_ReadWriteNoRep().
           Here in this function first lnb I2C write is done make the demod switch
           in right direction and demod I2C readwrite is done */
          
        while(1)
        {
            if(AccessFlag==0)
            {
                STTUNER_task_lock();
                AccessFlag=1;
                STTUNER_task_unlock();

                if(IOHandle == DemodHandleOne)
                    PCF8574RegisterValue = PCF8574RegisterValue & 0xFE; /* clear bit 0 - not FE1 */
                else if(IOHandle == DemodHandleTwo)
                    PCF8574RegisterValue = PCF8574RegisterValue | 0x1; /* set bit 0 - not FE1 */

                /* make I2c write for lnb to make demod switch point to right demod.
                   Here Subaddr=0, transfersize=1 and timeout=20 */
                Error = STTUNER_IOARCH_DirectToI2C(LNB_IOHandle, STTUNER_IO_WRITE, 0, &PCF8574RegisterValue, 1, IOREG_DEFAULT_TIMEOUT);
                STTUNER_TaskDelay(10);
                Error = STTUNER_IOARCH_DirectToI2C(IOHandle, Operation, SubAddr, Data, TransferSize, IOREG_DEFAULT_TIMEOUT);
                
                STTUNER_task_lock();
                AccessFlag=0;
                STTUNER_task_unlock();
                break;
            }
            else
            {
             	STTUNER_TaskDelay(10);
                continue;
            }
        }/* end of while*/
     
      
/****************************************************************/
#else
        /*****This function is used for single Tuner or Dual tuner with distinct address for demods*/    
        Error = STTUNER_IOARCH_ReadWriteNoRep(IOHandle, Operation, 0, Data, TransferSize, Timeout);
#endif
    }

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LSTEM
    STTBX_Print(("%s called\n", identity));
#endif
    
    return(Error);
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */


void LSTEM_UpdateLNB(IOARCH_Handle_t LNBHandle)
{
    STTBX_Print(("Update lnb "));
    if(LNBHandle==LnbHandleOne)
    {
        STTBX_Print(("1 = %02x", PCF8574RegisterValue));
        PCF8574RegisterValue = PCF8574RegisterValue & ~(0x1<<FSTEM_FE_SELECT); /* clear bit */
        STTBX_Print((" -> %02x\n", PCF8574RegisterValue));
    }
    else if(LNBHandle==LnbHandleTwo)
    {
        STTBX_Print(("2 = %02x", PCF8574RegisterValue));
        PCF8574RegisterValue = PCF8574RegisterValue | (0x1<<FSTEM_FE_SELECT); /* set bit */
        STTBX_Print((" -> %02x\n", PCF8574RegisterValue));
    }

    (void) STTUNER_IOARCH_DirectToI2C(LNBHandle, STTUNER_IO_WRITE, 0, &PCF8574RegisterValue, 1, 20);
    STTUNER_TaskDelay(10);
}


/*----------------------------------------------------
 FUNCTION      LSTEM_SetLnb
 ACTION        set the Lnb tone state (band selection: hi/lo band)
 PARAMS IN     Lnb -> true for High band, false for Low band.
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
void LSTEM_SetLnb(IOARCH_Handle_t IOHandle, int Lnb)
{
    /* Low band -> no 22KHz tone */
    /* High band -> 22KHz tone */

    U8 Bit;
    if(IOHandle==LnbHandleOne)
        Bit = 0x1 << FSTEM_FE1_TONE;
    else if(IOHandle==LnbHandleTwo)
        Bit = 0x1 << FSTEM_FE2_TONE;

    if ( Lnb == 1 )
        PCF8574RegisterValue = PCF8574RegisterValue | Bit; /* set bit */
    else if ( Lnb == 0 )
        PCF8574RegisterValue = PCF8574RegisterValue & ~(Bit); /* clear bit */
}

/*----------------------------------------------------
 FUNCTION      LSTEM_SetPolarization
 ACTION        set the polarization
 PARAMS IN     Polarization -> Polarization
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
void LSTEM_SetPolarization(IOARCH_Handle_t IOHandle, LNB_Polarization_t Polarization)
{
    U8 Bit;
    if(IOHandle==LnbHandleOne)
        Bit = 0x1 << FSTEM_FE1_VOLTAGE;
    else if(IOHandle==LnbHandleTwo)
        Bit = 0x1 << FSTEM_FE2_VOLTAGE;

    if ( Polarization == HORIZONTAL )
        PCF8574RegisterValue = PCF8574RegisterValue | Bit; /* set bit */
    else if ( Polarization == VERTICAL )
        PCF8574RegisterValue = PCF8574RegisterValue & ~(Bit); /* clear bit */
}

/*----------------------------------------------------
 FUNCTION      LSTEM_GetPower
 ACTION        get lnb power status
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
ST_ErrorCode_t LSTEM_GetPower(IOARCH_Handle_t IOHandle, LNB_Status_t *Status)
{
    U8 Bit;
    if(IOHandle==LnbHandleOne)
        Bit = 0x1 << FSTEM_FE1_LNB;
    else if(IOHandle==LnbHandleTwo)
        Bit = 0x1 << FSTEM_FE2_LNB;

    if (( PCF8574RegisterValue & Bit ) == 1 )
        *Status = LNB_STATUS_ON;
    else
        *Status = LNB_STATUS_OFF;
    
    return( ST_NO_ERROR );
}

/*----------------------------------------------------
 FUNCTION     LSTEM_SetPower
 ACTION        set lnb power
 PARAMS IN     
 PARAMS OUT    
 RETURN        
------------------------------------------------------*/
ST_ErrorCode_t LSTEM_SetPower(IOARCH_Handle_t IOHandle, LNB_Status_t Status)
{
    U8 Bit;
    if(IOHandle==LnbHandleOne)
        Bit = 0x1 << FSTEM_FE1_LNB;
    else if(IOHandle==LnbHandleTwo)
        Bit = 0x1 << FSTEM_FE2_LNB;

    if ( Status == LNB_STATUS_ON )
        PCF8574RegisterValue = PCF8574RegisterValue | Bit; /* set bit */
    else if ( Status == LNB_STATUS_OFF )
        PCF8574RegisterValue = PCF8574RegisterValue & ~(Bit); /* clear bit */

    return(ST_NO_ERROR);
}

/* End of l0299.c */
