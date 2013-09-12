/* ----------------------------------------------------------------------------
File Name: d0399E.c

Description: 

    stv0399E demod driver.




Copyright (C) 2003-2004 STMicroelectronics

History:
   date: 15-07-2004
   version: 1.0.0
   author : SD
   comment: First release for 399E.

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
/* STAPI */

#include "sttuner.h"                    

#ifndef STTUNER_MINIDRIVER
#include "stevt.h"
/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
   /* data types for databases */
#include "ioarch.h"     /* I/O for this driver */
#include "ioreg.h"      /* I/O for this driver */
#endif
/* LLA */
#include "dbtypes.h" 
#include "399_drv.h"   /* driver */
#include "util399E.h"  /* utility functions to support STV0399 LLA */
#include "d0399.h"      /* header for this file */
#include "sioctl.h"     /* data structure typedefs for all the the sat ioctl functions */

#include "sysdbase.h"   /* functions to accesss system data */
#include "stcommon.h" 

#if defined (ST_OS21) || defined (ST_OSLINUX)
#define STTUNER_TaskDelay(x) STOS_TaskDelay((signed int)x)
#else
#define STTUNER_TaskDelay(x) STOS_TaskDelay((unsigned int)x)
#endif

#ifdef STTUNER_DRV_SAT_SCR
extern ST_ErrorCode_t scr_scrdrv_SetFrequency   (STTUNER_Handle_t Handle, DEMOD_Handle_t DemodHandle, ST_DeviceName_t *DeviceName, U32  InitialFrequency,  U8 LNBIndex, U8 SCRBPF );
	
	#define BAND1_FREQUENCY                 1399000
	#define BAND2_FREQUENCY                 1516000
	#define BAND3_FREQUENCY                 1632000
	#define BAND4_FREQUENCY                 1748000
	
#endif
static  int FE_399_SignalQuality_LookUp[20][2] =	{
												{    	10,	 15},
												{	20,	 21},
												{	30,	 27},
												{	40,	 33},
												{	50,	 39},
												{	60,	 45},
												{	70,	 51},
												{	80,	 57},
												{	90,	 63},
												{	100, 69},
												{	110, 75},
												{	120, 81},
												{	130, 87},
												{	140, 93},
												{	150,100},
												{	160,100},
												{	170,100},
												{	180,100},
												{	190,100},
												{	200,100}
												};


U8 Def399Val[STV399_NBREGS]=
 {
/*   0       1       2      3       4       5       6        7       8       9       A       B       C       D       E        F */ 

/*  ID     I2CRPT  ACR    F22FR   DACR1   DACR2   DISEQC  DqCFIFO DqCSTATS DISEQC2 IOCFG1  IOCFG2  AGC0C   AGC0R   AGC1C    AGC1CN
0x*/0xb5,  0x07,   0x2a,  0x8e,   0xa2,   0x00,   0x60,    0x00,  0x02,    0x00,   0x20,   0x50,   0x01,   0x51,   0xe9,    0x81,

/*  RTC    AGC1R  AGC1RN  AGC2O   TLSR    CFD     ACLC     BCLC   R8PSK    LDT     LDT2    AGC0CMD AGC0I   AGC1S   AGC1P    AGC1IN	
1x*/0x23,  0x14,   0x14,  0x74,   0x84,   0xd9,   0x87,    0xa8,  0x03,    0xdd,   0xc9,   0xa0,   0x05,   0xc3,   0xbf,    0x80,

/*  TLIR   AGC2I1  AGC2I2 RTF     VSTATUS LDI     ECNTM    ECNTL  SFRH     SFRM    SFRL    CFRM    CFRL    NIRM    NIRL     VERROR	
2x*/0x78,  0x41,   0x7e,  0xff,   0x9a,   0x7f,   0x00,    0x00,  0x41,    0x2f,   0x60,   0xff,   0x17,   0x0d,   0xc4,    0x00,

/*  FECM   VTH0    VTH1   VTH2    VTH3    VTH4    VTH5     PR     VAVSRCH  RS	   RSOUT  ERRCTRL VITPROG ERRCTRL2 ECNTM2   ECNTL2	
3x*/0x01,  0x1e,   0x14,  0x0f,   0x09,   0x0c,   0x05,    0x2f,  0x19,    0xbc,   0x10,   0xb3,   0x01,   0x31,   0x00,    0x00,

/*  DCLK1  LPF     DCLK2 ACOARSE AFINEMSB AFNLSB SYNTCTRL SNCTRL2 SYSCTRL  AGC1EP  AGC1ES TSTDCADJ TAGC1   TAGC1N  TPOLYPH  TSTR
4x*/0x40,  0x3f,   0x08,  0x7a,   0x03,   0xff,   0x51,    0x00,  0x0e,    0x8f,   0xf6,   0x00,   0x00,   0x00,   0x00,    0x00,
	
/*  TAGC2  TCTL1   TCTL2  TSTRAM1 TSTRATE SELOUT  FORCEIN  TSTFIFO TSTRS   TSTDIS  TSTI2C  TSTCK   TSTRES  TSTOUT  TSTIN    READ
5x*/0x00,  0x00,   0x00,  0x00,   0x00,   0x00,   0x00,    0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,    0x00,

/* TSTFREE TSTTNR1 TSTTNR2
6x*/0x00,  0x00,   0x00
 
};



/* Private types/constants ------------------------------------------------ */
#ifndef STTUNER_MINIDRIVER
	#define ANALOG_CARRIER_DETECT_SYMBOL_RATE   5000000
	#define ANALOG_CARRIER_DETECT_AGC2_VALUE    25
	/* Device capabilities */
	#define MAX399_AGC                         255
	#define MAX399_SIGNAL_QUALITY              100
	#define MAX399_BER                         200000
#endif


/* Change STV0399 tuner scan step from default (calculated) value,
   bigger value == faster scanning, smaller value == more channels picked up */
#define STV0399_TUNER_SCAN_STEP   0

#define STV0399E_SYMBOL_RATE_MIN 1000000
#define STV0399E_SYMBOL_RATE_MAX 45000000

#define STV0399E_FREQ_MIN 950000
#define STV0399E_FREQ_MAX 2150000

/* private variables ------------------------------------------------------- */

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */

#ifndef STTUNER_MINIDRIVER
static BOOL        Installed = FALSE;
/* instance chain, the default boot value is invalid, to catch errors */
static D0399_InstanceData_t *InstanceChainTop = (D0399_InstanceData_t *)0x7fffffff;
#endif

/**************extern from  open.c************************/
#ifdef STTUNER_DRV_SAT_SCR
#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
extern U32 DemodDrvHandleOne;
#endif
#endif

/* For DiSEqC2.0*/
#ifdef STTUNER_DISEQC2_SWDECODE_VIA_PIO	
static U32 IndexforISR;
STTUNER_InstanceDbase_t *InstforISR;
#endif 


/* functions --------------------------------------------------------------- */
#ifndef STTUNER_MINIDRIVER
/* API */
ST_ErrorCode_t demod_d0399_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams);
ST_ErrorCode_t demod_d0399_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams);

ST_ErrorCode_t demod_d0399_Open (ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle);
ST_ErrorCode_t demod_d0399_Close(DEMOD_Handle_t  Handle, DEMOD_CloseParams_t *CloseParams);

ST_ErrorCode_t demod_d0399_IsAnalogCarrier (DEMOD_Handle_t Handle, BOOL *IsAnalog);        
ST_ErrorCode_t demod_d0399_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber);
ST_ErrorCode_t demod_d0399_GetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation);
ST_ErrorCode_t demod_d0399_SetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t  Modulation);
ST_ErrorCode_t demod_d0399_GetAGC          (DEMOD_Handle_t Handle, S16                  *Agc);
ST_ErrorCode_t demod_d0399_GetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t    *FECRates);
ST_ErrorCode_t demod_d0399_GetIQMode       (DEMOD_Handle_t Handle, STTUNER_IQMode_t     *IQMode); /*added for GNBvd26107->I2C failure due to direct access to demod device at API level*/
ST_ErrorCode_t demod_d0399_IsLocked        (DEMOD_Handle_t Handle, BOOL                 *IsLocked);
ST_ErrorCode_t demod_d0399_SetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t     FECRates);
ST_ErrorCode_t demod_d0399_Tracking        (DEMOD_Handle_t Handle, BOOL ForceTracking,   U32 *NewFrequency, BOOL *SignalFound);
ST_ErrorCode_t demod_d0399_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode);

ST_ErrorCode_t demod_d0399_ScanFrequency   (DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset, 
                                                                   U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                   U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                   U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                   U32  FreqOff,          U32   ChannelBW,     S32   EchoPos);

ST_ErrorCode_t demod_d0399_ioctl           (DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status);

/* added for DiSEqC API support*/
ST_ErrorCode_t demod_d0399_DiSEqC       (	DEMOD_Handle_t Handle, 
									    STTUNER_DiSEqCSendPacket_t *pDiSEqCSendPacket,
									    STTUNER_DiSEqCResponsePacket_t *pDiSEqCResponsePacket
									    );

ST_ErrorCode_t demod_d0399_DiSEqCGetConfig ( DEMOD_Handle_t Handle ,STTUNER_DiSEqCConfig_t * DiSEqCConfig);
ST_ErrorCode_t demod_d0399_DiSEqCBurstOFF ( DEMOD_Handle_t Handle );

ST_ErrorCode_t demod_d0399_tonedetection(DEMOD_Handle_t Handle,U32 StartFreq, U32 StopFreq,U8  *NbTones,U32 *ToneList, U8 mode,int* power_detection_level);
/* I/O API */
ST_ErrorCode_t demod_d0399_ioaccess(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle,
                  STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);
U16 addressarray[]=
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
0x001d,
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
0x0048,
0x0049,
0x004a,
0x0080,
0x0081,
0x0082,
0x0083,
0x0084,
0x0085,
0x0086,
0x0087,
0x0088,
0x0089,
0x008a,
0x008b,
0x008c,
0x008d,
0x008e,
0x008f,
0x0090,
0x0091,
0x0092,
0x0093,
0x0094,
0x0095,
0x0096,
0x0097,
};                  



                  
/* local functions --------------------------------------------------------- */
D0399_InstanceData_t *D0399_GetInstFromHandle(DEMOD_Handle_t Handle);
#endif
#ifdef STTUNER_MINIDRIVER
#define MAKEWORD(X,Y)   ((X<<8)+(Y))
D0399_InstanceData_t *DEMODInstance;
#endif
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
static char *FE_399_Errors2String(FE_399_Error_t Error);
#endif
#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0399_Install()

Description:
    install a satellite device driver into the demod database.
    
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0399_Install(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c STTUNER_DRV_DEMOD_STV0399_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s fail driver already installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }
   
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
    STTBX_Print(("%s installing sat:demod:STV0399...", identity));
#endif

    /* mark ID in database */
    Demod->ID = STTUNER_DEMOD_STV0399;

    /* map API */
    Demod->demod_Init = demod_d0399_Init;
    Demod->demod_Term = demod_d0399_Term;

    Demod->demod_Open  = demod_d0399_Open;
    Demod->demod_Close = demod_d0399_Close;

    Demod->demod_IsAnalogCarrier  = demod_d0399_IsAnalogCarrier; 
    Demod->demod_GetSignalQuality = demod_d0399_GetSignalQuality;
    Demod->demod_GetModulation    = demod_d0399_GetModulation;   
    Demod->demod_SetModulation    = demod_d0399_SetModulation;   
    Demod->demod_GetAGC           = demod_d0399_GetAGC;          
    Demod->demod_GetIQMode        = demod_d0399_GetIQMode; /*added for GNBvd26107->I2C failure due to direct access to demod device at API level*/
    Demod->demod_GetFECRates      = demod_d0399_GetFECRates;      
    Demod->demod_IsLocked         = demod_d0399_IsLocked ;   
    Demod->demod_SetFECRates      = demod_d0399_SetFECRates;     
    Demod->demod_Tracking         = demod_d0399_Tracking;        
    Demod->demod_ScanFrequency    = demod_d0399_ScanFrequency;
    Demod->demod_StandByMode      = demod_d0399_StandByMode;
    /*Added for DiSEqC Support*/
	Demod->demod_DiSEqC	  = demod_d0399_DiSEqC;
	Demod->demod_GetConfigDiSEqC   = demod_d0399_DiSEqCGetConfig;  
	Demod->demod_SetDiSEqCBurstOFF   = demod_d0399_DiSEqCBurstOFF;

    Demod->demod_ioaccess = demod_d0399_ioaccess;
    Demod->demod_ioctl    = demod_d0399_ioctl;
    Demod->demod_tonedetection = demod_d0399_tonedetection;

    InstanceChainTop = NULL;
    

   Lock_InitTermOpenClose= STOS_SemaphoreCreateFifo(NULL,1);


    Installed = TRUE;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
    STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DEMOD_STV0399_UnInstall()

Description:
    install a satellite device driver into the demod database.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0399_UnInstall(STTUNER_demod_dbase_t *Demod)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c STTUNER_DRV_DEMOD_STV0399_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }
   
    if(Demod->ID != STTUNER_DEMOD_STV0399)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s fail incorrect driver type\n", identity));
#endif
        return(STTUNER_ERROR_ID);
    }

    /* has all memory been freed, by Term() */
    if(InstanceChainTop != NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s fail at least one instance not terminated\n", identity));
#endif
        return(ST_ERROR_OPEN_HANDLE);
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
    STTBX_Print(("%s uninstalling sat:demod:STV0399...", identity));
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
    Demod->demod_StandByMode      = NULL;
    
	Demod->demod_DiSEqC	          = NULL;
	Demod->demod_GetConfigDiSEqC  = NULL;  
	Demod->demod_SetDiSEqCBurstOFF= NULL;

    Demod->demod_ioaccess = NULL;
    Demod->demod_ioctl    = NULL;
    Demod->demod_tonedetection = NULL;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("<"));
#endif
  
	STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);    


#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print((">"));
#endif

    InstanceChainTop = (D0399_InstanceData_t *)0x7ffffffe;
    Installed        = FALSE;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("ok\n"));
#endif

    return(Error);
}

#endif

/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------
Name: demod_d0399_Init()

Description:
    
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0399_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
    const char *identity = "STTUNER d0399.c demod_d0399_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
#ifndef STTUNER_MINIDRIVER
    D0399_InstanceData_t *InstanceNew, *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
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
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull(DeviceName);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s fail DeviceName not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( D0399_InstanceData_t ));
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
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
    InstanceNew->ExternalClock       = InitParams->ExternalClock;  
    InstanceNew->TSOutputMode        = InitParams->TSOutputMode;
    InstanceNew->SerialClockSource   = InitParams->SerialClockSource;
    InstanceNew->ClockPolarity       = InitParams->ClockPolarity;
    InstanceNew->DataClockAtParityBytes = InitParams->DataClockAtParityBytes;
    InstanceNew->TSDataOutputControl = InitParams->TSDataOutputControl;
    InstanceNew->BlockSyncMode       = InitParams->BlockSyncMode;
    InstanceNew->DataFIFOMode        = InitParams->DataFIFOMode;
    InstanceNew->OutputFIFOConfig    = InitParams->OutputFIFOConfig;
    InstanceNew->SerialDataMode      = InitParams->SerialDataMode;
       
    InstanceNew->FECMode             = InitParams->FECMode;
    InstanceNew->FE_399_Modulation   = FE_MOD_QPSK; /* default to QPSK modulation */

    /* parameters to pass to the CHIP API (I2C name, address etc.) */
    InstanceNew->DeviceMap.MemoryPartition = InitParams->MemoryPartition;
    InstanceNew->DeviceMap.Mode        = IOREG_MODE_SUBADR_8; /* normal <addr><reg No.><data> access */

    InstanceNew->DeviceMap.Registers= STV399_NBREGS; /* number of registers */
    /********Added for DiSEqC***************************/
    InstanceNew->DiSEqCConfig.Command   = STTUNER_DiSEqC_COMMAND;
    InstanceNew->DiSEqCConfig.ToneState = STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
	/**************************************************/
   
     
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes)\n", identity, InstanceNew->DeviceName, (U32)InstanceNew, sizeof( D0399_InstanceData_t ) ));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    
  #endif
/************************************MINIDRIVER******************************
******************************************************************************
********************************************************************************/
#ifdef STTUNER_MINIDRIVER
        #ifdef ST_OS21    
	    Lock_InitTermOpenClose = semaphore_create_fifo(1);
	#else
	    semaphore_init_fifo(&Lock_InitTermOpenClose, 1);
	#endif   
    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    DEMODInstance = memory_allocate_clear(InitParams->MemoryPartition, 1, sizeof( D0399_InstanceData_t ));
    if (DEMODInstance == NULL)
    {
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_NO_MEMORY);           
    }
    DEMODInstance->DeviceName          = DeviceName;
    DEMODInstance->TopLevelHandle      = STTUNER_MAX_HANDLES;
    DEMODInstance->MemoryPartition     = InitParams->MemoryPartition;
    
    DEMODInstance->ExternalClock       = InitParams->ExternalClock;  
    DEMODInstance->TSOutputMode        = InitParams->TSOutputMode;
    DEMODInstance->SerialDataMode      = InitParams->SerialDataMode;
    DEMODInstance->SerialClockSource   = InitParams->SerialClockSource;
    /* add block sync bit control for bug GNBvd27452*/
    #ifdef STTUNER_BLOCK_SYNC_MODE_SELECT
    DEMODInstance->BlockSyncMode       = InitParams->BlockSyncMode;
    DEMODInstance->DataFIFOMode        = InitParams->DataFIFOMode;
    DEMODInstance->OutputFIFOConfig    = InitParams->OutputFIFOConfig;
    #endif
    DEMODInstance->FECMode             = InitParams->FECMode;
    DEMODInstance->FE_399_Modulation   = FE_MOD_QPSK; /* default to QPSK modulation */
    /********Added for DiSEqC***************************/
    DEMODInstance->DiSEqCConfig.Command   = STTUNER_DiSEqC_COMMAND;
    DEMODInstance->DiSEqCConfig.ToneState = STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
	/**************************************************/
    
	#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
	    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes)\n", identity, InstanceNew->DeviceName, (U32)InstanceNew, sizeof( D0399_InstanceData_t ) ));
	#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    
  #endif
  
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: demod_d0399_Term()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0399_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c demod_d0399_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
#ifndef STTUNER_MINIDRIVER
    D0399_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
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
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }


    /* reap next matching DeviceName */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
    STTBX_Print(("%s reap next matching DeviceName[", identity));
#endif

    Instance = InstanceChainTop;
    while(1)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        if ( strcmp( (char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
            STTBX_Print(("<-- ]\n"));
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

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
            STTBX_Print(("%s freed block at 0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
            STTBX_Print(("%s terminated ok\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL) 
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
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
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
     SEM_UNLOCK(Lock_InitTermOpenClose);
#endif
/****************************MINIDRIVER*********************
*************************************************************
**************************************************************/
#ifdef STTUNER_MINIDRIVER
    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);
    memory_deallocate(DEMODInstance->MemoryPartition, DEMODInstance);
    SEM_UNLOCK(Lock_InitTermOpenClose);
    #ifdef ST_OS21
      semaphore_delete(Lock_InitTermOpenClose);
    #else
      semaphore_delete(&Lock_InitTermOpenClose);
    #endif 
#endif
   
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: demod_d0399_Open()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0399_Open(ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c demod_d0399_Open()";
#endif
#ifdef STTUNER_BLOCK_SYNC_MODE_SELECT
   int ENA8_LevelValue;
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
#ifndef STTUNER_MINIDRIVER
    D0399_InstanceData_t *Instance;
    U32 ChipID;
    STTUNER_InstanceDbase_t *Inst;
        
    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
#endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL) 
        {       
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print((" found ok\n"));
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
    STTBX_Print(("%s using block at 0x%08x\n", identity, (U32)Instance));
#endif

    /* check handle IS actually free */
    if (Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s fail driver instance already open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }
    /* else now got pointer to free (and valid) data block */

     ChipID = STTUNER_IOREG_GetRegister( &(Instance->DeviceMap), Instance->IOHandle, R399_ID);


    if ((ChipID & 0xF0) !=  0xB0)  /* 399 ID code 0xB? */
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s fail device not found (0x%02x)\n", identity, ChipID));
#endif
        /* Term LLA */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s FE_399_Term()\n", identity));
#endif
       

        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s device found, release/revision=%u\n", identity, ChipID & 0x0F));
#endif
    }

	Error = STTUNER_IOREG_Reset(&Instance->DeviceMap, Instance->IOHandle, Def399Val, addressarray);
     	if (Error != ST_NO_ERROR)
    	{
		#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
		        STTBX_Print(("%s fail reset device\n", identity));
		#endif
	        SEM_UNLOCK(Lock_InitTermOpenClose);
	        return(Error);           
    	}

     /* Set serial data mode */
    if (Instance->TSOutputMode == STTUNER_TS_MODE_SERIAL)   
    {     
    	Error |= STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F399_OUTRS_PS, 1);
    }
    else
    {
    	Error |= STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F399_OUTRS_PS, 0);
    }
        
    if (Instance->ClockPolarity == STTUNER_DATA_CLOCK_POLARITY_FALLING)    /* Rising Edge */
    {
        Error |= STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F399_CLK_POL, 1);
    }
    else
    {
        Error |= STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F399_CLK_POL, 0);
    }
    
 /* standard used : DVB, DSS ...	*/
    switch(Instance->FECMode)
    {
        case STTUNER_FEC_MODE_DVB:
        case STTUNER_FEC_MODE_DEFAULT:
            STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F399_FECMODE, 0);     
        break;

        case STTUNER_FEC_MODE_DIRECTV:
            STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F399_FECMODE, 4);
            STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F399_MODE_COEF, 1);
            STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F399_DESCRAM, 0);
            
        break;
        
        default:
            STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F399_FECMODE, 0);  
        break;
            
    }
 
 	
    if (Instance->DataClockAtParityBytes == STTUNER_DATACLOCK_NULL_AT_PARITYBYTES)     /* Parity enable changes done for 5528 live decode issue*/
    {
        Error |= STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F399_CLK_CFG, 1);
    }
    else
    {
        Error |= STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F399_CLK_CFG, 0);
    }			
	     
   if(Instance->BlockSyncMode == STTUNER_SYNC_FORCED)
   {
   		Error |= STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F399_MPEG, 1);
   }
   else
   {
   		Error |= STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F399_MPEG, 0);
   }   	
 
   /* Set Block Sync control for bug GNBvd27452. Block Sync bit will set sync byte 0x47 or 0xb8 after every 8th packet */
   
   #ifdef STTUNER_BLOCK_SYNC_MODE_SELECT
   if((Instance->BlockSyncMode == STTUNER_SYNC_NORMAL) || (Instance->BlockSyncMode == STTUNER_SYNC_DEFAULT))
   STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F399_MPEG, 0);
   else
   STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F399_MPEG, 1);
      
   if(Instance->DataFIFOMode == STTUNER_DATAFIFO_ENABLED)
   {
   	if(Instance->OutputFIFOConfig.OutputRateCompensationMode == STTUNER_COMPENSATE_DATACLOCK)
   	{
   		STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F399_EN_STBACKEND,    1);
   	}
   	else if(Instance->OutputFIFOConfig.OutputRateCompensationMode == STTUNER_COMPENSATE_DATAVALID)
   	{
   		STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F399_EN_STBACKEND,    0);
   	}
   	
   	if (Instance->TSOutputMode == STTUNER_TS_MODE_SERIAL)
   	{
   		ENA8_LevelValue = 0x01;/* use only ENA8_LEVEL[3:2] in serial mode*/
   		ENA8_LevelValue |= ((Instance->OutputFIFOConfig.CLOCKPERIOD - 1)<<2);
   		STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F399_ENA8_LEVEL,ENA8_LevelValue);
   	}
   	else if (Instance->TSOutputMode == STTUNER_TS_MODE_PARALLEL)
   	{
   		ENA8_LevelValue = (Instance->OutputFIFOConfig.CLOCKPERIOD) / 4;
   		STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F399_ENA8_LEVEL,ENA8_LevelValue);
   	}
   }
   #endif

    /* ERRCNTL #1: Set default error count mode i.e., QPSK bit error rate */
    STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F399_ERRMODE,    0);   /* error rate   */
    STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F399_ERR_SOURCE, 0);   /* source: QPSK */
    STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F399_NOE,        3);   /* 2^18 bytes   */


    /* Set capabilties */
    Capability->FECAvail        = STTUNER_FEC_1_2 | STTUNER_FEC_2_3 | STTUNER_FEC_3_4 |
                                  STTUNER_FEC_5_6 | STTUNER_FEC_6_7 | STTUNER_FEC_7_8;  /* direct mapping to STTUNER_FECRate_t    */
    Capability->ModulationAvail = STTUNER_MOD_ALL;  /* direct mapping to STTUNER_Modulation_t */
    Capability->AGCControl      = FALSE;
    Capability->SymbolMin       = STV0399E_SYMBOL_RATE_MIN; /*   1 MegaSymbols/sec */
    Capability->SymbolMax       = STV0399E_SYMBOL_RATE_MAX; /*  45 MegaSymbols/sec */
    Capability->FreqMin         = STV0399E_FREQ_MIN;  /* 950000 */
    Capability->FreqMax         = STV0399E_FREQ_MAX;  /* 2150000 */
    Capability->BerMax           = MAX399_BER; 
    Capability->SignalQualityMax = MAX399_SIGNAL_QUALITY;
    Capability->AgcMax           = MAX399_AGC;

    /* change tuner step size to best fixed value for STV0399 */
    Inst = STTUNER_GetDrvInst();
    Inst[OpenParams->TopLevelHandle].Sat.Ioctl.TunerStepSize = STV0399_TUNER_SCAN_STEP;
    #ifdef STTUNER_LNB_POWER_THRU_DEMODIO
    Inst[OpenParams->TopLevelHandle].Sat.Demod.DeviceMap       = &Instance->DeviceMap;
    Inst[OpenParams->TopLevelHandle].Sat.Demod.TEN_FieldIndex  = F399_OP0_1;
    Inst[OpenParams->TopLevelHandle].Sat.Demod.VEN_FieldIndex  = F399_OP1_1;
    Inst[OpenParams->TopLevelHandle].Sat.Demod.VSEL_FieldIndex = F399_LOCK_CONF;  
    #endif 

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

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
    STTBX_Print(("%s opened ok\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    
 
#endif    
/********************************MINIDRIVER********************************
***************************************************************************
*****************************************************************************/
#ifdef STTUNER_MINIDRIVER

	U8 Data;
	#ifdef STTUNER_BLOCK_SYNC_MODE_SELECT
	   int ENA8_LevelValue;
	#endif
    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);
    /* check handle IS actually free */
    if (DEMODInstance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }
    
    /* reset all chip registers */
    Error = STTUNER_IOREG_Reset(&Instance->DeviceMap, Instance->IOHandle, DefVal, addressarray);
     if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0299
        STTBX_Print(("%s fail reset device\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);           
    }
    /*Set OUTRSPS, CLKCFG,FEC settings  */    
    
    switch(DEMODInstance->FECMode)
    {
    	case STTUNER_FEC_MODE_DVB:
        case STTUNER_FEC_MODE_DEFAULT:
        Data = 0;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_FECM, F399_FECMODE, F399_FECMODE_L, &Data, 1, FALSE);
	
	break;
	case STTUNER_FEC_MODE_DIRECTV:
	
	Data = 4;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_FECM, F399_FECMODE, F399_FECMODE_L, &Data, 1, FALSE);
	
	Data = 1;
	/* Nyquist Filter to 20% for DIRECTV */
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_R8PSK, F399_MODE_COEF, F399_MODE_COEF_L, &Data, 1, FALSE);
	 	
	Data = 0;
	/* Descrambler is disactivated */
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_RS, F399_DESCRAM, F399_DESCRAM_L, &Data, 1, FALSE);
	
	break;
	default:
	break;
    }
			
    switch(DEMODInstance->TSOutputMode)
    {
	case STTUNER_TS_MODE_DEFAULT:
        case STTUNER_TS_MODE_PARALLEL:		
	
	Data = 0;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_RS, F399_OUTRS_PS, F399_OUTRS_PS_L, &Data, 1, FALSE);
	break;
	case STTUNER_TS_MODE_SERIAL:
	Data = 1;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_RS, F399_OUTRS_PS, F399_OUTRS_PS_L, &Data, 1, FALSE);
	break;
	default:
	break;
    }
        
    /* Set serial data mode */
    if (DEMODInstance->TSOutputMode == STTUNER_TS_MODE_SERIAL)   
    {     
        if ( ((DEMODInstance->SerialDataMode) & STTUNER_SDAT_VALID_RISING) != 0)    /* Rising edge */
        {
            Data = 0;
            STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_RS, F399_CLK_POL, F399_CLK_POL_L, &Data, 1, FALSE);
        }
        else
        {
            Data = 1;
            STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_RS, F399_CLK_POL, F399_CLK_POL_L, &Data, 1, FALSE);
        }
        
        if ((DEMODInstance->SerialDataMode & STTUNER_SDAT_PARITY_ENABLE) != 0)     /* Parity enable changes done for 5528 live decode issue*/
        {
	        Data = 0;
                STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_RS, F399_CLK_CFG, F399_CLK_CFG_L, &Data, 1, FALSE);
	        
        }
        else
        {
            	Data = 1;
                STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_RS, F399_CLK_CFG, F399_CLK_CFG_L, &Data, 1, FALSE);
        }
    }
 
   /* Set Block Sync control for bug GNBvd27452. Block Sync bit will set sync byte 0x47 or 0xb8 after every 8th packet */
   
   #ifdef STTUNER_BLOCK_SYNC_MODE_SELECT
   if((DEMODInstance->BlockSyncMode == STTUNER_SYNC_NORMAL) || (DEMODInstance->BlockSyncMode == STTUNER_SYNC_DEFAULT))
   {
   	
   	Data = 0;
        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_RS, F399_MPEG, F399_MPEG_L, &Data, 1, FALSE);
   } 
   else
   {
   	Data = 1;
        STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_RS, F399_MPEG, F399_MPEG_L, &Data, 1, FALSE);
   }      
   if(DEMODInstance->DataFIFOMode == STTUNER_DATAFIFO_ENABLED)
   {
   	if(DEMODInstance->OutputFIFOConfig.OutputRateCompensationMode == STTUNER_COMPENSATE_DATACLOCK)
   	{
   		Data = 1;
        	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_RSOUT, F399_EN_STBACKEND, F399_EN_STBACKEND_L, &Data, 1, FALSE);
   		
   	}
   	else if(DEMODInstance->OutputFIFOConfig.OutputRateCompensationMode == STTUNER_COMPENSATE_DATAVALID)
   	{
   		Data = 0;
        	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_RSOUT, F399_EN_STBACKEND, F399_EN_STBACKEND_L, &Data, 1, FALSE);
   	}
   	
   	if (DEMODInstance->TSOutputMode == STTUNER_TS_MODE_SERIAL)
   	{
   		ENA8_LevelValue = 0x01;/* use only ENA8_LEVEL[3:2] in serial mode*/
   		ENA8_LevelValue |= ((DEMODInstance->OutputFIFOConfig.CLOCKPERIOD - 1)<<2);
   		Data = ENA8_LevelValue;
        	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_RSOUT, F399_ENA8_LEVEL, F399_ENA8_LEVEL_L, &Data, 1, FALSE);
   		
   	}
   	else if (DEMODInstance->TSOutputMode == STTUNER_TS_MODE_PARALLEL)
   	{
   		ENA8_LevelValue = (Instance->OutputFIFOConfig.CLOCKPERIOD) / 4;
   		Data = ENA8_LevelValue;
        	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_RSOUT, F399_ENA8_LEVEL, F399_ENA8_LEVEL_L, &Data, 1, FALSE);
   	}
   }
   #endif
    /* ERRCNTL #1: Set default error count mode i.e., QPSK bit error rate */
    Data = 0;
    STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_ERRCTRL, F399_ERRMODE, F399_ERRMODE_L, &Data, 1, FALSE);
    
    Data = 0;
    STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_ERRCTRL, F399_ERR_SOURCE, F399_ERR_SOURCE_L, &Data, 1, FALSE);
    
    Data = 3;
    STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_ERRCTRL, F399_NOE, F399_NOE_L, &Data, 1, FALSE);
    

    Capability->SymbolMin       = STV0399E_SYMBOL_RATE_MIN; /*   1 MegaSymbols/sec */
    Capability->SymbolMax       = STV0399E_SYMBOL_RATE_MAX; /*  45 MegaSymbols/sec */
    Capability->FreqMin         = STV0399E_FREQ_MIN;  /* 950000 */
    Capability->FreqMax         = STV0399E_FREQ_MAX;  /* 2150000 */
    /* change tuner step size to best fixed value for STV0399 */
    Inst = STTUNER_GetDrvInst();
    Inst[OpenParams->TopLevelHandle].Sat.Ioctl.TunerStepSize = STV0399_TUNER_SCAN_STEP;   
    /* finally (as nor more errors to check for, allocate handle */
    DEMODInstance->TopLevelHandle = OpenParams->TopLevelHandle;
    /* in loopthriugh mode the second demod which is connected by loopthrough of first cannot send disecq-st 
       command because of a DC block in the loop-through path. Presently it depends upon toplevelhandle but when tone detection 
       LLA of SatCR will be written "DISECQ_ST_ENABLE" will be initalized automatically depending upon the tone sent*/
    #ifdef STTUNER_DRV_SAT_SCR
		#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
		    if(OpenParams->TopLevelHandle == 0)
		    DEMODInstance->DISECQ_ST_ENABLE = TRUE;
		    if(OpenParams->TopLevelHandle == 1)
		    DEMODInstance->DISECQ_ST_ENABLE = FALSE;
	    #endif
	#endif
   
	 *Handle = (U32)DEMODInstance;

    SEM_UNLOCK(Lock_InitTermOpenClose); 
#endif    
    return(Error);
}
#ifdef STTUNER_MINIDRIVER
ST_ErrorCode_t demod_d0399_Close(DEMOD_Handle_t Handle, DEMOD_CloseParams_t *CloseParams)
{
	DEMODInstance->TopLevelHandle = STTUNER_MAX_HANDLES;
	FE_399_Term();
	return(ST_NO_ERROR);
}
#endif
#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: demod_d0399_Close()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0399_Close(DEMOD_Handle_t Handle, DEMOD_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c demod_d0399_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0399_InstanceData_t     *Instance;

    /* private driver instance data */
    Instance = D0399_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }


    /* ---------- Term LLA ---------- */

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
    STTBX_Print(("%s FE_399_Term() with Instance->FE_399_Handle at 0x%08x\n", identity, Instance->FE_399_Handle));
#endif
       
    /* indidcate insatance is closed */
    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
    STTBX_Print(("%s closed\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}   
#endif
#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: demod_d0399_IsAnalogCarrier()

Description:
    This routine checks for an analog carrier on the current frequency
    by setting the symbol rate to 5M (never a digital signal).

Parameters:
    IsAnalog,   pointer to area to store result:
                TRUE - is analog
                FALSE - is not analog

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0399_IsAnalogCarrier(DEMOD_Handle_t Handle, BOOL *IsAnalog)   /* TODO */
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c demod_d0399_IsAnalogCarrier()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U8 loop,temp[1];
    U16 Agc2;
    D0399_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0399_GetInstFromHandle(Handle);

     Agc2 = 0;
    *IsAnalog = FALSE;  /* Assume no analog carrier */

    /* Set the symbol rate to analog carrier rate 144000000 */
    FE_399_SetSymbolRate(&Instance->DeviceMap, Instance->IOHandle, Instance->ExternalClock, ANALOG_CARRIER_DETECT_SYMBOL_RATE);

    /* Take four AGC2 samples */
    for (loop = 0; loop < 4; loop++)
    {
        /* Read AGC2I1 and AGC2I2 registers */
        STTUNER_IOREG_GetContigousRegisters(&Instance->DeviceMap, Instance->IOHandle, R399_AGC2I1, 2, temp);

        Agc2 += ((temp[0]<<8) + temp[1]);
    } 

    Agc2 = Agc2 / 4;  /* Average AGC2 values */

    /* Test for good signal strength -- centre of analog carrier */
    *IsAnalog = (Agc2 < ANALOG_CARRIER_DETECT_AGC2_VALUE) ? TRUE : FALSE; 

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
    STTBX_Print(("%s IsAnalog=%u\n", identity, *IsAnalog));
#endif

    return(Error);
}   
#endif

/* ----------------------------------------------------------------------------
Name: demod_d0399_GetSignalQuality()

Description:
  Obtains a signal quality setting for the current lock.
    
Parameters:
    SignalQuality_p,    pointer to area to store the signal quality value.
    Ber,                pointer to area to store the bit error rate.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0399_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber)
{
	int i;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c demod_d0399_GetSignalQuality()";
#endif
    D0399_InstanceData_t *Instance;
    
   
    ST_ErrorCode_t Error = ST_NO_ERROR;
    Instance = D0399_GetInstFromHandle(Handle);
  #ifndef STTUNER_MINIDRIVER
    
	*Ber = FE_399_GetError(&Instance->DeviceMap, Instance->IOHandle);
    	*SignalQuality_p = FE_399_GetCarrierToNoiseRatio(&Instance->DeviceMap, Instance->IOHandle); 
    /**SignalQuality_p = Info.C_N;*/
   
    for(i=0; i<20; ++i)
    {
	    if(FE_399_SignalQuality_LookUp[i][0] >= *SignalQuality_p)
	    {
		    *SignalQuality_p = FE_399_SignalQuality_LookUp[i][1];
		    break;
	    }
    }
	    
  
    
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
    STTBX_Print(("%s SignalQuality=%u Ber=%u\n", identity, *SignalQuality_p, *Ber));
#endif
    #endif
/**************************STTUNER_MINIDRIVER**************************
************************************************************************
*************************************************************************/
  #ifdef STTUNER_MINIDRIVER
    Error = FE_399_GetSignalInfo(&Info);
    if ( Error != ST_NO_ERROR)
    {
        return(Error);
    }
    for(i=0; i<20; ++i)
    {
	    if(FE_399_SignalQuality_LookUp[i][0] >= Info.C_N)
	    {
		    *SignalQuality_p = FE_399_SignalQuality_LookUp[i][1];
		    break;
	    }
    }	    
    *Ber = Info.BER;
  #endif
    
    return(Error);
}   
#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: demod_d0399_GetModulation()

Description:
    This routine returns the modulation scheme in use by this device.
    
Parameters:
    Modulation, pointer to area to store modulation scheme.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0399_GetModulation(DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c demod_d0399_GetModulation()";
#endif
    FE_399_Error_t Error=0;
    D0399_InstanceData_t *Instance;
    Instance = D0399_GetInstFromHandle(Handle);
    *Modulation = STTUNER_MOD_QPSK;
           
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
    STTBX_Print(("%s Modulation=%u\n", identity, *Modulation));
#endif
    return(Error);
}   
#endif
#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: demod_d0399_SetModulation()

Description:
    This routine sets the modulation scheme for use when scanning.
    
Parameters:
    Modulation.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0399_SetModulation(DEMOD_Handle_t Handle, STTUNER_Modulation_t Modulation)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c demod_d0399_SetModulation()";
#endif
    D0399_InstanceData_t *Instance;

    Instance = D0399_GetInstFromHandle(Handle);

    switch(Modulation)
    {
        case STTUNER_MOD_BPSK:
            Instance->FE_399_Modulation = FE_MOD_BPSK;
            break;

        case STTUNER_MOD_QPSK:
            Instance->FE_399_Modulation = FE_MOD_QPSK;
            break;

        case STTUNER_MOD_8PSK:
            Instance->FE_399_Modulation = FE_MOD_8PSK;
            break;

        default:
            return(ST_ERROR_BAD_PARAMETER);
    }

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
    STTBX_Print(("%s Modulation=%u\n", identity, Modulation));
#endif
    return(ST_NO_ERROR);
}   
#endif
#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: demod_d0399_GetAGC()

Description:
    Obtains the current RF signal power.

Parameters:
    AGC,    pointer to area to store power output value.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0399_GetAGC(DEMOD_Handle_t Handle, S16  *Agc)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c demod_d0399_GetAGC()";
#endif
    FE_399_Error_t Error=0;
    D0399_InstanceData_t *Instance;
    Instance = D0399_GetInstFromHandle(Handle);
   

    *Agc = FE_399_GetRFLevel(&Instance->DeviceMap,Instance->IOHandle);  /* Power of the RF signal (dBm) */

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
    STTBX_Print(("%s Agc=%u\n", identity, *Agc));
#endif
    return(Error);
}   
#endif
/* ----------------------------------------------------------------------------
Name: demod_d0399_GetFECRates()

Description:
     Checks the VEN rate register to deduce the forward error correction
    setting that is currently in use.
   
Parameters:
    FECRates,   pointer to area to store FEC rates in use.
  
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0399_GetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t *FECRates)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c demod_d0399_GetFECRates()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_FECRate_t CurFecRate = 0;
    U8 Data;
     D0399_InstanceData_t *Instance;
    Instance = D0399_GetInstFromHandle(Handle);
    #ifndef STTUNER_MINIDRIVER
    Data = STTUNER_IOREG_GetField(&Instance->DeviceMap, Instance->IOHandle, F399_PR);
    #endif
    #ifdef STTUNER_MINIDRIVER
    Error = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_VSTATUS, F399_PR, F399_PR_L, &Data,1, FALSE);
    #endif
    /* Convert 399 value to a STTUNER fecrate */
    switch (Data)
    {
        case STV0399_VSTATUS_PR_1_2:
            CurFecRate = STTUNER_FEC_1_2;
            break;

        case STV0399_VSTATUS_PR_2_3:
            CurFecRate = STTUNER_FEC_2_3;
            break;

        case STV0399_VSTATUS_PR_3_4:
            CurFecRate = STTUNER_FEC_3_4;
            break;

        case STV0399_VSTATUS_PR_5_6:
            CurFecRate = STTUNER_FEC_5_6;
            break;

        case STV0399_VSTATUS_PR_6_7:
            CurFecRate = STTUNER_FEC_6_7;
            break;

        case STV0399_VSTATUS_PR_7_8:
            CurFecRate = STTUNER_FEC_7_8;
            break;

        case STV0399_VSTATUS_PR_RE1:
            CurFecRate = STTUNER_FEC_NONE;
            break;

        case STV0399_VSTATUS_PR_RE2:
            CurFecRate = STTUNER_FEC_NONE;
            break;
    }

    *FECRates = CurFecRate;  /* Copy back for caller */
    
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
    STTBX_Print(("%s FECRate=%u\n", identity, CurFecRate));
#endif
    return(Error);
}   

/* ----------------------------------------------------------------------------
Name: demod_d0399_GetIQMode()
Function added for GNBvd26107->I2C failure due to direct access to demod device at API level
(Problem:STTUNER_GetTunerInfo was calling STTUNER_SAT_GetTunerInfo, which was reading 
STTUNER_IOCTL_IQMODE directly. This was causing I2C failure due to simultaneous I2C access
to the same device.
Resolution:This functions reads the value for current IQMode from the register which is updated
in the CurrentTunerInfo structure. So now in STTUNER_SAT_GetTunerInfo() (in get.c) no separate
I2C access (due to Ioctl) is required for retrieving the IQ mode )

Description:
     Retrieves the IQMode from the FECM register(NORMAL or INVERTED)
   
Parameters In:
    IQMode, pointer to area to store the IQMode in use.

Parameters Out:
    IQMode
  
Return Value:
    Error
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0399_GetIQMode(DEMOD_Handle_t Handle, STTUNER_IQMode_t *IQMode)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c demod_d0399_GetIQMode()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0399_InstanceData_t *Instance;
    Instance = D0399_GetInstFromHandle(Handle);
    #ifndef STTUNER_MINIDRIVER
    if(STTUNER_IOREG_GetField(&Instance->DeviceMap, Instance->IOHandle, F399_SYM)==1)
            *IQMode = STTUNER_IQ_MODE_INVERTED;
    else
            *IQMode = STTUNER_IQ_MODE_NORMAL;
    #endif
    #ifdef STTUNER_MINIDRIVER
    U8 Data;
    Error = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_FECM, F399_SYM, F399_SYM_L, &Data,1, FALSE);
    if(Data ==1)
            *IQMode = STTUNER_IQ_MODE_INVERTED;
    else
            *IQMode = STTUNER_IQ_MODE_NORMAL;
    #endif       
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
    STTBX_Print(("%s IQMode=%u\n", identity, *IQMode));
#endif
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: demod_d0399_IsLocked()

Description:
     Checks the LK register i.e., are we demodulating a digital carrier.
   
Parameters:
    IsLocked,   pointer to area to store result (bool):
                TRUE -- we are locked.
                FALSE -- no lock.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0399_IsLocked(DEMOD_Handle_t Handle, BOOL *IsLocked)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c demod_d0399_IsLocked()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    #ifdef STTUNER_MINIDRIVER
    FE_399_SignalInfo_t Info;
    #endif
    D0399_InstanceData_t *Instance;
    Instance = D0399_GetInstFromHandle(Handle);
    #ifndef STTUNER_MINIDRIVER
    *IsLocked = STTUNER_IOREG_GetField(&Instance->DeviceMap, Instance->IOHandle,F399_LK);
    #endif
    #ifdef STTUNER_MINIDRIVER
    Error = FE_399_GetSignalInfo(&Info);
     *IsLocked = Info.Locked;    /* Transponder locked */
    #endif
   

    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: demod_d0399_SetFECRates()

Description:
    Sets the FEC rates to be used during demodulation.
    Parameters:
    
Parameters:
      FECRates, bitmask of FEC rates to be applied.
  
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0399_SetFECRates(DEMOD_Handle_t Handle, STTUNER_FECRate_t FECRates)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c demod_d0399_SetFECRates()";
#endif
   ST_ErrorCode_t Error = ST_NO_ERROR;
    U8 Data = 0;
    D0399_InstanceData_t *Instance;
    Instance = D0399_GetInstFromHandle(Handle);
    /* Convert STTUNER FEC rates (see sttuner.h) to a VENRATE value to be applied to the STV0399 */
    if (FECRates & STTUNER_FEC_1_2) Data |= STV0399_PR_MASK_1_2;
    if (FECRates & STTUNER_FEC_2_3) Data |= STV0399_PR_MASK_2_3;
    if (FECRates & STTUNER_FEC_3_4) Data |= STV0399_PR_MASK_3_4;
    if (FECRates & STTUNER_FEC_5_6) Data |= STV0399_PR_MASK_5_6;
    if (FECRates & STTUNER_FEC_6_7) Data |= STV0399_PR_MASK_6_7;
    if (FECRates & STTUNER_FEC_7_8) Data |= STV0399_PR_MASK_7_8;
    #ifndef STTUNER_MINIDRIVER
    Error = STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle, R399_PR, Data);
    #endif
    #ifdef STTUNER_MINIDRIVER
    STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_PR, 0, 0, &Data, 1, FALSE);
    #endif
    if ( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s fail ChipSetField()\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s Field F0399_RATE = 0x%02x\n", identity, Data));
#endif
    }

    return(Error);
}   
#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: demod_d0399_Tracking()

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
ST_ErrorCode_t demod_d0399_Tracking(DEMOD_Handle_t Handle, BOOL ForceTracking, U32 *NewFrequency, BOOL *SignalFound)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c demod_d0399_Tracking()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0399_InstanceData_t *Instance;
    FE_399_SignalInfo_t pInfo;
    STTUNER_InstanceDbase_t  *Inst;
    Inst = STTUNER_GetDrvInst();
    Instance = D0399_GetInstFromHandle(Handle);
    Error = demod_d0399_IsLocked(Handle, SignalFound);
    if((Error == ST_NO_ERROR) && (*SignalFound == TRUE))
    {
		
		FE_399_GetSignalInfo(&Instance->DeviceMap, Instance->IOHandle, &pInfo);
	                
        	
		    *NewFrequency = pInfo.Frequency;
		    Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.SymbolRate = pInfo.SymbolRate;
		    Inst[Instance->TopLevelHandle].CurrentTunerInfo.BitErrorRate = pInfo.BER;
		    Inst[Instance->TopLevelHandle].CurrentTunerInfo.SignalQuality = pInfo.C_N;
		    Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.AGC = pInfo.Power;
		    demod_d0399_GetFECRates(Handle,&Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.FECRates);
		    Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.IQMode=STTUNER_IOREG_GetField(&Instance->DeviceMap, Instance->IOHandle,F399_SYM);
		      	    
       }
    
    #ifdef STTUNER_DRV_SAT_SCR
    if ((Inst[Instance->TopLevelHandle].Capability.SCREnable)&& (Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.LNB_SignalRouting == STTUNER_VIA_SCRENABLED_LNB))
    {
    	*NewFrequency = Inst[Instance->TopLevelHandle].ScanList.ScanList->ScrParams.SCRVCOFrequency - pInfo.Frequency;
    }
    else
    {
    	*NewFrequency = pInfo.Frequency;
    }
    #else
    *NewFrequency = pInfo.Frequency;
    #endif
        
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
    STTBX_Print(("%s SignalFound=%u NewFrequency=%u\n", identity, *SignalFound, *NewFrequency));
#endif
	   return(Error);
}   
#endif
/* ----------------------------------------------------------------------------
Name: demod_d0399_ScanFrequency()

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
    ScanSuccess,      boolean that indicates search success.
    NewFrequency,     pointer to area to store locked frequency.
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0399_ScanFrequency(DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset, 
                                                                U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                U32  FreqOff,          U32   ChannelBW,     S32   EchoPos)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c demod_d0399_ScanFrequency()";
#endif
    FE_399_Error_t error= FE_399_NO_ERROR;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FE_399_SearchParams_t Params;
    FE_399_SearchResult_t Result;
    STTUNER_InstanceDbase_t  *Inst;
    FE_399_SIGNALTYPE_t signalType;
    int noise=0, index;
    U8 temp[1];
    #ifdef STTUNER_DRV_SAT_SCR
    	U32 SCRVCOFrequency, TunerFrequency;
    #endif
     
  #ifndef STTUNER_MINIDRIVER
        D0399_InstanceData_t *Instance;
    	/* private driver instance data */
    	Instance = D0399_GetInstFromHandle(Handle);
	Inst = STTUNER_GetDrvInst();
	Params.Frequency   = InitialFrequency;              /* demodulator (output from LNB) frequency (in KHz) */
	Params.SymbolRate  = SymbolRate;                    /* transponder symbol rate  (in bds) */
	
	if (!Inst[Instance->TopLevelHandle].Sat.ScanExact)
    	{
    		Params.SearchRange =(( Inst[Instance->TopLevelHandle].Sat.SymbolWidthMin)*1000);                       /* range of the search; 10MHz (in Hz) (10,000,000) or MaxLNBOffset (14,850,000) */
    	}
    	else
    	Params.SearchRange = 10000000;
    	
    	Params.Modulation  = Instance->FE_399_Modulation;       /* previously setup modulation */
	Params.DemodIQMode = (STTUNER_IQMode_t) Spectrum;
	
	/* Make VCO on before scanning -> GNBvd27468*/
	Error  = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,F399_RESET_SYNT,0);
	Error |= STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,F399_RESET_PLL,0);
	#ifdef STTUNER_DRV_SAT_SCR
	if((Inst[Instance->TopLevelHandle].Capability.SCREnable)&& (Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.LNB_SignalRouting == STTUNER_VIA_SCRENABLED_LNB))
        {
		STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,F399_IAGC1R,0);
		STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,F399_AGC1R_REF,0x14);
		Error = scr_scrdrv_SetFrequency(Instance->TopLevelHandle, Handle, Instance->DeviceName, InitialFrequency/1000, Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->ScrParams.LNBIndex,Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->ScrParams.SCRBPF );
		TunerFrequency = (Inst[Instance->TopLevelHandle].Capability.SCRBPFFrequencies[Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->ScrParams.SCRBPF])*1000;
		Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->ScrParams.BPFFrequency = TunerFrequency;
		SCRVCOFrequency = TunerFrequency + InitialFrequency;
		Params.Frequency = TunerFrequency;
		Inst[Instance->TopLevelHandle].Sat.ScanInfo.Scan->ScrParams.SCRVCOFrequency = SCRVCOFrequency;
	}
        #endif
	
	while(Inst[Instance->TopLevelHandle].CurrentTunerInfo.ScanInfo.DishPositioning_ToneOnOff == TRUE)
	{
		error = FE_399_Search( &Instance->DeviceMap, Instance->IOHandle, &Params, &Result );
		
		signalType = Params.State;
		
		switch(signalType)
		{
		case NOAGC1_399E:
		case AGC1OK_399E:
		case NOAGC2_399E:
		case AGC2OK_399E:
		case NOTIMING_399E:
		case ANALOGCARRIER_399E:
		case NOCARRIER_399E:
		
		/* Slow Bip */
		STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle,R399_ACR,0xA0);
		WAIT_N_MS_399(175);
		STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle,R399_ACR,0x00);
		WAIT_N_MS_399(25);
		break;
		
		case TIMINGOK_399E:
		case CARRIEROK_399E:
		case NODATA_399E:
		case FALSELOCK_399E:
		
		/* Fast Bip */
		/* Slow Bip */
		STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle,R399_ACR,0xA0);
		WAIT_N_MS_399(50);
		STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle,R399_ACR,0x00);
		WAIT_N_MS_399(50);
		STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle,R399_ACR,0xAF);
		WAIT_N_MS_399(50);
		STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle,R399_ACR,0x00);
		WAIT_N_MS_399(50);
		break;
		
		case DATAOK_399E:
		case OUTOFRANGE_399E:
		case RANGEOK_399E:
		
		/* Continious tone proportional to C/N */
		for(index=0;index<10;index++)
		{
		STTUNER_IOREG_GetContigousRegisters(&Instance->DeviceMap, Instance->IOHandle,R399_NIRM,2,temp);
		noise += MAKEWORD(temp[0],temp[0]);
		
		}
		
		noise /= 10;
		
		if(noise >= 10240)
		noise = 10240;
		else if(noise < 2048)
		noise = 2048;
		
		noise -= 2048;
				
		STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle,R399_ACR,0x40+((noise & 0x1FC0) >> 6));
		break;
		}
	}
			
	STTUNER_IOREG_SetRegister(&Instance->DeviceMap, Instance->IOHandle,R399_ACR,0x00);
 
  	error = FE_399_Search( &Instance->DeviceMap, Instance->IOHandle, &Params, &Result );

    if (error == FE_399_BAD_PARAMETER)  /* element(s) of Params bad */
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s fail, scan not done: bad parameter(s) to FE_399_Search() == FE_BAD_PARAMETER\n", identity ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (error == FE_399_SEARCH_FAILED)  /* found no signal within limits */
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s FE_399_Search() == FE_SEARCH_FAILED\n", identity ));
#endif
        return(ST_NO_ERROR);    /* no error so try next F or stop if band limits reached */
    }

    *ScanSuccess = Result.Locked;

    if (Result.Locked == TRUE)
    {
        
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

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s NewFrequency=%u\t", identity, *NewFrequency));
        STTBX_Print(("%s ScanSuccess=%u\n",  identity, *ScanSuccess));
#endif
    }

  #endif
  /********************************MINIDRIVER****************************************
  ************************************************************************************
  *************************************************************************************/
    #ifdef STTUNER_MINIDRIVER
    	U8 Data, nsbuffer[2];
    	#ifdef STTUNER_DRV_SAT_SCR
    	SCR_OpenParams_t  OpenParams;
    	#endif
    	Inst = STTUNER_GetDrvInst();
	Params.Frequency   = InitialFrequency;              /* demodulator (output from LNB) frequency (in KHz) */
	Params.SymbolRate  = SymbolRate;                    /* transponder symbol rate  (in bds) */
	if (!Inst[DEMODInstance->TopLevelHandle].Sat.ScanExact)
    	{
    		Params.SearchRange =(( Inst[DEMODInstance->TopLevelHandle].Sat.SymbolWidthMin)*1000);                       /* range of the search; 10MHz (in Hz) (10,000,000) or MaxLNBOffset (14,850,000) */
    	}
    	else
    	Params.SearchRange = 10000000;
    	
    	Params.Modulation  = DEMODInstance->FE_399_Modulation;       /* previously setup modulation */
	Params.DemodIQMode = (STTUNER_IQMode_t) Spectrum;
	
	/* Set SCR frequency here */
    #ifdef STTUNER_DRV_SAT_SCR
		OpenParams.TopLevelHandle = DEMODInstance->TopLevelHandle;
		Inst[DEMODInstance->TopLevelHandle].ScanList.ScanList->ScrParams.LNBIndex=Inst[DEMODInstance->TopLevelHandle].CurrentTunerInfo.ScanInfo.ScrParams.LNBIndex;
		Inst[DEMODInstance->TopLevelHandle].ScanList.ScanList->ScrParams.SCRBPF=Inst[DEMODInstance->TopLevelHandle].CurrentTunerInfo.ScanInfo.ScrParams.SCRBPF;
		Error = scr_scrdrv_SetFrequency(DEMODInstance->TopLevelHandle, Handle, DEMODInstance->DeviceName, InitialFrequency/1000, Inst[DEMODInstance->TopLevelHandle].ScanList.ScanList->ScrParams.LNBIndex,Inst[DEMODInstance->TopLevelHandle].ScanList.ScanList->ScrParams.SCRBPF );
		Error = scr_scrdrv_SetFrequency(DEMODInstance->TopLevelHandle, Handle, DEMODInstance->DeviceName, InitialFrequency/1000, Inst[DEMODInstance->TopLevelHandle].ScanList.ScanList->ScrParams.LNBIndex,Inst[DEMODInstance->TopLevelHandle].ScanList.ScanList->ScrParams.SCRBPF );
	    	TunerFrequency = Inst[DEMODInstance->TopLevelHandle].ScanList.ScanList->ScrParams.SCRVCOFrequency - Params.Frequency;
	    
	    if(Inst[DEMODInstance->TopLevelHandle].ScanList.ScanList->ScrParams.SCRBPF == STTUNER_SCRBPF0)
	    {
		    Inst[DEMODInstance->TopLevelHandle].ScanList.ScanList->ScrParams.BPFFrequency = BAND1_FREQUENCY;
	    }
	    else if(Inst[DEMODInstance->TopLevelHandle].ScanList.ScanList->ScrParams.SCRBPF == STTUNER_SCRBPF1)
	    {
		    Inst[DEMODInstance->TopLevelHandle].ScanList.ScanList->ScrParams.BPFFrequency = BAND2_FREQUENCY;
	    }
	    else if(Inst[DEMODInstance->TopLevelHandle].ScanList.ScanList->ScrParams.SCRBPF == STTUNER_SCRBPF2)
	    {
		    Inst[DEMODInstance->TopLevelHandle].ScanList.ScanList->ScrParams.BPFFrequency = BAND3_FREQUENCY;
	    }
	    else if(Inst[DEMODInstance->TopLevelHandle].ScanList.ScanList->ScrParams.SCRBPF == STTUNER_SCRBPF3)
	    {
		    Inst[DEMODInstance->TopLevelHandle].ScanList.ScanList->ScrParams.BPFFrequency = BAND4_FREQUENCY;
	    }
	    
	    SCRVCOFrequency = Inst[DEMODInstance->TopLevelHandle].ScanList.ScanList->ScrParams.SCRVCOFrequency;
	    
	    Params.Frequency = TunerFrequency;
    #endif
    
    		while(Inst[DEMODInstance->TopLevelHandle].CurrentTunerInfo.ScanInfo.DishPositioning_ToneOnOff == TRUE)
		{
			error = FE_399_Search(&Params, &Result );
			
			signalType = Params.State;
			
			switch(signalType)
			{
			case NOAGC1_399E:
			case AGC1OK_399E:
			case NOAGC2_399E:
			case AGC2OK_399E:
			case NOTIMING_399E:
			case ANALOGCARRIER_399E:
			case NOCARRIER_399E:
			
			/* Slow Bip */
			Data = 0xA0;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_ACR, 0, 0, &Data, 1, FALSE);
			
			WAIT_N_MS_399(175);
			Data = 0x00;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_ACR, 0, 0, &Data, 1, FALSE);
			WAIT_N_MS_399(25);
			break;
			
			case TIMINGOK_399E:
			case CARRIEROK_399E:
			case NODATA_399E:
			case FALSELOCK_399E:
			
			/* Fast Bip */
			/* Slow Bip */
			Data = 0xA0;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_ACR, 0, 0, &Data, 1, FALSE);
			WAIT_N_MS_399(50);
			Data = 0x00;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_ACR, 0, 0, &Data, 1, FALSE);
			WAIT_N_MS_399(50);
			Data = 0xAF;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_ACR, 0, 0, &Data, 1, FALSE);
			WAIT_N_MS_399(50);
			Data = 0x00;
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_ACR, 0, 0, &Data, 1, FALSE);
			WAIT_N_MS_399(50);
			break;
			
			case DATAOK_399E:
			case OUTOFRANGE_399E:
			case RANGEOK_399E:
			
			/* Continious tone proportional to C/N */
			for(index=0;index<10;index++)
			{
			
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_NIRM, 0, 0, nsbuffer, 2, FALSE);
			noise += MAKEWORD(nsbuffer[0],nsbuffer[1]);
			
			}
			
			noise /= 10;
			
			if(noise >= 10240)
			noise = 10240;
			else if(noise < 2048)
			noise = 2048;
			
			noise -= 2048;
			Data = 0x40+((noise & 0x1FC0) >> 6);
			STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_ACR, 0, 0, &Data, 1, FALSE);		
			
			break;
			}
		}
				
		Data = 0x00;
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_ACR, 0, 0, &Data, 1, FALSE);
         
  	error = FE_399_Search( &Params, &Result );

    if (error == FE_BAD_PARAMETER)  /* element(s) of Params bad */
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s fail, scan not done: bad parameter(s) to FE_399_Search() == FE_BAD_PARAMETER\n", identity ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (error == FE_SEARCH_FAILED)  /* found no signal within limits */
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s FE_399_Search() == FE_SEARCH_FAILED\n", identity ));
#endif
        return(ST_NO_ERROR);    /* no error so try next F or stop if band limits reached */
    }

    *ScanSuccess = Result.Locked;

    if (Result.Locked == TRUE)
    {
        *NewFrequency = Result.Frequency;
        #ifdef STTUNER_DRV_SAT_SCR
        *NewFrequency = (SCRVCOFrequency - TunerFrequency) + (TunerFrequency - Result.Frequency);
        #endif
        Error = 0;
    }
 #endif
    
    return(Error);
}   

/* ----------------------------------------------------------------------------
Name: demod_d0399_DiSEqC
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

ST_ErrorCode_t demod_d0399_DiSEqC(DEMOD_Handle_t Handle, STTUNER_DiSEqCSendPacket_t *pDiSEqCSendPacket, STTUNER_DiSEqCResponsePacket_t *pDiSEqCResponsePacket )
{	
	int Error = ST_NO_ERROR;
	unsigned char No_of_Bytes, *pCurrent_Data;	
	unsigned int TimeoutCounter =0 ;
	U32 Delay=0; 
	U32 DigFreq, F22_Freq;
	U8 F22;
#ifdef STTUNER_DISEQC2_SWDECODE_VIA_PIO	
	U8 NumofBytes =0,BitPattern[MAX_NUM_BITS_IN_REPLY];
	U8 DecodedOneByte,i,j,ParityCounter;
        U32 DelayCount = 0;
#endif 

#ifdef STTUNER_DRV_SAT_LNBH21
#ifndef STTUNER_MINIDRIVER
	LNB_Handle_t LHandle;
#endif
#endif

#ifndef STTUNER_MINIDRIVER
STTUNER_InstanceDbase_t *Inst;
#endif


    #ifndef STTUNER_MINIDRIVER
	D0399_InstanceData_t *Instance;
	/* If framing byte wants response (DiSEqC 2.0) report error*/
	pCurrent_Data = pDiSEqCSendPacket->pFrmAddCmdData;

	if (( *pCurrent_Data == FB_COMMAND_REPLY)|
			(*(pDiSEqCSendPacket->pFrmAddCmdData) == FB_COMMAND_REPLY_REPEATED))
		{
#ifndef STTUNER_DISEQC2_SWDECODE_VIA_PIO	/* Check the device capability */
			Error = ST_ERROR_FEATURE_NOT_SUPPORTED; /*Not supported for EVAL 399 board*/	
			return (Error);
#endif 
		}
	
	Instance = D0399_GetInstFromHandle(Handle);
	
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
	
	
#ifdef STTUNER_DISEQC2_SWDECODE_VIA_PIO
	/* Get the top level handle */
	IndexforISR = Instance->TopLevelHandle;
	/* Get the instance database */
	InstforISR = STTUNER_GetDrvInst();	
#endif  
	
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
	
	DigFreq = FE_399_GetMclkFreq(&Instance->DeviceMap, Instance->IOHandle,27000000);     /*27000000->Quartz Frequency */
	F22 = STTUNER_IOREG_GetField(&Instance->DeviceMap, Instance->IOHandle, F399_F_REG);
	F22_Freq = FE_399_F22Freq(DigFreq,F22);
	if(F22_Freq<= 21800 || F22_Freq>=22200)
	{
		F22 = DigFreq/(32*22000);
		STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F399_F_REG, F22);
		STOS_TaskDelayUs(2000 );
		
	}
		
	/* wait for  FIFO empty to ensure previous execution is complete*/
	while(!(STTUNER_IOREG_GetField(&Instance->DeviceMap, Instance->IOHandle, F399_FE)))	
			  			
		{
				STTUNER_TaskDelay(50);
				if ((++TimeoutCounter)>0xfff)
						{ return(ST_ERROR_TIMEOUT);
						}
	     
				TimeoutCounter =0;
		}

	
	Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F399_TIM_OFF, 0x01);

	switch (pDiSEqCSendPacket->DiSEqCCommandType)
		
		{ 
 		case	STTUNER_DiSEqC_TONE_BURST_OFF:   /*Generic same as F22_low */
					
		case	STTUNER_DiSEqC_TONE_BURST_OFF_F22_LOW:  		/*DiSEqC pin low,Tone off*/
				
				/*DiSEqC(1:0)=00  DiSEqC(2)=X DiSEqC FIFO=empty */
				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F399_DISEQCMODE, 0x00);
				
				if(pDiSEqCSendPacket->DiSEqCCommandType==STTUNER_DiSEqC_TONE_BURST_OFF)
                Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_OFF ;
				else
		        Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_OFF_F22_LOW ;
                Instance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
				

				break;
	
		case	STTUNER_DiSEqC_TONE_BURST_OFF_F22_HIGH:			/*DiSEqC pin high, Tone off */
				
				/*DiSEqC(1:0)=01  DiSEqC(2)=X DiSEqC FIFO=empty */
				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F399_DISEQCMODE, 0x01);
				
				Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_OFF_F22_HIGH ;
                Instance->DiSEqCConfig.ToneState= STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
							
				break;

		case	STTUNER_DiSEqC_CONTINUOUS_TONE_BURST_ON:  		/* unmodulated */
				
				/*DiSEqC(1:0)=11  DiSEqC(2)=X DiSEqC FIFO=XX  */
				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F399_DISEQCMODE, 0x03);
				
				Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_CONTINUOUS_TONE_BURST_ON;
                Instance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS;
						
				break;

		case	STTUNER_DiSEqC_TONE_BURST_SEND_0_UNMODULATED:   /*send of 0 for 12.5 ms ;continuous tone*/
				
				/*DiSEqC(1:0)=10  DiSEqC(2)=0 DiSEqC FIFO=00 */
				
				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F399_DISEQCMODE, 0x02);
				if (Error !=ST_NO_ERROR){break;}

				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F399_DISEQC, 0x00);
				if (Error !=ST_NO_ERROR){break;}
				
				while(STTUNER_IOREG_GetField(&Instance->DeviceMap, Instance->IOHandle, F399_FF))		/* wait for Place in FIFO */
			  			
						{
						STTUNER_TaskDelay(50);
						if ((++TimeoutCounter)>0x1ff)
							{ return(ST_ERROR_TIMEOUT);
							}
	     
						TimeoutCounter =0;
					 	}
				
				STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,F399_DISEQCFIFO1,0x00);
				Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_SEND_0_UNMODULATED;
                Instance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
											
				break;

		case	STTUNER_DiSEqC_TONE_BURST_SEND_0_MODULATED:     /*0-2/3 duty cycle tone*/
				
				/*DiSEqC(1:0)=10  DiSEqC(2)=1 DiSEqC FIFO=00 */
				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F399_DISEQCMODE, 0x02);
				if (Error !=ST_NO_ERROR){break;}
				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F399_DISEQC, 0x01);
				if (Error !=ST_NO_ERROR){break;}
			    
				
				STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,F399_DISEQCFIFO1,0x00);
				Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_SEND_0_MODULATED;
                Instance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
				
				
				break;

		case	STTUNER_DiSEqC_TONE_BURST_SEND_1: 				/*1-1/3 duty cycle tone modulated*/
				
				/*DiSEqC(1:0)=10  DiSEqC(2)=1 DiSEqC FIFO=FF */
				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F399_DISEQCMODE, 0x02);
				if (Error !=ST_NO_ERROR){break;}
				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F399_DISEQC, 0x01);
				if (Error !=ST_NO_ERROR){break;}
				
					
				
				STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,F399_DISEQCFIFO1,0xff);
				Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_SEND_1;
                Instance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
				
				
				break;

		case	STTUNER_DiSEqC_COMMAND:							/*DiSEqC (1.2/2)command */
				
				if(STTUNER_IOREG_GetField(&Instance->DeviceMap, Instance->IOHandle,F399_OP0_1))					/* If 22 KHz tone is ON		*/
			STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle,F399_OP0_1,0);	
				/*DiSEqC(1:0)=10  DiSEqC(2)=1 DiSEqC FIFO=data */
				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F399_DISEQCMODE, 0x02);
				if (Error !=ST_NO_ERROR){break;}
				
				Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F399_DISEQC, 0x01);
				if (Error !=ST_NO_ERROR){break;}
				
			                            		
				for(No_of_Bytes = pDiSEqCSendPacket->uc_TotalNoOfBytes; No_of_Bytes>0; No_of_Bytes--)
					{
						while(STTUNER_IOREG_GetField(&Instance->DeviceMap, Instance->IOHandle, F399_FF))		/* wait for Place in FIFO */
			  			
							{
								STTUNER_TaskDelay(50);
							    if ((++TimeoutCounter)>0x1ff)
								{  
									return(ST_ERROR_TIMEOUT);
								}
	     
							    TimeoutCounter =0;
					 		}
						Error =  STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle, R399_DISEQCFIFO,
																			*pCurrent_Data);
						pCurrent_Data++;
				
					}
				
				Instance->DiSEqCConfig.Command=STTUNER_DiSEqC_COMMAND;
                Instance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
                /* Make Delay value to 135 */
                 Delay = 135;
				
				break;
		
		default:
				Error=ST_ERROR_BAD_PARAMETER;
	
	
		}/*end of Switch*/														
	
	if (Error !=ST_NO_ERROR)
		{	
			return(Error);
		}
/* Wait for the DiSEqC command to be sent completely --(TicksPermilisec *13.5* Numberofbytes sent)*/
	 DiSEqC_Delay_ms (((Delay*pDiSEqCSendPacket->uc_TotalNoOfBytes)/10));	
 #ifdef STTUNER_DISEQC2_SWDECODE_VIA_PIO	
 
if(Inst[Instance->TopLevelHandle].Sat.Lnb.Driver->ID == STTUNER_LNB_LNBH21)
 {
#ifdef STTUNER_DRV_SAT_LNBH21
	Error = (Inst[Instance->TopLevelHandle].Sat.Lnb.Driver->lnb_setttxmode)(LHandle,STTUNER_LNB_RX);
	if (Error != ST_NO_ERROR)
	{
		return(Error);		
	}
#endif	
}    
        /* make the pointer to the first location of the data array */
        pCurrent_Data -=pDiSEqCSendPacket->uc_TotalNoOfBytes; 
       /* Now check whether reply is required */
       if ((*pCurrent_Data == FB_COMMAND_REPLY)|
			(*(pDiSEqCSendPacket->pFrmAddCmdData) == FB_COMMAND_REPLY_REPEATED))
		{
		
		 /*Clear the counter before starting the interrupt */
		
		 InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter = 0;
		 InstforISR[IndexforISR].Sat.DiSEqC.IgnoreFirstIntr =0; /* Reset the PIO interrupt workaround flag */
		 InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg.ComparePattern =1;
		 InstforISR[IndexforISR].Sat.DiSEqC.FirstBitRcvdFlag = 0;
		 InstforISR[IndexforISR].Sat.DiSEqC.ReplyContinueFlag =0;
		 InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg.CompareEnable = InstforISR[IndexforISR].Sat.DiSEqC.PIOPin;
		 Error = STPIO_SetCompare(InstforISR[IndexforISR].Sat.DiSEqC.HandlePIO, &InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg);
		 
		 i=0;
		 /* Wait 150ms for the first bit of the reply to come */
		 while(i++ < 16 && InstforISR[IndexforISR].Sat.DiSEqC.FirstBitRcvdFlag == 0 )
		 {
		   DiSEqC_Delay_ms(10);
		 }
	  			
		if (i >= 16) /* If Interrupt is not called before time out is reached then return tmeout error*/
		{
			return STTUNER_ERROR_DISEQC_TIMEOUT;
		}
	

           /* Wait for the reply to stop */
           while(DelayCount < 200)
           {
           	if(InstforISR[IndexforISR].Sat.DiSEqC.ReplyContinueFlag == 0)
           	{
           		break; /* If reply is not coming then come out of the loop*/
           	}
           	else
           	{
           	 InstforISR[IndexforISR].Sat.DiSEqC.ReplyContinueFlag =0;
           	}
           	DiSEqC_Delay_ms(3); 
           	DelayCount +=3;
           }
	              
	       /* Now Disable interrupt */
	       InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg.CompareEnable = 0x00;
	       Error = STPIO_SetCompare(InstforISR[IndexforISR].Sat.DiSEqC.HandlePIO, &InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg);
	       
	       /* Clear the BitPattern[] array */
	       memset(BitPattern,0x00,MAX_NUM_BITS_IN_REPLY);
	       
	       /* Check whether any response is captured or not */
	       if (InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter == 0)
	       {
	         return STTUNER_ERROR_DISEQC_TIMEOUT; /* if no reply is received
	         				 then return timeout error*/
	       	}
	       /* Check whether the number of bits received is multile of 9 or not */
	     	if (InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter%9 != 0)
	     	{
	     	 return STTUNER_ERROR_DISEQC_CORRUPTED_REPLY;
	        } 
	       /* Now decode the bit pattern */
	       for(i=0;i<InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter;i++)
	       {
	       	if (InstforISR[IndexforISR].Sat.DiSEqC.PulseWidth[i] >=6 && InstforISR[IndexforISR].Sat.DiSEqC.PulseWidth[i] <=8 )
	       	{
	       		BitPattern[i] = 1;	
	       	}
	       	else if (InstforISR[IndexforISR].Sat.DiSEqC.PulseWidth[i] >=13 && InstforISR[IndexforISR].Sat.DiSEqC.PulseWidth[i] <=15)
	       	{
	       		BitPattern[i] = 0;
	       	}
	       	else
	       	{
	       	  /* For different pulse width return Corrupted DiSEqC reply error */
	       	  return STTUNER_ERROR_DISEQC_CORRUPTED_REPLY;
	       	}
	}
	
	
	     
	     /* Now check for Parity and make bytes to fill in response byte pointer */
	     NumofBytes = InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter/9;
	     for (i=0;i<NumofBytes;i++)
	     {
	     	DecodedOneByte = 0;
	     	ParityCounter =0;
	     	for(j=0;j<8;j++)
	     	{
	     		if (BitPattern[i*9+j] == 1)
	     		{
	     			ParityCounter++;
	     		}
	     	}
	     	if (ParityCounter%2 == 0) /* If number of 1 in the byte is even then 9th bit 
	     				should be 1 */
	     	 {      
	     	 	if (BitPattern[i*9+j] != 1)
	     	 	{
	     	 	 return STTUNER_ERROR_DISEQC_PARITY;
	     	 	}  
	     	 }   
	     	 else	/* If number of 1 in the byte is odd then 9th bit 
	     				should be 0 */
	     	 {
	     	 	if (BitPattern[i*9+j] != 0)
	     	 	{
	     	 	  return STTUNER_ERROR_DISEQC_PARITY;
	     	 	} 
	     	  } 
	     	  /*Now form the bits into bytes */
	     	  for(j=0;j<8;j++)
	          {
	     		DecodedOneByte = (BitPattern[i*9+7-j]<< j)|DecodedOneByte;
	     	  }
	     	    pDiSEqCResponsePacket->ReceivedFrmAddData[i] =   DecodedOneByte;            
	     	               
	     	}
	     	/* Update the number of byte received field of the response byte structure*/
	     	pDiSEqCResponsePacket->uc_TotalBytesReceived = NumofBytes;
	     	
		
	}
	
#endif 
	
	/* wait for 'pDiSEqCSendPacket->uc_msecBeforeNextCommand' millisec. */
	DiSEqC_Delay_ms(pDiSEqCSendPacket->uc_msecBeforeNextCommand); 

#endif		

/************************************MINIDRIVER************************
*************************************************************************
***************************************************************************/
#ifdef STTUNER_MINIDRIVER
	U8 Data;
	pCurrent_Data = pDiSEqCSendPacket->pFrmAddCmdData;
	if (( *pCurrent_Data == FB_COMMAND_REPLY)|
			(*(pDiSEqCSendPacket->pFrmAddCmdData) == FB_COMMAND_REPLY_REPEATED))
		{

			Error = ST_ERROR_FEATURE_NOT_SUPPORTED; /*Not supported for EVAL 399 board*/	
			return (Error);
		}

	/* In loopthrough mode send DiSEqC-ST commands through first demod */
	#ifdef STTUNER_DRV_SAT_SCR
		#ifdef STTUNER_DRV_SAT_SCR_LOOPTHROUGH
		    if(DEMODInstance->DISECQ_ST_ENABLE == FALSE)
		    {
			    Handle = DemodDrvHandleOne;
		    }
		#endif
	#endif
	DigFreq = FE_399_GetMclkFreq(27000000);     /*27000000->Quartz Frequency */
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_F22FR, F399_F_REG, F399_F_REG_L, &Data, 1, FALSE);
	F22 = Data;
	F22_Freq = FE_399_F22Freq(DigFreq,F22);
	if(F22_Freq<= 21800 || F22_Freq>=22200)
	{
		F22 = DigFreq/(32*22000);
		Data = F22;
		STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_F22FR, F399_F_REG, F399_F_REG_L, &Data, 1, FALSE);
		
		STOS_TaskDelayUs(2000);
		
	}		
	/* wait for  FIFO empty to ensure previous execution is complete*/
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_DISEQCSTATUS, F399_FE, F399_FE_L, &Data, 1, FALSE);
	while(!(Data))	
			  			
		{
				STTUNER_TaskDelay(50);
				if ((++TimeoutCounter)>0xfff)
						{ return(ST_ERROR_TIMEOUT);
						}
	     
				TimeoutCounter =0;
		}
	Data = 1;
	STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DISEQC, F399_TIM_OFF, F399_TIM_OFF_L, &Data, 1, FALSE);
	
	switch (pDiSEqCSendPacket->DiSEqCCommandType)
		
		{ 
 		case	STTUNER_DiSEqC_TONE_BURST_OFF:   /*Generic same as F22_low */
					
		case	STTUNER_DiSEqC_TONE_BURST_OFF_F22_LOW:  		/*DiSEqC pin low,Tone off*/				
				/*DiSEqC(1:0)=00  DiSEqC(2)=X DiSEqC FIFO=empty */
				Data = 0;
				STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DISEQC, F399_DISEQCMODE, F399_DISEQCMODE_L, &Data, 1, FALSE);
				
				if(pDiSEqCSendPacket->DiSEqCCommandType==STTUNER_DiSEqC_TONE_BURST_OFF)
                                DEMODInstance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_OFF ;
				else
		                DEMODInstance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_OFF_F22_LOW ;
                		DEMODInstance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
		break;
	
		case	STTUNER_DiSEqC_TONE_BURST_OFF_F22_HIGH:			/*DiSEqC pin high, Tone off */				
				/*DiSEqC(1:0)=01  DiSEqC(2)=X DiSEqC FIFO=empty */
				Data = 1;
				STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DISEQC, F399_DISEQCMODE, F399_DISEQCMODE_L, &Data, 1, FALSE);				
				DEMODInstance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_OFF_F22_HIGH ;
                		DEMODInstance->DiSEqCConfig.ToneState= STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;							
				break;
		case	STTUNER_DiSEqC_CONTINUOUS_TONE_BURST_ON:  		/* unmodulated */
				
				/*DiSEqC(1:0)=11  DiSEqC(2)=X DiSEqC FIFO=XX  */
				Data = 3;
				STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DISEQC, F399_DISEQCMODE, F399_DISEQCMODE_L, &Data, 1, FALSE);
				DEMODInstance->DiSEqCConfig.Command=STTUNER_DiSEqC_CONTINUOUS_TONE_BURST_ON;
                		DEMODInstance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS;
						
				break;

		case	STTUNER_DiSEqC_TONE_BURST_SEND_0_UNMODULATED:   /*send of 0 for 12.5 ms ;continuous tone*/
				
				Data = 2;
				STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DISEQC, F399_DISEQCMODE, F399_DISEQCMODE_L, &Data, 1, FALSE);
				if (Error !=ST_NO_ERROR){break;}
				
				Data = 0;
				STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DISEQC, F399_DISEQC, F399_DISEQC_L, &Data, 1, FALSE);
				if (Error !=ST_NO_ERROR){break;}
				STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_DISEQCSTATUS, F399_FF, F399_FF_L, &Data, 1, FALSE);				
				while(Data)		/* wait for Place in FIFO */
			  			
						{
						STTUNER_TaskDelay(50);
						if ((++TimeoutCounter)>0x1ff)
							{ return(ST_ERROR_TIMEOUT);
							}
	     
						TimeoutCounter =0;
					 	}
				
				
				Data = 0;
				STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DISEQCFIFO, F399_DISEQCFIFO1, F399_DISEQCFIFO1_L, &Data, 1, FALSE);
				DEMODInstance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_SEND_0_UNMODULATED;
                		DEMODInstance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
											
				break;

		case	STTUNER_DiSEqC_TONE_BURST_SEND_0_MODULATED:     /*0-2/3 duty cycle tone*/
				
				/*DiSEqC(1:0)=10  DiSEqC(2)=1 DiSEqC FIFO=00 */
				Data = 2;
				STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DISEQC, F399_DISEQCMODE, F399_DISEQCMODE_L, &Data, 1, FALSE);
				if (Error !=ST_NO_ERROR){break;}
				Data = 1;
				STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DISEQC, F399_DISEQC, F399_DISEQC_L, &Data, 1, FALSE);
				if (Error !=ST_NO_ERROR){break;}
				Data = 0;
				STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DISEQCFIFO, F399_DISEQCFIFO1, F399_DISEQCFIFO1_L, &Data, 1, FALSE);
				DEMODInstance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_SEND_0_MODULATED;
                		DEMODInstance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
				break;

		case	STTUNER_DiSEqC_TONE_BURST_SEND_1: 				/*1-1/3 duty cycle tone modulated*/
				/*DiSEqC(1:0)=10  DiSEqC(2)=1 DiSEqC FIFO=FF */
				Data = 2;
				STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DISEQC, F399_DISEQCMODE, F399_DISEQCMODE_L, &Data, 1, FALSE);
				if (Error !=ST_NO_ERROR){break;}
				Data = 1;
				STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DISEQC, F399_DISEQC, F399_DISEQC_L, &Data, 1, FALSE);
				if (Error !=ST_NO_ERROR){break;}
				Data = 0xff;
				STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DISEQCFIFO, F399_DISEQCFIFO1, F399_DISEQCFIFO1_L, &Data, 1, FALSE);
				DEMODInstance->DiSEqCConfig.Command=STTUNER_DiSEqC_TONE_BURST_SEND_1;
                		DEMODInstance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
				break;

		case	STTUNER_DiSEqC_COMMAND:							/*DiSEqC (1.2/2)command */
				
				STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_IOCFG2, F399_OP0_1, F399_OP0_1_L, &Data, 1, FALSE);	
				if(Data)					/* If 22 KHz tone is ON		*/
				{
					Data = 0;
					STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_IOCFG2, F399_OP0_1, F399_OP0_1_L, &Data, 1, FALSE);	
				}
				/*DiSEqC(1:0)=10  DiSEqC(2)=1 DiSEqC FIFO=data */
				Data = 2;
				STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DISEQC, F399_DISEQCMODE, F399_DISEQCMODE_L, &Data, 1, FALSE);
				if (Error !=ST_NO_ERROR){break;}
				
				Data = 1;
				STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DISEQC, F399_DISEQC, F399_DISEQC_L, &Data, 1, FALSE);
				
				if (Error !=ST_NO_ERROR){break;}
				for(No_of_Bytes = pDiSEqCSendPacket->uc_TotalNoOfBytes; No_of_Bytes>0; No_of_Bytes--)
					{
						STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, R399_DISEQCSTATUS, F399_FF, F399_FF_L, &Data, 1, FALSE);
						while(Data)		/* wait for Place in FIFO */			  			
							{
								STTUNER_TaskDelay(50);
							    if ((++TimeoutCounter)>0x1ff)
								{  
									return(ST_ERROR_TIMEOUT);
								}
	     
							    TimeoutCounter =0;
					 		}
						Data = *pCurrent_Data;
						STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, R399_DISEQCFIFO, 0, 0, &Data, 1, FALSE);
						pCurrent_Data++;
					}
				
				DEMODInstance->DiSEqCConfig.Command=STTUNER_DiSEqC_COMMAND;
                		DEMODInstance->DiSEqCConfig.ToneState=STTUNER_DiSEqC_TONE_CONTINUOUS_OFF;
                /* Make Delay value to 135 */
                 Delay = 135;
				
				break;
		
		default:
				Error=ST_ERROR_BAD_PARAMETER;
	
	
		}/*end of Switch*/														
	
	if (Error !=ST_NO_ERROR)
		{	
			return(Error);
		}
/* Wait for the DiSEqC command to be sent completely --(TicksPermilisec *13.5* Numberofbytes sent)*/
	 DiSEqC_Delay_ms (((Delay*pDiSEqCSendPacket->uc_TotalNoOfBytes)/10));	

	/* wait for 'pDiSEqCSendPacket->uc_msecBeforeNextCommand' millisec. */
	DiSEqC_Delay_ms(pDiSEqCSendPacket->uc_msecBeforeNextCommand); 
	
  #endif		
return(Error);
}
#ifndef STTUNER_MINIDRIVER
#ifdef STTUNER_DISEQC2_SWDECODE_VIA_PIO
/* ----------------------------------------------------------------------------
Name: CaptureReplyPulse1()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
void CaptureReplyPulse1(STPIO_Handle_t Handle, STPIO_BitMask_t ActiveBits)	
{
	if (InstforISR[IndexforISR].Sat.DiSEqC.IgnoreFirstIntr == 0)
	{
		InstforISR[IndexforISR].Sat.DiSEqC.IgnoreFirstIntr =1;
	}
	else
	{
		
	if(InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg.ComparePattern ==1)
	{
	   InstforISR[IndexforISR].Sat.DiSEqC.FallingTime [InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter] = STOS_time_now();
	}
	 else
	 {
	    InstforISR[IndexforISR].Sat.DiSEqC.PulseWidth[InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter] = STOS_time_minus(STOS_time_now(),InstforISR[IndexforISR].Sat.DiSEqC.FallingTime [InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter]);
	    InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter++;
	     
	 }
	 
	InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg.ComparePattern = (~(InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg.ComparePattern)& 0x01);
	/* set FirstBitRcvdFlag to 1  */
	if (InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter == 1)
	{
	InstforISR[IndexforISR].Sat.DiSEqC.FirstBitRcvdFlag=1;
	}
	/* Set ReplyContinueFlag to 1 */
	InstforISR[IndexforISR].Sat.DiSEqC.ReplyContinueFlag=1;	
	}
	
}
 
 
/* ----------------------------------------------------------------------------
Name: CaptureReplyPulse2()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
void CaptureReplyPulse2(STPIO_Handle_t Handle, STPIO_BitMask_t ActiveBits)	
{
	if (InstforISR[IndexforISR].Sat.DiSEqC.IgnoreFirstIntr == 0)
	{
		InstforISR[IndexforISR].Sat.DiSEqC.IgnoreFirstIntr =1;
	}
	else
	{
		
	if(InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg.ComparePattern ==1)
	{
	   InstforISR[IndexforISR].Sat.DiSEqC.FallingTime [InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter] = STOS_time_now();
	}
	 else
	 {
	    InstforISR[IndexforISR].Sat.DiSEqC.PulseWidth[InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter] = STOS_time_minus(STOS_time_now(),InstforISR[IndexforISR].Sat.DiSEqC.FallingTime [InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter]);
	    InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter++;
	     
	 }
	 
	InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg.ComparePattern = (~(InstforISR[IndexforISR].Sat.DiSEqC.setCmpReg.ComparePattern)& 0x01);
	/* set FirstBitRcvdFlag to 1  */
	if (InstforISR[IndexforISR].Sat.DiSEqC.PulseCounter == 1)
	{
	InstforISR[IndexforISR].Sat.DiSEqC.FirstBitRcvdFlag=1;
	}
	/* Set ReplyContinueFlag to 1 */
	InstforISR[IndexforISR].Sat.DiSEqC.ReplyContinueFlag=1;	
	}
	
}

#endif 
#endif
/* ----------------------------------------------------------------------------
Name: demod_d0399_DiSEqCGetConfig()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0399_DiSEqCGetConfig(DEMOD_Handle_t Handle,STTUNER_DiSEqCConfig_t *DiSEqCConfig)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    #ifndef STTUNER_MINIDRIVER
    D0399_InstanceData_t *Instance;
    Instance =D0399_GetInstFromHandle(Handle);
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
Name: demod_d0399_DiSEqCBurstOFF()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0399_DiSEqCBurstOFF ( DEMOD_Handle_t Handle )
{
   ST_ErrorCode_t Error = ST_NO_ERROR;
  /* D0299_InstanceData_t *Instance;*/
   STTUNER_DiSEqCSendPacket_t DiSEqCSendPacket;
   void *pDiSEqCResponsePacket=NULL;
   
   DiSEqCSendPacket.DiSEqCCommandType = STTUNER_DiSEqC_TONE_BURST_OFF;

   DiSEqCSendPacket.uc_msecBeforeNextCommand =0;
   Error=demod_d0399_DiSEqC(Handle,&DiSEqCSendPacket,pDiSEqCResponsePacket);

   
   return(Error);
}
#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: demod_d0399_ioctl()

Description:
    access device specific low-level functions
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0399_ioctl(DEMOD_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c demod_d0399_ioctl()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0399_InstanceData_t *Instance;

    /* private driver instance data */
    Instance = D0399_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s demod driver instance handle=%d\n", identity, Handle));
        STTBX_Print(("%s                     function=%d\n", identity, Function));
        STTBX_Print(("%s                   I/O handle=%d\n", identity, Instance->IOHandle));
#endif

    /* ---------- select what to do ---------- */
    switch(Function){

        case STTUNER_IOCTL_RAWIO: /* read/write device register (actual write to stv0399) */
            Error =  STTUNER_IOARCH_ReadWrite( Instance->IOHandle, 
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->Operation,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->SubAddr,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->Data,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->TransferSize,
                                                 ((SATIOCTL_IOARCH_Params_t *)InParams)->Timeout );
            break;

        case STTUNER_IOCTL_REG_IN: /* read device register */
            *(int *)OutParams = STTUNER_IOREG_GetRegister( &Instance->DeviceMap, Instance->IOHandle, *(int *)InParams);
            break;

        case STTUNER_IOCTL_REG_OUT: /* write device register. NOTE: RegIndex is <<INDEX>> TO register number. */
            Error =  STTUNER_IOREG_SetRegister( &Instance->DeviceMap, Instance->IOHandle,
                  ((SATIOCTL_SETREG_InParams_t *)InParams)->RegIndex,
                  ((SATIOCTL_SETREG_InParams_t *)InParams)->Value );
            break;

     
        case STTUNER_IOCTL_IQMODE: /* Get the IQ mode */  /* DS Modification to avoid NOT supported error !!!!! */

            if(STTUNER_IOREG_GetField(&Instance->DeviceMap, Instance->IOHandle, F399_SYM))
                *((STTUNER_IQMode_t *)OutParams)=STTUNER_IQ_MODE_INVERTED;
            else
                *((STTUNER_IQMode_t *)OutParams)=STTUNER_IQ_MODE_NORMAL;
            break;
           
        default:
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
    STTBX_Print(("%s called ok\n", identity, Function));    /* signal that function came back */
#endif
    return(Error);

}
/* ----------------------------------------------------------------------------
Name: demod_d0399_ioaccess()

Description:
    called from ioarch.c when some driver does I/O but with the repeater/passthru
    set to point to this function.
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t demod_d0399_ioaccess(DEMOD_Handle_t Handle, IOARCH_Handle_t IOHandle, STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)

{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c demod_d0399_ioaccess()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    IOARCH_Handle_t ThisIOHandle;
    D0399_InstanceData_t *Instance;

    Instance = D0399_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* this (demod) drivers I/O handle */
    ThisIOHandle = Instance->IOHandle;

    /* if STTUNER_IOARCH_MAX_HANDLES then passthrough function required using our device handle/address */ 
    if (IOHandle == STTUNER_IOARCH_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s write passthru\n", identity));
#endif
        Error = STTUNER_IOARCH_ReadWrite(ThisIOHandle, Operation, SubAddr, Data, TransferSize, Timeout);
    }
    else    /* repeater */
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s write repeater\n", identity));
#endif
#ifndef ST_OSLINUX
        STTUNER_task_lock();   /* prevent any other activity on this bus (no task must preempt us) */
#endif      
        /* enable repeater then send the data  using I/O handle supplied through ioarch.c */
        Error = STTUNER_IOREG_SetField(&Instance->DeviceMap, Instance->IOHandle, F399_I2CT_ON, 1);

        /* send callers data using their address. (nb: subaddress == 0)
         Function 'STTUNER_IOARCH_ReadWriteNoRep' is called as calling the normal 'STTUNER_IOARCH_ReadWrite'
        function would cause it to call the redirection function which is THIS function, and around we 
        would go forever. */
        Error = STTUNER_IOARCH_ReadWriteNoRep(IOHandle, Operation, 0, Data, TransferSize, Timeout);
 #ifndef ST_OSLINUX
	STTUNER_task_unlock();
#endif 
        
    }

    return(Error);
}




/* ----------------------------------------------------------------------------
Name: demod_d0399_StandByMode()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */



ST_ErrorCode_t demod_d0399_StandByMode(DEMOD_Handle_t Handle, STTUNER_StandByMode_t PowerMode)
{
	
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
   const char *identity = "STTUNER d0399.c demod_d0399_StandByMode()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    D0399_InstanceData_t *Instance;
    
   
    /* private driver instance data */
    Instance = D0399_GetInstFromHandle(Handle);
   
      
    switch ( PowerMode)
    {
       case STTUNER_NORMAL_POWER_MODE :
       if(Instance->StandBy_Flag == 1)
       {
          Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F399_STANDBY, 0);
       }
          
             
       if(Error==ST_NO_ERROR)
       {
          Instance->StandBy_Flag = 0 ;
       }
       
       break;
       case STTUNER_STANDBY_POWER_MODE :
       if(Instance->StandBy_Flag == 0)
       {
          Error|=STTUNER_IOREG_SetField(&(Instance->DeviceMap), Instance->IOHandle, F399_STANDBY, 1);
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

D0399_InstanceData_t *D0399_GetInstFromHandle(DEMOD_Handle_t Handle)
{
    return( (D0399_InstanceData_t *)Handle );
}

#endif


#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
static char *FE_399_Errors2String(FE_399_Error_t Error)
{
    static char *err399s[] = { "FE_NO_ERROR",      "FE_INVALID_HANDLE" ,     "FE_ALLOCATION", 
                               "FE_BAD_PARAMETER", "FE_ALREADY_INITIALIZED", "FE_I2C_ERROR",
                               "FE_SEARCH_FAILED", "FE_TRACKING_FAILED", "FE_TERM_FAILED", "FENCEPOST ERROR" };

    if (Error < FE_TERM_FAILED)
    {
        return err399s[Error];
    }
        return NULL;

}    
#endif

/* For tone detection */
ST_ErrorCode_t demod_d0399_tonedetection(DEMOD_Handle_t Handle,U32 StartFreq, U32 StopFreq, U8  *NbTones,U32 *ToneList, U8 mode,int* power_detection_level)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U8 MaxNbTone = 4;
    #ifndef STTUNER_MINIDRIVER
    D0399_InstanceData_t *Instance;
    Instance = D0399_GetInstFromHandle(Handle);

    if(STTUNER_Util_CheckPtrNull(Instance) != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_D0399
        STTBX_Print(("%s invalid handle\n", identity));
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }       
    *NbTones = FE_399_ToneDetection(&Instance->DeviceMap, Instance->IOHandle,StartFreq,StopFreq,ToneList,MaxNbTone);
    #endif
    
    #ifdef STTUNER_MINIDRIVER
     *NbTones = FE_399_ToneDetection(StartFreq,StopFreq,ToneList,MaxNbTone);
    #endif

    return(Error);
    
}   


/* End of d0399.c */
