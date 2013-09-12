/* ----------------------------------------------------------------------------
File Name: d0297JJ.c (was d0297J.c)

Description:

    stv0297JJ demod driver.

Copyright (C) 1999-2001 STMicroelectronics


   date: 15-May-2002
version: 3.5.0
 author: from STV0297JJ and MB validation drivers.
comment: Write for multi-instance/multi-FrontEnd.

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



 /* Standard includes */
#include "stcommon.h"

/* STAPI */
#include "sttbx.h"
#include "stevt.h"
#include "sttuner.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "ioarch.h"     /* I/O for this driver */
#include "ioreg.h"      /* I/O for this driver */

#include "reg0297j.h"   /* register mappings for the stv0297J */
#include "drv0297j.h"   /* misc driver functions */
#include "d0297j.h"     /* header for this file */

#include "cioctl.h"     /* data structure typedefs for all the the cable ioctl functions */


/* Private types/constants ------------------------------------------------ */

/* Device capabilities */
#define STV0297J_MAX_AGC                     1023
#define STV0297J_MAX_SIGNAL_QUALITY          100

/* Device ID */
#define STV0297J_DEVICE_VERSION              0x11        /* Latest Version of STV0297J (Missing on Cut 2.0!) */
#define STV0297J_SYMBOLMIN                   870000;     /* # 1  MegaSymbols/sec */
#define STV0297J_SYMBOLMAX                   11700000;   /* # 11 MegaSymbols/sec */

/* private variables ------------------------------------------------------- */


static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */

static BOOL        Installed = false;

U16 addressarray[]=
{
0x00,
0x01,
0x03,
0x04,
0x05,
0x06,
0x07,
0x08,
0x09,
0x0A,
0x0B,
0x0C,
0x0D,
0x0E,
0x10,
0x11,
0x12,
0x14,
0x15,
0x16,
0x17,
0x18,
0x19,
0x1A,
0x1C,
0x1D,
0x20,
0x21,
0x22,
0x23,
0x24,
0x25,
0x26,
0x27,
0x28,
0x29,
0x2A,
0x2B,
0x2C,
0x2D,
0x2E,
0x2F,
0x30,
0x31,
0x32,
0x33,
0x34,
0x35,
0x38,
0x39,
0x3A,
0x3B,
0x3C,
0x3D,
0x3E,
0x3F,
0x40,
0x41,
0x42,
0x43,
0x44,
0x45,
0x48,
0x49,
0x4A,
0x4C,
0x4D,
0x4E,
0x50,
0x51,
0x52,
0x53,
0x58,
0x59,
0x5A,
0x5B,
0x5C,
0x5D,
0x5E,
0x5F,
0x60,
0x61,
0x62,
0x64,
0x65,
0x66,
0x67,
0x68,
0x69,
0x6A,
0x6B,
0x6C,
0x6D,
0x6E,
0x6F,
0x70,
0x71,
0x72,
0x73,
0x74,
0x75,
0x76,
0x77,
0x78,
0x79,
0x7A,
0x7B,
0x7C,
0x80,
0x81,
0x84,
0x85,
0x86,
0x88,
0x89,
0x8C,
0x90,
0x91,
0x94,
0x95,
0x96,
0x97,
0x98,
0x99,
0x9A,
0x9C,
0x9D,
0xA0,
0xA1,
0xA2,
0xA3,
0xA6,
0xA8,
0xA9,
0xAA,
0xAB,
0xAC,
0xAD,
0xAE,
0xAF,
0xC0,
0xC1,
0xC2,
0xC3,
0xC4,
0xC5,
0xC6,
0xC7,
0xC8,
0xC9,
0xCA,
0xCB,
0xCC,
0xCD,
0xCE,
0xD0,
0xD1,
0xD2,
0xD4,
0xD5,
0xD6,
0xD7,
0xD8,
0xD9,
0xDA,
0xDB,
0xB1,
0xB2,
0xB3,
0xB4,
0xB5,
0xB6,
0xB7,
0xB9,
0xBA,
0xBB,
0xBC,
0xBD,
0xBE,
0xBF,
0xC0,
0xC1,
0xC2,
0xC3,
0xC4,
0xC5,
0xC6,
0xC7,
0xC8,
0xC9,
0xCA,
0xCB,
0xCC,
0xCD,
0xCE,
0xCF,
0xD0,
0xD1,
0xD2,
0xD3,
0xD4,
0xD5,
0xD6,
0xD7,
0xD8,
0xD9,
0xDA,
0xDB,
0xDC,
0xDD,
0xDE,
0xDF,
0xE0,
0xE1,
0xE2,
0xE3,
0xE4,
0xE5,
0xE6,
0xE7,
0xE8,
0xE9,
0xEA,
0xEB,
0xEC,
0xED,
0xEE,
0xEF,
0xF0,
0xF1,
0xF2,
0xF3,
0xF4,
0xF5,
0xF6,
0xF7,
0xF8,
0xF9,
0xFA,
0xFB,
};

U8 defval[]=
{
    0x09,
    0x69,
    0x00,
    0x00,
    0xff,
    0x0f,
    0x00,
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
    0x00,
    0x00,
    0x00,
    0x00,
    0x02,
    0x00,
    0x20,
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
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x16,
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
    0x00,
    0x00,
    0xff,
    0x04,
    0x00,
    0x00,
    0x00,
    0x00,
    0xff,
    0xff,
    0x00,
    0x00,
    0x14,
    0x00,
    0x06,
    0x12,
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
    0x00,
    0x00,
    0xf4,
    0x80,
    0x45,
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
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x91,
    0x0b,
    0x0b,
    0x53,
    0x53,
    0x00,
    0x44,
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
    0x00,
    0xe0,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x9c,
    0x04,
    0x77,
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
    0x00,
    0x80,
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
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
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
    0x00,
};

/* ---------- per instance of driver ---------- */
typedef struct
{
    ST_DeviceName_t           *DeviceName;           /* unique name for opening under */
    STTUNER_Handle_t          TopLevelHandle;       /* access tuner, lnb etc. using this */
    IOARCH_Handle_t           IOHandle;             /* instance access to I/O driver     */
    STTUNER_IOREG_DeviceMap_t DeviceMap;            /* stv0297J register map & data table */
    D0297J_StateBlock_t        StateBlock;           /* driver search/state information   */
    ST_Partition_t            *MemoryPartition;     /* which partition this data block belongs to */
    void                      *InstanceChainPrev;   /* previous data block in chain or NULL if not */
    void                      *InstanceChainNext;   /* next data block in chain or NULL if last */

    U32                         ExternalClock;  /* External VCO */
    STTUNER_TSOutputMode_t      TSOutputMode;
    STTUNER_SerialDataMode_t    SerialDataMode;
    STTUNER_SerialClockSource_t SerialClockSource;
    STTUNER_FECMode_t           FECMode;
    STTUNER_Sti5518_t           Sti5518;        /* 297J FEC-B: TS output compatibility with Sti5518 */
    U32                         StandBy_Flag; /*** To take care same Standby Mode doesnot
                                                   execute more than once ***/
 }
D0297J_InstanceData_t;

/* instance chain, the default boot value is invalid, to catch errors */
static D0297J_InstanceData_t *InstanceChainTop = (D0297J_InstanceData_t *)0x7fffffff;

/* functions --------------------------------------------------------------- */

/* API */
ST_ErrorCode_t demod_d0297J_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams);
ST_ErrorCode_t demod_d0297J_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams);

ST_ErrorCode_t demod_d0297J_Open (ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle);
ST_ErrorCode_t demod_d0297J_Close(DEMOD_Handle_t  Handle, DEMOD_CloseParams_t *CloseParams);


ST_ErrorCode_t demod_d0297J_IsAnalogCarrier (DEMOD_Handle_t Handle, BOOL *IsAnalog);
ST_ErrorCode_t demod_d0297J_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber_p);
ST_ErrorCode_t demod_d0297J_GetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation);
ST_ErrorCode_t demod_d0297J_GetAGC          (DEMOD_Handle_t Handle, S16                  *Agc);
ST_ErrorCode_t demod_d0297J_GetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t    *FECRates);
ST_ErrorCode_t demod_d0297J_IsLocked        (DEMOD_Handle_t Handle, BOOL                 *IsLocked);
ST_ErrorCode_t demod_d0297J_SetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t     FECRates);
ST_ErrorCode_t demod_d0297J_Tracking        (DEMOD_Handle_t Handle, BOOL ForceTracking,   U32 *NewFrequency, BOOL *SignalFound);
ST_ErrorCode_t demod_d0297J_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode);

ST_ErrorCode_t demod_d0297J_ScanFrequency   (DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset,
                                                                   U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                   U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                   U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                   U32  FreqOff,          U32   ChannelBW,     S32   EchoPos);
/* access device specific low-level functions */
ST_ErrorCode_t demod_d0297J_ioctl           (DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams,
                                             STTUNER_Da_Status_t *Status);

/* repeater/passthrough port for other drivers to use, type: STTUNER_IOARCH_RedirFn_t */
ST_ErrorCode_t demod_d0297J_ioaccess         (DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle,
                                             STTUNER_IOARCH_Operation_t Operation, U16 SubAddr,
                                             U8 *Data, U32 TransferSize, U32 Timeout);

/* local functions --------------------------------------------------------- */

D0297J_InstanceData_t *D0297J_GetInstFromHandle(DEMOD_Handle_t Handle);

/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0297J_Install()

Description:
    install a cable device driver into the demod database.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0297J_Install(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
   const char *identity = "STTUNER d0297J.c STTUNER_DRV_DEMOD_STV0297J_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s fail driver already installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
    STTBX_Print(("%s installing cable:demod:STV0297J...", identity));
#endif

    /* mark ID in database */
    Demod->ID = STTUNER_DEMOD_STV0297J;

    /* map API */
    Demod->demod_Init = demod_d0297J_Init;
    Demod->demod_Term = demod_d0297J_Term;

    Demod->demod_Open  = demod_d0297J_Open;
    Demod->demod_Close = demod_d0297J_Close;
    Demod->demod_StandByMode      = demod_d0297J_StandByMode;
    Demod->demod_IsAnalogCarrier  = demod_d0297J_IsAnalogCarrier;
    Demod->demod_GetSignalQuality = demod_d0297J_GetSignalQuality;
    Demod->demod_GetModulation    = demod_d0297J_GetModulation;
    Demod->demod_GetAGC           = demod_d0297J_GetAGC;
    Demod->demod_GetFECRates      = demod_d0297J_GetFECRates;
    Demod->demod_IsLocked         = demod_d0297J_IsLocked ;
    Demod->demod_SetFECRates      = demod_d0297J_SetFECRates;
    Demod->demod_Tracking         = demod_d0297J_Tracking;
    Demod->demod_ScanFrequency    = demod_d0297J_ScanFrequency;

    Demod->demod_ioaccess = demod_d0297J_ioaccess;
    Demod->demod_ioctl    = demod_d0297J_ioctl;

    InstanceChainTop = NULL;
      

  Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);


    Installed = TRUE;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
    STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0297J_UnInstall()

Description:
    install a cable device driver into the demod database.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0297J_UnInstall(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
   const char *identity = "STTUNER d0297J.c STTUNER_DRV_DEMOD_STV0297J_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Demod->ID != STTUNER_DEMOD_STV0297J)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s fail incorrect driver type\n", identity));
#endif
        return(STTUNER_ERROR_ID);
    }

    /* has all memory been freed, by Term() */
    if(InstanceChainTop != NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s fail at least one instance not terminated\n", identity));
#endif
        return(ST_ERROR_OPEN_HANDLE);
    }


#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
    STTBX_Print(("%s uninstalling cable:demod:STV0297J...", identity));
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

    Demod->demod_ioaccess = NULL;
    Demod->demod_ioctl    = NULL;
    Demod->demod_StandByMode      = NULL;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("<"));
#endif


 STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print((">"));
#endif

    InstanceChainTop = (D0297J_InstanceData_t *)0x7ffffffe;
    Installed        = FALSE;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */



/* ----------------------------------------------------------------------------
Name: demod_d0297J_Init()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297J_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
    const char *identity = "STTUNER d0297J.c demod_d0297J_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0297J_InstanceData_t *InstanceNew, *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
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
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull(DeviceName);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s fail DeviceName not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( D0297J_InstanceData_t ));
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
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
    InstanceNew->DeviceMap.Registers = DEF0297J_NBREG;
    InstanceNew->DeviceMap.Fields    = DEF0297J_NBFIELD;
    InstanceNew->DeviceMap.MemoryPartition = InitParams->MemoryPartition;
    InstanceNew->InstanceChainNext   = NULL; /* always last in the chain */

    InstanceNew->ExternalClock     = (InitParams->ExternalClock)/1000;          /* Unit KHz */
    InstanceNew->TSOutputMode      = InitParams->TSOutputMode;
    InstanceNew->SerialDataMode    = InitParams->SerialDataMode;
    InstanceNew->SerialClockSource = InitParams->SerialClockSource;
    InstanceNew->FECMode           = InitParams->FECMode;
    InstanceNew->Sti5518           = InitParams->Sti5518;

    /* reserve memory for register mapping */
    Error = STTUNER_IOREG_Open(&InstanceNew->DeviceMap);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s fail setup new register database\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* install a mapping */
    Error = Reg0297J_Install(&InstanceNew->DeviceMap);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s fail install mapping\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* set misc params for mapping */
    Error = Reg0297J_Open(&InstanceNew->DeviceMap, InstanceNew->ExternalClock);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s fail setup register database params\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* set drv specific parameters */
    Drv0297J_InitParams(&InstanceNew->DeviceMap, InstanceNew->IOHandle);

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes)\n", identity, InstanceNew->DeviceName, (U32)InstanceNew, sizeof( D0297J_InstanceData_t ) ));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297J_Term()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297J_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
   const char *identity = "STTUNER d0297J.c demod_d0297J_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0297J_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
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
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }


    /* reap next matching DeviceName */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
    STTBX_Print(("%s looking for first free handle[\n", identity));
#endif

    Instance = InstanceChainTop;
    while(1)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        if ( strcmp( (char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
            STTBX_Print(("]\n"));
#endif
            Error = STTUNER_IOREG_Close(&Instance->DeviceMap);
            if (Error != ST_NO_ERROR)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
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
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
            STTBX_Print(("%s terminated ok\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL)
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
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


#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297J_Open()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297J_Open(ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
   const char *identity = "STTUNER d0297J.c demod_d0297J_Open()";
#endif
    ST_ErrorCode_t          Error = ST_NO_ERROR;
    U8                      Version;
    D0297J_InstanceData_t    *Instance;
    STTUNER_InstanceDbase_t *Inst;


    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
#endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print((" found ok\n"));
#endif

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
    STTBX_Print(("%s using block at 0x%08x\n", identity, (U32)Instance));
#endif

    /* check handle IS actually free */
    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
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
    /*  soft reset of the whole chip */
    Error = STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle, R0297J_CTRL_0, 1);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s fail SOFT_RESET\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }
    Error = STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle, R0297J_CTRL_0, 0);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s fail SOFT_RESET\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /*
    --- Get chip ID
    */
    Version = Reg0297J_GetSTV0297JId(&Instance->DeviceMap, Instance->IOHandle);
    if ( Version !=  STV0297J_DEVICE_VERSION)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s fail device not found (0x%02x)\n", identity, Version));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s device found, release/revision=%u\n", identity, Version));
#endif
    }

    /* reset all chip registers */
    Error = STTUNER_IOREG_Reset(&Instance->DeviceMap, Instance->IOHandle, defval, addressarray);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
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
                                  STTUNER_J83_B |
                                  STTUNER_J83_C;

    Capability->AGCControl      = FALSE;
    Capability->SymbolMin       = STV0297J_SYMBOLMIN;        /*   1 MegaSymbols/sec (e.g) */
    Capability->SymbolMax       = STV0297J_SYMBOLMAX;        /*  11 MegaSymbols/sec (e.g) */

    Capability->BerMax           = 0;
    Capability->SignalQualityMax = STV0297J_MAX_SIGNAL_QUALITY;
    Capability->AgcMax           = STV0297J_MAX_AGC;

    /* Setup stateblock */
    Instance->StateBlock.ScanMode     = DEF0297J_CHANNEL;
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
#ifdef STTUNER_DRV_CAB_J83B
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_0, 0x4f);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_1, 0x00);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_2, 0x24);
#ifdef STTUNER_BLOCK_SYNC_MODE_SELECT
            if (Instance->Sti5518 == STTUNER_STI5518_NONE)
                STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_DATA_OUT_CTRL, 0x01);
            else
                STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_DATA_OUT_CTRL, 0x05);
#else
            /* Default value for all chips except 5518 */
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_DATA_OUT_CTRL, 0x01);
#endif
#else
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_0, 0x4f);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_1, 0x00);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_2, 0x24);
#endif
            break;

        case STTUNER_TS_MODE_DVBCI:
#ifdef STTUNER_DRV_CAB_J83B
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_0, 0x43);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_1, 0x60);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_2, 0x24);
#ifdef STTUNER_BLOCK_SYNC_MODE_SELECT
            if (Instance->Sti5518 == STTUNER_STI5518_NONE)
                STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_DATA_OUT_CTRL, 0x00);
            else
                STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_DATA_OUT_CTRL, 0x04);
#else
            /* Default value for all chips except 5518 */
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_DATA_OUT_CTRL, 0x00);
#endif
#else
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_0, 0x43);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_1, 0x60);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_2, 0x24);
#endif
            break;

        case STTUNER_TS_MODE_PARALLEL:
        case STTUNER_TS_MODE_DEFAULT:
#ifdef STTUNER_DRV_CAB_J83B
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_0, 0x4b);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_1, 0x00);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_2, 0x24);
#ifdef STTUNER_BLOCK_SYNC_MODE_SELECT
            if (Instance->Sti5518 == STTUNER_STI5518_NONE)
                STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_DATA_OUT_CTRL, 0x00);
            else
                STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_DATA_OUT_CTRL, 0x04);
#else
            /* Default value for all chips except 5518 */
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_DATA_OUT_CTRL, 0x00);
#endif
#else
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_0, 0x4b);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_1, 0x00);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_2, 0x24);
#endif
        default:
            break;
    }

    /* Set default error count mode, bit error rate */
    Reg0297J_StopBlkCounter(&Instance->DeviceMap, Instance->IOHandle);

    /* finally (as nor more errors to check for, allocate handle */
    Instance->TopLevelHandle = OpenParams->TopLevelHandle;
    *Handle = (U32)Instance;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
    STTBX_Print(("%s opened ok\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    Error |= Instance->DeviceMap.Error;

    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297J_Close()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297J_Close(DEMOD_Handle_t Handle, DEMOD_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
   const char *identity = "STTUNER d0297J.c demod_d0297J_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0297J_InstanceData_t     *Instance;

    /* private driver instance data */
    Instance = D0297J_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
    STTBX_Print(("%s closed\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: demod_d0297J_IsAnalogCarrier()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297J_IsAnalogCarrier(DEMOD_Handle_t Handle, BOOL *IsAnalog)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: demod_d0297J_GetSignalQuality()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297J_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber_p)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
   const char *identity = "STTUNER d0297J.c demod_d0297J_GetSignalQuality()";
#endif
    ST_ErrorCode_t              Error = ST_NO_ERROR;
    STTUNER_tuner_instance_t    *TunerInstance;
    STTUNER_InstanceDbase_t     *Inst;
    D0297J_InstanceData_t        *Instance;
    int                         Mean, CNdB100;
    int                         ApplicationBERCount;
    BOOL                        IsLocked;

    /*
    --- Set parameters
    */
    *SignalQuality_p = 0;
    *Ber_p = 0;
    Mean = 0;
    CNdB100 = 0;
    ApplicationBERCount = 0;    /* Bit Error counter */


    /* private driver instance data */
    Instance = D0297J_GetInstFromHandle(Handle);
    /* top level public instance data */
    Inst = STTUNER_GetDrvInst();
    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[Instance->TopLevelHandle].Cable.Tuner;

    /*
    --- Get Carrier And Data (TS) status
    */
    Error = demod_d0297J_IsLocked(Handle, &IsLocked);
    if ( Error == ST_NO_ERROR && IsLocked )
    {
        /*
        --- Read Blk Counter
        */
        Reg0297J_StopBlkCounter(&Instance->DeviceMap, Instance->IOHandle);

        /*
        --- Read noise estimations and BER
        */
        Driv0297JBertCount(&Instance->DeviceMap, Instance->IOHandle, &Instance->StateBlock,
                            TunerInstance, &ApplicationBERCount, Instance->StateBlock.Ber);
        Driv0297JCNEstimator(&Instance->DeviceMap, Instance->IOHandle, &Instance->StateBlock,
                            TunerInstance, &Mean, &CNdB100, &Instance->StateBlock.SignalQuality);

        /*
        --- Restart Blk Counter
        */
        Reg0297J_StartBlkCounter(&Instance->DeviceMap, Instance->IOHandle);

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

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
    STTBX_Print(("%s Mean=%d CN=%d dBx100 (%d %%)\n", identity, Mean, CNdB100, Instance->StateBlock.SignalQuality));
    STTBX_Print(("%s BERCount=%d BER[0..2] (cnt %d sat %d rate %d 10E-6)\n", identity,
                ApplicationBERCount,
                Instance->StateBlock.Ber[0],
                Instance->StateBlock.Ber[1],
                Instance->StateBlock.Ber[2]));
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
Name: demod_d0297J_GetModulation()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297J_GetModulation(DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
   const char *identity = "STTUNER d0297J.c demod_d0297J_GetModulation()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0297J_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0297J_GetInstFromHandle(Handle);

    /* This implementation of the DEMOD device only supports one type of modulation */
    *Modulation = Reg0297J_GetQAMSize(&Instance->DeviceMap, Instance->IOHandle);

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
    STTBX_Print(("%s Modulation=%u\n", identity, *Modulation));
#endif

    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297J_GetAGC()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297J_GetAGC(DEMOD_Handle_t Handle, S16  *Agc)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
   const char *identity = "STTUNER d0297J.c demod_d0297J_GetAGC()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0297J_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0297J_GetInstFromHandle(Handle);

    *Agc   = (S16)(0xFFFF & Reg0297J_GetAGC(&Instance->DeviceMap, Instance->IOHandle));

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
    STTBX_Print(("%s Agc=%u\n", identity, *Agc));
#endif

    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297J_GetFECRates()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297J_GetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t *FECRates)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297J_IsLocked()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297J_IsLocked(DEMOD_Handle_t Handle, BOOL *IsLocked)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
   const char *identity = "STTUNER d0297J.c demod_d0297J_IsLocked()";
#endif
    ST_ErrorCode_t          Error = ST_NO_ERROR;
    U8                      Version;
    D0297J_InstanceData_t    *Instance;
    D0297J_SignalType_t      SignalType;

    /* private driver instance data */
    Instance = D0297J_GetInstFromHandle(Handle);

    *IsLocked = FALSE;

    /*
    --- Get Version Id (In fact to test I2C Access
    */
    Version = Reg0297J_GetSTV0297JId(&Instance->DeviceMap, Instance->IOHandle);
    if ( Version !=  STV0297J_DEVICE_VERSION)
    {
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    /*
    --- Get Information from demod
    */
    SignalType = Drv0297J_CheckAgc(&Instance->DeviceMap, Instance->IOHandle, &Instance->StateBlock.Params);
    if (SignalType == E297J_AGCOK)
    {
        SignalType = Drv0297J_CheckData(&Instance->DeviceMap, Instance->IOHandle, &Instance->StateBlock.Params);
        if (SignalType == E297J_DATAOK)
        {
            Instance->StateBlock.Result.SignalType = E297J_LOCKOK;
            *IsLocked = TRUE;
        }
        else
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
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
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s FRONT END IS UNLOCKED !!!! NO AGC\n", identity));
#endif
        /*
        --- Update Satus
        */
        Instance->StateBlock.Result.SignalType = SignalType;
    }

/*
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
    STTBX_Print(("%s TUNER IS %s\n", identity, (*IsLocked)?"LOCKED":"UNLOCKED"));
#endif
*/

    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297J_SetFECRates()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297J_SetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t FECRates)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297J_Tracking()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297J_Tracking(DEMOD_Handle_t Handle, BOOL ForceTracking, U32 *NewFrequency, BOOL *SignalFound)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
   const char *identity = "STTUNER d0297J.c demod_d0297J_Tracking()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    STTUNER_tuner_instance_t    *TunerInstance;     /* don't really need all these variables, done for clarity */
    STTUNER_InstanceDbase_t     *Inst;              /* pointer to instance database */
    STTUNER_Handle_t            TopHandle;          /* instance that contains the demos, tuner, lnb & diseqc driver set */
    D0297J_InstanceData_t        *Instance;
    BOOL                        IsLocked, DataFound;

    /* private driver instance data */
    Instance = D0297J_GetInstFromHandle(Handle);
    /* top level public instance data */
    Inst = STTUNER_GetDrvInst();

    /* this driver knows what to level instance it belongs to */
    TopHandle = Instance->TopLevelHandle;

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[TopHandle].Cable.Tuner;

    /*
    --- Get Carrier And Data (TS) status
    */
    Error = demod_d0297J_IsLocked(Handle, &IsLocked);
    if ( Error == ST_NO_ERROR && IsLocked )
    {
        /*
        --- Tuner Is Locked
        */
        *SignalFound  = (Instance->StateBlock.Result.SignalType == E297J_LOCKOK)? TRUE : FALSE;
        *NewFrequency = 1000 * ((U32)Instance->StateBlock.Result.Frequency);
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s FRONT END IS LOCKED on %d Hz (CARRIER and DATA)\n", identity, *NewFrequency));
#endif
    }
    else
    {
        /*
        --- Tuner is UnLocked => Search Carrier and Data
        */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s FRONT END IS UNLOCKED ==> SEARCH CARRIER AND DATA\n", identity));
#endif
        DataFound = Driv0297JCarrierSearch(&Instance->DeviceMap, Instance->IOHandle, &Instance->StateBlock, TunerInstance);
        Error |= Instance->DeviceMap.Error;
        Instance->DeviceMap.Error = ST_NO_ERROR;
        if (Error != ST_NO_ERROR)
        {
            return(Error);
        }

        if (DataFound)
        {
            *SignalFound  = (Instance->StateBlock.Result.SignalType == E297J_LOCKOK)? TRUE : FALSE;
            *NewFrequency = 1000 * ((U32)Instance->StateBlock.Result.Frequency);
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
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
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
            STTBX_Print(("%s NO DATA on %d Hz !!!!\n", identity, *NewFrequency));
#endif
        }
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
    STTBX_Print(("%s %s, on %u Hz\n", identity, (*SignalFound)?"SIGNAL FOUND":"NO SIGNAL DOUND", *NewFrequency));
#endif

    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297J_ScanFrequency()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297J_ScanFrequency(DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset,
                                                                U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                U32  FreqOff,          U32   ChannelBW,     S32   EchoPos)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
    clock_t                     time_start_lock, time_end_lock;
   const char *identity = "STTUNER d0297J.c demod_d0297J_ScanFrequency()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_tuner_instance_t    *TunerInstance;
    STTUNER_InstanceDbase_t     *Inst;
    D0297J_InstanceData_t        *Instance;
    BOOL                        DataFound=FALSE;

    /* private driver instance data */
    Instance = D0297J_GetInstFromHandle(Handle);
    /* top level public instance data */
    Inst = STTUNER_GetDrvInst();

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[Instance->TopLevelHandle].Cable.Tuner;

    /*STTUNER_task_lock();*/

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
    Drv0297J_InitSearch(TunerInstance, &Instance->StateBlock,
                       Inst[Instance->TopLevelHandle].Cable.Modulation,
                       InitialFrequency,
                       SymbolRate,
                       Spectrum,
                       Inst[Instance->TopLevelHandle].Cable.ScanExact,
                       Inst[Instance->TopLevelHandle].Cable.SingleScan.J83);

    /*
    --- Try to lock
    */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
    time_start_lock = STOS_time_now();
#endif
    DataFound = Driv0297JCarrierSearch(&Instance->DeviceMap,Instance->IOHandle, &Instance->StateBlock, TunerInstance);
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
    time_end_lock = STOS_time_now();
#endif
    if (DataFound)
    {
        /*Instance->StateBlock.Result.SignalType = E297J_LOCKOK;*/
        /* Pass new frequency to caller */
        /*For GNBvd59268, for correct Outformat register settings to avoid AV Packet issue*/
        if(Instance->TSOutputMode==STTUNER_TS_MODE_SERIAL)
        {
        
#ifdef STTUNER_DRV_CAB_J83B
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_0, 0x4f);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_1, 0x00);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_2, 0x24);
#ifdef STTUNER_BLOCK_SYNC_MODE_SELECT
            if (Instance->Sti5518 == STTUNER_STI5518_NONE)
                STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_DATA_OUT_CTRL, 0x01);
            else
                STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_DATA_OUT_CTRL, 0x05);
#else
            /* Default value for all chips except 5518 */
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_DATA_OUT_CTRL, 0x01);
#endif
#else
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_0, 0x4f);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_1, 0x00);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_2, 0x24);
#endif
        }
        if(Instance->TSOutputMode==STTUNER_TS_MODE_DVBCI)
        {
#ifdef STTUNER_DRV_CAB_J83B
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_0, 0x43);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_1, 0x60);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_2, 0x24);
#ifdef STTUNER_BLOCK_SYNC_MODE_SELECT
            if (Instance->Sti5518 == STTUNER_STI5518_NONE)
                STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_DATA_OUT_CTRL, 0x00);
            else
                STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_DATA_OUT_CTRL, 0x04);
#else
            /* Default value for all chips except 5518 */
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_DATA_OUT_CTRL, 0x00);
#endif
#else
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_0, 0x43);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_1, 0x60);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_2, 0x24);
#endif
        }
        if(Instance->TSOutputMode==STTUNER_TS_MODE_PARALLEL || Instance->TSOutputMode==STTUNER_TS_MODE_DEFAULT)
        { 
#ifdef STTUNER_DRV_CAB_J83B
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_0, 0x4b);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_1, 0x00);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_2, 0x24);
#ifdef STTUNER_BLOCK_SYNC_MODE_SELECT
            if (Instance->Sti5518 == STTUNER_STI5518_NONE)
                STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_DATA_OUT_CTRL, 0x00);
            else
                STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_DATA_OUT_CTRL, 0x04);
#else
            /* Default value for all chips except 5518 */
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_DATA_OUT_CTRL, 0x00);
#endif
#else
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_0, 0x4b);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_1, 0x00);
            STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle, R0297J_OUTFORMAT_2, 0x24);
#endif 
         
        }
                 
        *NewFrequency = 1000 * ((U32)Instance->StateBlock.Result.Frequency);
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s  DATA FOUND on %u Hz\n", identity, *NewFrequency));
#endif
        *ScanSuccess = TRUE;
    }
    else
    {
        Instance->StateBlock.Result.SignalType = E297J_NODATA;
        *NewFrequency = InitialFrequency;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s NO DATA on %u Hz\n", identity, *NewFrequency));
#endif
        *ScanSuccess = FALSE;
    }

    /*STTUNER_task_unlock();*/

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
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
Name: demod_d0297J_StandByMode()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */



ST_ErrorCode_t demod_d0297J_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode)
{
	
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
   const char *identity = "STTUNER d0297J.c demod_d0297J_StandByMode()";
#endif
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
      
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297J_ioctl()

Description:
    access device specific low-level functions

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297J_ioctl(DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
   const char *identity = "STTUNER d0297J.c demod_d0297J_ioctl()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0297J_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0297J_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
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
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
    STTBX_Print(("%s called ok\n", identity, Function));    /* signal that function came back */
#endif

    Error |= Instance->DeviceMap.Error; /* Also accumulate I2C error */
    Instance->DeviceMap.Error = ST_NO_ERROR;

    return(Error);

}



/* ----------------------------------------------------------------------------
Name: demod_d0297J_ioaccess()

Description:
    called from ioarch.c when some driver does I/O but with the repeater/passthru
    set to point to this function.
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297J_ioaccess(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
   const char *identity = "STTUNER d0297J.c demod_d0297J_ioaccess()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    IOARCH_Handle_t ThisIOHandle;
    D0297J_InstanceData_t *Instance;

    Instance = D0297J_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
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
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
        STTBX_Print(("%s write passthru\n", identity));
#endif
*/
        Error = STTUNER_IOARCH_ReadWrite(ThisIOHandle, Operation, SubAddr, Data, TransferSize, Timeout);
    }
    else    /* repeater */
    {
/*
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J
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
        Error = STTUNER_IOREG_SetField(& Instance->DeviceMap, ThisIOHandle, F0297J_I2CT_EN, 1);
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

D0297J_InstanceData_t *D0297J_GetInstFromHandle(DEMOD_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J_HANDLES
   const char *identity = "STTUNER d0297J.c D0297J_GetInstFromHandle()";
#endif
    D0297J_InstanceData_t *Instance;

    Instance = (D0297J_InstanceData_t *)Handle;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297J_HANDLES
    STTBX_Print(("%s block at 0x%08x\n", identity, Instance));
#endif

    return(Instance);
}




/* End of d0297J.c */
