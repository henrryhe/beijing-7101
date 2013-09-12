/* ----------------------------------------------------------------------------
File Name: d0297.c (was d0299.c)

Description:

    stv0297 demod driver.

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



/* STAPI */

#include "stevt.h"
#include "sttuner.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "ioarch.h"     /* I/O for this driver */
#include "ioreg.h"      /* I/O for this driver */

#include "reg0297.h"    /* register mappings for the stv0297 */
#include "drv0297.h"    /* misc driver functions */
#include "d0297.h"      /* header for this file */

#include "cioctl.h"     /* data structure typedefs for all the the cable ioctl functions */


/* Private types/constants ------------------------------------------------ */

/* Device capabilities */
#define STV0297_MAX_AGC                     1023
#define STV0297_MAX_SIGNAL_QUALITY          100

/* Device ID */
#define STV0297_DEVICE_VERSION              2           /* Latest Version of STV0297 */
#define STV0297_SYMBOLMIN                   870000;     /* # 1  MegaSymbols/sec */
#define STV0297_SYMBOLMAX                   11700000;   /* # 11 MegaSymbols/sec */

/* private variables ------------------------------------------------------- */

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */

static BOOL        Installed = FALSE;

U16 Address[]=
   {
   	0x00,
0x01,
0x03,
0x04,
0x07,
0x08,
0x20,
0x21,
0x22,
0x23,
0x24,
0x25,
0x30,
0x31,
0x32,
0x33,
0x34,
0x35,
0x36,
0x37,
0x38,
0x40,
0x41,
0x42,
0x43,
0x44,
0x45,
0x46,
0x49,
0x4A,
0x4B,
0x52,
0x53,
0x55,
0x56,
0x57,
0x58,
0x59,
0x5A,
0x5B,
0x60,
0x61,
0x62,
0x63,
0x64,
0x65,
0x66,
0x67,
0x68,
0x69,
0x6A,
0x6B,
0x70,
0x71,
0x72,
0x73,
0x74,
0x80,
0x81,
0x82,
0x83,
0x84,
0x85,
0x86,
0x87,
0x88,
0x89,
0x90,
0x91,
0xA0,
0xA1,
0xA2,
0xB0,
0xB1,
0xC0,
0xC1,
0xC2,
0xD0,
0xD1,
0xD2,
0xD3,
0xD4,
0xD5,
0xDE,
0xDF,
   };
   
U8 DefVal[]=
   {
   	 0x09,
  0x69,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x40,
  0x08,
  0xFF,
  0x33,
  0x00,
  0x44,
  0x29,
  0x33,
  0x80,
  0x00,
  0x00,
  0x1A,
  0x00,
  0x02,
  0x20,
  0x00,
  0x00,
  0x00,
  0x04,
  0x00,
  0x00,
  0x30,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x5E,
  0x04,
  0x00,
  0x38,
  0x06,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x02,
  0x00,
  0xFF,
  0x04,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x04,
  0x80,
  0x00,
  0x63,
  0x00,
  0x00,
  0x01,
  0x04,
  0x00,
  0x00,
  0x00,
  0x91,
  0x0B,
  0x43,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
     };

/* ---------- per instance of driver ---------- */
typedef struct
{
    ST_DeviceName_t           *DeviceName;           /* unique name for opening under */
    STTUNER_Handle_t          TopLevelHandle;       /* access tuner, lnb etc. using this */
    IOARCH_Handle_t           IOHandle;             /* instance access to I/O driver     */
    STTUNER_IOREG_DeviceMap_t DeviceMap;            /* stv0297 register map & data table */
    D0297_StateBlock_t        StateBlock;           /* driver search/state information   */
    ST_Partition_t            *MemoryPartition;     /* which partition this data block belongs to */
    void                      *InstanceChainPrev;   /* previous data block in chain or NULL if not */
    void                      *InstanceChainNext;   /* next data block in chain or NULL if last */

    U32                         ExternalClock;  /* External VCO */
    STTUNER_TSOutputMode_t      TSOutputMode;
    STTUNER_SerialDataMode_t    SerialDataMode;
    STTUNER_SerialClockSource_t SerialClockSource;
    STTUNER_FECMode_t           FECMode;
    U32                         StandBy_Flag; /*** To take care same Standby Mode doesnot
                                                   execute more than once ***/
 }
D0297_InstanceData_t;

/* instance chain, the default boot value is invalid, to catch errors */
static D0297_InstanceData_t *InstanceChainTop = (D0297_InstanceData_t *)0x7fffffff;




/* functions --------------------------------------------------------------- */

/* API */
ST_ErrorCode_t demod_d0297_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams);
ST_ErrorCode_t demod_d0297_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams);

ST_ErrorCode_t demod_d0297_Open (ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle);
ST_ErrorCode_t demod_d0297_Close(DEMOD_Handle_t  Handle, DEMOD_CloseParams_t *CloseParams);
ST_ErrorCode_t demod_d0297_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode);


ST_ErrorCode_t demod_d0297_IsAnalogCarrier (DEMOD_Handle_t Handle, BOOL *IsAnalog);
ST_ErrorCode_t demod_d0297_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber_p);
ST_ErrorCode_t demod_d0297_GetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation);
ST_ErrorCode_t demod_d0297_GetAGC          (DEMOD_Handle_t Handle, S16                  *Agc);
ST_ErrorCode_t demod_d0297_GetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t    *FECRates);
ST_ErrorCode_t demod_d0297_IsLocked        (DEMOD_Handle_t Handle, BOOL                 *IsLocked);
ST_ErrorCode_t demod_d0297_SetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t     FECRates);
ST_ErrorCode_t demod_d0297_Tracking        (DEMOD_Handle_t Handle, BOOL ForceTracking,   U32 *NewFrequency, BOOL *SignalFound);

ST_ErrorCode_t demod_d0297_ScanFrequency   (DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset,
                                                                   U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                   U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                   U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                   U32  FreqOff,          U32   ChannelBW,     S32   EchoPos);
/* access device specific low-level functions */
ST_ErrorCode_t demod_d0297_ioctl           (DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams,
                                             STTUNER_Da_Status_t *Status);

/* repeater/passthrough port for other drivers to use, type: STTUNER_IOARCH_RedirFn_t */
ST_ErrorCode_t demod_d0297_ioaccess         (DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle,
                                             STTUNER_IOARCH_Operation_t Operation, U16 SubAddr,
                                             U8 *Data, U32 TransferSize, U32 Timeout);

/* local functions --------------------------------------------------------- */

D0297_InstanceData_t *D0297_GetInstFromHandle(DEMOD_Handle_t Handle);

/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0297_Install()

Description:
    install a cable device driver into the demod database.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0297_Install(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
   const char *identity = "STTUNER d0297.c STTUNER_DRV_DEMOD_STV0297_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail driver already installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    STTBX_Print(("%s installing cable:demod:STV0297...", identity));
#endif

    /* mark ID in database */
    Demod->ID = STTUNER_DEMOD_STV0297;

    /* map API */
    Demod->demod_Init = demod_d0297_Init;
    Demod->demod_Term = demod_d0297_Term;

    Demod->demod_Open  = demod_d0297_Open;
    Demod->demod_Close = demod_d0297_Close;

    Demod->demod_IsAnalogCarrier  = demod_d0297_IsAnalogCarrier;
    Demod->demod_GetSignalQuality = demod_d0297_GetSignalQuality;
    Demod->demod_GetModulation    = demod_d0297_GetModulation;
    Demod->demod_GetAGC           = demod_d0297_GetAGC;
    Demod->demod_GetFECRates      = demod_d0297_GetFECRates;
    Demod->demod_IsLocked         = demod_d0297_IsLocked ;
    Demod->demod_SetFECRates      = demod_d0297_SetFECRates;
    Demod->demod_Tracking         = demod_d0297_Tracking;
    Demod->demod_ScanFrequency    = demod_d0297_ScanFrequency;
    Demod->demod_StandByMode      = demod_d0297_StandByMode;
    Demod->demod_ioaccess = demod_d0297_ioaccess;
    Demod->demod_ioctl    = demod_d0297_ioctl;

    InstanceChainTop = NULL;
    

    Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);


    Installed = TRUE;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0297_UnInstall()

Description:
    install a cable device driver into the demod database.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0297_UnInstall(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
   const char *identity = "STTUNER d0297.c STTUNER_DRV_DEMOD_STV0297_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Demod->ID != STTUNER_DEMOD_STV0297)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail incorrect driver type\n", identity));
#endif
        return(STTUNER_ERROR_ID);
    }

    /* has all memory been freed, by Term() */
    if(InstanceChainTop != NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail at least one instance not terminated\n", identity));
#endif
        return(ST_ERROR_OPEN_HANDLE);
    }


#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    STTBX_Print(("%s uninstalling cable:demod:STV0297...", identity));
#endif

    /* mark ID in database */
    Demod->ID = STTUNER_NO_DRIVER;

    /* unmap API */
    Demod->demod_Init = NULL;
    Demod->demod_Term = NULL;

    Demod->demod_Open  = NULL;
    Demod->demod_Close = NULL;

    Demod->demod_IsAnalogCarrier  = NULL;
    Demod->demod_GetSignalQuality = NULL;
    Demod->demod_GetModulation    = NULL;
    Demod->demod_GetAGC           = NULL;
    Demod->demod_GetFECRates      = NULL;
    Demod->demod_IsLocked         = NULL;
    Demod->demod_SetFECRates      = NULL;
    Demod->demod_Tracking         = NULL;
    Demod->demod_ScanFrequency    = NULL;
    Demod->demod_StandByMode      = NULL;
    Demod->demod_ioaccess = NULL;
    Demod->demod_ioctl    = NULL;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("<"));
#endif


        STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print((">"));
#endif

    InstanceChainTop = (D0297_InstanceData_t *)0x7ffffffe;
    Installed        = FALSE;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */



/* ----------------------------------------------------------------------------
Name: demod_d0297_Init()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    const char *identity = "STTUNER d0297.c demod_d0297_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0297_InstanceData_t *InstanceNew, *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
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
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull(DeviceName);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail DeviceName not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( D0297_InstanceData_t ));
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
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
    InstanceNew->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
    InstanceNew->DeviceMap.Registers = DEF0297_NBREG;
    InstanceNew->DeviceMap.Fields    = DEF0297_NBFIELD;
    InstanceNew->DeviceMap.Mode      = IOREG_MODE_SUBADR_8; /* NEW as of 3.4.0: i/o addressing mode to use */
    InstanceNew->DeviceMap.MemoryPartition = InitParams->MemoryPartition;
    InstanceNew->InstanceChainNext   = NULL; /* always last in the chain */

    InstanceNew->ExternalClock     = (InitParams->ExternalClock)/1000;          /* Unit KHz */
    InstanceNew->TSOutputMode      = InitParams->TSOutputMode;
    InstanceNew->SerialDataMode    = InitParams->SerialDataMode;
    InstanceNew->SerialClockSource = InitParams->SerialClockSource;
    InstanceNew->FECMode           = InitParams->FECMode;

    /* reserve memory for register mapping */
    Error = STTUNER_IOREG_Open(&InstanceNew->DeviceMap);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail setup new register database\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* install a mapping */
    Error = Reg0297_Install(&InstanceNew->DeviceMap);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail install mapping\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* set misc params for mapping */
    Error = Reg0297_Open(&InstanceNew->DeviceMap, InstanceNew->ExternalClock);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail setup register database params\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* set drv specific parameters */
    Drv0297_InitParams(&InstanceNew->DeviceMap, InstanceNew->IOHandle);

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes)\n", identity, InstanceNew->DeviceName, (U32)InstanceNew, sizeof( D0297_InstanceData_t ) ));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297_Term()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
   const char *identity = "STTUNER d0297.c demod_d0297_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0297_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
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
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }


    /* reap next matching DeviceName */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    STTBX_Print(("%s looking for first free handle[\n", identity));
#endif

    Instance = InstanceChainTop;
    while(1)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        if ( strcmp( (char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
            STTBX_Print(("]\n"));
#endif
            Error = STTUNER_IOREG_Close(&Instance->DeviceMap);
            if (Error != ST_NO_ERROR)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
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
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
            STTBX_Print(("%s terminated ok\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL)
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
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


#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297_Open()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297_Open(ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
   const char *identity = "STTUNER d0297.c demod_d0297_Open()";
#endif
    ST_ErrorCode_t          Error = ST_NO_ERROR;
    U8                      Version;
    D0297_InstanceData_t    *Instance;
    STTUNER_InstanceDbase_t *Inst;


    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
#endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print((" found ok\n"));
#endif

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    STTBX_Print(("%s using block at 0x%08x\n", identity, (U32)Instance));
#endif

    /* check handle IS actually free */
    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail driver instance already open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    /* now got pointer to free (and valid) data block */

    Inst = STTUNER_GetDrvInst();    /* pointer to instance database */

    /*
    --- wake up chip if in standby, Reset
    */
    /*  SOFT_RESET */
    Error = STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle,  R0297_CTRL_0, 1);
   
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail SOFT_RESET\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }
    Error = STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle,  R0297_CTRL_0, 0);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail SOFT_RESET\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }
    /* RESET_DI */
    Error = STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle,  R0297_CTRL_1, 1);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail RESET_DI\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }
    Error = STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle,  R0297_CTRL_1, 0);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail RESET_DI\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /*
    --- Get chip ID
    */
    Version = Reg0297_GetSTV0297Id(&Instance->DeviceMap, Instance->IOHandle);
    if ( Version !=  STV0297_DEVICE_VERSION)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail device not found (0x%02x)\n", identity, Version));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s device found, release/revision=%u\n", identity, Version));
#endif
    }

    /* reset all chip registers */
    Error = STTUNER_IOREG_Reset(&Instance->DeviceMap, Instance->IOHandle, DefVal, Address);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail reset device\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* Set capabilties */
    Capability->FECAvail        = 0;
    Capability->ModulationAvail = STTUNER_MOD_QAM    |
                                  STTUNER_MOD_16QAM  |
                                  STTUNER_MOD_32QAM  |
                                  STTUNER_MOD_64QAM  |
                                  STTUNER_MOD_128QAM |
                                  STTUNER_MOD_256QAM;   /* direct mapping to STTUNER_Modulation_t */

    Capability->J83Avail        = STTUNER_J83_A |
                                  STTUNER_J83_C;

    Capability->AGCControl      = FALSE;
    Capability->SymbolMin       = STV0297_SYMBOLMIN;        /*   1 MegaSymbols/sec (e.g) */
    Capability->SymbolMax       = STV0297_SYMBOLMAX;        /*  11 MegaSymbols/sec (e.g) */

    Capability->BerMax           = 0;
    Capability->SignalQualityMax = STV0297_MAX_SIGNAL_QUALITY;
    Capability->AgcMax           = STV0297_MAX_AGC;

    /* Setup stateblock */
    Instance->StateBlock.ScanMode     = DEF0297_CHANNEL;
    Instance->StateBlock.lastAGC2Coef = 1;
    Instance->StateBlock.Ber[0] = 0;
    Instance->StateBlock.Ber[1] = 50;                       /* Default Ratio for BER */
    Instance->StateBlock.Ber[2] = 0;
    Instance->StateBlock.CNdB = 0;
    Instance->StateBlock.SignalQuality = 0;

    /* TS output mode */
    switch (Instance->TSOutputMode)
    {
         case STTUNER_TS_MODE_SERIAL:
            /*
            --- For Eval 5518 set jumpers to
            ---     JP08    off Serial
            ---     JP18    off FrontEnd
            ---     JP10    on  Inv TS/CLK
            ---     JP09    on  NotCLK
            ---     JP26    on  D0/D7
            */
         
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_OUTFORMAT_0, 0x47);
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_OUTFORMAT_1, 0x00);
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_OUTFORMAT_2, 0x00);
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_CTRL_4, 0x22);
            break;

        case STTUNER_TS_MODE_DVBCI:
            /*
            --- For Eval 5518 set jumpers to
            ---     JP08    on  Serial
            ---     JP18    off FrontEnd
            ---     JP10    off Inv TS/CLK
            ---     JP09    on  NotCLK
            ---     JP26    on  D0/D7
            */
          
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_OUTFORMAT_0, 0x43);
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_OUTFORMAT_1, 0x40);
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_OUTFORMAT_2, 0x12);
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_CTRL_4, 0x00);      /* ckx2dis = 0, ckx2sel = 0 */
            break;

        case STTUNER_TS_MODE_PARALLEL:
        case STTUNER_TS_MODE_DEFAULT:
        default:
            /*
            --- For Eval 5512 set jumpers to
            ---     JP26    off
            ---     JP27    on
            ---     JP28    on
            ---     JP29    on
            */
            /*
            --- For Eval 5518 set jumpers to
            ---     JP08    on  Serial
            ---     JP18    off FrontEnd
            ---     JP10    off Inv TS/CLK
            ---     JP09    on  NotCLK
            ---     JP26    on  D0/D7
            */
          
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_OUTFORMAT_0, 0x43);
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_OUTFORMAT_1, 0x00);
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_OUTFORMAT_2, 0x00);
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_CTRL_4, 0x04);
            break;
    }

    /* Set default error count mode, bit error rate */
    STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0297_BERT_ON, 0); /* Ber count Off */
    STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0297_ERR_MODE, 0);

    /* finally (as nor more errors to check for, allocate handle */
    Instance->TopLevelHandle = OpenParams->TopLevelHandle;
    *Handle = (U32)Instance;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    STTBX_Print(("%s opened ok\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    Error |= Instance->DeviceMap.Error;
   

    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297_Close()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297_Close(DEMOD_Handle_t Handle, DEMOD_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
   const char *identity = "STTUNER d0297.c demod_d0297_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0297_InstanceData_t     *Instance;

    /* private driver instance data */
    Instance = D0297_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    STTBX_Print(("%s closed\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297_StandByMode()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */



ST_ErrorCode_t demod_d0297_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode)
{
	
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
   const char *identity = "STTUNER d0297.c demod_d0297_StandByMode()";
#endif
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
      
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297_IsAnalogCarrier()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297_IsAnalogCarrier(DEMOD_Handle_t Handle, BOOL *IsAnalog)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: demod_d0297_GetSignalQuality()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber_p)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    const char *identity = "STTUNER d0297.c demod_d0297_GetSignalQuality()";
    clock_t                     time_start, time_end;
#endif
    ST_ErrorCode_t              Error = ST_NO_ERROR;
    STTUNER_tuner_instance_t    *TunerInstance;
    STTUNER_InstanceDbase_t     *Inst;
    D0297_InstanceData_t        *Instance;
    int                         Mean, CNdB100;
    int                         ApplicationBERCount;
    BOOL                        IsLocked;

    /*
    --- Set parameters
    */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    time_start = STOS_time_now();
#endif
    *SignalQuality_p = 0;
    *Ber_p = 0;
    Mean = 0;
    CNdB100 = 0;
    ApplicationBERCount = 0;    /* Bit Error counter */


    /* private driver instance data */
    Instance = D0297_GetInstFromHandle(Handle);
    /* top level public instance data */
    Inst = STTUNER_GetDrvInst();
    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[Instance->TopLevelHandle].Cable.Tuner;

    /*
    --- Get Carrier And Data (TS) status
    */
    Error = demod_d0297_IsLocked(Handle, &IsLocked);
    if ( Error == ST_NO_ERROR && IsLocked )
    {
        /*
        --- Read Blk Counter
        */
        Reg0297_StopBlkCounter(&Instance->DeviceMap, Instance->IOHandle);

        /*
        --- Read noise estimations and BER
        */
        Driv0297BertCount(&Instance->DeviceMap, Instance->IOHandle, &Instance->StateBlock,
                            TunerInstance, &ApplicationBERCount, Instance->StateBlock.Ber);
        Driv0297CNEstimator(&Instance->DeviceMap, Instance->IOHandle, &Instance->StateBlock,
                            TunerInstance, &Mean, &CNdB100, &Instance->StateBlock.SignalQuality);

        /*
        --- Restart Blk Counter
        */
        Reg0297_StartBlkCounter(&Instance->DeviceMap, Instance->IOHandle);

        /*
        --- Set results
        */
        *SignalQuality_p = Instance->StateBlock.SignalQuality;
        if ( Instance->StateBlock.Ber[2] == 0 && ApplicationBERCount != 0 )
        {
            *Ber_p = ApplicationBERCount;           /* Set BER to counter (no time to count) */
        }
        else
        {
            *Ber_p = Instance->StateBlock.Ber[2];
        }
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    time_end = STOS_time_now();
    STTBX_Print(("%s Mean=%d CN=%d dBx100 (%d %%) (%u ms)\n", identity,
                Mean, CNdB100, Instance->StateBlock.SignalQuality,
                ((STOS_time_minus(time_end, time_start))*1000)/ST_GetClocksPerSecond()
                ));
/*
    STTBX_Print(("%s BERCount=%d BER[0..2] (cnt %d sat %d rate %d 10E-6)\n", identity,
                ApplicationBERCount,
                Instance->StateBlock.Ber[0],
                Instance->StateBlock.Ber[1],
                Instance->StateBlock.Ber[2]));
*/
    STTBX_Print(("%s SignalQuality %u %% Ber %u 10E-6\n", identity, *SignalQuality_p, *Ber_p));
    STTBX_Print(("%s BlkCounter %9d CorrBlk %9d UncorrBlk %9d\n", identity,
                Instance->StateBlock.BlkCounter,
                Instance->StateBlock.CorrBlk,
                Instance->StateBlock.UncorrBlk
                ));
#endif

    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297_GetModulation()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297_GetModulation(DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation)
{
/*
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
   const char *identity = "STTUNER d0297.c demod_d0297_GetModulation()";
#endif
*/
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0297_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0297_GetInstFromHandle(Handle);

    /* This implementation of the DEMOD device only supports one type of modulation */
    *Modulation = Reg0297_GetQAMSize(&Instance->DeviceMap, Instance->IOHandle);

/*
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    STTBX_Print(("%s Modulation=%u\n", identity, *Modulation));
#endif
*/
    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297_GetAGC()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297_GetAGC(DEMOD_Handle_t Handle, S16  *Agc)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
   const char *identity = "STTUNER d0297.c demod_d0297_GetAGC()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0297_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0297_GetInstFromHandle(Handle);

    *Agc   = (S16)(0xFFFF & Reg0297_GetAGC(&Instance->DeviceMap, Instance->IOHandle));

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    STTBX_Print(("%s Agc=%u\n", identity, *Agc));
#endif

    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297_GetFECRates()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297_GetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t *FECRates)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297_IsLocked()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297_IsLocked(DEMOD_Handle_t Handle, BOOL *IsLocked)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
   const char *identity = "STTUNER d0297.c demod_d0297_IsLocked()";
#endif
    ST_ErrorCode_t          Error = ST_NO_ERROR;
    U8                      Version;
    D0297_InstanceData_t    *Instance;
    D0297_SignalType_t      SignalType;

    /* private driver instance data */
    Instance = D0297_GetInstFromHandle(Handle);

    *IsLocked = FALSE;

    /*
    --- Get Version Id (In fact to test I2C Access
    */
    Version = Reg0297_GetSTV0297Id(&Instance->DeviceMap, Instance->IOHandle);
    if ( Version !=  STV0297_DEVICE_VERSION)
    {
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    /*
    --- Get Information from demod
    */
    SignalType = Drv0297_CheckAgc(&Instance->DeviceMap, Instance->IOHandle, &Instance->StateBlock.Params);
    if (SignalType == E297_AGCOK)
    {
        SignalType = Drv0297_CheckData(&Instance->DeviceMap, Instance->IOHandle, &Instance->StateBlock.Params);
        if (SignalType == E297_DATAOK)
        {
            Instance->StateBlock.Result.SignalType = E297_LOCKOK;
            *IsLocked = TRUE;
        }
        else
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
            STTBX_Print(("%s FRONT END IS UNLOCKED !!!! NO DATA\n", identity));
#endif
            /*
            --- Update Satus
            */
            Instance->StateBlock.Result.SignalType = SignalType;
        }
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s FRONT END IS UNLOCKED !!!! NO AGC\n", identity));
#endif
        /*
        --- Update Satus
        */
        Instance->StateBlock.Result.SignalType = SignalType;
    }

/*
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    STTBX_Print(("%s TUNER IS %s\n", identity, (*IsLocked)?"LOCKED":"UNLOCKED"));
#endif
*/

    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297_SetFECRates()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297_SetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t FECRates)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297_Tracking()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297_Tracking(DEMOD_Handle_t Handle, BOOL ForceTracking, U32 *NewFrequency, BOOL *SignalFound)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
   const char *identity = "STTUNER d0297.c demod_d0297_Tracking()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    STTUNER_tuner_instance_t    *TunerInstance;     /* don't really need all these variables, done for clarity */
    STTUNER_InstanceDbase_t     *Inst;              /* pointer to instance database */
    STTUNER_Handle_t            TopHandle;          /* instance that contains the demos, tuner, lnb & diseqc driver set */
    D0297_InstanceData_t        *Instance;
    BOOL                        IsLocked, DataFound;

    /* private driver instance data */
    Instance = D0297_GetInstFromHandle(Handle);
    /* top level public instance data */
    Inst = STTUNER_GetDrvInst();

    /* this driver knows what to level instance it belongs to */
    TopHandle = Instance->TopLevelHandle;

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[TopHandle].Cable.Tuner;

    /*
    --- Get Carrier And Data (TS) status
    */
    Error = demod_d0297_IsLocked(Handle, &IsLocked);
    if ( Error == ST_NO_ERROR && IsLocked )
    {
        /*
        --- Tuner Is Locked
        */
        *SignalFound  = (Instance->StateBlock.Result.SignalType == E297_LOCKOK)? TRUE : FALSE;
        *NewFrequency = 1000 * ((U32)Instance->StateBlock.Result.Frequency);
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s FRONT END IS LOCKED on %d Hz (CARRIER and DATA)\n", identity, *NewFrequency));
#endif
    }
    else
    {
        /*
        --- Tuner is UnLocked => Search Carrier and Data
        */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s FRONT END IS UNLOCKED ==> SEARCH CARRIER AND DATA\n", identity));
#endif
        DataFound = Driv0297CarrierSearch(&Instance->DeviceMap, Instance->IOHandle, &Instance->StateBlock, TunerInstance);
        Error |= Instance->DeviceMap.Error;
        Instance->DeviceMap.Error = ST_NO_ERROR;
        if (Error != ST_NO_ERROR)
        {
            return(Error);
        }

        if (DataFound)
        {
            *SignalFound  = (Instance->StateBlock.Result.SignalType == E297_LOCKOK)? TRUE : FALSE;
            *NewFrequency = 1000 * ((U32)Instance->StateBlock.Result.Frequency);
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
            STTBX_Print(("%s DATA on %d Hz\n", identity, *NewFrequency));
#endif
        }
        else
        {
            /*
            --- Tuner Is UnLocked
            */
            *SignalFound  = FALSE;
            *NewFrequency = 1000 * ((U32)Instance->StateBlock.Result.Frequency);
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
            STTBX_Print(("%s NO DATA on %d Hz !!!!\n", identity, *NewFrequency));
#endif
        }
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    STTBX_Print(("%s %s, on %u Hz\n", identity, (*SignalFound)?"SIGNAL FOUND":"NO SIGNAL DOUND", *NewFrequency));
#endif

    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297_ScanFrequency()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297_ScanFrequency(DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset,
                                                                U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                U32  FreqOff,          U32   ChannelBW,     S32   EchoPos)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    const char *identity = "STTUNER d0297.c demod_d0297_ScanFrequency()";
    clock_t                     time_start_lock, time_end_lock;
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_tuner_instance_t    *TunerInstance;
    STTUNER_InstanceDbase_t     *Inst;
    D0297_InstanceData_t        *Instance;
    BOOL                        DataFound=FALSE;

    /* private driver instance data */
    Instance = D0297_GetInstFromHandle(Handle);
    /* top level public instance data */
    Inst = STTUNER_GetDrvInst();

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[Instance->TopLevelHandle].Cable.Tuner;

    #ifndef ST_OSLINUX
    STTUNER_task_lock();
    #endif
    /*
    --- Spectrum inversion is set to
    */
    switch(Spectrum)
    {
        case STTUNER_INVERSION_NONE :
        case STTUNER_INVERSION :
        case STTUNER_INVERSION_AUTO :
            break;
        default:
            Inst[Instance->TopLevelHandle].Cable.SingleScan.Spectrum = STTUNER_INVERSION_NONE;
            break;
    }

    /*
    --- Modulation type is set to
    */
    switch(Inst[Instance->TopLevelHandle].Cable.Modulation)
    {
        case STTUNER_MOD_16QAM :
        case STTUNER_MOD_32QAM :
        case STTUNER_MOD_64QAM :
        case STTUNER_MOD_128QAM :
        case STTUNER_MOD_256QAM :
            break;
        case STTUNER_MOD_QAM :
        default:
            Inst[Instance->TopLevelHandle].Cable.Modulation = STTUNER_MOD_64QAM;
            break;
    }

    /*
    --- Start Init
    */
    Drv0297_InitSearch(TunerInstance, &Instance->StateBlock,
                       Inst[Instance->TopLevelHandle].Cable.Modulation,
                       InitialFrequency,
                       SymbolRate,
                       Spectrum,
                       Inst[Instance->TopLevelHandle].Cable.ScanExact);

    /*
    --- Try to lock
    */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    time_start_lock = STOS_time_now();
#endif
    DataFound = Driv0297CarrierSearch(&Instance->DeviceMap,Instance->IOHandle, &Instance->StateBlock, TunerInstance);
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    time_end_lock = STOS_time_now();
#endif
    if (DataFound)
    {    
    	/*for Bug GNBvd54174 & GNBvd59268*/
    	if(Instance->TSOutputMode==STTUNER_TS_MODE_SERIAL)
    	{
    	    Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_OUTFORMAT_0, 0x47);
    	    Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_OUTFORMAT_1, 0x00);
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_OUTFORMAT_2, 0x00);
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_CTRL_4, 0x22);
    	}
        if(Instance->TSOutputMode==STTUNER_TS_MODE_DVBCI)
        {
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_OUTFORMAT_0, 0x43);
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_OUTFORMAT_1, 0x40);
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_OUTFORMAT_2, 0x12);
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_CTRL_4, 0x00);      /* ckx2dis = 0, ckx2sel = 0 */
        }  
        if(Instance->TSOutputMode==STTUNER_TS_MODE_PARALLEL)
        {
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_OUTFORMAT_0, 0x43);
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_OUTFORMAT_1, 0x00);
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_OUTFORMAT_2, 0x00);
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_CTRL_4, 0x04);
            
        }
        if(Instance->TSOutputMode==STTUNER_TS_MODE_DEFAULT)
        {  
          
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_OUTFORMAT_0, 0x43);
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_OUTFORMAT_1, 0x00);
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_OUTFORMAT_2, 0x00);
            Error |= STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297_CTRL_4, 0x04);
         
        }      			
    	/**********************/		
        /*Instance->StateBlock.Result.SignalType = E297_LOCKOK;*/
        /* Pass new frequency to caller */
        *NewFrequency = 1000 * ((U32)Instance->StateBlock.Result.Frequency);
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s  DATA FOUND on %u Hz\n", identity, *NewFrequency));
#endif
        *ScanSuccess = TRUE;
    }
    else
    {
        Instance->StateBlock.Result.SignalType = E297_NODATA;
        *NewFrequency = InitialFrequency;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s NO DATA on %u Hz\n", identity, *NewFrequency));
#endif
        *ScanSuccess = FALSE;
    }
    #ifndef ST_OSLINUX
    STTUNER_task_unlock();
    #endif

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    switch(Inst[Instance->TopLevelHandle].Cable.Modulation)
    {
        case STTUNER_MOD_16QAM :
            STTBX_Print(("%s QAM 16 : ", identity));
            break;
        case STTUNER_MOD_32QAM :
            STTBX_Print(("%s QAM 32 : ", identity));
            break;
        case STTUNER_MOD_64QAM :
            STTBX_Print(("%s QAM 64 : ", identity));
            break;
        case STTUNER_MOD_128QAM :
            STTBX_Print(("%s QAM 128 : ", identity));
            break;
        case STTUNER_MOD_256QAM :
            STTBX_Print(("%s QAM 256 : ", identity));
            break;
        case STTUNER_MOD_QAM :
            STTBX_Print(("%s QAM : ", identity));
            break;
    }
    STTBX_Print(("SR %d S/s TIME >> TUNER + DEMOD (%u ticks)(%u ms)\n", SymbolRate,
                STOS_time_minus(time_end_lock, time_start_lock),
                ((STOS_time_minus(time_end_lock, time_start_lock))*1000)/ST_GetClocksPerSecond()
                ));
#endif

    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}

/* ----------------------------------------------------------------------------
Name: demod_d0297_ioctl()

Description:
    access device specific low-level functions

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297_ioctl(DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
   const char *identity = "STTUNER d0297.c demod_d0297_ioctl()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0297_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0297_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s demod driver instance handle=%d\n", identity, Handle));
        STTBX_Print(("%s                     function=%d\n", identity, Function));
        STTBX_Print(("%s                   I/O handle=%d\n", identity, Instance->IOHandle));
#endif

    /* ---------- select what to do ---------- */
    switch(Function){

        case STTUNER_IOCTL_REG_IN: /* read device register */
            *(int *)OutParams = STTUNER_IOREG_GetRegister(&Instance->DeviceMap, Instance->IOHandle, *(int *)InParams);
            break;

        case STTUNER_IOCTL_REG_OUT: /* write device register */
            Error =  STTUNER_IOREG_SetRegister(  &Instance->DeviceMap,
                                                  Instance->IOHandle,
                  ((CABIOCTL_SETREG_InParams_t *)InParams)->RegIndex,
                  ((CABIOCTL_SETREG_InParams_t *)InParams)->Value );
            break;

        default:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
    STTBX_Print(("%s called ok\n", identity, Function));    /* signal that function came back */
#endif

    Error |= Instance->DeviceMap.Error; /* Also accumulate I2C error */
	Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);

}



/* ----------------------------------------------------------------------------
Name: demod_d0297_ioaccess()

Description:
    called from ioarch.c when some driver does I/O but with the repeater/passthru
    set to point to this function.
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297_ioaccess(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
   const char *identity = "STTUNER d0297.c demod_d0297_ioaccess()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    IOARCH_Handle_t ThisIOHandle;
    D0297_InstanceData_t *Instance;

    Instance = D0297_GetInstFromHandle(Handle);
  
    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
    
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* this (demod) drivers I/O handle */
    ThisIOHandle = Instance->IOHandle;

    /* if STTUNER_IOARCH_MAX_HANDLES then passthrough function required using our device handle/address */
    if (IOHandle == STTUNER_IOARCH_MAX_HANDLES)
    {
/*
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s write passthru\n", identity));
#endif
*/     
        Error = STTUNER_IOARCH_ReadWrite(ThisIOHandle, Operation, SubAddr, Data, TransferSize, Timeout);
    }
    else    /* repeater */
    {
/*
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297
        STTBX_Print(("%s write repeater\n", identity));
#endif
*/      
        #ifndef ST_OSLINUX
        STTUNER_task_lock();    /* prevent any other activity on this bus (no task must preempt us) */
        #endif
        #ifdef PROCESSOR_C1 /***For C1 core this delay needed otherwise STTUNER_task_lock(); not working fine ****/
        /*task_delay(5);*/
        #endif
        /* enable repeater then send the data  using I/O handle supplied through ioarch.c */
        Error = STTUNER_IOREG_SetField(& Instance->DeviceMap, ThisIOHandle, F0297_I2CT_EN, 1);
        if ( Error == ST_NO_ERROR )
        {
        	
            /*
            --- Send/Receive Data(s) to target at SubAddr
            */
            Error = STTUNER_IOARCH_ReadWriteNoRep(IOHandle, Operation, SubAddr, Data, TransferSize, Timeout*100);
        }
        #ifndef ST_OSLINUX
        STTUNER_task_unlock();
        #endif
    }

    return(Error);
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */

D0297_InstanceData_t *D0297_GetInstFromHandle(DEMOD_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297_HANDLES
   const char *identity = "STTUNER d0297.c D0297_GetInstFromHandle()";
#endif
    D0297_InstanceData_t *Instance;

    Instance = (D0297_InstanceData_t *)Handle;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297_HANDLES
    STTBX_Print(("%s block at 0x%08x\n", identity, Instance));
#endif

    return(Instance);
}




/* End of d0297.c */
