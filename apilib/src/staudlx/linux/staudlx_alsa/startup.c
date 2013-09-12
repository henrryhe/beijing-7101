/*****************************************************************************
File name : startup.c
Description : initialization of System for STAUDLX based 
ALSA Driver 
COPYRIGHT (C) ST-Microelectronics 2006. 
Date               Modification                 Name
----               ------------                 ----
12 Jan 2005         New written                 MJ
23 Aug 2006         Modified for ALSA			UK

*****************************************************************************/


//#define  STTBX_PRINT 1
/* Includes --------------------------------------------------------------- */
//#include "stdio.h>
//#include <stdlib.h>
//#include <string.h>
#ifdef STAUDLX_ALSA_TEST
#define DEBUG_PRINT(_x)  

#include "stdevice.h"
#include "staudlx.h"
#include "stclkrv.h"
#include "stcommon.h"
#include "stevt.h"

ST_ClockInfo_t            CLOCK_Info;

ST_Partition_t       *system_partition_stfae  = NULL;
ST_Partition_t       *ncache_partition_stfae  = NULL;


/* Global variables */
/* ---------------- */
ST_DeviceName_t EVT_DeviceName[]={"EVT0"};
ST_DeviceName_t STCKLRV_DeviceName[]={"STCLK0"};
STCLKRV_Handle_t  stclkrv_Handle;

int SystemStartUp(void);
static BOOL EVT_Init(void);
static BOOL CLKRV_Init(void);

/*******************************************************************************
Name        : EVT_Init
Description : Initialize EVT
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
static BOOL EVT_Init()
{
   
  BOOL     RetOk=TRUE;
  STEVT_InitParams_t EVT_InitParams;
  ST_ErrorCode_t Error;

  /* Setup Events parameters */
 /* ----------------------- */
 memset(&EVT_InitParams,0,sizeof(STEVT_InitParams_t));
 EVT_InitParams.ConnectMaxNum   = 90;
 EVT_InitParams.EventMaxNum     = 60;
 EVT_InitParams.SubscrMaxNum    = 90;
 EVT_InitParams.MemoryPartition = system_partition_stfae;
 EVT_InitParams.MemoryPoolSize  = 0;
 EVT_InitParams.MemorySizeFlag  = STEVT_UNKNOWN_SIZE;
// strcpy(stb7100->EvtDeviceName,EVT_DeviceName[0]);
Error = STEVT_Init(EVT_DeviceName[0], &EVT_InitParams);
    if(Error!= ST_NO_ERROR )
    {
        RetOk = FALSE;
        printk ("EVT_Init Failed !Error = %x\n", Error);
		  
    }

    return (RetOk);
}


/*******************************************************************************
Name        : CLKRV_Init
Description : Initialize the STCLKRV driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
static BOOL CLKRV_Init()
{
    
#ifndef STAUD_REMOVE_CLKRV_SUPPORT

	BOOL                    RetOk = TRUE;
    ST_ErrorCode_t          ErrorCode;
    STCLKRV_InitParams_t    CLKRV_InitParams;
    

    //strcpy(DeviceNam, STCLKRV_HANDLER_NAME);
 /* Initialize clock recovery for main decoder */
 /* ========================================== */
	memset(&CLKRV_InitParams,0,sizeof(STCLKRV_InitParams_t));
	/* Set the initialization structure */
	/* -------------------------------- */
	 
	CLKRV_InitParams.DeviceType          = STCLKRV_DEVICE_TYPE_7100;

	/*Udit Debugging */
	 CLKRV_InitParams.FSBaseAddress_p     = (U32 *)ST7100_CKG_BASE_ADDRESS;
	 CLKRV_InitParams.AUDCFGBaseAddress_p = (U32 *)ST7100_AUDIO_IF_BASE_ADDRESS;
	  CLKRV_InitParams.Partition_p         = (void *)1;
	  CLKRV_InitParams.InterruptNumber = MPEG_CLK_REC_INTERRUPT;


	CLKRV_InitParams.PCRMaxGlitch        = STCLKRV_PCR_MAX_GLITCH;
	CLKRV_InitParams.PCRDriftThres       = STCLKRV_PCR_DRIFT_THRES;
	CLKRV_InitParams.MinSampleThres      = STCLKRV_MIN_SAMPLE_THRESHOLD;
	CLKRV_InitParams.MaxWindowSize       = STCLKRV_MAX_WINDOW_SIZE;   

	strcpy( CLKRV_InitParams.EVTDeviceName, EVT_DeviceName[0]);
	/*required when driver is in Base Line mode*/
	strcpy( CLKRV_InitParams.PTIDeviceName, "STPTI4_DEV_A");
//	strcpy(stb7100->ClockDeviceName,STCKLRV_DeviceName[0]);
	 strcpy( CLKRV_InitParams.PCREvtHandlerName, "EVT0");

	DEBUG_PRINT((KERN_ALERT  "Calling STCLKRV_Init()....\n" ));
	ErrorCode = STCLKRV_Init(STCKLRV_DeviceName[0], &CLKRV_InitParams);
    if(ErrorCode != ST_NO_ERROR )
    {        
        RetOk = FALSE;
        printk (KERN_ALERT  "STCLKRV_Init() error 0x%x !\n", ErrorCode );
    }
    else
    {
        STCLKRV_OpenParams_t OpenParams;

        DEBUG_PRINT(( "%s Initialized.\n", STCLKRV_GetRevision()));

        ErrorCode = STCLKRV_Open(STCKLRV_DeviceName[0], &OpenParams, &stclkrv_Handle);
        if(ErrorCode != ST_NO_ERROR)
        {
            /* error handling */
            printk("STCLKRV_Open() error 0x%x !\n", ErrorCode);
            RetOk = FALSE;
        }
    }
    
    return RetOk;
   
#else 
	return TRUE;
#endif
} /* end CLKRV_Init() */


/*******************************************************************************
Name        : SetClockSpeed
Description : Sets CPU clock speed if CPU_CLOCK_SPEED
              is defined
Parameters  : None
Assumptions : Assumes STTBX has not been initialized yet.
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
static BOOL SetClockSpeed(void)
{
 
	ST_ErrorCode_t Error;
	BOOL RetOk = TRUE;
	
	/* Get clock configuration */
	/* ======================= */
	Error = ST_GetClockInfo(&CLOCK_Info);
	if (Error!=ST_NO_ERROR)
	{
		RetOk = FALSE;
	} 
	
	return RetOk;
}

int SystemStartUp(void)
{
    BOOL RetOk;
    
    RetOk = TRUE;

    if( RetOk )
    {
        /* Set clock speed as soon as STBOOT is initialized */
        RetOk = SetClockSpeed();
    }
	 
	 

    if( RetOk )
    {
        RetOk = EVT_Init();
    }
//	STTBX_Print(( "After EVT_Init Error = %d \n",RetOk)); 		  

#ifndef STAUD_REMOVE_CLKRV_SUPPORT 
    if( RetOk )
    {
        RetOk = CLKRV_Init();
    }
#endif
    return(0);

}
#endif 




