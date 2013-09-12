/* ----------------------------------------------------------------------------
File Name: dnone.c

Description: 

    null demod driver.


Copyright (C) 1999-2001 STMicroelectronics

History:
 
   date: 19-June-2001
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

#include "sttbx.h"
               

#include "stevt.h"
#include "sttuner.h"                    

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "ioarch.h"     /* I/O for this driver */

#include "dnone.h"      /* header for this file */



/* private variables ------------------------------------------------------- */

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */

static BOOL        Installed = FALSE;
static int index_demod = 0;

/* functions --------------------------------------------------------------- */

/* API */
ST_ErrorCode_t demod_dnone_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams);
ST_ErrorCode_t demod_dnone_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams);

ST_ErrorCode_t demod_dnone_Open (ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle);
ST_ErrorCode_t demod_dnone_Close(DEMOD_Handle_t  Handle, DEMOD_CloseParams_t *CloseParams);

ST_ErrorCode_t demod_dnone_IsAnalogCarrier (DEMOD_Handle_t Handle, BOOL *IsAnalog);        
ST_ErrorCode_t demod_dnone_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber);
ST_ErrorCode_t demod_dnone_GetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation);
ST_ErrorCode_t demod_dnone_GetAGC          (DEMOD_Handle_t Handle, S16  *Agc);
ST_ErrorCode_t demod_dnone_GetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t *FECRate);
ST_ErrorCode_t demod_dnone_IsLocked        (DEMOD_Handle_t Handle, BOOL *IsLocked);
ST_ErrorCode_t demod_dnone_SetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t FECRates);
ST_ErrorCode_t demod_dnone_Tracking        (DEMOD_Handle_t Handle, BOOL ForceTracking,   U32 *NewFrequency, BOOL *SignalFound);

ST_ErrorCode_t demod_dnone_ScanFrequency   (DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset, 
                                                                   U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                   U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                   U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                   U32  FreqOff,          U32   ChannelBW,     S32   EchoPos);

/* I/O API */
ST_ErrorCode_t demod_none_ioaccess(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle,
    STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);

/* access device specific low-level functions */
ST_ErrorCode_t demod_none_ioctl(DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status);



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_NONE_Install()

Description:
    install a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_NONE_Install(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
   const char *identity = "STTUNER dnone.c STTUNER_DRV_DEMOD_NONE_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
   

    if(Installed == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
        STTBX_Print(("%s fail driver already installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
        STTBX_Print(("%s installing sat:demod:none...", identity));
#endif

    Demod->ID = STTUNER_DEMOD_NONE;

    /* map API */
    Demod->demod_Init = demod_dnone_Init;
    Demod->demod_Term = demod_dnone_Term;

    Demod->demod_Open  = demod_dnone_Open;
    Demod->demod_Close = demod_dnone_Close;

    Demod->demod_IsAnalogCarrier  = demod_dnone_IsAnalogCarrier; 
    Demod->demod_GetSignalQuality = demod_dnone_GetSignalQuality;
    Demod->demod_GetModulation    = demod_dnone_GetModulation;   
    Demod->demod_GetAGC           = demod_dnone_GetAGC;          
    Demod->demod_GetFECRates       = demod_dnone_GetFECRates;      
    Demod->demod_IsLocked         = demod_dnone_IsLocked ;       
    Demod->demod_SetFECRates      = demod_dnone_SetFECRates;     
    Demod->demod_Tracking         = demod_dnone_Tracking;        
    Demod->demod_ScanFrequency    = demod_dnone_ScanFrequency;

    Demod->demod_ioaccess = demod_none_ioaccess;
    Demod->demod_ioctl    = demod_none_ioctl;


    Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);


    Installed = TRUE;

#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
        STTBX_Print(("ok\n"));
#endif    
    return(Error);
}


   
/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_NONE_UnInstall()

Description:
    install a satellite device driver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_NONE_UnInstall(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
   const char *identity = "STLNB dnone.c STTUNER_DRV_DEMOD_NONE_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }
   
    if(Demod->ID != STTUNER_DEMOD_NONE)
    {
#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
        STTBX_Print(("%s fail incorrect driver type\n", identity));
#endif
        return(STTUNER_ERROR_ID);
    }

    Demod->ID = STTUNER_NO_DRIVER;

    /* map API */
    Demod->demod_Init = NULL;
    Demod->demod_Term = NULL;

    Demod->demod_Open  = NULL;
    Demod->demod_Close = NULL;

    Demod->demod_IsAnalogCarrier  = NULL; 
    Demod->demod_GetSignalQuality = NULL;
    Demod->demod_GetModulation    = NULL;   
    Demod->demod_GetAGC           = NULL;          
    Demod->demod_GetFECRates      = NULL;      
    Demod->demod_IsLocked         = NULL ;       
    Demod->demod_SetFECRates      = NULL;     
    Demod->demod_Tracking         = NULL;        
    Demod->demod_ScanFrequency    = NULL;

    Demod->demod_ioaccess = NULL;
    Demod->demod_ioctl    = NULL;

#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
        STTBX_Print(("<"));
#endif

        STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);

#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
        STTBX_Print((">"));
#endif

    Installed = FALSE;

#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
        STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */


/* ----------------------------------------------------------------------------
Name: demod_dnone_Init()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_dnone_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
    const char *identity = "STTUNER dnone.c demod_dnone_Init()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
    STTBX_Print(("%s initalized (%s) ok\n", identity, DeviceName));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: demod_dnone_Term()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_dnone_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
   const char *identity = "STTUNER dnone.c demod_dnone_Term()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
    STTBX_Print(("%s terminated (%s) ok\n", identity, DeviceName));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: demod_dnone_Open()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_dnone_Open (ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
   const char *identity = "STTUNER dnone.c demod_dnone_Open()";
#endif

    index_demod++;
   *Handle = index_demod;

#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
    STTBX_Print(("%s opened (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: demod_dnone_Close()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_dnone_Close(DEMOD_Handle_t Handle, DEMOD_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
   const char *identity = "STTUNER dnone.c demod_dnone_Close()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
    STTBX_Print(("%s closed (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   


/* ----------------------------------------------------------------------------
Name: demod_dnone_IsAnalogCarrier()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_dnone_IsAnalogCarrier(DEMOD_Handle_t Handle, BOOL *IsAnalog)        
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
   const char *identity = "STTUNER dnone.c demod_dnone_IsAnalogCarrier()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
    STTBX_Print(("%s IsAnalogCarrier (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: demod_dnone_GetSignalQuality()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_dnone_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
   const char *identity = "STTUNER dnone.c demod_dnone_GetSignalQuality()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
    STTBX_Print(("%s GetSignalQuality (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: demod_dnone_GetModulation()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_dnone_GetModulation(DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
   const char *identity = "STTUNER dnone.c demod_dnone_GetModulation()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
    STTBX_Print(("%s GetModulation (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: demod_dnone_GetAGC()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_dnone_GetAGC(DEMOD_Handle_t Handle, S16  *Agc)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
   const char *identity = "STTUNER dnone.c demod_dnone_GetAGC()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
    STTBX_Print(("%s GetAGC (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: demod_dnone_GetFECRates()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_dnone_GetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t *FECRate)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
   const char *identity = "STTUNER dnone.c demod_dnone_GetFECRates()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
    STTBX_Print(("%s GetFECRate (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: demod_dnone_IsLocked()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_dnone_IsLocked(DEMOD_Handle_t Handle, BOOL *IsLocked)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
   const char *identity = "STTUNER dnone.c demod_dnone_IsLocked()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
    STTBX_Print(("%s IsLocked (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: demod_dnone_SetFECRates()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_dnone_SetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t FECRates)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
   const char *identity = "STTUNER dnone.c demod_dnone_SetFECRates()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
    STTBX_Print(("%s SetFECRates (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: demod_dnone_Tracking()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_dnone_Tracking(DEMOD_Handle_t Handle, BOOL ForceTracking, U32 *NewFrequency, BOOL *SignalFound)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
   const char *identity = "STTUNER dnone.c demod_dnone_Tracking()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
    STTBX_Print(("%s Tracking (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: demod_dnone_ScanFrequency()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_dnone_ScanFrequency(DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset, 
                                                                U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                U32  FreqOff,          U32   ChannelBW,     S32   EchoPos)
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
   const char *identity = "STTUNER dnone.c demod_dnone_ScanFrequency()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
    STTBX_Print(("%s ScanFrequency (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: demod_none_ioctl()

Description:
    access device specific low-level functions
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_none_ioctl(DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
   
{
#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
   const char *identity = "STTUNER dnone.c demod_none_ioctl()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
    STTBX_Print(("%s ioctl (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   



/* ----------------------------------------------------------------------------
Name: demod_none_ioaccess()

Description:
    we get called with the instance of
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_none_ioaccess(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)

{
#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
   const char *identity = "STTUNER dnone.c demod_none_ioaccess()";
#endif

#ifdef STTUNER_DEBUG_MODULE_SHARED_DNONE
    STTBX_Print(("%s ioaccess (%d) ok\n", identity, Handle));
#endif
    return(ST_NO_ERROR);
}   




/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */



/* End of dnone.c */
