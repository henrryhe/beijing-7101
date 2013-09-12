/* ----------------------------------------------------------------------------
File Name: d0299.c

Description: 

    stv0299 demod driver.


Copyright (C) 1999-2001 STMicroelectronics

History:
 
   date: 19-June-2001
version: 3.1.0
 author: GJP from work by LW
comment: write for multi-instance.
    
   date: 17-August-2001
version: 3.1.1
 author: GJP
comment: update SubAddr to U16

   date: 23-Oct-2001
version: 3.2.0
 author: GJP
comment: corrected chip ID bug (A0 instead of F0 mask)
         added SetModulation() function

Reference:

    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
   #include <string.h>
#endif
#include "stlite.h"     /* Standard includes */
#include "sttbx.h"
#include "sttuner.h"                    

/* local to sttuner */
#ifndef STTUNER_MINIDRIVER
#include "stevt.h"
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "ioarch.h"     /* I/O for this driver */
#include "ioreg.h"      /* I/O for this driver */
#include "sioctl.h"     /* data structure typedefs for all the the sat ioctl functions */
#endif
#include "reg0299.h"    /* register mappings for the stv0299 */
#include "drv0299.h"    /* misc driver functions */
#include "d0299.h"      /* header for this file */
#ifdef STTUNER_MINIDRIVER
#include "sysdbase.h"
#include "stcommon.h" 
#endif


#if defined (ST_OS21) || defined (ST_OSLINUX)
#define STTUNER_TaskDelay(x) STOS_TaskDelay((signed int)x)
#else
#define STTUNER_TaskDelay(x) STOS_TaskDelay((unsigned int)x)
#endif

/* Global variable to keep track of current running task */
extern task_t *TunerTask1, *TunerTask2; 

/* Global variable which sets to 2 when both task are running and helps
 to differentiate when single tuner is running and when dual tuner is running */
extern U8 ucDualTunerFlag;

#ifdef STTUNER_MINIDRIVER
long D0299_CarrierWidth(long SymbolRate);
#endif
/* Private types/constants ------------------------------------------------ */

#define ANALOG_CARRIER_DETECT_SYMBOL_RATE   5000000
#define ANALOG_CARRIER_DETECT_AGC2_VALUE    25
/* Device capabilities */
#define MAX_AGC                         255
#define MAX_SIGNAL_QUALITY              100
#define MAX_BER                         200000

#ifdef STTUNER_DRV_SAT_SCR
extern ST_ErrorCode_t scr_scrdrv_SetFrequency   (STTUNER_Handle_t Handle, DEMOD_Handle_t DemodHandle, ST_DeviceName_t *DeviceName, U32  InitialFrequency,  U8 LNBIndex, U8 SCRBPF );
#endif

#define STV0299_SYMBOL_RATE_MIN 1000000
#define STV0299_SYMBOL_RATE_MAX 50000000

/* Added by for Debug 19 July 2k2 */
#ifdef USE_TRACE_BUFFER_FOR_DEBUG
#define DMP
extern void DUMP_DATA(char *info) ;
extern char Trace_PrintBuffer[40]; /* global variable to be seen by modules */
#endif
/* private variables ------------------------------------------------------- */

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */


#ifndef STTUNER_MINIDRIVER
static BOOL        Installed = false;

U16 STV0299_Address[STV0299_NBREGS]={
 0x00,  0x01,   0x02,   0x03,    0x04,    0x05,     0x06,    0x07,    0x08,
 0x09,  0x0A,   0x0B,   0x0C,    0x0D,    0x0E,     0x0F,    0x10,    0x11,
 0x12,  0x13,   0x14,   0x15,    0x16,    0x17,     0x18,    0x19,    0x1A,
 0x1B,  0x1C,   0x1D,   0x1E,    0x1F,    0x20,     0x21,    0x22,    0x23,
 0x24,  0x25,   0x26,   0x28,    0x29,    0x2A,     0x2B,    0x2C,    0x2D,
 0x31,  0x32,   0x33,   0x34,    0x40,    0x41,     0x42,    0x43,    0x44,
 0x45,  0x46,   0x47,   0x48,    0x49,    0x4A,     0x4B,    0x4C,    0x4D,
 0x4E,  0x4F
};

 U8 STV0299_DefVal[STV0299_NBREGS]={
 0xA1,  0x15,  	0x00,   0x00,   0x7D,	  0x05,     0x02,    0x00,     0x40,
 0x00,  0x82,   0x00,   0x00,   0x81,     0x23,     0x15,    0x34,     0x84/*0x99*/,
 0xb9,  0x9b,   0x9e,   0xe3,   0x80,     0x18,     0xff,    0xff,     0x82,
 0x00,  0x7f,   0x00,   0x00,   0x00,     0x00,     0x00,    0x80,     0x0b,
 0x2b,  0x75,   0x1a,   0x00,   0x1E,     0x14,     0x0F,    0x09,     0x05,
 0x1F,  0x19,   0xFc,   0x13,   0x00,     0x00,	    0x00,    0x00,     0x00,
 0x00,  0x00,   0x00,   0x00,   0x00,     0x00,     0x00,    0x00,     0x00,
 0x00,  0x00
};
/* ---------- per instance of driver ---------- */
typedef struct
{
    ST_DeviceName_t            	*DeviceName;           /* unique name for opening under */
    STTUNER_Handle_t          	TopLevelHandle;       /* access tuner, lnb etc. using this */
    IOARCH_Handle_t          	IOHandle;             /* instance access to I/O driver     */
    STTUNER_IOREG_DeviceMap_t 	DeviceMap;            /* stv0299 register map & data table */
    D0299_StateBlock_t          StateBlock;           /* driver search/state information   */
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
    U32                         StandBy_Flag; /*** To take care same Standby Mode doesnot
                                                   execute more than once ***/
}D0299_InstanceData_t;
#endif
#ifdef STTUNER_MINIDRIVER
typedef struct
{
    IOARCH_Handle_t          	IOHandle;             /* instance access to I/O driver     */
    STTUNER_Handle_t          	TopLevelHandle;
    ST_DeviceName_t            	*DeviceName; 
    ST_Partition_t            	*MemoryPartition;     /* which partition this data block belongs to */
    U32                         ExternalClock;  /* External VCO */
    STTUNER_TSOutputMode_t      TSOutputMode;
    STTUNER_SerialDataMode_t    SerialDataMode;
    STTUNER_BlockSyncMode_t     BlockSyncMode;
    STTUNER_SerialClockSource_t SerialClockSource;
    STTUNER_FECMode_t           FECMode;
    STTUNER_DiSEqCConfig_t      DiSEqCConfig; /** Added DiSEqC configuration structure to get the trace of current state**/
    BOOL                        DISECQ_ST_ENABLE;
   
 }D0299_InstanceData_t;
#endif
#ifndef STTUNER_MINIDRIVER
/******Externing from Init.c********/
extern U32 TunerHandleOne;
extern U32 TunerHandleTwo;
#endif

/**************extern from  open.c************************/
#ifdef STTUNER_DRV_SAT_SCR
#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
extern U32 DemodDrvHandleOne;
#endif
#endif
/* instance chain, the default boot value is invalid, to catch errors */
#ifndef STTUNER_MINIDRIVER
static D0299_InstanceData_t *InstanceChainTop = (D0299_InstanceData_t *)0x7fffffff;
#endif
#ifdef STTUNER_MINIDRIVER
static D0299_InstanceData_t *DEMODInstance;
#endif
	   /* NOW FUNCTION DECLARATION is in header file  */
#ifndef STTUNER_MINIDRIVER

/* API */
ST_ErrorCode_t demod_d0299_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams);
ST_ErrorCode_t demod_d0299_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams);

ST_ErrorCode_t demod_d0299_Open (ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle);
ST_ErrorCode_t demod_d0299_Close(DEMOD_Handle_t  Handle, DEMOD_CloseParams_t *CloseParams);

ST_ErrorCode_t demod_d0299_IsAnalogCarrier (DEMOD_Handle_t Handle, BOOL *IsAnalog);        
ST_ErrorCode_t demod_d0299_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber);
ST_ErrorCode_t demod_d0299_GetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation);
ST_ErrorCode_t demod_d0299_SetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t  Modulation);
ST_ErrorCode_t demod_d0299_GetAGC          (DEMOD_Handle_t Handle, S16                  *Agc);
ST_ErrorCode_t demod_d0299_GetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t    *FECRates);
ST_ErrorCode_t demod_d0299_GetIQMode       (DEMOD_Handle_t Handle, STTUNER_IQMode_t     *IQMode); /*added for GNBvd26107->I2C failure due to direct access to demod device at API level*/
ST_ErrorCode_t demod_d0299_IsLocked        (DEMOD_Handle_t Handle, BOOL                 *IsLocked);
ST_ErrorCode_t demod_d0299_SetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t     FECRates);
ST_ErrorCode_t demod_d0299_Tracking        (DEMOD_Handle_t Handle, BOOL ForceTracking,   U32 *NewFrequency, BOOL *SignalFound);
ST_ErrorCode_t demod_d0299_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode);

ST_ErrorCode_t demod_d0299_ScanFrequency   (DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset, 
                                                                   U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                   U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                   U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                   U32  FreqOff,          U32   ChannelBW,     S32   EchoPos);

/* added for DiSEqC API support*/
ST_ErrorCode_t demod_d0299_DiSEqC       (	DEMOD_Handle_t Handle, 
									    STTUNER_DiSEqCSendPacket_t *pDiSEqCSendPacket,
									    STTUNER_DiSEqCResponsePacket_t *pDiSEqCResponsePacket
									   );

ST_ErrorCode_t demod_d0299_DiSEqCGetConfig ( DEMOD_Handle_t Handle ,STTUNER_DiSEqCConfig_t * DiSEqCConfig);
ST_ErrorCode_t demod_d0299_DiSEqCBurstOFF ( DEMOD_Handle_t Handle );

/***************************************/

ST_ErrorCode_t demod_d0299_ioctl           (DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams,
                                             STTUNER_Da_Status_t *Status);

/* I/O API */
ST_ErrorCode_t demod_d0299_ioaccess(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle,
                  STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);

/* local functions --------------------------------------------------------- */
#ifdef STTUNER_DRV_SAT_SCR
ST_ErrorCode_t demod_d0299_tonedetection(DEMOD_Handle_t Handle,U32 StartFreq, U32 StopFreq, U8  *NbTones,U32 *ToneList, U8 mode,int* power_detection_level);
#endif
D0299_InstanceData_t *D0299_GetInstFromHandle(DEMOD_Handle_t Handle);
#endif

#ifdef STTUNER_MINIDRIVER
	extern ST_ErrorCode_t tuner_tunsdrv_SetBandWidth (U32 Bandwidth);
	
	U8 DEFAULT_Register_0299[] = {0xa1,0x15,0x00,0x00,0x7d,0x05,0x02,0x00,0x40,0x00,0x82,0x00,0x40,0x81,0x23,0x15,
	                              0x34,0x84,0xb9,0x9b,0x9e,0xe3,0x80,0x18,0xff,0xff,0x82,0x00,0x7f,0x00,0x00,0x00,
	                              0x00,0x00,0x80,0x0b,0x2b,0x75,0x1a,0x00,0x00,0x1e,0x14,0x0f,0x09,0x05,0x00,0x00,
	                              0x00,0x1f,0x19,0xfc,0x13,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	                              0x00,0x00,0x00,0x00,0xa0
	                              };
#endif
/***************************************

Name: STTUNER_DRV_DEMOD_STV0299_Install()

Description:
    install a satellite device driver into the demod database.
    
Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifndef STTUNER_MINIDRIVER
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0299_Install(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
   const char *identity = "STTUNER d0299.c STTUNER_DRV_DEMOD_STV0299_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail driver already installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }
   
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("%s installing sat:demod:STV0299...", identity));
#endif

    /* mark ID in database */
    Demod->ID = STTUNER_DEMOD_STV0299;

    /* map API */
    Demod->demod_Init = demod_d0299_Init;
    Demod->demod_Term = demod_d0299_Term;

    Demod->demod_Open  = demod_d0299_Open;
    Demod->demod_Close = demod_d0299_Close;

    Demod->demod_IsAnalogCarrier  = demod_d0299_IsAnalogCarrier; 
    Demod->demod_GetSignalQuality = demod_d0299_GetSignalQuality;
    Demod->demod_GetModulation    = demod_d0299_GetModulation;   
    Demod->demod_SetModulation    = demod_d0299_SetModulation;   
    Demod->demod_GetAGC           = demod_d0299_GetAGC;          
    Demod->demod_GetFECRates      = demod_d0299_GetFECRates;  
    Demod->demod_GetIQMode        = demod_d0299_GetIQMode; /*added for GNBvd26107->I2C failure due to direct access to demod device at API level*/
    Demod->demod_IsLocked         = demod_d0299_IsLocked ;       
    Demod->demod_SetFECRates      = demod_d0299_SetFECRates;     
    Demod->demod_Tracking         = demod_d0299_Tracking;        
    Demod->demod_ScanFrequency    = demod_d0299_ScanFrequency;
	Demod->demod_DiSEqC	  = demod_d0299_DiSEqC;
	Demod->demod_GetConfigDiSEqC   = demod_d0299_DiSEqCGetConfig;  
	Demod->demod_SetDiSEqCBurstOFF   = demod_d0299_DiSEqCBurstOFF; 
    Demod->demod_ioaccess = demod_d0299_ioaccess;
    Demod->demod_ioctl    = demod_d0299_ioctl;
    #ifdef STTUNER_DRV_SAT_SCR
    Demod->demod_tonedetection = demod_d0299_tonedetection;
    #endif
    Demod->demod_StandByMode           = demod_d0299_StandByMode;
    InstanceChainTop = NULL;

    Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);


    Installed = TRUE;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0299_UnInstall()

Description:
    install a satellite device driver into the demod database.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0299_UnInstall(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
   const char *identity = "STTUNER d0299.c STTUNER_DRV_DEMOD_STV0299_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }
   
    if(Demod->ID != STTUNER_DEMOD_STV0299)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail incorrect driver type\n", identity));
#endif
        return(STTUNER_ERROR_ID);
    }

    /* has all memory been freed, by Term() */
    if(InstanceChainTop != NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail at least one instance not terminated\n", identity));
#endif
        return(ST_ERROR_OPEN_HANDLE);
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("%s uninstalling sat:demod:STV0299...", identity));
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
    Demod->demod_GetIQMode        = NULL; /*added for GNBvd26107->I2C failure due to direct access to demod device at API level*/
    Demod->demod_IsLocked         = NULL;       
    Demod->demod_SetFECRates      = NULL;     
    Demod->demod_Tracking         = NULL;        
    Demod->demod_ScanFrequency    = NULL;
	Demod->demod_DiSEqC			  = NULL;
	Demod->demod_GetConfigDiSEqC  = NULL;  
	Demod->demod_SetDiSEqCBurstOFF = NULL; 
    Demod->demod_ioaccess		  = NULL;
    Demod->demod_ioctl			  = NULL;
    Demod->demod_tonedetection = NULL;
    Demod->demod_StandByMode   = NULL;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("<"));
#endif


    STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);
  
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print((">"));
#endif

    InstanceChainTop = (D0299_InstanceData_t *)0x7ffffffe;
    Installed        = FALSE;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("ok\n"));
#endif

    return(Error);
}

#endif

/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
Name: demod_d0299_Init()

Description:
    
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0299_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    const char *identity = "STTUNER d0299.c demod_d0299_Init()";
#endif   
ST_ErrorCode_t Error = ST_NO_ERROR;
  #ifndef STTUNER_MINIDRIVER
    D0299_InstanceData_t *InstanceNew, *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
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
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull(DeviceName);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail DeviceName not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( D0299_InstanceData_t ));
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
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
    InstanceNew->DeviceMap.Registers = DEF0299_NBREG;
    InstanceNew->DeviceMap.Fields    = DEF0299_NBFIELD;
    InstanceNew->DeviceMap.Error   = ST_NO_ERROR;
    /* depreciated:    InstanceNew->DeviceMap.RegSize   = REGSIZE_8BITS; */
    InstanceNew->DeviceMap.Mode      = IOREG_MODE_SUBADR_8; /* NEW as of 3.4.0: i/o addressing mode to use */
    InstanceNew->DeviceMap.MemoryPartition = InitParams->MemoryPartition;
    InstanceNew->InstanceChainNext   = NULL; /* always last in the chain */
    InstanceNew->ExternalClock     = InitParams->ExternalClock;  
    InstanceNew->TSOutputMode      = InitParams->TSOutputMode;
    InstanceNew->SerialDataMode    = InitParams->SerialDataMode;
    #ifdef STTUNER_BLOCK_SYNC_MODE_SELECT
    InstanceNew->BlockSyncMode     = InitParams->BlockSyncMode;/* add block sync bit control for bug GNBvd27452*/
    #endif
    InstanceNew->SerialClockSource = InitParams->SerialClockSource;
    InstanceNew->FECMode           = InitParams->FECMode;
	/********Added for Diseqc***************************/
	InstanceNew->DiSEqCConfig.Command=STTUNER_DiSEqC_COMMAND;
    InstanceNew->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
	/**************************************************/
    
    /* reserve memory for register mapping */
    Error = STTUNER_IOREG_Open(&InstanceNew->DeviceMap);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail setup new register database\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);           
    }

    /* install a mapping */
    /*Error = Reg0299_Install(&InstanceNew->DeviceMap);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail install mapping\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);           
    }*/

    /*  DTV needs this (see TunerFecMode() in sttuner/tests/sat/sat_test.c) without this
       for an EVAL board we would have to hack the cable & for the 299 stem it would not work at all.
        Note also that for DTV decode using STPTI and STTUNER set STPTI_InitParams.DiscardSyncByte = TRUE
       and for packet injector (we have found, although YMMV) set STPTI_InitParams.DiscardSyncByte = 0 
       If you have a hacked cable (or PCB) then you can use STTUNER_Ioctl() to reset the register,
       directly after a call to STTUNER_Open() would be best.
    */
   

    /* set misc params for mapping */
    Error = Reg0299_Open(&InstanceNew->DeviceMap, InstanceNew->ExternalClock);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail setup register database params\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);           
    }

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes)\n", identity, InstanceNew->DeviceName, (U32)InstanceNew, sizeof( D0299_InstanceData_t ) ));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
#endif
/*********************************************MINIDRIVER*********************
*******************************************************************************
**********************************************************************************/    
#ifdef STTUNER_MINIDRIVER
	#if defined(ST_OS21) || defined(ST_OSLINUX)
	    Lock_InitTermOpenClose = semaphore_create_fifo(1);
	#else
	    semaphore_init_fifo(&Lock_InitTermOpenClose, 1);
	#endif   
/* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);
    DEMODInstance = memory_allocate_clear(InitParams->MemoryPartition, 1, sizeof( D0299_InstanceData_t ));
    if (DEMODInstance == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail memory allocation InstanceNew\n", identity));
#endif    
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_NO_MEMORY);           
    }
    DEMODInstance->DeviceName          = DeviceName;
    DEMODInstance->ExternalClock     = InitParams->ExternalClock;  
    DEMODInstance->TSOutputMode      = InitParams->TSOutputMode;
    DEMODInstance->SerialDataMode    = InitParams->SerialDataMode;
    DEMODInstance->MemoryPartition   = InitParams->MemoryPartition;
    #ifdef STTUNER_BLOCK_SYNC_MODE_SELECT
    DEMODInstance->BlockSyncMode     = InitParams->BlockSyncMode;/* add block sync bit control for bug GNBvd27452*/
    #endif
    DEMODInstance->SerialClockSource = InitParams->SerialClockSource;
    DEMODInstance->FECMode           = InitParams->FECMode;
    /********Added for Diseqc***************************/
    DEMODInstance->DiSEqCConfig.Command=STTUNER_DiSEqC_COMMAND;
    DEMODInstance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
    /**************************************************/
    SEM_UNLOCK(Lock_InitTermOpenClose);
        
    #endif
    return(Error);
}   

/* ----------------------------------------------------------------------------
Name: demod_d0299_Term()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0299_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
   const char *identity = "STTUNER d0299.c demod_d0299_Term()";
#endif	 
ST_ErrorCode_t Error = ST_NO_ERROR;
#ifndef STTUNER_MINIDRIVER
    D0299_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
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
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* reap next matching DeviceName */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("%s looking for first free handle[\n", identity));
#endif

    Instance = InstanceChainTop;
    while(1)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        if ( strcmp( (char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
            STTBX_Print(("]\n"));
#endif
            Error = STTUNER_IOREG_Close(&Instance->DeviceMap);
            if (Error != ST_NO_ERROR)
            {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
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
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
            STTBX_Print(("%s terminated ok\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL) 
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
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


#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    #endif
/*********************************MINIDRIVER*********************
****************************************************************
**********************************************************************/    
#ifdef STTUNER_MINIDRIVER
    
     /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    memory_deallocate(DEMODInstance->MemoryPartition, DEMODInstance);

    SEM_UNLOCK(Lock_InitTermOpenClose);
	    #if defined(ST_OS21) || defined(ST_OSLINUX)
	    semaphore_delete(Lock_InitTermOpenClose);
	    #else
	    semaphore_delete(&Lock_InitTermOpenClose);
	    #endif 
    
#endif
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: demod_d0299_Open()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0299_Open(ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
   const char *identity = "STTUNER d0299.c demod_d0299_Open()";
#endif		   
ST_ErrorCode_t Error = ST_NO_ERROR;
    
#ifndef STTUNER_MINIDRIVER
    U8 ChipID;
   
    D0299_InstanceData_t     *Instance;
    STTUNER_tuner_instance_t *TunerInstance;
    STTUNER_InstanceDbase_t  *Inst;
     

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
#endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL) 
        {       
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print((" found ok\n"));
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("%s using block at 0x%08x\n", identity, (U32)Instance));
#endif

    /* check handle IS actually free */
    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail driver instance already open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    /* now got pointer to free (and valid) data block */

    Inst = STTUNER_GetDrvInst();    /* pointer to instance database */

    /* wake up chip if in standby */
    
    Error = STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle, R0299_MCR, 0x00); /*reset value =0x00*/
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail wake device\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);           
    }

    /* check to see if chip is of the expected type, this is done afer register setup with
    reset values because the standby/sleep mode may have been done and  Reg0299_Reset() wakes
    the device up (depending on the polarity of the STDBY pin - see STV0299 data sheet */
    ChipID = STTUNER_IOREG_GetRegister(&Instance->DeviceMap, Instance->IOHandle, R0299_ID);
    
    if ( (ChipID & 0xF0) !=  0xA0)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail device not found (0x%02x)\n", identity, ChipID));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s device found, release/revision=%u\n", identity, ChipID & 0x0F));
#endif
    }
    /* reset all chip registers */
    Error|= STTUNER_IOREG_Reset(&Instance->DeviceMap, Instance->IOHandle, STV0299_DefVal, STV0299_Address);

  /* change resetvalues if the board contains DSF8910*/       
if((Inst[OpenParams->TopLevelHandle].Sat.Tuner.Driver->ID)==STTUNER_TUNER_DSF8910)
{ 
				
	Error |= STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle,  R0299_I2CRPT, 0x35);
	Error |= STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle,  R0299_RTC, 0x22);
	Error |= STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle,  R0299_AGC1R, 0xd9);
	Error |= STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle,  R0299_AGC2O, 0x3e);
	Error |= STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle,  R0299_ACLC, 0x88);
	Error |= STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle,  R0299_BCLC, 0x95);
	
	
} 
 
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail reset device\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);           
    }
    
    /* Set capabilities */
    Capability->FECAvail        = STTUNER_FEC_1_2 | STTUNER_FEC_2_3 | STTUNER_FEC_3_4 |
                                  STTUNER_FEC_5_6 | STTUNER_FEC_6_7 | STTUNER_FEC_7_8;
    Capability->ModulationAvail = STTUNER_MOD_ALL;  /* direct mapping to STTUNER_Modulation_t */
    Capability->AGCControl      = FALSE;
    Capability->SymbolMin       = STV0299_SYMBOL_RATE_MIN; /*   1 MegaSymbols/sec */
    Capability->SymbolMax       = STV0299_SYMBOL_RATE_MAX; /*  50 MegaSymbols/sec */

    Capability->BerMax           = MAX_BER; 
    Capability->SignalQualityMax = MAX_SIGNAL_QUALITY;
    Capability->AgcMax           = MAX_AGC;

    /* Setup stateblock */
    Instance->StateBlock.ScanMode     = DEF0299_CHANNEL;
    Instance->StateBlock.lastAGC2Coef =  1;
    #ifdef STTUNER_LNB_POWER_THRU_DEMODIO
	    Inst[OpenParams->TopLevelHandle].Sat.Demod.DeviceMap = &Instance->DeviceMap;
	    #ifdef STTUNER_LNB_TONE_THRU_DEMOD_DISEQC_PIN
	    Inst[OpenParams->TopLevelHandle].Sat.Demod.TEN_FieldIndex = F0299_DISEQCMODE;
	    #else
	    Inst[OpenParams->TopLevelHandle].Sat.Demod.TEN_FieldIndex = F0299_OP0VALUE;
	    #endif
	    Inst[OpenParams->TopLevelHandle].Sat.Demod.VEN_FieldIndex = F0299_OP1VALUE;
	    Inst[OpenParams->TopLevelHandle].Sat.Demod.VSEL_FieldIndex= F0299_LOCKOUTPUT;
    #endif
    /* make sure trigger facilities are off because it is not used for production and
      is not very nice anyway if you want to actually use the DAC 
     Reg0299_RegSetTrigger(&Instance->DeviceMap, IOREG_NO);*/

    /* TS output mode */
    switch (Instance->TSOutputMode)
    {
        case STTUNER_TS_MODE_SERIAL:
            STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0299_OUTPUTTYPE, 1);
            break;

        default:
        case STTUNER_TS_MODE_PARALLEL:
        case STTUNER_TS_MODE_DEFAULT:
            STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0299_OUTPUTTYPE, 0);
            break;
    }

    /* Set FEC mode */
    switch (Instance->FECMode)
    {
        case STTUNER_FEC_MODE_DIRECTV:
            STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0299_FECMODE, 4);
			STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0299_NYQUISTFILTER, 1);/* Nyquist Filter, GNBvd31071*/
			STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0299_DESCRAMBLER, 0);/* descrambler, GNBvd31071 */
            break;

        default:
        case STTUNER_FEC_MODE_DEFAULT:
        case STTUNER_FEC_MODE_DVB:
            STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0299_FECMODE, 0);
            break;
    }

    /* Only set serial configuration if serial mode output */
    if (Instance->TSOutputMode == STTUNER_TS_MODE_SERIAL)
    {
        /* Set serial clock source */
        switch(Instance->SerialClockSource)
        {
            case STTUNER_SCLK_VCODIV6:
                STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0299_SERCLK, 1);
                break;

            default:
            case STTUNER_SCLK_DEFAULT:
            case STTUNER_SCLK_MASTER:
                STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0299_SERCLK, 0);
                break;
        }

        /* Set serial data mode */
        
        if ( (Instance->SerialDataMode & STTUNER_SDAT_VALID_RISING) != 0)    /* Rising edge according to 299 data sheet*/
        {
            STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0299_OUTPUTCLOCKPOLARITY, 0);  
        }
        else
        {
            STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0299_OUTPUTCLOCKPOLARITY, 1);
        }

        if ((Instance->SerialDataMode & STTUNER_SDAT_PARITY_ENABLE) != 0)     /* Parity enable according to 299 data sheet */
        {
            STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0299_OUTPUTCLOCKCONFIG, 0);
        }
        else
        {
            STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0299_OUTPUTCLOCKCONFIG, 1);
        }

    } /* if(OpenParams->TSOutputMode) */

   /* This will control sync byte, 0x47 or 0xb8 after every 8th packet*/
   #ifdef STTUNER_BLOCK_SYNC_MODE_SELECT
    switch(Instance->BlockSyncMode)
    { case STTUNER_SYNC_NORMAL:
           STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0299_BLOCKSYNCHRO, 0);
           break;
      case STTUNER_SYNC_DEFAULT:
      case STTUNER_SYNC_FORCED:
           STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0299_BLOCKSYNCHRO, 1);
           break;
       default:
           STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0299_BLOCKSYNCHRO, 1);
            break;
     }
     #endif
   
    
    /* Set default error count mode i.e., QPSK bit error rate */
    STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0299_ERRORMODE,   0);
    STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0299_ERRORSOURCE, 0);
    STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0299_NOE,         3);


    /* if an EVALMAX is connected to this stv0299 then the stv0299 IAGC field needs
     to be set to one else zero it */
    TunerInstance = &Inst[OpenParams->TopLevelHandle].Sat.Tuner; /* pointer to tuner for this instance */

    if((TunerInstance->Driver->ID == STTUNER_TUNER_EVALMAX) || (TunerInstance->Driver->ID == STTUNER_TUNER_DSF8910) || (TunerInstance->Driver->ID == STTUNER_TUNER_STB6000))
    {
        STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0299_IAGC, 1);
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s setup for EVALMAX F0299_IAGC=1\n", identity));
#endif
    }
    else
    {
        STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0299_IAGC, 0);
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s no EVALMAX tuner driver, F0299_IAGC=0\n", identity));
#endif
    }


    /* finally (as nor more errors to check for, allocate handle */
    Instance->TopLevelHandle = OpenParams->TopLevelHandle;
    
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

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("%s opened ok\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    
    /* GNBvd17801-> Error Handling for any error returned during I2C operation inside the driver */
    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;
    #endif
/**************************************************************************
*****************************************************************************/    
#ifdef STTUNER_MINIDRIVER
    U8 Data;
        /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);
   
    /* write all demod register with default values */
    Error = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, 0x00,0,0,DEFAULT_Register_0299, 69, FALSE);
    
    if (Error != ST_NO_ERROR)
    {
        SEM_UNLOCK(Lock_InitTermOpenClose);
	return(Error);
    }
    Capability->SymbolMin       = STV0299_SYMBOL_RATE_MIN; /*   1 MegaSymbols/sec */
    Capability->SymbolMax       = STV0299_SYMBOL_RATE_MAX; /*  50 MegaSymbols/sec */

    /* TS output mode */
    switch (DEMODInstance->TSOutputMode)
    {
        case STTUNER_TS_MODE_SERIAL:
             Data = 1;
             STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_FECM, F0299_OUTPUTTYPE, F0299_OUTPUTTYPE_L, &Data, 1, FALSE);
             break;

        default:
        case STTUNER_TS_MODE_PARALLEL:
        case STTUNER_TS_MODE_DEFAULT:
             Data = 0;
             STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_FECM, F0299_OUTPUTTYPE, F0299_OUTPUTTYPE_L, &Data, 1, FALSE);
             break;
    }
    /* Set FEC mode */
    switch (DEMODInstance->FECMode)
    {
        case STTUNER_FEC_MODE_DIRECTV:
        Data = 4;
        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_FECM, F0299_FECMODE, F0299_FECMODE_L, &Data, 1, FALSE);
        Data = 1;
        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_IOCFG, F0299_NYQUISTFILTER, F0299_NYQUISTFILTER_L, &Data, 1, FALSE);
        Data = 0;
        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_RS, F0299_DESCRAMBLER, F0299_DESCRAMBLER_L, &Data, 1, FALSE);
        break;

        default:
        case STTUNER_FEC_MODE_DEFAULT:
        case STTUNER_FEC_MODE_DVB:
        Data = 0;
        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_FECM, F0299_FECMODE, F0299_FECMODE_L, &Data, 1, FALSE);
        break;
    }

    /* Only set serial configuration if serial mode output */
    if (DEMODInstance->TSOutputMode == STTUNER_TS_MODE_SERIAL)
    {
        /* Set serial clock source */
        switch(DEMODInstance->SerialClockSource)
        {
            case STTUNER_SCLK_VCODIV6:
            Data = 1;
            STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_MCR, F0299_SERCLK, F0299_SERCLK_L, &Data, 1, FALSE);
            break;

            default:
            case STTUNER_SCLK_DEFAULT:
            case STTUNER_SCLK_MASTER:
            Data = 0;
            STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_MCR, F0299_SERCLK, F0299_SERCLK_L, &Data, 1, FALSE);
            break;
        }

        /* Set serial data mode */
        
        if ( (DEMODInstance->SerialDataMode & STTUNER_SDAT_VALID_RISING) != 0)    /* Rising edge according to 299 data sheet*/
        {
            Data = 0;
            STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_RS,F0299_OUTPUTCLOCKPOLARITY,F0299_OUTPUTCLOCKPOLARITY_L, &Data, 1, FALSE);
            
        }
        else
        {
            Data = 1;
            STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_RS,F0299_OUTPUTCLOCKPOLARITY,F0299_OUTPUTCLOCKPOLARITY_L, &Data, 1, FALSE);
            
        }

        if ((DEMODInstance->SerialDataMode & STTUNER_SDAT_PARITY_ENABLE) != 0)     /* Parity enable according to 299 data sheet */
        {
            Data = 0;
            STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_RS, F0299_OUTPUTCLOCKCONFIG, F0299_OUTPUTCLOCKCONFIG_L, &Data, 1, FALSE);
            
        }
        else
        {
            Data = 1;
            STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_RS, F0299_OUTPUTCLOCKCONFIG, F0299_OUTPUTCLOCKCONFIG_L, &Data, 1, FALSE);
            
        }

    } /* if(OpenParams->TSOutputMode) */

   /* This will control sync byte, 0x47 or 0xb8 after every 8th packet*/
   #ifdef STTUNER_BLOCK_SYNC_MODE_SELECT
    switch(DEMODInstance->BlockSyncMode)
    { case STTUNER_SYNC_NORMAL:
    	   Data = 0;
    	   STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_RS, F0299_BLOCKSYNCHRO, F0299_BLOCKSYNCHRO_L, &Data,1, FALSE);
           
           break;
      case STTUNER_SYNC_DEFAULT:
      case STTUNER_SYNC_FORCED:
           Data = 1;
           STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_RS, F0299_BLOCKSYNCHRO, F0299_BLOCKSYNCHRO_L, &Data, 1, FALSE);
           break;
       default:
           Data = 1;
           STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_RS, F0299_BLOCKSYNCHRO,F0299_BLOCKSYNCHRO_L, &Data, 1, FALSE);
            break;
     }
     #endif
     
     Data = 1;
     STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_AGC1R, F0299_IAGC,F0299_IAGC_L, &Data,1, FALSE);
   
   DEMODInstance->TopLevelHandle = OpenParams->TopLevelHandle;

   #ifdef STTUNER_DRV_SAT_SCR
		#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
		    if(OpenParams->TopLevelHandle == 0)
		    DEMODInstance->DISECQ_ST_ENABLE = TRUE;
		    if(OpenParams->TopLevelHandle == 1)
		    DEMODInstance->DISECQ_ST_ENABLE = FALSE;
	    #endif
	#endif
    
	*Handle = (U32)DEMODInstance;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("%s opened ok\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    
    
    #endif
    
    return(Error);
}   

#ifdef STTUNER_MINIDRIVER
ST_ErrorCode_t demod_d0299_Close(DEMOD_Handle_t Handle, DEMOD_CloseParams_t *CloseParams)
{
	DEMODInstance->TopLevelHandle = STTUNER_MAX_HANDLES;
	return(ST_NO_ERROR);
}
#endif

#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: demod_d0299_Close()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0299_Close(DEMOD_Handle_t Handle, DEMOD_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
   const char *identity = "STTUNER d0299.c demod_d0299_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0299_InstanceData_t     *Instance;

    /* private driver instance data */
    Instance = D0299_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    /* put chip to sleep */
/*
    Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,  F0299_STDBY, 1);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail set standby\n", identity));
#endif
    } */   /* continue */

    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("%s closed\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: demod_d0299_IsAnalogCarrier()

Description:
    This routine checks for an analog carrier on the current frequency
    by setting the symbol rate to 5M (never a digital signal).

Parameters:
    IsAnalog,   pointer to area to store result:
                TRUE - is analog
                FALSE - is not analog

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0299_IsAnalogCarrier(DEMOD_Handle_t Handle, BOOL *IsAnalog)        
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
   const char *identity = "STTUNER d0299.c demod_d0299_IsAnalogCarrier()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U8 loop;
    U16 Agc2;
    U8 AGCIntegrator[2];
    D0299_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0299_GetInstFromHandle(Handle);

     Agc2 = 0;
    *IsAnalog = FALSE;  /* Assume no analog carrier */

    /* Set the symbol rate to analog carrier rate */
    Reg0299_RegSetSymbolRate(& Instance->DeviceMap, Instance->IOHandle, ANALOG_CARRIER_DETECT_SYMBOL_RATE);

    /* Take four AGC2 samples */
    for (loop = 0; loop < 4; loop++)
    {
        /* Read AGC2I1 and AGC2I2 registers */
        STTUNER_IOREG_GetContigousRegisters(&Instance->DeviceMap, Instance->IOHandle, R0299_AGC2I1, 2,AGCIntegrator);

        Agc2 += (AGCIntegrator[0]<<8) + AGCIntegrator[1];
    }

    Agc2 /= 4; /* Average AGC2 values */

    /* Test for good signal strength -- centre of analog carrier */
    *IsAnalog = (Agc2 < ANALOG_CARRIER_DETECT_AGC2_VALUE) ? TRUE : FALSE;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("%s IsAnalog=%u\n", identity, *IsAnalog));
#endif
    
    /* GNBvd17801-> Error Handling for any error returned during I2C operation inside the driver */
    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;
    
    return(Error);
}   
#endif


/* ----------------------------------------------------------------------------
Name: demod_d0299_GetSignalQuality()

Description:
  Obtains a signal quality setting for the current lock.
    
Parameters:
    SignalQuality_p,    pointer to area to store the signal quality value.
    Ber,                pointer to area to store the bit error rate.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0299_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
   const char *identity = "STTUNER d0299.c demod_d0299_GetSignalQuality()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
#ifndef STTUNER_MINIDRIVER
    D0299_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0299_GetInstFromHandle(Handle);

    /* Read noise estimations for C/N and BER */
    Drv0299_GetNoiseEstimator(&Instance->DeviceMap, Instance->IOHandle, SignalQuality_p, Ber);
    
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("%s SignalQuality=%u Ber=%u\n", identity, *SignalQuality_p, *Ber));
    #endif
    
    /* GNBvd17801-> Error Handling for any error returned during I2C operation inside the driver */
    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;
#endif   

#ifdef STTUNER_MINIDRIVER
     Drv0299_GetNoiseEstimator(SignalQuality_p, Ber);
   
#endif    

    return(Error);
}   
#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: demod_d0299_GetModulation()

Description:
    This routine returns the modulation scheme in use by this device.
    ** Currently only QPSK is supported.
    
Parameters:
    Modulation, pointer to area to store modulation scheme.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0299_GetModulation(DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
   const char *identity = "STTUNER d0299.c demod_d0299_GetModulation()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0299_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0299_GetInstFromHandle(Handle);

    /* This implementation of the DEMOD device only supports one type of modulation */
    *Modulation = STTUNER_MOD_QPSK;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("%s Modulation=%u\n", identity, *Modulation));
#endif
    return(Error);
}   
#endif

#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: demod_d0299_SetModulation()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0299_SetModulation(DEMOD_Handle_t Handle, STTUNER_Modulation_t Modulation)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
   const char *identity = "STTUNER d0299.c demod_d0299_SetModulation()";
#endif

    /* if modulations include STTUNER_MOD_QPSK */
    if ( (Modulation & STTUNER_MOD_QPSK) == STTUNER_MOD_QPSK)
    { 
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s Modulation: STTUNER_MOD_QPSK accepted ok.\n", identity));
#endif
        return(ST_NO_ERROR);
    }

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("%s fail, only STTUNER_MOD_QPSK supported.\n", identity));
#endif
    return(ST_ERROR_BAD_PARAMETER);
}   
#endif
#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: demod_d0299_GetAGC()

Description:
    Obtains the current value from the AGC integrator register and computes
    a look-up of power output.
    
Parameters:
    AGC,    pointer to area to store power output value.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0299_GetAGC(DEMOD_Handle_t Handle, S16  *Agc)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
   const char *identity = "STTUNER d0299.c demod_d0299_GetAGC()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0299_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0299_GetInstFromHandle(Handle);

    *Agc = Drv0299_GetPowerEstimator(&Instance->DeviceMap, Instance->IOHandle);

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("%s Agc=%u\n", identity, *Agc));
#endif
    
    /* GNBvd17801-> Error Handling for any error returned during I2C operation inside the driver */
    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;
    
    return(Error);
}   
#endif


/* ----------------------------------------------------------------------------
Name: demod_d0299_GetFECRates()

Description:
     Checks the VEN rate register to deduce the forward error correction
    setting that is currently in use.
   
Parameters:
    FECRates,   pointer to area to store FEC rates in use.
  
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0299_GetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t *FECRates)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
   const char *identity = "STTUNER d0299.c demod_d0299_GetFECRates()";
#endif
    U8 Data;
    U8 uData1; /* Used for retrieving the FEC Mode*/
    STTUNER_FECRate_t CurFecRate;
    ST_ErrorCode_t Error = ST_NO_ERROR;
	#ifndef STTUNER_MINIDRIVER
    D0299_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0299_GetInstFromHandle(Handle);

    /*
    ** uData1 - FEC Mode Register
    ** 0 - DVB QPSK, 1 - DVB BPSK, 4 = DSS
    */
    uData1 = STTUNER_IOREG_GetField(&Instance->DeviceMap, Instance->IOHandle,F0299_FECMODE);

    /* Convert venrate value to a STTUNER fecrate */
    /* Following switch statement is added to preempt bug GNBvd14519. Which required
       an error to be reported whenever a wrong FEC rate was specified in either of DVB or DirecTv Mode*/
    Data = STTUNER_IOREG_GetField(&Instance->DeviceMap, Instance->IOHandle, F0299_CPR);
    switch (Data)
    {
    case STV0299_VSTATUS_PR_1_2:
        CurFecRate = STTUNER_FEC_1_2;
        break;

    case STV0299_VSTATUS_PR_2_3:
        CurFecRate = STTUNER_FEC_2_3;
        break;

    case STV0299_VSTATUS_PR_3_4:
        CurFecRate = STTUNER_FEC_3_4;
        break;

    case STV0299_VSTATUS_PR_5_6:
        CurFecRate = STTUNER_FEC_5_6;
        break;
        
    case STV0299_VSTATUS_PR_7_8:
        if(uData1 == 4) /* DSS Mode */
            CurFecRate = STTUNER_FEC_6_7;
        else
            CurFecRate = STTUNER_FEC_7_8;
        break;
    default:
        CurFecRate = STTUNER_FEC_NONE;;
        break;
    }

    *FECRates = CurFecRate;  /* Copy back for caller */
    
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("%s FECRate=%u\n", identity, CurFecRate));
#endif
    
    /* GNBvd17801-> Error Handling for any error returned during I2C operation inside the driver */
    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;
	#endif
#ifdef STTUNER_MINIDRIVER
    Error = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R0299_VSTATUS, F0299_CPR, F0299_CPR_L, &Data,1, FALSE);
    switch (Data)
    {
    case STV0299_VSTATUS_PR_1_2:
        CurFecRate = STTUNER_FEC_1_2;
        break;

    case STV0299_VSTATUS_PR_2_3:
        CurFecRate = STTUNER_FEC_2_3;
        break;

    case STV0299_VSTATUS_PR_3_4:
        CurFecRate = STTUNER_FEC_3_4;
        break;

    case STV0299_VSTATUS_PR_5_6:
        CurFecRate = STTUNER_FEC_5_6;
        break;
        
    case STV0299_VSTATUS_PR_7_8:
        if(uData1 == 4) /* DSS Mode */
            CurFecRate = STTUNER_FEC_6_7;
        else
            CurFecRate = STTUNER_FEC_7_8;
        break;
    default:
        CurFecRate = STTUNER_FEC_NONE;;
        break;
    }

    *FECRates = CurFecRate;  /* Copy back for caller */
#endif
    
    return(Error);
}

#ifndef STTUNER_MINIDRIVER

/* ----------------------------------------------------------------------------
Name: demod_d0299_GetIQMode()
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
ST_ErrorCode_t demod_d0299_GetIQMode(DEMOD_Handle_t Handle, STTUNER_IQMode_t *IQMode)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
   const char *identity = "STTUNER d0299.c demod_d0299_GetIQMode()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0299_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0299_GetInstFromHandle(Handle);

    if(STTUNER_IOREG_GetField(&Instance->DeviceMap,Instance->IOHandle, F0299_IQ)==1)
            *IQMode = STTUNER_IQ_MODE_INVERTED;
    else
            *IQMode = STTUNER_IQ_MODE_NORMAL;
            
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("%s IQMode=%u\n", identity, *IQMode));
#endif
    return(Error);
}   
#endif


/* ----------------------------------------------------------------------------
Name: demod_d0299_IsLocked()

Description:
     Checks the LK register i.e., are we demodulating a digital carrier.
   
Parameters:
    IsLocked,   pointer to area to store result (bool):
                TRUE -- we are locked.
                FALSE -- no lock.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0299_IsLocked(DEMOD_Handle_t Handle, BOOL *IsLocked)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
   const char *identity = "STTUNER d0299.c demod_d0299_IsLocked()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
#ifndef STTUNER_MINIDRIVER
	D0299_InstanceData_t *Instance;
    /* private driver instance data */
    Instance = D0299_GetInstFromHandle(Handle);
    *IsLocked = STTUNER_IOREG_GetField(&Instance->DeviceMap, Instance->IOHandle, F0299_LK) ? TRUE : FALSE;
    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("%s IsLocked=%u\n", identity, *IsLocked));
    #endif
    /* GNBvd17801-> Error Handling for any error returned during I2C operation inside the driver */
    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;
#endif
#ifdef STTUNER_MINIDRIVER
	U8 Data;
	Error = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R0299_VSTATUS, F0299_LK, F0299_LK_L, &Data,1, FALSE);
	if(Data == 1)
	*IsLocked = TRUE;
	else
	*IsLocked = FALSE;
#endif    
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: demod_d0299_SetFECRates()

Description:
    Sets the FEC rates to be used during demodulation.
    Parameters:
    
Parameters:
      FECRates, bitmask of FEC rates to be applied.
  
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0299_SetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t FECRates)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
   const char *identity = "STTUNER d0299.c demod_d0299_SetFECRates()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U8 Data = 0, uData1;
#ifndef STTUNER_MINIDRIVER
    D0299_InstanceData_t *Instance;
    
    
    /* private driver instance data */
    Instance = D0299_GetInstFromHandle(Handle);

    /*
    ** uData1 - FEC Mode Register
    ** 0 - DVB QPSK, 1 - DVB BPSK, 4 = DSS
    */
    uData1 = STTUNER_IOREG_GetField(&Instance->DeviceMap, Instance->IOHandle,F0299_FECMODE);

    /*
    ** For each STTUNER FEC rates (see sttuner.h)
    ** Set specific bit of enable puncture rate register
    ** Bit 0 - 1/2, 1 - 2/3, 2 - 3/4, 3 - 5/6, 4 - 7/8(DVB) or 6/7(DSS)
    */
    if (FECRates & STTUNER_FEC_1_2) Data |= STV0299_VENRATE_E0_MSK;
    if (FECRates & STTUNER_FEC_2_3) Data |= STV0299_VENRATE_E1_MSK;
    if (FECRates & STTUNER_FEC_3_4) Data |= STV0299_VENRATE_E2_MSK;
    if (FECRates & STTUNER_FEC_5_6) Data |= STV0299_VENRATE_E3_MSK;
    if (((FECRates & STTUNER_FEC_6_7) && (uData1 == 4)) ||
        ((FECRates & STTUNER_FEC_7_8) && (uData1 != 4)))
    {
        Data |= STV0299_VENRATE_E4_MSK;
    }

    /* return if no FECs match */
    if ( Data == 0 )
        return(ST_ERROR_BAD_PARAMETER);

    Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F0299_RATE, Data);

    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("%s Field F0299_RATE = 0x%02x\n", identity, Data));
    #endif
    
    /* GNBvd17801-> Error Handling for any error returned during I2C operation inside the driver */
    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;
#endif
#ifdef STTUNER_MINIDRIVER

    Error = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R0299_FECM, F0299_FECMODE, F0299_FECMODE_L, &uData1, 1, FALSE);
    /*
    ** For each STTUNER FEC rates (see sttuner.h)
    ** Set specific bit of enable puncture rate register
    ** Bit 0 - 1/2, 1 - 2/3, 2 - 3/4, 3 - 5/6, 4 - 7/8(DVB) or 6/7(DSS)
    */
    if (FECRates & STTUNER_FEC_1_2) Data |= STV0299_VENRATE_E0_MSK;
    if (FECRates & STTUNER_FEC_2_3) Data |= STV0299_VENRATE_E1_MSK;
    if (FECRates & STTUNER_FEC_3_4) Data |= STV0299_VENRATE_E2_MSK;
    if (FECRates & STTUNER_FEC_5_6) Data |= STV0299_VENRATE_E3_MSK;
    if (((FECRates & STTUNER_FEC_6_7) && (uData1 == 4)) ||
        ((FECRates & STTUNER_FEC_7_8) && (uData1 != 4)))
    {
        Data |= STV0299_VENRATE_E4_MSK;
    }

    /* return if no FECs match */
    if ( Data == 0 )
        return(ST_ERROR_BAD_PARAMETER);
    Error = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_PR, F0299_RATE, F0299_RATE_L, &Data,1, FALSE);
    
#endif
    
    return(Error);
}   

#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: demod_d0299_Tracking()

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
ST_ErrorCode_t demod_d0299_Tracking(DEMOD_Handle_t Handle, BOOL ForceTracking, U32 *NewFrequency, BOOL *SignalFound)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
   const char *identity = "STTUNER d0299.c demod_d0299_Tracking()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    STTUNER_tuner_instance_t *TunerInstance;    /* don't really need all these variables, done for clarity */
    STTUNER_InstanceDbase_t  *Inst;             /* pointer to instance database */
    STTUNER_Handle_t          TopHandle;        /* instance that contains the demos, tuner and lnb driver set */
    D0299_InstanceData_t     *Instance;

    /* private driver instance data */
    Instance = D0299_GetInstFromHandle(Handle);
    /* top level public instance data */ 
    Inst = STTUNER_GetDrvInst();

    /* this driver knows what to level instance it belongs to */
    TopHandle = Instance->TopLevelHandle;

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[TopHandle].Sat.Tuner;

    Drv0299_CarrierTracking(&Instance->DeviceMap, Instance->IOHandle, &Instance->StateBlock, TopHandle);
	
	*NewFrequency = (U32)((Instance->StateBlock).Result.Frequency);
    Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.SymbolRate =(U32) (Instance->StateBlock).Result.SymbolRate;
    Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.AGC = Drv0299_GetPowerEstimator(&Instance->DeviceMap, Instance->IOHandle);
    demod_d0299_GetFECRates(Handle,&Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.FECRates);
    Drv0299_GetNoiseEstimator(&Instance->DeviceMap, Instance->IOHandle,&Inst[Instance->TopLevelHandle].CurrentTunerInfo.SignalQuality, &Inst[Instance->TopLevelHandle].CurrentTunerInfo.BitErrorRate);
    Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.IQMode=STTUNER_IOREG_GetField(&Instance->DeviceMap, Instance->IOHandle,F0299_IQ);
    *SignalFound  =     (Instance->StateBlock.Result.SignalType == E299_RANGEOK);
    
    
    #ifdef STTUNER_DRV_SAT_SCR
    if((Inst[Instance->TopLevelHandle].Capability.SCREnable)&& (Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.LNB_SignalRouting == STTUNER_VIA_SCRENABLED_LNB))
    {
      *NewFrequency = Inst[Instance->TopLevelHandle].ScanList.ScanList->ScrParams.SCRVCOFrequency - (U32)Instance->StateBlock.Result.Frequency;
    }
    else
    {
      *NewFrequency = (U32)Instance->StateBlock.Result.Frequency;
    }
    #else 
      *NewFrequency = (U32)Instance->StateBlock.Result.Frequency;
    #endif
    
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("%s SignalFound=%u NewFrequency=%u\n", identity, *SignalFound, *NewFrequency));
#endif
    
    /* GNBvd17801-> Error Handling for any error returned during I2C operation inside the driver */
    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;
    return(Error);
}   
#endif


/* ----------------------------------------------------------------------------
Name: demod_d0299_ScanFrequency()

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
ST_ErrorCode_t demod_d0299_ScanFrequency(DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset, 
                                                                U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                U32  FreqOff,          U32   ChannelBW,     S32   EchoPos)
 
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
   const char *identity = "STTUNER d0299.c demod_d0299_ScanFrequency()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t  *Inst;
    #ifdef STTUNER_DRV_SAT_SCR
    	U32 SCRVCOFrequency, TunerFrequency;
    #endif
#ifndef STTUNER_MINIDRIVER   
    D0299_InstanceData_t *Instance;
    STTUNER_tuner_instance_t *TunerInstance; 
    /* private driver instance data */
    Instance = D0299_GetInstFromHandle(Handle);
    /* top level public instance data */ 
    Inst = STTUNER_GetDrvInst();

    /* get the tuner instance for this driver from the top level handle */
    TunerInstance = &Inst[Instance->TopLevelHandle].Sat.Tuner;

    Drv0299_InitParams(&Instance->DeviceMap, Instance->IOHandle, &Instance->StateBlock);
    Drv0299_InitSearch(TunerInstance, &Instance->StateBlock, InitialFrequency, SymbolRate, MaxOffset*2, DerotatorStep);
    #ifdef STTUNER_DRV_SAT_SCR
    if((Inst[Instance->TopLevelHandle].Capability.SCREnable)&& (Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.LNB_SignalRouting == STTUNER_VIA_SCRENABLED_LNB))
    {
	    if(Instance->StateBlock.Params.SearchRange<20000000)
	    {
	    	Instance->StateBlock.Params.SearchRange = 20000000;
	    }
    }
    #endif
    /* Spectrum is used as the demode IQ mode within the sat driver */
    Instance->StateBlock.Params.DemodIQMode=(STTUNER_IQMode_t) Spectrum;
    
    /* Set SCR frequency here -> Now SCR Index starts from "0" v0.3 ST7 firmware*/
    #ifdef STTUNER_DRV_SAT_SCR
    if((Inst[Instance->TopLevelHandle].Capability.SCREnable)&& (Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.LNB_SignalRouting == STTUNER_VIA_SCRENABLED_LNB))
    { 
	    Error = scr_scrdrv_SetFrequency(Instance->TopLevelHandle, Handle, Instance->DeviceName, InitialFrequency/1000, Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->ScrParams.LNBIndex,Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->ScrParams.SCRBPF );
	    
	    TunerFrequency = (Inst[Instance->TopLevelHandle].Capability.SCRBPFFrequencies[Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->ScrParams.SCRBPF])*1000;
	    Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->ScrParams.BPFFrequency = TunerFrequency;
	    SCRVCOFrequency = TunerFrequency + InitialFrequency;
	    Instance->StateBlock.Params.Frequency = TunerFrequency;
	    Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->ScrParams.SCRVCOFrequency = SCRVCOFrequency;
    }
    #endif
    
    Drv0299_AutoSearchAlgo(&Instance->DeviceMap,Instance->IOHandle, &Instance->StateBlock, Instance->TopLevelHandle);

    if (Instance->StateBlock.Result.SignalType == E299_RANGEOK)
    {
        /* Pass new frequency to caller */
        
        #ifdef STTUNER_DRV_SAT_SCR
        if((Inst[Instance->TopLevelHandle].Capability.SCREnable)&& (Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.LNB_SignalRouting == STTUNER_VIA_SCRENABLED_LNB))
        { /*IFA new feature*/
        	*NewFrequency = SCRVCOFrequency - (U32)Instance->StateBlock.Result.Frequency;
        } 
        else
        {
        	*NewFrequency = (U32)Instance->StateBlock.Result.Frequency;
        }
        #else
        *NewFrequency = (U32)Instance->StateBlock.Result.Frequency;
        #endif
        
        #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s NewFrequency=%u\n", identity, *NewFrequency));
        #endif
    }

    *ScanSuccess = (Instance->StateBlock.Result.SignalType == E299_RANGEOK);

    #ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
	    STTBX_Print(("%s ScanSuccess=%u\n", identity, *ScanSuccess));
    #endif
    
    /* GNBvd17801-> Error Handling for any error returned during I2C operation inside the driver */
    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;
#endif
#ifdef STTUNER_MINIDRIVER
     D0299_SearchParams_t Params;    
     Inst = STTUNER_GetDrvInst();

     #ifdef STTUNER_DRV_SAT_SCR
     Error = scr_scrdrv_SetFrequency(DEMODInstance->TopLevelHandle, Handle, DEMODInstance->DeviceName, InitialFrequency/1000, Inst[DEMODInstance->TopLevelHandle].Sat.SingleScan.ScrParams.LNBIndex,Inst[DEMODInstance->TopLevelHandle].Sat.SingleScan.ScrParams.SCRBPF );
     TunerFrequency = (Inst[DEMODInstance->TopLevelHandle].Capability.SCRBPFFrequencies[Inst[DEMODInstance->TopLevelHandle].Sat.SingleScan.ScrParams.SCRBPF])*1000;
     SCRVCOFrequency = TunerFrequency + InitialFrequency;
     InitialFrequency = TunerFrequency;
     Inst[DEMODInstance->TopLevelHandle].Sat.SingleScan.ScrParams.SCRVCOFrequency = SCRVCOFrequency;
     #endif

    tuner_tunsdrv_SetBandWidth((U32)D0299_CarrierWidth(SymbolRate));
    Params.DemodIQMode = (STTUNER_IQMode_t) Spectrum;
    Drv0299_AutoSearchAlgo(InitialFrequency, SymbolRate, &Params);

    if (Params.State == E299_RANGEOK)
    {
        /* Pass new frequency to caller */
        *NewFrequency = (U32)Params.Frequency;
        #ifdef STTUNER_DRV_SAT_SCR
        *NewFrequency = SCRVCOFrequency - (U32)Params.Frequency;;
        #endif
    }

    *ScanSuccess = (Params.State == E299_RANGEOK);

#endif
   return(Error); 
}   
#ifdef STTUNER_MINIDRIVER
long D0299_CarrierWidth(long SymbolRate)
{
    return (SymbolRate  + (SymbolRate * 35)/100);
}   
#endif
/* ----------------------------------------------------------------------------
Name: demod_d0299_DiSEqC
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

ST_ErrorCode_t demod_d0299_DiSEqC       (	DEMOD_Handle_t Handle, 
									    STTUNER_DiSEqCSendPacket_t *pDiSEqCSendPacket,
									    STTUNER_DiSEqCResponsePacket_t *pDiSEqCResponsePacket
									   )
{	
	int Error=ST_NO_ERROR;
	
	unsigned char No_of_Bytes, *pCurrent_Data;
	#ifndef STTUNER_MINIDRIVER
	#ifdef STTUNER_DRV_SAT_LNBH21
	LNB_Handle_t LHandle;
	#endif
	#endif
	U8 Delay = 0;
#ifndef STTUNER_MINIDRIVER
	unsigned char Timeout_counter =0 ;/*for future implementation*/
	D0299_InstanceData_t *Instance;
	STTUNER_InstanceDbase_t *Inst;	
	/* If framing byte wants response (DiSEqC 2.0) report error*/
	pCurrent_Data = pDiSEqCSendPacket->pFrmAddCmdData;

	if (( *pCurrent_Data == FB_COMMAND_REPLY)|
			(*(pDiSEqCSendPacket->pFrmAddCmdData) == FB_COMMAND_REPLY_REPEATED))
    {	
        Error = ST_ERROR_FEATURE_NOT_SUPPORTED; /*Not supported for Dual 299 board*/	
        return (Error);
    }
		
	Instance = D0299_GetInstFromHandle(Handle);	
	Inst = STTUNER_GetDrvInst();
	if(Inst[Instance->TopLevelHandle].Sat.Lnb.Driver->ID == STTUNER_LNB_LNBH21)
	{
#ifdef STTUNER_DRV_SAT_LNBH21			
	LHandle = Inst[Instance->TopLevelHandle].Sat.Lnb.DrvHandle;
	Error = (Inst[Instance->TopLevelHandle].Sat.Lnb.Driver->lnb_setttxmode)(LHandle,STTUNER_LNB_TX);
	if (Error != ST_NO_ERROR)
	{
		return(Error);		
	}
#endif	
	}
	
	/* In loopthrough mode send DiSEqC-ST commands through first demod */
	#ifdef STTUNER_DRV_SAT_SCR
		#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
		    if(Instance->DISECQ_ST_ENABLE == FALSE)
		    {
			    Handle = DemodDrvHandleOne;
			    Instance = D0399_GetInstFromHandle(Handle);
		    }
		#endif
	#endif
	
	    while( STTUNER_IOREG_GetField(&Instance->DeviceMap, Instance->IOHandle, F0299_FIFOFULL) )
	    {
	        Timeout_counter++;
	        STTUNER_TaskDelay(50);
						 	
	    } /* Start with FIFO empty*/
		
	switch (pDiSEqCSendPacket->DiSEqCCommandType)
		
		{ 
		case	STTUNER_DiSEqC_TONE_BURST_OFF :/*Generic same as F22_low */
				
			
		case	STTUNER_DiSEqC_TONE_BURST_OFF_F22_LOW:  		/*f22 pin low a off*/
				
				/*DiSEqC(1:0)=00  DiSEqC(2)=X DiSEqC FIFO=empty */
				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,  F0299_DISEQCMODE, 0x00);

				if(pDiSEqCSendPacket->DiSEqCCommandType==STTUNER_DiSEqC_TONE_BURST_OFF)
                Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_OFF ;
				else
		        Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_OFF_F22_LOW ;
                Instance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
				
				break;
	
		case	STTUNER_DiSEqC_TONE_BURST_OFF_F22_HIGH:			/*f22 pin high  off */
				
				/*DiSEqC(1:0)=01  DiSEqC(2)=X DiSEqC FIFO=empty */
				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,  F0299_DISEQCMODE, 0x01);

				Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_OFF_F22_HIGH ;
                Instance->DiSEqCConfig.ToneState= STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
				
			
				break;

		
		case	STTUNER_DiSEqC_CONTINUOUS_TONE_BURST_ON:  		/*unmodulated */
				
				/*DiSEqC(1:0)=11  DiSEqC(2)=X DiSEqC FIFO=XX  */
				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,  F0299_DISEQCMODE, 0x03);
				
				Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_CONTINUOUS_TONE_BURST_ON;
                		Instance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS;
						
				break;

		case	STTUNER_DiSEqC_TONE_BURST_SEND_0_UNMODULATED:   /*send of 0 for 12.5 ms ;continuous tone*/
				
				Error = STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle,  R0299_DISEQCFIFO, 0x00);
				
				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,  F0299_UNMODULATEDBURST, 0x00);
				if (Error !=ST_NO_ERROR){break;}
				
				/*DiSEqC(1:0)=10  DiSEqC(2)=0 DiSEqC FIFO=00 */
				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,  F0299_DISEQCMODE, 0x02);
				if (Error !=ST_NO_ERROR){break;}

				Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_SEND_0_UNMODULATED;
                		Instance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
												
				break;

		case	STTUNER_DiSEqC_TONE_BURST_SEND_0_MODULATED:     /*0-2/3 duty cycle tone*/
				
				/*DiSEqC(1:0)=10  DiSEqC(2)=1 DiSEqC FIFO=00 */
				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,  F0299_DISEQCMODE, 0x02);
				if (Error !=ST_NO_ERROR){break;}
				
				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,  F0299_UNMODULATEDBURST, 0x01);
				
				if (Error !=ST_NO_ERROR){break;}
				
				
				Error = STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle,  R0299_DISEQCFIFO, 0x00);
				Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_SEND_0_MODULATED;
                Instance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
							
				break;

		case	STTUNER_DiSEqC_TONE_BURST_SEND_1: 				/*1-1/3 duty cycle tone modulated*/
								
				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,  F0299_UNMODULATEDBURST, 0x01);
				if (Error !=ST_NO_ERROR){break;}
								
				Error = STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle,  R0299_DISEQCFIFO, 0xff);
				/*DiSEqC(1:0)=10  DiSEqC(2)=1 DiSEqC FIFO=FF */
				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,  F0299_DISEQCMODE, 0x02);
				if (Error !=ST_NO_ERROR){break;}
				
				Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_SEND_1;
                		Instance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
								
				break;

		case	STTUNER_DiSEqC_COMMAND:							/*DiSEqC (1.2/2)command */
				
				/*DiSEqC(1:0)=10  DiSEqC(2)=1 DiSEqC FIFO=data */
				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,  F0299_DISEQCMODE, 0x02);
				if (Error !=ST_NO_ERROR){break;}
				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,  F0299_UNMODULATEDBURST, 0x01);
				if (Error !=ST_NO_ERROR){break;}
				for(No_of_Bytes = pDiSEqCSendPacket->uc_TotalNoOfBytes;No_of_Bytes>0;No_of_Bytes--)
					{
						while( STTUNER_IOREG_GetField(&Instance->DeviceMap, Instance->IOHandle, F0299_FIFOFULL) )
							
							{
								Timeout_counter++;
								STTUNER_TaskDelay(50);
					 		}
						Error = STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle,  R0299_DISEQCFIFO, *pCurrent_Data);
						pCurrent_Data++;
				
					}
				Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_COMMAND;
                		Instance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
                		Delay = 135;
				break;
		
		default:
				Error=ST_ERROR_BAD_PARAMETER;
	
	
		}/*end of Switch*/
	
	/* GNBvd17801-> Error Handling for any error returned during I2C operation inside the driver */
	Error |= Instance->DeviceMap.Error;
	Instance->DeviceMap.Error = ST_NO_ERROR;
	#endif
#ifdef STTUNER_MINIDRIVER
       U8 Data;
      
	pCurrent_Data = pDiSEqCSendPacket->pFrmAddCmdData;

	if (( *pCurrent_Data == FB_COMMAND_REPLY)|
			(*(pDiSEqCSendPacket->pFrmAddCmdData) == FB_COMMAND_REPLY_REPEATED))
        {	
	        Error = ST_ERROR_FEATURE_NOT_SUPPORTED; /*Not supported for Dual 299 board*/	
	        return (Error);
        }
	
	do
        {
         STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R0299_DISEQCSTATUS, F0299_FIFOFULL, F0299_FIFOFULL_L, &Data, 1, FALSE);
	 if(!Data)
	 break;
	 STTUNER_TaskDelay(50);
	}while( Data ); /* Start with FIFO empty*/
		
	switch (pDiSEqCSendPacket->DiSEqCCommandType)
		
		{ 
		case	STTUNER_DiSEqC_TONE_BURST_OFF :/*Generic same as F22_low */
		case	STTUNER_DiSEqC_TONE_BURST_OFF_F22_LOW:  		/*f22 pin low a off*/
			/*DiSEqC(1:0)=00  DiSEqC(2)=X DiSEqC FIFO=empty */
			Data = 0x00;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_DISEQCSTATUS, F0299_DISEQCMODE, F0299_DISEQCMODE_L, &Data, 1, FALSE);
			if(pDiSEqCSendPacket->DiSEqCCommandType==STTUNER_DiSEqC_TONE_BURST_OFF)
                	DEMODInstance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_OFF ;
			else
		        DEMODInstance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_OFF_F22_LOW ;
                	DEMODInstance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
			break;
	
		case	STTUNER_DiSEqC_TONE_BURST_OFF_F22_HIGH:			/*f22 pin high  off */
			/*DiSEqC(1:0)=01  DiSEqC(2)=X DiSEqC FIFO=empty */
			Data = 0x01;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_DISEQCSTATUS, F0299_DISEQCMODE, F0299_DISEQCMODE_L, &Data, 1, FALSE);
			DEMODInstance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_OFF_F22_HIGH ;
                	DEMODInstance->DiSEqCConfig.ToneState= STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
			break;

		
		case	STTUNER_DiSEqC_CONTINUOUS_TONE_BURST_ON:  		/*unmodulated */
			/*DiSEqC(1:0)=11  DiSEqC(2)=X DiSEqC FIFO=XX  */
			Data = 0x03;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_DISEQCSTATUS, F0299_DISEQCMODE, F0299_DISEQCMODE_L, &Data, 1, FALSE);
			DEMODInstance->DiSEqCConfig.Command=STTUNER_DiSEqC_CONTINUOUS_TONE_BURST_ON;
                	DEMODInstance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS;
			break;

		case	STTUNER_DiSEqC_TONE_BURST_SEND_0_UNMODULATED:   /*send of 0 for 12.5 ms ;continuous tone*/
			Data = 0x00;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_DISEQCFIFO, 0x00, 0x00, &Data, 1, FALSE);
			Data = 0x00;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_DISEQCFIFO, F0299_UNMODULATEDBURST, F0299_UNMODULATEDBURST_L, &Data, 1, FALSE);
			/*DiSEqC(1:0)=10  DiSEqC(2)=0 DiSEqC FIFO=00 */
			Data = 0x02;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_DISEQCSTATUS, F0299_DISEQCMODE, F0299_DISEQCMODE_L, &Data, 1, FALSE);
			if (Error !=ST_NO_ERROR){break;}
			DEMODInstance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_SEND_0_UNMODULATED;
                	DEMODInstance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
			break;

		case	STTUNER_DiSEqC_TONE_BURST_SEND_0_MODULATED:     /*0-2/3 duty cycle tone*/
			/*DiSEqC(1:0)=10  DiSEqC(2)=1 DiSEqC FIFO=00 */
			Data = 0x02;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_DISEQCSTATUS, F0299_DISEQCMODE, F0299_DISEQCMODE_L, &Data, 1, FALSE);
			Data = 0x01;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_DISEQCFIFO, F0299_UNMODULATEDBURST, F0299_UNMODULATEDBURST_L, &Data, 1, FALSE);
			Data = 0x00;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_DISEQCFIFO, 0x00, 0x00, &Data, 1, FALSE);
			DEMODInstance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_SEND_0_MODULATED;
			DEMODInstance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
			break;

		case	STTUNER_DiSEqC_TONE_BURST_SEND_1: 				/*1-1/3 duty cycle tone modulated*/
								
			Data = 0x01;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_DISEQCFIFO, F0299_UNMODULATEDBURST, F0299_UNMODULATEDBURST_L, &Data, 1, FALSE);
			Data = 0xff;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_DISEQCFIFO, 0x00, 0x00, &Data, 1, FALSE);
			/*DiSEqC(1:0)=10  DiSEqC(2)=1 DiSEqC FIFO=FF */
			Data = 0x02;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_DISEQCSTATUS, F0299_DISEQCMODE, F0299_DISEQCMODE_L, &Data, 1, FALSE);
			DEMODInstance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_SEND_1;
			DEMODInstance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
			break;

		case	STTUNER_DiSEqC_COMMAND:							/*DiSEqC (1.2/2)command */
		
			/*DiSEqC(1:0)=10  DiSEqC(2)=1 DiSEqC FIFO=data */
			Data = 0x02;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_DISEQC, F0299_DISEQCMODE, F0299_DISEQCMODE_L, &Data, 1, FALSE);
			Data = 0x01;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_DISEQC, F0299_UNMODULATEDBURST, F0299_UNMODULATEDBURST_L, &Data, 1, FALSE);
			for(No_of_Bytes = pDiSEqCSendPacket->uc_TotalNoOfBytes;No_of_Bytes>0;No_of_Bytes--)
				{
					
					do
				        {
				         STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R0299_DISEQCSTATUS, F0299_FIFOFULL, F0299_FIFOFULL_L, &Data, 1, FALSE);
					 if(!Data)
					 break;
					 STTUNER_TaskDelay(50);
					}while( Data ); /* Start with FIFO empty*/
					Data = 0xff;
			                STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R0299_DISEQCFIFO, 0x00, 0x00, pCurrent_Data, 1, FALSE);
					pCurrent_Data++;
			
				}
			
			DEMODInstance->DiSEqCConfig.Command=STTUNER_DiSEqC_COMMAND;
			DEMODInstance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
			Delay = 135;
			break;
		
		default:
		Error=ST_ERROR_BAD_PARAMETER;
		}/*end of Switch*/														

#endif
	
	if (Error !=ST_NO_ERROR)
		{	return(Error);
		}
	/* Wait till DiSEqC command is sent 13.5 msec * total_no_of_bytes*/
	DiSEqC_Delay_ms ((Delay*pDiSEqCSendPacket->uc_TotalNoOfBytes)/10);
	/* wait for 'pDiSEqCSendPacket->uc_msecBeforeNextCommand' millisec. */
	/*DiSEqC_Delay_ms(pDiSEqCSendPacket->uc_msecBeforeNextCommand); */
	
	
	return(Error);
}


/* ----------------------------------------------------------------------------
Name: demod_d0299_DiSEqCGetConfig()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0299_DiSEqCGetConfig(DEMOD_Handle_t Handle,STTUNER_DiSEqCConfig_t *DiSEqCConfig)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
#ifndef STTUNER_MINIDRIVER
    D0299_InstanceData_t *Instance;

    Instance =D0299_GetInstFromHandle(Handle);

    DiSEqCConfig->Command=Instance->DiSEqCConfig.Command;
    DiSEqCConfig->ToneState=Instance->DiSEqCConfig.ToneState;
#endif
#ifdef STTUNER_MINIDRIVER
    DiSEqCConfig->Command=DEMODInstance->DiSEqCConfig.Command;
    DiSEqCConfig->ToneState=DEMODInstance->DiSEqCConfig.ToneState;
#endif

    return(Error);
}  

/* ----------------------------------------------------------------------------
Name: demod_d0299_DiSEqCBurstOFF()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0299_DiSEqCBurstOFF ( DEMOD_Handle_t Handle )
{
   ST_ErrorCode_t Error = ST_NO_ERROR;
  /* D0299_InstanceData_t *Instance;*/
   STTUNER_DiSEqCSendPacket_t DiSEqCSendPacket;
   void *pDiSEqCResponsePacket=NULL;
   
   /*Instance =D0299_GetInstFromHandle(Handle);
   Instance->DiSEqCConfig.Command=DiSEqCConfig->Command;
   Instance->DiSEqCConfig.ToneState=DiSEqCConfig->ToneState;*/

   DiSEqCSendPacket.DiSEqCCommandType = STTUNER_DiSEqC_TONE_BURST_OFF;

   DiSEqCSendPacket.uc_msecBeforeNextCommand =0;
   Error=demod_d0299_DiSEqC(Handle,&DiSEqCSendPacket,pDiSEqCResponsePacket);

   
   return(Error);
}
#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: demod_d0299_ioctl()

Description:
    access device specific low-level functions
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0299_ioctl(DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
   const char *identity = "STTUNER d0299.c demod_d0299_ioctl()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0299_InstanceData_t *Instance;
    STTUNER_InstanceDbase_t  *Inst;             /* pointer to instance database */
    

    /* private driver instance data */
    Instance = D0299_GetInstFromHandle(Handle);

    /* top level public instance data */ 
    Inst = STTUNER_GetDrvInst();

    
    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s demod driver instance handle=%d\n", identity, Handle));
        STTBX_Print(("%s                     function=%d\n", identity, Function));
        STTBX_Print(("%s                   I/O handle=%d\n", identity, Instance->IOHandle));
#endif

    /* ---------- select what to do ---------- */
    switch(Function){

        case STTUNER_IOCTL_RAWIO: /* read/write device register (actual write to stv0299) */
            Error =  STTUNER_IOARCH_ReadWrite( Instance->IOHandle, 
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->Operation,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->SubAddr,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->Data,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->TransferSize,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->Timeout );
            break;

        case STTUNER_IOCTL_REG_IN: /* read device register */
            *(int *)OutParams = STTUNER_IOREG_GetRegister(&Instance->DeviceMap, Instance->IOHandle, *(int *)InParams);
            break;

        case STTUNER_IOCTL_REG_OUT: /* write device register. NOTE: RegIndex is <<INDEX>> TO register number. */
            Error =  STTUNER_IOREG_SetRegister(  &Instance->DeviceMap, 
                                                  Instance->IOHandle,
                  ((SATIOCTL_SETREG_InParams_t *)InParams)->RegIndex,
                  ((SATIOCTL_SETREG_InParams_t *)InParams)->Value );
            break;
        case STTUNER_IOCTL_IQMODE: /* Get the IQ mode */

            if(STTUNER_IOREG_GetField(&Instance->DeviceMap,Instance->IOHandle, F0299_IQ)==1)
                *((STTUNER_IQMode_t *)OutParams)=STTUNER_IQ_MODE_INVERTED;
            else
                *((STTUNER_IQMode_t *)OutParams)=STTUNER_IQ_MODE_NORMAL;
            break;
        case STTUNER_IOCTL_SEARCH_TEST: /* For testing the search termination */

            /* Call the function to turn on the infinite loop in the search algorithm */
            /* get the tuner instance for this driver from the top level handle */
            Drv0299_TurnOnInfiniteLoop(Instance->TopLevelHandle,(BOOL *)InParams);
            
        default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
    STTBX_Print(("%s called ok\n", identity, Function));    /* signal that function came back */
#endif
    
    /* GNBvd17801-> Error Handling for any error returned during I2C operation inside the driver */
    Error |= Instance->DeviceMap.Error;
    Instance->DeviceMap.Error = ST_NO_ERROR;
    
    return(Error);

}



/* ----------------------------------------------------------------------------
Name: demod_d0299_ioaccess()

Description:
    called from ioarch.c when some driver does I/O but with the repeater/passthru
    set to point to this function.
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0299_ioaccess(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)

{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
   const char *identity = "STTUNER d0299.c demod_d0299_ioaccess()";
   
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    IOARCH_Handle_t ThisIOHandle;
    D0299_InstanceData_t *Instance;
    
    Instance = D0299_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* this (demod) drivers I/O handle */
    ThisIOHandle = Instance->IOHandle;

    
    /* if STTUNER_IOARCH_MAX_HANDLES then passthrough function required using our device handle/address */ 
    if (IOHandle == STTUNER_IOARCH_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s write passthru\n", identity));
#endif
        Error = STTUNER_IOARCH_ReadWrite(ThisIOHandle, Operation, SubAddr, Data, TransferSize, Timeout);
    }
    /* DIRECT Route Added by  we do nothing here */
    else    /* repeater */
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s write repeater\n", identity));
#endif

         
        /* enable repeater then send the data  using I/O handle supplied through ioarch.c */
 /**************For Debug purpose************************/
     #ifdef DMP     
     memset( Trace_PrintBuffer, '\0', 40 );	
     sprintf(Trace_PrintBuffer,"SETTING I2CT REGISTER TIO[%d]",ThisIOHandle );  
     DUMP_DATA (Trace_PrintBuffer );    
     #endif     
     
     Error = STTUNER_IOREG_SetField(& Instance->DeviceMap, ThisIOHandle, F0299_I2CT, 1);
     
    /******************For DEbug Purpose***************************/
     #ifdef DMP
     memset( Trace_PrintBuffer, '\0', 40 );	
     sprintf(Trace_PrintBuffer,"I2CT NOW SET TIO[%d]",ThisIOHandle );  
     DUMP_DATA (Trace_PrintBuffer );    
     #endif
/**********************************************************/
     
     
        /* send callers data using their address. (nb: subaddress == 0)
         Function 'STTUNER_IOARCH_ReadWriteNoRep' is called as calling the normal 'STTUNER_IOARCH_ReadWrite'
        function would cause it to call the redirection function which is THIS function, and around we 
        would go forever. */
        Error = STTUNER_IOARCH_ReadWriteNoRep(IOHandle, Operation, 0, Data, TransferSize, Timeout);
       
  }

  return(Error);
}
#endif



/* ----------------------------------------------------------------------------
Name: demod_d0299_StandByMode()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */



ST_ErrorCode_t demod_d0299_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode)
{
	
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
   const char *identity = "STTUNER d0299.c demod_d0299_StandByMode()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0299_InstanceData_t *Instance;
   
    /* private driver instance data */
    Instance = D0299_GetInstFromHandle(Handle);
   
      
    switch ( PowerMode)
    {
       case STTUNER_NORMAL_POWER_MODE :
       if(Instance->StandBy_Flag == 1)
       {
          Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0299_STDBY, 0);
       }
          
             
       if(Error==ST_NO_ERROR)
       {
       	 
          Instance->StandBy_Flag = 0 ;
       }
       
       break;
       case STTUNER_STANDBY_POWER_MODE :
       if(Instance->StandBy_Flag == 0)
       {
          Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F0299_STDBY, 1);
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

D0299_InstanceData_t *D0299_GetInstFromHandle(DEMOD_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299_HANDLES
   const char *identity = "STTUNER d0299.c D0299_GetInstFromHandle()";
#endif
    D0299_InstanceData_t *Instance;

    Instance = (D0299_InstanceData_t *)Handle;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299_HANDLES
    STTBX_Print(("%s block at 0x%08x\n", identity, Instance));
#endif

    return(Instance);
}
#ifndef STTUNER_MINIDRIVER
#ifdef STTUNER_DRV_SAT_SCR
/* For tone detection */
ST_ErrorCode_t demod_d0299_tonedetection(DEMOD_Handle_t Handle,U32 StartFreq, U32 StopFreq, U8  *NbTones,U32 *ToneList,U8 mode,int* power_detection_level)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0299_InstanceData_t *Instance;
    
    Instance = D0299_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }
    
       
    *NbTones = Drv0299_ToneDetection(&(Instance->DeviceMap),Instance->TopLevelHandle,StartFreq,StopFreq,ToneList,mode,power_detection_level);


    return(Error);
    
}
#endif
#endif
/* End of d0299.c */

