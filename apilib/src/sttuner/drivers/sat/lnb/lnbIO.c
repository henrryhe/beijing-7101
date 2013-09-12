/* ----------------------------------------------------------------------------
File Name: lnbIO.c
Description:
This file is to control LNB IC through GPIOs of demod .
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
#include "lnbIO.h"      /* header for this file */
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
    STTUNER_Handle_t          TopLevelHandle;   /* access tuner, demod etc. using this */
    LNB_Config_t              Config;           /* LNB config for each instance        */
    ST_Partition_t            *MemoryPartition;     /* which partition this data block belongs to */
    void                      *InstanceChainPrev;   /* previous data block in chain or NULL if not */
    void                      *InstanceChainNext;   /* next data block in chain or NULL if last */
    
    #ifdef STTUNER_DRV_SAT_SCR
    BOOL DISECQ_ST_ENABLE;
    #endif
    
} LNB_DemodIO_InstanceData_t;
#endif

#ifdef STTUNER_MINIDRIVER

typedef struct
{
    LNB_Config_t              Config;               /* LNB config for each instance        */
    STTUNER_Handle_t          TopLevelHandle;
    ST_Partition_t            *MemoryPartition;     /* which partition this data block belongs to */
} LNB_DemodIO_InstanceData_t;

#endif
/* instance chain, the default boot value is invalid, to catch errors */

/************extern from open.c for SatCR loopthrough mode application ************/
#ifdef STTUNER_DRV_SAT_SCR
#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
extern U32 LnbDrvHandleOne;
#endif
#endif
#ifndef STTUNER_MINIDRIVER
static LNB_DemodIO_InstanceData_t *InstanceChainTop = (LNB_DemodIO_InstanceData_t *)0x7fffffff;
#endif

#ifdef STTUNER_MINIDRIVER
LNB_DemodIO_InstanceData_t *LNBInstance;
#endif

/* functions --------------------------------------------------------------- */


#ifndef STTUNER_MINIDRIVER
/* I/O API */
ST_ErrorCode_t lnb_lnb_demodIO_ioaccess(LNB_Handle_t Handle, IOARCH_Handle_t IOHandle,
    STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);

/* access device specific low-level functions */
ST_ErrorCode_t lnb_lnb_demodIO_ioctl(LNB_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status);
ST_ErrorCode_t lnb_lnb_demodIO_overloadcheck(LNB_Handle_t Handle, BOOL  *IsOverTemp, BOOL *IsCurrentOvrLoad);

ST_ErrorCode_t lnb_lnb_demodIO_setttxmode(LNB_Handle_t Handle, STTUNER_LnbTTxMode_t Ttxmode);

#endif

/* local functions */
#ifndef STTUNER_MINIDRIVER
void           LNB_DemodIO_SetLnb         (STTUNER_demod_instance_t *DemodInstance, int Lnb);
void           LNB_DemodIO_SetTone_Demod  (STTUNER_demod_instance_t *DemodInstance, int Lnb);
void           LNB_DemodIO_SetPolarization(STTUNER_demod_instance_t *DemodInstance, LNB_Polarization_t Polarization);
void           LNB_DemodIO_SetDACPolarization(STTUNER_demod_instance_t *DemodInstance, LNB_Polarization_t Polarization);
ST_ErrorCode_t LNB_DemodIO_SetPower       (STTUNER_demod_instance_t *DemodInstance, LNB_Status_t  Status);
ST_ErrorCode_t LNB_DemodIO_Set_DACPower   (STTUNER_demod_instance_t *DemodInstance, LNB_Status_t  Status);
ST_ErrorCode_t LNB_DemodIO_GetPower       (STTUNER_demod_instance_t *DemodInstance, LNB_Status_t *Status);
#endif

#ifdef STTUNER_MINIDRIVER
void           LNB_DemodIO_SetLnb(int Lnb);
void           LNB_DemodIO_SetPolarization(LNB_Polarization_t Polarization);
ST_ErrorCode_t LNB_DemodIO_SetPower(LNB_Status_t  Status);
#ifdef STTUNER_DRV_SAT_STV0299
#define VEN_REG_INDEX      R0299_IOCFG
#define VEN_FIELD_INDEX    F0299_OP1VALUE
#define VEN_FIELD_INDEX_L  F0299_OP1VALUE_L
#ifdef STTUNER_LNB_TONE_THRU_DEMOD_DISEQC_PIN
	#define TEN_REG_INDEX      R0299_DISEQC
	#define TEN_FIELD_INDEX    F0299_DISEQCMODE
	#define TEN_FIELD_INDEX_L  F0299_DISEQCMODE_L
#else
	#define TEN_REG_INDEX      R0299_IOCFG
	#define TEN_FIELD_INDEX    F0299_OP0VALUE
	#define TEN_FIELD_INDEX_L  F0299_OP0VALUE_L
#endif
#define VSEL_REG_INDEX     R0299_DISEQC
#define VSEL_FIELD_INDEX   F0299_LOCKOUTPUT
#define VSEL_FIELD_INDEX_L F0299_LOCKOUTPUT_L
#elif defined(STTUNER_DRV_SAT_STV0399E)
#define VEN_REG_INDEX      R399_IOCFG2
#define VEN_FIELD_INDEX    F399_OP1_1
#define VEN_FIELD_INDEX_L  F399_OP1_1_L
#define TEN_REG_INDEX      R399_IOCFG2
#define TEN_FIELD_INDEX    F399_OP0_1
#define TEN_FIELD_INDEX_L  F399_OP0_1_L
#define VSEL_REG_INDEX     R399_IOCFG1
#define VSEL_FIELD_INDEX   F399_LOCK_CONF
#define VSEL_FIELD_INDEX_L F399_LOCK_CONF_L
#endif
#endif


#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_LNB_STV0299_Install()

Description:
    install a satellite device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_LNB_DemodIO_Install(STTUNER_lnb_dbase_t *Lnb)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == TRUE)
    {
        return(STTUNER_ERROR_INITSTATE);
    }
    /* mark ID in database */
    Lnb->ID = STTUNER_LNB_DEMODIO;

    /* map API */
    Lnb->lnb_Init  = lnb_lnb_demodIO_Init;
    Lnb->lnb_Term  = lnb_lnb_demodIO_Term;
    Lnb->lnb_Open  = lnb_lnb_demodIO_Open;
    Lnb->lnb_Close = lnb_lnb_demodIO_Close;

    Lnb->lnb_GetConfig = lnb_lnb_demodIO_GetConfig;
    Lnb->lnb_SetConfig = lnb_lnb_demodIO_SetConfig;

    Lnb->lnb_ioaccess = lnb_lnb_demodIO_ioaccess;
    Lnb->lnb_ioctl    = lnb_lnb_demodIO_ioctl;
    Lnb->lnb_overloadcheck    = lnb_lnb_demodIO_overloadcheck;
    Lnb->lnb_setttxmode       = lnb_lnb_demodIO_setttxmode;
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
ST_ErrorCode_t STTUNER_DRV_LNB_DemodIO_UnInstall(STTUNER_lnb_dbase_t *Lnb)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
	return(STTUNER_ERROR_INITSTATE);
    }
    if(Lnb->ID != STTUNER_LNB_DEMODIO)
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
     
    InstanceChainTop = (LNB_DemodIO_InstanceData_t *)0x7ffffffe;
    Installed        = FALSE;
    return(Error);
}

#endif
/* ----------------------------------------------------------------------------
Name: lnb_lnb_demodIO_Init()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnb_demodIO_Init(ST_DeviceName_t *DeviceName,LNB_InitParams_t *InitParams)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    #ifndef STTUNER_MINIDRIVER
    LNB_DemodIO_InstanceData_t *InstanceNew, *Instance;
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

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( LNB_DemodIO_InstanceData_t ));
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
    InstanceNew->InstanceChainNext   = NULL; /* always last in the chain */
    
    SEM_UNLOCK(Lock_InitTermOpenClose);
#endif
#ifdef STTUNER_MINIDRIVER
    LNBInstance = memory_allocate_clear(InitParams->MemoryPartition, 1, sizeof( LNB_DemodIO_InstanceData_t ));
    LNBInstance->MemoryPartition = InitParams->MemoryPartition;
#endif
    return(Error);
}
/* ----------------------------------------------------------------------------
Name: lnb_lnb_demodIO_Term()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnb_demodIO_Term(ST_DeviceName_t *DeviceName,LNB_TermParams_t *TermParams)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    #ifndef STTUNER_MINIDRIVER
    LNB_DemodIO_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

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
Name: lnb_lnb_demodIO_Open()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnb_demodIO_Open (ST_DeviceName_t *DeviceName, LNB_OpenParams_t *OpenParams, LNB_Capability_t *Capability, LNB_Handle_t *Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    #ifndef STTUNER_MINIDRIVER
    LNB_DemodIO_InstanceData_t     *Instance;
    STTUNER_demod_instance_t *DemodInstance;
    STTUNER_InstanceDbase_t  *Inst;
    
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
    /* this LNB driver is only for use with an STV0299 demod */
    Inst = STTUNER_GetDrvInst();    /* pointer to instance database */
    /* pointer to **OPENED** demod for this instance */
    DemodInstance = &Inst[OpenParams->TopLevelHandle].Sat.Demod;
    
    /* Set LNB capabilties */
    Capability->ShortCircuitDetect = FALSE;  /* lie (rewrite to get pwr status using stv0299 i/o pin etc.) */
    Capability->PowerAvailable     = TRUE;
    Capability->PolarizationSelect = STTUNER_PLR_ALL;
    
    /* Set latest configuration */
    Instance->Config.Status    = LNB_STATUS_OFF;
    Instance->Config.Polarization = STTUNER_LNB_OFF;
    
    Instance->Config.ToneState = STTUNER_LNB_TONE_OFF;
    if((Inst[OpenParams->TopLevelHandle].Sat.Tuner.Driver->ID)==STTUNER_TUNER_DSF8910)
    {
      Instance->Config.LNBPowerControl  = STTUNER_LNB_POWER_DemodDACPin;
      Instance->Config.ToneSourceControl  = STTUNER_22KHz_TONE_DemodDiseqcPin;
      LNB_DemodIO_SetTone_Demod(DemodInstance, 0);  /*LNB Tone OFF*/
      LNB_DemodIO_Set_DACPower(DemodInstance, Instance->Config.Status);/*set LNB power 18V*/
    }
    else
    {
	    Instance->Config.ToneSourceControl  = STTUNER_22KHz_TONE_DemodOP0Pin;
	    Instance->Config.LNBPowerControl    = STTUNER_LNB_POWER_LNBPDefault;
	    LNB_DemodIO_SetLnb(DemodInstance, 0);/* LNB tone off */
	    LNB_DemodIO_SetPower(DemodInstance, Instance->Config.Status);/*LNB power on*/
    }
    /* wait 500uS (0.5mS) then get power status */
    UTIL_Delay(500);
    Error = LNB_DemodIO_GetPower(DemodInstance, &Instance->Config.Status);

    if (Error == ST_NO_ERROR)   /* last i/o (above) operation was good */
    {
        if (Instance->Config.Status == LNB_STATUS_SHORT_CIRCUIT)
        {
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_HWFAIL);
        }
    }
    else
    {
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }


    Instance->TopLevelHandle = OpenParams->TopLevelHandle;  /* mark as valid */
     /* In loopthrough mode 13 to 18V transition for DiSEQC-ST command should be on first demod */
    #ifdef STTUNER_DRV_SAT_SCR
		#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
		    if(OpenParams->TopLevelHandle == 0)
		    Instance->DISECQ_ST_ENABLE = TRUE;
		    if(OpenParams->TopLevelHandle == 1)
		    Instance->DISECQ_ST_ENABLE = FALSE;
    		#endif
    #endif
    *Handle = (U32)Instance;

    SEM_UNLOCK(Lock_InitTermOpenClose);
    #endif
    
    #ifdef STTUNER_MINIDRIVER
     /* Set latest configuration */
    LNBInstance->Config.Status    = LNB_STATUS_ON;
    LNBInstance->Config.ToneState = STTUNER_LNB_TONE_OFF;
    LNBInstance->Config.Polarization = STTUNER_PLR_HORIZONTAL;/* set default pol->Horizontal*/
    
    LNBInstance->Config.ToneSourceControl  = STTUNER_22KHz_TONE_DemodOP0Pin;
    LNBInstance->Config.LNBPowerControl    = STTUNER_LNB_POWER_LNBPDefault;
    LNB_DemodIO_SetLnb(0);/* LNB tone off */
    LNB_DemodIO_SetPower(LNBInstance->Config.Status);/*LNB power on*/
    LNB_DemodIO_SetPolarization(STTUNER_PLR_HORIZONTAL);
    *Handle = (U32)LNBInstance;
    #endif
    return(Error);
}
/* ----------------------------------------------------------------------------
Name: lnb_lnb_demodIO_Close()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnb_demodIO_Close(LNB_Handle_t Handle, LNB_CloseParams_t *CloseParams)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    #ifndef STTUNER_MINIDRIVER
    LNB_DemodIO_InstanceData_t     *Instance;
    Instance = (LNB_DemodIO_InstanceData_t *)Handle;

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

    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

    SEM_UNLOCK(Lock_InitTermOpenClose);
    #endif
    #ifdef STTUNER_MINIDRIVER
    LNBInstance->TopLevelHandle = STTUNER_MAX_HANDLES;
    #endif
    return(Error);
}

/* ----------------------------------------------------------------------------
Name: lnb_lnb_demodIO_overloadcheck()

Description:
    Dummy Function
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t lnb_lnb_demodIO_overloadcheck(LNB_Handle_t Handle, BOOL  *IsOverTemp, BOOL *IsCurrentOvrLoad)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    return(Error);
}

/* ----------------------------------------------------------------------------
Name: lnb_lnb_demodIO_setttxmode()

Description:
    Dummy Function
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t lnb_lnb_demodIO_setttxmode(LNB_Handle_t Handle, STTUNER_LnbTTxMode_t Ttxmode)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    return(Error);
}

/* ----------------------------------------------------------------------------
Name: lnb_lnb_demodIO_GetConfig()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnb_demodIO_GetConfig(LNB_Handle_t Handle, LNB_Config_t *Config)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    #ifndef STTUNER_MINIDRIVER
    LNB_DemodIO_InstanceData_t     *Instance;

    Instance = (LNB_DemodIO_InstanceData_t *)Handle;

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
Name: lnb_lnb_demodIO_SetConfig()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnb_demodIO_SetConfig(LNB_Handle_t Handle, LNB_Config_t *Config)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    #ifndef STTUNER_MINIDRIVER
    LNB_DemodIO_InstanceData_t     *Instance;
    STTUNER_demod_instance_t *DemodInstance;
    STTUNER_InstanceDbase_t  *Inst;
    Instance = (LNB_DemodIO_InstanceData_t *)Handle;
    Inst = STTUNER_GetDrvInst(); 
    DemodInstance = &Inst[Instance->TopLevelHandle].Sat.Demod;
    #ifdef STTUNER_DRV_SAT_SCR
		#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
		    if(Instance->DISECQ_ST_ENABLE == FALSE)
		    {
			    /*Error = FE_399_SetPower(Instance->FE_399_Handle, LNB_STATUS_OFF);*/  /* Make LNB power off of second NIM because of no use */
			    Handle = LnbDrvHandleOne;
			    Instance = (LNB_DemodIO_InstanceData_t *)Handle;
		    }
		#endif
	#endif
    /* Select LNB band */
    
    if(Instance->Config.ToneState != Config->ToneState)
      {
	    switch(Instance->Config.ToneSourceControl)
        {
	    case STTUNER_22KHz_TONE_DemodOP0Pin:
          
	    switch (Config->ToneState)
        
	    {
			case STTUNER_LNB_TONE_OFF:      /* No tone */
			LNB_DemodIO_SetLnb(DemodInstance, 0);
			break;

			case STTUNER_LNB_TONE_22KHZ:    /* 22KHz tone */
			LNB_DemodIO_SetLnb(DemodInstance, 1);
			break;

			case STTUNER_LNB_TONE_DEFAULT:  /* no change */
			break;

			default:
			return(ST_ERROR_BAD_PARAMETER);
        }
        break;
        
        case STTUNER_22KHz_TONE_DemodDiseqcPin:
        switch (Config->ToneState)
        
	    {
			case STTUNER_LNB_TONE_OFF:      /* No tone */
			LNB_DemodIO_SetTone_Demod(DemodInstance, 0);
			break;

			case STTUNER_LNB_TONE_22KHZ:    /* 22KHz tone */
			LNB_DemodIO_SetTone_Demod(DemodInstance, 1);
			break;

			case STTUNER_LNB_TONE_DEFAULT:  /* no change */
			break;

			default:
			return(ST_ERROR_BAD_PARAMETER);
        }
             
        break; 
        default:
        break;    

      }
        
        Instance->Config.ToneState = Config->ToneState;
    }


    /* Select polarization */
    if (Instance->Config.Polarization != Config->Polarization)
    {
        switch(Instance->Config.LNBPowerControl)
        {
	        case  STTUNER_LNB_POWER_LNBPDefault:
	        
	    	switch (Config->Polarization)
			{
				case STTUNER_PLR_VERTICAL:      /* OP_2 controls -- 18V (high) */
				LNB_DemodIO_SetPolarization(DemodInstance, VERTICAL);
				break;

				case STTUNER_PLR_HORIZONTAL:    /* OP_2 controls -- 13V (low) */
				LNB_DemodIO_SetPolarization(DemodInstance, HORIZONTAL);
				break;
				
				case STTUNER_LNB_OFF:
				LNB_DemodIO_SetPolarization(DemodInstance, NOPOLARIZATION);
				break;

				default:
				
				return(ST_ERROR_BAD_PARAMETER);
			}
          
         break;
       
      	case STTUNER_LNB_POWER_DemodDACPin:
      
	    switch (Config->Polarization)
	    {
			case STTUNER_PLR_VERTICAL:      /* set DAC for 13V */
			LNB_DemodIO_SetDACPolarization(DemodInstance, VERTICAL);
			break;

			case STTUNER_PLR_HORIZONTAL:    /* set DAC for 18V */
			LNB_DemodIO_SetDACPolarization(DemodInstance, HORIZONTAL);
			break;
			
			case STTUNER_LNB_OFF:
			LNB_DemodIO_SetDACPolarization(DemodInstance, NOPOLARIZATION);
			break;

			default:
			return(ST_ERROR_BAD_PARAMETER);
        }
       
       break;
       
       default:
       break;
      }
      Instance->Config.Polarization = Config->Polarization;
    }


    /* Select power status */
    if (Instance->Config.Status != Config->Status)
    {
	    switch(Instance->Config.LNBPowerControl)
        {
	        case  STTUNER_LNB_POWER_LNBPDefault:
	    
			switch (Config->Status)
			{
				case LNB_STATUS_ON:
				case LNB_STATUS_OFF:
				LNB_DemodIO_SetPower(DemodInstance, Config->Status);
				break;
				default:
				
				return(ST_ERROR_BAD_PARAMETER);
			}
	       
	       break;
       
          case STTUNER_LNB_POWER_DemodDACPin:
	      
          switch (Config->Status)
	      {
			case LNB_STATUS_ON:
			case LNB_STATUS_OFF:
			LNB_DemodIO_Set_DACPower(DemodInstance, Config->Status);
			break;

			default:
			return(ST_ERROR_BAD_PARAMETER);
          }
	       
	       break;
       
       default:
       break;
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
			LNB_DemodIO_SetLnb(0);
			break;

			case STTUNER_LNB_TONE_22KHZ:    /* 22KHz tone */
			LNB_DemodIO_SetLnb(1);
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
				LNB_DemodIO_SetPolarization(VERTICAL);
				break;

				case STTUNER_PLR_HORIZONTAL:    /* OP_2 controls -- 13V (low) */
				LNB_DemodIO_SetPolarization(HORIZONTAL);
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
		LNB_DemodIO_SetPower(Config->Status);
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
Name: lnb_lnb_demodIO_ioctl()

Description:
    access device specific low-level functions

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnb_demodIO_ioctl(LNB_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)

{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB_DEMODIO
   const char *identity = "STTUNER lnb_demodIO.c lnb_lnb_demodIO_ioctl()";
#endif
    ST_ErrorCode_t Error=0;
   
    LNB_DemodIO_InstanceData_t     *Instance;
    STTUNER_demod_instance_t *DemodInstance;
    STTUNER_InstanceDbase_t  *Inst;
    Instance = (LNB_DemodIO_InstanceData_t *)Handle;
    Inst = STTUNER_GetDrvInst(); 
    DemodInstance = &Inst[Instance->TopLevelHandle].Sat.Demod;

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNB_DEMODIO
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    switch(Function){

        case STTUNER_IOCTL_RAWIO: /* read/write device register (actual write to stv0299) */
            Error =  STTUNER_IOARCH_ReadWrite( DemodInstance->IOHandle,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->Operation,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->SubAddr,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->Data,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->TransferSize,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->Timeout );
            break;

        case STTUNER_IOCTL_REG_IN: /* read device register */
            *(int *)OutParams = STTUNER_IOREG_GetRegister(DemodInstance->DeviceMap, DemodInstance->IOHandle, *(int *)InParams);
            break;

        case STTUNER_IOCTL_REG_OUT: /* write device register */
            Error =  STTUNER_IOREG_SetRegister(  DemodInstance->DeviceMap,
                                                  DemodInstance->IOHandle,
                  ((SATIOCTL_SETREG_InParams_t *)InParams)->RegIndex,
                  ((SATIOCTL_SETREG_InParams_t *)InParams)->Value );
            break;

        default:

            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


    return(Error);

}
#endif

#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: lnb_lnb_demodIO_ioaccess()

Description:
    we get called with the instance of
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_lnb_demodIO_ioaccess(LNB_Handle_t Handle, IOARCH_Handle_t IOHandle,STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)

{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    return(Error);
}
#endif

/*----------------------------------------------------
 FUNCTION      LNB_DemodIO_SetLnb
 ACTION        set the Lnb
 PARAMS IN     Lnb -> true for LnbHigh, false for LnbLow
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
#ifndef STTUNER_MINIDRIVER
void LNB_DemodIO_SetLnb(STTUNER_demod_instance_t *DemodInstance, int Lnb)
{
	
#ifdef STTUNER_LNB_TONE_THRU_DEMOD_DISEQC_PIN
    if(DemodInstance->Driver->ID == STTUNER_DEMOD_STX0288)
    {
    	/* Low band -> no 22KHz tone */
	if (Lnb == 0)   STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->TEN_FieldIndex, 0x1);
	/* High band -> 22KHz tone */
	if (Lnb == 1)   STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->TEN_FieldIndex, 0x0);
    }
    else
    {
	if (Lnb == 0)   STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->TEN_FieldIndex, 0x0);	
	/* High band -> 22KHz tone */
	if (Lnb == 1)   STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->TEN_FieldIndex, 0x3);
    }
#else
    /* ST EVALNB_DemodIO board */
    STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->TEN_FieldIndex, Lnb);


#endif
}
#endif

#ifdef STTUNER_MINIDRIVER
void LNB_DemodIO_SetLnb(int Lnb)
{
U8 Data;	
#ifdef STTUNER_LNB_TONE_THRU_DEMOD_DISEQC_PIN
    /* FUTARQUE 299 board */
    /* Low band -> no 22KHz tone */
    if (Lnb == 0)
    {
    	Data = 0x0;
    	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, TEN_REG_INDEX, TEN_FIELD_INDEX, TEN_FIELD_INDEX_L, &Data, 1, FALSE);
    }
    /* High band -> 22KHz tone */
    if (Lnb == 1)
    {
    	Data = 0x3;
    	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, TEN_REG_INDEX, TEN_FIELD_INDEX, TEN_FIELD_INDEX_L, &Data, 1, FALSE);
    }

#else
    /* ST EVALNB_DemodIO board */
    Data = Lnb;
    STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, TEN_REG_INDEX, TEN_FIELD_INDEX, TEN_FIELD_INDEX_L, &Data, 1, FALSE);
   
#endif
}
#endif


#ifndef STTUNER_MINIDRIVER
void LNB_DemodIO_SetTone_Demod(STTUNER_demod_instance_t *DemodInstance, int Lnb)
{
    if(DemodInstance->Driver->ID == STTUNER_DEMOD_STX0288)
    {
	if (Lnb == 0)   STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->TEN_FieldIndex, 0x1);
	/* High band -> 22KHz tone */
	if (Lnb == 1)   STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->TEN_FieldIndex, 0x0);
    }
    else
    {
	if (Lnb == 0)   STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->TEN_FieldIndex, 0x0);	
	/* High band -> 22KHz tone */
	if (Lnb == 1)   STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->TEN_FieldIndex, 0x3);
    }
}


/*----------------------------------------------------
 FUNCTION      LNB_DemodIO_SetPolarization
 ACTION        set the polarization
 PARAMS IN     Polarization -> Polarization
 PARAMS OUT    NONE
 RETURN        NONE
------------------------------------------------------*/
void LNB_DemodIO_SetPolarization(STTUNER_demod_instance_t *DemodInstance, LNB_Polarization_t Polarization)
{

    if(Polarization == VERTICAL)
    {
        STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->VEN_FieldIndex, 1);
        STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->VSEL_FieldIndex, 0);
    }
    else if(Polarization == NOPOLARIZATION)
    {
        STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->VEN_FieldIndex, 0);
    }
    else
    {
	STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->VEN_FieldIndex, 1);
	STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->VSEL_FieldIndex, 1);
    }
}
#endif

#ifdef STTUNER_MINIDRIVER
void LNB_DemodIO_SetPolarization(LNB_Polarization_t Polarization)
{
	U8 Data;
    
	    if(Polarization == VERTICAL)
	    {
	    	Data = 1;
	        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, VEN_REG_INDEX, VEN_FIELD_INDEX, VEN_FIELD_INDEX_L, &Data, 1, FALSE);
	    	Data = 0;
	        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, VSEL_REG_INDEX, VSEL_FIELD_INDEX, VSEL_FIELD_INDEX_L, &Data, 1, FALSE);
	    }
	    else if(Polarization == HORIZONTAL)
	    {
	    	Data = 1;
	        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, VEN_REG_INDEX, VEN_FIELD_INDEX, VEN_FIELD_INDEX_L, &Data, 1, FALSE);
	    	Data = 1;
	        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, VSEL_REG_INDEX, VSEL_FIELD_INDEX, VSEL_FIELD_INDEX_L, &Data, 1, FALSE);
	    }
	    else
	    {
	    	Data = 0;
	        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, VEN_REG_INDEX, VEN_FIELD_INDEX, VEN_FIELD_INDEX_L, &Data, 1, FALSE);
	    }
}
#endif
#ifndef STTUNER_MINIDRIVER
void LNB_DemodIO_SetDACPolarization(STTUNER_demod_instance_t *DemodInstance, LNB_Polarization_t Polarization)
{
    if(Polarization == VERTICAL)/*set 13V*/
        {
	    STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->VEN_FieldIndex, 0xCE);
            STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->VSEL_FieldIndex, 0xA4);
        }
    if(Polarization == NOPOLARIZATION)/*LNB off*/
        {
            STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->VEN_FieldIndex, 0x00);
            STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->VSEL_FieldIndex, 0x02);
        }
    else  /*set 18V*/
        {
	    STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->VEN_FieldIndex, 0x40);
            STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->VSEL_FieldIndex, 0xA1);
        }
}
#endif

#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION      LNB_DemodIO_GetPower
 ACTION        get lnb power status
 PARAMS IN
 PARAMS OUT
 RETURN
------------------------------------------------------*/
ST_ErrorCode_t LNB_DemodIO_GetPower(STTUNER_demod_instance_t *DemodInstance, LNB_Status_t *Status)
{
    /* always report LNB is on (not s/c or off) */
    
    if ( STTUNER_IOREG_GetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->VEN_FieldIndex) == 1)
    *Status = LNB_STATUS_ON;
    else
    *Status = LNB_STATUS_OFF;

    return(ST_NO_ERROR);
}
#endif

#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION      LNB_DemodIO_SetPower
 ACTION        set lnb power
 PARAMS IN
 PARAMS OUT
 RETURN
------------------------------------------------------*/
ST_ErrorCode_t LNB_DemodIO_SetPower(STTUNER_demod_instance_t *DemodInstance, LNB_Status_t Status)
{
    switch(Status)
    {
        case LNB_STATUS_ON:
        STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->VEN_FieldIndex, 1);
        break;
        case LNB_STATUS_OFF:
        STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->VEN_FieldIndex, 0);
        break;
        default:
        return(ST_ERROR_BAD_PARAMETER);
    }
    return(ST_NO_ERROR);
}
#endif

#ifdef STTUNER_MINIDRIVER
/*----------------------------------------------------
 FUNCTION      LNB_DemodIO_SetPower
 ACTION        set lnb power
 PARAMS IN
 PARAMS OUT
 RETURN
------------------------------------------------------*/
ST_ErrorCode_t LNB_DemodIO_SetPower(LNB_Status_t Status)
{
	
    U8 Data;
    switch (Status)
    {
        case LNB_STATUS_ON:
        Data = 1;
        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, VEN_REG_INDEX, VEN_FIELD_INDEX, VEN_FIELD_INDEX_L, &Data, 1, FALSE);
        break;

        case LNB_STATUS_OFF:
        Data = 0;
        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, VEN_REG_INDEX, VEN_FIELD_INDEX, VEN_FIELD_INDEX_L, &Data, 1, FALSE);
        break;

        default:
        return(ST_ERROR_BAD_PARAMETER);
    }

    return(ST_NO_ERROR);
}
#endif

#ifndef STTUNER_MINIDRIVER
ST_ErrorCode_t LNB_DemodIO_Set_DACPower(STTUNER_demod_instance_t *DemodInstance, LNB_Status_t Status)
{
    switch (Status)
    {
        case LNB_STATUS_ON:
        STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->VEN_FieldIndex, 0x40);
        STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->VSEL_FieldIndex, 0xA1);
        break;

        case LNB_STATUS_OFF:
        STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->VEN_FieldIndex, 0x00);
        STTUNER_IOREG_SetField(DemodInstance->DeviceMap, DemodInstance->IOHandle,  DemodInstance->VSEL_FieldIndex, 0x02);
        break;

        default:
        return(ST_ERROR_BAD_PARAMETER);
    }
    return(ST_NO_ERROR);
}
#endif

/* End of lnb_demodIO.c */
