/* ----------------------------------------------------------------------------
File Name: d0288.c

Description: 
    stx0288 demod driver.


Copyright (C) 2005-2006 STMicroelectronics

History: 
   date: 04-Aug-2005
version: 1.0.0A1
 author: SD
comment: First LLA Integration

  date: 01-Aug-2006
version: 1.0.0A1
 author:HS, SD
comment: Changing according to new IOREG
 
---------------------------------------------------------------------------- */

/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
   #include  <stdlib.h>
   #include <string.h>
#endif
#include "stlite.h"     /* Standard includes */
#include "sttbx.h"
#include "sttuner.h"                    
/* local to sttuner */
#include "stevt.h"
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "ioarch.h"     /* I/O for this driver */
#include "ioreg.h"      /* I/O for this driver */
#include "chip.h"
#include "sioctl.h"     /* data structure typedefs for all the the sat ioctl functions */
#include "drv0288.h"    /* misc driver functions */
#include "d0288.h"      /* header for this file */
#include "util288.h"
/* Device capabilities */
#define MAX_AGC                         255
#define MAX_SIGNAL_QUALITY              100
#define MAX_BER                         200000

#ifdef STTUNER_DRV_SAT_SCR
extern ST_ErrorCode_t scr_scrdrv_SetFrequency   (STTUNER_Handle_t Handle, DEMOD_Handle_t DemodHandle, ST_DeviceName_t *DeviceName, U32  InitialFrequency,  U8 LNBIndex, U8 SCRBPF );
#endif
#define I2C_HANDLE(x)      (*((STI2C_Handle_t *)x))
#define STX0288_SYMBOL_RATE_MIN 1000000
#define STX0288_SYMBOL_RATE_MAX 60000000
#define STX0288_MCLK 100000000
/* private variables ------------------------------------------------------- */

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */

extern IOARCH_HandleData_t IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];


static BOOL        Installed = FALSE;
	/* ---------- per instance of driver ---------- */
typedef struct
{
    ST_DeviceName_t            	*DeviceName;           /* unique name for opening under */
    STTUNER_Handle_t          	TopLevelHandle;       /* access tuner, lnb etc. using this */
    IOARCH_Handle_t          	IOHandle;             /* instance access to I/O driver     */
    ST_Partition_t            	*MemoryPartition;     /* which partition this data block belongs to */
    void                      	*InstanceChainPrev;   /* previous data block in chain or NULL if not */
    void                      	*InstanceChainNext;   /* next data block in chain or NULL if last */
    U32                         ExternalClock;  /* External VCO */
    STTUNER_TSOutputMode_t      TSOutputMode;
    STTUNER_SerialDataMode_t    SerialDataMode;  
    STTUNER_BlockSyncMode_t     BlockSyncMode;
    STTUNER_SerialClockSource_t SerialClockSource;
    STTUNER_FECMode_t           FECMode;
    STTUNER_DiSEqCConfig_t      DiSEqCConfig; /** Added DiSEqC configuration structure to get the trace of current state**/
    BOOL                        DISECQ_ST_ENABLE;
    STTUNER_DataClockPolarity_t      ClockPolarity;
    STTUNER_DataClockAtParityBytes_t DataClockAtParityBytes;
    STTUNER_DataFIFOMode_t      DataFIFOMode;      /* add block sync bit control for bug GNBvd27452*/
    STTUNER_OutputFIFOConfig_t  OutputFIFOConfig;  /* add block sync bit control for bug GNBvd27452*/
    U32 Symbolrate;
    U32                         DiSEqC_RxFreq;
    BOOL                        SET_TSCLOCK; 
    U32                         StandBy_Flag; /*** To take care same Standby Mode doesnot
                                                   execute more than once ***/
}
D0288_InstanceData_t;


/**************extern from  open.c************************/
#ifdef STTUNER_DRV_SAT_SCR
#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
extern U32 DemodDrvHandleOne;
#endif
#endif

/* instance chain, the default boot value is invalid, to catch errors */
static D0288_InstanceData_t *InstanceChainTop = (D0288_InstanceData_t *)0x7fffffff;

/* API */
ST_ErrorCode_t demod_d0288_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams);
ST_ErrorCode_t demod_d0288_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams);

ST_ErrorCode_t demod_d0288_Open (ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle);
ST_ErrorCode_t demod_d0288_Close(DEMOD_Handle_t  Handle, DEMOD_CloseParams_t *CloseParams);
    
ST_ErrorCode_t demod_d0288_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber);
ST_ErrorCode_t demod_d0288_GetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation);
ST_ErrorCode_t demod_d0288_SetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t  Modulation);
ST_ErrorCode_t demod_d0288_GetAGC          (DEMOD_Handle_t Handle, S16                  *Agc);
ST_ErrorCode_t demod_d0288_GetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t    *FECRates);
ST_ErrorCode_t demod_d0288_GetIQMode       (DEMOD_Handle_t Handle, STTUNER_IQMode_t     *IQMode); /*added for GNBvd26107->I2C failure due to direct access to demod device at API level*/
ST_ErrorCode_t demod_d0288_IsLocked        (DEMOD_Handle_t Handle, BOOL                 *IsLocked);
ST_ErrorCode_t demod_d0288_SetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t     FECRates);
ST_ErrorCode_t demod_d0288_Tracking        (DEMOD_Handle_t Handle, BOOL ForceTracking,    U32  *NewFrequency,  BOOL *SignalFound);

ST_ErrorCode_t demod_d0288_ScanFrequency   (DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset, 
                                                                   U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                   U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                   U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                   U32  FreqOff,          U32   ChannelBW,     S32   EchoPos);

/* added for DiSEqC API support*/
ST_ErrorCode_t demod_d0288_DiSEqC       (DEMOD_Handle_t Handle, 
							       STTUNER_DiSEqCSendPacket_t *pDiSEqCSendPacket,
							       STTUNER_DiSEqCResponsePacket_t *pDiSEqCResponsePacket
							       );

ST_ErrorCode_t demod_d0288_DiSEqCGetConfig ( DEMOD_Handle_t Handle ,STTUNER_DiSEqCConfig_t * DiSEqCConfig);
ST_ErrorCode_t demod_d0288_DiSEqCBurstOFF ( DEMOD_Handle_t Handle );
ST_ErrorCode_t demod_0288_GetSymbolrate(DEMOD_Handle_t Handle,   U32  *SymbolRate);
ST_ErrorCode_t demod_d0288_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode);



U8 Def288Val[STX288_NBREGS]=
{
	/* 0x0000 RO */ 0x10,/* 0x0001    */ 0x25,/* 0x0002    */ 0x20,/* 0x0003    */ 0x8e,/* 0x0004    */ 0x8e,
	/* 0x0005    */ 0x16,/* 0x0006    */ 0x00,/* 0x0007 RO */ 0x20,/* 0x0008 RO */ 0x00,/* 0x0009    */ 0x00,
	/* 0x000a    */ 0x04,/* 0x000b    */ 0x00,/* 0x000c    */ 0x00,/* 0x000d    */ 0x00,/* 0x000e    */ 0xd4,
	/* 0x000f    */ 0x48,/* 0x0010 RO */ 0xac,/* 0x0011    */ 0x7a,/* 0x0012    */ 0x03,/* 0x0013    */ 0x48,
	/* 0x0014    */ 0x84,/* 0x0015    */ 0xc5,/* 0x0016    */ 0xb7,/* 0x0017    */ 0x9c,/* 0x0018    */ 0x00,
	/* 0x0019    */ 0xa6,/* 0x001a    */ 0x88,/* 0x001b    */ 0x8f,/* 0x001c    */ 0xf0,/* 0x001e RO */ 0x80,
	/* 0x001f RO */ 0x26,/* 0x0020    */ 0x0b,/* 0x0021    */ 0x54,/* 0x0022    */ 0xff,/* 0x0023    */ 0x01,
	/* 0x0024 RO */ 0x9a,/* 0x0025 RO */ 0x7f,/* 0x0026 RO */ 0x00,/* 0x0027 RO */ 0x00,/* 0x0028    */ 0x46,
	/* 0x0029    */ 0x66,/* 0x002a    */ 0x90,/* 0x002b    */ 0xfe,/* 0x002c    */ 0x78,/* 0x002d RO */ 0x0e,
	/* 0x002e RO */ 0x5d,/* 0x002f RO */ 0x00,/* 0x0030    */ 0x00,/* 0x0031    */ 0x1e,/* 0x0032    */ 0x14,
	/* 0x0033    */ 0x0f,/* 0x0034    */ 0x09,/* 0x0035    */ 0x0c,/* 0x0036    */ 0x05,/* 0x0037    */ 0x2f,
	/* 0x0038    */ 0x16,/* 0x0039    */ 0xbd,/* 0x003a    */ 0x00,/* 0x003b    */ 0x13,/* 0x003c    */ 0x11,
#ifdef STTUNER_DRV_SAT_5188
	/* 0x003d    */ 0x30,/* 0x003e RO */ 0x00,/* 0x003f RO */ 0x00,/* 0x0040    */ 0x13,/* 0x0041    */ 0x00,	/* for STX5188 with 30MHz clock */    
#else
	/* 0x003d    */ 0x30,/* 0x003e RO */ 0x00,/* 0x003f RO */ 0x00,/* 0x0040    */ 0x63,/* 0x0041    */ 0x04,	/* for STX0288 with 4MHz crystal */
#endif
	/* 0x0042    */ 0x60,/* 0x0043    */ 0x00,/* 0x0044    */ 0x00,/* 0x0045    */ 0x00,/* 0x0046    */ 0x00,   
	/* 0x0047    */ 0x00,/* 0x004a    */ 0x00,/* 0x004b RO */ 0x1b,/* 0x004c RO */ 0x28,/* 0x0050    */ 0x10,
	/* 0x0051    */ 0x36,/* 0x0052    */ 0x09,/* 0x0053    */ 0x94,/* 0x0054    */ 0xb2,/* 0x0055    */ 0x29,
	/* 0x0056    */ 0x64,/* 0x0057    */ 0x2b,/* 0x0058    */ 0x54,/* 0x0059    */ 0x86,/* 0x005a    */ 0x00,
	/* 0x005b    */ 0x96,/* 0x005c    */ 0x08,/* 0x005d    */ 0x7f,/* 0x005e    */ 0xff,/* 0x005f    */ 0x8d,
	/* 0x0060    */ 0x82,/* 0x0061    */ 0x82,/* 0x0062    */ 0x82,/* 0x0063    */ 0x00,/* 0x0064    */ 0x02,
	/* 0x0065    */ 0x02,/* 0x0066    */ 0x82,/* 0x0067    */ 0x82,/* 0x0068    */ 0x82,/* 0x0069    */ 0x82,
	/* 0x006a RO */ 0x38,/* 0x006b RO */ 0x0c,/* 0x006c RO */ 0x00,/* 0x0070    */ 0x00,/* 0x0071    */ 0x00,
	/* 0x0072    */ 0x00,/* 0x0074    */ 0x00,/* 0x0075    */ 0x00,/* 0x0076    */ 0x00,/* 0x0081    */ 0x00,
	/* 0x0082    */ 0x3f,/* 0x0083    */ 0x3f,/* 0x0084    */ 0x00,/* 0x0085    */ 0x00,/* 0x0088    */ 0x00,
	/* 0x0089    */ 0x00,/* 0x008a    */ 0x00,/* 0x008b    */ 0x00,/* 0x008c    */ 0x00,/* 0x0090    */ 0x00,
	/* 0x0091    */ 0x00,/* 0x0092    */ 0x00,/* 0x0093    */ 0x00,/* 0x0094    */ 0x1c,/* 0x0097    */ 0x00,
	/* 0x00a0    */ 0x48,/* 0x00a1    */ 0x00,/* 0x00b0    */ 0xb8,/* 0x00b1    */ 0x3a,/* 0x00b2    */ 0x10,
	/* 0x00b3    */ 0x82,/* 0x00b4    */ 0x00,/* 0x00b5    */ 0x82,/* 0x00b6    */ 0x82,/* 0x00b7    */ 0x82,
	/* 0x00b8    */ 0x20,/* 0x00b9    */ 0x00,/* 0x00f1    */ 0x00,/* 0x00f0    */ 0x00,/* 0x00f2    */ 0xc0
};


U16 Addressarray[STX288_NBREGS]=
{	
0x0000,
0x0001,
0x0002,
0x0003,
0x0004,
0x0005,
0x0006,
0x0007,
0x0008,
0x0009,
0x000a,
0x000b,
0x000c,
0x000d,
0x000e,
0x000f,
0x0010,
0x0011,
0x0012,
0x0013,
0x0014,
0x0015,
0x0016,
0x0017,
0x0018,
0x0019,
0x001a,
0x001b,
0x001c,
0x001e,
0x001f,
0x0020,
0x0021,
0x0022,
0x0023,
0x0024,
0x0025,
0x0026,
0x0027,
0x0028,
0x0029,
0x002a,
0x002b,
0x002c,
0x002d,
0x002e,
0x002f,
0x0030,
0x0031,
0x0032,
0x0033,
0x0034,
0x0035,
0x0036,
0x0037,
0x0038,
0x0039,
0x003a,
0x003b,
0x003c,
0x003d,
0x003e,
0x003f,
0x0040,
0x0041,
0x0042,
0x0043,
0x0044,
0x0045,
0x0046,
0x0047,
0x004a,
0x004b,
0x004c,
0x0050,
0x0051,
0x0052,
0x0053,
0x0054,
0x0055,
0x0056,
0x0057,
0x0058,
0x0059,
0x005a,
0x005b,
0x005c,
0x005d,
0x005e,
0x005f,
0x0060,
0x0061,
0x0062,
0x0063,
0x0064,
0x0065,
0x0066,
0x0067,
0x0068,
0x0069,
0x006a,
0x006b,
0x006c,
0x0070,
0x0071,
0x0072,
0x0074,
0x0075,
0x0076,
0x0081,
0x0082,
0x0083,
0x0084,
0x0085,
0x0088,
0x0089,
0x008a,
0x008b,
0x008c,
0x0090,
0x0091,
0x0092,
0x0093,
0x0094,
0x0097,
0x00a0,
0x00a1,
0x00b0,
0x00b1,
0x00b2,
0x00b3,
0x00b4,
0x00b5,
0x00b6,
0x00b7,
0x00b8,
0x00b9,
0x00f1,
0x00f0,
0x00f2,
};


#define MAX_ADDRESS 0x00f2

/***************************************/
ST_ErrorCode_t demod_d0288_ioctl  (DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams,
                                             STTUNER_Da_Status_t *Status);

/* I/O API */

ST_ErrorCode_t demod_d0288_repeateraccess(DEMOD_Handle_t Handle, BOOL REPEATER_STATUS );

ST_ErrorCode_t demod_d0288_DiseqcInit(DEMOD_Handle_t Handle);
/* local functions --------------------------------------------------------- */
#ifdef STTUNER_DRV_SAT_SCR
ST_ErrorCode_t demod_d0288_tonedetection(DEMOD_Handle_t Handle,U32 StartFreq, U32 StopFreq, U8  *NbTones,U32 *ToneList, U8 mode,int* power_detection_level);
#endif
D0288_InstanceData_t *D0288_GetInstFromHandle(DEMOD_Handle_t Handle);

/***************************************

Name: STTUNER_DRV_DEMOD_STV0288_Install()

Description:
    install a satellite device driver into the demod database.
    
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STX0288_Install(STTUNER_demod_dbase_t *Demod)
{
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    const char *identity = "STTUNER d0288.c STTUNER_DRV_DEMOD_STX0288_Install()";
    #endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == TRUE)
    {
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s fail driver already installed\n", identity));
    #endif
    return(STTUNER_ERROR_INITSTATE);
    }
   
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print(("%s installing sat:demod:STX0288...", identity));
    #endif

    /* mark ID in database */
    Demod->ID = STTUNER_DEMOD_STX0288;

    /* map API */
    Demod->demod_Init                  = demod_d0288_Init;
    Demod->demod_Term                  = demod_d0288_Term;
    Demod->demod_Open                  = demod_d0288_Open;
    Demod->demod_Close                 = demod_d0288_Close;
    Demod->demod_GetSignalQuality      = demod_d0288_GetSignalQuality;
    Demod->demod_GetModulation         = demod_d0288_GetModulation;   
    Demod->demod_SetModulation         = demod_d0288_SetModulation;   
    Demod->demod_GetAGC                = demod_d0288_GetAGC;          
    Demod->demod_GetFECRates           = demod_d0288_GetFECRates;  
    Demod->demod_GetIQMode             = demod_d0288_GetIQMode; /*added for GNBvd26107->I2C failure due to direct access to demod device at API level*/
    Demod->demod_IsLocked              = demod_d0288_IsLocked ;       
    Demod->demod_SetFECRates           = demod_d0288_SetFECRates;     
    Demod->demod_Tracking              = demod_d0288_Tracking;        
    Demod->demod_ScanFrequency         = demod_d0288_ScanFrequency;
    Demod->demod_DiSEqC	               = demod_d0288_DiSEqC;
    Demod->demod_GetConfigDiSEqC       = demod_d0288_DiSEqCGetConfig;  
    Demod->demod_SetDiSEqCBurstOFF     = demod_d0288_DiSEqCBurstOFF; 
    Demod->demod_ioaccess                     = NULL;
    Demod->demod_repeateraccess              = demod_d0288_repeateraccess;
    Demod->demod_ioctl                 = demod_d0288_ioctl;
    Demod->demod_GetSymbolrate         = demod_0288_GetSymbolrate;
    #ifdef STTUNER_DRV_SAT_SCR
    Demod->demod_tonedetection         = demod_d0288_tonedetection;
    #endif
    Demod->demod_StandByMode           = demod_d0288_StandByMode;
    InstanceChainTop = NULL;

    Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);
   

    Installed = TRUE;

    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print(("ok\n"));
    #endif

    return(Error);
}

/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STX0288_UnInstall()

Description:
    install a satellite device driver into the demod database.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STX0288_UnInstall(STTUNER_demod_dbase_t *Demod)
{
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    const char *identity = "STTUNER d0288.c STTUNER_DRV_DEMOD_STX0288_UnInstall()";
    #endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s fail driver not installed\n", identity));
    #endif
    return(STTUNER_ERROR_INITSTATE);
    }
   
    if(Demod->ID != STTUNER_DEMOD_STX0288)
    {
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s fail incorrect driver type\n", identity));
    #endif
    return(STTUNER_ERROR_ID);
    }

    /* has all memory been freed, by Term() */
    if(InstanceChainTop != NULL)
    {
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s fail at least one instance not terminated\n", identity));
    #endif
    return(ST_ERROR_OPEN_HANDLE);
    }


    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print(("%s uninstalling sat:demod:STV0288...", identity));
    #endif

    Demod->ID = STTUNER_NO_DRIVER;
    Demod->demod_Init             = NULL;
    Demod->demod_Term             = NULL;
    Demod->demod_Open             = NULL;
    Demod->demod_Close            = NULL;
    Demod->demod_IsAnalogCarrier  = NULL; 
    Demod->demod_GetSignalQuality = NULL;
    Demod->demod_GetModulation    = NULL;   
    Demod->demod_SetModulation    = NULL;   
    Demod->demod_GetAGC           = NULL;          
    Demod->demod_GetFECRates      = NULL;      
    Demod->demod_GetIQMode        = NULL; /*added for GNBvd26107->I2C failure due to direct access to demod device at API level*/
    Demod->demod_IsLocked         = NULL;       
    Demod->demod_SetFECRates      = NULL;     
    Demod->demod_Tracking         = NULL;        
    Demod->demod_ScanFrequency    = NULL;
    Demod->demod_DiSEqC		  = NULL;
    Demod->demod_GetConfigDiSEqC  = NULL;  
    Demod->demod_SetDiSEqCBurstOFF= NULL; 
    Demod->demod_ioaccess	  = NULL;
    Demod->demod_repeateraccess	  = NULL;
    Demod->demod_ioctl		  = NULL;
    Demod->demod_GetSymbolrate    = NULL;
    #ifdef STTUNER_DRV_SAT_SCR
    Demod->demod_tonedetection    = NULL;
    #endif
    Demod->demod_StandByMode      = NULL;
    
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print(("<"));
    #endif

    STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);
  

    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print((">"));
    #endif

    InstanceChainTop = (D0288_InstanceData_t *)0x7ffffffe;
    Installed        = FALSE;

    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("ok\n"));
    #endif

    return(Error);
}

/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
Name: demod_d0288_Init()

Description:
    
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0288_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams)
{
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    const char *identity = "STTUNER d0288.c demod_d0288_Init()";
    #endif   
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0288_InstanceData_t *InstanceNew, *Instance;
    if(Installed == FALSE)
    {
        #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
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
	#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
	#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull(DeviceName);
    if( Error != ST_NO_ERROR)
    {
	#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s fail DeviceName not valid\n", identity));
	#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( D0288_InstanceData_t ));
    if (InstanceNew == NULL)
    {
	#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
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

    
    
	IOARCH_Handle[InstanceNew->IOHandle ].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
	IOARCH_Handle[InstanceNew->IOHandle ].DeviceMap.Registers = STX288_NBREGS;
	IOARCH_Handle[InstanceNew->IOHandle ].DeviceMap.Fields    = STX288_NBFIELDS;
	IOARCH_Handle[InstanceNew->IOHandle ].DeviceMap.Mode      = IOREG_MODE_SUBADR_8; /* NEW as of 3.4.0: i/o addressing mode to use */
	IOARCH_Handle[InstanceNew->IOHandle ].DeviceMap.MemoryPartition = InitParams->MemoryPartition;
	IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.DefVal= (U32 *)&Def288Val[0];
	IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[InstanceNew->IOHandle].DeviceMap.MemoryPartition,MAX_ADDRESS,sizeof(U8));

    
    
    InstanceNew->InstanceChainNext   = NULL; /* always last in the chain */
    InstanceNew->ExternalClock       = InitParams->ExternalClock;  
    #ifdef STTUNER_DRV_SAT_5188
    InstanceNew->ExternalClock = 30000000;
    #else
    InstanceNew->ExternalClock = 4000000;
    #endif
    InstanceNew->TSOutputMode        = InitParams->TSOutputMode;
    InstanceNew->SerialDataMode      = InitParams->SerialDataMode;
    InstanceNew->BlockSyncMode       = InitParams->BlockSyncMode;/* add block sync bit control for bug GNBvd27452*/
    InstanceNew->FECMode             = InitParams->FECMode;
    InstanceNew->ClockPolarity       = InitParams->ClockPolarity;
    InstanceNew->DataClockAtParityBytes = InitParams->DataClockAtParityBytes;
    InstanceNew->DataFIFOMode        = InitParams->DataFIFOMode;
    InstanceNew->OutputFIFOConfig    = InitParams->OutputFIFOConfig;
    InstanceNew->DiSEqC_RxFreq       = 22000;
    /*********************Added for Diseqc***************************/
    InstanceNew->DiSEqCConfig.Command=STTUNER_DiSEqC_COMMAND;
    InstanceNew->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
    /**************************************************/



#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes)\n", identity, InstanceNew->DeviceName, (U32)InstanceNew, sizeof( D0288_InstanceData_t ) ));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);

    return(Error);
}   

/* ----------------------------------------------------------------------------
Name: demod_d0288_Term()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0288_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams)
{
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    const char *identity = "STTUNER d0288.c demod_d0288_Term()";
    #endif	 
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0288_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
	#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
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
	#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s fail TermParams not valid\n", identity));
	#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
	#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s fail nothing initalized\n", identity));
	#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    Instance = InstanceChainTop;
    while(1)
    {
	#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("(%s)", Instance->DeviceName));
	#endif
        if ( strcmp( (char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
	    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
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
            STOS_MemoryDeallocate(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream);
	    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
	    #endif
	    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
            STTBX_Print(("%s terminated ok\n", identity));
            #endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL) 
        {       /* error we should have found a matching name before the end of the list */
		#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
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


    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
    #endif
    SEM_UNLOCK(Lock_InitTermOpenClose);

    return(Error);
}   

/* ----------------------------------------------------------------------------
Name: demod_d0288_Open()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0288_Open(ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle)
{
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    const char *identity = "STTUNER d0288.c demod_d0288_Open()";
    #endif		   
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    U8 ChipID,index;
    
    D0288_InstanceData_t     *Instance;
    STTUNER_tuner_instance_t *TunerInstance;
    STTUNER_InstanceDbase_t  *Inst;

    if(Installed == FALSE)
    {
        #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s fail driver not installed\n", identity));
	#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
	#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s fail nothing initalized\n", identity));
	#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }
    /* find  handle from name */
    Instance = InstanceChainTop;
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
    #endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL) 
        {       
	    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
            #endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
	#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("(%s)", Instance->DeviceName));
	#endif
    }
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print((" found ok\n"));
    #endif

    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print(("%s using block at 0x%08x\n", identity, (U32)Instance));
    #endif
    /* check handle IS actually free */
    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
        #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s fail driver instance already open\n", identity));
	#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }
    /* now got pointer to free (and valid) data block */
    Inst = STTUNER_GetDrvInst();    /* pointer to instance database */
   

    ChipID = ChipGetOneRegister( Instance->IOHandle, R288_ID);
#ifndef STTUNER_DRV_SAT_5188
    if ( (ChipID & 0xF0) !=  0x10)
    {
	#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s fail device not found (0x%02x)\n", identity, ChipID));
	#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    else
    {
	#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s device found, release/revision=%u\n", identity, ChipID & 0x0F));
	#endif
    }
#endif


   
   for (index=0;index < STX288_NBREGS;index++)
     {
     ChipSetOneRegister(Instance->IOHandle,Addressarray[index],Def288Val[index]);
     }
    
    
    if(((Inst[OpenParams->TopLevelHandle].Sat.Tuner.Driver->ID)==STTUNER_TUNER_STB6000) || ((Inst[OpenParams->TopLevelHandle].Sat.Tuner.Driver->ID)==STTUNER_TUNER_STB6100))
    {
	ChipSetField( Instance->IOHandle, F288_IAGC,1);
    }
    else
    {
	ChipSetField( Instance->IOHandle, F288_IAGC,0);	
    }

    /* Set MasterClock Freq */
   
    if (Error != ST_NO_ERROR)
    {
	#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s fail reset device\n", identity));
	#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);           
    } 
    /* Set capabilities */
    Capability->FECAvail         = STTUNER_FEC_1_2 | STTUNER_FEC_2_3 | STTUNER_FEC_3_4 |
                                   STTUNER_FEC_5_6 | STTUNER_FEC_6_7 | STTUNER_FEC_7_8;
    Capability->ModulationAvail  = STTUNER_MOD_ALL;  /* direct mapping to STTUNER_Modulation_t */
    Capability->AGCControl       = FALSE;
    Capability->SymbolMin        = STX0288_SYMBOL_RATE_MIN; /*   1 MegaSymbols/sec */
    Capability->SymbolMax        = STX0288_SYMBOL_RATE_MAX; /*  60 MegaSymbols/sec */
    Capability->BerMax           = MAX_BER; 
    Capability->SignalQualityMax = MAX_SIGNAL_QUALITY;
    Capability->AgcMax           = MAX_AGC;
    
    switch (Instance->TSOutputMode)
    {
        case STTUNER_TS_MODE_SERIAL:
            ChipSetField( Instance->IOHandle, F288_OUTRS_PS, 1);
            break;

        default:
        case STTUNER_TS_MODE_PARALLEL:
        case STTUNER_TS_MODE_DEFAULT:
            ChipSetField( Instance->IOHandle, F288_OUTRS_PS, 0);
            break;
    }

    /* Set FEC mode */
    switch (Instance->FECMode)
    {
        case STTUNER_FEC_MODE_DIRECTV:
            ChipSetField( Instance->IOHandle, F288_FECMODE,0x04);
	    ChipSetField( Instance->IOHandle, F288_MODE_COEF, 1);/* Nyquist Filter, GNBvd31071*/
	    ChipSetField( Instance->IOHandle, F288_DESCRAM, 0);/* descrambler, GNBvd31071 */
	    /* Enable 2/3,6/7 and disable 1/2,3/4,5/6,7/8 */
	   ChipSetField( Instance->IOHandle,F288_PR_1_2,0);
	   ChipSetField( Instance->IOHandle,F288_PR_2_3,1);
	    ChipSetField( Instance->IOHandle,F288_PR_3_4,0);
	   ChipSetField( Instance->IOHandle,F288_PR_5_6,0);
	   ChipSetField( Instance->IOHandle,F288_PR_6_7,1);
	    ChipSetField( Instance->IOHandle,F288_PR_7_8,0);
	   
	    ChipSetField( Instance->IOHandle,F288_SWAP_ENABLE,0);
            break;

        default:
        case STTUNER_FEC_MODE_DEFAULT:
        case STTUNER_FEC_MODE_DVB:
       
            ChipSetField( Instance->IOHandle, F288_FECMODE, 0);
            ChipSetField( Instance->IOHandle, F288_MODE_COEF, 0);/* Nyquist Filter, GNBvd31071*/
	    ChipSetField( Instance->IOHandle, F288_DESCRAM, 1);/* descrambler, GNBvd31071 */
	    /* Enable 1/2,2/3,3/4,5/6,7/8 and disable 6/7*/
	    ChipSetField( Instance->IOHandle,F288_PR_1_2,1);
	    ChipSetField( Instance->IOHandle,F288_PR_2_3,1);
	    ChipSetField( Instance->IOHandle,F288_PR_3_4,1);
	   ChipSetField( Instance->IOHandle,F288_PR_5_6,1);
	    ChipSetField( Instance->IOHandle,F288_PR_6_7,0);
	    ChipSetField( Instance->IOHandle,F288_PR_7_8,1);
	    
	    ChipSetField( Instance->IOHandle,F288_SWAP_ENABLE,1);	/* enable automatic IQ invertion */
            break;
    }

    if (Instance->ClockPolarity == STTUNER_DATA_CLOCK_POLARITY_FALLING)    /* Rising Edge */
    {
        Error |= ChipSetField( Instance->IOHandle, F288_CLK_POL, 1);
    }
    else
    {
        Error |= ChipSetField( Instance->IOHandle, F288_CLK_POL, 0);
    }
        
    if (Instance->DataClockAtParityBytes == STTUNER_DATACLOCK_NULL_AT_PARITYBYTES)     /* Parity enable changes done for 5528 live decode issue*/
    {
        Error |= ChipSetField( Instance->IOHandle, F288_CLK_CFG, 1);
    }
    else
    {
        Error |= ChipSetField( Instance->IOHandle, F288_CLK_CFG, 0);
    }
    
    if(Instance->OutputFIFOConfig.OutputRateCompensationMode == STTUNER_COMPENSATE_DATAVALID)
    {
   	Error |= ChipSetField( Instance->IOHandle, F288_EN_STBACKEND,    0);
    }
    else if(Instance->OutputFIFOConfig.OutputRateCompensationMode == STTUNER_COMPENSATE_DATACLOCK)
    {
   	Error |= ChipSetField( Instance->IOHandle, F288_EN_STBACKEND,    1);
    }
    
   /* This will control sync byte, 0x47 or 0xb8 after every 8th packet*/
    switch(Instance->BlockSyncMode)
    { case STTUNER_SYNC_NORMAL:
           ChipSetField( Instance->IOHandle, F288_MPEG, 0);
           break;
      case STTUNER_SYNC_DEFAULT:
      case STTUNER_SYNC_FORCED:
           ChipSetField( Instance->IOHandle, F288_MPEG, 1);
           break;
       default:
           ChipSetField( Instance->IOHandle, F288_MPEG, 1);
            break;
     }
         
    /* Set default error count mode i.e., QPSK bit error rate */
    ChipSetField( Instance->IOHandle, F288_ERRMODE,   0);
    ChipSetField( Instance->IOHandle, F288_ERR_SOURCE, 0);
    ChipSetField( Instance->IOHandle, F288_NOE,         3);

    TunerInstance = &Inst[OpenParams->TopLevelHandle].Sat.Tuner; /* pointer to tuner for this instance */

    

    /* finally (as nor more errors to check for, allocate handle */
    Instance->TopLevelHandle = OpenParams->TopLevelHandle;
    #ifdef STTUNER_LNB_POWER_THRU_DEMODIO
    Inst[OpenParams->TopLevelHandle].Sat.Demod.DeviceMap = &IOARCH_Handle[Instance->IOHandle].DeviceMap;
    Inst[OpenParams->TopLevelHandle].Sat.Demod.TEN_FieldIndex = F288_IOP3_CFG;
    Inst[OpenParams->TopLevelHandle].Sat.Demod.VEN_FieldIndex = F288_IOP4_CFG;
    Inst[OpenParams->TopLevelHandle].Sat.Demod.VSEL_FieldIndex= F288_IOP5_CFG;
    #endif
    #ifdef STTUNER_LNB_TONE_THRU_DEMOD_DISEQC_PIN
    Inst[OpenParams->TopLevelHandle].Sat.Demod.DeviceMap = &IOARCH_Handle[Instance->IOHandle].DeviceMap;
    Inst[OpenParams->TopLevelHandle].Sat.Demod.TEN_FieldIndex = F288_DISEQC_MODE;
    #endif
    /* in loopthriugh mode the second demod which is connected by loopthrough of first cannot send disecq-st 
       command because of a DC block in the loop-through path. Presently it depends upon toplevelhandle but when tone detection 
       LLA of SatCR will be written "DISECQ_ST_ENABLE" will be initalized automatically depending upon the tone sent*/
    #ifdef STTUNER_DRV_SAT_SCR
		#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
		    if(OpenParams->TopLevelHandle == 0)
		    Instance->DISECQ_ST_ENABLE = TRUE;
		    if(OpenParams->TopLevelHandle == 1)
		    Instance->DISECQ_ST_ENABLE = FALSE;
	        #endif
	#endif
    
    *Handle = (U32)Instance;
    /*Error |= demod_d0288_DiseqcInit(*Handle);*/
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print(("%s opened ok\n", identity));
    #endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    /* GNBvd17801-> Error Handling for any error returned during I2C operation inside the driver */
    Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;
    return(Error);
}   

/* ----------------------------------------------------------------------------
Name: demod_d0288_Close()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0288_Close(DEMOD_Handle_t Handle, DEMOD_CloseParams_t *CloseParams)
{
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    const char *identity = "STTUNER d0288.c demod_d0288_Close()";
    #endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0288_InstanceData_t     *Instance;

    /* private driver instance data */
    Instance = D0288_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
	#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s fail driver not installed\n", identity));
	#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
	#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s fail nothing initalized\n", identity));
	#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }
    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
	#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s fail driver instance not open\n", identity));
	#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print(("%s closed\n", identity));
    #endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}   
/* ----------------------------------------------------------------------------
Name: demod_d0288_GetSignalQuality()

Description:
  Obtains a signal quality setting for the current lock.
    
Parameters:
    SignalQuality_p,    pointer to area to store the signal quality value.
    Ber,                pointer to area to store the bit error rate.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0288_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber)
{
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    const char *identity = "STTUNER d0288.c demod_d0288_GetSignalQuality()";
    #endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0288_InstanceData_t *Instance;
    /* private driver instance data */
    Instance = D0288_GetInstFromHandle(Handle);
    /* Read noise estimations for C/N and BER */
    *SignalQuality_p = FE_288_GetCarrierToNoiseRatio( Instance->IOHandle);
    *Ber = /*FE_288_GetDirectErrors( Instance->IOHandle, COUNTER1_288);*/0;
   
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print(("%s SignalQuality=%u Ber=%u\n", identity, *SignalQuality_p, *Ber));
    #endif
    
    /* GNBvd17801-> Error Handling for any error returned during I2C operation inside the driver */
    Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;

    return(Error);
}   
/* ----------------------------------------------------------------------------
Name: demod_d0288_GetModulation()

Description:
    This routine returns the modulation scheme in use by this device.
    ** Currently only QPSK is supported.
    
Parameters:
    Modulation, pointer to area to store modulation scheme.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0288_GetModulation(DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation)
{
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    const char *identity = "STTUNER d0288.c demod_d0288_GetModulation()";
    #endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0288_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0288_GetInstFromHandle(Handle);

    /* This implementation of the DEMOD device only supports one type of modulation */
    *Modulation = STTUNER_MOD_QPSK;

    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print(("%s Modulation=%u\n", identity, *Modulation));
    #endif
    return(Error);
}   
/* ----------------------------------------------------------------------------
Name: demod_d0288_SetModulation()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0288_SetModulation(DEMOD_Handle_t Handle, STTUNER_Modulation_t Modulation)
{
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    const char *identity = "STTUNER d0288.c demod_d0288_SetModulation()";
    #endif
    /* if modulations include STTUNER_MOD_QPSK */
    if((Modulation & STTUNER_MOD_QPSK) == STTUNER_MOD_QPSK)
    { 
        #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s Modulation: STTUNER_MOD_QPSK accepted ok.\n", identity));
	#endif
        return(ST_NO_ERROR);
    }
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print(("%s fail, only STTUNER_MOD_QPSK supported.\n", identity));
    #endif
    return(ST_ERROR_BAD_PARAMETER);
}   
/* ----------------------------------------------------------------------------
Name: demod_d0288_GetAGC()

Description:
    Obtains the current value from the AGC integrator register and computes
    a look-up of power output.
    
Parameters:
    AGC,    pointer to area to store power output value.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0288_GetAGC(DEMOD_Handle_t Handle, S16  *Agc)
{
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    const char *identity = "STTUNER d0288.c demod_d0288_GetAGC()";
    #endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0288_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0288_GetInstFromHandle(Handle);

    *Agc = FE_288_GetRFLevel( Instance->IOHandle);

    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print(("%s Agc=%u\n", identity, *Agc));
    #endif
    
    /* GNBvd17801-> Error Handling for any error returned during I2C operation inside the driver */
    Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;
    
    return(Error);
}   
/* ----------------------------------------------------------------------------
Name: demod_d0288_GetFECRates()

Description:
     Checks the VEN rate register to deduce the forward error correction
    setting that is currently in use.
   
Parameters:
    FECRates,   pointer to area to store FEC rates in use.
  
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0288_GetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t *FECRates)
{
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    const char *identity = "STTUNER d0288.c demod_d0288_GetFECRates()";
    #endif
    U8 Data;
    U8 uData1; /* Used for retrieving the FEC Mode*/
    STTUNER_FECRate_t CurFecRate;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0288_InstanceData_t *Instance;
    /* private driver instance data */
    Instance = D0288_GetInstFromHandle(Handle);
    /*
    ** uData1 - FEC Mode Register
    ** 0 - DVB QPSK, 1 - DVB BPSK, 4 = DSS
    */
    uData1 = ChipGetField( Instance->IOHandle,F288_FECMODE);
    /* Convert venrate value to a STTUNER fecrate */
    /* Following switch statement is added to preempt bug GNBvd14519. Which required
       an error to be reported whenever a wrong FEC rate was specified in either of DVB or DirecTv Mode*/
    Data = ChipGetField( Instance->IOHandle, F288_PR);
    switch (Data)
    {
    case 0:
        CurFecRate = STTUNER_FEC_1_2;
        break;

    case 1:
        CurFecRate = STTUNER_FEC_2_3;
        break;

    case 2:
        CurFecRate = STTUNER_FEC_3_4;
        break;

    case 3:
        CurFecRate = STTUNER_FEC_5_6;
        break;
        
    case 4:
        
          CurFecRate = STTUNER_FEC_6_7;
          break;
    case 5:
          CurFecRate = STTUNER_FEC_7_8;
          break;
    default:
        CurFecRate = STTUNER_FEC_NONE;;
        break;
    }

    *FECRates = CurFecRate;  /* Copy back for caller */
    
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print(("%s FECRate=%u\n", identity, CurFecRate));
    #endif
    
    /* GNBvd17801-> Error Handling for any error returned during I2C operation inside the driver */
    Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;
    return(Error);
}
/* ----------------------------------------------------------------------------
Name: demod_d0288_GetIQMode()
Function added for GNBvd26107->I2C failure due to direct access to demod device at API level
(Problem:STTUNER_GetTunerInfo was calling STTUNER_SAT_GetTunerInfo, which was reading 
STTUNER_IOCTL_IQMODE directly. This was causing I2C failure due to simultaneous I2C access
to the same device.
Resolution:This functions reads the value for current IQMode from the register which is updated
in the CurrentTunerInfo structure. So now in STTUNER_SAT_GetTunerInfo() (in get.c) no separate
I2C access (due to Ioctl) is required for retrieving the IQ mode )

Description:
     Retrieves the IQMode from the IOCFG register(NORMAL or INVERTED)
   
Parameters:
    IQMode, pointer to area to store the IQMode in use.
  
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0288_GetIQMode(DEMOD_Handle_t Handle, STTUNER_IQMode_t *IQMode)
{
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    const char *identity = "STTUNER d0288.c demod_d0288_GetIQMode()";
    #endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0288_InstanceData_t *Instance;
    /* private driver instance data */
    Instance = D0288_GetInstFromHandle(Handle);
    if(ChipGetField( Instance->IOHandle, F288_SYM)==1)
            *IQMode = STTUNER_IQ_MODE_INVERTED;
    else
            *IQMode = STTUNER_IQ_MODE_NORMAL;
                        
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print(("%s IQMode=%u\n", identity, *IQMode));
    #endif
    return(Error);
}   
/* ----------------------------------------------------------------------------
Name: demod_d0288_IsLocked()

Description:
     Checks the LK register i.e., are we demodulating a digital carrier.
   
Parameters:
    IsLocked,   pointer to area to store result (bool):
                TRUE -- we are locked.
                FALSE -- no lock.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0288_IsLocked(DEMOD_Handle_t Handle, BOOL *IsLocked)
{
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    const char *identity = "STTUNER d0288.c demod_d0288_IsLocked()";
    #endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0288_InstanceData_t *Instance;
    /* private driver instance data */
    Instance = D0288_GetInstFromHandle(Handle);
    *IsLocked = ChipGetField( Instance->IOHandle, F288_LK) ? TRUE : FALSE;
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print(("%s IsLocked=%u\n", identity, *IsLocked));
    #endif
    Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;
  
    return(Error);
}   

/* ----------------------------------------------------------------------------
Name: demod_d0288_SetFECRates()

Description:
    Sets the FEC rates to be used during demodulation.
    Parameters:
    
Parameters:
      FECRates, bitmask of FEC rates to be applied.
  
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0288_SetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t FECRates)
{
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    const char *identity = "STTUNER d0288.c demod_d0288_SetFECRates()";
    #endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U8 Data = 0, uData1;
    D0288_InstanceData_t *Instance;
    /* private driver instance data */
    Instance = D0288_GetInstFromHandle(Handle);
    /*
    ** uData1 - FEC Mode Register
    ** 0 - DVB QPSK, 1 - DVB BPSK, 4 = DSS
    */
    uData1 = ChipGetField( Instance->IOHandle,F288_FECMODE);

    /*
    ** For each STTUNER FEC rates (see sttuner.h)
    ** Set specific bit of enable puncture rate register
    ** Bit 0 - 1/2, 1 - 2/3, 2 - 3/4, 3 - 5/6, 4 - 7/8(DVB) or 6/7(DSS)
    */
    if (FECRates & STTUNER_FEC_1_2) Data |= 1;
    if (FECRates & STTUNER_FEC_2_3) Data |= 2;
    if (FECRates & STTUNER_FEC_3_4) Data |= 4;
    if (FECRates & STTUNER_FEC_5_6) Data |= 8;
    if (FECRates & STTUNER_FEC_6_7) Data |= 16;
    if (FECRates & STTUNER_FEC_7_8) Data |= 32;
    
    Error = ChipSetOneRegister( Instance->IOHandle, R288_PR, /*Data*/0x7f/*setALL*/);

    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print(("%s Field F0288_RATE = 0x%02x\n", identity, Data));
    #endif
    Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;
    
    return(Error);
}   
/* ----------------------------------------------------------------------------
Name: demod_d0288_Tracking()

Description:
    This routine checks the carrier against a certain threshold value and will
    perform derotator centering, if necessary -- using the ForceTracking
    option ensures that derotator centering is always performed when
    this routine is called.

    This routine should be periodically called once a lock has been
    established in order to maintain the lock.
    
Parameters:
    ForceTracking, boolean to control whether to always perform
                   derotator centering, regardless of the carrier.

    NewFrequency,  pointer to area where to store the new frequency
                   value -- it may be changed when trying to optimize
                   the derotator.

    SignalFound,   indicates that whether or not we're still locked
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0288_Tracking(DEMOD_Handle_t Handle, BOOL ForceTracking,    U32  *NewFrequency,  BOOL *SignalFound)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
   const char *identity = "STTUNER d0288.c demod_d0288_Tracking()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    STTUNER_tuner_instance_t *TunerInstance;    /* don't really need all these variables, done for clarity */
    STTUNER_InstanceDbase_t  *Inst;             /* pointer to instance database */
    STTUNER_Handle_t          TopHandle;        /* instance that contains the demos, tuner and lnb driver set */
    D0288_InstanceData_t     *Instance;
    TUNER_Status_t  TunerStatus; 
    U32  MasterClock, Mclk;
    FE_288_SignalInfo_t	pInfo;
    /* private driver instance data */
    Instance = D0288_GetInstFromHandle(Handle);
    /* top level public instance data */ 
    Inst = STTUNER_GetDrvInst();

    /* this driver knows what to level instance it belongs to */
    TopHandle = Instance->TopLevelHandle;

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[TopHandle].Sat.Tuner;

    /*Drv0288_CarrierTracking( Instance->IOHandle, &Instance->StateBlock, TopHandle);*/

    Error  =  demod_d0288_IsLocked(Handle, SignalFound);
    /**NewFrequency = (U32)Instance->StateBlock.Result.Frequency;*/
    if((Error == ST_NO_ERROR) && (*SignalFound == TRUE))
    {
	    Error = (TunerInstance->Driver->tuner_GetStatus)(TunerInstance->DrvHandle, &TunerStatus);
	   
	    MasterClock = FE_288_GetMclkFreq( Instance->IOHandle,Instance->ExternalClock);
	   
	    Mclk = (U32)(MasterClock/65536L); 
	    Inst[TopHandle].CurrentTunerInfo.ScanInfo.SymbolRate = FE_288_GetSymbolRate( Instance->IOHandle, MasterClock);
	    FE_288_GetSignalInfo( Instance->IOHandle, &pInfo);
	    *NewFrequency = TunerInstance->realfrequency;
	    Inst[TopHandle].CurrentTunerInfo.BitErrorRate = pInfo.BER;
	    Inst[TopHandle].CurrentTunerInfo.SignalQuality = pInfo.CN_dBx10;
	    Inst[TopHandle].CurrentTunerInfo.ScanInfo.AGC = pInfo.Power_dBm;
	    Inst[TopHandle].CurrentTunerInfo.ScanInfo.FECRates = pInfo.Rate;
	    Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.IQMode=pInfo.SpectralInv;
	    Inst[TopHandle].CurrentTunerInfo.ScanInfo.Modulation = STTUNER_MOD_QPSK;
	    if(Instance->SET_TSCLOCK == TRUE)
	    {
	    	Instance->SET_TSCLOCK = FALSE;
            }
    }
   
     #ifdef STTUNER_DRV_SAT_SCR
        if((Inst[Instance->TopLevelHandle].Capability.SCREnable) && (Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.LNB_SignalRouting == STTUNER_VIA_SCRENABLED_LNB))
        {
        	*NewFrequency = Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->ScrParams.SCRVCOFrequency - TunerInstance->realfrequency;
        }
       
     #endif
    
    if(Inst[TopHandle].Sat.Ioctl.SignalNoise == TRUE)
#ifndef ST_OSLINUX
    {
        {
            static BOOL SRand = TRUE;
            U32 r;

            if (SRand)
            {
                srand((unsigned int)STOS_time_now());
                SRand = FALSE;
            }

            /* Calculate percentage fluctuation from current position */
            r = rand() % 100;               /* Percent change */

            Inst[TopHandle].CurrentTunerInfo.SignalQuality -= (
                (((Inst[TopHandle].CurrentTunerInfo.SignalQuality << 7) * r) / 100) >> 7 );
	   /* Estimate ber from signal quality based on a linear relationship */
            Inst[TopHandle].CurrentTunerInfo.BitErrorRate = ( MAX_BER - (
                (Inst[TopHandle].CurrentTunerInfo.SignalQuality * MAX_BER) / 100 ));
        }
    }
      
#endif
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print(("%s SignalFound=%u NewFrequency=%u\n", identity, *SignalFound, *NewFrequency));
#endif
    
    /* GNBvd17801-> Error Handling for any error returned during I2C operation inside the driver */
    Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;
    return(Error);
}   
/* ----------------------------------------------------------------------------
Name: demod_d0288_ScanFrequency()

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
ST_ErrorCode_t demod_d0288_ScanFrequency(DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset, 
                                                                U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                U32  FreqOff,          U32   ChannelBW,     S32   EchoPos)
 
{
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    const char *identity = "STTUNER d0288.c demod_d0288_ScanFrequency()";
    #endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FE_288_SearchParams_t Params;
    FE_288_SearchResult_t Result;
    STTUNER_InstanceDbase_t  *Inst;
    #ifdef STTUNER_DRV_SAT_SCR
    	U32 SCRVCOFrequency, TunerFrequency;
    #endif
 
    D0288_InstanceData_t *Instance;
    STTUNER_tuner_instance_t *TunerInstance; 
    /* private driver instance data */
    Instance = D0288_GetInstFromHandle(Handle);
    /* top level public instance data */ 
    Inst = STTUNER_GetDrvInst();
    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[Instance->TopLevelHandle].Sat.Tuner;
    Params.ExternalClock = Instance->ExternalClock;
    Params.Frequency   = InitialFrequency;              /* demodulator (output from LNB) frequency (in KHz) */
    Params.Symbolrate  = SymbolRate;                    /* transponder symbol rate  (in bds) */
    if (Inst[Instance->TopLevelHandle].Sat.BlindScan)
    Params.SearchRange = 4000000;
    else if(Inst[Instance->TopLevelHandle].Sat.ScanExact)
    Params.SearchRange = 10000000;
    else
    Params.SearchRange = MaxOffset*2;
   
    Params.Modulation  = FE_288_MOD_QPSK;
    
    Params.DemodIQMode=(STTUNER_IQMode_t) Spectrum;
  
    
    /* Set SCR frequency here -> Now SCR Index starts from "0" v0.3 ST7 firmware*/
    #ifdef STTUNER_DRV_SAT_SCR

    if((Inst[Instance->TopLevelHandle].Capability.SCREnable) && (Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.LNB_SignalRouting == STTUNER_VIA_SCRENABLED_LNB))
    {
	    if(!Inst[Instance->TopLevelHandle].Sat.BlindScan)
	    {
	    	Error = scr_scrdrv_SetFrequency(Instance->TopLevelHandle, Handle, Instance->DeviceName, InitialFrequency/1000, Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->ScrParams.LNBIndex,Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->ScrParams.SCRBPF );
	    	TunerFrequency = (Inst[Instance->TopLevelHandle].Capability.SCRBPFFrequencies[Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->ScrParams.SCRBPF])*1000;
	    	Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->ScrParams.BPFFrequency = TunerFrequency;
	    	SCRVCOFrequency = TunerFrequency + InitialFrequency;
	        Params.Frequency = TunerFrequency;
	        Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->ScrParams.SCRVCOFrequency = SCRVCOFrequency;
	    }
	    else
	    {
	    	Error = scr_scrdrv_SetFrequency(Instance->TopLevelHandle, Handle, Instance->DeviceName, InitialFrequency/1000, Inst[Instance->TopLevelHandle].Sat.BlindScanInfo.ScrParams.LNBIndex,Inst[Instance->TopLevelHandle].Sat.BlindScanInfo.ScrParams.SCRBPF );
	    	TunerFrequency = (Inst[Instance->TopLevelHandle].Capability.SCRBPFFrequencies[Inst[Instance->TopLevelHandle].Sat.BlindScanInfo.ScrParams.SCRBPF])*1000;
	    	Inst[Instance->TopLevelHandle].Sat.BlindScanInfo.ScrParams.BPFFrequency = TunerFrequency;
	    	SCRVCOFrequency = TunerFrequency + InitialFrequency;
	        Params.Frequency = TunerFrequency;
	        Inst[Instance->TopLevelHandle].Sat.BlindScanInfo.ScrParams.SCRVCOFrequency = SCRVCOFrequency;
	    }   
    }
    #endif
    
    Error |= FE_288_Search( Instance->IOHandle, &Params, &Result, Instance->TopLevelHandle );

    if (Error == FE_288_BAD_PARAMETER)  /* element(s) of Params bad */
    {
        #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s fail, scan not done: bad parameter(s) to FE_399_Search() == FE_899_BAD_PARAMETER\n", identity ));
		#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    else if (Error == FE_288_SEARCH_FAILED)  /* found no signal within limits */
    {
	#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s FE_399_Search() == FE_288_SEARCH_FAILED\n", identity ));
	#endif
        return(ST_NO_ERROR);    /* no error so try next F or stop if band limits reached */
    }
    *ScanSuccess = Result.Locked;

    if (Result.Locked == TRUE)
    {
        
        Instance->Symbolrate = Result.Symbolrate;
        Instance->SET_TSCLOCK = TRUE;
        #ifdef STTUNER_DRV_SAT_SCR
        if((Inst[Instance->TopLevelHandle].Capability.SCREnable)&& (Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.LNB_SignalRouting == STTUNER_VIA_SCRENABLED_LNB))
        {
        	*NewFrequency = (SCRVCOFrequency - TunerFrequency) + (TunerFrequency - Result.Frequency);
        }
        else
        {
        	*NewFrequency = Result.Frequency;
        }
        #else
        *NewFrequency = Result.Frequency;
        #endif
      }

    /* GNBvd17801-> Error Handling for any error returned during I2C operation inside the driver */
    Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;

   return(Error); 
}   

ST_ErrorCode_t demod_0288_GetSymbolrate(DEMOD_Handle_t Handle,   U32  *SymbolRate)
{
	ST_ErrorCode_t Error = ST_NO_ERROR;
	D0288_InstanceData_t *Instance;
	Instance = D0288_GetInstFromHandle(Handle);
	*SymbolRate = Instance->Symbolrate;
	return Error;
}


/* ----------------------------------------------------------------------------
Name: demod_d0288_DiseqcInit
Description:
    Selects the DiSEqC F22RX for maximum hits. THis function should be called when DiSEqC 2.0 Rx 
    Envelop mode is not used.
  Parameters:
    Handle,					Demod Handle
  Return Value:
    ST_NO_ERROR,                the operation was carried out without error.
  ---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0288_DiseqcInit(DEMOD_Handle_t Handle)
{
	ST_ErrorCode_t Error = ST_NO_ERROR;      
	U8 f22Tx, f22Rx, sendData[3]={0xE2,0x00,0x00},/*diseqC command to send */
		         ReceveData[8]={0};
	STTUNER_DiSEqCSendPacket_t pDiSEqCSendPacket;
	STTUNER_DiSEqCResponsePacket_t pDiSEqCResponsePacket;
	D0288_InstanceData_t *Instance;
	U32 mclk, txFreq=22000, RxFreq,
     	    recevedok=0,
    	    i,
    	    trial = 0,	/*try number maximum = 2 (try 20khz and 17.5 khz)*/
    	    sucess1=0,  /*20 Khz sucess */
    	    sucess2=0;  /*17.5 Khz sucess */
    	    
        Instance = D0288_GetInstFromHandle(Handle);
        pDiSEqCSendPacket.DiSEqCCommandType = STTUNER_DiSEqC_COMMAND;
        pDiSEqCSendPacket.pFrmAddCmdData = &sendData[0];
	pDiSEqCSendPacket.uc_TotalNoOfBytes = 3;
	pDiSEqCResponsePacket.ReceivedFrmAddData = ReceveData;
	
	Error = ChipSetField( Instance->IOHandle, F288_ONECHIP_TRX,0); /*Disable Tx spy off */
	Error |= ChipSetField( Instance->IOHandle, F288_DIS_RESET,1);
	Error |= ChipSetField( Instance->IOHandle, F288_DIS_RESET,0);
	
	mclk = FE_288_GetMclkFreq( Instance->IOHandle,Instance->ExternalClock); /* For 5188 */
	
	f22Tx= mclk /(txFreq*32);
	Error |= ChipSetOneRegister( Instance->IOHandle, F288_FFR_REG, f22Tx);/*Set DiseqC Tx freq */
	
	RxFreq=20000;
	f22Rx= mclk /((RxFreq)*32);
	if(Error == ST_NO_ERROR)	
	{
		
		while ((recevedok <5) && (trial <2))
		{
			Instance->DiSEqC_RxFreq = RxFreq;
			ChipSetOneRegister( Instance->IOHandle, F288_FRX_REG, f22Rx);	/*Set Rx freq 2 possible value 20khz or 17.5 khz*/
			
			for(i=0;i<5;i++)
			{
				pDiSEqCResponsePacket.uc_TotalBytesReceived = 0;
				Error = demod_d0288_DiSEqC(Handle, &pDiSEqCSendPacket, &pDiSEqCResponsePacket);
				if(Error != ST_NO_ERROR)
				{
					return Error;
				}
				if(pDiSEqCResponsePacket.uc_TotalBytesReceived >=1)
				{
					if((pDiSEqCResponsePacket.ReceivedFrmAddData[pDiSEqCResponsePacket.uc_TotalBytesReceived-1]==0xe4)||(pDiSEqCResponsePacket.ReceivedFrmAddData[pDiSEqCResponsePacket.uc_TotalBytesReceived-1]==0xe5))
					recevedok++;
				}
				
			}	
			
			if(trial==0)
				sucess1=recevedok;
			else
				sucess2=recevedok;
					
			trial++;
			RxFreq=17500;
			f22Rx= mclk /((RxFreq)*32);
		}
		if(sucess1>=sucess2)
		{
			RxFreq=20000;
			f22Rx= mclk /((RxFreq)*32);
		}
		else
		{
			RxFreq=17500;
			f22Rx= mclk /((RxFreq)*32);
		}
		
		Error = ChipSetOneRegister( Instance->IOHandle, F288_FRX_REG, f22Rx);
        }
	if((sucess1==0)&&(sucess2==0))
	RxFreq=0; /*No diseqC 2.x slave detected */

	Instance->DiSEqC_RxFreq = RxFreq;
	if(RxFreq != 0)
	{
		#ifndef NO_DISEQC_RX_ENVELOPMODE /* In Envelop Receiving Mode, F22Rx = 22000 Hz */
		Instance->DiSEqC_RxFreq = 22000;
		#endif
	}
	return Error;
}


/* ----------------------------------------------------------------------------
Name: demod_d0288_DiSEqC
Description:
    Sends the DiSEqc command through F22

  Parameters:
    Handle,					handle to sttuner device
    pDiSEqCSendPacket		pointer to the command packet to be sent
	pDiSEqCResponsePacket	pointer to the  packet to be received; void for diseqc 1.2

  Return Value:
    ST_NO_ERROR,                the operation was carried out without error.
    ST_ERROR_BAD_PARAMETER      Handle NULL or incorrect tone setting.

  Calling function  

  See Also:
    Nothing.
---------------------------------------------------------------------------- */

ST_ErrorCode_t demod_d0288_DiSEqC       (	DEMOD_Handle_t Handle, 
									    STTUNER_DiSEqCSendPacket_t *pDiSEqCSendPacket,
									    STTUNER_DiSEqCResponsePacket_t *pDiSEqCResponsePacket
									   )
{	
	int Error=ST_NO_ERROR;
	unsigned char No_of_Bytes, *pCurrent_Data;
	U32 Delay=0;
	U32 DigFreq, F22_Freq;
	U8 F22, i=0;
	U16  TimeoutCounter =0 ;/*for future implementation*/
	D0288_InstanceData_t *Instance;
	STTUNER_InstanceDbase_t *Inst;	
	/* If framing byte wants response (DiSEqC 2.0) report error*/
	pCurrent_Data = pDiSEqCSendPacket->pFrmAddCmdData;
	Instance = D0288_GetInstFromHandle(Handle);	
	Inst = STTUNER_GetDrvInst();
	/*In loopthrough mode send DiSEqC-ST commands through first demod */
	#ifdef STTUNER_DRV_SAT_SCR
		#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
		    if(Instance->DISECQ_ST_ENABLE == FALSE)
		    {
			    Handle = DemodDrvHandleOne;
			    Instance = D0288_InstanceData_t(Handle);
		    }
		#endif
	#endif
	/* Set 22KHz before transmission */
	DigFreq = FE_288_GetMclkFreq( Instance->IOHandle,Instance->ExternalClock); /* For 5188 */
	F22 = ChipGetField( Instance->IOHandle, F288_FFR_REG);
	F22_Freq = FE_288_F22Freq(DigFreq,F22);
	if(F22_Freq<= 21800 || F22_Freq>=22200)
	{
		F22 = DigFreq/(32*22000);
		ChipSetField( Instance->IOHandle, F288_FFR_REG, F22);
		
	}

	/*****************************************/
	/*Take care for receiving freq as it may vary w.r.t master clock frequency -> add Rx_Freq check here*/
	/*****************************************/

	if(Instance->DiSEqC_RxFreq !=0 )
	{
		ChipSetField( Instance->IOHandle, F288_ONECHIP_TRX,0); 
		F22 = ChipGetField( Instance->IOHandle, F288_FRX_REG);
		F22_Freq = FE_288_F22Freq(DigFreq,F22);
		if((F22_Freq <= (Instance->DiSEqC_RxFreq - 500)) || (F22_Freq >= (Instance->DiSEqC_RxFreq + 500)))
		{
			F22 = DigFreq/(32*(Instance->DiSEqC_RxFreq));
			ChipSetField( Instance->IOHandle, F288_FRX_REG, F22);
			WAIT_N_MS_288(2);
		}		
	}

	Error = ChipSetField( Instance->IOHandle, F288_DIS_PRECHARGE, 0);
	/*wait for  FIFO empty to ensure previous execution is complete*/
	ChipSetField( Instance->IOHandle, F288_DIS_RESET, 1);
	ChipSetField( Instance->IOHandle, F288_DIS_RESET, 0);
	
	while((ChipGetField( Instance->IOHandle, F288_READ_WRITE_COUNTER) != 0))				  			
		{
				STX0288_Delay(50);
				if ((++TimeoutCounter)>0xfff)
						{ return(ST_ERROR_TIMEOUT);
						}
	     
				TimeoutCounter =0;
		}
	
	/* Reset previos FIFO contents */
		

	switch (pDiSEqCSendPacket->DiSEqCCommandType)		
	{				
	case	STTUNER_DiSEqC_TONE_BURST_SEND_0_UNMODULATED:   /*send of 0 for 12.5 ms ;continuous tone*/
				
			
			Error = ChipSetField( Instance->IOHandle, F288_DISEQC_MODE, 0x3);
			/* Put Tx in WAITING State */
			Error = ChipSetField( Instance->IOHandle, F288_DIS_PRECHARGE, 1);
			Error = ChipSetOneRegister( Instance->IOHandle, R288_DISEQCFIFO, 00);
			/* Start transmission */
			Error = ChipSetField( Instance->IOHandle, F288_DIS_PRECHARGE, 0);
			WAIT_N_MS_288(14);/*13.5 mSec to Tx one byte */
			Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_SEND_0_UNMODULATED;
        	Instance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
									
			break;

		case	STTUNER_DiSEqC_TONE_BURST_SEND_0_MODULATED:     /*0-2/3 duty cycle tone*/
				
			/* DiSEqC(1:0)=10  DiSEqC FIFO=00  */
			Error = ChipSetField( Instance->IOHandle, F288_DISEQC_MODE, 0x2);
			/* Put Tx in WAITING State */
			Error = ChipSetField( Instance->IOHandle, F288_DIS_PRECHARGE, 1);
			Error = ChipSetOneRegister( Instance->IOHandle, R288_DISEQCFIFO, 00);
			/* Start transmission */
			Error = ChipSetField( Instance->IOHandle, F288_DIS_PRECHARGE, 0);
			WAIT_N_MS_288(14);/*13.5 mSec to Tx one byte */
			Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_SEND_0_MODULATED;
        		Instance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
			break;

		case	STTUNER_DiSEqC_TONE_BURST_SEND_1: 				/*1-1/3 duty cycle tone modulated*/
				
			
			Error = ChipSetField( Instance->IOHandle, F288_DISEQC_MODE, 0x2);
			/* Put Tx in WAITING State */
			Error = ChipSetField( Instance->IOHandle, F288_DIS_PRECHARGE, 1);
			Error = ChipSetOneRegister( Instance->IOHandle, R288_DISEQCFIFO, 0xFF);
			/* Start transmission */
			Error = ChipSetField( Instance->IOHandle, F288_DIS_PRECHARGE, 0);
			 /* Wait for the DiSEqC command to be sent completely --(TicksPermilisec *13.5* Numberofbytes sent)*/
			WAIT_N_MS_288(14);/*13.5 mSec to Tx one byte */
			Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_SEND_1;
        	Instance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
        	
			break;

		case	STTUNER_DiSEqC_COMMAND:							/*DiSEqC (1.2/2)command */
			
			ChipSetField( Instance->IOHandle, F288_RECEIVER_ON, 0);
			Error = ChipSetField( Instance->IOHandle, F288_DISEQC_MODE, 2);
			/* Put Tx in WAITING State */
			Error = ChipSetField( Instance->IOHandle, F288_DIS_PRECHARGE, 1);
			
			for(No_of_Bytes = pDiSEqCSendPacket->uc_TotalNoOfBytes; No_of_Bytes>0; No_of_Bytes--)
			{
				Error = ChipSetOneRegister( Instance->IOHandle, R288_DISEQCFIFO, *pCurrent_Data);
				
				pCurrent_Data++;
			
			}
			Error = ChipSetField( Instance->IOHandle, F288_DIS_PRECHARGE, 0);
			Instance->DiSEqCConfig.Command = STTUNER_DiSEqC_COMMAND;
        	Instance->DiSEqCConfig.ToneState = STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
	        /* Make Delay value to 135 */
	        Delay = 140;
	        /* Wait for the DiSEqC command to be sent completely --(TicksPermilisec *13.5* Numberofbytes sent)*/
			WAIT_N_MS_288((Delay*pDiSEqCSendPacket->uc_TotalNoOfBytes)/10);
			
			break;
		
		default:
				
	        break;
	
		}/*end of Switch*/														
	
	if (Error !=ST_NO_ERROR)
		{	
			return(Error);
		}
	
	if(Instance->DiSEqC_RxFreq !=0 )
	{
		
		ChipSetField( Instance->IOHandle, F288_PIN_SELECT, 0);
		Error = ChipSetField( Instance->IOHandle, F288_RECEIVER_ON, 1);
		Error |= ChipSetField( Instance->IOHandle, F288_ONECHIP_TRX,1); 
		if (Error != ST_NO_ERROR)
		{
			return(Error);		
		}
		while((ChipGetField( Instance->IOHandle,F288_RX_END)!=1) && (i<10))
		{
			WAIT_N_MS_288(10); 
			i++;
		}
			
		if(ChipGetField( Instance->IOHandle, F288_RX_END))
		{
			pDiSEqCResponsePacket->uc_TotalBytesReceived = ChipGetField( Instance->IOHandle, F288_FIFO_BYTENBR);
			for(i=0;i<(pDiSEqCResponsePacket->uc_TotalBytesReceived);i++)
			pDiSEqCResponsePacket->ReceivedFrmAddData[i]=ChipGetOneRegister( Instance->IOHandle, R288_DISEQCFIFO);
			ChipSetField( Instance->IOHandle, F288_DIS_RESET, 1);
			ChipSetField( Instance->IOHandle, F288_DIS_RESET, 0);
		}	
		
	}		
	
	return(Error);
}


/* ----------------------------------------------------------------------------
Name: demod_d0288_DiSEqCGetConfig()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0288_DiSEqCGetConfig(DEMOD_Handle_t Handle,STTUNER_DiSEqCConfig_t *DiSEqCConfig)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;

    D0288_InstanceData_t *Instance;

    Instance =D0288_GetInstFromHandle(Handle);

    DiSEqCConfig->Command=Instance->DiSEqCConfig.Command;
    DiSEqCConfig->ToneState=Instance->DiSEqCConfig.ToneState;

    return(Error);
}  

/* ----------------------------------------------------------------------------
Name: demod_d0288_DiSEqCBurstOFF()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0288_DiSEqCBurstOFF ( DEMOD_Handle_t Handle )
{
   ST_ErrorCode_t Error = ST_NO_ERROR;
  /* D0288_InstanceData_t *Instance;*/
   STTUNER_DiSEqCSendPacket_t DiSEqCSendPacket;
   void *pDiSEqCResponsePacket=NULL;
   
    DiSEqCSendPacket.DiSEqCCommandType = STTUNER_DiSEqC_TONE_BURST_OFF;

   DiSEqCSendPacket.uc_msecBeforeNextCommand =0;
   Error=demod_d0288_DiSEqC(Handle,&DiSEqCSendPacket,pDiSEqCResponsePacket);

   
   return(Error);
}

/* ----------------------------------------------------------------------------
Name: demod_d0288_ioctl()

Description:
    access device specific low-level functions
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0288_ioctl(DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
   const char *identity = "STTUNER d0288.c demod_d0288_ioctl()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0288_InstanceData_t *Instance;
    STTUNER_InstanceDbase_t  *Inst;             /* pointer to instance database */
    U8 Buffer[30];
    U32 Size;
    U8  SubAddress;
    

    /* private driver instance data */
    Instance = D0288_GetInstFromHandle(Handle);

    /* top level public instance data */ 
    Inst = STTUNER_GetDrvInst();

    
    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s demod driver instance handle=%d\n", identity, Handle));
        STTBX_Print(("%s                     function=%d\n", identity, Function));
        STTBX_Print(("%s                   I/O handle=%d\n", identity, Instance->IOHandle));
#endif
       SubAddress = (U8)(((SATIOCTL_IOARCH_Params_t *)InParams)->SubAddr & 0xFF);
    /* ---------- select what to do ---------- */
    switch(Function)
    {

        case STTUNER_IOCTL_RAWIO: /* read/write device register (actual write to stv0288) */                                             
               switch(((SATIOCTL_IOARCH_Params_t *)InParams)->Operation)
  	             {
  	    	      case STTUNER_IO_SA_READ:
                  Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev),&SubAddress, 1, ((SATIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);  
					Error |= STI2C_Read(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev),((SATIOCTL_IOARCH_Params_t *)InParams)->Data,((SATIOCTL_IOARCH_Params_t *)InParams)->TransferSize, ((SATIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
                  break;
                
                case STTUNER_IO_SA_WRITE:
                	 Buffer[0] = SubAddress;
			        memcpy( (Buffer + 1), ((SATIOCTL_IOARCH_Params_t *)InParams)->Data, ((SATIOCTL_IOARCH_Params_t *)InParams)->TransferSize);
			        Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Buffer, (((SATIOCTL_IOARCH_Params_t *)InParams)->TransferSize+1), ((SATIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
		            break;
	    
                default:
                	break;
                }
         break;

        case STTUNER_IOCTL_REG_IN: /* read device register */
            *(int *)OutParams = ChipGetOneRegister( Instance->IOHandle, *(int *)InParams);
            break;

        case STTUNER_IOCTL_REG_OUT: /* write device register. NOTE: RegIndex is <<INDEX>> TO register number. */
            Error =  ChipSetOneRegister( 
                                                  Instance->IOHandle,
                  ((SATIOCTL_SETREG_InParams_t *)InParams)->RegIndex,
                  ((SATIOCTL_SETREG_InParams_t *)InParams)->Value );
            break;
        case STTUNER_IOCTL_IQMODE: /* Get the IQ mode */

            if(ChipGetField( Instance->IOHandle ,F288_SYM)==1)
                *((STTUNER_IQMode_t *)OutParams)=STTUNER_IQ_MODE_INVERTED;
            else
                *((STTUNER_IQMode_t *)OutParams)=STTUNER_IQ_MODE_NORMAL;
            break;
        case STTUNER_IOCTL_SEARCH_TEST: /* For testing the search termination */

            /* Call the function to turn on the infinite loop in the search algorithm */
            /* get the tuner instance for this driver from the top level handle */
            /*Drv0288_TurnOnInfiniteLoop(Instance->TopLevelHandle,(BOOL *)InParams);*/
            
        default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
    STTBX_Print(("%s called ok\n", identity, Function));    /* signal that function came back */
#endif
    
    /* GNBvd17801-> Error Handling for any error returned during I2C operation inside the driver */
    Error |= IOARCH_Handle[Instance->IOHandle].DeviceMap.Error;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Error = ST_NO_ERROR;
    
    return(Error);

}





/* ----------------------------------------------------------------------------
Name: demod_d0288_repeateraccess()

Description:
    called from ioarch.c when some driver does I/O but with the repeater/passthru
    set to point to this function.
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t demod_d0288_repeateraccess(DEMOD_Handle_t Handle, BOOL REPEATER_STATUS )
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
   const char *identity = "STTUNER d0288.c demod_d0288_repeateraccess()";
#endif

    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0288_InstanceData_t *Instance;
    Instance = D0288_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }


if(REPEATER_STATUS)
{
     Error = ChipSetField(Instance->IOHandle,F288_I2CT_ON, 1);
}
 
  return(Error);
}



/* ----------------------------------------------------------------------------
Name: demod_d0288_StandByMode()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */



ST_ErrorCode_t demod_d0288_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode)
{
	
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
   const char *identity = "STTUNER d0288.c demod_d0288_StandByMode()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0288_InstanceData_t *Instance;
   
    /* private driver instance data */
    Instance = D0288_GetInstFromHandle(Handle);
   
      
    switch ( PowerMode)
    {
       case STTUNER_NORMAL_POWER_MODE :
       if(Instance->StandBy_Flag == 1)
       {
          Error|=ChipSetField( Instance->IOHandle, F288_STANDBY, 0);
       }
          
             
       if(Error==ST_NO_ERROR)
       {
          Instance->StandBy_Flag = 0 ;
       }
       
       break;
       case STTUNER_STANDBY_POWER_MODE :
       if(Instance->StandBy_Flag == 0)
       {
          Error|=ChipSetField( Instance->IOHandle, F288_STANDBY, 1);
          if(Error==ST_NO_ERROR)
          {
              
             Instance->StandBy_Flag = 1 ;
             
          }
       }
       break ;
	   /* Switch statement */ 
  
    }
       
 

    /*Resolve Bug GNBvd17801 - For proper I2C error handling inside the driver*/
   
    return(Error);
}




/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */

D0288_InstanceData_t *D0288_GetInstFromHandle(DEMOD_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288_HANDLES
   const char *identity = "STTUNER d0288.c D0288_GetInstFromHandle()";
#endif
    D0288_InstanceData_t *Instance;

    Instance = (D0288_InstanceData_t *)Handle;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288_HANDLES
    STTBX_Print(("%s block at 0x%08x\n", identity, Instance));
#endif

    return(Instance);
}

#ifdef STTUNER_DRV_SAT_SCR
/* For tone detection */
ST_ErrorCode_t demod_d0288_tonedetection(DEMOD_Handle_t Handle,U32 StartFreq, U32 StopFreq, U8  *NbTones,U32 *ToneList, U8 mode,int* power_detection_level)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0288_InstanceData_t *Instance;
    
    Instance = D0288_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0288
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }
    
       
    *NbTones = Drv0288_ToneDetection(Instance->TopLevelHandle,StartFreq,StopFreq,ToneList,mode, power_detection_level);


    return(Error);
    
}
#endif



/* End of d0288.c */


