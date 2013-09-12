/* ----------------------------------------------------------------------------
File Name: lnbBEIO.c
Description:
This file is to control LNB IC through GPIOs of backend or CPU.
Copyright (C) 2005-2006 STMicroelectronics
History:
date: 30-March-2006
version: 0.1.0
author: SD
Reference:
---------------------------------------------------------------------------- */
#ifndef ST_OSLINUX
   #include <string.h>
#endif
#include "stlite.h"     /* Standard includes */
#include "sttbx.h"
/* STAPI */
#include "stcommon.h"   /* for ST_GetClocksPerSecond() */
#include "stevt.h"
#include "lnbBEIO.h"      /* header for this file */
#include "sttuner.h"
#include "util.h"  
#ifndef STTUNER_MINIDRIVER
/* local to stlnb */
/* generic utility functions for stlnb */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "ioarch.h"     /* I/O for this driver */
#include "ioreg.h"      /* I/O register mapping */
#include "sdrv.h"       /* utilities */
#include "sioctl.h"     /* data structure typedefs for all the the sat ioctl functions */
#include "chip.h"
#endif
#ifdef STTUNER_MINIDRIVER
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "iodirect.h"
#ifdef STTUNER_DRV_SAT_STV0299
	#include "reg0299.h"
	#elif defined(STTUNER_DRV_SAT_STV0399E)
	#include "init399E.h"
	#endif
#endif

#ifndef STTUNER_MINIDRIVER
/* private variables ------------------------------------------------------- */

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */

static BOOL        Installed = FALSE;
/* ---------- per instance of driver ---------- */
typedef struct
{
    ST_DeviceName_t           *DeviceName;  
    STTUNER_Handle_t          TopLevelHandle;   /* access tuner, backend etc. using this */
    LNB_Config_t              Config;           /* LNB config for each instance        */
    STPIO_Handle_t            LNBVSEL_PIOHandle,LNBVEN_PIOHandle,LNBTEN_PIOHandle;
    STTUNER_LNB_Via_PIO_t     *LnbIOPort;
    ST_Partition_t            *MemoryPartition;     /* which partition this data block belongs to */
    void                      *InstanceChainPrev;   /* previous data block in chain or NULL if not */
    void                      *InstanceChainNext;   /* next data block in chain or NULL if last */
} 
LNB_BackEndIO_InstanceData_t;
#endif

#ifdef STTUNER_MINIDRIVER

typedef struct
{
    LNB_Config_t              Config;               /* LNB config for each instance        */
    STTUNER_Handle_t          TopLevelHandle;
    ST_Partition_t            *MemoryPartition;     /* which partition this data block belongs to */
    STTUNER_LNB_Via_PIO_t     LnbIOPort;
} LNB_BackEndIO_InstanceData_t;

#endif
/* instance chain, the default boot value is invalid, to catch errors */

/************extern from open.c for SatCR loopthrough mode application ************/
#ifdef STTUNER_DRV_SAT_SCR
#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
extern U32 LnbDrvHandleOne;
#endif
#endif

#ifndef STTUNER_MINIDRIVER
static LNB_BackEndIO_InstanceData_t *InstanceChainTop = (LNB_BackEndIO_InstanceData_t *)0x7fffffff;
#endif

#ifdef STTUNER_MINIDRIVER
LNB_BackEndIO_InstanceData_t *LNBInstance;
#endif

/* functions --------------------------------------------------------------- */


#ifndef STTUNER_MINIDRIVER
/* I/O API */
ST_ErrorCode_t lnb_backendIO_ioaccess(LNB_BackEndIO_InstanceData_t *Instance,
    STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);

/* access device specific low-level functions */
ST_ErrorCode_t lnb_backendIO_ioctl(LNB_BackEndIO_InstanceData_t *Instance, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status);
ST_ErrorCode_t lnb_backendIO_overloadcheck(LNB_BackEndIO_InstanceData_t *Instance, BOOL  *IsOverTemp, BOOL *IsCurrentOvrLoad);

ST_ErrorCode_t lnb_backendIO_setttxmode(LNB_BackEndIO_InstanceData_t *Instance, STTUNER_LnbTTxMode_t Ttxmode);

#endif

/* local functions */
#ifndef STTUNER_MINIDRIVER
void           LNB_BackEndIO_SetLnb         (LNB_BackEndIO_InstanceData_t *Instance, int Lnb);
void           LNB_BackEndIO_SetPolarization(LNB_BackEndIO_InstanceData_t *Instance, LNB_Polarization_t Polarization);
ST_ErrorCode_t LNB_BackEndIO_SetPower       (LNB_BackEndIO_InstanceData_t *Instance, LNB_Status_t  Status);
ST_ErrorCode_t LNB_BackEndIO_GetPower       (LNB_BackEndIO_InstanceData_t *Instance, LNB_Status_t *Status);
#endif


#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_LNB_STV0299_Install()

Description:
    install a satellite device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_LNB_BackEndIO_Install(STTUNER_lnb_dbase_t *Lnb)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == TRUE)
    {
        return(STTUNER_ERROR_INITSTATE);
    }
    /* mark ID in database */
    Lnb->ID = STTUNER_LNB_BACKENDGPIO;

    /* map API */
    Lnb->lnb_Init  = lnb_backendIO_Init;
    Lnb->lnb_Term  = lnb_backendIO_Term;
    Lnb->lnb_Open  = lnb_backendIO_Open;
    Lnb->lnb_Close = lnb_backendIO_Close;

    Lnb->lnb_GetConfig = lnb_backendIO_GetConfig;
    Lnb->lnb_SetConfig = lnb_backendIO_SetConfig;

    /*Lnb->lnb_ioaccess = lnb_backendIO_ioaccess;
    Lnb->lnb_ioctl    = lnb_backendIO_ioctl;
    Lnb->lnb_overloadcheck    = lnb_backendIO_overloadcheck;
    Lnb->lnb_setttxmode       = lnb_backendIO_setttxmode;*/
    InstanceChainTop = NULL;

    Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);
  

    Installed = TRUE;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_LNB_STV0299_UnInstall()

Description:
    install a satellite device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_LNB_BackEndIO_UnInstall(STTUNER_lnb_dbase_t *Lnb)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
	return(STTUNER_ERROR_INITSTATE);
    }
    if(Lnb->ID != STTUNER_LNB_BACKENDGPIO)
    {
	return(STTUNER_ERROR_ID);
    }
    /* has all memory been freed, by Term() */
    if(InstanceChainTop != NULL)
    {
    	return(ST_ERROR_OPEN_HANDLE);
    }
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
    Lnb->lnb_overloadcheck    = NULL;
    Lnb->lnb_setttxmode       = NULL;
    
   
        STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);
       
    InstanceChainTop = (LNB_BackEndIO_InstanceData_t *)0x7ffffffe;
    Installed        = FALSE;
    return(Error);
}

#endif
/* ----------------------------------------------------------------------------
Name: lnb_backendIO_Init()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_backendIO_Init(ST_DeviceName_t *DeviceName,LNB_InitParams_t *InitParams)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    #ifndef STTUNER_MINIDRIVER
    LNB_BackEndIO_InstanceData_t *InstanceNew, *Instance;
    if(Installed == FALSE)
    {
    	return(STTUNER_ERROR_INITSTATE);
    }
    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);
    /* ---------- check partition ---------- */
    Error = STTUNER_Util_CheckPtrNull(InitParams->MemoryPartition);
    if( Error != ST_NO_ERROR)
    {
	SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( LNB_BackEndIO_InstanceData_t ));
    if (InstanceNew == NULL)
    {
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
    InstanceNew->MemoryPartition     = InitParams->MemoryPartition;
    InstanceNew->LnbIOPort = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( STTUNER_LNB_Via_PIO_t ));
    strcpy(InstanceNew->LnbIOPort->PIODeviceName,InitParams->LnbIOPort->PIODeviceName);
    InstanceNew->LnbIOPort->VSEL_PIOPort           = InitParams->LnbIOPort->VSEL_PIOPort;
    InstanceNew->LnbIOPort->VSEL_PIOPin           = InitParams->LnbIOPort->VSEL_PIOPin;
    InstanceNew->LnbIOPort->VSEL_PIOBit           = InitParams->LnbIOPort->VSEL_PIOBit;
    InstanceNew->LnbIOPort->VEN_PIOPort           = InitParams->LnbIOPort->VEN_PIOPort;
    InstanceNew->LnbIOPort->VEN_PIOPin           = InitParams->LnbIOPort->VEN_PIOPin;
    InstanceNew->LnbIOPort->VEN_PIOBit           = InitParams->LnbIOPort->VEN_PIOBit;
    InstanceNew->LnbIOPort->TEN_PIOPort           = InitParams->LnbIOPort->TEN_PIOPort;
    InstanceNew->LnbIOPort->TEN_PIOPin           = InitParams->LnbIOPort->TEN_PIOPin;
    InstanceNew->LnbIOPort->TEN_PIOBit           = InitParams->LnbIOPort->TEN_PIOBit;
    
    InstanceNew->InstanceChainNext   = NULL; /* always last in the chain */
    
    SEM_UNLOCK(Lock_InitTermOpenClose);
#endif
#ifdef STTUNER_MINIDRIVER
    LNBInstance = memory_allocate_clear(InitParams->MemoryPartition, 1, sizeof( LNB_BackEndIO_InstanceData_t ));
    LNBInstance->MemoryPartition = InitParams->MemoryPartition;
#endif
    return(Error);
}
/* ----------------------------------------------------------------------------
Name: lnb_backendIO_Term()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_backendIO_Term(ST_DeviceName_t *DeviceName,LNB_TermParams_t *TermParams)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    #ifndef STTUNER_MINIDRIVER
    LNB_BackEndIO_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
	return(STTUNER_ERROR_INITSTATE);
    }
    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);
    /* ---------- check params ---------- */
    Error = STTUNER_Util_CheckPtrNull(TermParams);
    if( Error != ST_NO_ERROR)
    {
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }
    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }
    Instance = InstanceChainTop;
    while(1)
    {
	if ( strcmp((char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
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
	    STOS_MemoryDeallocate(Instance->MemoryPartition, Instance->LnbIOPort);
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL)
        {       /* error we should have found a matching name before the end of the list */
   		SEM_UNLOCK(Lock_InitTermOpenClose);
                return(STTUNER_ERROR_INITSTATE);
        }
        else
        {
            Instance = Instance->InstanceChainNext;   /* next block */
        }

    }


    SEM_UNLOCK(Lock_InitTermOpenClose);
    #endif
    
    #ifdef STTUNER_MINIDRIVER
    memory_deallocate(LNBInstance->MemoryPartition, LNBInstance);
    Error = ST_NO_ERROR;
    #endif
    return(Error);
}

/* ----------------------------------------------------------------------------
Name: lnb_backendIO_Open()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_backendIO_Open (ST_DeviceName_t *DeviceName, LNB_OpenParams_t *OpenParams, LNB_Capability_t *Capability, LNB_Handle_t *Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    #ifndef STTUNER_MINIDRIVER
    LNB_BackEndIO_InstanceData_t     *Instance;
    STTUNER_InstanceDbase_t          *Inst;
    STPIO_OpenParams_t               PIO_OpenParams;

/*
	static ST_DeviceName_t PIODeviceName[] =
    {
	    "STPIO[0]",
	    "STPIO[1]",
	    "STPIO[2]",
	    "STPIO[3]",
	    "STPIO[4]",
	    "STPIO[5]",
	    "STPIO[6]",
	    "STPIO[7]" 
    };
   


	static ST_DeviceName_t PIODeviceName[] =
    {
	    "PIO0",
	     "PIO1",
	     "PIO2",
	     "PIO3",
	     "PIO4",
	     "PIO5",
	     "PIO6",
	     "PIO7" 
    };
*/  
    if(Installed == FALSE)
    {
	return(STTUNER_ERROR_INITSTATE);
    }
    Inst = STTUNER_GetDrvInst();
    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);
    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
	SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }
    /* find  handle from name */
    Instance = InstanceChainTop;

    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL)
        {
	    SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */

    }
    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }
    /* this LNB driver is only for use with an STV0299 backend */
    Inst = STTUNER_GetDrvInst();    /* pointer to instance database */
    /* pointer to **OPENED** backend for this instance */
    
    /* Set LNB capabilties */
    Capability->ShortCircuitDetect = FALSE;  /* lie (rewrite to get pwr status using stv0299 i/o pin etc.) */
    Capability->PowerAvailable     = TRUE;
    Capability->PolarizationSelect = STTUNER_PLR_ALL;
    
    /* Set latest configuration */
    Instance->Config.Status        = LNB_STATUS_OFF;
    Instance->Config.Polarization  = STTUNER_LNB_OFF;
    Instance->Config.ToneState     = STTUNER_LNB_TONE_OFF;
    
    PIO_OpenParams.ReservedBits    = Instance->LnbIOPort->VSEL_PIOPin;
    PIO_OpenParams.BitConfigure[0] = STPIO_BIT_NOT_SPECIFIED;
    PIO_OpenParams.BitConfigure[1] = STPIO_BIT_NOT_SPECIFIED;
    PIO_OpenParams.BitConfigure[2] = STPIO_BIT_NOT_SPECIFIED;
    PIO_OpenParams.BitConfigure[3] = STPIO_BIT_NOT_SPECIFIED;
    PIO_OpenParams.BitConfigure[4] = STPIO_BIT_NOT_SPECIFIED;
    PIO_OpenParams.BitConfigure[5] = STPIO_BIT_NOT_SPECIFIED;
    PIO_OpenParams.BitConfigure[6] = STPIO_BIT_NOT_SPECIFIED;
    PIO_OpenParams.BitConfigure[7] = STPIO_BIT_NOT_SPECIFIED;

    PIO_OpenParams.BitConfigure[Instance->LnbIOPort->VSEL_PIOBit] = STPIO_BIT_OUTPUT;
    PIO_OpenParams.IntHandler   = NULL;

    /*Error = STPIO_Open(PIODeviceName[Instance->LnbIOPort->VSEL_PIOPort],&PIO_OpenParams,
                                          &(Instance->LNBVSEL_PIOHandle));*/
                           
    Error = STPIO_Open(Instance->LnbIOPort->PIODeviceName,&PIO_OpenParams,
                                          &(Instance->LNBVSEL_PIOHandle));
    
    PIO_OpenParams.ReservedBits    = Instance->LnbIOPort->VEN_PIOPin;
    PIO_OpenParams.BitConfigure[Instance->LnbIOPort->VEN_PIOBit] = STPIO_BIT_OUTPUT;
    PIO_OpenParams.IntHandler   = NULL;
    Error |= STPIO_Open(Instance->LnbIOPort->PIODeviceName,&PIO_OpenParams,
                                          &(Instance->LNBVEN_PIOHandle));
    /*Error |= STPIO_Open(PIODeviceName[Instance->LnbIOPort->VEN_PIOPort],&PIO_OpenParams,
                                          &(Instance->LNBVEN_PIOHandle));*/
    
    PIO_OpenParams.ReservedBits    = Instance->LnbIOPort->TEN_PIOPin;
    PIO_OpenParams.BitConfigure[Instance->LnbIOPort->TEN_PIOBit] = STPIO_BIT_OUTPUT;
    PIO_OpenParams.IntHandler   = NULL;
    Error |= STPIO_Open(Instance->LnbIOPort->PIODeviceName,&PIO_OpenParams,
                                          &(Instance->LNBTEN_PIOHandle));
    /*Error |= STPIO_Open(PIODeviceName[Instance->LnbIOPort->TEN_PIOPort],&PIO_OpenParams,
                                          &(Instance->LNBTEN_PIOHandle));*/
    
    /* Make LNB Power OFF during STTUNER_Open()*/    
    Error = STPIO_Clear(Instance->LNBVEN_PIOHandle, Instance->LnbIOPort->VEN_PIOPin);
    /* wait 500uS (0.5mS) then get power status */
    UTIL_Delay(500);
   
    Instance->TopLevelHandle = OpenParams->TopLevelHandle;  /* mark as valid */
   
    *Handle = (U32)Instance;

    SEM_UNLOCK(Lock_InitTermOpenClose);
    #endif
    
    #ifdef STTUNER_MINIDRIVER
     /* Set latest configuration */
    LNBInstance->Config.Status    = LNB_STATUS_ON;
    LNBInstance->Config.ToneState = STTUNER_LNB_TONE_OFF;
    LNBInstance->Config.Polarization = STTUNER_PLR_HORIZONTAL;/* set default pol->Horizontal*/
    
    LNBInstance->Config.ToneSourceControl  = STTUNER_22KHz_TONE_BackEndOP0Pin;
    LNBInstance->Config.LNBPowerControl    = STTUNER_LNB_POWER_LNBPDefault;
    LNB_BackEndIO_SetLnb(0);/* LNB tone off */
    LNB_BackEndIO_SetPower(LNBInstance->Config.Status);/*LNB power on*/
    LNB_BackEndIO_SetPolarization(STTUNER_PLR_HORIZONTAL);
    *Handle = (U32)LNBInstance;
    #endif
    return(Error);
}
/* ----------------------------------------------------------------------------
Name: lnb_backendIO_Close()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_backendIO_Close(LNB_Handle_t Handle, LNB_CloseParams_t *CloseParams)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    #ifndef STTUNER_MINIDRIVER
    LNB_BackEndIO_InstanceData_t     *Instance;
    Instance = (LNB_BackEndIO_InstanceData_t *)Handle;

    if(Installed == FALSE)
    {
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }
    STPIO_Close(Instance->LNBTEN_PIOHandle);
    STPIO_Close(Instance->LNBVEN_PIOHandle);
    STPIO_Close(Instance->LNBVSEL_PIOHandle);
    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

    SEM_UNLOCK(Lock_InitTermOpenClose);
    #endif
    #ifdef STTUNER_MINIDRIVER
    LNBInstance->TopLevelHandle = STTUNER_MAX_HANDLES;
    #endif
    return(Error);
}

/* ----------------------------------------------------------------------------
Name: lnb_backendIO_overloadcheck()

Description:
    Dummy Function
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t lnb_backendIO_overloadcheck(LNB_BackEndIO_InstanceData_t *Instance, BOOL  *IsOverTemp, BOOL *IsCurrentOvrLoad)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    return(Error);
}

/* ----------------------------------------------------------------------------
Name: lnb_backendIO_setttxmode()

Description:
    Dummy Function
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t lnb_backendIO_setttxmode(LNB_BackEndIO_InstanceData_t *Instance, STTUNER_LnbTTxMode_t Ttxmode)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    return(Error);
}

/* ----------------------------------------------------------------------------
Name: lnb_backendIO_GetConfig()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_backendIO_GetConfig(LNB_Handle_t Handle, LNB_Config_t *Config)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    #ifndef STTUNER_MINIDRIVER
    LNB_BackEndIO_InstanceData_t     *Instance;

    Instance = (LNB_BackEndIO_InstanceData_t *)Handle;

    Config->Status       = Instance->Config.Status;
    Config->Polarization = Instance->Config.Polarization;
    Config->ToneState    = Instance->Config.ToneState;

    #endif
    
    #ifdef STTUNER_MINIDRIVER
	    Config->Status       = LNBInstance->Config.Status;
	    Config->Polarization = LNBInstance->Config.Polarization;
	    Config->ToneState    = LNBInstance->Config.ToneState;
    #endif
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: lnb_backendIO_SetConfig()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_backendIO_SetConfig(LNB_Handle_t Handle, LNB_Config_t *Config)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    #ifndef STTUNER_MINIDRIVER
    LNB_BackEndIO_InstanceData_t     *Instance;
    STTUNER_InstanceDbase_t  *Inst;
    Instance = (LNB_BackEndIO_InstanceData_t *)Handle;
    Inst = STTUNER_GetDrvInst(); 
    
    /* Select LNB band */
    
    if(Instance->Config.ToneState != Config->ToneState)
      {
	   switch (Config->ToneState)
        
	    {
			case STTUNER_LNB_TONE_OFF:      /* No tone */
			LNB_BackEndIO_SetLnb(Instance, 0);
			break;

			case STTUNER_LNB_TONE_22KHZ:    /* 22KHz tone */
			LNB_BackEndIO_SetLnb(Instance, 1);
			break;

			case STTUNER_LNB_TONE_DEFAULT:  /* no change */
			break;

			default:
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
				LNB_BackEndIO_SetPolarization(Instance, VERTICAL);
				break;

				case STTUNER_PLR_HORIZONTAL:    /* OP_2 controls -- 13V (low) */
				LNB_BackEndIO_SetPolarization(Instance, HORIZONTAL);
				break;
				
				case STTUNER_LNB_OFF:
				LNB_BackEndIO_SetPolarization(Instance, NOPOLARIZATION);
				break;

				default:
				
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
				LNB_BackEndIO_SetPower(Instance, Config->Status);
				break;
				default:
				
				return(ST_ERROR_BAD_PARAMETER);
			}
	       
	      
     Instance->Config.Status = Config->Status;  
    }

    UTIL_Delay(1000);

    #endif
    
#ifdef STTUNER_MINIDRIVER
   
    if(LNBInstance->Config.ToneState != Config->ToneState)
    {
	   switch (Config->ToneState)
           {
			case STTUNER_LNB_TONE_OFF:      /* No tone */
			LNB_BackEndIO_SetLnb(0);
			break;

			case STTUNER_LNB_TONE_22KHZ:    /* 22KHz tone */
			LNB_BackEndIO_SetLnb(1);
			break;

			case STTUNER_LNB_TONE_DEFAULT:  /* no change */
			break;

			default:

			return(ST_ERROR_BAD_PARAMETER);
        }
        
       
        
        LNBInstance->Config.ToneState = Config->ToneState;
    }


    /* Select polarization */
    if (LNBInstance->Config.Polarization != Config->Polarization)
    {
        	switch (Config->Polarization)
			{
				case STTUNER_PLR_VERTICAL:      /* OP_2 controls -- 18V (high) */
				LNB_BackEndIO_SetPolarization(VERTICAL);
				break;

				case STTUNER_PLR_HORIZONTAL:    /* OP_2 controls -- 13V (low) */
				LNB_BackEndIO_SetPolarization(HORIZONTAL);
				break;
				
				case STTUNER_LNB_OFF:  
				break;

				default:
				
				return(ST_ERROR_BAD_PARAMETER);
			}
          
       
      LNBInstance->Config.Polarization = Config->Polarization;
    }


    /* Select power status */
    if (LNBInstance->Config.Status != Config->Status)
    {
	switch (Config->Status)
	{
		case LNB_STATUS_ON:
		case LNB_STATUS_OFF:
		LNB_BackEndIO_SetPower(Config->Status);
		break;
		default:
		
		return(ST_ERROR_BAD_PARAMETER);
	}

     LNBInstance->Config.Status = Config->Status;  
    }
    UTIL_Delay(1000);
    #endif
    return(Error);
}

#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: lnb_backendIO_ioctl()

Description:
    access device specific low-level functions

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_backendIO_ioctl(LNB_BackEndIO_InstanceData_t     *Instance, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)

{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB_DEMODIO
   const char *identity = "STTUNER lnb_backendIO.c lnb_backendIO_ioctl()";
#endif
    ST_ErrorCode_t Error;
   
   
    return(Error);

}
#endif

#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: lnb_backendIO_ioaccess()

Description:
    we get called with the instance of
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_backendIO_ioaccess(LNB_BackEndIO_InstanceData_t     *Instance,STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)

{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    return(Error);
}
#endif

/*----------------------------------------------------
 FUNCTION      LNB_BackEndIO_SetLnb
 ACTION        set the Lnb
 PARAMS IN     Lnb -> true for LnbHigh, false for LnbLow
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
#ifndef STTUNER_MINIDRIVER
void LNB_BackEndIO_SetLnb(LNB_BackEndIO_InstanceData_t     *Instance, int Lnb)
{
    
#ifdef STTUNER_LNB_TONE_THRU_DEMOD_DISEQC_PIN
    STTUNER_demod_instance_t *DemodInstance;
    STTUNER_InstanceDbase_t  *Inst;
    Inst = STTUNER_GetDrvInst(); 
    DemodInstance = &Inst[Instance->TopLevelHandle].Sat.Demod;
    
    if(DemodInstance->Driver->ID == STTUNER_DEMOD_STX0288)
    {
	if (Lnb == 0)   ChipSetField(DemodInstance->IOHandle,  DemodInstance->TEN_FieldIndex, 0x1);
	/* High band -> 22KHz tone */
	if (Lnb == 1)   ChipSetField(DemodInstance->IOHandle,  DemodInstance->TEN_FieldIndex, 0x0);
    }
    else
    {
	if (Lnb == 0)   ChipSetField(DemodInstance->IOHandle,  DemodInstance->TEN_FieldIndex, 0x0);	
	/* High band -> 22KHz tone */
	if (Lnb == 1)   ChipSetField(DemodInstance->IOHandle,  DemodInstance->TEN_FieldIndex, 0x3);
    }
#else
    /* Low band -> no 22KHz tone */
    if (Lnb == 0)   STPIO_Clear(Instance->LNBTEN_PIOHandle, Instance->LnbIOPort->TEN_PIOPin);

    /* High band -> 22KHz tone */
    if (Lnb == 1)   STPIO_Set(Instance->LNBTEN_PIOHandle, Instance->LnbIOPort->TEN_PIOPin);
    
#endif
}
#endif

#ifndef STTUNER_MINIDRIVER

/*----------------------------------------------------
 FUNCTION      LNB_BackEndIO_SetPolarization
 ACTION        set the polarization
 PARAMS IN     Polarization -> Polarization
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
void LNB_BackEndIO_SetPolarization(LNB_BackEndIO_InstanceData_t     *Instance, LNB_Polarization_t Polarization)
{

    if(Polarization == VERTICAL)
    {
        STPIO_Set(Instance->LNBVEN_PIOHandle, Instance->LnbIOPort->VEN_PIOPin);
        STPIO_Clear(Instance->LNBVSEL_PIOHandle, Instance->LnbIOPort->VSEL_PIOPin);
    }
    else if(Polarization == NOPOLARIZATION)
    {
        STPIO_Clear(Instance->LNBVEN_PIOHandle, Instance->LnbIOPort->VEN_PIOPin);
    }
    else
    {
		STPIO_Set(Instance->LNBVEN_PIOHandle, Instance->LnbIOPort->VEN_PIOPin);
        STPIO_Set(Instance->LNBVSEL_PIOHandle, Instance->LnbIOPort->VSEL_PIOPin);
    }
}
#endif



#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION      LNB_BackEndIO_GetPower
 ACTION        get lnb power status
 PARAMS IN
 PARAMS OUT
 RETURN
------------------------------------------------------*/
ST_ErrorCode_t LNB_BackEndIO_GetPower(LNB_BackEndIO_InstanceData_t     *Instance, LNB_Status_t *Status)
{
   
    *Status = LNB_STATUS_ON;

    return(ST_NO_ERROR);
}
#endif

#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION      LNB_BackEndIO_SetPower
 ACTION        set lnb power
 PARAMS IN
 PARAMS OUT
 RETURN
------------------------------------------------------*/
ST_ErrorCode_t LNB_BackEndIO_SetPower(LNB_BackEndIO_InstanceData_t     *Instance, LNB_Status_t Status)
{
    switch(Status)
    {
        case LNB_STATUS_ON:
        STPIO_Set(Instance->LNBVEN_PIOHandle, Instance->LnbIOPort->VEN_PIOPin);
        break;
        case LNB_STATUS_OFF:
        STPIO_Clear(Instance->LNBVEN_PIOHandle, Instance->LnbIOPort->VEN_PIOPin);
        break;
        default:
        return(ST_ERROR_BAD_PARAMETER);
    }
    return(ST_NO_ERROR);
}
#endif



/* End of lnb_backendIO.c */
