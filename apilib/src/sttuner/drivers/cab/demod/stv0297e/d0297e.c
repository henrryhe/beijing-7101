/* ----------------------------------------------------------------------------
File Name: d0297e.c

Description:

    stv0297e demod driver.

Copyright (C) 1999-2006 STMicroelectronics

---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
/* C libs */
#include <string.h>

#endif
#include "sttbx.h"
#include "stlite.h"     /* Standard includes */
#include "stcommon.h"
#include "chip.h"

/* STAPI */

#include "stevt.h"
#include "sttuner.h"

/* local to sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "ioarch.h"     /* I/O for this driver */
#include "ioreg.h"      /* I/O for this driver */
#include "util.h"       /* generic utility functions for sttuner */
#include "drv0297e.h"   /* misc driver functions */
#include "d0297e.h"     /* header for this file */
#include "util297e.h"   /* utility functions */

#include "cioctl.h"     /* data structure typedefs for all the the cable ioctl functions */


/* Private types/constants ------------------------------------------------ */

/* Device capabilities */
#define STV0297E_MAX_AGC                     1023
#define STV0297E_MAX_SIGNAL_QUALITY          100
#define STCHIP_HANDLE(x) ((STCHIP_InstanceData_t *)x)

/* Device ID */
#define STV0297E_DEVICE_VERSION              0x10
#define STV0297E_SYMBOLMIN                   870000;     /* # 1  MegaSymbols/sec */
#define STV0297E_SYMBOLMAX                   11700000;   /* # 11 MegaSymbols/sec */

/* private variables ------------------------------------------------------- */

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */

static BOOL        Installed = FALSE;


/* ---------- per instance of driver ---------- */
typedef struct
{
    ST_DeviceName_t           *DeviceName;           /* unique name for opening under */
    STTUNER_Handle_t          TopLevelHandle;       /* access tuner, lnb etc. using this */
    IOARCH_Handle_t           IOHandle;             /* instance access to I/O driver     */
    FE_297e_Modulation_t      FE_297e_Modulation;
    ST_Partition_t            *MemoryPartition;     /* which partition this data block belongs to */
    void                      *InstanceChainPrev;   /* previous data block in chain or NULL if not */
    void                      *InstanceChainNext;   /* next data block in chain or NULL if last */

    U32                         ExternalClock;  /* External VCO */
    STTUNER_TSOutputMode_t      TSOutputMode;
    STTUNER_SerialDataMode_t    SerialDataMode;
    STTUNER_SerialClockSource_t SerialClockSource;
    STTUNER_FECMode_t           FECMode;
    STTUNER_DataClockPolarity_t ClockPolarity;
    STTUNER_SyncStrip_t         SyncStripping;
    STTUNER_BlockSyncMode_t     BlockSyncMode;
    U32 Symbolrate;
    U32                         StandBy_Flag; /*** To take care same Standby Mode doesnot
                                                   execute more than once ***/
 }
D0297E_InstanceData_t;

/* instance chain, the default boot value is invalid, to catch errors */
static D0297E_InstanceData_t *InstanceChainTop = (D0297E_InstanceData_t *)0x7fffffff;

/* functions --------------------------------------------------------------- */

/* API */
ST_ErrorCode_t demod_d0297E_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams);
ST_ErrorCode_t demod_d0297E_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams);

ST_ErrorCode_t demod_d0297E_Open (ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle);
ST_ErrorCode_t demod_d0297E_Close(DEMOD_Handle_t  Handle, DEMOD_CloseParams_t *CloseParams);


ST_ErrorCode_t demod_d0297E_IsAnalogCarrier (DEMOD_Handle_t Handle, BOOL *IsAnalog);
ST_ErrorCode_t demod_d0297E_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber_p);
ST_ErrorCode_t demod_d0297E_GetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation);
ST_ErrorCode_t demod_d0297E_SetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t Modulation);
ST_ErrorCode_t demod_d0297E_GetAGC          (DEMOD_Handle_t Handle, S16                  *Agc);
ST_ErrorCode_t demod_d0297E_GetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t    *FECRates);
ST_ErrorCode_t demod_d0297E_IsLocked        (DEMOD_Handle_t Handle, BOOL                 *IsLocked);
ST_ErrorCode_t demod_d0297E_SetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t     FECRates);
ST_ErrorCode_t demod_d0297E_Tracking        (DEMOD_Handle_t Handle, BOOL ForceTracking,   U32 *NewFrequency, BOOL *SignalFound);
ST_ErrorCode_t demod_d0297E_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode);

ST_ErrorCode_t demod_d0297E_ScanFrequency   (DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset,
                                                                   U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                   U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                   U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                   U32  FreqOff,          U32   ChannelBW,     S32   EchoPos);
/* access device specific low-level functions */
ST_ErrorCode_t demod_d0297E_ioctl           (DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams,
                                             STTUNER_Da_Status_t *Status);


ST_ErrorCode_t demod_d0297E_repeateraccess(DEMOD_Handle_t Handle,BOOL REPEATER_STATUS);

/* local functions --------------------------------------------------------- */

D0297E_InstanceData_t *D0297E_GetInstFromHandle(DEMOD_Handle_t Handle);
extern IOARCH_HandleData_t IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];

U8 Def297eVal[STB0297E_NBREGS]=	/* Default values for STB0297E registers	*/ 
{
	/* register at @ 0x06 is put on top of the list as it defines the pll output frequency */
	/* 0x0006    */ 0x12,/* 0x0000 RO */ 0x10,/* 0x0002    */ 0x40,/* 0x0003    */ 0x00,/* 0x0004    */ 0x01,
	/* 0x0005    */ 0x10,/* 0x0007    */ 0x40,/* 0x0008 RO */ 0x6b,/* 0x0009 RO */ 0x08,/* 0x000a    */ 0x00,
	/* 0x000b    */ 0x00,/* 0x000c RO */ 0x04,/* 0x000e    */ 0x00,/* 0x000f    */ 0x00,/* 0x0010    */ 0x77,
	/* 0x0011    */ 0x50,/* 0x0012    */ 0x00,/* 0x0013    */ 0x00,/* 0x0014    */ 0x5a,/* 0x0015    */ 0x00,
	/* 0x0016    */ 0xcc,/* 0x0017    */ 0x04,/* 0x0018    */ 0x99,/* 0x0019    */ 0x09,/* 0x001a    */ 0x00,
	/* 0x001b    */ 0x00,/* 0x001c RO */ 0x75,/* 0x001d RO */ 0xe9,/* 0x001e RO */ 0x03,/* 0x0020    */ 0xb5,
	/* 0x0021    */ 0x0c,/* 0x0022    */ 0x1a,/* 0x0023    */ 0x01,/* 0x0024    */ 0x00,/* 0x0025    */ 0x1f,
	/* 0x0026    */ 0x95,/* 0x0027    */ 0x4d,/* 0x0028    */ 0x9c,/* 0x0029    */ 0x6e,/* 0x002a    */ 0xf9,
	/* 0x002b    */ 0x1d,/* 0x002c    */ 0xee,/* 0x002d    */ 0x00,/* 0x0030    */ 0x03,/* 0x0031    */ 0x00,
	/* 0x0032    */ 0x00,/* 0x0033    */ 0x00,/* 0x0034    */ 0x39,/* 0x0035    */ 0xba,/* 0x0036    */ 0xb6,
	/* 0x0037    */ 0xb6,/* 0x0038    */ 0x98,/* 0x0039    */ 0x6a,/* 0x003a    */ 0xb3,/* 0x003b    */ 0x01,
	/* 0x003c    */ 0x0d,/* 0x003d    */ 0x94,/* 0x0040    */ 0xe0,/* 0x0041    */ 0x0f,/* 0x0042    */ 0x8b,
	/* 0x0043    */ 0xbe,/* 0x0044    */ 0xf5,/* 0x0045    */ 0xf5,/* 0x0046    */ 0x65,/* 0x0047    */ 0x23,
	/* 0x0048    */ 0x00,/* 0x0049    */ 0x57,/* 0x004a    */ 0x74,/* 0x004b    */ 0x94,/* 0x004c    */ 0x16,
	/* 0x004d    */ 0x18,/* 0x004e    */ 0x0c,/* 0x004f    */ 0x59,/* 0x0050    */ 0x39,/* 0x0051    */ 0x94,
	/* 0x0052    */ 0x54,/* 0x0053    */ 0xec,/* 0x0054    */ 0x85,/* 0x0055    */ 0x50,/* 0x0056    */ 0x04,
	/* 0x0057    */ 0x7f,/* 0x0058    */ 0x2d,/* 0x0064    */ 0x20,/* 0x0070    */ 0x28,/* 0x0071    */ 0x44,
	/* 0x0072    */ 0x22,/* 0x0073    */ 0x03,/* 0x0074    */ 0x04,/* 0x0075    */ 0x11,/* 0x0076    */ 0x20,
	/* 0x0080    */ 0xa0,/* 0x0081    */ 0x08,/* 0x0082 RO */ 0x0c,/* 0x0083    */ 0x00,/* 0x0084    */ 0x00,
	/* 0x0085    */ 0x00,/* 0x0086    */ 0x00,/* 0x0087    */ 0x00,/* 0x0088    */ 0x00,/* 0x0089    */ 0x32,
	/* 0x0094 RO */ 0xf7,/* 0x0095 RO */ 0xff,/* 0x0096 RO */ 0x1f,/* 0x0097    */ 0x00,/* 0x0098 RO */ 0x25,
	/* 0x0099 RO */ 0x00,/* 0x009a RO */ 0x00,/* 0x009b    */ 0x06,/* 0x009c RO */ 0xeb,/* 0x009d RO */ 0xff,
	/* 0x009e    */ 0x00,/* 0x009f    */ 0x00,/* 0x00a0 RO */ 0x91,/* 0x00a1 RO */ 0x03,/* 0x00a2 RO */ 0xb0,
	/* 0x00a3 RO */ 0x03,/* 0x00a4    */ 0xc5,/* 0x00a5    */ 0x80,/* 0x00a6 RO */ 0x4c,/* 0x00a7 RO */ 0x00,
	/* 0x00a8    */ 0x00,/* 0x00a9    */ 0x00,/* 0x00aa    */ 0x36,/* 0x00ab    */ 0xaa,/* 0x00ac    */ 0x00,
	/* 0x00ae    */ 0x63,/* 0x00af    */ 0xdf,/* 0x00b0    */ 0x88,/* 0x00b1    */ 0x41,/* 0x00b2    */ 0xc1,
	/* 0x00b3    */ 0xa7,/* 0x00b4    */ 0x06,/* 0x00b5    */ 0x85,/* 0x00b6    */ 0xe2,/* 0x00b7    */ 0x1d,
	/* 0x00b8    */ 0x00,/* 0x00b9    */ 0x00,/* 0x00ba    */ 0x00,/* 0x00bb    */ 0x00,/* 0x00bc    */ 0x40,
	/* 0x00bd    */ 0x90,/* 0x00be    */ 0xa7,/* 0x00c0    */ 0x82,/* 0x00c1    */ 0x82,/* 0x00c2    */ 0x82,
	/* 0x00c3    */ 0x12,/* 0x00c4    */ 0x0d,/* 0x00c5    */ 0x82,/* 0x00c6    */ 0x82,/* 0x00c7    */ 0x10,
	/* 0x00c8    */ 0x98,/* 0x00c9    */ 0x96,/* 0x00d8    */ 0x16,/* 0x00d9    */ 0x0b,/* 0x00da    */ 0x88,
	/* 0x00db    */ 0x02,/* 0x00dc RO */ 0x12,/* 0x00de RO */ 0x48,/* 0x00df RO */ 0x0e,/* 0x00e0 RO */ 0x00,
	/* 0x00e1 RO */ 0x00,/* 0x00e2 RO */ 0x00,/* 0x00e3 RO */ 0x00,/* 0x00e4    */ 0x01,/* 0x00e5    */ 0x26,
	/* 0x00e6 RO */ 0x00,/* 0x00e7 RO */ 0x00,/* 0x00e8    */ 0x02,/* 0x00e9    */ 0x22,/* 0x00ea    */ 0x08,
	/* 0x00ec    */ 0x01,/* 0x00ed    */ 0xc6,/* 0x00ef    */ 0x43,/* 0x00f0    */ 0x00,/* 0x00f1    */ 0x00,
	/* 0x00f2    */ 0x00,/* 0x00f3    */ 0x00,/* 0x00f4 RO */ 0x00,/* 0x00f5 RO */ 0x00,/* 0x00f6 RO */ 0x00,
	/* 0x00f7 RO */ 0x00,/* 0x00f8 RO */ 0x00,/* 0x00f9 RO */ 0x00,/* 0x00fa RO */ 0xa0,/* 0x00fb    */ 0x00,
	/* 0x00fc RO */ 0x00,/* 0x00fd RO */ 0x00,/* 0x00fe RO */ 0x00,/* 0x00ff RO */ 0x00,/* 0x006d    */ 0x20
};


U16 Addressarray[STB0297E_NBREGS]=
{
0x0006, 0x0000, 0x0002, 0x0003, 0x0004, 0x0005, 0x0007, 0x0008, 0x0009, 0x000A,
0x000B, 0x000C, 0x000E, 0x000F, 0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015,
0x0016, 0x0017, 0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x0020,
0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A,
0x002B, 0x002C, 0x002D, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036,
0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x0040, 0x0041, 0x0042,
0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C,
0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056,
0x0057, 0x0058, 0x0064, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076,
0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087, 0x0088, 0x0089,
0x0094, 0x0095, 0x0096, 0x0097, 0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D,
0x009E, 0x009F, 0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AE, 0x00AF, 0x00B0, 0x00B1, 0x00B2,
0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC,
0x00BD, 0x00BE, 0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
0x00C8, 0x00C9, 0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DE, 0x00DF, 0x00E0,
0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7, 0x00E8, 0x00E9, 0x00EA,
0x00EC, 0x00ED, 0x00EF, 0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6,
0x00F7, 0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF, 0x006D
};

#define MAX_ADDRESS 0x00FF  /*This much bytes memory will be allocated for register map*/


/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0297E_Install()

Description:
    install a cable device driver into the demod database.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0297E_Install(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
   const char *identity = "STTUNER d0297E.c STTUNER_DRV_DEMOD_STV0297E_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s fail driver already installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
    STTBX_Print(("%s installing cable:demod:STV0297E...", identity));
#endif

    /* mark ID in database */
    Demod->ID = STTUNER_DEMOD_STV0297E;

    /* map API */
    Demod->demod_Init = demod_d0297E_Init;
    Demod->demod_Term = demod_d0297E_Term;

    Demod->demod_Open  = demod_d0297E_Open;
    Demod->demod_Close = demod_d0297E_Close;

    Demod->demod_IsAnalogCarrier  = demod_d0297E_IsAnalogCarrier;
    Demod->demod_GetSignalQuality = demod_d0297E_GetSignalQuality;
    Demod->demod_GetModulation    = demod_d0297E_GetModulation;
    Demod->demod_SetModulation    = demod_d0297E_SetModulation;
    Demod->demod_GetAGC           = demod_d0297E_GetAGC;
    Demod->demod_GetFECRates      = demod_d0297E_GetFECRates;
    Demod->demod_IsLocked         = demod_d0297E_IsLocked ;
    Demod->demod_SetFECRates      = demod_d0297E_SetFECRates;
    Demod->demod_Tracking         = demod_d0297E_Tracking;
    Demod->demod_ScanFrequency    = demod_d0297E_ScanFrequency;
    Demod->demod_StandByMode      = demod_d0297E_StandByMode;
    Demod->demod_ioaccess = NULL;
   Demod->demod_repeateraccess = demod_d0297E_repeateraccess;
    Demod->demod_ioctl    = demod_d0297E_ioctl;

    InstanceChainTop = NULL;

	Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL, 1);

    Installed = TRUE;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
    STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0297E_UnInstall()

Description:
    install a cable device driver into the demod database.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0297E_UnInstall(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
   const char *identity = "STTUNER d0297E.c STTUNER_DRV_DEMOD_STV0297E_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Demod->ID != STTUNER_DEMOD_STV0297E)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s fail incorrect driver type\n", identity));
#endif
        return(STTUNER_ERROR_ID);
    }

    /* has all memory been freed, by Term() */
    if(InstanceChainTop != NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s fail at least one instance not terminated\n", identity));
#endif
        return(ST_ERROR_OPEN_HANDLE);
    }


#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
    STTBX_Print(("%s uninstalling cable:demod:STV0297E...", identity));
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
    Demod->demod_SetModulation    = NULL;
    Demod->demod_GetAGC           = NULL;
    Demod->demod_GetFECRates      = NULL;
    Demod->demod_IsLocked         = NULL;
    Demod->demod_SetFECRates      = NULL;
    Demod->demod_Tracking         = NULL;
    Demod->demod_ScanFrequency    = NULL;
    Demod->demod_StandByMode      = NULL;
    Demod->demod_repeateraccess = NULL;
    Demod->demod_ioctl    = NULL;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("<"));
#endif

STOS_SemaphoreDelete(NULL, Lock_InitTermOpenClose);

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print((">"));
#endif

    InstanceChainTop = (D0297E_InstanceData_t *)0x7ffffffe;
    Installed        = FALSE;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */



/* ----------------------------------------------------------------------------
Name: demod_d0297E_Init()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297E_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
    const char *identity = "STTUNER d0297E.c demod_d0297E_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0297E_InstanceData_t *InstanceNew, *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
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
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull(DeviceName);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s fail DeviceName not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( D0297E_InstanceData_t ));
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
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

    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.Registers = STB0297E_NBREGS;
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.Fields    = STB0297E_NBFIELDS;
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.Mode      = IOREG_MODE_SUBADR_8; /* i/o addressing mode to use */
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.MemoryPartition = InitParams->MemoryPartition;
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.DefVal= (U32 *)&Def297eVal[0];    
    IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.MemoryPartition,MAX_ADDRESS,sizeof(U8));


    InstanceNew->InstanceChainNext   = NULL; /* always last in the chain */

    InstanceNew->ExternalClock     = InitParams->ExternalClock;          /* Unit Hz */
    InstanceNew->TSOutputMode      = InitParams->TSOutputMode;
    InstanceNew->SerialDataMode    = InitParams->SerialDataMode;
    InstanceNew->SerialClockSource = InitParams->SerialClockSource;
    InstanceNew->FECMode           = InitParams->FECMode;
    InstanceNew->ClockPolarity     = InitParams->ClockPolarity;
    InstanceNew->SyncStripping     = InitParams->SyncStripping;
    InstanceNew->BlockSyncMode     = InitParams->BlockSyncMode;
    InstanceNew->FE_297e_Modulation = FE_297e_MOD_QAM64; /*default 64QAM*/ /*will be filled from scanparams during scan/set frequency*/

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes)\n", identity, InstanceNew->DeviceName, (U32)InstanceNew, sizeof( D0297E_InstanceData_t ) ));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297E_Term()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297E_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
   const char *identity = "STTUNER d0297E.c demod_d0297E_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0297E_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
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
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }


    /* reap next matching DeviceName */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
    STTBX_Print(("%s looking for first free handle[\n", identity));
#endif

    Instance = InstanceChainTop;
    while(1)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        if ( strcmp( (char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
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
           STOS_MemoryDeallocate(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream);

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
            STTBX_Print(("%s terminated ok\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL)
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
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


#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297E_Open()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297E_Open(ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
   const char *identity = "STTUNER d0297E.c demod_d0297E_Open()";
#endif
    ST_ErrorCode_t          Error = ST_NO_ERROR;
    D0297E_InstanceData_t   *Instance;
    U8 ChipID, i=0;
    STTUNER_InstanceDbase_t *Inst;
    U8 index;


    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
#endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print((" found ok\n"));
#endif

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
    STTBX_Print(("%s using block at 0x%08x\n", identity, (U32)Instance));
#endif

    /* check handle IS actually free */
    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
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
    /*  soft reset of the whole chip, 297e starts in standby mode by default */
    Error = ChipSetOneRegister( Instance->IOHandle, R297e_CTRL_0, 0x42);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s fail SOFT_RESET\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }
    Error = ChipSetOneRegister( Instance->IOHandle, R297e_CTRL_0, 0x40);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s fail SOFT_RESET\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /*
    --- Get chip ID
    */
    ChipID = ChipGetOneRegister( Instance->IOHandle, R297e_MIS_ID);
    if ( (ChipID & 0xF0) !=  0x10)
    {
		#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s fail device not found (0x%02x)\n", identity, ChipID));
		#endif
		SEM_UNLOCK(Lock_InitTermOpenClose);
		return(ST_ERROR_UNKNOWN_DEVICE);
    }
    else
    {
    	#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
    	STTBX_Print(("%s device found, release/revision=%u\n", identity, ChipID & 0x0F));
    	#endif
    }

     for (index=0;index < STB0297E_NBREGS;index++)
     {
     ChipSetOneRegister(Instance->IOHandle,Addressarray[index],Def297eVal[index]);
     }
    
    
    
    
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s fail reset device\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* Set capabilties */
    Capability->FECAvail        = 0;
    Capability->ModulationAvail = STTUNER_MOD_QAM    |
                                  STTUNER_MOD_4QAM   |
                                  STTUNER_MOD_16QAM  |
                                  STTUNER_MOD_32QAM  |
                                  STTUNER_MOD_64QAM  |
                                  STTUNER_MOD_128QAM |
                                  STTUNER_MOD_256QAM;   /* direct mapping to STTUNER_Modulation_t */

    Capability->J83Avail        = STTUNER_J83_A |
                                  STTUNER_J83_C;

    Capability->AGCControl      = FALSE;
    Capability->SymbolMin       = STV0297E_SYMBOLMIN;        /*   1 MegaSymbols/sec (e.g) */
    Capability->SymbolMax       = STV0297E_SYMBOLMAX;        /*  11 MegaSymbols/sec (e.g) */

    Capability->BerMax           = 0;
    Capability->SignalQualityMax = STV0297E_MAX_SIGNAL_QUALITY;
    Capability->AgcMax           = STV0297E_MAX_AGC;


    /* TS output mode */
    switch (Instance->TSOutputMode)
    {
         case STTUNER_TS_MODE_SERIAL:
            ChipSetOneRegister( Instance->IOHandle, R297e_OUTFORMAT_0, 0x09);
            ChipSetOneRegister( Instance->IOHandle, R297e_FEC_AC_CTR_0, 0x1E);
            ChipSetOneRegister( Instance->IOHandle, R297e_OUTFORMAT_1, 0x24);
            break;

        case STTUNER_TS_MODE_DVBCI:
            ChipSetOneRegister( Instance->IOHandle, R297e_OUTFORMAT_0, 0x0A);
            ChipSetOneRegister( Instance->IOHandle, R297e_FEC_AC_CTR_0, 0x16);
            ChipSetOneRegister( Instance->IOHandle, R297e_OUTFORMAT_1, 0x24);
            break;

        case STTUNER_TS_MODE_PARALLEL:
        case STTUNER_TS_MODE_DEFAULT:
            ChipSetOneRegister( Instance->IOHandle, R297e_OUTFORMAT_0, 0x08);
            ChipSetOneRegister( Instance->IOHandle, R297e_FEC_AC_CTR_0, 0x1E);            
            ChipSetOneRegister( Instance->IOHandle, R297e_OUTFORMAT_1, 0x24);

        default:
            break;
    }

    /*set data clock polarity mode (rising/falling)*/
    switch(Instance->ClockPolarity)
    {
       case STTUNER_DATA_CLOCK_POLARITY_RISING:
    	 ChipSetField( Instance->IOHandle, F297e_CLK_POLARITY, 0);
    	 break;
       case STTUNER_DATA_CLOCK_POLARITY_FALLING:
    	 ChipSetField( Instance->IOHandle, F297e_CLK_POLARITY, 1);
    	 break;
       case STTUNER_DATA_CLOCK_POLARITY_DEFAULT:
       default:
    	 ChipSetField( Instance->IOHandle, F297e_CLK_POLARITY, 0);
    	break;
    }
    
    /*set Sync Stripping (On/Off)*/
    switch(Instance->SyncStripping)
    {
       case STTUNER_SYNC_STRIP_OFF:
    	 ChipSetField( Instance->IOHandle, F297e_SYNC_STRIP, 0);
    	 break;
       case STTUNER_SYNC_STRIP_ON:
    	 ChipSetField( Instance->IOHandle, F297e_SYNC_STRIP, 1);
    	 break;
	   case STTUNER_SYNC_STRIP_DEFAULT:
       default: /*OFF*/
    	 ChipSetField( Instance->IOHandle, F297e_SYNC_STRIP, 0);
    	break;
    }
    
    /*set Sync Stripping (On/Off)*/
    switch(Instance->BlockSyncMode)
    {
       case STTUNER_SYNC_NORMAL:
    	 ChipSetField( Instance->IOHandle, F297e_REFRESH47, 0);
    	 break;
       case STTUNER_SYNC_FORCED:
    	 ChipSetField( Instance->IOHandle, F297e_REFRESH47, 1);
    	 break;
    	case STTUNER_SYNC_DEFAULT:
       default: /*FORCED*/
    	 ChipSetField( Instance->IOHandle, F297e_REFRESH47, 1);
    	break;
    }
    

    if( (Inst[OpenParams->TopLevelHandle].Cable.Tuner.Driver->ID)==STTUNER_TUNER_DCT7045)
    {
	/* Setting to add on the eval board and the NIM */
	ChipSetField( Instance->IOHandle,F297e_CTR_INC_GAIN,0);
	/* Setting to add on the ST NIM */
	/* NIM runs with a different crystal and PLL division ratio and AGC settings are different */
	ChipSetField( Instance->IOHandle,F297e_PLL_BYPASS,1);
	ChipSetField( Instance->IOHandle,F297e_PLL_POFF,1);
	ChipSetField( Instance->IOHandle,F297e_PLL_NDIV,0x10);
	ChipSetField( Instance->IOHandle,F297e_PLL_POFF,0);
	while((ChipGetField( Instance->IOHandle,F297e_PLL_LOCK) == 1)&&(i<10))
	{
		STOS_TaskDelay(ST_GetClocksPerSecond()/1000 ); /*1 msec delay*/
		i ++;
	}
	/*if (ChipGetField( Instance->IOHandle,F297e_PLL_LOCK) == 1)*/
		ChipSetField( Instance->IOHandle,F297e_PLL_BYPASS,0);
	
	ChipSetOneRegister( Instance->IOHandle,R297e_AD_CFG,0x00);
	ChipSetOneRegister( Instance->IOHandle,R297e_AGC_PWM_CFG,0x03);
	ChipSetField( Instance->IOHandle,F297e_AGC_RF_TH_LO,0x65);
	ChipSetField( Instance->IOHandle,F297e_AGC_RF_TH_HI,0x06);
	ChipSetOneRegister( Instance->IOHandle,R297e_GPIO_7_CFG,0x11);
    }
    else if( (Inst[OpenParams->TopLevelHandle].Cable.Tuner.Driver->ID)==STTUNER_TUNER_DCT7045EVAL)
    {
	/* Setting to add on the eval board and the NIM */
	ChipSetField( Instance->IOHandle,F297e_CTR_INC_GAIN,0);
	}
    else if( (Inst[OpenParams->TopLevelHandle].Cable.Tuner.Driver->ID)==STTUNER_TUNER_TDQE3)
    {
    ChipSetField( Instance->IOHandle,F297e_PLL_BYPASS,1);
	ChipSetField( Instance->IOHandle,F297e_PLL_POFF,1);
	ChipSetField( Instance->IOHandle,F297e_PLL_NDIV,0x10);
	ChipSetField( Instance->IOHandle,F297e_PLL_POFF,0);
	while((ChipGetField( Instance->IOHandle,F297e_PLL_LOCK) == 1)&&(i<10))
	{
		STOS_TaskDelay(ST_GetClocksPerSecond()/1000 ); /*1 msec delay*/
		i ++;
	}
	/*if (ChipGetField( Instance->IOHandle,F297e_PLL_LOCK) == 1)*/
		ChipSetField( Instance->IOHandle,F297e_PLL_BYPASS,0);

	ChipSetOneRegister( Instance->IOHandle,R297e_AD_CFG,0x00);
	ChipSetOneRegister( Instance->IOHandle,R297e_AGC_PWM_CFG,0x03);
						
	ChipSetField( Instance->IOHandle,F297e_AGC_IF_THLO_LO,0x00);
	ChipSetField( Instance->IOHandle,F297e_AGC_IF_THLO_HI,0x08);
	ChipSetField( Instance->IOHandle,F297e_AGC_IF_THHI_LO,0xFF);
	ChipSetField( Instance->IOHandle,F297e_AGC_IF_THHI_HI,0x07);
	ChipSetOneRegister( Instance->IOHandle,R297e_GPIO_7_CFG,0x82);
    }
    
    else if( (Inst[OpenParams->TopLevelHandle].Cable.Tuner.Driver->ID)==STTUNER_TUNER_MT2060)
    {
    	/* NIM rus on a 16MHz crystal and uses a different PLL division ration to get a clock of 52MHz
		Note this clock must be changed in case the user wants a serial MPEG stream at the highest symbol rate */
		ChipSetField(Instance->IOHandle,F297e_PLL_BYPASS,1);
		ChipSetField(Instance->IOHandle,F297e_PLL_POFF,1);
		ChipSetField(Instance->IOHandle,F297e_PLL_NDIV,0x19);
		ChipSetField(Instance->IOHandle,F297e_PLL_POFF,0);
		while( (ChipGetField(Instance->IOHandle,F297e_PLL_LOCK) == 1)&&(i<10) )
		{
			STOS_TaskDelay(ST_GetClocksPerSecond()/1000 ); /*1 msec delay*/
			i ++;
		}
		/*if (ChipGetField(Instance->IOHandle,F297e_PLL_LOCK) == 0)*/
			ChipSetField(Instance->IOHandle,F297e_PLL_BYPASS,0);

		ChipSetOneRegister(Instance->IOHandle,R297e_AD_CFG,0x00);
		ChipSetOneRegister(Instance->IOHandle,R297e_AGC_PWM_CFG,0x03);
		ChipSetField(Instance->IOHandle,F297e_AGC_RF_TH_LO,0x66);
		ChipSetField(Instance->IOHandle,F297e_AGC_RF_TH_HI,0x0e);
		ChipSetField(Instance->IOHandle,F297e_AGC_IF_THLO_LO,0xcc);
		ChipSetField(Instance->IOHandle,F297e_AGC_IF_THLO_HI,0x08);
		ChipSetField(Instance->IOHandle,F297e_AGC_IF_THHI_LO,0xcc);
		ChipSetField(Instance->IOHandle,F297e_AGC_IF_THHI_HI,0x0c);
 		/*ChipSetRegisters(Instance->IOHandle,R297e_AGC_RF_TH_L,6);*/

		ChipSetOneRegister(Instance->IOHandle,R297e_GPIO_2_CFG,0x12);
		ChipSetOneRegister(Instance->IOHandle,R297e_GPIO_2_CFG,0x10);
		ChipSetOneRegister(Instance->IOHandle,R297e_GPIO_7_CFG,0x82);
		ChipSetOneRegister(Instance->IOHandle,R297e_GPIO_8_CFG,0x16);
		ChipSetOneRegister(Instance->IOHandle,R297e_GPIO_9_CFG,0x18);
	}


    /* Set default error count mode, bit error rate */
    ChipSetField( Instance->IOHandle, F297e_CT_HOLD, 1); /* holds the counters from being updated */

    /* finally (as nor more errors to check for, allocate handle */
    Instance->TopLevelHandle = OpenParams->TopLevelHandle;
    *Handle = (U32)Instance;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
    STTBX_Print(("%s opened ok\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;

    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297E_Close()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297E_Close(DEMOD_Handle_t Handle, DEMOD_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
   const char *identity = "STTUNER d0297E.c demod_d0297E_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0297E_InstanceData_t     *Instance;

    /* private driver instance data */
    Instance = D0297E_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    /* indicate instance is closed */
    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
    STTBX_Print(("%s closed\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: demod_d0297E_IsAnalogCarrier()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297E_IsAnalogCarrier(DEMOD_Handle_t Handle, BOOL *IsAnalog)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: demod_d0297E_GetSignalQuality()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297E_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber_p)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
   const char *identity = "STTUNER d0297E.c demod_d0297E_GetSignalQuality()";
#endif
    ST_ErrorCode_t              Error = ST_NO_ERROR;
    STTUNER_tuner_instance_t    *TunerInstance;
    STTUNER_InstanceDbase_t     *Inst;
    D0297E_InstanceData_t       *Instance;
    FE_297e_SignalInfo_t        pInfo;
    BOOL                        IsLocked;
    *SignalQuality_p = 0;
    *Ber_p = 0;
    
    pInfo.BER = 0;
    pInfo.CN_dBx10 = 0;

    /* private driver instance data */
    Instance = D0297E_GetInstFromHandle(Handle);
    /* top level public instance data */
    Inst = STTUNER_GetDrvInst();
    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[Instance->TopLevelHandle].Cable.Tuner;

    /*
    --- Get Carrier And Data (TS) status
    */
    Error = demod_d0297E_IsLocked(Handle, &IsLocked);
    if ( Error == ST_NO_ERROR && IsLocked )
    {
        /*
        --- Read Blk Counter
        */
        pInfo.SymbolRate = FE_297e_GetSymbolRate(Instance->IOHandle,FE_297e_GetMclkFreq( Instance->IOHandle, Instance->ExternalClock) );
        ChipSetField( Instance->IOHandle, F297e_CT_HOLD, 1); /* holds the counters from being updated */
		Error = FE_297e_GetSignalInfo( Instance->IOHandle, &pInfo);
		*Ber_p = pInfo.BER;
		*SignalQuality_p = pInfo.CN_dBx10;
		ChipSetField( Instance->IOHandle, F297e_CT_HOLD, 0);
        ChipSetField( Instance->IOHandle, F297e_CT_CLEAR, 0);
        ChipSetField( Instance->IOHandle, F297e_CT_CLEAR, 1);
	}

    Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297E_GetModulation()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297E_GetModulation(DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
   const char *identity = "STTUNER d0297E.c demod_d0297E_GetModulation()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0297E_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0297E_GetInstFromHandle(Handle);

    switch( ChipGetField( Instance->IOHandle, F297e_QAM_MODE) )
    {
    	case 0 /*000*/:
    		*Modulation = STTUNER_MOD_4QAM;
    		#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
            STTBX_Print(("%s QAM4\n", identity));
			#endif
    		break;
    		
    	case 1 /*001*/:
    		*Modulation = STTUNER_MOD_16QAM;
    		#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
            STTBX_Print(("%s QAM16\n", identity));
			#endif
    		break;
    		
    	case 2 /*010*/:
    		*Modulation = STTUNER_MOD_32QAM;
    		#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
            STTBX_Print(("%s QAM32\n", identity));
			#endif
    		break;
    		
    	case 3 /*011*/:
    		*Modulation = STTUNER_MOD_64QAM;
    		#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
            STTBX_Print(("%s QAM64\n", identity));
			#endif
    		break;
    		
    	case 4 /*100*/:
    		*Modulation = STTUNER_MOD_128QAM;
    		#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
            STTBX_Print(("%s QAM128\n", identity));
			#endif
    		break;
    		
    	case 5 /*101*/:
    		*Modulation = STTUNER_MOD_256QAM;
    		#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
            STTBX_Print(("%s QAM256\n", identity));
			#endif
    		break;
    		
    	default:
    		*Modulation = STTUNER_MOD_64QAM;
    		#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
            STTBX_Print(("%s QAM64-DEFAULT\n", identity));
			#endif
    		break;
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
    STTBX_Print(("%s Modulation=%u\n", identity, *Modulation));
#endif

    Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}




/* ----------------------------------------------------------------------------
Name: demod_d0297E_StandByMode()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */



ST_ErrorCode_t demod_d0297E_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode)
{
	
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
   const char *identity = "STTUNER d0297E.c demod_d0297E_StandByMode()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0297E_InstanceData_t *Instance;
 
    /* private driver instance data */
    Instance = D0297E_GetInstFromHandle(Handle);
   
      
    switch ( PowerMode)
    {
       case STTUNER_NORMAL_POWER_MODE :
       if(Instance->StandBy_Flag == 1)
       {
          Error|=ChipSetField( Instance->IOHandle, F297e_STANDBY, 0);
          
       }
          
             
       if(Error==ST_NO_ERROR)
       {
       	 
          Instance->StandBy_Flag = 0 ;
       }
       
       break;
       case STTUNER_STANDBY_POWER_MODE :
       if(Instance->StandBy_Flag == 0)
       {
          Error|=ChipSetField( Instance->IOHandle, F297e_STANDBY, 1);
          
          if(Error==ST_NO_ERROR)
          {
                  
             Instance->StandBy_Flag = 1 ;
             
          }
       }
       break ;
	   /* Switch statement */ 
  
    }
   
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297E_SetModulation()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297E_SetModulation(DEMOD_Handle_t Handle, STTUNER_Modulation_t Modulation)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
   const char *identity = "STTUNER d0297E.c demod_d0297E_SetModulation()";
#endif
    STTUNER_InstanceDbase_t  *Inst;
    D0297E_InstanceData_t   *Instance;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    Instance = D0297E_GetInstFromHandle(Handle);
    Inst = STTUNER_GetDrvInst();
        
    switch(Modulation)
    {
        case STTUNER_MOD_4QAM:
            Instance->FE_297e_Modulation = FE_297e_MOD_QAM4;
            break;

        case STTUNER_MOD_16QAM:
            Instance->FE_297e_Modulation = FE_297e_MOD_QAM16;
            break;

        case STTUNER_MOD_32QAM:
            Instance->FE_297e_Modulation = FE_297e_MOD_QAM32;
            break;

        case STTUNER_MOD_64QAM:
            Instance->FE_297e_Modulation = FE_297e_MOD_QAM64;
            break;

        case STTUNER_MOD_128QAM:
            Instance->FE_297e_Modulation = FE_297e_MOD_QAM128;
            break;

        case STTUNER_MOD_256QAM:
            Instance->FE_297e_Modulation = FE_297e_MOD_QAM256;
            break;

        case STTUNER_MOD_QAM:
            Instance->FE_297e_Modulation = FE_297e_MOD_QAM64;
            break;
            
        default:
            return(ST_ERROR_BAD_PARAMETER);
    }

	Error = ChipSetField( Instance->IOHandle, F297e_QAM_MODE, Instance->FE_297e_Modulation);

	#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
	    STTBX_Print(("%s Modulation=%u\n", identity, Modulation));
	#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297E_GetAGC()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297E_GetAGC(DEMOD_Handle_t Handle, S16  *Agc)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
   const char *identity = "STTUNER d0297E.c demod_d0297E_GetAGC()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0297E_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0297E_GetInstFromHandle(Handle);

    *Agc = FE_297e_GetRFLevel( Instance->IOHandle);

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
    STTBX_Print(("%s Agc=%u\n", identity, *Agc));
#endif


    Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;
    
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297E_GetFECRates()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297E_GetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t *FECRates)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297E_IsLocked()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297E_IsLocked(DEMOD_Handle_t Handle, BOOL *IsLocked)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
   const char *identity = "STTUNER d0297E.c demod_d0297E_IsLocked()";
#endif
    ST_ErrorCode_t          Error = ST_NO_ERROR;
    D0297E_InstanceData_t    *Instance;

    /* private driver instance data */
    Instance = D0297E_GetInstFromHandle(Handle);

    *IsLocked = ChipGetField( Instance->IOHandle, F297e_QAMFEC_LOCK) ? TRUE : FALSE;
    #ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
    STTBX_Print(("%s IsLocked=%u\n", identity, *IsLocked));
    #endif    

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
    STTBX_Print(("%s TUNER IS %s\n", identity, (*IsLocked)?"LOCKED":"UNLOCKED"));
#endif

    Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297E_SetFECRates()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297E_SetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t FECRates)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297E_Tracking()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297E_Tracking(DEMOD_Handle_t Handle, BOOL ForceTracking, U32 *NewFrequency, BOOL *SignalFound)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
   const char *identity = "STTUNER d0297E.c demod_d0297E_Tracking()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    STTUNER_tuner_instance_t    *TunerInstance;     /* don't really need all these variables, done for clarity */
    STTUNER_InstanceDbase_t     *Inst;              /* pointer to instance database */
    STTUNER_Handle_t            TopHandle;          /* instance that contains the demods & tuner driver set */
    D0297E_InstanceData_t       *Instance;
    BOOL                        IsLocked;
    
    TUNER_Status_t  TunerStatus; 
    U32  MasterClock;
    FE_297e_SignalInfo_t	pInfo;
    

    /* private driver instance data */
    Instance = D0297E_GetInstFromHandle(Handle);
    /* top level public instance data */
    Inst = STTUNER_GetDrvInst();

    /* this driver knows what to level instance it belongs to */
    TopHandle = Instance->TopLevelHandle;

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[TopHandle].Cable.Tuner;

    Error = demod_d0297E_IsLocked(Handle, &IsLocked);
     *SignalFound = IsLocked;
    if ( (Error == ST_NO_ERROR) && IsLocked )
    {
        Error = (TunerInstance->Driver->tuner_GetStatus)(TunerInstance->DrvHandle, &TunerStatus);
	   
	    MasterClock = FE_297e_GetMclkFreq( Instance->IOHandle,Instance->ExternalClock);

	    Inst[TopHandle].CurrentTunerInfo.ScanInfo.SymbolRate = FE_297e_GetSymbolRate( Instance->IOHandle, MasterClock);
	    pInfo.SymbolRate = Inst[TopHandle].CurrentTunerInfo.ScanInfo.SymbolRate; /*for passing to FE_297e_GetSignalInfo*/
	    FE_297e_GetSignalInfo( Instance->IOHandle, &pInfo);
	    *NewFrequency = TunerInstance->realfrequency;
	    Inst[TopHandle].CurrentTunerInfo.BitErrorRate = pInfo.BER;
	    Inst[TopHandle].CurrentTunerInfo.SignalQuality = pInfo.CN_dBx10;
	    Inst[TopHandle].CurrentTunerInfo.ScanInfo.AGC = pInfo.Power_dBmx10;
	    demod_d0297E_GetModulation(Handle, &(Inst[TopHandle].CurrentTunerInfo.ScanInfo.Modulation) );
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
    STTBX_Print(("%s SignalFound=%u NewFrequency=%u\n", identity, *SignalFound, *NewFrequency));
#endif
    
    Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0297E_ScanFrequency()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297E_ScanFrequency(DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset,
                                                                U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                U32  FreqOff,          U32   ChannelBW,     S32   EchoPos)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
    clock_t                     time_start_lock, time_end_lock;
    const char *identity = "STTUNER d0297E.c demod_d0297E_ScanFrequency()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FE_297e_SearchParams_t Params;
    FE_297e_SearchResult_t Result;
    STTUNER_InstanceDbase_t     *Inst;
    D0297E_InstanceData_t        *Instance;
    STTUNER_Handle_t            TopHandle;
    STTUNER_tuner_instance_t    *TunerInstance;
    FE_297e_SignalInfo_t	pInfo;

    /* private driver instance data */
    Instance = D0297E_GetInstFromHandle(Handle);
    /* top level public instance data */
    Inst = STTUNER_GetDrvInst();

    /* this driver knows what to level instance it belongs to */
    TopHandle = Instance->TopLevelHandle;

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[Instance->TopLevelHandle].Cable.Tuner;
    
    Params.ExternalClock = Instance->ExternalClock;
    Params.Frequency_Khz   = InitialFrequency;              /* demodulator (output from LNB) frequency (in KHz) */
    Params.SymbolRate_Bds  = SymbolRate;                    /* transponder symbol rate  (in bds) */
    Params.SearchRange_Hz = 5000000;  /*nominal 280Khz*/
    Error = demod_d0297E_SetModulation(Handle, Inst[TopHandle].CurrentTunerInfo.ScanInfo.Modulation); /*fills in Instance->FE_297e_Modulation*/
    Params.Modulation  = Instance->FE_297e_Modulation;
    Params.SweepRate_Hz = 0;


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
            Inst[Instance->TopLevelHandle].Cable.Modulation = Instance->FE_297e_Modulation;
            break;
    }

    Error |= FE_297e_Search( Instance->IOHandle, &Params, &Result, Instance->TopLevelHandle );

    if (Error == FE_297e_BAD_PARAMETER)  /* element(s) of Params bad */
    {
        #ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s fail, scan not done: bad parameter(s) to FE_297e_Search() == FE_297e_BAD_PARAMETER\n", identity ));
        #endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    else if (Error == FE_297e_SEARCH_FAILED)  /* found no signal within limits */
    {
	#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s FE_297e_Search() == FE_297e_SEARCH_FAILED\n", identity ));
 	#endif
        return(ST_NO_ERROR);    /* no error so try next F or stop if band limits reached */
    }
    *ScanSuccess = Result.Locked;

    if (Result.Locked == TRUE)
    {
        
        Instance->Symbolrate = Result.SymbolRate_Bds;
        
        *NewFrequency = 1000*(Result.Frequency_Khz);
        /*lines for agc/rf level, ber,quality etc updation during initial scan/set freq before tracking is in effect, below*/
	    pInfo.SymbolRate = Result.SymbolRate_Bds; /*for passing to FE_297e_GetSignalInfo*/
	    /*FE_297e_GetSignalInfo( Instance->IOHandle, &pInfo);*/ /*To speedup aquisition, BER reporting only done while tracking*/
	    Inst[TopHandle].CurrentTunerInfo.BitErrorRate = 0/*pInfo.BER*/;
	    /*Inst[TopHandle].CurrentTunerInfo.SignalQuality = pInfo.CN_dBx10*/;
	    Inst[TopHandle].CurrentTunerInfo.SignalQuality = FE_297e_GetCarrierToNoiseRatio_u32(Instance->IOHandle,Instance->FE_297e_Modulation);
	    /*Inst[TopHandle].CurrentTunerInfo.ScanInfo.AGC = pInfo.Power_dBmx10;*/
	    Inst[TopHandle].CurrentTunerInfo.ScanInfo.SymbolRate=Instance->Symbolrate;
      }


#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
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

    Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}

/* ----------------------------------------------------------------------------
Name: demod_d0297E_ioctl()

Description:
    access device specific low-level functions

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0297E_ioctl(DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
   const char *identity = "STTUNER d0297E.c demod_d0297E_ioctl()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0297E_InstanceData_t *Instance;
    STTUNER_InstanceDbase_t  *Inst;             /* pointer to instance database */

    /* private driver instance data */
    Instance = D0297E_GetInstFromHandle(Handle);
    /* top level public instance data */ 
    Inst = STTUNER_GetDrvInst();

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s demod driver instance handle=%d\n", identity, Handle));
        STTBX_Print(("%s                     function=%d\n", identity, Function));
        STTBX_Print(("%s                   I/O handle=%d\n", identity, Instance->IOHandle));
#endif

    /* ---------- select what to do ---------- */
    switch(Function){

        case STTUNER_IOCTL_RAWIO: /* read/write device register (actual write to stv0297e) */
                                                  
  	        switch(((CABIOCTL_IOARCH_Params_t *)InParams)->Operation)
  	        {
  	    	    case STTUNER_IO_READ:
                  Error = ChipGetRegisters(Instance->IOHandle,((CABIOCTL_IOARCH_Params_t *)InParams)->SubAddr, ((CABIOCTL_IOARCH_Params_t *)InParams)->TransferSize,((CABIOCTL_IOARCH_Params_t *)InParams)->Data);
                break;
                
                case STTUNER_IO_WRITE:
                	Error = ChipSetRegisters(Instance->IOHandle, ((CABIOCTL_IOARCH_Params_t *)InParams)->SubAddr, ((CABIOCTL_IOARCH_Params_t *)InParams)->Data, ((CABIOCTL_IOARCH_Params_t *)InParams)->TransferSize);
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
                  ((CABIOCTL_SETREG_InParams_t *)InParams)->RegIndex,
                  ((CABIOCTL_SETREG_InParams_t *)InParams)->Value );
            break;

        default:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
    STTBX_Print(("%s called ok\n", identity, Function));    /* signal that function came back */
#endif

    Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error; /* Also accumulate I2C error */
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;

    return(Error);

}



/* ----------------------------------------------------------------------------
Name: demod_d0297E_repeateraccess()

Description:
    called from ioarch.c when some driver does I/O but with the repeater/passthru
    set to point to this function.
Parameters:

Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t demod_d0297E_repeateraccess(DEMOD_Handle_t Handle,BOOL REPEATER_STATUS)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0297E_InstanceData_t *Instance;
    Instance = D0297E_GetInstFromHandle(Handle);  
    
    
    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }
     
       if (REPEATER_STATUS)
       {
        
          Error = ChipSetField( Instance->IOHandle, F297e_I2CT_EN, 1);
       }
       
    return(Error);
}


/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */

D0297E_InstanceData_t *D0297E_GetInstFromHandle(DEMOD_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E_HANDLES
   const char *identity = "STTUNER d0297E.c D0297E_GetInstFromHandle()";
#endif
    D0297E_InstanceData_t *Instance;

    Instance = (D0297E_InstanceData_t *)Handle;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_D0297E_HANDLES
    STTBX_Print(("%s block at 0x%08x\n", identity, Instance));
#endif

    return(Instance);
}


/* End of d0297E.c */

