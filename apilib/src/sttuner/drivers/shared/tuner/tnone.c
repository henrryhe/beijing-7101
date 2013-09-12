/* ----------------------------------------------------------------------------
File Name: tnone.c

Description: 

    null tuner driver.


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

    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */

/* C libs */
#ifndef ST_OSLINUX

#include <string.h>  
#endif                   
#include "stlite.h"     /* Standard includes */

/* STAPI */
#include "sttbx.h"


/* STAPI */

#include "stevt.h"
#include "sttuner.h"                    

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "ioarch.h"     /* I/O for this driver */

#include "tnone.h"      /* header for this file */



/* private variables ------------------------------------------------------- */

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */

static BOOL        Installed = FALSE;
static int index_tuner = 0;


/* functions --------------------------------------------------------------- */

/* API */

ST_ErrorCode_t tuner_none_Init(ST_DeviceName_t *DeviceName, TUNER_InitParams_t *InitParams);
ST_ErrorCode_t tuner_none_Term(ST_DeviceName_t *DeviceName, TUNER_TermParams_t *TermParams);

ST_ErrorCode_t tuner_none_Open (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t  *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
ST_ErrorCode_t tuner_none_Close(TUNER_Handle_t  Handle, TUNER_CloseParams_t *CloseParams);
        
ST_ErrorCode_t tuner_none_SetFrequency (TUNER_Handle_t Handle, U32 Frequency, U32 *NewFrequency);
ST_ErrorCode_t tuner_none_GetStatus    (TUNER_Handle_t Handle, TUNER_Status_t *Status);
ST_ErrorCode_t tuner_none_IsTunerLocked(TUNER_Handle_t Handle, BOOL *Locked);
ST_ErrorCode_t tuner_none_SetBandWidth (TUNER_Handle_t Handle, U32 BandWidth, U32 *NewBandWidth);


/* I/O API */
ST_ErrorCode_t tuner_none_ioaccess(TUNER_Handle_t Handle, IOARCH_Handle_t IOHandle,
    STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);

/* access device specific low-level functions */
ST_ErrorCode_t tuner_none_ioctl(TUNER_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status);


/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_TUNER_NONE_Install()

Description:
    install a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_TUNER_NONE_Install(STTUNER_tuner_dbase_t  *Tuner)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
   const char *identity = "STTUNER tnone.c STTUNER_DRV_TUNER_NONE_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

   
    if(Installed == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
        STTBX_Print(("%s fail driver already installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
        STTBX_Print(("%s installing sat:tuner:none...", identity));
#endif

    Tuner->ID = STTUNER_TUNER_NONE;

    /* map API */
    Tuner->tuner_Init  = tuner_none_Init;
    Tuner->tuner_Term  = tuner_none_Term;
    Tuner->tuner_Open  = tuner_none_Open;
    Tuner->tuner_Close = tuner_none_Close;

    Tuner->tuner_SetFrequency  = tuner_none_SetFrequency; 
    Tuner->tuner_GetStatus     = tuner_none_GetStatus;
    Tuner->tuner_IsTunerLocked = tuner_none_IsTunerLocked;   
    Tuner->tuner_SetBandWidth  = tuner_none_SetBandWidth;          

    Tuner->tuner_ioaccess = tuner_none_ioaccess;
    Tuner->tuner_ioctl    = tuner_none_ioctl;


   Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);


    Installed = TRUE;

#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
        STTBX_Print(("ok\n"));
#endif    
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_TUNER_NONE_UnInstall()

Description:
    install a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_TUNER_NONE_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
   const char *identity = "STLNB tnone.c STTUNER_DRV_TUNER_NONE_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }
   
    if(Tuner->ID != STTUNER_TUNER_NONE)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
        STTBX_Print(("%s fail incorrect driver type\n", identity));
#endif
        return(STTUNER_ERROR_ID);
    }

    Tuner->ID = STTUNER_NO_DRIVER;

    /* unmap API */
    Tuner->tuner_Init  = NULL;
    Tuner->tuner_Term  = NULL;
    Tuner->tuner_Open  = NULL;
    Tuner->tuner_Close = NULL;

    Tuner->tuner_SetFrequency  = NULL; 
    Tuner->tuner_GetStatus     = NULL;
    Tuner->tuner_IsTunerLocked = NULL;   
    Tuner->tuner_SetBandWidth  = NULL;          

    Tuner->tuner_ioaccess = NULL;
    Tuner->tuner_ioctl    = NULL;

#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
        STTBX_Print(("<"));
#endif


       STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);


#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
        STTBX_Print((">"));
#endif

    Installed = FALSE;

#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
        STTBX_Print(("ok\n"));
#endif

    return(Error);
}
   


/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */


   
/* ----------------------------------------------------------------------------
Name: tuner_none_Init()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_none_Init(ST_DeviceName_t *DeviceName, TUNER_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
   const char *identity = "STTUNER tnone.c tuner_none_Init()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
    STTBX_Print(("%s initalized (%s) ok\n", identity, DeviceName));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: tuner_none_Term()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_none_Term(ST_DeviceName_t *DeviceName, TUNER_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
   const char *identity = "STTUNER tnone.c tuner_none_Term()";
#endif


#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
    STTBX_Print(("%s terminated (%s) ok\n", identity, DeviceName));
#endif
    return(ST_NO_ERROR);
} 

/* ----------------------------------------------------------------------------
Name: tuner_none_Open()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_none_Open (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t  *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
   const char *identity = "STTUNER tnone.c tuner_none_Open()";
#endif

    index_tuner++;
   *Handle = index_tuner;

#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
    STTBX_Print(("%s opened (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   
   



/* ----------------------------------------------------------------------------
Name: tuner_none_Close()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_none_Close(TUNER_Handle_t Handle, TUNER_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
   const char *identity = "STTUNER tnone.c tuner_none_Close()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
    STTBX_Print(("%s Close (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   
   



/* ----------------------------------------------------------------------------
Name: tuner_none_SetFrequency()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_none_SetFrequency (TUNER_Handle_t Handle, U32 Frequency, U32 *NewFrequency)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
   const char *identity = "STTUNER tnone.c tuner_none_SetFrequency()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
    STTBX_Print(("%s SetFrequency (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: tuner_none_GetStatus()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_none_GetStatus(TUNER_Handle_t Handle, TUNER_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
   const char *identity = "STTUNER tnone.c tuner_none_GetStatus()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
    STTBX_Print(("%s GetStatus (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: tuner_none_IsTunerLocked()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_none_IsTunerLocked(TUNER_Handle_t Handle, BOOL *Locked)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
   const char *identity = "STTUNER tnone.c tuner_none_IsTunerLocked()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
    STTBX_Print(("%s IsTunerLocked (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: tuner_none_SetBandWidth()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_none_SetBandWidth(TUNER_Handle_t Handle, U32 BandWidth, U32 *NewBandWidth)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
   const char *identity = "STTUNER tnone.c tuner_none_SetBandWidth()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
    STTBX_Print(("%s SetBandWidth (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: tuner_none_ioctl()

Description:
    access device specific low-level functions
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_none_ioctl(TUNER_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
   const char *identity = "STTUNER tnone.c tuner_none_ioctl()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
    STTBX_Print(("%s ioctl (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: tuner_none_ioaccess()

Description:
    we get called with the instance of
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_none_ioaccess(TUNER_Handle_t Handle, IOARCH_Handle_t IOHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
   const char *identity = "STTUNER tnone.c tuner_none_ioaccess()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_TNONE
    STTBX_Print(("%s ioaccess (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */



/* End of tnone.c */
