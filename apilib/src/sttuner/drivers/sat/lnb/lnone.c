/* ----------------------------------------------------------------------------
File Name: lnone.c

Description: 

    null demod driver.


Copyright (C) 1999-2001 STMicroelectronics

History:
 
   date: 20-June-2001
version: 3.1.0
 author: GJP
comment: write for multi-instance.
    
   date: 17-August-2001
version: 3.1.1
 author: GJP
comment: update SubAddr to U16

Reference:

    ST API Definition "LNB Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */

/* C libs */
#include <string.h>                     

#include "stlite.h"     /* Standard includes */

/* STAPI */
#include "sttbx.h"
#include "stevt.h"
#include "sttuner.h"                    

/* local to stlnb */
#include "util.h"       /* generic utility functions for stlnb */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "ioarch.h"     /* I/O for this driver */
#include "sdrv.h"       /* utilities */

#include "lnone.h"      /* header for this file */



/* private variables ------------------------------------------------------- */

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */

static BOOL        Installed = FALSE;
int Index = 0;

/* functions --------------------------------------------------------------- */

/* API */
ST_ErrorCode_t lnb_none_Init(ST_DeviceName_t *DeviceName, LNB_InitParams_t *InitParams);
ST_ErrorCode_t lnb_none_Term(ST_DeviceName_t *DeviceName, LNB_TermParams_t *TermParams);

ST_ErrorCode_t lnb_none_Open (ST_DeviceName_t *DeviceName, LNB_OpenParams_t *OpenParams, LNB_Capability_t *Capability, LNB_Handle_t *Handle);
ST_ErrorCode_t lnb_none_Close(LNB_Handle_t  Handle, LNB_CloseParams_t *CloseParams);

ST_ErrorCode_t lnb_none_GetConfig(LNB_Handle_t Handle, LNB_Config_t *Config);
ST_ErrorCode_t lnb_none_SetConfig(LNB_Handle_t Handle, LNB_Config_t *Config);


/* I/O API */
ST_ErrorCode_t lnb_none_ioaccess(LNB_Handle_t Handle, IOARCH_Handle_t IOHandle,
    STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);

/* access device specific low-level functions */
ST_ErrorCode_t lnb_none_ioctl(LNB_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status);


/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_LNB_NONE_Install()

Description:
    install a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_LNB_NONE_Install(STTUNER_lnb_dbase_t *Lnb)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
   const char *identity = "STLNB lnone.c STTUNER_DRV_LNB_NONE_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

   
    if(Installed == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
        STTBX_Print(("%s fail driver already installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
    STTBX_Print(("%s installing sat:lnb:none...", identity));
#endif

    Lnb->ID = STTUNER_LNB_NONE;

    /* map API */
    Lnb->lnb_Init      = lnb_none_Init;
    Lnb->lnb_Term      = lnb_none_Term;
    Lnb->lnb_Open      = lnb_none_Open;
    Lnb->lnb_Close     = lnb_none_Close;
    Lnb->lnb_GetConfig = lnb_none_GetConfig; 
    Lnb->lnb_SetConfig = lnb_none_SetConfig;
    Lnb->lnb_ioaccess  = lnb_none_ioaccess;
    Lnb->lnb_ioctl     = lnb_none_ioctl;
    #if defined(STTUNER_DRV_SAT_LNB21) || defined(STTUNER_DRV_SAT_LNBH21) /*Added for for GNBvd40148-->Error
    if sttuner compiled with LNBH21 option but use another one: This will only come when more than 1 lnb is
    in use & atleast one of them is LNB21 or LNBH21*/
    Lnb->lnb_overloadcheck    = lnb_lnone_overloadcheck;
    #if defined(STTUNER_DRV_SAT_LNBH21)
    Lnb->lnb_setttxmode       = lnb_lnone_setttxmode;
    #endif
    #endif


   Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);


    Installed = TRUE;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
        STTBX_Print(("ok\n"));
#endif    
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_LNB_NONE_UnInstall()

Description:
    install a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_LNB_NONE_UnInstall(STTUNER_lnb_dbase_t *Lnb)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
   const char *identity = "STLNB lnone.c STTUNER_DRV_LNB_NONE_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }
   
    if(Lnb->ID != STTUNER_LNB_NONE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
        STTBX_Print(("%s fail incorrect driver type\n", identity));
#endif
        return(STTUNER_ERROR_ID);
    }


    Lnb->ID = STTUNER_NO_DRIVER;

    /* map API */
    Lnb->lnb_Init      = NULL;
    Lnb->lnb_Term      = NULL;
    Lnb->lnb_Open      = NULL;
    Lnb->lnb_Close     = NULL;
    Lnb->lnb_GetConfig = NULL; 
    Lnb->lnb_SetConfig = NULL;
    Lnb->lnb_ioaccess  = NULL;
    Lnb->lnb_ioctl     = NULL;
    #if defined(STTUNER_DRV_SAT_LNB21) || defined(STTUNER_DRV_SAT_LNBH21)
    Lnb->lnb_overloadcheck    = NULL;
    #if defined(STTUNER_DRV_SAT_LNBH21)
    Lnb->lnb_setttxmode       = NULL;
    #endif
    #endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
        STTBX_Print(("<"));
#endif

        STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);
       
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
        STTBX_Print((">"));
#endif

    Installed = FALSE;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
        STTBX_Print(("ok\n"));
#endif

    return(Error);
}

/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */


   
/* ----------------------------------------------------------------------------
Name: lnb_none_Init()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_none_Init(ST_DeviceName_t *DeviceName, LNB_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
   const char *identity = "STTUNER lnone.c lnb_none_Init()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
    STTBX_Print(("%s initalized (%s) ok\n", identity, DeviceName));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: lnb_none_Term()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_none_Term(ST_DeviceName_t *DeviceName, LNB_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
   const char *identity = "STTUNER lnone.c lnb_none_Term()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
    STTBX_Print(("%s terminated (%s) ok\n", identity, DeviceName));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: lnb_none_Open()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_none_Open(ST_DeviceName_t *DeviceName, LNB_OpenParams_t *OpenParams, LNB_Capability_t *Capability, LNB_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
   const char *identity = "STLNB lnone.c lnb_none_Open()";
#endif
    Index++;
   *Handle = Index;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
    STTBX_Print(("%s opened (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   

#if defined(STTUNER_DRV_SAT_LNB21) || defined(STTUNER_DRV_SAT_LNBH21)
/* ----------------------------------------------------------------------------
Name: lnb_lnone_overloadcheck()

Description:
    Dummy Function
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t lnb_lnone_overloadcheck(LNB_Handle_t Handle, BOOL  *IsOverTemp, BOOL *IsCurrentOvrLoad)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    return(Error);
}

#if defined(STTUNER_DRV_SAT_LNBH21)
/* ----------------------------------------------------------------------------
Name: lnb_lnone_setttxmode()

Description:
    Dummy Function
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t lnb_lnone_setttxmode(LNB_Handle_t Handle, STTUNER_LnbTTxMode_t Ttxmode)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    return(Error);
}
#endif
#endif



/* ----------------------------------------------------------------------------
Name: lnb_none_Close()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_none_Close(LNB_Handle_t Handle, LNB_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
   const char *identity = "STLNB lnone.c lnb_none_Close()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
    STTBX_Print(("%s closed (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: lnb_none_GetConfig()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_none_GetConfig(LNB_Handle_t Handle, LNB_Config_t *Config)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
   const char *identity = "STLNB lnone.c lnb_none_GetConfig()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
    STTBX_Print(("%s GetConfig (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   


/* ----------------------------------------------------------------------------
Name: lnb_none_SetConfig()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_none_SetConfig(LNB_Handle_t Handle, LNB_Config_t *Config)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
   const char *identity = "STLNB lnone.c lnb_none_SetConfig()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
    STTBX_Print(("%s SetConfig (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: lnb_none_ioctl()

Description:
    access device specific low-level functions
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_none_ioctl(LNB_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
   
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
   const char *identity = "STLNB lnone.c lnb_none_ioctl()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
    STTBX_Print(("%s ioctl (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   




/* ----------------------------------------------------------------------------
Name: lnb_none_ioaccess()

Description:
    we get called with the instance of
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t lnb_none_ioaccess(LNB_Handle_t Handle, IOARCH_Handle_t IOHandle,STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)
   
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
   const char *identity = "STLNB lnone.c lnb_none_ioaccess()";
#endif

 #ifdef STTUNER_DEBUG_MODULE_SATDRV_LNONE
    STTBX_Print(("%s ioaccess (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */


/* End of lnone.c */
