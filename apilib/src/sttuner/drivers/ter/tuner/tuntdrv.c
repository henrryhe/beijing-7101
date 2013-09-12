/* ----------------------------------------------------------------------------
File Name: tuntdrv.c

Description: external tuner drivers.


Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 13-Sept-2001
version: 3.1.0
 author: GB from the work by GJP
comment: rewrite for terrestrial.

Reference:

    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */

/* C libs */
#ifndef STTUNER_MINIDRIVER
#ifndef ST_OSLINUX
   
#include <string.h>
#endif
#include "ioreg.h"
#include "stlite.h"     /* Standard includes */

/* STAPI */
#include "sttbx.h"

#include "stevt.h"
#include "sttuner.h"


/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "ioarch.h"     /* I/O for this driver */
      /* I/O for this driver */
#include "tdrv.h"       /* utilities */

#include "tioctl.h"     /* data structure typedefs for all the the ter ioctl functions */

#include "tuntdrv.h"    /* header for this file */
#include "ll_tun0360.h" /* low level functions */

#include "chip.h"

#ifndef ST_OSLINUX
#include "RF4000Init.h"
#include "RF4KCCLib.h"
#endif

#ifdef STTUNER_DRV_TER_TUN_MT2266
#include "mt2266.h"
#endif


#define TUNTDRV_IO_TIMEOUT 10

#define WRITE 1
#define READ  0

#define WAIT_N_MS_TUNT(X)     STOS_TaskDelayUs(X *  1000 )  
#define I2C_HANDLE(x)      (*((STI2C_Handle_t *)x))


 partition_t             *system_partition;


/* private variables ------------------------------------------------------- */
int dbg_counter;

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */


static BOOL Installed            = FALSE;
	static BOOL Installed_DTT7572    = FALSE;
	static BOOL Installed_DTT7578    = FALSE;
	static BOOL Installed_DTT7592    = FALSE;
	static BOOL Installed_TDLB7      = FALSE;
	static BOOL Installed_TDQD3      = FALSE;
	static BOOL Installed_DTT7300X      = FALSE;
	static BOOL Installed_ENG47402G1 = FALSE;
	static BOOL Installed_EAL2780 = FALSE;
	static BOOL Installed_TDA6650 = FALSE;
	static BOOL Installed_TDM1316 = FALSE;
	static BOOL Installed_TDEB2 = FALSE;
	static BOOL Installed_ED5058 = FALSE;
	static BOOL Installed_MIVAR = FALSE;
	static BOOL Installed_TDED4 = FALSE;
	static BOOL Installed_DTT7102 = FALSE;
	static BOOL Installed_TECC2849PG = FALSE;
	static BOOL Installed_TDCC2345 = FALSE;
	
	#ifndef ST_OSLINUX
	static BOOL Installed_RF4000 = FALSE;  
	#endif
	
	static BOOL Installed_TDTGD108   = FALSE;
	static BOOL Installed_MT2266   = FALSE;
	
static BOOL Installed_DTVS223 = FALSE; 



 
/* instance chain, the default boot value is invalid, to catch errors */

static TUNTDRV_InstanceData_t *InstanceChainTop = (TUNTDRV_InstanceData_t *)0x7fffffff;

extern IOARCH_HandleData_t IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];


/* functions --------------------------------------------------------------- */
/* API */
 
ST_ErrorCode_t tuner_tuntdrv_Init(ST_DeviceName_t *DeviceName, TUNER_InitParams_t *InitParams);
ST_ErrorCode_t tuner_tuntdrv_Term(ST_DeviceName_t *DeviceName, TUNER_TermParams_t *TermParams);

ST_ErrorCode_t tuner_tuntdrv_Open(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Handle_t *Handle);

	#ifdef STTUNER_DRV_TER_TUN_DTT7572
	ST_ErrorCode_t tuner_tuntdrv_Open_DTT7572    (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
	#endif
	#ifdef STTUNER_DRV_TER_TUN_DTT7578
	ST_ErrorCode_t tuner_tuntdrv_Open_DTT7578    (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
	#endif
	#ifdef STTUNER_DRV_TER_TUN_DTT7592
	ST_ErrorCode_t tuner_tuntdrv_Open_DTT7592    (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDLB7
	ST_ErrorCode_t tuner_tuntdrv_Open_TDLB7      (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDQD3
	ST_ErrorCode_t tuner_tuntdrv_Open_TDQD3      (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
	#endif
	#ifdef STTUNER_DRV_TER_TUN_DTT7300X
	ST_ErrorCode_t tuner_tuntdrv_Open_DTT7300X      (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
	#endif
	#ifdef STTUNER_DRV_TER_TUN_ENG47402G1
	ST_ErrorCode_t tuner_tuntdrv_Open_ENG47402G1      (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
	#endif
	#ifdef STTUNER_DRV_TER_TUN_EAL2780
	ST_ErrorCode_t tuner_tuntdrv_Open_EAL2780 (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDA6650
	ST_ErrorCode_t tuner_tuntdrv_Open_TDA6650 (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDM1316
	ST_ErrorCode_t tuner_tuntdrv_Open_TDM1316 (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDEB2
	ST_ErrorCode_t tuner_tuntdrv_Open_TDEB2 (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
	#endif
	#ifdef STTUNER_DRV_TER_TUN_ED5058
	ST_ErrorCode_t tuner_tuntdrv_Open_ED5058 (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
	#endif
	#ifdef STTUNER_DRV_TER_TUN_MIVAR
	
	ST_ErrorCode_t tuner_tuntdrv_Open_MIVAR (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDED4
	ST_ErrorCode_t tuner_tuntdrv_Open_TDED4 (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
	#endif
	#ifdef STTUNER_DRV_TER_TUN_DTT7102
	ST_ErrorCode_t tuner_tuntdrv_Open_DTT7102 (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TECC2849PG
	ST_ErrorCode_t tuner_tuntdrv_Open_TECC2849PG (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDCC2345
	ST_ErrorCode_t tuner_tuntdrv_Open_TDCC2345 (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams,TUNER_Capability_t *Capability,  TUNER_Handle_t *Handle);
	#endif
	#ifdef STTUNER_DRV_TER_TUN_RF4000
	#ifndef ST_OSLINUX
	ST_ErrorCode_t tuner_tuntdrv_Open_RF4000(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
	#endif
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDTGD108
	ST_ErrorCode_t tuner_tuntdrv_Open_TDTGD108 (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
	#endif
	#ifdef STTUNER_DRV_TER_TUN_MT2266
	ST_ErrorCode_t tuner_tuntdrv_Open_MT2266(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif	


#ifdef STTUNER_DRV_TER_TUN_DTVS223
ST_ErrorCode_t tuner_tuntdrv_Open_DTVS223(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams,TUNER_Capability_t *Capability,  TUNER_Handle_t *Handle);
#endif

ST_ErrorCode_t tuner_tuntdrv_Close(TUNER_Handle_t Handle, TUNER_CloseParams_t *CloseParams);

ST_ErrorCode_t tuner_tuntdrv_SetFrequency (TUNER_Handle_t Handle, U32 Frequency, U32 *NewFrequency);
ST_ErrorCode_t tuner_tuntdrv_IsTunerLocked(TUNER_Handle_t Handle, BOOL *Locked);
ST_ErrorCode_t tuner_tuntdrv_GetStatus(TUNER_Handle_t Handle, TUNER_Status_t *Status);

void TunerSetNbSteps(TUNER_Handle_t Handle, int nbsteps);
int  TunerGetNbSteps(TUNER_Handle_t Handle);
void TunerInit      (TUNER_Handle_t Handle);

/* I/O API */

ST_ErrorCode_t tuner_tuntdrv_ioaccess(TUNER_Handle_t Handle, IOARCH_Handle_t IOHandle,
STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);

/* access device specific low-level functions */
ST_ErrorCode_t tuner_tuntdrv_ioctl(TUNER_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status);


/* local functions --------------------------------------------------------- */

ST_ErrorCode_t STTUNER_DRV_TUNER_TUNTDRV_Install(  STTUNER_tuner_dbase_t  *Tuner, STTUNER_TunerType_t TunerType);
ST_ErrorCode_t STTUNER_DRV_TUNER_TUNTDRV_UnInstall(STTUNER_tuner_dbase_t  *Tuner, STTUNER_TunerType_t TunerType);


U32 TunerGetStepsize(TUNTDRV_InstanceData_t *Instance);

#ifdef STTUNER_DRV_TER_TUN_MT2266
U8 MT2266_DefVal[MT2266_NBREGS]=
	{
				
	    0x85,
	    0x08,
	    0x88,
	    0x22,
	    0x00,
	    0x52,
	    0xdd,
	    0x3f,
	    0x20,
	    0x20,
	    0x20,
	    0x20,
	    0x20,
	    0x20,
	    0x20,
	    0x20,
	    0x00,
	    0x01,
	    0x40,
	    0x0a,
	    0xc0,
	    0x00,
	    0x30,
	    0x6d,
	    0x71,
	    0x61,
	    0xcd,
	    0xbf,
	    0xff,
	    0xdc,
	    0x00,
	    0x0a,
	    0xd4,
	    0x03,
	    0x64,
	    0x64,
	    0x64,
	    0x64,
	    0x22,
	    0xaa,
	    0xf2,
	    0x1e,
	    0x80,
	    0x14,
	    0x01,
	    0x01,
	    0x01,
	    0x01,
	    0x01,
	    0x01,
	    0x7f,
	    0x5e,
	    0x3f,
	    0xff,
	    0xff,
	    0xff,
	    0x00,
	    0x77,
	    0x0f,
	    0x2d,
	    0x00
	   
				
	};
U16 MT2266_Address[MT2266_NBREGS]=
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
0x000A,
0x000B,
0x000C,
0x000D,
0x000E,
0x000F,
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
0x001A,
0x001B,
0x001C,
0x001D,
0x001E,
0x001F,
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
0x002A,
0x002B,
0x002C,
0x002D,
0x002E,
0x002F,
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
0x003A,
0x003B,
0x003C
};
	
#endif
#ifdef STTUNER_DRV_TER_TUN_DTVS223

U8 DTVS223_DefVal[DTVS223_NBREGS]= {0x0034,0x00C0,0x00C3,0x0008,0x0081,0x00};


U16  DTVS223_Address[DTVS223_NBREGS]={0x0000,0x0001,0x0002,0x0003,0x0004,0x0005};
#endif

#ifdef STTUNER_DRV_TER_TUN_TDQD3
U16  TDQD3_Address[TDQD3_NBREGS]=
{


0x0000,
0x0001,
0x0002,
0x0003,
0x0004,
0x0005
};


U8  TDQD3_DefVal[TDQD3_NBREGS]=
{0x0B,0xF5,0xA0,0xEC,0xC6,0x00};
#endif
#ifdef STTUNER_DRV_TER_TUN_DTT7300X
U16  DTT7300X_Address[DTT7300X_NBREGS]=
{


0x0000,
0x0001,
0x0002,
0x0003,
0x0004
};


U8  DTT7300X_DefVal[DTT7300X_NBREGS]=
{0x000B,0x00F4,0x009C,0x0020,0x00/*0x0C,0xE4,0x9C,0xB0,0x72*/};
#endif
#ifdef STTUNER_DRV_TER_TUN_TDTGD108
U16  TDTGD108_Address[TDTGD108_NBREGS]=
{


0x0000,
0x0001,
0x0002,
0x0003,
0x0000,
0x0001,
0x0002,
0x0003,
0x0004
};


U8  TDTGD108_DefVal[TDTGD108_NBREGS]=
{0x0b,0xf5,0xc4,0x04,0x0b,0xf5,0xdc,0x40,0xff
};
#endif

/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_TUNER_TUNTDRV_Install()

Description:
    install a terrestrial device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_TUNER_TUNTDRV_Install(STTUNER_tuner_dbase_t  *Tuner, STTUNER_TunerType_t TunerType)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c STTUNER_DRV_TUNER_TUNTDRV_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    /* driver wide bits to init once only */
    if (Installed == FALSE)
    {
        InstanceChainTop = NULL;

        Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);

        Installed = TRUE;
    }

  


  switch(TunerType)
    {
   
	#ifdef STTUNER_DRV_TER_TUN_DTT7572
	case STTUNER_TUNER_DTT7572:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s installing ter:tuner:DTT7572...", identity));
		#endif
	            if (Installed_DTT7572 == TRUE)
	            {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	                STTBX_Print(("fail already installed\n"));
	#endif
	                return(STTUNER_ERROR_INITSTATE);
	            }
	            Tuner->ID = STTUNER_TUNER_DTT7572;
	            Tuner->tuner_Open = tuner_tuntdrv_Open_DTT7572;
	            Installed_DTT7572 = TRUE;
	            break;  
	#endif
	#ifdef STTUNER_DRV_TER_TUN_DTT7578
	case STTUNER_TUNER_DTT7578:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s installing ter:tuner:DTT7578...", identity));
	        #endif    
	
	            if (Installed_DTT7578 == TRUE)
	            {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	                STTBX_Print(("fail already installed\n"));
	#endif
	                return(STTUNER_ERROR_INITSTATE);
	             }
	            
	            Tuner->ID = STTUNER_TUNER_DTT7578;
	            
	            Tuner->tuner_Open = tuner_tuntdrv_Open_DTT7578;
	             Installed_DTT7578 =true;
	break;
	#endif
	#ifdef STTUNER_DRV_TER_TUN_DTT7592
	case STTUNER_TUNER_DTT7592:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s installing ter:tuner:DTT7592...", identity));
	        #endif    
	
	            if (Installed_DTT7592 == TRUE)
	            {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	                STTBX_Print(("fail already installed\n"));
	#endif
	                return(STTUNER_ERROR_INITSTATE);
	             }
	            
	            Tuner->ID = STTUNER_TUNER_DTT7592;
	            
	            Tuner->tuner_Open = tuner_tuntdrv_Open_DTT7592;
	            Installed_DTT7592 = TRUE;
	                    break;
	#endif
	#ifdef  STTUNER_DRV_TER_TUN_TDLB7
	case STTUNER_TUNER_TDLB7:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s installing ter:tuner:TDLB7...", identity));
	        #endif    
	
	            if (Installed_TDLB7 == TRUE)
	            {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	                STTBX_Print(("fail already installed\n"));
	#endif
	                return(STTUNER_ERROR_INITSTATE);
	            }
	            Tuner->ID = STTUNER_TUNER_TDLB7;
	            Tuner->tuner_Open = tuner_tuntdrv_Open_TDLB7;
	            Installed_TDLB7 = TRUE;
	break;
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDQD3
	case STTUNER_TUNER_TDQD3:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s installing ter:tuner:TDQD3...", identity));
	        #endif
	
	            if (Installed_TDQD3 == TRUE)
	            {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	                STTBX_Print(("fail already installed\n"));
	#endif
	                return(STTUNER_ERROR_INITSTATE);
	            }
	            Tuner->ID = STTUNER_TUNER_TDQD3;
	            Tuner->tuner_Open = tuner_tuntdrv_Open_TDQD3;
	            Installed_TDQD3 = TRUE;
	            break;
	#endif
	
	#ifdef STTUNER_DRV_TER_TUN_DTT7300X
	case STTUNER_TUNER_DTT7300X:
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s installing ter:tuner:DTT7300X...", identity));
	#endif
	            if (Installed_DTT7300X == TRUE)
	            {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	                STTBX_Print(("fail already installed\n"));
	#endif
	                return(STTUNER_ERROR_INITSTATE);
	            }
	            Tuner->ID = STTUNER_TUNER_DTT7300X;
	            Tuner->tuner_Open = tuner_tuntdrv_Open_DTT7300X;
	            Installed_DTT7300X = TRUE;
	            break;
	
	#endif
	
	
	
	#ifdef STTUNER_DRV_TER_TUN_ENG47402G1
	case STTUNER_TUNER_ENG47402G1:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s installing ter:tuner:ENG47402G1...", identity));
	            #endif
	
	            if (Installed_ENG47402G1 == TRUE)
	            {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	                STTBX_Print(("fail already installed\n"));
	#endif
	                return(STTUNER_ERROR_INITSTATE);
	            }
	            Tuner->ID = STTUNER_TUNER_ENG47402G1;
	            Tuner->tuner_Open = tuner_tuntdrv_Open_ENG47402G1;
	            Installed_ENG47402G1 = TRUE;
	            break; 
	 #endif        
	#ifdef STTUNER_DRV_TER_TUN_EAL2780
	case STTUNER_TUNER_EAL2780:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s installing ter:tuner:EAL2780...", identity));
	            #endif
	
	            if (Installed_EAL2780 == TRUE)
	            {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	                STTBX_Print(("fail already installed\n"));
	#endif
	                return(STTUNER_ERROR_INITSTATE);
	            }
	            Tuner->ID = STTUNER_TUNER_EAL2780;
	            Tuner->tuner_Open = tuner_tuntdrv_Open_EAL2780;
	            Installed_EAL2780 = TRUE;
	            break;
	#endif
	
	#ifdef STTUNER_DRV_TER_TUN_TDA6650
	case STTUNER_TUNER_TDA6650:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s installing ter:tuner:TDA6650...", identity));
	        #endif    
	
	            if (Installed_TDA6650 == TRUE)
	            {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	                STTBX_Print(("fail already installed\n"));
	#endif
	                return(STTUNER_ERROR_INITSTATE);
	            }
	            Tuner->ID = STTUNER_TUNER_TDA6650;
	            Tuner->tuner_Open = tuner_tuntdrv_Open_TDA6650;
	            Installed_TDA6650 = TRUE;
	            break;
	 #endif           
	#ifdef STTUNER_DRV_TER_TUN_TDM1316
	case STTUNER_TUNER_TDM1316:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s installing ter:tuner:TDM1316...", identity));
	        #endif    
	
	            if (Installed_TDM1316 == TRUE)
	            {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	                STTBX_Print(("fail already installed\n"));
	#endif
	                return(STTUNER_ERROR_INITSTATE);
	            }
	            Tuner->ID = STTUNER_TUNER_TDM1316;
	            Tuner->tuner_Open = tuner_tuntdrv_Open_TDM1316;
	            Installed_TDM1316 = TRUE;
	            break;
	 #endif           
	#ifdef STTUNER_DRV_TER_TUN_TDEB2
	case STTUNER_TUNER_TDEB2:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s installing ter:tuner:TDEB2...", identity));
	         #endif   
	
	            if (Installed_TDEB2 == TRUE)
	            {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	                STTBX_Print(("fail already installed\n"));
	#endif
	                return(STTUNER_ERROR_INITSTATE);
	            }
	            Tuner->ID = STTUNER_TUNER_TDEB2;
	            Tuner->tuner_Open = tuner_tuntdrv_Open_TDEB2;
	            Installed_TDEB2 = TRUE;
	            break;
	#endif
	            
	#ifdef STTUNER_DRV_TER_TUN_ED5058
	case STTUNER_TUNER_ED5058:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s installing ter:tuner:ED5058...", identity));
	        #endif    
	
	            if (Installed_ED5058 == TRUE)
	            {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	                STTBX_Print(("fail already installed\n"));
	#endif
	                return(STTUNER_ERROR_INITSTATE);
	            }
	            Tuner->ID = STTUNER_TUNER_ED5058;
	            Tuner->tuner_Open = tuner_tuntdrv_Open_ED5058;
	            Installed_ED5058 = TRUE;
	
	             break;
	 #endif           
	#ifdef STTUNER_DRV_TER_TUN_MIVAR
	case STTUNER_TUNER_MIVAR:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s installing ter:tuner:MIVAR...", identity));
	        #endif
	
	            if (Installed_MIVAR == TRUE)
	            {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	                STTBX_Print(("fail already installed\n"));
	#endif
	                return(STTUNER_ERROR_INITSTATE);
	            }
	            Tuner->ID = STTUNER_TUNER_MIVAR;
	            Tuner->tuner_Open = tuner_tuntdrv_Open_MIVAR;
	            Installed_MIVAR = TRUE;
	            break;
	 #endif           
	#ifdef STTUNER_DRV_TER_TUN_TDED4
	case STTUNER_TUNER_TDED4:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s installing ter:tuner:TDED4...", identity));
	         #endif
	
	            if (Installed_TDED4 == TRUE)
	            {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	                STTBX_Print(("fail already installed\n"));
	#endif
	                return(STTUNER_ERROR_INITSTATE);
	            }
	            Tuner->ID = STTUNER_TUNER_TDED4;
	            Tuner->tuner_Open = tuner_tuntdrv_Open_TDED4;
	            Installed_TDED4 = TRUE;
	            break;
	#endif            
	#ifdef STTUNER_DRV_TER_TUN_DTT7102
	case STTUNER_TUNER_DTT7102:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s installing ter:tuner:DTT7102...", identity));
	        #endif    
	
	            if (Installed_DTT7102 == TRUE)
	            {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	                STTBX_Print(("fail already installed\n"));
	#endif
	                return(STTUNER_ERROR_INITSTATE);
	            }
	            Tuner->ID = STTUNER_TUNER_DTT7102;
	            Tuner->tuner_Open = tuner_tuntdrv_Open_DTT7102;
	            Installed_DTT7102 = TRUE;
	            break;
	 #endif           
	#ifdef STTUNER_DRV_TER_TUN_TECC2849PG
	case STTUNER_TUNER_TECC2849PG:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s installing ter:tuner:TECC2849PG...", identity));
	         #endif   
	
	            if (Installed_TECC2849PG == TRUE)
	            {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	                STTBX_Print(("fail already installed\n"));
	#endif
	                return(STTUNER_ERROR_INITSTATE);
	            }
	            Tuner->ID = STTUNER_TUNER_TECC2849PG;
	            Tuner->tuner_Open = tuner_tuntdrv_Open_TECC2849PG;
	            Installed_TECC2849PG = TRUE;
	 
	             break;
	 #endif          
	#ifdef STTUNER_DRV_TER_TUN_TDCC2345
	            case STTUNER_TUNER_TDCC2345:
	            #ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s installing ter:tuner:TDCC2345...", identity));
	            #endif
	
	            if (Installed_TDCC2345 == TRUE)
	            {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	                STTBX_Print(("fail already installed\n"));
	#endif
	                return(STTUNER_ERROR_INITSTATE);
	            }
	            Tuner->ID = STTUNER_TUNER_TDCC2345;
	            Tuner->tuner_Open = tuner_tuntdrv_Open_TDCC2345;
	            Installed_TDCC2345 = TRUE;
	            break;
	 #endif 
	
	 
	#ifdef STTUNER_DRV_TER_TUN_RF4000
	#ifndef ST_OSLINUX
	case STTUNER_TUNER_RF4000:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s installing ter:tuner:RF4000...", identity));
	            #endif
	
	            if (Installed_RF4000 == TRUE)
	            {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	                STTBX_Print(("fail already installed\n"));
	#endif
	                return(STTUNER_ERROR_INITSTATE);
	            }
	            Tuner->ID = STTUNER_TUNER_RF4000;
	            Tuner->tuner_Open = tuner_tuntdrv_Open_RF4000;
	            Installed_RF4000 =TRUE;
	 break;              
	#endif
	#endif
	
	#ifdef STTUNER_DRV_TER_TUN_TDTGD108
	case STTUNER_TUNER_TDTGD108:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s installing ter:tuner:TDTGD108...", identity));
		#endif
	            if (Installed_TDTGD108 == TRUE)
	            {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	                STTBX_Print(("fail already installed\n"));
	#endif
	                return(STTUNER_ERROR_INITSTATE);
	            }
	            Tuner->ID = STTUNER_TUNER_TDTGD108;
	            Tuner->tuner_Open = tuner_tuntdrv_Open_TDTGD108;
	            Installed_TDTGD108 = TRUE;
	            break;  
	#endif
	
	#ifdef STTUNER_DRV_TER_TUN_MT2266
	case STTUNER_TUNER_MT2266:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s installing ter:tuner:MT2266...", identity));
#endif
	            if (Installed_MT2266 == TRUE)
	            {
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	                STTBX_Print(("fail already installed\n"));
	#endif
	                return(STTUNER_ERROR_INITSTATE);
	            }
	            Tuner->ID = STTUNER_TUNER_MT2266;
	            Tuner->tuner_Open = tuner_tuntdrv_Open_MT2266;
	            Installed_MT2266 = TRUE;
	            break;  
	#endif
	
#ifdef STTUNER_DRV_TER_TUN_DTVS223
case STTUNER_TUNER_DTVS223:
            #ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV          
            STTBX_Print(("%s installing ter:tuner:DTVS223...", identity));
		#endif
            if (Installed_DTVS223 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_DTVS223;
            Tuner->tuner_Open = tuner_tuntdrv_Open_DTVS223;
            Installed_DTVS223 = TRUE;
            break;
#endif
default:
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
            STTBX_Print(("%s incorrect tuner index", identity));            

#endif
          return(ST_ERROR_UNKNOWN_DEVICE);
break;
        
   }         
  





    /* map rest of API */
    Tuner->tuner_Init  = tuner_tuntdrv_Init;
    Tuner->tuner_Term  = tuner_tuntdrv_Term;
    Tuner->tuner_Close = tuner_tuntdrv_Close;

    Tuner->tuner_SetFrequency  = tuner_tuntdrv_SetFrequency;
    Tuner->tuner_GetFrequency  = tuner_tuntdrv_GetFrequency;
    Tuner->tuner_GetProperties = tuner_tuntdrv_GetProperties;
    Tuner->tuner_Select        = tuner_tuntdrv_Select;
    Tuner->tuner_GetStatus     = tuner_tuntdrv_GetStatus;
    Tuner->tuner_IsTunerLocked = tuner_tuntdrv_IsTunerLocked;
    Tuner->tuner_SetBandWidth  = NULL;
    
    
   Tuner->tuner_ioaccess = tuner_tuntdrv_ioaccess;

    Tuner->tuner_ioctl    = tuner_tuntdrv_ioctl;
    



#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("ok\n"));
#endif
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_TUNER_TUNTDRV_UnInstall()

Description:
    install a terrestrial device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_TUNER_TUNTDRV_UnInstall(STTUNER_tuner_dbase_t  *Tuner, STTUNER_TunerType_t TunerType)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c STTUNER_DRV_TUNER_TUNTDRV_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;


   

 switch(TunerType)
    {
	#ifdef STTUNER_DRV_TER_TUN_DTT7572
	case STTUNER_TUNER_DTT7572:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s uninstalling ter:tuner:DTT7572\n", identity));
		#endif
	            Installed_DTT7572 = FALSE;
	break;
	 #endif          
	
	#ifdef STTUNER_DRV_TER_TUN_DTT7578
	case STTUNER_TUNER_DTT7578:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s uninstalling ter:tuner:DTT7578\n", identity));
		#endif
	            Installed_DTT7578 = FALSE;
	break;
	 #endif           
	      
	#ifdef STTUNER_DRV_TER_TUN_DTT7592
	case STTUNER_TUNER_DTT7592:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s uninstalling ter:tuner:DTT7592\n", identity));
		#endif
	            Installed_DTT7592 = FALSE;
	break;
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDLB7
	case STTUNER_TUNER_TDLB7:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s uninstalling ter:tuner:TDLB7\n", identity));
		#endif
	            Installed_TDLB7 = FALSE;
	break;
	#endif  
	#ifdef STTUNER_DRV_TER_TUN_TDQD3
	case STTUNER_TUNER_TDQD3:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s uninstalling ter:tuner:TDQD3\n", identity));
		#endif
	            Installed_TDQD3 = FALSE;
	break;
	 #endif 
	  #ifdef STTUNER_DRV_TER_TUN_DTT7300X
	case STTUNER_TUNER_DTT7300X:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s uninstalling ter:tuner:DTT7300X\n", identity));
		#endif
	            Installed_DTT7300X = FALSE;
	break;
	 #endif
          
	#ifdef STTUNER_DRV_TER_TUN_ENG47402G1
	case STTUNER_TUNER_ENG47402G1:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s uninstalling ter:tuner:ENG47402G1\n", identity));
		#endif
	            Installed_ENG47402G1 = FALSE;
	break;
	#endif    
	
	 #ifdef STTUNER_DRV_TER_TUN_EAL2780
	case STTUNER_TUNER_EAL2780:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s uninstalling ter:tuner:EAL2780\n", identity));
		#endif
	            Installed_EAL2780 = FALSE;
	break;
	#endif
	            
	#ifdef STTUNER_DRV_TER_TUN_TDA6650
	case STTUNER_TUNER_TDA6650:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s uninstalling ter:tuner:TDA6650\n", identity));
		#endif
	            Installed_TDA6650 = FALSE;
	break;
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDM1316
	case STTUNER_TUNER_TDM1316:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s uninstalling ter:tuner:TDM1316\n", identity));
		#endif
	            Installed_TDM1316 = FALSE;
	break;
	#endif
	            
	#ifdef STTUNER_DRV_TER_TUN_TDEB2
	case STTUNER_TUNER_TDEB2:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s uninstalling ter:tuner:TDEB2\n", identity));
			#endif
	            Installed_TDEB2 = FALSE;
	break;
	#endif 
	#ifdef STTUNER_DRV_TER_TUN_ED5058
	case STTUNER_TUNER_ED5058:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s uninstalling ter:tuner:ED5058\n", identity));
		#endif
	            Installed_ED5058 = FALSE;
	break;
	#endif
	            
	#ifdef STTUNER_DRV_TER_TUN_MIVAR
	case STTUNER_TUNER_MIVAR:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s uninstalling ter:tuner:MIVAR\n", identity));
		#endif
	            Installed_MIVAR = FALSE;
	break;
	#endif  
	#ifdef STTUNER_DRV_TER_TUN_TDED4
	case STTUNER_TUNER_TDED4:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s uninstalling ter:tuner:TDED4\n", identity));
		#endif
	            Installed_TDED4 = FALSE;
	 break;
	 #endif
	            
	#ifdef STTUNER_DRV_TER_TUN_DTT7102
	case STTUNER_TUNER_DTT7102:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s uninstalling ter:tuner:DTT7102\n", identity));
		#endif
	            Installed_DTT7102 = FALSE;
	break;
	 #endif           
	#ifdef STTUNER_DRV_TER_TUN_TECC2849PG
	case STTUNER_TUNER_TECC2849PG:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s uninstalling ter:tuner:TECC2849PG\n", identity));
		#endif
	            Installed_TECC2849PG = FALSE;
	break;
	#endif
	            
	#ifdef STTUNER_DRV_TER_TUN_TDCC2345
	case STTUNER_TUNER_TDCC2345:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s uninstalling ter:tuner:TDCC2345\n", identity));
		#endif
	            Installed_TDCC2345 = FALSE;
	break;
	#endif
	  
	#ifdef STTUNER_DRV_TER_TUN_RF4000
	#ifndef ST_OSLINUX
	case STTUNER_TUNER_RF4000:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s uninstalling ter:tuner:RF4000\n", identity));
		#endif
	            Installed_RF4000 = FALSE;
	break;
	#endif
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDTGD108
	case STTUNER_TUNER_TDTGD108:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s uninstalling ter:tuner:TDTGD108\n", identity));
		#endif
	            Installed_TDTGD108 = FALSE;
	break;
	 #endif 
	 
	#ifdef STTUNER_DRV_TER_TUN_MT2266
	case STTUNER_TUNER_MT2266:
		#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
	            STTBX_Print(("%s uninstalling ter:tuner:MT2266\n", identity));
#endif
	            Installed_MT2266 = FALSE;
	break;
	 #endif 

#ifdef STTUNER_DRV_TER_TUN_DTVS223
case STTUNER_TUNER_DTVS223:
	#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
            STTBX_Print(("%s uninstalling ter:tuner:DTVS223\n", identity));
	#endif
            Installed_DTVS223 = FALSE;
break;
#endif
default:
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
            STTBX_Print(("%s incorrect tuner index", identity));
#endif
           return(ST_ERROR_UNKNOWN_DEVICE);
            break;
    }








    /* unmap API */
    Tuner->ID = STTUNER_NO_DRIVER;

    Tuner->tuner_Init  = NULL;
    Tuner->tuner_Term  = NULL;
    Tuner->tuner_Close = NULL;

    Tuner->tuner_SetFrequency  = NULL;
    Tuner->tuner_GetFrequency  = NULL;
    Tuner->tuner_GetProperties = NULL;
    Tuner->tuner_Select        = NULL;
    Tuner->tuner_GetStatus     = NULL;
    Tuner->tuner_IsTunerLocked = NULL;
    Tuner->tuner_SetBandWidth  = NULL;

    Tuner->tuner_ioaccess = NULL;
    Tuner->tuner_ioctl    = NULL;



    if ( 
    	(Installed_DTT7572    == FALSE) &&
         (Installed_DTT7578    == FALSE) &&
         (Installed_DTT7592    == FALSE)  &&
         (Installed_TDLB7      == FALSE) &&
         (Installed_TDQD3      == FALSE) &&
         (Installed_DTT7300X   == FALSE) &&
         (Installed_ENG47402G1 == FALSE) &&
         (Installed_EAL2780    == FALSE) &&
         (Installed_TDA6650    == FALSE) &&
         (Installed_TDM1316    == FALSE) &&
         (Installed_TDEB2      == FALSE) &&
         (Installed_ED5058      == FALSE) &&
         (Installed_MIVAR      == FALSE) &&
         (Installed_TDED4      == FALSE) &&
         (Installed_DTT7102      == FALSE) &&
         (Installed_TECC2849PG      == FALSE) &&
         (Installed_TDCC2345      == FALSE )&&
         (Installed_TDTGD108      == FALSE )&&
         (Installed_MT2266      == FALSE )&&
         #ifndef ST_OSLINUX
         (Installed_RF4000 	== FALSE)&&
         #endif
         (Installed_DTVS223 == FALSE)  ) 
         
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s <", identity));
#endif

        STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print((">"));
#endif

        InstanceChainTop = (TUNTDRV_InstanceData_t *)0x7ffffffe;
        Installed        = FALSE;
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("all tuntdrv drivers uninstalled\n"));
#endif
    }


#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("ok\n"));
#endif
    return(Error);
}



/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_DTT7572_Install()
        STTUNER_DRV_TUNER_DTT7578_Install()
        STTUNER_DRV_TUNER_DTT7592_Install()
        STTUNER_DRV_TUNER_TDLB7_Install()
        STTUNER_DRV_TUNER_TDQD3_Install()
        STTUNER_DRV_TUNER_ENG47402G1_Install()
        STTUNER_DRV_TUNER_EAL2780_Install()
        STTUNER_DRV_TUNER_TDA6650_Install()
        STTUNER_DRV_TUNER_TDM1316_Install()
        STTUNER_DRV_TUNER_TDEB2_Install()
        STTUNER_DRV_TUNER_ED5058_Install()
        STTUNER_DRV_TUNER_MIVAR_Install()
        STTUNER_DRV_TUNER_TDED4_Install()
        STTUNER_DRV_TUNER_DTT7102_Install()
        STTUNER_DRV_TUNER_TECC2849PG_Install()
        STTUNER_DRV_TUNER_TDCC2345_Install()
        
       
        STTUNER_DRV_TUNER_DTVS223_Install()

Description:
    install a terrestrial device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
	#ifdef STTUNER_DRV_TER_TUN_DTT7572
	ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7572_Install(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_Install(Tuner, STTUNER_TUNER_DTT7572) );
	}
	#endif
	#ifdef STTUNER_DRV_TER_TUN_DTT7578
	ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7578_Install(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_Install(Tuner, STTUNER_TUNER_DTT7578) );
	}
	#endif
	#ifdef STTUNER_DRV_TER_TUN_DTT7592
	ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7592_Install(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_Install(Tuner, STTUNER_TUNER_DTT7592) );
	}
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDLB7
	ST_ErrorCode_t STTUNER_DRV_TUNER_TDLB7_Install(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_Install(Tuner, STTUNER_TUNER_TDLB7) );
	}
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDQD3
	ST_ErrorCode_t STTUNER_DRV_TUNER_TDQD3_Install(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_Install(Tuner, STTUNER_TUNER_TDQD3) );
	}
	#endif
	#ifdef STTUNER_DRV_TER_TUN_DTT7300X
	ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7300X_Install(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_Install(Tuner, STTUNER_TUNER_DTT7300X) );
	}
	
	#endif
	#ifdef STTUNER_DRV_TER_TUN_ENG47402G1
	ST_ErrorCode_t STTUNER_DRV_TUNER_ENG47402G1_Install(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_Install(Tuner, STTUNER_TUNER_ENG47402G1) );
	}
	#endif
	#ifdef STTUNER_DRV_TER_TUN_EAL2780
	ST_ErrorCode_t STTUNER_DRV_TUNER_EAL2780_Install(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_Install(Tuner, STTUNER_TUNER_EAL2780) );
	}
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDA6650
	
	ST_ErrorCode_t STTUNER_DRV_TUNER_TDA6650_Install(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_Install(Tuner, STTUNER_TUNER_TDA6650) );
	}
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDM1316
	ST_ErrorCode_t STTUNER_DRV_TUNER_TDM1316_Install(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_Install(Tuner, STTUNER_TUNER_TDM1316) );
	}
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDEB2
	ST_ErrorCode_t STTUNER_DRV_TUNER_TDEB2_Install(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_Install(Tuner, STTUNER_TUNER_TDEB2) );
	}
	#endif
	#ifdef STTUNER_DRV_TER_TUN_ED5058
	ST_ErrorCode_t STTUNER_DRV_TUNER_ED5058_Install(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_Install(Tuner, STTUNER_TUNER_ED5058) );
	}
	#endif
	#ifdef STTUNER_DRV_TER_TUN_MIVAR
	ST_ErrorCode_t STTUNER_DRV_TUNER_MIVAR_Install(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_Install(Tuner, STTUNER_TUNER_MIVAR) );
	}
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDED4
	ST_ErrorCode_t STTUNER_DRV_TUNER_TDED4_Install(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_Install(Tuner, STTUNER_TUNER_TDED4) );
	}
	#endif
	#ifdef STTUNER_DRV_TER_TUN_DTT7102
	ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7102_Install(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_Install(Tuner, STTUNER_TUNER_DTT7102) );
	}
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TECC2849PG
	ST_ErrorCode_t STTUNER_DRV_TUNER_TECC2849PG_Install(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_Install(Tuner, STTUNER_TUNER_TECC2849PG) );
	}
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDCC2345
	ST_ErrorCode_t STTUNER_DRV_TUNER_TDCC2345_Install(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_Install(Tuner, STTUNER_TUNER_TDCC2345) );
	}
	#endif
	
	#ifdef STTUNER_DRV_TER_TUN_RF4000
	#ifndef ST_OSLINUX
	ST_ErrorCode_t STTUNER_DRV_TUNER_RF4000_Install(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_Install(Tuner, STTUNER_TUNER_RF4000) );
	}
	#endif
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDTGD108
	ST_ErrorCode_t STTUNER_DRV_TUNER_TDTGD108_Install(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_Install(Tuner, STTUNER_TUNER_TDTGD108) );
	}
	#endif

	#ifdef STTUNER_DRV_TER_TUN_MT2266
	ST_ErrorCode_t STTUNER_DRV_TUNER_MT2266_Install(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_Install(Tuner, STTUNER_TUNER_MT2266) );
	}
#endif

#ifdef STTUNER_DRV_TER_TUN_DTVS223
ST_ErrorCode_t STTUNER_DRV_TUNER_DTVS223_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNTDRV_Install(Tuner, STTUNER_TUNER_DTVS223) );
}
#endif

/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_DTT7572_UnInstall()
        STTUNER_DRV_TUNER_DTT7578_UnInstall()
        STTUNER_DRV_TUNER_DTT7592_UnInstall()
        STTUNER_DRV_TUNER_TDLB7_UnInstall()
        STTUNER_DRV_TUNER_TDQD3_UnInstall()
        STTUNER_DRV_TUNER_ENG47402G1_UnInstall()
        STTUNER_DRV_TUNER_EAL2780_UnInstall()
        STTUNER_DRV_TUNER_TDA6650_UnInstall()
        STTUNER_DRV_TUNER_TDM1316_UnInstall()
        STTUNER_DRV_TUNER_TDEB2_UnInstall()
        STTUNER_DRV_TUNER_ED5058_UnInstall()
        STTUNER_DRV_TUNER_MIVAR_UnInstall()
        STTUNER_DRV_TUNER_TDED4_UnInstall()
        STTUNER_DRV_TUNER_DTT7102_UnInstall()
        STTUNER_DRV_TUNER_TECC2849PG_UnInstall()
        STTUNER_DRV_TUNER_TDCC2345_UnInstall()
       
        STTUNER_DRV_TUNER_STB4000_UnInstall()
        STTUNER_DRV_TUNER_DTVS223_UnInstall()
        
Description:
    uninstall a terrestrial device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */

	#ifdef STTUNER_DRV_TER_TUN_DTT7572
	ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7572_UnInstall (STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_UnInstall(Tuner, STTUNER_TUNER_DTT7572) );
	}
	
	#endif
	#ifdef STTUNER_DRV_TER_TUN_DTT7578
	ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7578_UnInstall (STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_UnInstall(Tuner, STTUNER_TUNER_DTT7578) );
	}
	
	#endif
	#ifdef STTUNER_DRV_TER_TUN_DTT7592
	ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7592_UnInstall (STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_UnInstall(Tuner, STTUNER_TUNER_DTT7592) );
	}
	
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDLB7
	ST_ErrorCode_t STTUNER_DRV_TUNER_TDLB7_UnInstall(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_UnInstall(Tuner, STTUNER_TUNER_TDLB7) );
	}
	
	
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDQD3
	ST_ErrorCode_t STTUNER_DRV_TUNER_TDQD3_UnInstall(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_UnInstall(Tuner, STTUNER_TUNER_TDQD3) );
	}
	
	
	#endif
	#ifdef STTUNER_DRV_TER_TUN_DTT7300X
	ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7300X_UnInstall(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_UnInstall(Tuner, STTUNER_TUNER_DTT7300X) );
	}
	
	
	#endif
	#ifdef STTUNER_DRV_TER_TUN_ENG47402G1
	ST_ErrorCode_t STTUNER_DRV_TUNER_ENG47402G1_UnInstall(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_UnInstall(Tuner, STTUNER_TUNER_ENG47402G1) );
	}
	
	#endif
	#ifdef STTUNER_DRV_TER_TUN_EAL2780
	ST_ErrorCode_t STTUNER_DRV_TUNER_EAL2780_UnInstall(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_UnInstall(Tuner, STTUNER_TUNER_EAL2780) );
	}
	
	
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDA6650
	ST_ErrorCode_t STTUNER_DRV_TUNER_TDA6650_UnInstall(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_UnInstall(Tuner, STTUNER_TUNER_TDA6650) );
	}
	
	
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDM1316
	ST_ErrorCode_t STTUNER_DRV_TUNER_TDM1316_UnInstall(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_UnInstall(Tuner, STTUNER_TUNER_TDM1316) );
	}
	
	
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDEB2
	ST_ErrorCode_t STTUNER_DRV_TUNER_TDEB2_UnInstall(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_UnInstall(Tuner, STTUNER_TUNER_TDEB2) );
	}
	
	
	#endif
	#ifdef STTUNER_DRV_TER_TUN_ED5058
	ST_ErrorCode_t STTUNER_DRV_TUNER_ED5058_UnInstall(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_UnInstall(Tuner, STTUNER_TUNER_ED5058) );
	}
	
	
	#endif
	#ifdef STTUNER_DRV_TER_TUN_MIVAR
	ST_ErrorCode_t STTUNER_DRV_TUNER_MIVAR_UnInstall(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_UnInstall(Tuner, STTUNER_TUNER_MIVAR) );
	}
	
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDED4
	ST_ErrorCode_t STTUNER_DRV_TUNER_TDED4_UnInstall(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_UnInstall(Tuner, STTUNER_TUNER_TDED4) );
	}
	
	#endif
	#ifdef STTUNER_DRV_TER_TUN_DTT7102
	ST_ErrorCode_t STTUNER_DRV_TUNER_DTT7102_UnInstall(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_UnInstall(Tuner, STTUNER_TUNER_DTT7102) );
	}
	
	
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TECC2849PG
	ST_ErrorCode_t STTUNER_DRV_TUNER_TECC2849PG_UnInstall(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_UnInstall(Tuner, STTUNER_TUNER_TECC2849PG) );
	}
	
	
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDCC2345
	ST_ErrorCode_t STTUNER_DRV_TUNER_TDCC2345_UnInstall(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_UnInstall(Tuner, STTUNER_TUNER_TDCC2345) );
	}
	
	
	#endif
	
	
	#ifdef STTUNER_DRV_TER_TUN_RF4000
	#ifndef ST_OSLINUX
	ST_ErrorCode_t STTUNER_DRV_TUNER_RF4000_UnInstall(STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_UnInstall(Tuner, STTUNER_TUNER_RF4000) );
	}
	#endif
	#endif
	#ifdef STTUNER_DRV_TER_TUN_TDTGD108
	ST_ErrorCode_t STTUNER_DRV_TUNER_TDTGD108_UnInstall (STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_UnInstall(Tuner, STTUNER_TUNER_TDTGD108) );
	}
	
	#endif
	
	#ifdef STTUNER_DRV_TER_TUN_MT2266
	ST_ErrorCode_t STTUNER_DRV_TUNER_MT2266_UnInstall (STTUNER_tuner_dbase_t *Tuner)
	{
	    return( STTUNER_DRV_TUNER_TUNTDRV_UnInstall(Tuner, STTUNER_TUNER_MT2266) );
	}
	
#endif
#ifdef STTUNER_DRV_TER_TUN_DTVS223
ST_ErrorCode_t STTUNER_DRV_TUNER_DTVS223_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TUNTDRV_UnInstall(Tuner, STTUNER_TUNER_DTVS223) );
}
#endif

/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */



/* ----------------------------------------------------------------------------
Name: tuner_tuntdrv_Init()

Description: called for every perdriver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tuntdrv_Init(ST_DeviceName_t *DeviceName, TUNER_InitParams_t *InitParams)
{


 
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Init()";
#endif


    
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *InstanceNew, *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no driver installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check params ---------- */
    Error = STTUNER_Util_CheckPtrNull(InitParams->MemoryPartition);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull(DeviceName);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail DeviceName not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( TUNTDRV_InstanceData_t ));
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
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
    InstanceNew->TunerType           = InitParams->TunerType;
    InstanceNew->DeviceMap.MemoryPartition = InitParams->MemoryPartition;
    

  
     switch(InstanceNew->TunerType)
    {
       #ifdef STTUNER_DRV_TER_TUN_DTT7572
       case STTUNER_TUNER_DTT7572:
            InstanceNew->PLLType = TUNER_PLL_DTT7572;
          break;  
        #endif

        #ifdef STTUNER_DRV_TER_TUN_DTT7578
        case STTUNER_TUNER_DTT7578:
            InstanceNew->PLLType = TUNER_PLL_DTT7578;
          break;  
         #endif

        #ifdef STTUNER_DRV_TER_TUN_DTT7592
                case STTUNER_TUNER_DTT7592:
            InstanceNew->PLLType = TUNER_PLL_DTT7592;
          break;  
         #endif
        #ifdef STTUNER_DRV_TER_TUN_TDLB7
                case STTUNER_TUNER_TDLB7:
            InstanceNew->PLLType = TUNER_PLL_TDLB7;
           break; 
          #endif
        #ifdef STTUNER_DRV_TER_TUN_TDQD3
        case STTUNER_TUNER_TDQD3:
            InstanceNew->PLLType = TUNER_PLL_TDQD3;
          break;  
         #endif
 	#ifdef STTUNER_DRV_TER_TUN_DTT7300X
	case STTUNER_TUNER_DTT7300X:
            InstanceNew->PLLType = TUNER_PLL_DTT7300X;
            break;

	#endif
        #ifdef STTUNER_DRV_TER_TUN_ENG47402G1
                case STTUNER_TUNER_ENG47402G1:
            InstanceNew->PLLType = TUNER_PLL_ENG47402G1;
          break;  
         #endif
        #ifdef STTUNER_DRV_TER_TUN_EAL2780
                case STTUNER_TUNER_EAL2780:
            InstanceNew->PLLType = TUNER_PLL_EAL2780;
          break;  
          #endif
        #ifdef STTUNER_DRV_TER_TUN_TDA6650
        case STTUNER_TUNER_TDA6650:
            InstanceNew->PLLType = TUNER_PLL_TDA6650;
          break;  
         #endif
       #ifdef STTUNER_DRV_TER_TUN_TDM1316
       case STTUNER_TUNER_TDM1316:
            InstanceNew->PLLType = TUNER_PLL_TDM1316;
          break;  
         #endif
            
        #ifdef STTUNER_DRV_TER_TUN_TDEB2
        case STTUNER_TUNER_TDEB2:
            InstanceNew->PLLType = TUNER_PLL_TDEB2;
          break;  
         #endif
        #ifdef STTUNER_DRV_TER_TUN_ED5058
        case STTUNER_TUNER_ED5058:
            InstanceNew->PLLType = TUNER_PLL_ED5058;
          break;  
         #endif
        #ifdef STTUNER_DRV_TER_TUN_MIVAR
                case STTUNER_TUNER_MIVAR:
            InstanceNew->PLLType = TUNER_PLL_MIVAR;
           break; 
            #endif
        #ifdef STTUNER_DRV_TER_TUN_TDED4
        case STTUNER_TUNER_TDED4:
            InstanceNew->PLLType = TUNER_PLL_TDED4;
            break;
             #endif
        #ifdef STTUNER_DRV_TER_TUN_DTT7102
        case STTUNER_TUNER_DTT7102:
            InstanceNew->PLLType = TUNER_PLL_DTT7102;
            break;
            #endif
            
        #ifdef STTUNER_DRV_TER_TUN_TECC2849PG
        case STTUNER_TUNER_TECC2849PG:
            InstanceNew->PLLType = TUNER_PLL_TECC2849PG;
           break; 
             #endif
            
        #ifdef STTUNER_DRV_TER_TUN_TDCC2345
        case STTUNER_TUNER_TDCC2345:
            InstanceNew->PLLType = TUNER_PLL_TDCC2345;
           break; 
             #endif
        
        
        #ifdef STTUNER_DRV_TER_TUN_RF4000
        #ifndef ST_OSLINUX
        case STTUNER_TUNER_RF4000:
            InstanceNew->PLLType = TUNER_PLL_RF4000;
          break;  
        #endif  
        #endif
        #ifdef STTUNER_DRV_TER_TUN_TDTGD108
       case STTUNER_TUNER_TDTGD108:
            InstanceNew->PLLType = TUNER_PLL_TDTGD108;
          break;  
        #endif
            
         #ifdef STTUNER_DRV_TER_TUN_MT2266
       case STTUNER_TUNER_MT2266:
            InstanceNew->PLLType = TUNER_PLL_MT2266;
          break;  
#endif            
            
        #ifdef STTUNER_DRV_TER_TUN_DTVS223
         case STTUNER_TUNER_DTVS223:
         InstanceNew->PLLType = TUNER_PLL_DTVS223;
          break;
       #endif
       default:
        #ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
            STTBX_Print(("%s incorrect tuner index", identity));
	#endif
                  return(ST_ERROR_UNKNOWN_DEVICE);
            break;
    }
      
    
   
    
    

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes) for  tuner ID=%d\n", identity, InstanceNew->DeviceName, InstanceNew, sizeof( TUNTDRV_InstanceData_t ), InstanceNew->TunerType ));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
   
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: tuner_tuntdrv_Term()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tuntdrv_Term(ST_DeviceName_t *DeviceName, TUNER_TermParams_t *TermParams)
{




#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
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
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }


    /* reap next matching DeviceName */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print(("Looking (%s)", Instance->DeviceName));
#endif
    while(1)
    {
        if ( strcmp((char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
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
            /*Deallocate memory for ioreg for  stb4000*/
         switch (Instance->TunerType)
         {
         #if  defined (STTUNER_DRV_TER_TUN_RF4000) || defined (STTUNER_DRV_TER_TUN_TDQD3) || defined (STTUNER_DRV_TER_TUN_DTT7300X)|| defined (STTUNER_DRV_TER_TUN_TDTGD108) || defined (STTUNER_DRV_TER_TUN_MT2266)
         case STTUNER_TUNER_TDQD3: 
         case STTUNER_TUNER_DTT7300X:      
         case STTUNER_TUNER_TDTGD108:
         case STTUNER_TUNER_MT2266:
         #ifndef ST_OSLINUX
         case STTUNER_TUNER_RF4000:
         #endif
         #endif

   #if defined(STTUNER_DRV_TER_TUN_DTVS223)
         case STTUNER_TUNER_DTVS223:
#endif    


     	    #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
             if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	          {
            Error = STTUNER_IOREG_Close(&Instance->DeviceMap);
            }
             if (Error != ST_NO_ERROR)
            {
	          #ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
            STTBX_Print(("%s fail deallocate register database\n", identity));
	         #endif
           SEM_UNLOCK(Lock_InitTermOpenClose);
           return(Error);           
            }
           #endif
           
           
          
             break;
          default:        
             break;
        }
        
        #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
        STOS_MemoryDeallocate(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition, IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream);
        #endif 
            
       STOS_MemoryDeallocate(Instance->MemoryPartition, Instance);
            
		STOS_MemoryDeallocate(Instance->MemoryPartition, Instance->TunerRegVal);
		
            
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
            STTBX_Print(("%s terminated ok\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL)
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
                STTBX_Print(("\n%s fail no free handle before end of list\n", identity));
#endif
                SEM_UNLOCK(Lock_InitTermOpenClose);
                return(STTUNER_ERROR_INITSTATE);
        }
        else
        {
            Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
            STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        }

    }


#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);

    return(Error);
}


/* ----------------------------------------------------------------------------
Name: tuner_tuntdrv_Open()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tuntdrv_Open(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
#endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL)
        {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print((" found ok\n"));
#endif

    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail driver instance already open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    /* now got pointer to free (and valid) data block */
     Instance->TopLevelHandle = OpenParams->TopLevelHandle;
    *Handle = (U32)Instance;



#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print(("%s opened ok\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}


/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_DTT7572()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */

#ifdef STTUNER_DRV_TER_TUN_DTT7572
ST_ErrorCode_t tuner_tuntdrv_Open_DTT7572(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open_DTT7572()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;

    Error = tuner_tuntdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TUNTDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_DTT7572)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_DTT7572 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif
    
    LL_TunerInit(Instance);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail\n", identity));
#endif
    return(Error);
}

#endif

/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_DTT7578()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_TER_TUN_DTT7578
ST_ErrorCode_t tuner_tuntdrv_Open_DTT7578(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open_DTT7578()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;

    Error = tuner_tuntdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TUNTDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_DTT7578)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_DTT7578 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
      #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif
    LL_TunerInit(Instance);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail\n", identity));
#endif

    return(Error);
}
#endif
/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_DTT7592()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_TER_TUN_DTT7592
ST_ErrorCode_t tuner_tuntdrv_Open_DTT7592(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open_DTT7578()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;

    Error = tuner_tuntdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TUNTDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_DTT7592)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_DTT7592 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }



      #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif
    LL_TunerInit(Instance);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail\n", identity));
#endif

    return(Error);
}

#endif
/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_TDLB7()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_TER_TUN_TDLB7
ST_ErrorCode_t tuner_tuntdrv_Open_TDLB7(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open_TDLB7()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;

    Error = tuner_tuntdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TUNTDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_TDLB7)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_TDLB7 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
      #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif
    LL_TunerInit(Instance);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail\n", identity));
#endif

    return(Error);
}

#endif
/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_TDQD3()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_TER_TUN_TDQD3
ST_ErrorCode_t tuner_tuntdrv_Open_TDQD3(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open_TDQD3()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;
    int  i;
    
    Error = tuner_tuntdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }
     SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNTDRV_GetInstFromHandle(*Handle);
    if (Instance->TunerType != STTUNER_TUNER_TDQD3)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_TDQD3 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    Instance->TunerRegVal = STOS_MemoryAllocateClear(Instance->MemoryPartition,  TDQD3_NBREGS, sizeof( U8 ));
  
    
for ( i=0;i<TDQD3_NBREGS;i++)

	{
	Instance->TunerRegVal[i]=TDQD3_DefVal[i];
	
	}

   Instance->Status.IntermediateFrequency =36000;
   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
   Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   Instance->DeviceMap.Registers = TDQD3_NBREGS;
   Instance->DeviceMap.Fields = TDQD3_NBFIELDS;
   Instance->DeviceMap.Mode = IOREG_MODE_NOSUBADR;
   Instance->DeviceMap.WrStart =RTDQD3_DIV1;
   Instance->DeviceMap.WrSize = 5;
   Instance->DeviceMap.RdStart = RTDQD3_STATUS;
   Instance->DeviceMap.RdSize =1;
   Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{
   Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle,TDQD3_DefVal,TDQD3_Address);
   }
   #endif
   #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
  IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = TDQD3_NBREGS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields    = TDQD3_NBFIELDS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart =RTDQD3_DIV1;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 5;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdStart = RTDQD3_STATUS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,TDQD3_NBREGS+1,sizeof(U8));
   if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
   {
   Error = ChipSetRegisters(Instance->IOHandle,0,TDQD3_DefVal,TDQD3_NBREGS);
   }
   #endif
   
   
   
        /* now safe to unlock semaphore */
        SEM_UNLOCK(Lock_InitTermOpenClose);
    
        if (Error == ST_NO_ERROR)
        {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
        }
     else
       {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
       }
    return(Error);
}
#endif
/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_DTT7300X()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_TER_TUN_DTT7300X
ST_ErrorCode_t tuner_tuntdrv_Open_DTT7300X(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open_DTT7300X()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;

    U32             i;
    
    Error = tuner_tuntdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }
     SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNTDRV_GetInstFromHandle(*Handle);
    if (Instance->TunerType != STTUNER_TUNER_DTT7300X)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_DTT7300X for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    
    Instance->TunerRegVal = STOS_MemoryAllocateClear(Instance->MemoryPartition,  DTT7300X_NBREGS, sizeof( U8 ));
  
    
for ( i=0;i<DTT7300X_NBREGS;i++)

	{
	Instance->TunerRegVal[i]=DTT7300X_DefVal[i];
	
	}
   Instance->Status.IntermediateFrequency =36000;
   Instance->ChannelBW = STTUNER_CHAN_BW_8M;
   
   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
    Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
    Instance->DeviceMap.Registers = DTT7300X_NBREGS;
    Instance->DeviceMap.Fields    = DTT7300X_NBFIELDS;
    Instance->DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    Instance->DeviceMap.WrStart =RDTT7300X_P_DIV1;
    Instance->DeviceMap.WrSize = 4;
    Instance->DeviceMap.RdStart = RDTT7300X_STATUS;
    Instance->DeviceMap.RdSize =1;
    Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   	{
   Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle,DTT7300X_DefVal,DTT7300X_Address);
   }
    #endif
    
     #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = DTT7300X_NBREGS;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields    = DTT7300X_NBFIELDS;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart =RDTT7300X_P_DIV1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdStart = RDTT7300X_STATUS;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
     IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,DTT7300X_NBREGS+1,sizeof(U8));
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
     	{
     		 Error = ChipSetRegisters(Instance->IOHandle,0,DTT7300X_DefVal,DTT7300X_NBREGS);
        }
    #endif
  
   if (Error == ST_NO_ERROR)
        {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif 
        }
     else
       {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
       }
       
       /* now safe to unlock semaphore */
       SEM_UNLOCK(Lock_InitTermOpenClose);
       
       
    return(Error);
    
}
   
#endif
/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_ENG47402G1()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_TER_TUN_ENG47402G1
ST_ErrorCode_t tuner_tuntdrv_Open_ENG47402G1(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open_ENG47402G1()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;

    Error = tuner_tuntdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TUNTDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_ENG47402G1)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_ENG47402G1 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
      #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif
    LL_TunerInit(Instance);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail\n", identity));
#endif

    return(Error);
}

#endif
/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_EAL2780()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_TER_TUN_EAL2780
ST_ErrorCode_t tuner_tuntdrv_Open_EAL2780(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open_EAL2780()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;

    Error = tuner_tuntdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TUNTDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_EAL2780)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_EAL2780 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
   #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif
    LL_TunerInit(Instance);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail\n", identity));
#endif

    return(Error);
}

#endif
/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_TDA6650()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_TER_TUN_TDA6650
ST_ErrorCode_t tuner_tuntdrv_Open_TDA6650(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open_TDA6650()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;

    Error = tuner_tuntdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TUNTDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_TDA6650)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_TDA6650 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif
    LL_TunerInit(Instance);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail\n", identity));
#endif

    return(Error);
}

#endif
/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_TDM1316()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_TER_TUN_TDM1316
ST_ErrorCode_t tuner_tuntdrv_Open_TDM1316(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open_TDM1316()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;

    Error = tuner_tuntdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TUNTDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_TDM1316)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_TDM1316 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
  #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif
    LL_TunerInit(Instance);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail\n", identity));
#endif

    return(Error);
}

#endif
/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_TDEB2()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_TER_TUN_TDEB2
ST_ErrorCode_t tuner_tuntdrv_Open_TDEB2(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open_TDEB2()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;

    Error = tuner_tuntdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TUNTDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_TDEB2)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_TDEB2 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif
    LL_TunerInit(Instance);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail\n", identity));
#endif

    return(Error);
}

#endif
/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_ED5058()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_TER_TUN_ED5058
ST_ErrorCode_t tuner_tuntdrv_Open_ED5058(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open_ED5058()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;

    Error = tuner_tuntdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TUNTDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_ED5058)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_ED5058 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
   #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif
    LL_TunerInit(Instance);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail\n", identity));
#endif

    return(Error);
}

#endif
/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_MIVAR()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_TER_TUN_MIVAR
ST_ErrorCode_t tuner_tuntdrv_Open_MIVAR(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open_MIVAR()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;

    Error = tuner_tuntdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TUNTDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_MIVAR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_MIVAR for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
   #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif
    LL_TunerInit(Instance);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail\n", identity));
#endif

    return(Error);
}

#endif
/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_TDED4()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_TER_TUN_TDED4
ST_ErrorCode_t tuner_tuntdrv_Open_TDED4(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open_TDED4()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;

    Error = tuner_tuntdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TUNTDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_TDED4)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_TDED4 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
   #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif
    LL_TunerInit(Instance);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail\n", identity));
#endif

    return(Error);
}
#endif
/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_DTT7102()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_TER_TUN_DTT7102
ST_ErrorCode_t tuner_tuntdrv_Open_DTT7102(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open_DTT7102()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;

    Error = tuner_tuntdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TUNTDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_DTT7102)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_DTT7102 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
   #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif
    LL_TunerInit(Instance);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail\n", identity));
#endif

    return(Error);
}
#endif
/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_TECC2849PG()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_TER_TUN_TECC2849PG
ST_ErrorCode_t tuner_tuntdrv_Open_TECC2849PG(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open_TECC2849PG()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;

    Error = tuner_tuntdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TUNTDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_TECC2849PG)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_TECC2849PG for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif
    LL_TunerInit(Instance);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail\n", identity));
#endif

    return(Error);
}

#endif
/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_TDCC2345()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_TER_TUN_TDCC2345
ST_ErrorCode_t tuner_tuntdrv_Open_TDCC2345(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open_TDCC2345()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;

    Error = tuner_tuntdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TUNTDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_TDCC2345)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_TDCC2345 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
   #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif
    LL_TunerInit(Instance);

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail\n", identity));
#endif

    return(Error);
}

#endif


#ifdef STTUNER_DRV_TER_TUN_RF4000
#ifndef ST_OSLINUX

/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_RF4000()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tuntdrv_Open_RF4000(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open_RF4000()";
#endif
	
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;
    Error = tuner_tuntdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNTDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_RF4000)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_RF4000 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
     
   
    
   Instance->StepSize =1000000;/*Step size is hard coded here. ?*/
   Instance->IF = 36000;
   Instance->ChannelBW = STTUNER_CHAN_BW_8M;/*Give enum values for this */
   Instance->FreqFactor = 1;
   Instance->I_Q= STTUNER_IQ_MODE_NORMAL;
    #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
   Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   Instance->DeviceMap.Registers = RF4000_NBREGS;
   Instance->DeviceMap.Fields = RF4000_NBFIELDS;
   Instance->DeviceMap.Mode =IOREG_MODE_SUBADR_8_NS;
   
   Instance->DeviceMap.RdStart = RRF4000_REGISTER0;
   Instance->DeviceMap.RdSize =15; 
   Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));
   
   #endif
   
   #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = RF4000_NBREGS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields    = RF4000_NBFIELDS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_SUBADR_8_NS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdStart = RRF4000_REGISTER0;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =15;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,RF4000_NBREGS+1,sizeof(U8));
  #endif
 
   
   Error |= RF4000_doInit (Instance);

   /* now safe to unlock semaphore */

   SEM_UNLOCK(Lock_InitTermOpenClose);
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail\n", identity));
#endif

    return(Error);
}
#endif
#endif


/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_TDTGD108()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_TER_TUN_TDTGD108
ST_ErrorCode_t tuner_tuntdrv_Open_TDTGD108(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open_TDTGD108()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;
    int  i;
    
    Error = tuner_tuntdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }
     SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNTDRV_GetInstFromHandle(*Handle);
    if (Instance->TunerType != STTUNER_TUNER_TDTGD108)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_TDTGD108 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    Instance->TunerRegVal = STOS_MemoryAllocateClear(Instance->MemoryPartition,  TDTGD108_NBREGS, sizeof( U8 ));
  
    
for ( i=0;i<TDTGD108_NBREGS;i++)

	{
	Instance->TunerRegVal[i]=TDTGD108_DefVal[i];
	
	}

   Instance->Status.IntermediateFrequency =36167;
   Instance->ChannelBW = STTUNER_CHAN_BW_8M;
    #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
  Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   Instance->DeviceMap.Registers = TDTGD108_NBREGS;
   Instance->DeviceMap.Fields = TDTGD108_NBFIELDS;
   Instance->DeviceMap.Mode = IOREG_MODE_NOSUBADR;
   Instance->DeviceMap.WrStart =RTDTGD108_P_DIV1;
   Instance->DeviceMap.WrSize = 4;
   Instance->DeviceMap.RdStart = RTDTGD108_STATUS;
   Instance->DeviceMap.RdSize =1;
   Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    	{
       Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle,TDTGD108_DefVal,TDTGD108_Address);
       }
   #endif
   #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = TDTGD108_NBREGS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields    = TDTGD108_NBFIELDS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart =RTDTGD108_P_DIV1;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdStart = RTDTGD108_STATUS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;  
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,TDTGD108_NBREGS+1,sizeof(U8));
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    	{
        Error = ChipSetRegisters(Instance->IOHandle,0,TDTGD108_DefVal,TDTGD108_NBREGS);
       }
   #endif
   
 
        
        /* now safe to unlock semaphore */
        SEM_UNLOCK(Lock_InitTermOpenClose);
    
        if (Error == ST_NO_ERROR)
        {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
        }
     else
       {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
       }
    return(Error);
}
#endif

/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_MT2266()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_TER_TUN_MT2266
ST_ErrorCode_t tuner_tuntdrv_Open_MT2266(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open_MT2266()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;
    int  i;
    
    Error = tuner_tuntdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }
     SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNTDRV_GetInstFromHandle(*Handle);
    if (Instance->TunerType != STTUNER_TUNER_MT2266)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_MT2266 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    Instance->TunerRegVal = memory_allocate_clear(Instance->MemoryPartition,  MT2266_NBREGS, sizeof( U8 ));
    
          
    
for ( i=0;i<MT2266_NBREGS;i++)

	{
	Instance->TunerRegVal[i]=MT2266_DefVal[i];
	
	}

   Instance->Status.IntermediateFrequency =36000;
    #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
   Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   Instance->DeviceMap.Registers = MT2266_NBREGS;
   Instance->DeviceMap.Fields = MT2266_NBFIELDS;
   Instance->DeviceMap.Mode = IOREG_MODE_SUBADR_8_NS_MICROTUNE;
   Instance->DeviceMap.WrStart =RMT2266_LO_CTRL_1;
   Instance->DeviceMap.WrSize = 59;
   Instance->DeviceMap.RdStart = RMT2266_PART_REV;
   Instance->DeviceMap.RdSize =60;
   Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));
   #endif
   
   #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = MT2266_NBREGS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields    = MT2266_NBFIELDS;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_SUBADR_8_NS_MICROTUNE;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart =RMT2266_LO_CTRL_1;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 59;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdStart = RMT2266_PART_REV;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =60;
   IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,MT2266_NBREGS+1,sizeof(U8));
   #endif
   
   Instance->ChannelBW = STTUNER_CHAN_BW_8M;
   Instance->MT2266_Info.tuner_id = Instance->TunerRegVal[MT2266_PART_REV];
   Instance->MT2266_Info.f_Ref = REF_FREQ;
   Instance->MT2266_Info.f_Step=Instance->StepSize = TUNE_STEP_SIZE * 1000;  /* kHz -> Hz */   
   Instance->MT2266_Info.f_in = UHF_DEFAULT_FREQ;
   Instance->MT2266_Info.f_LO = UHF_DEFAULT_FREQ;
   Instance->MT2266_Info.f_bw = OUTPUT_BW;
   Instance->MT2266_Info.band = MT2266_UHF_BAND;
   Instance->MT2266_Info.LNAConfig =15; /*check all the parameter values  */
   Instance->MT2266_Info.UHFSens = 0;
   
   
 
   Error= MT2266_ReInit(Instance);
   
   
   
        /* now safe to unlock semaphore */
        SEM_UNLOCK(Lock_InitTermOpenClose);
    
        if (Error == ST_NO_ERROR)
        {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
        }
     else
       {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
       }
    return(Error);
}
#endif

/* ----------------------------------------------------------------------------
Name:   tuner_tuntdrv_Open_DTVS223()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_TER_TUN_DTVS223
ST_ErrorCode_t tuner_tuntdrv_Open_DTVS223(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Open_DTVS223()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;
    U32     i;
  
    Error = tuner_tuntdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }
     SEM_LOCK(Lock_InitTermOpenClose);
    Instance = TUNTDRV_GetInstFromHandle(*Handle);
    if (Instance->TunerType != STTUNER_TUNER_DTVS223)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_DTVS223 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    
     Instance->TunerRegVal = STOS_MemoryAllocateClear(Instance->MemoryPartition,  DTVS223_NBREGS, sizeof( U8 ));

for ( i=0;i<DTVS223_NBREGS;i++)

	{
	Instance->TunerRegVal[i]=DTVS223_DefVal[i];
	
	}
	#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
   Instance->Status.IntermediateFrequency =44000;
   Instance->DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
   Instance->DeviceMap.Registers = DTVS223_NBREGS;
   Instance->DeviceMap.Fields =DTVS223_NBFIELDS;
   Instance->DeviceMap.Mode = IOREG_MODE_NOSUBADR;
   Instance->DeviceMap.WrStart =RDTVS223_DIV1;
   Instance->DeviceMap.WrSize = 5;
   Instance->DeviceMap.RdStart = RDTVS223_STATUS;
   Instance->DeviceMap.RdSize =1;
   Error = STTUNER_IOREG_Open(&(Instance->DeviceMap));
   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL) 
   	{ 
     Error = STTUNER_IOREG_Reset(&(Instance->DeviceMap), Instance->IOHandle,DTVS223_DefVal,DTVS223_Address);
    }
   #endif
   
 
 

   
    #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = IOREG_DEFAULT_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Registers = DTVS223_NBREGS;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Fields    = DTVS223_NBFIELDS;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrStart =RDTVS223_DIV1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 5;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdStart = RDTVS223_STATUS;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition = Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,DTT7300X_NBREGS+1,sizeof(U8));
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
    	{
       Error = ChipSetRegisters(Instance->IOHandle,0,DTVS223_DefVal,DTVS223_NBREGS);
      }
    #endif

   
   
    /* now safe to unlock semaphore */
  SEM_UNLOCK(Lock_InitTermOpenClose);
    
  if (Error == ST_NO_ERROR)
   {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }
    return(Error);
}
#endif
/* ----------------------------------------------------------------------------
Name: tuner_tuntdrv_Close()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tuntdrv_Close(TUNER_Handle_t Handle, TUNER_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Close()";
#endif

    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;

    Instance = TUNTDRV_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print(("%s closed\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);

}
/*****************************************************
**FUNCTION  ::  tuner_tuntdrv_GetStatus
**ACTION	::	Get the status of tuner
**PARAMS IN	::	NONE
**PARAMS OUT::	tnr	==> tuner properties
**RETURN	::	NONE
*****************************************************/

ST_ErrorCode_t tuner_tuntdrv_GetStatus(TUNER_Handle_t Handle, TUNER_Status_t *Status)
{
    TUNTDRV_InstanceData_t *Instance;
    ST_ErrorCode_t Error= ST_NO_ERROR;

    Instance = TUNTDRV_GetInstFromHandle(Handle);

    switch (Instance->TunerType)
    {
        #if defined (STTUNER_DRV_TER_TUN_TDQD3) || defined (STTUNER_DRV_TER_TUN_DTT7300X) || defined (STTUNER_DRV_TER_TUN_RF4000)|| defined (STTUNER_DRV_TER_TUN_TDTGD108) || defined(STTUNER_DRV_TER_TUN_MT2266)
        case STTUNER_TUNER_TDQD3:
        case STTUNER_TUNER_DTT7300X:
        case STTUNER_TUNER_TDTGD108:
      
        #ifndef ST_OSLINUX
        case STTUNER_TUNER_RF4000:
        #endif
        case STTUNER_TUNER_MT2266:
        #endif

  #if defined (STTUNER_DRV_TER_TUN_DTVS223)

    
        case STTUNER_TUNER_DTVS223:
#endif        	
 		    *Status = Instance->Status;
 		    Error=ST_NO_ERROR;
 
        break;
        
 

        default:
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
            STTBX_Print(("WARNING - no TUNER specified\n"));
#endif
        break;
    }
    return(Error);
        
}

/*****************************************************
**FUNCTION  ::  tuner_tuntdrv_GetProperties
**ACTION	::	Get the properties of the tuner
**PARAMS IN	::	NONE
**PARAMS OUT::	tnr	==> tuner properties
**RETURN	::	NONE
*****************************************************/
void tuner_tuntdrv_GetProperties(TUNER_Handle_t Handle, void *ptnr)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_GetProperties()";
#endif

 
    TUNTDRV_InstanceData_t *tnr;
    TUNTDRV_InstanceData_t *Instance;

    Instance = TUNTDRV_GetInstFromHandle(Handle);
   
    tnr = (TUNTDRV_InstanceData_t *)ptnr;
    if(!Instance->Error)
	{
        LL_TunerGetProperties(Instance, tnr);
    }
   

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print(("%s ok\n", identity));
#endif
}


/*****************************************************
**FUNCTION  ::  tuner_tuntdrv_Select
**ACTION	::	Select the type of tuner used
**PARAMS IN	::	type	==> type of the tuner
**				address	==> I2C address of the tuner
**PARAMS OUT::	NONE
**RETURN	::	NONE
*****************************************************/
void tuner_tuntdrv_Select(TUNER_Handle_t Handle, STTUNER_TunerType_t type, unsigned char address)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Select()";
#endif

 
    TUNTDRV_InstanceData_t *Instance;

    Instance = TUNTDRV_GetInstFromHandle(Handle);

    if(!Instance->Error)
	{
        LL_TunerSelect(Instance, type, address);
    }

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print(("%s ok\n", identity));
#endif
}



/*****************************************************
**FUNCTION  ::  tuner_tuntdrv_GetFrequency
**ACTION	::	Get the frequency of the tuner
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE
**RETURN	::	frequency of the tuner (KHz), 0 if an error occur
*****************************************************/
ST_ErrorCode_t tuner_tuntdrv_GetFrequency(TUNER_Handle_t Handle, U32 *Freq)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_GetFrequency()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;

    Instance = TUNTDRV_GetInstFromHandle(Handle);

    if(!Instance->Error)
	{
        *Freq = LL_TunerGetFrequency(Instance);
    }
  

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print(("%s ok\n", identity));
#endif

    return(Error);
}


/*****************************************************
**FUNCTION  ::  tuner_tuntdrv_SetFrequency
**ACTION	::	Set the frequency of the tuner
**PARAMS IN	::	frequency ==> the frequency of the tuner (KHz)
**PARAMS OUT::	NONE
**RETURN	::	real tuner frequency, 0 if an error occur
*****************************************************/
ST_ErrorCode_t tuner_tuntdrv_SetFrequency (TUNER_Handle_t Handle, U32 Frequency, U32 *NewFrequency)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_SetFrequency()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;
    
     #ifdef STTUNER_DRV_TER_TUN_DTT7300X
     S32 atc;
     #endif  
     #if defined(STTUNER_DRV_TER_TUN_DTT7300X)|| defined(STTUNER_DRV_TER_TUN_DTVS223)|| defined (STTUNER_DRV_TER_TUN_TDQD3)|| defined (STTUNER_DRV_TER_TUN_RF4000)|| defined (STTUNER_DRV_TER_TUN_TDTGD108) || defined(STTUNER_DRV_TER_TUN_MT2266)
     U32 frequency;
      #endif
     #if defined(STTUNER_DRV_TER_TUN_DTT7300X)|| defined(STTUNER_DRV_TER_TUN_DTVS223)|| defined (STTUNER_DRV_TER_TUN_TDQD3) || defined (STTUNER_DRV_TER_TUN_TDTGD108)
     U32 divider,value1,value2;
     #endif    

 #ifdef STTUNER_DRV_TER_TUN_MT2266
    U32 mt_ferror=0,mt_id;	 
    #endif

    Instance = TUNTDRV_GetInstFromHandle(Handle);
    if(!Instance->Error)
	{
	   switch(Instance->TunerType)
	   {
	   	#if  defined (STTUNER_DRV_TER_TUN_TDQD3) || defined (STTUNER_DRV_TER_TUN_DTT7300X) || defined (STTUNER_DRV_TER_TUN_RF4000) || defined (STTUNER_DRV_TER_TUN_TDTGD108)|| defined(STTUNER_DRV_TER_TUN_MT2266)
	     	
	   	case  STTUNER_TUNER_TDQD3:
	   	case  STTUNER_TUNER_DTT7300X:
	   	case  STTUNER_TUNER_TDTGD108:
	   	#ifndef ST_OSLINUX
	   	case STTUNER_TUNER_RF4000:
	   	#endif
	   	case STTUNER_TUNER_MT2266:
		#endif

#if  defined (STTUNER_DRV_TER_TUN_DTVS223) 
	   	case  STTUNER_TUNER_DTVS223:
#endif	   		
	   	break;
	   	default:			
        LL_TunerSetFrequency(Instance, Frequency, NewFrequency);
        break;
            }
         }
 
  
   if(!Instance->Error)
      {
      	switch(Instance->TunerType)
		{
			
			#ifdef STTUNER_DRV_TER_TUN_DTVS223
			case STTUNER_TUNER_DTVS223:
			        frequency = Frequency +(Instance->Status.IntermediateFrequency);
				divider = (frequency * 100) / (TunerGetStepsize(Instance)  / 10);
			
				
				ChipSetFieldImage(FDTVS223_N_MSB,((divider >> 8) & 0x7F),Instance->TunerRegVal);
				
				
				ChipSetFieldImage(FDTVS223_N_LSB,((divider ) & 0xFF),Instance->TunerRegVal);
				
				if( Frequency<=170000)
				{
				
					ChipSetFieldImage(FDTVS223_BS21,1,Instance->TunerRegVal);
					ChipSetFieldImage(FDTVS223_BS4,0,Instance->TunerRegVal);
				
				}
				else  if( Frequency<=470000) 
				{
					
					ChipSetFieldImage(FDTVS223_BS21,2,Instance->TunerRegVal);
				
					ChipSetFieldImage(FDTVS223_BS4,0,Instance->TunerRegVal);
				
				}
				else  if( Frequency<=861000)
				{
					
					ChipSetFieldImage(FDTVS223_BS21,0,Instance->TunerRegVal);
					ChipSetFieldImage(FDTVS223_BS4,1,Instance->TunerRegVal);
				
				}
				
				 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{  
				    Error =ChipSetRegisters(Instance->IOHandle,RDTVS223_DIV1,Instance->TunerRegVal,5);
				   }
				#endif
				#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			   	   {
			   	   	Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RDTVS223_DIV1,Instance->TunerRegVal,5);
			   	  }
			   	 #endif
			   	  
			   	  
				if( Error != ST_NO_ERROR)
    				  {
				  #ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        				STTBX_Print(("%s fail, error=%d\n", identity, Error));
				 #endif
        
        				return(Error);
    				  }
				/*Get Frequency*/
				divider = 0;
				frequency =0;
				     
				
				value1=ChipGetFieldImage(FDTVS223_N_MSB,Instance->TunerRegVal)<<8;
		 		value2=ChipGetFieldImage(FDTVS223_N_LSB,Instance->TunerRegVal);
		 
				 divider=value1+value2;
				frequency = (divider*TunerGetStepsize(Instance) )/1000 - Instance->Status.IntermediateFrequency;
				*NewFrequency = Instance->Status.Frequency = frequency;
				break;
				#endif
				#ifdef STTUNER_DRV_TER_TUN_TDQD3
			case STTUNER_TUNER_TDQD3:
			        frequency = Frequency +(Instance->Status.IntermediateFrequency);
				divider = (frequency * 100) / (TunerGetStepsize(Instance) / 10);
			
				ChipSetFieldImage(FTDQD3_N_MSB,((divider >> 8) & 0x7F),Instance->TunerRegVal);
				ChipSetFieldImage(FTDQD3_N_LSB,((divider ) & 0xFF),Instance->TunerRegVal);
			

				if 	(Frequency <= 230000) /*Check which frequency u need to  check??? freq+IF or freq???*/
				{
					ChipSetFieldImage(FTDQD3_CP ,0x02,Instance->TunerRegVal);
					ChipSetFieldImage(FTDQD3_BS21 ,0x02,Instance->TunerRegVal);
					ChipSetFieldImage(FTDQD3_BS4  ,0x00,Instance->TunerRegVal);
				}
				
				else if (Frequency <= 782000) 
				{
					ChipSetFieldImage(FTDQD3_CP,0x03,Instance->TunerRegVal);
					ChipSetFieldImage(FTDQD3_BS21,0x00,Instance->TunerRegVal);
					ChipSetFieldImage(FTDQD3_BS4,0x01,Instance->TunerRegVal);  
				}
				
				else if (Frequency <= 862000)
				{
					ChipSetFieldImage(FTDQD3_CP,0x00,Instance->TunerRegVal);
					ChipSetFieldImage(FTDQD3_BS21,0x00,Instance->TunerRegVal);
					ChipSetFieldImage(FTDQD3_BS4,0x01,Instance->TunerRegVal);
				}
              #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{  
				Error =ChipSetRegisters(Instance->IOHandle,RTDQD3_DIV1,Instance->TunerRegVal,5);
			    }
			    #endif
			   #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			   	{
			    Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RTDQD3_DIV1,Instance->TunerRegVal,5);
			    }
			    #endif
				if( Error != ST_NO_ERROR)
    				  {
				  #ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        				STTBX_Print(("%s fail, error=%d\n", identity, Error));
				 #endif
        
        				return(Error);
    				  }
    				  divider = 0;
				frequency =0;
    				          
				value1=ChipGetFieldImage(FTDQD3_N_MSB,Instance->TunerRegVal)<<8;
		 		value2=ChipGetFieldImage(FTDQD3_N_LSB,Instance->TunerRegVal);
		 
				 
				 divider=value1+value2;
				
				
				
				
				frequency = (divider*TunerGetStepsize(Instance))/1000 - Instance->Status.IntermediateFrequency;;
				
				*NewFrequency = Instance->Status.Frequency = frequency;
				
			break;
			#endif
			#ifdef STTUNER_DRV_TER_TUN_DTT7300X
			case STTUNER_TUNER_DTT7300X:
			       /*Add code for set step size*/
				frequency = Frequency +(Instance->Status.IntermediateFrequency);
				divider = (frequency * 100) / (TunerGetStepsize(Instance) / 10);
			
				ChipSetFieldImage(FDTT7300X_N_MSB,((divider >> 8) & 0x7F),Instance->TunerRegVal);
				ChipSetFieldImage(FDTT7300X_N_LSB,((divider ) & 0xFF),Instance->TunerRegVal);

				if (frequency <= 305000) /*Check which frequency u need to  check??? freq+IF or freq???*/
				{
					
					ChipSetFieldImage(FDTT7300X_BW_AUX,0x02,Instance->TunerRegVal);
					ChipSetFieldImage(FDTT7300X_CP,0,Instance->TunerRegVal);
					ChipSetFieldImage(FDTT7300X_T,(ChipGetFieldImage(FDTT7300X_T,Instance->TunerRegVal))&0x06,Instance->TunerRegVal);
				}
				
				else if (frequency <= 405000) 
				{
					
					ChipSetFieldImage(FDTT7300X_BW_AUX,0x02,Instance->TunerRegVal);
					ChipSetFieldImage(FDTT7300X_CP,0,Instance->TunerRegVal);
					ChipSetFieldImage(FDTT7300X_T,(ChipGetFieldImage(FDTT7300X_T,Instance->TunerRegVal))|0x01,Instance->TunerRegVal);
				}
				
				else if (frequency <= 445000) 
				{
					
					ChipSetFieldImage(FDTT7300X_BW_AUX,0x02,Instance->TunerRegVal);
					ChipSetFieldImage(FDTT7300X_CP,1,Instance->TunerRegVal);
					ChipSetFieldImage(FDTT7300X_T,(ChipGetFieldImage(FDTT7300X_T,Instance->TunerRegVal))&0x06,Instance->TunerRegVal);
				}
				
				else if (frequency <= 465000) 
				{
					
					ChipSetFieldImage(FDTT7300X_BW_AUX,0x02,Instance->TunerRegVal);
					ChipSetFieldImage(FDTT7300X_CP,1,Instance->TunerRegVal);
					ChipSetFieldImage(FDTT7300X_T,(ChipGetFieldImage(FDTT7300X_T,Instance->TunerRegVal))|0x01,Instance->TunerRegVal);
				}
				
				else if (frequency <= 735000) 
				{
					
					ChipSetFieldImage(FDTT7300X_BW_AUX,0x08,Instance->TunerRegVal);
					ChipSetFieldImage(FDTT7300X_CP,0,Instance->TunerRegVal);
					ChipSetFieldImage(FDTT7300X_T,(ChipGetFieldImage(FDTT7300X_T,Instance->TunerRegVal))|0x01,Instance->TunerRegVal);
					
					
				}
				
				else if (frequency <= 835000)
				{
					
					ChipSetFieldImage(FDTT7300X_BW_AUX,0x08,Instance->TunerRegVal);
					ChipSetFieldImage(FDTT7300X_CP,1,Instance->TunerRegVal);
					ChipSetFieldImage(FDTT7300X_T,(ChipGetFieldImage(FDTT7300X_T,Instance->TunerRegVal))&0x06,Instance->TunerRegVal);
				}
				
				else if (frequency <= 896000)
				{
					
					ChipSetFieldImage(FDTT7300X_BW_AUX,0x08,Instance->TunerRegVal);
					ChipSetFieldImage(FDTT7300X_CP,1,Instance->TunerRegVal);
					ChipSetFieldImage(FDTT7300X_T,(ChipGetFieldImage(FDTT7300X_T,Instance->TunerRegVal))|0x01,Instance->TunerRegVal);
				}
				
				
				atc=ChipGetFieldImage(FDTT7300X_ATC,Instance->TunerRegVal);
				ChipSetFieldImage(FDTT7300X_ATC,0,Instance->TunerRegVal);
				/****/
				ChipSetFieldImage(FDTT7300X_T,0x00,Instance->TunerRegVal);
				/****/
				if(Instance->ChannelBW ==STTUNER_CHAN_BW_8M)  			/* 8 MHz of Bandwidth */
				{  
					ChipSetFieldImage(FDTT7300X_BW_AUX,ChipGetFieldImage(FDTT7300X_BW_AUX,Instance->TunerRegVal)&0x0F,Instance->TunerRegVal);  
					
				}
				else if(Instance->ChannelBW == STTUNER_CHAN_BW_7M)		/* 7 MHz of Bandwidth */  
				{
					ChipSetFieldImage(FDTT7300X_BW_AUX,(ChipGetFieldImage(FDTT7300X_BW_AUX,Instance->TunerRegVal)&0x0F)+0x10,Instance->TunerRegVal); 
				}

				#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
				    Error = ChipSetRegisters(Instance->IOHandle,RDTT7300X_P_DIV1,Instance->TunerRegVal,4);
				  }
				#endif
				
				#if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			   	   {
				    Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RDTT7300X_P_DIV1,Instance->TunerRegVal,4);
                  }
              #endif
              
			        if( Error != ST_NO_ERROR)
    				  {
				  #ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        				STTBX_Print(("%s fail, error=%d\n", identity, Error));
				 #endif
        
        				return(Error);
    				  }
				WAIT_N_MS_TUNT(100); /* wait 100ms...why this much of delay?*/
		
				ChipSetFieldImage(FDTT7300X_ATC,atc,Instance->TunerRegVal);
				ChipSetFieldImage(FDTT7300X_T,0x03,Instance->TunerRegVal); 
				ChipSetFieldImage(FDTT7300X_BW_AUX,0x30,Instance->TunerRegVal); 
				
				#if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{
                     Error =ChipSetRegisters(Instance->IOHandle,RDTT7300X_P_DIV1,Instance->TunerRegVal,4);
                  }
              #endif
              
              #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			   	   {
                     Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RDTT7300X_P_DIV1,Instance->TunerRegVal,4);
                 }
              #endif
              
				if( Error != ST_NO_ERROR)
    				  {
				  #ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        				STTBX_Print(("%s fail, error=%d\n", identity, Error));
				 #endif
        
        				return(Error);
    				  }
    				  
    				 value1=(ChipGetFieldImage(FDTT7300X_N_MSB,Instance->TunerRegVal)<<8);
    				value2= (ChipGetFieldImage(FDTT7300X_N_LSB,Instance->TunerRegVal));
    		
    				divider =   value1+value2;
       
				frequency = (divider*TunerGetStepsize(Instance))/1000 - Instance->Status.IntermediateFrequency;;
				*NewFrequency = Instance->Status.Frequency = frequency;
				
			break;
			#endif
			#ifdef STTUNER_DRV_TER_TUN_RF4000
			#ifndef ST_OSLINUX
			
			case STTUNER_TUNER_RF4000:
			/* SetChannel takes the frequency in Hz, so convert the KHz to Hz.
				 Extract the stepsize and xtal from the tuner structure.
				 There is no structure element for the crystal frequency, but it
				 can be extracted from the IF.  Borrow the "frequency" variable
				 for the crystal frequency since it is not going to be needed.
				stepsize = Instance->StepSize;*/
				if (Instance->IF == 36000)
				{
					frequency = 24;		/* The crystal frquency is in MHz.*/
				}
				else
				{
					frequency = 22;		/* This is the only other allowed cyrstal frequency.*/
				}
				
				if (setChannel(&(Instance->DeviceMap),Instance->IOHandle ,(Frequency * 1000), frequency, Instance->StepSize) != ST_NO_ERROR)
				{
					  #ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        				STTBX_Print(("%s fail, error=%d\n", identity, Error));
				 #endif
        
        				return(Error);
				}
				
				*NewFrequency = Instance->Status.Frequency = (getChannelFreq(&(Instance->DeviceMap),Instance->IOHandle) / 1000);

				break;
			#endif
			#endif
			
			#ifdef STTUNER_DRV_TER_TUN_TDTGD108
			case STTUNER_TUNER_TDTGD108:
		
				frequency = Frequency +(Instance->Status.IntermediateFrequency);
				divider = (frequency * 100) / (TunerGetStepsize(Instance)  / 10);
				
				
				
				
				ChipSetFieldImage(FTDTGD108_N_MSB,((divider >> 8) & 0x7F),Instance->TunerRegVal);
				ChipSetFieldImage(FTDTGD108_N_LSB,((divider ) & 0xFF),Instance->TunerRegVal);
			
				ChipSetFieldImage(FTDTGD108_N_MSB_2,((divider >> 8) & 0x7F),Instance->TunerRegVal);
				ChipSetFieldImage(FTDTGD108_N_LSB_2,((divider ) & 0xFF),Instance->TunerRegVal);
			
				 ChipSetFieldImage(FTDTGD108_CP,0x01,Instance->TunerRegVal);  /*50 or 650uA*/
				 ChipSetFieldImage(FTDTGD108_CP_2,0x01,Instance->TunerRegVal);  /*50 or 650uA  */
				 
				if 	(Frequency <= 144000)
				{
				 	 ChipSetFieldImage(FTDTGD108_P3_0,0x01,Instance->TunerRegVal);
				   ChipSetFieldImage(FTDTGD108_T21,0x00,Instance->TunerRegVal);  /*50 ua*/
					 ChipSetFieldImage(FTDTGD108_T0,0x00,Instance->TunerRegVal);   /*50 ua*/
				}
				else if (Frequency <= 424000)
				{
					 ChipSetFieldImage(FTDTGD108_P3_0,0x02,Instance->TunerRegVal); 
					 ChipSetFieldImage(FTDTGD108_T21,0x00,Instance->TunerRegVal);
					 ChipSetFieldImage(FTDTGD108_T0,0x00,Instance->TunerRegVal); 
				}
				
				else if (Frequency <= 746000)   /* 50uA transition only*/
				{
				   ChipSetFieldImage(FTDTGD108_P3_0,0x04,Instance->TunerRegVal);
				   ChipSetFieldImage(FTDTGD108_T21,0x00,Instance->TunerRegVal);
					 ChipSetFieldImage(FTDTGD108_T0,0x00,Instance->TunerRegVal); 

				}  
				else if (Frequency <= 863250)
				{
					 ChipSetFieldImage(FTDTGD108_P3_0,0x04,Instance->TunerRegVal); 
					 ChipSetFieldImage(FTDTGD108_T21,0x03,Instance->TunerRegVal);   /*650uA*/
					 ChipSetFieldImage(FTDTGD108_T0,0x01,Instance->TunerRegVal); 

				}
					
				
				if(Instance->ChannelBW == STTUNER_CHAN_BW_8M) 
					{
									ChipSetFieldImage(FTDTGD108_P4,0x00,Instance->TunerRegVal);  /* 8 MHz of Bandwidth */
				}
				else if(Instance->ChannelBW == STTUNER_CHAN_BW_7M)
					{
					ChipSetFieldImage(FTDTGD108_P4,0x01,Instance->TunerRegVal); 
				}				

    		ChipSetFieldImage(FTDTGD108_T21_2,0x01,Instance->TunerRegVal);
        ChipSetFieldImage(FTDTGD108_T0_2,0x01,Instance->TunerRegVal);
        ChipSetFieldImage(FTDTGD108_AL  ,0x2,Instance->TunerRegVal);
 		 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{		
                    Error = ChipSetRegisters(Instance->IOHandle,RTDTGD108_P_DIV1,Instance->TunerRegVal,4);
                  }
         #endif
         #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			   	   {
                  Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RTDTGD108_P_DIV1,Instance->TunerRegVal,4);
                }
        #endif
				if( Error != ST_NO_ERROR)
    				  {
				  #ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        				STTBX_Print(("%s fail, error=%d\n", identity, Error));
				 #endif
        
        				return(Error);
    				  }    				  
    			
          ChipSetFieldImage(FTDTGD108_ATC,0x01,Instance->TunerRegVal);
           
            
          
           #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{		
                    Error = ChipSetRegisters(Instance->IOHandle,RTDTGD108_P_DIV1_2,Instance->TunerRegVal,4);
                  }
         #endif
         #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			   	   {
                  Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RTDTGD108_P_DIV1_2,Instance->TunerRegVal,4);
                }
        #endif
          
					if( Error != ST_NO_ERROR)
    				  {
				  #ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        				STTBX_Print(("%s fail, error=%d\n", identity, Error));
				  #endif
        
        				return(Error);
    				  }
           WAIT_N_MS_TUNT(100); /* wait 100ms*/

          ChipSetFieldImage(FTDTGD108_ATC,0x00,Instance->TunerRegVal);
             
         
          
          #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{		
                    Error = ChipSetRegisters(Instance->IOHandle,RTDTGD108_P_DIV1_2,Instance->TunerRegVal,4);
                  }
         #endif
         #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			   	   {
                 Error = STTUNER_IOREG_SetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,RTDTGD108_P_DIV1_2,Instance->TunerRegVal,4);
                }
        #endif
          if( Error != ST_NO_ERROR)
    				  {
				  #ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        				STTBX_Print(("%s fail, error=%d\n", identity, Error));
				  #endif
        
        				return(Error);
    				  }

    		
    				  
    		divider = 0;
				frequency =0;
    				          
				value1=ChipGetFieldImage(FTDTGD108_N_MSB,Instance->TunerRegVal)<<8;
		 		value2=ChipGetFieldImage(FTDTGD108_N_LSB,Instance->TunerRegVal);
		 
				 
				 divider=value1+value2;
				
				frequency = (divider*TunerGetStepsize(Instance))/1000 - Instance->Status.IntermediateFrequency;;
				
				*NewFrequency = Instance->Status.Frequency = frequency;
			break;
			#endif
			/**/
			#ifdef STTUNER_DRV_TER_TUN_MT2266
		
			case STTUNER_TUNER_MT2266:
				
				
				frequency = Frequency *1000;
				if (frequency < MAX_VHF_FREQ)
				{
					Instance->MT2266_Info.band = MT2266_VHF_BAND;
				}
				else if (frequency < MAX_UHF_FREQ)
				{
					Instance->MT2266_Info.band = MT2266_UHF_BAND;
				}
				/*****/
				
				
				if (frequency<177500000 || (frequency>226500000 && frequency<474000000) || frequency>858000000) 
				{
					mt_ferror= 1;
			  }
				 
				
				 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{		
                   mt_id= ChipGetOneRegister(Instance->IOHandle,RMT2266_PART_REV);
                  }
            #endif
             #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			   	   {
                  mt_id=STTUNER_IOREG_GetRegister(&(Instance->DeviceMap), Instance->IOHandle,RMT2266_PART_REV);
                }
             #endif

				 if (mt_id != 0x85 || mt_ferror) {
 				#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        				STTBX_Print(("%s fail, MT2266 tuner  I2C Acknowledhgement Error\n", identity));
				 #endif 							/* Not really, but only way to get the Panel "red */
				
					 break;
				 }	
 				
				
		 		 
				 MT2266_SetParam(Instance,MT2266_OUTPUT_BW,(Instance->ChannelBW*1000000));
				 MT2266_SetParam(Instance,MT2266_LNA_GAIN,(Instance->MT2266_Info.LNAConfig));
	 			 if (Instance->MT2266_Info.UHFSens) 
	 			 {
	 			 	MT2266_SetParam(Instance,MT2266_UHF_MAXSENS,0);
	 			 }
				 else 
				 {
				 	MT2266_SetParam(Instance,MT2266_UHF_NORMAL,0);
				 }


				MT2266_SetParam(Instance,MT2266_SRO_FREQ,(Instance->MT2266_Info.f_Ref)/**1000000*/); /*Chech about the parameter with the GUI*/
				
				MT2266_ChangeFreq(Instance,frequency ); /*Chech for error condition*/
				
			
				*NewFrequency = Frequency;/*Update with a good parameter*/
				break;
			#endif
			
			/***/
			
			default:
			break;
	}
      }


#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print(("%s ok\n", identity));
#endif

    return(Error);
}

/*****************************************************
**FUNCTION	::	TunerGetStepsize
**ACTION	::	Get tuner astep size
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE
**RETURN	::	NONE
*****************************************************/

U32 TunerGetStepsize(TUNTDRV_InstanceData_t *Instance)
{
	U32 Stepsize = 0;
	#if defined(STTUNER_DRV_TER_TUN_DTT7300X)|| defined(STTUNER_DRV_TER_TUN_DTVS223)|| defined (STTUNER_DRV_TER_TUN_TDQD3)|| defined (STTUNER_DRV_TER_TUN_TDTGD108)
		U8 u8 = 0;
	#endif

	
	switch(Instance->TunerType)
	  {
	  	
		    #ifdef STTUNER_DRV_TER_TUN_DTVS223
		case STTUNER_TUNER_DTVS223:
		  
		    u8=ChipGetFieldImage(FDTVS223_R,Instance->TunerRegVal);
		
		    switch(u8)
		     {				
			case 3:
			   Stepsize = 50000; /* 50 KHz */
			   break; 
				 	
		         default:
			   break;
		     }
		     
		break;
		#endif
		#ifdef STTUNER_DRV_TER_TUN_TDQD3
		case STTUNER_TUNER_TDQD3:
		    
		    u8=ChipGetFieldImage(FTDQD3_R,Instance->TunerRegVal);
		 
		    switch(u8)
			{
			   case 0:
			     Stepsize = 166667; /*62.5 KHz */
			     break; 
			   case 1:
			     Stepsize = 142857; /*142.86KHz */
			     break; 
			   case 2:
			     Stepsize = 80000 ; /* 80 KHz */
			     break; 
			   case 3:
			     Stepsize = 62500 ; /* 62.5 KHz */
			     break; 
			   case 4:
			     Stepsize = 31250;  /* 31.25KHz */
			     break; 
			 }
		break;
		#endif
		#ifdef STTUNER_DRV_TER_TUN_DTT7300X
		case STTUNER_TUNER_DTT7300X:
		    
		    u8=ChipGetFieldImage(FDTT7300X_RS,Instance->TunerRegVal);
		    switch(u8)
			{
				
			    case 0:
				Stepsize = 50000; /*50KHz */
				break;
			    case 1:
				Stepsize = 31250; /*31.25KHz */
			 	break; 
	                    case 2:
				Stepsize = 166667; /* 166.667KHz */
			 	break; 
			    case 3:
				Stepsize = 62500; /* 62500KHz */
			 	break; 
		        }
		     break;
		     #endif
		     #ifdef STTUNER_DRV_TER_TUN_TDTGD108
		    case STTUNER_TUNER_TDTGD108:
		/****/
			u8 = ChipGetFieldImage(FTDTGD108_RS,Instance->TunerRegVal) ; 
				switch(u8)
				{
				
					case 0:
						if (ChipGetFieldImage(FTDTGD108_T21,Instance->TunerRegVal)==0) 
							{
							Stepsize = 50000; /*50KHz */
						}
						else if (ChipGetFieldImage(FTDTGD108_T21,Instance->TunerRegVal)==3)  
							{
							Stepsize = 125000; /*125KHz */ 
						}
			 		break;
					
					case 1:
						if (ChipGetFieldImage(FTDTGD108_T21,Instance->TunerRegVal)==0)
							{ 
							Stepsize = 31250; /*31.25KHz */
						  }
						else if (ChipGetFieldImage(FTDTGD108_T21,Instance->TunerRegVal)==3)  
							{
							Stepsize = 142860; /*142.86KHz */ 
						  }
			 		break; 
					case 2:
					Stepsize = 166667; /* 166.667KHz */
			 		break; 
					case 3:
					Stepsize = 62500; /* 62500KHz */
			 		break; 
			 		default:
				 	break;
				 }
		 break;
			 #endif 
		     
		     
		default:
		    break;
		}	
	return Stepsize;
}



/*****************************************************
**FUNCTION	::	TunerInit
**ACTION	::	Initialize the tuner according to its type
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE
**RETURN	::	NONE
*****************************************************/
void TunerInit(TUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c TunerInit()";
#endif
    TUNTDRV_InstanceData_t *Instance;

    Instance = TUNTDRV_GetInstFromHandle(Handle);

    if(!Instance->Error)
	{
    LL_TunerInit(Instance);
    }
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print(("%s ok\n", identity));
#endif
}


/*****************************************************
**FUNCTION	::	TunerSetNbSteps
**ACTION	::	Set the number of steps of the tuner
**PARAMS IN	::	nbsteps ==> number of steps of the tuner
**PARAMS OUT::	NONE
**RETURN	::	NONE
*****************************************************/
void TunerSetNbSteps(TUNER_Handle_t Handle, int nbsteps)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c TunerSetNbSteps()";
#endif
    TUNTDRV_InstanceData_t *Instance;

    Instance = TUNTDRV_GetInstFromHandle(Handle);

    if(!Instance->Error)
	{
        LL_TunerSetNbSteps(Instance, nbsteps);
	}

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print(("%s ok\n", identity));
#endif
}


/*****************************************************
**FUNCTION	::	TunerGetNbSteps
**ACTION	::	Get the number of steps of the tuner
**PARAMS IN	::	NONE
**PARAMS OUT::	NONE
**RETURN	::	Number of steps, 0 if an error occur
*****************************************************/
int TunerGetNbSteps(TUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c TunerGetNbSteps()";
#endif
    int nbsteps = 0;
    TUNTDRV_InstanceData_t *Instance;

    Instance = TUNTDRV_GetInstFromHandle(Handle);

    if(!Instance->Error)
	{
        nbsteps = LL_TunerGetNbSteps(Instance);
    }
	else
	{
		dbg_counter++;
        
	}

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print(("%s ok\n", identity));
#endif

	return nbsteps;
}


/*****************************************************
**FUNCTION	::	TunerReadWrite
**ACTION	::	Read or write the tuner and put/use data in RdBuffer/WrBuffer
**PARAMS IN	::	mode ==> READ or WRITE
**PARAMS OUT::	NONE
**RETURN	::	NONE
*****************************************************/
void TunerReadWrite(TUNER_Handle_t Handle, int mode)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c TunerReadWrite()";
#endif
    int status = 0;
    TUNTDRV_InstanceData_t *Instance;

    Instance = TUNTDRV_GetInstFromHandle(Handle);

    if(!Instance->Error)
	{

		if(mode==WRITE)
		{
			 #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{ 
            status = ChipSetRegisters(Instance->IOHandle, Instance->Address, (U8 *)Instance->WrBuffer, Instance->WrSize);
              }
           #endif
           #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			   	   {
            status = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, Instance->Address, Instance->WrBuffer, Instance->WrSize, 20);
                }
           #endif     
        }
        else
		{
			  #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
				if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
					{ 
            status = ChipGetRegisters(Instance->IOHandle,Instance->Address, Instance->RdSize,(U8 *)Instance->RdBuffer);
              }
             #endif
            #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			  {
            status = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_READ, Instance->Address, Instance->RdBuffer, Instance->RdSize, 20);
            }
           #endif
        }
        
        
 
        if(status != ST_NO_ERROR)
            Instance->Error = TNR_ACK_ERR;
	}

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print(("%s ok\n", identity));
#endif
}


/* ----------------------------------------------------------------------------
Name: tuner_tuntdrv_ioctl()

Description:
    access device specific low-level functions

Parameters:

Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t tuner_tuntdrv_ioctl(TUNER_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)

{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_ioctl()";
#endif
    ST_ErrorCode_t Error=ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;
    #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372)
    U8 Buffer[30];
    U32 Size;
    U8  SubAddress;
    #endif

    Instance = TUNTDRV_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s tuner driver instance handle=%d\n", identity, Handle));
        STTBX_Print(("%s                     function=%d\n", identity, Function));
        STTBX_Print(("%s                   I/O handle=%d\n", identity, Instance->IOHandle));
#endif

    /* ---------- select what to do ---------- */
    switch(Function){

    case STTUNER_IOCTL_RAWIO: /* raw I/O to device */
    #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			   	   {
			   	   	
			   	    Error =  STTUNER_IOARCH_ReadWrite( Instance->IOHandle,
                                                 ((TERIOCTL_IOARCH_Params_t *)InParams)->Operation,
                                                 ((TERIOCTL_IOARCH_Params_t *)InParams)->SubAddr,
                                                 ((TERIOCTL_IOARCH_Params_t *)InParams)->Data,
                                                 ((TERIOCTL_IOARCH_Params_t *)InParams)->TransferSize,
                                                 ((TERIOCTL_IOARCH_Params_t *)InParams)->Timeout );
                  }
   #endif		
   #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
	                    SubAddress = (U8)(((TERIOCTL_IOARCH_Params_t *)InParams)->SubAddr & 0xFF);
					      if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
					     { 
 					       	switch(((TERIOCTL_IOARCH_Params_t *)InParams)->Operation)
 					  	       {
							  	   case STTUNER_IO_SA_READ_NS:
							            Error = STI2C_WriteNoStop(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev),&SubAddress, 1, ((TERIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
				                       Error |= STI2C_Read(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), ((TERIOCTL_IOARCH_Params_t *)InParams)->Data,((TERIOCTL_IOARCH_Params_t *)InParams)->TransferSize, ((TERIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
							            break;
							
							     case STTUNER_IO_SA_READ:
							           Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev),&SubAddress, 1, ((TERIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size); /* fix for cable (297 chip) */
							            /* fallthrough (no break;) */
							     case STTUNER_IO_READ:
							           Error |= STI2C_Read(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), ((TERIOCTL_IOARCH_Params_t *)InParams)->Data,((TERIOCTL_IOARCH_Params_t *)InParams)->TransferSize, ((TERIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
							            break;
				                case STTUNER_IO_SA_WRITE_NS:
							            Buffer[0] = SubAddress;
							            memcpy( (Buffer + 1), ((TERIOCTL_IOARCH_Params_t *)InParams)->Data, ((TERIOCTL_IOARCH_Params_t *)InParams)->TransferSize);
							            Error = STI2C_WriteNoStop(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Buffer, ((TERIOCTL_IOARCH_Params_t *)InParams)->TransferSize+1, ((TERIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
							            break;
							     case STTUNER_IO_SA_WRITE:				         
							           Buffer[0] = SubAddress;
								        memcpy( (Buffer + 1), ((TERIOCTL_IOARCH_Params_t *)InParams)->Data, ((TERIOCTL_IOARCH_Params_t *)InParams)->TransferSize);
								        Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Buffer, ((TERIOCTL_IOARCH_Params_t *)InParams)->TransferSize+1, ((TERIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
							            break;
							     case STTUNER_IO_WRITE:
							            Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), ((TERIOCTL_IOARCH_Params_t *)InParams)->Data, ((TERIOCTL_IOARCH_Params_t *)InParams)->TransferSize, ((TERIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
							            break;
							     default:
							            Error = STTUNER_ERROR_ID;
							            break;
 				   
 				               }
				        }
   #endif                      
   break;

       default:
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }



#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print(("%s function %d called\n", identity, Function));
#endif
    return(Error);
}




/* ----------------------------------------------------------------------------
Name: tuner_tuntdrv_ioaccess()

Description:
    no passthru for a tuner.
Parameters:

Return Value:
---------------------------------------------------------------------------- */

ST_ErrorCode_t tuner_tuntdrv_ioaccess(TUNER_Handle_t Handle, IOARCH_Handle_t IOHandle,STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)

{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_ioaccess()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;
    #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372)
    U8 Buffer[30];
    U32 Size;
    U8  SubAddress;
    #endif

    Instance = TUNTDRV_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

        /* if STTUNER_IOARCH_MAX_HANDLES then passthrough function required using our device handle/address */
    if (IOHandle == STTUNER_IOARCH_MAX_HANDLES)
    {
        
       #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
			   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
			   	   {
	                      
                      Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, Operation, SubAddr, Data, TransferSize, Timeout);
                  }
	   #endif		
	   #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
		   SubAddress = (U8)(SubAddr & 0xFF);
					      if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
					     { 
 					       	switch(Operation)
 					  	       {
							  	   case STTUNER_IO_SA_READ_NS:
							            Error = STI2C_WriteNoStop(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev),&SubAddress, 1, Timeout, &Size);
				                       Error |= STI2C_Read(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Data,TransferSize, Timeout, &Size);
							            break;
							
							     case STTUNER_IO_SA_READ:
							           Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev),&SubAddress, 1, Timeout, &Size); /* fix for cable (297 chip) */
							            /* fallthrough (no break;) */
							     case STTUNER_IO_READ:
							           Error |= STI2C_Read(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Data,TransferSize, Timeout, &Size);
							            break;
				                case STTUNER_IO_SA_WRITE_NS:
							            Buffer[0] = SubAddress;
							            memcpy( (Buffer + 1), Data, TransferSize);
							            Error = STI2C_WriteNoStop(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Buffer, TransferSize+1, Timeout, &Size);
							            break;
							     case STTUNER_IO_SA_WRITE:				         
							           Buffer[0] = SubAddress;
								        memcpy( (Buffer + 1), Data, TransferSize);
								        Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Buffer, TransferSize+1, Timeout, &Size);
							            break;
							     case STTUNER_IO_WRITE:
							            Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Data, TransferSize, Timeout, &Size);
							            break;
							     default:
							            Error = STTUNER_ERROR_ID;
							            break;
 				   
 				               }
				        }
	   #endif 
        
    }
    else    /* repeater */
    {
        Error = ST_ERROR_FEATURE_NOT_SUPPORTED; /* not supported for this tuner */
    }


#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print(("%s called\n", identity));
#endif
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: tuner_tuntdrv_IsTunerLocked()

Description:
    
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tuntdrv_IsTunerLocked(TUNER_Handle_t Handle, BOOL *Locked)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_IsTunerLocked()";
#endif
	
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;


    Instance = TUNTDRV_GetInstFromHandle(Handle);
    if (Instance->TunerType != STTUNER_TUNER_RF4000)
    {
		      #if defined(STTUNER_DRV_TER_STV0362) || defined(STTUNER_DRV_TER_STV0372) 
						if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
							{ 
		                  Error =  ChipGetRegisters(Instance->IOHandle,IOARCH_Handle[Instance->IOHandle].DeviceMap.RdStart,IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize,Instance->TunerRegVal);
		                 }
		      #endif
		      #if defined(STTUNER_DRV_TER_STV0360) || defined(STTUNER_DRV_TER_STV0361) || defined(STTUNER_DRV_TER_STB0370VSB)
					   if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
					   	   {
		                 Error =  STTUNER_IOREG_GetContigousRegisters(&(Instance->DeviceMap),Instance->IOHandle,Instance->DeviceMap.RdStart,Instance->DeviceMap.RdSize,Instance->TunerRegVal);
		                  }
		      #endif
}
    if (Error == ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print(("%s Error during reading tuner  ...\n", identity));
#endif
    }

    switch(Instance->TunerType)
    {   
    	
    	        #ifdef STTUNER_DRV_TER_TUN_DTVS223
    	 case STTUNER_TUNER_DTVS223:        	
    		*Locked = FALSE;    /* Assume not locked */
    		*Locked = ChipGetFieldImage(FDTVS223_FL,Instance->TunerRegVal);   		
    		break;
    		#endif

    	 #ifdef STTUNER_DRV_TER_TUN_TDQD3	
    	 case STTUNER_TUNER_TDQD3:        	
    		*Locked = FALSE;    /* Assume not locked */
    		
    		*Locked=ChipGetFieldImage(FTDQD3_FL,Instance->TunerRegVal);
		  		
    		break;
    		#endif
    	#ifdef STTUNER_DRV_TER_TUN_DTT7300X	
        case STTUNER_TUNER_DTT7300X:        	
    		*Locked = FALSE;    /* Assume not locked */
    		*Locked = ChipGetFieldImage(FDTT7300X_FL,Instance->TunerRegVal);   
    				
    		break;
    		#endif
    	#ifdef STTUNER_DRV_TER_TUN_RF4000	    
    	#ifndef ST_OSLINUX
    	case STTUNER_TUNER_RF4000:
    	       *Locked = checkLockStatus(&(Instance->DeviceMap),Instance->IOHandle);
    		break;
    	#endif
    	#endif
    	#ifdef STTUNER_DRV_TER_TUN_TDTGD108	
        case STTUNER_TUNER_TDTGD108:        	
    		*Locked = FALSE;    /* Assume not locked */
    		*Locked = ChipGetFieldImage(FTDTGD108_FL,Instance->TunerRegVal);   
    				
    		break;
    	#endif
        
        #ifdef STTUNER_DRV_TER_TUN_MT2266	
        case STTUNER_TUNER_MT2266:        	
    		*Locked = FALSE;    /* Assume not locked */
    		  
    		    *Locked = MT2266_GetLocked(Instance);
    		break;
    	#endif
        default:
               break;    			
    }
        
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print(("%s Locked: ", identity));
    if (*Locked == TRUE)
    {
        STTBX_Print(("Yes\n"));
    }
    else
    {
        STTBX_Print(("no\n"));
    }
#endif

    return(Error);
}   




/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */

TUNTDRV_InstanceData_t *TUNTDRV_GetInstFromHandle(TUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV_HANDLES
   const char *identity = "STTUNER tuntdrv.c TUNTDRV_GetInstFromHandle()";
#endif
    TUNTDRV_InstanceData_t *Instance;

    Instance = (TUNTDRV_InstanceData_t *)Handle;
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV_HANDLES
    STTBX_Print(("%s block at 0x%08x\n", identity, Instance));
#endif

    return(Instance);
}
#endif

#ifdef STTUNER_MINIDRIVER
#ifdef ST_OSLINUX
   #include "stos.h"
#else
#include <string.h>                     
#include "stcommon.h"
#include "stlite.h"     /* Standard includes */

/* STAPI */
#include "sttbx.h"
#endif
#include "stevt.h"
#include "sttuner.h"                    

/* local to sttuner */

#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "iodirect.h"      
#include "tdrv.h"       /* utilities */
#include "tuntdrv.h"       /* header for this file */
#include "ll_tun0360.h" /* low level functions */

#define TUNTDRV_IO_TIMEOUT 10

#define WRITE 1
#define READ  0



/* private variables ------------------------------------------------------- */
int dbg_counter;
#if defined(ST_OS21) || defined(ST_OSLINUX)
static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */
#else
static semaphore_t Lock_InitTermOpenClose; /* guard calls to the functions */
#endif
static TUNTDRV_InstanceData_t * TUNERInstance;

/***************************************************************************/

TUNTDRV_InstanceData_t *TUNTDRV_GetInstFromHandle(TUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV_HANDLES
   const char *identity = "STTUNER tuntdrv.c TUNTDRV_GetInstFromHandle()";
#endif
    TUNTDRV_InstanceData_t *Instance;

    Instance = (TUNTDRV_InstanceData_t *)Handle;
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV_HANDLES
    STTBX_Print(("%s block at 0x%08x\n", identity, Instance));
#endif

    return(Instance);
}
/***************************************************************************/
/* ----------------------------------------------------------------------------
Name: tuner_tuntdrv_Init()

Description: called for every perdriver.
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tuntdrv_Init(ST_DeviceName_t *DeviceName, TUNER_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

#if defined(ST_OS21) || defined(ST_OSLINUX)  
    Lock_InitTermOpenClose = semaphore_create_fifo(1);
#else
    semaphore_init_fifo(&Lock_InitTermOpenClose, 1);
#endif  
   
    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    TUNERInstance = memory_allocate_clear(InitParams->MemoryPartition, 1, sizeof( TUNTDRV_InstanceData_t ));
    TUNERInstance->MemoryPartition = InitParams->MemoryPartition;
    TUNERInstance->TunerType       = InitParams->TunerType;
    if (TUNERInstance == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
        STTBX_Print(("%s fail memory allocation InstanceNew\n", identity));
#endif    
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_NO_MEMORY);           
    }

    
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}

/* ----------------------------------------------------------------------------
Name: tuner_tuntdrv_Term()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tuntdrv_Term(ST_DeviceName_t *DeviceName, TUNER_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);
    memory_deallocate(TUNERInstance->MemoryPartition, TUNERInstance);
    SEM_UNLOCK(Lock_InitTermOpenClose);
  
    return(Error);
}

/* ----------------------------------------------------------------------------
Name: tuner_tuntdrv_Open()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tuntdrv_Open(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t  *TUNER_Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tunsdrv.c tuner_tunsdrv_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
   
    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* now got pointer to free (and valid) data block */
         
    /* Write Default tuner setting */
    /***********************************/
    /***********************************/
    SEM_UNLOCK(Lock_InitTermOpenClose);
    /*********** Initialize the tuner **************/
     LL_TunerInit(TUNERInstance);
    /**********************************************/
    *Handle = (U32)TUNERInstance;
    return(Error);
}

/* ----------------------------------------------------------------------------
Name: tuner_tuntdrv_Close()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tuntdrv_Close( TUNER_CloseParams_t *CloseParams)
{


    
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNERInstance->TopLevelHandle = STTUNER_MAX_HANDLES;
   
    return(Error);

}   


/*****************************************************
**FUNCTION  ::  tuner_tuntdrv_Select
**ACTION	::	Select the type of tuner used
**PARAMS IN	::	type	==> type of the tuner
**				address	==> I2C address of the tuner
**PARAMS OUT::	NONE
**RETURN	::	NONE
*****************************************************/
void tuner_tuntdrv_Select(STTUNER_TunerType_t type, unsigned char address)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_Select()";
#endif

 
    TUNTDRV_InstanceData_t *Instance;

    Instance = TUNERInstance;

    
        LL_TunerSelect(Instance, type, address);
    

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print(("%s ok\n", identity));
#endif
}

/*****************************************************
**FUNCTION  ::  tuner_tuntdrv_SetFrequency
**ACTION	::	Set the frequency of the tuner
**PARAMS IN	::	frequency ==> the frequency of the tuner (KHz)
**PARAMS OUT::	NONE
**RETURN	::	real tuner frequency, 0 if an error occur
*****************************************************/
ST_ErrorCode_t tuner_tuntdrv_SetFrequency (U32 Frequency, U32 *NewFrequency)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c tuner_tuntdrv_SetFrequency()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TUNTDRV_InstanceData_t *Instance;
      
    Instance = TUNERInstance;

    LL_TunerSetFrequency(Instance, Frequency, NewFrequency);
   
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print(("%s ok\n", identity));
#endif

    return(Error);
}

/*****************************************************
**FUNCTION	::	TunerReadWrite
**ACTION	::	Read or write the tuner and put/use data in RdBuffer/WrBuffer
**PARAMS IN	::	mode ==> READ or WRITE
**PARAMS OUT::	NONE
**RETURN	::	NONE
*****************************************************/
void TunerReadWrite(int mode)
{
#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
   const char *identity = "STTUNER tuntdrv.c TunerReadWrite()";
#endif
    int status = 0;
    TUNTDRV_InstanceData_t *Instance;

    Instance = TUNERInstance;

    if(!Instance->Error)
	{

		if(mode==WRITE)
		{
            status = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, Instance->Address, 0, 0, Instance->WrBuffer, Instance->WrSize, TRUE);   
        }
        else
		{
            status = STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, Instance->Address, 0, 0, Instance->RdBuffer, Instance->RdSize, TRUE) ;   
 
        }
        if(status != ST_NO_ERROR)
            Instance->Error = TNR_ACK_ERR;
	}

#ifdef STTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
    STTBX_Print(("%s ok\n", identity));
#endif
}
#endif /**** #ifdef STTUNER_MINIDRIVER ******/

/* End of tuntdrv.c */

