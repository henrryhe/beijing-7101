/* ----------------------------------------------------------------------------
File Name: scrdrv.c

Description: 

    stv0299 demod driver.


Copyright (C) 2003-2004 STMicroelectronics

History:
 
   date: 25-March-2004
version: 1.0.0
 author: SD
comment: write for multi-instance.

date: 07-April-2004
version: 
 author: SD 
comment: remove commented code.

date: 22-July-2004
version: 
 author: SD 
comment: Tone Detection Algorithm Added.
    
Reference:

    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
   #include <string.h>
#endif
#include "stlite.h"     /* Standard includes */
#include "sttbx.h"
#include "stevt.h"
#include "sttuner.h"                    

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "ioarch.h"     /* I/O for this driver */
#include "ioreg.h"      /* I/O for this driver */
#include "d0299.h"

#include "sioctl.h" 
#include "scr.h"     /* data structure typedefs for all the the sat ioctl functions */
#ifdef STTUNER_MINIDRIVER
  #include "l0299.h"
  #include "lnb21.h"
  #include "lnbh21.h"
  #include "d0399.h"
  #include "d0299.h"

#endif


static semaphore_t *Lock_InitTermOpenClose, *DiSEqC_Semaphore; /* guard calls to the functions */


#ifndef STTUNER_MINIDRIVER
static BOOL        Installed = FALSE;


/* ---------- per instance of driver ---------- */
typedef struct
{
    ST_DeviceName_t           *DeviceName;           /* unique name for opening under */
    STTUNER_Handle_t          TopLevelHandle;       /* access tuner, lnb etc. using this */
    IOARCH_Handle_t           IOHandle;             /* instance access to I/O driver     */
        
    ST_Partition_t            *MemoryPartition;     /* which partition this data block belongs to */
    void                      *InstanceChainPrev;   /* previous data block in chain or NULL if not */
    void                      *InstanceChainNext;   /* next data block in chain or NULL if last */
    STTUNER_DiSEqCConfig_t     DiSEqCConfig; /** Added DiSEqC configuration structure to get the trace of 										      current state**/
    STTUNER_SCRLNB_Index_t     LNBIndex ;   /* From which LNB to get RF signal*/
    STTUNER_SCRBPF_t           SCRBPF ; 
    STTUNER_SCR_Mode_t         SCR_Mode;
    STTUNER_ScrType_t          Scr_App_Type;
    U8 NbLnb;
    U16   SCRLNB_LO_Frequencies[8]; /* In MHz*/
    U8 NbScr;
    U16   SCRBPFFrequencies[8]; /* In MHz*/
    BOOL    SCREnable           ; /*IFA new feature*/
 }
SCR_InstanceData_t;
#endif

#ifdef STTUNER_MINIDRIVER
typedef struct
{
    STTUNER_Handle_t          TopLevelHandle;       /* access tuner, lnb etc. using this */
    IOARCH_Handle_t           IOHandle;             /* instance access to I/O driver     */        
    ST_Partition_t           *MemoryPartition;     /* which partition this data block belongs to */
    STTUNER_DiSEqCConfig_t    DiSEqCConfig; /** Added DiSEqC configuration structure to get the trace of current state**/
    STTUNER_SCRLNB_Index_t    LNBIndex ;   /* From which LNB to get RF signal*/
    STTUNER_SCRBPF_t          SCRBPF ; 
    STTUNER_SCR_Mode_t        SCR_Mode;
    STTUNER_ScrType_t         Scr_App_Type;
    U8 NbLnb;
    U16   SCRLNB_LO_Frequencies[8]; /* In MHz*/
    U8 NbScr;
    U16   SCRBPFFrequencies[8]; /* In MHz*/		
    BOOL                        SCREnable; 
 }
SCR_InstanceData_t;
#endif

#ifndef STTUNER_MINIDRIVER
/* instance chain, the default boot value is invalid, to catch errors */
static SCR_InstanceData_t *InstanceChainTop = (SCR_InstanceData_t *)0x7fffffff;
#endif
#ifdef STTUNER_MINIDRIVER
static SCR_InstanceData_t *SCRInstance;
#endif
#ifndef STTUNER_MINIDRIVER
static U16 LO_LUT[13] = 
		{
			0000,/* Not used*/
			0xfff,/* not used */
			9750,
			10000,
			10600,
			10750,
			11000,
			11250,
			11475,
			20250,
			5150,
			1585,
			13850
		};
	
#endif
/* functions --------------------------------------------------------------- */
#ifndef STTUNER_MINIDRIVER
/* API */
ST_ErrorCode_t scr_scrdrv_Init(ST_DeviceName_t *DeviceName, SCR_InitParams_t *InitParams);
ST_ErrorCode_t scr_scrdrv_Term(ST_DeviceName_t *DeviceName, SCR_TermParams_t *TermParams);

ST_ErrorCode_t scr_scrdrv_Open (ST_DeviceName_t *DeviceName, SCR_OpenParams_t  *OpenParams, SCR_Handle_t  *Handle, DEMOD_Handle_t DemodHandle, SCR_Capability_t *Capability);
ST_ErrorCode_t scr_scrdrv_Close(SCR_Handle_t  Handle, SCR_CloseParams_t *CloseParams);

ST_ErrorCode_t scr_scrdrv_IsLocked(SCR_Handle_t  Handle, DEMOD_Handle_t DemodHandle, BOOL  *IsLocked);

ST_ErrorCode_t scr_scrdrv_SetFrequency   (STTUNER_Handle_t Handle, DEMOD_Handle_t DemodHandle, ST_DeviceName_t *DeviceName, U32  InitialFrequency,  U8 LNBIndex, U8 SCRBPF );

ST_ErrorCode_t scr_scrdrv_Off (SCR_OpenParams_t  *OpenParams, DEMOD_Handle_t DemodHandle, U8 SCRBPF);
ST_ErrorCode_t scr_auto_detect(SCR_OpenParams_t  *OpenParams, DEMOD_Handle_t DemodHandle, U32 StartFreq, U32 StopFreq, U8  *NbTones, U32 *ToneList,int* power_detection_level);

ST_ErrorCode_t scr_tone_enable(SCR_OpenParams_t  *OpenParams, DEMOD_Handle_t DemodHandle);
U8 scr_get_application_number(SCR_OpenParams_t *OpenParams, DEMOD_Handle_t DemodHandle, U8 SCRBPF, U16 SCRCenterFrequency,int* power_detection_level);
U8 scr_get_LO_frequency(SCR_OpenParams_t *OpenParams, DEMOD_Handle_t DemodHandle, U8 SCRBPF, U16 SCRCenterFrequency, U16 *SCRLNB_Frequency,int* power_detection_level);
/* local functions --------------------------------------------------------- */
ST_ErrorCode_t scr_tone_enable_x(SCR_OpenParams_t  *OpenParams, DEMOD_Handle_t DemodHandle, U32 ScrIndex,U32 ScrNb);
SCR_InstanceData_t *SCR_GetInstFromHandle(SCR_Handle_t Handle);

extern ST_ErrorCode_t (* demod_tonedetection)(DEMOD_Handle_t Handle,U32 StartFreq, U32 StopFreq, U8  *NbTones,U32 *ToneListm, U8 mode);
#endif

#ifdef STTUNER_MINIDRIVER

#ifdef STTUNER_DRV_SAT_STV0399
extern ST_ErrorCode_t demod_d0399_tonedetection(DEMOD_Handle_t Handle,U32 StartFreq, U32 StopFreq, U8  *NbTones,U32 *ToneList);
#endif
#ifdef STTUNER_DRV_SAT_STV0399E
extern ST_ErrorCode_t demod_d0399_tonedetection(DEMOD_Handle_t Handle,U32 StartFreq, U32 StopFreq, U8  *NbTones,U32 *ToneList);
#endif

#endif
/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0299_Install()

Description:
    install a satellite device driver into the scr database.
    
Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifndef STTUNER_MINIDRIVER
ST_ErrorCode_t STTUNER_DRV_SCR_APPLICATION_Install(STTUNER_scr_dbase_t *Scr)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
        STTBX_Print(("%s fail driver already installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }
   
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
    STTBX_Print(("%s installing sat:scr:application3...", identity));
#endif

    /* mark ID in database */
    Scr->ID = STTUNER_SCR_APPLICATION;

    /* map API */
    Scr->scr_Init = scr_scrdrv_Init;
    Scr->scr_Term = scr_scrdrv_Term;

    Scr->scr_Open  = scr_scrdrv_Open;
    Scr->scr_Close = scr_scrdrv_Close;
    /*Scr->scr_IsLocked         = scr_scrdrv_IsLocked ;  */     
    Scr->scr_SetFrequency     = scr_scrdrv_SetFrequency;
    
    Scr->scr_Off = scr_scrdrv_Off;
    InstanceChainTop = NULL;
   
    Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);
    DiSEqC_Semaphore  = STOS_SemaphoreCreateFifo(NULL,1);

    Installed = TRUE;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
    STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_SCR_APPLICATION_3_unInstall()

Description:
    uninstall  satellite device driver into the scr database.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_SCR_APPLICATION_UnInstall(STTUNER_scr_dbase_t *Scr)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
   const char *identity = "STTUNER scr STTUNER_SCR_APPLICATION_3_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }
   
    if(Scr->ID != STTUNER_SCR_APPLICATION)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
        STTBX_Print(("%s fail incorrect driver type\n", identity));
#endif
        return(STTUNER_ERROR_ID);
    }

    /* has all memory been freed, by Term() */
    if(InstanceChainTop != NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
        STTBX_Print(("%s fail at least one instance not terminated\n", identity));
#endif
        return(ST_ERROR_OPEN_HANDLE);
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
    STTBX_Print(("%s uninstalling sat:scr...", identity));
#endif

    /* mark ID in database */
    Scr->ID = STTUNER_NO_DRIVER;

    /* unmap API */
    Scr->scr_Init = NULL;
    Scr->scr_Term = NULL;

    Scr->scr_Open  = NULL;
    Scr->scr_Close = NULL;

    Scr->scr_IsLocked         = NULL;       
    Scr->scr_SetFrequency    = NULL;
	
    Scr->scr_Off    = NULL;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
    STTBX_Print(("<"));
#endif

    STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);
    STOS_SemaphoreDelete(NULL,DiSEqC_Semaphore);

    
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
    STTBX_Print((">"));
#endif

    InstanceChainTop = (SCR_InstanceData_t *)0x7ffffffe;
    Installed        = FALSE;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
        STTBX_Print(("ok\n"));
#endif

    return(Error);
}
#endif

/* ----------------------------------------------------------------------------
Name: scr_scrdrv_Init()

Description:
    
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t scr_scrdrv_Init(ST_DeviceName_t *DeviceName, SCR_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
    const char *identity = "STTUNER scr scr_scrdrv_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U8 i;
#ifndef STTUNER_MINIDRIVER
    SCR_InstanceData_t *InstanceNew, *Instance;   
    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check params ---------- */
    Error = STTUNER_Util_CheckPtrNull(InitParams->MemoryPartition);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull(DeviceName);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
        STTBX_Print(("%s fail DeviceName not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( SCR_InstanceData_t ));
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
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
    InstanceNew->TopLevelHandle      = STTUNER_MAX_HANDLES;
    InstanceNew->IOHandle            = InitParams->IOHandle;
    InstanceNew->MemoryPartition     = InitParams->MemoryPartition;
    InstanceNew->InstanceChainNext   = NULL; /* always last in the chain */
    
    InstanceNew->LNBIndex            = InitParams->LNBIndex;
    InstanceNew->SCRBPF              = InitParams->SCRBPF;
    InstanceNew->SCR_Mode            = InitParams->SCR_Mode;
    if(InitParams->Scr_App_Type != STTUNER_SCR_NULL)	
    {
        InstanceNew->SCREnable=TRUE;
    }
    else	
    {
	InstanceNew->SCREnable=FALSE;
    }

    if(InstanceNew->SCR_Mode == STTUNER_SCR_MANUAL || InstanceNew->SCR_Mode == STTUNER_UBCHANGE)
    {
    	InstanceNew->Scr_App_Type  = InitParams->Scr_App_Type;
   	InstanceNew->NbScr = InitParams->Number_of_SCR;
   	InstanceNew->NbLnb = InitParams->Number_of_LNB;
	for(i=0; i<InstanceNew->NbScr; ++i)
   	 {
   		InstanceNew->SCRBPFFrequencies[i] = InitParams->SCRBPFFrequencies[i];
   	 }
   	 for(i=0; i<InstanceNew->NbLnb; ++i)
   	 {
   		InstanceNew->SCRLNB_LO_Frequencies[i] = InitParams->SCRLNB_LO_Frequencies[i];
   		
   	 }
     }
   	

#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes)\n", identity, InstanceNew->DeviceName, (U32)InstanceNew, sizeof( SCR_InstanceData_t ) ));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    
#endif
/****************************MINIDRIVER**********************
**************************************************************
***************************************************************/
#ifdef STTUNER_MINIDRIVER
	
	#if defined(ST_OS21) || defined(ST_OSLINUX)      
	    Lock_InitTermOpenClose = semaphore_create_fifo(1);
	    DiSEqC_Semaphore  = semaphore_create_fifo(1);
	#else
	    semaphore_init_fifo(&Lock_InitTermOpenClose, 1);
	    semaphore_init_fifo(&DiSEqC_Semaphore, 1);
	#endif
    
        /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    SCRInstance = memory_allocate_clear(InitParams->MemoryPartition, 1, sizeof( SCR_InstanceData_t ));
    if (SCRInstance == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
        STTBX_Print(("%s fail memory allocation InstanceNew\n", identity));
#endif    
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_NO_MEMORY);           
    }
    
    SCRInstance->TopLevelHandle      = STTUNER_MAX_HANDLES;
    SCRInstance->IOHandle            = InitParams->IOHandle;
    SCRInstance->MemoryPartition     = InitParams->MemoryPartition;
    
    SCRInstance->SCRBPF              = InitParams->SCRBPF;
    SCRInstance->SCR_Mode            = InitParams->SCR_Mode;
    if(SCRInstance->SCR_Mode == STTUNER_SCR_MANUAL)
    {
    	SCRInstance->Scr_App_Type  = InitParams->Scr_App_Type;
   	SCRInstance->NbScr = InitParams->Number_of_SCR;
   	SCRInstance->NbLnb  = InitParams->Number_of_LNB;
	for(i=0; i<SCRInstance->NbScr; ++i)
   	 {
   		SCRInstance->SCRBPFFrequencies[i] = InitParams->SCRBPFFrequencies[i];
   	 }
   	 for(i=0; i<SCRInstance->NbLnb; ++i)
   	 {
   		SCRInstance->SCRLNB_LO_Frequencies[i] = InitParams->SCRLNB_LO_Frequencies[i];
   		
   	 }
     }
     else 
     Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
	/**************************************************/
    SEM_UNLOCK(Lock_InitTermOpenClose);
    
#endif
    return(Error);
}   



/*----------------------------------------------------------------------------
Name: scr_scrdrv_Term()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t scr_scrdrv_Term(ST_DeviceName_t *DeviceName, SCR_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
   const char *identity = "STTUNER scr scr_scrdrv_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
#ifndef STTUNER_MINIDRIVER
    SCR_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
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
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }


    /* reap next matching DeviceName */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
    STTBX_Print(("%s looking for first free handle[\n", identity));
#endif

    Instance = InstanceChainTop;
    while(1)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        if ( strcmp( (char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
            STTBX_Print(("]\n"));
#endif
            
            /* found so now xlink prev and next(if applicable) and deallocate memory */
            InstancePrev = Instance->InstanceChainPrev;
            InstanceNext = Instance->InstanceChainNext;

            /* if instance to delete is first in chain */
            if (Instance->InstanceChainPrev == NULL)
            {
                InstanceChainTop = InstanceNext;        /* which would be NULL if last block to be term'd */
                if(InstanceNext != NULL)
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
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
            STTBX_Print(("%s terminated ok\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL) 
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
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


#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
#endif
/*********************************MINIDRIVER*****************************
**************************************************************************
***************************************************************************/
#ifdef STTUNER_MINIDRIVER
   
    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);
	memory_deallocate(SCRInstance->MemoryPartition, SCRInstance);
    SEM_UNLOCK(Lock_InitTermOpenClose);
   #if defined(ST_OS21)|| defined(ST_OSLINUX)
      semaphore_delete(Lock_InitTermOpenClose);
       semaphore_delete(DiSEqC_Semaphore);
    #else
      semaphore_delete(&Lock_InitTermOpenClose);
       semaphore_delete(&DiSEqC_Semaphore);
    #endif 
#endif    
    return(Error);
}   

/* ----------------------------------------------------------------------------
Name: scr_scrdrv_Open()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t scr_scrdrv_Open(ST_DeviceName_t *DeviceName, SCR_OpenParams_t  *OpenParams, SCR_Handle_t  *Handle, DEMOD_Handle_t DemodHandle, SCR_Capability_t *Capability)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
   const char *identity = "STTUNER scr scr_scrdrv_Open()";
#endif
    U8 i;
    ST_ErrorCode_t Error = ST_NO_ERROR;
#ifndef STTUNER_MINIDRIVER
   SCR_InstanceData_t     *Instance;
    STTUNER_InstanceDbase_t  *Inst;
    U32 ToneList[8] = {0};
    U32 ToneFreq[8] = {0};
    U32 ToneFound;
    U8  NbScr=0;
    U32 StartFreq, StopFreq;
    U32 index=0;
    U8  NbTones=0,j; 
    int  power_detection_level=0;
    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
#endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL) 
        {       
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
        STTBX_Print((" found ok\n"));
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
    STTBX_Print(("%s using block at 0x%08x\n", identity, (U32)Instance));
#endif

    /* check handle IS actually free */
    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
        STTBX_Print(("%s fail driver instance already open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    /* now got pointer to free (and valid) data block */

    Inst = STTUNER_GetDrvInst();    /* pointer to instance database */

     Capability->SCREnable=Instance->SCREnable; 
     if(Instance->SCREnable==TRUE)
     {
      	if(Instance->SCR_Mode == STTUNER_SCR_AUTO)
   	{ 
   		 Capability->SCRBPF  = STTUNER_SCRBPF0;
   		 StartFreq = (U32)950000000l;
	   	 StopFreq  = (U32)2150000000ul;
	   	 Error = scr_auto_detect(OpenParams, DemodHandle, StartFreq, StopFreq, &NbScr, ToneFreq,&power_detection_level);
	   	 index=0;
                for(i=0;i<NbScr;i++) 
                {
                 scr_tone_enable_x(OpenParams,DemodHandle,i,NbScr);
                 for(j=index;j<NbScr;j++)
                 {
                  StartFreq=ToneFreq[j]-10000000;
                  StopFreq=ToneFreq[j]+10000000;
                  ToneFound=0;
                  Error |= Inst[OpenParams->TopLevelHandle].Sat.Demod.Driver->demod_tonedetection(DemodHandle, StartFreq, StopFreq, &NbTones, &ToneFound,0,&power_detection_level);
                  if((ToneFound>StartFreq) && (ToneFound<StopFreq))
                  {
                   ToneList[index]=ToneFreq[j];
                   index++;
                   break;
                  }
                 }
                }
                Instance->NbScr=index;
                
	   	 Capability->Number_of_SCR = Instance->NbScr;
	   	 Capability->SCR_Mode=STTUNER_SCR_AUTO;
	   	 if(Instance->NbScr)
	   	 {
		   	 for(i=0; i<Instance->NbScr; ++i)
		   	 {
		   		scr_scrdrv_Off (OpenParams, DemodHandle, i);
		   		Instance->SCRBPFFrequencies[i] = (U16)(ToneList[i]/1000000l);
		   		Capability->SCRBPFFrequencies[i] = (U16)(ToneList[i]/1000000l);
		   		/*printf("\n SCR  freq[%d] = %d \n", i, Instance->SCRBPFFrequencies[i]);*/
		   		
		   	 }
		   		   	
		   	Instance->SCRBPF = STTUNER_SCRBPF0; /* Use SCR Index 0 to extract installation parameters*/
		   		   	
		     	Instance->NbLnb = scr_get_LO_frequency(OpenParams, DemodHandle, Instance->SCRBPF, Instance->SCRBPFFrequencies[Instance->SCRBPF], Instance->SCRLNB_LO_Frequencies,&power_detection_level);
		     	Capability->Number_of_LNB = Instance->NbLnb;
		     	for(i=0; i<Instance->NbLnb; ++i)
		   	 {
		   		Capability->SCRLNB_LO_Frequencies[i] = (U16)(Instance->SCRLNB_LO_Frequencies[i]);
		   		/*printf("\n LNB  freq[%d] = %d \n", i, Capability->SCRLNB_LO_Frequencies[i]);*/
		   	 }
		     	
		     	Instance->Scr_App_Type = scr_get_application_number(OpenParams, DemodHandle, Instance->SCRBPF, Instance->SCRBPFFrequencies[Instance->SCRBPF],&power_detection_level);
		     	if(Instance->Scr_App_Type == STTUNER_SCR_APPLICATION_NOT_SUPPORTED)
		     	{
		     		SEM_UNLOCK(Lock_InitTermOpenClose);
		   		return ST_ERROR_FEATURE_NOT_SUPPORTED;
		   	}
		   	else
		   	Capability->Scr_App_Type = Instance->Scr_App_Type;
		   	
		   	scr_scrdrv_Off (OpenParams, DemodHandle, Instance->SCRBPF);
		}
		else
		{
			SEM_UNLOCK(Lock_InitTermOpenClose);
	   		return ST_ERROR_FEATURE_NOT_SUPPORTED;
	   	}
   	}
   	else if(Instance->SCR_Mode == STTUNER_SCR_MANUAL)
   	{
   		/* make scr off*/
   		scr_scrdrv_Off (OpenParams, DemodHandle, Instance->SCRBPF);
   		Capability->SCR_Mode      = STTUNER_SCR_MANUAL;
   		Capability->SCRBPF        = Instance->SCRBPF;
   		Capability->Scr_App_Type  = Instance->Scr_App_Type;
   		Capability->Number_of_SCR = Instance->NbScr;
   		Capability->Number_of_LNB = Instance->NbLnb;
   		Capability->LNBIndex      = Instance->LNBIndex;
   		for(i=0; i<Instance->NbScr; ++i)
	   	 {
	   		Capability->SCRBPFFrequencies[i] = Instance->SCRBPFFrequencies[i];
	   	 }
	   	 for(i=0; i<Instance->NbLnb; ++i)
	   	 {
	   		Capability->SCRLNB_LO_Frequencies[i] = Instance->SCRLNB_LO_Frequencies[i];
	   		
	   	 }
	 }
	 else if(Instance->SCR_Mode == STTUNER_UBCHANGE)
   	{
   		/* make scr off*/
   		scr_scrdrv_Off (OpenParams, DemodHandle, Instance->SCRBPF);
   		Capability->SCR_Mode      = STTUNER_UBCHANGE;
   		Capability->SCRBPF        = Instance->SCRBPF;
   		Capability->Scr_App_Type  = Instance->Scr_App_Type;
   		Capability->Number_of_SCR = Instance->NbScr;
   		Capability->Number_of_LNB = Instance->NbLnb;
   		Capability->LNBIndex      = Instance->LNBIndex;
   		for(i=0; i<Instance->NbScr; ++i)
	   	 {
	   		Capability->SCRBPFFrequencies[i] = Instance->SCRBPFFrequencies[i];
	   	 }
	   	 for(i=0; i<Instance->NbLnb; ++i)
	   	 {
	   		Capability->SCRLNB_LO_Frequencies[i] = Instance->SCRLNB_LO_Frequencies[i];
	   		
	   	 }
	 }
   }
   
    /* finally (as nor more errors to check for, allocate handle */
    Instance->TopLevelHandle = OpenParams->TopLevelHandle;
    *Handle = (U32)Instance;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
    STTBX_Print(("%s opened ok\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
#endif
/******************************************MINIDRIVER*****************
************************************************************************
*************************************************************************/    
#ifdef STTUNER_MINIDRIVER
  
    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);
    /* check handle IS actually free */
    if(SCRInstance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
        STTBX_Print(("%s fail driver instance already open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }
     
   /* Check for free SCR, if not free return error*/
   	if(SCRInstance->SCR_Mode != STTUNER_SCR_MANUAL)
   	{ 
	   	 SEM_UNLOCK(Lock_InitTermOpenClose);
   		return ST_ERROR_FEATURE_NOT_SUPPORTED;
   	
   	}
        Capability->Number_of_SCR = SCRInstance->NbScr;
   	Capability->Number_of_LNB = SCRInstance->NbLnb;
   	for(i=0; i<SCRInstance->NbScr; ++i)
	{
		Capability->SCRBPFFrequencies[i] = SCRInstance->SCRBPFFrequencies[i];
	}
	for(i=0; i<SCRInstance->NbLnb; ++i)
	{
		Capability->SCRLNB_LO_Frequencies[i] = SCRInstance->SCRLNB_LO_Frequencies[i];
	}
  
    Capability->SCR_Mode      = STTUNER_SCR_MANUAL;
    Capability->LegacySupport = TRUE;
   
   /* make scr1 off*/
   scr_scrdrv_Off (OpenParams, DemodHandle, SCRInstance->SCRBPF);
   
    /* finally (as nor more errors to check for, allocate handle */
    SCRInstance->TopLevelHandle = OpenParams->TopLevelHandle;
    *Handle = (U32)SCRInstance;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
    STTBX_Print(("%s opened ok\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
#endif
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: scr_scrdrv_Close()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t scr_scrdrv_Close(SCR_Handle_t Handle, SCR_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
   const char *identity = "STTUNER scr scr_scrdrv_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
#ifndef STTUNER_MINIDRIVER
    SCR_InstanceData_t     *Instance;

    /* private driver instance data */
    Instance = SCR_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }


    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
    STTBX_Print(("%s closed\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
#endif
#ifdef STTUNER_MINIDRIVER
SCRInstance->TopLevelHandle = STTUNER_MAX_HANDLES;
#endif

    return(Error);
}   

#ifndef STTUNER_MINIDRIVER 
ST_ErrorCode_t scr_tone_enable_x(SCR_OpenParams_t  *OpenParams, DEMOD_Handle_t DemodHandle, U32 ScrIndex,U32 ScrNb)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t  *Inst;
    U32 i=0;
    Inst = STTUNER_GetDrvInst();
    

	Error = scr_tone_enable(OpenParams, DemodHandle);
	for(i=0;i<ScrNb;i++)
	{
		if(i!=ScrIndex)
		{
			scr_scrdrv_Off (OpenParams, DemodHandle, i);
	  	}
        }

  return Error;
}
#endif
/* ----------------------------------------------------------------------------
Name: scr_scrdrv_IsLocked()

Description:
     Checks the LK register i.e., are we demodulating a digital carrier.
   
Parameters:
    IsLocked,   pointer to area to store result (bool):
                TRUE -- we are locked.
                FALSE -- no lock.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t scr_scrdrv_IsLocked(SCR_Handle_t  Handle, DEMOD_Handle_t DemodHandle, BOOL  *IsLocked)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
   const char *identity = "STTUNER scr scr_scrdrv_IsLocked()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
   /* read ststus through DiSEqC command->presently not supported*/

#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
    STTBX_Print(("%s IsLocked=%u\n", identity, *IsLocked));
#endif
    return(Error);
}   

#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: SCR_GetInstFromHandle()

Description:
     Get SCR Instance from SCR Handle
   
Parameters:
         Handle -- SCR Handle
    
Return Value: Instance pointer
---------------------------------------------------------------------------- */
SCR_InstanceData_t *SCR_GetInstFromHandle(SCR_Handle_t Handle)
{

    SCR_InstanceData_t *Instance;

    Instance = (SCR_InstanceData_t *)Handle;


    return(Instance);
}
#endif

/* ----------------------------------------------------------------------------
Name: scr_scrdrv_Off()

Description:
     It's to put  the SCR in off mode. Because during power on there are a lot of noise bands at the output.
   
Parameters:
    			OpenParams - To get the TopLevelHandle
                DemodHandle -- Demod handle for DiSEqC operations.
                LNBIndex -- LNB index number.
                SCRBPF   -- Which SCR to switch off.
    
Return Value:Error
---------------------------------------------------------------------------- */

ST_ErrorCode_t scr_scrdrv_Off (SCR_OpenParams_t  *OpenParams, DEMOD_Handle_t DemodHandle, U8 SCRBPF)
{
    STTUNER_DiSEqCSendPacket_t pDiSEqCSendPacket;
    STTUNER_DiSEqCResponsePacket_t pDiSEqCResponsePacket;
    LNB_Config_t      LnbConfig;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    unsigned char command_array[] = {0xE0, 0x10, 0x5A, 0x00, 0x00};     
#ifndef STTUNER_MINIDRIVER
    STTUNER_InstanceDbase_t  *Inst;
     
    Inst = STTUNER_GetDrvInst();
       
	pDiSEqCSendPacket.DiSEqCCommandType = STTUNER_DiSEqC_COMMAND;
	pDiSEqCSendPacket.uc_TotalNoOfBytes = 5;
	
	command_array[3] |= (SCRBPF<<5);
	
	pDiSEqCSendPacket.pFrmAddCmdData = &command_array[0];
	
	pDiSEqCSendPacket.uc_msecBeforeNextCommand = 50;
      
	LnbConfig.TopLevelHandle = OpenParams->TopLevelHandle;
	LnbConfig.Status         = LNB_STATUS_ON;              /* Set the Satellite Dish LNB power to ON */
	LnbConfig.Polarization   = STTUNER_PLR_VERTICAL;       /* Setup the Satellite Dish LNB Polarization */
	LnbConfig.ToneState      = STTUNER_LNB_TONE_OFF;       /* Setup the require tone state */
	
	/* Use semaphore not to distrub the SCR setting during DiSEqC- ST Commands  */
	SEM_LOCK(DiSEqC_Semaphore);
	/* Invoke new configuration 13V*/
	Error = (Inst[OpenParams->TopLevelHandle].Sat.Lnb.Driver->lnb_SetConfig)(Inst[OpenParams->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
	
	/*18V*/
	LnbConfig.Polarization = STTUNER_PLR_HORIZONTAL;
	Error = (Inst[OpenParams->TopLevelHandle].Sat.Lnb.Driver->lnb_SetConfig)(Inst[OpenParams->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);

	STOS_TaskDelayUs(25000 );
	Error = (Inst[OpenParams->TopLevelHandle].Sat.Demod.Driver->demod_DiSEqC)(DemodHandle, &pDiSEqCSendPacket, &pDiSEqCResponsePacket );

	STOS_TaskDelayUs(10000 );
      
	/*come back on 13V*/
	LnbConfig.Polarization = STTUNER_PLR_VERTICAL;
	Error = (Inst[OpenParams->TopLevelHandle].Sat.Lnb.Driver->lnb_SetConfig)(Inst[OpenParams->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
	
	SEM_UNLOCK(DiSEqC_Semaphore);
#endif
/*****************************MINIDRIVER********************
*************************************************************
**************************************************************/
#ifdef STTUNER_MINIDRIVER
    STTUNER_InstanceDbase_t  *Inst;
     
    Inst = STTUNER_GetDrvInst();
       
	pDiSEqCSendPacket.DiSEqCCommandType = STTUNER_DiSEqC_COMMAND;
	pDiSEqCSendPacket.uc_TotalNoOfBytes = 5;
	
	command_array[3] = SCRBPF<<5;
	
	pDiSEqCSendPacket.pFrmAddCmdData = &command_array[0];
	
	pDiSEqCSendPacket.uc_msecBeforeNextCommand = 25;
      
	LnbConfig.TopLevelHandle = OpenParams->TopLevelHandle;
	LnbConfig.Status         = LNB_STATUS_ON;              /* Set the Satellite Dish LNB power to ON */
	LnbConfig.Polarization   = STTUNER_PLR_VERTICAL;       /* Setup the Satellite Dish LNB Polarization */
	LnbConfig.ToneState      = STTUNER_LNB_TONE_OFF;       /* Setup the require tone state */
	
	/* Use semaphore not to distrub the SCR setting during DiSEqC- ST Commands  */
	SEM_LOCK(DiSEqC_Semaphore);
	
      #ifdef STTUNER_DRV_SAT_STV0299
	/* Invoke new configuration 13V*/
	#ifdef STTUNER_DRV_SAT_LNB21
		Error = lnb_lnb21_SetConfig(&LnbConfig);
	#elif defined STTUNER_DRV_SAT_LNBH21
		Error = lnb_lnbh21_SetConfig(&LnbConfig);
	#else
		Error = lnb_l0299_SetConfig(Inst[OpenParams->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
	#endif
	/*18V*/
	LnbConfig.Polarization = STTUNER_PLR_HORIZONTAL;
	#ifdef STTUNER_DRV_SAT_LNB21
		Error = lnb_lnb21_SetConfig(&LnbConfig);
	#elif defined STTUNER_DRV_SAT_LNBH21
		Error = lnb_lnbh21_SetConfig(&LnbConfig);
	#else
		lnb_l0299_SetConfig(Inst[OpenParams->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
	#endif
		STOS_TaskDelayUs(10000 );
		Error = demod_d0299_DiSEqC(DemodHandle, &pDiSEqCSendPacket, &pDiSEqCResponsePacket );
		STOS_TaskDelayUs(2000 );
      
	/*come back on 13V*/
	LnbConfig.Polarization = STTUNER_PLR_VERTICAL;
	#ifdef STTUNER_DRV_SAT_LNB21
	Error = lnb_lnb21_SetConfig(&LnbConfig);
	#elif defined STTUNER_DRV_SAT_LNBH21
	Error = lnb_lnbh21_SetConfig(&LnbConfig);
	#else
	Error = lnb_l0299_SetConfig(Inst[OpenParams->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
	#endif
    #endif
    #if defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STV0399E)
	/* Invoke new configuration 13V*/
	#ifdef STTUNER_DRV_SAT_LNB21
		Error = lnb_lnb21_SetConfig(&LnbConfig);
	#elif defined STTUNER_DRV_SAT_LNBH21
		Error = lnb_lnbh21_SetConfig(&LnbConfig);
	#else
		Error = lnb_l0399_SetConfig(Inst[OpenParams->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
	#endif
	/*18V*/
	LnbConfig.Polarization = STTUNER_PLR_HORIZONTAL;
	#ifdef STTUNER_DRV_SAT_LNB21
		Error = lnb_lnb21_SetConfig(&LnbConfig);
	#elif defined STTUNER_DRV_SAT_LNBH21
		Error = lnb_lnbh21_SetConfig(&LnbConfig);
	#else
		lnb_l0399_SetConfig(Inst[OpenParams->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
	#endif
		STOS_TaskDelayUs(10000 );
		Error = demod_d0399_DiSEqC(DemodHandle, &pDiSEqCSendPacket, &pDiSEqCResponsePacket );
		STOS_TaskDelayUs(2000 );
      
	/*come back on 13V*/
	LnbConfig.Polarization = STTUNER_PLR_VERTICAL;
	#ifdef STTUNER_DRV_SAT_LNB21
		Error = lnb_lnb21_SetConfig(&LnbConfig);
	#elif defined STTUNER_DRV_SAT_LNBH21
		Error = lnb_lnbh21_SetConfig(&LnbConfig);
	#else
		Error = lnb_l0399_SetConfig(Inst[OpenParams->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
	#endif
    #endif
	
	SEM_UNLOCK(DiSEqC_Semaphore); 
#endif 
   return(Error);
   
}
#ifndef STTUNER_MINIDRIVER
ST_ErrorCode_t scr_tone_enable(SCR_OpenParams_t  *OpenParams, DEMOD_Handle_t DemodHandle)
{
    STTUNER_DiSEqCSendPacket_t pDiSEqCSendPacket;
    STTUNER_DiSEqCResponsePacket_t pDiSEqCResponsePacket;
    LNB_Config_t      LnbConfig;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    unsigned char command_array [5] = {0xE0, 0x10, 0x5B, 0x00, 0x00};/* E0(Framing)  10/11(address)   5A/5B(command)  data1(Sub_func = 0)   data2 = 00*/
    STTUNER_InstanceDbase_t  *Inst;
    Inst = STTUNER_GetDrvInst();

	pDiSEqCSendPacket.DiSEqCCommandType = STTUNER_DiSEqC_COMMAND;
	pDiSEqCSendPacket.uc_TotalNoOfBytes = 5;
	
	
	LnbConfig.TopLevelHandle                    = OpenParams->TopLevelHandle;
	LnbConfig.Status                            = LNB_STATUS_ON;              /* Set the Satellite Dish LNB power to ON */
	LnbConfig.Polarization                      = STTUNER_PLR_VERTICAL;       /* Setup the Satellite Dish LNB Polarization */
	LnbConfig.ToneState                         = STTUNER_LNB_TONE_OFF;       /* Setup the require tone state */
	pDiSEqCSendPacket.uc_msecBeforeNextCommand  = 50;
	pDiSEqCSendPacket.pFrmAddCmdData            = &command_array[0];
	
	SEM_LOCK(DiSEqC_Semaphore);
	
	/* Invoke new configuration 13V*/
	Error |= (Inst[OpenParams->TopLevelHandle].Sat.Lnb.Driver->lnb_SetConfig)(Inst[OpenParams->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
	
	/*18V*/
	LnbConfig.Polarization = STTUNER_PLR_HORIZONTAL;
	Error |= (Inst[OpenParams->TopLevelHandle].Sat.Lnb.Driver->lnb_SetConfig)(Inst[OpenParams->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
	STOS_TaskDelayUs(25000);
	/*for(;;)*/
	/* send DiSEqC Commands */
	Error |= (Inst[OpenParams->TopLevelHandle].Sat.Demod.Driver->demod_DiSEqC)(DemodHandle, &pDiSEqCSendPacket, &pDiSEqCResponsePacket );
	STOS_TaskDelayUs(10000);
	/* Come back to 13V  */
	LnbConfig.Polarization = STTUNER_PLR_VERTICAL;
	Error |= (Inst[OpenParams->TopLevelHandle].Sat.Lnb.Driver->lnb_SetConfig)(Inst[OpenParams->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
      
	SEM_UNLOCK(DiSEqC_Semaphore);
	return Error;

}
#endif
/* ----------------------------------------------------------------------------
Name: scr_auto_detect()

Description:
    This routine will find free scr with auto detection of tone sent by free scrs
    
Parameters:
        
Return Value:
---------------------------------------------------------------------------- */
#ifndef STTUNER_MINIDRIVER 
ST_ErrorCode_t scr_auto_detect(SCR_OpenParams_t  *OpenParams, DEMOD_Handle_t DemodHandle, U32 StartFreq, U32 StopFreq, U8  *NbTones, U32 *ToneList,int* power_detection_level)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
   
   	STTUNER_InstanceDbase_t  *Inst;
        Inst = STTUNER_GetDrvInst();
	
	Error = scr_tone_enable(OpenParams, DemodHandle);
	if(Error == ST_NO_ERROR)
	{
		Error |= Inst[OpenParams->TopLevelHandle].Sat.Demod.Driver->demod_tonedetection(DemodHandle, StartFreq, StopFreq, NbTones, ToneList, 1,power_detection_level);
	}
	
		
   return Error;

}
#endif
#ifndef STTUNER_MINIDRIVER 
U8 scr_get_LO_frequency(SCR_OpenParams_t *OpenParams, DEMOD_Handle_t DemodHandle, U8 SCRBPF,  U16 SCRCenterFrequency, U16 *SCRLNB_Frequency,int* power_detection_level)
{
    STTUNER_DiSEqCSendPacket_t     pDiSEqCSendPacket;
    STTUNER_DiSEqCResponsePacket_t pDiSEqCResponsePacket;
    LNB_Config_t                   LnbConfig;
    ST_ErrorCode_t                 Error = ST_NO_ERROR;
    U8                             lo_num, NbTones, nbLnbs=0;
    U32                            StartFreq, StopFreq, CenterFreq, tonefreq;
    unsigned char command_array [] = {0xE0, 0x10, 0x5B, 0x02, 0x00};/* E0(Framing)  10/11(address)   5B(command)  data1(Sub_func = 2)   data2 = Lo_num*/
    STTUNER_InstanceDbase_t        *Inst;
    Inst = STTUNER_GetDrvInst();
    
        pDiSEqCSendPacket.DiSEqCCommandType = STTUNER_DiSEqC_COMMAND;
	pDiSEqCSendPacket.pFrmAddCmdData    = &command_array[0];
	pDiSEqCSendPacket.uc_TotalNoOfBytes = 5;
	
	LnbConfig.TopLevelHandle                    = OpenParams->TopLevelHandle;
	LnbConfig.Status                            = LNB_STATUS_ON;              /* Set the Satellite Dish LNB power to ON */
	LnbConfig.Polarization                      = STTUNER_PLR_VERTICAL;       /* Setup the Satellite Dish LNB Polarization */
	LnbConfig.ToneState                         = STTUNER_LNB_TONE_OFF;       /* Setup the require tone state */
	pDiSEqCSendPacket.uc_msecBeforeNextCommand  = 50;
    
	CenterFreq = (U32)(SCRCenterFrequency*1000000);
	StartFreq = (U32)(CenterFreq - 10000000);
	StopFreq  = (U32)(CenterFreq + 10000000);
	command_array[3] |= (SCRBPF<<5);
		
	/* Invoke new configuration 13V*/
	Error |= (Inst[OpenParams->TopLevelHandle].Sat.Lnb.Driver->lnb_SetConfig)(Inst[OpenParams->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
	
	for(lo_num = 2; lo_num<=0x0c; lo_num++)
	{
		command_array[4] = lo_num;
		
		SEM_LOCK(DiSEqC_Semaphore);	
			
		/*18V*/
		LnbConfig.Polarization = STTUNER_PLR_HORIZONTAL;
		Error |= (Inst[OpenParams->TopLevelHandle].Sat.Lnb.Driver->lnb_SetConfig)(Inst[OpenParams->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
		STOS_TaskDelayUs(25000 );
		
		/* send DiSEqC Commands */
		Error |= (Inst[OpenParams->TopLevelHandle].Sat.Demod.Driver->demod_DiSEqC)(DemodHandle, &pDiSEqCSendPacket, &pDiSEqCResponsePacket );
		STOS_TaskDelayUs(10000);
		/* Come back to 13V  */
		LnbConfig.Polarization = STTUNER_PLR_VERTICAL;
		Error |= (Inst[OpenParams->TopLevelHandle].Sat.Lnb.Driver->lnb_SetConfig)(Inst[OpenParams->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
	        SEM_UNLOCK(DiSEqC_Semaphore);
		Error |= Inst[OpenParams->TopLevelHandle].Sat.Demod.Driver->demod_tonedetection(DemodHandle, StartFreq, StopFreq, &NbTones, &tonefreq,0,power_detection_level);
		if(NbTones)
		{
			if((tonefreq>= StartFreq) && (tonefreq<= StopFreq))
			{SCRLNB_Frequency[nbLnbs] = LO_LUT[lo_num]; ++nbLnbs;}
		}
	}
	return (nbLnbs);
}

#endif		
#ifndef STTUNER_MINIDRIVER
U8 scr_get_application_number(SCR_OpenParams_t *OpenParams, DEMOD_Handle_t DemodHandle, U8 SCRBPF, U16 SCRCenterFrequency,int* power_detection_level)
{
    STTUNER_DiSEqCSendPacket_t     pDiSEqCSendPacket;
    STTUNER_DiSEqCResponsePacket_t pDiSEqCResponsePacket;
    LNB_Config_t                   LnbConfig;
    ST_ErrorCode_t                 Error = ST_NO_ERROR;
    U8                             app_num, NbTones;
    U32                            StartFreq, StopFreq, CenterFreq, tonefreq = 0;
    unsigned char command_array [] = {0xE0, 0x10, 0x5B, 0x01, 0x00};
    STTUNER_InstanceDbase_t        *Inst;
    Inst = STTUNER_GetDrvInst();

	pDiSEqCSendPacket.DiSEqCCommandType = STTUNER_DiSEqC_COMMAND;
	pDiSEqCSendPacket.pFrmAddCmdData    = &command_array[0];
	pDiSEqCSendPacket.uc_TotalNoOfBytes = 5;
	
	
	LnbConfig.TopLevelHandle                    = OpenParams->TopLevelHandle;
	LnbConfig.Status                            = LNB_STATUS_ON;              /* Set the Satellite Dish LNB power to ON */
	LnbConfig.Polarization                      = STTUNER_PLR_VERTICAL;       /* Setup the Satellite Dish LNB Polarization */
	LnbConfig.ToneState                         = STTUNER_LNB_TONE_OFF;       /* Setup the require tone state */
	pDiSEqCSendPacket.uc_msecBeforeNextCommand  = 50;
	
	
	CenterFreq = (U32)(SCRCenterFrequency*1000000);
	StartFreq = (U32)(CenterFreq - 10000000);
	StopFreq  = (U32)(CenterFreq + 10000000);
	command_array[3] |= (SCRBPF<<5);
		
	/* Invoke new configuration 13V*/
	Error |= (Inst[OpenParams->TopLevelHandle].Sat.Lnb.Driver->lnb_SetConfig)(Inst[OpenParams->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
	
	for(app_num = 1; app_num<=7; app_num++)
	{
		command_array[4] = app_num;
		
		SEM_LOCK(DiSEqC_Semaphore);	
			
		/*18V*/
		LnbConfig.Polarization = STTUNER_PLR_HORIZONTAL;
		Error |= (Inst[OpenParams->TopLevelHandle].Sat.Lnb.Driver->lnb_SetConfig)(Inst[OpenParams->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
		STOS_TaskDelayUs(25000 );
		
		/* send DiSEqC Commands */
		Error |= (Inst[OpenParams->TopLevelHandle].Sat.Demod.Driver->demod_DiSEqC)(DemodHandle, &pDiSEqCSendPacket, &pDiSEqCResponsePacket );
		STOS_TaskDelayUs(10000 );
		/* Come back to 13V  */
		LnbConfig.Polarization = STTUNER_PLR_VERTICAL;
		Error |= (Inst[OpenParams->TopLevelHandle].Sat.Lnb.Driver->lnb_SetConfig)(Inst[OpenParams->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
	        SEM_UNLOCK(DiSEqC_Semaphore);
		Error |= Inst[OpenParams->TopLevelHandle].Sat.Demod.Driver->demod_tonedetection(DemodHandle, StartFreq, StopFreq, &NbTones, &tonefreq,0,power_detection_level);
		if(NbTones)
		{
			if((tonefreq>= StartFreq) && (tonefreq<= StopFreq))
			{
				return(app_num);
			}
		}
		
	}

	return STTUNER_SCR_APPLICATION_NOT_SUPPORTED;
	
}
#endif
/* ----------------------------------------------------------------------------
Name: scr_scrdrv_SetFrequency()

Description:
    This routine will attempt to scan and find a QPSK signal based on the
    passed in parameters.
    
Parameters:
    InitialFrequency, IF value to commence scan (in kHz).
    SymbolRate,       required symbol bit rate (in Hz).
    MaxLNBOffset,     maximum allowed LNB offset (in Hz).
    TunerStep,        Tuner's step value -- enables override of
                      TNR device's internal setting.
    DerotatorStep,    derotator step (usually 6).
    ScanSuccess,      boolean that indicates QPSK search success.
    NewFrequency,     pointer to area to store locked frequency.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t scr_scrdrv_SetFrequency   (STTUNER_Handle_t Handle, DEMOD_Handle_t DemodHandle, ST_DeviceName_t *DeviceName, U32  InitialFrequency, U8 LNBIndex, U8 SCRBPF) 
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
   const char *identity = "STTUNER scr scr_scrdrv_SetFrequency()";
#endif
unsigned char command_array [5] = {0xE0, 0x10, 0x5A, 0x00, 0x00};
    LNB_Config_t      LnbConfig;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_DiSEqCSendPacket_t pDiSEqCSendPacket;
    STTUNER_DiSEqCResponsePacket_t pDiSEqCResponsePacket;
    U32 tuning_word, temp;
#ifndef STTUNER_MINIDRIVER
    STTUNER_InstanceDbase_t  *Inst;
    SCR_InstanceData_t     *Instance;      
    /* top level public instance data */ 
    Inst = STTUNER_GetDrvInst();
    
    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* find  handle from name */
    /***************************************************/
    Instance = InstanceChainTop;
	
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL) 
        {       
		#ifdef STTUNER_DEBUG_MODULE_SATDRV_SCR
		            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
		#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
    }

        
    SEM_UNLOCK(Lock_InitTermOpenClose);
    /*******************************************************/
    /* For Auto Detect update scanlist for free SCR found */
    /*if(Instance->SCR_Mode == STTUNER_SCR_AUTO)
    {
    	Inst[Instance->TopLevelHandle].ScanList.ScanList->ScrParams.SCRBPF=Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.ScrParams.SCRBPF = Instance->SCRBPF;
    	SCRBPF = Instance->SCRBPF;
    }*/
    /******************************************************/   
    
    pDiSEqCSendPacket.DiSEqCCommandType = STTUNER_DiSEqC_COMMAND;
    pDiSEqCSendPacket.uc_TotalNoOfBytes = 5;
    pDiSEqCSendPacket.uc_msecBeforeNextCommand = 50;
    
    /* do the frequency translation here*/
    
    InitialFrequency += Instance->SCRBPFFrequencies[SCRBPF];
          
    tuning_word = (InitialFrequency/4) - 350; /* Formula according to data sheet */
    /*Inst[Instance->TopLevelHandle].ScanList.ScanList->ScrParams.SCRVCOFrequency = (tuning_word + 350)*4000;*/
    command_array[4] = tuning_word & 0xFF;
    temp = tuning_word & 0x300;
    temp = temp>>8;
    command_array[3] |= temp;
    temp = SCRBPF<<5;
    command_array[3] |= temp;
    temp = LNBIndex<<2;
    command_array[3] |= temp;
     
    pDiSEqCSendPacket.pFrmAddCmdData = &command_array[0];
  
    LnbConfig.TopLevelHandle = Handle;
    LnbConfig.Status         = LNB_STATUS_ON;          /* Set the Satellite Dish LNB power to ON */
    LnbConfig.Polarization   = STTUNER_PLR_VERTICAL;   /* Setup the Satellite Dish LNB Polarization */
    LnbConfig.ToneState      = STTUNER_LNB_TONE_OFF;   /* Setup the require tone state */
    
    /* Use semaphore not to distrub the SCR setting during DiSEqC- ST Commands  */
    SEM_LOCK(DiSEqC_Semaphore);
	/* Invoke new configuration 13V*/
	Error = (Inst[Handle].Sat.Lnb.Driver->lnb_SetConfig)(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
	
	/*18V*/
	LnbConfig.Polarization = STTUNER_PLR_HORIZONTAL;
	Error = (Inst[Handle].Sat.Lnb.Driver->lnb_SetConfig)(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
	STOS_TaskDelayUs(25000 );
   
    Error = (Inst[Handle].Sat.Demod.Driver->demod_DiSEqC)(DemodHandle, &pDiSEqCSendPacket, &pDiSEqCResponsePacket );
	STOS_TaskDelayUs(10000);
  	/*come back on 13V*/
	LnbConfig.Polarization = STTUNER_PLR_VERTICAL;
	Error = (Inst[Handle].Sat.Lnb.Driver->lnb_SetConfig)(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
  
   SEM_UNLOCK(DiSEqC_Semaphore);
#endif
 /************************MINIDRIVER************************
 **************************************************************
 ****************************************************************/  
#ifdef STTUNER_MINIDRIVER
    STTUNER_InstanceDbase_t  *Inst;
    Inst = STTUNER_GetDrvInst();
    /*******************************************************/
    /* For Auto Detect update scanlist for free SCR found */
    if(SCRInstance->SCR_Mode == STTUNER_SCR_AUTO)
    {
    	Inst[SCRInstance->TopLevelHandle].ScanList.ScanList->ScrParams.SCRBPF=Inst[SCRInstance->TopLevelHandle].CurrentTunerInfo.ScanInfo.ScrParams.SCRBPF = SCRInstance->SCRBPF;
    	SCRBPF = SCRInstance->SCRBPF;
    }
    /******************************************************/   
    
    pDiSEqCSendPacket.DiSEqCCommandType = STTUNER_DiSEqC_COMMAND;
    pDiSEqCSendPacket.uc_TotalNoOfBytes = 5;
    pDiSEqCSendPacket.uc_msecBeforeNextCommand = 25;
    
    /* do the frequency translation here*/
    InitialFrequency += SCRInstance->SCRBPFFrequencies[SCRBPF];
      
    tuning_word = (InitialFrequency/4) - 350; /* Formula according to data sheet */
    /*Inst[SCRInstance->TopLevelHandle].ScanList.ScanList->ScrParams.SCRVCOFrequency = (tuning_word + 350)*4000;*/
    command_array[4] = tuning_word & 0xFF;
    temp = tuning_word & 0x300;
    temp = temp>>8;
    command_array[3] |= temp;
    temp = SCRBPF<<5;
    command_array[3] |= temp;
    temp = LNBIndex<<2;
    command_array[3] |= temp;
     
    pDiSEqCSendPacket.pFrmAddCmdData = &command_array[0];
  
    LnbConfig.TopLevelHandle = Handle;
    LnbConfig.Status         = LNB_STATUS_ON;          /* Set the Satellite Dish LNB power to ON */
    LnbConfig.Polarization   = STTUNER_PLR_VERTICAL;   /* Setup the Satellite Dish LNB Polarization */
    LnbConfig.ToneState      = STTUNER_LNB_TONE_OFF;   /* Setup the require tone state */
    
    #ifdef STTUNER_DRV_SAT_STV0299
		/* Use semaphore not to distrub the SCR setting during DiSEqC- ST Commands  */
                SEM_LOCK(DiSEqC_Semaphore);
		/* Invoke new configuration 13V*/
		#ifdef STTUNER_DRV_SAT_LNB21
			Error = lnb_lnb21_SetConfig(&LnbConfig);
		#elif defined STTUNER_DRV_SAT_LNBH21
			Error = lnb_lnbh21_SetConfig(&LnbConfig);
		#else
			Error = lnb_l0299_SetConfig(Inst[SCRInstance->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
		#endif
    
		/*18V*/
	        LnbConfig.Polarization = STTUNER_PLR_HORIZONTAL;
	        #ifdef STTUNER_DRV_SAT_LNB21
			Error = lnb_lnb21_SetConfig(&LnbConfig);
		#elif defined STTUNER_DRV_SAT_LNBH21
			Error = lnb_lnbh21_SetConfig(&LnbConfig);
		#else
			Error = lnb_l0299_SetConfig(Inst[SCRInstance->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
		#endif
			STOS_TaskDelayUs(30000 );
   
    		Error = demod_d0299_DiSEqC(DemodHandle, &pDiSEqCSendPacket, &pDiSEqCResponsePacket );
			    STOS_TaskDelayUs(10000);
          	
	  	/*come back on 13V*/
		LnbConfig.Polarization = STTUNER_PLR_VERTICAL;
		#ifdef STTUNER_DRV_SAT_LNB21
			Error = lnb_lnb21_SetConfig(&LnbConfig);
		#elif defined STTUNER_DRV_SAT_LNBH21
			Error = lnb_lnbh21_SetConfig(&LnbConfig);
		#else
			Error = lnb_l0299_SetConfig(Inst[SCRInstance->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
		#endif
  
   		SEM_UNLOCK(DiSEqC_Semaphore);   		
   	#endif
   	
   	#ifdef STTUNER_DRV_SAT_STV0399
		/* Use semaphore not to distrub the SCR setting during DiSEqC- ST Commands  */
                SEM_LOCK(DiSEqC_Semaphore);
		/* Invoke new configuration 13V*/
		#ifdef STTUNER_DRV_SAT_LNB21
			Error = lnb_lnb21_SetConfig(&LnbConfig);
		#elif defined STTUNER_DRV_SAT_LNBH21
			Error = lnb_lnbh21_SetConfig(&LnbConfig);
		#else
			Error = lnb_l0399_SetConfig(Inst[SCRInstance->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
		#endif
    
		/*18V*/
	        LnbConfig.Polarization = STTUNER_PLR_HORIZONTAL;
	        #ifdef STTUNER_DRV_SAT_LNB21
			Error = lnb_lnb21_SetConfig(&LnbConfig);
		#elif defined STTUNER_DRV_SAT_LNBH21
			Error = lnb_lnbh21_SetConfig(&LnbConfig);
		#else
			Error = lnb_l0399_SetConfig(Inst[SCRInstance->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
		#endif
			STOS_TaskDelayUs(10000 );
   
    		Error = demod_d0399_DiSEqC(DemodHandle, &pDiSEqCSendPacket, &pDiSEqCResponsePacket );
		    STOS_TaskDelayUs(2000);
		    
	  	/*come back on 13V*/
		LnbConfig.Polarization = STTUNER_PLR_VERTICAL;
		#ifdef STTUNER_DRV_SAT_LNB21
			Error = lnb_lnb21_SetConfig(&LnbConfig);
		#elif defined STTUNER_DRV_SAT_LNBH21
			Error = lnb_lnbh21_SetConfig(&LnbConfig);
		#else
			Error = lnb_l0399_SetConfig(Inst[SCRInstance->TopLevelHandle].Sat.Lnb.DrvHandle, &LnbConfig);
		#endif
  
   		SEM_UNLOCK(DiSEqC_Semaphore);   		
   	#endif
#endif
   return(Error);
 
}   

