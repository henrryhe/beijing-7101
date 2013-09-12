/* ----------------------------------------------------------------------------
File Name: init.c

Description: 

     This module handles the sat management functions for the STAPI level
    interface for STTUNER for Init.


Copyright (C) 1999-2001 STMicroelectronics

History:
 
   date: 13-June-2001
version: 3.1.0
 author: GJP from work by LW
comment: rewrite for multi-instance.
   
date: 07-April-2004
version: 
 author: SD 
comment: Add support for SatCR.
 
Reference:

    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
  
/* C libs */
#include <string.h>                     
#endif
/* Standard includes */
#include "stlite.h"

/* STAPI */
#include "sttbx.h"

#include "sttuner.h"                    
#include "chip.h"
/* local to sttuner */      /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "satapi.h"     /* satellite manager API (functions in this file exported via this header) */

#ifdef STTUNER_MINIDRIVER
  #include "scr.h"
  #ifdef STTUNER_DRV_SAT_STV0299
	#include "d0299.h"
	#include "tunsdrv.h"
	#ifdef STTUNER_DRV_SAT_LNBH21
	  #include "lnbh21.h"
	#elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
	  #include "lnbh24.h"
	#elif defined STTUNER_DRV_SAT_LNB21
	  #include "lnb21.h"
	#else
	  #include "lnbIO.h"
	#endif
	#include "iodirect.h"
  #endif
  #if defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STV0399E)
        #include "d0399.h"
	#ifdef STTUNER_DRV_SAT_LNBH21
	  #include "lnbh21.h"
	#elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
	  #include "lnbh24.h"
	#elif defined STTUNER_DRV_SAT_LNB21
	  #include "lnb21.h"
	#else
	  #include "lnbIO.h"
	#endif
	#include "iodirect.h"
  #endif  
#endif
/* driver API*/
#ifndef STTUNER_MINIDRIVER
  #include "util.h"
  #include "stevt.h"
  #include "drivers.h"
#endif

#ifdef STTUNER_DRV_SAT_STX0288
#define STX0288_BLIND_SCAN_STEP 2000
#endif


/* invalid index into an array */
#define BAD_INDEX (0xffffffff)
#ifndef STTUNER_MINIDRIVER
U32 SAT_FindIndex_SharedTuner(STTUNER_TunerType_t TunerType);
U32 SAT_FindIndex_SharedDemod(STTUNER_DemodType_t DemodType);
/**********Global Variable declared for getting Tuner Handle**********/
U32 TunerHandleOne=0;
U32 TunerHandleTwo=0;
/***************************************************************/
/*********Global Variable declared for getting LNB Handle*********/
U32 LnbHandleOne=0;
U32 LnbHandleTwo=0;
/********Global Variable declared for getting Demod Handle************/
U32 DemodHandleOne=0;
U32 DemodHandleTwo=0;
U8 DemodHandleOneFlag=0;
U8 DemodHandleTwoFlag=0;
/*********************************************************************/
	/**********Global Variable declared for getting Scr Handle**********/
	#ifdef STTUNER_DRV_SAT_SCR
		U32 ScrHandleOne=0;
		U32 ScrHandleTwo=0;
	#endif	
	/***************************************************************/

#endif

/* ----------------------------------------------------------------------------
Name: STTUNER_SAT_Init()

Description:
    Initializes the tuner device.
    
Parameters:
    DeviceName  STTUNER global instance name device name as set during initialization.
   *InitParams  pointer to initialization parameters.
    
Return Value:
    ST_NO_ERROR,                    the operation was carried out without error.
    ST_ERROR_NO_MEMORY,             memory allocation failed.
    
See Also:
    STTUNER_Term()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SAT_Init(STTUNER_Handle_t Handle, STTUNER_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
    const char *identity = "STTUNER init.c STTUNER_SAT_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    DEMOD_InitParams_t  Demod_InitParams;
    LNB_InitParams_t    Lnb_InitParams;
    
    IOARCH_Handle_t              Demod_IOHandle;
    STTUNER_IOARCH_OpenParams_t  Demod_IOParams;
    
    #ifdef STTUNER_DRV_SAT_SCR 
    SCR_InitParams_t    Scr_InitParams;
    U8 i;
    #endif
    
    #ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
    DISEQC_InitParams_t    Diseqc_InitParams;
    IOARCH_Handle_t        Diseqc_IOHandle;
    STTUNER_IOARCH_OpenParams_t Diseqc_IOParams;
    U32 Diseqc_DriverIndex;
    #endif
    STTUNER_DriverDbase_t   *Drv;
    STTUNER_InstanceDbase_t *Inst;
    
#ifndef STTUNER_MINIDRIVER
	#ifdef STTUNER_DRV_SAT_SCR 
	
	IOARCH_Handle_t      Scr_IOHandle;
	STTUNER_IOARCH_OpenParams_t Scr_IOParams;
	U32  Scr_DriverIndex;
	#endif
  TUNER_InitParams_t  Tuner_InitParams;
  U32 Demod_DriverIndex, Tuner_DriverIndex, Lnb_DriverIndex;
  IOARCH_Handle_t         Lnb_IOHandle, Tuner_IOHandle;
  STTUNER_IOARCH_OpenParams_t Lnb_IOParams, Tuner_IOParams;

    /* ---------- check satellite specific bits of InitParams data ---------- */
    Error = STTUNER_SAT_Utils_InitParamCheck(InitParams);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s fail InitParams Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    /* ---------- access driver database and instance database ---------- */
    Drv  = STTUNER_GetDrvDb();
    Inst = STTUNER_GetDrvInst();


    /* clear out any old instance info */
    memset(&Inst[Handle].Sat, 0x00, sizeof(STTUNER_SAT_InstanceDbase_t));

#ifdef STTUNER_DISEQC2_SWDECODE_VIA_PIO
    /* Copy DiSEqC2.0 related fields into Instance Database */
    Inst[Handle].Sat.DiSEqC = InitParams->DiSEqC;     
#endif 
    /* ---------- look for a shared demod driver of the required type ---------- */
    Demod_DriverIndex = SAT_FindIndex_SharedDemod(InitParams->DemodType);
    if (Demod_DriverIndex == BAD_INDEX)
    {
        /* ---------- look for a sat demod driver of the required type ---------- */
        Demod_DriverIndex = 0;
        while(Drv->Sat.DemodDbase[Demod_DriverIndex].ID != InitParams->DemodType)
        {
            Demod_DriverIndex++;
            if (Demod_DriverIndex >= Drv->Sat.DemodDbaseSize)
            {

#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
                STTBX_Print(("%s fail demod not found\n", identity));
#endif
                return(ST_ERROR_UNKNOWN_DEVICE);
            }
        }
        Inst[Handle].Sat.Demod.Driver = &Drv->Sat.DemodDbase[Demod_DriverIndex];
#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s SAT demod %d dbase index %d\n", identity, InitParams->DemodType, Demod_DriverIndex));
#endif
    }
    else
    {
#ifdef STTUNER_USE_SHARED
        Inst[Handle].Sat.Demod.Driver = &Drv->Share.DemodDbase[Demod_DriverIndex];
#endif
#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s SHARED demod %d dbase index %d\n", identity, InitParams->DemodType, Demod_DriverIndex));
#endif
    }


    /* ---------- look for a shared tuner driver of the required type ---------- */
    Tuner_DriverIndex = SAT_FindIndex_SharedTuner(InitParams->TunerType);
    if (Tuner_DriverIndex == BAD_INDEX)
    {
        /* ---------- look for a tuner driver of the required type ---------- */
        Tuner_DriverIndex = 0;
        while(Drv->Sat.TunerDbase[Tuner_DriverIndex].ID != InitParams->TunerType)
        {
            Tuner_DriverIndex++;
            if (Tuner_DriverIndex >= Drv->Sat.TunerDbaseSize)
            {

#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
                STTBX_Print(("%s fail Tuner not found\n", identity));
#endif
                return(ST_ERROR_UNKNOWN_DEVICE);
            }
        }
        Inst[Handle].Sat.Tuner.Driver = &Drv->Sat.TunerDbase[Tuner_DriverIndex];
#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s SAT Tuner %d dbase index %d\n", identity, InitParams->TunerType, Tuner_DriverIndex));
#endif
    }
    else
    {
#ifdef STTUNER_USE_SHARED
        Inst[Handle].Sat.Tuner.Driver = &Drv->Share.TunerDbase[Tuner_DriverIndex];
#endif
#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s SHARED Tuner %d dbase index %d\n", identity, InitParams->TunerType, Tuner_DriverIndex));
#endif
    }


    /* ---------- look for a Lnb driver of the required type ---------- */
    Lnb_DriverIndex = 0;
    while(Drv->Sat.LnbDbase[Lnb_DriverIndex].ID != InitParams->LnbType)
    {
        Lnb_DriverIndex++;
        if (Lnb_DriverIndex >= Drv->Sat.LnbDbaseSize)
        {
            
#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s fail Lnb not found\n", identity));
#endif
        return(ST_ERROR_UNKNOWN_DEVICE);
        }
    }
    Inst[Handle].Sat.Lnb.Driver = &Drv->Sat.LnbDbase[Lnb_DriverIndex];
#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
    STTBX_Print(("%s Lnb %d dbase index %d\n", identity, InitParams->LnbType, Lnb_DriverIndex));
#endif

#ifdef STTUNER_DRV_SAT_SCR
	if(InitParams->Scr_App_Type != STTUNER_SCR_NULL)	
        {
            Inst[Handle].Capability.SCREnable=TRUE;
        }
	   else	
        {
		Inst[Handle].Capability.SCREnable=FALSE;
	}
        if(Inst[Handle].Capability.SCREnable) 
        {
	/* ---------- look for a Scr driver of the required type ---------- */
	    Scr_DriverIndex = 0;
	    while(Drv->Sat.ScrDbase[Scr_DriverIndex].ID != InitParams->Scr_App_Type)
	    {
	        Scr_DriverIndex++;
	        if (Scr_DriverIndex >= Drv->Sat.ScrDbaseSize)
	        {

			#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
	        STTBX_Print(("%s fail Scr not found\n", identity));
			#endif
	        return(ST_ERROR_UNKNOWN_DEVICE);
	        }
	    }
	    Inst[Handle].Sat.Scr.Driver = &Drv->Sat.ScrDbase[Scr_DriverIndex];
		#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
	    STTBX_Print(("%s Scr %d dbase index %d\n", identity, InitParams->Scr_App_Type, Scr_DriverIndex));
		#endif
	}
#endif

/* Look for a DiseqC driver of required type*/
#ifdef  STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
     Diseqc_DriverIndex = 0;
      while(Drv->Sat.DiseqcDbase[Diseqc_DriverIndex].ID != InitParams->DiseqcType)
	    {
	        Diseqc_DriverIndex++;
	        if (Diseqc_DriverIndex >= Drv->Sat.DiseqcDbaseSize)
	        {
	           #ifdef STTUNER_DEBUG_MODULE_SAT_INIT
	            STTBX_Print(("%s fail Scr not found\n", identity));
		   #endif
	            return(ST_ERROR_UNKNOWN_DEVICE);
	        }
	    }
	    Inst[Handle].Sat.Diseqc.Driver = &Drv->Sat.DiseqcDbase[Diseqc_DriverIndex];
	    #ifdef STTUNER_DEBUG_MODULE_SAT_INIT
	    STTBX_Print(("%s Diseqc %d dbase index %d\n", identity, InitParams->DiseqcType, Diseqc_DriverIndex));
	    #endif
#endif 
/********/

    /* ---------- configure demod I/O ---------- */
    Demod_IOParams.IORoute        = InitParams->DemodIO.Route;    /* direct, repeater or passthru */
    Demod_IOParams.IODriver       = InitParams->DemodIO.Driver;   /* null, I2C or debug driver    */
    Demod_IOParams.Address        = InitParams->DemodIO.Address;  /* IO address to use */
    #ifdef STTUNER_DRV_SET_HIGH_I2C_BAUDRATE
    Demod_IOParams.BaudRate       = 400000;  /* IO address to use */
    #endif
    /* location of handle to instance of redirection fn(), DrvHandle is returned when driver is opened */
    /* warning: demod opened before lnb so i/o from demod will use an uninitalized lnb handle when performing
                i/o at demod_open() if demod ioroute set to repeater or passthrough, _may_ only be valid mode for
                299 STEM (TBD) */
    Demod_IOParams.hInstance      =  &Inst[Handle].Sat.Lnb.DrvHandle;
    /* use lnb for default repeater/passthrough */
    Demod_IOParams.TargetFunction =   Inst[Handle].Sat.Lnb.Driver->lnb_ioaccess;

    strcpy(Demod_IOParams.DriverName, InitParams->DemodIO.DriverName); /* name, of device address (I2C etc.) */

    Error = ChipOpen(&Demod_IOHandle, &Demod_IOParams);
   
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s fail open demod I/O Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    if(DemodHandleOneFlag==0)
    {
       DemodHandleOne=Demod_IOHandle;
       DemodHandleOneFlag=1;
       
    }
    else if (DemodHandleTwoFlag==0)
    {
       DemodHandleTwo=Demod_IOHandle;
       DemodHandleTwoFlag=1;
       
    }
    
    /* save I/O handle (as driver is not responsible for open/close of this handle) */
    Inst[Handle].Sat.Demod.IOHandle = Demod_IOHandle;


    /* ---------- configure tuner I/O ---------- */
    Tuner_IOParams.IORoute  = InitParams->TunerIO.Route;    /* direct, repeater or passthru */
    Tuner_IOParams.IODriver = InitParams->TunerIO.Driver;   /* null, I2C or debug driver    */
    Tuner_IOParams.Address  = InitParams->TunerIO.Address;  /* IO address to use */
    /* location of handle to instance of redirection fn(), DrvHandle is returned when driver is opened */
    Tuner_IOParams.hInstance      = &Inst[Handle].Sat.Demod.DrvHandle;                      
    /* use demod for repeater/passthrough */
    Tuner_IOParams.TargetFunction = Inst[Handle].Sat.Demod.Driver->demod_ioaccess;
    Tuner_IOParams.RepeaterFn = Inst[Handle].Sat.Demod.Driver->demod_repeateraccess;
    strcpy(Tuner_IOParams.DriverName, InitParams->TunerIO.DriverName); /* name, of device address (I2C etc.) */
    #ifdef STTUNER_DRV_SET_HIGH_I2C_BAUDRATE
    Tuner_IOParams.BaudRate        = 100000;  /* IO address to use */
    #endif
    Error = ChipOpen(&Tuner_IOHandle, &Tuner_IOParams);
    
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s fail open tuner I/O Error=%d\n", identity, Error));
#endif
        ChipClose(Demod_IOHandle);
        return(Error);
    }
    /*****TO set the TunerHandles*****************/
    if (TunerHandleOne==0)
    {
       TunerHandleOne=Tuner_IOHandle;
    }
    else if(TunerHandleTwo==0)
    {
       TunerHandleTwo=Tuner_IOHandle;
    
    }
    
    /* save I/O handle (as driver is not responsible for open/close of this handle) */
    Inst[Handle].Sat.Tuner.IOHandle = Tuner_IOHandle;


    /* ---------- configure lnb I/O ---------- */
    Lnb_IOParams.IORoute  = InitParams->LnbIO.Route;    /* direct, repeater or passthru */
    Lnb_IOParams.IODriver = InitParams->LnbIO.Driver;   /* null, I2C or debug driver    */
    Lnb_IOParams.Address  = InitParams->LnbIO.Address;  /* IO address to use */
    /* location of handle to instance of redirection fn() */
    Lnb_IOParams.hInstance      = &Inst[Handle].Sat.Demod.DrvHandle;                      
    /* use demod for repeater/passthrough */
    Lnb_IOParams.TargetFunction =  Inst[Handle].Sat.Demod.Driver->demod_ioaccess;
    #ifdef STTUNER_DRV_SET_HIGH_I2C_BAUDRATE
    Lnb_IOParams.BaudRate        = 100000;  /* IO address to use */
    #endif
    strcpy(Lnb_IOParams.DriverName, InitParams->LnbIO.DriverName); /* name, of device address (I2C etc.) */

    Error = ChipOpen(&Lnb_IOHandle, &Lnb_IOParams);
    
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s fail open lnb I/O Error=%d\n", identity, Error));
#endif
        ChipClose(Tuner_IOHandle);
        ChipClose(Demod_IOHandle);
        return(Error);
    }
    if(LnbHandleOne==0)
    {
       LnbHandleOne=Lnb_IOHandle;
    }
    else if (LnbHandleTwo==0)
    {
       LnbHandleTwo=Lnb_IOHandle;
    }
    
    /* save I/O handle (as driver is not responsible for open/close of this handle) */
    Inst[Handle].Sat.Lnb.IOHandle = Lnb_IOHandle;

#ifdef STTUNER_DRV_SAT_SCR   
    if(Inst[Handle].Capability.SCREnable)
    {  
	    /* ---------- configure scr I/O ---------- */
	    Scr_IOParams.IORoute  = InitParams->ScrIO.Route;    /* direct, repeater or passthru */
	    Scr_IOParams.IODriver = InitParams->ScrIO.Driver;   /* null, I2C or debug driver    */
	    Scr_IOParams.Address  = InitParams->ScrIO.Address;  /* IO address to use */
	    /* location of handle to instance of redirection fn() */
	    Scr_IOParams.hInstance      = &Inst[Handle].Sat.Demod.DrvHandle;                      
	    /* use demod for repeater/passthrough */
	    Scr_IOParams.TargetFunction =  Inst[Handle].Sat.Demod.Driver->demod_ioaccess;
	
	    strcpy(Scr_IOParams.DriverName, InitParams->ScrIO.DriverName); /* name, of device address (I2C etc.) */
	
	    Error = ChipOpen(&Scr_IOHandle, &Scr_IOParams);
	    
	    if (Error != ST_NO_ERROR)
	    {
			#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
	        STTBX_Print(("%s fail open scr I/O Error=%d\n", identity, Error));
			#endif
	        ChipClose(Tuner_IOHandle);
	        ChipClose(Demod_IOHandle);
	        ChipClose(Lnb_IOHandle);
	        return(Error);
	    }
	    if(ScrHandleOne==0)
	    {
	       ScrHandleOne=Scr_IOHandle;
	    }
	    else if (ScrHandleTwo==0)
	    {
	       ScrHandleTwo=Scr_IOHandle;
	    }
	    
	    /* save I/O handle (as driver is not responsible for open/close of this handle) */
	    Inst[Handle].Sat.Scr.IOHandle = Scr_IOHandle;
    }
#endif

/* For DiSEqC*/
#ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
Diseqc_IOParams.IORoute = InitParams->DiseqcIO.Route;/* direct, repeater or passthru */
Diseqc_IOParams.IODriver = InitParams->DiseqcIO.Driver;/* null, I2C or debug driver    */
Diseqc_IOParams.Address = InitParams->DiseqcIO.Address;/* IO address to use */
/*Diseqc_IOParams.Init_Diseqc.BaseAddress = InitParams->Init_Diseqc.BaseAddress;
Diseqc_IOParams.Init_Diseqc.InterruptNumber = InitParams->Init_Diseqc.InterruptNumber;
Diseqc_IOParams.Init_Diseqc.InterruptNumber = InitParams->Init_Diseqc.InterruptNumber;*/
/* location of handle to instance of redirection fn() */
Diseqc_IOParams.hInstance      = NULL;/*&Inst[Handle].Sat.Diseqc.DrvHandle;*/
/* use demod for repeater/passthrough */
Diseqc_IOParams.TargetFunction = NULL ;/*Inst[Handle].Sat.Diseqc.Driver->demod_ioaccess;*/
strcpy(Diseqc_IOParams.DriverName, InitParams->DiseqcIO.DriverName); /* name, of device address (I2C etc.) */


      /* save I/O handle (as driver is not responsible for open/close of this handle) */
    Inst[Handle].Sat.Diseqc.IOHandle = Diseqc_IOHandle;
#endif 

/*****************/
    /* ---------- get misc values ---------- */
    Inst[Handle].Sat.ExternalClock = InitParams->ExternalClock;

    /* ---------- initalize demod driver ---------- */
    Demod_InitParams.IOHandle          = Demod_IOHandle;
    Demod_InitParams.MemoryPartition   = InitParams->DriverPartition;
    Demod_InitParams.ExternalClock     = InitParams->ExternalClock;
    Demod_InitParams.TSOutputMode      = InitParams->TSOutputMode;
    Demod_InitParams.SerialDataMode    = InitParams->SerialDataMode;
    Demod_InitParams.SerialClockSource = InitParams->SerialClockSource;
    Demod_InitParams.BlockSyncMode     = InitParams->BlockSyncMode;      /* add block sync bit control for bug GNBvd27452*/
    Demod_InitParams.DataFIFOMode      = InitParams->DataFIFOMode;       /* add block sync bit control for bug GNBvd27452*/
    Demod_InitParams.OutputFIFOConfig  = InitParams->OutputFIFOConfig;   /* add block sync bit control for bug GNBvd27452*/
    Demod_InitParams.FECMode           = InitParams->FECMode;
    Demod_InitParams.Path	       = InitParams->Path;
    Error = (Inst[Handle].Sat.Demod.Driver->demod_Init)(&Inst[Handle].Name, &Demod_InitParams);
    Inst[Handle].Sat.Demod.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s fail init() demod Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {

#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s demod type %d at index #%d opened ok\n", identity, InitParams->DemodType, Demod_DriverIndex));
#endif
    }

    /* ---------- initalize tuner driver ---------- */
    Tuner_InitParams.IOHandle        = Tuner_IOHandle;
    Tuner_InitParams.MemoryPartition = InitParams->DriverPartition;
    Tuner_InitParams.TunerType       = InitParams->TunerType;
     Tuner_InitParams.ExternalClock  = InitParams->ExternalClock;

    Error = (Inst[Handle].Sat.Tuner.Driver->tuner_Init)(&Inst[Handle].Name, &Tuner_InitParams);
    Inst[Handle].Sat.Tuner.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s fail init() tuner Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {

#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s tuner type %d at index #%d opened ok\n", identity, InitParams->TunerType, Tuner_DriverIndex));
#endif
    }

    /* ---------- initalize lnb driver ---------- */
    Lnb_InitParams.IOHandle        = Lnb_IOHandle;
    Lnb_InitParams.MemoryPartition = InitParams->DriverPartition;
    #ifdef STTUNER_LNB_POWER_THRU_BACKENDGPIO
    if(InitParams->LnbType == STTUNER_LNB_BACKENDGPIO)
    {
    	Lnb_InitParams.LnbIOPort       = &(InitParams->LnbIOPort);
    }
    #endif
    
    Error = (Inst[Handle].Sat.Lnb.Driver->lnb_Init)(&Inst[Handle].Name, &Lnb_InitParams);
    Inst[Handle].Sat.Lnb.Error = Error;
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s fail init() lnb Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    else
    {

#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s lnb type %d at index #%d opened ok\n", identity, InitParams->LnbType, Lnb_DriverIndex));
#endif
    }

#ifdef STTUNER_DRV_SAT_SCR   
    if (Inst[Handle].Capability.SCREnable)
    {
    /* ---------- initalize scr driver ---------- */
   
        Scr_InitParams.SCREnable       = Inst[Handle].Capability.SCREnable; /*IFA new feature*/
        Scr_InitParams.IOHandle  = Scr_IOHandle;
        Scr_InitParams.MemoryPartition = InitParams->DriverPartition;
        Scr_InitParams.SCRBPF= InitParams->SCRBPF;
        Scr_InitParams.SCR_Mode= InitParams->SCR_Mode;
        Scr_InitParams.LNBIndex        = InitParams->LNBIndex;
        Scr_InitParams.SCREnable        = Inst[Handle].Capability.SCREnable;
        Scr_InitParams.Scr_App_Type  = InitParams->Scr_App_Type;
        if(InitParams->SCR_Mode == STTUNER_SCR_MANUAL || InitParams->SCR_Mode == STTUNER_UBCHANGE)
        {
        	
	   	Scr_InitParams.Number_of_SCR = InitParams->NbScr;
	   	Scr_InitParams.Number_of_LNB = InitParams->NbLnb;
		for(i=0; i<Scr_InitParams.Number_of_SCR; ++i)
	   	 {
	   		Scr_InitParams.SCRBPFFrequencies[i] = InitParams->SCRBPFFrequencies[i];
	   	 }
	   	 for(i=0; i<Scr_InitParams.Number_of_LNB; ++i)
	   	 {
	   		Scr_InitParams.SCRLNB_LO_Frequencies[i] = InitParams->SCRLNB_LO_Frequencies[i];
	   		
	   	 }
	}
   /*----------------------------------------------*/

    Error = (Inst[Handle].Sat.Scr.Driver->scr_Init)(&Inst[Handle].Name, &Scr_InitParams);
    Inst[Handle].Sat.Scr.Error = Error;
    if (Error != ST_NO_ERROR)
    {
		#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s fail init() scr Error=%d\n", identity, Error));
		#endif
        return(Error);
    }
    else
    {

	#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
    STTBX_Print(("%s scr type %d at index #%d opened ok\n", identity, InitParams->Scr_App_Type, Scr_DriverIndex));
	#endif
    }
   }
#endif

/*Diseqc */
#ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
        Diseqc_InitParams.IOHandle  = Diseqc_IOHandle;
        Diseqc_InitParams.MemoryPartition = InitParams->DriverPartition;
        Diseqc_InitParams.Init_Diseqc.BaseAddress = InitParams->Init_Diseqc.BaseAddress;
	Diseqc_InitParams.Init_Diseqc.InterruptNumber = InitParams->Init_Diseqc.InterruptNumber;
	Diseqc_InitParams.Init_Diseqc.InterruptLevel = InitParams->Init_Diseqc.InterruptLevel;
   /*----------------------------------------------*/

    Error = (Inst[Handle].Sat.Diseqc.Driver->diseqc_Init)(&Inst[Handle].Name, &Diseqc_InitParams);
    Inst[Handle].Sat.Diseqc.Error = Error;
    if (Error != ST_NO_ERROR)
    {
	#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s fail init() diseqc Error=%d\n", identity, Error));
	#endif
        return(Error);
    }
    else
    {

	#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
    STTBX_Print(("%s diseqc type %d at index #%d opened ok\n", identity, InitParams->DiseqcType, Diseqc_DriverIndex));
	#endif
    }
#endif
/**********/
    /* --- setup sat specific ioctl --- */

    Inst[Handle].Sat.Ioctl.SignalNoise   = FALSE;   /* turn off test mode for generating random BER */
    Inst[Handle].Sat.Ioctl.TunerStepSize = 0;
    
    #ifdef STTUNER_DRV_SAT_STX0288
    if(InitParams->DemodType == STTUNER_DEMOD_STX0288)
    {Inst[Handle].Sat.Ioctl.TunerStepSize = STX0288_BLIND_SCAN_STEP;}       /* default tuner step size (calculated) */
    #endif
    
#endif

#ifdef STTUNER_MINIDRIVER
   #ifdef STTUNER_DRV_SAT_STV0299
   TUNER_InitParams_t  Tuner_InitParams;
   IOARCH_Handle_t              Tuner_IOHandle;
   STTUNER_IOARCH_OpenParams_t Tuner_IOParams;
   #endif
   #if defined(STTUNER_DRV_SAT_LNB21) || defined(STTUNER_DRV_SAT_LNBH21) || defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
   IOARCH_Handle_t         Lnb_IOHandle;
   STTUNER_IOARCH_OpenParams_t Lnb_IOParams;
   #endif
    /* ---------- access driver database and instance database ---------- */
    Drv  = STTUNER_GetDrvDb();
    Inst = STTUNER_GetDrvInst();

    /* clear out any old instance info */
    memset(&Inst[Handle].Sat, 0x00, sizeof(STTUNER_SAT_InstanceDbase_t));

	#ifdef STTUNER_DISEQC2_SWDECODE_VIA_PIO
	    /* Copy DiSEqC2.0 related fields into Instance Database */
	    Inst[Handle].Sat.DiSEqC = InitParams->DiSEqC;     
	#endif 
	    
    /* ---------- configure demod I/O ---------- */
    
    Demod_IOParams.Address        = InitParams->DemodIO.Address;  /* IO address to use */
    strcpy(Demod_IOParams.DriverName, InitParams->DemodIO.DriverName); /* name, of device address (I2C etc.) */

    Error = STTUNER_IODEMOD_Open(&Demod_IOHandle, &Demod_IOParams);
   
    if (Error != ST_NO_ERROR)
    {
	#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
	        STTBX_Print(("%s fail open demod I/O Error=%d\n", identity, Error));
	#endif
        return(Error);
    }
    
     /* ---------- configure tuner I/O ---------- */
   #ifdef STTUNER_DRV_SAT_STV0299
    Tuner_IOParams.Address        = InitParams->TunerIO.Address;  /* IO address to use */
    strcpy(Tuner_IOParams.DriverName, InitParams->TunerIO.DriverName); /* name, of device address (I2C etc.) */

    Error = STTUNER_IOTUNER_Open(&Tuner_IOHandle, &Tuner_IOParams);
   
    if (Error != ST_NO_ERROR)
    {
	#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
	        STTBX_Print(("%s fail open demod I/O Error=%d\n", identity, Error));
	#endif
        return(Error);
    }
   #endif
#if defined(STTUNER_DRV_SAT_LNB21) || defined(STTUNER_DRV_SAT_LNBH21) || defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
        /* ---------- configure lnb I/O ---------- */
    
	    Lnb_IOParams.Address  = InitParams->LnbIO.Address;  /* IO address to use */
	    strcpy(Lnb_IOParams.DriverName, InitParams->LnbIO.DriverName); /* name, of device address (I2C etc.) */
	
	    Error = STTUNER_IOLNB_Open(&Lnb_IOHandle, &Lnb_IOParams);
	    
	    if (Error != ST_NO_ERROR)
	    {
	        return(Error);
	    }
#endif


    /* ---------- get misc values ---------- */
    Inst[Handle].Sat.ExternalClock = InitParams->ExternalClock;

    Demod_InitParams.MemoryPartition   = InitParams->DriverPartition;
    Demod_InitParams.ExternalClock     = InitParams->ExternalClock;
    Demod_InitParams.TSOutputMode      = InitParams->TSOutputMode;
    Demod_InitParams.SerialDataMode    = InitParams->SerialDataMode;
    Demod_InitParams.SerialClockSource = InitParams->SerialClockSource;
    Demod_InitParams.BlockSyncMode     = InitParams->BlockSyncMode;      /* add block sync bit control for bug GNBvd27452*/
    Demod_InitParams.DataFIFOMode      = InitParams->DataFIFOMode;       /* add block sync bit control for bug GNBvd27452*/
    Demod_InitParams.OutputFIFOConfig  = InitParams->OutputFIFOConfig;   /* add block sync bit control for bug GNBvd27452*/
    Demod_InitParams.FECMode           = InitParams->FECMode;
    Demod_InitParams.Path	       = InitParams->Path;
    #ifdef STTUNER_DRV_SAT_STV0299
    Error = demod_d0299_Init(&Inst[Handle].Name, &Demod_InitParams);
    #elif defined(STTUNER_DRV_SAT_STV0399)  || defined(STTUNER_DRV_SAT_STV0399E)
    Error = demod_d0399_Init(&Inst[Handle].Name, &Demod_InitParams);
    #endif
    Inst[Handle].Sat.Demod.Error = Error;
    if (Error != ST_NO_ERROR)
    {
	#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
	        STTBX_Print(("%s fail init() demod Error=%d\n", identity, Error));
	#endif
        return(Error);
    }
    else
    {

	#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
	        STTBX_Print(("%s demod type %d at index #%d opened ok\n", identity, InitParams->DemodType, Demod_DriverIndex));
	#endif
    }
     #ifdef STTUNER_DRV_SAT_STV0299
	    /* ---------- initalize tuner driver ---------- */
	    Tuner_InitParams.MemoryPartition = InitParams->DriverPartition;
	    Tuner_InitParams.TunerType       = InitParams->TunerType;
	
	    Error = tuner_tunsdrv_Init(&Inst[Handle].Name, &Tuner_InitParams);
	    Inst[Handle].Sat.Tuner.Error = Error;
	    if (Error != ST_NO_ERROR)
	    {
		#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
		        STTBX_Print(("%s fail init() tuner Error=%d\n", identity, Error));
		#endif
	        return(Error);
	    }
	    else
	    {
	
		#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
		        STTBX_Print(("%s tuner type %d at index #%d opened ok\n", identity, InitParams->TunerType, Tuner_DriverIndex));
		#endif
	    }
    #endif
    /* ---------- initalize lnb driver ---------- */
    Lnb_InitParams.MemoryPartition = InitParams->DriverPartition;
    
 #ifdef STTUNER_DRV_SAT_STV0299
    #ifdef STTUNER_DRV_SAT_LNB21
    Error = lnb_lnb21_Init(&Inst[Handle].Name, &Lnb_InitParams);
    #elif defined STTUNER_DRV_SAT_LNBH21
    Error = lnb_lnbh21_Init(&Inst[Handle].Name, &Lnb_InitParams);
    #elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
    Error = lnb_lnbh24_Init(&Inst[Handle].Name, &Lnb_InitParams);
    #elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
    Error = lnb_lnb_demodIO_Init(&Inst[Handle].Name, &Lnb_InitParams);
    #endif
#endif
  #if defined(STTUNER_DRV_SAT_STV0399)  || defined(STTUNER_DRV_SAT_STV0399E)
    #ifdef STTUNER_DRV_SAT_LNB21
    Error = lnb_lnb21_Init(&Inst[Handle].Name, &Lnb_InitParams);
    #elif defined STTUNER_DRV_SAT_LNBH21
    Error = lnb_lnbh21_Init(&Inst[Handle].Name, &Lnb_InitParams);
    #elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
    Error = lnb_lnbh21_Init(&Inst[Handle].Name, &Lnb_InitParams);
    #elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
    Error = lnb_lnb_demodIO_Init(&Inst[Handle].Name, &Lnb_InitParams);
    #endif
  #endif
  
    Inst[Handle].Sat.Lnb.Error = Error;
    if (Error != ST_NO_ERROR)
	    {
	#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
	        STTBX_Print(("%s fail init() lnb Error=%d\n", identity, Error));
	#endif
        return(Error);
    }
    else
    {

	#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
	        STTBX_Print(("%s lnb type %d at index #%d opened ok\n", identity, InitParams->LnbType, Lnb_DriverIndex));
	#endif
    }

	#ifdef STTUNER_DRV_SAT_SCR   
	    /* ---------- initalize scr driver ---------- */
	   
	        Scr_InitParams.MemoryPartition = InitParams->DriverPartition;
	        Scr_InitParams.SCRBPF= InitParams->SCRBPF;
	        Scr_InitParams.SCR_Mode= InitParams->SCR_Mode;
		if(InitParams->SCR_Mode == STTUNER_SCR_MANUAL || InitParams->SCR_Mode == STTUNER_UBCHANGE)
	        {
	        	Scr_InitParams.Scr_App_Type  = InitParams->Scr_App_Type;
		   	Scr_InitParams.Number_of_SCR = InitParams->NbScr;
		   	Scr_InitParams.Number_of_LNB = InitParams->NbLnb;
			for(i=0; i<Scr_InitParams.Number_of_SCR; ++i)
		   	 {
		   		Scr_InitParams.SCRBPFFrequencies[i] = InitParams->SCRBPFFrequencies[i];
		   	 }
		   	 for(i=0; i<Scr_InitParams.Number_of_LNB; ++i)
		   	 {
		   		Scr_InitParams.SCRLNB_LO_Frequencies[i] = InitParams->SCRLNB_LO_Frequencies[i];
		   		
		   	 }
		}
	   /*----------------------------------------------*/
	
	    Error = scr_scrdrv_Init(&Inst[Handle].Name, &Scr_InitParams);
	    Inst[Handle].Sat.Scr.Error = Error;
	    if (Error != ST_NO_ERROR)
	    {
			#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
	        STTBX_Print(("%s fail init() scr Error=%d\n", identity, Error));
			#endif
	        return(Error);
	    }
	    else
	    {
	
		#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
	    STTBX_Print(("%s scr type %d at index #%d opened ok\n", identity, InitParams->Scr_App_Type, Scr_DriverIndex));
		#endif
	    }
	#endif
   
#endif
#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s initalization completed\n", identity));
#endif

    return(Error);

}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */

#ifndef STTUNER_MINIDRIVER
U32 SAT_FindIndex_SharedDemod(STTUNER_DemodType_t DemodType)
{
#ifdef STTUNER_USE_SHARED

#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
    const char *identity = "STTUNER init.c SAT_FindIndex_SharedDemod()";
#endif
    U32 Demod_DriverIndex;
    STTUNER_DriverDbase_t *Drv;

    Drv  = STTUNER_GetDrvDb();
    Demod_DriverIndex = 0;

    if (Drv->Share.DemodDbaseSize < 1)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s shared driver database empty\n", identity));
#endif
        return(BAD_INDEX);
    }

    while(Drv->Share.DemodDbase[Demod_DriverIndex].ID != DemodType)
    {
        Demod_DriverIndex++;
        if (Demod_DriverIndex >= Drv->Share.DemodDbaseSize)
        {

#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s info: shared demod not found\n", identity));
#endif
        return(BAD_INDEX);
        }
    }
#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
       STTBX_Print(("%s shared demod %d at dbase index %d found\n", identity, DemodType, Demod_DriverIndex));
#endif
    return(Demod_DriverIndex);

#else
    return(BAD_INDEX);
#endif

}   




U32 SAT_FindIndex_SharedTuner(STTUNER_TunerType_t TunerType)
{
#ifdef STTUNER_USE_SHARED

#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
    const char *identity = "STTUNER init.c SAT_FindIndex_SharedTuner()";
#endif
    U32 Tuner_DriverIndex;
    STTUNER_DriverDbase_t *Drv;

    Drv  = STTUNER_GetDrvDb();
    Tuner_DriverIndex = 0;

    if (Drv->Share.TunerDbaseSize < 1)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s shared tuner driver database empty\n", identity));
#endif
        return(BAD_INDEX);
    }

    while(Drv->Share.TunerDbase[Tuner_DriverIndex].ID != TunerType)
    {
        Tuner_DriverIndex++;
        if (Tuner_DriverIndex >= Drv->Share.TunerDbaseSize)
        {

#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
        STTBX_Print(("%s info: shared tuner not found\n", identity));
#endif
        return(BAD_INDEX);
        }
    }
#ifdef STTUNER_DEBUG_MODULE_SAT_INIT
       STTBX_Print(("%s shared tuner %d at dbase index %d found\n", identity, TunerType, Tuner_DriverIndex));
#endif
    return(Tuner_DriverIndex);

#else
    return(BAD_INDEX);
#endif
}   
#endif

/* End of init.c */
